#pragma once
#include "euicc.h"

#include "es10a.h"
#include "es10b.h"
#include "es10c.h"
#include "es10cex.h"

int es10x_init(struct euicc_ctx *ctx);
void es10x_fini(struct euicc_ctx *ctx);
