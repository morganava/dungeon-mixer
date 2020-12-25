#pragma once

#define MX_DEBUGBREAK(...) __builtin_trap()

#define MX_STRINGIFY(X) MX_STRINGIFY_IMPL(X)
#define MX_STRINGIFY_IMPL(X) #X

#define __STR_LINE__ MX_STRINGIFY(__LINE__)

#define MX_FAIL_FAST(...) do {tabletopify::get_logger()->blocking_log(tabletopify::log_level::error, "Fail Fast : " __FILE__ ":" __STR_LINE__ " " __VA_ARGS__); MX_DEBUGBREAK(); } while(false);
#define MX_ASSERT(X) do {bool result = (X); if(!result) { tabletopify::get_logger()->blocking_log(tabletopify::log_level::error, "Assertion Failure: " __FILE__ ":" __STR_LINE__ " " #X); MX_DEBUGBREAK();} } while(false);
#define MX_STATIC_ASSERT(X) static_assert(X, MX_STRINGIFY(X))
#define MX_THROW_IF_FALSE(X) do {bool result = (X); if(!result) {throw std::runtime_error("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " #X);} } while(0);
#define MX_THROW_IF_NULL(X) MX_THROW_IF_FALSE((X) != nullptr)
#define MX_THROW_IF_NOT_NULL(X) MX_THROW_IF_FALSE((X) == nullptr)
#define MX_THROW_MSG(...) throw std::runtime_error(fmt::format("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " __VA_ARGS__));
#define MX_THROW_MSG_IF_FALSE(X, ...) do { bool result = (X); if (!result) { throw std::runtime_error(fmt::format("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " __VA_ARGS__)); } } while(0)

#define MX_FORCE_INLINE __attribute__ ((always_inline))
#define MX_NEVER_INLINE __attribute__ ((noinline))

#include "logger.hpp"

// extra print operators
std::ostream& operator<<(std::ostream& out, const sf::Vector3f& vec);

namespace tabletopify
{
    // remove whitespace from head and tail of string
    std::string trim(const std::string& str);

    fs::path open_dialog(const char* filterList);
    fs::path save_dialog(const char* filterList);

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
