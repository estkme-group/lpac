#include "es8p.h"

#include "base64.h"
#include "derutil.h"
#include "hexutil.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void es8p_metadata_access_rules_free(struct es8p_metadata_access_rule **access_rules) {
    struct es8p_metadata_access_rule *rule = *access_rules;

    while (rule) {
        struct es8p_metadata_access_rule *next = rule->next;
        free(rule->certificateHash);
        free(rule->packageName);
        free(rule);
        rule = next;
    }

    *access_rules = NULL;
}

static int es8p_metadata_parse_access_rules(struct es8p_metadata_access_rule **access_rules, const uint8_t *buffer,
                                            uint32_t buffer_len) {
    struct euicc_derutil_node n_ref_ar_do_entry = {0};
    struct es8p_metadata_access_rule *last = NULL;
    struct es8p_metadata_access_rule *rule = NULL;

    *access_rules = NULL;

    n_ref_ar_do_entry.self.ptr = buffer;
    n_ref_ar_do_entry.self.length = 0;

    // Each 0xBF76 tag may contain multiple REF-AR-DO (0xE2) entries
    while (euicc_derutil_unpack_next(&n_ref_ar_do_entry, &n_ref_ar_do_entry, buffer, buffer_len) == 0) {
        struct euicc_derutil_node n_ref_ar_do_child = {0};
        struct euicc_derutil_node n_ref_do_child = {0};
        struct euicc_derutil_node n_ref_do = {0};
        int found_ref_do = 0;

        if (n_ref_ar_do_entry.tag != 0xE2) {
            continue;
        }

        rule = calloc(1, sizeof(*rule));
        if (!rule) {
            goto err;
        }

        n_ref_ar_do_child.self.ptr = n_ref_ar_do_entry.value;
        n_ref_ar_do_child.self.length = 0;

        // Each REF-AR-DO must contain EXACTLY 1 REF-DO (0xE1)
        while (euicc_derutil_unpack_next(&n_ref_ar_do_child, &n_ref_ar_do_child, n_ref_ar_do_entry.value,
                                         n_ref_ar_do_entry.length)
               == 0) {
            if (n_ref_ar_do_child.tag == 0xE1) {
                if (found_ref_do) {
                    // We can't have multiple REF-DO's per REF-AR-DO
                    goto err;
                } else {
                    n_ref_do = n_ref_ar_do_child;
                    found_ref_do = 1;
                }
            }
        }

        // If we don't find REF-DO, abort
        if (!found_ref_do) {
            goto err;
        }

        n_ref_do_child.self.ptr = n_ref_do.value;
        n_ref_do_child.self.length = 0;

        // Now we have REF-DO, each MUST contain a 0xC1 (cert hash)
        // Optionally, a 0xCA (package name)
        while (euicc_derutil_unpack_next(&n_ref_do_child, &n_ref_do_child, n_ref_do.value, n_ref_do.length) == 0) {
            switch (n_ref_do_child.tag) {
            case 0xC1:
                // There can only be one 0xC1 (cert hash) per REF-DO
                if (rule->certificateHash) {
                    goto err;
                }

                rule->certificateHash = malloc((n_ref_do_child.length * 2) + 1);
                if (!rule->certificateHash) {
                    goto err;
                }

                if (euicc_hexutil_bin2hex(rule->certificateHash, (n_ref_do_child.length * 2) + 1, n_ref_do_child.value,
                                          n_ref_do_child.length)
                    < 0) {
                    goto err;
                }
                break;
            case 0xCA:
                // There can only be one 0xCA (package name)
                if (rule->packageName) {
                    goto err;
                }

                rule->packageName = malloc(n_ref_do_child.length + 1);
                if (!rule->packageName) {
                    goto err;
                }

                memcpy(rule->packageName, n_ref_do_child.value, n_ref_do_child.length);
                rule->packageName[n_ref_do_child.length] = '\0';
                break;
            }
        }

        // Each REF-DO child must at least contain a cert hash
        if (!rule->certificateHash) {
            goto err;
        }

        if (!*access_rules) {
            *access_rules = rule;
        } else {
            last->next = rule;
        }
        last = rule;
        rule = NULL;
    }

    return 0;

err:
    if (rule) {
        free(rule->certificateHash);
        free(rule->packageName);
        free(rule);
    }
    es8p_metadata_access_rules_free(access_rules);
    return -1;
}

int es8p_metadata_parse(struct es8p_metadata **stru_metadata, const char *b64_Metadata) {
    int ret;
    uint8_t *metadata = NULL;
    int metadata_len = 0;
    struct euicc_derutil_node n_metadata, n_iter;
    struct es8p_metadata *p = NULL;

    *stru_metadata = NULL;

    memset(&n_metadata, 0x00, sizeof(n_metadata));
    memset(&n_iter, 0x00, sizeof(n_iter));

    metadata = malloc(euicc_base64_decode_len(b64_Metadata));
    if (!metadata) {
        goto err;
    }

    if ((metadata_len = euicc_base64_decode(metadata, b64_Metadata)) < 0) {
        goto err;
    }

    if (euicc_derutil_unpack_find_tag(&n_metadata, 0xBF25, metadata, metadata_len) < 0) {
        goto err;
    }

    if (!(p = malloc(sizeof(struct es8p_metadata)))) {
        goto err;
    }

    memset(p, 0, sizeof(*p));

    n_iter.self.ptr = n_metadata.value;
    n_iter.self.length = 0;

    p->profileClass = ES10C_PROFILE_CLASS_NULL;
    p->iconType = ES10C_ICON_TYPE_NULL;

    while (euicc_derutil_unpack_next(&n_iter, &n_iter, n_metadata.value, n_metadata.length) == 0) {
        int tmplong;
        switch (n_iter.tag) {
        case 0x5A:
            euicc_hexutil_bin2gsmbcd(p->iccid, sizeof(p->iccid), n_iter.value, n_iter.length);
            break;
        case 0x91:
            p->serviceProviderName = malloc(n_iter.length + 1);
            if (p->serviceProviderName) {
                memcpy(p->serviceProviderName, n_iter.value, n_iter.length);
                p->serviceProviderName[n_iter.length] = '\0';
            }
            break;
        case 0x92:
            p->profileName = malloc(n_iter.length + 1);
            if (p->profileName) {
                memcpy(p->profileName, n_iter.value, n_iter.length);
                p->profileName[n_iter.length] = '\0';
            }
            break;
        case 0x93:
            tmplong = euicc_derutil_convert_bin2long(n_iter.value, n_iter.length);
            switch (tmplong) {
            case ES10C_ICON_TYPE_JPEG:
            case ES10C_ICON_TYPE_PNG:
                p->iconType = tmplong;
                break;
            default:
                p->iconType = ES10C_ICON_TYPE_UNDEFINED;
                break;
            }
            break;
        case 0x94:
            p->icon = malloc(euicc_base64_encode_len(n_iter.length));
            if (p->icon) {
                euicc_base64_encode(p->icon, n_iter.value, n_iter.length);
            }
            break;
        case 0x95:
            tmplong = euicc_derutil_convert_bin2long(n_iter.value, n_iter.length);
            switch (tmplong) {
            case ES10C_PROFILE_CLASS_TEST:
            case ES10C_PROFILE_CLASS_PROVISIONING:
            case ES10C_PROFILE_CLASS_OPERATIONAL:
                p->profileClass = tmplong;
                break;
            default:
                p->profileClass = ES10C_PROFILE_CLASS_UNDEFINED;
                break;
            }
            break;
        case 0xBF76:
            // Android's extension, AR-DO in profile metadata
            if (es8p_metadata_parse_access_rules(&p->accessRules, n_iter.value, n_iter.length) < 0) {
                goto err;
            }
            break;
        case 0xB6:
        case 0xB7:
        case 0x99:
            euicc_apdu_unhandled_tag_print(NULL, &n_iter); // Assuming logging is not needed here
            break;
        }
    }

    *stru_metadata = p;
    ret = 0;
    goto exit;

err:
    ret = -1;
    free(*stru_metadata);
    *stru_metadata = NULL;
    es8p_metadata_free(&p);
exit:
    free(metadata);
    metadata = NULL;

    return ret;
}

void es8p_metadata_free(struct es8p_metadata **stru_metadata) {
    struct es8p_metadata *p = *stru_metadata;

    if (p == NULL) {
        return;
    }

    free(p->serviceProviderName);
    free(p->profileName);
    free(p->icon);
    es8p_metadata_access_rules_free(&p->accessRules);
    free(p);

    *stru_metadata = NULL;
}
