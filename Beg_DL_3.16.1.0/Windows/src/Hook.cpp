#include "Hook.h"
#include "Console.h"
#include "PointerScanner.h"
#include "Offset.h"
#include "CheatData.h"
#include <thread>
#include <stdio.h>

namespace Hook {
    static bool isRunning = false;
    static HANDLE hookThread = nullptr;

    static DWORD64 LocalPlayerAddress = 0;

    static void UpdateThread() {
        DWORD64 moduleBase = g_cheatdata->moduleBase;
        std::vector<std::pair<DWORD64, bool>> debugSteps;

        while (isRunning) {
            // 更新内存地址

        }
    }

    void Initialize() {
        Console::Initialize();
        isRunning = true;
        hookThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)UpdateThread, nullptr, 0, nullptr);
    }

    void Cleanup() {
        isRunning = false;
        if (hookThread) {
            WaitForSingleObject(hookThread, INFINITE);
            CloseHandle(hookThread);
            hookThread = nullptr;
        }
        Console::Cleanup();
    }

    DWORD64 GetLocalPlayerAddress() {
        return LocalPlayerAddress;
    }
}