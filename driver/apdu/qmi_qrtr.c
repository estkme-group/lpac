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
#include "qmi_common.h"

int lastChannelId = -1;
int uimSlot = -1;
GMainContext *context = NULL;
static QrtrBus *bus = NULL;
QmiClientUim *uimClient = NULL;

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

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct)
{
    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));

    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = qmi_apdu_interface_disconnect;
    ifstruct->logic_channel_open = qmi_apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = qmi_apdu_interface_logic_channel_close;
    ifstruct->transmit = qmi_apdu_interface_transmit;

    // Install cleanup routine
    atexit(qmi_cleanup);
    signal(SIGINT, qmi_sighandler);

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

static void libapduinterface_fini(struct euicc_apdu_interface *ifstruct)
{
}

const struct euicc_driver driver_apdu_qmi_qrtr = {
    .type = DRIVER_APDU,
    .name = "qmi_qrtr",
    .init = (int (*)(void *))libapduinterface_init,
    .main = libapduinterface_main,
    .fini = (void (*)(void *))libapduinterface_fini,
};
