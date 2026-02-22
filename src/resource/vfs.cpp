/**
 * src/resource/vfs.cpp — Virtual File System 実装
 */
#include <engine/resource/vfs.hpp>
#include <engine/core/log.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace engine::resource {

void VFS::mount(const MountPoint& mp) {
    mounts_.push_back(mp);
    std::sort(mounts_.begin(), mounts_.end(),
              [](auto& a, auto& b) { return a.priority > b.priority; });
    ENG_INFO("VFS: mounted '%s' → '%s'", mp.prefix.c_str(), mp.real_path.c_str());
}

void VFS::unmount(const std::string& prefix) {
    mounts_.erase(
        std::remove_if(mounts_.begin(), mounts_.end(),
            [&](auto& mp) { return mp.prefix == prefix; }),
        mounts_.end());
}

Result<std::string> VFS::resolve(const std::string& vpath) const {
    for (auto& mp : mounts_) {
        if (vpath.starts_with(mp.prefix)) {
            std::string rel = vpath.substr(mp.prefix.size());
            if (!rel.empty() && rel[0] == '/') rel = rel.substr(1);
            std::string real = mp.real_path + "/" + rel;
            if (fs::exists(real)) return real;
        }
    }
    return std::unexpected(Error::NotFound);
}

Result<std::vector<u8>> VFS::read_file(const std::string& vpath) const {
    auto resolved = resolve(vpath);
    if (!resolved) return std::unexpected(resolved.error());

    std::ifstream file(*resolved, std::ios::binary | std::ios::ate);
    if (!file) return std::unexpected(Error::IOError);

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<u8> data(static_cast<usize>(size));
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

Result<std::string> VFS::read_text(const std::string& vpath) const {
    auto data = read_file(vpath);
    if (!data) return std::unexpected(data.error());
    return std::string(data->begin(), data->end());
}

Result<void> VFS::write_file(const std::string& vpath, std::span<const u8> data) {
    // 最初にマッチするマウントポイントに書き込み
    for (auto& mp : mounts_) {
        if (vpath.starts_with(mp.prefix)) {
            std::string rel = vpath.substr(mp.prefix.size());
            if (!rel.empty() && rel[0] == '/') rel = rel.substr(1);
            std::string real = mp.real_path + "/" + rel;

            // ディレクトリ作成
            fs::create_directories(fs::path(real).parent_path());

            std::ofstream file(real, std::ios::binary);
            if (!file) return std::unexpected(Error::IOError);
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
            return {};
        }
    }
    return std::unexpected(Error::NotFound);
}

bool VFS::exists(const std::string& vpath) const {
    return resolve(vpath).has_value();
}

std::vector<std::string> VFS::list_dir(const std::string& vpath) const {
    std::vector<std::string> result;
    auto resolved = resolve(vpath);
    if (!resolved) return result;

    for (auto& entry : fs::directory_iterator(*resolved)) {
        result.push_back(entry.path().filename().string());
    }
    return result;
}

} // namespace engine::resource
