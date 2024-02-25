#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

#include <euicc/es10b.h>
#include <euicc/tostr.h>

static int applet_main(int argc, char **argv)
{
    struct es10b_notification_metadata_list *notifications, *rptr;
    cJSON *jdata = NULL;

    if (es10b_list_notification(&euicc_ctx, &notifications))
    {
        jprint_error("es10b_list_notification", NULL);
        return -1;
    }

    jdata = cJSON_CreateArray();
    rptr = notifications;
    while (rptr)
    {
        cJSON *jnotification = NULL;

        jnotification = cJSON_CreateObject();
        cJSON_AddNumberToObject(jnotification, "seqNumber", rptr->seqNumber);
        cJSON_AddStringOrNullToObject(jnotification, "profileManagementOperation", euicc_profilemanagementoperation2str(rptr->profileManagementOperation));
        cJSON_AddStringOrNullToObject(jnotification, "notificationAddress", rptr->notificationAddress);
        cJSON_AddStringOrNullToObject(jnotification, "iccid", rptr->iccid);
        cJSON_AddItemToArray(jdata, jnotification);

        rptr = rptr->next;
    }
    es10b_notification_metadata_list_free_all(notifications);

    jprint_success(jdata);

    return 0;
}

struct applet_entry applet_notification_list = {
    .name = "list",
    .main = applet_main,
};
