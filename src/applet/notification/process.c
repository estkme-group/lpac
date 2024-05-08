#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

#include <euicc/es10b.h>
#include <euicc/es9p.h>

static void _help(const char *applet_name)
{
    printf("Usage: %s [seqNumber] [-r]\r\n", applet_name);
    printf("\t -r\tAutomatically remove processed notifications\r\n");
}

static int applet_main(int argc, char **argv)
{
    unsigned long seqNumber;
    char *str_seqNumber = NULL;
    int autoremove = 0;
    struct es10b_pending_notification notification;

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0)
        {
            _help(argv[0]);
            return -1;
        }
        else if (strcmp(argv[i], "-r") == 0)
        {
            autoremove = 1;
        }
        else
        {
            if (i && !str_seqNumber)
            {
                str_seqNumber = argv[i];
            }
        }
    }

    if (str_seqNumber == NULL)
    {
        _help(argv[0]);
        return -1;
    }

    seqNumber = atol(str_seqNumber);

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

    if (autoremove)
    {
        jprint_progress("es10b_remove_notification_from_list", str_seqNumber);
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
