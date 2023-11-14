#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include <euicc/interface.h>

struct http_trans_response_data
{
    uint8_t *data;
    size_t size;
};

static size_t http_trans_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct http_trans_response_data *mem = (struct http_trans_response_data *)userp;

    mem->data = realloc(mem->data, mem->size + realsize + 1);
    if (mem->data == NULL)
    {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

static int http_interface_transmit(const char *url, uint32_t *rcode, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len)
{
    int fret = 0;
    CURL *curl;
    CURLcode res;
    struct http_trans_response_data responseData = {0};
    struct curl_slist *headers = NULL, *nheaders = NULL;
    long response_code;
    const char *h[] = {
        "User-Agent: gsma-rsp-lpad",
        "X-Admin-Protocol: gsma/rsp/v2.2.0",
        "Content-Type: application/json",
        NULL,
    };

    (*rx) = NULL;
    (*rcode) = 0;

    curl = curl_easy_init();
    if (!curl)
    {
        goto err;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_trans_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&responseData);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    for (int i = 0; h[i] != NULL; i++)
    {
        nheaders = curl_slist_append(headers, h[i]);
        if (nheaders == NULL)
        {
            goto err;
        }
        headers = nheaders;
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (tx != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, tx);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, tx_len);
    }

    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        goto err;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    *rcode = response_code;
    *rx = responseData.data;
    *rx_len = responseData.size;

    fret = 0;
    goto exit;

err:
    fret = -1;
    free(responseData.data);
exit:
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return fret;
}

int libhttpinterface_main(struct euicc_http_interface *ifstruct)
{
    memset(ifstruct, 0, sizeof(struct euicc_http_interface));

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
    {
        return -1;
    }

    ifstruct->transmit = http_interface_transmit;

    return 0;
}
