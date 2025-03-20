#include "helpers.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static bool is_numeric(const char *value) {
    if (value == NULL) return false;
    for (size_t i = strlen(value); i > 0; --i) {
        if (isdigit(value[i])) continue;
        return false;
    }
    return true;
}

const char *getenv_str_or_default(const char *name, const char *default_value) {
    const char *value = getenv(name);
    if (value == NULL) return default_value;
    return value;
}

bool getenv_bool_or_default(const char *name, const bool default_value) {
    const char *value = getenv(name);
    if (value == NULL) return default_value;
    if (is_numeric(value)) return strcmp(value, "0") != 0;
    return strcasecmp(value, "y") == 0 ||
           strcasecmp(value, "yes") == 0 ||
           strcasecmp(value, "true") == 0;
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
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}
