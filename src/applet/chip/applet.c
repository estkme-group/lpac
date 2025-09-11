#include "applet.h"

#include "main.h"

#include <applet.h>

static const struct applet_entry *applets[] = {
    &(struct applet_entry){.name = "info", .main = applet_chip_info_main},
    &(struct applet_entry){.name = "defaultsmdp", .main = applet_chip_default_smdp_main},
    &(struct applet_entry){.name = "purge", .main = applet_chip_purge_main},
    NULL,
};

int applet_chip_main(const int argc, char **argv) {
    if (main_init_driver() != 0)
        return -1;
    if (main_init_euicc() != 0)
        return -1;
    return applet_entry(argc, argv, applets);
}
