#include <string.h>
#include "es9p_errors.h"

struct es9p_error {
    const char *subject_code;
    const char *reason_code;
    const char *description;
};

static const struct es9p_error es9p_errors[] = {
    {"8.1",    "4.8", "eUICC does not have sufficient space for this Profile"},
    {"8.1",    "6.1", "eUICC signature is invalid or serverChallenge is invalid"},
    {"8.1.1",  "2.2", "Indicates that the EID is missing in the context of this order (SM-DS address provided or MatchingID value is empty)"},
    {"8.1.1",  "3.1", "Indicates that a different EID is already associated with this ICCID"},
    {"8.1.1",  "3.8", "EID doesn't match the expected value"},
    {"8.1.2",  "6.1", "EUM Certificate is invalid"},
    {"8.1.2",  "6.3", "EUM Certificate has expired"},
    {"8.1.3",  "6.1", "eUICC Certificate is invalid"},
    {"8.1.3",  "6.3", "eUICC Certificate has expired"},
    {"8.2",    "1.2", "Profile has not yet been released"},
    {"8.2",    "3.7", "BPP is not available for a new binding"},
    {"8.2.1",  "1.2", "Indicates that the function caller is not allowed to perform this function on the target Profile"},
    {"8.2.1",  "3.1", "Indicates that a different EID is associated with this ICCID"},
    {"8.2.1",  "3.3", "Indicates that the Profile identified by the provided ICCID is not available"},
    {"8.2.1",  "3.5", "Indicates that the target Profile cannot be released"},
    {"8.2.1",  "3.9", "Indicates that the Profile Type identified by this Profile Type is unknown to the SM-DP+"},
    {"8.2.5",  "1.2", "Indicates that the function caller is not allowed to perform this function on the Profile Type"},
    {"8.2.5",  "3.7", "No more Profile available for the requested Profile Type"},
    {"8.2.5",  "3.8", "Indicates that the Profile Type identified by this Profile Type is not aligned with the Profile Type of Profile identified by the ICCID"},
    {"8.2.5",  "3.9", "Indicates that the Profile Type identified by this Profile Type is unknown to the SM-DP+"},
    {"8.2.5",  "4.3", "No eligible Profile for this eUICC/Device"},
    {"8.2.6",  "3.1", "Indicates that a different MatchingID is associated with this ICCID"},
    {"8.2.6",  "3.3", "Conflicting MatchingID value"},
    {"8.2.6",  "3.8", "MatchingID (AC_Token or EventID) is refused"},
    {"8.2.7",  "2.2", "Confirmation Code is missing"},
    {"8.2.7",  "3.8", "Confirmation Code is refused"},
    {"8.2.7",  "6.4", "The maximum number of retries for the Confirmation Code has been exceeded"},
    {"8.8",    "3.1", "The provided SM-DP+ OID is invalid"},
    {"8.8.1",  "3.8", "Invalid SM-DP+ Address"},
    {"8.8.2",  "3.1", "None of the proposed Public Key Identifiers is supported by the SM-DP+"},
    {"8.8.3",  "3.1", "The Specification Version Number indicated by the eUICC is not supported by the SM-DP+"},
    {"8.8.4",  "3.7", "The SM-DP+ has no CERT.DPauth.ECDSA signed by one of the CI Public Key supported by the eUICC"},
    {"8.8.5",  "4.1", "The Download order has expired"},
    {"8.8.5",  "6.4", "The maximum number of retries for the Profile download order has been exceeded"},
    {"8.9",    "4.2", "Root SM-DS has raised an error"},
    {"8.9",    "5.1", "Root SM-DS was unavailable"},
    {"8.9.1",  "3.8", "Invalid SM-DS Address"},
    {"8.9.2",  "3.1", "None of the proposed Public Key Identifiers is supported by the SM-DS"},
    {"8.9.3",  "3.1", "The Specification Version Number indicated by the eUICC is not supported by the SM-DS"},
    {"8.9.4",  "3.7", "The SM-DS has no CERT.DS.ECDSA signed by one of the GSMA CI Public Key supported by the eUICC"},
    {"8.9.5",  "3.3", "The Event Record already exist in the SM-DS (EventID duplicated)"},
    {"8.9.5",  "3.9", "No Event identified by the Event ID for the EID exists"},
    {"8.10.1", "3.9", "The RSP session identified by the TransactionID is unknown"},
    {"8.11.1", "3.9", "Unknown CI Public Key. The CI used by the EUM Certificate is not a trusted root."},
};

const char *es9p_error_message(const char *subject_code, const char *reason_code)
{
    struct es9p_error error;
    for (int i = 0; i < sizeof(es9p_errors) / sizeof(es9p_errors[0]); i++)
    {
        error = es9p_errors[i];
        if (strcmp(error.subject_code, subject_code) == 0 && strcmp(error.reason_code, reason_code) == 0)
        {
            return error.description;
        }
    }
    return NULL;
}
