#pragma once
#include <cjson/cJSON.h>

void jprint_error(const char *function_name, const char *detail);
void jprint_progress(const char *function_name);
void jprint_success(cJSON *jdata);
