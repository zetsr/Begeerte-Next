#include "Memory.h"
#include <stdio.h>

namespace Memory {
    template<typename T>
    bool Read(DWORD64 address, T& value) {
        // ��ָ����
        if (!address) {
            printf("[Begeerte] Memory::Read failed: Null address.\n");
            return false;
        }

        __try {
            // ����ַ�Ƿ�ɶ���ģ�� RPM �İ�ȫ�ԣ�
            MEMORY_BASIC_INFORMATION mbi;
            if (!VirtualQuery((LPCVOID)address, &mbi, sizeof(mbi))) {
                printf("[Begeerte] Memory::Read failed: Invalid address 0x%llX (VirtualQuery failed).\n", address);
                return false;
            }
            if (!(mbi.Protect & (PAGE_READONLY | PAGE_READWRITE))) {
                printf("[Begeerte] Memory::Read failed: Address 0x%llX not readable.\n", address);
                return false;
            }

            // ֱ��ָ���ȡ
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
        // ��ָ����
        if (!address) {
            printf("[Begeerte] Memory::Write failed: Null address.\n");
            return false;
        }

        __try {
            // ����ַ�Ƿ��д
            MEMORY_BASIC_INFORMATION mbi;
            if (!VirtualQuery((LPCVOID)address, &mbi, sizeof(mbi))) {
                printf("[Begeerte] Memory::Write failed: Invalid address 0x%llX (VirtualQuery failed).\n", address);
                return false;
            }
            if (!(mbi.Protect & PAGE_READWRITE)) {
                printf("[Begeerte] Memory::Write failed: Address 0x%llX not writable.\n", address);
                return false;
            }

            // ֱ��ָ��д��
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