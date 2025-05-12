#include <Windows.h>
#include <stdio.h>
#include "ProxyVersionDll.h"
#include "Hook.h"
#include "Cheat.h"
#include "ErrorHandler.h"
#include "CheatData.h"
#include "plugins.h"

static void InitializeDll() {
    printf("[Begeerte] Last update %s.\n", g_cheatdata ? g_cheatdata->last_update : "unknown");
    ErrorHandlerNs::RegisterExceptionFilter();
    Hook::Initialize();
    // Cheat::Start();
    BegeerteScript::Plugins::Init();
}

static void CleanupDll() {
    // Cheat::Stop();
    Hook::Cleanup();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        if (!ProxyVersionDllNs::InitializeProxy()) {
            printf("[Begeerte CRITICAL] Failed to initialize proxy.\n");
            return FALSE;
        }
        InitializeDll();
        break;
    case DLL_PROCESS_DETACH:
        CleanupDll();
        ProxyVersionDllNs::CleanupProxy();
        break;
    }
    return TRUE;
}