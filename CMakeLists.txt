cmake_minimum_required(VERSION 3.20)

project(PrefixTree LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(pool_allocator
    allocator/src/Pool.cc
    allocator/src/PoolAllocator.cc
)
target_include_directories(pool_allocator PUBLIC allocator/include)
target_compile_features(pool_allocator PUBLIC cxx_std_20)

add_library(tree src/tree.cc)
target_include_directories(tree PUBLIC include)
target_link_libraries(tree PUBLIC pool_allocator)
target_compile_features(tree PUBLIC cxx_std_20)

add_executable(PrefixTree main.cc config/config.cc)
target_include_directories(PrefixTree PRIVATE config)
target_link_libraries(PrefixTree PRIVATE tree)

configure_file(config/config.json config/config.json COPYONLY)
