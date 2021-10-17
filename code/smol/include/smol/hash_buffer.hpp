#pragma once

#include <smol/serialization_buffer.hpp>

namespace smol
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

        void clear();
        void reset();
        bool operator==(const hash_buffer& that) const;
        size_t size() const;
    private:
        size_t hash() const;

        mutable size_t _hash = 0;
        serialization_buffer _buffer;
    };
}
