#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <euicc/interface.h>
#include <euicc/hexutil.h>
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

#define ISD_R_AID_STR_LENGTH 16

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
    const char *custom_aid_str = getenv("LPAC_CUSTOM_ISD_R_AID");
    if (custom_aid_str)
    {
        unsigned char custom_aid[ISD_R_AID_STR_LENGTH];
        const int custom_aid_len = euicc_hexutil_hex2bin(custom_aid, ISD_R_AID_STR_LENGTH, custom_aid_str);
        if (custom_aid_len != ISD_R_AID_STR_LENGTH)
        {
            jprint_error("euicc_init", "invalid custom AID given");
            exit(-1);
        }

        euicc_ctx.aid = custom_aid;
        euicc_ctx.aid_len = custom_aid_len;
    }

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
    const char *apdu_driver;
    const char *http_driver;

    memset(&euicc_ctx, 0, sizeof(euicc_ctx));

    apdu_driver = getenv("LPAC_APDU");
    if (apdu_driver == NULL)
    {
        apdu_driver = "pcsc";
    }

    http_driver = getenv("LPAC_HTTP");
    if (http_driver == NULL)
    {
        http_driver = "curl";
    }

    if (euicc_driver_init(apdu_driver, http_driver))
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
