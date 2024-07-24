// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2024, Robert Marko <robert.marko@sartura.hr>
 */
#include "qmi.h"

#include <stdio.h>
#include "qmi_common.h"

static int apdu_interface_connect(struct euicc_ctx *ctx)
{
    struct qmi_data *qmi_priv = ctx->apdu.interface->userdata;
    g_autoptr(GError) error = NULL;
    QmiDevice *device = NULL;
    QmiClient *client = NULL;
    const char *device_path;
    GFile *file;

    if (!(device_path = getenv("QMI_DEVICE"))) {
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

    qmi_device_open_sync(device, qmi_priv->context, &error);
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

    return 0;
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct)
{
    struct qmi_data *qmi_priv;

    qmi_priv = malloc(sizeof(struct qmi_data));
    if(!qmi_priv) {
        fprintf(stderr, "Failed allocating memory\n");
        return -1;
    }

    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));
    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = qmi_apdu_interface_disconnect;
    ifstruct->logic_channel_open = qmi_apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = qmi_apdu_interface_logic_channel_close;
    ifstruct->transmit = qmi_apdu_interface_transmit;

    /*
    * Allow the user to select the SIM card slot via environment variable.
    * Use the primary SIM slot if not set.
    */
    if (getenv("UIM_SLOT"))
        qmi_priv->uimSlot = atoi(getenv("UIM_SLOT"));
    else
        qmi_priv->uimSlot = 1;

    ifstruct->userdata = qmi_priv;

    return 0;
}

static int libapduinterface_main(int argc, char **argv)
{
    return 0;
}

static void libapduinterface_fini(struct euicc_apdu_interface *ifstruct)
{
    struct qmi_data *qmi_priv = ifstruct->userdata;

    qmi_cleanup(qmi_priv);

    free(qmi_priv);
}

const struct euicc_driver driver_apdu_qmi = {
    .type = DRIVER_APDU,
    .name = "qmi",
    .init = (int (*)(void *))libapduinterface_init,
    .main = libapduinterface_main,
    .fini = (void (*)(void *))libapduinterface_fini,
};
