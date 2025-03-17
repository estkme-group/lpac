#pragma once

#define HTTP_ENV_NAME(DRIVER, NAME) "LPAC_HTTP_" #DRIVER "_" #NAME
#define APDU_ENV_NAME(DRIVER, NAME) "LPAC_APDU_" #DRIVER "_" #NAME

#ifdef _WIN32
int setenv(const char *name, const char *value, int overwrite);
#endif

void set_deprecated_env_name(const char *name, const char *deprecated_name);
