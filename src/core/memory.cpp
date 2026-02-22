/**
 * src/core/memory.cpp — メモリアロケータ実装
 */
#include <engine/core/memory.hpp>
#include <engine/core/log.hpp>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <new>

namespace engine {

// ── ArenaAllocator ──────────────────────────────────────

ArenaAllocator::ArenaAllocator(usize capacity)
    : capacity_(capacity)
{
    buffer_ = static_cast<u8*>(std::malloc(capacity));
    if (!buffer_) throw std::bad_alloc{};
}

ArenaAllocator::~ArenaAllocator() {
    if (buffer_) std::free(buffer_);
}

ArenaAllocator::ArenaAllocator(ArenaAllocator&& o) noexcept
    : buffer_(o.buffer_), capacity_(o.capacity_), offset_(o.offset_)
{
    // atomic は個別に移動  (コピー不可のため)
    stats_.total_allocated.store(o.stats_.total_allocated.load(), std::memory_order_relaxed);
    stats_.total_freed.store(o.stats_.total_freed.load(), std::memory_order_relaxed);
    stats_.current_usage.store(o.stats_.current_usage.load(), std::memory_order_relaxed);
    stats_.peak_usage.store(o.stats_.peak_usage.load(), std::memory_order_relaxed);
    stats_.alloc_count.store(o.stats_.alloc_count.load(), std::memory_order_relaxed);
    stats_.free_count.store(o.stats_.free_count.load(), std::memory_order_relaxed);
    o.buffer_ = nullptr; o.offset_ = 0; o.capacity_ = 0;
}

ArenaAllocator& ArenaAllocator::operator=(ArenaAllocator&& o) noexcept {
    if (this != &o) {
        if (buffer_) std::free(buffer_);
        buffer_ = o.buffer_; offset_ = o.offset_; capacity_ = o.capacity_;
        stats_.total_allocated.store(o.stats_.total_allocated.load(), std::memory_order_relaxed);
        stats_.total_freed.store(o.stats_.total_freed.load(), std::memory_order_relaxed);
        stats_.current_usage.store(o.stats_.current_usage.load(), std::memory_order_relaxed);
        stats_.peak_usage.store(o.stats_.peak_usage.load(), std::memory_order_relaxed);
        stats_.alloc_count.store(o.stats_.alloc_count.load(), std::memory_order_relaxed);
        stats_.free_count.store(o.stats_.free_count.load(), std::memory_order_relaxed);
        o.buffer_ = nullptr; o.offset_ = 0; o.capacity_ = 0;
    }
    return *this;
}

void* ArenaAllocator::allocate(usize size, usize alignment) {
    usize aligned = (offset_ + alignment - 1) & ~(alignment - 1);
    if (aligned + size > capacity_) return nullptr;
    void* ptr = buffer_ + aligned;
    offset_ = aligned + size;
    stats_.record_alloc(static_cast<u64>(size));
    return ptr;
}

void ArenaAllocator::reset() {
    stats_.record_free(static_cast<u64>(offset_));
    offset_ = 0;
}

// ── FrameAllocator ──────────────────────────────────────

FrameAllocator::FrameAllocator(usize capacity) : arena_(capacity) {}

void* FrameAllocator::allocate(usize size, usize alignment) {
    return arena_.allocate(size, alignment);
}

void FrameAllocator::clear() { arena_.reset(); }

// ── PoolAllocator ───────────────────────────────────────

PoolAllocator::PoolAllocator(usize block_size, u32 block_count)
    : block_size_(block_size < sizeof(void*) ? sizeof(void*) : block_size),
      block_count_(block_count)
{
    buffer_ = static_cast<u8*>(std::malloc(block_size_ * block_count_));
    if (!buffer_) throw std::bad_alloc{};
    free_head_ = buffer_;
    for (u32 i = 0; i < block_count_ - 1; ++i) {
        u8* cur = buffer_ + i * block_size_;
        u8* nxt = buffer_ + (i + 1) * block_size_;
        *reinterpret_cast<void**>(cur) = nxt;
    }
    *reinterpret_cast<void**>(buffer_ + (block_count_ - 1) * block_size_) = nullptr;
}

PoolAllocator::~PoolAllocator() {
    if (buffer_) std::free(buffer_);
}

PoolAllocator::PoolAllocator(PoolAllocator&& o) noexcept
    : buffer_(o.buffer_), free_head_(o.free_head_),
      block_size_(o.block_size_), block_count_(o.block_count_)
{
    stats_.total_allocated.store(o.stats_.total_allocated.load(), std::memory_order_relaxed);
    stats_.total_freed.store(o.stats_.total_freed.load(), std::memory_order_relaxed);
    stats_.current_usage.store(o.stats_.current_usage.load(), std::memory_order_relaxed);
    stats_.peak_usage.store(o.stats_.peak_usage.load(), std::memory_order_relaxed);
    stats_.alloc_count.store(o.stats_.alloc_count.load(), std::memory_order_relaxed);
    stats_.free_count.store(o.stats_.free_count.load(), std::memory_order_relaxed);
    o.buffer_ = nullptr; o.free_head_ = nullptr;
}

PoolAllocator& PoolAllocator::operator=(PoolAllocator&& o) noexcept {
    if (this != &o) {
        if (buffer_) std::free(buffer_);
        buffer_ = o.buffer_; free_head_ = o.free_head_;
        block_size_ = o.block_size_; block_count_ = o.block_count_;
        stats_.total_allocated.store(o.stats_.total_allocated.load(), std::memory_order_relaxed);
        stats_.total_freed.store(o.stats_.total_freed.load(), std::memory_order_relaxed);
        stats_.current_usage.store(o.stats_.current_usage.load(), std::memory_order_relaxed);
        stats_.peak_usage.store(o.stats_.peak_usage.load(), std::memory_order_relaxed);
        stats_.alloc_count.store(o.stats_.alloc_count.load(), std::memory_order_relaxed);
        stats_.free_count.store(o.stats_.free_count.load(), std::memory_order_relaxed);
        o.buffer_ = nullptr; o.free_head_ = nullptr;
    }
    return *this;
}

void* PoolAllocator::alloc() {
    if (!free_head_) return nullptr;
    void* ptr = free_head_;
    free_head_ = *reinterpret_cast<void**>(free_head_);
    stats_.record_alloc(static_cast<u64>(block_size_));
    return ptr;
}

void PoolAllocator::free(void* ptr) {
    if (!ptr) return;
    *reinterpret_cast<void**>(ptr) = free_head_;
    free_head_ = ptr;
    stats_.record_free(static_cast<u64>(block_size_));
}

// ── LinearAllocator ─────────────────────────────────────

LinearAllocator::LinearAllocator(usize capacity)
    : capacity_(capacity)
{
    buffer_ = static_cast<u8*>(std::malloc(capacity));
    if (!buffer_) throw std::bad_alloc{};
}

LinearAllocator::~LinearAllocator() {
    if (buffer_) std::free(buffer_);
}

LinearAllocator::LinearAllocator(LinearAllocator&& o) noexcept
    : buffer_(o.buffer_), capacity_(o.capacity_), offset_(o.offset_)
{
    stats_.total_allocated.store(o.stats_.total_allocated.load(), std::memory_order_relaxed);
    stats_.total_freed.store(o.stats_.total_freed.load(), std::memory_order_relaxed);
    stats_.current_usage.store(o.stats_.current_usage.load(), std::memory_order_relaxed);
    stats_.peak_usage.store(o.stats_.peak_usage.load(), std::memory_order_relaxed);
    stats_.alloc_count.store(o.stats_.alloc_count.load(), std::memory_order_relaxed);
    stats_.free_count.store(o.stats_.free_count.load(), std::memory_order_relaxed);
    o.buffer_ = nullptr; o.offset_ = 0; o.capacity_ = 0;
}

LinearAllocator& LinearAllocator::operator=(LinearAllocator&& o) noexcept {
    if (this != &o) {
        if (buffer_) std::free(buffer_);
        buffer_ = o.buffer_; offset_ = o.offset_; capacity_ = o.capacity_;
        stats_.total_allocated.store(o.stats_.total_allocated.load(), std::memory_order_relaxed);
        stats_.total_freed.store(o.stats_.total_freed.load(), std::memory_order_relaxed);
        stats_.current_usage.store(o.stats_.current_usage.load(), std::memory_order_relaxed);
        stats_.peak_usage.store(o.stats_.peak_usage.load(), std::memory_order_relaxed);
        stats_.alloc_count.store(o.stats_.alloc_count.load(), std::memory_order_relaxed);
        stats_.free_count.store(o.stats_.free_count.load(), std::memory_order_relaxed);
        o.buffer_ = nullptr; o.offset_ = 0; o.capacity_ = 0;
    }
    return *this;
}

void* LinearAllocator::allocate(usize size, usize alignment) {
    usize aligned = (offset_ + alignment - 1) & ~(alignment - 1);
    if (aligned + size > capacity_) return nullptr;
    void* ptr = buffer_ + aligned;
    offset_ = aligned + size;
    stats_.record_alloc(static_cast<u64>(size));
    return ptr;
}

void LinearAllocator::reset() {
    stats_.record_free(static_cast<u64>(offset_));
    offset_ = 0;
}

// ── メモリスナップショット ──────────────────────────────

MemorySnapshot take_memory_snapshot() {
    return MemorySnapshot{};
}

void report_memory_leaks(const MemorySnapshot& snap) {
    if (snap.current_usage > 0) {
        ENG_WARN("Memory leak detected: %llu bytes (%llu allocs - %llu frees)",
                 snap.current_usage, snap.alloc_count, snap.free_count);
    }
}

} // namespace engine
