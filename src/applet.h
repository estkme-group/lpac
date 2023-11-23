#pragma once

struct applet_entry
{
    const char *name;
    int (*main)(int argc, char **argv);
};

int applet_entry(int argc, char **argv, const struct applet_entry **entries);
