#pragma once
#include <Windows.h>
#include <vector>

// ������ά������ָ��
namespace Offset {
    // ����ʵ���б� (EntityList) ��ָ��ɨ�����
    namespace EntityList {
        static constexpr DWORD64 MODULE_OFFSET = 0x02D433E8;                  // ʵ���б�Ļ�ַƫ��
        static const std::vector<DWORD64> FIXED_PREFIX_OFFSETS{ 0x38, 0xA0 };   // ǰ�ù̶�ƫ��
        static constexpr DWORD64 ENTITY_OFFSET_START = 0x10;                 // ʵ��ƫ�Ʒ�Χ���
        static constexpr DWORD64 ENTITY_OFFSET_END = 0x4000;                  // ʵ��ƫ�Ʒ�Χ�յ�
        static constexpr DWORD64 ENTITY_OFFSET_STEP = 0x20;                   // ʵ��ƫ�Ʋ���
        static const std::vector<DWORD64> FIXED_SUFFIX_OFFSETS{ 0x90, 0x0 };  // ���ù̶�ƫ��
    }
}