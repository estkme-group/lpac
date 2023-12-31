/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPDefinitions"
 * 	found in "../../../asn1/rsp.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#ifndef	_EUICCInfo2_H_
#define	_EUICCInfo2_H_


#include "asn_application.h"

/* Including external dependencies */
#include "VersionType.h"
#include "OCTET_STRING.h"
#include "UICCCapability.h"
#include "RspCapability.h"
#include "INTEGER.h"
#include "PprIds.h"
#include "UTF8String.h"
#include "SubjectKeyIdentifier.h"
#include "asn_SEQUENCE_OF.h"
#include "constr_SEQUENCE_OF.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum EUICCInfo2__euiccCategory {
	EUICCInfo2__euiccCategory_other	= 0,
	EUICCInfo2__euiccCategory_basicEuicc	= 1,
	EUICCInfo2__euiccCategory_mediumEuicc	= 2,
	EUICCInfo2__euiccCategory_contactlessEuicc	= 3
} e_EUICCInfo2__euiccCategory;

/* Forward declarations */
struct CertificationDataObject;

/* EUICCInfo2 */
typedef struct EUICCInfo2 {
	VersionType_t	 profileVersion;
	VersionType_t	 svn;
	VersionType_t	 euiccFirmwareVer;
	OCTET_STRING_t	 extCardResource;
	UICCCapability_t	 uiccCapability;
	VersionType_t	*javacardVersion	/* OPTIONAL */;
	VersionType_t	*globalplatformVersion	/* OPTIONAL */;
	RspCapability_t	 rspCapability;
	struct EUICCInfo2__euiccCiPKIdListForVerification {
		A_SEQUENCE_OF(SubjectKeyIdentifier_t) list;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} euiccCiPKIdListForVerification;
	struct EUICCInfo2__euiccCiPKIdListForSigning {
		A_SEQUENCE_OF(SubjectKeyIdentifier_t) list;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} euiccCiPKIdListForSigning;
	INTEGER_t	*euiccCategory	/* OPTIONAL */;
	PprIds_t	*forbiddenProfilePolicyRules	/* OPTIONAL */;
	VersionType_t	 ppVersion;
	UTF8String_t	 sasAcreditationNumber;
	struct CertificationDataObject	*certificationDataObject	/* OPTIONAL */;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} EUICCInfo2_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_EUICCInfo2;
extern asn_SEQUENCE_specifics_t asn_SPC_EUICCInfo2_specs_1;
extern asn_TYPE_member_t asn_MBR_EUICCInfo2_1[15];

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "CertificationDataObject.h"

#endif	/* _EUICCInfo2_H_ */
#include "asn_internal.h"
