#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __MINGW32__
#include <dlfcn-win32/dlfcn.h>
#else
#include <dlfcn.h>
#endif

#include <euicc/interface.h>

/* BEGIN MINIMAL CURL DEFINE */
#if defined (_WIN32) || defined (__CYGWIN__)
#define LIBCURL_DEFAULT_PATH "libcurl.dll"
#elif defined(__APPLE__)
#define LIBCURL_DEFAULT_PATH "libcurl.4.dylib"
#else
#define LIBCURL_DEFAULT_PATH "libcurl.so.4"
#endif

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

static struct libcurl_interface
{
    CURLcode (*curl_global_init)(long flags);
    CURL *(*curl_easy_init)(void);
    CURLcode (*curl_easy_setopt)(CURL *curl, CURLoption option, ...);
    CURLcode (*curl_easy_perform)(CURL *curl);
    CURLcode (*curl_easy_getinfo)(CURL *curl, CURLINFO info, ...);
    const char *(*curl_easy_strerror)(CURLcode);
    void (*curl_easy_cleanup)(CURL *curl);

    struct curl_slist *(*curl_slist_append)(struct curl_slist *list, const char *data);
    void (*curl_slist_free_all)(struct curl_slist *list);
} libcurl;
/* END MINIMAL CURL DEFINE */

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

    curl = libcurl.curl_easy_init();
    if (!curl)
    {
        goto err;
    }

    libcurl.curl_easy_setopt(curl, CURLOPT_URL, url);
    libcurl.curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_trans_write_callback);
    libcurl.curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&responseData);
    libcurl.curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    libcurl.curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    for (int i = 0; h[i] != NULL; i++)
    {
        nheaders = libcurl.curl_slist_append(headers, h[i]);
        if (nheaders == NULL)
        {
            goto err;
        }
        headers = nheaders;
    }
    libcurl.curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (tx != NULL)
    {
        libcurl.curl_easy_setopt(curl, CURLOPT_POSTFIELDS, tx);
        libcurl.curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, tx_len);
    }

    res = libcurl.curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", libcurl.curl_easy_strerror(res));
        goto err;
    }

    libcurl.curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    *rcode = response_code;
    *rx = responseData.data;
    *rx_len = responseData.size;

    fret = 0;
    goto exit;

err:
    fret = -1;
    free(responseData.data);
exit:
    libcurl.curl_easy_cleanup(curl);
    libcurl.curl_slist_free_all(headers);
    return fret;
}

EUICC_SHARED_EXPORT int libhttpinterface_init(struct euicc_http_interface *ifstruct)
{
    const char *libcurl_path;

    memset(ifstruct, 0, sizeof(struct euicc_http_interface));

    if (!(libcurl_path = getenv("LIBCURL")))
    {
        libcurl_path = LIBCURL_DEFAULT_PATH;
    }

    if (!(libcurl_interface_dlhandle = dlopen(libcurl_path, RTLD_LAZY)))
    {
        fprintf(stderr, "libcurl env missing, current: LIBCURL=%s err:%s\n", libcurl_path, dlerror());
        return -1;
    }

    libcurl.curl_global_init = dlsym(libcurl_interface_dlhandle, "curl_global_init");
    libcurl.curl_easy_init = dlsym(libcurl_interface_dlhandle, "curl_easy_init");
    libcurl.curl_easy_setopt = dlsym(libcurl_interface_dlhandle, "curl_easy_setopt");
    libcurl.curl_easy_perform = dlsym(libcurl_interface_dlhandle, "curl_easy_perform");
    libcurl.curl_easy_getinfo = dlsym(libcurl_interface_dlhandle, "curl_easy_getinfo");
    libcurl.curl_easy_strerror = dlsym(libcurl_interface_dlhandle, "curl_easy_strerror");
    libcurl.curl_easy_cleanup = dlsym(libcurl_interface_dlhandle, "curl_easy_cleanup");
    libcurl.curl_slist_append = dlsym(libcurl_interface_dlhandle, "curl_slist_append");
    libcurl.curl_slist_free_all = dlsym(libcurl_interface_dlhandle, "curl_slist_free_all");

    if (libcurl.curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
    {
        return -1;
    }

    ifstruct->transmit = http_interface_transmit;

    return 0;
}

EUICC_SHARED_EXPORT int libhttpinterface_main(int argc, char **argv)
{
    return 0;
}
