#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

#include <euicc/es10b.h>
#include <euicc/es9p.h>

static int applet_main(int argc, char **argv)
{
    unsigned long seqNumber;
    int autoremove;
    struct es10b_pending_notification notification;

    if (argc < 2)
    {
        printf("Usage: %s <seqNumber> [autoremove]\n", argv[0]);
        printf("\t[autoremove]: optional\n");
        return -1;
    }

    seqNumber = atol(argv[1]);
    autoremove = 0;
    if (argc > 2)
    {
        autoremove = atoi(argv[2]);
    }

    jprint_progress("es10b_retrieve_notifications_list");
    if (es10b_retrieve_notifications_list(&euicc_ctx, &notification, seqNumber))
    {
        jprint_error("es10b_retrieve_notifications_list", NULL);
        return -1;
    }

    euicc_ctx.http.server_address = notification.notificationAddress;

    jprint_progress("es9p_handle_notification");
    if (es9p_handle_notification(&euicc_ctx, notification.b64_PendingNotification))
    {
        jprint_error("es9p_handle_notification", NULL);
        return -1;
    }

    es10b_pending_notification_free(&notification);

    if (autoremove)
    {
        jprint_progress("es10b_remove_notification_from_list");
        if (es10b_remove_notification_from_list(&euicc_ctx, seqNumber))
        {
            jprint_error("es10b_remove_notification_from_list", NULL);
            return -1;
        }
    }

    jprint_success(NULL);

    return 0;
}

struct applet_entry applet_notification_process = {
    .name = "process",
    .main = applet_main,
};
