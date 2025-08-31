#include "utils.h"

#include <cjson/cJSON_ex.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *getenv_str_or_default(const char *name, const char *default_value) {
    const char *value = getenv(name);
    if (value == NULL || strlen(value) == 0)
        return default_value;
    return value;
}

bool getenv_bool_or_default(const char *name, const bool default_value) {
    const char *value = getenv(name);
    if (value == NULL || strlen(value) == 0)
        return default_value;
    if (strcasecmp(value, "1") == 0 || strcasecmp(value, "y") == 0 || strcasecmp(value, "on") == 0
        || strcasecmp(value, "yes") == 0 || strcasecmp(value, "true") == 0)
        return true;
    if (strcasecmp(value, "0") == 0 || strcasecmp(value, "n") == 0 || strcasecmp(value, "off") == 0
        || strcasecmp(value, "no") == 0 || strcasecmp(value, "false") == 0)
        return false;
    fprintf(stderr, "WARNING: Invalid value '%s' for environment variable '%s', falling back to default (%s)\n", value,
            name, default_value ? "true" : "false");
    return default_value;
}

int getenv_int_or_default(const char *name, const int default_value) {
    return (int)getenv_long_or_default(name, default_value);
}

long getenv_long_or_default(const char *name, const long default_value) {
    const char *value = getenv(name);
    if (value == NULL || strlen(value) == 0)
        return default_value;
    return strtol(value, NULL, 10);
}

void set_deprecated_env_name(const char *name, const char *deprecated_name) {
    const char *value = getenv(name);
    if (value != NULL)
        return; // new env var already set
    value = getenv(deprecated_name);
    if (value == NULL)
        return; // deprecated env var not set
    fprintf(stderr, "WARNING: Please use '%s' instead of '%s'\n", name, deprecated_name);
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

bool json_print(char *type, cJSON *jpayload) {
    _cleanup_cjson_ cJSON *jroot = NULL;
    _cleanup_free_ char *jstr = NULL;

    if (jpayload == NULL) {
        goto err;
    }

    jroot = cJSON_CreateObject();
    if (jroot == NULL) {
        goto err;
    }

    if (cJSON_AddStringOrNullToObject(jroot, "type", type) == NULL) {
        goto err;
    }

    if (cJSON_AddItemReferenceToObject(jroot, "payload", jpayload) == 0) {
        goto err;
    }

    jstr = cJSON_PrintUnformatted(jroot);

    if (jstr == NULL) {
        goto err;
    }

    fprintf(stdout, "%s\n", jstr);
    fflush(stdout);

    return true;

err:
    return false;
}
