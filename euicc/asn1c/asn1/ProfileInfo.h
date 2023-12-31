/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPDefinitions"
 * 	found in "../../../asn1/rsp.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#ifndef	_ProfileInfo_H_
#define	_ProfileInfo_H_


#include "asn_application.h"

/* Including external dependencies */
#include "Iccid.h"
#include "OctetTo16.h"
#include "ProfileState.h"
#include "UTF8String.h"
#include "IconType.h"
#include "OCTET_STRING.h"
#include "ProfileClass.h"
#include "PprIds.h"
#include "asn_SEQUENCE_OF.h"
#include "constr_SEQUENCE_OF.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct OperatorID;
struct DpProprietaryData;
struct NotificationConfigurationInformation;
struct RefArDo;

/* ProfileInfo */
typedef struct ProfileInfo {
	Iccid_t	*iccid	/* OPTIONAL */;
	OctetTo16_t	*isdpAid	/* OPTIONAL */;
	ProfileState_t	*profileState	/* OPTIONAL */;
	UTF8String_t	*profileNickname	/* OPTIONAL */;
	UTF8String_t	*serviceProviderName	/* OPTIONAL */;
	UTF8String_t	*profileName	/* OPTIONAL */;
	IconType_t	*iconType	/* OPTIONAL */;
	OCTET_STRING_t	*icon	/* OPTIONAL */;
	ProfileClass_t	*profileClass	/* DEFAULT 2 */;
	struct ProfileInfo__notificationConfigurationInfo {
		A_SEQUENCE_OF(struct NotificationConfigurationInformation) list;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} *notificationConfigurationInfo;
	struct OperatorID	*profileOwner	/* OPTIONAL */;
	struct DpProprietaryData	*dpProprietaryData	/* OPTIONAL */;
	PprIds_t	*profilePolicyRules	/* OPTIONAL */;
	struct ProfileInfo__refArDo {
		A_SEQUENCE_OF(struct RefArDo) list;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} *refArDo;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ProfileInfo_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ProfileInfo;
extern asn_SEQUENCE_specifics_t asn_SPC_ProfileInfo_specs_1;
extern asn_TYPE_member_t asn_MBR_ProfileInfo_1[14];

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "OperatorID.h"
#include "DpProprietaryData.h"
#include "NotificationConfigurationInformation.h"
#include "RefArDo.h"

#endif	/* _ProfileInfo_H_ */
#include "asn_internal.h"
