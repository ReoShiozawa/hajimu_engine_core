/**
 * src/resource/resource_manager.cpp — 非同期リソースマネージャ実装
 */
#include <engine/resource/resource_manager.hpp>
#include <engine/core/log.hpp>

namespace engine::resource {

ResourceManager::ResourceManager(VFS& vfs) : vfs_(vfs) {
    ENG_INFO("ResourceManager initialized");
}

ResourceManager::~ResourceManager() {
    // 全アセット解放
    for (auto& [id, entry] : assets_) {
        if (entry.data && entry.state == AssetState::Loaded) {
            auto it = unloaders_.find(entry.type_id);
            if (it != unloaders_.end()) it->second(entry.data);
        }
    }
}

void ResourceManager::register_loader(TypeID type, LoaderFn loader, UnloaderFn unloader) {
    loaders_[type] = std::move(loader);
    unloaders_[type] = std::move(unloader);
}

u64 ResourceManager::load_raw(const std::string& path, TypeID type) {
    std::lock_guard lock(mutex_);
    u64 id = next_id_++;
    AssetEntry entry;
    entry.path = path;
    entry.type_id = type;
    entry.state = AssetState::Loading;
    assets_[id] = std::move(entry);
    pending_.push_back(id);
    return id;
}

u64 ResourceManager::load_raw_sync(const std::string& path, TypeID type) {
    std::lock_guard lock(mutex_);
    u64 id = next_id_++;

    AssetEntry entry;
    entry.path = path;
    entry.type_id = type;

    auto raw = vfs_.read_file(path);
    if (!raw) {
        ENG_ERROR("Failed to load asset: %s", path.c_str());
        entry.state = AssetState::Failed;
        assets_[id] = std::move(entry);
        return id;
    }

    auto loader_it = loaders_.find(type);
    if (loader_it == loaders_.end()) {
        ENG_ERROR("No loader for type %016llx", static_cast<unsigned long long>(type));
        entry.state = AssetState::Failed;
        assets_[id] = std::move(entry);
        return id;
    }

    auto result = loader_it->second(*raw);
    if (!result) {
        entry.state = AssetState::Failed;
    } else {
        entry.data = *result;
        entry.data_size = raw->size();
        entry.state = AssetState::Loaded;
        entry.ref_count = 1;
    }
    assets_[id] = std::move(entry);
    return id;
}

void ResourceManager::unload(u64 asset_id) {
    std::lock_guard lock(mutex_);
    auto it = assets_.find(asset_id);
    if (it == assets_.end()) return;

    auto& entry = it->second;
    if (entry.data && entry.state == AssetState::Loaded) {
        auto uit = unloaders_.find(entry.type_id);
        if (uit != unloaders_.end()) uit->second(entry.data);
    }
    assets_.erase(it);
}

void ResourceManager::gc() {
    std::lock_guard lock(mutex_);
    std::vector<u64> to_remove;
    for (auto& [id, entry] : assets_) {
        if (entry.ref_count == 0 &&
            entry.state == AssetState::Loaded) {
            to_remove.push_back(id);
        }
    }
    for (auto id : to_remove) unload(id);
}

void ResourceManager::update() {
    std::lock_guard lock(mutex_);
    std::vector<u64> completed;
    for (auto id : pending_) {
        auto& entry = assets_[id];
        auto raw = vfs_.read_file(entry.path);
        if (!raw) {
            entry.state = AssetState::Failed;
            completed.push_back(id);
            continue;
        }
        auto loader_it = loaders_.find(entry.type_id);
        if (loader_it == loaders_.end()) {
            entry.state = AssetState::Failed;
            completed.push_back(id);
            continue;
        }
        auto result = loader_it->second(*raw);
        if (!result) {
            entry.state = AssetState::Failed;
        } else {
            entry.data = *result;
            entry.data_size = raw->size();
            entry.state = AssetState::Loaded;
            entry.ref_count = 1;
        }
        completed.push_back(id);
    }
    for (auto id : completed) {
        pending_.erase(std::remove(pending_.begin(), pending_.end(), id), pending_.end());
    }
}

AssetEntry* ResourceManager::find_entry(u64 id) {
    auto it = assets_.find(id);
    return it != assets_.end() ? &it->second : nullptr;
}

u32 ResourceManager::loaded_count() const {
    u32 c = 0;
    for (auto& [_, e] : assets_) {
        if (e.state == AssetState::Loaded) ++c;
    }
    return c;
}

u32 ResourceManager::pending_count() const {
    return static_cast<u32>(pending_.size());
}

} // namespace engine::resource
