/**
 * src/input/input_system.cpp — 入力システム + アクションマップ実装
 */
#include <engine/input/input_system.hpp>
#include <engine/input/action_map.hpp>
#include <engine/core/log.hpp>

namespace engine::input {

// ── InputSystem ─────────────────────────────────────────

InputSystem::InputSystem() {
    ENG_INFO("InputSystem initialized");
}

InputSystem::~InputSystem() = default;

void InputSystem::begin_frame() {
    previous_ = current_;
    mouse_delta_ = {0, 0};
}

void InputSystem::push_event(const InputEvent& event) {
    current_[event.code] = event.pressed;
    axes_[event.code] = event.value;

    if (event.device == DeviceType::Mouse) {
        // マウス移動はdeltaとして蓄積
        // code 0xF000 = mouse X, 0xF001 = mouse Y
        if (event.code == 0xF000) {
            mouse_delta_.x += event.value;
            mouse_pos_.x = event.value;
        } else if (event.code == 0xF001) {
            mouse_delta_.y += event.value;
            mouse_pos_.y = event.value;
        }
    }
}

bool InputSystem::key_down(Key key) const {
    auto it = current_.find(static_cast<u16>(key));
    return it != current_.end() && it->second;
}

bool InputSystem::key_pressed(Key key) const {
    auto code = static_cast<u16>(key);
    auto cur = current_.find(code);
    auto prev = previous_.find(code);
    bool now = cur != current_.end() && cur->second;
    bool was = prev != previous_.end() && prev->second;
    return now && !was;
}

bool InputSystem::key_released(Key key) const {
    auto code = static_cast<u16>(key);
    auto cur = current_.find(code);
    auto prev = previous_.find(code);
    bool now = cur != current_.end() && cur->second;
    bool was = prev != previous_.end() && prev->second;
    return !now && was;
}

bool InputSystem::mouse_button_down(MouseButton btn) const {
    auto it = current_.find(static_cast<u16>(btn) + 0xFF00);
    return it != current_.end() && it->second;
}

f32 InputSystem::axis(u16 axis_id) const {
    auto it = axes_.find(axis_id);
    return it != axes_.end() ? it->second : 0.0f;
}

// ── ActionMap ───────────────────────────────────────────

void ActionMap::add_context(ActionContext ctx) {
    context_map_[ctx.name] = contexts_.size();
    if (active_.empty()) active_ = ctx.name;
    contexts_.push_back(std::move(ctx));
}

void ActionMap::set_active(const std::string& context_name) {
    if (context_map_.contains(context_name)) {
        active_ = context_name;
    }
}

bool ActionMap::action_down(const std::string& action_name, const InputSystem& input) const {
    auto cit = context_map_.find(active_);
    if (cit == context_map_.end()) return false;
    auto& ctx = contexts_[cit->second];
    for (auto& action : ctx.actions) {
        if (action.name == action_name) {
            for (auto& b : action.bindings) {
                if (b.device == DeviceType::Keyboard && input.key_down(static_cast<Key>(b.code)))
                    return true;
            }
        }
    }
    return false;
}

bool ActionMap::action_pressed(const std::string& action_name, const InputSystem& input) const {
    auto cit = context_map_.find(active_);
    if (cit == context_map_.end()) return false;
    auto& ctx = contexts_[cit->second];
    for (auto& action : ctx.actions) {
        if (action.name == action_name) {
            for (auto& b : action.bindings) {
                if (b.device == DeviceType::Keyboard && input.key_pressed(static_cast<Key>(b.code)))
                    return true;
            }
        }
    }
    return false;
}

ActionContext ActionMap::default_gameplay_context() {
    ActionContext ctx;
    ctx.name = "gameplay";
    ctx.actions = {
        {"ジャンプ",  {{DeviceType::Keyboard, static_cast<u16>(Key::Space)}}},
        {"攻撃",      {{DeviceType::Keyboard, static_cast<u16>(Key::Z)}}},
        {"防御",      {{DeviceType::Keyboard, static_cast<u16>(Key::X)}}},
        {"上",        {{DeviceType::Keyboard, static_cast<u16>(Key::Up)},
                       {DeviceType::Keyboard, static_cast<u16>(Key::W)}}},
        {"下",        {{DeviceType::Keyboard, static_cast<u16>(Key::Down)},
                       {DeviceType::Keyboard, static_cast<u16>(Key::S)}}},
        {"左",        {{DeviceType::Keyboard, static_cast<u16>(Key::Left)},
                       {DeviceType::Keyboard, static_cast<u16>(Key::A)}}},
        {"右",        {{DeviceType::Keyboard, static_cast<u16>(Key::Right)},
                       {DeviceType::Keyboard, static_cast<u16>(Key::D)}}},
    };
    return ctx;
}

} // namespace engine::input
