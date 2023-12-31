/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPDefinitions"
 * 	found in "../../../asn1/rsp.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#ifndef	_LoadCRLResponse_H_
#define	_LoadCRLResponse_H_


#include "asn_application.h"

/* Including external dependencies */
#include "LoadCRLResponseOk.h"
#include "LoadCRLResponseError.h"
#include "constr_CHOICE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum LoadCRLResponse_PR {
	LoadCRLResponse_PR_NOTHING,	/* No components present */
	LoadCRLResponse_PR_loadCRLResponseOk,
	LoadCRLResponse_PR_loadCRLResponseError
	/* Extensions may appear below */
	
} LoadCRLResponse_PR;

/* LoadCRLResponse */
typedef struct LoadCRLResponse {
	LoadCRLResponse_PR present;
	union LoadCRLResponse_u {
		LoadCRLResponseOk_t	 loadCRLResponseOk;
		LoadCRLResponseError_t	 loadCRLResponseError;
		/*
		 * This type is extensible,
		 * possible extensions are below.
		 */
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} LoadCRLResponse_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_LoadCRLResponse;

#ifdef __cplusplus
}
#endif

#endif	/* _LoadCRLResponse_H_ */
#include "asn_internal.h"
