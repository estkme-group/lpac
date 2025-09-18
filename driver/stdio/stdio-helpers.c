#include "stdio-helpers.h"

#include "lpac/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// getline is a GNU extension, Mingw32 macOS and FreeBSD don't have (a working) one
inline int afgets(char **obuf, FILE *fp) {
    uint32_t len = 0;
    char buffer[2];
    char *obuf_new = NULL;

    *obuf = malloc(1);
    if ((*obuf) == NULL) {
        goto err;
    }
    (*obuf)[0] = '\0';

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        uint32_t fgets_len = strlen(buffer);

        len += fgets_len + 1;
        obuf_new = realloc(*obuf, len);
        if (obuf_new == NULL) {
            goto err;
        }
        *obuf = obuf_new;
        strcat(*obuf, buffer);

        if (buffer[fgets_len - 1] == '\n') {
            break;
        }
    }

    (*obuf)[strcspn(*obuf, "\n")] = 0;

    return 0;

err:
    free(*obuf);
    *obuf = NULL;
    return -1;
}

int receive_payload(FILE *fp, const char *expected_type, cJSON **payload) {
    _cleanup_free_ char *rx_json = NULL;
    _cleanup_cjson_ cJSON *rx_jroot = NULL;
    if (afgets(&rx_json, fp) < 0 || rx_json == NULL) {
        return -1;
    }
    if ((rx_jroot = cJSON_Parse(rx_json)) == NULL) {
        return -1;
    }
    const char *type = cJSON_GetStringValue(cJSON_GetObjectItem(rx_jroot, "type"));
    if (type == NULL || strcmp(type, expected_type) != 0) {
        return -1;
    }
    const cJSON *rx_payload = cJSON_GetObjectItem(rx_jroot, "payload");
    if (!cJSON_IsObject(rx_payload))
        return -1;
    *payload = cJSON_Duplicate(rx_payload, true);
    return 0;
}
