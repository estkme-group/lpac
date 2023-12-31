/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPDefinitions"
 * 	found in "../../../asn1/rsp.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#ifndef	_ProfilePolicyAuthorisationRule_H_
#define	_ProfilePolicyAuthorisationRule_H_


#include "asn_application.h"

/* Including external dependencies */
#include "PprIds.h"
#include "BIT_STRING.h"
#include "asn_SEQUENCE_OF.h"
#include "constr_SEQUENCE_OF.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum ProfilePolicyAuthorisationRule__pprFlags {
	ProfilePolicyAuthorisationRule__pprFlags_consentRequired	= 0
} e_ProfilePolicyAuthorisationRule__pprFlags;

/* Forward declarations */
struct OperatorID;

/* ProfilePolicyAuthorisationRule */
typedef struct ProfilePolicyAuthorisationRule {
	PprIds_t	 pprIds;
	struct ProfilePolicyAuthorisationRule__allowedOperators {
		A_SEQUENCE_OF(struct OperatorID) list;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} allowedOperators;
	BIT_STRING_t	 pprFlags;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ProfilePolicyAuthorisationRule_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ProfilePolicyAuthorisationRule;
extern asn_SEQUENCE_specifics_t asn_SPC_ProfilePolicyAuthorisationRule_specs_1;
extern asn_TYPE_member_t asn_MBR_ProfilePolicyAuthorisationRule_1[3];

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "OperatorID.h"

#endif	/* _ProfilePolicyAuthorisationRule_H_ */
#include "asn_internal.h"
