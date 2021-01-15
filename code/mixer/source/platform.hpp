#pragma once

#include "utilities.hpp"
#include "enums.hpp"

// todo: actually detect the platform we're on one we're cross-platform
#define MX_PLATFORM_LINUX
// #define MX_PLATFORM_WINDOWS
// #define MX_PLATOFRM_MACOS

namespace dm
{
	MX_MAKE_ENUM(platform, size_t,
		windows,
		linux,
		macos
	);

	constexpr platform_t current_platform = []() -> platform_t
	{
	#ifdef MX_PLATFORM_LINUX
		return platform::linux;
	#endif
	#ifdef MX_PLATFORM_WINDOWS
		return platform::windows;
	#endif
	#ifdef MX_PLATOFRM_MACOS
		return platform::macos;
	#endif
		return platform::invalid;
	}();
}