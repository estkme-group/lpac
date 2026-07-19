#pragma once

#ifdef interface
#    undef interface
#endif

#if defined(_WIN32)
#    ifdef DRIVER_EXPORTS
#        define DRIVER_API __declspec(dllexport)
#    else
#        define DRIVER_API __declspec(dllimport)
#    endif
#else
#    define DRIVER_API
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

#if defined(_WIN32)
#    define DRIVER_INTERFACE __declspec(dllexport) const struct euicc_driver driver_if
#else
#    define DRIVER_INTERFACE const struct euicc_driver driver_if
#endif
