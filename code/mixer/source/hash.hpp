#pragma once

namespace tabletopify
{
    size_t calc_hash_raw(const void* buffer, size_t len, size_t seed);
    size_t calc_hash_raw(const void* buffer, size_t len);

    template<typename T>
    struct hash
    {
        typename std::enable_if<std::is_trivially_copyable<T>::value, size_t>::type
        operator()(const T& val, size_t seed) const
        {
            return calc_hash_raw(&val, sizeof(T), seed);
        }
    };
}

template <> struct std::hash<fs::path>
{
    size_t operator()(const fs::path&) const;
};
