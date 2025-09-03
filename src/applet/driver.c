#include "driver.h"

#include "driver/driver.h"

#include <applet.h>

static const struct applet_entry *applets[] = {
    &(struct applet_entry){.name = "apdu", .main = euicc_driver_main_apdu},
    &(struct applet_entry){.name = "http", .main = euicc_driver_main_http},
    &(struct applet_entry){.name = "list", .main = euicc_driver_list},
    NULL,
};

int applet_driver_main(const int argc, char **argv) { return applet_entry(argc, argv, applets); }
