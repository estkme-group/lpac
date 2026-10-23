#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include <wincrypt.h>
#include <winhttp.h>

#include <driver.h>
#include <euicc/interface.h>
#include <lpac/utils.h>

DEFINE_TRIVIAL_CLEANUP_FUNC(HINTERNET, WinHttpCloseHandle);

/*
 * By default lpac does not verify the SM-DP+/SM-DS server's TLS certificate,
 * since some servers use certificates issued by CAs that are not (yet) part
 * of the GSMA CI trust store shipped with common OS trust stores. Setting
 * this environment variable to a truthy value re-enables standard
 * certificate/hostname verification. Shared with the curl backend, see
 * driver/http/curl.c.
 */
#define ENV_SSL_VERIFY "LPAC_HTTP_SSL_VERIFY"

/*
 * Path to a PEM file with the CA certificates to trust, e.g. the GSMA CI
 * certificates. If set, only these certificates are trusted (the Windows
 * certificate stores are not used) and verification is enabled unless
 * ENV_SSL_VERIFY is explicitly set to a falsy value. Shared with the curl
 * backend, see driver/http/curl.c.
 */
#define ENV_TRUST_STORE "LPAC_HTTP_TRUST_STORE"

/* Populated once in libhttpinterface_init() from ENV_SSL_VERIFY. */
static DWORD security_flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA;

/*
 * Populated once in libhttpinterface_init() from ENV_TRUST_STORE, NULL if not
 * set: the certificates of the trust store and a certificate chain engine,
 * which uses them as its exclusive trusted roots.
 */
static HCERTSTORE trust_store = NULL;
static HCERTCHAINENGINE trust_engine = NULL;

static wchar_t *utf8_to_wide(const char *input) {
    if (input == NULL)
        return NULL;
    const int n = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);
    if (n < 0)
        return NULL;
    wchar_t *output = calloc(n, sizeof(wchar_t));
    if (output == NULL)
        return NULL;
    if (MultiByteToWideChar(CP_UTF8, 0, input, -1, output, n) > 0)
        return output;
    free(output);
    return NULL;
}

/*
 * Verify the certificate of the server, which the request is connected to,
 * against the trust store, including the hostname and the server
 * authentication usage.
 */
static bool verify_server_certificate(HINTERNET hRequest, const wchar_t *hostname) {
    LPSTR server_auth = szOID_PKIX_KP_SERVER_AUTH;
    CERT_CHAIN_PARA chain_para = {
        .cbSize = sizeof(chain_para),
        .RequestedUsage = {.dwType = USAGE_MATCH_TYPE_AND,
                           .Usage = {.cUsageIdentifier = 1, .rgpszUsageIdentifier = &server_auth}},
    };
    SSL_EXTRA_CERT_CHAIN_POLICY_PARA ssl_para = {
        .cbSize = sizeof(ssl_para),
        .dwAuthType = AUTHTYPE_SERVER,
        .pwszServerName = (WCHAR *)hostname,
    };
    CERT_CHAIN_POLICY_PARA policy_para = {.cbSize = sizeof(policy_para), .pvExtraPolicyPara = &ssl_para};
    CERT_CHAIN_POLICY_STATUS policy_status = {.cbSize = sizeof(policy_status)};
    PCCERT_CONTEXT cert = NULL;
    PCCERT_CHAIN_CONTEXT chain = NULL;
    DWORD size = sizeof(cert);
    bool verified = false;

    /* The store of the server certificate holds the intermediate certificates sent by the server. */
    if (!WinHttpQueryOption(hRequest, WINHTTP_OPTION_SERVER_CERT_CONTEXT, &cert, &size) || cert == NULL) {
        fprintf(stderr, "WinHTTP: cannot get the server certificate: %d\n", (int)GetLastError());
    } else if (!CertGetCertificateChain(trust_engine, cert, NULL, cert->hCertStore, &chain_para, 0, NULL, &chain)) {
        fprintf(stderr, "WinHTTP: cannot build the server certificate chain: %d\n", (int)GetLastError());
    } else if ((chain->TrustStatus.dwErrorStatus & ~(DWORD)CERT_TRUST_IS_NOT_TIME_NESTED) != CERT_TRUST_NO_ERROR) {
        /*
         * Checked explicitly (like curl's Schannel backend does), since not
         * every implementation of the SSL policy below rejects e.g. a chain
         * that doesn't end at a certificate of the trust store at all
         * (CERT_TRUST_IS_PARTIAL_CHAIN).
         */
        fprintf(stderr, "WinHTTP: server certificate not trusted by '%s': chain error status 0x%08lx\n",
                ENV_TRUST_STORE, (unsigned long)chain->TrustStatus.dwErrorStatus);
    } else if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, chain, &policy_para, &policy_status)) {
        fprintf(stderr, "WinHTTP: cannot verify the server certificate chain: %d\n", (int)GetLastError());
    } else if (policy_status.dwError != ERROR_SUCCESS) {
        /* e.g. CERT_E_CN_NO_MATCH, if the hostname doesn't match */
        fprintf(stderr, "WinHTTP: server certificate verification failed: 0x%08lx\n",
                (unsigned long)policy_status.dwError);
    } else {
        verified = true;
    }

    if (chain != NULL)
        CertFreeCertificateChain(chain);
    if (cert != NULL)
        CertFreeCertificateContext(cert);
    return verified;
}

static int http_interface_transmit(struct euicc_ctx *ctx, const char *url, uint32_t *rcode, uint8_t **rx,
                                   uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len, const char **h) {
    int fret = 0;
    _cleanup_(WinHttpCloseHandlep) HINTERNET hSession = NULL;
    _cleanup_(WinHttpCloseHandlep) HINTERNET hConnect = NULL;
    _cleanup_(WinHttpCloseHandlep) HINTERNET hRequest = NULL;
    wchar_t hostname[256];
    wchar_t pathname[128];
    BOOL bResults = FALSE;

    *rx = NULL;
    *rx_len = 0;
    *rcode = 0;

    URL_COMPONENTS urlComp = {
        .dwStructSize = sizeof(urlComp),
        .dwHostNameLength = sizeof(hostname),
        .lpszHostName = hostname,
        .dwUrlPathLength = sizeof(pathname),
        .lpszUrlPath = pathname,
        .nPort = INTERNET_DEFAULT_HTTPS_PORT,
    };
    _cleanup_free_ const wchar_t *wUrl = utf8_to_wide(url);
    if (wUrl == NULL)
        goto error;
    if (!WinHttpCrackUrl(wUrl, wcslen(wUrl), 0, &urlComp))
        goto error;

    hSession =
        WinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession == NULL)
        goto error;

    hConnect = WinHttpConnect(hSession, hostname, urlComp.nPort, 0);
    if (hConnect == NULL)
        goto error;

    hRequest = WinHttpOpenRequest(hConnect, L"POST", pathname, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                  WINHTTP_FLAG_SECURE);
    if (!hRequest)
        goto error;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &security_flags, sizeof(security_flags));
    /*
     * With a trust store, each server has to be verified against it, so don't
     * follow redirects then (the curl backend never follows them either).
     */
    DWORD redirect_policy =
        trust_engine != NULL ? WINHTTP_OPTION_REDIRECT_POLICY_NEVER : WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirect_policy, sizeof(redirect_policy));
    WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);

    for (int i = 0; h[i] != NULL; i++) {
        _cleanup_free_ const wchar_t *wHeader = utf8_to_wide(h[i]);
        if (wHeader == NULL)
            goto error;
        if (!WinHttpAddRequestHeaders(hRequest, wHeader, (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD))
            goto error;
    }

    if (trust_engine == NULL) {
        bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)tx, tx_len, tx_len, 0);
        if (!bResults)
            goto error;
    } else {
        /*
         * WinHTTP accepts unknown CAs here (see security_flags), so only send
         * the request headers first, which establishes the TLS connection,
         * and send the body only after the server certificate has been
         * verified against the trust store.
         */
        bResults =
            WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, tx_len, 0);
        if (!bResults)
            goto error;
        if (!verify_server_certificate(hRequest, hostname)) {
            fret = -1;
            goto exit;
        }
        for (DWORD sent = 0, written = 0; sent < tx_len; sent += written) {
            if (!WinHttpWriteData(hRequest, tx + sent, tx_len - sent, &written) || written == 0)
                goto error;
        }
    }

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
        if (new_buffer == NULL) {
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
    if (final_buf != NULL)
        final_buf[total_size] = 0;

    *rx = final_buf;
    *rx_len = total_size;

    fret = 0;
    goto exit;

error:
    fret = -1;
    fprintf(stderr, "WinHTTP error: %d\n", (int)GetLastError());
exit:
    return fret;
}

static void close_trust_store(void) {
    if (trust_engine != NULL) {
        CertFreeCertificateChainEngine(trust_engine);
        trust_engine = NULL;
    }
    if (trust_store != NULL) {
        CertCloseStore(trust_store, 0);
        trust_store = NULL;
    }
}

/* Read a whole file into a NUL terminated buffer. */
static char *read_file(const char *path) {
    char *data = NULL;
    long size = -1;

    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
        return NULL;
    if (fseek(fp, 0, SEEK_END) == 0)
        size = ftell(fp);
    if (size >= 0 && fseek(fp, 0, SEEK_SET) == 0)
        data = malloc((size_t)size + 1);
    if (data != NULL && fread(data, 1, (size_t)size, fp) == (size_t)size) {
        data[size] = '\0';
    } else {
        free(data);
        data = NULL;
    }
    fclose(fp);
    return data;
}

/*
 * Load the PEM encoded certificates from path into trust_store and create
 * trust_engine, which uses them as its exclusive trusted roots. Like OpenSSL
 * does, any text outside of the certificate PEM blocks is ignored.
 */
static int load_trust_store(const char *path) {
    static const char pem_begin[] = "-----BEGIN CERTIFICATE-----";
    static const char pem_end[] = "-----END CERTIFICATE-----";
    _cleanup_free_ char *pem = read_file(path);
    CERT_CHAIN_ENGINE_CONFIG config;
    int count = 0;

    if (pem == NULL) {
        fprintf(stderr, "%s: cannot read '%s': %s\n", ENV_TRUST_STORE, path, strerror(errno));
        return -1;
    }

    trust_store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_CREATE_NEW_FLAG, NULL);
    if (trust_store == NULL) {
        fprintf(stderr, "%s: cannot create a certificate store: %d\n", ENV_TRUST_STORE, (int)GetLastError());
        goto error;
    }

    for (const char *block = strstr(pem, pem_begin); block != NULL; block = strstr(block, pem_begin)) {
        const char *block_end = strstr(block, pem_end);
        /* A block without end line is passed on completely and rejected by CryptStringToBinaryA(). */
        const DWORD block_len = (DWORD)(block_end != NULL ? block_end - block + strlen(pem_end) : strlen(block));
        _cleanup_free_ BYTE *der = NULL;
        DWORD der_len = 0;

        count++;
        if (!CryptStringToBinaryA(block, block_len, CRYPT_STRING_BASE64HEADER, NULL, &der_len, NULL, NULL)
            || (der = malloc(der_len)) == NULL
            || !CryptStringToBinaryA(block, block_len, CRYPT_STRING_BASE64HEADER, der, &der_len, NULL, NULL)
            || !CertAddEncodedCertificateToStore(trust_store, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, der, der_len,
                                                 CERT_STORE_ADD_USE_EXISTING, NULL)) {
            fprintf(stderr, "%s: invalid certificate #%d in '%s'\n", ENV_TRUST_STORE, count, path);
            goto error;
        }
        block += block_len;
    }
    if (count == 0) {
        fprintf(stderr, "%s: no certificates found in '%s'\n", ENV_TRUST_STORE, path);
        goto error;
    }

    /*
     * Use the Windows 7 layout of the structure (up to hExclusiveTrustedPeople,
     * like curl's Schannel backend does), which later Windows versions and
     * Wine accept as well.
     */
    memset(&config, 0, sizeof(config));
    config.cbSize = (DWORD)(offsetof(CERT_CHAIN_ENGINE_CONFIG, hExclusiveTrustedPeople) + sizeof(HCERTSTORE));
    config.hExclusiveRoot = trust_store;
    if (!CertCreateCertificateChainEngine(&config, &trust_engine)) {
        fprintf(stderr, "%s: cannot create a certificate chain engine: %d\n", ENV_TRUST_STORE, (int)GetLastError());
        goto error;
    }
    return 0;

error:
    close_trust_store();
    return -1;
}

static int libhttpinterface_init(struct euicc_http_interface *ifstruct) {
    if (WinHttpCheckPlatform() == FALSE) {
        return -1;
    }

    memset(ifstruct, 0, sizeof(struct euicc_http_interface));

    /*
     * TLS certificate verification is disabled by default for backward
     * compatibility (see ENV_SSL_VERIFY above). Set the env variable to
     * enable standard verification. Configuring a trust store
     * (ENV_TRUST_STORE) enables it too, unless ENV_SSL_VERIFY is explicitly
     * disabled.
     */
    const char *trust_store_path = getenv_or_default(ENV_TRUST_STORE, (char *)NULL);
    const bool verify = getenv_or_default(ENV_SSL_VERIFY, (bool)(trust_store_path != NULL));
    if (!verify && trust_store_path != NULL) {
        fprintf(stderr, "WARNING: '%s' is ignored, since '%s' disables TLS verification\n", ENV_TRUST_STORE,
                ENV_SSL_VERIFY);
        trust_store_path = NULL;
    }

    close_trust_store();
    if (trust_store_path != NULL && load_trust_store(trust_store_path) != 0) {
        return -1;
    }

    /*
     * With a trust store, WinHTTP still has to accept the CA, which is
     * unknown to Windows: http_interface_transmit() verifies the server
     * certificate against the trust store instead.
     */
    security_flags = (verify && trust_engine == NULL) ? 0 : SECURITY_FLAG_IGNORE_UNKNOWN_CA;

    ifstruct->transmit = http_interface_transmit;

    return 0;
}

static void libhttpinterface_fini(struct euicc_http_interface *ifstruct) { close_trust_store(); }

DRIVER_INTERFACE = {
    .type = DRIVER_HTTP,
    .name = "winhttp",
    .init = (int (*)(void *))libhttpinterface_init,
    .main = NULL,
    .fini = (void (*)(void *))libhttpinterface_fini,
};
