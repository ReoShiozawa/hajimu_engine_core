/**
 * engine/ecs/command_buffer.hpp — 遅延コマンドバッファ
 *
 * フレーム中の Entity 操作をキューに蓄積し、
 * フレーム境界で一括適用 → スレッドセーフ維持。
 */
#pragma once

#include "entity.hpp"
#include <engine/core/types.hpp>
#include <vector>
#include <functional>
#include <mutex>

namespace engine::ecs {

class World; // forward

// ── コマンド種別 ────────────────────────────────────────
enum class CommandType : u8 {
    Spawn,          // Entity 生成
    Despawn,        // Entity 破棄
    AddComponent,   // コンポーネント追加
    RemoveComponent,// コンポーネント削除
    SetComponent,   // コンポーネント上書き
};

// ── 単一コマンド ────────────────────────────────────────
struct Command {
    CommandType  type;
    Entity       entity;
    TypeID       comp_id     = 0;
    usize        comp_size   = 0;
    u8           data[256] = {};  // インライン POD データ (256B まで)
};

// ── CommandBuffer ────────────────────────────────────────
class CommandBuffer {
public:
    CommandBuffer() = default;

    /// Entity 生成予約
    Entity spawn();

    /// Entity 破棄予約
    void despawn(Entity entity);

    /// コンポーネント追加
    template <Component T>
    void add_component(Entity entity, const T& comp) {
        Command cmd;
        cmd.type = CommandType::AddComponent;
        cmd.entity = entity;
        cmd.comp_id = type_id<T>();
        cmd.comp_size = sizeof(T);
        static_assert(sizeof(T) <= sizeof(cmd.data), "Component too large for inline buffer");
        std::memcpy(cmd.data, &comp, sizeof(T));
        push(cmd);
    }

    /// コンポーネント削除
    template <Component T>
    void remove_component(Entity entity) {
        Command cmd;
        cmd.type = CommandType::RemoveComponent;
        cmd.entity = entity;
        cmd.comp_id = type_id<T>();
        push(cmd);
    }

    /// コンポーネント上書き
    template <Component T>
    void set_component(Entity entity, const T& comp) {
        Command cmd;
        cmd.type = CommandType::SetComponent;
        cmd.entity = entity;
        cmd.comp_id = type_id<T>();
        cmd.comp_size = sizeof(T);
        static_assert(sizeof(T) <= sizeof(cmd.data), "Component too large for inline buffer");
        std::memcpy(cmd.data, &comp, sizeof(T));
        push(cmd);
    }

    /// World に一括適用
    void apply(World& world);

    /// バッファクリア
    void clear();

    [[nodiscard]] usize pending() const { return commands_.size(); }

private:
    void push(const Command& cmd);

    std::vector<Command> commands_;
    std::mutex           mutex_;
    u32                  next_temp_index_ = 0xFFFF0000;
};

} // namespace engine::ecs
