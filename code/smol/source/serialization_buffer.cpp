#include <smol/serialization_buffer.hpp>

namespace smol
{
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
}