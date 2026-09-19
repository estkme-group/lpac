// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2024, Luca Weiss <luca.weiss@fairphone.com>
 */
#pragma once

#include "qmi_helpers.h"

#include <euicc/interface.h>

struct qmi_data {
    int lastChannelId;
    int uimSlot;
    GMainContext *context;
    QmiClientUim *uimClient;
#ifdef LPAC_WITH_DRIVER_APDU_QMI_QRTR
    QrtrBus *qrtrBus;
#endif

    /*
     * SIM power-cycle after a profile has been enabled/disabled
     * (see ENV_SIM_REFRESH / ENV_SIM_REFRESH_DELAY_MS in qmi_helpers.h).
     * Whether a toggle actually happened is tracked via ctx->profile_toggled,
     * set by the profile enable/disable applets.
     */
    gboolean sim_refresh_enabled; /* feature toggled on via environment variable */
    guint sim_refresh_delay_ms;  /* delay between SIM Power Off and SIM Power On */
};

int qmi_apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx,
                                uint32_t tx_len);
int qmi_apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len);
void qmi_apdu_interface_logic_channel_close(struct qmi_data *qmi_priv, uint8_t channel);
void qmi_apdu_interface_disconnect(struct euicc_ctx *ctx);
void qmi_cleanup(struct qmi_data *qmi_priv);

/*
 * If ctx->profile_toggled is set (a profile enable/disable applet just
 * completed successfully) and the feature is enabled, performs SIM Power
 * Off, waits sim_refresh_delay_ms and performs SIM Power On. Called from
 * qmi_apdu_interface_disconnect().
 */
void qmi_apdu_maybe_power_cycle_sim(struct euicc_ctx *ctx);
