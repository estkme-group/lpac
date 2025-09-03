#include "entry.h"

#include <applet.h>

#include <stdlib.h>

#include "chip/applet.h"
#include "driver.h"
#include "notification/applet.h"
#include "profile/applet.h"
#include "version.h"

static const struct applet_entry *applets[] = {
    &(struct applet_entry){.name = "driver", .main = applet_driver_main},
    &(struct applet_entry){.name = "chip", .main = applet_chip_main},
    &(struct applet_entry){.name = "profile", .main = applet_profile_main},
    &(struct applet_entry){.name = "notification", .main = applet_notification_main},
    &(struct applet_entry){.name = "version", .main = applet_version_main},
    NULL,
};

int applet_main(const int argc, char **argv) { return applet_entry(argc, argv, applets); }
