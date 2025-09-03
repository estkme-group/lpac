#include "applet.h"

#include "main.h"

static const struct applet_entry *applets[] = {
    &(struct applet_entry){.name = "list", .main = applet_profile_list_main},
    &(struct applet_entry){.name = "enable", .main = applet_profile_enable_main},
    &(struct applet_entry){.name = "disable", .main = applet_profile_disable_main},
    &(struct applet_entry){.name = "nickname", .main = applet_profile_nickname_main},
    &(struct applet_entry){.name = "delete", .main = applet_profile_delete_main},
    &(struct applet_entry){.name = "download", .main = applet_profile_download_main},
    &(struct applet_entry){.name = "discovery", .main = applet_profile_discovery_main},
    NULL,
};

int applet_profile_main(const int argc, char **argv) {
    if (main_init_driver() != 0)
        return -1;
    if (main_init_euicc() != 0)
        return -1;
    return applet_entry(argc, argv, applets);
}
