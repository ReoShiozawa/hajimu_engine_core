/**
 * src/network/net_system.cpp — ネットワークシステムスタブ実装
 */
#include <engine/network/net_system.hpp>
#include <engine/ecs/world.hpp>
#include <engine/core/log.hpp>

namespace engine::network {

class StandaloneNetSystem final : public NetSystem {
public:
    Result<void> init(NetMode mode) override {
        mode_ = mode;
        ENG_INFO("NetSystem initialized (standalone mode)");
        return {};
    }
    void shutdown() override {}

    Result<void> connect(const std::string& host, u16 port) override {
        ENG_WARN("NetSystem: connect() called in standalone mode (%s:%u)", host.c_str(), port);
        return std::unexpected(Error::NotSupported);
    }

    Result<void> listen(u16 port) override {
        ENG_WARN("NetSystem: listen() called in standalone mode (port %u)", port);
        return std::unexpected(Error::NotSupported);
    }

    void register_sync(const SyncComponentDesc&) override {}

    Snapshot take_snapshot(u64 frame, ecs::World&) override {
        return Snapshot{frame, {}};
    }

    void rollback(const Snapshot&, ecs::World&) override {}

    void update(f32, ecs::World&) override {}

    ConnectionState state() const override { return ConnectionState::Disconnected; }
    f32 rtt() const override { return 0.0f; }
    u32 player_count() const override { return 1; }

private:
    NetMode mode_ = NetMode::Standalone;
};

std::unique_ptr<NetSystem> create_net_system() {
    return std::make_unique<StandaloneNetSystem>();
}

} // namespace engine::network
