#include <cjson-ext/cJSON_ex.h>
#include <driver.h>
#include <euicc/interface.h>
#include <lpac/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#    include "pcsc_win32.h"
#    include <winscard.h>
#else
#    include <PCSC/winscard.h>
#    include <PCSC/wintypes.h>
#endif

#ifdef interface
#    undef interface
#endif

#define ENV_DRV_IFID APDU_ENV_NAME(PCSC, DRV_IFID)
#define ENV_DRV_NAME APDU_ENV_NAME(PCSC, DRV_NAME)
#define ENV_DRV_IGNORE_NAME APDU_ENV_NAME(PCSC, DRV_IGNORE_NAME)

#define EUICC_INTERFACE_BUFSZ 264

// #define APDU_ST33_MAGIC "\x90\xBD\x36\xBB\x00"
#define APDU_TERMINAL_CAPABILITIES "\x80\xAA\x00\x00\x0A\xA9\x08\x81\x00\x82\x01\x01\x83\x01\x07"
#define APDU_OPENLOGICCHANNEL "\x00\x70\x00\x00\x01"
#define APDU_CLOSELOGICCHANNEL "\x00\x70\x80\xFF\x00"
#define APDU_SELECT_HEADER "\x00\xA4\x04\x00\xFF"

struct pcsc_userdata {
    SCARDCONTEXT ctx;
    SCARDHANDLE hCard;
    LPSTR mszReaders;
};

static void pcsc_error(const char *method, const int32_t code) {
    fprintf(stderr, "%s failed: %08X (%s)\n", method, code, pcsc_stringify_error(code));
}

static bool is_ignored_reader_name(const char *reader) {
    char *value = getenv(ENV_DRV_IGNORE_NAME);
    if (value == NULL)
        return false;
    const char *token = NULL;
    for (token = strtok(value, ";"); token != NULL; token = strtok(NULL, ";")) {
        if (strstr(reader, token) == NULL)
            continue;
        return true; // reader name is in ignore list, skip
    }
    return false;
}

static int pcsc_ctx_open(struct pcsc_userdata *userdata) {
    DWORD dwReaders;

    userdata->ctx = 0;
    userdata->hCard = 0;
    userdata->mszReaders = NULL;

    int ret = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &userdata->ctx);
    if (ret != SCARD_S_SUCCESS) {
        pcsc_error("SCardEstablishContext()", ret);
        return -1;
    }

#ifdef SCARD_AUTOALLOCATE
    dwReaders = SCARD_AUTOALLOCATE;
    ret = SCardListReaders(userdata->ctx, NULL, (LPSTR)&userdata->mszReaders, &dwReaders);
#else
    // macOS does not support SCARD_AUTOALLOCATE, so we need to call SCardListReaders twice.
    // First call to get the size of the buffer, second call to get the actual data.
    ret = SCardListReaders(userdata->ctx, NULL, NULL, &dwReaders);
    if (ret != SCARD_S_SUCCESS) {
        pcsc_error("SCardListReaders()", ret);
        return -1;
    }
    userdata->mszReaders = malloc(sizeof(char) * dwReaders);
    if (userdata->mszReaders == NULL) {
        fprintf(stderr, "malloc: not enough memory\n");
        return -1;
    }
    ret = SCardListReaders(userdata->ctx, NULL, userdata->mszReaders, &dwReaders);
#endif
    if (ret != SCARD_S_SUCCESS) {
        pcsc_error("SCardListReaders()", ret);
        return -1;
    }

    return 0;
}

static int pcsc_iter_reader(struct pcsc_userdata *userdata,
                            int (*callback)(struct pcsc_userdata *userdata, int index, const char *reader,
                                            void *context),
                            void *context) {
    int index = 0;
    LPSTR pReader = userdata->mszReaders;
    while (*pReader != '\0') {
        const int ret = callback(userdata, index, pReader, context);
        if (ret < 0)
            return -1;
        if (ret > 0)
            return 0;
        pReader += strlen(pReader) + 1;
        index++;
    }
    return -1;
}

static int pcsc_open_hCard_iter(struct pcsc_userdata *userdata, const int index, const char *reader, void *context) {
    DWORD dwActiveProtocol;

    const int id = getenv_or_default(ENV_DRV_IFID, (int)-1);
    if (id != -1 && id != index) {
        const char *part_name = getenv(ENV_DRV_NAME);
        if (strstr(reader, part_name) == NULL) {
            return 0;
        }
    }

    if (is_ignored_reader_name(reader)) {
        return 0; // skip ignored reader names
    }

    const int ret = SCardConnect(userdata->ctx, reader, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_T0, &userdata->hCard,
                                 &dwActiveProtocol);
    if (ret != SCARD_S_SUCCESS) {
        pcsc_error("SCardConnect()", ret);
        // see <https://blog.apdu.fr/posts/2024/12/gnupg-and-pcsc-conflicts-episode-3/>
        if (ret == SCARD_E_SHARING_VIOLATION)
            return 0; // skip
        return -1;
    }

    return 1;
}

static int pcsc_open_hCard(struct pcsc_userdata *userdata) {
    return pcsc_iter_reader(userdata, pcsc_open_hCard_iter, NULL);
}

static void pcsc_close(const struct pcsc_userdata *userdata) {
    if (userdata->mszReaders != NULL) {
// macOS does not support SCARD_AUTOALLOCATE, so we need to free the buffer manually.
#ifdef SCARD_AUTOALLOCATE
        SCardFreeMemory(userdata->ctx, userdata->mszReaders);
#else
        // on macOS, mszReaders is allocated by malloc()
        free(userdata->mszReaders);
#endif
    }

    if (userdata->hCard) {
        SCardDisconnect(userdata->hCard, SCARD_UNPOWER_CARD);
    }
    if (userdata->ctx) {
        SCardReleaseContext(userdata->ctx);
    }
}

static int pcsc_transmit_lowlevel(const struct pcsc_userdata *userdata, uint8_t *rx, uint32_t *rx_len,
                                  const uint8_t *tx, const uint8_t tx_len) {
    int ret;
    DWORD rx_len_merged;

    rx_len_merged = *rx_len;
    ret = SCardTransmit(userdata->hCard, SCARD_PCI_T0, tx, tx_len, NULL, rx, &rx_len_merged);
    if (ret != SCARD_S_SUCCESS) {
        pcsc_error("SCardTransmit()", ret);
        return -1;
    }

    *rx_len = rx_len_merged;
    return 0;
}

static void pcsc_logic_channel_close(const struct pcsc_userdata *userdata, const uint8_t channel) {
    uint8_t tx[sizeof(APDU_CLOSELOGICCHANNEL) - 1];
    uint8_t rx[EUICC_INTERFACE_BUFSZ];
    uint32_t rx_len;

    memcpy(tx, APDU_CLOSELOGICCHANNEL, sizeof(tx));
    tx[3] = channel;

    rx_len = sizeof(rx);

    pcsc_transmit_lowlevel(userdata, rx, &rx_len, tx, sizeof(tx));
}

static int pcsc_logic_channel_open(const struct pcsc_userdata *userdata, const uint8_t *aid, uint8_t aid_len) {
    int channel = 0;
    uint8_t tx[EUICC_INTERFACE_BUFSZ];
    uint8_t *tx_wptr;
    uint8_t rx[EUICC_INTERFACE_BUFSZ];
    uint32_t rx_len;

    if (aid_len > 32) {
        goto err;
    }

    rx_len = sizeof(rx);
    if (pcsc_transmit_lowlevel(userdata, rx, &rx_len, (const uint8_t *)APDU_OPENLOGICCHANNEL,
                               sizeof(APDU_OPENLOGICCHANNEL) - 1)
        < 0) {
        goto err;
    }

    if (rx_len != 3) {
        goto err;
    }

    if ((rx[1] & 0xF0) != 0x90) {
        goto err;
    }

    channel = rx[0];

    tx_wptr = tx;
    memcpy(tx_wptr, APDU_SELECT_HEADER, sizeof(APDU_SELECT_HEADER) - 1);
    tx_wptr += sizeof(APDU_SELECT_HEADER) - 1;
    memcpy(tx_wptr, aid, aid_len);
    tx_wptr += aid_len;

    tx[0] = (tx[0] & 0xF0) | channel;
    tx[4] = aid_len;

    rx_len = sizeof(rx);
    if (pcsc_transmit_lowlevel(userdata, rx, &rx_len, tx, tx_wptr - tx) < 0) {
        goto err;
    }

    if (rx_len < 2) {
        goto err;
    }

    switch (rx[rx_len - 2]) {
    case 0x90:
    case 0x61:
        return channel;
    default:
        goto err;
    }

err:
    if (channel) {
        pcsc_logic_channel_close(userdata, channel);
    }

    return -1;
}

static int apdu_interface_connect(struct euicc_ctx *ctx) {
    struct pcsc_userdata *userdata = ctx->apdu.interface->userdata;

    uint8_t rx[EUICC_INTERFACE_BUFSZ];
    uint32_t rx_len;

    if (pcsc_open_hCard(userdata) < 0) {
        return -1;
    }

    rx_len = sizeof(rx);
    pcsc_transmit_lowlevel(userdata, rx, &rx_len, (const uint8_t *)APDU_TERMINAL_CAPABILITIES,
                           sizeof(APDU_TERMINAL_CAPABILITIES) - 1);

    return 0;
}

static void apdu_interface_disconnect(struct euicc_ctx *ctx) {
    const struct pcsc_userdata *userdata = ctx->apdu.interface->userdata;
    pcsc_close(userdata);
}

static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx,
                                   uint32_t tx_len) {
    const struct pcsc_userdata *userdata = ctx->apdu.interface->userdata;
    *rx = malloc(EUICC_INTERFACE_BUFSZ);
    if (!*rx) {
        fprintf(stderr, "SCardTransmit() RX buffer alloc failed\n");
        return -1;
    }
    *rx_len = EUICC_INTERFACE_BUFSZ;

    if (pcsc_transmit_lowlevel(userdata, *rx, rx_len, tx, tx_len) < 0) {
        free(*rx);
        *rx_len = 0;
        return -1;
    }

    return 0;
}

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len) {
    const struct pcsc_userdata *userdata = ctx->apdu.interface->userdata;
    return pcsc_logic_channel_open(userdata, aid, aid_len);
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel) {
    const struct pcsc_userdata *userdata = ctx->apdu.interface->userdata;
    pcsc_logic_channel_close(userdata, channel);
}

static int pcsc_list_iter(struct pcsc_userdata *userdata, const int index, const char *reader, void *context) {
    cJSON *json = context;
    char index_str[16];

    snprintf(index_str, sizeof(index_str), "%d", index);

    cJSON *jreader = cJSON_CreateObject();
    if (!jreader) {
        return -1;
    }

    if (!cJSON_AddStringOrNullToObject(jreader, "env", index_str)) {
        return -1;
    }

    if (!cJSON_AddStringOrNullToObject(jreader, "name", reader)) {
        return -1;
    }

    if (!cJSON_AddItemToArray(json, jreader)) {
        return -1;
    }

    return 0;
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct) {
    set_deprecated_env_name(ENV_DRV_IFID, "DRIVER_IFID");
    set_deprecated_env_name(ENV_DRV_NAME, "DRIVER_NAME");

    struct pcsc_userdata *userdata = malloc(sizeof(struct pcsc_userdata));
    if (userdata == NULL)
        return -1;
    memset(userdata, 0, sizeof(struct pcsc_userdata));

    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));

    if (pcsc_ctx_open(userdata) < 0) {
        return -1;
    }

    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;
    ifstruct->userdata = userdata;

    return 0;
}

static int libapduinterface_main(const struct euicc_apdu_interface *ifstruct, int argc, char **argv) {
    struct pcsc_userdata *userdata = ifstruct->userdata;

    if (userdata == NULL) {
        fprintf(stderr, "No userdata set\n");
        return -1;
    }

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <list>\n", argv[0]);
        return -1;
    }

    if (strcmp(argv[1], "list") == 0) {
        cJSON *payload = cJSON_CreateObject();
        if (!payload) {
            return -1;
        }

        if (!cJSON_AddStringOrNullToObject(payload, "env", ENV_DRV_IFID)) {
            return -1;
        }

        cJSON *data = cJSON_CreateArray();
        if (!data) {
            return -1;
        }

        pcsc_iter_reader(userdata, pcsc_list_iter, data);

        if (!cJSON_AddItemToObject(payload, "data", data)) {
            return -1;
        }

        json_print("driver", payload);

        return 0;
    }

    return 0;
}

static void libapduinterface_fini(const struct euicc_apdu_interface *ifstruct) {
    struct pcsc_userdata *userdata = ifstruct->userdata;
    free(userdata);
}

const struct euicc_driver driver_if = {
    .type = DRIVER_APDU,
    .name = "pcsc",
    .init = (int (*)(void *))libapduinterface_init,
    .main = (int (*)(void *, int, char **))libapduinterface_main,
    .fini = (void (*)(void *))libapduinterface_fini,
};
