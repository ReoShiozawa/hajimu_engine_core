/**
 * engine/render/render_graph.hpp — RenderGraph
 *
 * GPU パスの依存関係グラフ。
 * リソース自動管理・並列パス実行。
 */
#pragma once

#include <engine/core/types.hpp>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace engine::render {

// ── リソース種別 ────────────────────────────────────────
enum class RenderResourceType : u8 {
    Texture2D, TextureCube, Buffer, DepthStencil, SwapChain
};

// ── リソース記述 ────────────────────────────────────────
struct RenderResourceDesc {
    std::string         name;
    RenderResourceType  type = RenderResourceType::Texture2D;
    u32                 width = 0;
    u32                 height = 0;
    u32                 format = 0;   // バックエンド依存
};

// ── パス種別 ────────────────────────────────────────────
enum class PassType : u8 {
    Graphics, Compute, Transfer, Present
};

// ── RenderPass ──────────────────────────────────────────
struct RenderPass {
    std::string              name;
    PassType                 type = PassType::Graphics;
    std::vector<std::string> inputs;     // 読み取りリソース名
    std::vector<std::string> outputs;    // 書き込みリソース名
    std::function<void()>    execute;    // 実行コールバック
};

// ── RenderGraph ─────────────────────────────────────────
class RenderGraph {
public:
    RenderGraph() = default;

    /// 一時リソースの宣言
    void declare_resource(const RenderResourceDesc& desc);

    /// パス追加
    void add_pass(RenderPass pass);

    /// グラフコンパイル (依存関係解決, 実行順決定)
    Result<void> compile();

    /// 実行
    void execute();

    /// グラフクリア (毎フレーム rebuild)
    void clear();

    [[nodiscard]] u32 pass_count() const { return static_cast<u32>(passes_.size()); }
    [[nodiscard]] const std::vector<RenderPass>& passes() const { return passes_; }

private:
    std::vector<RenderPass>                          passes_;
    std::vector<RenderResourceDesc>                  resources_;
    std::vector<u32>                                 execution_order_;
    std::unordered_map<std::string, u32>             resource_map_;
};

} // namespace engine::render
