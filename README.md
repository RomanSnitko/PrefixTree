# PrefixTree

Just another prefix tree implemented in C++ using a custom pool allocator.

The main goal of this project is to learn how data structures, memory management and CPU architecture affect performance in C++.

The tree supports lowercase English letters (`a-z`), word insertion, exact search and prefix search.

## Architecture

```text
Tree
  -> MemoryBlock (owns raw memory)
       -> PoolAllocator (size classes and shards)
            -> Pool (chunk allocation and reuse)
```

The tree is responsible for its structure and algorithms. The allocator is responsible for raw memory allocation and knows nothing about tree nodes.

## Engineering notes

### 1. Trie and time complexity

Words with the same prefix share the same nodes. A flag separates a complete word from an existing prefix.

Each node contains an array of 26 pointers. The character itself is not stored in the node. It is represented by the position in the parent's array:

```cpp
index = symbol - 'a';
```

| Operation | Time complexity |
| --- | --- |
| Child access | O(1) |
| Insertion | O(L)* |
| Exact search | O(L) |
| Prefix search | O(L) |

`L` is the length of the input string. Space complexity is O(N), where `N` is the number of nodes (with a fixed alphabet).

*Insertion also includes the cost of allocating new memory blocks when needed.

### 2. Memory layout and trade-offs

On a typical x86-64 system:
```text
26 pointers * 8 bytes = 208 bytes
bool                  =   1 byte
padding               =   7 bytes
---------------------------------
sizeof(TreeNode)      = 216 bytes
alignof(TreeNode)     =   8 bytes
```

The current implementation favors simple and direct child access over memory efficiency.

Advantages: constant-time child access, simple node structure and no additional search among children.

Disadvantages: every node stores 26 pointers even if it has only one child. This increases memory consumption and the working set, so fewer nodes fit into the CPU cache.

### 3. Pool allocator

Instead of calling `new` for every node, the tree allocates large blocks of raw memory and constructs nodes inside them.

The allocator uses two mechanisms:

- Bump allocation: new chunks are calculated using `base + index * chunk_size`.
- Intrusive free list: released chunks store links to other free chunks inside their own memory. No separate allocation is needed for the list.

`Pool` manages chunks of the same size. `PoolAllocator` manages size classes and available shards. It uses `std::bit_ceil` and `std::bit_width` to calculate chunk sizes and class indices.

The normal acquire/release path is O(1) for a valid configuration. The tree itself uses only the allocation path because individual nodes are never removed.

### 4. Internal fragmentation

With the default configuration, a 216-byte node is placed in a 256-byte chunk (the next power of two).

| Metric | Default size class | Fixed chunk size |
| --- | ---: | ---: |
| Chunk size | 256 B | 216 B |
| Unused bytes per chunk | 40 B | 0 B |
| Nodes per 256 KiB block | 1024 | 1213 |

These calculations assume the x86-64 node layout and exclude allocator metadata. Fixed-size chunks can be enabled through `fixed_chunk_size` in `config/config.json`. The size must satisfy the node's size and alignment requirements.

### 5. Ownership and object lifetime

Memory allocation and object construction are separate operations:

```cpp
void* chunk = allocator.acquire(sizeof(TreeNode));
auto* node = std::construct_at(static_cast<TreeNode*>(chunk));
```

All nodes live until the tree is destroyed. Since `TreeNode` is trivially destructible, entire memory blocks can be released without recursively visiting every node. This requirement is checked with `static_assert`.

### 6. CPU cache and data locality

Sequential allocation places newly created nodes close to each other in memory. This can improve spatial locality and reduce calls to the general-purpose allocator.

However, a Trie still uses pointer chasing: the CPU must load the address of the next node before it can access that node. These dependent memory accesses can become expensive when the tree no longer fits in the CPU cache.

A smaller node layout can reduce the working set and potentially reduce cache and TLB misses. Big-O complexity alone does not describe these hardware costs.

## Usage

```cpp
tree::Tree prefix_tree(readAllocatorConfig("config/config.json"));

prefix_tree.insert("apple");

prefix_tree.search("apple");   // true
prefix_tree.search("app");     // false
prefix_tree.startsWith("app"); // true
```
