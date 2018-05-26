#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(_MSC_VER)
#   define ARES_PLATFORM_IS_WINDOWS 1
#elif defined(__unix__)
#   define ARES_PLATFORM_IS_POSIX 1
#elif (defined (__APPLE__) && defined (__MACH__))
#   define ARES_PLATFORM_IS_MAC 1
#else
#   define ARES_PLATFORM_IS_UNKNOWN 1
#endif

