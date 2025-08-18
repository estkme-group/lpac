#include "gbinder_hidl.h"

#include <euicc/euicc.h>
#include <euicc/hexutil.h>
#include <euicc/interface.h>
#include <lpac/utils.h>

#include <gbinder.h>
#include <stdio.h>
#include <stdlib.h>

#define ENV_DEBUG APDU_ENV_NAME(GBINDER, DEBUG)

#define HIDL_SERVICE_DEVICE "/dev/hwbinder"
#define HIDL_SERVICE_IFACE "android.hardware.radio@1.0::IRadio"
#define HIDL_SERVICE_IFACE_CALLBACK "android.hardware.radio@1.0::IRadioResponse"

// ref: IRadio
#define HIDL_SERVICE_SET_RESPONSE_FUNCTIONS GBINDER_FIRST_CALL_TRANSACTION
#define HIDL_SERVICE_ICC_OPEN_LOGICAL_CHANNEL (GBINDER_FIRST_CALL_TRANSACTION + 105)
#define HIDL_SERVICE_ICC_CLOSE_LOGICAL_CHANNEL (GBINDER_FIRST_CALL_TRANSACTION + 106)
#define HIDL_SERVICE_ICC_TRANSMIT_APDU_LOGICAL_CHANNEL (GBINDER_FIRST_CALL_TRANSACTION + 107)

// ref: IRadioResponse
#define HIDL_SERVICE_ICC_OPEN_LOGICAL_CHANNEL_CALLBACK (GBINDER_FIRST_CALL_TRANSACTION + 104)
#define HIDL_SERVICE_ICC_CLOSE_LOGICAL_CHANNEL_CALLBACK (GBINDER_FIRST_CALL_TRANSACTION + 105)
#define HIDL_SERVICE_ICC_TRANSMIT_APDU_LOGICAL_CHANNEL_CALLBACK (GBINDER_FIRST_CALL_TRANSACTION + 106)

struct radio_response_info {
    int32_t type;
    int32_t serial;
    int32_t error;
};

struct icc_io_result {
    int32_t sw1;
    int32_t sw2;
    GBinderHidlString simResponse;
};

struct sim_apdu {
    int32_t sessionId;
    int32_t cla;
    int32_t instruction;
    int32_t p1;
    int32_t p2;
    int32_t p3;
    GBinderHidlString data;
};

// clang-format off
static const GBinderWriterType sim_apdu_t = {
    .name = GBINDER_WRITER_STRUCT_NAME_AND_SIZE(struct sim_apdu),
    .fields = (GBinderWriterField[]){
        GBINDER_WRITER_FIELD_HIDL_STRING(struct sim_apdu, data),
        GBINDER_WRITER_FIELD_END()
    }
};
// clang-format on

struct gbinder_userdata {
    int lastChannelId;
    GBinderServiceManager *sm;

    // IRadioResponse
    GBinderLocalObject *response_callback;

    // IRadio
    GBinderRemoteObject *remote;
    GBinderClient *client;

    GMainLoop *binder_loop;

    int lastIntResp;
    int lastRadioErr;

    struct icc_io_result lastIccIoResult;
};

static GBinderLocalReply *gbinder_radio_response_transact(GBinderLocalObject *obj, GBinderRemoteRequest *req,
                                                          const guint code, guint flags, int *status, void *user_data) {
    struct gbinder_userdata *userdata = user_data;

    GBinderReader reader;

    gbinder_remote_request_init_reader(req, &reader);
    const struct radio_response_info *resp = gbinder_reader_read_hidl_struct(&reader, struct radio_response_info);
    userdata->lastRadioErr = resp->error;

    if (userdata->lastRadioErr != 0)
        goto out;

    switch (code) {
    case HIDL_SERVICE_ICC_OPEN_LOGICAL_CHANNEL_CALLBACK:
        gbinder_reader_read_int32(&reader, &userdata->lastIntResp);
        break;
    case HIDL_SERVICE_ICC_TRANSMIT_APDU_LOGICAL_CHANNEL_CALLBACK: {
        const struct icc_io_result *icc_io_res = gbinder_reader_read_hidl_struct(&reader, struct icc_io_result);
        // We cannot rely on the *req pointer being valid after we return
        userdata->lastIccIoResult.sw1 = icc_io_res->sw1;
        userdata->lastIccIoResult.sw2 = icc_io_res->sw2;
        userdata->lastIccIoResult.simResponse.data.str =
            strndup(icc_io_res->simResponse.data.str, icc_io_res->simResponse.len);
        userdata->lastIccIoResult.simResponse.len = icc_io_res->simResponse.len;
        userdata->lastIccIoResult.simResponse.owns_buffer = TRUE;
        break;
    }
    default:
        break;
    }

out:
    g_main_loop_quit(userdata->binder_loop);
    return NULL;
}

static void gbinder_cleanup_channel(const struct gbinder_userdata *userdata, const int id) {
    GBinderLocalRequest *req = gbinder_client_new_request(userdata->client);
    GBinderWriter writer;
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, 1000);
    gbinder_writer_append_int32(&writer, id);
    gbinder_client_transact_sync_oneway(userdata->client, HIDL_SERVICE_ICC_CLOSE_LOGICAL_CHANNEL, req);
    gbinder_local_request_unref(req);
    g_main_loop_run(userdata->binder_loop);
}

static void gbinder_cleanup(struct gbinder_userdata *userdata) {
    if (userdata->lastChannelId == -1)
        return;
    fprintf(stderr, "Cleaning up leaked APDU channel %d\n", userdata->lastChannelId);
    gbinder_cleanup_channel(userdata, userdata->lastChannelId);
    userdata->lastChannelId = -1;
}

static int gbinder_try_open_slot(struct gbinder_userdata *userdata, const int slotId, const uint8_t *aid,
                                 const uint32_t aid_len) {
    // First, try to connect to the HIDL service for this slot
    char fqname[255];
    snprintf(fqname, 255, "%s/slot%d", HIDL_SERVICE_IFACE, slotId);
    fprintf(stderr, "Attempting to connect to %s\n", fqname);

    int status = 0;
    userdata->sm = gbinder_servicemanager_new(HIDL_SERVICE_DEVICE);
    userdata->remote =
        gbinder_remote_object_ref(gbinder_servicemanager_get_service_sync(userdata->sm, fqname, &status));
    userdata->client = gbinder_client_new(userdata->remote, HIDL_SERVICE_IFACE);

    if (!userdata->client) {
        fprintf(stderr, "Failed to connect to IRadio\n");
        gbinder_client_unref(userdata->client);
        gbinder_remote_object_unref(userdata->remote);
        gbinder_servicemanager_unref(userdata->sm);
        return -1;
    }

    userdata->response_callback = gbinder_servicemanager_new_local_object(userdata->sm, HIDL_SERVICE_IFACE_CALLBACK,
                                                                          gbinder_radio_response_transact, userdata);

    GBinderLocalRequest *req = gbinder_client_new_request(userdata->client);
    GBinderWriter writer;
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_local_object(&writer, userdata->response_callback);
    gbinder_writer_append_local_object(&writer, NULL);
    gbinder_client_transact_sync_reply(userdata->client, HIDL_SERVICE_SET_RESPONSE_FUNCTIONS, req, &status);
    gbinder_local_request_unref(req);

    if (status < 0) {
        fprintf(stderr, "Failed to call IRadio::setResponseFunctions");
        return -1;
    }

    // Now, try to open the AID
    _cleanup_free_ char *aid_hex = calloc(aid_len * 2 + 1, 1);
    euicc_hexutil_bin2hex(aid_hex, aid_len * 2 + 1, aid, aid_len);

    req = gbinder_client_new_request(userdata->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, 1000);
    gbinder_writer_append_hidl_string_copy(&writer, aid_hex);
    gbinder_writer_append_int32(&writer, 0);
    status = gbinder_client_transact_sync_oneway(userdata->client, HIDL_SERVICE_ICC_OPEN_LOGICAL_CHANNEL, req);
    gbinder_local_request_unref(req);

    if (status < 0) {
        fprintf(stderr, "Failed to call IRadio::iccOpenLogicalChannel: %d\n", status);
        return status;
    }

    g_main_loop_run(userdata->binder_loop);

    if (userdata->lastRadioErr != 0) {
        fprintf(stderr, "Failed to open APDU logical channel: %d\n", userdata->lastRadioErr);
        return -userdata->lastRadioErr;
    }
    fprintf(stderr, "opened logical channel id: %d\n", userdata->lastIntResp);

    return userdata->lastIntResp;
}

static int apdu_interface_connect(struct euicc_ctx *ctx) { return 0; }

static void apdu_interface_disconnect(struct euicc_ctx *ctx) {
    struct gbinder_userdata *userdata = ctx->apdu.interface->userdata;

    gbinder_cleanup(userdata);
}

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, const uint8_t aid_len) {
    struct gbinder_userdata *userdata = ctx->apdu.interface->userdata;

    // We only start to use gbinder connection here, because only now can we detect whether
    // a given slot is a valid eSIM slot. This way we can automatically fall back in the case
    // where a device has only one eSIM -- we don't want to force the user to choose in this case.
    int res = gbinder_try_open_slot(userdata, 1, aid, aid_len);
    if (res < 0)
        res = gbinder_try_open_slot(userdata, 2, aid, aid_len);
    if (res >= 0)
        userdata->lastChannelId = res;
    return res;
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel) {
    struct gbinder_userdata *userdata = ctx->apdu.interface->userdata;

    gbinder_cleanup_channel(userdata, channel);
    if (userdata->lastChannelId == channel)
        userdata->lastChannelId = -1;

    // Only do this cleanup here, because on exit these objects will be destroyed anyway
    gbinder_client_unref(userdata->client);
    gbinder_remote_object_unref(userdata->remote);
    gbinder_servicemanager_unref(userdata->sm);
}

static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx,
                                   const uint32_t tx_len) {
    const struct gbinder_userdata *userdata = ctx->apdu.interface->userdata;

    GBinderLocalRequest *req = gbinder_client_new_request(userdata->client);
    GBinderWriter writer;
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, 1000);

    char *tx_hex = calloc(4096, 1);
    euicc_hexutil_bin2hex(tx_hex, 4096, &tx[5], tx_len - 5);

    if (getenv_or_default(ENV_DEBUG, (bool)false))
        fprintf(stderr, "APDU req: %s\n", tx_hex);

    const struct sim_apdu apdu = {
        .sessionId = userdata->lastChannelId,
        .cla = tx[0],
        .instruction = tx[1],
        .p1 = tx[2],
        .p2 = tx[3],
        .p3 = tx[4],
        .data =
            {
                .data = {.str = (const char *)tx_hex},
                .len = strlen(tx_hex) + 1,
                .owns_buffer = FALSE,
            },
    };
    gbinder_writer_append_struct(&writer, &apdu, &sim_apdu_t, NULL);
    const int status =
        gbinder_client_transact_sync_oneway(userdata->client, HIDL_SERVICE_ICC_TRANSMIT_APDU_LOGICAL_CHANNEL, req);
    gbinder_local_request_unref(req);

    if (status < 0) {
        fprintf(stderr, "Failed to call IRadio::iccTransmitApduLogicalChannel: %d\n", status);
        return status;
    }

    g_main_loop_run(userdata->binder_loop);

    if (userdata->lastRadioErr != 0) {
        return -userdata->lastRadioErr;
    }

    if (getenv_or_default(ENV_DEBUG, (bool)false))
        fprintf(stderr, "APDU resp: %d%d %d %s\n", userdata->lastIccIoResult.sw1, userdata->lastIccIoResult.sw2,
                userdata->lastIccIoResult.simResponse.len, userdata->lastIccIoResult.simResponse.data.str);

    *rx_len = userdata->lastIccIoResult.simResponse.len / 2 + 2;
    *rx = calloc(*rx_len, sizeof(uint8_t));
    euicc_hexutil_hex2bin_r(*rx, *rx_len, userdata->lastIccIoResult.simResponse.data.str,
                            userdata->lastIccIoResult.simResponse.len);
    (*rx)[*rx_len - 2] = userdata->lastIccIoResult.sw1;
    (*rx)[*rx_len - 1] = userdata->lastIccIoResult.sw2;

    // see radio_response_transact -- this is our buffer.
    free((void *)userdata->lastIccIoResult.simResponse.data.str);

    return 0;
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct) {
    set_deprecated_env_name(ENV_DEBUG, "GBINDER_APDU_DEBUG");

    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));

    struct gbinder_userdata *userdata = malloc(sizeof(struct gbinder_userdata));
    memset(userdata, 0, sizeof(struct gbinder_userdata));
    userdata->lastChannelId = -1;
    userdata->lastIntResp = -1;
    userdata->lastRadioErr = 0;
    // The glib loop is detached from any client object, so create it here.
    userdata->binder_loop = g_main_loop_new(NULL, FALSE);

    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;
    ifstruct->userdata = userdata;

    return 0;
}

static void libapduinterface_fini(struct euicc_apdu_interface *ifstruct) {
    struct gbinder_userdata *userdata = ifstruct->userdata;

    gbinder_cleanup(userdata);
}

const struct euicc_driver driver_apdu_gbinder_hidl = {
    .type = DRIVER_APDU,
    .name = "gbinder_hidl",
    .init = (int (*)(void *))libapduinterface_init,
    .main = NULL,
    .fini = (void (*)(void *))libapduinterface_fini,
};
