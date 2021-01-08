#pragma once

#define MX_DEBUGBREAK(...) __builtin_trap()

#define MX_STRINGIFY(X) MX_STRINGIFY_IMPL(X)
#define MX_STRINGIFY_IMPL(X) #X

#define __STR_LINE__ MX_STRINGIFY(__LINE__)

#define MX_FAIL_FAST(...) do {g_logger.log<dm::log_level::error>("Fail Fast : " __FILE__ ":" __STR_LINE__ " " __VA_ARGS__).wait(); MX_DEBUGBREAK(); } while(false)
#define MX_ASSERT(X) do { if(!(X)) { g_logger.log<dm::log_level::error>("Assertion Failure: " __FILE__ ":" __STR_LINE__ " " #X); MX_DEBUGBREAK();} } while(false)
#define MX_THROW_IF_FALSE(X) do { if(!(X)) { throw std::runtime_error("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " #X);} } while(0)
#define MX_THROW_IF_NULL(X) MX_THROW_IF_FALSE((X) != nullptr)
#define MX_THROW_IF_NOT_NULL(X) MX_THROW_IF_FALSE((X) == nullptr)
#define MX_THROW_MSG(...) throw std::runtime_error(fmt::format("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " __VA_ARGS__));
#define MX_THROW_MSG_IF_FALSE(X, ...) do { if (!(X)) { throw std::runtime_error(fmt::format("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " __VA_ARGS__)); } } while(0)

#define MX_FORCE_INLINE __attribute__ ((always_inline))
#define MX_NEVER_INLINE __attribute__ ((noinline))

#include "logger.hpp"

// extra print operators
std::ostream& operator<<(std::ostream& out, const sf::Vector3f& vec);

namespace dm
{
    // remove whitespace from head and tail of string
    std::string trim(const std::string& str);

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

    template<typename T, typename Deleter> class out_unique_ptr;

    template<typename T, typename Deleter>
    out_unique_ptr<T, Deleter> out(std::unique_ptr<T, Deleter>& ptr);

    template<typename T, typename Deleter>
    class out_unique_ptr
    {
    public:
        out_unique_ptr() = delete;
        out_unique_ptr(const out_unique_ptr&) = delete;
        out_unique_ptr(out_unique_ptr&&) = delete;
        out_unique_ptr& operator=(const out_unique_ptr&) = delete;
        out_unique_ptr& operator=(out_unique_ptr&&) = delete;

        ~out_unique_ptr()
        {
            u.reset(ptr);
        }

        operator T**()
        {
            return &ptr;
        }
    private:
        out_unique_ptr(std::unique_ptr<T, Deleter>& u) : u(u) {}
        friend out_unique_ptr out<T, Deleter>(std::unique_ptr<T, Deleter>&);

        T* ptr = nullptr;
        std::unique_ptr<T, Deleter>& u;
    };

    //
    // usage for legacy APIs with out pointer variables
    ///
    // void give_int(int** outInt)
    // std::unique_ptr<int> pint;
    // give_int(out(pint));
    // int val = *pint;
    //
    template<typename T, typename Deleter>
    out_unique_ptr<T, Deleter> out(std::unique_ptr<T, Deleter>& ptr)
    {
        return {ptr};
    }

    template<typename T>
    class range
    {
        range() = delete;
        range(const range&) = delete;
        range& operator=(const range&) = delete;
        range& operator=(range&&) = delete;
    public:
        constexpr range(range&&) = default;

        constexpr range(T from, T to) : range(from, to, from <= to ? 1 : -1)
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

	// list must be a string in the form "foo, bar, baz"
    template<size_t N>
    constexpr std::string_view string_list_get_entry(const char (&list)[N], size_t index)
    {
        auto head = std::begin(list);
        if (index > 0)
        {
            for(size_t k = 0 ; k < index; k++)
            {
                head = std::find(head + 1, std::end(list), ',');
            }
            // skip ", "
            head += 2;
        }
        auto tail = std::find(head, std::end(list), ',');
        return std::string_view(head, tail - head);
    }
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
