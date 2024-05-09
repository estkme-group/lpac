#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <euicc/interface.h>
#include <euicc/euicc.h>
#include <driver.h>

#include "applet.h"
#include "applet/chip.h"
#include "applet/profile.h"
#include "applet/notification.h"
#include "applet/version.h"

#ifdef WIN32
#include <windef.h>
#include <processthreadsapi.h>
#include <shellapi.h>
#include <stringapiset.h>
#include <processenv.h>
#endif

static int driver_applet_main(int argc, char **argv)
{
    const struct applet_entry *applets[] = {
        &(struct applet_entry){
            .name = "apdu",
            .main = euicc_driver_main_apdu,
        },
        &(struct applet_entry){
            .name = "http",
            .main = euicc_driver_main_http,
        },
        NULL,
    };
    return applet_entry(argc, argv, applets);
}

struct applet_entry driver_applet = {
    .name = "driver",
    .main = driver_applet_main,
};

static const struct applet_entry *applets[] = {
    &driver_applet,
    &applet_chip,
    &applet_profile,
    &applet_notification,
    &applet_version,
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

#ifdef WIN32
static char **warg_to_arg(const int wargc, wchar_t **wargv)
{
    char **argv = malloc(wargc * sizeof(char *));
    if (argv == NULL)
    {
        return NULL;
    }
    for (int i = 0; i < wargc; ++i)
    {
        const int size = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, NULL, 0, NULL, NULL);
        argv[i] = malloc(size);
        if (argv[i] == NULL)
        {
            for (int j = 0; j < i; ++j)
            {
                free(argv[j]);
            }
            free(argv);
            return NULL;
        }
        WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, argv[i], size, NULL, NULL);
    }
    return argv;
}
#endif

int main(int argc, char **argv)
{
    int ret = 0;

    memset(&euicc_ctx, 0, sizeof(euicc_ctx));

    if (euicc_driver_init(getenv("LPAC_APDU"), getenv("LPAC_HTTP")))
    {
        return -1;
    }

    euicc_ctx.apdu.interface = &euicc_driver_interface_apdu;
    euicc_ctx.http.interface = &euicc_driver_interface_http;

#ifdef WIN32
    argv = warg_to_arg(argc, CommandLineToArgvW(GetCommandLineW(), &argc));
    if (argv == NULL)
    {
        return -1;
    }
#endif

    ret = applet_entry(argc, argv, applets);

    main_fini_euicc();

    euicc_driver_fini();

    return ret;
}
