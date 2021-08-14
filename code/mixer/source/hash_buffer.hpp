#include "hash.hpp"

namespace dm
{
    struct hash_buffer
    {
        template<typename TYPE>
        void push_back(const TYPE& val)
        {
            const auto beginSize = _buffer.size();
            _buffer.write(val);
            const auto finalSize = _buffer.size();

            _hash = calc_hash_raw(_buffer.head() + beginSize, finalSize - beginSize);
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
            if (this == &that)
            {
                return true;
            }
            if (this->_hash != that._hash)
            {
                return false;
            }
            return this->_buffer == that._buffer;
        }

        size_t size() const
        {
            return _buffer.size();
        }

    private:
        size_t _hash = 0;
        serialization_buffer _buffer;
    };
}
