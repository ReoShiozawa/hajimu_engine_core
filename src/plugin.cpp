/**
 * src/plugin.cpp — はじむプラグインエントリポイント
 *
 * engine_core の全機能をはじむスクリプトから利用可能にする。
 * 日本語 API 関数テーブルをエクスポート。
 */

extern "C" {
#include "hajimu_plugin.h"
}

#include <engine/engine.hpp>
#include <string>
#include <memory>
#include <cstring>
#include <unordered_map>

using namespace engine;
using namespace engine::ecs;
using namespace engine::scene;
using namespace engine::input;
using namespace engine::resource;
using namespace engine::render;
using namespace engine::physics;
using namespace engine::audio;

// ── グローバル状態 ──────────────────────────────────────

static std::unique_ptr<World>           g_world;
static std::unique_ptr<SceneGraph>      g_scene;
static std::unique_ptr<InputSystem>     g_input;
static std::unique_ptr<ActionMap>       g_actions;
static std::unique_ptr<VFS>             g_vfs;
static std::unique_ptr<ResourceManager> g_res;
static std::unique_ptr<RenderGraph>     g_render_graph;
static std::unique_ptr<PhysicsWorld>    g_physics;

// Entity → ID マッピング
static std::unordered_map<u64, Entity> g_entities;

// ── スクリプト ECS ─────────────────────────────────────
// JP スクリプトから動的にコンポーネントを登録・操作するための
// 軽量フロートアレイベースのコンポーネントストア。
// C++ ネイティブ ECS (World) の Entity と共存する。

struct ScriptCompDef {
    std::string name;
    int         default_size;  // デフォルトのフロート数 (参考用)
};

// 登録済みコンポーネント (index = comp_id - 1)
static std::vector<ScriptCompDef>               g_sc_defs;
// 名前 → comp_id マッピング (1-indexed)
static std::unordered_map<std::string, uint8_t> g_sc_name_map;
// エンティティ別データ: entity.id → { comp_id → float[] }
static std::unordered_map<u64, std::unordered_map<uint8_t, std::vector<float>>> g_sc_data;
// エンティティ別コンポーネントビットマスク: entity.id → mask
static std::unordered_map<u64, uint32_t>        g_sc_mask;
// クエリ結果バッファ
static std::vector<u64>                         g_sc_query;

static void sc_clear_all() {
    g_sc_defs.clear();
    g_sc_name_map.clear();
    g_sc_data.clear();
    g_sc_mask.clear();
    g_sc_query.clear();
}

static void sc_clear_entities() {
    g_sc_data.clear();
    g_sc_mask.clear();
    g_sc_query.clear();
}

// CSV 文字列 → float 配列
static std::vector<float> sc_parse_csv(const std::string& s) {
    std::vector<float> out;
    if (s.empty()) return out;
    size_t pos = 0;
    while (pos <= s.size()) {
        size_t end = s.find(',', pos);
        if (end == std::string::npos) end = s.size();
        std::string tok = s.substr(pos, end - pos);
        if (!tok.empty()) out.push_back(std::stof(tok));
        pos = end + 1;
    }
    return out;
}

// float 配列 → CSV 文字列
static std::string sc_to_csv(const std::vector<float>& v) {
    std::string out;
    char buf[32];
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) out += ',';
        snprintf(buf, sizeof(buf), "%.6g", (double)v[i]);
        out += buf;
    }
    return out;
}

// ── ヘルパー ────────────────────────────────────────────

static Entity resolve_entity(Value* argv) {
    if (argv[0].type == VALUE_NUMBER) {
        u64 id = static_cast<u64>(argv[0].number);
        return Entity{id};
    }
    return Entity::null();
}

static Value entity_to_value(Entity e) {
    return hajimu_number(static_cast<double>(e.id));
}

// =============================================================================
// ワールド / エンジン管理
// =============================================================================

static Value fn_engine_init(int argc, Value* argv) {
    (void)argc; (void)argv;
    g_world = std::make_unique<World>();
    g_scene = std::make_unique<SceneGraph>();
    g_input = std::make_unique<InputSystem>();
    g_actions = std::make_unique<ActionMap>();
    g_vfs = std::make_unique<VFS>();
    g_res = std::make_unique<ResourceManager>(*g_vfs);
    g_render_graph = std::make_unique<RenderGraph>();
    g_physics = create_physics_world();
    g_physics->init({0, -9.81f, 0});
    ENG_INFO("Engine Core initialized");
    return hajimu_bool(true);
}

static Value fn_engine_shutdown(int argc, Value* argv) {
    (void)argc; (void)argv;
    g_physics.reset();
    g_render_graph.reset();
    g_res.reset();
    g_vfs.reset();
    g_actions.reset();
    g_input.reset();
    g_scene.reset();
    g_world.reset();
    g_entities.clear();
    sc_clear_all();
    ENG_INFO("Engine Core shutdown");
    return hajimu_null();
}

static Value fn_engine_version(int argc, Value* argv) {
    (void)argc; (void)argv;
    return hajimu_string(engine::VERSION);
}

// =============================================================================
// Entity 操作
// =============================================================================

static Value fn_entity_spawn(int argc, Value* argv) {
    (void)argc; (void)argv;
    if (!g_world) return hajimu_null();
    Entity e = g_world->spawn();
    g_entities[e.id] = e;
    return entity_to_value(e);
}

static Value fn_entity_despawn(int argc, Value* argv) {
    if (!g_world) return hajimu_null();
    Entity e = resolve_entity(argv);
    g_world->despawn(e);
    g_entities.erase(e.id);
    return hajimu_null();
}

static Value fn_entity_alive(int argc, Value* argv) {
    if (!g_world) return hajimu_bool(false);
    Entity e = resolve_entity(argv);
    return hajimu_bool(g_world->alive(e));
}

static Value fn_entity_count(int argc, Value* argv) {
    (void)argc; (void)argv;
    if (!g_world) return hajimu_number(0);
    return hajimu_number(g_world->entity_count());
}

// =============================================================================
// シーングラフ
// =============================================================================

static Value fn_scene_add(int argc, Value* argv) {
    if (!g_scene) return hajimu_null();
    Entity e = resolve_entity(argv);
    std::string name = "Node";
    if (argc >= 2 && argv[1].type == VALUE_STRING) {
        name = std::string(argv[1].string.data, argv[1].string.length);
    }
    Entity parent = Entity::null();
    if (argc >= 3 && argv[2].type == VALUE_NUMBER) {
        parent = Entity{static_cast<u64>(argv[2].number)};
    }
    g_scene->add_node(e, name, parent);
    return entity_to_value(e);
}

static Value fn_scene_remove(int argc, Value* argv) {
    if (!g_scene) return hajimu_null();
    g_scene->remove_node(resolve_entity(argv));
    return hajimu_null();
}

static Value fn_scene_find(int argc, Value* argv) {
    if (!g_scene) return hajimu_null();
    if (argv[0].type == VALUE_STRING) {
        std::string name(argv[0].string.data, argv[0].string.length);
        Entity e = g_scene->find_by_name(name);
        return entity_to_value(e);
    }
    return hajimu_null();
}

static Value fn_scene_reparent(int argc, Value* argv) {
    if (!g_scene || argc < 2) return hajimu_null();
    Entity child = resolve_entity(argv);
    Entity parent = Entity{static_cast<u64>(argv[1].number)};
    g_scene->reparent(child, parent);
    return hajimu_null();
}

static Value fn_scene_node_count(int argc, Value* argv) {
    (void)argc; (void)argv;
    if (!g_scene) return hajimu_number(0);
    return hajimu_number(g_scene->node_count());
}

// =============================================================================
// 入力
// =============================================================================

static Value fn_input_key_down(int argc, Value* argv) {
    if (!g_input || argc < 1) return hajimu_bool(false);
    Key key = static_cast<Key>(static_cast<u16>(argv[0].number));
    return hajimu_bool(g_input->key_down(key));
}

static Value fn_input_key_pressed(int argc, Value* argv) {
    if (!g_input || argc < 1) return hajimu_bool(false);
    Key key = static_cast<Key>(static_cast<u16>(argv[0].number));
    return hajimu_bool(g_input->key_pressed(key));
}

static Value fn_input_mouse_pos(int argc, Value* argv) {
    (void)argc; (void)argv;
    if (!g_input) return hajimu_null();
    Vec2 pos = g_input->mouse_position();
    Value arr = hajimu_array();
    hajimu_array_push(&arr, hajimu_number(pos.x));
    hajimu_array_push(&arr, hajimu_number(pos.y));
    return arr;
}

static Value fn_input_begin_frame(int argc, Value* argv) {
    (void)argc; (void)argv;
    if (g_input) g_input->begin_frame();
    return hajimu_null();
}

static Value fn_action_pressed(int argc, Value* argv) {
    if (!g_actions || !g_input || argc < 1) return hajimu_bool(false);
    std::string name(argv[0].string.data, argv[0].string.length);
    return hajimu_bool(g_actions->action_pressed(name, *g_input));
}

static Value fn_action_down(int argc, Value* argv) {
    if (!g_actions || !g_input || argc < 1) return hajimu_bool(false);
    std::string name(argv[0].string.data, argv[0].string.length);
    return hajimu_bool(g_actions->action_down(name, *g_input));
}

// =============================================================================
// VFS / リソース
// =============================================================================

static Value fn_vfs_mount(int argc, Value* argv) {
    if (!g_vfs || argc < 2) return hajimu_bool(false);
    MountPoint mp;
    mp.prefix = std::string(argv[0].string.data, argv[0].string.length);
    mp.real_path = std::string(argv[1].string.data, argv[1].string.length);
    if (argc >= 3) mp.priority = static_cast<i32>(argv[2].number);
    g_vfs->mount(mp);
    return hajimu_bool(true);
}

static Value fn_vfs_read(int argc, Value* argv) {
    if (!g_vfs || argc < 1) return hajimu_null();
    std::string path(argv[0].string.data, argv[0].string.length);
    auto text = g_vfs->read_text(path);
    if (!text) return hajimu_null();
    return hajimu_string(text->c_str());
}

static Value fn_vfs_exists(int argc, Value* argv) {
    if (!g_vfs || argc < 1) return hajimu_bool(false);
    std::string path(argv[0].string.data, argv[0].string.length);
    return hajimu_bool(g_vfs->exists(path));
}

// =============================================================================
// 物理
// =============================================================================

static Value fn_physics_step(int argc, Value* argv) {
    if (!g_physics) return hajimu_null();
    f32 dt = (argc >= 1) ? static_cast<f32>(argv[0].number) : (1.0f / 60.0f);
    g_physics->step(dt);
    return hajimu_null();
}

static Value fn_physics_add_body(int argc, Value* argv) {
    if (!g_physics || argc < 1) return hajimu_null();
    Entity e = resolve_entity(argv);
    RigidBody body;
    if (argc >= 2) body.mass = static_cast<f32>(argv[1].number);
    CollisionShape shape;
    shape.type = ShapeType::Sphere;
    shape.radius = 0.5f;
    g_physics->add_body(e, body, shape);
    return hajimu_null();
}

static Value fn_physics_apply_force(int argc, Value* argv) {
    if (!g_physics || argc < 4) return hajimu_null();
    Entity e = resolve_entity(argv);
    Vec3 force{
        static_cast<f32>(argv[1].number),
        static_cast<f32>(argv[2].number),
        static_cast<f32>(argv[3].number)
    };
    g_physics->apply_force(e, force);
    return hajimu_null();
}

static Value fn_physics_sync(int argc, Value* argv) {
    (void)argc; (void)argv;
    if (!g_physics || !g_world) return hajimu_null();
    g_physics->sync_transforms(*g_world);
    return hajimu_null();
}

// =============================================================================
// レンダーグラフ
// =============================================================================

static Value fn_render_add_pass(int argc, Value* argv) {
    if (!g_render_graph || argc < 1) return hajimu_null();
    RenderPass pass;
    pass.name = std::string(argv[0].string.data, argv[0].string.length);
    g_render_graph->add_pass(std::move(pass));
    return hajimu_bool(true);
}

static Value fn_render_compile(int argc, Value* argv) {
    (void)argc; (void)argv;
    if (!g_render_graph) return hajimu_bool(false);
    auto result = g_render_graph->compile();
    return hajimu_bool(result.has_value());
}

static Value fn_render_execute(int argc, Value* argv) {
    (void)argc; (void)argv;
    if (!g_render_graph) return hajimu_null();
    g_render_graph->execute();
    return hajimu_null();
}

static Value fn_render_clear(int argc, Value* argv) {
    (void)argc; (void)argv;
    if (g_render_graph) g_render_graph->clear();
    return hajimu_null();
}

// =============================================================================
// メモリ / ログ / ユーティリティ
// =============================================================================

static Value fn_log_info(int argc, Value* argv) {
    if (argc < 1) return hajimu_null();
    if (argv[0].type == VALUE_STRING) {
        ENG_INFO("%s", std::string(argv[0].string.data, argv[0].string.length).c_str());
    }
    return hajimu_null();
}

static Value fn_log_warn(int argc, Value* argv) {
    if (argc < 1) return hajimu_null();
    if (argv[0].type == VALUE_STRING) {
        ENG_WARN("%s", std::string(argv[0].string.data, argv[0].string.length).c_str());
    }
    return hajimu_null();
}

static Value fn_log_error(int argc, Value* argv) {
    if (argc < 1) return hajimu_null();
    if (argv[0].type == VALUE_STRING) {
        ENG_ERROR("%s", std::string(argv[0].string.data, argv[0].string.length).c_str());
    }
    return hajimu_null();
}

static Value fn_archetype_count(int argc, Value* argv) {
    (void)argc; (void)argv;
    if (!g_world) return hajimu_number(0);
    return hajimu_number(g_world->archetype_count());
}

static Value fn_flush_commands(int argc, Value* argv) {
    (void)argc; (void)argv;
    if (g_world) g_world->flush_commands();
    return hajimu_null();
}

// =============================================================================
// スクリプト ECS — 動的コンポーネントシステム
//
// JP スクリプトから名前ベースでコンポーネントを登録・操作する。
// データは float 配列として保持し、カンマ区切り文字列で受け渡す。
// C++ ネイティブ World の Entity と同じ ID を共有する。
// =============================================================================

static Value fn_sc_comp_register(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VALUE_STRING) return hajimu_number(0);
    std::string name(argv[0].string.data, argv[0].string.length);
    int def_sz = (argc >= 2 && argv[1].type == VALUE_NUMBER) ? (int)argv[1].number : 4;

    // 既存チェック
    auto it = g_sc_name_map.find(name);
    if (it != g_sc_name_map.end()) return hajimu_number(it->second);

    if (g_sc_defs.size() >= 32) return hajimu_number(0);
    uint8_t cid = static_cast<uint8_t>(g_sc_defs.size() + 1);
    g_sc_defs.push_back({name, def_sz});
    g_sc_name_map[name] = cid;
    return hajimu_number(cid);
}

static Value fn_sc_comp_find(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VALUE_STRING) return hajimu_number(0);
    std::string name(argv[0].string.data, argv[0].string.length);
    auto it = g_sc_name_map.find(name);
    return hajimu_number(it != g_sc_name_map.end() ? it->second : 0);
}

static Value fn_sc_comp_name(int argc, Value* argv) {
    if (argc < 1) return hajimu_string("");
    int cid = (int)argv[0].number;
    if (cid < 1 || cid > (int)g_sc_defs.size()) return hajimu_string("");
    return hajimu_string(g_sc_defs[cid - 1].name.c_str());
}

static Value fn_sc_comp_count(int argc, Value* argv) {
    (void)argc; (void)argv;
    return hajimu_number(g_sc_defs.size());
}

static Value fn_sc_comp_set(int argc, Value* argv) {
    if (argc < 3) return hajimu_null();
    u64 eid = static_cast<u64>(argv[0].number);
    uint8_t cid = static_cast<uint8_t>(argv[1].number);
    if (cid < 1 || cid > (int)g_sc_defs.size()) return hajimu_null();

    std::string csv;
    if (argv[2].type == VALUE_STRING)
        csv = std::string(argv[2].string.data, argv[2].string.length);
    auto data = sc_parse_csv(csv);

    g_sc_data[eid][cid] = data;
    g_sc_mask[eid] |= (1u << (cid - 1));
    return hajimu_null();
}

static Value fn_sc_comp_get(int argc, Value* argv) {
    if (argc < 2) return hajimu_string("");
    u64 eid = static_cast<u64>(argv[0].number);
    uint8_t cid = static_cast<uint8_t>(argv[1].number);
    auto eit = g_sc_data.find(eid);
    if (eit == g_sc_data.end()) return hajimu_string("");
    auto cit = eit->second.find(cid);
    if (cit == eit->second.end()) return hajimu_string("");
    return hajimu_string(sc_to_csv(cit->second).c_str());
}

static Value fn_sc_comp_set_f(int argc, Value* argv) {
    if (argc < 4) return hajimu_null();
    u64 eid = static_cast<u64>(argv[0].number);
    uint8_t cid = static_cast<uint8_t>(argv[1].number);
    int idx = (int)argv[2].number;
    float val = (float)argv[3].number;
    if (cid < 1 || cid > (int)g_sc_defs.size() || idx < 0 || idx >= 64) return hajimu_null();
    auto& v = g_sc_data[eid][cid];
    if (idx >= (int)v.size()) v.resize(idx + 1, 0.0f);
    v[idx] = val;
    g_sc_mask[eid] |= (1u << (cid - 1));
    return hajimu_null();
}

static Value fn_sc_comp_get_f(int argc, Value* argv) {
    if (argc < 3) return hajimu_number(0);
    u64 eid = static_cast<u64>(argv[0].number);
    uint8_t cid = static_cast<uint8_t>(argv[1].number);
    int idx = (int)argv[2].number;
    auto eit = g_sc_data.find(eid);
    if (eit == g_sc_data.end()) return hajimu_number(0);
    auto cit = eit->second.find(cid);
    if (cit == eit->second.end()) return hajimu_number(0);
    if (idx < 0 || idx >= (int)cit->second.size()) return hajimu_number(0);
    return hajimu_number(cit->second[idx]);
}

static Value fn_sc_comp_has(int argc, Value* argv) {
    if (argc < 2) return hajimu_bool(false);
    u64 eid = static_cast<u64>(argv[0].number);
    uint8_t cid = static_cast<uint8_t>(argv[1].number);
    auto it = g_sc_mask.find(eid);
    if (it == g_sc_mask.end()) return hajimu_bool(false);
    return hajimu_bool((it->second & (1u << (cid - 1))) != 0);
}

static Value fn_sc_comp_remove(int argc, Value* argv) {
    if (argc < 2) return hajimu_null();
    u64 eid = static_cast<u64>(argv[0].number);
    uint8_t cid = static_cast<uint8_t>(argv[1].number);
    auto eit = g_sc_data.find(eid);
    if (eit != g_sc_data.end()) eit->second.erase(cid);
    auto mit = g_sc_mask.find(eid);
    if (mit != g_sc_mask.end()) mit->second &= ~(1u << (cid - 1));
    return hajimu_null();
}

static Value fn_sc_tag_add(int argc, Value* argv) {
    if (argc < 2) return hajimu_null();
    u64 eid = static_cast<u64>(argv[0].number);
    uint8_t cid = static_cast<uint8_t>(argv[1].number);
    if (cid < 1 || cid > (int)g_sc_defs.size()) return hajimu_null();
    if (g_sc_data[eid].find(cid) == g_sc_data[eid].end())
        g_sc_data[eid][cid] = {};
    g_sc_mask[eid] |= (1u << (cid - 1));
    return hajimu_null();
}

static Value fn_sc_tag_remove(int argc, Value* argv) {
    return fn_sc_comp_remove(argc, argv);
}

// クエリ: comp_mask で AND 検索 (g_sc_mask を走査)
static Value fn_sc_query(int argc, Value* argv) {
    uint32_t mask = (argc >= 1) ? static_cast<uint32_t>(argv[0].number) : 0;
    g_sc_query.clear();
    for (auto& [eid, emask] : g_sc_mask) {
        if ((emask & mask) == mask)
            g_sc_query.push_back(eid);
    }
    return hajimu_number(g_sc_query.size());
}

// クエリ: comp_id 一覧からマスクを作って AND 検索 (最大 8 個)
static Value fn_sc_query_comps(int argc, Value* argv) {
    uint32_t mask = 0;
    for (int i = 0; i < argc && i < 8; ++i) {
        int cid = (int)argv[i].number;
        if (cid >= 1 && cid <= 32) mask |= (1u << (cid - 1));
    }
    g_sc_query.clear();
    for (auto& [eid, emask] : g_sc_mask) {
        if ((emask & mask) == mask)
            g_sc_query.push_back(eid);
    }
    return hajimu_number(g_sc_query.size());
}

static Value fn_sc_query_get(int argc, Value* argv) {
    if (argc < 1) return hajimu_number(0);
    int idx = (int)argv[0].number;
    if (idx < 0 || idx >= (int)g_sc_query.size()) return hajimu_number(0);
    return hajimu_number(g_sc_query[idx]);
}

static Value fn_sc_mask_make(int argc, Value* argv) {
    uint32_t mask = 0;
    for (int i = 0; i < argc; ++i) {
        int cid = (int)argv[i].number;
        if (cid >= 1 && cid <= 32) mask |= (1u << (cid - 1));
    }
    return hajimu_number(mask);
}

// ワールドクリア (エンティティ + スクリプトコンポーネントデータを消去)
static Value fn_sc_world_clear(int argc, Value* argv) {
    (void)argc; (void)argv;
    if (g_world) {
        // World を再作成して全 Entity を削除
        g_world = std::make_unique<World>();
    }
    g_entities.clear();
    sc_clear_entities();
    return hajimu_null();
}

// =============================================================================
// 関数テーブル
// =============================================================================

static HajimuPluginFunc engine_functions[] = {
    // ── エンジン管理 ────────────────────────────────────
    {"エンジン初期化",         fn_engine_init,        0, 0},
    {"エンジン終了",           fn_engine_shutdown,     0, 0},
    {"エンジンバージョン",     fn_engine_version,      0, 0},

    // ── Entity (ECS) ────────────────────────────────────
    {"エンティティ作成",       fn_entity_spawn,        0, 0},
    {"エンティティ削除",       fn_entity_despawn,      1, 1},
    {"エンティティ生存確認",   fn_entity_alive,        1, 1},
    {"エンティティ数",         fn_entity_count,        0, 0},
    {"アーキタイプ数",         fn_archetype_count,     0, 0},
    {"コマンド実行",           fn_flush_commands,      0, 0},

    // ── スクリプト ECS (動的コンポーネントシステム) ──────
    {"コンポーネント登録",     fn_sc_comp_register,    1, 2},
    {"コンポーネントID取得",   fn_sc_comp_find,        1, 1},
    {"コンポーネント名",       fn_sc_comp_name,        1, 1},
    {"コンポーネント種別数",   fn_sc_comp_count,       0, 0},
    {"コンポーネント設定",     fn_sc_comp_set,         3, 3},
    {"コンポーネント取得",     fn_sc_comp_get,         2, 2},
    {"コンポーネント値設定",   fn_sc_comp_set_f,       4, 4},
    {"コンポーネント値取得",   fn_sc_comp_get_f,       3, 3},
    {"コンポーネント有無",     fn_sc_comp_has,         2, 2},
    {"コンポーネント削除",     fn_sc_comp_remove,      2, 2},
    {"タグ追加",               fn_sc_tag_add,          2, 2},
    {"タグ削除",               fn_sc_tag_remove,       2, 2},
    {"クエリ実行",             fn_sc_query,            1, 1},
    {"クエリコンポーネント",   fn_sc_query_comps,      1, 8},
    {"クエリ結果取得",         fn_sc_query_get,        1, 1},
    {"コンポーネントマスク",   fn_sc_mask_make,        1, 8},
    {"ワールドクリア",         fn_sc_world_clear,      0, 0},

    // ── シーングラフ ────────────────────────────────────
    {"シーン追加",            fn_scene_add,            1, 3},
    {"シーン削除",            fn_scene_remove,         1, 1},
    {"シーン検索",            fn_scene_find,           1, 1},
    {"シーン親変更",          fn_scene_reparent,       2, 2},
    {"シーンノード数",        fn_scene_node_count,     0, 0},

    // ── 入力 ────────────────────────────────────────────
    {"キー押下中",            fn_input_key_down,       1, 1},
    {"キー押下",              fn_input_key_pressed,    1, 1},
    {"マウス座標",            fn_input_mouse_pos,      0, 0},
    {"入力フレーム開始",      fn_input_begin_frame,    0, 0},
    {"アクション押下",        fn_action_pressed,       1, 1},
    {"アクション押下中",      fn_action_down,          1, 1},

    // ── VFS / リソース ──────────────────────────────────
    {"VFSマウント",           fn_vfs_mount,            2, 3},
    {"VFS読込",               fn_vfs_read,             1, 1},
    {"VFS存在確認",           fn_vfs_exists,           1, 1},

    // ── 物理 ────────────────────────────────────────────
    {"物理ステップ",          fn_physics_step,         0, 1},
    {"物理ボディ追加",        fn_physics_add_body,     1, 2},
    {"物理力適用",            fn_physics_apply_force,  4, 4},
    {"物理同期",              fn_physics_sync,         0, 0},

    // ── レンダー ────────────────────────────────────────
    {"レンダーパス追加",      fn_render_add_pass,      1, 1},
    {"レンダーコンパイル",    fn_render_compile,       0, 0},
    {"レンダー実行",          fn_render_execute,       0, 0},
    {"レンダークリア",        fn_render_clear,         0, 0},

    // ── ログ ────────────────────────────────────────────
    {"ログ情報",              fn_log_info,             1, 1},
    {"ログ警告",              fn_log_warn,             1, 1},
    {"ログエラー",            fn_log_error,            1, 1},
};

static const int ENGINE_FUNC_COUNT = sizeof(engine_functions) / sizeof(engine_functions[0]);

// =============================================================================
// プラグインエクスポート
// =============================================================================

extern "C" {

HAJIMU_PLUGIN_EXPORT HajimuPluginInfo* hajimu_plugin_init(void) {
    static HajimuPluginInfo info = {
        .name         = "engine_core",
        .version      = "1.1.0",
        .author       = "はじむ開発チーム",
        .description  = "はじむ Engine Core — ゲームエンジンコアパッケージ (ECS+スクリプトコンポーネント, Scene, Physics, Render, Input, VFS)",
        .functions    = engine_functions,
        .function_count = ENGINE_FUNC_COUNT,
    };
    return &info;
}

HAJIMU_PLUGIN_EXPORT void hajimu_plugin_set_runtime(HajimuRuntime* rt) {
    __hajimu_runtime = rt;
}

} // extern "C"
