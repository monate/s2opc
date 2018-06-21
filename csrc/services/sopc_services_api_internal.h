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

/**
 *  \file sopc_sockets_api_internal.h
 *
 *  \brief Event oriented API of the Services layer for internal use only (from Services layer).
 */

#ifndef SOPC_SERVICES_API_INTERNAL_H
#define SOPC_SERVICES_API_INTERNAL_H

#include "sopc_services_api.h"

/* API to enqueue an internal event for services */
void SOPC_Services_InternalEnqueueEvent(SOPC_Services_Event seEvent, uint32_t id, void* params, uintptr_t auxParam);

/* API to enqueue an internal event in priority for services */
void SOPC_Services_InternalEnqueuePrioEvent(SOPC_Services_Event seEvent, uint32_t id, void* params, uintptr_t auxParam);

// Internal use only (timers)
SOPC_EventDispatcherManager* SOPC_Services_GetEventDispatcher(void);

typedef struct SOPC_Internal_AsyncSendMsgData
{
    uint32_t requestId;     // t_request_context
    uint32_t requestHandle; // t_request_handle
    void* msgToSend;        // OpcUa_<Msg> *
} SOPC_Internal_AsyncSendMsgData;

#endif /* SOPC_SERVICES_API_INTERNAL_H */
