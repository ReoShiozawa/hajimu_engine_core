/**
 * engine/core/task_graph.hpp — Work-stealing タスクシステム
 *
 * JobSystem: ワーカースレッド群 + lock-free キュー
 * TaskGraph: 依存関係のあるジョブのDAG実行
 */
#pragma once

#include "types.hpp"
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <memory>

namespace engine {

// ── ジョブ ──────────────────────────────────────────────
using JobFunc = std::function<void()>;

struct Job {
    JobFunc              func;
    std::atomic<i32>     unfinished_deps{0};
    std::vector<Job*>    dependents;        // このジョブ完了後に発火
    std::atomic<bool>    completed{false};
};

// ── ジョブシステム (Work-Stealing) ──────────────────────
class JobSystem {
public:
    explicit JobSystem(u32 worker_count = 0);
    ~JobSystem();

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    /// ジョブをキューに投入
    void submit(Job* job);

    /// ジョブの完了を待つ
    void wait(Job* job);

    /// 全ジョブ完了を待つ
    void wait_all();

    /// ワーカー数
    [[nodiscard]] u32 worker_count() const { return static_cast<u32>(workers_.size()); }

private:
    void worker_loop(u32 id);
    Job* steal();

    std::vector<std::thread>   workers_;
    std::vector<std::deque<Job*>> queues_;  // 各ワーカーのローカルキュー
    std::mutex                  mutex_;
    std::condition_variable     cv_;
    std::atomic<bool>           shutdown_{false};
    std::atomic<u32>            active_jobs_{0};
};

// ── タスクグラフ (DAG) ──────────────────────────────────
class TaskGraph {
public:
    explicit TaskGraph(JobSystem& js) : js_(js) {}

    /// ノード追加 (名前 + 関数)
    Job* add(std::string_view name, JobFunc func);

    /// 依存関係: before → after (before が完了してから after 実行)
    void depends_on(Job* after, Job* before);

    /// グラフ実行 (全ノード完了まで待機)
    void execute();

    /// リセット
    void clear();

private:
    JobSystem&               js_;
    std::vector<std::unique_ptr<Job>> jobs_;
    std::vector<Job*>        roots_;   // 依存なしのルートジョブ
};

// ── グローバルJobSystem ─────────────────────────────────
JobSystem& global_job_system();

} // namespace engine
