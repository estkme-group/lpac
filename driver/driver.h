#pragma once
#include <stddef.h>
#include <inttypes.h>
#include <euicc/interface.h>

extern struct euicc_apdu_interface driver_interface_apdu;
extern struct euicc_http_interface driver_interface_http;
extern int (*driver_main_apdu)(int argc, char **argv);
extern int (*driver_main_http)(int argc, char **argv);

int driver_init(void);
void driver_fini(void);
