#include "notification.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <main.h>

#include "notification/list.h"
#include "notification/process.h"
#include "notification/remove.h"

static const struct applet_entry *applets[] = {
    &applet_notification_list,
    &applet_notification_process,
    &applet_notification_remove,
    NULL,
};

static int applet_main(int argc, char **argv)
{
    main_init_euicc();
    return applet_entry(argc, argv, applets);
}

struct applet_entry applet_notification = {
    .name = "notification",
    .main = applet_main,
};
