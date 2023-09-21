#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

#include <euicc/interface.h>
#include <euicc/euicc.h>
#include <cjson/cJSON.h>

#include "dlsym_interface.h"
// LPA:1$vie-gto.prod.ondemandconnectivity.com$6F279578FC7A4AD6CC2CF13D1102CAABAE397E08CF86FAB25293C63467420FFF

static uint8_t output_as_json = 0;
static uint8_t entry_inited = 0;
static struct euicc_ctx ctx = {0};

static int entry_init(void)
{
    if (entry_inited)
        return 0;

    if (es10x_init(&ctx))
    {
        if (output_as_json)
        {
            printf("{\"return\":-1,\"message\":\"es10x_init failed\",\"data\":null}\n");
        }
        else
        {
            printf("es10x_init failed\n");
        }
        es10x_fini(&ctx);
        return -1;
    }

    entry_inited = 1;
    return 0;
}

static void entry_fini(void)
{
    if (!entry_inited)
        return;
    es10x_fini(&ctx);
    entry_inited = 0;
}

static int entry_info(void)
{
    char *eid;
    char *default_smdp;
    char *default_smds;

    if (entry_init())
        return -1;

    if (es10c_get_eid(&ctx, &eid))
    {
        if (!output_as_json)
        {
            printf("es10c_get_eid failed\n");
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10c_get_eid\",\"data\":null}\n");
        }
        return -1;
    }

    if (es10a_get_euicc_configured_addresses(&ctx, &default_smdp, &default_smds))
    {
        if (!output_as_json)
        {
            printf("es10a_get_euicc_configured_addresses failed\n");
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10a_get_euicc_configured_addresses\",\"data\":null}\n");
        }
        return -1;
    }

    if (!output_as_json)
    {
        printf("eid: %s\n", eid);
        printf("default_smds: %s\n", default_smds);
        printf("default_smdp: %s\n", default_smdp);
    }
    else
    {
        cJSON *jroot = NULL;
        cJSON *jdata = NULL;
        char *jstr = NULL;

        jroot = cJSON_CreateObject();
        cJSON_AddNumberToObject(jroot, "return", 0);
        cJSON_AddStringToObject(jroot, "message", "success");
        jdata = cJSON_CreateObject();
        cJSON_AddStringToObject(jdata, "eid", eid);
        cJSON_AddStringToObject(jdata, "default_smds", default_smds);
        cJSON_AddStringToObject(jdata, "default_smdp", default_smdp);
        cJSON_AddItemToObject(jroot, "data", jdata);

        jstr = cJSON_PrintUnformatted(jroot);
        printf("%s\n", jstr);

        cJSON_Delete(jroot);
        free(jstr);
    }

    free(eid);

    return 0;
}

static int entry_profile_list(void)
{
    struct es10c_profile_info *profiles;
    int profiles_count;

    if (entry_init())
        return -1;

    if (es10c_get_profiles_info(&ctx, &profiles, &profiles_count))
    {
        if (!output_as_json)
        {
            printf("es10c_get_profiles_info failed\n");
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10c_get_profiles_info\",\"data\":null}\n");
        }
        return -1;
    }

    if (!output_as_json)
    {
        printf("profile count: %d\n", profiles_count);

        for (int i = 0; i < profiles_count; i++)
        {
            struct es10c_profile_info *profile = &profiles[i];
            printf("profile %d:\n", i);
            es10c_profile_info_print(profile);
        }
    }
    else
    {
        cJSON *jroot = NULL;
        cJSON *jdata = NULL;

        char *jstr = NULL;

        jroot = cJSON_CreateObject();
        cJSON_AddNumberToObject(jroot, "return", 0);
        cJSON_AddStringToObject(jroot, "message", "success");
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
        cJSON_AddItemToObject(jroot, "data", jdata);

        jstr = cJSON_PrintUnformatted(jroot);
        printf("%s\n", jstr);

        cJSON_Delete(jroot);
        free(jstr);
    }

    es10c_profile_info_free_all(profiles, profiles_count);

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

    if (entry_init())
        return -1;

    if ((ret = es10c_set_nickname(&ctx, iccid, new_name)))
    {
        const char *reason;
        switch (ret)
        {
        case 1:
            reason = "iccid not found";
            break;
        default:
            reason = "unknown error";
            break;
        }
        if (!output_as_json)
        {
            printf("es10c_set_nickname failed\n");
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10c_set_nickname\",\"data\":null}\n");
        }
        return -1;
    }

    if (!output_as_json)
    {
        printf("rename profile success\n");
    }
    else
    {
        printf("{\"return\":0,\"message\":\"success\",\"data\":null}\n");
    }

    return 0;
}

static int entry_profile_enable(int argc, char **argv)
{
    int ret;
    const char *iccid;

    if (argc < 4)
    {
        printf("Usage: %s profile enable <iccid>\n", argv[0]);
        return -1;
    }

    iccid = argv[3];

    if (entry_init())
        return -1;

    if ((ret = es10c_enable_profile_iccid(&ctx, iccid)))
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
            reason = "unknown error";
            break;
        }
        if (!output_as_json)
        {
            printf("es10c_enable_profile_iccid failed: %s\n", reason);
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10c_enable_profile_iccid\",\"data\":\"%s\"}\n", reason);
        }
        return -1;
    }

    if (!output_as_json)
    {
        printf("enable profile success\n");
    }
    else
    {
        printf("{\"return\":0,\"message\":\"success\",\"data\":null}\n");
    }

    return 0;
}

static int entry_profile_disable(int argc, char **argv)
{
    int ret;
    const char *iccid;

    if (argc < 4)
    {
        printf("Usage: %s profile disable <iccid>\n", argv[0]);
        return -1;
    }

    iccid = argv[3];

    if (entry_init())
        return -1;

    if ((ret = es10c_disable_profile_iccid(&ctx, iccid)))
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
            reason = "unknown error";
            break;
        }
        if (!output_as_json)
        {
            printf("es10c_disable_profile_iccid failed: %s\n", reason);
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10c_disable_profile_iccid\",\"data\":\"%s\"}\n", reason);
        }
        return -1;
    }

    if (!output_as_json)
    {
        printf("disable profile success\n");
    }
    else
    {
        printf("{\"return\":0,\"message\":\"success\",\"data\":null}\n");
    }

    return 0;
}

static int entry_profile_delete(int argc, char **argv)
{
    int ret;
    const char *iccid;

    if (argc < 4)
    {
        printf("Usage: %s profile delete <iccid>\n", argv[0]);
        return -1;
    }

    iccid = argv[3];

    if (entry_init())
        return -1;

    if ((ret = es10c_delete_profile_iccid(&ctx, iccid)))
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
            reason = "unknown error";
            break;
        }
        if (!output_as_json)
        {
            printf("es10c_delete_profile_iccid failed: %s\n", reason);
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10c_delete_profile_iccid\",\"data\":\"%s\"}\n", reason);
        }
        return -1;
    }

    if (!output_as_json)
    {
        printf("delete profile success\n");
    }
    else
    {
        printf("{\"return\":0,\"message\":\"success\",\"data\":null}\n");
    }

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

    if (entry_init())
        return -1;

    if (es10b_list_notification(&ctx, &notifications, &notifications_count))
    {
        if (!output_as_json)
        {
            printf("es10b_get_pending_notifications failed\n");
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10b_get_pending_notifications\",\"data\":null}\n");
        }
        return -1;
    }

    if (!output_as_json)
    {
        printf("notification count: %d\n", notifications_count);
        for (int i = 0; i < notifications_count; i++)
        {
            struct es10b_notification_metadata *notification = &notifications[i];
            printf("notification %d:\n", i);
            es10b_notification_metadata_print(notification);
        }
    }
    else
    {
        cJSON *jroot = NULL;
        cJSON *jdata = NULL;

        char *jstr = NULL;

        jroot = cJSON_CreateObject();
        cJSON_AddNumberToObject(jroot, "return", 0);
        cJSON_AddStringToObject(jroot, "message", "success");
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
        cJSON_AddItemToObject(jroot, "data", jdata);

        jstr = cJSON_PrintUnformatted(jroot);
        printf("%s\n", jstr);

        cJSON_Delete(jroot);
        free(jstr);
    }

    es10b_notification_metadata_free_all(notifications, notifications_count);

    return 0;
}

static int entry_notification_process(int argc, char **argv)
{
    unsigned long seqNumber;
    char *b64payload;
    char *receiver;

    if (argc < 4)
    {
        printf("Usage: %s notification process <seqNumber> [autodelete]\n", argv[0]);
        printf("\t[autodelete]: optional, [yes|other] auto-delete notification when smdp return success\n");
        return -1;
    }

    seqNumber = atol(argv[3]);

    if (entry_init())
        return -1;

    if (es10b_retrieve_notification(&ctx, &b64payload, &receiver, seqNumber))
    {
        if (!output_as_json)
        {
            printf("es10b_retrieve_notification failed\n");
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10b_retrieve_notification\",\"data\":null}\n");
        }
        return -1;
    }

    if (es9p_handle_notification(&ctx, receiver, b64payload))
    {
        if (!output_as_json)
        {
            printf("es9p_handle_notification failed\n");
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es9p_handle_notification\",\"data\":null}\n");
        }
        return -1;
    }

    if (argc > 4 && strcmp(argv[4], "yes") == 0)
    {
        if (es10b_remove_notification_from_list(&ctx, seqNumber))
        {
            if (!output_as_json)
            {
                printf("es10b_remove_notification_from_list failed\n");
            }
            else
            {
                printf("{\"return\":-1,\"message\":\"es10b_remove_notification_from_list\",\"data\":null}\n");
            }
            return -1;
        }
    }

    if (!output_as_json)
    {
        printf("process notification success\n");
    }
    else
    {
        printf("{\"return\":0,\"message\":\"success\",\"data\":null}\n");
    }

    free(b64payload);
    free(receiver);

    return 0;
}

static int entry_notification_remove(int argc, char **argv)
{
    int ret;
    unsigned long seqNumber;

    if (argc < 4)
    {
        printf("Usage: %s notification retrieve <seqNumber>\n", argv[0]);
        return -1;
    }

    seqNumber = atol(argv[3]);

    if (entry_init())
        return -1;

    if ((ret = es10b_remove_notification_from_list(&ctx, seqNumber)))
    {
        const char *reason;
        switch (ret)
        {
        case 1:
            reason = "seqNumber not found";
            break;
        default:
            reason = "unknown error";
            break;
        }
        if (!output_as_json)
        {
            printf("es10b_remove_notification_from_list failed: %s\n", reason);
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10b_remove_notification_from_list\",\"data\":\"%s\"}\n", reason);
        }
        return -1;
    }

    if (!output_as_json)
    {
        printf("remove notification success\n");
    }
    else
    {
        printf("{\"return\":0,\"message\":\"success\",\"data\":null}\n");
    }

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
    int opt;
    static const char *opt_string = "a:m:i:c:h?";

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

    opt = getopt(argc, argv, opt_string);
    while (opt != -1)
    {
        switch (opt)
        {
        case 'a':
            smdp = strdup(optarg);
            break;
        case 'm':
            matchingId = strdup(optarg);
            break;
        case 'i':
            imei = strdup(optarg);
            break;
        case 'c':
            confirmation_code = strdup(optarg);
            break;
        case 'h':
        case '?':
            printf("Usage: %s download [options]\n", argv[0]);
            printf("\t -a <smdp>\n");
            printf("\t -m <matchingId>\n");
            printf("\t -i <imei>\n");
            printf("\t -c <confirmation code>\n");
            printf("\t -h this help message\n");
            return -1;
            break;
        }
        opt = getopt(argc, argv, opt_string);
    }

    if (entry_init())
        return -1;

    if (smdp == NULL)
    {
        if (es10a_get_euicc_configured_addresses(&ctx, &smdp, NULL))
        {
            if (!output_as_json)
            {
                printf("es10a_get_euicc_configured_addresses failed\n");
            }
            else
            {
                printf("{\"return\":-1,\"message\":\"es10a_get_euicc_configured_addresses\",\"data\":null}\n");
            }
            return -1;
        }
    }

    if (es10b_get_euicc_challenge(&ctx, &b64_euicc_challenge))
    {
        if (!output_as_json)
        {
            printf("es10b_get_euicc_challenge failed\n");
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10b_get_euicc_challenge\",\"data\":null}\n");
        }
        return -1;
    }

    if (es10b_get_euicc_info(&ctx, &b64_euicc_info_1))
    {
        if (!output_as_json)
        {
            printf("es10b_get_euicc_info failed\n");
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10b_get_euicc_info\",\"data\":null}\n");
        }
        return -1;
    }

    if (es9p_initiate_authentication(&ctx, smdp, b64_euicc_challenge, b64_euicc_info_1, &es9p_initiate_authentication_resp))
    {
        if (!output_as_json)
        {
            printf("es9p_initiate_authentication failed: %s\n", es9p_initiate_authentication_resp.status);
        }
        else
        {
            cJSON *jroot = NULL;
            char *jstr = NULL;

            jroot = cJSON_CreateObject();
            cJSON_AddNumberToObject(jroot, "return", -1);
            cJSON_AddStringToObject(jroot, "message", "es9p_initiate_authentication");
            cJSON_AddStringToObject(jroot, "data", es9p_initiate_authentication_resp.status);

            jstr = cJSON_PrintUnformatted(jroot);
            printf("%s\n", jstr);

            cJSON_Delete(jroot);
            free(jstr);
        }
        return -1;
    }

    es10b_authenticate_server_param.b64_server_signed_1 = es9p_initiate_authentication_resp.b64_server_signed_1;
    es10b_authenticate_server_param.b64_server_signature_1 = es9p_initiate_authentication_resp.b64_server_signature_1;
    es10b_authenticate_server_param.b64_euicc_ci_pkid_to_be_used = es9p_initiate_authentication_resp.b64_euicc_ci_pkid_to_be_used;
    es10b_authenticate_server_param.b64_server_certificate = es9p_initiate_authentication_resp.b64_server_certificate;
    es10b_authenticate_server_param.matchingId = matchingId;
    es10b_authenticate_server_param.imei = imei;
    es10b_authenticate_server_param.tac = NULL;

    if (es10b_authenticate_server(&ctx, &b64_authenticate_server_response, &es10b_authenticate_server_param))
    {
        if (!output_as_json)
        {
            printf("es10b_authenticate_server failed\n");
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10b_authenticate_server\",\"data\":null}\n");
        }
        return -1;
    }

    if (es9p_authenticate_client(&ctx, smdp, es9p_initiate_authentication_resp.transaction_id, b64_authenticate_server_response, &es9p_authenticate_client_resp))
    {
        if (!output_as_json)
        {
            printf("es9p_authenticate_client failed: %s\n", es9p_authenticate_client_resp.status);
        }
        else
        {
            cJSON *jroot = NULL;
            char *jstr = NULL;

            jroot = cJSON_CreateObject();
            cJSON_AddNumberToObject(jroot, "return", -1);
            cJSON_AddStringToObject(jroot, "message", "es9p_authenticate_client");
            cJSON_AddStringToObject(jroot, "data", es9p_authenticate_client_resp.status);

            jstr = cJSON_PrintUnformatted(jroot);
            printf("%s\n", jstr);

            cJSON_Delete(jroot);
            free(jstr);
        }
        return -1;
    }

    es10b_prepare_download_param.b64_smdp_signed_2 = es9p_authenticate_client_resp.b64_smdp_signed_2;
    es10b_prepare_download_param.b64_smdp_signature_2 = es9p_authenticate_client_resp.b64_smdp_signature_2;
    es10b_prepare_download_param.b64_smdp_certificate = es9p_authenticate_client_resp.b64_smdp_certificate;
    if (confirmation_code)
    {
        es10b_prepare_download_param.str_checkcode = confirmation_code;
        es10b_prepare_download_param.hexstr_transcation_id = es9p_initiate_authentication_resp.transaction_id;
    }
    else
    {
        es10b_prepare_download_param.str_checkcode = NULL;
    }

    if (es10b_prepare_download(&ctx, &b64_prepare_download_response, &es10b_prepare_download_param))
    {
        if (!output_as_json)
        {
            printf("es10b_prepare_download failed\n");
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10b_prepare_download\",\"data\":null}\n");
        }
        return -1;
    }

    if (es9p_get_bound_profile_package(&ctx, smdp, es9p_initiate_authentication_resp.transaction_id, b64_prepare_download_response, &es9p_get_bound_profile_package_resp))
    {
        if (!output_as_json)
        {
            printf("es9p_get_bound_profile_package failed: %s\n", es9p_get_bound_profile_package_resp.status);
        }
        else
        {
            cJSON *jroot = NULL;
            char *jstr = NULL;

            jroot = cJSON_CreateObject();
            cJSON_AddNumberToObject(jroot, "return", -1);
            cJSON_AddStringToObject(jroot, "message", "es9p_get_bound_profile_package");
            cJSON_AddStringToObject(jroot, "data", es9p_get_bound_profile_package_resp.status);

            jstr = cJSON_PrintUnformatted(jroot);
            printf("%s\n", jstr);

            cJSON_Delete(jroot);
            free(jstr);
        }
        return -1;
    }

    if (es10b_load_bound_profile_package(&ctx, es9p_get_bound_profile_package_resp.b64_bpp))
    {
        if (!output_as_json)
        {
            printf("es10b_load_bound_profile_package failed\n");
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10b_load_bound_profile_package\",\"data\":null}\n");
        }
        return -1;
    }

    if (!output_as_json)
    {
        printf("download success\n");
    }
    else
    {
        printf("{\"return\":0,\"message\":\"success\",\"data\":null}\n");
    }

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

    if (entry_init())
        return -1;

    if (es10a_set_default_dp_address(&ctx, smdp))
    {
        if (!output_as_json)
        {
            printf("es10a_set_default_dp_address failed\n");
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10a_set_default_dp_address\",\"data\":null}\n");
        }
        return -1;
    }

    if (!output_as_json)
    {
        printf("set default smdp success\n");
    }
    else
    {
        printf("{\"return\":0,\"message\":\"success\",\"data\":null}\n");
    }

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

    if (entry_init())
        return -1;

    if ((ret = es10c_euicc_memory_reset(&ctx, 1, 1, 1)))
    {
        const char *reason;
        switch (ret)
        {
        case 1:
            reason = "nothing to delete";
            break;
        default:
            reason = "unknown error";
            break;
        }
        if (!output_as_json)
        {
            printf("es10c_euicc_memory_reset failed: %s\n", reason);
        }
        else
        {
            printf("{\"return\":-1,\"message\":\"es10c_euicc_memory_reset\",\"data\":\"%s\"}\n", reason);
        }
        return -1;
    }

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

    ctx.interface.apdu = dlsym_apdu_interface;
    ctx.interface.es9p = dlsym_es9p_interface;

    if (getenv("OUTPUT_JSON"))
        output_as_json = 1;

    if (argc < 2)
    {
        printf("Usage: %s <command> [options]\n", argv[0]);
        printf("\t<command>: [info|profile|notification|download|defaultsmdp|purge]\n");
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
    else
    {
        printf("Unknown command: %s\n", argv[1]);
        return -1;
    }

    entry_fini();
    return ret;
}
