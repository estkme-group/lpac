#include "delete.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

#include <euicc/es10c.h>

static int applet_main(int argc, char **argv)
{
    int ret;
    const char *param;

    if (argc < 2)
    {
        printf("Usage: %s [iccid/aid]\n", argv[0]);
        return -1;
    }

    param = argv[1];

    ret = es10c_delete_profile(&euicc_ctx, param);

    if (ret)
    {
        const char *reason;
        switch (ret)
        {
        case 1:
            reason = "iccid or aid not found";
            break;
        case 2:
            reason = "profile not in disabled state";
            break;
        case 3:
            reason = "disallowed by policy";
            break;
        case -1:
            reason = "internal error, maybe illegal iccid/aid coding";
            break;
        default:
            reason = "unknown";
            break;
        }
        jprint_error("es10c_delete_profile", reason);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}

struct applet_entry applet_profile_delete = {
    .name = "delete",
    .main = applet_main,
};
