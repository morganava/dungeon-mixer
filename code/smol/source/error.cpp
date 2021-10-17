#include <smol/macros.hpp>
#include <smol/platform.hpp>

namespace smol
{
    static failure_callback_t on_failure = nullptr;

    void register_failure_callback(failure_callback_t onFailure)
    {
        smol::on_failure = onFailure;
    }

    void debug_break()
    {
        if constexpr(current_compiler == compiler::gcc)
        {
            __builtin_trap();
        }
    }

    SMOL_NEVER_INLINE
    void fail_fast(std::experimental::source_location location, std::string_view msg)
    {
        if (on_failure)
        {
            on_failure(
                fmt::format(
                    "Fail Fast: {} {}:{} {}",
                    location.file_name(),
                    location.function_name(),
                    location.line(),
                    msg));
        }

        debug_break();
    }

    SMOL_NEVER_INLINE
    void check_assertion(std::experimental::source_location location, bool val, std::string_view valString)
    {
        if (!val) [[unlikely]]
        {
            if (on_failure)
            {
                on_failure(
                    fmt::format(
                        "Assertion Failure: {} {}:{} {}",
                        location.file_name(),
                        location.function_name(),
                        location.line(),
                        valString));
            }

            debug_break();
        }
    }

    SMOL_NEVER_INLINE
    [[noreturn]]void throw_message(std::experimental::source_location location, const std::string& message)
    {
        throw std::runtime_error(
                fmt::format(
                    "Runtime Failure: {} {}:{} {}",
                    location.file_name(),
                    location.function_name(),
                    location.line(),
                    message));
    }
}
