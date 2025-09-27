#include "main.h"

#include "applet.h"
#include "applet/chip.h"
#include "applet/notification.h"
#include "applet/profile.h"
#include "applet/version.h"

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <euicc-driver-loader.h>
#include <euicc/euicc.h>
#include <euicc/hexutil.h>
#include <lpac/utils.h>

#ifdef WIN32
#    include <windef.h>
// windef.h MUST before other Windows headers
#    include <processenv.h>
#    include <processthreadsapi.h>
#    include <shellapi.h>
#    include <stringapiset.h>
#    include <winbase.h>
#endif

#define ENV_HTTP_DEBUG ENV_HTTP_DRIVER "_DEBUG"
#define ENV_APDU_DEBUG ENV_APDU_DRIVER "_DEBUG"

#define ENV_ISD_R_AID CUSTOM_ENV_NAME(ISD_R_AID)
#define ISD_R_AID_MIN_LENGTH 2
#define ISD_R_AID_MAX_LENGTH 32

#define ENV_ES10X_MSS CUSTOM_ENV_NAME(ES10X_MSS)
#define ES10X_MSS_MIN_VALUE 6
#define ES10X_MSS_MAX_VALUE 255

static int driver_applet_main(const int argc, char **argv) {
    const struct applet_entry *applets[] = {
        &(struct applet_entry){
            .name = "apdu",
            .main = euicc_driver_main_apdu,
        },
        &(struct applet_entry){
            .name = "http",
            .main = euicc_driver_main_http,
        },
        &(struct applet_entry){
            .name = "list",
            .main = euicc_driver_list,
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
    &driver_applet, &applet_chip, &applet_profile, &applet_notification, &applet_version, NULL,
};

static int euicc_ctx_inited = 0;
struct euicc_ctx euicc_ctx = {0};

static bool setup_isdr_aid(const uint8_t **aid, uint8_t *aid_len) {
    *aid = NULL;
    *aid_len = 0;

    const char *value = getenv_or_default(ENV_ISD_R_AID, "A0000005591010FFFFFFFF8900000100");
    const size_t n = strlen(value);
    if (n % 2 != 0 || n < ISD_R_AID_MIN_LENGTH || n > ISD_R_AID_MAX_LENGTH)
        return false;

    *aid_len = (uint8_t)n / 2;
    *aid = calloc(*aid_len, sizeof(uint8_t));
    return euicc_hexutil_hex2bin_r((uint8_t *)*aid, *aid_len, value, (uint32_t)n) > 0;
}

static bool setup_es10x_mss(uint8_t *mss) {
    *mss = 0;

    const long value = getenv_or_default(ENV_ES10X_MSS, (long)0);
    if (value == 0)
        return true; // use default
    if (errno == ERANGE || value < ES10X_MSS_MIN_VALUE || value > ES10X_MSS_MAX_VALUE)
        return false;

    *mss = (uint8_t)value;
    return true;
}

static bool setup_logger(FILE **apdu_log_fp, FILE **http_log_fp) {
    set_deprecated_env_name(ENV_APDU_DEBUG, "LIBEUICC_DEBUG_APDU");
    set_deprecated_env_name(ENV_HTTP_DEBUG, "LIBEUICC_DEBUG_HTTP");

    *apdu_log_fp = NULL;
    *http_log_fp = NULL;

    if (stderr == NULL)
        return false;

    if (getenv_or_default(ENV_APDU_DEBUG, (bool)false))
        *apdu_log_fp = stderr;
    if (getenv_or_default(ENV_HTTP_DEBUG, (bool)false))
        *http_log_fp = stderr;
    return true;
}

int main_init_euicc(void) {
    if (!setup_isdr_aid(&euicc_ctx.aid, &euicc_ctx.aid_len)) {
        jprint_error("euicc_init", "invalid custom ISD-R applet id given");
        return -1;
    }
    if (!setup_es10x_mss(&euicc_ctx.es10x_mss)) {
        jprint_error("euicc_init", "invalid custom ES10x MSS given");
        return -1;
    }
    if (!setup_logger(&euicc_ctx.apdu.log_fp, &euicc_ctx.http.log_fp)) {
        jprint_error("euicc_init", "invalid log file given");
        return -1;
    }
    if (euicc_init(&euicc_ctx)) {
        jprint_error("euicc_init", NULL);
        return -1;
    }
    euicc_ctx_inited = 1;
    return 0;
}

void main_fini_euicc(void) {
    if (!euicc_ctx_inited) {
        return;
    }
    euicc_fini(&euicc_ctx);
    euicc_ctx_inited = 0;
}

#ifdef WIN32
bool check_windows_version() {
    DWORDLONG dwlConditionMask = 0;

    OSVERSIONINFOEX osvi = {
        .dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX),
        .dwMajorVersion = 10,
        .dwMinorVersion = 0,
        .dwBuildNumber = 18362 // Windows 10 1903
    };

    VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);

    // Only work when manifest embedded correctly.
    return VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, dwlConditionMask);
}

static char **warg_to_arg(const int wargc, wchar_t **wargv) {
    char **argv = calloc((size_t)wargc, sizeof(char *));
    if (argv == NULL) {
        return NULL;
    }
    for (int i = 0; i < wargc; ++i) {
        const int size = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, NULL, 0, NULL, NULL);
        argv[i] = malloc((size_t)size);
        if (argv[i] == NULL) {
            for (int j = 0; j < i; ++j) {
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

int main(int argc, char **argv) {
    int ret = 0;

    setlocale(LC_ALL, "C.UTF-8");

    memset(&euicc_ctx, 0, sizeof(euicc_ctx));

    const char *apdu_driver = getenv(ENV_APDU_DRIVER);

    const char *http_driver = getenv(ENV_HTTP_DRIVER);

    if (euicc_driver_init(apdu_driver, http_driver)) {
        return -1;
    }

    euicc_ctx.apdu.interface = &euicc_driver_interface_apdu;
    euicc_ctx.http.interface = &euicc_driver_interface_http;

#ifdef WIN32
    if (!check_windows_version()) {
        if (GetLastError() == ERROR_OLD_WIN_VERSION) {
            fputs("lpac requires Windows 10 version 1903 or above to run correctly.\n", stderr);
        } else {
            fputs("Failed to get Windows version.\n", stderr);
        }
        return -1;
    }
    argv = warg_to_arg(argc, CommandLineToArgvW(GetCommandLineW(), &argc));
    if (argv == NULL) {
        return -1;
    }
#endif

    ret = applet_entry(argc, argv, applets);

    main_fini_euicc();

    euicc_driver_fini();

    return ret;
}
