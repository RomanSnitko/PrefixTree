#include "Pool.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace MemoryManagement
{

Pool::Pool(Pool&& other) noexcept
{
    swap(other);
}

Pool& Pool::operator=(Pool&& other) noexcept
{
    if (this != &other)
    {
        Pool temporary(std::move(other));
        swap(temporary);
    }

    return *this;
}

void Pool::swap(Pool& other) noexcept
{
    using std::swap;

    swap(shard_begin, other.shard_begin);
    swap(chunk_size, other.chunk_size);
    swap(chunk_capacity, other.chunk_capacity);
    swap(next_unused_chunk, other.next_unused_chunk);
    swap(released_chunk_head, other.released_chunk_head);
}

void Pool::init(std::span<std::byte> shard, std::size_t requested_size) noexcept
{
    shard_begin = shard.data();
    chunk_size = requested_size;
    chunk_capacity = shard.size() / chunk_size;
    next_unused_chunk = 0;
    released_chunk_head = nullptr;
}

/*
Grow-only (not issued)
Chunks that have never been allocated yet are calculated arithmetically:
addr = base + index * chunk_size
*/
void* Pool::acquire() noexcept
{
    // the second src of free chunks: released list
    if (released_chunk_head != nullptr)
    {
        auto* released_chunk = released_chunk_head;
        released_chunk_head = released_chunk->next;
        std::destroy_at(released_chunk);
        
        return released_chunk;
    }

    if (next_unused_chunk == chunk_capacity)
    {
        return nullptr;
    }

    return shard_begin + next_unused_chunk++ * chunk_size;
}

bool Pool::release(void* pointer) noexcept
{
    if (!owns(pointer))
    {
        return false;
    }

    released_chunk_head = std::construct_at(static_cast<FreeNode*>(pointer), 
                                            FreeNode{released_chunk_head});

    return true;
}

bool Pool::owns(const void* pointer) const noexcept
{
    const auto address = reinterpret_cast<std::uintptr_t>(pointer);
    const auto beginning = reinterpret_cast<std::uintptr_t>(shard_begin);

    return (shard_begin != nullptr)                               && 
           (address >= beginning)                                 && 
           (address - beginning < next_unused_chunk * chunk_size) && 
           ((address - beginning) % chunk_size == 0);
}

bool Pool::isExhausted() const noexcept
{
    return released_chunk_head == nullptr && 
           next_unused_chunk == chunk_capacity;
}

}
