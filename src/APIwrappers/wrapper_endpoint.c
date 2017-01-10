/*
 *  Copyright (C) 2016 Systerel and others.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "wrapper_endpoint.h"
#include "wrapper_encodeableobject.h"

#include <string.h>

#include "pki_stack.h"

typedef enum
{
    TMP_Invalid_PKI   = 0,
    TMP_NO_PKI        = 1,
    TMP_Override      = 2,
    TMP_DefaultPKI    = 3
} PKI_Types;

typedef struct
{
    PKI_Types       type;
    char*           trustListLocation;
    char*           revocationListLocation;
    char*           untrustedListLocation;
    uint32_t        flags;
    void*           unused;
} PKIConfig;

typedef struct {
    SOPC_Endpoint_PfnEndpointCallback* callback;
    void*                              callbackData;
} OpcUa_InternalEndpoint_CallbackData;

OpcUa_InternalEndpoint_CallbackData* OpcUa_Create_EndpointCallbackData(SOPC_Endpoint_PfnEndpointCallback* callback,
                                                                       void*                              callbackData)
{
    OpcUa_InternalEndpoint_CallbackData* result = malloc(sizeof(OpcUa_InternalEndpoint_CallbackData));
    if(result != NULL){
        result->callback = callback;
        result->callbackData = callbackData;
    }
    return result;
}

void OpcUa_Delete_EndpointCallbackData(OpcUa_InternalEndpoint_CallbackData* chCbData){
    if(chCbData != NULL){
        free(chCbData);
    }
}

SOPC_StatusCode SOPC_WrapperEndpointEvent_CB(SOPC_Endpoint             endpoint,
                                             void*                     cbData,
                                             SOPC_EndpointEvent        event,
                                             SOPC_StatusCode           status,
                                             uint32_t                  secureChannelId,
                                             const Certificate*        clientCertificate,
                                             const SOPC_String*        securityPolicy,
                                             OpcUa_MessageSecurityMode securityMode)
{
    SOPC_StatusCode retStatus = STATUS_INVALID_PARAMETERS;
    OpcUa_InternalEndpoint_CallbackData* iCbData = NULL;
    uint16_t secuModeMask = 0;
    SOPC_ByteString sCert;
    SOPC_ByteString_Initialize(&sCert);
    uint32_t certLength = 0;

    if(cbData != NULL){
        iCbData = (OpcUa_InternalEndpoint_CallbackData*) cbData;
        retStatus = STATUS_OK;
    }

    if(STATUS_OK == retStatus){
        switch(securityMode){
            case OpcUa_MessageSecurityMode_Invalid:
                secuModeMask = 0;
                status = STATUS_INVALID_STATE;
                break;
            case OpcUa_MessageSecurityMode_None:
                secuModeMask = SECURITY_MODE_NONE_MASK;
                break;
            case OpcUa_MessageSecurityMode_Sign:
                secuModeMask = SECURITY_MODE_SIGN_MASK;
                break;
            case OpcUa_MessageSecurityMode_SignAndEncrypt:
                secuModeMask = SECURITY_MODE_SIGNANDENCRYPT_MASK;
                break;
        }

        retStatus = KeyManager_Certificate_CopyDER(clientCertificate, &sCert.Data, &certLength);
    }

    if(STATUS_OK == retStatus){
        if(certLength > INT32_MAX){
            retStatus = STATUS_NOK;
        }else{
            sCert.Length = (int32_t) certLength;
        }
    }
    if(STATUS_OK == retStatus){
        retStatus = iCbData->callback(endpoint,
                                      iCbData->callbackData,
                                      event,
                                      status,
                                      secureChannelId,
                                      &sCert,
                                      (SOPC_String*) securityPolicy,
                                      secuModeMask);
    }
    SOPC_ByteString_Clear(&sCert);
    return retStatus;
}

SOPC_StatusCode OpcUa_Endpoint_Create(SOPC_Endpoint*               endpoint,
                                      SOPC_Endpoint_SerializerType serializerType,
                                      SOPC_ServiceType**           supportedServices){
    return SOPC_Endpoint_Create(endpoint, serializerType, supportedServices);
}

void OpcUa_Endpoint_Delete(SOPC_Endpoint* endpoint){
    OpcUa_Endpoint_Close(*endpoint);
    SOPC_Endpoint_Delete(endpoint);
}

SOPC_StatusCode OpcUa_Endpoint_GetMessageSecureChannelId(SOPC_Endpoint               endpoint,
                                                         struct SOPC_RequestContext* context,
                                                         uint32_t*                   secureChannelId){
    (void) endpoint;
    return SOPC_Endpoint_GetContextSecureChannelId(context, secureChannelId);
}

SOPC_StatusCode OpcUa_Endpoint_GetMessageSecureChannelSecurityPolicy(SOPC_Endpoint               endpoint,
                                                                     struct SOPC_RequestContext* context,
                                                                     SOPC_SecurityPolicy*        securityPolicy){
    SOPC_StatusCode status = STATUS_INVALID_PARAMETERS;
    OpcUa_MessageSecurityMode secuMode = OpcUa_MessageSecurityMode_Invalid;
    (void) endpoint;
    if(securityPolicy != NULL){
        securityPolicy->padding = NULL;
        status = SOPC_Endpoint_GetContextSecureChannelSecurityPolicy(context,
                                                                     &securityPolicy->securityPolicy,
                                                                     &secuMode);
         switch(secuMode){
             case OpcUa_MessageSecurityMode_Invalid:
                 securityPolicy->securityModes = 0;
                 status = STATUS_INVALID_STATE;
                 break;
             case OpcUa_MessageSecurityMode_None:
                 securityPolicy->securityModes = SECURITY_MODE_NONE_MASK;
                 break;
             case OpcUa_MessageSecurityMode_Sign:
                 securityPolicy->securityModes = SECURITY_MODE_SIGN_MASK;
                 break;
             case OpcUa_MessageSecurityMode_SignAndEncrypt:
                 securityPolicy->securityModes = SECURITY_MODE_SIGNANDENCRYPT_MASK;
                 break;
         }
     }
     return status;
    }

SOPC_StatusCode OpcUa_Endpoint_Open(SOPC_Endpoint                      endpoint,
                                    char*                              url,
#ifdef STACK_1_03
                                    uint8_t                            listenOnAllInterfaces,
#endif
                                    SOPC_Endpoint_PfnEndpointCallback* endpointCallback,
                                    void*                              endpointCallbackData,
                                    SOPC_ByteString*                   serverCertificate,
                                    SOPC_WrapperKey*                   serverPrivateKey,
                                    void*                              pkiConfig,
                                    uint32_t                           noOfSecurityPolicies,
                                    SOPC_SecurityPolicy*               securityPolicies){
    SOPC_StatusCode status = STATUS_OK;
    OpcUa_InternalEndpoint_CallbackData* internalCbData = NULL;
    Certificate* cert = NULL;
    Certificate* crt_ca = NULL;
    AsymmetricKey* key = NULL;
    PKIProvider *pki;
    PKIConfig *pPKIConfig = pkiConfig;
    SC_ServerEndpoint* sEndpoint = (SC_ServerEndpoint*) endpoint;
#ifdef STACK_1_03
    if(listenOnAllInterfaces == FALSE){
        return STATUS_INVALID_PARAMETERS;
    }
#endif
    internalCbData = OpcUa_Create_EndpointCallbackData(endpointCallback, endpointCallbackData);
    if(serverCertificate != NULL && serverCertificate->Length > 0 &&
       serverPrivateKey != NULL && serverPrivateKey->key.Length > 0 &&
       pkiConfig != NULL){
        if(serverCertificate != NULL){
            status = KeyManager_Certificate_CreateFromDER(serverCertificate->Data,
                                                          serverCertificate->Length,
                                                          &cert);
        }
        if(STATUS_OK == status && serverPrivateKey != NULL){
            status = KeyManager_AsymmetricKey_CreateFromBuffer(serverPrivateKey->key.Data,
                                                               serverPrivateKey->key.Length,
                                                               &key);
        }
        //TODO: CA folder != CA cert: how to deal with that ?
        if(STATUS_OK == status){
            const char* cacertname = "/cacert.der";
            char cacert[strlen(pPKIConfig->trustListLocation) + strlen(cacertname) + 1];
            if(cacert != memcpy(cacert, pPKIConfig->trustListLocation, strlen(pPKIConfig->trustListLocation)))
                status = STATUS_NOK;
            if(&cacert[strlen(pPKIConfig->trustListLocation)] !=
                memcpy(&cacert[strlen(pPKIConfig->trustListLocation)], cacertname, strlen(cacertname) + 1))
                status = STATUS_NOK;
            if(STATUS_OK == status){
                status = KeyManager_Certificate_CreateFromFile(cacert, &crt_ca);
            }
        }
        if(STATUS_OK == status){
            status = PKIProviderStack_Create(crt_ca, NULL, &pki);
        }
    }

    if(STATUS_OK == status){
        status = SOPC_Endpoint_Open(endpoint,
                                    url,
                                    SOPC_WrapperEndpointEvent_CB,
                                    internalCbData,
                                    cert,
                                    key,
                                    pki,
                                    noOfSecurityPolicies,
                                    securityPolicies);
    }
    if(STATUS_OK != status){
        if(pki != NULL){
            KeyManager_Certificate_Free(pki->pUserCertAuthList);
            PKIProviderStack_Free(pki);
        }
        KeyManager_Certificate_Free(cert);
        KeyManager_AsymmetricKey_Free(key);
        if(internalCbData != NULL){
            OpcUa_Delete_EndpointCallbackData(internalCbData);
        }
        if(sEndpoint != NULL){
            sEndpoint->pkiProvider = NULL;
            sEndpoint->serverCertificate = NULL;
            sEndpoint->serverKey = NULL;
            sEndpoint->callbackData = NULL;
        }
    }
    return status;
}

SOPC_StatusCode OpcUa_Endpoint_Close(SOPC_Endpoint endpoint){
    OpcUa_Delete_EndpointCallbackData(SOPC_Endpoint_GetCallbackData(endpoint));
    SC_ServerEndpoint* sEndpoint = (SC_ServerEndpoint*) endpoint;
    SOPC_StatusCode status = SOPC_Endpoint_Close(endpoint);
    if(sEndpoint != NULL){
        if(sEndpoint->pkiProvider != NULL){
            KeyManager_Certificate_Free(sEndpoint->pkiProvider->pUserCertAuthList);
            PKIProviderStack_Free((PKIProvider*) sEndpoint->pkiProvider);
        }
        sEndpoint->pkiProvider = NULL;
        KeyManager_Certificate_Free((Certificate*) sEndpoint->serverCertificate);
        sEndpoint->serverCertificate = NULL;
        KeyManager_AsymmetricKey_Free((AsymmetricKey*) sEndpoint->serverKey);
        sEndpoint->serverKey = NULL;
        sEndpoint->callbackData = NULL;
    }
    return status;
}

SOPC_StatusCode OpcUa_Endpoint_BeginSendResponse(SOPC_Endpoint         endpoint,
                                                 void*                 context,
                                                 void**                response,
                                                 SOPC_EncodeableType** responseType){
    SOPC_StatusCode status = STATUS_INVALID_PARAMETERS;
    (void) endpoint;
    if(context != NULL && response != NULL && responseType != NULL){
        status = SOPC_Endpoint_GetContextResponseType(context, responseType);
        if(STATUS_OK == status){
            status = OpcUa_EncodeableObject_Create(*responseType, response);
        }
    }
    return status;
}

SOPC_StatusCode OpcUa_Endpoint_EndSendResponse(SOPC_Endpoint        endpoint,
                                               void*                context,
                                               SOPC_StatusCode      statusCode,
                                               void*                response,
                                               SOPC_EncodeableType* responseType){
    SOPC_StatusCode status = statusCode;
    if(statusCode == STATUS_OK){
        status = SOPC_Endpoint_SendResponse(endpoint,
                                            responseType,
                                            response,
                                            (struct SOPC_RequestContext**) &context);
    }

    if(status != STATUS_OK){
        OpcUa_Endpoint_CancelSendResponse(endpoint,
                                          statusCode,
                                          NULL,
                                          context);
    }
    return status;
}

SOPC_StatusCode OpcUa_Endpoint_CancelSendResponse(SOPC_Endpoint        endpoint,
                                                  SOPC_StatusCode      statusCode,
                                                  SOPC_String*         reason,
                                                  void*                context){

    return SOPC_Endpoint_AbortResponse(endpoint, statusCode, reason, context);
}

SOPC_StatusCode OpcUa_Endpoint_GetServiceFunction(SOPC_Endpoint        endpoint,
                                                  void*                context,
                                                  SOPC_InvokeService** invokeService){
    return SOPC_Endpoint_GetServiceFunction(endpoint, (struct SOPC_RequestContext*) context, invokeService);
}

SOPC_StatusCode OpcUa_Endpoint_UpdateServiceFunctions(SOPC_Endpoint            endpoint,
                                                      uint32_t                 requestTypeId,
                                                      SOPC_BeginInvokeService* beginInvokeService,
                                                      SOPC_InvokeService*      invokeService){
    SOPC_StatusCode status = STATUS_INVALID_PARAMETERS;
    SC_ServerEndpoint* sEndpoint = (SC_ServerEndpoint*) endpoint;
    SOPC_ServiceType* current = NULL;
    SOPC_ServiceType* result = NULL;
    uint32_t idx = 0;
    if(sEndpoint != NULL && sEndpoint->servicesTable != NULL){
        status = STATUS_NOK;
        do{
            current = sEndpoint->servicesTable[idx];
            if(current->RequestTypeId == requestTypeId){
                result = current;
            }
            idx++;
        }
        while(current != NULL && result == NULL);
        if(result != NULL){
            status = STATUS_OK;
            ((SOPC_ServiceType*)sEndpoint->servicesTable[idx-1])->BeginInvokeService = beginInvokeService;
            ((SOPC_ServiceType*)sEndpoint->servicesTable[idx-1])->InvokeService = invokeService;
        }
    }
    return status;
}

SOPC_StatusCode OpcUa_Endpoint_GetCallbackData(SOPC_Endpoint  endpoint,
                                               void**         callbackData){
    SOPC_StatusCode status = STATUS_INVALID_PARAMETERS;
    SC_ServerEndpoint* sEndpoint = (SC_ServerEndpoint*) endpoint;
    if(sEndpoint != NULL && callbackData != NULL){
        if(sEndpoint->callbackData != NULL){
            *callbackData = ((OpcUa_InternalEndpoint_CallbackData*) sEndpoint->callbackData)->callbackData;
        }
    }
    return status;
}

