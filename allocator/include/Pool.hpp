#pragma once

#include <cstddef>
#include <span>

namespace MemoryManagement
{

class Pool
{
public:
    Pool() = default;
    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;

    Pool(Pool&& other) noexcept;
    Pool& operator=(Pool&& other) noexcept;

    void init(std::span<std::byte> shard, std::size_t chunk_size) noexcept;

    [[nodiscard]] void* acquire() noexcept;
    [[nodiscard]] bool release(void* pointer) noexcept;
    [[nodiscard]] bool isExhausted() const noexcept;

private:
    struct FreeNode
    {
        FreeNode* next{};
    };

    void swap(Pool& other) noexcept;
    [[nodiscard]] bool owns(const void* pointer) const noexcept;

    std::byte* shard_begin{};
    std::size_t chunk_size{};
    std::size_t chunk_capacity{};
    std::size_t next_unused_chunk{};
    FreeNode* released_chunk_head{};
};

}
