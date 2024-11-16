// SPDX-License-Identifier: MIT
/*
 Copyright (c) 2024, Frans Klaver <frans.klaver@vislink.com>
 */

#include "mbim_helpers.h"
#include <libmbim-glib.h>

static void
async_result_ready(GObject *source_object,
                   GAsyncResult *res,
                   gpointer user_data)
{
    GAsyncResult **result_out = user_data;

    g_assert(*result_out == NULL);
    *result_out = g_object_ref(res);
}

MbimDevice *
mbim_device_new_from_path(GFile *file,
                         GMainContext *context,
                         GError **error)
{
    g_autoptr(GMainContextPusher) pusher = NULL;
    g_autoptr(GAsyncResult) result = NULL;
    g_autofree gchar *id = NULL;

    pusher = g_main_context_pusher_new(context);

    id = g_file_get_path (file);
    if (id)
        mbim_device_new(file,
                       NULL,
                       async_result_ready,
                       &result);

    while (!result)
        g_main_context_iteration(context, TRUE);

    return mbim_device_new_finish(result, error);
}

gboolean
mbim_device_open_sync(MbimDevice *device,
                      MbimDeviceOpenFlags open_flags,
                     GMainContext *context,
                     GError **error)
{
    g_autoptr(GMainContextPusher) pusher = NULL;
    g_autoptr(GAsyncResult) result = NULL;

    pusher = g_main_context_pusher_new(context);

    mbim_device_open_full(device,
                    open_flags,
                    15,
                    NULL,
                    async_result_ready,
                    &result);

    while (!result)
        g_main_context_iteration(context, TRUE);

    return mbim_device_open_finish(device, result, error);
}

MbimMessage *
mbim_device_command_sync(MbimDevice *device, GMainContext *context, MbimMessage *request, GError **error)
{
    g_autoptr(GMainContextPusher) pusher = NULL;
    g_autoptr(GAsyncResult) result = NULL;

    pusher = g_main_context_pusher_new(context);

    mbim_device_command(device, request, 10, NULL, async_result_ready, &result);
    mbim_message_unref(request);

    while (result == NULL)
        g_main_context_iteration(context, TRUE);

    MbimMessage *response = mbim_device_command_finish(device, result, error);
    if (!response) {
        return NULL;
    }

    if (!mbim_message_response_get_result(response, MBIM_MESSAGE_TYPE_COMMAND_DONE, error)) {
        return NULL;
    }

    return response;
}

gboolean
mbim_device_close_sync(
    MbimDevice *device,
    GMainContext *context,
    GError **error)
{
    g_autoptr(GMainContextPusher) pusher = NULL;
    g_autoptr(GAsyncResult) result = NULL;

    pusher = g_main_context_pusher_new(context);

    mbim_device_close(device, 20, NULL, async_result_ready, &result);

    while (result == NULL)
        g_main_context_iteration(context, TRUE);

    return mbim_device_close_finish(device, result, error);
}
