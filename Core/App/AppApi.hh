#pragma once

#define ARES_APP_API_EXTERN extern "C"

#ifdef _MSC_VER
#   define ARES_APP_API ARES_APP_API_EXTERN __declspec(dllexport)
#else
#   define ARES_APP_API ARES_APP_API_EXTERN __attribute__((visibility("default")))
#endif
