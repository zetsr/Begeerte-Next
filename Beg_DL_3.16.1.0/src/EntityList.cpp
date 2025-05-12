#include "EntityList.h"
#include "Offset.h"
#include "PointerScanner.h"
#include "Memory.h"
#include <stdio.h>
#include <set>

namespace EntityList {
    static std::vector<DWORD64> entityPointers;  // 存储所有实体的最终地址

    // 检查内存地址是否可读
    static bool IsValidAddress(DWORD64 address) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == 0) {
            return false;  // 查询失败
        }
        return (mbi.State == MEM_COMMIT && (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE)) != 0);
    }

    void Update() {
        DWORD64 moduleBase = g_cheatdata->moduleBase;
        if (moduleBase == 0) {
            printf("[Begeerte] Failed to get module base address.\n");
            return;  // 模块基址无效，直接返回
        }

        std::vector<std::pair<DWORD64, bool>> debugSteps;
        entityPointers.clear();  // 每次更新时清空列表
        std::set<DWORD64> uniqueAddresses;  // 用于跟踪已添加的地址

        DWORD64 baseAddress = moduleBase + Offset::EntityList::MODULE_OFFSET;
        const auto& prefixOffsets = Offset::EntityList::FIXED_PREFIX_OFFSETS;
        const auto& suffixOffsets = Offset::EntityList::FIXED_SUFFIX_OFFSETS;

        for (DWORD64 entityOffset = Offset::EntityList::ENTITY_OFFSET_START;
            entityOffset <= Offset::EntityList::ENTITY_OFFSET_END;
            entityOffset += Offset::EntityList::ENTITY_OFFSET_STEP) {
            std::vector<DWORD64> offsets = prefixOffsets;
            offsets.push_back(entityOffset);
            offsets.insert(offsets.end(), suffixOffsets.begin(), suffixOffsets.end());

            /*
                > 那就去做吧。
                > 只要你清楚，这不是情绪的冲动，而是意志的抉择。
                > 走吧，去做这件你决定的事。
                > 天亮之前，燃尽这片夜色。
            */

            auto [finalAddress, success] = PointerScanner::Scan(baseAddress, offsets, debugSteps);
            if (success && finalAddress != 0 && IsValidAddress(finalAddress)) {
                // 检查是否已存在该地址
                if (uniqueAddresses.find(finalAddress) == uniqueAddresses.end()) {
                    uniqueAddresses.insert(finalAddress);
                    entityPointers.push_back(finalAddress);
                }
            }
        }
    }

    size_t GetMaxPlayers() {
        return entityPointers.size();
    }

    DWORD64 GetEntity(int id) {
        if (id <= 0 || static_cast<size_t>(id) > entityPointers.size()) {
            return 0;  // 无效 ID 返回 0
        }
        return entityPointers[static_cast<size_t>(id) - 1];
    }

    Player* GetPlayer(int id) {
        DWORD64 address = GetEntity(id);
        if (address == 0 || !IsValidAddress(address)) {
            return nullptr;  // 无效地址返回 nullptr
        }
        return reinterpret_cast<Player*>(address);
    }

    const std::vector<DWORD64>& GetAllEntities() {
        return entityPointers;
    }

}