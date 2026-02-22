/**
 * engine/scene/transform.hpp — Transform コンポーネント
 *
 * ローカル座標 → ワールド行列変換。
 * Position, Rotation (Quaternion), Scale。
 */
#pragma once

#include <engine/core/types.hpp>

namespace engine::ecs { class World; }

namespace engine::scene {

// ── Transform (ECS コンポーネント) ──────────────────────
struct Transform {
    Vec3 position{0, 0, 0};
    Quat rotation{0, 0, 0, 1};
    Vec3 scale{1, 1, 1};

    /// ローカル行列を計算
    [[nodiscard]] Mat4 local_matrix() const;

    /// forward / right / up ベクトル
    [[nodiscard]] Vec3 forward() const;
    [[nodiscard]] Vec3 right() const;
    [[nodiscard]] Vec3 up() const;

    /// 平行移動
    void translate(Vec3 delta) { position = {position.x + delta.x, position.y + delta.y, position.z + delta.z}; }
};

// ── WorldTransform (キャッシュ済みワールド行列) ─────────
struct WorldTransform {
    Mat4 matrix{};
    bool dirty = true;
};

/// シーングラフの親子関係からワールド行列を再計算
void update_world_transforms(class SceneGraph& graph,
                             class ecs::World& world);

} // namespace engine::scene
