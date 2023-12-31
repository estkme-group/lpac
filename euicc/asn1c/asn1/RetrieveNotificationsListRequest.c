/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPDefinitions"
 * 	found in "../../../asn1/rsp.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#include "RetrieveNotificationsListRequest.h"

static asn_oer_constraints_t asn_OER_type_searchCriteria_constr_2 CC_NOTUSED = {
	{ 0, 0 },
	-1};
static asn_per_constraints_t asn_PER_type_searchCriteria_constr_2 CC_NOTUSED = {
	{ APC_CONSTRAINED | APC_EXTENSIBLE,  1,  1,  0,  1 }	/* (0..1,...) */,
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	0, 0	/* No PER value map */
};
static asn_TYPE_member_t asn_MBR_searchCriteria_2[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct RetrieveNotificationsListRequest__searchCriteria, choice.seqNumber),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_INTEGER,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"seqNumber"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct RetrieveNotificationsListRequest__searchCriteria, choice.profileManagementOperation),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_NotificationEvent,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"profileManagementOperation"
		},
};
static const asn_TYPE_tag2member_t asn_MAP_searchCriteria_tag2el_2[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* seqNumber */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 } /* profileManagementOperation */
};
static asn_CHOICE_specifics_t asn_SPC_searchCriteria_specs_2 = {
	sizeof(struct RetrieveNotificationsListRequest__searchCriteria),
	offsetof(struct RetrieveNotificationsListRequest__searchCriteria, _asn_ctx),
	offsetof(struct RetrieveNotificationsListRequest__searchCriteria, present),
	sizeof(((struct RetrieveNotificationsListRequest__searchCriteria *)0)->present),
	asn_MAP_searchCriteria_tag2el_2,
	2,	/* Count of tags in the map */
	0, 0,
	2	/* Extensions start */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_searchCriteria_2 = {
	"searchCriteria",
	"searchCriteria",
	&asn_OP_CHOICE,
	0,	/* No effective tags (pointer) */
	0,	/* No effective tags (count) */
	0,	/* No tags (pointer) */
	0,	/* No tags (count) */
	{ &asn_OER_type_searchCriteria_constr_2, &asn_PER_type_searchCriteria_constr_2, CHOICE_constraint },
	asn_MBR_searchCriteria_2,
	2,	/* Elements count */
	&asn_SPC_searchCriteria_specs_2	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_RetrieveNotificationsListRequest_1[] = {
	{ ATF_POINTER, 1, offsetof(struct RetrieveNotificationsListRequest, searchCriteria),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		+1,	/* EXPLICIT tag at current level */
		&asn_DEF_searchCriteria_2,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"searchCriteria"
		},
};
static const int asn_MAP_RetrieveNotificationsListRequest_oms_1[] = { 0 };
static const ber_tlv_tag_t asn_DEF_RetrieveNotificationsListRequest_tags_1[] = {
	(ASN_TAG_CLASS_CONTEXT | (43 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_RetrieveNotificationsListRequest_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 } /* searchCriteria */
};
static asn_SEQUENCE_specifics_t asn_SPC_RetrieveNotificationsListRequest_specs_1 = {
	sizeof(struct RetrieveNotificationsListRequest),
	offsetof(struct RetrieveNotificationsListRequest, _asn_ctx),
	asn_MAP_RetrieveNotificationsListRequest_tag2el_1,
	1,	/* Count of tags in the map */
	asn_MAP_RetrieveNotificationsListRequest_oms_1,	/* Optional members */
	1, 0,	/* Root/Additions */
	1,	/* First extension addition */
};
asn_TYPE_descriptor_t asn_DEF_RetrieveNotificationsListRequest = {
	"RetrieveNotificationsListRequest",
	"RetrieveNotificationsListRequest",
	&asn_OP_SEQUENCE,
	asn_DEF_RetrieveNotificationsListRequest_tags_1,
	sizeof(asn_DEF_RetrieveNotificationsListRequest_tags_1)
		/sizeof(asn_DEF_RetrieveNotificationsListRequest_tags_1[0]) - 1, /* 1 */
	asn_DEF_RetrieveNotificationsListRequest_tags_1,	/* Same as above */
	sizeof(asn_DEF_RetrieveNotificationsListRequest_tags_1)
		/sizeof(asn_DEF_RetrieveNotificationsListRequest_tags_1[0]), /* 2 */
	{ 0, 0, SEQUENCE_constraint },
	asn_MBR_RetrieveNotificationsListRequest_1,
	1,	/* Elements count */
	&asn_SPC_RetrieveNotificationsListRequest_specs_1	/* Additional specs */
};

