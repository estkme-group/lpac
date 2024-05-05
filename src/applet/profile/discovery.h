#pragma once

#include <applet.h>
#include <stdbool.h>

extern struct applet_entry applet_profile_discovery;

static bool is_invalid_smds_address(const char *address);
