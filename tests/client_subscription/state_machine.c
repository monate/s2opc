/*
 * Licensed to Systerel under one or more contributor license
 * agreements. See the NOTICE file distributed with this work
 * for additional information regarding copyright ownership.
 * Systerel licenses this file to you under the Apache
 * License, Version 2.0 (the "License"); you may not use this
 * file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/** \file
 *
 * \brief The state machine of the subscribing client. See state_machine.h
 *
 */

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sopc_atomic.h"
#include "sopc_encodeable.h"
#include "sopc_mutexes.h"
#include "sopc_singly_linked_list.h"
#include "sopc_toolkit_async_api.h"
#include "sopc_user_app_itf.h"

#include "state_machine.h"
#include "toolkit_helpers.h"

/* =========
 * Internals
 * =========
 */

/* Structures */
struct SOPC_StaMac_ReqCtx
{
    uint32_t uid;                          /* Unique request identifier */
    uintptr_t appCtx;                      /* Application context, chosen outside of the state machine */
    SOPC_StaMac_RequestScope requestScope; /* Whether the request is started by the state machine or the applicative */
};

struct SOPC_StaMac_Machine
{
    Mutex mutex;
    SOPC_StaMac_State state;
    uint32_t iscConfig;                       /* Toolkit scConfig ID */
    uint32_t iCliId;                          /* LibSub connection ID, used by the callback */
    SOPC_LibSub_DataChangeCbk cbkDataChanged; /* Callback when subscribed data changed */
    uintptr_t iSessionCtx;                    /* Toolkit Session Context, used to identify session events */
    uint32_t iSessionID;                      /* OPC UA Session ID */
    SOPC_SLinkedList* pListReqCtx;            /* List of yet-to-be-answered requests,
                                               * id is unique request identifier, value is a SOPC_StaMac_ReqCtx */
    double fPublishInterval;                  /* The publish interval, in ms */
    uint32_t iCntMaxKeepAlive;                /* Number of skipped response before sending an empty KeepAlive */
    uint32_t iCntLifetime;                    /* Number of deprived publish cycles before subscription deletion */
    uint32_t iSubscriptionID;                 /* OPC UA subscription ID, non 0 when subscription is created */
    SOPC_SLinkedList* pListMonIt;             /* List of monitored items, where the appCtx is the list value,
                                               * and the id is the uint32_t OPC UA monitored item ID */
    uint32_t nTokenTarget;                    /* Target number of available tokens */
    uint32_t nTokenUsable;                    /* Tokens available to the server
                                               * (PublishRequest_sent - PublishResponse_sent) */
    bool bAckSubscr;                          /* Indicates whether an acknowledgement should be sent
                                               * in the next PublishRequest */
    uint32_t iAckSeqNum;                      /* The sequence number to acknowledge after a PublishResponse */
    const char* szPolicyId;                   /* See SOPC_LibSub_ConnectionCfg */
    const char* szUsername;                   /* See SOPC_LibSub_ConnectionCfg */
    const char* szPassword;                   /* See SOPC_LibSub_ConnectionCfg */
};

/* Global variables */
static int32_t nSentReqs = 0;      /* Number of sent requests, used to uniquely associate a response to its request. */
static uintptr_t nPublishReqs = 0; /* Number of sent publish requests */

/* Internal functions */
static bool StaMac_IsEventTargeted(SOPC_StaMac_Machine* pSM,
                                   uintptr_t* pAppCtx,
                                   SOPC_App_Com_Event event,
                                   uint32_t arg,
                                   void* pParam,
                                   uintptr_t appCtx);
static void StaMac_ProcessEvent_stActivating(SOPC_StaMac_Machine* pSM,
                                             SOPC_App_Com_Event event,
                                             uint32_t arg,
                                             void* pParam,
                                             uintptr_t appCtx);
static void StaMac_ProcessEvent_stClosing(SOPC_StaMac_Machine* pSM,
                                          SOPC_App_Com_Event event,
                                          uint32_t arg,
                                          void* pParam,
                                          uintptr_t appCtx);
static void StaMac_ProcessEvent_stActivated(SOPC_StaMac_Machine* pSM,
                                            SOPC_App_Com_Event event,
                                            uint32_t arg,
                                            void* pParam,
                                            uintptr_t appCtx);
static void StaMac_ProcessEvent_stCreatingSubscr(SOPC_StaMac_Machine* pSM,
                                                 SOPC_App_Com_Event event,
                                                 uint32_t arg,
                                                 void* pParam,
                                                 uintptr_t appCtx);
static void StaMac_ProcessEvent_stCreatingMonIt(SOPC_StaMac_Machine* pSM,
                                                SOPC_App_Com_Event event,
                                                uint32_t arg,
                                                void* pParam,
                                                uintptr_t appCtx);
static void StaMac_ProcessEvent_stError(SOPC_StaMac_Machine* pSM,
                                        SOPC_App_Com_Event event,
                                        uint32_t arg,
                                        void* pParam,
                                        uintptr_t appCtx);
static void StaMac_PostProcessActions(SOPC_StaMac_Machine* pSM, SOPC_StaMac_State oldState);

/* ==================
 * API implementation
 * ==================
 */

SOPC_ReturnStatus SOPC_StaMac_Create(uint32_t iscConfig,
                                     uint32_t iCliId,
                                     const char* szPolicyId,
                                     const char* szUsername,
                                     const char* szPassword,
                                     SOPC_LibSub_DataChangeCbk cbkDataChanged,
                                     double fPublishInterval,
                                     uint32_t iCntMaxKeepAlive,
                                     uint32_t iCntLifetime,
                                     uint32_t iTokenTarget,
                                     SOPC_StaMac_Machine** ppSM)
{
    SOPC_StaMac_Machine* pSM = malloc(sizeof(SOPC_StaMac_Machine));

    if (NULL == pSM)
    {
        return SOPC_STATUS_OUT_OF_MEMORY;
    }

    SOPC_ReturnStatus status = Mutex_Initialization(&pSM->mutex);
    if (SOPC_STATUS_OK == status)
    {
        pSM->state = stInit;
        pSM->iscConfig = iscConfig;
        pSM->iCliId = iCliId;
        pSM->cbkDataChanged = cbkDataChanged;
        pSM->iSessionCtx = 0;
        pSM->iSessionID = 0;
        pSM->pListReqCtx = SOPC_SLinkedList_Create(0);
        pSM->fPublishInterval = fPublishInterval;
        pSM->iCntMaxKeepAlive = iCntMaxKeepAlive;
        pSM->iCntLifetime = iCntLifetime;
        pSM->iSubscriptionID = 0;
        pSM->pListMonIt = SOPC_SLinkedList_Create(0);
        pSM->nTokenTarget = iTokenTarget;
        pSM->nTokenUsable = 0;
        pSM->bAckSubscr = false;
        pSM->iAckSeqNum = 0;
        /* TODO: deep copy the strings */
        pSM->szPolicyId = szPolicyId;
        pSM->szUsername = szUsername;
        pSM->szPassword = szPassword;
    }

    if (SOPC_STATUS_OK == status && (NULL == pSM->pListReqCtx || NULL == pSM->pListMonIt))
    {
        status = SOPC_STATUS_OUT_OF_MEMORY;
    }

    /* Handles the value to the caller */
    if (SOPC_STATUS_OK == status)
    {
        *ppSM = pSM;
    }
    else
    {
        SOPC_StaMac_Delete(&pSM);
    }

    return status;
}

void SOPC_StaMac_Delete(SOPC_StaMac_Machine** ppSM)
{
    if (NULL != ppSM && NULL != *ppSM)
    {
        SOPC_StaMac_Machine* pSM = *ppSM;
        SOPC_ReturnStatus status = Mutex_Lock(&pSM->mutex);
        assert(SOPC_STATUS_OK == status);
        SOPC_SLinkedList_Delete(pSM->pListReqCtx);
        pSM->pListReqCtx = NULL;
        SOPC_SLinkedList_Delete(pSM->pListMonIt);
        pSM->pListMonIt = NULL;
        status = Mutex_Unlock(&pSM->mutex);
        assert(SOPC_STATUS_OK == status);
        Mutex_Clear(&pSM->mutex);
        free(pSM);
        *ppSM = NULL;
    }
}

SOPC_ReturnStatus SOPC_StaMac_StartSession(SOPC_StaMac_Machine* pSM)
{
    if (NULL == pSM)
    {
        return SOPC_STATUS_INVALID_PARAMETERS;
    }

    if (INT32_MAX == nSentReqs)
    {
        return SOPC_STATUS_INVALID_STATE;
    }

    SOPC_ReturnStatus status = SOPC_STATUS_OK;

    SOPC_ReturnStatus mutStatus = Mutex_Lock(&pSM->mutex);
    assert(SOPC_STATUS_OK == mutStatus);

    if (pSM->state != stInit)
    {
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "The state machine shall be in stInit state to start a session.");
        status = SOPC_STATUS_INVALID_STATE;
    }

    /* Sends the request */
    if (SOPC_STATUS_OK == status)
    {
        SOPC_Atomic_Int_Add(&nSentReqs, 1);
        pSM->iSessionCtx =
            (uintptr_t) nSentReqs; /* Record the session context, must be reset when connection is closed. */
        if (NULL == pSM->szUsername)
        {
            status = SOPC_ToolkitClient_AsyncActivateSession_Anonymous(pSM->iscConfig, (uintptr_t) pSM->iSessionCtx,
                                                                       pSM->szPolicyId);
        }
        else
        {
            status = SOPC_ToolkitClient_AsyncActivateSession_UsernamePassword(
                pSM->iscConfig, (uintptr_t) pSM->iSessionCtx, pSM->szPolicyId, pSM->szUsername,
                (const uint8_t*) pSM->szPassword, pSM->szPassword != NULL ? (int32_t) strlen(pSM->szPassword) : 0);
        }
    }
    if (SOPC_STATUS_OK == status)
    {
        pSM->state = stActivating;
    }
    else if (NULL != pSM)
    {
        pSM->state = stError;
    }

    mutStatus = Mutex_Unlock(&pSM->mutex);
    assert(SOPC_STATUS_OK == mutStatus);

    return status;
}

SOPC_ReturnStatus SOPC_StaMac_StopSession(SOPC_StaMac_Machine* pSM)
{
    SOPC_ReturnStatus status = Mutex_Lock(&pSM->mutex);
    assert(SOPC_STATUS_OK == status);

    if (!SOPC_StaMac_IsConnected(pSM))
    {
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "StopSession on a disconnected machine.");
        pSM->state = stError;
        status = SOPC_STATUS_NOK;
    }

    if (SOPC_STATUS_OK == status)
    {
        SOPC_ToolkitClient_AsyncCloseSession(pSM->iSessionID);
        pSM->state = stClosing;
    }

    status = Mutex_Unlock(&pSM->mutex);
    assert(SOPC_STATUS_OK == status);

    return status;
}

SOPC_ReturnStatus SOPC_StaMac_SendRequest(SOPC_StaMac_Machine* pSM,
                                          void* requestStruct,
                                          uintptr_t appCtx,
                                          SOPC_StaMac_RequestScope requestScope)
{
    SOPC_ReturnStatus status = SOPC_STATUS_OK;
    SOPC_StaMac_ReqCtx* pReqCtx = NULL;

    if (NULL == pSM || !SOPC_StaMac_IsConnected(pSM) || INT32_MAX == nSentReqs)
    {
        return SOPC_STATUS_INVALID_STATE;
    }

    assert(SOPC_REQUEST_SCOPE_STATE_MACHINE == requestScope || SOPC_REQUEST_SCOPE_APPLICATION == requestScope);

    pReqCtx = malloc(sizeof(SOPC_StaMac_ReqCtx));
    if (NULL == pReqCtx)
    {
        return SOPC_STATUS_OUT_OF_MEMORY;
    }

    SOPC_ReturnStatus mutStatus = Mutex_Lock(&pSM->mutex);
    assert(SOPC_STATUS_OK == mutStatus);

    /* Adds it to the list of yet-to-be-answered requests */
    SOPC_Atomic_Int_Add(&nSentReqs, 1);
    pReqCtx->uid = (uint32_t) nSentReqs;
    pReqCtx->appCtx = appCtx;
    pReqCtx->requestScope = requestScope;
    /* Asserts that there cannot be two requests with the same id when receiving a response */
    void* found = SOPC_SLinkedList_FindFromId(pSM->pListReqCtx, pReqCtx->uid);
    assert(NULL == found);
    if (SOPC_SLinkedList_Append(pSM->pListReqCtx, pReqCtx->uid, (void*) pReqCtx) != pReqCtx)
    {
        status = SOPC_STATUS_NOK;
    }

    /* Sends the request */
    if (SOPC_STATUS_OK == status)
    {
        SOPC_ToolkitClient_AsyncSendRequestOnSession(pSM->iSessionID, requestStruct, (uintptr_t) pReqCtx);
    }

    if (SOPC_STATUS_OK != status)
    {
        pSM->state = stError;
        free(pReqCtx);
    }

    mutStatus = Mutex_Unlock(&pSM->mutex);
    assert(SOPC_STATUS_OK == mutStatus);

    return status;
}

SOPC_ReturnStatus SOPC_StaMac_CreateMonitoredItem(SOPC_StaMac_Machine* pSM,
                                                  const char* szNodeId,
                                                  uint32_t iAttrId,
                                                  uintptr_t* pAppCtx,
                                                  uint32_t* pCliHndl)
{
    SOPC_ReturnStatus status = SOPC_STATUS_OK;
    SOPC_NodeId* pNid = NULL;
    size_t szLen = 0;
    void* pReq = NULL;
    uint32_t iCliHndl = 0;

    if (NULL == pSM || NULL == szNodeId || NULL == pCliHndl)
    {
        return SOPC_STATUS_INVALID_PARAMETERS;
    }

    if (!SOPC_StaMac_HasSubscription(pSM))
    {
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "the machine shall have a created subscription.");
        return SOPC_STATUS_INVALID_STATE;
    }
    if (INT32_MAX == nSentReqs)
    {
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "creating monitored item, too much sent requests.");
        return SOPC_STATUS_INVALID_STATE;
    }

    /* Create the NodeId */
    szLen = strlen(szNodeId);
    if (INT32_MAX < szLen)
    {
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "creating monitored item, szNodeId is too long.");
        return SOPC_STATUS_INVALID_PARAMETERS;
    }
    pNid = SOPC_NodeId_FromCString(szNodeId, (int32_t) szLen);
    if (NULL == pNid)
    {
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "creating monitored item, could not convert \"%s\" to NodeId.",
                    szNodeId);
        return SOPC_STATUS_NOK;
    }

    SOPC_ReturnStatus mutStatus = Mutex_Lock(&pSM->mutex);
    assert(SOPC_STATUS_OK == mutStatus);

    if (stActivated != pSM->state)
    {
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR,
                    "creating monitored item, the machine should be in the stActivated state (is in %i).", pSM->state);
        status = SOPC_STATUS_INVALID_STATE;
    }

    /* Create the CreateMonitoredItemRequest */
    if (SOPC_STATUS_OK == status)
    {
        SOPC_Atomic_Int_Add(&nSentReqs, 1);
        iCliHndl = (uint32_t) nSentReqs;
        status = Helpers_NewCreateMonitoredItemsRequest(pNid, iAttrId, pSM->iSubscriptionID, MONIT_TIMESTAMPS_TO_RETURN,
                                                        iCliHndl, MONIT_QSIZE, &pReq);
    }

    /* Send it */
    if (SOPC_STATUS_OK == status)
    {
        status = SOPC_StaMac_SendRequest(pSM, pReq, iCliHndl, SOPC_REQUEST_SCOPE_STATE_MACHINE);
    }

    /* Update the machine, the *pAppCtx, and *pCliHndl */
    if (SOPC_STATUS_OK == status)
    {
        pSM->state = stCreatingMonIt;
        if (NULL != pAppCtx)
        {
            *pAppCtx = iCliHndl;
        }
        *pCliHndl = iCliHndl;
    }
    else
    {
        SOPC_Encodeable_Delete(&OpcUa_CreateMonitoredItemsRequest_EncodeableType, (void**) &pReq);
    }

    free(pNid);
    pNid = NULL;

    mutStatus = Mutex_Unlock(&pSM->mutex);
    assert(SOPC_STATUS_OK == mutStatus);

    return status;
}

bool SOPC_StaMac_IsConnectable(SOPC_StaMac_Machine* pSM)
{
    if (NULL == pSM)
    {
        return false;
    }

    SOPC_ReturnStatus status = Mutex_Lock(&pSM->mutex);
    assert(SOPC_STATUS_OK == status);
    bool return_code = (stInit == pSM->state);
    status = Mutex_Unlock(&pSM->mutex);
    assert(SOPC_STATUS_OK == status);

    return return_code;
}

bool SOPC_StaMac_IsConnected(SOPC_StaMac_Machine* pSM)
{
    if (NULL == pSM)
    {
        return false;
    }

    SOPC_ReturnStatus status = Mutex_Lock(&pSM->mutex);
    assert(SOPC_STATUS_OK == status);

    bool bConnected = false;
    switch (pSM->state)
    {
    case stActivated:
    case stCreatingSubscr:
    case stCreatingMonIt:
    case stCreatingPubReq:
    case stClosing:
        bConnected = true;
        break;
    default:
        break;
    }

    status = Mutex_Unlock(&pSM->mutex);
    assert(SOPC_STATUS_OK == status);

    return bConnected;
}

bool SOPC_StaMac_IsError(SOPC_StaMac_Machine* pSM)
{
    if (NULL == pSM)
    {
        return false;
    }

    SOPC_ReturnStatus status = Mutex_Lock(&pSM->mutex);
    assert(SOPC_STATUS_OK == status);
    bool return_code = (stError == pSM->state);
    status = Mutex_Unlock(&pSM->mutex);
    assert(SOPC_STATUS_OK == status);

    return return_code;
}

bool SOPC_StaMac_HasSubscription(SOPC_StaMac_Machine* pSM)
{
    if (NULL == pSM)
    {
        return false;
    }

    SOPC_ReturnStatus status = Mutex_Lock(&pSM->mutex);
    assert(SOPC_STATUS_OK == status);
    bool return_code = (0 != pSM->iSubscriptionID);
    status = Mutex_Unlock(&pSM->mutex);
    assert(SOPC_STATUS_OK == status);

    return return_code;
}

bool SOPC_StaMac_HasMonItByAppCtx(SOPC_StaMac_Machine* pSM, uintptr_t appCtx)
{
    if (NULL == pSM || NULL == pSM->pListMonIt)
    {
        return false;
    }

    SOPC_ReturnStatus status = Mutex_Lock(&pSM->mutex);
    assert(SOPC_STATUS_OK == status);
    bool bHasMonIt = false;
    SOPC_SLinkedListIterator pIter = NULL;
    pIter = SOPC_SLinkedList_GetIterator(pSM->pListMonIt);

    while (!bHasMonIt && NULL != pIter)
    {
        if (SOPC_SLinkedList_Next(&pIter) == (void*) appCtx)
        {
            bHasMonIt = true;
        }
    }

    status = Mutex_Unlock(&pSM->mutex);
    assert(SOPC_STATUS_OK == status);

    return bHasMonIt;
}

bool SOPC_StaMac_EventDispatcher(SOPC_StaMac_Machine* pSM,
                                 uintptr_t* pAppCtx,
                                 SOPC_App_Com_Event event,
                                 uint32_t arg,
                                 void* pParam,
                                 uintptr_t appCtx)
{
    bool bProcess = false;
    uintptr_t intAppCtx = 0; /* Internal appCtx, the one wrapped in the (ReqCtx*)appCtx */
    SOPC_StaMac_State oldState = stError;

    SOPC_ReturnStatus mutStatus = Mutex_Lock(&pSM->mutex);
    assert(SOPC_STATUS_OK == mutStatus);
    bProcess = StaMac_IsEventTargeted(pSM, &intAppCtx, event, arg, pParam, appCtx);

    if (bProcess)
    {
        oldState = pSM->state;
        if (NULL != pAppCtx)
        {
            *pAppCtx = intAppCtx;
        }

        /* Treat event, if needed */
        switch (pSM->state)
        {
        /* Session states */
        case stActivating:
            StaMac_ProcessEvent_stActivating(pSM, event, arg, pParam, intAppCtx);
            break;
        case stClosing:
            StaMac_ProcessEvent_stClosing(pSM, event, arg, pParam, intAppCtx);
            break;
        /* Main state */
        case stActivated:
            StaMac_ProcessEvent_stActivated(pSM, event, arg, pParam, intAppCtx);
            break;
        /* Creating* states */
        case stCreatingSubscr:
            StaMac_ProcessEvent_stCreatingSubscr(pSM, event, arg, pParam, intAppCtx);
            break;
        case stCreatingMonIt:
            StaMac_ProcessEvent_stCreatingMonIt(pSM, event, arg, pParam, intAppCtx);
            break;
        case stCreatingPubReq:
            /* TODO: remove this non existing state */
            break;
        /* Invalid states */
        case stInit:
            pSM->state = stError;
            Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "Event received in stInit state.");
            break;
        case stError:
            StaMac_ProcessEvent_stError(pSM, event, arg, pParam, intAppCtx);
            break;
        default:
            pSM->state = stError;
            Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "Dispatching in unknown state %i, event %i.", pSM->state, event);
            break;
        }

        StaMac_PostProcessActions(pSM, oldState);
    }

    mutStatus = Mutex_Unlock(&pSM->mutex);
    assert(SOPC_STATUS_OK == mutStatus);

    return bProcess;
}

/* ========================
 * Internals implementation
 * ========================
 */

/**
 * \brief Tells whether the event is targeted to this machine.
 *
 * It also unwraps the appCtx if it is a SOPC_StaMac_ReqCtx*, and set *pAppCtx to pReqCtx->appCtx.
 * Machine's mutex shall already be locked by the caller.
 */
static bool StaMac_IsEventTargeted(SOPC_StaMac_Machine* pSM,
                                   uintptr_t* pAppCtx,
                                   SOPC_App_Com_Event event,
                                   uint32_t arg,
                                   void* pParam,
                                   uintptr_t appCtx)
{
    /* avoid unused parameter compiler warning */
    (void) arg;
    (void) pParam;

    bool bProcess = true;
    SOPC_SLinkedListIterator pListIter = NULL;

    if (NULL == pSM)
    {
        return false;
    }

    /* As long as the appCtx is not surely a SOPC_StaMac_ReqCtx*, it is not possible to dereference it.
     * But it contains the uid of the request, which is the id of the pListReqCtx.
     * The appCtx is a uintptr_t, so it cannot be set as the id of the SOPC_SLinkedList,
     * and searched for easily. */

    /* Depending on the event, check either by request ctx or session ctx. */
    switch (event)
    {
    /* appCtx is request context */
    case SE_RCV_SESSION_RESPONSE:
    case SE_RCV_DISCOVERY_RESPONSE:
    case SE_SND_REQUEST_FAILED:
        bProcess = false;
        pListIter = SOPC_SLinkedList_GetIterator(pSM->pListReqCtx);
        while (!bProcess && NULL != pListIter)
        {
            if ((uintptr_t) SOPC_SLinkedList_Next(&pListIter) == appCtx)
            {
                bProcess = true;
                /* A response with a known pReqCtx shall free it, and return the appCtx to the caller */
                if (NULL != pAppCtx)
                {
                    *pAppCtx = ((SOPC_StaMac_ReqCtx*) appCtx)->appCtx;
                }
                if (SOPC_SLinkedList_RemoveFromId(pSM->pListReqCtx, ((SOPC_StaMac_ReqCtx*) appCtx)->uid) == NULL)
                {
                    Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_WARNING, "failed to pop the request from the pListReqCtx.");
                }
                free((void*) appCtx);
                appCtx = 0;
            }
        }
        break;
    /* appCtx is session context */
    case SE_SESSION_ACTIVATION_FAILURE:
    case SE_ACTIVATED_SESSION:
    case SE_SESSION_REACTIVATING:
    case SE_CLOSED_SESSION:
        if (pSM->iSessionCtx != appCtx)
        {
            bProcess = false;
        }
        break;
    default:
        bProcess = false;
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "Unexpected event received by a machine.");
        break;
    }

    return bProcess;
}

static void StaMac_ProcessEvent_stActivating(SOPC_StaMac_Machine* pSM,
                                             SOPC_App_Com_Event event,
                                             uint32_t arg,
                                             void* pParam,
                                             uintptr_t appCtx)
{
    (void) (pParam);
    (void) (appCtx);

    switch (event)
    {
    case SE_ACTIVATED_SESSION:
        pSM->state = stActivated;
        pSM->iSessionID = arg;
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_INFO, "Session activated.");
        break;
    case SE_SESSION_ACTIVATION_FAILURE:
        pSM->state = stError;
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "Failed session activation.");
        break;
    default:
        pSM->state = stError;
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "In state Activation, unexpected event %i.", event);
        break;
    }
}

static void StaMac_ProcessEvent_stClosing(SOPC_StaMac_Machine* pSM,
                                          SOPC_App_Com_Event event,
                                          uint32_t arg,
                                          void* pParam,
                                          uintptr_t appCtx)
{
    (void) (arg);
    (void) (pParam);
    (void) (appCtx);

    switch (event)
    {
    case SE_SESSION_ACTIVATION_FAILURE:
    case SE_CLOSED_SESSION:
        /* Put the machine in a correct closed state, events may still be received */
        pSM->state = stError;
        break;
    case SE_SESSION_REACTIVATING:
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_WARNING, "Ignoring reactivation notification in stClosing state.");
        break;
    case SE_SND_REQUEST_FAILED:
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_WARNING, "Ignoring send request failure notification in stClosing state.");
        break;
    default:
        /* This might be a response to a pending request, so this might not an error */
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_WARNING, "Unexpected event %i in stClosing state, ignoring.", event);
        break;
    }
}

static void StaMac_ProcessEvent_stActivated(SOPC_StaMac_Machine* pSM,
                                            SOPC_App_Com_Event event,
                                            uint32_t arg,
                                            void* pParam,
                                            uintptr_t appCtx)
{
    (void) (arg);

    SOPC_ReturnStatus status = SOPC_STATUS_OK;
    int32_t i = 0;
    SOPC_EncodeableType* pEncType = NULL;
    OpcUa_PublishResponse* pPubResp = NULL;
    OpcUa_NotificationMessage* pNotifMsg = NULL;
    OpcUa_DataChangeNotification* pDataNotif = NULL;
    OpcUa_MonitoredItemNotification* pMonItNotif = NULL;
    SOPC_LibSub_Value* plsVal = NULL;

    switch (event)
    {
    case SE_RCV_SESSION_RESPONSE:
        /* There should be an EncodeableType pointer in the first field of the message struct */
        pEncType = *(SOPC_EncodeableType**) pParam;
        if (&OpcUa_PublishResponse_EncodeableType == pEncType)
        {
            Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_INFO, "PublishResponse received.");
            pPubResp = (OpcUa_PublishResponse*) pParam;
            /* Take note to acknowledge later. There is no ack with KeepAlive. */
            /* TODO: this limits the benefits of having multiple pending PublishRequest, maybe
             * it would be more appropriate to have a list of SeqNumbsToAck... */
            assert(!pSM->bAckSubscr);
            if (0 < pPubResp->NoOfAvailableSequenceNumbers)
            {
                pSM->bAckSubscr = true;
                /* Only take the last one when more than 1 is available */
                pSM->iAckSeqNum = pPubResp->AvailableSequenceNumbers[pPubResp->NoOfAvailableSequenceNumbers - 1];
            }
            pSM->nTokenUsable -= 1;
            /* Traverse the notifications and calls the callback */
            pNotifMsg = &pPubResp->NotificationMessage;
            /* For now, only handles at most a NotificationData */
            assert(1 >= pNotifMsg->NoOfNotificationData);
            if (0 < pNotifMsg->NoOfNotificationData)
            {
                assert(&OpcUa_DataChangeNotification_EncodeableType ==
                       pNotifMsg->NotificationData[0].Body.Object.ObjType);
                pDataNotif = (OpcUa_DataChangeNotification*) pNotifMsg->NotificationData[0].Body.Object.Value;
                for (i = 0; i < pDataNotif->NoOfMonitoredItems; ++i)
                {
                    pMonItNotif = &pDataNotif->MonitoredItems[i];
                    status = Helpers_NewValueFromDataValue(&pMonItNotif->Value, &plsVal);
                    if (SOPC_STATUS_OK == status)
                    {
                        pSM->cbkDataChanged(pSM->iCliId, pMonItNotif->ClientHandle, plsVal);
                        free(plsVal->value);
                        free(plsVal);
                    }
                }
            }
            /* TODO: verify the results[] which contains a status for each Ack */
        }
        else
        {
            Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_INFO, "Unknown response received.");
        }
        break;
    case SE_SND_REQUEST_FAILED:
        pSM->state = stError;
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "Send request failed, type %s, context 0x%" PRIxPTR ".",
                    ((SOPC_EncodeableType*) pParam)->TypeName, appCtx);
        break;
    case SE_SESSION_REACTIVATING:
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_INFO, "Session reactivated.");
        break;
    default:
        pSM->state = stError;
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "In state stActivated, unexpected event %i.", event);
        break;
    }
}

static void StaMac_ProcessEvent_stCreatingSubscr(SOPC_StaMac_Machine* pSM,
                                                 SOPC_App_Com_Event event,
                                                 uint32_t arg,
                                                 void* pParam,
                                                 uintptr_t appCtx)
{
    (void) (arg);
    (void) (appCtx);

    switch (event)
    {
    case SE_RCV_SESSION_RESPONSE:
        /* TODO: verify revised values?? */
        assert(pSM->iSubscriptionID == 0);
        pSM->iSubscriptionID = ((OpcUa_CreateSubscriptionResponse*) pParam)->SubscriptionId;
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_INFO, "Subscription created.");
        pSM->state = stActivated;
        break;
    case SE_SND_REQUEST_FAILED:
        pSM->state = stError;
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "Send subscription request failed.");
        break;
    default:
        pSM->state = stError;
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "In state stCreatingSubscr, unexpected event %i.", event);
        break;
    }
}

static void StaMac_ProcessEvent_stCreatingMonIt(SOPC_StaMac_Machine* pSM,
                                                SOPC_App_Com_Event event,
                                                uint32_t arg,
                                                void* pParam,
                                                uintptr_t appCtx)
{
    (void) (arg);

    SOPC_ReturnStatus status = SOPC_STATUS_OK;
    int32_t i = 0;
    OpcUa_CreateMonitoredItemsResponse* pMonItResp = NULL;

    switch (event)
    {
    case SE_RCV_SESSION_RESPONSE:
        /* There should be only one result element */
        pMonItResp = (OpcUa_CreateMonitoredItemsResponse*) pParam;
        assert(NULL != pMonItResp);
        for (i = 0; SOPC_STATUS_OK == status && i < pMonItResp->NoOfResults; ++i)
        {
            /* TODO: verify revised values?? */
            if (0 != pMonItResp->Results[i].StatusCode) /* OpcUa_Good does not exist... */
            {
                status = SOPC_STATUS_NOK;
                Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR,
                            "Server could not create monitored item, sc = 0x%08" PRIX32 ".",
                            pMonItResp->Results[i].StatusCode);
            }
            else
            {
                if (SOPC_SLinkedList_Append(pSM->pListMonIt, pMonItResp->Results[i].MonitoredItemId, (void*) appCtx) !=
                    (void*) appCtx)
                {
                    status = SOPC_STATUS_NOK;
                }
            }
            if (SOPC_STATUS_OK == status)
            {
                Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_INFO, "MonitoredItem created.");
            }
        }
        if (SOPC_STATUS_OK == status)
        {
            pSM->state = stActivated;
        }
        else
        {
            pSM->state = stError;
        }
        break;
    case SE_SND_REQUEST_FAILED:
        pSM->state = stError;
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "Send create monitored items request failed.");
        break;
    default:
        pSM->state = stError;
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_ERROR, "In state stCreatingMonIt, unexpected event %i.", event);
        break;
    }
}

static void StaMac_ProcessEvent_stError(SOPC_StaMac_Machine* pSM,
                                        SOPC_App_Com_Event event,
                                        uint32_t arg,
                                        void* pParam,
                                        uintptr_t appCtx)
{
    (void) (pSM);
    (void) (arg);
    (void) (appCtx);

    switch (event)
    {
    case SE_SESSION_ACTIVATION_FAILURE:
    case SE_CLOSED_SESSION:
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_INFO, "Received post-closed closed event or activation failure, ignored.");
        break;
    case SE_SND_REQUEST_FAILED:
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_INFO, "Received post-closed send request failure for message of type %s.",
                    ((SOPC_EncodeableType*) pParam)->TypeName);
        break;
    case SE_SESSION_REACTIVATING:
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_INFO,
                    "Reactivating event received, but closed connection are considered lost.");
        break;
    default:
        Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_WARNING, "Receiving unexpected event %i in closed state, ignored.", event);
        break;
    }
}

/**
 * \brief Do the post process actions: create a subscription, keep the target number of PublishRequest, ...
 *
 * Machine's mutex shall already be locked by the caller.
 */
static void StaMac_PostProcessActions(SOPC_StaMac_Machine* pSM, SOPC_StaMac_State oldState)
{
    assert(NULL != pSM);

    SOPC_ReturnStatus status = SOPC_STATUS_OK;
    void* pRequest = NULL;

    switch (pSM->state)
    {
    /* Mostly when stActivated is reached */
    case stActivated:
        /* First, create the subscription */
        if (0 == pSM->iSubscriptionID)
        {
            /* Creates the subscription */
            /* The request is freed by the Toolkit */
            /* TODO: make all value configurable */
            Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_INFO, "Creating subscription.");
            status = Helpers_NewCreateSubscriptionRequest(pSM->fPublishInterval, pSM->iCntMaxKeepAlive,
                                                          pSM->iCntLifetime, &pRequest);
            if (SOPC_STATUS_OK == status)
            {
                status = SOPC_StaMac_SendRequest(pSM, pRequest, 0, SOPC_REQUEST_SCOPE_STATE_MACHINE);
            }
            if (SOPC_STATUS_OK == status)
            {
                pSM->state = stCreatingSubscr;
            }
            else
            {
                pSM->state = stError;
            }
        }
        /* Then add tokens, but wait for at least a monitored item */
        else if (pSM->nTokenUsable < pSM->nTokenTarget)
        {
            while (SOPC_STATUS_OK == status && pSM->nTokenUsable < pSM->nTokenTarget)
            {
                /* Send a PublishRequest */
                Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_INFO, "Adding publish token.");
                status = Helpers_NewPublishRequest(pSM->bAckSubscr, pSM->iSubscriptionID, pSM->iAckSeqNum, &pRequest);
                if (SOPC_STATUS_OK == status)
                {
                    if (nPublishReqs < UINTPTR_MAX)
                    {
                        /* TODO: This addition is not atomic */
                        uintptr_t new_val = (uintptr_t)(SOPC_Atomic_Ptr_Get((void**) &nPublishReqs)) + 1;
                        SOPC_Atomic_Ptr_Set((void**) &nPublishReqs, (void*) new_val);
                        status = SOPC_StaMac_SendRequest(pSM, pRequest, nPublishReqs, SOPC_REQUEST_SCOPE_STATE_MACHINE);
                    }
                    else
                    {
                        status = SOPC_STATUS_INVALID_STATE;
                    }
                }
                if (SOPC_STATUS_OK == status)
                {
                    /* This is the reason nTokenUsable and nTokenTarget are uint32_t: uint16_t arithmetics
                     * would raise a warning, as 1 cannot be interpreted as a uint16_t... */
                    pSM->nTokenUsable += 1;
                    pSM->bAckSubscr = false;
                }
                else
                {
                    pSM->state = stError;
                }
            }
        }
        break;
    /* Try to send a close session if the session was connected before the error */
    case stError:
        if (stError != oldState && stClosing != oldState)
        {
            pSM->state = oldState;
            if (SOPC_StaMac_IsConnected(pSM))
            {
                status = SOPC_StaMac_StopSession(pSM);
                if (SOPC_STATUS_OK != status)
                {
                    pSM->state = stError;
                }
                else
                {
                    Helpers_Log(SOPC_TOOLKIT_LOG_LEVEL_INFO, "Closing the connection because of the previous error.");
                }
            }
            else
            {
                pSM->state = stError;
            }
        }
        break;
    default:
        break;
    }
}
