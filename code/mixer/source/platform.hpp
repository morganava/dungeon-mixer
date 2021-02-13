#pragma once

#include "utilities.hpp"
#include "enums.hpp"

// todo: actually detect the platform we're on one we're cross-platform
#define MX_PLATFORM_LINUX
// #define MX_PLATFORM_WINDOWS
// #define MX_PLATOFRM_MACOS

#define MX_COMPILER_GCC
// #define MX_COMPILER_CLANG

#ifdef MX_COMPILER_GCC
	#define MX_FORCE_INLINE __attribute__ ((always_inline))
	#define MX_NEVER_INLINE __attribute__ ((noinline))
#else
	#define MX_FORCE_INLINE
	#define MX_NEVER_INLINE
#endif

namespace dm
{
	MX_MAKE_ENUM(platform, size_t,
		windows,
		linux,
		macos
	);

	MX_MAKE_ENUM(compiler, size_t,
		gcc
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

	constexpr compiler_t current_compiler = []() -> compiler_t
	{
	#ifdef MX_COMPILER_GCC
		return compiler::gcc;
	#endif
		return compiler::invalid;
	}();
}