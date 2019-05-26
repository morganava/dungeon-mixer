#pragma once

#include "logger.hpp"

#define MX_DEBUGBREAK(...) __builtin_trap()

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)
#define __STR_LINE__ STRINGIFY(__LINE__)

#define MX_FAIL_FAST(...) do {tabletopify::get_logger()->blocking_log(LogLevel_Error, "Fail Fast : " __FILE__ ":" __STR_LINE__ " " __VA_ARGS__); MX_DEBUGBREAK(); } while(false);
#define MX_ASSERT(X) do {bool result = (X); if(!result) { tabletopify::get_logger()->blocking_log(LogLevel_Error, "Assertion Failure: " __FILE__ ":" __STR_LINE__ " " #X); MX_DEBUGBREAK();} } while(false);
#define MX_STATIC_ASSERT(X) static_assert(X, STRINGIFY(X))
#define MX_THROW_IF_FALSE(X) do {bool result = (X); if(!result) {throw std::runtime_error("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " #X);} } while(0);
#define MX_THROW_IF_NULL(X) MX_THROW_IF_FALSE((X) != nullptr)
#define MX_THROW_IF_NOT_NULL(X) MX_THROW_IF_FALSE((X) == nullptr)
#define MX_THROW_MSG(...) throw std::runtime_error(fmt::format("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " __VA_ARGS__));
#define MX_THROW_MSG_IF_FALSE(X, ...) do { bool result = (X); if (!result) { throw std::runtime_error(fmt::format("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " __VA_ARGS__)); } } while(0)


#define MX_TRACE(...) fprintf(stderr, __FILE__"(" __STR_LINE__ "): " __VA_ARGS__); fputs("\n", stderr);

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
