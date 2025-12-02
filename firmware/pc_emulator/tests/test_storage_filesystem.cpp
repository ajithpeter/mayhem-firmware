/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

/**
 * @file test_storage_filesystem.cpp
 * @brief Unit tests for filesystem storage implementation.
 */

#include "doctest.h"
#include "storage_filesystem.hpp"
#include <filesystem>
#include <fstream>
#include <cstring>
#include <unistd.h>

namespace fs = std::filesystem;

// Test fixture helper
class TestStorageFixture {
public:
    std::string test_dir;
    hal::StorageFilesystem storage;

    TestStorageFixture() {
        test_dir = "/tmp/portapack_test_" + std::to_string(getpid());
        fs::create_directories(test_dir);
        storage.init(test_dir);
    }

    ~TestStorageFixture() {
        storage.shutdown();
        fs::remove_all(test_dir);
    }

    std::string test_path(const std::string& name) {
        return test_dir + "/" + name;
    }
};

TEST_SUITE_BEGIN("StorageFilesystem");

// ============================================================================
// StorageFilesystem Constants Tests
// ============================================================================

TEST_CASE("StorageFilesystem constants") {
    SUBCASE("Invalid handle value") {
        CHECK_EQ(hal::IStorage::INVALID_HANDLE, static_cast<hal::IStorage::FileHandle>(-1));
    }

    SUBCASE("OpenMode enum values") {
        CHECK(static_cast<int>(hal::IStorage::OpenMode::Read) == 0);
        CHECK(static_cast<int>(hal::IStorage::OpenMode::Write) == 1);
        CHECK(static_cast<int>(hal::IStorage::OpenMode::ReadWrite) == 2);
        CHECK(static_cast<int>(hal::IStorage::OpenMode::Append) == 3);
    }

    SUBCASE("SeekOrigin enum values") {
        CHECK(static_cast<int>(hal::IStorage::SeekOrigin::Begin) == 0);
        CHECK(static_cast<int>(hal::IStorage::SeekOrigin::Current) == 1);
        CHECK(static_cast<int>(hal::IStorage::SeekOrigin::End) == 2);
    }

    SUBCASE("Error enum values") {
        CHECK(static_cast<int>(hal::IStorage::Error::None) == 0);
        CHECK(static_cast<int>(hal::IStorage::Error::NotFound) == 1);
        CHECK(static_cast<int>(hal::IStorage::Error::AccessDenied) == 2);
    }
}

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_CASE("StorageFilesystem initialization") {
    hal::StorageFilesystem storage;

    SUBCASE("Init with valid path") {
        std::string temp_dir = "/tmp/portapack_storage_test_" + std::to_string(getpid());
        fs::create_directories(temp_dir);
        CHECK(storage.init(temp_dir) == true);
        CHECK(storage.is_mounted() == true);
        storage.shutdown();
        fs::remove_all(temp_dir);
    }

    SUBCASE("Shutdown without init") {
        storage.shutdown();  // Should not crash
        CHECK(true);
    }

    SUBCASE("Init changes mounted state") {
        std::string temp_dir = "/tmp/portapack_storage_test2_" + std::to_string(getpid());
        fs::create_directories(temp_dir);
        CHECK(storage.is_mounted() == false);
        storage.init(temp_dir);
        CHECK(storage.is_mounted() == true);
        storage.shutdown();
        CHECK(storage.is_mounted() == false);
        fs::remove_all(temp_dir);
    }
}

// ============================================================================
// Space Information Tests
// ============================================================================

TEST_CASE("Storage space information") {
    TestStorageFixture fixture;

    SUBCASE("Free space is positive") {
        uint64_t free = fixture.storage.free_space();
        CHECK(free > 0);
    }

    SUBCASE("Total space is positive") {
        uint64_t total = fixture.storage.total_space();
        CHECK(total > 0);
    }

    SUBCASE("Free space is less than or equal to total") {
        uint64_t free = fixture.storage.free_space();
        uint64_t total = fixture.storage.total_space();
        CHECK(free <= total);
    }
}

// ============================================================================
// File Operations Tests
// ============================================================================

TEST_CASE("File operations") {
    TestStorageFixture fixture;

    SUBCASE("Create and write file") {
        auto handle = fixture.storage.open("test.txt", hal::IStorage::OpenMode::Write);
        CHECK(handle != hal::IStorage::INVALID_HANDLE);

        const char* data = "Hello, World!";
        int64_t written = fixture.storage.write(handle, data, strlen(data));
        CHECK_EQ(written, static_cast<int64_t>(strlen(data)));

        fixture.storage.close(handle);

        // Verify file exists
        CHECK(fs::exists(fixture.test_path("test.txt")));
    }

    SUBCASE("Read file") {
        // Create a test file first
        std::ofstream ofs(fixture.test_path("read_test.txt"));
        ofs << "Test content";
        ofs.close();

        auto handle = fixture.storage.open("read_test.txt", hal::IStorage::OpenMode::Read);
        CHECK(handle != hal::IStorage::INVALID_HANDLE);

        char buffer[256] = {0};
        int64_t read = fixture.storage.read(handle, buffer, sizeof(buffer) - 1);
        CHECK(read > 0);
        CHECK(std::string(buffer) == "Test content");

        fixture.storage.close(handle);
    }

    SUBCASE("Append to file") {
        // Create initial file
        std::ofstream ofs(fixture.test_path("append_test.txt"));
        ofs << "Initial";
        ofs.close();

        auto handle = fixture.storage.open("append_test.txt", hal::IStorage::OpenMode::Append);
        CHECK(handle != hal::IStorage::INVALID_HANDLE);

        const char* data = " Appended";
        fixture.storage.write(handle, data, strlen(data));
        fixture.storage.close(handle);

        // Verify content
        std::ifstream ifs(fixture.test_path("append_test.txt"));
        std::string content((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
        CHECK(content == "Initial Appended");
    }

    SUBCASE("File not found") {
        auto handle = fixture.storage.open("nonexistent.txt", hal::IStorage::OpenMode::Read);
        CHECK_EQ(handle, hal::IStorage::INVALID_HANDLE);
    }

    SUBCASE("Close returns no error") {
        auto handle = fixture.storage.open("close_test.txt", hal::IStorage::OpenMode::Write);
        REQUIRE(handle != hal::IStorage::INVALID_HANDLE);
        auto err = fixture.storage.close(handle);
        CHECK(err == hal::IStorage::Error::None);
    }

    SUBCASE("Sync file") {
        auto handle = fixture.storage.open("sync_test.txt", hal::IStorage::OpenMode::Write);
        REQUIRE(handle != hal::IStorage::INVALID_HANDLE);
        fixture.storage.write(handle, "data", 4);
        auto err = fixture.storage.sync(handle);
        CHECK(err == hal::IStorage::Error::None);
        fixture.storage.close(handle);
    }
}

// ============================================================================
// Seek Operations Tests
// ============================================================================

TEST_CASE("Seek operations") {
    TestStorageFixture fixture;

    // Create test file
    std::ofstream ofs(fixture.test_path("seek_test.txt"));
    ofs << "0123456789";
    ofs.close();

    auto handle = fixture.storage.open("seek_test.txt", hal::IStorage::OpenMode::Read);
    REQUIRE(handle != hal::IStorage::INVALID_HANDLE);

    SUBCASE("Seek from beginning") {
        fixture.storage.seek(handle, 5, hal::IStorage::SeekOrigin::Begin);
        char c;
        fixture.storage.read(handle, &c, 1);
        CHECK_EQ(c, '5');
    }

    SUBCASE("Seek from current") {
        // Seek to position 5 via current position
        int64_t pos = fixture.storage.seek(handle, 3, hal::IStorage::SeekOrigin::Begin);
        CHECK_EQ(pos, 3);
        pos = fixture.storage.seek(handle, 2, hal::IStorage::SeekOrigin::Current);
        CHECK_EQ(pos, 5);
        // Read and verify we're at position 5
        char c;
        fixture.storage.read(handle, &c, 1);
        CHECK_EQ(c, '5');
    }

    SUBCASE("Seek from end") {
        fixture.storage.seek(handle, -3, hal::IStorage::SeekOrigin::End);
        char c;
        fixture.storage.read(handle, &c, 1);
        CHECK_EQ(c, '7');
    }

    SUBCASE("Tell position") {
        fixture.storage.seek(handle, 5, hal::IStorage::SeekOrigin::Begin);
        int64_t pos = fixture.storage.tell(handle);
        CHECK_EQ(pos, 5);
    }

    SUBCASE("Size via handle") {
        int64_t size = fixture.storage.size(handle);
        CHECK_EQ(size, 10);
    }

    fixture.storage.close(handle);
}

// ============================================================================
// Directory Operations Tests
// ============================================================================

TEST_CASE("Directory operations") {
    TestStorageFixture fixture;

    SUBCASE("Create directory") {
        auto err = fixture.storage.mkdir("subdir");
        CHECK(err == hal::IStorage::Error::None);
        CHECK(fs::is_directory(fixture.test_path("subdir")));
    }

    SUBCASE("Remove empty directory") {
        fs::create_directory(fixture.test_path("empty_dir"));
        auto err = fixture.storage.rmdir("empty_dir");
        CHECK(err == hal::IStorage::Error::None);
        CHECK_FALSE(fs::exists(fixture.test_path("empty_dir")));
    }

    SUBCASE("Directory exists check") {
        fs::create_directory(fixture.test_path("check_dir"));
        CHECK(fixture.storage.exists("check_dir"));
        CHECK(fixture.storage.is_directory("check_dir"));
    }

    SUBCASE("List directory") {
        // Create some test files and directories
        std::ofstream(fixture.test_path("file1.txt")) << "data";
        std::ofstream(fixture.test_path("file2.txt")) << "data";
        fs::create_directory(fixture.test_path("subdir1"));

        auto entries = fixture.storage.list_directory("");
        CHECK(entries.size() >= 3);

        // Verify we can find our created items
        bool found_file1 = false, found_file2 = false, found_subdir = false;
        for (const auto& entry : entries) {
            if (entry.name == "file1.txt") found_file1 = true;
            if (entry.name == "file2.txt") found_file2 = true;
            if (entry.name == "subdir1") {
                found_subdir = true;
                CHECK(entry.is_directory == true);
            }
        }
        CHECK(found_file1);
        CHECK(found_file2);
        CHECK(found_subdir);
    }
}

// ============================================================================
// File Info Tests
// ============================================================================

TEST_CASE("File info") {
    TestStorageFixture fixture;

    // Create test file
    std::ofstream ofs(fixture.test_path("info_test.txt"));
    ofs << "1234567890";
    ofs.close();

    SUBCASE("File exists") {
        CHECK(fixture.storage.exists("info_test.txt"));
    }

    SUBCASE("File does not exist") {
        CHECK_FALSE(fixture.storage.exists("nonexistent.txt"));
    }

    SUBCASE("Is not directory") {
        CHECK_FALSE(fixture.storage.is_directory("info_test.txt"));
    }

    SUBCASE("Stat file") {
        auto entry_opt = fixture.storage.stat("info_test.txt");
        CHECK(entry_opt.has_value());
        if (entry_opt) {
            CHECK_EQ(entry_opt->name, "info_test.txt");
            CHECK_EQ(entry_opt->size, 10);
            CHECK_EQ(entry_opt->is_directory, false);
        }
    }

    SUBCASE("Stat non-existent file") {
        auto entry_opt = fixture.storage.stat("nonexistent.txt");
        CHECK_FALSE(entry_opt.has_value());
    }
}

// ============================================================================
// File Remove/Rename Tests
// ============================================================================

TEST_CASE("File remove and rename") {
    TestStorageFixture fixture;

    SUBCASE("Remove file") {
        std::ofstream(fixture.test_path("remove_me.txt")) << "data";
        auto err = fixture.storage.remove("remove_me.txt");
        CHECK(err == hal::IStorage::Error::None);
        CHECK_FALSE(fs::exists(fixture.test_path("remove_me.txt")));
    }

    SUBCASE("Rename file") {
        std::ofstream(fixture.test_path("old_name.txt")) << "data";
        auto err = fixture.storage.rename("old_name.txt", "new_name.txt");
        CHECK(err == hal::IStorage::Error::None);
        CHECK_FALSE(fs::exists(fixture.test_path("old_name.txt")));
        CHECK(fs::exists(fixture.test_path("new_name.txt")));
    }
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_CASE("Error handling") {
    TestStorageFixture fixture;

    SUBCASE("Write to invalid handle") {
        int64_t written = fixture.storage.write(hal::IStorage::INVALID_HANDLE, "test", 4);
        CHECK(written == -1);
    }

    SUBCASE("Read from invalid handle") {
        char buffer[10];
        int64_t read = fixture.storage.read(hal::IStorage::INVALID_HANDLE, buffer, 10);
        CHECK(read == -1);
    }

    SUBCASE("Seek on invalid handle") {
        int64_t pos = fixture.storage.seek(hal::IStorage::INVALID_HANDLE, 0, hal::IStorage::SeekOrigin::Begin);
        CHECK(pos == -1);
    }

    SUBCASE("Tell on invalid handle") {
        int64_t pos = fixture.storage.tell(hal::IStorage::INVALID_HANDLE);
        CHECK(pos == -1);
    }

    SUBCASE("Size on invalid handle") {
        int64_t size = fixture.storage.size(hal::IStorage::INVALID_HANDLE);
        CHECK(size == -1);
    }
}

TEST_SUITE_END();
