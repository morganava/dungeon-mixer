 #pragma once

namespace tabletopify
{
    template<typename T>
    class array
    {
    public:
        array(size_t N)
        : _size(N)
        , _data(std::make_unique<T[]>(N))
        { }

        array(std::initializer_list<T> il)
        : array(il.size())
        {
            std::copy(il.begin(), il.end(), _data.get());
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
            return _data.get() + _size;
        }

        constexpr size_t size() const
        {
            return _size;
        }

    private:
        const size_t _size;
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