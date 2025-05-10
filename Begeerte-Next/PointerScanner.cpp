#include "PointerScanner.h"
#include "Console.h"
#include <stdio.h>

namespace PointerScanner {
    std::pair<DWORD64, bool> Scan(DWORD64 baseAddress, const std::vector<DWORD64>& offsets, std::vector<std::pair<DWORD64, bool>>& debugSteps) {
        debugSteps.clear();
        DWORD64 currentAddress = baseAddress;
        debugSteps.push_back({ currentAddress, false }); // 初始基址

        if (offsets.empty()) {
            // printf("[Begeerte] Offsets empty.\n"); // 偏移为空，已移除错误提示，现为有效情况
            return { currentAddress, true }; // 直接返回基址
        }

        __try {
            // 第一步：解引用基址
            currentAddress = *(volatile DWORD64*)currentAddress;
            debugSteps.push_back({ currentAddress, true }); // 解引用结果
            // printf("[Begeerte] Step 0: Base = 0x%llX, Dereferenced = 0x%llX\n", baseAddress, currentAddress); // 步骤0：基址和解引用后的地址

            if (!currentAddress) {
                // printf("[Begeerte] Base dereference failed.\n"); // 基址解引用失败
                return { 0, false };
            }

            // 应用后续偏移
            for (size_t i = 0; i < offsets.size(); ++i) {
                currentAddress += offsets[i];
                debugSteps.push_back({ currentAddress, false }); // 加偏移
                // printf("[Begeerte] Step %zu: Address = 0x%llX, Offset = 0x%llX\n", i + 1, currentAddress, offsets[i]); // 步骤i+1：地址和偏移量

                // 非最后一级时解引用
                if (i < offsets.size() - 1) {
                    currentAddress = *(volatile DWORD64*)currentAddress;
                    debugSteps.push_back({ currentAddress, true }); // 解引用结果
                    // printf("[Begeerte] Step %zu: Dereferenced = 0x%llX\n", i + 1, currentAddress); // 步骤i+1：解引用后的地址

                    if (!currentAddress) {
                        // printf("[Begeerte] Pointer chain broken at offset: 0x%llX (Step %zu)\n", offsets[i], i); // 指针链在偏移处中断
                        return { 0, false };
                    }
                }
            }
            // printf("[Begeerte] Scan completed: Final address = 0x%llX\n", currentAddress); // 扫描完成：最终地址
            return { currentAddress, true };
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            // printf("[Begeerte] Exception in pointer scan: 0x%X\n", GetExceptionCode()); // 指针扫描中发生异常
            return { 0, false };
        }
    }
}