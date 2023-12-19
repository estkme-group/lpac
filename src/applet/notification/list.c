#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

static int applet_main(int argc, char **argv)
{
    struct es10b_notification_metadata *notifications;
    int notifications_count;
    cJSON *jdata = NULL;

    if (es10b_list_notification(&euicc_ctx, &notifications, &notifications_count))
    {
        jprint_error("es10b_list_notification", NULL);
        return -1;
    }

    jdata = cJSON_CreateArray();
    for (int i = 0; i < notifications_count; i++)
    {
        cJSON *jnotification = NULL;

        jnotification = cJSON_CreateObject();
        cJSON_AddNumberToObject(jnotification, "seqNumber", notifications[i].seqNumber);
        cJSON_AddStringOrNullToObject(jnotification, "profileManagementOperation", notifications[i].profileManagementOperation);
        cJSON_AddStringOrNullToObject(jnotification, "notificationAddress", notifications[i].notificationAddress);
        cJSON_AddStringOrNullToObject(jnotification, "iccid", notifications[i].iccid);

        cJSON_AddItemToArray(jdata, jnotification);
    }

    es10b_notification_metadata_free_all(notifications, notifications_count);

    jprint_success(jdata);

    return 0;
}

struct applet_entry applet_notification_list = {
    .name = "list",
    .main = applet_main,
};
