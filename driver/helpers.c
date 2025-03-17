#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
int setenv(const char *name, const char *value, const int overwrite) {
    if (overwrite) goto replace;
    size_t n = 0;
    const int errcode = getenv_s(&n, NULL, 0, name);
    if (errcode || n) return errcode;
replace:
    return _putenv_s(name, value);
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
