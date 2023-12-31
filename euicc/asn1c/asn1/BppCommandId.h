/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPDefinitions"
 * 	found in "../../../asn1/rsp.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#ifndef	_BppCommandId_H_
#define	_BppCommandId_H_


#include "asn_application.h"

/* Including external dependencies */
#include "INTEGER.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum BppCommandId {
	BppCommandId_initialiseSecureChannel	= 0,
	BppCommandId_configureISDP	= 1,
	BppCommandId_storeMetadata	= 2,
	BppCommandId_storeMetadata2	= 3,
	BppCommandId_replaceSessionKeys	= 4,
	BppCommandId_loadProfileElements	= 5
} e_BppCommandId;

/* BppCommandId */
typedef INTEGER_t	 BppCommandId_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_BppCommandId;
asn_struct_free_f BppCommandId_free;
asn_struct_print_f BppCommandId_print;
asn_constr_check_f BppCommandId_constraint;
ber_type_decoder_f BppCommandId_decode_ber;
der_type_encoder_f BppCommandId_encode_der;
xer_type_decoder_f BppCommandId_decode_xer;
xer_type_encoder_f BppCommandId_encode_xer;
oer_type_decoder_f BppCommandId_decode_oer;
oer_type_encoder_f BppCommandId_encode_oer;
per_type_decoder_f BppCommandId_decode_uper;
per_type_encoder_f BppCommandId_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _BppCommandId_H_ */
#include "asn_internal.h"
