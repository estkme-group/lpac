#pragma once

#include <euicc/euicc.h>
#include <jprint.h>

extern struct euicc_ctx euicc_ctx;

int main_init_euicc(void);
void main_fini_euicc(void);
