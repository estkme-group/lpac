#pragma once

#ifdef interface
#    undef interface
#endif

enum euicc_driver_type {
    DRIVER_APDU,
    DRIVER_HTTP,
};

struct euicc_driver {
    enum euicc_driver_type type;
    const char *name;
    int (*init)(void *interface);
    int (*main)(void *interface, int argc, char **argv);
    void (*fini)(void *interface);
};
