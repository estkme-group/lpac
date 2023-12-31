/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "PKIX1Explicit88"
 * 	found in "../../../asn1/PKIXExplicit88.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#include "TeletexCommonName.h"

int
TeletexCommonName_constraint(const asn_TYPE_descriptor_t *td, const void *sptr,
			asn_app_constraint_failed_f *ctfailcb, void *app_key) {
	const TeletexString_t *st = (const TeletexString_t *)sptr;
	size_t size;
	
	if(!sptr) {
		ASN__CTFAIL(app_key, td, sptr,
			"%s: value not given (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
	
	size = st->size;
	
	if((size >= 1 && size <= 64)) {
		/* Constraint check succeeded */
		return 0;
	} else {
		ASN__CTFAIL(app_key, td, sptr,
			"%s: constraint failed (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
}

/*
 * This type is implemented using TeletexString,
 * so here we adjust the DEF accordingly.
 */
static asn_oer_constraints_t asn_OER_type_TeletexCommonName_constr_1 CC_NOTUSED = {
	{ 0, 0 },
	-1	/* (SIZE(0..MAX)) */};
static asn_per_constraints_t asn_PER_type_TeletexCommonName_constr_1 CC_NOTUSED = {
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	0, 0	/* No PER value map */
};
static const ber_tlv_tag_t asn_DEF_TeletexCommonName_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (20 << 2))
};
asn_TYPE_descriptor_t asn_DEF_TeletexCommonName = {
	"TeletexCommonName",
	"TeletexCommonName",
	&asn_OP_TeletexString,
	asn_DEF_TeletexCommonName_tags_1,
	sizeof(asn_DEF_TeletexCommonName_tags_1)
		/sizeof(asn_DEF_TeletexCommonName_tags_1[0]), /* 1 */
	asn_DEF_TeletexCommonName_tags_1,	/* Same as above */
	sizeof(asn_DEF_TeletexCommonName_tags_1)
		/sizeof(asn_DEF_TeletexCommonName_tags_1[0]), /* 1 */
	{ &asn_OER_type_TeletexCommonName_constr_1, &asn_PER_type_TeletexCommonName_constr_1, TeletexCommonName_constraint },
	0, 0,	/* No members */
	0	/* No specifics */
};

