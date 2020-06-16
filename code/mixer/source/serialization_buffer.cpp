#include "utf8_string.hpp"
#include "serialization_buffer.hpp"

namespace tabletopify
{
    template<>
    void serialization_buffer::write<utf8_string>(const utf8_string& str)
    {
        const auto offset = _raw.data() - _read_head;

        size_t size = str.size();
        auto ptr = reinterpret_cast<const uint8_t*>(&size);
        _raw.insert(_raw.end(), ptr, ptr + sizeof(size_t));
        _raw.insert(_raw.end(), str.c_str(), str.c_str() + str.size());

        _read_head = _raw.data() + offset;
    }

    void serialization_buffer::write(const uint8_t* data, size_t count)
    {
        // save off the offset because _raw's backing buffer can be reallocated on growth
        const auto offset = _raw.data() - _read_head;

        // save off bytes
        _raw.insert(_raw.end(), data, data + count);

        // recalculate read head
        _read_head = _raw.data() + offset;
    }

    bool serialization_buffer::read(uint8_t* out, size_t count)
    {
        if (_read_head + count <= _raw.data() + _raw.size())
        {
            std::memcpy(out, _read_head, count);
            _read_head += count;
            return true;
        }
        return false;
    }

    template<>
    bool serialization_buffer::read<utf8_string_view>(utf8_string_view& sv)
    {
        size_t len = 0;
        if (read<size_t>(len))
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