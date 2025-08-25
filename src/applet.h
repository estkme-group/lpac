#pragma once

#include <stdbool.h>

struct applet_entry {
    const char *name;
    int (*main)(int argc, char **argv);
    bool skip_init_driver;
    bool skip_init_euicc;
    const struct applet_entry **subapplets;
};

int applet_entry(int argc, char **argv, const struct applet_entry **entries);
