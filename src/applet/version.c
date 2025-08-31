#include "version.h"
#include "main.h"

#ifndef LPAC_VERSION
#    define LPAC_VERSION "v0.0.0-unknown"
#endif

static int applet_main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
    jprint_success(cJSON_CreateString(LPAC_VERSION));
    return 0;
}

struct applet_entry applet_version = {
    .name = "version",
    .init = NULL,
    .main = applet_main,
};
