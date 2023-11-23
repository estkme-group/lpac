#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <euicc/interface.h>
#include <euicc/euicc.h>

#include "dlsym_interface.h"

#include <applet.h>
#include <applet/chip.h>
#include <applet/profile.h>
#include <applet/notification.h>

static const struct applet_entry *applets[] = {
    &applet_chip,
    &applet_profile,
    &applet_notification,
    NULL,
};

struct euicc_ctx euicc_ctx = {0};

int main(int argc, char **argv)
{
    int ret = 0;

    memset(&euicc_ctx, 0, sizeof(euicc_ctx));

    if (dlsym_interface_init())
    {
        return -1;
    }

    euicc_ctx.interface.apdu = &dlsym_apdu_interface;
    euicc_ctx.interface.http = &dlsym_http_interface;

    if (es10x_init(&euicc_ctx))
    {
        jprint_error("es10x_init", NULL);
        return -1;
    }

    ret = applet_entry(argc, argv, applets);

    es10x_fini(&euicc_ctx);

    return ret;
}
