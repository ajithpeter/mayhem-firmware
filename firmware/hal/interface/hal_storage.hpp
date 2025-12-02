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

#ifndef __HAL_STORAGE_HPP__
#define __HAL_STORAGE_HPP__

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <optional>

namespace hal {

/**
 * @brief Abstract interface for storage hardware abstraction layer.
 *
 * This interface defines the contract that all storage implementations
 * must fulfill, allowing the application to work with different storage
 * backends (SD card via SDIO, local filesystem, etc.)
 */
class IStorage {
public:
    // File entry information
    struct FileEntry {
        std::string name;
        uint64_t size;
        bool is_directory;
        uint32_t timestamp;  // Unix timestamp
    };

    // File open modes
    enum class OpenMode {
        Read,
        Write,
        ReadWrite,
        Append
    };

    // Seek origins
    enum class SeekOrigin {
        Begin,
        Current,
        End
    };

    // Error codes
    enum class Error {
        None = 0,
        NotFound,
        AccessDenied,
        InvalidPath,
        DiskFull,
        IoError,
        AlreadyExists,
        NotEmpty,
        NotMounted
    };

    // File handle type (opaque)
    using FileHandle = int32_t;
    static constexpr FileHandle INVALID_HANDLE = -1;

    virtual ~IStorage() = default;

    /**
     * @brief Initialize the storage system
     * @param root_path Base path for storage (e.g., SD card mount point)
     * @return true on success, false on failure
     */
    virtual bool init(const std::string& root_path) = 0;

    /**
     * @brief Shutdown the storage system
     */
    virtual void shutdown() = 0;

    /**
     * @brief Check if storage is mounted and available
     * @return true if storage is available
     */
    virtual bool is_mounted() const = 0;

    /**
     * @brief Get free space on storage
     * @return Free space in bytes, or 0 if unavailable
     */
    virtual uint64_t free_space() const = 0;

    /**
     * @brief Get total storage capacity
     * @return Total capacity in bytes
     */
    virtual uint64_t total_space() const = 0;

    // File operations

    /**
     * @brief Open a file
     * @param path File path (relative to storage root)
     * @param mode Open mode
     * @return File handle, or INVALID_HANDLE on failure
     */
    virtual FileHandle open(const std::string& path, OpenMode mode) = 0;

    /**
     * @brief Close an open file
     * @param handle File handle
     * @return Error::None on success
     */
    virtual Error close(FileHandle handle) = 0;

    /**
     * @brief Read from an open file
     * @param handle File handle
     * @param buffer Buffer to read into
     * @param size Number of bytes to read
     * @return Number of bytes read, or -1 on error
     */
    virtual int64_t read(FileHandle handle, void* buffer, size_t size) = 0;

    /**
     * @brief Write to an open file
     * @param handle File handle
     * @param buffer Buffer to write from
     * @param size Number of bytes to write
     * @return Number of bytes written, or -1 on error
     */
    virtual int64_t write(FileHandle handle, const void* buffer, size_t size) = 0;

    /**
     * @brief Seek to position in file
     * @param handle File handle
     * @param offset Offset from origin
     * @param origin Seek origin
     * @return New position, or -1 on error
     */
    virtual int64_t seek(FileHandle handle, int64_t offset, SeekOrigin origin) = 0;

    /**
     * @brief Get current position in file
     * @param handle File handle
     * @return Current position, or -1 on error
     */
    virtual int64_t tell(FileHandle handle) const = 0;

    /**
     * @brief Get file size
     * @param handle File handle
     * @return File size, or -1 on error
     */
    virtual int64_t size(FileHandle handle) const = 0;

    /**
     * @brief Sync file to storage
     * @param handle File handle
     * @return Error::None on success
     */
    virtual Error sync(FileHandle handle) = 0;

    // Directory operations

    /**
     * @brief List directory contents
     * @param path Directory path
     * @return Vector of file entries, or empty on error
     */
    virtual std::vector<FileEntry> list_directory(const std::string& path) = 0;

    /**
     * @brief Create a directory
     * @param path Directory path
     * @return Error::None on success
     */
    virtual Error mkdir(const std::string& path) = 0;

    /**
     * @brief Remove an empty directory
     * @param path Directory path
     * @return Error::None on success
     */
    virtual Error rmdir(const std::string& path) = 0;

    // File system operations

    /**
     * @brief Check if path exists
     * @param path File or directory path
     * @return true if path exists
     */
    virtual bool exists(const std::string& path) const = 0;

    /**
     * @brief Check if path is a directory
     * @param path Path to check
     * @return true if path is a directory
     */
    virtual bool is_directory(const std::string& path) const = 0;

    /**
     * @brief Delete a file
     * @param path File path
     * @return Error::None on success
     */
    virtual Error remove(const std::string& path) = 0;

    /**
     * @brief Rename/move a file or directory
     * @param old_path Current path
     * @param new_path New path
     * @return Error::None on success
     */
    virtual Error rename(const std::string& old_path, const std::string& new_path) = 0;

    /**
     * @brief Get file information
     * @param path File path
     * @return Optional FileEntry, or nullopt if not found
     */
    virtual std::optional<FileEntry> stat(const std::string& path) const = 0;
};

} // namespace hal

#endif // __HAL_STORAGE_HPP__
