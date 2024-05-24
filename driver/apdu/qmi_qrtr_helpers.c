// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2024, Luca Weiss <luca.weiss@fairphone.com>
 */

#include "qmi_qrtr_helpers.h"

static void
async_result_ready(GObject *source_object,
                   GAsyncResult *res,
                   gpointer user_data)
{
    GAsyncResult **result_out = user_data;

    g_assert(*result_out == NULL);
    *result_out = g_object_ref(res);
}

QrtrBus *
qrtr_bus_new_sync(GMainContext *context,
                  GError **error)
{
    g_autoptr(GMainContextPusher) pusher = NULL;
    g_autoptr(GAsyncResult) result = NULL;

    pusher = g_main_context_pusher_new(context);

    qrtr_bus_new(1000, /* ms */
                 NULL,
                 async_result_ready,
                 &result);

    while (result == NULL)
        g_main_context_iteration(context, TRUE);

    return qrtr_bus_new_finish(result, error);
}

QmiDevice *
qmi_device_new_from_node_sync(QrtrNode *node,
                              GMainContext *context,
                              GError **error)
{
    g_autoptr(GMainContextPusher) pusher = NULL;
    g_autoptr(GAsyncResult) result = NULL;

    pusher = g_main_context_pusher_new(context);

    qmi_device_new_from_node(node,
                             NULL,
                             async_result_ready,
                             &result);

    while (result == NULL)
        g_main_context_iteration(context, TRUE);

    return qmi_device_new_from_node_finish(result, error);
}

gboolean
qmi_device_open_sync(QmiDevice *device,
                     GMainContext *context,
                     GError **error)
{
    g_autoptr(GMainContextPusher) pusher = NULL;
    g_autoptr(GAsyncResult) result = NULL;

    pusher = g_main_context_pusher_new(context);

    qmi_device_open(device,
                    QMI_DEVICE_OPEN_FLAGS_NONE,
                    15,
                    NULL,
                    async_result_ready,
                    &result);

    while (result == NULL)
        g_main_context_iteration(context, TRUE);

    return qmi_device_open_finish(device, result, error);
}

QmiClient *
qmi_device_allocate_client_sync(QmiDevice *device,
                                GMainContext *context,
                                GError **error)
{
    g_autoptr(GMainContextPusher) pusher = NULL;
    g_autoptr(GAsyncResult) result = NULL;

    pusher = g_main_context_pusher_new(context);

    qmi_device_allocate_client(device,
                               QMI_SERVICE_UIM,
                               QMI_CID_NONE,
                               10,
                               NULL,
                               async_result_ready,
                               &result);

    while (result == NULL)
        g_main_context_iteration(context, TRUE);

    return qmi_device_allocate_client_finish(device, result, error);
}

gboolean
qmi_device_release_client_sync(QmiDevice *device,
                               QmiClient *client,
                               GMainContext *context,
                               GError **error)
{
    g_autoptr(GMainContextPusher) pusher = NULL;
    g_autoptr(GAsyncResult) result = NULL;

    pusher = g_main_context_pusher_new(context);

    qmi_device_release_client(device,
                              client,
                              QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
                              10,
                              NULL,
                              async_result_ready,
                              &result);

    while (result == NULL)
        g_main_context_iteration(context, TRUE);

    return qmi_device_release_client_finish(device, result, error);
}

QmiMessageUimOpenLogicalChannelOutput *
qmi_client_uim_open_logical_channel_sync(
    QmiClientUim *client,
    QmiMessageUimOpenLogicalChannelInput *input,
    GMainContext *context,
    GError **error)
{
    g_autoptr(GMainContextPusher) pusher = NULL;
    g_autoptr(GAsyncResult) result = NULL;

    pusher = g_main_context_pusher_new(context);

    qmi_client_uim_open_logical_channel(client,
                                        input,
                                        10,
                                        NULL,
                                        async_result_ready,
                                        &result);

    while (result == NULL)
        g_main_context_iteration(context, TRUE);

    return qmi_client_uim_open_logical_channel_finish(client, result, error);
}

QmiMessageUimLogicalChannelOutput *
qmi_client_uim_logical_channel_sync(
    QmiClientUim *client,
    QmiMessageUimLogicalChannelInput *input,
    GMainContext *context,
    GError **error)
{
    g_autoptr(GMainContextPusher) pusher = NULL;
    g_autoptr(GAsyncResult) result = NULL;

    pusher = g_main_context_pusher_new(context);

    qmi_client_uim_logical_channel(client,
                                   input,
                                   10,
                                   NULL,
                                   async_result_ready,
                                   &result);

    while (result == NULL)
        g_main_context_iteration(context, TRUE);

    return qmi_client_uim_logical_channel_finish(client, result, error);
}

QmiMessageUimSendApduOutput *
qmi_client_uim_send_apdu_sync(
    QmiClientUim *client,
    QmiMessageUimSendApduInput *input,
    GMainContext *context,
    GError **error)
{
    g_autoptr(GMainContextPusher) pusher = NULL;
    g_autoptr(GAsyncResult) result = NULL;

    pusher = g_main_context_pusher_new(context);

    qmi_client_uim_send_apdu(client,
                             input,
                             10,
                             NULL,
                             async_result_ready,
                             &result);

    while (result == NULL)
        g_main_context_iteration(context, TRUE);

    return qmi_client_uim_send_apdu_finish(client, result, error);
}
