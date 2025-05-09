#pragma once
#include <Windows.h>

namespace Hook {
    void Initialize();
    void Cleanup();
    DWORD64 GetLocalPlayerAddress();
}