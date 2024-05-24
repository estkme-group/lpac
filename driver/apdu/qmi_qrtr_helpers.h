// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2024, Luca Weiss <luca.weiss@fairphone.com>
 */

#include <libqrtr-glib.h>
#include <libqmi-glib.h>

QrtrBus *qrtr_bus_new_sync(
    GMainContext *context,
    GError **error);

QmiDevice *
qmi_device_new_from_node_sync(
    QrtrNode *node,
    GMainContext *context,
    GError **error);

gboolean
qmi_device_open_sync(
    QmiDevice *device,
    GMainContext *context,
    GError **error);

QmiClient *
qmi_device_allocate_client_sync(
    QmiDevice *device,
    GMainContext *context,
    GError **error);

gboolean
qmi_device_release_client_sync(
    QmiDevice *device,
    QmiClient *client,
    GMainContext *context,
    GError **error);

QmiMessageUimOpenLogicalChannelOutput *
qmi_client_uim_open_logical_channel_sync(
    QmiClientUim *client,
    QmiMessageUimOpenLogicalChannelInput *input,
    GMainContext *context,
    GError **error);

QmiMessageUimLogicalChannelOutput *
qmi_client_uim_logical_channel_sync(
    QmiClientUim *client,
    QmiMessageUimLogicalChannelInput *input,
    GMainContext *context,
    GError **error);

QmiMessageUimSendApduOutput *
qmi_client_uim_send_apdu_sync(
    QmiClientUim *client,
    QmiMessageUimSendApduInput *input,
    GMainContext *context,
    GError **error);
