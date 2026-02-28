/**
 * src/ecs/archetype.cpp — Archetype テーブル + ComponentColumn 実装
 */
#include <engine/ecs/archetype.hpp>
#include <engine/core/log.hpp>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <numeric>

#ifdef _WIN32
#include <malloc.h>
static inline void* engine_aligned_alloc(size_t align, size_t size) {
    return _aligned_malloc(size, align);
}
static inline void engine_aligned_free(void* ptr) {
    _aligned_free(ptr);
}
#else
static inline void* engine_aligned_alloc(size_t align, size_t size) {
    return std::aligned_alloc(align, size);
}
static inline void engine_aligned_free(void* ptr) {
    std::free(ptr);
}
#endif

namespace engine::ecs {

// ── ComponentColumn ─────────────────────────────────────

ComponentColumn::ComponentColumn(usize elem_size, usize elem_align, u32 capacity)
    : elem_size_(elem_size), elem_align_(elem_align), capacity_(capacity)
{
    if (capacity_ > 0) {
        data_ = static_cast<u8*>(engine_aligned_alloc(elem_align_, elem_size_ * capacity_));
        if (!data_) throw std::bad_alloc{};
    }
}

ComponentColumn::~ComponentColumn() {
    if (data_) engine_aligned_free(data_);
}

ComponentColumn::ComponentColumn(ComponentColumn&& o) noexcept
    : data_(o.data_), elem_size_(o.elem_size_), elem_align_(o.elem_align_),
      count_(o.count_), capacity_(o.capacity_)
{
    o.data_ = nullptr;
    o.count_ = 0;
    o.capacity_ = 0;
}

ComponentColumn& ComponentColumn::operator=(ComponentColumn&& o) noexcept {
    if (this != &o) {
        if (data_) engine_aligned_free(data_);
        data_ = o.data_; elem_size_ = o.elem_size_; elem_align_ = o.elem_align_;
        count_ = o.count_; capacity_ = o.capacity_;
        o.data_ = nullptr; o.count_ = 0; o.capacity_ = 0;
    }
    return *this;
}

void ComponentColumn::push_back(const void* data) {
    if (count_ >= capacity_) grow();
    std::memcpy(data_ + count_ * elem_size_, data, elem_size_);
    ++count_;
}

void* ComponentColumn::at(u32 index) {
    assert(index < count_);
    return data_ + index * elem_size_;
}

const void* ComponentColumn::at(u32 index) const {
    assert(index < count_);
    return data_ + index * elem_size_;
}

void ComponentColumn::swap_remove(u32 index) {
    assert(index < count_);
    if (index < count_ - 1) {
        std::memcpy(data_ + index * elem_size_,
                     data_ + (count_ - 1) * elem_size_,
                     elem_size_);
    }
    --count_;
}

void ComponentColumn::grow() {
    u32 new_cap = capacity_ == 0 ? 64 : capacity_ * 2;
    u8* new_data = static_cast<u8*>(engine_aligned_alloc(elem_align_, elem_size_ * new_cap));
    if (!new_data) throw std::bad_alloc{};
    if (data_ && count_ > 0) {
        std::memcpy(new_data, data_, elem_size_ * count_);
    }
    if (data_) engine_aligned_free(data_);
    data_ = new_data;
    capacity_ = new_cap;
}

// ── Archetype ───────────────────────────────────────────

Archetype::Archetype(std::vector<ComponentInfo> components)
    : components_(std::move(components))
{
    // TypeID でソート (Archetype ID の一意性保証)
    std::sort(components_.begin(), components_.end(),
              [](auto& a, auto& b) { return a.id < b.id; });

    // Archetype ID を計算
    std::vector<TypeID> ids;
    ids.reserve(components_.size());
    for (auto& c : components_) ids.push_back(c.id);
    id_ = compute_archetype_id(ids);

    // カラム作成
    columns_.reserve(components_.size());
    for (u32 i = 0; i < components_.size(); ++i) {
        comp_index_[components_[i].id] = i;
        columns_.emplace_back(components_[i].size, components_[i].alignment);
    }
}

u32 Archetype::add_entity(Entity entity) {
    u32 row = entity_count_;
    entities_.push_back(entity);
    // 各カラムにゼロ初期化データを追加
    for (auto& col : columns_) {
        std::vector<u8> zeros(col.elem_size(), 0);
        col.push_back(zeros.data());
    }
    ++entity_count_;
    return row;
}

void Archetype::remove_entity(u32 row) {
    assert(row < entity_count_);
    // swap-remove: 末尾と入れ替え
    if (row < entity_count_ - 1) {
        entities_[row] = entities_[entity_count_ - 1];
    }
    entities_.pop_back();
    for (auto& col : columns_) {
        col.swap_remove(row);
    }
    --entity_count_;
}

void* Archetype::get_component(u32 row, TypeID comp_id) {
    auto it = comp_index_.find(comp_id);
    if (it == comp_index_.end()) return nullptr;
    return columns_[it->second].at(row);
}

const void* Archetype::get_component(u32 row, TypeID comp_id) const {
    auto it = comp_index_.find(comp_id);
    if (it == comp_index_.end()) return nullptr;
    return columns_[it->second].at(row);
}

void Archetype::set_component(u32 row, TypeID comp_id, const void* data) {
    void* dst = get_component(row, comp_id);
    if (!dst || !data) return;
    auto it = comp_index_.find(comp_id);
    std::memcpy(dst, data, components_[it->second].size);
}

bool Archetype::has_component(TypeID comp_id) const {
    return comp_index_.contains(comp_id);
}

ComponentColumn* Archetype::column(TypeID comp_id) {
    auto it = comp_index_.find(comp_id);
    return it != comp_index_.end() ? &columns_[it->second] : nullptr;
}

const ComponentColumn* Archetype::column(TypeID comp_id) const {
    auto it = comp_index_.find(comp_id);
    return it != comp_index_.end() ? &columns_[it->second] : nullptr;
}

} // namespace engine::ecs
