#pragma once

#include <smol/error.hpp>
#include <smol/serialization_buffer.hpp>
#include <smol/hash.hpp>

namespace smol
{
    template<typename T>
    class array
    {
    public:
        array() = default;
        array(const array& that)
        : array(that.size())
        {
            std::copy(that.begin(), that.end(), _data.get());
        }
        array(array&& that)
        : _size(that._size)
        , _data(std::move(that._data))
        {
            that.~array();
            new(&that) array();
        }

        explicit array(size_t N)
        : _size(N)
        , _data(std::make_unique<T[]>(N))
        { }

        array(std::initializer_list<T> il)
        : array(il.size())
        {
            const auto N = this->_size;
            for(size_t i = 0; i < N; ++i)
            {
                this->_data[i] = std::move(il.begin()[i]);
            }
        }

        array(const std::vector<T>& vec)
        : array(vec.size())
        {
            std::copy(vec.begin(), vec.end(), _data.get());
        }

        array& operator=(std::initializer_list<T> il)
        {
            const auto N = il.size();
            if(N == this->_size)
            {
                for(size_t i = 0; i < N; ++i)
                {
                    this->_data[i] = std::move(il.begin()[i]);
                }
            }
            else
            {
                this->~array();
                new (this) array(il);
            }

            return *this;
        }

        array& operator=(const array& that)
        {
            this->~array();
            new (this) array(that);
            return *this;
        }

        array concat(std::initializer_list<T> il) const
        {
            array retval(this->size() + il.size());
            // copy over this array's data
            std::copy(this->begin(), this->end(), retval._data.get());
            // copy over the new data
            std::copy(il.begin(), il.end(), retval._data.get() + this->size());

            return retval;
        }

        bool operator==(const array& that) const
        {
            if (this == &that) return true;
            const auto N = this->_size;
            if (N != that._size) return false;

            for(size_t i = 0; i < N; ++i)
            {
                if (this->_data[i] != that._data[i]) return false;
            }

            return true;
        }

        T& operator[](size_t idx)
        {
            SMOL_THROW_IF_FALSE(idx < _size);
            return _data[idx];
        }

        const T& operator[](size_t idx) const
        {
            SMOL_THROW_IF_FALSE(idx < _size);
            return _data[idx];
        }

        T* begin()
        {
            return _data.get();
        }

        T* end()
        {
            return this->begin() + _size;
        }

        T* const begin() const
        {
            return _data.get();
        }

        T* const end() const
        {
            return this->begin() + _size;
        }

        constexpr size_t size() const
        {
            return _size;
        }

        const T* data() const
        {
            return _data.get();
        }

        T* data()
        {
            return _data.get();
        }

    private:
        const size_t _size = 0;
        std::unique_ptr<T[]> _data;
    };

    template<typename T>
    void serialization_buffer_write(serialization_buffer& self, const array<T>& array)
    {
        self.write(array.size());
        if constexpr (std::is_trivially_copyable<T>::value)
        {
            self.write(reinterpret_cast<const uint8_t*>(array.data()), array.size() * sizeof(T));
        }
        else
        {
            for(const auto& val : array)
            {
                self.write(val);
            }
        }
    }

    template<typename T>
    bool serialization_buffer_read(serialization_buffer& self, array<T>& out)
    {
        size_t size;
        if (self.read(size))
        {
            array<T> retval(size);
            if constexpr (std::is_trivially_copyable<T>::value)
            {
                if (!self.read(reinterpret_cast<uint8_t*>(retval.data()), size * sizeof(T)))
                {
                    return false;
                }
            }
            else
            {
                for(size_t i = 0; i < size; ++i)
                {
                    T val;
                    if(self.read(val))
                    {
                        retval[i] = std::move(val);
                    }
                    else
                    {
                        return false;
                    }
                }
            }

            out = std::move(retval);
            return true;
        }
        return false;
    }

    template<typename T>
    size_t calc_hash(const smol::array<T>& a, size_t seed)
    {
        size_t retval = calc_hash(a.size(), seed);
        for(const T& val : a)
        {
            retval = calc_hash(std::forward<const T&>(val), retval);
        }
        return retval;
    }
}

namespace nlohmann
{
    template<typename T>
    void from_json(const nl::json& js, smol::array<T>& array)
    {
        SMOL_THROW_IF_FALSE(js.is_array());
        smol::array<T> retval(js.size());
        for(size_t i = 0; i < retval.size(); ++i)
        {
            retval[i] = js[i].get<T>();
        }
        array = std::move(retval);
    }
}