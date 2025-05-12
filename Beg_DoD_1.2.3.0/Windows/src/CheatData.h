#ifndef CHEAT_DATA_H
#define CHEAT_DATA_H

#include <windows.h>
#include "Memory.h"

struct CheatData {
    byte EntityValidFlag;
    DWORD64 moduleBase;
    const char* last_update;
};

inline static CheatData cheatDataInstance = {
    .EntityValidFlag = 208,
    .moduleBase = Memory::GetModuleBase("DragonsServer-Win64-Shipping.exe"),
    .last_update = "2025-5-12"
};

// 全局指针，指向静态实例
inline static CheatData* g_cheatdata = &cheatDataInstance;

#endif // CHEAT_DATA_H