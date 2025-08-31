#include "driver.h"

#include "driver/driver.h"
#include "main.h"

#include <string.h>

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

static int applet_main(const int argc, char **argv) {
    if (strcmp(argv[1], "list") == 0)
        goto entry;
    const int ret = main_init_driver();
    if (ret != 0)
        return ret;
entry:
    return applet_entry(argc, argv, applets);
}

struct applet_entry applet_driver = {
    .name = "driver",
    .main = applet_main,
};
