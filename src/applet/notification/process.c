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
    char* seqNumberStr = NULL;
    int autoremove = 0;
    struct es10b_pending_notification notification;

    static const char *opt_string = "rh";

    int opt;

    char helpText[256];
    snprintf(helpText, 256, "Usage: %s [-r] seq-number\n\t -r\tAutomatically remove processed notifications\n", argv[0]);

    while (optind < argc) {
        if ((opt = getopt(argc, argv, opt_string)) != -1) {
            switch(opt) {
                case 'r':
                    autoremove = 1;
                    break;
                case 'h':
                    printf("%s", helpText);
                    return -1;
                    break;
            }
        } else {
            // allow optional arguments after positional arguments 
            seqNumberStr = strdup(argv[optind]);
            optind++;
        }
    }

    if (seqNumberStr == NULL) {
        jprint_error("Sequence number must be specified", NULL);
        printf("%s", helpText);
        return -1;
    }

    seqNumber = atoi(seqNumberStr);

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
