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

#include "wrapper_proxystub.h"

#include "sopc_stack_config.h"
#include "sopc_threads.h"
#include "sopc_mutexes.h"
#include "sopc_sockets.h"

void* OpcUa_ProxyStub_g_PlatformLayerCalltable = NULL;
void* OpcUa_ProxyStub_g_Configuration = NULL;

static SOPC_NamespaceTable* gNsTable = NULL;

static struct
{
    uint8_t  initDone;
    uint8_t  stopFlag;
    Mutex    tMutex;
    Thread   thread;
} receptionThread = {
    .initDone = FALSE,
    .stopFlag = FALSE,
};

void* OpcUa_ThreadStartReception(void* nullData){
    (void) nullData;
    const uint32_t sleepTimeout = 500;
    SOPC_StatusCode status = STATUS_OK;
    Mutex_Lock(&receptionThread.tMutex);
    while(STATUS_OK == status && receptionThread.stopFlag == FALSE){
        Mutex_Unlock(&receptionThread.tMutex);
        status = SOPC_SocketManager_Loop(SOPC_SocketManager_GetGlobal(), sleepTimeout);
        SOPC_Sleep(50);
        Mutex_Lock(&receptionThread.tMutex);
    }
    Mutex_Unlock(&receptionThread.tMutex);
    return NULL;
}

void OpcUa_ReceptionThread_Start(){
    if(receptionThread.initDone == FALSE){
        Mutex_Inititalization(&receptionThread.tMutex);
        Mutex_Lock(&receptionThread.tMutex);
        receptionThread.initDone = 1;
        SOPC_Thread_Create(&receptionThread.thread, OpcUa_ThreadStartReception, NULL);
        Mutex_Unlock(&receptionThread.tMutex);
    }

}

void OpcUa_ReceptionThread_Stop(){
    if(receptionThread.initDone != FALSE){
        Mutex_Lock(&receptionThread.tMutex);
        // stop the reception thread
        receptionThread.stopFlag = 1;
        Mutex_Unlock(&receptionThread.tMutex);
        SOPC_Thread_Join(receptionThread.thread);
    }
}


SOPC_StatusCode OpcUa_ProxyStub_Initialize(void* pCalltable,
                                           void* pConfig)
{
    SOPC_StatusCode status = STATUS_OK;
    gNsTable = NULL;
    OpcUa_ProxyStub_g_PlatformLayerCalltable = NULL;
    OpcUa_ProxyStub_g_Configuration = NULL;
    if(pCalltable != NULL){
        OpcUa_ProxyStub_g_PlatformLayerCalltable = pCalltable;
    }
    OpcUa_ProxyStub_g_Configuration = pConfig;
    status = StackConfiguration_Initialize();
#ifdef WRAPPER_RECEPTION_THREAD
    OpcUa_ReceptionThread_Start();
#endif
    return status;
}

void OpcUa_ProxyStub_Clear(void)
{
    if(gNsTable != NULL){
        Namespace_Delete(gNsTable);
        gNsTable = NULL;
    }
    StackConfiguration_Clear();
#ifdef WRAPPER_RECEPTION_THREAD
    OpcUa_ReceptionThread_Stop();
#endif
}

SOPC_StatusCode OpcUa_ProxyStub_ReInitialize(void* pConfig){
    // Clear without thread stop
    if(gNsTable != NULL){
        Namespace_Delete(gNsTable);
        gNsTable = NULL;
    }
    StackConfiguration_Clear();
    return OpcUa_ProxyStub_Initialize(OpcUa_ProxyStub_g_PlatformLayerCalltable, pConfig);
}

SOPC_StatusCode OpcUa_ProxyStub_AddTypes(SOPC_EncodeableType** types)
{
    uint32_t idx = 0;
    SOPC_EncodeableType* encType = types[idx];
    for(idx = 1; idx < UINT32_MAX && encType != NULL; idx++){
        encType = types[idx];
    }
    return StackConfiguration_AddTypes(types, idx);
}

SOPC_StatusCode OpcUa_ProxyStub_SetNamespaceUris(char** namespaceUris)
{
    if(gNsTable != NULL){
        return STATUS_NOK;
    }
    // count namespaces
    char* ns = NULL;
    uint32_t idx = 0;
    for(idx = 0; idx < UINT32_MAX && ns != NULL; idx++){
        ns = namespaceUris[idx];
    }
    gNsTable = Namespace_CreateTable(idx);
    for(idx = 0; idx < UINT32_MAX && ns != NULL; idx++){
        gNsTable->namespaceArray[idx].namespaceIndex = idx;
        gNsTable->namespaceArray[idx].namespaceName = namespaceUris[idx];
    }
    return StackConfiguration_SetNamespaceUris(gNsTable);
}

char* OpcUa_ProxyStub_GetVersion(){
    return "INGOPCS";
}

char* OpcUa_ProxyStub_GetConfigString(){
    return "INGOPCS";
}

char* OpcUa_ProxyStub_GetStaticConfigString(){
    return "INGOPCS";
}
