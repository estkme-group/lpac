#include "notification.h"

#include "main.h"
#include "notification/dump.h"
#include "notification/list.h"
#include "notification/process.h"
#include "notification/remove.h"
#include "notification/replay.h"

static const struct applet_entry *applets[] = {
    &applet_notification_list, &applet_notification_process, &applet_notification_remove,
    &applet_notification_dump, &applet_notification_replay,  NULL,
};

static int applet_main(const int argc, char **argv) {
    int ret = 0;
    if ((ret = main_init_driver()) != 0)
        return ret;
    if ((ret = main_init_euicc()) != 0)
        return ret;
    return applet_entry(argc, argv, applets);
}

struct applet_entry applet_notification = {
    .name = "notification",
    .main = applet_main,
};
