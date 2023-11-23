#include "chip.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <main.h>

#include <chip/info.h>
#include <chip/defaultsmdp.h>
#include <chip/purge.h>

static const struct applet_entry *applets[] = {
    &applet_chip_info,
    &applet_chip_defaultsmdp,
    &applet_chip_purge,
    NULL,
};

static int main(int argc, char **argv)
{
    return applet_entry(argc, argv, applets);
}

struct applet_entry applet_chip = {
    .name = "chip",
    .main = main,
};
