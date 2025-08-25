#include "chip.h"

#include "chip/defaultsmdp.h"
#include "chip/info.h"
#include "chip/purge.h"

#include <stddef.h>

static const struct applet_entry *applets[] = {
    &applet_chip_info,
    &applet_chip_defaultsmdp,
    &applet_chip_purge,
    NULL,
};

struct applet_entry applet_chip = {
    .name = "chip",
    .main = NULL,
    .subapplets = applets,
};
