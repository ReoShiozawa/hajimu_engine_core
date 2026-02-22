/**
 * engine/resource/vfs.hpp — Virtual File System
 *
 * res:// パスからの透過的アクセス。
 * ディスク, ZIP, メモリ, ネットワーク バックエンド対応。
 */
#pragma once

#include <engine/core/types.hpp>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace engine::resource {

// ── VFS バックエンド種別 ────────────────────────────────
enum class MountType : u8 {
    Directory, ZipArchive, Memory
};

// ── マウントポイント ────────────────────────────────────
struct MountPoint {
    std::string prefix;      // e.g., "res://textures"
    std::string real_path;   // e.g., "/path/to/assets/textures"
    MountType   type = MountType::Directory;
    i32         priority = 0; // 高い方が優先
};

// ── VFS ─────────────────────────────────────────────────
class VFS {
public:
    VFS() = default;
    ~VFS() = default;

    /// マウントポイント追加
    void mount(const MountPoint& mp);

    /// マウントポイント除去
    void unmount(const std::string& prefix);

    /// ファイル読み込み (全バイト)
    [[nodiscard]] Result<std::vector<u8>> read_file(const std::string& vpath) const;

    /// テキストファイル読み込み
    [[nodiscard]] Result<std::string> read_text(const std::string& vpath) const;

    /// ファイル書き込み
    [[nodiscard]] Result<void> write_file(const std::string& vpath, std::span<const u8> data);

    /// ファイル存在チェック
    [[nodiscard]] bool exists(const std::string& vpath) const;

    /// ディレクトリ列挙
    [[nodiscard]] std::vector<std::string> list_dir(const std::string& vpath) const;

    /// vpath → 実ファイルパス解決
    [[nodiscard]] Result<std::string> resolve(const std::string& vpath) const;

private:
    std::vector<MountPoint> mounts_;
};

} // namespace engine::resource
