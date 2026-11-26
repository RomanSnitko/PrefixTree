#pragma once

#include <PoolAllocator.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

namespace tree
{

/*
    Tree based on a custom PoolAllocator.

    Instead of allocating every node separately via new,
    we allocate large memory blocks and construct nodes inside them.

    Tree responsible for:
    - building the tree structure;
    - inserting and searching words;
    - managing the lifetime of all memory blocks.

    PoolAllocator is responsible for raw memory allocation.
*/
class Tree
{
public:
    explicit Tree(MemoryManagement::PoolAllocator::Config allocator_config);

    ~Tree();

    Tree(const Tree&) = delete;
    Tree& operator=(const Tree&) = delete;

    /*
        O(L), L - word.length()

        Reuse existing nodes and create missing ones,
        mark the last node as the end of the word.
            true  -> new word inserted
            false -> word already exists
    */
    bool insert(std::string_view word);

    // O(L)
    [[nodiscard]] bool search(std::string_view word) const noexcept;

    /*
        Prefix search.

        prefix_tree.insert("apple");
        startsWith("apple") -> true
        startsWith("app")   -> true
        startsWith("abc")   -> false
    */
    [[nodiscard]] bool startsWith(std::string_view prefix) const noexcept;

private:
    static constexpr int ALPHABET_SIZE = 26;

    /*
        On x86-64:

        26 * 8 = 208 bytes for pointers
        1 byte for bool
        7 bytes of padding for alignment

        sizeof(TreeNode) = 216 bytes, alignof(TreeNode) = 8 bytes

        speed > memory -> fast child access O(1) :)

        Even if a node has only one child, it stores all 26 pointers.
        I thought about using a bitmap to save memory, but this implementation is mainly for learning.

        some trade-off:
        + direct child access, no additional search;
        + simple node structure;
        - a lot of memory;
        - fewer nodes fit into CPU cache.
    */
    struct TreeNode
    {
        /*
            Every node contains an array of 26 pointers.

            We dont store the character itself inside the node.
            Character is represented by its position in parent's array.

            children[0]  -> 'a'
            children[1]  -> 'b'
            children[25] -> 'z'

            index = symbol - 'a'

            Raw pointers are non-owning. All nodes are stored inside memory blocks, which are owned by Tree.
        */
        std::array<TreeNode*, ALPHABET_SIZE> children{};

        /*
            Separates an existing prefix from a complete word.

            After insert("apple"):
            root -> a -> p -> p -> l -> e(true)
            After insert("app"):
            root -> a -> p -> p(true) -> l -> e(true)

            just change this flag.
        */
        bool is_end_of_word{};
    };

    /*
        MemoryBlock contains:
        - unique_ptr<byte[]> which owns raw memory;
        - PoolAllocator which manages this memory.

        MemoryBlock
        raw memory [chunk][chunk][chunk]...
        PoolAllocator allocates chunks from this memory.
    */
    struct MemoryBlock;

    /*
        Allocation and construction are separate operations.

        1. Try to acquire raw memory from the last MemoryBlock.
        2. If the block is full or doesnt exist, create a new one.
        3. Acquire a chunk via PoolAllocator.
        4. Construct TreeNode via std::construct_at()
            (smth like safety placement new + constexpr use).
        5. Return pointer to the constr. obj.

        PoolAllocator::acquire() -> raw memory  (void*)
        std::construct_at()      -> live object (TreeNode*)

        Only the last block is checked because we dont
        remove individual nodes from the tree.

        [FULL][FULL][FULL][AVAILABLE]

        Previous blocks will never have free chunks again.
    */
    [[nodiscard]] TreeNode* createNode();

    /*
        Common traversal logic for search() and startsWith().

        Start from root and go through children using:
        current_node->children[symbol - 'a']

        If a path doesnt exist -> nullptr;
        Otherwise              -> pointer to the last node;
    */
    [[nodiscard]] const TreeNode* findNode(std::string_view text) const noexcept;

    /*
        Configuration used to create new memory blocks.

        shard_size       -> size of raw memory block
        min_chunk_size   -> minimum size class
        max_chunk_size   -> maximum size class
        fixed_chunk_size -> fixed size instead of size classes

        For example:
        {
            sizeof(TreeNode) = 216 bytes
            chunk_size       = 256 bytes (next power of 2)

            256 - 216 = 40 unused bytes per chunk.
            This is internal fragmentation.

            fixed_chunk_size = 216 eliminates this fragmentation for TreeNode on x86-64.
        }
    */
    MemoryManagement::PoolAllocator::Config allocator_config;

    /*
        Owns all memory blocks used to allocate tree nodes.

        Ownership hierarchy:
        Tree
            -> vector
            -> unique_ptr<MemoryBlock>
            -> unique_ptr<byte[]>

        TreeNode* pointers don't own memory.
        They only provide access to objects inside memory blocks.

        MemoryBlock and TreeNode addresses remain unchanged,
        because nodes store pointers to each other.

        Since TreeNode is trivially destructible,
        we can release entire memory blocks without calling
        the destructor of every individual node.

        Just deallocation instead of recursive tree traversal.
    */
    std::vector<std::unique_ptr<MemoryBlock>> memory_blocks;

    /*
        Entry point for all tree operations.
    */
    TreeNode* root{};
};

}