#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <euicc/interface.h>
#include <euicc/euicc.h>
#include <cjson/cJSON.h>

#include "dlsym_interface.h"

static struct euicc_ctx ctx = {0};

static void jprint_error(const char *function, const char *detail)
{
    cJSON *jroot = NULL;
    cJSON *jpayload = NULL;
    char *jstr = NULL;

    if (detail == NULL)
    {
        detail = "";
    }

    jroot = cJSON_CreateObject();
    cJSON_AddStringToObject(jroot, "type", "lpa");
    jpayload = cJSON_CreateObject();
    cJSON_AddNumberToObject(jpayload, "code", -1);
    cJSON_AddStringToObject(jpayload, "message", function);
    cJSON_AddStringToObject(jpayload, "data", detail);
    cJSON_AddItemToObject(jroot, "payload", jpayload);

    jstr = cJSON_PrintUnformatted(jroot);
    cJSON_Delete(jroot);

    printf("%s\n", jstr);
    free(jstr);
}

static void jprint_success(cJSON *jdata)
{
    cJSON *jroot = NULL;
    cJSON *jpayload = NULL;
    char *jstr = NULL;

    jroot = cJSON_CreateObject();
    cJSON_AddStringToObject(jroot, "type", "lpa");
    jpayload = cJSON_CreateObject();
    cJSON_AddNumberToObject(jpayload, "code", 0);
    cJSON_AddStringToObject(jpayload, "message", "success");
    if (jdata)
    {
        cJSON_AddItemToObject(jpayload, "data", jdata);
    }
    else
    {
        cJSON_AddNullToObject(jpayload, "data");
    }
    cJSON_AddItemToObject(jroot, "payload", jpayload);

    jstr = cJSON_PrintUnformatted(jroot);
    cJSON_Delete(jroot);

    printf("%s\n", jstr);
    free(jstr);
}

static int entry_info(void)
{
    char *eid = NULL;
    char *default_smdp = NULL;
    char *default_smds = NULL;
    cJSON *jdata = NULL;

    if (es10c_get_eid(&ctx, &eid))
    {
        jprint_error("es10c_get_eid", NULL);
        return -1;
    }

    if (es10a_get_euicc_configured_addresses(&ctx, &default_smdp, &default_smds))
    {
        jprint_error("es10a_get_euicc_configured_addresses", NULL);
        return -1;
    }

    jdata = cJSON_CreateObject();
    cJSON_AddStringToObject(jdata, "eid", eid);
    cJSON_AddStringToObject(jdata, "default_smds", default_smds);
    cJSON_AddStringToObject(jdata, "default_smdp", default_smdp);
    jprint_success(jdata);

    free(eid);
    free(default_smdp);
    free(default_smds);

    return 0;
}

static int entry_profile_list(void)
{
    struct es10c_profile_info *profiles;
    int profiles_count;
    cJSON *jdata = NULL;

    if (es10c_get_profiles_info(&ctx, &profiles, &profiles_count))
    {
        jprint_error("es10c_get_profiles_info", NULL);
        return -1;
    }

    jdata = cJSON_CreateArray();
    for (int i = 0; i < profiles_count; i++)
    {
        cJSON *jprofile = NULL;

        jprofile = cJSON_CreateObject();
        cJSON_AddStringToObject(jprofile, "iccid", profiles[i].iccid);
        cJSON_AddStringToObject(jprofile, "isdpAid", profiles[i].isdpAid);
        cJSON_AddNumberToObject(jprofile, "profileState", profiles[i].profileState);
        cJSON_AddStringToObject(jprofile, "profileNickname", profiles[i].profileNickname);
        cJSON_AddStringToObject(jprofile, "serviceProviderName", profiles[i].serviceProviderName);
        cJSON_AddStringToObject(jprofile, "profileName", profiles[i].profileName);
        cJSON_AddNumberToObject(jprofile, "profileClass", profiles[i].profileClass);
        cJSON_AddItemToArray(jdata, jprofile);
    }
    es10c_profile_info_free_all(profiles, profiles_count);

    jprint_success(jdata);

    return 0;
}

static int entry_profile_rename(int argc, char **argv)
{
    int ret;
    const char *iccid;
    const char *new_name;

    if (argc < 4)
    {
        printf("Usage: %s profile rename <iccid> [new_name]\n", argv[0]);
        printf("\t[new_name]: optional\n");
        return -1;
    }

    iccid = argv[3];
    if (argc > 4)
    {
        new_name = argv[4];
    }
    else
    {
        new_name = "";
    }

    if ((ret = es10c_set_nickname(&ctx, iccid, new_name)))
    {
        const char *reason;
        switch (ret)
        {
        case 1:
            reason = "iccid not found";
            break;
        default:
            reason = "unknown";
            break;
        }
        jprint_error("es10c_set_nickname", reason);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}

static int entry_profile_enable(int argc, char **argv)
{
    int ret;
    int len;
    const char *param;
    const char *refreshFlag;

    if (argc < 4 || argc > 5)
    {
        printf("Usage: %s profile enable <iccid/aid>\n", argv[0]);
        return -1;
    }

    if (argc == 5)
    {
        int rf0;
        int rf1;
        rf0 = strcmp(argv[4], "0");
        rf1 = strcmp(argv[4], "1");
        if (rf0 == 0)
        {
            refreshFlag = "0";
        }
        else if (rf1 == 0)
        {
            refreshFlag = "1";
        }
        else
        {
            const char *reason;
            reason = "You commit a wrong refreshFlag type. the right param is 1 or 0.";
            jprint_error("es10c_enable_profile_syntax", reason);
            return -1;
        }
    }
    else
    {
        refreshFlag = "1";
    }

    param = argv[3];
    len = strlen(param);

    if (len == 19 || len == 20)
    {
        if ((ret = es10c_enable_profile_iccid(&ctx, param, refreshFlag)))
        {
            const char *reason;
            switch (ret)
            {
            case 1:
                reason = "iccid not found";
                break;
            case 2:
                reason = "profile not in disabled state";
                break;
            case 3:
                reason = "disallowed by policy";
                break;
            case 4:
                reason = "wrong profile reenabling";
                break;
            default:
                reason = "unknown";
                break;
            }
            jprint_error("es10c_enable_profile_iccid", reason);
            return -1;
        }
    }
    else if (len == 32)
    {
        if ((ret = es10c_enable_profile_aid(&ctx, param, refreshFlag)))
        {
            const char *reason;
            switch (ret)
            {
            case 1:
                reason = "aid not found";
                break;
            case 2:
                reason = "profile not in disabled state";
                break;
            case 3:
                reason = "disallowed by policy";
                break;
            case 4:
                reason = "wrong profile reenabling";
                break;
            default:
                reason = "unknown";
                break;
            }
            jprint_error("es10c_enable_profile_aid", reason);
            return -1;
        }
    }
    else
    {
        const char *reason;
        reason = "You commit a wrong param. the right param is iccid or Aid.";
        jprint_error("es10c_enable_profile_syntax", reason);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}

static int entry_profile_disable(int argc, char **argv)
{
    int ret;
    int len;
    const char *refreshFlag;
    const char *param;

    if (argc < 4 || argc > 5)
    {
        printf("Usage: %s profile disable <iccid/aid>\n", argv[0]);
        return -1;
    }

    if (argc == 5)
    {
        int rf0;
        int rf1;
        rf0 = strcmp(argv[4], "0");
        rf1 = strcmp(argv[4], "1");
        if (rf0 == 0)
        {
            refreshFlag = "0";
        }
        else if (rf1 == 0)
        {
            refreshFlag = "1";
        }
        else
        {
            const char *reason;
            reason = "You commit a wrong refreshFlag type. the right param is 1 or 0.";
            jprint_error("es10c_enable_profile_syntax", reason);
            return -1;
        }
    }
    else
    {
        refreshFlag = "1";
    }

    param = argv[3];
    len = strlen(param);

    if (len == 19 || len == 20)
    {
        if ((ret = es10c_disable_profile_iccid(&ctx, param, refreshFlag)))
        {
            const char *reason;
            switch (ret)
            {
            case 1:
                reason = "iccid not found";
                break;
            case 2:
                reason = "profile not in enabled state";
                break;
            case 3:
                reason = "disallowed by policy";
                break;
            default:
                reason = "unknown";
                break;
            }
            jprint_error("es10c_disable_profile_iccid", reason);
            return -1;
        }
    }
    else if (len == 32)
    {
        if ((ret = es10c_disable_profile_aid(&ctx, param, refreshFlag)))
        {
            const char *reason;
            switch (ret)
            {
            case 1:
                reason = "aid not found";
                break;
            case 2:
                reason = "profile not in enabled state";
                break;
            case 3:
                reason = "disallowed by policy";
                break;
            default:
                reason = "unknown";
                break;
            }
            jprint_error("es10c_disable_profile_aid", reason);
            return -1;
        }
    }
    else
    {
        const char *reason;
        reason = "You commit a wrong param. the right param is iccid or Aid.";
        jprint_error("es10c_disable_profile_syntax", reason);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}

static int entry_profile_delete(int argc, char **argv)
{
    int ret;
    int len;
    const char *param;

    if (argc < 4)
    {
        printf("Usage: %s profile delete <iccid/aid>\n", argv[0]);
        return -1;
    }

    param = argv[3];
    len = strlen(param);

    if (len == 19 || len == 20)
    {
        if ((ret = es10c_delete_profile_iccid(&ctx, param)))
        {
            const char *reason;
            switch (ret)
            {
            case 1:
                reason = "iccid not found";
                break;
            case 2:
                reason = "profile not in disabled state";
                break;
            case 3:
                reason = "disallowed by policy";
                break;
            default:
                reason = "unknown";
                break;
            }
            jprint_error("es10c_delete_profile_iccid", reason);
            return -1;
        }
    }
    else if (len == 32)
    {
        if ((ret = es10c_delete_profile_aid(&ctx, param)))
        {
            const char *reason;
            switch (ret)
            {
            case 1:
                reason = "aid not found";
                break;
            case 2:
                reason = "profile not in disabled state";
                break;
            case 3:
                reason = "disallowed by policy";
                break;
            default:
                reason = "unknown";
                break;
            }
            jprint_error("es10c_delete_profile_aid", reason);
            return -1;
        }
    }
    else
    {
        const char *reason;
        reason = "You commit a wrong param. the right param is iccid or Aid.";
        jprint_error("es10c_delete_profile_syntax", reason);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}

static int entry_profile(int argc, char **argv)
{
    int ret = 0;

    if (argc < 3)
    {
        printf("Usage: %s profile <command> [options]\n", argv[0]);
        printf("\t<command>: [list|rename|enable|disable|delete]\n");
        return -1;
    }

    if (strcmp(argv[2], "list") == 0)
    {
        ret = entry_profile_list();
    }
    else if (strcmp(argv[2], "rename") == 0)
    {
        ret = entry_profile_rename(argc, argv);
    }
    else if (strcmp(argv[2], "enable") == 0)
    {
        ret = entry_profile_enable(argc, argv);
    }
    else if (strcmp(argv[2], "disable") == 0)
    {
        ret = entry_profile_disable(argc, argv);
    }
    else if (strcmp(argv[2], "delete") == 0)
    {
        ret = entry_profile_delete(argc, argv);
    }
    else
    {
        printf("Unknown command: %s\n", argv[2]);
        return -1;
    }
    return ret;
}

static int entry_notification_list(void)
{
    struct es10b_notification_metadata *notifications;
    int notifications_count;
    cJSON *jdata = NULL;

    if (es10b_list_notification(&ctx, &notifications, &notifications_count))
    {
        jprint_error("es10b_list_notification", NULL);
        return -1;
    }

    jdata = cJSON_CreateArray();
    for (int i = 0; i < notifications_count; i++)
    {
        cJSON *jnotification = NULL;

        jnotification = cJSON_CreateObject();
        cJSON_AddNumberToObject(jnotification, "seqNumber", notifications[i].seqNumber);
        cJSON_AddNumberToObject(jnotification, "profileManagementOperation", notifications[i].profileManagementOperation);
        cJSON_AddStringToObject(jnotification, "notificationAddress", notifications[i].notificationAddress);
        cJSON_AddStringToObject(jnotification, "iccid", notifications[i].iccid);

        cJSON_AddItemToArray(jdata, jnotification);
    }

    es10b_notification_metadata_free_all(notifications, notifications_count);

    jprint_success(jdata);

    return 0;
}

static int entry_notification_process(int argc, char **argv)
{
    unsigned long seqNumber;
    char *b64payload = NULL;
    char *receiver = NULL;

    if (argc < 4)
    {
        printf("Usage: %s notification process <seqNumber>\n", argv[0]);
        return -1;
    }

    seqNumber = atol(argv[3]);

    if (es10b_retrieve_notification(&ctx, &b64payload, &receiver, seqNumber))
    {
        jprint_error("es10b_retrieve_notification", NULL);
        return -1;
    }

    if (es9p_handle_notification(&ctx, receiver, b64payload))
    {
        jprint_error("es9p_handle_notification", NULL);
        return -1;
    }

    free(b64payload);
    free(receiver);

    jprint_success(NULL);

    return 0;
}

static int entry_notification_remove(int argc, char **argv)
{
    int ret;
    unsigned long seqNumber;

    if (argc < 4)
    {
        printf("Usage: %s notification remove <seqNumber>\n", argv[0]);
        return -1;
    }

    seqNumber = atol(argv[3]);

    if ((ret = es10b_remove_notification_from_list(&ctx, seqNumber)))
    {
        const char *reason;
        switch (ret)
        {
        case 1:
            reason = "seqNumber not found";
            break;
        default:
            reason = "unknown";
            break;
        }
        jprint_error("es10b_remove_notification_from_list", reason);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}

static int entry_notification(int argc, char **argv)
{
    int ret = 0;

    if (argc < 3)
    {
        printf("Usage: %s notification <command> [options]\n", argv[0]);
        printf("\t<command>: [list|process|remove]\n");
        return -1;
    }

    if (strcmp(argv[2], "list") == 0)
    {
        ret = entry_notification_list();
    }
    else if (strcmp(argv[2], "process") == 0)
    {
        ret = entry_notification_process(argc, argv);
    }
    else if (strcmp(argv[2], "remove") == 0)
    {
        ret = entry_notification_remove(argc, argv);
    }
    else
    {
        printf("Unknown command: %s\n", argv[2]);
        return -1;
    }

    return ret;
}

static int entry_download(int argc, char **argv)
{
    char *transaction_id = NULL;

    char *smdp = NULL;
    char *matchingId = NULL;
    char *imei = NULL;
    char *confirmation_code = NULL;

    char *b64_euicc_challenge = NULL;
    char *b64_euicc_info_1 = NULL;
    struct es9p_initiate_authentication_resp es9p_initiate_authentication_resp;

    struct es10b_authenticate_server_param es10b_authenticate_server_param;
    char *b64_authenticate_server_response = NULL;

    struct es9p_authenticate_client_resp es9p_authenticate_client_resp;

    struct es10b_prepare_download_param es10b_prepare_download_param;
    char *b64_prepare_download_response = NULL;

    struct es9p_get_bound_profile_package_resp es9p_get_bound_profile_package_resp;

    smdp = getenv("SMDP");
    matchingId = getenv("MATCHINGID");
    imei = getenv("IMEI");
    confirmation_code = getenv("CONFIRMATION_CODE");

    if (smdp == NULL)
    {
        if (es10a_get_euicc_configured_addresses(&ctx, &smdp, NULL))
        {
            jprint_error("es10a_get_euicc_configured_addresses", NULL);
            return -1;
        }
    }

    if (!smdp || (strlen(smdp) == 0))
    {
        jprint_error("smdp is null", NULL);
        return -1;
    }

    if (es10b_get_euicc_challenge(&ctx, &b64_euicc_challenge))
    {
        jprint_error("es10b_get_euicc_challenge", NULL);
        return -1;
    }

    if (es10b_get_euicc_info(&ctx, &b64_euicc_info_1))
    {
        jprint_error("es10b_get_euicc_info", NULL);
        return -1;
    }

    if (es9p_initiate_authentication(&ctx, smdp, b64_euicc_challenge, b64_euicc_info_1, &es9p_initiate_authentication_resp))
    {
        jprint_error("es9p_initiate_authentication", es9p_initiate_authentication_resp.status);
        return -1;
    }

    transaction_id = strdup(es9p_initiate_authentication_resp.transaction_id);
    es10b_authenticate_server_param.b64_server_signed_1 = es9p_initiate_authentication_resp.b64_server_signed_1;
    es10b_authenticate_server_param.b64_server_signature_1 = es9p_initiate_authentication_resp.b64_server_signature_1;
    es10b_authenticate_server_param.b64_euicc_ci_pkid_to_be_used = es9p_initiate_authentication_resp.b64_euicc_ci_pkid_to_be_used;
    es10b_authenticate_server_param.b64_server_certificate = es9p_initiate_authentication_resp.b64_server_certificate;
    es10b_authenticate_server_param.matchingId = matchingId;
    es10b_authenticate_server_param.imei = imei;
    es10b_authenticate_server_param.tac = NULL;

    if (es10b_authenticate_server(&ctx, &b64_authenticate_server_response, &es10b_authenticate_server_param))
    {
        jprint_error("es10b_authenticate_server", NULL);
        return -1;
    }

    if (es9p_authenticate_client(&ctx, smdp, transaction_id, b64_authenticate_server_response, &es9p_authenticate_client_resp))
    {
        jprint_error("es9p_authenticate_client", es9p_authenticate_client_resp.status);
        return -1;
    }

    es10b_prepare_download_param.b64_smdp_signed_2 = es9p_authenticate_client_resp.b64_smdp_signed_2;
    es10b_prepare_download_param.b64_smdp_signature_2 = es9p_authenticate_client_resp.b64_smdp_signature_2;
    es10b_prepare_download_param.b64_smdp_certificate = es9p_authenticate_client_resp.b64_smdp_certificate;
    if (confirmation_code)
    {
        es10b_prepare_download_param.str_checkcode = confirmation_code;
        es10b_prepare_download_param.hexstr_transcation_id = transaction_id;
    }
    else
    {
        es10b_prepare_download_param.str_checkcode = NULL;
    }

    if (es10b_prepare_download(&ctx, &b64_prepare_download_response, &es10b_prepare_download_param))
    {
        jprint_error("es10b_prepare_download", NULL);
        return -1;
    }

    if (es9p_get_bound_profile_package(&ctx, smdp, transaction_id, b64_prepare_download_response, &es9p_get_bound_profile_package_resp))
    {
        jprint_error("es9p_get_bound_profile_package", es9p_get_bound_profile_package_resp.status);
        return -1;
    }

    if (es10b_load_bound_profile_package(&ctx, es9p_get_bound_profile_package_resp.b64_bpp))
    {
        jprint_error("es10b_load_bound_profile_package", NULL);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}

static int entry_defaultsmdp(int argc, char **argv)
{
    const char *smdp;

    if (argc < 3)
    {
        printf("Usage: %s defaultsmdp <smdp>\n", argv[0]);
        return -1;
    }

    smdp = argv[2];

    if (es10a_set_default_dp_address(&ctx, smdp))
    {
        jprint_error("es10a_set_default_dp_address", NULL);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}

static int entry_purge(int argc, char **argv)
{
    int ret;

    if (argc < 3)
    {
        printf("Usage: %s purge <confirm>\n", argv[0]);
        printf("\t<confirm>: [yes|other]\n");
        printf("\t\tConfirm purge eUICC, all data will lost!\n");
        return -1;
    }

    if (strcmp(argv[2], "yes") != 0)
    {
        printf("Purge canceled\n");
        return -1;
    }

    if ((ret = es10c_euicc_memory_reset(&ctx, 1, 1, 1)))
    {
        const char *reason;
        switch (ret)
        {
        case 1:
            reason = "nothing to delete";
            break;
        default:
            reason = "unknown";
            break;
        }
        jprint_error("es10c_euicc_memory_reset", reason);
        return -1;
    }

    jprint_success(NULL);

    return 0;
}

static int entry_discovery(void)
{
    char *transaction_id = NULL;

    char *smds = NULL;
    char *imei = NULL;

    char *b64_euicc_challenge = NULL;
    char *b64_euicc_info_1 = NULL;
    struct es9p_initiate_authentication_resp es11_initiate_authentication_resp;

    struct es10b_authenticate_server_param es10b_authenticate_server_param;
    char *b64_authenticate_server_response = NULL;

    struct es11_authenticate_client_resp es11_authenticate_client_resp;

    imei = getenv("IMEI");

    smds = getenv("SMDS");
    if (smds == NULL)
    {
        // smds = "prod.smds.rsp.goog";
        // smds = "lpa.live.esimdiscovery.com";
        smds = "lpa.ds.gsma.com";
    }

    if (es10b_get_euicc_challenge(&ctx, &b64_euicc_challenge))
    {
        jprint_error("es10b_get_euicc_challenge", NULL);
        return -1;
    }

    if (es10b_get_euicc_info(&ctx, &b64_euicc_info_1))
    {
        jprint_error("es10b_get_euicc_info", NULL);
        return -1;
    }

    if (es9p_initiate_authentication(&ctx, smds, b64_euicc_challenge, b64_euicc_info_1, &es11_initiate_authentication_resp))
    {
        jprint_error("es11_initiate_authentication", es11_initiate_authentication_resp.status);
        return -1;
    }

    transaction_id = strdup(es11_initiate_authentication_resp.transaction_id);

    es10b_authenticate_server_param.b64_server_signed_1 = es11_initiate_authentication_resp.b64_server_signed_1;
    es10b_authenticate_server_param.b64_server_signature_1 = es11_initiate_authentication_resp.b64_server_signature_1;
    es10b_authenticate_server_param.b64_euicc_ci_pkid_to_be_used = es11_initiate_authentication_resp.b64_euicc_ci_pkid_to_be_used;
    es10b_authenticate_server_param.b64_server_certificate = es11_initiate_authentication_resp.b64_server_certificate;
    es10b_authenticate_server_param.matchingId = NULL;
    es10b_authenticate_server_param.imei = imei;
    es10b_authenticate_server_param.tac = NULL;

    if (es10b_authenticate_server(&ctx, &b64_authenticate_server_response, &es10b_authenticate_server_param))
    {
        jprint_error("es10b_authenticate_server", NULL);
        return -1;
    }

    if (es11_authenticate_client(&ctx, smds, transaction_id, b64_authenticate_server_response, &es11_authenticate_client_resp))
    {
        jprint_error("es11_authenticate_client", es11_authenticate_client_resp.status);
        return -1;
    }

    jprint_success((cJSON *)es11_authenticate_client_resp.cjson_array_result);
    return 0;
}

int main(int argc, char **argv)
{
    int ret = 0;

    memset(&ctx, 0, sizeof(ctx));

    if (dlsym_interface_init())
    {
        return -1;
    }

    ctx.interface.apdu = &dlsym_apdu_interface;
    ctx.interface.http = &dlsym_http_interface;

    if (es10x_init(&ctx))
    {
        jprint_error("es10x_init", NULL);
        return -1;
    }

    if (argc < 2)
    {
        printf("Usage: %s <command> [options]\n", argv[0]);
        printf("\t<command>: [info|profile|notification|download|defaultsmdp|purge|discovery]\n");
        return -1;
    }
    if (strcmp(argv[1], "info") == 0)
    {
        ret = entry_info();
    }
    else if (strcmp(argv[1], "profile") == 0)
    {
        ret = entry_profile(argc, argv);
    }
    else if (strcmp(argv[1], "notification") == 0)
    {
        ret = entry_notification(argc, argv);
    }
    else if (strcmp(argv[1], "download") == 0)
    {
        ret = entry_download(argc, argv);
    }
    else if (strcmp(argv[1], "defaultsmdp") == 0)
    {
        ret = entry_defaultsmdp(argc, argv);
    }
    else if (strcmp(argv[1], "purge") == 0)
    {
        ret = entry_purge(argc, argv);
    }
    else if (strcmp(argv[1], "discovery") == 0)
    {
        ret = entry_discovery();
    }
    else
    {
        printf("Unknown command: %s\n", argv[1]);
        return -1;
    }

    es10x_fini(&ctx);

    return ret;
}
