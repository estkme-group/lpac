#pragma once

enum euicc_driver_type
{
    DRIVER_APDU,
    DRIVER_HTTP,
};

struct euicc_driver
{
    enum euicc_driver_type type;
    const char *name;
    void *(*userdata)();
    int (*init)(void *interface);
    int (*main)(int argc, char **argv);
    void (*fini)(void *interface);
};
