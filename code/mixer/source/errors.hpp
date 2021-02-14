#pragma once
#include "macros.hpp"

#define MX_DEBUGBREAK(...) dm::debug_break()
#define MX_FAIL_FAST(...) dm::fail_fast(std::experimental::source_location::current() __VA_OPT__(,) __VA_ARGS__);
#define MX_ASSERT(X) dm::check_assertion(std::experimental::source_location::current(), X, #X);

// throwing  macros
#define MX_THROW_MSG_IMPL(LOC, MSG)\
    throw \
        std::runtime_error(\
            fmt::format(\
                "Runtime Failure: {} {}:{} {}",\
                LOC.file_name(),\
                LOC.function_name(),\
                LOC.line(),\
                MSG))

#define MX_THROW_IF_FALSE(X) dm::throw_if_false(std::experimental::source_location::current(), X, #X)
#define MX_THROW_IF_NULL(X) MX_THROW_IF_FALSE((X) != nullptr)
#define MX_THROW_IF_NOT_NULL(X) MX_THROW_IF_FALSE((X) == nullptr)
#define MX_THROW_MSG(...) throw std::runtime_error(fmt::format("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " __VA_ARGS__));
#define MX_THROW_MSG_IF_FALSE(X, ...) do { if (!(X)) { throw std::runtime_error(fmt::format("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " __VA_ARGS__)); } } while(0)

namespace dm
{
    void debug_break();

    void fail_fast(std::experimental::source_location location, std::string&& msg);
    template<typename... ARGS>
    void fail_fast(std::experimental::source_location location)
    {
        fail_fast(location, {});
    }
    template<size_t N, typename... ARGS>
    void fail_fast(std::experimental::source_location location, char const(&format)[N], ARGS... args)
    {
        fail_fast(location, fmt::format(format, std::forward<ARGS>(args)...));
    }

    void check_assertion(std::experimental::source_location location, bool val, std::string_view valString);

    void throw_message(std::experimental::source_location location, std::string_view message);

    constexpr void throw_if_false(std::experimental::source_location location, bool val, std::string_view valString)
    {
        if (!val) [[unlikely]]
        {
            MX_THROW_MSG_IMPL(location, fmt::format("expected true expression: {}", valString));
        }
    }
    constexpr void throw_if_true(std::experimental::source_location location, bool val, std::string_view valString)
    {
        if (val) [[unlikely]]
        {
            MX_THROW_MSG_IMPL(location, fmt::format("expected false expression: {}", valString));
        }
    }
    constexpr void throw_if_null(std::experimental::source_location location, void* ptr, std::string_view ptrString)
    {
        if (ptr == nullptr) [[unlikely]]
        {
            MX_THROW_MSG_IMPL(location, fmt::format("expected non-null pointer: {}", ptrString));
        }
    }
}