/*
 *  Copyright (C) 2018 Systerel and others.
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

#include <assert.h>
#include <stdlib.h>

#include "constants.h"
#include "sopc_toolkit_config_internal.h"
#include "sopc_toolkit_constants.h"
#include "util_discovery_services.h"

const SOPC_String tcpUaTransportProfileURI = {
    .Length = 65,
    .DoNotClear = true,
    .Data = (SOPC_Byte*) "http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary",
};

const OpcUa_UserTokenPolicy anonymousUserTokenPolicy = {
    .PolicyId = {9, true, (SOPC_Byte*) "anonymous"},
    .TokenType = OpcUa_UserTokenType_Anonymous,
    .IssuedTokenType = {0, true, NULL},
    .IssuerEndpointUrl = {0, true, NULL},
    .SecurityPolicyUri = {0, true, NULL},
};

static void SOPC_SetServerCertificate(SOPC_Endpoint_Config* sopcEndpointConfig, OpcUa_EndpointDescription* epDesc)
{
    SOPC_ReturnStatus status = SOPC_STATUS_NOK;
    uint32_t tmpLength = 0;

    if (sopcEndpointConfig->serverCertificate != NULL)
    {
        status = SOPC_KeyManager_Certificate_CopyDER(sopcEndpointConfig->serverCertificate,
                                                     &epDesc->ServerCertificate.Data, &tmpLength);
    }
    assert(SOPC_STATUS_OK == status && tmpLength <= INT32_MAX);
    epDesc->ServerCertificate.Length = (int32_t) tmpLength;
}

static void SOPC_SetServerApplicationDescription(SOPC_Endpoint_Config* sopcEndpointConfig,
                                                 OpcUa_ApplicationDescription* appDesc)
{
    int32_t idx = 0;
    SOPC_String_AttachFrom(&appDesc->ApplicationUri, &sopcEndpointConfig->serverDescription.ApplicationUri);
    SOPC_String_AttachFrom(&appDesc->ProductUri, &sopcEndpointConfig->serverDescription.ProductUri);
    SOPC_LocalizedText_Copy(&appDesc->ApplicationName, &sopcEndpointConfig->serverDescription.ApplicationName);
    assert(sopcEndpointConfig->serverDescription.ApplicationType != OpcUa_ApplicationType_Client);
    appDesc->ApplicationType = sopcEndpointConfig->serverDescription.ApplicationType;
    SOPC_String_AttachFrom(&appDesc->GatewayServerUri, &sopcEndpointConfig->serverDescription.GatewayServerUri);
    SOPC_String_AttachFrom(&appDesc->DiscoveryProfileUri, &sopcEndpointConfig->serverDescription.DiscoveryProfileUri);

    if (sopcEndpointConfig->serverDescription.NoOfDiscoveryUrls >= 0)
    {
        appDesc->DiscoveryUrls =
            calloc((size_t) sopcEndpointConfig->serverDescription.NoOfDiscoveryUrls, sizeof(SOPC_String));
    }
    else
    {
        appDesc->DiscoveryUrls = NULL;
    }
    if (appDesc->DiscoveryUrls != NULL)
    {
        for (idx = 0; idx < sopcEndpointConfig->serverDescription.NoOfDiscoveryUrls; idx++)
        {
            SOPC_String_AttachFrom(&appDesc->DiscoveryUrls[idx],
                                   &sopcEndpointConfig->serverDescription.DiscoveryUrls[idx]);
        }
    }
}

constants__t_StatusCode_i SOPC_Discovery_GetEndPointsDescriptions(
    const constants__t_endpoint_config_idx_i endpoint_config_idx,
    bool isCreateSessionResponse,
    SOPC_String* requestEndpointUrl,
    uint32_t* nbOfEndpointDescriptions,
    OpcUa_EndpointDescription** endpointDescriptions)
{
    constants__t_StatusCode_i serviceResult = constants__e_sc_bad_invalid_argument;

    SOPC_Endpoint_Config* sopcEndpointConfig = NULL;
    SOPC_ReturnStatus status = SOPC_STATUS_NOK;
    SOPC_String configEndPointURL;
    uint8_t nbSecuConfigs = 0;
    uint32_t nbEndpointDescription = 0;
    SOPC_SecurityPolicy* tabSecurityPolicy = NULL;
    OpcUa_EndpointDescription* currentConfig_EndpointDescription = NULL;

    SOPC_String_Initialize(&configEndPointURL);

    sopcEndpointConfig = SOPC_ToolkitServer_GetEndpointConfig(endpoint_config_idx);

    if (NULL != sopcEndpointConfig)
    {
        status = SOPC_String_AttachFromCstring(&configEndPointURL, sopcEndpointConfig->endpointURL);
        assert(SOPC_STATUS_OK == status);

        /* Note: comparison with requested URL is not necessary since we have to return a default URL in any case */
        (void) requestEndpointUrl;
        serviceResult = constants__e_sc_ok;
    }
    else
    {
        serviceResult = constants__e_sc_bad_unexpected_error;
    }

    if (constants__e_sc_ok == serviceResult)
    {
        nbSecuConfigs = sopcEndpointConfig->nbSecuConfigs;
        tabSecurityPolicy = sopcEndpointConfig->secuConfigurations;

        /* if false == isCreateSessionResponse
         * see part 4 §5.6.2.2: it is recommended that Servers only include the endpointUrl,
         * securityMode, securityPolicyUri, userIdentityTokens, transportProfileUri
         * and securityLevel with all other parameters set to null. */

        // TODO: this code section can probably be optimized
        currentConfig_EndpointDescription = malloc(sizeof(OpcUa_EndpointDescription) * 3 * nbSecuConfigs);
        nbEndpointDescription = 0;
        OpcUa_EndpointDescription newEndPointDescription;
        OpcUa_EndpointDescription_Initialize(&newEndPointDescription);

        if (currentConfig_EndpointDescription != NULL)
        {
            // Set endpointUrl
            newEndPointDescription.EndpointUrl = configEndPointURL;

            /* Note: current server only support anonymous user authentication */

            // Set transport profile URI
            newEndPointDescription.TransportProfileUri = tcpUaTransportProfileURI;

            for (int iSecuConfig = 0; iSecuConfig < nbSecuConfigs; iSecuConfig++)
            {
                SOPC_SecurityPolicy currentSecurityPolicy = tabSecurityPolicy[iSecuConfig];
                uint16_t securityModes = currentSecurityPolicy.securityModes;

                // Set securityPolicyUri
                SOPC_String_AttachFrom(&newEndPointDescription.SecurityPolicyUri,
                                       &currentSecurityPolicy.securityPolicy);

                // Add an EndpointDescription per security mode
                if ((SOPC_SECURITY_MODE_NONE_MASK & securityModes) != 0)
                {
                    // Set securityMode
                    newEndPointDescription.SecurityMode = OpcUa_MessageSecurityMode_None;

                    // Set userIdentityTokens
                    newEndPointDescription.UserIdentityTokens = calloc(1, sizeof(OpcUa_UserTokenPolicy));
                    if (NULL != newEndPointDescription.UserIdentityTokens)
                    {
                        newEndPointDescription.NoOfUserIdentityTokens = 1;
                        newEndPointDescription.UserIdentityTokens[0] = anonymousUserTokenPolicy;
                    }

                    // Set securityLevel
                    /* see §7.10 Part4 - Value 0 is for not recommended endPoint.
                       Others values corresponds to more secured endPoints.*/
                    newEndPointDescription.SecurityLevel = 0;

                    OpcUa_ApplicationDescription_Initialize(&newEndPointDescription.Server);
                    if (false == isCreateSessionResponse)
                    {
                        // Set ApplicationDescription
                        SOPC_SetServerApplicationDescription(sopcEndpointConfig, &newEndPointDescription.Server);
                    }
                    else
                    {
                        // Set Server.ApplicationUri only (see mantis #3578 + part 4 v1.04 RC)
                        SOPC_String_AttachFrom(&newEndPointDescription.Server.ApplicationUri,
                                               &sopcEndpointConfig->serverDescription.ApplicationUri);
                    }

                    currentConfig_EndpointDescription[nbEndpointDescription] = newEndPointDescription;
                    nbEndpointDescription++;
                }

                if ((SOPC_SECURITY_MODE_SIGN_MASK & securityModes) != 0)
                {
                    // Set securityMode
                    newEndPointDescription.SecurityMode = OpcUa_MessageSecurityMode_Sign;

                    // Set userIdentityTokens
                    newEndPointDescription.UserIdentityTokens = calloc(1, sizeof(OpcUa_UserTokenPolicy));
                    if (NULL != newEndPointDescription.UserIdentityTokens)
                    {
                        newEndPointDescription.NoOfUserIdentityTokens = 1;
                        newEndPointDescription.UserIdentityTokens[0] = anonymousUserTokenPolicy;
                    }

                    // Set securityLevel
                    newEndPointDescription.SecurityLevel = 1;

                    OpcUa_ApplicationDescription_Initialize(&newEndPointDescription.Server);
                    if (false == isCreateSessionResponse)
                    {
                        // Set serverCertificate
                        SOPC_SetServerCertificate(sopcEndpointConfig, &newEndPointDescription);
                        // Set ApplicationDescription
                        SOPC_SetServerApplicationDescription(sopcEndpointConfig, &newEndPointDescription.Server);
                    }
                    else
                    {
                        // Set Server.ApplicationUri only (see mantis #3578 + part 4 v1.04 RC)
                        SOPC_String_AttachFrom(&newEndPointDescription.Server.ApplicationUri,
                                               &sopcEndpointConfig->serverDescription.ApplicationUri);
                    }

                    currentConfig_EndpointDescription[nbEndpointDescription] = newEndPointDescription;
                    nbEndpointDescription++;
                }

                if ((SOPC_SECURITY_MODE_SIGNANDENCRYPT_MASK & securityModes) != 0)
                {
                    // Set securityMode
                    newEndPointDescription.SecurityMode = OpcUa_MessageSecurityMode_SignAndEncrypt;

                    // Set userIdentityTokens
                    newEndPointDescription.UserIdentityTokens = calloc(1, sizeof(OpcUa_UserTokenPolicy));
                    if (NULL != newEndPointDescription.UserIdentityTokens)
                    {
                        newEndPointDescription.NoOfUserIdentityTokens = 1;
                        newEndPointDescription.UserIdentityTokens[0] = anonymousUserTokenPolicy;
                    }

                    // Set securityLevel
                    newEndPointDescription.SecurityLevel = 1;

                    OpcUa_ApplicationDescription_Initialize(&newEndPointDescription.Server);
                    if (false == isCreateSessionResponse)
                    {
                        // Set serverCertificate
                        SOPC_SetServerCertificate(sopcEndpointConfig, &newEndPointDescription);
                        // Set ApplicationDescription
                        SOPC_SetServerApplicationDescription(sopcEndpointConfig, &newEndPointDescription.Server);
                    }
                    else
                    {
                        // Set Server.ApplicationUri only (see mantis #3578 + part 4 v1.04 RC)
                        SOPC_String_AttachFrom(&newEndPointDescription.Server.ApplicationUri,
                                               &sopcEndpointConfig->serverDescription.ApplicationUri);
                    }

                    currentConfig_EndpointDescription[nbEndpointDescription] = newEndPointDescription;
                    nbEndpointDescription++;
                }
            }
        }
        else
        {
            serviceResult = constants__e_sc_bad_out_of_memory;
        }

        OpcUa_EndpointDescription* final_OpcUa_EndpointDescription = NULL;
        if (nbEndpointDescription > 0)
        {
            final_OpcUa_EndpointDescription = calloc((size_t) nbEndpointDescription, sizeof(OpcUa_EndpointDescription));
            if (final_OpcUa_EndpointDescription != NULL)
            {
                for (uint32_t i = 0; i < nbEndpointDescription; i++)
                {
                    final_OpcUa_EndpointDescription[i] = currentConfig_EndpointDescription[i];
                }
            }
            else
            {
                serviceResult = constants__e_sc_bad_out_of_memory;
            }
        }
        *nbOfEndpointDescriptions = nbEndpointDescription;
        *endpointDescriptions = final_OpcUa_EndpointDescription;

        if (currentConfig_EndpointDescription != NULL)
        {
            free(currentConfig_EndpointDescription);
        }
    }

    return serviceResult;
}

bool SOPC_Discovery_ContainsBinaryProfileURI(uint32_t nbOfProfileURI, SOPC_String* profileUris)
{
    bool result = false;
    uint32_t i = 0;
    for (i = 0; i < nbOfProfileURI && false == result; i++)
    {
        result = SOPC_String_Equal(&tcpUaTransportProfileURI, &profileUris[i]);
    }
    return result;
}
