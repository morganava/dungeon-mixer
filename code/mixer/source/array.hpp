 #pragma once

#include "utilities.hpp"
#include "serialization_buffer.hpp"

namespace tabletopify
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
            MX_THROW_IF_FALSE(idx < _size);
            return _data[idx];
        }

        const T& operator[](size_t idx) const
        {
            MX_THROW_IF_FALSE(idx < _size);
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

    private:
        const size_t _size = 0;
        std::unique_ptr<T[]> _data;
    };

    template<typename T>
    void serialization_buffer_write(serialization_buffer& self, const array<T>& array)
    {
        self.write(array.size());
        for(const auto& val : array)
        {
            self.write(val);
        }
    }

    template<typename T>
    bool serialization_buffer_read(serialization_buffer& self, array<T>& out)
    {
        size_t size;
        if (self.read(size))
        {
            array<T> retval(size);
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
            out = std::move(retval);
            return true;
        }
        return false;
    }

    template<typename T>
    void from_json(const nl::json& js, tabletopify::array<T>& array)
    {
        MX_THROW_IF_FALSE(js.is_array());
        tabletopify::array<T> retval(js.size());
        for(size_t i = 0; i < retval.size(); ++i)
        {
            retval[i] = js[i].get<T>();
        }
        array = std::move(retval);
    }

    template<typename T>
    class array_view
    {
    public:
        constexpr array_view() noexcept = default;
        constexpr array_view(const array_view&) noexcept = default;
        constexpr array_view(T* begin, T* end)
        : _begin(begin)
        , _end(end)
        {
            MX_THROW_IF_FALSE(begin <= end);
        };

        constexpr array_view& operator=(const array_view&) noexcept = default;

        constexpr size_t size() const
        {
            return _end - _begin;
        }

        constexpr T* begin() noexcept
        {
            return _begin;
        }

        constexpr T* end() noexcept
        {
            return _end;
        }

        constexpr T& operator[](size_t idx)
        {
            MX_THROW_IF_FALSE((_begin + idx) < _end);
            return _begin[idx];
        }

        constexpr const T& operator[](size_t idx) const
        {
            MX_THROW_IF_FALSE((_begin + idx) < _end);
            return _begin[idx];
        }

    private:
        T* _begin;
        T* _end;
    };
};