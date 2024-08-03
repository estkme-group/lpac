#pragma once

#include <applet.h>
#include <euicc/es10b.h>

extern struct applet_entry applet_profile_download;

int download_cancel_session(struct euicc_ctx *ctx, enum es10b_cancel_session_reason reason);
