/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPDefinitions"
 * 	found in "../../../asn1/rsp.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#ifndef	_DisableProfileResponse_H_
#define	_DisableProfileResponse_H_


#include "asn_application.h"

/* Including external dependencies */
#include "INTEGER.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum DisableProfileResponse__disableResult {
	DisableProfileResponse__disableResult_ok	= 0,
	DisableProfileResponse__disableResult_iccidOrAidNotFound	= 1,
	DisableProfileResponse__disableResult_profileNotInEnabledState	= 2,
	DisableProfileResponse__disableResult_disallowedByPolicy	= 3,
	DisableProfileResponse__disableResult_undefinedError	= 127
} e_DisableProfileResponse__disableResult;

/* DisableProfileResponse */
typedef struct DisableProfileResponse {
	INTEGER_t	 disableResult;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} DisableProfileResponse_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_DisableProfileResponse;

#ifdef __cplusplus
}
#endif

#endif	/* _DisableProfileResponse_H_ */
#include "asn_internal.h"
