#include "remove.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

#include <euicc/es10b.h>

static int applet_main(int argc, char **argv)
{
    int ret;
    unsigned long seqNumber;

    if (argc < 2)
    {
        printf("Usage: %s <seqNumber>\n", argv[0]);
        return -1;
    }

    seqNumber = atol(argv[1]);

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

    jprint_success(NULL);

    return 0;
}

struct applet_entry applet_notification_remove = {
    .name = "remove",
    .main = applet_main,
};
