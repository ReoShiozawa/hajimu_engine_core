/**
 * engine/network/net_system.hpp — ネットワーク (状態同期 + ロールバック)
 *
 * クライアント-サーバー / P2P / ロールバックネットコード。
 */
#pragma once

#include <engine/core/types.hpp>
#include <engine/ecs/entity.hpp>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace engine::ecs { class World; }

namespace engine::network {

using ecs::Entity;

// ── 接続状態 ────────────────────────────────────────────
enum class ConnectionState : u8 {
    Disconnected, Connecting, Connected, Reconnecting
};

// ── ネットワークモード ──────────────────────────────────
enum class NetMode : u8 {
    Standalone, Client, Server, ListenServer, P2P
};

// ── 同期コンポーネント登録 ──────────────────────────────
struct SyncComponentDesc {
    TypeID     comp_id;
    u8         priority    = 5;    // 同期優先度 (1-10)
    bool       reliable    = true;
    bool       compressed  = false;
    f32        update_rate = 20.0f; // Hz
};

// ── ロールバックスナップショット ────────────────────────
struct Snapshot {
    u64                 frame;
    std::vector<u8>     state_data;
};

// ── NetSystem ───────────────────────────────────────────
class NetSystem {
public:
    NetSystem() = default;
    virtual ~NetSystem() = default;

    virtual Result<void> init(NetMode mode) = 0;
    virtual void shutdown() = 0;

    /// 接続
    virtual Result<void> connect(const std::string& host, u16 port) = 0;

    /// サーバー起動
    virtual Result<void> listen(u16 port) = 0;

    /// 同期コンポーネント登録
    virtual void register_sync(const SyncComponentDesc& desc) = 0;

    /// 状態スナップショット保存
    virtual Snapshot take_snapshot(u64 frame, ecs::World& world) = 0;

    /// ロールバック
    virtual void rollback(const Snapshot& snapshot, ecs::World& world) = 0;

    /// 毎フレーム更新
    virtual void update(f32 dt, ecs::World& world) = 0;

    [[nodiscard]] virtual ConnectionState state() const = 0;
    [[nodiscard]] virtual f32 rtt() const = 0;  // Round-Trip Time (ms)
    [[nodiscard]] virtual u32 player_count() const = 0;
};

std::unique_ptr<NetSystem> create_net_system();

} // namespace engine::network
