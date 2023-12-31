/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPDefinitions"
 * 	found in "../../../asn1/rsp.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#ifndef	_CtxParamsForCommonAuthentication_H_
#define	_CtxParamsForCommonAuthentication_H_


#include "asn_application.h"

/* Including external dependencies */
#include "UTF8String.h"
#include "DeviceInfo.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CtxParamsForCommonAuthentication */
typedef struct CtxParamsForCommonAuthentication {
	UTF8String_t	*matchingId	/* OPTIONAL */;
	DeviceInfo_t	 deviceInfo;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} CtxParamsForCommonAuthentication_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_CtxParamsForCommonAuthentication;
extern asn_SEQUENCE_specifics_t asn_SPC_CtxParamsForCommonAuthentication_specs_1;
extern asn_TYPE_member_t asn_MBR_CtxParamsForCommonAuthentication_1[2];

#ifdef __cplusplus
}
#endif

#endif	/* _CtxParamsForCommonAuthentication_H_ */
#include "asn_internal.h"
