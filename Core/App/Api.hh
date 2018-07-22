#pragma once

#define ARES_API_EXTERN extern "C"

#ifdef _MSC_VER
#   define ARES_API ARES_API_EXTERN __declspec(dllexport)
#else
#   define ARES_API ARES_API_EXTERN __attribute__((visibility("default")))
#endif
