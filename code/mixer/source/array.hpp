 #pragma once

#include "utilities.hpp"

namespace tabletopify
{
    template<typename T>
    class array
    {
    public:
        array() = default;
        array(array&& that)
        : _size(that._size)
        , _data(std::move(that._data))
        {
            that._size = 0;
        }

        array(size_t N)
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
            if(N != this->_size)
            {
                this->_size = N;
                this->_data = std::make_unique<T[]>(N);
            }

            for(size_t i = 0; i < N; ++i)
            {
                this->_data[i] = std::move(il.begin()[i]);
            }

            return *this;
        }

/*
        array& operator=(const array& that)
        {

        }
*/
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
        size_t _size = 0;
        std::unique_ptr<T[]> _data;
    };

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