#pragma once

// Useful macros
//
#include <cstdio>

#define M_UNUSED(a) (void)a;

#if defined(_MSC_VER)
#  define DECL_ALIGN(x) __declspec(align(x))
#elif defined(__GNUC__)
#  define DECL_ALIGN(x) __attribute((aligned(x)))
#endif

/**
 * Determination a platform of an operation system
 * Fully supported supported only GNU GCC/G++, partially on Clang/LLVM
 */

#if defined(_WIN32)
    #define PLATFORM_NAME "windows" // Windows
#elif defined(_WIN64)
    #define PLATFORM_NAME "windows" // Windows
#elif defined(__CYGWIN__) && !defined(_WIN32)
    #define PLATFORM_NAME "windows" // Windows (Cygwin POSIX under Microsoft Window)
    #define TARGET_CYGWIN 1
    #define TARGET_WINDOWS 1
#elif defined(__ANDROID__)
    #define PLATFORM_NAME "android" // Android (implies Linux, so it must come first)
    #define TARGET_ANDROID 1
#elif defined(__linux__)
    #define PLATFORM_NAME "linux" // Debian, Ubuntu, Gentoo, Fedora, openSUSE, RedHat, Centos and other
#elif defined(__unix__) || !defined(__APPLE__) && defined(__MACH__)
    #include <sys/param.h>
    #if defined(BSD)
        #define PLATFORM_NAME "bsd" // FreeBSD, NetBSD, OpenBSD, DragonFly BSD
    #endif
#elif defined(__hpux)
    #define PLATFORM_NAME "hp-ux" // HP-UX
#elif defined(_AIX)
    #define PLATFORM_NAME "aix" // IBM AIX
#elif defined(__APPLE__) && defined(__MACH__) // Apple OSX and iOS (Darwin)
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR == 1
        #define PLATFORM_NAME "ios" // Apple iOS
    #elif TARGET_OS_IPHONE == 1
        #define PLATFORM_NAME "ios" // Apple iOS
    #elif TARGET_OS_MAC == 1
        #define PLATFORM_NAME "osx" // Apple OSX
    #endif
#elif defined(__sun) && defined(__SVR4)
    #define PLATFORM_NAME "solaris" // Oracle Solaris, Open Indiana
#else
    #define PLATFORM_NAME NULL
#endif

// Return a name of platform, if determined, otherwise - an empty string
inline const char *get_platform_name() {
    return (PLATFORM_NAME == NULL) ? "" : PLATFORM_NAME;
}

#ifdef _WIN64
    #define TARGET_WINDOWS 1
    #define TARGET_WIN64 1
#elif _WIN32
    #define TARGET_WINDOWS 1
    #define TARGET_WIN32 1
#elif __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
        #define TARGET_IOS 1
        #define TARGET_IPHONE 1
        #define TARGET_SIMULATOR 1
        // define something for simulator   
    #elif TARGET_OS_IPHONE
        // define something for iphone  
        #define TARGET_IOS 1
        #define TARGET_SIMULATOR 1
    #else
        #define TARGET_OS_OSX 1
        // define something for OSX
    #endif
#elif __linux
    // linux
    #define TARGET_LINUX 1
    #define TARGET_UNIX_VARIANT 1
#elif __unix // all unices not caught above
    // Unix
    #define TARGET_UNIX 1
    #define TARGET_UNIX_VARIANT 1
#elif __posix
    // POSIX
    #define TARGET_POSIX 1
    #define TARGET_UNIX_VARIANT 1
#endif

