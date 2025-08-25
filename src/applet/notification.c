#include "notification.h"

#include "notification/dump.h"
#include "notification/list.h"
#include "notification/process.h"
#include "notification/remove.h"
#include "notification/replay.h"

#include <stddef.h>

static const struct applet_entry *applets[] = {
    &applet_notification_list, &applet_notification_process, &applet_notification_remove,
    &applet_notification_dump, &applet_notification_replay,  NULL,
};

struct applet_entry applet_notification = {
    .name = "notification",
    .main = NULL,
    .subapplets = applets,
};
