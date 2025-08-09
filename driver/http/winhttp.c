#include "winhttp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <windows.h>
#include <winhttp.h>

#include <euicc/interface.h>

wchar_t *utf8_to_wide(const char *input) {
    const int len = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);
    if (len == 0) return NULL;
    wchar_t *output = (wchar_t *) malloc(len * sizeof(wchar_t));
    if (!output) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, input, -1, output, len);
    return output;
}

static int http_interface_transmit(struct euicc_ctx *ctx, const char *url, uint32_t *rcode, uint8_t **rx,
                                   uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len, const char **h) {
    int fret = 0;
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    BOOL bResults = FALSE;

    URL_COMPONENTS urlComp = {0};
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwHostNameLength = 256;
    urlComp.lpszHostName = calloc(urlComp.dwHostNameLength, sizeof(WCHAR));
    urlComp.dwUrlPathLength = 1024;
    urlComp.lpszUrlPath = calloc(urlComp.dwUrlPathLength, sizeof(WCHAR));

    wchar_t *wUrl = utf8_to_wide(url);
    if (!WinHttpCrackUrl(wUrl, 0, 0, &urlComp)) goto exit;
    free(wUrl);

    // Open a session
    hSession = WinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
                           0);
    if (hSession == NULL) goto error;
    // Connect to the server
    hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (hConnect == NULL) goto error;
    // Open Request
    hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath, NULL, WINHTTP_NO_REFERER,
                                  WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (hRequest == NULL) goto error;
    // Set the SSL options
    {
        DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA;
        bResults = WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(DWORD));
        if (!bResults) goto error;
        bResults = WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);
        if (!bResults) goto error;
    }
    // Send Request
    {
        char *headers = strdup(h[0]);
        for (int i = 1; h[i] != NULL; i++) {
            headers = strcat(headers, "\r\n");
            headers = strcat(headers, h[i]);
        }
        wchar_t *wHeaders = utf8_to_wide(headers);
        wchar_t *wPayload = utf8_to_wide((const char *) tx);
        bResults = WinHttpSendRequest(hRequest, wHeaders, -1L, (LPVOID) wPayload, tx_len, tx_len, 0);
        free(wHeaders);
        free(wPayload);
        if (!bResults) goto error;
    }
    // Receive Response
    bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResults) goto error;
    // Get the response status code
    bResults = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                   WINHTTP_HEADER_NAME_BY_INDEX, &rcode, NULL, WINHTTP_NO_HEADER_INDEX);
    if (!bResults) goto error;
    // Read the response data
    {
        *rx = NULL;
        *rx_len = 0;

        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        do {
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
            if (dwSize == 0) break;
            char *pszOutBuffer = malloc(dwSize + 1);
            if (!pszOutBuffer) break;
            if (!WinHttpReadData(hRequest, (LPVOID) pszOutBuffer, dwSize, &dwDownloaded)) break;
            pszOutBuffer[dwDownloaded] = '\0';
            printf("%s", pszOutBuffer);
            free(pszOutBuffer);
        } while (dwSize > 0);

        // *rx = buffer;
        // *rx_len = totalSize;
    }

    goto exit;
error:
    fret = (int) GetLastError();
    printf("Error: %d\n", fret);
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
}

const struct euicc_driver driver_http_winhttp = {
    .type = DRIVER_HTTP,
    .name = "winhttp",
    .init = (int (*)(void *)) libhttpinterface_init,
    .main = libhttpinterface_main,
    .fini = (void (*)(void *)) libhttpinterface_fini,
};
