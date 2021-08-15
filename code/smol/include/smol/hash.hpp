#pragma once

namespace smol
{
    size_t calc_hash_raw(const void* buffer, size_t len, size_t seed);
    size_t calc_hash_raw(const void* buffer, size_t len);

    // trivial data type
    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value, size_t>::type
    calc_hash(const T& obj, size_t seed)
    {
        return calc_hash_raw(&obj, sizeof(obj), seed);
    }

    // specializations
    size_t calc_hash(const fs::path&, size_t seed);


    // auto-forward no-seed variant
    template<typename T>
    size_t calc_hash(const T& obj)
    {
        return calc_hash(std::forward<const T&>(obj), size_t(0));
    }
}

// define a std::hash in terms of smol::calc_hash()
#define SMOL_MAKE_STD_HASH(TYPE)\
template<> struct std::hash<TYPE>\
{\
    size_t operator()(const TYPE& obj) const\
    {\
        return smol::calc_hash(std::forward<const TYPE&>(obj), size_t(0));\
    }\
}

SMOL_MAKE_STD_HASH(fs::path);
