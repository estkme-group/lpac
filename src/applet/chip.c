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
    const int ret = main_init();
    if (ret != 0)
        return ret;
    return applet_entry(argc, argv, applets);
}

struct applet_entry applet_chip = {
    .name = "chip",
    .main = applet_main,
};
