#pragma once

namespace smol
{
    // non-POD read and write methods for types to override
    class serialization_buffer;
    void serialization_buffer_write(serialization_buffer&, const auto&);
    bool serialization_buffer_read(serialization_buffer&, auto&);

    class serialization_buffer
    {
    public:
        // Write Methods

        // write non-POD T
        template<typename T>
        typename std::enable_if<!std::is_trivially_copyable<T>::value, void>::type
        write(const T& val)
        {
            serialization_buffer_write(*this, val);
        }

        // write POD T
        template<typename T>
        typename std::enable_if<std::is_trivially_copyable<T>::value, void>::type
        write(const T& val)
        {
            write(reinterpret_cast<const uint8_t*>(&val), sizeof(val));
        }

        // write array of bytes
        void write(const uint8_t* data, size_t count);

        // write array of POD T
        template<typename T>
        typename std::enable_if<std::is_trivially_copyable<T>::value, void>::type
        write(const T* data, size_t count)
        {
            write(reinterpret_cast<const uint8_t*>(data), count * sizeof(T));
        }

        // Read Methods

        // read non-POD T
        template<typename T>
        typename std::enable_if<!std::is_trivially_copyable<T>::value, bool>::type
        read(T& outval)
        {
            return serialization_buffer_read(*this, outval);
        }

        // read POD T
        template<typename T>
        typename std::enable_if<std::is_trivially_copyable<T>::value, bool>::type
        read(T& outval)
        {
            return read(reinterpret_cast<uint8_t*>(&outval), sizeof(outval));
        }

        // read array of bytes
        bool read(uint8_t* out, size_t count);

        // read array of POD T
        template<typename T>
        typename std::enable_if<std::is_trivially_copyable<T>::value, bool>::type
        read(T* out, size_t count)
        {
            return read(reinterpret_cast<uint8_t*>(out), count * sizeof(T));
        }

        // read array of non-POD T
        template<typename T>
        typename std::enable_if<!std::is_trivially_copyable<T>::value, bool>::type
        read(T* out, size_t count)
        {
            for(size_t i = 0; i < count; ++i)
            {
                if (!read(out[i]))
                {
                    return false;
                }
            }
            return true;
        }

        // empty out buffer
        void clear()
        {
            _raw.clear();
            reset();
        }

        // reset read to begining
        void reset()
        {
            _read_head = _raw.data();
        }

        const uint8_t* read_head() const
        {
            return _read_head;
        }

        const uint8_t* head() const
        {
            return _raw.data();
        }

        bool skip(size_t count)
        {
            if (_read_head + count < head() + size())
            {
                _read_head += count;
                return true;
            }
            return false;
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
}