#include "utf8_string.hpp"
#include "hash_buffer.hpp"
#include "utilities.hpp"
#include "hash.hpp"

namespace tabletopify
{
    // template<>
    // void hash_buffer::push_back<utf8_string>(const utf8_string& str)
    // {
    //     push_back<size_t>(str.size());
    //     push_back(str.c_str(), str.size());
    // }

    // template<>
    // bool hash_buffer::next<utf8_string_view>(utf8_string_view& sv)
    // {
    //     size_t len = 0;
    //     if (next<size_t>(len))
    //     {
    //         if (_read_head + len <= _raw.data() + _raw.size())
    //         {
    //             sv = utf8_string_view {
    //                 reinterpret_cast<const utf8proc_uint8_t*>(_read_head),
    //                 reinterpret_cast<const utf8proc_uint8_t*>(_read_head) + len
    //             };
    //             _read_head += len;
    //             return true;
    //         }
    //     }
    //     return false;
    // }

    // void hash_buffer::push_back(const void* pVal, size_t valSize)
    // {
    //     MX_ASSERT(_raw.data() == _read_head);

    //     // copy into raw buffer
    //     auto ptr = reinterpret_cast<const uint8_t*>(pVal);
    //     _raw.insert(_raw.end(), ptr, ptr+valSize);
    //     // update read head in case _raw resized internally
    //     _read_head = _raw.data();

    //     // update the hash
    //     _hash = calc_hash_raw(ptr, valSize, _hash);
    // }
}