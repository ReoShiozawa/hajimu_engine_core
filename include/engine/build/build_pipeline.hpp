/**
 * engine/build/build_pipeline.hpp — アセットクッキングパイプライン
 *
 * ソースアセット → ランタイム形式への変換。
 * マルチターゲット対応 (PC / モバイル / Web)。
 */
#pragma once

#include <engine/core/types.hpp>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace engine::build {

// ── ビルドターゲット ────────────────────────────────────
enum class BuildTarget : u8 {
    PC_Windows, PC_macOS, PC_Linux,
    Mobile_iOS, Mobile_Android,
    Web_Wasm,
    Console_Switch, Console_PS5, Console_Xbox
};

// ── アセット処理ステージ ────────────────────────────────
enum class CookStage : u8 {
    Import,       // ソースファイル読み込み
    Process,      // 変換・最適化
    Compress,     // 圧縮
    Package,      // パッケージング
};

// ── クッキングジョブ ────────────────────────────────────
struct CookJob {
    std::string   input_path;
    std::string   output_path;
    BuildTarget   target;
    std::string   asset_type;   // "texture", "mesh", "audio" 等
};

// ── クッキング結果 ──────────────────────────────────────
struct CookResult {
    std::string input_path;
    std::string output_path;
    bool        success;
    std::string error;
    usize       input_size;
    usize       output_size;
    f64         time_ms;
};

// ── AssetCooker (型ごとの処理器) ────────────────────────
using CookerFn = std::function<Result<CookResult>(const CookJob& job)>;

// ── BuildPipeline ───────────────────────────────────────
class BuildPipeline {
public:
    BuildPipeline() = default;
    ~BuildPipeline() = default;

    /// クッカー登録 (アセットタイプごと)
    void register_cooker(const std::string& asset_type, CookerFn cooker);

    /// 単一アセットの cook
    Result<CookResult> cook(const CookJob& job);

    /// ディレクトリ全体の cook
    std::vector<CookResult> cook_all(const std::string& input_dir,
                                      const std::string& output_dir,
                                      BuildTarget target);

    /// インクリメンタルビルド (変更分のみ)
    std::vector<CookResult> cook_incremental(const std::string& input_dir,
                                              const std::string& output_dir,
                                              BuildTarget target);

    [[nodiscard]] u32 cooker_count() const { return static_cast<u32>(cookers_.size()); }

private:
    std::unordered_map<std::string, CookerFn> cookers_;
};

} // namespace engine::build
