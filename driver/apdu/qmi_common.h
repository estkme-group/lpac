// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2024, Luca Weiss <luca.weiss@fairphone.com>
 */

#include <euicc/interface.h>
#include <euicc/euicc.h>
#include "qmi_helpers.h"

struct qmi_data {
	int lastChannelId;
	int uimSlot;
	GMainContext *context;
	QmiClientUim *uimClient;
};

int qmi_apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len);
int qmi_apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len);
void qmi_apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel);
void qmi_apdu_interface_disconnect(struct euicc_ctx *ctx);
void qmi_cleanup(struct qmi_data *qmi_priv);
