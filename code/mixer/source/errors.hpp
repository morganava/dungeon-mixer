#pragma once
#include "macros.hpp"

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
}

#define MX_DEBUGBREAK(...) dm::debug_break()
#define MX_FAIL_FAST(...) dm::fail_fast(std::experimental::source_location() __VA_OPT__(,) __VA_ARGS__);
#define MX_ASSERT(X) dm::check_assertion(std::experimental::source_location(), X, #X);

// throwing  macros
#define MX_THROW_IF_FALSE(X) do { if(!(X)) { throw std::runtime_error("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " #X);} } while(0)
#define MX_THROW_IF_NULL(X) MX_THROW_IF_FALSE((X) != nullptr)
#define MX_THROW_IF_NOT_NULL(X) MX_THROW_IF_FALSE((X) == nullptr)
#define MX_THROW_MSG(...) throw std::runtime_error(fmt::format("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " __VA_ARGS__));
#define MX_THROW_MSG_IF_FALSE(X, ...) do { if (!(X)) { throw std::runtime_error(fmt::format("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " __VA_ARGS__)); } } while(0)
