#pragma once

#define HTTP_ENV_NAME(DRIVER, NAME) "LPAC_HTTP_" #DRIVER "_" #NAME
#define APDU_ENV_NAME(DRIVER, NAME) "LPAC_APDU_" #DRIVER "_" #NAME

void set_deprecated_env_name(const char *name, const char *deprecated_name);
