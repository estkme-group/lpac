#include "profile.h"

#include "profile/delete.h"
#include "profile/disable.h"
#include "profile/discovery.h"
#include "profile/download.h"
#include "profile/enable.h"
#include "profile/list.h"
#include "profile/nickname.h"

#include <stddef.h>

static const struct applet_entry *applets[] = {
    &applet_profile_list,   &applet_profile_enable,   &applet_profile_disable,   &applet_profile_nickname,
    &applet_profile_delete, &applet_profile_download, &applet_profile_discovery, NULL,
};

struct applet_entry applet_profile = {
    .name = "profile",
    .main = NULL,
    .subapplets = applets,
};
