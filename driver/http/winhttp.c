#include "winhttp.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <winhttp.h>

#include <euicc/interface.h>
#include <lpac/utils.h>

static wchar_t *utf8_to_wide(const char *input) {
    if (input == NULL)
        return NULL;
    const int n = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);
    if (n == 0)
        return NULL;
    wchar_t *output = calloc(n, sizeof(wchar_t));
    if (output == NULL)
        return NULL;
    MultiByteToWideChar(CP_UTF8, 0, input, -1, output, n);
    return output;
}

static int http_interface_transmit(struct euicc_ctx *ctx, const char *url, uint32_t *rcode, uint8_t **rx,
                                   uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len, const char **h) {
    int fret = 0;
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    BOOL bResults = FALSE;

    *rx = NULL;
    *rx_len = 0;
    *rcode = 0;

    URL_COMPONENTS urlComp = {0};
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwHostNameLength = 256;
    urlComp.lpszHostName = calloc(urlComp.dwHostNameLength, sizeof(WCHAR));
    urlComp.dwUrlPathLength = 1024;
    urlComp.lpszUrlPath = calloc(urlComp.dwUrlPathLength, sizeof(WCHAR));
    urlComp.nPort = INTERNET_DEFAULT_HTTPS_PORT;
    if (urlComp.lpszHostName == NULL || urlComp.lpszUrlPath == NULL)
        goto exit;

    wchar_t *wUrl = utf8_to_wide(url);
    if (wUrl == NULL)
        goto exit;

    if (!WinHttpCrackUrl(wUrl, wcslen(wUrl), 0, &urlComp)) {
        free(wUrl);
        goto error;
    }
    free(wUrl);

    hSession =
        WinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession == NULL)
        goto error;

    hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nPort, 0);
    if (hConnect == NULL)
        goto error;

    hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath, NULL, WINHTTP_NO_REFERER,
                                  WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest)
        goto error;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &(DWORD){SECURITY_FLAG_IGNORE_UNKNOWN_CA}, sizeof(DWORD));
    WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &(DWORD){WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS},
                     sizeof(DWORD));
    WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);

    if (h) {
        for (int i = 0; h[i] != NULL; i++) {
            wchar_t *wHeader = utf8_to_wide(h[i]);
            if (!wHeader)
                goto error;
            bResults = WinHttpAddRequestHeaders(hRequest, wHeader, (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
            free(wHeader);
            if (!bResults)
                goto error;
        }
    }

    bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)tx, tx_len, tx_len, 0);
    if (!bResults)
        goto error;

    bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResults)
        goto error;

    DWORD dwStatusCode = 0;
    DWORD dwSize = sizeof(dwStatusCode);
    bResults = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                   WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
    if (!bResults)
        goto error;
    *rcode = dwStatusCode;

    DWORD dwDataSize = 0, dwDownloaded = 0;
    uint8_t *response_buffer = NULL;
    uint32_t total_size = 0;

    do {
        if (!WinHttpQueryDataAvailable(hRequest, &dwDataSize)) {
            fprintf(stderr, "WinHttpQueryDataAvailable error: %d\n", (int)GetLastError());
            free(response_buffer);
            goto error;
        }
        if (dwDataSize == 0)
            break;

        uint8_t *new_buffer = realloc(response_buffer, total_size + dwDataSize);
        if (!new_buffer) {
            free(response_buffer);
            fret = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        response_buffer = new_buffer;

        if (!WinHttpReadData(hRequest, response_buffer + total_size, dwDataSize, &dwDownloaded)) {
            fret = (int)GetLastError();
            fprintf(stderr, "WinHttpReadData error: %d\n", fret);
            free(response_buffer);
            goto exit;
        }
        total_size += dwDownloaded;
    } while (dwDataSize > 0);

    uint8_t *final_buf = realloc(response_buffer, total_size + 1);
    if (!final_buf && total_size != 0) {
        free(response_buffer);
        goto error;
    }
    if (final_buf)
        final_buf[total_size] = 0;

    *rx = final_buf;
    *rx_len = total_size;

    fret = 0;
    goto exit;

error:
    fret = -1;
    fprintf(stderr, "WinHTTP error: %d\n", (int)GetLastError());
exit:
    if (hRequest != NULL)
        WinHttpCloseHandle(hRequest);
    if (hConnect != NULL)
        WinHttpCloseHandle(hConnect);
    if (hSession != NULL)
        WinHttpCloseHandle(hSession);
    if (urlComp.lpszHostName != NULL)
        free(urlComp.lpszHostName);
    if (urlComp.lpszUrlPath != NULL)
        free(urlComp.lpszUrlPath);
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

const struct euicc_driver driver_http_winhttp = {
    .type = DRIVER_HTTP,
    .name = "winhttp",
    .init = (int (*)(void *))libhttpinterface_init,
    .main = NULL,
    .fini = NULL,
};
