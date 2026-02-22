/**
 * engine/resource/resource_manager.hpp — 非同期リソースマネージャ
 *
 * VFS + 非同期ロード + 依存グラフ + 参照カウント。
 */
#pragma once

#include "vfs.hpp"
#include "asset_handle.hpp"
#include <engine/core/types.hpp>
#include <functional>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace engine::resource {

// ── ローダーコールバック ────────────────────────────────
using LoaderFn = std::function<Result<void*>(const std::vector<u8>& raw_data)>;
using UnloaderFn = std::function<void(void* data)>;

// ── ResourceManager ─────────────────────────────────────
class ResourceManager {
public:
    explicit ResourceManager(VFS& vfs);
    ~ResourceManager();

    /// ローダー登録 (型ごと)
    void register_loader(TypeID type, LoaderFn loader, UnloaderFn unloader);

    /// アセット読み込み要求 (非同期)
    template <typename T>
    AssetHandle<T> load(const std::string& path) {
        auto id = load_raw(path, type_id<T>());
        return AssetHandle<T>{id};
    }

    /// 同期読み込み
    template <typename T>
    AssetHandle<T> load_sync(const std::string& path) {
        auto id = load_raw_sync(path, type_id<T>());
        return AssetHandle<T>{id};
    }

    /// アセットデータ取得
    template <typename T>
    [[nodiscard]] T* get(AssetHandle<T> handle) {
        auto* entry = find_entry(handle.id());
        return entry ? static_cast<T*>(entry->data) : nullptr;
    }

    /// アセット解放
    void unload(u64 asset_id);

    /// 全未使用アセットを解放
    void gc();

    /// 非同期ロード処理を進める (毎フレーム呼ぶ)
    void update();

    [[nodiscard]] u32 loaded_count() const;
    [[nodiscard]] u32 pending_count() const;

private:
    u64  load_raw(const std::string& path, TypeID type);
    u64  load_raw_sync(const std::string& path, TypeID type);
    AssetEntry* find_entry(u64 id);

    VFS&                                    vfs_;
    std::unordered_map<u64, AssetEntry>     assets_;
    std::unordered_map<TypeID, LoaderFn>    loaders_;
    std::unordered_map<TypeID, UnloaderFn>  unloaders_;
    std::vector<u64>                        pending_;
    u64                                     next_id_ = 1;
    std::mutex                              mutex_;
};

} // namespace engine::resource
