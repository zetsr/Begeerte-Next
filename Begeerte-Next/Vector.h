#pragma once
#include <Windows.h>
#include <cmath>

namespace Vector {
    // 二维向量结构体
    struct Vec2 {
        float x;
        float y;

        // 默认构造函数
        Vec2(float x_ = 0.0f, float y_ = 0.0f) : x(x_), y(y_) {}

        // 判断是否为零向量
        bool IsZero() const {
            return x == 0.0f && y == 0.0f;
        }

        // 计算向量长度
        float Length() const {
            return sqrt(x * x + y * y);
        }

        // 归一化向量
        Vec2 Normalize() const {
            float len = Length();
            if (len == 0.0f) return Vec2(0.0f, 0.0f);
            return Vec2(x / len, y / len);
        }

        // 点积
        float Dot(const Vec2& other) const {
            return x * other.x + y * other.y;
        }

        // 距离
        float Distance(const Vec2& other) const {
            return sqrt((x - other.x) * (x - other.x) + (y - other.y) * (y - other.y));
        }
    };

    // 三维向量结构体
    struct Vec3 {
        float x;
        float y;
        float z;

        // 默认构造函数
        Vec3(float x_ = 0.0f, float y_ = 0.0f, float z_ = 0.0f) : x(x_), y(y_), z(z_) {}

        // 判断是否为零向量
        bool IsZero() const {
            return x == 0.0f && y == 0.0f && z == 0.0f;
        }

        // 计算向量长度
        float Length() const {
            return sqrt(x * x + y * y + z * z);
        }

        // 归一化向量
        Vec3 Normalize() const {
            float len = Length();
            if (len == 0.0f) return Vec3(0.0f, 0.0f, 0.0f);
            return Vec3(x / len, y / len, z / len);
        }

        // 点积
        float Dot(const Vec3& other) const {
            return x * other.x + y * other.y + z * other.z;
        }

        // 距离
        float Distance(const Vec3& other) const {
            return sqrt((x - other.x) * (x - other.x) +
                (y - other.y) * (y - other.y) +
                (z - other.z) * (z - other.z));
        }
    };

    // 四维向量结构体
    struct Vec4 {
        float x;
        float y;
        float z;
        float w;

        // 默认构造函数
        Vec4(float x_ = 0.0f, float y_ = 0.0f, float z_ = 0.0f, float w_ = 0.0f)
            : x(x_), y(y_), z(z_), w(w_) {
        }

        // 判断是否为零向量
        bool IsZero() const {
            return x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f;
        }

        // 计算向量长度
        float Length() const {
            return sqrt(x * x + y * y + z * z + w * w);
        }

        // 归一化向量
        Vec4 Normalize() const {
            float len = Length();
            if (len == 0.0f) return Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            return Vec4(x / len, y / len, z / len, w / len);
        }

        // 点积
        float Dot(const Vec4& other) const {
            return x * other.x + y * other.y + z * other.z + w * other.w;
        }

        // 距离
        float Distance(const Vec4& other) const {
            return sqrt((x - other.x) * (x - other.x) +
                (y - other.y) * (y - other.y) +
                (z - other.z) * (z - other.z) +
                (w - other.w) * (w - other.w));
        }
    };

    // 从内存地址读取向量数据
    Vec2 GetVec2(DWORD64 baseAddress);
    Vec3 GetVec3(DWORD64 baseAddress);
    Vec4 GetVec4(DWORD64 baseAddress);

    // 工厂函数，用于创建向量实例
    Vec2 CreateVec2(float x, float y);
    Vec3 CreateVec3(float x, float y, float z);
    Vec4 CreateVec4(float x, float y, float z, float w);
}
