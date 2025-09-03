#include "applet.h"

#include "main.h"

static const struct applet_entry *applets[] = {
    &applet_notification_list, &applet_notification_process, &applet_notification_remove,
    &applet_notification_dump, &applet_notification_replay,  NULL,
};

static int applet_main(const int argc, char **argv) {
    const int ret = main_init_euicc();
    if (ret != 0)
        return ret;
    return applet_entry(argc, argv, applets);
}

struct applet_entry applet_notification = {
    .name = "notification",
    .main = applet_main,
};
