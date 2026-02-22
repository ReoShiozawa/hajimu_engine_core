/**
 * engine/engine.hpp — メインアンブレラインクルード
 *
 * #include <engine/engine.hpp> で全モジュールにアクセス可能。
 */
#pragma once

// ── Core ────────────────────────────────────────────────
#include <engine/core/types.hpp>
#include <engine/core/memory.hpp>
#include <engine/core/log.hpp>
#include <engine/core/reflection.hpp>
#include <engine/core/task_graph.hpp>

// ── ECS ─────────────────────────────────────────────────
#include <engine/ecs/entity.hpp>
#include <engine/ecs/component.hpp>
#include <engine/ecs/archetype.hpp>
#include <engine/ecs/world.hpp>
#include <engine/ecs/system.hpp>
#include <engine/ecs/query.hpp>
#include <engine/ecs/command_buffer.hpp>

// ── Input ───────────────────────────────────────────────
#include <engine/input/input_system.hpp>
#include <engine/input/action_map.hpp>

// ── Scene ───────────────────────────────────────────────
#include <engine/scene/scene_graph.hpp>
#include <engine/scene/transform.hpp>
#include <engine/scene/prefab.hpp>

// ── Resource ────────────────────────────────────────────
#include <engine/resource/vfs.hpp>
#include <engine/resource/asset_handle.hpp>
#include <engine/resource/resource_manager.hpp>

// ── Render ──────────────────────────────────────────────
#include <engine/render/render_graph.hpp>
#include <engine/render/render_backend.hpp>
#include <engine/render/shader_compiler.hpp>

// ── Physics ─────────────────────────────────────────────
#include <engine/physics/physics_world.hpp>

// ── Audio ───────────────────────────────────────────────
#include <engine/audio/audio_system.hpp>

// ── Network ─────────────────────────────────────────────
#include <engine/network/net_system.hpp>

// ── Script ──────────────────────────────────────────────
#include <engine/script/script_runtime.hpp>

// ── Build ───────────────────────────────────────────────
#include <engine/build/build_pipeline.hpp>

namespace engine {
    constexpr const char* VERSION     = "1.0.0";
    constexpr const char* ENGINE_NAME = "はじむ Engine Core";
}
