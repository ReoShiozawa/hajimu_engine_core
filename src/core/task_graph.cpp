/**
 * src/core/task_graph.cpp — ジョブシステム + タスクグラフ実装
 */
#include <engine/core/task_graph.hpp>
#include <engine/core/log.hpp>
#include <cassert>
#include <algorithm>

namespace engine {

// ── JobSystem ───────────────────────────────────────────

JobSystem::JobSystem(u32 worker_count) {
    if (worker_count == 0) {
        worker_count = std::max(1u, std::thread::hardware_concurrency() - 1);
    }
    queues_.resize(worker_count);
    workers_.reserve(worker_count);
    for (u32 i = 0; i < worker_count; ++i) {
        workers_.emplace_back([this, i] { worker_loop(i); });
    }
    ENG_INFO("JobSystem: %u workers started", worker_count);
}

JobSystem::~JobSystem() {
    shutdown_.store(true, std::memory_order_release);
    cv_.notify_all();
    for (auto& w : workers_) {
        if (w.joinable()) w.join();
    }
}

void JobSystem::submit(Job* job) {
    if (!job) return;
    if (job->unfinished_deps.load(std::memory_order_acquire) <= 0) {
        // 即座にキューへ
        {
            std::lock_guard lock(mutex_);
            u32 idx = active_jobs_.fetch_add(1, std::memory_order_relaxed)
                      % static_cast<u32>(queues_.size());
            queues_[idx].push_back(job);
        }
        cv_.notify_one();
    }
    // deps > 0 の場合は、依存完了時に dependents 経由で投入される
}

void JobSystem::wait(Job* job) {
    while (!job->completed.load(std::memory_order_acquire)) {
        Job* stolen = steal();
        if (stolen) {
            if (stolen->func) stolen->func();
            stolen->completed.store(true, std::memory_order_release);
            // 依存先に通知
            for (auto* dep : stolen->dependents) {
                auto prev = dep->unfinished_deps.fetch_sub(1, std::memory_order_acq_rel);
                if (prev == 1) submit(dep);
            }
        } else {
            std::this_thread::yield();
        }
    }
}

void JobSystem::wait_all() {
    // 全キューが空になるまで手伝う
    while (true) {
        bool all_empty = true;
        {
            std::lock_guard lock(mutex_);
            for (auto& q : queues_) {
                if (!q.empty()) { all_empty = false; break; }
            }
        }
        if (all_empty) break;
        Job* stolen = steal();
        if (stolen) {
            if (stolen->func) stolen->func();
            stolen->completed.store(true, std::memory_order_release);
            for (auto* dep : stolen->dependents) {
                auto prev = dep->unfinished_deps.fetch_sub(1, std::memory_order_acq_rel);
                if (prev == 1) submit(dep);
            }
        } else {
            std::this_thread::yield();
        }
    }
}

Job* JobSystem::steal() {
    std::lock_guard lock(mutex_);
    for (auto& q : queues_) {
        if (!q.empty()) {
            Job* job = q.front();
            q.pop_front();
            return job;
        }
    }
    return nullptr;
}

void JobSystem::worker_loop(u32 id) {
    while (!shutdown_.load(std::memory_order_acquire)) {
        Job* job = nullptr;
        {
            std::lock_guard lock(mutex_);
            if (id < queues_.size() && !queues_[id].empty()) {
                job = queues_[id].front();
                queues_[id].pop_front();
            }
        }
        if (!job) job = steal();
        if (job) {
            if (job->func) job->func();
            job->completed.store(true, std::memory_order_release);
            for (auto* dep : job->dependents) {
                auto prev = dep->unfinished_deps.fetch_sub(1, std::memory_order_acq_rel);
                if (prev == 1) submit(dep);
            }
        } else {
            std::unique_lock lock(mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(1));
        }
    }
}

JobSystem& global_job_system() {
    static JobSystem sys;
    return sys;
}

// ── TaskGraph ───────────────────────────────────────────

Job* TaskGraph::add(std::string_view /*name*/, JobFunc func) {
    auto job = std::make_unique<Job>();
    job->func = std::move(func);
    Job* ptr = job.get();
    jobs_.push_back(std::move(job));
    roots_.push_back(ptr);
    return ptr;
}

void TaskGraph::depends_on(Job* after, Job* before) {
    if (!after || !before) return;
    after->unfinished_deps.fetch_add(1, std::memory_order_relaxed);
    before->dependents.push_back(after);
    // after はもはや root ではない
    auto it = std::find(roots_.begin(), roots_.end(), after);
    if (it != roots_.end()) roots_.erase(it);
}

void TaskGraph::execute() {
    // ルートジョブを submit
    for (auto* root : roots_) {
        js_.submit(root);
    }
    // 全ジョブの完了を待機
    for (auto& job : jobs_) {
        js_.wait(job.get());
    }
}

void TaskGraph::clear() {
    jobs_.clear();
    roots_.clear();
}

} // namespace engine
