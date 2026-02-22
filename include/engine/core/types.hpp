/**
 * engine/core/types.hpp — エンジン基本型定義
 *
 * C++20 固定幅整数 + 数学プリミティブ + ハッシュ
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <span>
#include <array>
#include <vector>
#include <optional>
#include <expected>
#include <functional>
#include <type_traits>
#include <concepts>
#include <bit>

namespace engine {

// ── 固定幅型エイリアス ──────────────────────────────────
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;
using usize = size_t;

// ── エラー型 ────────────────────────────────────────────
enum class Error : u32 {
    None = 0,
    OutOfMemory,
    InvalidArgument,
    NotFound,
    AlreadyExists,
    IOError,
    Timeout,
    NotSupported,
    CorruptedData,
    PermissionDenied,
    InternalError,
    InvalidState,
};

inline constexpr const char* error_string(Error e) {
    switch (e) {
        case Error::None:             return "成功";
        case Error::OutOfMemory:      return "メモリ不足";
        case Error::InvalidArgument:  return "不正な引数";
        case Error::NotFound:         return "見つかりません";
        case Error::AlreadyExists:    return "既に存在します";
        case Error::IOError:          return "I/Oエラー";
        case Error::Timeout:          return "タイムアウト";
        case Error::NotSupported:     return "未対応";
        case Error::CorruptedData:    return "データ破損";
        case Error::PermissionDenied: return "権限なし";
        case Error::InternalError:    return "内部エラー";
        case Error::InvalidState:     return "不正な状態";
    }
    return "不明なエラー";
}

template <typename T>
using Result = std::expected<T, Error>;

// ── 数学プリミティブ ────────────────────────────────────
struct Vec2 {
    f32 x = 0, y = 0;
    constexpr Vec2 operator+(Vec2 o) const { return {x+o.x, y+o.y}; }
    constexpr Vec2 operator-(Vec2 o) const { return {x-o.x, y-o.y}; }
    constexpr Vec2 operator*(f32 s) const  { return {x*s, y*s}; }
    constexpr f32  dot(Vec2 o) const       { return x*o.x + y*o.y; }
    constexpr f32  length_sq() const       { return dot(*this); }
};

struct Vec3 {
    f32 x = 0, y = 0, z = 0;
    constexpr Vec3 operator+(Vec3 o) const { return {x+o.x, y+o.y, z+o.z}; }
    constexpr Vec3 operator-(Vec3 o) const { return {x-o.x, y-o.y, z-o.z}; }
    constexpr Vec3 operator*(f32 s) const  { return {x*s, y*s, z*s}; }
    constexpr f32  dot(Vec3 o) const       { return x*o.x + y*o.y + z*o.z; }
    constexpr Vec3 cross(Vec3 o) const {
        return {y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x};
    }
    constexpr f32 length_sq() const { return dot(*this); }
};

struct Vec4 {
    f32 x = 0, y = 0, z = 0, w = 0;
};

struct Quat {
    f32 x = 0, y = 0, z = 0, w = 1;
    static constexpr Quat identity() { return {0, 0, 0, 1}; }
};

struct Mat4 {
    f32 m[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    static constexpr Mat4 identity() { return {}; }
};

struct AABB {
    Vec3 min{}, max{};
};

struct Color {
    f32 r = 1, g = 1, b = 1, a = 1;
    static constexpr Color white() { return {1,1,1,1}; }
    static constexpr Color black() { return {0,0,0,1}; }
    static constexpr Color red()   { return {1,0,0,1}; }
};

// ── ハッシュユーティリティ ──────────────────────────────
inline constexpr u64 hash_fnv1a(const void* data, usize len) {
    const u8* p = static_cast<const u8*>(data);
    u64 h = 14695981039346656037ULL;
    for (usize i = 0; i < len; ++i) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

inline constexpr u64 hash_string(std::string_view sv) {
    return hash_fnv1a(sv.data(), sv.size());
}

// ── コンセプト ──────────────────────────────────────────
template <typename T>
concept Trivial = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

template <typename T>
concept Component = Trivial<T> && (sizeof(T) <= 4096);

// ── TypeID (コンパイル時型ID) ────────────────────────────
using TypeID = u64;

namespace detail {
    template <typename T>
    struct TypeIDHolder {
        static inline const char tag = 0;
    };
}

template <typename T>
constexpr TypeID type_id() {
    return reinterpret_cast<TypeID>(&detail::TypeIDHolder<std::remove_cvref_t<T>>::tag);
}

} // namespace engine
