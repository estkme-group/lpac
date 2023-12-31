/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPDefinitions"
 * 	found in "../../../asn1/rsp.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#ifndef	_ExpirationDate_H_
#define	_ExpirationDate_H_


#include "asn_application.h"

/* Including external dependencies */
#include "Time.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ExpirationDate */
typedef Time_t	 ExpirationDate_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ExpirationDate;
asn_struct_free_f ExpirationDate_free;
asn_struct_print_f ExpirationDate_print;
asn_constr_check_f ExpirationDate_constraint;
ber_type_decoder_f ExpirationDate_decode_ber;
der_type_encoder_f ExpirationDate_encode_der;
xer_type_decoder_f ExpirationDate_decode_xer;
xer_type_encoder_f ExpirationDate_encode_xer;
oer_type_decoder_f ExpirationDate_decode_oer;
oer_type_encoder_f ExpirationDate_encode_oer;
per_type_decoder_f ExpirationDate_decode_uper;
per_type_encoder_f ExpirationDate_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _ExpirationDate_H_ */
#include "asn_internal.h"
