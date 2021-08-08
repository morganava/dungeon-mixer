#pragma once

#include <smol/enum.hpp>

// todo: actually detect the platform we're on once we're cross-platform
#define SMOL_PLATFORM_LINUX
// #define SMOL_PLATFORM_WINDOWS
// #define SMOL_PLATOFRM_MACOS

#define SMOL_COMPILER_GCC
// #define SMOL_COMPILER_CLANG

#ifdef SMOL_COMPILER_GCC
    #define SMOL_FORCE_INLINE __attribute__ ((always_inline))
    #define SMOL_NEVER_INLINE __attribute__ ((noinline))
#else
    #define SMOL_FORCE_INLINE
    #define SMOL_NEVER_INLINE
#endif

namespace smol
{
    SMOL_MAKE_ENUM(platform, size_t,
        windows,
        linux,
        macos
    );

    SMOL_MAKE_ENUM(compiler, size_t,
        gcc
    );

    constexpr platform_t current_platform = []() -> platform_t
    {
    #ifdef SMOL_PLATFORM_LINUX
        return platform::linux;
    #endif
    #ifdef SMOL_PLATFORM_WINDOWS
        return platform::windows;
    #endif
    #ifdef SMOL_PLATOFRM_MACOS
        return platform::macos;
    #endif
        return platform::invalid;
    }();

    constexpr compiler_t current_compiler = []() -> compiler_t
    {
    #ifdef SMOL_COMPILER_GCC
        return compiler::gcc;
    #endif
        return compiler::invalid;
    }();
}