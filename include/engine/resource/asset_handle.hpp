/**
 * engine/resource/asset_handle.hpp — 参照カウント付きアセットハンドル
 */
#pragma once

#include <engine/core/types.hpp>
#include <atomic>
#include <string>

namespace engine::resource {

// ── アセット状態 ────────────────────────────────────────
enum class AssetState : u8 {
    Unloaded, Loading, Loaded, Failed
};

// ── AssetHandle ─────────────────────────────────────────
template <typename T>
class AssetHandle {
public:
    AssetHandle() = default;
    explicit AssetHandle(u64 id) : id_(id) {}

    [[nodiscard]] u64  id() const    { return id_; }
    [[nodiscard]] bool valid() const { return id_ != 0; }

    bool operator==(const AssetHandle& o) const { return id_ == o.id_; }
    bool operator!=(const AssetHandle& o) const { return id_ != o.id_; }

    static AssetHandle null() { return AssetHandle{0}; }

private:
    u64 id_ = 0;
};

// ── AssetEntry (内部管理用) ─────────────────────────────
struct AssetEntry {
    std::string        path;
    TypeID             type_id;
    AssetState         state = AssetState::Unloaded;
    u32                ref_count = 0;  // mutex 保護下で使用
    void*              data = nullptr;    // 型消去されたアセットデータ
    usize              data_size = 0;
};

} // namespace engine::resource
