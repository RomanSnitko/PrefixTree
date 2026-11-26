#include <tree/tree.hpp>

#include "config.hpp"

#include <exception>
#include <iostream>

int main(int argument_count, char** arguments)
try
{
    const auto* config_path = argument_count > 1 ? arguments[1] : "config/config.json";
    tree::Tree prefix_tree(readAllocatorConfig(config_path));
    prefix_tree.insert("apple");

    std::cout << std::boolalpha;
    std::cout << "search(apple): " << prefix_tree.search("apple") << '\n';
    std::cout << "search(app): " << prefix_tree.search("app") << '\n';
    std::cout << "startsWith(app): " << prefix_tree.startsWith("app") << '\n';
}
catch (const std::exception& error)
{
    std::cerr << error.what() << '\n';
    return 1;
}
