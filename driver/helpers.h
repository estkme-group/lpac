#pragma once
#include <stdbool.h>

#define HTTP_ENV_NAME(DRIVER, NAME) "LPAC_HTTP_" #DRIVER "_" #NAME
#define APDU_ENV_NAME(DRIVER, NAME) "LPAC_APDU_" #DRIVER "_" #NAME

#ifdef _WIN32
int setenv(const char *name, const char *value, int overwrite);
#endif

const char *getenv_or_default(const char *name, const char *default_value);

bool getenv_bool(const char *name, bool default_value);

long getenv_long(const char *name, long default_value);

void set_deprecated_env_name(const char *name, const char *deprecated_name);
