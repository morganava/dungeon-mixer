#include "logger.hpp"
#include "platform.hpp"

namespace dm
{
    void debug_break()
    {
        if constexpr(dm::current_compiler == compiler::gcc)
        {
            __builtin_trap();
        }
    }

	void fail_fast(std::experimental::source_location location, std::string&& msg)
	{
            MX_LOG_ERROR_BLOCKING(
                "Fail Fast: {} {}: {}",
                location.file_name(),
                location.function_name(),
                location.line(),
                std::move(msg));

            debug_break();
	}

	void check_assertion(std::experimental::source_location location, bool val, std::string_view valString)
	{
		if (!val) [[unlikely]]
		{
			MX_LOG_ERROR_BLOCKING(
				"Assertion Failure: {} {}:{} {}",
				location.file_name(),
				location.function_name(),
				location.line(),
				valString);

			debug_break();
		}
	}

    void throw_message(std::experimental::source_location location, std::string_view message)
    {
        MX_THROW_MSG_IMPL(location, message);
    }
}
