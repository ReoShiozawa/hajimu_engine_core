/**
 * src/scene/transform.cpp — Transform 実装
 */
#include <engine/scene/transform.hpp>
#include <engine/scene/scene_graph.hpp>
#include <engine/ecs/world.hpp>
#include <cmath>

namespace engine::scene {

Mat4 Transform::local_matrix() const {
    // Scale → Rotate(Quaternion) → Translate の順
    // クォータニオン → 回転行列変換
    f32 xx = rotation.x * rotation.x;
    f32 yy = rotation.y * rotation.y;
    f32 zz = rotation.z * rotation.z;
    f32 xy = rotation.x * rotation.y;
    f32 xz = rotation.x * rotation.z;
    f32 yz = rotation.y * rotation.z;
    f32 wx = rotation.w * rotation.x;
    f32 wy = rotation.w * rotation.y;
    f32 wz = rotation.w * rotation.z;

    Mat4 m{};
    m.m[0]  = (1.0f - 2.0f * (yy + zz)) * scale.x;
    m.m[1]  = (2.0f * (xy + wz))         * scale.x;
    m.m[2]  = (2.0f * (xz - wy))         * scale.x;
    m.m[3]  = 0.0f;

    m.m[4]  = (2.0f * (xy - wz))         * scale.y;
    m.m[5]  = (1.0f - 2.0f * (xx + zz))  * scale.y;
    m.m[6]  = (2.0f * (yz + wx))         * scale.y;
    m.m[7]  = 0.0f;

    m.m[8]  = (2.0f * (xz + wy))         * scale.z;
    m.m[9]  = (2.0f * (yz - wx))         * scale.z;
    m.m[10] = (1.0f - 2.0f * (xx + yy))  * scale.z;
    m.m[11] = 0.0f;

    m.m[12] = position.x;
    m.m[13] = position.y;
    m.m[14] = position.z;
    m.m[15] = 1.0f;

    return m;
}

Vec3 Transform::forward() const {
    f32 xx = rotation.x * rotation.x;
    f32 yy = rotation.y * rotation.y;
    f32 xz = rotation.x * rotation.z;
    f32 yz = rotation.y * rotation.z;
    f32 wx = rotation.w * rotation.x;
    f32 wy = rotation.w * rotation.y;
    return Vec3{
        2.0f * (xz + wy),
        2.0f * (yz - wx),
        1.0f - 2.0f * (xx + yy)
    };
}

Vec3 Transform::right() const {
    f32 yy = rotation.y * rotation.y;
    f32 zz = rotation.z * rotation.z;
    f32 xy = rotation.x * rotation.y;
    f32 xz = rotation.x * rotation.z;
    f32 wy = rotation.w * rotation.y;
    f32 wz = rotation.w * rotation.z;
    return Vec3{
        1.0f - 2.0f * (yy + zz),
        2.0f * (xy + wz),
        2.0f * (xz - wy)
    };
}

Vec3 Transform::up() const {
    f32 xx = rotation.x * rotation.x;
    f32 zz = rotation.z * rotation.z;
    f32 xy = rotation.x * rotation.y;
    f32 yz = rotation.y * rotation.z;
    f32 wx = rotation.w * rotation.x;
    f32 wz = rotation.w * rotation.z;
    return Vec3{
        2.0f * (xy - wz),
        1.0f - 2.0f * (xx + zz),
        2.0f * (yz + wx)
    };
}

// ── ワールド行列更新 ────────────────────────────────────

static Mat4 mat4_multiply(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            r.m[i * 4 + j] = 0;
            for (int k = 0; k < 4; ++k) {
                r.m[i * 4 + j] += a.m[i * 4 + k] * b.m[k * 4 + j];
            }
        }
    }
    return r;
}

static void update_recursive(SceneGraph& graph, ecs::World& world,
                              ecs::Entity entity, const Mat4& parent_world) {
    auto* tf = world.get_component<Transform>(entity);
    if (!tf) return;

    Mat4 local = tf->local_matrix();
    Mat4 world_mat = mat4_multiply(parent_world, local);

    auto* wtf = world.get_component<WorldTransform>(entity);
    if (wtf) {
        wtf->matrix = world_mat;
        wtf->dirty = false;
    }

    auto* node = graph.find(entity);
    if (!node) return;
    for (auto child : node->children) {
        update_recursive(graph, world, child, world_mat);
    }
}

void update_world_transforms(SceneGraph& graph, ecs::World& world) {
    Mat4 identity{};
    for (int i = 0; i < 16; ++i) identity.m[i] = 0;
    identity.m[0] = identity.m[5] = identity.m[10] = identity.m[15] = 1.0f;

    for (auto root : graph.roots()) {
        update_recursive(graph, world, root, identity);
    }
}

} // namespace engine::scene
