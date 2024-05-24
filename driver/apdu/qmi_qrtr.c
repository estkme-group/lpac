// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2024, Luca Weiss <luca.weiss@fairphone.com>
 */
#include "qmi_qrtr.h"

#include <euicc/interface.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libqrtr-glib.h>
#include "qmi_qrtr_helpers.h"

static int lastChannelId = -1;
static int uimSlot = -1;
static GMainContext *context = NULL;
static QrtrBus *bus = NULL;
static QmiClientUim *uimClient = NULL;

static int apdu_interface_connect(struct euicc_ctx *ctx)
{
    g_autoptr(GError) error = NULL;
    QrtrNode *node = NULL;
    QmiDevice *device = NULL;
    QmiClient *client = NULL;
    bool found = false;

    context = g_main_context_new();

    bus = qrtr_bus_new_sync(context, &error);
    if (bus == NULL)
    {
        fprintf(stderr, "error: connect to QRTR bus failed: %s\n", error->message);
        return -1;
    }

    /* Find QRTR node for UIM service */
    for (GList *l = qrtr_bus_peek_nodes(bus); l != NULL; l = l->next)
    {
        node = l->data;

        if (node && qrtr_node_lookup_port(node, QMI_SERVICE_UIM) >= 0)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        fprintf(stderr, "error: find QRTR node with UIM service failed\n");
        return -1;
    }

    device = qmi_device_new_from_node_sync(node, context, &error);
    if (!device)
    {
        fprintf(stderr, "error: create QMI device from QRTR node failed: %s\n", error->message);
        return -1;
    }

    qmi_device_open_sync(device, context, &error);
    if (error)
    {
        fprintf(stderr, "error: open QMI device failed: %s\n", error->message);
        return -1;
    }

    client = qmi_device_allocate_client_sync(device, context, &error);
    if (!client)
    {
        fprintf(stderr, "error: allocate QMI client failed: %s\n", error->message);
        return -1;
    }

    uimClient = QMI_CLIENT_UIM(client);

    return 0;
}

static void apdu_interface_disconnect(struct euicc_ctx *ctx)
{
    g_autoptr(GError) error = NULL;
    QmiClient *client = QMI_CLIENT(uimClient);
    QmiDevice *device = QMI_DEVICE(qmi_client_get_device(client));

    qmi_device_release_client_sync(device, client, context, &error);
    uimClient = NULL;

    g_main_context_unref(context);
    context = NULL;
}

static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GArray) apdu_data = NULL;

    /* Convert tx into request GArray */
    apdu_data = g_array_new(FALSE, FALSE, sizeof(guint8));
    for (uint32_t i = 0; i < tx_len; i++)
        g_array_append_val(apdu_data, tx[i]);

    QmiMessageUimSendApduInput *input;
    input = qmi_message_uim_send_apdu_input_new();
    qmi_message_uim_send_apdu_input_set_slot(input, uimSlot, NULL);
    qmi_message_uim_send_apdu_input_set_channel_id(input, lastChannelId, NULL);
    qmi_message_uim_send_apdu_input_set_apdu(input, apdu_data, NULL);

    QmiMessageUimSendApduOutput *output;
    output = qmi_client_uim_send_apdu_sync(uimClient, input, context, &error);

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

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len)
{
    g_autoptr(GError) error = NULL;
    guint8 channel_id;

    GArray *aid_data = g_array_new(FALSE, FALSE, sizeof(guint8));
    for (int i = 0; i < aid_len; i++)
        g_array_append_val(aid_data, aid[i]);

    QmiMessageUimOpenLogicalChannelInput *input;
    input = qmi_message_uim_open_logical_channel_input_new();
    qmi_message_uim_open_logical_channel_input_set_slot(input, uimSlot, NULL);
    qmi_message_uim_open_logical_channel_input_set_aid(input, aid_data, NULL);

    QmiMessageUimOpenLogicalChannelOutput *output;
    output = qmi_client_uim_open_logical_channel_sync(uimClient, input, context, &error);

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
    lastChannelId = channel_id;

    g_debug("Opened logical channel with id %d", channel_id);

    qmi_message_uim_open_logical_channel_output_unref(output);

    return channel_id;
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel)
{
    g_autoptr(GError) error = NULL;

    QmiMessageUimLogicalChannelInput *input;
    input = qmi_message_uim_logical_channel_input_new();
    qmi_message_uim_logical_channel_input_set_slot(input, uimSlot, NULL);
    qmi_message_uim_logical_channel_input_set_channel_id(input, channel, NULL);

    QmiMessageUimLogicalChannelOutput *output;
    output = qmi_client_uim_logical_channel_sync(uimClient, input, context, &error);

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
    if (channel == lastChannelId)
        lastChannelId = -1;

    g_debug("Closed logical channel with id %d", channel);

    qmi_message_uim_logical_channel_output_unref(output);
}

static void cleanup(void)
{
    if (lastChannelId != -1)
    {
        fprintf(stderr, "Cleaning up leaked APDU channel %d\n", lastChannelId);
        apdu_interface_logic_channel_close(NULL, lastChannelId);
        lastChannelId = -1;
    }
}

static void sighandler(int sig)
{
    // This triggers atexit() hooks and therefore call cleanup()
    exit(0);
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct)
{
    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));

    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;

    // Install cleanup routine
    atexit(cleanup);
    signal(SIGINT, sighandler);

    /*
     * Allow the user to select the SIM card slot via environment variable.
     * Use the primary SIM slot if not set.
     */
    if (getenv("UIM_SLOT"))
    {
        uimSlot = atoi(getenv("UIM_SLOT"));
    }
    else
    {
        uimSlot = 1;
    }

    return 0;
}

static int libapduinterface_main(int argc, char **argv)
{
    return 0;
}

static void libapduinterface_fini(void)
{
}

const struct euicc_driver driver_apdu_qmi_qrtr = {
    .type = DRIVER_APDU,
    .name = "qmi_qrtr",
    .init = (int (*)(void *))libapduinterface_init,
    .main = libapduinterface_main,
    .fini = libapduinterface_fini,
};
