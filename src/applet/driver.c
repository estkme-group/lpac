#include "driver.h"

#include "driver/driver.h"
#include "main.h"

#include <applet.h>
#include <string.h>

static const struct applet_entry *applets[] = {
    &(struct applet_entry){.name = "apdu", .main = euicc_driver_main_apdu},
    &(struct applet_entry){.name = "http", .main = euicc_driver_main_http},
    &(struct applet_entry){.name = "list", .main = euicc_driver_list},
    NULL,
};

int applet_driver_main(const int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "list") == 0)
        goto skip_init;
    if (main_init_driver() != 0)
        return -1;
skip_init:
    return applet_entry(argc, argv, applets);
}
