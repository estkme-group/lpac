#include "chip.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <main.h>

#include "chip/defaultsmdp.h"
#include "chip/info.h"
#include "chip/purge.h"

static const struct applet_entry *applets[] = {
    &applet_chip_info,
    &applet_chip_defaultsmdp,
    &applet_chip_purge,
    NULL,
};

static int applet_main(const int argc, char **argv)
{
    const int ret = main_init_euicc();
    if (ret != 0)
        return ret;
    return applet_entry(argc, argv, applets);
}

struct applet_entry applet_chip = {
    .name = "chip",
    .main = applet_main,
};
