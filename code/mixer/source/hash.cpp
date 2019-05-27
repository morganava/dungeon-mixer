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

    // specializations

    size_t calc_hash(const std::string& str, size_t seed)
    {
        return calc_hash_raw(str.c_str(), str.size(), seed);
    }

    size_t calc_hash(const utf8_string& str, size_t seed)
    {
        return calc_hash_raw(str.c_str(), str.size(), seed);
    }
}
using namespace tabletopify;

namespace std
{
    size_t hash<fs::path>::operator()(const fs::path& p) const
    {
        return calc_hash(p.native());
    }

    size_t hash<utf8_string>::operator()(const utf8_string& str) const
    {
        return calc_hash(str);
    }

    size_t hash<uuid>::operator()(const uuid& id) const
    {
        return calc_hash(id.bytes);
    }

    size_t hash<selector>::operator()(const selector& sel) const
    {
        auto retval = calc_hash(sel.type);
        for(auto& currentTag : sel.tags)
        {
            retval = calc_hash(currentTag, retval);
        }
        retval = calc_hash(sel.state, retval);
        return retval;
    }
}