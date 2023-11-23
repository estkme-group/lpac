#include "profile.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <main.h>

#include "profile/list.h"
#include "profile/enable.h"
#include "profile/disable.h"
#include "profile/nickname.h"
#include "profile/delete.h"
#include "profile/download.h"
#include "profile/discovery.h"

static const struct applet_entry *applets[] = {
    &applet_profile_list,
    &applet_profile_enable,
    &applet_profile_disable,
    &applet_profile_nickname,
    &applet_profile_delete,
    &applet_profile_download,
    &applet_profile_discovery,
    NULL,
};

static int applet_main(int argc, char **argv)
{
    main_init_euicc();
    return applet_entry(argc, argv, applets);
}

struct applet_entry applet_profile = {
    .name = "profile",
    .main = applet_main,
};
