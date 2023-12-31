/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPDefinitions"
 * 	found in "../../../asn1/rsp.asn"
 * 	`asn1c -fwide-types -fcompound-names -fincludes-quoted -no-gen-example`
 */

#ifndef	_RemoteProfileProvisioningRequest_H_
#define	_RemoteProfileProvisioningRequest_H_


#include "asn_application.h"

/* Including external dependencies */
#include "InitiateAuthenticationRequest.h"
#include "AuthenticateClientRequest.h"
#include "GetBoundProfilePackageRequest.h"
#include "CancelSessionRequestEs9.h"
#include "HandleNotification.h"
#include "constr_CHOICE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum RemoteProfileProvisioningRequest_PR {
	RemoteProfileProvisioningRequest_PR_NOTHING,	/* No components present */
	RemoteProfileProvisioningRequest_PR_initiateAuthenticationRequest,
	RemoteProfileProvisioningRequest_PR_authenticateClientRequest,
	RemoteProfileProvisioningRequest_PR_getBoundProfilePackageRequest,
	RemoteProfileProvisioningRequest_PR_cancelSessionRequestEs9,
	RemoteProfileProvisioningRequest_PR_handleNotification
	/* Extensions may appear below */
	
} RemoteProfileProvisioningRequest_PR;

/* RemoteProfileProvisioningRequest */
typedef struct RemoteProfileProvisioningRequest {
	RemoteProfileProvisioningRequest_PR present;
	union RemoteProfileProvisioningRequest_u {
		InitiateAuthenticationRequest_t	 initiateAuthenticationRequest;
		AuthenticateClientRequest_t	 authenticateClientRequest;
		GetBoundProfilePackageRequest_t	 getBoundProfilePackageRequest;
		CancelSessionRequestEs9_t	 cancelSessionRequestEs9;
		HandleNotification_t	 handleNotification;
		/*
		 * This type is extensible,
		 * possible extensions are below.
		 */
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} RemoteProfileProvisioningRequest_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_RemoteProfileProvisioningRequest;

#ifdef __cplusplus
}
#endif

#endif	/* _RemoteProfileProvisioningRequest_H_ */
#include "asn_internal.h"
