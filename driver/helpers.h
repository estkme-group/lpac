#pragma once

#define HTTP_ENV_NAME(TYPE, NAME) "LPAC_HTTP_" TYPE "_" NAME
#define APDU_ENV_NAME(TYPE, NAME) "LPAC_APDU_" TYPE "_" NAME

void set_deprecated_env(const char *name, const char *deprecated_name);
