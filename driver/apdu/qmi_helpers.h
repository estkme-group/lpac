// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2024, Luca Weiss <luca.weiss@fairphone.com>
 */
#pragma once

#include <lpac/utils.h>

#include <libqmi-glib.h>

#define ENV_UIM_SLOT APDU_ENV_NAME(QMI, UIM_SLOT)
#define ENV_DEVICE APDU_ENV_NAME(QMI, DEVICE)

/*
 * If set to a truthy value, the SIM will be power-cycled (SIM Power Off
 * followed by SIM Power On) right after a profile has been successfully
 * enabled or disabled, so that the modem re-reads the (u)SIM.
 */
#define ENV_SIM_REFRESH APDU_ENV_NAME(QMI, SIM_REFRESH_AFTER_PROFILE_TOGGLE)

/* Delay in milliseconds between SIM Power Off and SIM Power On. Defaults to 1000ms. */
#define ENV_SIM_REFRESH_DELAY_MS APDU_ENV_NAME(QMI, SIM_REFRESH_DELAY_MS)

#ifdef LPAC_WITH_DRIVER_APDU_QMI_QRTR
#    include <libqrtr-glib.h>

QrtrBus *qrtr_bus_new_sync(GMainContext *context, GError **error);

QmiDevice *qmi_device_new_from_node_sync(QrtrNode *node, GMainContext *context, GError **error);
#endif

#ifdef LPAC_WITH_DRIVER_APDU_QMI
QmiDevice *qmi_device_new_from_path(GFile *file, GMainContext *context, GError **error);
#endif

gboolean qmi_device_open_sync(QmiDevice *device, QmiDeviceOpenFlags flags, GMainContext *context, GError **error);

QmiClient *qmi_device_allocate_client_sync(QmiDevice *device, GMainContext *context, GError **error);

gboolean qmi_device_release_client_sync(QmiDevice *device, QmiClient *client, GMainContext *context, GError **error);

QmiMessageUimOpenLogicalChannelOutput *
qmi_client_uim_open_logical_channel_sync(QmiClientUim *client, QmiMessageUimOpenLogicalChannelInput *input,
                                         GMainContext *context, GError **error);

QmiMessageUimLogicalChannelOutput *qmi_client_uim_logical_channel_sync(QmiClientUim *client,
                                                                       QmiMessageUimLogicalChannelInput *input,
                                                                       GMainContext *context, GError **error);

QmiMessageUimSendApduOutput *qmi_client_uim_send_apdu_sync(QmiClientUim *client, QmiMessageUimSendApduInput *input,
                                                           GMainContext *context, GError **error);

QmiMessageUimGetSlotStatusOutput *qmi_client_uim_get_slot_status_sync(QmiClientUim *client, GMainContext *context,
                                                                      GError **error);

QmiMessageUimSwitchSlotOutput *qmi_client_uim_switch_slot_sync(QmiClientUim *client,
                                                               QmiMessageUimSwitchSlotInput *input,
                                                               GMainContext *context, GError **error);

QmiMessageUimGetCardStatusOutput *qmi_client_uim_get_card_status_sync(QmiClientUim *client, GMainContext *context,
                                                                      GError **error);

QmiMessageUimPowerOffSimOutput *qmi_client_uim_power_off_sim_sync(QmiClientUim *client,
                                                                   QmiMessageUimPowerOffSimInput *input,
                                                                   GMainContext *context, GError **error);

QmiMessageUimPowerOnSimOutput *qmi_client_uim_power_on_sim_sync(QmiClientUim *client,
                                                                 QmiMessageUimPowerOnSimInput *input,
                                                                 GMainContext *context, GError **error);
