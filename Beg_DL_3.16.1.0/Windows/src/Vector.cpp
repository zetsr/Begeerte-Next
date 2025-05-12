#include "Vector.h"

namespace Vector {
    Vec2 GetVec2(DWORD64 baseAddress) {
        if (!baseAddress) return Vec2(0.0f, 0.0f);
        float* ptr = (float*)baseAddress;
        return Vec2(ptr[0], ptr[1]);
    }

    Vec3 GetVec3(DWORD64 baseAddress) {
        if (!baseAddress) return Vec3(0.0f, 0.0f, 0.0f);
        float* ptr = (float*)baseAddress;
        return Vec3(ptr[0], ptr[1], ptr[2]);
    }

    Vec4 GetVec4(DWORD64 baseAddress) {
        if (!baseAddress) return Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        float* ptr = (float*)baseAddress;
        return Vec4(ptr[0], ptr[1], ptr[2], ptr[3]);
    }

    Vec2 CreateVec2(float x, float y) {
        return Vec2(x, y);
    }

    Vec3 CreateVec3(float x, float y, float z) {
        return Vec3(x, y, z);
    }

    Vec4 CreateVec4(float x, float y, float z, float w) {
        return Vec4(x, y, z, w);
    }
}