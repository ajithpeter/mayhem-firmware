/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __STORAGE_FILESYSTEM_HPP__
#define __STORAGE_FILESYSTEM_HPP__

#include "../interface/hal_storage.hpp"
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace hal {

/**
 * @brief Filesystem-based storage implementation for PC emulation.
 *
 * This class implements the IStorage interface using the C++17 filesystem
 * library, emulating the SD card storage of PortaPack using a local
 * directory on the PC.
 */
class StorageFilesystem : public IStorage {
public:
    StorageFilesystem();
    ~StorageFilesystem() override;

    // IStorage interface implementation
    bool init(const std::string& root_path) override;
    void shutdown() override;
    bool is_mounted() const override;
    uint64_t free_space() const override;
    uint64_t total_space() const override;

    FileHandle open(const std::string& path, OpenMode mode) override;
    Error close(FileHandle handle) override;
    int64_t read(FileHandle handle, void* buffer, size_t size) override;
    int64_t write(FileHandle handle, const void* buffer, size_t size) override;
    int64_t seek(FileHandle handle, int64_t offset, SeekOrigin origin) override;
    int64_t tell(FileHandle handle) const override;
    int64_t size(FileHandle handle) const override;
    Error sync(FileHandle handle) override;

    std::vector<FileEntry> list_directory(const std::string& path) override;
    Error mkdir(const std::string& path) override;
    Error rmdir(const std::string& path) override;

    bool exists(const std::string& path) const override;
    bool is_directory(const std::string& path) const override;
    Error remove(const std::string& path) override;
    Error rename(const std::string& old_path, const std::string& new_path) override;
    std::optional<FileEntry> stat(const std::string& path) const override;

private:
    std::filesystem::path resolve_path(const std::string& path) const;

    // Open file tracking
    struct OpenFile {
        std::fstream stream;
        std::filesystem::path path;
        OpenMode mode;
    };

    std::filesystem::path root_path_;
    std::unordered_map<FileHandle, OpenFile> open_files_;
    FileHandle next_handle_ = 1;

    mutable std::mutex mutex_;
    std::atomic<bool> mounted_{false};
};

} // namespace hal

#endif // __STORAGE_FILESYSTEM_HPP__
