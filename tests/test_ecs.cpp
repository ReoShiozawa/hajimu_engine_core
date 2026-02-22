/**
 * tests/test_ecs.cpp — ECS ユニットテスト
 */
#include <engine/core/types.hpp>
#include <engine/ecs/entity.hpp>
#include <engine/ecs/world.hpp>
#include <cassert>
#include <cstdio>

using namespace engine;
using namespace engine::ecs;

// テスト用コンポーネント
struct Position {
    f32 x, y, z;
};

struct Velocity {
    f32 vx, vy, vz;
};

struct Health {
    i32 hp;
    i32 max_hp;
};

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void test_##name(); \
    struct TestRunner_##name { TestRunner_##name() { \
        printf("  テスト: %s ... ", #name); \
        try { test_##name(); printf("OK\n"); tests_passed++; } \
        catch (...) { printf("FAIL\n"); tests_failed++; } \
    }} runner_##name; \
    static void test_##name()

#define ASSERT(cond) do { if (!(cond)) { \
    printf("ASSERT FAILED: %s (line %d)\n", #cond, __LINE__); \
    throw 1; \
}} while(0)

// ── テスト ──────────────────────────────────────────────

TEST(entity_basics) {
    Entity e1(42, 1);
    ASSERT(e1.index() == 42);
    ASSERT(e1.generation() == 1);
    ASSERT(e1.valid());

    Entity null = Entity::null();
    ASSERT(!null.valid());
    ASSERT(null.id == 0);
}

TEST(entity_equality) {
    Entity a(1, 1);
    Entity b(1, 1);
    Entity c(1, 2);
    ASSERT(a == b);
    ASSERT(a != c);
}

TEST(world_spawn_despawn) {
    World world;
    Entity e1 = world.spawn();
    Entity e2 = world.spawn();
    ASSERT(world.entity_count() == 2);
    ASSERT(world.alive(e1));
    ASSERT(world.alive(e2));

    world.despawn(e1);
    ASSERT(world.entity_count() == 1);
    ASSERT(!world.alive(e1));
    ASSERT(world.alive(e2));
}

TEST(world_generation) {
    World world;
    Entity e1 = world.spawn();
    u32 idx = e1.index();
    world.despawn(e1);

    Entity e2 = world.spawn();
    // 同じインデックスが再利用されるが generation が異なる
    ASSERT(e2.index() == idx);
    ASSERT(e2.generation() > e1.generation());
    ASSERT(!world.alive(e1)); // 旧ハンドルは無効
    ASSERT(world.alive(e2));
}

TEST(add_get_component) {
    World world;
    Entity e = world.spawn();

    Position pos{1.0f, 2.0f, 3.0f};
    world.add_component(e, pos);
    ASSERT(world.has_component<Position>(e));

    auto* p = world.get_component<Position>(e);
    ASSERT(p != nullptr);
    ASSERT(p->x == 1.0f);
    ASSERT(p->y == 2.0f);
    ASSERT(p->z == 3.0f);
}

TEST(multiple_components) {
    World world;
    Entity e = world.spawn();

    world.add_component(e, Position{10, 20, 30});
    world.add_component(e, Velocity{1, 0, -1});
    world.add_component(e, Health{100, 100});

    ASSERT(world.has_component<Position>(e));
    ASSERT(world.has_component<Velocity>(e));
    ASSERT(world.has_component<Health>(e));

    auto* v = world.get_component<Velocity>(e);
    ASSERT(v->vx == 1.0f);
    auto* h = world.get_component<Health>(e);
    ASSERT(h->hp == 100);
}

TEST(remove_component) {
    World world;
    Entity e = world.spawn();
    world.add_component(e, Position{1, 2, 3});
    world.add_component(e, Velocity{4, 5, 6});

    world.remove_component<Position>(e);
    ASSERT(!world.has_component<Position>(e));
    ASSERT(world.has_component<Velocity>(e));

    auto* v = world.get_component<Velocity>(e);
    ASSERT(v->vx == 4.0f);
}

TEST(query_for_each) {
    World world;
    for (int i = 0; i < 10; ++i) {
        Entity e = world.spawn();
        world.add_component(e, Position{static_cast<f32>(i), 0, 0});
        world.add_component(e, Velocity{1, 0, 0});
    }

    // 5つは Position のみ
    for (int i = 0; i < 5; ++i) {
        Entity e = world.spawn();
        world.add_component(e, Position{100.0f + i, 0, 0});
    }

    int count = 0;
    world.query().with<Position, Velocity>().for_each<Position, Velocity>(
        [&](Entity, Position& p, Velocity& v) {
            p.x += v.vx;
            count++;
        });

    ASSERT(count == 10);
}

TEST(many_entities) {
    World world;
    for (int i = 0; i < 1000; ++i) {
        Entity e = world.spawn();
        world.add_component(e, Position{static_cast<f32>(i), 0, 0});
    }
    ASSERT(world.entity_count() == 1000);
}

// ── メイン ──────────────────────────────────────────────

int main() {
    printf("=== engine_core ECS テスト ===\n");
    // テストはグローバルコンストラクタで自動実行済み
    printf("\n結果: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
