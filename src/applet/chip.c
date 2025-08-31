#include "chip.h"

#include "chip/defaultsmdp.h"
#include "chip/info.h"
#include "chip/purge.h"
#include "main.h"

static const struct applet_entry *applets[] = {
    &applet_chip_info,
    &applet_chip_defaultsmdp,
    &applet_chip_purge,
    NULL,
};

static int applet_main(const int argc, char **argv) {
    int ret = 0;
    if ((ret = main_init_driver()) != 0)
        return ret;
    if ((ret = main_init_euicc()) != 0)
        return ret;
    return applet_entry(argc, argv, applets);
}

struct applet_entry applet_chip = {
    .name = "chip",
    .main = applet_main,
};
