#include "uuid.hpp"
#include "utf8_string.hpp"

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

    size_t calc_hash(const void* buffer, size_t len, size_t seed)
    {
        return calc_hash_impl<sizeof(size_t)>(buffer, len, seed);
    }

    size_t calc_hash(const void* buffer, size_t len)
    {
        return calc_hash(buffer, len, 0);
    }

    size_t calc_hash(const std::string& str)
    {
        return calc_hash(str.data(), str.size());
    }

    // UUID
    size_t uuid_hash::operator()(const tabletopify::uuid& id) const
    {
        return calc_hash(id.bytes, sizeof(id.bytes));
    }

    // utf8_string
    size_t utf8_string_hash::operator()(const utf8_string& str) const
    {
        return calc_hash(str.c_str(), str.size());
    }
}

