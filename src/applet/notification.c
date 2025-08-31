#include "notification.h"

#include "main.h"
#include "notification/dump.h"
#include "notification/list.h"
#include "notification/process.h"
#include "notification/remove.h"
#include "notification/replay.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const struct applet_entry *applets[] = {
    &applet_notification_list, &applet_notification_process, &applet_notification_remove,
    &applet_notification_dump, &applet_notification_replay,  NULL,
};

static int applet_main(const int argc, char **argv) { return applet_entry(argc, argv, applets); }

struct applet_entry applet_notification = {
    .name = "notification",
    .init = main_init,
    .main = applet_main,
};
