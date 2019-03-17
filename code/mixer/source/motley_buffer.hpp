#pragma once

#include "utf8_string.hpp"

namespace tabletopify
{
    struct motley_buffer
    {
        template<typename T>
        typename std::enable_if<!std::is_trivially_copyable<T>::value, void>::type
        push_back(const T& val);

        template<typename T>
        typename std::enable_if<std::is_trivially_copyable<T>::value, void>::type
        push_back(const T& val)
        {
            const auto offset = _raw.data() - _read_head;

            auto ptr = reinterpret_cast<const uint8_t*>(&val);
            _raw.insert(_raw.end(), ptr, ptr+sizeof(val));

            _read_head = _raw.data() + offset;
        }

        template<typename T>
        typename std::enable_if<!std::is_trivially_copyable<T>::value, bool>::type
        next(T& outval);

        template<typename T>
        typename std::enable_if<std::is_trivially_copyable<T>::value, bool>::type
        next(T& outval)
        {
            if (_read_head + sizeof(T) <= _raw.data() + _raw.size())
            {
                std::memcpy(&outval, _read_head, sizeof(T));
                _read_head += sizeof(T);
                return true;
            }
            return false;
        }

        void clear()
        {
            _raw.clear();
            _read_head = _raw.data();
        }

        void reset()
        {
            _read_head = _raw.data();
        }

        size_t size() const
        {
            return _raw.size();
        }

        bool operator==(const motley_buffer& that) const
        {
            if (this == &that)
            {
               return true;
            }
            if (this->size() != that.size())
            {
                return false;
            }

            return std::memcmp(this->_raw.data(), that._raw.data(), this->size()) == 0;
        }

    private:
        std::vector<uint8_t> _raw;
        uint8_t* _read_head = _raw.data();
    };

    class utf8_string;
    class utf8_string_view;

    template<>
    void motley_buffer::push_back<utf8_string>(const utf8_string& str);
    template<>
    bool motley_buffer::next<utf8_string_view>(utf8_string_view& sv);
}