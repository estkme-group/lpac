// vim: expandtab sw=4 ts=4:
#include <euicc/euicc.h>
#include <euicc/hexutil.h>
#include <euicc/interface.h>
#include <gbinder.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG (getenv("GBINDER_APDU_DEBUG") != NULL && strcmp("true", getenv("GBINDER_APDU_DEBUG")) == 0)

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

static int lastChannelId = -1;

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

static const GBinderWriterField sim_apdu_f[] = {
    GBINDER_WRITER_FIELD_HIDL_STRING(struct sim_apdu, data),
    GBINDER_WRITER_FIELD_END()
};

static const GBinderWriterType sim_apdu_t = {
    GBINDER_WRITER_STRUCT_NAME_AND_SIZE(struct sim_apdu), sim_apdu_f
};

static GBinderServiceManager *sm;
// IRadioResponse
static GBinderLocalObject *response_callback;
// IRadio
static GBinderRemoteObject *remote;
static GBinderClient *client;

static GMainLoop *binder_loop;

static int lastIntResp = -1;
static int lastRadioErr = 0;
static struct icc_io_result lastIccIoResult = {0};

static GBinderLocalReply *radio_response_transact(
        GBinderLocalObject *obj,
        GBinderRemoteRequest *req,
        guint code, guint flags, int *status, void *user_data)
{
    GBinderReader reader;

    gbinder_remote_request_init_reader(req, &reader);
    const struct radio_response_info *resp =
        gbinder_reader_read_hidl_struct(&reader, struct radio_response_info);
    lastRadioErr = resp->error;

    if (lastRadioErr != 0)
        goto out;

    switch (code) {
        case HIDL_SERVICE_ICC_OPEN_LOGICAL_CHANNEL_CALLBACK:
            gbinder_reader_read_int32(&reader, &lastIntResp);
            break;
        case HIDL_SERVICE_ICC_TRANSMIT_APDU_LOGICAL_CHANNEL_CALLBACK:
            const struct icc_io_result *icc_io_res = gbinder_reader_read_hidl_struct(&reader, struct icc_io_result);
            // We cannot rely on the *req pointer being valid after we return
            lastIccIoResult.sw1 = icc_io_res->sw1;
            lastIccIoResult.sw2 = icc_io_res->sw2;
            lastIccIoResult.simResponse.data.str = strndup(icc_io_res->simResponse.data.str, icc_io_res->simResponse.len);
            lastIccIoResult.simResponse.len = icc_io_res->simResponse.len;
            lastIccIoResult.simResponse.owns_buffer = TRUE;
            break;
    }

out:
    g_main_loop_quit(binder_loop);
    return NULL;
}

static void cleanup_channel(int id)
{
    GBinderLocalRequest *req = gbinder_client_new_request(client);
    GBinderWriter writer;
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, 1000);
    gbinder_writer_append_int32(&writer, id);
    gbinder_client_transact_sync_oneway(client, HIDL_SERVICE_ICC_CLOSE_LOGICAL_CHANNEL, req);
    gbinder_local_request_unref(req);
    g_main_loop_run(binder_loop);
}

static void cleanup(void)
{
    if (lastChannelId != -1) {
        fprintf(stderr, "Cleaning up leaked APDU channel %d\n", lastChannelId);
        cleanup_channel(lastChannelId);
        lastChannelId = -1;
    }
}

static void sighandler(int sig)
{
    // This would trigger atexit() hooks
    exit(0);
}

static int try_open_slot(int slotId, const uint8_t *aid, uint32_t aid_len)
{
    // First, try to connect to the HIDL service for this slot
    char fqname[255];
    snprintf(fqname, 255, "%s/slot%d", HIDL_SERVICE_IFACE, slotId);
    fprintf(stderr, "Attempting to connect to %s\n", fqname);

    int status = 0;
    sm = gbinder_servicemanager_new(HIDL_SERVICE_DEVICE);
    remote = gbinder_remote_object_ref(
            gbinder_servicemanager_get_service_sync(sm, fqname, &status));
    client = gbinder_client_new(remote, HIDL_SERVICE_IFACE);

    if (!client) {
        fprintf(stderr, "Failed to connect to IRadio\n");
        gbinder_client_unref(client);
        gbinder_remote_object_unref(remote);
        gbinder_servicemanager_unref(sm);
        return -1;
    }

    response_callback = gbinder_servicemanager_new_local_object(
            sm, HIDL_SERVICE_IFACE_CALLBACK, radio_response_transact, NULL);

    GBinderLocalRequest *req = gbinder_client_new_request(client);
    GBinderWriter writer;
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_local_object(&writer, response_callback);
    gbinder_writer_append_local_object(&writer, NULL);
    gbinder_client_transact_sync_reply(client, HIDL_SERVICE_SET_RESPONSE_FUNCTIONS, req, &status);
    gbinder_local_request_unref(req);

    if (status < 0) {
        fprintf(stderr, "Failed to call IRadio::setResponseFunctions");
        return -1;
    }

    // Now, try to open the AID
    uint8_t aid_hex[255];
    euicc_hexutil_bin2hex(aid_hex, 255, aid, aid_len);

    req = gbinder_client_new_request(client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, 1000);
    gbinder_writer_append_hidl_string_copy(&writer, aid_hex);
    gbinder_writer_append_int32(&writer, 0);
    status = gbinder_client_transact_sync_oneway(client, HIDL_SERVICE_ICC_OPEN_LOGICAL_CHANNEL, req);
    gbinder_local_request_unref(req);

    if (status < 0) {
        fprintf(stderr, "Failed to call IRadio::iccOpenLogicalChannel: %d\n", status);
        return status;
    }

    g_main_loop_run(binder_loop);

    if (lastRadioErr != 0) {
        fprintf(stderr, "Failed to open APDU logical channel: %d\n", lastRadioErr);
        return -lastRadioErr;
    }
    fprintf(stderr, "opened logical channel id: %d\n", lastIntResp);

    return lastIntResp;
}

static int apdu_interface_connect(struct euicc_ctx *ctx)
{
    return 0;
}

static void apdu_interface_disconnect(struct euicc_ctx *ctx)
{
    cleanup();
}

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len)
{
    // We only start to use gbinder connection here, because only now can we detect whether
    // a given slot is a valid eSIM slot. This way we can automatically fall back in the case
    // where a device has only one eSIM -- we don't want to force the user to choose in this case.
    int res = try_open_slot(1, aid, aid_len);
    if (res < 0)
        res = try_open_slot(2, aid, aid_len);
    if (res >= 0)
        lastChannelId = res;
    return res;
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel)
{
    cleanup_channel(channel);
    if (lastChannelId == channel)
        lastChannelId = -1;

    // Only do this cleanup here, because on exit these objects will be destroyed anyway
    gbinder_client_unref(client);
    gbinder_remote_object_unref(remote);
    gbinder_servicemanager_unref(sm);
}

static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len)
{
    GBinderLocalRequest *req = gbinder_client_new_request(client);
    GBinderWriter writer;
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, 1000);

    uint8_t tx_hex[4096] = {0};
    euicc_hexutil_bin2hex(tx_hex, 4096, &tx[5], tx_len - 5);

    if (DEBUG)
        fprintf(stderr, "APDU req: %s\n", tx_hex);

    struct sim_apdu apdu = {
        .sessionId = lastChannelId,
        .cla = tx[0],
        .instruction = tx[1],
        .p1 = tx[2],
        .p2 = tx[3],
        .p3 = tx[4],
        .data = {
            .data = {
                .str = (const char *) tx_hex
            },
            .len = strlen(tx_hex) + 1,
            .owns_buffer = FALSE,
        },
    };
    gbinder_writer_append_struct(&writer, &apdu, &sim_apdu_t, NULL);
    int status = gbinder_client_transact_sync_oneway(client, HIDL_SERVICE_ICC_TRANSMIT_APDU_LOGICAL_CHANNEL, req);
    gbinder_local_request_unref(req);

    if (status < 0) {
        fprintf(stderr, "Failed to call IRadio::iccTransmitApduLogicalChannel: %d\n", status);
        return status;
    }

    g_main_loop_run(binder_loop);

    if (lastRadioErr != 0) {
        return -lastRadioErr;
    }

    if (DEBUG)
        fprintf(stderr, "APDU resp: %d%d %d %s\n", lastIccIoResult.sw1, lastIccIoResult.sw2, lastIccIoResult.simResponse.len, lastIccIoResult.simResponse.data.str);

    *rx_len = lastIccIoResult.simResponse.len / 2 + 2;
    *rx = calloc(*rx_len, sizeof(uint8_t));
    euicc_hexutil_hex2bin_r(*rx, *rx_len, lastIccIoResult.simResponse.data.str, lastIccIoResult.simResponse.len);
    (*rx)[*rx_len - 2] = lastIccIoResult.sw1;
    (*rx)[*rx_len - 1] = lastIccIoResult.sw2;

    // see radio_response_transact -- this is our buffer.
    free((void *) lastIccIoResult.simResponse.data.str);

    return 0;
}

EUICC_SHARED_EXPORT int libapduinterface_init(struct euicc_apdu_interface *ifstruct)
{
    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;

    // Install cleanup routine
    atexit(cleanup);
    signal(SIGINT, sighandler);

    // The glib loop is detached from any client object, so create it here.
    binder_loop = g_main_loop_new(NULL, FALSE);

    return 0;
}

EUICC_SHARED_EXPORT int libapduinterface_main(int argc, char **argv)
{
    return 0;
}
