#include "remove.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <main.h>

#include <euicc/es10b.h>

static int _delete_single(uint32_t seqNumber)
{
    char str_seqNumber[11];
    int ret;

    snprintf(str_seqNumber, sizeof(str_seqNumber), "%u", seqNumber);

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

    return 0;
}

static int applet_main(int argc, char **argv)
{
    static const char *opt_string = "ah?";

    int fret = 0;
    int all = 0;
    int argc_seq_offset = 1;

    int opt = getopt(argc, argv, opt_string);
    for (int i = 0; opt != -1; i++)
    {
        switch (opt)
        {
        case 'a':
            all = 1;
            break;
        case 'h':
        case '?':
            printf("Usage: %s [OPTIONS] [seqNumber_0] [seqNumber_1]...\r\n", argv[0]);
            printf("\t -a All notifications\r\n");
            return -1;
        default:
            goto run;
            break;
        }
        argc_seq_offset++;
        opt = getopt(argc, argv, opt_string);
    }

run:
    if (all)
    {
        struct es10b_notification_metadata_list *notifications, *rptr;

        jprint_progress("es10b_list_notification", NULL);
        if (es10b_list_notification(&euicc_ctx, &notifications))
        {
            jprint_error("es10b_list_notification", NULL);
            return -1;
        }

        rptr = notifications;
        while (rptr)
        {
            if (_delete_single(rptr->seqNumber))
            {
                fret = -1;
                break;
            }
            rptr = rptr->next;
        }

        es10b_notification_metadata_list_free_all(notifications);
    }
    else
    {
        for (int i = argc_seq_offset; i < argc; i++)
        {
            unsigned long seqNumber;

            errno = 0;
            seqNumber = strtoul(argv[i], NULL, 10);
            if (errno != 0)
            {
                continue;
            }

            if (_delete_single(seqNumber))
            {
                fret = -1;
                break;
            }
        }
    }

    if (fret == 0)
    {
        jprint_success(NULL);
    }

    return fret;
}

struct applet_entry applet_notification_remove = {
    .name = "remove",
    .main = applet_main,
};
