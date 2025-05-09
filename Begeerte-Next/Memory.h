#pragma once
#include <Windows.h>

namespace Memory {
    template<typename T>
    bool Read(DWORD64 address, T& value);

    template<typename T>
    bool Write(DWORD64 address, T value);

    DWORD64 GetModuleBase(const char* moduleName);
}