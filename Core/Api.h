#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(_MSC_VER)
#   if defined(ARES_EXPORTS)
#       define ARES_API __declspec(dllexport)
#   else
#       define ARES_API __declspec(dllimport)
#   endif
#else
#   define ARES_API __attribute__((visibility("default")))
#endif
