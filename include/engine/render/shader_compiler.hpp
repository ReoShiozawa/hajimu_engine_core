/**
 * engine/render/shader_compiler.hpp — シェーダクロスコンパイラ
 *
 * 統一シェーダ言語 → SPIRV / MSL / HLSL / WGSL 変換。
 */
#pragma once

#include <engine/core/types.hpp>
#include <string>
#include <vector>

namespace engine::render {

// ── ターゲット言語 ──────────────────────────────────────
enum class ShaderTarget : u8 {
    SPIRV, MSL, HLSL, WGSL, GLSL
};

// ── シェーダステージ ────────────────────────────────────
enum class ShaderStage : u8 {
    Vertex, Fragment, Compute, Geometry, TessControl, TessEval
};

// ── コンパイル結果 ──────────────────────────────────────
struct ShaderOutput {
    ShaderTarget         target;
    std::vector<u8>      bytecode;    // SPIRV の場合バイナリ
    std::string          source;      // テキスト出力の場合
    std::vector<std::string> warnings;
};

// ── ShaderCompiler ──────────────────────────────────────
class ShaderCompiler {
public:
    ShaderCompiler() = default;
    ~ShaderCompiler() = default;

    /// 統一シェーダソースからコンパイル
    [[nodiscard]] Result<ShaderOutput> compile(
        const std::string& source,
        ShaderStage stage,
        ShaderTarget target) const;

    /// ファイルからコンパイル
    [[nodiscard]] Result<ShaderOutput> compile_file(
        const std::string& path,
        ShaderStage stage,
        ShaderTarget target) const;

    /// プラットフォーム推奨ターゲット
    [[nodiscard]] static ShaderTarget platform_target();
};

} // namespace engine::render
