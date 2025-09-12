#include "curl.h"

#include <euicc/interface.h>
#include <lpac/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

DEFINE_TRIVIAL_CLEANUP_FUNC(struct curl_slist *, curl_slist_free_all);

struct http_trans_response_data {
    uint8_t *data;
    size_t size;
};

__attribute__((unused)) static void http_trans_response_data_free(struct http_trans_response_data *ptr) {
    if (ptr == NULL)
        return;
    if (ptr->data != NULL)
        free(ptr->data);
    free(ptr);
}

static size_t http_trans_write_callback(const void *contents, const size_t size, const size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct http_trans_response_data *mem = userp;

    mem->data = realloc(mem->data, mem->size + realsize + 1);
    if (mem->data == NULL) {
        fprintf(stderr, "not enough memory\n"); /* out of memory! */
        return 0;
    }

    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

static int http_interface_transmit(__attribute__((unused)) struct euicc_ctx *ctx, const char *url, uint32_t *rcode,
                                   uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, const uint32_t tx_len,
                                   const char **h) {
    _cleanup_(curl_easy_cleanup) CURL *curl = NULL;
    _cleanup_(http_trans_response_data_free) struct http_trans_response_data response_data = {0};
    _cleanup_(curl_slist_free_allp) struct curl_slist *headers = NULL;
    _cleanup_(curl_slist_free_allp) struct curl_slist *nheaders = NULL;
    long response_code;

    *rx = NULL;
    *rcode = 0;

    curl = curl_easy_init();
    if (curl == NULL) {
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_trans_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_data);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    for (int i = 0; h[i] != NULL; i++) {
        nheaders = curl_slist_append(headers, h[i]);
        if (nheaders == NULL) {
            return -1;
        }
        headers = nheaders;
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (tx != NULL) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, tx);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)tx_len);
    }

    const CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        return -1;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    *rcode = response_code;
    *rx = response_data.data;
    *rx_len = response_data.size;

    return 0;
}

static int libhttpinterface_init(struct euicc_http_interface *ifstruct) {
    memset(ifstruct, 0, sizeof(struct euicc_http_interface));

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        return -1;
    }

    ifstruct->transmit = http_interface_transmit;

    return 0;
}

static int libhttpinterface_main(__attribute__((unused)) const struct euicc_apdu_interface *ifstruct, int argc,
                                 char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <info>\n", argv[0]);
        return -1;
    }
    if (strcmp(argv[1], "info") == 0) {
        cJSON *payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "curl_version", curl_version());
        json_print("driver", payload);
    }
    return 0;
}

const struct euicc_driver driver_http_curl = {
    .type = DRIVER_HTTP,
    .name = "curl",
    .init = (int (*)(void *))libhttpinterface_init,
    .main = (int (*)(void *, int, char **))libhttpinterface_main,
    .fini = NULL,
};
