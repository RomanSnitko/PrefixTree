#pragma once

#include "Pool.hpp"

#include <optional>
#include <vector>

namespace MemoryManagement
{

class PoolAllocator
{
public:
    struct Config
    {
        std::size_t shard_size;
        std::size_t min_chunk_size;
        std::size_t max_chunk_size;

        // Zero preserves the original power-of-two size classes.
        std::size_t fixed_chunk_size;
    };

    explicit PoolAllocator(std::span<std::byte> global_memory, Config config);

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&& other) noexcept;
    PoolAllocator& operator=(PoolAllocator&& other) noexcept;

    [[nodiscard]] void* acquire(std::size_t size) noexcept;
    // Release only live chunks, after ending the client object lifetime.
    [[nodiscard]] bool release(void* pointer) noexcept;

private:
    void swap(PoolAllocator& other) noexcept;

    struct ControlBlock
    {
        Pool pool;
        std::size_t size_class_index{};
        std::optional<std::size_t> next_available_shard;
    };

    /*
    * shard_begin = global_base + i * shard_size
    * shard_end   = shard_begin + shard_size
    */
    std::span<std::byte> global_memory;

    Config config;

    /*
    bin_heads_[0] -> pools for 32
    bin_heads_[1] -> pools for 64
    bin_heads_[2] -> pools for 128
    */
    std::vector<std::optional<std::size_t>> available_shard_heads;
    
    std::vector<ControlBlock> shard_controls;
    std::size_t next_unused_shard{};
};

}
