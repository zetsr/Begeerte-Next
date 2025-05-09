#pragma once
#include <Windows.h>
#include <vector>

// 在这里维护所有指针
namespace Offset {
    // 定义实体列表 (EntityList) 的指针扫描参数
    namespace EntityList {
        static constexpr DWORD64 MODULE_OFFSET = 0x02D433E8;                  // 实体列表的基址偏移
        static const std::vector<DWORD64> FIXED_PREFIX_OFFSETS{ 0x38, 0xA0 };   // 前置固定偏移
        static constexpr DWORD64 ENTITY_OFFSET_START = 0x10;                 // 实体偏移范围起点
        static constexpr DWORD64 ENTITY_OFFSET_END = 0x4000;                  // 实体偏移范围终点
        static constexpr DWORD64 ENTITY_OFFSET_STEP = 0x20;                   // 实体偏移步长
        static const std::vector<DWORD64> FIXED_SUFFIX_OFFSETS{ 0x90, 0x0 };  // 后置固定偏移
    }
}