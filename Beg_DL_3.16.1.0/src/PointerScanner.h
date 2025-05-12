#pragma once
#include <Windows.h>
#include <vector>
#include <utility>

namespace PointerScanner {
    std::pair<DWORD64, bool> Scan(DWORD64 baseAddress, const std::vector<DWORD64>& offsets, std::vector<std::pair<DWORD64, bool>>& debugSteps);
}