/**
 * src/script/script_runtime.cpp — はじむスクリプトランタイムスタブ
 *
 * 実際の実装はlibhajimuとのリンク時に有効化。
 * ここでは基本的なスケルトン。
 */
#include <engine/script/script_runtime.hpp>
#include <engine/ecs/world.hpp>
#include <engine/core/log.hpp>
#include <unordered_map>

namespace engine::script {

class DefaultScriptRuntime final : public ScriptRuntime {
public:
    Result<void> init() override {
        ENG_INFO("ScriptRuntime initialized (hajimu bridge)");
        return {};
    }
    void shutdown() override {
        scripts_.clear();
    }

    Result<u64> load_script(const std::string& path) override {
        u64 id = next_id_++;
        ScriptInstance inst;
        inst.path = path;
        inst.state = ScriptState::Loaded;
        scripts_[id] = std::move(inst);
        ENG_DEBUG("Script loaded: '%s' (id=%llu)", path.c_str(), static_cast<unsigned long long>(id));
        return id;
    }

    void unload_script(u64 instance_id) override {
        scripts_.erase(instance_id);
    }

    u32 hot_reload() override {
        u32 reloaded = 0;
        // TODO: ファイル変更検知 → 再ロード
        return reloaded;
    }

    void register_function(const NativeFunction& func) override {
        native_funcs_[func.name] = func;
        ENG_DEBUG("Native function registered: '%s'", func.name.c_str());
    }

    Result<void> call(u64 instance_id, const std::string& func_name,
                      const std::vector<std::string>&) override {
        auto it = scripts_.find(instance_id);
        if (it == scripts_.end()) return std::unexpected(Error::NotFound);
        // TODO: はじむランタイム経由で呼び出し
        ENG_DEBUG("Script call: %llu::%s", static_cast<unsigned long long>(instance_id), func_name.c_str());
        return {};
    }

    void on_update(u64, f32) override {}
    void on_start(u64) override {}
    void on_destroy(u64) override {}

    void set_sandbox(bool enabled) override {
        sandbox_ = enabled;
        ENG_INFO("Script sandbox: %s", enabled ? "ON" : "OFF");
    }

    void update(f32 dt, ecs::World&) override {
        for (auto& [id, inst] : scripts_) {
            if (inst.state == ScriptState::Running) {
                on_update(id, dt);
            }
        }
    }

private:
    struct ScriptInstance {
        std::string path;
        ScriptState state = ScriptState::Unloaded;
    };

    std::unordered_map<u64, ScriptInstance>        scripts_;
    std::unordered_map<std::string, NativeFunction> native_funcs_;
    u64  next_id_ = 1;
    bool sandbox_ = false;
};

std::unique_ptr<ScriptRuntime> create_script_runtime() {
    return std::make_unique<DefaultScriptRuntime>();
}

} // namespace engine::script
