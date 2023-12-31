/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "PKIX1Explicit88"
 * 	found in "../../../asn1/PKIXExplicit88.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#include "ExtensionAttributes.h"

static asn_oer_constraints_t asn_OER_type_ExtensionAttributes_constr_1 CC_NOTUSED = {
	{ 0, 0 },
	-1	/* (SIZE(1..256)) */};
asn_per_constraints_t asn_PER_type_ExtensionAttributes_constr_1 CC_NOTUSED = {
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	{ APC_CONSTRAINED,	 8,  8,  1,  256 }	/* (SIZE(1..256)) */,
	0, 0	/* No PER value map */
};
asn_TYPE_member_t asn_MBR_ExtensionAttributes_1[] = {
	{ ATF_POINTER, 0, 0,
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_ExtensionAttribute,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		""
		},
};
static const ber_tlv_tag_t asn_DEF_ExtensionAttributes_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (17 << 2))
};
asn_SET_OF_specifics_t asn_SPC_ExtensionAttributes_specs_1 = {
	sizeof(struct ExtensionAttributes),
	offsetof(struct ExtensionAttributes, _asn_ctx),
	0,	/* XER encoding is XMLDelimitedItemList */
};
asn_TYPE_descriptor_t asn_DEF_ExtensionAttributes = {
	"ExtensionAttributes",
	"ExtensionAttributes",
	&asn_OP_SET_OF,
	asn_DEF_ExtensionAttributes_tags_1,
	sizeof(asn_DEF_ExtensionAttributes_tags_1)
		/sizeof(asn_DEF_ExtensionAttributes_tags_1[0]), /* 1 */
	asn_DEF_ExtensionAttributes_tags_1,	/* Same as above */
	sizeof(asn_DEF_ExtensionAttributes_tags_1)
		/sizeof(asn_DEF_ExtensionAttributes_tags_1[0]), /* 1 */
	{ &asn_OER_type_ExtensionAttributes_constr_1, &asn_PER_type_ExtensionAttributes_constr_1, SET_OF_constraint },
	asn_MBR_ExtensionAttributes_1,
	1,	/* Single element */
	&asn_SPC_ExtensionAttributes_specs_1	/* Additional specs */
};

