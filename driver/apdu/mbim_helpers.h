// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2024, Frans Klaver <frans.klaver@vislink.com>
 */

#include <libmbim-glib.h>

MbimDevice *
mbim_device_new_from_path(
    GFile *file,
    GMainContext *context,
    GError **error);

gboolean
mbim_device_open_sync(
    MbimDevice *device,
    MbimDeviceOpenFlags open_flags,
    GMainContext *context,
    GError **error);

gboolean
mbim_device_close_sync(
    MbimDevice *device,
    GMainContext *context,
    GError **error);

MbimMessage *
mbim_device_command_sync(
    MbimDevice *device,
    GMainContext *context,
    MbimMessage *request,
    GError **error);
