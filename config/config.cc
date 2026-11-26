#include "config.hpp"

#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{

[[nodiscard]] std::size_t readConfigValue(const std::string& json, const std::string& field)
{
    const auto field_position  = json.find('"' + field + '"');
    const auto value_separator = json.find(':', field_position);

    if (value_separator != std::string::npos)
    {
        std::istringstream value_stream(json.substr(value_separator + 1));
        std::size_t value;

        if (value_stream >> value)
        {
            return value;
        }
    }

    throw std::runtime_error("Cannot read allocator config field: " + field);
}

}

MemoryManagement::PoolAllocator::Config readAllocatorConfig(const std::filesystem::path& path)
{
    std::ifstream config_file(path);

    if (!config_file)
    {
        throw std::runtime_error("Cannot open allocator config: " + path.string());
    }

    // The config is a JSON object with four integer fields.
    const std::string json{std::istreambuf_iterator<char>(config_file), {}};

    return {
        .shard_size       = readConfigValue(json, "shard_size"),
        .min_chunk_size   = readConfigValue(json, "min_chunk_size"),
        .max_chunk_size   = readConfigValue(json, "max_chunk_size"),
        .fixed_chunk_size = readConfigValue(json, "fixed_chunk_size"),
    };
}
