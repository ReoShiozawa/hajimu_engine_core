/**
 * src/physics/physics_world.cpp — 物理エンジンスタブ実装
 *
 * デフォルトの簡易物理 (重力 + AABB衝突のみ)。
 * 本格的な物理は外部バックエンド (Jolt, Box2D) を接続。
 */
#include <engine/physics/physics_world.hpp>
#include <engine/ecs/world.hpp>
#include <engine/scene/transform.hpp>
#include <engine/core/log.hpp>
#include <cmath>

namespace engine::physics {

class SimplePhysicsWorld final : public PhysicsWorld {
public:
    Result<void> init(Vec3 gravity) override {
        gravity_ = gravity;
        ENG_INFO("SimplePhysicsWorld initialized (gravity: %.2f, %.2f, %.2f)",
                 gravity.x, gravity.y, gravity.z);
        return {};
    }

    void step(f32 dt) override {
        for (auto& [entity, body] : bodies_) {
            if (body.type != BodyType::Dynamic) continue;
            if (body.gravity_enabled) {
                body.velocity.x += gravity_.x * dt;
                body.velocity.y += gravity_.y * dt;
                body.velocity.z += gravity_.z * dt;
            }
            // ダンピング
            body.velocity.x *= (1.0f - body.linear_damping * dt);
            body.velocity.y *= (1.0f - body.linear_damping * dt);
            body.velocity.z *= (1.0f - body.linear_damping * dt);
        }
    }

    void add_body(Entity entity, const RigidBody& body, const CollisionShape& shape) override {
        bodies_[entity.id] = body;
        shapes_[entity.id] = shape;
    }

    void remove_body(Entity entity) override {
        bodies_.erase(entity.id);
        shapes_.erase(entity.id);
    }

    void apply_force(Entity entity, Vec3 force) override {
        auto it = bodies_.find(entity.id);
        if (it == bodies_.end() || it->second.mass <= 0) return;
        f32 inv_mass = 1.0f / it->second.mass;
        it->second.velocity.x += force.x * inv_mass;
        it->second.velocity.y += force.y * inv_mass;
        it->second.velocity.z += force.z * inv_mass;
    }

    void apply_impulse(Entity entity, Vec3 impulse) override {
        auto it = bodies_.find(entity.id);
        if (it == bodies_.end() || it->second.mass <= 0) return;
        f32 inv_mass = 1.0f / it->second.mass;
        it->second.velocity.x += impulse.x * inv_mass;
        it->second.velocity.y += impulse.y * inv_mass;
        it->second.velocity.z += impulse.z * inv_mass;
    }

    std::vector<RaycastHit> raycast(Vec3, Vec3, f32) const override {
        return {}; // TODO: 実装
    }

    std::vector<CollisionEvent> poll_collisions() const override {
        return {}; // TODO: AABB 衝突検出
    }

    void sync_transforms(ecs::World& world) override {
        for (auto& [eid, body] : bodies_) {
            Entity entity{eid};
            auto* tf = world.get_component<scene::Transform>(entity);
            if (!tf) continue;
            // 速度による位置更新
            tf->position.x += body.velocity.x * (1.0f / 60.0f);
            tf->position.y += body.velocity.y * (1.0f / 60.0f);
            tf->position.z += body.velocity.z * (1.0f / 60.0f);
        }
    }

private:
    Vec3 gravity_{0, -9.81f, 0};
    std::unordered_map<u64, RigidBody>      bodies_;
    std::unordered_map<u64, CollisionShape>  shapes_;
};

std::unique_ptr<PhysicsWorld> create_physics_world() {
    return std::make_unique<SimplePhysicsWorld>();
}

} // namespace engine::physics
