#pragma once
#include <cjson/cJSON_ex.h>

void jprint_error(const char *function_name, const char *message);
void jprint_error_details(const char *function_name, const char *message, cJSON *details);
void jprint_progress(const char *function_name, const char *detail);
void jprint_progress_obj(const char *function_name, cJSON *jdata);
void jprint_success(cJSON *jdata);
