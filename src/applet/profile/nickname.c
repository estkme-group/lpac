#include "nickname.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

#include <euicc/es10c.h>

static int applet_main(int argc, char **argv)
{
    int ret;
    const char *iccid;
    const char *new_name;

    if (argc < 2)
    {
        printf("Usage: %s [iccid] [new_name]\n", argv[0]);
        printf("\t[new_name]: optional\n");
        return -1;
    }

    iccid = argv[1];
    if (argc > 2)
    {
        new_name = argv[2];
    }
    else
    {
        new_name = "";
    }

    if ((ret = es10c_set_nickname(&euicc_ctx, iccid, new_name)))
    {
        const char *reason;
        switch (ret)
        {
        case 1:
            reason = "iccid not found";
            break;
        default:
            reason = "unknown";
            break;
        }
        jprint_error("es10c_set_nickname", reason);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}

struct applet_entry applet_profile_nickname = {
    .name = "nickname",
    .main = applet_main,
};
