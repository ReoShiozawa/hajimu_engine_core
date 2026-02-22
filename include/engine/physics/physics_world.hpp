/**
 * engine/physics/physics_world.hpp — 物理エンジン統合インターフェース
 *
 * 2D/3D 物理シミュレーション抽象層。
 * 外部バックエンド (Box2D, Jolt, Bullet 等) をラップ。
 */
#pragma once

#include <engine/core/types.hpp>
#include <engine/ecs/entity.hpp>
#include <vector>
#include <functional>

namespace engine::ecs { class World; }

namespace engine::physics {

using ecs::Entity;

// ── コリジョン形状 ──────────────────────────────────────
enum class ShapeType : u8 {
    Sphere, Box, Capsule, Plane, Mesh, HeightField,
    Circle2D, Rect2D, Polygon2D   // 2D 用
};

struct CollisionShape {
    ShapeType type;
    Vec3      half_extents{0, 0, 0};
    f32       radius = 0;
    f32       height = 0;
};

// ── Rigidbody ───────────────────────────────────────────
enum class BodyType : u8 { Static, Kinematic, Dynamic };

struct RigidBody {
    BodyType type = BodyType::Dynamic;
    f32      mass = 1.0f;
    f32      linear_damping = 0.01f;
    f32      angular_damping = 0.05f;
    f32      restitution = 0.3f;
    f32      friction = 0.5f;
    Vec3     velocity{0, 0, 0};
    Vec3     angular_velocity{0, 0, 0};
    bool     gravity_enabled = true;
};

// ── コリジョンイベント ──────────────────────────────────
struct CollisionEvent {
    Entity entity_a;
    Entity entity_b;
    Vec3   contact_point{0, 0, 0};
    Vec3   contact_normal{0, 0, 0};
    f32    penetration = 0;
};

// ── レイキャスト結果 ────────────────────────────────────
struct RaycastHit {
    Entity entity;
    Vec3   point{0, 0, 0};
    Vec3   normal{0, 0, 0};
    f32    distance = 0;
};

// ── PhysicsWorld ────────────────────────────────────────
class PhysicsWorld {
public:
    PhysicsWorld() = default;
    virtual ~PhysicsWorld() = default;

    /// ワールド初期化
    virtual Result<void> init(Vec3 gravity = {0, -9.81f, 0}) = 0;

    /// ステップ
    virtual void step(f32 dt) = 0;

    /// ボディ追加
    virtual void add_body(Entity entity, const RigidBody& body, const CollisionShape& shape) = 0;

    /// ボディ除去
    virtual void remove_body(Entity entity) = 0;

    /// 力を加える
    virtual void apply_force(Entity entity, Vec3 force) = 0;

    /// 衝撃を加える
    virtual void apply_impulse(Entity entity, Vec3 impulse) = 0;

    /// レイキャスト
    [[nodiscard]] virtual std::vector<RaycastHit> raycast(Vec3 origin, Vec3 direction, f32 max_dist) const = 0;

    /// コリジョンイベント取得
    [[nodiscard]] virtual std::vector<CollisionEvent> poll_collisions() const = 0;

    /// ECSと同期 (Transform ↔ RigidBody)
    virtual void sync_transforms(ecs::World& world) = 0;
};

/// デフォルト物理ワールド生成
std::unique_ptr<PhysicsWorld> create_physics_world();

} // namespace engine::physics
