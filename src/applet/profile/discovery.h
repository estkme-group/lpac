#pragma once

#include <applet.h>
#include <stdbool.h>

extern struct applet_entry applet_profile_discovery;

static bool is_invalid_smds_address(const char *address);

static bool is_valid_fqdn_name(const char *name);
