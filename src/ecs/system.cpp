/**
 * src/ecs/system.cpp — システムスケジューラ実装
 */
#include <engine/ecs/system.hpp>
#include <engine/ecs/world.hpp>
#include <engine/core/log.hpp>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <queue>

namespace engine::ecs {

void SystemScheduler::add_system(SystemDesc desc) {
    ENG_DEBUG("System registered: '%s'", desc.name.c_str());
    systems_.push_back(std::move(desc));
}

void SystemScheduler::add_trigger(ReactiveTrigger trigger) {
    ENG_DEBUG("Reactive trigger registered: '%s' on %016llx",
              trigger.name.c_str(), static_cast<unsigned long long>(trigger.component));
    triggers_.push_back(std::move(trigger));
}

void SystemScheduler::run() {
    // トポロジカルソート (依存順)
    std::unordered_map<std::string, u32> name_to_idx;
    for (u32 i = 0; i < systems_.size(); ++i) {
        name_to_idx[systems_[i].name] = i;
    }

    // 入次数計算
    std::vector<u32> in_degree(systems_.size(), 0);
    std::vector<std::vector<u32>> adj(systems_.size());
    for (u32 i = 0; i < systems_.size(); ++i) {
        for (auto& dep : systems_[i].run_after) {
            auto it = name_to_idx.find(dep);
            if (it != name_to_idx.end()) {
                adj[it->second].push_back(i);
                in_degree[i]++;
            }
        }
    }

    // BFS (Kahn のアルゴリズム)
    std::queue<u32> q;
    for (u32 i = 0; i < systems_.size(); ++i) {
        if (in_degree[i] == 0) q.push(i);
    }

    std::vector<u32> order;
    order.reserve(systems_.size());
    while (!q.empty()) {
        u32 cur = q.front(); q.pop();
        order.push_back(cur);
        for (u32 next : adj[cur]) {
            if (--in_degree[next] == 0) q.push(next);
        }
    }

    // 実行
    for (u32 idx : order) {
        systems_[idx].execute(world_);
    }
}

std::vector<std::string> SystemScheduler::system_names() const {
    std::vector<std::string> names;
    names.reserve(systems_.size());
    for (auto& s : systems_) names.push_back(s.name);
    return names;
}

} // namespace engine::ecs
