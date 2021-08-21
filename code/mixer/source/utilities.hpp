#pragma once

namespace dm
{
    template<size_t N>
    struct framerate_calculator
    {
        void push_dt(double dt)
        {
            sum += dt - _dt_buffer[_idx];
            _dt_buffer[_idx] = dt;

            _idx = (_idx + 1) % N;
        }

        double framerate() const
        {
            return (double)N / sum;
        }
    private:
        double sum = 0.0;
        double _dt_buffer[N] = {};
        size_t _idx = 0;
    };

    typedef std::unique_ptr<::FILE> unique_file;

    struct memory_buffer
    {
        memory_buffer() = default;
        memory_buffer(const memory_buffer&) = delete;
        memory_buffer(memory_buffer&&) = default;

        memory_buffer(const fs::path&);

        const uint8_t* data() const
        {
            return _memory.get();
        }

        const uint8_t* begin() const
        {
            return _memory.get();
        }

        const uint8_t* end() const
        {
            return begin() + size();
        }

        constexpr size_t size() const
        {
            return _bytes;
        }
    private:
        std::unique_ptr<uint8_t[]> _memory;
        size_t _bytes;
    };

/*
    template<typename T>
    class range
    {
        range() = delete;
        range(const range&) = delete;
        range& operator=(const range&) = delete;
        range& operator=(range&&) = delete;
    public:
        constexpr range(range&&) = default;

        constexpr range(T from, T to) : range(from, to, static_cast<T>(from <= to ? 1 : -1))
        { }

        constexpr range(T from, T to, T inc)
        : from(std::move(from))
        , to(std::move(to))
        , inc(std::move(inc))
        { }

        class iterator
        {
            iterator() = delete;
            iterator& operator=(const iterator&) = delete;
            iterator& operator=(iterator&&) = delete;
        public:

            constexpr iterator(const iterator&) = default;
            constexpr iterator(iterator&&) = default;

            iterator(T first, T last, T inc)
            : first(first)
            , last(last)
            , inc(inc)
            , index(0)
            , count((size_t)((last - first) / inc) + 1)
            { }

            constexpr iterator& operator++()
            {
                increment();
                return *this;
            }

            constexpr iterator operator++(int)
            {
                auto retval = *this;
                increment();
                return retval;
            }

            constexpr bool operator==(const iterator& that) const
            {
                return index == that.index;
            }

            constexpr bool operator!=(const iterator& that) const
            {
                return index != that.index;
            }

            constexpr T operator*() const
            {
                return this->value();
            }

        private:
            constexpr void increment()
            {
                index = std::min(index + 1, count);
            }

            constexpr T value() const
            {
                if (index == 0) return first;
                if (index == (count - 1)) return last;
                return first + index * inc;
            }

            const T first;
            const T last;
            const T inc;
            size_t index;
            const size_t count;

            friend class range;
        };

        constexpr range::iterator begin()
        {
            return range::iterator(from, to, inc);
        }

        constexpr range::iterator end()
        {
            auto retval = begin();
            retval.index = retval.count;
            return retval;
        }
    private:

        const T from;
        const T to;
        const T inc;
    };
*/
}

namespace std
{
    template<>
    class default_delete<std::FILE>
    {
    public:
        void operator()(std::FILE* file)
        {
            std::fclose(file);
        }
    };
}
