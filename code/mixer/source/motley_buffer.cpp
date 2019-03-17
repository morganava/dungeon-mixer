#include "utf8_string.hpp"
#include "motley_buffer.hpp"

namespace tabletopify
{
    template<>
    void motley_buffer::push_back<utf8_string>(const utf8_string& str)
    {
        const auto offset = _raw.data() - _read_head;

        size_t size = str.size();
        auto ptr = reinterpret_cast<const uint8_t*>(&size);
        _raw.insert(_raw.end(), ptr, ptr+sizeof(size_t));
        _raw.insert(_raw.end(), str.c_str(), str.c_str() + str.size());

        _read_head = _raw.data() + offset;
    }

    template<>
    bool motley_buffer::next<utf8_string_view>(utf8_string_view& sv)
    {
        size_t len = 0;
        if (next<size_t>(len))
        {
            if (_read_head + len <= _raw.data() + _raw.size())
            {
                sv = utf8_string_view {
                    reinterpret_cast<const utf8proc_uint8_t*>(_read_head),
                    reinterpret_cast<const utf8proc_uint8_t*>(_read_head) + len
                };
                _read_head += len;
                return true;
            }
        }
        return false;
    }
}