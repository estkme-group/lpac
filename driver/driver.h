#pragma once
#include <stddef.h>
#include <inttypes.h>
#include <euicc/interface.h>

extern struct euicc_apdu_interface euicc_driver_interface_apdu;
extern struct euicc_http_interface euicc_driver_interface_http;
extern int (*euicc_driver_main_apdu)(int argc, char **argv);
extern int (*euicc_driver_main_http)(int argc, char **argv);

int euicc_driver_init(const char *apdu_driver_name, const char *http_driver_name);
void euicc_driver_fini(void);
