#include "jprint.h"

#include <lpac/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void jprint_error(const char *function_name, const char *detail) {
    _cleanup_cjson_ cJSON *jroot = NULL;
    cJSON *jpayload = NULL;
    _cleanup_cjson_free_ char *jstr = NULL;

    if (detail == NULL) {
        detail = "";
    }

    jroot = cJSON_CreateObject();
    cJSON_AddStringOrNullToObject(jroot, "type", "lpa");
    jpayload = cJSON_CreateObject();
    cJSON_AddNumberToObject(jpayload, "code", -1);
    cJSON_AddStringOrNullToObject(jpayload, "message", function_name);
    cJSON_AddStringOrNullToObject(jpayload, "data", detail);
    cJSON_AddItemToObject(jroot, "payload", jpayload);

    jstr = cJSON_PrintUnformatted(jroot);

    printf("%s\n", jstr);
    fflush(stdout);
}

void jprint_progress(const char *function_name, const char *detail) {
    _cleanup_cjson_ cJSON *jroot = NULL;
    cJSON *jpayload = NULL;
    _cleanup_cjson_free_ char *jstr = NULL;

    jroot = cJSON_CreateObject();
    cJSON_AddStringOrNullToObject(jroot, "type", "progress");
    jpayload = cJSON_CreateObject();
    cJSON_AddNumberToObject(jpayload, "code", 0);
    cJSON_AddStringOrNullToObject(jpayload, "message", function_name);
    cJSON_AddStringOrNullToObject(jpayload, "data", detail);
    cJSON_AddItemToObject(jroot, "payload", jpayload);

    jstr = cJSON_PrintUnformatted(jroot);

    printf("%s\n", jstr);
    fflush(stdout);
}

void jprint_progress_obj(const char *function_name, cJSON *jdata) {
    _cleanup_cjson_ cJSON *jroot = NULL;
    cJSON *jpayload = NULL;
    _cleanup_cjson_free_ char *jstr = NULL;

    jroot = cJSON_CreateObject();
    cJSON_AddStringOrNullToObject(jroot, "type", "progress");
    jpayload = cJSON_CreateObject();
    cJSON_AddNumberToObject(jpayload, "code", 0);
    cJSON_AddStringOrNullToObject(jpayload, "message", function_name);
    if (jdata) {
        cJSON_AddItemToObject(jpayload, "data", jdata);
    } else {
        cJSON_AddNullToObject(jpayload, "data");
    }
    cJSON_AddItemToObject(jroot, "payload", jpayload);

    jstr = cJSON_PrintUnformatted(jroot);

    printf("%s\n", jstr);
    fflush(stdout);
}

void jprint_success(cJSON *jdata) {
    _cleanup_cjson_ cJSON *jroot = NULL;
    cJSON *jpayload = NULL;
    _cleanup_cjson_free_ char *jstr = NULL;

    jroot = cJSON_CreateObject();
    cJSON_AddStringOrNullToObject(jroot, "type", "lpa");
    jpayload = cJSON_CreateObject();
    cJSON_AddNumberToObject(jpayload, "code", 0);
    cJSON_AddStringOrNullToObject(jpayload, "message", "success");
    if (jdata) {
        cJSON_AddItemToObject(jpayload, "data", jdata);
    } else {
        cJSON_AddNullToObject(jpayload, "data");
    }
    cJSON_AddItemToObject(jroot, "payload", jpayload);

    jstr = cJSON_PrintUnformatted(jroot);

    printf("%s\n", jstr);
    fflush(stdout);
}
