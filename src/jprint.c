#include "jprint.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void jprint_error(const char *function_name, const char *detail)
{
    cJSON *jroot = NULL;
    cJSON *jpayload = NULL;
    char *jstr = NULL;

    if (detail == NULL)
    {
        detail = "";
    }

    jroot = cJSON_CreateObject();
    cJSON_AddStringToObject(jroot, "type", "lpa");
    jpayload = cJSON_CreateObject();
    cJSON_AddNumberToObject(jpayload, "code", -1);
    cJSON_AddStringToObject(jpayload, "message", function_name);
    cJSON_AddStringToObject(jpayload, "data", detail);
    cJSON_AddItemToObject(jroot, "payload", jpayload);

    jstr = cJSON_PrintUnformatted(jroot);
    cJSON_Delete(jroot);

    printf("%s\n", jstr);
    free(jstr);
}

void jprint_success(cJSON *jdata)
{
    cJSON *jroot = NULL;
    cJSON *jpayload = NULL;
    char *jstr = NULL;

    jroot = cJSON_CreateObject();
    cJSON_AddStringToObject(jroot, "type", "lpa");
    jpayload = cJSON_CreateObject();
    cJSON_AddNumberToObject(jpayload, "code", 0);
    cJSON_AddStringToObject(jpayload, "message", "success");
    if (jdata)
    {
        cJSON_AddItemToObject(jpayload, "data", jdata);
    }
    else
    {
        cJSON_AddNullToObject(jpayload, "data");
    }
    cJSON_AddItemToObject(jroot, "payload", jpayload);

    jstr = cJSON_PrintUnformatted(jroot);
    cJSON_Delete(jroot);

    printf("%s\n", jstr);
    free(jstr);
}
