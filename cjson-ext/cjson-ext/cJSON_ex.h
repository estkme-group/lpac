#pragma once

#include <cjson/cJSON.h>

#if defined(_WIN32)
#    define CJSON_EXT_API __declspec(dllexport)
#else
#    define CJSON_EXT_API
#endif

CJSON_EXT_API cJSON *cJSON_AddStringOrNullToObject(cJSON *const object, const char *const name,
                                                   const char *const string);
