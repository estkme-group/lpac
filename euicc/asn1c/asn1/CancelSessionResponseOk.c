/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPDefinitions"
 * 	found in "../../../asn1/rsp.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#include "CancelSessionResponseOk.h"

asn_TYPE_member_t asn_MBR_CancelSessionResponseOk_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct CancelSessionResponseOk, euiccCancelSessionSigned),
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_EuiccCancelSessionSigned,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"euiccCancelSessionSigned"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct CancelSessionResponseOk, euiccCancelSessionSignature),
		(ASN_TAG_CLASS_APPLICATION | (55 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_OCTET_STRING,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"euiccCancelSessionSignature"
		},
};
static const ber_tlv_tag_t asn_DEF_CancelSessionResponseOk_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_CancelSessionResponseOk_tag2el_1[] = {
    { (ASN_TAG_CLASS_UNIVERSAL | (16 << 2)), 0, 0, 0 }, /* euiccCancelSessionSigned */
    { (ASN_TAG_CLASS_APPLICATION | (55 << 2)), 1, 0, 0 } /* euiccCancelSessionSignature */
};
asn_SEQUENCE_specifics_t asn_SPC_CancelSessionResponseOk_specs_1 = {
	sizeof(struct CancelSessionResponseOk),
	offsetof(struct CancelSessionResponseOk, _asn_ctx),
	asn_MAP_CancelSessionResponseOk_tag2el_1,
	2,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	2,	/* First extension addition */
};
asn_TYPE_descriptor_t asn_DEF_CancelSessionResponseOk = {
	"CancelSessionResponseOk",
	"CancelSessionResponseOk",
	&asn_OP_SEQUENCE,
	asn_DEF_CancelSessionResponseOk_tags_1,
	sizeof(asn_DEF_CancelSessionResponseOk_tags_1)
		/sizeof(asn_DEF_CancelSessionResponseOk_tags_1[0]), /* 1 */
	asn_DEF_CancelSessionResponseOk_tags_1,	/* Same as above */
	sizeof(asn_DEF_CancelSessionResponseOk_tags_1)
		/sizeof(asn_DEF_CancelSessionResponseOk_tags_1[0]), /* 1 */
	{ 0, 0, SEQUENCE_constraint },
	asn_MBR_CancelSessionResponseOk_1,
	2,	/* Elements count */
	&asn_SPC_CancelSessionResponseOk_specs_1	/* Additional specs */
};

