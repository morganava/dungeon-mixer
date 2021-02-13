#include "logger.hpp"

namespace dm
{
	void fail_fast(std::experimental::source_location location, std::string&& msg)
	{
        g_logger.log<dm::log_level::error>(
            "Fail Fast: {} {}: {}",
            location.file_name(),
            location.function_name(),
            location.line());

        MX_DEBUGBREAK();
	}

	void check_assertion(std::experimental::source_location location, bool val, std::string_view valString)
	{
		if (!val)
		{
			g_logger.log<dm::log_level::error>(
				"Assertion Failure: {} {}:{} {}",
				location.file_name(),
				location.function_name(),
				location.line(),
				valString);

			MX_DEBUGBREAK();
		}
	}
}