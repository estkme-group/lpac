#include "helpers.h"

#include <ctype.h>
#include <string.h>

char *notification_strstrip(char *input) {
    if (input == NULL)
        return NULL;

    // Remove leading whitespace
    while (isspace((unsigned char)*input)) {
        input++;
    }

    // Remove trailing whitespace
    if (*input == '\0')
        return input;

    // Check if the string is not empty after leading whitespace removal
    char *end = input + strlen(input) - 1;
    while (end >= input && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    return input;
}

cJSON *build_notification(const char *eid, const uint32_t seqNumber,
                          const struct es10b_pending_notification *notification) {
    cJSON *jroot = cJSON_CreateObject();
    if (jroot == NULL)
        return jroot;
    cJSON_AddStringToObject(jroot, "type", "notification");
    cJSON_AddStringToObject(jroot, "eid", eid);
    cJSON_AddNumberToObject(jroot, "seqNumber", seqNumber);
    cJSON_AddStringToObject(jroot, "notificationAddress", notification_strstrip(notification->notificationAddress));
    cJSON_AddStringToObject(jroot, "pendingNotification", notification->b64_PendingNotification);
    return jroot;
}

bool parse_notification(const cJSON *jroot, const char *eid, uint32_t *seqNumber,
                        struct es10b_pending_notification *notification) {
    const char *value = NULL;
    const cJSON *jvalue = NULL;

    jvalue = cJSON_GetObjectItem(jroot, "type");
    if (!cJSON_IsString(jvalue))
        return false;
    value = cJSON_GetStringValue(jvalue);
    if (strcmp(value, "notification") != 0)
        return false;

    jvalue = cJSON_GetObjectItem(jroot, "eid");
    if (!cJSON_IsString(jvalue))
        return false;
    value = cJSON_GetStringValue(jvalue);
    if (strcmp(value, eid) != 0)
        return false;

    jvalue = cJSON_GetObjectItem(jroot, "seqNumber");
    if (!cJSON_IsNumber(jvalue))
        return false;
    *seqNumber = (uint32_t)cJSON_GetNumberValue(jvalue);

    jvalue = cJSON_GetObjectItem(jroot, "notificationAddress");
    if (!cJSON_IsString(jvalue))
        return false;
    notification->notificationAddress = notification_strstrip(cJSON_GetStringValue(jvalue));

    jvalue = cJSON_GetObjectItem(jroot, "pendingNotification");
    if (!cJSON_IsString(jvalue))
        return false;
    notification->b64_PendingNotification = cJSON_GetStringValue(jvalue);

    return true;
}
