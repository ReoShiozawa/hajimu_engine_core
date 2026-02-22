/**
 * src/audio/audio_system.cpp — オーディオシステムスタブ実装
 */
#include <engine/audio/audio_system.hpp>
#include <engine/ecs/world.hpp>
#include <engine/core/log.hpp>

namespace engine::audio {

class NullAudioSystem final : public AudioSystem {
public:
    Result<void> init() override {
        ENG_INFO("NullAudioSystem initialized (no audio output)");
        return {};
    }
    void shutdown() override {}

    AudioClipID load_clip(const AudioClipDesc& desc) override {
        ENG_DEBUG("Audio: loaded clip '%s'", desc.path.c_str());
        return next_id_++;
    }
    void unload_clip(AudioClipID) override {}

    void play(Entity) override {}
    void stop(Entity) override {}
    void pause(Entity) override {}

    void set_listener(const AudioListener&) override {}
    void set_channel_volume(const std::string&, f32) override {}

    void update(f32, ecs::World&) override {}

private:
    AudioClipID next_id_ = 1;
};

std::unique_ptr<AudioSystem> create_audio_system() {
    return std::make_unique<NullAudioSystem>();
}

} // namespace engine::audio
