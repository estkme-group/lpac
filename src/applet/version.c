#include "version.h"

#include <main.h>

static int applet_main(int argc, char **argv)
{
    jprint_success(cJSON_CreateString(LPAC_VERSION));
    return 0;
}

struct applet_entry applet_version = {
        .name = "version",
        .main = applet_main,
};
