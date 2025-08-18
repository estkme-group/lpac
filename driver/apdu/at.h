#pragma once

#include <driver.private.h>

// This driver is for AT commands over a standard APDU interface.
extern const struct euicc_driver driver_apdu_at;

// This driver is for AT commands over a CSIM interface.
extern const struct euicc_driver driver_apdu_at_csim;
