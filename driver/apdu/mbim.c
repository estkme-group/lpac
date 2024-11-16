// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2024, Frans Klaver <frans.klaver@vislink.com>
 */
#include "mbim.h"

#include <libmbim-glib.h>
#include <stdio.h>
#include <euicc/interface.h>
#include <euicc/euicc.h>
#include "mbim_helpers.h"

struct mbim_data {
    const char *device_path;
    int last_channel_id;
    gboolean use_proxy;
    guint32 uim_slot;
    GMainContext *context;
    MbimDevice *device;
};

static gboolean is_sim_available(struct mbim_data *mbim_priv)
{
    MbimMessage *request = mbim_message_subscriber_ready_status_query_new(NULL);
    g_autoptr(MbimMessage) response = mbim_device_command_sync(
        mbim_priv->device, mbim_priv->context, request, NULL
    );
    if (!response)
        return FALSE;

    MbimSubscriberReadyState ready_state;
    if (!mbim_message_subscriber_ready_status_response_parse(
        response, &ready_state, NULL, NULL, NULL, NULL, NULL, NULL
    )) {
        return FALSE;
    }

    switch (ready_state) {
    case MBIM_SUBSCRIBER_READY_STATE_NO_ESIM_PROFILE:
    case MBIM_SUBSCRIBER_READY_STATE_INITIALIZED:
        return TRUE;
    default:
        return FALSE;
    }
}

static int select_sim_slot(struct mbim_data *mbim_priv)
{
    g_autoptr(GError) error = NULL;

    MbimMessage *current_slot_request =
        mbim_message_ms_basic_connect_extensions_device_slot_mappings_query_new(NULL);

    g_autoptr(MbimMessage) current_slot_response = mbim_device_command_sync(
        mbim_priv->device, mbim_priv->context, current_slot_request, &error
    );
    if (!current_slot_response) {
        fprintf(stderr, "error: device didn't respond: %s\n", error->message);
        return -1;
    }

    guint32 current_slot_count;
    g_autoptr(MbimSlotArray) current_slots = NULL;
    if (!mbim_message_ms_basic_connect_extensions_device_slot_mappings_response_parse(
        current_slot_response, &current_slot_count, &current_slots, &error
    )) {
        fprintf(stderr, "error: sim select response could not be parsed: %s\n", error->message);
        return -1;
    }

    if (current_slot_count && current_slots[0]->slot == mbim_priv->uim_slot) {
        return 0;
    }

    g_autoptr(GPtrArray) new_slot_array = g_ptr_array_new_with_free_func(g_free);
    MbimSlot *new_slot = g_new(MbimSlot, 1);
    new_slot->slot = mbim_priv->uim_slot;
    g_ptr_array_add(new_slot_array, new_slot);

    MbimMessage *update_slot_request = mbim_message_ms_basic_connect_extensions_device_slot_mappings_set_new(
        new_slot_array->len, (const MbimSlot **)new_slot_array->pdata, &error
    );
    if (!update_slot_request) {
        fprintf(stderr, "error: unable to select sim slot: %s\n", error->message);
        return -1;
    }

    g_autoptr(MbimMessage) update_slot_response = mbim_device_command_sync(
        mbim_priv->device, mbim_priv->context, update_slot_request, &error
    );
    if (!update_slot_response) {
        fprintf(stderr, "error: device didn't respond: %s\n", error->message);
        return -1;
    }

    guint32 slot_count;
    g_autoptr(MbimSlotArray) updated_slots = NULL;
    if (!mbim_message_ms_basic_connect_extensions_device_slot_mappings_response_parse(
        update_slot_response, &slot_count, &updated_slots, &error
    )) {
        fprintf(stderr, "error: sim select response could not be parsed: %s\n", error->message);
        return -1;
    }

    int retries = 20;
    while (retries--) {
        if (is_sim_available(mbim_priv)) {
            return 0;
        }
    }

    fprintf(stderr, "sim did not become available\n");
    return -1;
}

static int apdu_interface_connect(struct euicc_ctx *ctx)
{
    struct mbim_data *mbim_priv = ctx->apdu.interface->userdata;
    g_autoptr(GError) error = NULL;
    GFile *file;

    file = g_file_new_for_path(mbim_priv->device_path);

    mbim_priv->context = g_main_context_new();

    mbim_priv->device = mbim_device_new_from_path(file, mbim_priv->context, &error);
    if (!mbim_priv->device) {
        fprintf(stderr, "error: create mbim device from path failed: %s\n", error->message);
        return -1;
    }

    MbimDeviceOpenFlags open_flags = MBIM_DEVICE_OPEN_FLAGS_NONE;
    if (mbim_priv->use_proxy)
        open_flags |= MBIM_DEVICE_OPEN_FLAGS_NONE;

    mbim_device_open_sync(mbim_priv->device, open_flags, mbim_priv->context, &error);
    if (error) {
        fprintf(stderr, "error: open mbim device failed: %s\n", error->message);
        return -1;
    }

    return select_sim_slot(mbim_priv);
}

/*
 * Allocate storage in rx and copy the contents of response_data there. Also
 * tack the status at the end, as the MBIM protocol separates the status from
 * the rest of the response.
 */
static int copy_data_with_status(
    uint8_t **rx, uint32_t *rx_len,
    const guint8 *response_data, guint32 response_size,
    guint32 status)
{
    *rx_len = response_size + 2;
    *rx = malloc(*rx_len);
    if (!*rx)
        return -1;

    memcpy(*rx, response_data, response_size);
    (*rx)[*rx_len - 2] = status & 0xff;
    (*rx)[*rx_len - 1] = (status >> 8) & 0xff;

    return 0;
}

static int mbim_apdu_interface_transmit(
    struct euicc_ctx *ctx,
    uint8_t **rx, uint32_t *rx_len,
    const uint8_t *tx, uint32_t tx_len)
{
    struct mbim_data *mbim_priv = ctx->apdu.interface->userdata;
    g_autoptr(GError) error = NULL;

    MbimMessage *request = mbim_message_ms_uicc_low_level_access_apdu_set_new(
        mbim_priv->last_channel_id,
        MBIM_UICC_SECURE_MESSAGING_NONE,
        MBIM_UICC_CLASS_BYTE_TYPE_INTER_INDUSTRY,
        tx_len,
        tx,
        &error
    );
    if (!request) {
        fprintf(stderr, "error: creating apdu message failed: %s\n", error->message);
        return -1;
    }

    g_autoptr(MbimMessage) response = mbim_device_command_sync(
        mbim_priv->device, mbim_priv->context, request, &error
    );
    if (!response) {
        fprintf(stderr, "error: no apdu response received: %s\n", error->message);
        return -1;
    }

    guint32 status = 0;
    guint32 response_size = 0;
    const guint8 *response_data = NULL;

    if (!mbim_message_ms_uicc_low_level_access_apdu_response_parse(
       response, &status, &response_size, &response_data, &error
   )) {
        fprintf(stderr, "error: unable to parse apdu response: %s\n", error->message);
        return -1;
    }

    return copy_data_with_status(rx, rx_len, response_data, response_size, status);
}

static int mbim_apdu_interface_logic_channel_open(
    struct euicc_ctx *ctx,
    const uint8_t *aid,
    uint8_t aid_len)
{
    struct mbim_data *mbim_priv = ctx->apdu.interface->userdata;
    g_autoptr(GError) error = NULL;
    guint8 channel_id;

    MbimMessage *request = mbim_message_ms_uicc_low_level_access_open_channel_set_new(
        aid_len, aid, 0, 1, &error
    );
    if (!request) {
        fprintf(stderr, "error: creating channel message failed: %s\n", error->message);
        return -1;
    }

    g_autoptr(MbimMessage) response = mbim_device_command_sync(
        mbim_priv->device, mbim_priv->context, request, &error
    );
    if (!response) {
        fprintf(stderr, "error: no channel response received: %s\n", error->message);
        return -1;
    }

    guint32 status = 0;
    guint32 channel = -1;
    guint32 response_size = 0;
    const guint8 *response_data = NULL;

    if (!mbim_message_ms_uicc_low_level_access_open_channel_response_parse(
        response, &status, &channel, &response_size, &response_data, &error
    )) {
        fprintf(stderr, "error: unable to parse channel response: %s\n", error->message);
        return -1;
    }

    mbim_priv->last_channel_id = channel;
    return channel;
}

static void mbim_apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel)
{
    struct mbim_data *mbim_priv = ctx->apdu.interface->userdata;
    g_autoptr(GError) error = NULL;

    MbimMessage *request = mbim_message_ms_uicc_low_level_access_close_channel_set_new(
        channel, 1, &error
    );
    if (!request) {
        fprintf(stderr, "error: creating channel message failed: %s\n", error->message);
        return;
    }

    g_autoptr(MbimMessage) response = mbim_device_command_sync(
        mbim_priv->device, mbim_priv->context, request, &error
    );
    if (!response) {
        fprintf(stderr, "error: no channel response received: %s\n", error->message);
        return;
    }

    guint32 status = 0;

    if (!mbim_message_ms_uicc_low_level_access_close_channel_response_parse(
        response, &status, &error
    )) {
        fprintf(stderr, "error: unable to parse channel response: %s\n", error->message);
        return;
    }

    if (channel == mbim_priv->last_channel_id)
        mbim_priv->last_channel_id = -1;
}

static void mbim_apdu_interface_disconnect(struct euicc_ctx *ctx)
{
    struct mbim_data *mbim_priv = ctx->apdu.interface->userdata;
    g_autoptr(GError) error = NULL;

    if (mbim_priv->last_channel_id > 0)
    {
        fprintf(stderr, "Cleaning up leaked APDU channel %d\n", mbim_priv->last_channel_id);
        mbim_apdu_interface_logic_channel_close(NULL, mbim_priv->last_channel_id);
        mbim_priv->last_channel_id = -1;
    }

    mbim_device_close_sync(mbim_priv->device, mbim_priv->context, &error);

    g_main_context_unref(mbim_priv->context);
    mbim_priv->context = NULL;
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct)
{
    struct mbim_data *mbim_priv;

    guint32 uim_slot = 0;
    /*
     * We're using the same UIM_SLOT environment variable as the QMI backends.
     * QMI uses 1-based indexing for the sim slots. MBIM uses 0-based indexing,
     * so account for that.
     */
    const char *env_uim_slot = getenv("UIM_SLOT");
    if (env_uim_slot) {
        uim_slot = atoi(env_uim_slot);
        if (uim_slot == 0) {
            fprintf(stderr, "error: Invalid UIM_SLOT: '%s'\n", env_uim_slot);
            return -1;
        }
        uim_slot--;
    }

    mbim_priv = malloc(sizeof(struct mbim_data));
    if (!mbim_priv) {
        fprintf(stderr, "Failed allocating memory\n");
        return -1;
    }

    mbim_priv->uim_slot = uim_slot;

    const char *use_proxy = getenv("MBIM_USE_PROXY");
    if (use_proxy && strcmp(use_proxy, "0") != 0) {
        mbim_priv->use_proxy = TRUE;
    }

    if (!(mbim_priv->device_path = getenv("MBIM_DEVICE"))) {
        mbim_priv->device_path = "/dev/cdc-wdm0";
    }

    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));
    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = mbim_apdu_interface_disconnect;
    ifstruct->logic_channel_open = mbim_apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = mbim_apdu_interface_logic_channel_close;
    ifstruct->transmit = mbim_apdu_interface_transmit;
    ifstruct->userdata = mbim_priv;

    return 0;
}

static int libapduinterface_main(int argc, char **argv)
{
    return 0;
}

static void libapduinterface_fini(struct euicc_apdu_interface *ifstruct)
{
    g_free(ifstruct->userdata);
}

const struct euicc_driver driver_apdu_mbim = {
    .type = DRIVER_APDU,
    .name = "mbim",
    .init = (int (*)(void *))libapduinterface_init,
    .main = libapduinterface_main,
    .fini = (void (*)(void *))libapduinterface_fini,
};
