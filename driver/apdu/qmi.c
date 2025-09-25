// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2024, Robert Marko <robert.marko@sartura.hr>
 */
#include "qmi_common.h"

#include <driver.h>
#include <lpac/utils.h>

#include <libqmi-glib.h>
#include <stdio.h>

static gboolean is_sim_available(struct qmi_data *qmi_priv) {
    g_autoptr(GError) error = NULL;
    g_autoptr(QmiMessageUimGetCardStatusOutput) card_status_output = NULL;
    GArray *cards = NULL;
    guint i, j;

    // Get card status
    card_status_output = qmi_client_uim_get_card_status_sync(qmi_priv->uimClient, qmi_priv->context, &error);
    if (!card_status_output) {
        fprintf(stderr, "error: get card status failed: %s\n", error->message);
        return FALSE;
    }

    // Check if the operation was successful
    if (!qmi_message_uim_get_card_status_output_get_result(card_status_output, &error)) {
        fprintf(stderr, "error: get card status operation failed: %s\n", error->message);
        return FALSE;
    }

    // Get card status details
    if (!qmi_message_uim_get_card_status_output_get_card_status(card_status_output,
                                                                NULL, // index_gw_primary
                                                                NULL, // index_1x_primary
                                                                NULL, // index_gw_secondary
                                                                NULL, // index_1x_secondary
                                                                &cards, &error)) {
        fprintf(stderr, "error: get card status details failed: %s\n", error->message);
        return FALSE;
    }

    // Check if any card is present and has a USIM application that is ready
    for (i = 0; i < cards->len; i++) {
        QmiMessageUimGetCardStatusOutputCardStatusCardsElement *card_element;
        card_element = &g_array_index(cards, QmiMessageUimGetCardStatusOutputCardStatusCardsElement, i);

        // Check applications on this card
        for (j = 0; j < card_element->applications->len; j++) {
            QmiMessageUimGetCardStatusOutputCardStatusCardsElementApplicationsElementV2 *app_element;
            app_element =
                &g_array_index(card_element->applications,
                               QmiMessageUimGetCardStatusOutputCardStatusCardsElementApplicationsElementV2, j);

            // Check if this is a USIM application and if it's ready
            if (app_element->type == QMI_UIM_CARD_APPLICATION_TYPE_USIM
                && app_element->state == QMI_UIM_CARD_APPLICATION_STATE_READY) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static gboolean select_sim_slot(struct qmi_data *qmi_priv) {
    g_autoptr(GError) error = NULL;
    g_autoptr(QmiMessageUimGetSlotStatusOutput) slot_status_output = NULL;
    g_autoptr(QmiMessageUimSwitchSlotInput) switch_slot_input = NULL;
    g_autoptr(QmiMessageUimSwitchSlotOutput) switch_slot_output = NULL;
    g_autoptr(QmiMessageUimGetCardStatusOutput) card_status_output = NULL;
    GArray *physical_slot_status = NULL;
    guint8 active_slot = 0;
    guint8 target_slot;
    guint i;
    int retries;

    // Get the target slot (1-based indexing)
    target_slot = qmi_priv->uimSlot;

    // Get current slot status
    slot_status_output = qmi_client_uim_get_slot_status_sync(qmi_priv->uimClient, qmi_priv->context, &error);
    if (!slot_status_output) {
        fprintf(stderr, "error: get slot status failed: %s\n", error->message);
        return FALSE;
    }

    // Check if the operation was successful
    if (!qmi_message_uim_get_slot_status_output_get_result(slot_status_output, &error)) {
        // Some older devices do not support the GetSlotStatusRequest QMI command
        if (error->code == QMI_PROTOCOL_ERROR_NOT_SUPPORTED) {
            return TRUE;
        }
        fprintf(stderr, "error: get slot status operation failed: %s\n", error->message);
        return FALSE;
    }

    // Get physical slot status
    if (!qmi_message_uim_get_slot_status_output_get_physical_slot_status(slot_status_output, &physical_slot_status,
                                                                         &error)) {
        fprintf(stderr, "error: get physical slot status failed: %s\n", error->message);
        return FALSE;
    }

    // Find the active slot
    active_slot = 0;
    for (i = 0; i < physical_slot_status->len; i++) {
        QmiPhysicalSlotStatusSlot *element;
        element = &g_array_index(physical_slot_status, QmiPhysicalSlotStatusSlot, i);
        if (element->physical_slot_status == QMI_UIM_SLOT_STATE_ACTIVE) {
            active_slot = i + 1; // 1-based indexing
            break;
        }
    }

    // If the active slot is not the target slot, switch to the target slot
    if (active_slot != target_slot) {
        // Create switch slot input
        switch_slot_input = qmi_message_uim_switch_slot_input_new();

        if (!qmi_message_uim_switch_slot_input_set_logical_slot(switch_slot_input, 1, &error)) {
            fprintf(stderr, "error: set logical slot failed: %s\n", error->message);
            return FALSE;
        }

        // Set the physical slot (use the logical slot number as physical slot)
        if (!qmi_message_uim_switch_slot_input_set_physical_slot(switch_slot_input, target_slot, &error)) {
            fprintf(stderr, "error: set physical slot failed: %s\n", error->message);
            return FALSE;
        }

        // Switch to the target slot
        switch_slot_output =
            qmi_client_uim_switch_slot_sync(qmi_priv->uimClient, switch_slot_input, qmi_priv->context, &error);
        if (!switch_slot_output) {
            fprintf(stderr, "error: switch slot failed: %s\n", error->message);
            return FALSE;
        }

        // Check if the operation was successful
        if (!qmi_message_uim_switch_slot_output_get_result(switch_slot_output, &error)) {
            fprintf(stderr, "error: switch slot operation failed: %s\n", error->message);
            return FALSE;
        }

        // Wait for SIM to be available
        for (retries = 0; retries < 20; retries++) {
            if (is_sim_available(qmi_priv)) {
                return TRUE;
            }
            // Wait a bit and retry
            g_usleep(500000); // 0.5 seconds
        }

        fprintf(stderr, "error: SIM not available after switching slot\n");
        return FALSE;
    }

    return TRUE;
}

static int apdu_interface_connect(struct euicc_ctx *ctx) {
    struct qmi_data *qmi_priv = ctx->apdu.interface->userdata;
    g_autoptr(GError) error = NULL;
    QmiDevice *device = NULL;
    QmiClient *client = NULL;
    const char *device_path = getenv_or_default(ENV_DEVICE, "/dev/cdc-wdm0");
    GFile *file;

    if (device_path == NULL) {
        fprintf(stderr, "No QMI device path specified!\n");
        return -1;
    }
    file = g_file_new_for_path(device_path);

    qmi_priv->context = g_main_context_new();

    device = qmi_device_new_from_path(file, qmi_priv->context, &error);
    if (!device) {
        fprintf(stderr, "error: create QMI device from path failed: %s\n", error->message);
        return -1;
    }

    qmi_device_open_sync(device, QMI_DEVICE_OPEN_FLAGS_PROXY, qmi_priv->context, &error);
    if (error) {
        fprintf(stderr, "error: open QMI device failed: %s\n", error->message);
        return -1;
    }

    client = qmi_device_allocate_client_sync(device, qmi_priv->context, &error);
    if (!client) {
        fprintf(stderr, "error: allocate QMI client failed: %s\n", error->message);
        return -1;
    }

    qmi_priv->uimClient = QMI_CLIENT_UIM(client);
    if (select_sim_slot(qmi_priv) == FALSE) {
        fprintf(stderr, "error: select SIM slot failed\n");
        return -1;
    }

    // In QMI mode, we need to keep the SIM slot set to 1, because once the
    // configured slot becomes active, it will be assigned as slot 1.
    qmi_priv->uimSlot = 1;
    return 0;
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel) {
    struct qmi_data *qmi_priv = ctx->apdu.interface->userdata;
    qmi_apdu_interface_logic_channel_close(qmi_priv, channel);
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct) {
    set_deprecated_env_name(ENV_UIM_SLOT, "UIM_SLOT");
    set_deprecated_env_name(ENV_DEVICE, "QMI_DEVICE");

    struct qmi_data *qmi_priv;

    qmi_priv = calloc(1, sizeof(struct qmi_data));
    if (!qmi_priv) {
        fprintf(stderr, "Failed allocating memory\n");
        return -1;
    }

    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));
    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = qmi_apdu_interface_disconnect;
    ifstruct->logic_channel_open = qmi_apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = qmi_apdu_interface_transmit;

    /*
     * Allow the user to select the SIM card slot via environment variable.
     * Use the primary SIM slot if not set.
     */
    qmi_priv->uimSlot = getenv_or_default(ENV_UIM_SLOT, (int)1);

    ifstruct->userdata = qmi_priv;

    return 0;
}

static void libapduinterface_fini(struct euicc_apdu_interface *ifstruct) {
    struct qmi_data *qmi_priv = ifstruct->userdata;

    qmi_cleanup(qmi_priv);

    free(qmi_priv);
}

const struct euicc_driver driver_if = {
    .type = DRIVER_APDU,
    .name = "qmi",
    .init = (int (*)(void *))libapduinterface_init,
    .main = NULL,
    .fini = (void (*)(void *))libapduinterface_fini,
};
