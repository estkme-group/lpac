/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPDefinitions"
 * 	found in "../../../asn1/rsp.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#include "ConfigureISDPRequest.h"

static asn_TYPE_member_t asn_MBR_ConfigureISDPRequest_1[] = {
	{ ATF_POINTER, 1, offsetof(struct ConfigureISDPRequest, dpProprietaryData),
		(ASN_TAG_CLASS_CONTEXT | (24 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_DpProprietaryData,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"dpProprietaryData"
		},
};
static const int asn_MAP_ConfigureISDPRequest_oms_1[] = { 0 };
static const ber_tlv_tag_t asn_DEF_ConfigureISDPRequest_tags_1[] = {
	(ASN_TAG_CLASS_CONTEXT | (36 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_ConfigureISDPRequest_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (24 << 2)), 0, 0, 0 } /* dpProprietaryData */
};
static asn_SEQUENCE_specifics_t asn_SPC_ConfigureISDPRequest_specs_1 = {
	sizeof(struct ConfigureISDPRequest),
	offsetof(struct ConfigureISDPRequest, _asn_ctx),
	asn_MAP_ConfigureISDPRequest_tag2el_1,
	1,	/* Count of tags in the map */
	asn_MAP_ConfigureISDPRequest_oms_1,	/* Optional members */
	1, 0,	/* Root/Additions */
	1,	/* First extension addition */
};
asn_TYPE_descriptor_t asn_DEF_ConfigureISDPRequest = {
	"ConfigureISDPRequest",
	"ConfigureISDPRequest",
	&asn_OP_SEQUENCE,
	asn_DEF_ConfigureISDPRequest_tags_1,
	sizeof(asn_DEF_ConfigureISDPRequest_tags_1)
		/sizeof(asn_DEF_ConfigureISDPRequest_tags_1[0]) - 1, /* 1 */
	asn_DEF_ConfigureISDPRequest_tags_1,	/* Same as above */
	sizeof(asn_DEF_ConfigureISDPRequest_tags_1)
		/sizeof(asn_DEF_ConfigureISDPRequest_tags_1[0]), /* 2 */
	{ 0, 0, SEQUENCE_constraint },
	asn_MBR_ConfigureISDPRequest_1,
	1,	/* Elements count */
	&asn_SPC_ConfigureISDPRequest_specs_1	/* Additional specs */
};

