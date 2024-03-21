#include "curl.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <euicc/interface.h>

#ifndef _WIN32
#include <curl/curl.h>
#else
#include <dlfcn-win32/dlfcn.h>
#define CURL_GLOBAL_DEFAULT ((1 << 0) | (1 << 1))
#define CURLE_OK 0
#define CURLOPT_URL 10002
#define CURLOPT_WRITEFUNCTION 20011
#define CURLOPT_WRITEDATA 10001
#define CURLOPT_SSL_VERIFYPEER 64
#define CURLOPT_SSL_VERIFYHOST 81
#define CURLOPT_HTTPHEADER 10023
#define CURLOPT_POSTFIELDS 10015
#define CURLOPT_POSTFIELDSIZE 60
#define CURLINFO_RESPONSE_CODE 2097154

typedef void CURL;
typedef int CURLcode;
typedef int CURLoption;
typedef int CURLINFO;

static void *libcurl_interface_dlhandle = NULL;
#endif

struct http_trans_response_data
{
    uint8_t *data;
    size_t size;
};

static struct libcurl_interface
{
    CURLcode (*_curl_global_init)(long flags);
    CURL *(*_curl_easy_init)(void);
    CURLcode (*_curl_easy_setopt)(CURL *curl, CURLoption option, ...);
    CURLcode (*_curl_easy_perform)(CURL *curl);
    CURLcode (*_curl_easy_getinfo)(CURL *curl, CURLINFO info, ...);
    const char *(*_curl_easy_strerror)(CURLcode);
    void (*_curl_easy_cleanup)(CURL *curl);

    struct curl_slist *(*_curl_slist_append)(struct curl_slist *list, const char *data);
    void (*_curl_slist_free_all)(struct curl_slist *list);
} libcurl;

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

static int http_interface_transmit(struct euicc_ctx *ctx, const char *url, uint32_t *rcode, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len, const char **h)
{
    int fret = 0;
    CURL *curl;
    CURLcode res;
    struct http_trans_response_data responseData = {0};
    struct curl_slist *headers = NULL, *nheaders = NULL;
    long response_code;

    (*rx) = NULL;
    (*rcode) = 0;

    curl = libcurl._curl_easy_init();
    if (!curl)
    {
        goto err;
    }

    libcurl._curl_easy_setopt(curl, CURLOPT_URL, url);
    libcurl._curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_trans_write_callback);
    libcurl._curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&responseData);
    libcurl._curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    libcurl._curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    for (int i = 0; h[i] != NULL; i++)
    {
        nheaders = libcurl._curl_slist_append(headers, h[i]);
        if (nheaders == NULL)
        {
            goto err;
        }
        headers = nheaders;
    }
    libcurl._curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (tx != NULL)
    {
        libcurl._curl_easy_setopt(curl, CURLOPT_POSTFIELDS, tx);
        libcurl._curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, tx_len);
    }

    res = libcurl._curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", libcurl._curl_easy_strerror(res));
        goto err;
    }

    libcurl._curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    *rcode = response_code;
    *rx = responseData.data;
    *rx_len = responseData.size;

    fret = 0;
    goto exit;

err:
    fret = -1;
    free(responseData.data);
exit:
    libcurl._curl_easy_cleanup(curl);
    libcurl._curl_slist_free_all(headers);
    return fret;
}

static int _init_libcurl(void)
{
#ifdef _WIN32
    if (!(libcurl_interface_dlhandle = dlopen("libcurl.dll", RTLD_LAZY)))
    {
        fprintf(stderr, "libcurl init err: %s\n", dlerror());
        return -1;
    }

    libcurl._curl_global_init = dlsym(libcurl_interface_dlhandle, "curl_global_init");
    libcurl._curl_easy_init = dlsym(libcurl_interface_dlhandle, "curl_easy_init");
    libcurl._curl_easy_setopt = dlsym(libcurl_interface_dlhandle, "curl_easy_setopt");
    libcurl._curl_easy_perform = dlsym(libcurl_interface_dlhandle, "curl_easy_perform");
    libcurl._curl_easy_getinfo = dlsym(libcurl_interface_dlhandle, "curl_easy_getinfo");
    libcurl._curl_easy_strerror = dlsym(libcurl_interface_dlhandle, "curl_easy_strerror");
    libcurl._curl_easy_cleanup = dlsym(libcurl_interface_dlhandle, "curl_easy_cleanup");
    libcurl._curl_slist_append = dlsym(libcurl_interface_dlhandle, "curl_slist_append");
    libcurl._curl_slist_free_all = dlsym(libcurl_interface_dlhandle, "curl_slist_free_all");
#else
    libcurl._curl_global_init = curl_global_init;
    libcurl._curl_easy_init = curl_easy_init;
    libcurl._curl_easy_setopt = curl_easy_setopt;
    libcurl._curl_easy_perform = curl_easy_perform;
    libcurl._curl_easy_getinfo = curl_easy_getinfo;
    libcurl._curl_easy_strerror = curl_easy_strerror;
    libcurl._curl_easy_cleanup = curl_easy_cleanup;
    libcurl._curl_slist_append = curl_slist_append;
    libcurl._curl_slist_free_all = curl_slist_free_all;
#endif

    return 0;
}

static int libhttpinterface_init(struct euicc_http_interface *ifstruct)
{
    memset(ifstruct, 0, sizeof(struct euicc_http_interface));

    if (_init_libcurl() != 0)
    {
        return -1;
    }

    if (libcurl._curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
    {
        return -1;
    }

    ifstruct->transmit = http_interface_transmit;

    return 0;
}

static int libhttpinterface_main(int argc, char **argv)
{
    return 0;
}

static void libhttpinterface_fini(void)
{
}

const struct euicc_driver driver_http_curl = {
    .type = DRIVER_HTTP,
    .name = "curl",
    .init = (int (*)(void *))libhttpinterface_init,
    .main = libhttpinterface_main,
    .fini = libhttpinterface_fini,
};
