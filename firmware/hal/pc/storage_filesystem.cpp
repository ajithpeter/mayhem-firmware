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

#include "storage_filesystem.hpp"
#include <iostream>
#include <chrono>

namespace fs = std::filesystem;

namespace hal {

StorageFilesystem::StorageFilesystem() = default;

StorageFilesystem::~StorageFilesystem() {
    shutdown();
}

bool StorageFilesystem::init(const std::string& root_path) {
    std::lock_guard<std::mutex> lock(mutex_);

    root_path_ = fs::absolute(root_path);

    // Create the root directory if it doesn't exist
    std::error_code ec;
    if (!fs::exists(root_path_, ec)) {
        if (!fs::create_directories(root_path_, ec)) {
            std::cerr << "Failed to create storage root: " << root_path_
                      << " - " << ec.message() << std::endl;
            return false;
        }
    }

    mounted_ = true;
    std::cout << "StorageFilesystem initialized at: " << root_path_ << std::endl;
    return true;
}

void StorageFilesystem::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Close all open files
    for (auto& [handle, file] : open_files_) {
        if (file.stream.is_open()) {
            file.stream.close();
        }
    }
    open_files_.clear();

    mounted_ = false;
}

bool StorageFilesystem::is_mounted() const {
    return mounted_;
}

uint64_t StorageFilesystem::free_space() const {
    if (!mounted_) return 0;

    std::error_code ec;
    auto space = fs::space(root_path_, ec);
    if (ec) return 0;

    return space.available;
}

uint64_t StorageFilesystem::total_space() const {
    if (!mounted_) return 0;

    std::error_code ec;
    auto space = fs::space(root_path_, ec);
    if (ec) return 0;

    return space.capacity;
}

fs::path StorageFilesystem::resolve_path(const std::string& path) const {
    // Remove leading slashes and combine with root
    std::string clean_path = path;
    while (!clean_path.empty() && (clean_path[0] == '/' || clean_path[0] == '\\')) {
        clean_path = clean_path.substr(1);
    }
    return root_path_ / clean_path;
}

IStorage::FileHandle StorageFilesystem::open(const std::string& path, OpenMode mode) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!mounted_) return INVALID_HANDLE;

    fs::path full_path = resolve_path(path);

    std::ios::openmode open_mode = std::ios::binary;
    switch (mode) {
        case OpenMode::Read:
            open_mode |= std::ios::in;
            break;
        case OpenMode::Write:
            open_mode |= std::ios::out | std::ios::trunc;
            break;
        case OpenMode::ReadWrite:
            open_mode |= std::ios::in | std::ios::out;
            break;
        case OpenMode::Append:
            open_mode |= std::ios::out | std::ios::app;
            break;
    }

    OpenFile file;
    file.path = full_path;
    file.mode = mode;
    file.stream.open(full_path, open_mode);

    if (!file.stream.is_open()) {
        // If write mode and file doesn't exist, try creating it
        if (mode == OpenMode::Write || mode == OpenMode::Append) {
            // Ensure parent directory exists
            std::error_code ec;
            fs::create_directories(full_path.parent_path(), ec);
            file.stream.open(full_path, open_mode);
        }

        if (!file.stream.is_open()) {
            return INVALID_HANDLE;
        }
    }

    FileHandle handle = next_handle_++;
    open_files_[handle] = std::move(file);
    return handle;
}

IStorage::Error StorageFilesystem::close(FileHandle handle) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = open_files_.find(handle);
    if (it == open_files_.end()) {
        return Error::NotFound;
    }

    if (it->second.stream.is_open()) {
        it->second.stream.close();
    }

    open_files_.erase(it);
    return Error::None;
}

int64_t StorageFilesystem::read(FileHandle handle, void* buffer, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = open_files_.find(handle);
    if (it == open_files_.end()) {
        return -1;
    }

    it->second.stream.read(static_cast<char*>(buffer), size);
    return it->second.stream.gcount();
}

int64_t StorageFilesystem::write(FileHandle handle, const void* buffer, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = open_files_.find(handle);
    if (it == open_files_.end()) {
        return -1;
    }

    auto pos_before = it->second.stream.tellp();
    it->second.stream.write(static_cast<const char*>(buffer), size);
    auto pos_after = it->second.stream.tellp();

    if (it->second.stream.fail()) {
        return -1;
    }

    return static_cast<int64_t>(pos_after - pos_before);
}

int64_t StorageFilesystem::seek(FileHandle handle, int64_t offset, SeekOrigin origin) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = open_files_.find(handle);
    if (it == open_files_.end()) {
        return -1;
    }

    std::ios::seekdir dir;
    switch (origin) {
        case SeekOrigin::Begin:   dir = std::ios::beg; break;
        case SeekOrigin::Current: dir = std::ios::cur; break;
        case SeekOrigin::End:     dir = std::ios::end; break;
    }

    // Only seek the appropriate pointer based on file mode to avoid
    // double-seek issues with SeekOrigin::Current
    switch (it->second.mode) {
        case OpenMode::Read:
            it->second.stream.seekg(offset, dir);
            break;
        case OpenMode::Write:
        case OpenMode::Append:
            it->second.stream.seekp(offset, dir);
            break;
        case OpenMode::ReadWrite:
            // For read-write, sync both positions but use absolute positioning
            // to avoid current-position issues
            if (origin == SeekOrigin::Current) {
                auto current_pos = it->second.stream.tellg();
                it->second.stream.seekg(current_pos + offset, std::ios::beg);
                it->second.stream.seekp(current_pos + offset, std::ios::beg);
            } else {
                it->second.stream.seekg(offset, dir);
                it->second.stream.seekp(offset, dir);
            }
            break;
    }

    return it->second.stream.tellg();
}

int64_t StorageFilesystem::tell(FileHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = open_files_.find(handle);
    if (it == open_files_.end()) {
        return -1;
    }

    return const_cast<std::fstream&>(it->second.stream).tellg();
}

int64_t StorageFilesystem::size(FileHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = open_files_.find(handle);
    if (it == open_files_.end()) {
        return -1;
    }

    std::error_code ec;
    auto sz = fs::file_size(it->second.path, ec);
    if (ec) return -1;
    return static_cast<int64_t>(sz);
}

IStorage::Error StorageFilesystem::sync(FileHandle handle) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = open_files_.find(handle);
    if (it == open_files_.end()) {
        return Error::NotFound;
    }

    it->second.stream.flush();
    return Error::None;
}

std::vector<IStorage::FileEntry> StorageFilesystem::list_directory(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<FileEntry> entries;

    if (!mounted_) return entries;

    fs::path full_path = resolve_path(path);

    std::error_code ec;
    if (!fs::exists(full_path, ec) || !fs::is_directory(full_path, ec)) {
        return entries;
    }

    for (const auto& entry : fs::directory_iterator(full_path, ec)) {
        FileEntry fe;
        fe.name = entry.path().filename().string();
        fe.is_directory = entry.is_directory();

        if (!fe.is_directory) {
            fe.size = entry.file_size(ec);
        } else {
            fe.size = 0;
        }

        auto ftime = entry.last_write_time(ec);
        if (!ec) {
            // Convert file_time to system_clock::time_point, then to Unix timestamp
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            fe.timestamp = static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    sctp.time_since_epoch()
                ).count()
            );
        } else {
            fe.timestamp = 0;
        }

        entries.push_back(fe);
    }

    return entries;
}

IStorage::Error StorageFilesystem::mkdir(const std::string& path) {
    if (!mounted_) return Error::NotMounted;

    fs::path full_path = resolve_path(path);

    std::error_code ec;
    if (fs::create_directories(full_path, ec)) {
        return Error::None;
    }

    if (fs::exists(full_path, ec)) {
        return Error::AlreadyExists;
    }

    return Error::IoError;
}

IStorage::Error StorageFilesystem::rmdir(const std::string& path) {
    if (!mounted_) return Error::NotMounted;

    fs::path full_path = resolve_path(path);

    std::error_code ec;
    if (!fs::exists(full_path, ec)) {
        return Error::NotFound;
    }

    if (!fs::is_directory(full_path, ec)) {
        return Error::InvalidPath;
    }

    if (!fs::is_empty(full_path, ec)) {
        return Error::NotEmpty;
    }

    if (fs::remove(full_path, ec)) {
        return Error::None;
    }

    return Error::IoError;
}

bool StorageFilesystem::exists(const std::string& path) const {
    if (!mounted_) return false;

    fs::path full_path = resolve_path(path);
    std::error_code ec;
    return fs::exists(full_path, ec);
}

bool StorageFilesystem::is_directory(const std::string& path) const {
    if (!mounted_) return false;

    fs::path full_path = resolve_path(path);
    std::error_code ec;
    return fs::is_directory(full_path, ec);
}

IStorage::Error StorageFilesystem::remove(const std::string& path) {
    if (!mounted_) return Error::NotMounted;

    fs::path full_path = resolve_path(path);

    std::error_code ec;
    if (!fs::exists(full_path, ec)) {
        return Error::NotFound;
    }

    if (fs::remove(full_path, ec)) {
        return Error::None;
    }

    return Error::IoError;
}

IStorage::Error StorageFilesystem::rename(const std::string& old_path, const std::string& new_path) {
    if (!mounted_) return Error::NotMounted;

    fs::path full_old = resolve_path(old_path);
    fs::path full_new = resolve_path(new_path);

    std::error_code ec;
    if (!fs::exists(full_old, ec)) {
        return Error::NotFound;
    }

    fs::rename(full_old, full_new, ec);
    if (ec) {
        return Error::IoError;
    }

    return Error::None;
}

std::optional<IStorage::FileEntry> StorageFilesystem::stat(const std::string& path) const {
    if (!mounted_) return std::nullopt;

    fs::path full_path = resolve_path(path);

    std::error_code ec;
    if (!fs::exists(full_path, ec)) {
        return std::nullopt;
    }

    FileEntry entry;
    entry.name = full_path.filename().string();
    entry.is_directory = fs::is_directory(full_path, ec);

    if (!entry.is_directory) {
        entry.size = fs::file_size(full_path, ec);
    } else {
        entry.size = 0;
    }

    auto ftime = fs::last_write_time(full_path, ec);
    if (!ec) {
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        entry.timestamp = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                sctp.time_since_epoch()
            ).count()
        );
    } else {
        entry.timestamp = 0;
    }

    return entry;
}

} // namespace hal
