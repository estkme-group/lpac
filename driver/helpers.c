#include "helpers.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
int setenv(const char *name, const char *value, const int overwrite) {
    if (overwrite != 0 && getenv(name) != NULL) return -1;
    const size_t n = strlen(name) + 1 /* = */ + strlen(value) + 1 /* \0 */;
    char env[n];
    snprintf(env, n, "%s=%s", name, value);
    const int errcode = _putenv(env);
    free(env);
    return errcode;
}
#endif

const char *getenv_str_or_default(const char *name, const char *default_value) {
    const char *value = getenv(name);
    if (value == NULL) return default_value;
    return value;
}

bool getenv_bool_or_default(const char *name, const bool default_value) {
    const char *value = getenv(name);
    if (value == NULL) return default_value;
    return strcmp(value, "1") == 0 ||
           strcmp(value, "y") == 0 ||
           strcmp(value, "Y") == 0 ||
           strcmp(value, "yes") == 0 ||
           strcmp(value, "YES") == 0 ||
           strcmp(value, "true") == 0 ||
           strcmp(value, "TRUE") == 0;
}

int getenv_int_or_default(const char *name, const int default_value) {
    return (int) getenv_long_or_default(name, default_value);
}

long getenv_long_or_default(const char *name, const long default_value) {
    const char *value = getenv(name);
    if (value == NULL) return default_value;
    return strtol(value, NULL, 10);
}

void set_deprecated_env_name(const char *name, const char *deprecated_name) {
    const char *value = getenv(name);
    if (value != NULL) return; // new env var already set
    value = getenv(deprecated_name);
    if (value == NULL) return; // deprecated env var not set
    fprintf(stderr, "WARNING: Please use '%s' instead of '%s'\n", name, deprecated_name);
    setenv(name, value, 0);
}
