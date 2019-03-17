#pragma once

namespace tabletopify
{
    size_t calc_hash_raw(const void* buffer, size_t len, size_t seed);
    size_t calc_hash_raw(const void* buffer, size_t len);

    // non-pod types
    size_t calc_hash(const std::string& str, size_t seed);
    size_t calc_hash(const class utf8_string& str, size_t seed);

    // generic method for trivally copyable types
    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value, size_t>::type
    calc_hash(const T& val, size_t seed)
    {
        return calc_hash_raw(&val, sizeof(T), seed);
    }

    // template function without seed
    template<typename T>
    size_t calc_hash(const T& val)
    {
        return calc_hash(val, size_t(0));
    }
}

template <> struct std::hash<fs::path>
{
    size_t operator()(const fs::path&) const;
};
