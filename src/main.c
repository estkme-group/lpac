#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <euicc/interface.h>
#include <euicc/euicc.h>

#include "dlsym_interface.h"

#include "applet.h"
#include "applet/chip.h"
#include "applet/profile.h"
#include "applet/notification.h"

static const struct applet_entry *applets[] = {
    &applet_dlsym_interface,
    &applet_chip,
    &applet_profile,
    &applet_notification,
    NULL,
};

static int euicc_ctx_inited = 0;
struct euicc_ctx euicc_ctx = {0};

void main_init_euicc()
{
    if (euicc_init(&euicc_ctx))
    {
        jprint_error("euicc_init", NULL);
        exit(-1);
    }
    euicc_ctx_inited = 1;
}

void main_fini_euicc()
{
    if (!euicc_ctx_inited)
    {
        return;
    }
    euicc_fini(&euicc_ctx);
    euicc_ctx_inited = 0;
}

int main(int argc, char **argv)
{
    int ret = 0;

    memset(&euicc_ctx, 0, sizeof(euicc_ctx));

    if (dlsym_interface_init())
    {
        return -1;
    }

    euicc_ctx.apdu.interface = &dlsym_apdu_interface;
    euicc_ctx.http.interface = &dlsym_http_interface;

    ret = applet_entry(argc, argv, applets);

    main_fini_euicc();

    return ret;
}
