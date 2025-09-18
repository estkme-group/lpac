# cURL HTTP Backend

The cURL HTTP backend uses [libcurl](https://curl.se/libcurl/) to perform HTTP requests.

## Windows Specific Notes

Windows needs `libcurl.dll` to run.

Download libcurl from <https://curl.se/download.html> and place it as `libcurl.dll` aside `lpac.exe`.

## Environment Variables

- `http_proxy`: specify a proxy server for HTTP requests. \
  ([string](types.md#string-type), please read the <https://curl.se/libcurl/c/CURLOPT_PROXY.html>)
