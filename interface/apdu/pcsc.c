#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef __MINGW32__
#include <winscard.h>
#else
#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#endif

#include <euicc/interface.h>

#define EUICC_INTERFACE_BUFSZ 264

// #define APDU_ST33_MAGIC "\x90\xBD\x36\xBB\x00"
#define APDU_TERMINAL_CAPABILITIES "\x80\xAA\x00\x00\x0A\xA9\x08\x81\x00\x82\x01\x01\x83\x01\x07"
#define APDU_OPENLOGICCHANNEL "\x00\x70\x00\x00\x01"
#define APDU_CLOSELOGICCHANNEL "\x00\x70\x80\xFF\x00"
#define APDU_SELECT_HEADER "\x00\xA4\x04\x00\xFF"

static SCARDCONTEXT pcsc_ctx;
static SCARDHANDLE pcsc_hCard;
static LPSTR pcsc_mszReaders;

static int pcsc_ctx_open(void)
{
    int ret;
    DWORD dwReaders;

    pcsc_ctx = 0;
    pcsc_hCard = 0;
    pcsc_mszReaders = NULL;

    ret = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &pcsc_ctx);
    if (ret != SCARD_S_SUCCESS)
    {
        fprintf(stderr, "SCardEstablishContext() failed: %08X\n", ret);
        return -1;
    }

    dwReaders = SCARD_AUTOALLOCATE;
    ret = SCardListReaders(pcsc_ctx, NULL, (LPSTR)&pcsc_mszReaders, &dwReaders);
    if (ret != SCARD_S_SUCCESS)
    {
        fprintf(stderr, "SCardListReaders() failed: %08X\n", ret);
        return -1;
    }

    return 0;
}

static int pcsc_iter_reader(int (*callback)(int index, const char *reader))
{
    int ret;
    LPSTR psReader;

    psReader = pcsc_mszReaders;
    for (int i = 0, n = 0;; i++)
    {
        char *p = pcsc_mszReaders + i;
        if (*p == '\0')
        {
            ret = callback(n, psReader);
            if (ret < 0)
                return -1;
            if (ret > 0)
                return 0;
            if (*(p + 1) == '\0')
            {
                break;
            }
            psReader = p + 1;
            n++;
        }
    }
    return -1;
}

static int pcsc_open_hCard_iter(int index, const char *reader)
{
    int ret;
    int id;
    DWORD dwActiveProtocol;

    id = 0;
    if (getenv("DRIVER_IFID"))
    {
        id = atoi(getenv("DRIVER_IFID"));
    }

    if (id != index)
    {
        return 0;
    }

    ret = SCardConnect(pcsc_ctx, reader, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_T0, &pcsc_hCard, &dwActiveProtocol);
    if (ret != SCARD_S_SUCCESS)
    {
        fprintf(stderr, "SCardConnect() failed: %08X\n", ret);
        return -1;
    }

    return 1;
}

static int pcsc_open_hCard(void)
{
    return pcsc_iter_reader(pcsc_open_hCard_iter);
}

static void pcsc_close(void)
{
    if (pcsc_mszReaders)
    {
        SCardFreeMemory(pcsc_ctx, pcsc_mszReaders);
    }
    if (pcsc_hCard)
    {
        SCardDisconnect(pcsc_hCard, SCARD_UNPOWER_CARD);
    }
    if (pcsc_ctx)
    {
        SCardReleaseContext(pcsc_ctx);
    }
    pcsc_ctx = 0;
    pcsc_hCard = 0;
    pcsc_mszReaders = NULL;
}

static int pcsc_transmit_lowlevel(uint8_t *rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len)
{
    int ret;
    DWORD rx_len_merged;

    rx_len_merged = *rx_len;
    ret = SCardTransmit(pcsc_hCard, SCARD_PCI_T0, tx, tx_len, NULL, rx, &rx_len_merged);
    if (ret != SCARD_S_SUCCESS)
    {
        fprintf(stderr, "SCardTransmit() failed: %08X\n", ret);
        return -1;
    }

    *rx_len = rx_len_merged;
    return 0;
}

static void pcsc_logic_channel_close(uint8_t channel)
{
    uint8_t tx[sizeof(APDU_CLOSELOGICCHANNEL) - 1];
    uint8_t rx[EUICC_INTERFACE_BUFSZ];
    uint32_t rx_len;

    memcpy(tx, APDU_CLOSELOGICCHANNEL, sizeof(tx));
    tx[3] = channel;

    rx_len = sizeof(rx);

    pcsc_transmit_lowlevel(rx, &rx_len, tx, sizeof(tx));
}

static int pcsc_logic_channel_open(const uint8_t *aid, uint8_t aid_len)
{
    int channel = 0;
    uint8_t tx[EUICC_INTERFACE_BUFSZ];
    uint8_t *tx_wptr;
    uint8_t rx[EUICC_INTERFACE_BUFSZ];
    uint32_t rx_len;

    if (aid_len > 32)
    {
        goto err;
    }

    rx_len = sizeof(rx);
    if (pcsc_transmit_lowlevel(rx, &rx_len, (const uint8_t *)APDU_OPENLOGICCHANNEL, sizeof(APDU_OPENLOGICCHANNEL) - 1) < 0)
    {
        goto err;
    }

    if (rx_len != 3)
    {
        goto err;
    }

    if ((rx[1] & 0xF0) != 0x90)
    {
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
    if (pcsc_transmit_lowlevel(rx, &rx_len, tx, tx_wptr - tx) < 0)
    {
        goto err;
    }

    if (rx_len < 2)
    {
        goto err;
    }

    switch (rx[rx_len - 2])
    {
    case 0x90:
    case 0x61:
        return channel;
    default:
        goto err;
    }

err:
    if (channel)
    {
        pcsc_logic_channel_close(channel);
    }

    return -1;
}

static int apdu_interface_connect(void)
{
    uint8_t rx[EUICC_INTERFACE_BUFSZ];
    uint32_t rx_len;

    if (pcsc_ctx_open() < 0)
    {
        return -1;
    }

    if (pcsc_open_hCard() < 0)
    {
        return -1;
    }

    rx_len = sizeof(rx);
    pcsc_transmit_lowlevel(rx, &rx_len, (const uint8_t *)APDU_TERMINAL_CAPABILITIES, sizeof(APDU_TERMINAL_CAPABILITIES) - 1);

    return 0;
}

static void apdu_interface_disconnect(void)
{
    pcsc_close();
}

static int apdu_interface_transmit(uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len)
{
    *rx = malloc(EUICC_INTERFACE_BUFSZ);
    if (!*rx)
    {
        fprintf(stderr, "SCardTransmit() RX buffer alloc failed\n");
        return -1;
    }
    *rx_len = EUICC_INTERFACE_BUFSZ;

    if (pcsc_transmit_lowlevel(*rx, rx_len, tx, tx_len) < 0)
    {
        free(*rx);
        *rx_len = 0;
        return -1;
    }

    return 0;
}

static int apdu_interface_logic_channel_open(const uint8_t *aid, uint8_t aid_len)
{
    return pcsc_logic_channel_open(aid, aid_len);
}

static void apdu_interface_logic_channel_close(uint8_t channel)
{
    pcsc_logic_channel_close(channel);
}

int libapduinterface_main(struct euicc_apdu_interface *ifstruct)
{
    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;

    return 0;
}
