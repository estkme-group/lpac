#pragma once
#include <euicc/interface.h>

extern struct euicc_apdu_interface dlsym_apdu_interface;
extern struct euicc_http_interface dlsym_http_interface;

int dlsym_interface_init(void);
