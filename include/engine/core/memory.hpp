/**
 * engine/core/memory.hpp — 用途別アロケータ
 *
 * Arena / Frame / Pool / Linear の4種アロケータ
 * メモリリーク検出・スナップショット機能内蔵
 */
#pragma once

#include "types.hpp"
#include <cstdlib>
#include <new>
#include <mutex>
#include <atomic>

namespace engine {

// ── 統計情報 ────────────────────────────────────────────
struct AllocStats {
    std::atomic<u64> total_allocated{0};
    std::atomic<u64> total_freed{0};
    std::atomic<u64> current_usage{0};
    std::atomic<u64> peak_usage{0};
    std::atomic<u32> alloc_count{0};
    std::atomic<u32> free_count{0};

    void record_alloc(u64 size) {
        total_allocated += size;
        current_usage += size;
        alloc_count++;
        u64 cur = current_usage.load();
        u64 peak = peak_usage.load();
        while (cur > peak && !peak_usage.compare_exchange_weak(peak, cur)) {}
    }

    void record_free(u64 size) {
        total_freed += size;
        current_usage -= size;
        free_count++;
    }

    [[nodiscard]] bool has_leak() const {
        return current_usage.load() > 0;
    }
};

// ── ArenaAllocator (バンプ / 一括解放) ──────────────────
class ArenaAllocator {
public:
    explicit ArenaAllocator(usize capacity);
    ~ArenaAllocator();

    ArenaAllocator(ArenaAllocator&& o) noexcept;
    ArenaAllocator& operator=(ArenaAllocator&& o) noexcept;
    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    [[nodiscard]] void* allocate(usize size, usize alignment = 16);
    void  reset();      // 全解放 (ポインタ先頭に戻す)

    [[nodiscard]] usize used() const { return offset_; }
    [[nodiscard]] usize capacity() const { return capacity_; }
    [[nodiscard]] const AllocStats& stats() const { return stats_; }

private:
    u8*    buffer_   = nullptr;
    usize  capacity_ = 0;
    usize  offset_   = 0;
    AllocStats stats_;
};

// ── FrameAllocator (毎フレーム clear) ──────────────────
class FrameAllocator {
public:
    explicit FrameAllocator(usize capacity);
    ~FrameAllocator() = default;

    [[nodiscard]] void* allocate(usize size, usize alignment = 16);
    void  clear();      // フレーム末尾で呼び出す

    [[nodiscard]] usize used() const { return arena_.used(); }

private:
    ArenaAllocator arena_;
};

// ── PoolAllocator (固定サイズブロック) ──────────────────
class PoolAllocator {
public:
    PoolAllocator(usize block_size, u32 block_count);
    ~PoolAllocator();

    PoolAllocator(PoolAllocator&& o) noexcept;
    PoolAllocator& operator=(PoolAllocator&& o) noexcept;
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    [[nodiscard]] void* alloc();
    void  free(void* ptr);

    [[nodiscard]] u32 total_count() const { return block_count_; }
    [[nodiscard]] const AllocStats& stats() const { return stats_; }

private:
    u8*   buffer_      = nullptr;
    void* free_head_   = nullptr;
    usize block_size_  = 0;
    u32   block_count_ = 0;
    AllocStats stats_;
};

// ── LinearAllocator (順次書き込み専用) ──────────────────
class LinearAllocator {
public:
    explicit LinearAllocator(usize capacity);
    ~LinearAllocator();

    LinearAllocator(LinearAllocator&& o) noexcept;
    LinearAllocator& operator=(LinearAllocator&& o) noexcept;
    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    [[nodiscard]] void* allocate(usize size, usize alignment = 8);
    void  reset();

    [[nodiscard]] usize used() const { return offset_; }

private:
    u8*   buffer_   = nullptr;
    usize capacity_ = 0;
    usize offset_   = 0;
    AllocStats stats_;
};

// ── メモリスナップショット (リーク検出) ────────────────
struct MemorySnapshot {
    u64 current_usage = 0;
    u64 alloc_count   = 0;
    u64 free_count    = 0;
};

MemorySnapshot take_memory_snapshot();
void           report_memory_leaks(const MemorySnapshot& snap);

} // namespace engine
