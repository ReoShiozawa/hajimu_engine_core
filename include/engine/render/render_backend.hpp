/**
 * engine/render/render_backend.hpp — レンダリングバックエンド抽象
 *
 * GPU API (Metal / Vulkan / DirectX / WebGPU) を抽象化。
 * GPU リソースの作成・描画コマンド発行。
 */
#pragma once

#include <engine/core/types.hpp>
#include <string>
#include <vector>
#include <memory>

namespace engine::render {

// ── GPU バックエンド種別 ────────────────────────────────
enum class BackendType : u8 {
    None, Metal, Vulkan, DirectX12, WebGPU
};

// ── バッファ ────────────────────────────────────────────
using GPUBufferID  = u64;
using GPUTextureID = u64;
using GPUShaderID  = u64;

struct GPUBufferDesc {
    usize size = 0;
    bool  vertex = false;
    bool  index = false;
    bool  uniform = false;
    bool  storage = false;
};

struct GPUTextureDesc {
    u32  width = 0;
    u32  height = 0;
    u32  depth = 1;
    u32  mip_levels = 1;
    u32  format = 0;
};

// ── DrawCommand ─────────────────────────────────────────
struct DrawCommand {
    GPUBufferID  vertex_buffer;
    GPUBufferID  index_buffer;
    GPUShaderID  shader;
    u32          index_count = 0;
    u32          instance_count = 1;
};

// ── RenderBackend (インターフェース) ────────────────────
class RenderBackend {
public:
    virtual ~RenderBackend() = default;

    virtual Result<void> init() = 0;
    virtual void shutdown() = 0;

    virtual GPUBufferID  create_buffer(const GPUBufferDesc& desc) = 0;
    virtual GPUTextureID create_texture(const GPUTextureDesc& desc) = 0;
    virtual GPUShaderID  create_shader(const std::string& source, const std::string& entry) = 0;

    virtual void destroy_buffer(GPUBufferID id) = 0;
    virtual void destroy_texture(GPUTextureID id) = 0;
    virtual void destroy_shader(GPUShaderID id) = 0;

    virtual void upload_buffer(GPUBufferID id, const void* data, usize size) = 0;

    virtual void begin_frame() = 0;
    virtual void submit(const DrawCommand& cmd) = 0;
    virtual void end_frame() = 0;

    [[nodiscard]] virtual BackendType type() const = 0;
    [[nodiscard]] virtual std::string name() const = 0;
};

/// プラットフォーム最適バックエンド生成
std::unique_ptr<RenderBackend> create_default_backend();

} // namespace engine::render
