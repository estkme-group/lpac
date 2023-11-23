#pragma once

#include <jprint.h>
#include <euicc/euicc.h>

extern struct euicc_ctx euicc_ctx;

void main_init_euicc(void);
void main_fini_euicc(void);
