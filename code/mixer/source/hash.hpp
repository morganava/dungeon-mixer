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
            return tabletopify::calc_hash(str.data(), str.size());
        }
    };
}

