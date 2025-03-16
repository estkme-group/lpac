#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
static int setenv(const char *name, const char *value, int replace) {
    if (replace != 0) goto put;
    size_t n = 0;
    const int errcode = getenv_s(&n, NULL, 0, name);
    if (errcode || n) return errcode;
put:
    return _putenv_s(name, value);
}
#endif

void set_deprecated_env(const char *name, const char *deprecated_name) {
    const char *deprecated_value = getenv(deprecated_name);
    if (deprecated_value == NULL) return; // deprecated env var not set
    fprintf(stderr, "WARNING: Please use '%s' instead of '%s'\n", name, deprecated_name);
    const char *value = getenv(name);
    if (value != NULL) return; // new env var already set
    setenv(name, deprecated_value, 1);
}
