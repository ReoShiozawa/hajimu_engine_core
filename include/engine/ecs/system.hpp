/**
 * engine/ecs/system.hpp — システム (クエリベース並列実行)
 *
 * System: 名前付き関数 + クエリ条件
 * Scheduler: 依存関係に基づくシステム実行順序決定
 * Reactive System: 変更検知トリガー
 */
#pragma once

#include "entity.hpp"
#include "query.hpp"
#include <engine/core/types.hpp>
#include <string>
#include <vector>
#include <functional>

namespace engine::ecs {

class World;

// ── システム定義 ────────────────────────────────────────
struct SystemDesc {
    std::string          name;
    std::vector<TypeID>  reads;       // 読み取りコンポーネント
    std::vector<TypeID>  writes;      // 書き込みコンポーネント
    std::vector<std::string> run_after;  // 依存先システム名
    std::function<void(World&)> execute;
};

// ── リアクティブトリガー ────────────────────────────────
enum class TriggerEvent : u8 {
    OnAdd,      // コンポーネント追加時
    OnChange,   // コンポーネント変更時
    OnRemove,   // コンポーネント削除時
};

struct ReactiveTrigger {
    std::string name;
    TypeID      component;
    TriggerEvent event;
    std::function<void(World&, Entity)> handler;
};

// ── システムスケジューラ ────────────────────────────────
class SystemScheduler {
public:
    explicit SystemScheduler(World& world) : world_(world) {}

    /// システム登録
    void add_system(SystemDesc desc);

    /// リアクティブトリガー登録
    void add_trigger(ReactiveTrigger trigger);

    /// 全システムを依存順に実行
    void run();

    /// 全システム名一覧
    [[nodiscard]] std::vector<std::string> system_names() const;

private:
    World&                          world_;
    std::vector<SystemDesc>         systems_;
    std::vector<ReactiveTrigger>    triggers_;
};

} // namespace engine::ecs
