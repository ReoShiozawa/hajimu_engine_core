/**
 * engine/input/input_system.hpp — Action ベース入力システム
 *
 * キーボード/マウス/ゲームパッド/タッチ/VR の抽象化。
 * Action Map による間接的入力バインディング。
 */
#pragma once

#include <engine/core/types.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace engine::input {

// ── 入力デバイス種別 ────────────────────────────────────
enum class DeviceType : u8 {
    Keyboard, Mouse, GamepadAxis, GamepadButton, Touch, VR
};

// ── キー定義 (主要キーのみ) ─────────────────────────────
enum class Key : u16 {
    Unknown = 0,
    // 文字・数字
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    // 制御
    Space = 32, Enter = 13, Escape = 27, Tab = 9, Backspace = 8,
    // 矢印
    Left = 263, Right = 262, Up = 265, Down = 264,
    // 修飾
    LShift = 340, RShift = 344, LCtrl = 341, RCtrl = 345,
    LAlt = 342, RAlt = 346,
};

// ── マウスボタン ────────────────────────────────────────
enum class MouseButton : u8 { Left = 0, Right, Middle };

// ── 入力イベント ────────────────────────────────────────
struct InputEvent {
    DeviceType device;
    u16        code;     // Key or button code
    f32        value;    // 0.0~1.0 (axis) or 0/1 (button)
    bool       pressed;
};

// ── InputSystem ─────────────────────────────────────────
class InputSystem {
public:
    InputSystem();
    ~InputSystem();

    /// フレーム先頭で呼ぶ
    void begin_frame();

    /// イベント投入
    void push_event(const InputEvent& event);

    /// キー状態
    [[nodiscard]] bool key_down(Key key) const;
    [[nodiscard]] bool key_pressed(Key key) const;    // このフレームで押された
    [[nodiscard]] bool key_released(Key key) const;   // このフレームで離された

    /// マウス
    [[nodiscard]] Vec2 mouse_position() const { return mouse_pos_; }
    [[nodiscard]] Vec2 mouse_delta() const { return mouse_delta_; }
    [[nodiscard]] bool mouse_button_down(MouseButton btn) const;

    /// 軸値 (ゲームパッド)
    [[nodiscard]] f32 axis(u16 axis_id) const;

private:
    std::unordered_map<u16, bool> current_;
    std::unordered_map<u16, bool> previous_;
    std::unordered_map<u16, f32>  axes_;
    Vec2 mouse_pos_{0, 0};
    Vec2 mouse_delta_{0, 0};
};

} // namespace engine::input
