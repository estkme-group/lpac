#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct es9p_trans_response_data
{
    uint8_t *data;
    size_t size;
};

static size_t es9p_trans_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct es9p_trans_response_data *mem = (struct es9p_trans_response_data *)userp;

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

int euicc_es9p_interface_transmit(unsigned int *rcode, unsigned char **rbuf, const char *url, const unsigned char *sbuf, unsigned int slen)
{
    int fret = 0;
    CURL *curl;
    CURLcode res;
    struct es9p_trans_response_data responseData = {0};
    struct curl_slist *headers = NULL, *nheaders = NULL;
    long response_code;
    const char *h[] = {
        "User-Agent: gsma-rsp-lpad",
        "X-Admin-Protocol: gsma/rsp/v2.2.0",
        "Content-Type: application/json",
        NULL,
    };

    (*rbuf) = NULL;
    (*rcode) = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl)
    {
        goto err;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, es9p_trans_write_callback);
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

    if (sbuf != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sbuf);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, slen);
    }

    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        goto err;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    *rcode = (uint16_t)response_code;
    *rbuf = responseData.data;
    fret = responseData.size;
    goto exit;

err:
    fret = -1;
    free(responseData.data);
exit:
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    curl_global_cleanup();
    return fret;
}
