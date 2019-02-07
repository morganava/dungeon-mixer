#pragma once

namespace tabletopify
{
    size_t calc_hash(const void* buffer, size_t len, size_t seed);
    size_t calc_hash(const void* buffer, size_t len);
    size_t calc_hash(const std::string& str);

    struct string_hash
    {
        size_t operator()(const std::string& str) const
        {
            return calc_hash(str);
        }
    };

    struct path_hash
    {
        size_t operator()(const fs::path& p) const
        {
            return calc_hash(p.native());
        }
    };
}
