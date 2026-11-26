#pragma once

#include <PoolAllocator.hpp>

#include <filesystem>

[[nodiscard]] MemoryManagement::PoolAllocator::Config readAllocatorConfig(const std::filesystem::path& path);
