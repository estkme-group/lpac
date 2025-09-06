#include "applet.h"

#include "main.h"

static const struct applet_entry *applets[] = {
    &(struct applet_entry){.name = "list", .main = applet_notification_list_main},
    &(struct applet_entry){.name = "process", .main = applet_notification_process_main},
    &(struct applet_entry){.name = "remove", .main = applet_notification_remove_main},
    &(struct applet_entry){.name = "dump", .main = applet_notification_dump_main},
    &(struct applet_entry){.name = "replay", .main = applet_notification_replay_main},
    NULL,
};

int applet_notification_main(const int argc, char **argv) {
    if (main_init_driver() != 0)
        return -1;
    if (main_init_euicc() != 0)
        return -1;
    return applet_entry(argc, argv, applets);
}
