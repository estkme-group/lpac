/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPDefinitions"
 * 	found in "../../../asn1/rsp.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#ifndef	_EuiccMemoryResetRequest_H_
#define	_EuiccMemoryResetRequest_H_


#include "asn_application.h"

/* Including external dependencies */
#include "BIT_STRING.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum EuiccMemoryResetRequest__resetOptions {
	EuiccMemoryResetRequest__resetOptions_deleteOperationalProfiles	= 0,
	EuiccMemoryResetRequest__resetOptions_deleteFieldLoadedTestProfiles	= 1,
	EuiccMemoryResetRequest__resetOptions_resetDefaultSmdpAddress	= 2
} e_EuiccMemoryResetRequest__resetOptions;

/* EuiccMemoryResetRequest */
typedef struct EuiccMemoryResetRequest {
	BIT_STRING_t	 resetOptions;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} EuiccMemoryResetRequest_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_EuiccMemoryResetRequest;

#ifdef __cplusplus
}
#endif

#endif	/* _EuiccMemoryResetRequest_H_ */
#include "asn_internal.h"
