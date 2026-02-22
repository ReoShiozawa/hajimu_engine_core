/**
 * src/scene/scene_graph.cpp — シーングラフ実装
 */
#include <engine/scene/scene_graph.hpp>
#include <engine/core/log.hpp>
#include <algorithm>

namespace engine::scene {

Entity SceneGraph::add_node(Entity entity, const std::string& name, Entity parent) {
    SceneNode node;
    node.entity = entity;
    node.name = name;
    node.parent = parent;

    if (parent.valid()) {
        auto it = nodes_.find(parent.id);
        if (it != nodes_.end()) {
            it->second.children.push_back(entity);
        }
    } else {
        roots_.push_back(entity);
    }

    nodes_[entity.id] = std::move(node);
    return entity;
}

void SceneGraph::reparent(Entity entity, Entity new_parent) {
    auto it = nodes_.find(entity.id);
    if (it == nodes_.end()) return;
    auto& node = it->second;

    // 旧親から除去
    if (node.parent.valid()) {
        auto pit = nodes_.find(node.parent.id);
        if (pit != nodes_.end()) {
            auto& children = pit->second.children;
            children.erase(std::remove_if(children.begin(), children.end(),
                [&](Entity e) { return e == entity; }), children.end());
        }
    } else {
        roots_.erase(std::remove_if(roots_.begin(), roots_.end(),
            [&](Entity e) { return e == entity; }), roots_.end());
    }

    // 新親に追加
    node.parent = new_parent;
    if (new_parent.valid()) {
        auto nit = nodes_.find(new_parent.id);
        if (nit != nodes_.end()) {
            nit->second.children.push_back(entity);
        }
    } else {
        roots_.push_back(entity);
    }
}

void SceneGraph::remove_node(Entity entity) {
    auto it = nodes_.find(entity.id);
    if (it == nodes_.end()) return;

    // 子を再帰的に削除
    auto children_copy = it->second.children;
    for (auto child : children_copy) {
        remove_node(child);
    }

    // 親から除去
    if (it->second.parent.valid()) {
        auto pit = nodes_.find(it->second.parent.id);
        if (pit != nodes_.end()) {
            auto& children = pit->second.children;
            children.erase(std::remove_if(children.begin(), children.end(),
                [&](Entity e) { return e == entity; }), children.end());
        }
    } else {
        roots_.erase(std::remove_if(roots_.begin(), roots_.end(),
            [&](Entity e) { return e == entity; }), roots_.end());
    }

    nodes_.erase(it);
}

SceneNode* SceneGraph::find(Entity entity) {
    auto it = nodes_.find(entity.id);
    return it != nodes_.end() ? &it->second : nullptr;
}

const SceneNode* SceneGraph::find(Entity entity) const {
    auto it = nodes_.find(entity.id);
    return it != nodes_.end() ? &it->second : nullptr;
}

Entity SceneGraph::find_by_name(const std::string& name) const {
    for (auto& [_, node] : nodes_) {
        if (node.name == name) return node.entity;
    }
    return Entity::null();
}

void SceneGraph::traverse(Entity root, std::function<void(Entity, u32)> visitor) const {
    struct Frame { Entity entity; u32 depth; };
    std::vector<Frame> stack;
    stack.push_back({root, 0});

    while (!stack.empty()) {
        auto [entity, depth] = stack.back();
        stack.pop_back();

        visitor(entity, depth);

        auto it = nodes_.find(entity.id);
        if (it != nodes_.end()) {
            for (auto& child : it->second.children) {
                stack.push_back({child, depth + 1});
            }
        }
    }
}

} // namespace engine::scene
