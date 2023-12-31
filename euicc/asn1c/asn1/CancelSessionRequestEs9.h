/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPDefinitions"
 * 	found in "../../../asn1/rsp.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#ifndef	_CancelSessionRequestEs9_H_
#define	_CancelSessionRequestEs9_H_


#include "asn_application.h"

/* Including external dependencies */
#include "TransactionId.h"
#include "CancelSessionResponse.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CancelSessionRequestEs9 */
typedef struct CancelSessionRequestEs9 {
	TransactionId_t	 transactionId;
	CancelSessionResponse_t	 cancelSessionResponse;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} CancelSessionRequestEs9_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_CancelSessionRequestEs9;
extern asn_SEQUENCE_specifics_t asn_SPC_CancelSessionRequestEs9_specs_1;
extern asn_TYPE_member_t asn_MBR_CancelSessionRequestEs9_1[2];

#ifdef __cplusplus
}
#endif

#endif	/* _CancelSessionRequestEs9_H_ */
#include "asn_internal.h"
