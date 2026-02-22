/**
 * engine/ecs/query.hpp — 型安全なクエリ
 *
 * Query<Position, Velocity> で必要コンポーネントを指定し、
 * 該当する全 Archetype を横断して反復。
 */
#pragma once

#include "entity.hpp"
#include "archetype.hpp"
#include <functional>

namespace engine::ecs {

class World; // forward declaration

// ── QueryResult: 1つの Archetype 分のマッチ結果 ────────
struct QueryMatch {
    Archetype*            archetype;
    u32                   count;
    std::vector<void*>    columns;  // 要求コンポーネントのカラムポインタ
};

// ── QueryBuilder ────────────────────────────────────────
class QueryBuilder {
public:
    explicit QueryBuilder(World& world) : world_(world) {}

    /// 必要コンポーネントを追加 (可変長)
    template <Component... Ts>
    QueryBuilder& with() {
        (required_.push_back(type_id<Ts>()), ...);
        return *this;
    }

    /// 除外コンポーネント
    template <Component... Ts>
    QueryBuilder& without() {
        (excluded_.push_back(type_id<Ts>()), ...);
        return *this;
    }

    /// クエリ実行 — マッチする全Archetypeを返す
    [[nodiscard]] std::vector<QueryMatch> execute() const;

    /// for_each: 各Entity の全コンポーネントに対してコールバック
    template <Component... Ts>
    void for_each(std::function<void(Entity, Ts&...)> func) const;

private:
    World&             world_;
    std::vector<TypeID> required_;
    std::vector<TypeID> excluded_;
};

// ── テンプレート実装は world.hpp インクルード後に定義 ────

} // namespace engine::ecs
