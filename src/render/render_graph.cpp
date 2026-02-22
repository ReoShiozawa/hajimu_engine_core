/**
 * src/render/render_graph.cpp — RenderGraph 実装
 */
#include <engine/render/render_graph.hpp>
#include <engine/core/log.hpp>
#include <algorithm>
#include <queue>
#include <unordered_set>

namespace engine::render {

void RenderGraph::declare_resource(const RenderResourceDesc& desc) {
    resource_map_[desc.name] = static_cast<u32>(resources_.size());
    resources_.push_back(desc);
}

void RenderGraph::add_pass(RenderPass pass) {
    passes_.push_back(std::move(pass));
}

Result<void> RenderGraph::compile() {
    u32 n = static_cast<u32>(passes_.size());
    execution_order_.clear();

    // パス名 → インデックスマップ
    std::unordered_map<std::string, u32> resource_producer;
    for (u32 i = 0; i < n; ++i) {
        for (auto& out : passes_[i].outputs) {
            resource_producer[out] = i;
        }
    }

    // 依存グラフ構築
    std::vector<std::vector<u32>> adj(n);
    std::vector<u32> in_degree(n, 0);
    for (u32 i = 0; i < n; ++i) {
        for (auto& inp : passes_[i].inputs) {
            auto it = resource_producer.find(inp);
            if (it != resource_producer.end() && it->second != i) {
                adj[it->second].push_back(i);
                in_degree[i]++;
            }
        }
    }

    // トポロジカルソート
    std::queue<u32> q;
    for (u32 i = 0; i < n; ++i) {
        if (in_degree[i] == 0) q.push(i);
    }

    while (!q.empty()) {
        u32 cur = q.front(); q.pop();
        execution_order_.push_back(cur);
        for (u32 next : adj[cur]) {
            if (--in_degree[next] == 0) q.push(next);
        }
    }

    if (execution_order_.size() != n) {
        ENG_ERROR("RenderGraph: cycle detected in pass dependencies");
        return std::unexpected(Error::InvalidState);
    }

    return {};
}

void RenderGraph::execute() {
    if (execution_order_.empty()) {
        // 未コンパイルなら順序通り
        for (auto& pass : passes_) {
            if (pass.execute) pass.execute();
        }
        return;
    }
    for (u32 idx : execution_order_) {
        if (passes_[idx].execute) passes_[idx].execute();
    }
}

void RenderGraph::clear() {
    passes_.clear();
    resources_.clear();
    execution_order_.clear();
    resource_map_.clear();
}

} // namespace engine::render
