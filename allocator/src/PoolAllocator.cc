#include "PoolAllocator.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <utility>

namespace MemoryManagement
{

PoolAllocator::PoolAllocator(std::span<std::byte> memory, Config requested_config)
    : global_memory(memory), config(requested_config)
{
    // min=32 (2^5), max=8192 (2^13)
    const auto bin_count = [&]() 
    {
        if (config.fixed_chunk_size != 0) 
        {
            return 1;
        }
        return std::bit_width(config.max_chunk_size) - std::bit_width(config.min_chunk_size) + 1;
    }();

    available_shard_heads.resize(bin_count);
    shard_controls.resize(memory.size() / config.shard_size);
}

PoolAllocator::PoolAllocator(PoolAllocator&& other) noexcept
    : global_memory{}, config{}
{
    swap(other);
}

PoolAllocator& PoolAllocator::operator=(PoolAllocator&& other) noexcept
{
    if (this != &other)
    {
        PoolAllocator temporary(std::move(other));
        swap(temporary);
    }
    return *this;
}

void PoolAllocator::swap(PoolAllocator& other) noexcept
{
    using std::swap;

    swap(global_memory, other.global_memory);
    swap(config, other.config);
    swap(available_shard_heads, other.available_shard_heads);
    swap(shard_controls, other.shard_controls);
    swap(next_unused_shard, other.next_unused_shard);
}

void* PoolAllocator::acquire(std::size_t size) noexcept
{
    if (available_shard_heads.empty())
    {
        return nullptr;
    }

    std::size_t chunk_size = 0;
    std::size_t bin = 0;

    if (config.fixed_chunk_size != 0)
    {
        if (size > config.fixed_chunk_size)
        {
            return nullptr;
        }

        chunk_size = config.fixed_chunk_size;
        bin = 0;
    }
    else
    {
        if (size > config.max_chunk_size)
        {
            return nullptr;
        }

        chunk_size = std::max(config.min_chunk_size, std::bit_ceil(size));

        bin = std::bit_width(chunk_size) - std::bit_width(config.min_chunk_size);
    }

    auto& available_shard = available_shard_heads[bin];

    if (!available_shard)
    {
        if (next_unused_shard == shard_controls.size())
        {
            return nullptr;
        }

        const auto shard_index = next_unused_shard++;

        auto& shard_control = shard_controls[shard_index];

        shard_control.pool.init
        (
            global_memory.subspan(shard_index * config.shard_size, config.shard_size),
            chunk_size
        );

        shard_control.size_class_index = bin;
        available_shard = shard_index;
    }

    auto& shard_control = shard_controls[*available_shard];
    auto* result = shard_control.pool.acquire();

    if (shard_control.pool.isExhausted())
    {
        available_shard = shard_control.next_available_shard;
        shard_control.next_available_shard.reset();
    }

    return result;
}


bool PoolAllocator::release(void* pointer) noexcept
{
    const auto address = reinterpret_cast<std::uintptr_t>(pointer);
    const auto beginning = reinterpret_cast<std::uintptr_t>(global_memory.data());

    if (pointer == nullptr || address < beginning || address - beginning >= global_memory.size())
    {
        return false;
    }

    const auto shard_index = (address - beginning) / config.shard_size;

    if (shard_index >= shard_controls.size())
    {
        return false;
    }

    auto& shard_control = shard_controls[shard_index];

    const bool was_exhausted = shard_control.pool.isExhausted();

    if (!shard_control.pool.release(pointer))
    {
        return false;
    }

    if (was_exhausted)
    {
        shard_control.next_available_shard = available_shard_heads[shard_control.size_class_index];
        available_shard_heads[shard_control.size_class_index] = shard_index;
    }
    
    return true;
}

}
