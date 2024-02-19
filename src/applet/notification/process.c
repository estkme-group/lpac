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
    struct es10b_notification notification;
    struct es9p_ctx es9p_ctx = {0};

    if (argc < 2)
    {
        printf("Usage: %s <seqNumber>\n", argv[0]);
        return -1;
    }

    seqNumber = atol(argv[1]);

    jprint_progress("es10b_RetrieveNotificationsList");
    if (es10b_RetrieveNotificationsList(&euicc_ctx, &notification, seqNumber))
    {
        jprint_error("es10b_RetrieveNotificationsList", NULL);
        return -1;
    }

    es9p_ctx.euicc_ctx = &euicc_ctx;
    es9p_ctx.address = notification.notificationAddress;

    jprint_progress("es9p_handle_notification");
    if (es9p_handle_notification(&es9p_ctx, notification.b64_payload))
    {
        jprint_error("es9p_handle_notification", NULL);
        return -1;
    }

    es10b_notification_free(&notification);

    jprint_success(NULL);

    return 0;
}

struct applet_entry applet_notification_process = {
    .name = "process",
    .main = applet_main,
};
