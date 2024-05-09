#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <main.h>

#include <euicc/es10b.h>
#include <euicc/es9p.h>

static const char *opt_string = "rh?";

static int applet_main(int argc, char **argv)
{
    bool auto_remove = false;
    int opt = getopt(argc, argv, opt_string);
    while (opt != -1)
    {
        switch (opt)
        {
            case 'r':
                auto_remove = true;
                break;
            case 'h':
            case '?':
                printf("Usage: %s [seqNumber] etc [OPTIONS]\r\n", argv[0]);
                printf("\t -r Automatically remove processed notifications\r\n");
                return -1;
            default:
                break;
        }
        opt = getopt(argc, argv, opt_string);
    }

    int seqNumber;
    char *str_seqNumber = NULL;
    struct es10b_pending_notification notification;
    int ret;

    for (int i = 1; i < argc; i++)
    {
        errno = 0;
        seqNumber = (int) strtol(argv[i], NULL, 10);
        if (errno != 0)
        {
            continue;
        }
        str_seqNumber = argv[i];

        jprint_progress("es10b_retrieve_notifications_list", str_seqNumber);
        if (es10b_retrieve_notifications_list(&euicc_ctx, &notification, seqNumber))
        {
            jprint_error("es10b_retrieve_notifications_list", NULL);
            return -1;
        }

        euicc_ctx.http.server_address = notification.notificationAddress;

        jprint_progress("es9p_handle_notification", str_seqNumber);
        if (es9p_handle_notification(&euicc_ctx, notification.b64_PendingNotification))
        {
            jprint_error("es9p_handle_notification", NULL);
            return -1;
        }

        es10b_pending_notification_free(&notification);

        if (!auto_remove)
        {
            continue;
        }

        jprint_progress("es10b_remove_notification_from_list", str_seqNumber);
        if ((ret = es10b_remove_notification_from_list(&euicc_ctx, seqNumber)))
        {
            const char *reason;
            switch (ret)
            {
                case 1:
                    reason = "seqNumber not found";
                    break;
                default:
                    reason = "unknown";
                    break;
            }
            jprint_error("es10b_remove_notification_from_list", reason);
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
