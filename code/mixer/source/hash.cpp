#include "hash.hpp"
#include "uuid.hpp"
#include "utf8_string.hpp"
#include "stylesheet.hpp"

namespace tabletopify
{
    template<size_t SIZE>
    size_t calc_hash_impl(const void* buffer, size_t len, size_t seed);

    template<>
    size_t calc_hash_impl<sizeof(uint32_t)>(const void* buffer, size_t len, size_t seed)
    {
        return ::XXH32(buffer, len, seed);
    }

    template<>
    size_t calc_hash_impl<sizeof(uint64_t)>(const void* buffer, size_t len, size_t seed)
    {
        return ::XXH64(buffer, len, seed);
    }

    // base functions

    size_t calc_hash_raw(const void* buffer, size_t len, size_t seed)
    {
        return calc_hash_impl<sizeof(size_t)>(buffer, len, seed);
    }

    size_t calc_hash_raw(const void* buffer, size_t len)
    {
        return calc_hash_raw(buffer, len, 0);
    }
}
using namespace tabletopify;

namespace std
{
    size_t hash<fs::path>::operator()(const fs::path& p) const
    {
        const auto& pathString = p.native();
        return calc_hash_raw(pathString.c_str(), pathString.size());
    }
}