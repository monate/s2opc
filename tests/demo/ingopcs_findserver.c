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

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sopc_builtintypes.h"
#include "sopc_crypto_provider.h"
#include "sopc_time.h"
#include "sopc_toolkit_async_api.h"
#include "sopc_toolkit_config.h"
#include "sopc_types.h"

#include "config.h"
#include "state_machine.h"

/* The state machine which handles async events.
 * It is shared between the main thread and the Toolkit event thread.
 * It should be protected by a Mutex.
 */
static StateMachine_Machine* g_pSM = NULL;

/* Event handler of the Discovery */
static void EventDispatcher_Discovery(SOPC_App_Com_Event event, uint32_t arg, void* pParam, uintptr_t smCtx);

static void PrintServers(OpcUa_FindServersResponse* pResp);

int main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;
    SOPC_ReturnStatus status = SOPC_STATUS_OK;
    uint32_t iWait = 0;

    printf("INGOPCS find server demo.\n");
    /* Init */
    status = SOPC_Toolkit_Initialize(EventDispatcher_Discovery);
    g_pSM = StateMachine_Create();

    if (SOPC_STATUS_OK == status && NULL == g_pSM)
    {
        status = SOPC_STATUS_NOK;
    }

    /* Configuration, which include Secure Channel configuration. */
    if (SOPC_STATUS_OK == status)
    {
        status = StateMachine_ConfigureMachine(g_pSM);
    }

    if (SOPC_STATUS_OK == status)
    {
        status = SOPC_Toolkit_Configured();
        if (SOPC_STATUS_OK == status)
        {
            printf("# Info: Toolkit configuration done.\n");
            printf("# Info: Sending FindServers.\n");
        }
        else
        {
            printf("# Error: Toolkit configuration failed.\n");
        }
    }

    /* Starts Discovery */
    if (SOPC_STATUS_OK == status)
    {
        status = StateMachine_StartFindServers(g_pSM);
    }

    /* Active wait */
    while (SOPC_STATUS_OK == status && !StateMachine_IsIdle(g_pSM) && iWait * SLEEP_LENGTH <= SC_LIFETIME)
    {
        iWait += 1;
        SOPC_Sleep(SLEEP_LENGTH);
    }

    /* Finish it */
    SOPC_Toolkit_Clear();
    StateMachine_Delete(&g_pSM);

    return (status == SOPC_STATUS_OK) ? 0 : 1;
}

static void EventDispatcher_Discovery(SOPC_App_Com_Event event, uint32_t arg, void* pParam, uintptr_t smCtx)
{
    uintptr_t appCtx = 0;

    if (StateMachine_EventDispatcher(g_pSM, &appCtx, event, arg, pParam, smCtx))
    {
        switch (event)
        {
        case SE_RCV_DISCOVERY_RESPONSE:
            PrintServers((OpcUa_FindServersResponse*) pParam);
            break;
        default:
            break;
        }
    }
    else
    {
        printf("# Error: Received event %i not processed by the machine.\n", event);
        g_pSM->state = stError;
    }
}

static void PrintServers(OpcUa_FindServersResponse* pResp)
{
    int32_t i = 0;
    int32_t j = 0;
    OpcUa_ApplicationDescription* pAppDesc = NULL;

    if (SOPC_GoodGenericStatus != pResp->ResponseHeader.ServiceResult)
    {
        printf("# Error: FindServers failed with status code %i.\n", pResp->ResponseHeader.ServiceResult);
    }
    else
    {
        printf("%" PRIi32 " servers through endpoint %s:\n", pResp->NoOfServers, ENDPOINT_URL);
    }

    for (i = 0; i < pResp->NoOfServers; ++i)
    {
        pAppDesc = &pResp->Servers[i];
        printf("- Server\n");
        printf("  ApplicationUri: %s\n", SOPC_String_GetRawCString(&pAppDesc->ApplicationUri));
        printf("  ProductUri: %s\n", SOPC_String_GetRawCString(&pAppDesc->ProductUri));
        printf("  ApplicationName: %s\n", SOPC_String_GetRawCString(&pAppDesc->ApplicationName.Text));
        printf("  ApplicationType: ");
        switch (pAppDesc->ApplicationType)
        {
        case OpcUa_ApplicationType_Server:
            printf("Server");
            break;
        case OpcUa_ApplicationType_Client:
            printf("Client");
            break;
        case OpcUa_ApplicationType_ClientAndServer:
            printf("ClientAndServer");
            break;
        case OpcUa_ApplicationType_DiscoveryServer:
            printf("DiscoveryServer");
            break;
        default:
            printf("<err>");
            break;
        }
        printf("\n  GatewayServerUri: %s\n", SOPC_String_GetRawCString(&pAppDesc->GatewayServerUri));
        printf("  DiscoveryProfileUri: %s\n", SOPC_String_GetRawCString(&pAppDesc->DiscoveryProfileUri));
        printf("  DiscoveryUrls\n");
        for (j = 0; j < pAppDesc->NoOfDiscoveryUrls; ++j)
        {
            printf("  -  %s\n", SOPC_String_GetRawCString(&pAppDesc->DiscoveryUrls[j]));
        }
    }
}