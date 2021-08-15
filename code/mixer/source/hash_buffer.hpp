#pragma once

namespace dm
{
    struct hash_buffer
    {
        template<typename TYPE>
        void push_back(const TYPE& val)
        {
            _buffer.write(val);
        }

        template<typename TYPE>
        bool next(TYPE& outval)
        {
            return _buffer.read<TYPE>(outval);
        }

        void clear()
        {
            _buffer.clear();
            _hash = 0;
        }

        void reset()
        {
            _buffer.reset();
        }

        bool operator==(const hash_buffer& that) const
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

        size_t size() const
        {
            return _buffer.size();
        }

        size_t hash() const
        {
            if (_hash == 0)
            {
                _hash = smol::calc_hash_raw(_buffer.head(), _buffer.size());
            }
            return _hash;
        }

    private:
        mutable size_t _hash = 0;
        serialization_buffer _buffer;
    };
}
