#pragma once
#include <stddef.h>
#include <inttypes.h>
#include <euicc/interface.h>

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

extern struct euicc_apdu_interface driver_interface_apdu;
extern struct euicc_http_interface driver_interface_http;
extern int (*driver_main_apdu)(int argc, char **argv);
extern int (*driver_main_http)(int argc, char **argv);

int driver_init(void);
void driver_fini(void);
