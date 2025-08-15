#include "list.h"

#include "main.h"
#include "notification_common.h"

#include <euicc/es10b.h>
#include <euicc/tostr.h>
#include <lpac/utils.h>

#include <stdio.h>
#include <unistd.h>

static int applet_main(int argc, char **argv) {
    _cleanup_es10b_notification_metadata_list_ struct es10b_notification_metadata_list *notifications, *rptr;
    cJSON *jdata = NULL;

    if (es10b_list_notification(&euicc_ctx, &notifications)) {
        jprint_error("es10b_list_notification", NULL);
        return -1;
    }

    jdata = cJSON_CreateArray();
    rptr = notifications;
    while (rptr) {
        cJSON *jnotification = NULL;

        jnotification = cJSON_CreateObject();
        cJSON_AddNumberToObject(jnotification, "seqNumber", rptr->seqNumber);
        cJSON_AddStringOrNullToObject(jnotification, "profileManagementOperation",
                                      euicc_profilemanagementoperation2str(rptr->profileManagementOperation));
        cJSON_AddStringOrNullToObject(jnotification, "notificationAddress",
                                      notification_strstrip(rptr->notificationAddress));
        cJSON_AddStringOrNullToObject(jnotification, "iccid", rptr->iccid);
        cJSON_AddItemToArray(jdata, jnotification);

        rptr = rptr->next;
    }

    jprint_success(jdata);

    return 0;
}

struct applet_entry applet_notification_list = {
    .name = "list",
    .main = applet_main,
};
