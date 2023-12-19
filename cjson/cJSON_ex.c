#include "cJSON_ex.h"

CJSON_PUBLIC(cJSON *) cJSON_AddStringOrNullToObject(cJSON *const object, const char *const name, const char *const string)
{
    if (string)
    {
        return cJSON_AddStringToObject(object, name, string);
    }
    else
    {
        return cJSON_AddNullToObject(object, name);
    }
}
