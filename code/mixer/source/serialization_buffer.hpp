#pragma once

#include "utf8_string.hpp"

namespace tabletopify
{
    struct serialization_buffer
    {
        template<typename T>
        typename std::enable_if<!std::is_trivially_copyable<T>::value, void>::type
        write(const T& val);

        template<typename T>
        typename std::enable_if<std::is_trivially_copyable<T>::value, void>::type
        write(const T& val)
        {
            write(reinterpret_cast<const uint8_t*>(&val), sizeof(val));
        }

        void write(const uint8_t* data, size_t count);

        template<typename T>
        typename std::enable_if<!std::is_trivially_copyable<T>::value, bool>::type
        read(T& outval);

        template<typename T>
        typename std::enable_if<std::is_trivially_copyable<T>::value, bool>::type
        read(T& outval)
        {
            return read(reinterpret_cast<uint8_t*>(&outval), sizeof(outval));
        }

        bool read(uint8_t* out, size_t count);

        void clear()
        {
            _raw.clear();
            _read_head = _raw.data();
        }

        void reset()
        {
            _read_head = _raw.data();
        }

        bool finished_reading() const
        {
            return _read_head == (_raw.data() + _raw.size());
        }

        size_t size() const
        {
            return _raw.size();
        }

        bool operator==(const serialization_buffer& that) const
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
    void serialization_buffer::write<utf8_string>(const utf8_string& str);
    template<>
    bool serialization_buffer::read<utf8_string_view>(utf8_string_view& sv);
}