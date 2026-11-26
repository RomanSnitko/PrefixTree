#include <tree/tree.hpp>

#include <algorithm>
#include <bit>
#include <stdexcept>

#include <type_traits>

namespace tree
{

struct Tree::MemoryBlock
{
    std::unique_ptr<std::byte[]> memory;
    MemoryManagement::PoolAllocator allocator;

    explicit MemoryBlock(const MemoryManagement::PoolAllocator::Config& config)
        : memory(std::make_unique_for_overwrite<std::byte[]>(config.shard_size)),
          allocator(std::span(memory.get(), config.shard_size), 
          config)
    {
        static_assert(alignof(TreeNode) <= alignof(std::max_align_t));
    }
};

Tree::Tree(MemoryManagement::PoolAllocator::Config config)
    : allocator_config(config)
{
    std::size_t chunk_size = 0;

    if (config.fixed_chunk_size != 0)
    {
        chunk_size = config.fixed_chunk_size;
    }
    else
    {
        chunk_size = std::max(config.min_chunk_size, std::bit_ceil(sizeof(TreeNode)));

        if (chunk_size > config.max_chunk_size)
        {
            throw std::invalid_argument("Pool max_chunk_size is too small for TreeNode");
        }
    }

    if (chunk_size < sizeof(TreeNode) || chunk_size > config.shard_size || chunk_size % alignof(TreeNode) != 0)
    {
        throw std::invalid_argument("Pool configuration cannot store tree nodes");
    }
}

Tree::~Tree()
{
    static_assert(std::is_trivially_destructible_v<TreeNode>);
}

Tree::TreeNode* Tree::createNode()
{
    if (!memory_blocks.empty())
    {
        if (auto* chunk = memory_blocks.back()->allocator.acquire(sizeof(TreeNode)))
        {
            return std::construct_at(static_cast<TreeNode*>(chunk));
        }
    }

    // Nodes are kept until destruction, so only the last block can have room.
    auto& memory_block = memory_blocks.emplace_back(std::make_unique<MemoryBlock>(allocator_config));
    auto* chunk = memory_block->allocator.acquire(sizeof(TreeNode));

    return std::construct_at(static_cast<TreeNode*>(chunk));
}

bool Tree::insert(std::string_view word)
{
    if (root == nullptr)
    {
        root = createNode();
    }

    auto* current_node = root;

    for (const auto symbol : word)
    {
        auto& child_node = current_node->children[symbol - 'a'];

        if (child_node == nullptr)
        {
            child_node = createNode();
        }

        current_node = child_node;
    }

    if (current_node->is_end_of_word)
    {
        return false;
    }

    current_node->is_end_of_word = true;
    return true;
}

const Tree::TreeNode* Tree::findNode(std::string_view text) const noexcept
{
    const auto* current_node = root;

    for (const auto symbol : text)
    {
        if (current_node == nullptr)
        {
            return nullptr;
        }

        current_node = current_node->children[symbol - 'a'];
    }

    return current_node;
}

bool Tree::search(std::string_view word) const noexcept
{
    const auto* current_node = findNode(word);

    return current_node != nullptr && current_node->is_end_of_word;
}

bool Tree::startsWith(std::string_view prefix) const noexcept
{
    return prefix.empty() || findNode(prefix) != nullptr;
}

}
