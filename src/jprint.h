#pragma once
#include <cjson/cJSON_ex.h>

void jprint_error(const char *function_name, const char *detail);
void jprint_progress(const char *function_name, const char *detail);
void jprint_success(cJSON *jdata);
