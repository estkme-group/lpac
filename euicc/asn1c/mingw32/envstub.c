#include "envstub.h"
#include <windows.h>
#include <string.h>
#include <stdlib.h>

int setenv(const char *key, const char *value, int replace)
{
    char *envstring;

    envstring = malloc(strlen(key) + strlen("=") + strlen(value) + 1);
    if (!envstring)
    {
        return -1;
    }

    envstring[0] = '\0';

    strcat(envstring, key);
    strcat(envstring, "=");
    strcat(envstring, value);

    return _putenv(envstring);
}

int unsetenv(const char *key)
{
    char *envstring;

    envstring = malloc(strlen(key) + strlen("=") + 1);
    if (!envstring)
    {
        return -1;
    }

    envstring[0] = '\0';

    strcat(envstring, key);
    strcat(envstring, "=");

    return _putenv(envstring);
}
