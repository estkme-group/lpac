#pragma once

#include "cJSON.h"

CJSON_PUBLIC(cJSON *) cJSON_AddStringOrNullToObject(cJSON *const object, const char *const name, const char *const string);
