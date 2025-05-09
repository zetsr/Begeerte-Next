#include "ErrorHandler.h"
#include <windows.h>
#include <DbgHelp.h>
#include <Psapi.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstdio>

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "Psapi.lib")

namespace ErrorHandlerNs {
    static void InitializeDbgHelp() {
        SymInitialize(GetCurrentProcess(), NULL, TRUE);
        printf("[Begeerte] DbgHelp 初始化完成。\n");
    }

    static std::string GetStackTrace(CONTEXT* context) {
        std::ostringstream oss;
        STACKFRAME64 stackFrame = { 0 };
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

#ifdef _M_X64
        stackFrame.AddrPC.Offset = context->Rip;
        stackFrame.AddrStack.Offset = context->Rsp;
        stackFrame.AddrFrame.Offset = context->Rbp;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
#else
        stackFrame.AddrPC.Offset = context->Eip;
        stackFrame.AddrStack.Offset = context->Esp;
        stackFrame.AddrFrame.Offset = context->Ebp;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        DWORD machineType = IMAGE_FILE_MACHINE_I386;
#endif

        oss << "调用栈：\n";
        for (int i = 0; i < 10; ++i) {
            if (!StackWalk64(machineType, process, thread, &stackFrame, context, NULL,
                SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
                printf("[Begeerte] StackWalk64 第 %d 层调用失败。\n", i);
                break;
            }

            DWORD64 address = stackFrame.AddrPC.Offset;
            if (address == 0) break;

            IMAGEHLP_MODULE64 moduleInfo = { sizeof(IMAGEHLP_MODULE64) };
            if (SymGetModuleInfo64(process, address, &moduleInfo)) {
                oss << "[" << moduleInfo.ModuleName << "] ";
            }

            char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)] = { 0 };
            PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;
            DWORD64 displacement = 0;
            if (SymFromAddr(process, address, &displacement, symbol)) {
                oss << symbol->Name << " + 0x" << std::hex << displacement;
            }
            else {
                oss << "0x" << std::hex << address;
                printf("[Begeerte] 无法获取符号信息，地址：0x%llx\n", address);
            }
            oss << "\n";
        }
        return oss.str();
    }

    static std::string GetModuleInfo() {
        std::ostringstream oss;
        HANDLE process = GetCurrentProcess();
        HMODULE modules[1024];
        DWORD cbNeeded;
        if (EnumProcessModules(process, modules, sizeof(modules), &cbNeeded)) {
            oss << "加载的模块：\n";
            for (DWORD i = 0; i < (cbNeeded / sizeof(HMODULE)); ++i) {
                char moduleName[MAX_PATH];
                if (GetModuleFileNameA(modules[i], moduleName, MAX_PATH)) {
                    oss << moduleName << "\n";
                }
                else {
                    printf("[Begeerte] 无法获取模块文件名，第 %lu 个模块，错误码：%lu\n", i, GetLastError());
                }
            }
        }
        else {
            DWORD err = GetLastError();
            oss << "无法枚举模块，错误代码：" << err << "\n";
            printf("[Begeerte] EnumProcessModules 失败，错误代码：%lu\n", err);
        }
        return oss.str();
    }

    static void CopyToClipboard(const std::string& text) {
        if (OpenClipboard(NULL)) {
            EmptyClipboard();
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
            if (hMem) {
                char* pMem = (char*)GlobalLock(hMem);
                if (pMem) {
                    memcpy(pMem, text.c_str(), text.size() + 1);
                    GlobalUnlock(hMem);
                    SetClipboardData(CF_TEXT, hMem);
                    printf("[Begeerte] 已复制信息到剪贴板。\n");
                }
                else {
                    GlobalFree(hMem);
                    printf("[Begeerte] GlobalLock 失败，无法复制到剪贴板。\n");
                }
            }
            else {
                printf("[Begeerte] GlobalAlloc 失败，无法复制到剪贴板。\n");
            }
            CloseClipboard();
        }
        else {
            printf("[Begeerte] OpenClipboard 失败，错误码：%lu\n", GetLastError());
        }
    }

    LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo) {
        std::ostringstream oss;
        oss << "应用程序发生严重错误，已终止运行。\n"
            << "异常代码：0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode << "\n"
            << "故障地址：0x" << ExceptionInfo->ExceptionRecord->ExceptionAddress << "\n"
            << "异常标志：0x" << ExceptionInfo->ExceptionRecord->ExceptionFlags << "\n"
            << "参数数量：" << ExceptionInfo->ExceptionRecord->NumberParameters << "\n";

        printf("[Begeerte] 捕获未处理异常：0x%08X\n", ExceptionInfo->ExceptionRecord->ExceptionCode);

        if (ExceptionInfo->ExceptionRecord->NumberParameters > 0) {
            for (DWORD i = 0; i < ExceptionInfo->ExceptionRecord->NumberParameters; ++i) {
                oss << "参数 " << i << "：0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionInformation[i] << "\n";
            }
        }

        oss << "\n寄存器：\n";
#ifdef _M_X64
        oss << "RAX：0x" << std::hex << ExceptionInfo->ContextRecord->Rax << "\n"
            << "RBX：0x" << ExceptionInfo->ContextRecord->Rbx << "\n"
            << "RCX：0x" << ExceptionInfo->ContextRecord->Rcx << "\n"
            << "RDX：0x" << ExceptionInfo->ContextRecord->Rdx << "\n"
            << "RSI：0x" << ExceptionInfo->ContextRecord->Rsi << "\n"
            << "RDI：0x" << ExceptionInfo->ContextRecord->Rdi << "\n"
            << "RBP：0x" << ExceptionInfo->ContextRecord->Rbp << "\n"
            << "RSP：0x" << ExceptionInfo->ContextRecord->Rsp << "\n"
            << "RIP：0x" << ExceptionInfo->ContextRecord->Rip << "\n"
            << "R8：0x" << ExceptionInfo->ContextRecord->R8 << "\n"
            << "R9：0x" << ExceptionInfo->ContextRecord->R9 << "\n"
            << "R10：0x" << ExceptionInfo->ContextRecord->R10 << "\n"
            << "R11：0x" << ExceptionInfo->ContextRecord->R11 << "\n";
#else
        oss << "EAX：0x" << std::hex << ExceptionInfo->ContextRecord->Eax << "\n"
            << "EBX：0x" << ExceptionInfo->ContextRecord->Ebx << "\n"
            << "ECX：0x" << ExceptionInfo->ContextRecord->Ecx << "\n"
            << "EDX：0x" << ExceptionInfo->ContextRecord->Edx << "\n"
            << "ESI：0x" << ExceptionInfo->ContextRecord->Esi << "\n"
            << "EDI：0x" << ExceptionInfo->ContextRecord->Edi << "\n"
            << "EBP：0x" << ExceptionInfo->ContextRecord->Ebp << "\n"
            << "ESP：0x" << ExceptionInfo->ContextRecord->Esp << "\n"
            << "EIP：0x" << ExceptionInfo->ContextRecord->Eip << "\n";
#endif

        oss << "\n" << GetStackTrace(ExceptionInfo->ContextRecord);
        // oss << "\n" << GetModuleInfo();

        std::string fullText = oss.str();
        printf("[Begeerte] 异常详情收集完毕。\n");

        std::vector<std::string> lines;
        std::istringstream iss(fullText);
        std::string line;
        while (std::getline(iss, line)) {
            lines.push_back(line);
        }

        const size_t maxLines = 100;
        std::string displayText;
        if (lines.size() <= maxLines) {
            displayText = fullText + "\n\n[完整信息已复制到剪贴板。]";
        }
        else {
            for (size_t i = 0; i < maxLines - 2; ++i) {
                displayText += lines[i] + "\n";
            }
            displayText += "\n[内容过长，已截断。完整信息已复制到剪贴板。]";
        }

        CopyToClipboard(fullText);

        MessageBoxA(NULL, displayText.c_str(), "应用程序错误", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        printf("[Begeerte] 异常信息已显示并复制。\n");

        return EXCEPTION_EXECUTE_HANDLER;
    }

    void RegisterExceptionFilter() {
        InitializeDbgHelp();
        SetUnhandledExceptionFilter(UnhandledExceptionFilter);
        printf("[Begeerte] 未处理异常过滤器已注册。\n");
    }
}