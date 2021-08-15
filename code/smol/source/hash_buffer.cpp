#include <smol/hash.hpp>
#include <smol/hash_buffer.hpp>

namespace smol
{
    void hash_buffer::clear()
    {
        _buffer.clear();
        _hash = 0;
    }

    void hash_buffer::reset()
    {
        _buffer.reset();
    }

    bool hash_buffer::operator==(const hash_buffer& that) const
    {
        if (this == &that) [[unlikely]]
        {
            return true;
        }
        else if (this->size() != that.size()) [[likely]]
        {
            return false;
        }
        else if (this->hash() != that.hash()) [[likely]]
        {
            return false;
        }
        else [[unlikely]]
        {
            return this->_buffer == that._buffer;
        }
    }

    size_t hash_buffer::size() const
    {
        return _buffer.size();
    }

    size_t hash_buffer::hash() const
    {
        if (_hash == 0)
        {
            _hash = smol::calc_hash_raw(_buffer.head(), _buffer.size());
        }
        return _hash;
    }
}
