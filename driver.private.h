#pragma once

enum lpac_driver_type
{
    DRIVER_APDU,
    DRIVER_HTTP,
};

struct lpac_driver
{
    enum lpac_driver_type type;
    const char *name;
    int (*init)(void *interface);
    int (*main)(int argc, char **argv);
    void (*fini)(void);
};
