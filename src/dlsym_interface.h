#pragma once
#include <euicc/interface.h>

extern struct euicc_apdu_interface dlsym_apdu_interface;
extern struct euicc_es9p_interface dlsym_es9p_interface;

int dlsym_interface_init(void);
