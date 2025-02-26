#ifdef _WIN32
#include "pcsc_win32.h"

char *pcsc_stringify_error(const int32_t err) {
  switch (err) {
    case SCARD_S_SUCCESS: return "SCARD_S_SUCCESS";
    case SCARD_F_INTERNAL_ERROR: return "SCARD_F_INTERNAL_ERROR";
    case SCARD_E_CANCELLED: return "SCARD_E_CANCELLED";
    case SCARD_E_INVALID_HANDLE: return "SCARD_E_INVALID_HANDLE";
    case SCARD_E_INVALID_PARAMETER: return "SCARD_E_INVALID_PARAMETER";
    case SCARD_E_INVALID_TARGET: return "SCARD_E_INVALID_TARGET";
    case SCARD_E_NO_MEMORY: return "SCARD_E_NO_MEMORY";
    case SCARD_F_WAITED_TOO_LONG: return "SCARD_F_WAITED_TOO_LONG";
    case SCARD_E_INSUFFICIENT_BUFFER: return "SCARD_E_INSUFFICIENT_BUFFER";
    case SCARD_E_UNKNOWN_READER: return "SCARD_E_UNKNOWN_READER";
    case SCARD_E_TIMEOUT: return "SCARD_E_TIMEOUT";
    case SCARD_E_SHARING_VIOLATION: return "SCARD_E_SHARING_VIOLATION";
    case SCARD_E_NO_SMARTCARD: return "SCARD_E_NO_SMARTCARD";
    case SCARD_E_UNKNOWN_CARD: return "SCARD_E_UNKNOWN_CARD";
    case SCARD_E_CANT_DISPOSE: return "SCARD_E_CANT_DISPOSE";
    case SCARD_E_PROTO_MISMATCH: return "SCARD_E_PROTO_MISMATCH";
    case SCARD_E_NOT_READY: return "SCARD_E_NOT_READY";
    case SCARD_E_INVALID_VALUE: return "SCARD_E_INVALID_VALUE";
    case SCARD_E_SYSTEM_CANCELLED: return "SCARD_E_SYSTEM_CANCELLED";
    case SCARD_F_COMM_ERROR: return "SCARD_F_COMM_ERROR";
    case SCARD_F_UNKNOWN_ERROR: return "SCARD_F_UNKNOWN_ERROR";
    case SCARD_E_INVALID_ATR: return "SCARD_E_INVALID_ATR";
    case SCARD_E_NOT_TRANSACTED: return "SCARD_E_NOT_TRANSACTED";
    case SCARD_E_READER_UNAVAILABLE: return "SCARD_E_READER_UNAVAILABLE";
    case SCARD_P_SHUTDOWN: return "SCARD_P_SHUTDOWN";
    case SCARD_E_PCI_TOO_SMALL: return "SCARD_E_PCI_TOO_SMALL";
    case SCARD_E_READER_UNSUPPORTED: return "SCARD_E_READER_UNSUPPORTED";
    case SCARD_E_DUPLICATE_READER: return "SCARD_E_DUPLICATE_READER";
    case SCARD_E_CARD_UNSUPPORTED: return "SCARD_E_CARD_UNSUPPORTED";
    case SCARD_E_NO_SERVICE: return "SCARD_E_NO_SERVICE";
    case SCARD_E_SERVICE_STOPPED: return "SCARD_E_SERVICE_STOPPED";
    case SCARD_E_UNEXPECTED: return "SCARD_E_UNEXPECTED";
    case SCARD_E_ICC_INSTALLATION: return "SCARD_E_ICC_INSTALLATION";
    case SCARD_E_ICC_CREATEORDER: return "SCARD_E_ICC_CREATEORDER";
    case SCARD_E_DIR_NOT_FOUND: return "SCARD_E_DIR_NOT_FOUND";
    case SCARD_E_FILE_NOT_FOUND: return "SCARD_E_FILE_NOT_FOUND";
    case SCARD_E_NO_DIR: return "SCARD_E_NO_DIR";
    case SCARD_E_NO_FILE: return "SCARD_E_NO_FILE";
    case SCARD_E_NO_ACCESS: return "SCARD_E_NO_ACCESS";
    case SCARD_E_WRITE_TOO_MANY: return "SCARD_E_WRITE_TOO_MANY";
    case SCARD_E_BAD_SEEK: return "SCARD_E_BAD_SEEK";
    case SCARD_E_INVALID_CHV: return "SCARD_E_INVALID_CHV";
    case SCARD_E_UNKNOWN_RES_MNG: return "SCARD_E_UNKNOWN_RES_MNG";
    case SCARD_E_NO_SUCH_CERTIFICATE: return "SCARD_E_NO_SUCH_CERTIFICATE";
    case SCARD_E_CERTIFICATE_UNAVAILABLE: return "SCARD_E_CERTIFICATE_UNAVAILABLE";
    case SCARD_E_NO_READERS_AVAILABLE: return "SCARD_E_NO_READERS_AVAILABLE";
    case SCARD_E_COMM_DATA_LOST: return "SCARD_E_COMM_DATA_LOST";
    case SCARD_E_NO_KEY_CONTAINER: return "SCARD_E_NO_KEY_CONTAINER";
    case SCARD_E_SERVER_TOO_BUSY: return "SCARD_E_SERVER_TOO_BUSY";
    case SCARD_W_UNSUPPORTED_CARD: return "SCARD_W_UNSUPPORTED_CARD";
    case SCARD_W_UNRESPONSIVE_CARD: return "SCARD_W_UNRESPONSIVE_CARD";
    case SCARD_W_UNPOWERED_CARD: return "SCARD_W_UNPOWERED_CARD";
    case SCARD_W_RESET_CARD: return "SCARD_W_RESET_CARD";
    case SCARD_W_REMOVED_CARD: return "SCARD_W_REMOVED_CARD";
    case SCARD_W_SECURITY_VIOLATION: return "SCARD_W_SECURITY_VIOLATION";
    case SCARD_W_WRONG_CHV: return "SCARD_W_WRONG_CHV";
    case SCARD_W_CHV_BLOCKED: return "SCARD_W_CHV_BLOCKED";
    case SCARD_W_EOF: return "SCARD_W_EOF";
    case SCARD_W_CANCELLED_BY_USER: return "SCARD_W_CANCELLED_BY_USER";
    case SCARD_W_CARD_NOT_AUTHENTICATED: return "SCARD_W_CARD_NOT_AUTHENTICATED";
    default: return "Unknown error";
  }
}
#endif
