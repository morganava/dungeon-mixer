#pragma once
#include <smol/macros.hpp>

#define SMOL_DEBUGBREAK(...) smol::debug_break()
#define SMOL_FAIL_FAST(...) smol::fail_fast(std::experimental::source_location::current() __VA_OPT__(,) __VA_ARGS__);
#define SMOL_ASSERT(X) smol::check_assertion(std::experimental::source_location::current(), X, #X);

#define SMOL_THROW_IF_FALSE(X) smol::throw_if_false(std::experimental::source_location::current(), X, #X)
#define SMOL_THROW_IF_TRUE(X) smol::throw_if_true(std::experimental::source_location::current(), X, #X)
#define SMOL_THROW_IF_NULL(X) smol::throw_if_null(std::experimental::source_location::current(), X, #X)
#define SMOL_THROW_IF_NOT_NULL(X) smol::throw_if_not_null(std::experimental::source_location::current(), X, #X)
#define SMOL_THROW_MSG(...) smol::throw_message(std::experimental::source_location::current(), fmt::format(__VA_ARGS__))
#define SMOL_THROW_MSG_IF_FALSE(X, ...) do { if (!(X)) { SMOL_THROW_MSG(__VA_ARGS__); } } while(0)

namespace smol
{
    // called when assertion fails or fail_fast is hit
    typedef void(*failure_callback_t)(std::string_view);
    void register_failure_callback(failure_callback_t);

    void debug_break();

    void fail_fast(std::experimental::source_location location, std::string_view msg);
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
    [[noreturn]] void throw_message(std::experimental::source_location location, const std::string& message);

    constexpr void throw_if_false(std::experimental::source_location location, bool val, std::string_view valString)
    {
        if (!val) [[unlikely]]
        {
            throw_message(location, fmt::format("expected true expression: {}", valString));
        }
    }
    constexpr void throw_if_true(std::experimental::source_location location, bool val, std::string_view valString)
    {
        if (val) [[unlikely]]
        {
            throw_message(location, fmt::format("expected false expression: {}", valString));
        }
    }

    constexpr void throw_if_null(std::experimental::source_location location, auto ptr, std::string_view ptrString)
    {
        if (ptr == nullptr) [[unlikely]]
        {
            throw_message(location, fmt::format("expected non-null pointer: {}", ptrString));
        }
    }

    constexpr void throw_if_not_null(std::experimental::source_location location, auto ptr, std::string_view ptrString)
    {
        if (ptr != nullptr) [[unlikely]]
        {
            throw_message(location, fmt::format("expected null pointer: {}", ptrString));
        }
    }
}