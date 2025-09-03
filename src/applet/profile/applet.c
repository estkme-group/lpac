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
    const int ret = main_init_euicc();
    if (ret != 0)
        return ret;
    return applet_entry(argc, argv, applets);
}
