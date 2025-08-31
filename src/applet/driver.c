#include "driver.h"
#include "driver/driver.h"

#include "main.h"

const struct applet_entry *applets[] = {
    &(struct applet_entry){
        .name = "apdu",
        .init = main_init_driver,
        .main = euicc_driver_main_apdu,
    },
    &(struct applet_entry){
        .name = "http",
        .init = main_init_driver,
        .main = euicc_driver_main_http,
    },
    &(struct applet_entry){
        .name = "list",
        .init = NULL,
        .main = euicc_driver_list,
    },
    NULL,
};

static int applet_main(const int argc, char **argv) { return applet_entry(argc, argv, applets); }

struct applet_entry applet_driver = {
    .name = "driver",
    .main = applet_main,
};
