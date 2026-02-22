/**
 * engine/audio/audio_system.hpp — 空間オーディオ + ミキシング
 *
 * 3D ポジショナルオーディオ, ミキサー, エフェクト。
 */
#pragma once

#include <engine/core/types.hpp>
#include <engine/ecs/entity.hpp>
#include <string>
#include <vector>
#include <memory>

namespace engine::ecs { class World; }

namespace engine::audio {

using ecs::Entity;

// ── オーディオクリップ ──────────────────────────────────
using AudioClipID = u64;

struct AudioClipDesc {
    std::string path;
    bool        streaming = false;
    bool        loop      = false;
    f32         volume    = 1.0f;
};

// ── 空間オーディオソース ────────────────────────────────
struct AudioSource {
    AudioClipID clip;
    f32         volume = 1.0f;
    f32         pitch  = 1.0f;
    f32         min_distance = 1.0f;
    f32         max_distance = 100.0f;
    bool        spatial = true;
    bool        playing = false;
    bool        loop    = false;
};

// ── リスナー ────────────────────────────────────────────
struct AudioListener {
    Vec3 position{0, 0, 0};
    Vec3 forward{0, 0, -1};
    Vec3 up{0, 1, 0};
};

// ── ミキサーチャンネル ──────────────────────────────────
struct MixerChannel {
    std::string name;
    f32         volume = 1.0f;
    bool        muted = false;
};

// ── AudioSystem ─────────────────────────────────────────
class AudioSystem {
public:
    AudioSystem() = default;
    virtual ~AudioSystem() = default;

    virtual Result<void> init() = 0;
    virtual void shutdown() = 0;

    /// クリップ読み込み
    virtual AudioClipID load_clip(const AudioClipDesc& desc) = 0;
    virtual void unload_clip(AudioClipID clip) = 0;

    /// 再生制御
    virtual void play(Entity entity) = 0;
    virtual void stop(Entity entity) = 0;
    virtual void pause(Entity entity) = 0;

    /// リスナー設定
    virtual void set_listener(const AudioListener& listener) = 0;

    /// ミキサー
    virtual void set_channel_volume(const std::string& channel, f32 volume) = 0;

    /// 毎フレーム更新
    virtual void update(f32 dt, ecs::World& world) = 0;
};

std::unique_ptr<AudioSystem> create_audio_system();

} // namespace engine::audio
