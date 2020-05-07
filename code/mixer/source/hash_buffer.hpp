#include "motley_buffer.hpp"
#include "hash.hpp"
#include "utilities.hpp"

namespace tabletopify
{
    struct hash_buffer
    {
        template<typename TYPE>
        void push_back(const TYPE& val)
        {
            _buffer.push_back(val);
            _hash = calc_hash(val, _hash);
        }

        template<typename TYPE>
        bool next(TYPE& outval)
        {
            return _buffer.next<TYPE>(outval);
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
        motley_buffer _buffer;
    };
}
