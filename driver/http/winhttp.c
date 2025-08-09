#include "winhttp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <windows.h>
#include <winhttp.h>

#include <euicc/interface.h>

#include "euicc/euicc.h"

struct winhttp_userdata {
    HINTERNET hSession;
};

static wchar_t *toLPCWSTR(const char *input) {
    const int n = MultiByteToWideChar(CP_ACP, 0, input, -1, NULL, 0);
    wchar_t *output = calloc(sizeof(wchar_t), n);
    MultiByteToWideChar(CP_ACP, 0, input, -1, output, 4096);
    return output;
}

static int toUrlComponents(const char *url, URL_COMPONENTS *urlComponents) {
    memset(urlComponents, 0, sizeof(URL_COMPONENTS));
    urlComponents->dwStructSize = sizeof(URL_COMPONENTS);
    urlComponents->dwHostNameLength = -1;
    urlComponents->dwUrlPathLength = -1;
    wchar_t *wUrl = toLPCWSTR(url);
    const int ret = WinHttpCrackUrl(wUrl, 0, 0, urlComponents);
    free(wUrl);
    return ret;
}

static int http_interface_transmit(struct euicc_ctx *ctx, const char *url, uint32_t *rcode, uint8_t **rx,
                                   uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len, const char **h) {
    int fret = 0;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    URL_COMPONENTS urlComponents;
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    BOOL bResults = FALSE;

    if (toUrlComponents(url, &urlComponents) != 0) {
        fret = -1;
        goto exit;
    }
    // Open a session
    {
        hSession = WinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
                               0);
        if (hSession == NULL) goto error;
    }
    // Set the SSL options
    {
        DWORD dwOptions = SECURITY_FLAG_IGNORE_UNKNOWN_CA;
        bResults = WinHttpSetOption(hSession, WINHTTP_OPTION_SECURITY_FLAGS, &dwOptions, sizeof(DWORD));
        if (bResults == FALSE) goto error;
    }
    // Connect to the server
    {
        hConnect = WinHttpConnect(hSession, urlComponents.lpszHostName, INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (hConnect == NULL) goto error;
    }
    // Open Request
    {
        hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComponents.lpszUrlPath, NULL, WINHTTP_NO_REFERER,
                                      WINHTTP_DEFAULT_ACCEPT_TYPES,
                                      WINHTTP_FLAG_SECURE);
        if (hRequest == NULL) goto error;
    }
    // Add HTTP header to hRequest
    {
        wchar_t *header = NULL;
        for (int i = 0; h[i] != NULL; i++) {
            header = toLPCWSTR(h[i]);
            bResults = WinHttpAddRequestHeaders(hRequest, header, (ULONG) -1L, WINHTTP_ADDREQ_FLAG_ADD);
            if (bResults == FALSE) goto error;
        }
        free(header);
    }
    // Send Request
    {
        wchar_t *body = toLPCWSTR((const char *) tx);
        bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, body, tx_len, tx_len, 0);
        free(body);
        if (bResults == FALSE) goto error;
    }
    // Receive Response
    {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
        if (bResults == FALSE) goto error;
    }
    // Get the response status code
    {
        bResults = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                       WINHTTP_HEADER_NAME_BY_INDEX,
                                       &rcode, NULL, WINHTTP_NO_HEADER_INDEX);
        if (bResults == FALSE) goto error;
    }

    *rx = NULL;
    *rx_len = 0;

    LPSTR pszOutBuffer;
    uint8_t *rx_buffer = NULL;
    uint32_t rx_offset = 0;
    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) goto error;
        pszOutBuffer = malloc(dwSize + 1);
        if (pszOutBuffer == NULL) goto error;
        memset(pszOutBuffer, 0, dwSize + 1);
        if (!WinHttpReadData(hRequest, pszOutBuffer, dwSize, &dwDownloaded)) goto error;

        rx_buffer = realloc(rx_buffer, dwDownloaded);
        memcpy(rx_buffer + rx_offset, rx, dwSize);
        rx_offset += dwSize;

        free(pszOutBuffer);

        if (!dwDownloaded) break;
    } while (dwSize > 0);

    *rx = rx_buffer;
    *rx_len = dwDownloaded;

    goto exit;
error:
    fret = (int) GetLastError();
exit:
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    return fret;
}

static int libhttpinterface_init(struct euicc_http_interface *ifstruct) {
    if (WinHttpCheckPlatform() == FALSE) {
        return -1;
    }

    memset(ifstruct, 0, sizeof(struct euicc_http_interface));
    ifstruct->transmit = http_interface_transmit;

    return 0;
}

static int libhttpinterface_main(int argc, char **argv) {
    return 0;
}

static void libhttpinterface_fini(struct euicc_http_interface *ifstruct) {
    if (ifstruct->userdata == NULL) return;
    struct winhttp_userdata *userdata = ifstruct->userdata;
    if (userdata->hSession != NULL) {
        WinHttpCloseHandle(userdata->hSession);
    }
    free(userdata);
}

const struct euicc_driver driver_http_winhttp = {
    .type = DRIVER_HTTP,
    .name = "winhttp",
    .init = (int (*)(void *)) libhttpinterface_init,
    .main = libhttpinterface_main,
    .fini = (void (*)(void *)) libhttpinterface_fini,
};
