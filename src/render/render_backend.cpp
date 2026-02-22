/**
 * src/render/render_backend.cpp — レンダリングバックエンドスタブ
 *
 * プラットフォーム固有の実装は別ライブラリで提供。
 * ここでは Null バックエンド (ヘッドレス) を実装。
 */
#include <engine/render/render_backend.hpp>
#include <engine/core/log.hpp>

namespace engine::render {

// ── NullBackend: ヘッドレス実行用 ──────────────────────
class NullBackend final : public RenderBackend {
public:
    Result<void> init() override {
        ENG_INFO("NullBackend initialized (headless mode)");
        return {};
    }
    void shutdown() override {}

    GPUBufferID  create_buffer(const GPUBufferDesc&) override { return next_id_++; }
    GPUTextureID create_texture(const GPUTextureDesc&) override { return next_id_++; }
    GPUShaderID  create_shader(const std::string&, const std::string&) override { return next_id_++; }

    void destroy_buffer(GPUBufferID) override {}
    void destroy_texture(GPUTextureID) override {}
    void destroy_shader(GPUShaderID) override {}

    void upload_buffer(GPUBufferID, const void*, usize) override {}

    void begin_frame() override {}
    void submit(const DrawCommand&) override { draw_calls_++; }
    void end_frame() override {
        total_draws_ += draw_calls_;
        draw_calls_ = 0;
    }

    BackendType type() const override { return BackendType::None; }
    std::string name() const override { return "NullBackend"; }

private:
    u64 next_id_ = 1;
    u32 draw_calls_ = 0;
    u64 total_draws_ = 0;
};

std::unique_ptr<RenderBackend> create_default_backend() {
    // TODO: プラットフォーム検出で Metal / Vulkan を返す
    return std::make_unique<NullBackend>();
}

} // namespace engine::render
