#include "Memory.h"
#include <stdio.h>

namespace Memory {
    template<typename T>
    bool Read(DWORD64 address, T& value) {
        // 空指针检查
        if (!address) {
            printf("[Begeerte] Memory::Read failed: Null address.\n");
            return false;
        }

        __try {
            // 检查地址是否可读（模拟 RPM 的安全性）
            MEMORY_BASIC_INFORMATION mbi;
            if (!VirtualQuery((LPCVOID)address, &mbi, sizeof(mbi))) {
                printf("[Begeerte] Memory::Read failed: Invalid address 0x%llX (VirtualQuery failed).\n", address);
                return false;
            }
            if (!(mbi.Protect & (PAGE_READONLY | PAGE_READWRITE))) {
                printf("[Begeerte] Memory::Read failed: Address 0x%llX not readable.\n", address);
                return false;
            }

            // 直接指针读取
            value = *(volatile T*)address;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("[Begeerte] Memory::Read failed: Exception at 0x%llX (Code: 0x%X).\n", address, GetExceptionCode());
            return false;
        }
    }

    template<typename T>
    bool Write(DWORD64 address, T value) {
        // 空指针检查
        if (!address) {
            printf("[Begeerte] Memory::Write failed: Null address.\n");
            return false;
        }

        __try {
            // 检查地址是否可写
            MEMORY_BASIC_INFORMATION mbi;
            if (!VirtualQuery((LPCVOID)address, &mbi, sizeof(mbi))) {
                printf("[Begeerte] Memory::Write failed: Invalid address 0x%llX (VirtualQuery failed).\n", address);
                return false;
            }
            if (!(mbi.Protect & PAGE_READWRITE)) {
                printf("[Begeerte] Memory::Write failed: Address 0x%llX not writable.\n", address);
                return false;
            }

            // 直接指针写入
            *(volatile T*)address = value;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("[Begeerte] Memory::Write failed: Exception at 0x%llX (Code: 0x%X).\n", address, GetExceptionCode());
            return false;
        }
    }

    DWORD64 GetModuleBase(const char* moduleName) {
        return (DWORD64)GetModuleHandleA(moduleName);
    }
}