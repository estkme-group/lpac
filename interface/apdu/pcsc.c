#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <winscard.h>

#define EUICC_INTERFACE_BUFSZ 264

#define APDU_OPENLOGICCHANNEL "\x00\x70\x00\x00\x01"
#define APDU_CLOSELOGICCHANNEL "\x00\x70\x80\xFF\x00"
#define APDU_SELECT_HEADER "\x00\xA4\x04\x00"

static SCARDCONTEXT ctx;
static SCARDHANDLE hCard;

int euicc_apdu_interface_connect(void)
{
    int ret;
    DWORD dwReaders;
    LPSTR mszReaders;
    LPSTR psReader;
    DWORD dwActiveProtocol;

    ret = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &ctx);
    if (ret != SCARD_S_SUCCESS)
    {
        fprintf(stderr, "SCardEstablishContext() failed: %08X\n", ret);
        return -1;
    }

    dwReaders = SCARD_AUTOALLOCATE;
    ret = SCardListReaders(ctx, NULL, (LPSTR)&mszReaders, &dwReaders);
    if (ret != SCARD_S_SUCCESS)
    {
        fprintf(stderr, "SCardListReaders() failed: %08X\n", ret);
        return -1;
    }

    psReader = mszReaders;
    for (int i = 0, n = 0;; i++)
    {
        char *p = mszReaders + i;
        psReader = mszReaders + i;
        if (*p == '\0')
        {
            psReader = p + 1;
            n++;
        }
        if (n == 0)
        {
            break;
        }
    }

    ret = SCardConnect(ctx, psReader, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0, &hCard, &dwActiveProtocol);
    if (ret != SCARD_S_SUCCESS)
    {
        fprintf(stderr, "SCardConnect() failed: %08X\n", ret);
        SCardFreeMemory(ctx, mszReaders);
        return -1;
    }

    SCardFreeMemory(ctx, mszReaders);
    return 0;
}

void euicc_apdu_interface_disconnect(void)
{
    SCardDisconnect(hCard, SCARD_UNPOWER_CARD);
    SCardReleaseContext(ctx);
    ctx = 0;
    hCard = 0;
}

int euicc_apdu_interface_transmit(unsigned char *tx, unsigned int tx_len, unsigned char *rx, long unsigned int *rx_len)
{
    int ret;
    DWORD rx_len_merged;

    rx_len_merged = *rx_len;
    ret = SCardTransmit(hCard, SCARD_PCI_T0, tx, tx_len, NULL, rx, &rx_len_merged);
    if (ret != SCARD_S_SUCCESS)
    {
        fprintf(stderr, "SCardTransmit() failed: %08X\n", ret);
        return -1;
    }
    *rx_len = rx_len_merged;
    return 0;
}

int euicc_apdu_interface_logic_channel_open(const unsigned char *aid, unsigned char aid_len)
{
    int channel;
    unsigned char tx[EUICC_INTERFACE_BUFSZ];
    unsigned int tx_len = sizeof(tx);
    unsigned char rx[EUICC_INTERFACE_BUFSZ];
    long unsigned int rx_len = sizeof(rx);
    unsigned char *wptrtx;

    rx_len = sizeof(rx);
    if (euicc_apdu_interface_transmit(APDU_OPENLOGICCHANNEL, sizeof(APDU_OPENLOGICCHANNEL) - 1, rx, &rx_len) < 0)
    {
        return -1;
    }
    if (rx_len < 2)
    {
        return -1;
    }
    if ((rx[rx_len - 2] & 0xF0) != 0x90)
    {
        return -1;
    }

    channel = rx[0];

    wptrtx = tx;
    memcpy(wptrtx, APDU_SELECT_HEADER, sizeof(APDU_SELECT_HEADER) - 1);
    wptrtx += sizeof(APDU_SELECT_HEADER) - 1;
    *wptrtx++ = aid_len;
    memcpy(wptrtx, aid, aid_len);
    wptrtx = wptrtx + aid_len;

    tx_len = wptrtx - tx;
    tx[0] = (tx[0] & 0xF0) | channel;
    rx_len = sizeof(rx);
    if (euicc_apdu_interface_transmit(tx, tx_len, rx, &rx_len) < 0)
    {
        return -1;
    }
    if (rx_len < 2)
    {
        return -1;
    }

    switch (rx[rx_len - 2])
    {
    case 0x90:
    case 0x61:
        return channel;
    default:
        return -1;
    }

    return channel;
}

void euicc_apdu_interface_logic_channel_close(unsigned char channel)
{
    unsigned char tx[sizeof(APDU_CLOSELOGICCHANNEL) - 1];
    unsigned char rx[EUICC_INTERFACE_BUFSZ];
    long unsigned int rx_len = sizeof(rx);

    memcpy(tx, APDU_CLOSELOGICCHANNEL, sizeof(tx));
    tx[3] = channel;
    euicc_apdu_interface_transmit(tx, sizeof(tx), rx, &rx_len);
}
