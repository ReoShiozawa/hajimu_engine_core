/**
 * engine/input/action_map.hpp — Action Map (アクションベース入力)
 *
 * "ジャンプ" → [Space, GamepadA] のような抽象マッピング。
 * コンテキスト切替 (UI / ゲームプレイ / カットシーン)
 */
#pragma once

#include "input_system.hpp"
#include <engine/core/types.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace engine::input {

// ── バインディング ──────────────────────────────────────
struct Binding {
    DeviceType device;
    u16        code;     // Key, MouseButton, GamepadButton 等
};

// ── Action 定義 ─────────────────────────────────────────
struct Action {
    std::string          name;
    std::vector<Binding> bindings;
};

// ── ActionContext (切替可能なマッピングセット) ───────────
struct ActionContext {
    std::string          name;
    std::vector<Action>  actions;
};

// ── ActionMap ───────────────────────────────────────────
class ActionMap {
public:
    ActionMap() = default;

    /// コンテキスト登録
    void add_context(ActionContext ctx);

    /// アクティブコンテキスト設定
    void set_active(const std::string& context_name);

    /// Action が押されているか
    [[nodiscard]] bool action_down(const std::string& action_name, const InputSystem& input) const;

    /// Action がこのフレームで押されたか
    [[nodiscard]] bool action_pressed(const std::string& action_name, const InputSystem& input) const;

    /// デフォルトゲームコンテキストを生成
    static ActionContext default_gameplay_context();

private:
    std::vector<ActionContext>                                   contexts_;
    std::unordered_map<std::string, usize>                      context_map_;
    std::string                                                 active_;
};

} // namespace engine::input
