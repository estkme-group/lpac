#pragma once
#include "cjson/cJSON.h"
#include <stdbool.h>

#define HTTP_ENV_NAME(DRIVER, NAME) "LPAC_HTTP_" #DRIVER "_" #NAME
#define APDU_ENV_NAME(DRIVER, NAME) "LPAC_APDU_" #DRIVER "_" #NAME

#define getenv_or_default(name, default_value) \
  _Generic((default_value), \
    bool: getenv_bool_or_default, \
    int: getenv_int_or_default, \
    long: getenv_long_or_default, \
    char *: getenv_str_or_default \
  )(name, default_value)

const char *getenv_str_or_default(const char *name, const char *default_value);

bool getenv_bool_or_default(const char *name, bool default_value);

int getenv_int_or_default(const char *name, int default_value);

long getenv_long_or_default(const char *name, long default_value);

void set_deprecated_env_name(const char *name, const char *deprecated_name);

bool json_print(char *type, cJSON *jpayload);
