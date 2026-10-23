/**
 * src/build/build_pipeline.cpp — ビルドパイプライン実装
 */
#include <engine/build/build_pipeline.hpp>
#include <engine/core/log.hpp>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

namespace engine::build {

void BuildPipeline::register_cooker(const std::string& asset_type, CookerFn cooker) {
    cookers_[asset_type] = std::move(cooker);
    ENG_DEBUG("BuildPipeline: registered cooker for '%s'", asset_type.c_str());
}

Result<CookResult> BuildPipeline::cook(const CookJob& job) {
    auto it = cookers_.find(job.asset_type);
    if (it == cookers_.end()) {
        return std::unexpected(Error::NotFound);
    }
    return it->second(job);
}

std::vector<CookResult> BuildPipeline::cook_all(
    const std::string& input_dir,
    const std::string& output_dir,
    BuildTarget target)
{
    std::vector<CookResult> results;

    if (!fs::exists(input_dir)) return results;

    for (auto& entry : fs::recursive_directory_iterator(input_dir)) {
        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();
        std::string asset_type;
        if (ext == ".png" || ext == ".jpg" || ext == ".bmp") asset_type = "texture";
        else if (ext == ".obj" || ext == ".fbx" || ext == ".gltf") asset_type = "mesh";
        else if (ext == ".wav" || ext == ".ogg" || ext == ".mp3") asset_type = "audio";
        else if (ext == ".glsl" || ext == ".hlsl" || ext == ".wgsl") asset_type = "shader";
        else continue;

        CookJob job;
        job.input_path = entry.path().string();
        job.output_path = output_dir + "/" +
            fs::relative(entry.path(), input_dir).string();
        job.target = target;
        job.asset_type = asset_type;

        auto result = cook(job);
        if (result) {
            results.push_back(*result);
        } else {
            CookResult fail;
            fail.input_path = job.input_path;
            fail.success = false;
            fail.error = "Failed to cook";
            results.push_back(fail);
        }
    }

    return results;
}

std::vector<CookResult> BuildPipeline::cook_incremental(
    const std::string& input_dir,
    const std::string& output_dir,
    BuildTarget target)
{
    // TODO: タイムスタンプ比較によるインクリメンタルビルド
    // 現時点ではフルビルド
    return cook_all(input_dir, output_dir, target);
}

} // namespace engine::build
