#include <stdlib.h>
#include <stdio.h>

void set_deprecated_env(const char *name, const char *deprecated_name) {
    const char *deprecated_value = getenv(deprecated_name);
    if (deprecated_value == NULL) return; // deprecated env var not set
    fprintf(stderr, "WARNING: Please use '%s' instead of '%s'\n", name, deprecated_name);
    const char *value = getenv(name);
    if (value != NULL) return; // new env var already set
#ifdef _WIN32
    _putenv_s(name, deprecated_value);
#else
    setenv(name, deprecated_value, 1);
#endif
}
