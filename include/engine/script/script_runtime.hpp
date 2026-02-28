/**
 * engine/script/script_runtime.hpp — はじむスクリプトランタイム
 *
 * はじむ言語との Zero-Copy FFI ブリッジ。
 * ホットリロード + サンドボックス実行。
 */
#pragma once

#include <engine/core/types.hpp>
#include <engine/ecs/entity.hpp>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace engine::ecs { class World; }

namespace engine::script {

// ── スクリプト状態 ──────────────────────────────────────
enum class ScriptState : u8 {
    Unloaded, Loaded, Running, Paused, Error
};

// ── スクリプトコンポーネント ────────────────────────────
struct ScriptComponent {
    std::string script_path;
    ScriptState state = ScriptState::Unloaded;
    u64         instance_id = 0;
};

// ── FFI 関数登録 ────────────────────────────────────────
struct NativeFunction {
    std::string name;         // はじむ側の関数名 (日本語)
    void*       func_ptr;
    u8          min_args;
    u8          max_args;
};

// ── ScriptRuntime ───────────────────────────────────────
class ScriptRuntime {
public:
    ScriptRuntime() = default;
    virtual ~ScriptRuntime() = default;

    /// 初期化 (はじむランタイムの初期化)
    virtual Result<void> init() = 0;
    virtual void shutdown() = 0;

    /// スクリプトロード
    virtual Result<u64> load_script(const std::string& path) = 0;

    /// スクリプトアンロード
    virtual void unload_script(u64 instance_id) = 0;

    /// ホットリロード (変更検知 → 再ロード)
    virtual u32 hot_reload() = 0;

    /// ネイティブ関数登録 (C++ → はじむ)
    virtual void register_function(const NativeFunction& func) = 0;

    /// はじむ関数呼び出し (はじむ → C++)
    virtual Result<void> call(u64 instance_id, const std::string& func_name,
                              const std::vector<std::string>& args) = 0;

    /// イベントコールバック
    virtual void on_update(u64 instance_id, f32 dt) = 0;
    virtual void on_start(u64 instance_id) = 0;
    virtual void on_destroy(u64 instance_id) = 0;

    /// サンドボックスモード
    virtual void set_sandbox(bool enabled) = 0;

    /// 毎フレーム更新
    virtual void update(f32 dt, ecs::World& world) = 0;
};

std::unique_ptr<ScriptRuntime> create_script_runtime();

} // namespace engine::script
