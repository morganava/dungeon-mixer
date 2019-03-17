#pragma once

namespace tabletopify
{
    size_t calc_hash_raw(const void* buffer, size_t len, size_t seed);
    size_t calc_hash_raw(const void* buffer, size_t len);
}

template <> struct std::hash<fs::path>
{
    size_t operator()(const fs::path&) const;
};
