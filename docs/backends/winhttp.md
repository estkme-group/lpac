# WinHTTP HTTP Backend

This backend uses [WinHTTP](https://learn.microsoft.com/en-us/windows/win32/winhttp/about-winhttp) to access HTTPS
resources.

## Environment Variables

No backend-specific environment variables are used by this backend.

For TLS verification, see `LPAC_HTTP_SSL_VERIFY` and `LPAC_HTTP_TRUST_STORE` in
[Environment Variables](../ENVVARS.md). If a trust store is set, HTTP redirects are not followed.
