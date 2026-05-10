#include <driver.h>
#include <euicc/euicc.h>
#include <lpac/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

#define ARGOS_VID 0x0B0C
#define ARGOS_PID 0x0009

// All USB transfers are exactly 8 bytes. Frame: [TYPE, PARAM, d0..d5].
// TYPE 0x01 = data (PARAM = payload count 0..6), TYPE 0x02 = command/status.
#define FRAME_SIZE  8
#define MAX_PAYLOAD 6

#define CMD_POWER_ON 0x01
#define CMD_ABORT    0x0B
#define CMD_GET_FW   0x0E
#define CMD_T0_XMIT  0x13

#define EUICC_INTERFACE_BUFSZ 264

#define APDU_TERMINAL_CAPABILITIES "\x80\xAA\x00\x00\x0A\xA9\x08\x81\x00\x82\x01\x01\x83\x01\x07"
#define APDU_OPENLOGICCHANNEL "\x00\x70\x00\x00\x01"
#define APDU_CLOSELOGICCHANNEL "\x00\x70\x80\xFF\x00"
#define APDU_SELECT_HEADER "\x00\xA4\x04\x00\xFF"

struct argos_userdata {
    libusb_context *usb_ctx;
    libusb_device_handle *dev;
    int interface_num;
    int kernel_detached;
    uint8_t ep_in;
    uint8_t ep_out;
    uint8_t di;
};

static int argos_send(struct argos_userdata *ud, const uint8_t buf[FRAME_SIZE], int timeout) {
    int xfer = 0;
    int rc = libusb_bulk_transfer(ud->dev, ud->ep_out, (uint8_t *)buf, FRAME_SIZE, &xfer, timeout);
    return (rc < 0) ? -1 : 0;
}

static int argos_recv(struct argos_userdata *ud, uint8_t buf[FRAME_SIZE], int timeout) {
    int xfer = 0;
    int rc = libusb_bulk_transfer(ud->dev, ud->ep_in, buf, FRAME_SIZE, &xfer, timeout);
    if (rc == LIBUSB_ERROR_TIMEOUT)
        return -2;
    return (rc < 0 || xfer < 2) ? -1 : 0;
}

static int argos_cmd(struct argos_userdata *ud, uint8_t cmd, uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3) {
    uint8_t f[FRAME_SIZE] = {0x02, cmd, p0, p1, p2, p3, 0, 0};
    return argos_send(ud, f, 1000);
}

static int argos_send_data(struct argos_userdata *ud, const uint8_t *data, int len) {
    int sent = 0;
    do {
        uint8_t f[FRAME_SIZE] = {0};
        int n = len - sent;
        if (n > MAX_PAYLOAD)
            n = MAX_PAYLOAD;
        if (n < 0)
            n = 0;
        f[0] = 0x01;
        f[1] = (uint8_t)n;
        if (n > 0)
            memcpy(&f[2], data + sent, n);
        if (argos_send(ud, f, 1000) < 0)
            return -1;
        sent += n;
    } while (sent < len);
    return 0;
}

static int argos_recv_frame(struct argos_userdata *ud, uint8_t f[FRAME_SIZE], int timeout) {
    if (argos_recv(ud, f, timeout) < 0)
        return -1;
    return (f[0] == 0x01 || f[0] == 0x02) ? f[0] : -1;
}

static int argos_power_on(struct argos_userdata *ud) {
    uint8_t f[FRAME_SIZE];
    uint8_t atr[33];
    int atr_len = 0, type;

    if (argos_cmd(ud, CMD_POWER_ON, 0, 0, 0, 0) < 0)
        return -1;

    // A card-present status frame [0x02,0x81,0x01] may precede or interleave
    // with ATR data frames and does not end the sequence.
    for (int i = 0; i < 50; i++) {
        type = argos_recv_frame(ud, f, 5000);
        if (type < 0)
            return -1;
        if (type == 0x01) {
            int n = f[1] > MAX_PAYLOAD ? MAX_PAYLOAD : f[1];
            if (n > 0 && atr_len + n <= (int)sizeof(atr)) {
                memcpy(&atr[atr_len], &f[2], n);
                atr_len += n;
            }
        } else if (f[1] == 0x81 && f[2] == 0x00) {
            return -1;
        }
        usleep(100000);
        if (type == 0x01 && f[1] < MAX_PAYLOAD && atr_len > 0)
            break;
    }

    if (atr_len == 0)
        return -1;

    ud->di = 1;
    if (argos_cmd(ud, CMD_GET_FW, 0, 0, 0, 0) == 0) {
        type = argos_recv_frame(ud, f, 1000);
        if (type == 0x01 && f[1] >= 3 && f[2] == 0x83)
            ud->di = f[4];
    }

    return 0;
}

static int argos_transmit_lowlevel(struct argos_userdata *ud, uint8_t *rx, uint32_t *rx_len, const uint8_t *tx,
                                   uint32_t tx_len) {
    uint8_t f[FRAME_SIZE];
    uint32_t pos = 0, max = *rx_len;
    int type, n;
    *rx_len = 0;

    if (tx_len < 4)
        return -1;

    // Case 2 APDU (5 bytes): Le = tx[4], with Le=0 meaning 256 per ISO 7816.
    // Case 3 APDU (>5 bytes): Lc = tx[4], no Le.
    int le = 0;
    if (tx_len == 5) {
        le = tx[4] ? tx[4] : 256;
    }

    uint8_t le_lo = 0, le_hi = 0;
    if (le > 0 && le < 256) {
        le_lo = (uint8_t)le;
    } else if (le >= 256) {
        le_lo = 0xFF;
        le_hi = (uint8_t)((le & 0xFF) + 1);
    }

    if (argos_send_data(ud, tx, tx_len) < 0)
        return -1;
    if (argos_cmd(ud, CMD_T0_XMIT, (uint8_t)(tx_len & 0xFF), ud->di, le_lo, le_hi) < 0)
        return -1;

    uint32_t expected = le > 0 ? (uint32_t)le + 2 : 2;

    type = argos_recv_frame(ud, f, 80000);
    if (type != 0x01)
        return -1;

    n = f[1] > MAX_PAYLOAD ? MAX_PAYLOAD : f[1];
    if (n == 1)
        return -1;

    if (n > 0 && pos + n <= max) {
        memcpy(&rx[pos], &f[2], n);
        pos += n;
    }

    // First frame with n==2 carries only SW1+SW2; otherwise drain until expected length.
    if (n != 2) {
        while (pos < expected) {
            type = argos_recv_frame(ud, f, 1000);
            if (type != 0x01)
                break;
            n = f[1] > MAX_PAYLOAD ? MAX_PAYLOAD : f[1];
            if (n > 0 && pos + n <= max) {
                memcpy(&rx[pos], &f[2], n);
                pos += n;
            }
        }
    }

    if (pos < 2)
        return -1;

    *rx_len = pos;
    return 0;
}

static int argos_find_endpoints(struct argos_userdata *ud) {
    struct libusb_config_descriptor *config = NULL;
    if (libusb_get_active_config_descriptor(libusb_get_device(ud->dev), &config) < 0)
        return -1;

    ud->ep_in = 0;
    ud->ep_out = 0;

    for (int i = 0; i < config->bNumInterfaces; i++) {
        const struct libusb_interface *iface = &config->interface[i];
        for (int j = 0; j < iface->num_altsetting; j++) {
            const struct libusb_interface_descriptor *alt = &iface->altsetting[j];
            for (int k = 0; k < alt->bNumEndpoints; k++) {
                const struct libusb_endpoint_descriptor *ep = &alt->endpoint[k];
                if ((ep->bmAttributes & 0x03) != LIBUSB_TRANSFER_TYPE_BULK)
                    continue;
                if (ep->bEndpointAddress & LIBUSB_ENDPOINT_IN)
                    ud->ep_in = ep->bEndpointAddress;
                else
                    ud->ep_out = ep->bEndpointAddress;
            }
            if (ud->ep_in && ud->ep_out) {
                ud->interface_num = alt->bInterfaceNumber;
                libusb_free_config_descriptor(config);
                return 0;
            }
        }
    }

    libusb_free_config_descriptor(config);
    return -1;
}

static void argos_logic_channel_close(struct argos_userdata *ud, uint8_t channel) {
    uint8_t tx[sizeof(APDU_CLOSELOGICCHANNEL) - 1];
    uint8_t rx[EUICC_INTERFACE_BUFSZ];
    uint32_t rx_len = sizeof(rx);

    memcpy(tx, APDU_CLOSELOGICCHANNEL, sizeof(tx));
    tx[3] = channel;
    argos_transmit_lowlevel(ud, rx, &rx_len, tx, sizeof(tx));
}

static int argos_logic_channel_open(struct argos_userdata *ud, const uint8_t *aid, uint8_t aid_len) {
    int channel = 0;
    uint8_t tx[EUICC_INTERFACE_BUFSZ];
    uint8_t *tx_wptr;
    uint8_t rx[EUICC_INTERFACE_BUFSZ];
    uint32_t rx_len;

    if (aid_len > 32)
        goto err;

    rx_len = sizeof(rx);
    if (argos_transmit_lowlevel(ud, rx, &rx_len, (const uint8_t *)APDU_OPENLOGICCHANNEL,
                                sizeof(APDU_OPENLOGICCHANNEL) - 1)
        < 0)
        goto err;

    if (rx_len != 3 || (rx[1] & 0xF0) != 0x90)
        goto err;

    channel = rx[0];

    tx_wptr = tx;
    memcpy(tx_wptr, APDU_SELECT_HEADER, sizeof(APDU_SELECT_HEADER) - 1);
    tx_wptr += sizeof(APDU_SELECT_HEADER) - 1;
    memcpy(tx_wptr, aid, aid_len);
    tx_wptr += aid_len;
    tx[0] = (tx[0] & 0xF0) | channel;
    tx[4] = aid_len;

    rx_len = sizeof(rx);
    if (argos_transmit_lowlevel(ud, rx, &rx_len, tx, tx_wptr - tx) < 0)
        goto err;

    if (rx_len < 2)
        goto err;

    switch (rx[rx_len - 2]) {
    case 0x90:
    case 0x61:
        return channel;
    default:
        goto err;
    }

err:
    if (channel)
        argos_logic_channel_close(ud, channel);
    return -1;
}

static int apdu_interface_connect(struct euicc_ctx *ctx) {
    struct argos_userdata *ud = ctx->apdu.interface->userdata;
    uint8_t rx[EUICC_INTERFACE_BUFSZ];
    uint32_t rx_len;
    int rc;

    ud->dev = libusb_open_device_with_vid_pid(ud->usb_ctx, ARGOS_VID, ARGOS_PID);
    if (!ud->dev) {
        fprintf(stderr, "argos: device %04x:%04x not found\n", ARGOS_VID, ARGOS_PID);
        return -1;
    }

    if (argos_find_endpoints(ud) < 0)
        goto err_close;

    if (libusb_kernel_driver_active(ud->dev, ud->interface_num) == 1) {
        if (libusb_detach_kernel_driver(ud->dev, ud->interface_num) < 0)
            goto err_close;
        ud->kernel_detached = 1;
    }

    rc = libusb_claim_interface(ud->dev, ud->interface_num);
    if (rc < 0)
        goto err_reattach;

    if (argos_power_on(ud) < 0) {
        fprintf(stderr, "argos: card power on failed (is a card inserted?)\n");
        goto err_release;
    }

    rx_len = sizeof(rx);
    argos_transmit_lowlevel(ud, rx, &rx_len, (const uint8_t *)APDU_TERMINAL_CAPABILITIES,
                            sizeof(APDU_TERMINAL_CAPABILITIES) - 1);
    return 0;

err_release:
    libusb_release_interface(ud->dev, ud->interface_num);
err_reattach:
    if (ud->kernel_detached)
        libusb_attach_kernel_driver(ud->dev, ud->interface_num);
    ud->kernel_detached = 0;
err_close:
    libusb_close(ud->dev);
    ud->dev = NULL;
    return -1;
}

static void apdu_interface_disconnect(struct euicc_ctx *ctx) {
    struct argos_userdata *ud = ctx->apdu.interface->userdata;
    if (!ud->dev)
        return;
    argos_cmd(ud, CMD_ABORT, 0, 0, 0, 0);
    libusb_release_interface(ud->dev, ud->interface_num);
    if (ud->kernel_detached) {
        libusb_attach_kernel_driver(ud->dev, ud->interface_num);
        ud->kernel_detached = 0;
    }
    libusb_close(ud->dev);
    ud->dev = NULL;
}

static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx,
                                   uint32_t tx_len) {
    struct argos_userdata *ud = ctx->apdu.interface->userdata;
    *rx = malloc(EUICC_INTERFACE_BUFSZ);
    if (!*rx)
        return -1;
    *rx_len = EUICC_INTERFACE_BUFSZ;
    if (argos_transmit_lowlevel(ud, *rx, rx_len, tx, tx_len) < 0) {
        free(*rx);
        *rx_len = 0;
        return -1;
    }
    return 0;
}

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len) {
    return argos_logic_channel_open(ctx->apdu.interface->userdata, aid, aid_len);
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel) {
    argos_logic_channel_close(ctx->apdu.interface->userdata, channel);
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct) {
    struct argos_userdata *ud = malloc(sizeof(struct argos_userdata));
    if (ud == NULL)
        return -1;
    memset(ud, 0, sizeof(struct argos_userdata));
    ud->di = 1;

    if (libusb_init(&ud->usb_ctx) < 0) {
        free(ud);
        return -1;
    }

    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));
    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;
    ifstruct->userdata = ud;
    return 0;
}

static void libapduinterface_fini(const struct euicc_apdu_interface *ifstruct) {
    struct argos_userdata *ud = ifstruct->userdata;
    if (!ud)
        return;
    if (ud->usb_ctx)
        libusb_exit(ud->usb_ctx);
    free(ud);
}

DRIVER_INTERFACE = {
    .type = DRIVER_APDU,
    .name = "argos",
    .init = (int (*)(void *))libapduinterface_init,
    .main = NULL,
    .fini = (void (*)(void *))libapduinterface_fini,
};
