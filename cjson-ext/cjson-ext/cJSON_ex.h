#pragma once

#include <cjson/cJSON.h>

cJSON *cJSON_AddStringOrNullToObject(cJSON *const object, const char *const name, const char *const string);
