/**
 * engine/ecs/entity.hpp — Entity ハンドル
 *
 * Entity = uint64 (下位32bit: インデックス, 上位32bit: Generation)
 * Generation でダングリングハンドルを検出
 */
#pragma once

#include <engine/core/types.hpp>

namespace engine::ecs {

struct Entity {
    u64 id = 0;

    static constexpr u32 index_bits = 32;
    static constexpr u64 index_mask = (1ULL << index_bits) - 1;

    constexpr Entity() = default;
    constexpr explicit Entity(u64 raw) : id(raw) {}
    constexpr Entity(u32 index, u32 generation)
        : id(static_cast<u64>(generation) << index_bits | index) {}

    [[nodiscard]] constexpr u32 index() const      { return static_cast<u32>(id & index_mask); }
    [[nodiscard]] constexpr u32 generation() const  { return static_cast<u32>(id >> index_bits); }
    [[nodiscard]] constexpr bool valid() const      { return id != 0; }
    constexpr bool operator==(Entity o) const       { return id == o.id; }
    constexpr bool operator!=(Entity o) const       { return id != o.id; }
    constexpr bool operator<(Entity o) const        { return id < o.id; }

    static constexpr Entity null() { return Entity{0}; }
};

} // namespace engine::ecs

// std::hash 特殊化
template <>
struct std::hash<engine::ecs::Entity> {
    size_t operator()(engine::ecs::Entity e) const noexcept { return std::hash<uint64_t>{}(e.id); }
};
