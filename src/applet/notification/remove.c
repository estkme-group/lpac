#include "remove.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <main.h>

#include <euicc/es10b.h>

static const char *opt_string = "h?";

static int applet_main(int argc, char **argv)
{
    int opt = getopt(argc, argv, opt_string);
    while (opt != -1)
    {
        switch (opt)
        {
            case 'h':
            case '?':
                printf("Usage: %s [seqNumber] etc\r\n", argv[0]);
                return -1;
            default:
                break;
        }
        opt = getopt(argc, argv, opt_string);
    }

    int seqNumber;
    int ret;

    for (int i = 1; i < argc; i++)
    {
        errno = 0;
        seqNumber = (int) strtol(argv[i], NULL, 10);
        if (errno != 0)
        {
            continue;
        }
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

struct applet_entry applet_notification_remove = {
    .name = "remove",
    .main = applet_main,
};
