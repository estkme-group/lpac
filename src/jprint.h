#pragma once
#include <cjson/cJSON_ex.h>

#define fprintlnf(f, fmt, ...) fprintf(f, fmt "\n", ## __VA_ARGS__); fflush(f)
#define printlnf(fmt, ...) fprintlnf(stdout, fmt, ## __VA_ARGS__)

void jprint_error(const char *function_name, const char *detail);
void jprint_progress(const char *function_name, const char *detail);
void jprint_progress_obj(const char *function_name, cJSON *jdata);
void jprint_success(cJSON *jdata);
