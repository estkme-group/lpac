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

#define ENV_ISD_R_AID "LPAC_CUSTOM_ISD_R_AID"
#define ISD_R_AID_MAX_LENGTH 16

#define ENV_ES10X_MSS "LPAC_CUSTOM_ES10X_MSS"
#define ES10X_MSS_MIN_VALUE 6
#define ES10X_MSS_MAX_VALUE 255

#define ENV_APDU_DRIVER "LPAC_APDU"
#define ENV_HTTP_DRIVER "LPAC_HTTP"

static int driver_applet_main(const int argc, char **argv)
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

static int setup_aid(const uint8_t **aid, uint8_t *aid_len) {
    *aid = NULL;
    *aid_len = 0;

    const char *value = getenv(ENV_ISD_R_AID);
    if (value == NULL) return 0;

    uint8_t *parsed = malloc(ISD_R_AID_MAX_LENGTH);
    const int n = euicc_hexutil_hex2bin(parsed, ISD_R_AID_MAX_LENGTH, value);
    if (n < 1) return -1;

    *aid = parsed;
    *aid_len = n;
    return 0;
}

static int setup_mss(uint8_t *mss) {
    *mss = 0;

    const char *value = getenv(ENV_ES10X_MSS);
    if (value == NULL) return 0;

    const long parsed = strtol(value, NULL, 10);
    if (parsed == 0) return 0;
    if (parsed < ES10X_MSS_MIN_VALUE) return -1;
    if (parsed > ES10X_MSS_MAX_VALUE) return -1;

    *mss = (uint8_t) parsed;
    return 0;
}

int main_init_euicc()
{
    if (setup_aid(&euicc_ctx.aid, &euicc_ctx.aid_len))
    {
        jprint_error("euicc_init", "invalid custom ISD-R applet id given");
        return -1;
    }
    if (setup_mss(&euicc_ctx.es10x_mss))
    {
        jprint_error("euicc_init", "invalid custom ES10x MSS given");
        return -1;
    }
    if (euicc_init(&euicc_ctx))
    {
        jprint_error("euicc_init", NULL);
        return -1;
    }
    euicc_ctx_inited = 1;
    return 0;
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

    apdu_driver = getenv(ENV_APDU_DRIVER);
    if (apdu_driver == NULL)
    {
        apdu_driver = "pcsc";
    }

    http_driver = getenv(ENV_HTTP_DRIVER);
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
