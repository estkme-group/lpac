// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2024, Luca Weiss <luca.weiss@fairphone.com>
 */

#include <stdio.h>

#include "qmi_common.h"

int qmi_apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len)
{
    struct qmi_data *qmi_priv = ctx->apdu.interface->userdata;
    g_autoptr(GError) error = NULL;
    g_autoptr(GArray) apdu_data = NULL;

    /* Convert tx into request GArray */
    apdu_data = g_array_new(FALSE, FALSE, sizeof(guint8));
    for (uint32_t i = 0; i < tx_len; i++)
        g_array_append_val(apdu_data, tx[i]);

    QmiMessageUimSendApduInput *input;
    input = qmi_message_uim_send_apdu_input_new();
    qmi_message_uim_send_apdu_input_set_slot(input, qmi_priv->uimSlot, NULL);
    qmi_message_uim_send_apdu_input_set_channel_id(input, qmi_priv->lastChannelId, NULL);
    qmi_message_uim_send_apdu_input_set_apdu(input, apdu_data, NULL);

    QmiMessageUimSendApduOutput *output;
    output = qmi_client_uim_send_apdu_sync(qmi_priv->uimClient, input, qmi_priv->context, &error);

    qmi_message_uim_send_apdu_input_unref(input);

    if (!qmi_message_uim_send_apdu_output_get_result(output, &error))
    {
        fprintf(stderr, "error: send apdu operation failed: %s\n", error->message);
        return -1;
    }

    GArray *apdu_res = NULL;
    if (!qmi_message_uim_send_apdu_output_get_apdu_response(output, &apdu_res, &error))
    {
        fprintf(stderr, "error: get apdu response operation failed: %s\n", error->message);
        return -1;
    }

    /* Convert response GArray into rx */
    *rx_len = apdu_res->len;
    *rx = malloc(*rx_len);
    if (!*rx)
        return -1;
    for (guint i = 0; i < apdu_res->len; i++)
        (*rx)[i] = apdu_res->data[i];

    qmi_message_uim_send_apdu_output_unref(output);

    return 0;
}

int qmi_apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len)
{
    struct qmi_data *qmi_priv = ctx->apdu.interface->userdata;
    g_autoptr(GError) error = NULL;
    guint8 channel_id;

    GArray *aid_data = g_array_new(FALSE, FALSE, sizeof(guint8));
    for (int i = 0; i < aid_len; i++)
        g_array_append_val(aid_data, aid[i]);

    QmiMessageUimOpenLogicalChannelInput *input;
    input = qmi_message_uim_open_logical_channel_input_new();
    qmi_message_uim_open_logical_channel_input_set_slot(input, qmi_priv->uimSlot, NULL);
    qmi_message_uim_open_logical_channel_input_set_aid(input, aid_data, NULL);

    QmiMessageUimOpenLogicalChannelOutput *output;
    output = qmi_client_uim_open_logical_channel_sync(qmi_priv->uimClient, input, qmi_priv->context, &error);

    qmi_message_uim_open_logical_channel_input_unref(input);
    g_array_unref(aid_data);

    if (!output)
    {
        fprintf(stderr, "error: send Open Logical Channel command failed: %s\n", error->message);
        return -1;
    }

    if (!qmi_message_uim_open_logical_channel_output_get_result(output, &error))
    {
        fprintf(stderr, "error: open logical channel operation failed: %s\n", error->message);
        return -1;
    }

    if (!qmi_message_uim_open_logical_channel_output_get_channel_id(output, &channel_id, &error))
    {
        fprintf(stderr, "error: get channel id operation failed: %s\n", error->message);
        return -1;
    }
    qmi_priv->lastChannelId = channel_id;

    g_debug("Opened logical channel with id %d", channel_id);

    qmi_message_uim_open_logical_channel_output_unref(output);

    return channel_id;
}

void qmi_apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel)
{
    struct qmi_data *qmi_priv = ctx->apdu.interface->userdata;
    g_autoptr(GError) error = NULL;

    QmiMessageUimLogicalChannelInput *input;
    input = qmi_message_uim_logical_channel_input_new();
    qmi_message_uim_logical_channel_input_set_slot(input, qmi_priv->uimSlot, NULL);
    qmi_message_uim_logical_channel_input_set_channel_id(input, channel, NULL);

    QmiMessageUimLogicalChannelOutput *output;
    output = qmi_client_uim_logical_channel_sync(qmi_priv->uimClient, input, qmi_priv->context, &error);

    qmi_message_uim_logical_channel_input_unref(input);

    if (error)
    {
        fprintf(stderr, "error: send Close Logical Channel command failed: %s\n", error->message);
        return;
    }

    if (!qmi_message_uim_logical_channel_output_get_result(output, &error))
    {
        fprintf(stderr, "error: logical channel operation failed: %s\n", error->message);
        return;
    }

    /* Mark channel as having been cleaned up */
    if (channel == qmi_priv->lastChannelId)
        qmi_priv->lastChannelId = -1;

    g_debug("Closed logical channel with id %d", channel);

    qmi_message_uim_logical_channel_output_unref(output);
}

void qmi_apdu_interface_disconnect(struct euicc_ctx *ctx)
{
    struct qmi_data *qmi_priv = ctx->apdu.interface->userdata;
    g_autoptr(GError) error = NULL;
    QmiClient *client = QMI_CLIENT(qmi_priv->uimClient);
    QmiDevice *device = QMI_DEVICE(qmi_client_get_device(client));

    qmi_device_release_client_sync(device, client, qmi_priv->context, &error);
    qmi_priv->uimClient = NULL;

    g_main_context_unref(qmi_priv->context);
    qmi_priv->context = NULL;
}

void qmi_cleanup(struct qmi_data *qmi_priv)
{
    if (qmi_priv->lastChannelId > 0)
    {
        fprintf(stderr, "Cleaning up leaked APDU channel %d\n", qmi_priv->lastChannelId);
        qmi_apdu_interface_logic_channel_close(NULL, qmi_priv->lastChannelId);
        qmi_priv->lastChannelId = -1;
    }
}
