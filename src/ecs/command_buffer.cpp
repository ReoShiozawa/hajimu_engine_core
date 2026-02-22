/**
 * src/ecs/command_buffer.cpp — コマンドバッファ実装
 */
#include <engine/ecs/command_buffer.hpp>
#include <engine/ecs/world.hpp>
#include <engine/core/log.hpp>
#include <cstring>

namespace engine::ecs {

Entity CommandBuffer::spawn() {
    Command cmd;
    cmd.type = CommandType::Spawn;
    // 一時 Entity (apply 時に正式 ID が割り当てられる)
    cmd.entity = Entity{next_temp_index_++, 0};
    push(cmd);
    return cmd.entity;
}

void CommandBuffer::despawn(Entity entity) {
    Command cmd;
    cmd.type = CommandType::Despawn;
    cmd.entity = entity;
    push(cmd);
}

void CommandBuffer::apply(World& world) {
    std::lock_guard lock(mutex_);

    // 一時 Entity → 正式 Entity のマッピング
    std::unordered_map<u64, Entity> temp_to_real;

    for (auto& cmd : commands_) {
        // 一時 Entity を正式 Entity に変換
        Entity entity = cmd.entity;
        auto it = temp_to_real.find(entity.id);
        if (it != temp_to_real.end()) {
            entity = it->second;
        }

        switch (cmd.type) {
            case CommandType::Spawn: {
                Entity real = world.spawn();
                temp_to_real[cmd.entity.id] = real;
                break;
            }
            case CommandType::Despawn:
                world.despawn(entity);
                break;

            case CommandType::AddComponent:
                world.add_component_raw_public(entity, cmd.comp_id, cmd.comp_size, 8, cmd.data);
                break;

            case CommandType::RemoveComponent:
                world.remove_component_raw_public(entity, cmd.comp_id);
                break;

            case CommandType::SetComponent: {
                // get_component_raw + memcpy
                void* ptr = world.get_component_raw_public(entity, cmd.comp_id);
                if (ptr) std::memcpy(ptr, cmd.data, cmd.comp_size);
                break;
            }
        }
    }
    commands_.clear();
}

void CommandBuffer::clear() {
    std::lock_guard lock(mutex_);
    commands_.clear();
}

void CommandBuffer::push(const Command& cmd) {
    std::lock_guard lock(mutex_);
    commands_.push_back(cmd);
}

} // namespace engine::ecs
