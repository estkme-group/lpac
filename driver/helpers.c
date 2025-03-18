#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
int setenv(const char *name, const char *value, const int overwrite) {
    if (overwrite != 0 && getenv(name) != NULL) return -1;
    const size_t n = strlen(name) + 1 /* = */ + strlen(value) + 1 /* \0 */;
    char *env = calloc(n, sizeof(char));
    if (env == NULL) return -1;
    snprintf(env, n, "%s=%s", name, value);
    const int errcode = _putenv(env);
    free(env);
    return errcode;
}
#endif

void set_deprecated_env_name(const char *name, const char *deprecated_name) {
    const char *value = getenv(name);
    if (value != NULL) return; // new env var already set
    value = getenv(deprecated_name);
    if (value == NULL) return; // deprecated env var not set
    fprintf(stderr, "WARNING: Please use '%s' instead of '%s'\n", name, deprecated_name);
    setenv(name, value, 0);
}
