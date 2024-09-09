/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 #include "legato.h"
#include "interfaces.h"
#include "tafDiagDoIPSvr.hpp"
#include "tafDoIPStack.h"

using namespace telux::tafsvc;

taf_DiagDoIPSvr& taf_DiagDoIPSvr::GetInstance
(
)
{
    static taf_DiagDoIPSvr instance;

    return instance;
}

void taf_DiagDoIPSvr::Init
(
)
{
    // Create memory pools.
    svcPool = le_mem_CreatePool("DoIPSvcPool", sizeof(taf_DoIPSVC_t));
    sessPool = le_mem_CreatePool("DoIPSessPool", sizeof(taf_DoIPSession_t));

    // Create reference maps
    svcRefMap = le_ref_CreateMap("DoIPSvcRefMap", DEFAULT_SVC_REF_CNT);
    sessRefMap = le_ref_CreateMap("DoIPSessRefMap", DEFAULT_SESSION_REF_CNT);

    // Create client session close hander.
    le_msg_AddServiceCloseHandler(taf_diagDoIP_GetServiceRef(),
        taf_DiagDoIPSvr::OnClientDisconnection, NULL);

    LE_INFO("Diag DoIP service initialization successful in thread%p!", le_thread_GetCurrent());
}

taf_diagDoIP_ServiceRef_t taf_DiagDoIPSvr::FindOrCreateService
(
    uint16_t identifier
)
{
    taf_DoIPSVC_t *servicePtr = GetServiceObj(identifier);
    if (servicePtr != nullptr)
    {
        goto out;
    }

    servicePtr = (taf_DoIPSVC_t*)le_mem_ForceAlloc(svcPool);
    memset(servicePtr, 0, sizeof(taf_DoIPSVC_t));

    servicePtr->ref = (taf_diagDoIP_ServiceRef_t)le_ref_CreateRef(svcRefMap, servicePtr);
    servicePtr->sessionList = LE_DLS_LIST_INIT;
    servicePtr->id = identifier;
    servicePtr->doipRef = taf_doip_Get(identifier);

    servicePtr->doiphandlerRef = taf_doip_AddEventHandler(
        servicePtr->doipRef, DoIPEventHandler, NULL);

    LE_INFO("DoIP service: serviceRef%p of client%p is created",
        servicePtr->ref, taf_diagDoIP_GetClientSessionRef());

out:
    AddSessionToService(servicePtr, taf_diagDoIP_GetClientSessionRef());
    return servicePtr->ref;
}

le_result_t taf_DiagDoIPSvr::RemoveService
(
    taf_diagDoIP_ServiceRef_t svcRef
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, LE_FAULT, "svcRef is null");
    le_msg_SessionRef_t sessionRef = taf_diagDoIP_GetClientSessionRef();

    le_ref_IterRef_t iterRef = le_ref_GetIterator(svcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_DoIPSVC_t* svcPtr = (taf_DoIPSVC_t*)le_ref_GetValue(iterRef);
        if (svcPtr == NULL || svcPtr->ref != svcRef)
        {
            continue;
        }

        RemoveSessionFromService(svcPtr, sessionRef);

        //If session number of links is 0, release DTC context
        if( le_dls_NumLinks(&svcPtr->sessionList) == 0)
        {
            // Clear service object
            LE_INFO("Clear service object%p", svcPtr);
            taf_doip_RemoveEventHandler(svcPtr->doiphandlerRef);
            le_ref_DeleteRef(svcRefMap, (void*)svcPtr->ref);
            le_mem_Release(svcPtr);
        }
    }

    return LE_OK;
}

taf_diagDoIP_EventHandlerRef_t taf_DiagDoIPSvr::AddEventHandler
(
    taf_diagDoIP_ServiceRef_t svcRef,
    taf_diagDoIP_EventHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    TAF_ERROR_IF_RET_VAL(svcRef == NULL, NULL, "Null ptr(svcRef)");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Null ptr(handlerPtr)");

    taf_DoIPSVC_t *servicePtr = (taf_DoIPSVC_t*)le_ref_Lookup(svcRefMap, svcRef);
    if (servicePtr == NULL)
    {
        LE_ERROR("svcRef invalid");
        return NULL;
    }

    taf_DoIPSession_t *sessionPtr = AddSessionToService(servicePtr,
        taf_diagDoIP_GetClientSessionRef());
    if (sessionPtr == NULL)
    {
        LE_ERROR("Cannot find session object");
        return NULL;
    }
    else if (sessionPtr->func != NULL)
    {
        LE_ERROR("Event handler has been set");
        return NULL;
    }

    SetSessionEventHandler(sessionPtr, handlerPtr, contextPtr);

    LE_INFO("Registered event handler to DoIP service");

    return (taf_diagDoIP_EventHandlerRef_t)sessionPtr->safeRef;
}

void taf_DiagDoIPSvr::RemoveEventHandler
(
    taf_diagDoIP_EventHandlerRef_t handlerRef
)
{
    taf_DoIPSession_t *sessPtr;

    sessPtr = (taf_DoIPSession_t*)le_ref_Lookup(sessRefMap, handlerRef);
    if (sessPtr == NULL)
    {
        LE_ERROR("Invalid reference");
        return;
    }

    if (sessPtr->sessionRef != taf_diagDoIP_GetClientSessionRef())
    {
        LE_WARN("Remove handler in another session");
    }

    SetSessionEventHandler(sessPtr, NULL, NULL);
}

le_result_t taf_DiagDoIPSvr::SetVIN
(
    const char* vin
)
{
    return taf_doip_SetVin(vin);
}

le_result_t taf_DiagDoIPSvr::GetVIN
(
    char* vin,
    size_t vinSize
)
{
    return taf_doip_GetVin(vin, vinSize);
}

le_result_t taf_DiagDoIPSvr::SetEID
(
    const char* eid
)
{
    return taf_doip_SetEid(eid);
}

le_result_t taf_DiagDoIPSvr::GetEID
(
    char* eid,
    size_t eidSize
)
{
    return taf_doip_GetEid(eid, eidSize);
}

le_result_t taf_DiagDoIPSvr::SetGID
(
    const char* gid
)
{
    return taf_doip_SetGid(gid);
}

le_result_t taf_DiagDoIPSvr::GetGID
(
    char* gid,
    size_t gidSize
)
{
    return taf_doip_GetGid(gid, gidSize);
}

void taf_DiagDoIPSvr::DoIPEventHandler
(
    taf_doip_Ref_t doipRef,
    taf_doip_Event_t event,
    uint16_t remoteAddr,
    void* userPtr
)
{
    taf_diagDoIP_EventType_t eventType;

    switch (event)
    {
        case TAF_DOIP_EVENT_CONNECTION:
            eventType = TAF_DIAGDOIP_CONNECTION;
            break;
        case TAF_DOIP_EVENT_DISCONNECTION:
            eventType = TAF_DIAGDOIP_DISCONNECTION;
            break;
        default:
            // No need to report.
            return;
    }

    taf_DiagDoIPSvr& doipSvr = taf_DiagDoIPSvr::GetInstance();

    le_ref_IterRef_t iterRef = le_ref_GetIterator(doipSvr.svcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_DoIPSVC_t* svcPtr = (taf_DoIPSVC_t*)le_ref_GetValue(iterRef);
        if (svcPtr != NULL && svcPtr->doipRef == doipRef)
        {
            le_dls_Link_t* linkPtr = NULL;
            linkPtr = le_dls_Peek(&(svcPtr->sessionList));
            while (linkPtr)
            {
                // Loop to report to all clients.
                taf_DoIPSession_t* sessionPtr = CONTAINER_OF(linkPtr, taf_DoIPSession_t, link);
                linkPtr = le_dls_PeekNext(&(svcPtr->sessionList), linkPtr);
                if (sessionPtr->func != NULL)
                {
                    // Report to the application
                    sessionPtr->func(svcPtr->ref, eventType, remoteAddr, sessionPtr->ctxPtr);
                }
            }
        }
    }
}

void taf_DiagDoIPSvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    taf_DiagDoIPSvr& doipSvr = taf_DiagDoIPSvr::GetInstance();

    le_ref_IterRef_t iterRef = le_ref_GetIterator(doipSvr.svcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_DoIPSVC_t* svcPtr = (taf_DoIPSVC_t*)le_ref_GetValue(iterRef);
        if (svcPtr == NULL)
        {
            continue;
        }

        doipSvr.RemoveSessionFromService(svcPtr, sessionRef);

        //If session number of links is 0, release DTC context
        if( le_dls_NumLinks(&svcPtr->sessionList) == 0)
        {
            // Clear service object
            LE_INFO("Clear service object%p", svcPtr);
            le_ref_DeleteRef(doipSvr.svcRefMap, (void*)svcPtr->ref);
            le_mem_Release(svcPtr);
        }
    }
}

taf_DoIPSVC_t* taf_DiagDoIPSvr::GetServiceObj
(
    uint16_t identifier
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(svcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_DoIPSVC_t* servicePtr = (taf_DoIPSVC_t*)le_ref_GetValue(iterRef);
        if (servicePtr != NULL && servicePtr->id == identifier)
        {
            return servicePtr;
        }
    }

    return NULL;
}

taf_DoIPSession_t *taf_DiagDoIPSvr::AddSessionToService
(
    taf_DoIPSVC_t* servicePtr,
    le_msg_SessionRef_t sessionRef
)
{
    le_dls_Link_t* linkPtr = NULL;

    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, NULL, "service is NULL");

    linkPtr = le_dls_Peek(&(servicePtr->sessionList));
    while (linkPtr)
    {
        taf_DoIPSession_t* sessionPtr = CONTAINER_OF(linkPtr, taf_DoIPSession_t, link);
        linkPtr = le_dls_PeekNext(&(servicePtr->sessionList), linkPtr);

        if (sessionPtr->sessionRef == sessionRef)
        {
            LE_DEBUG("Session(%p) has been added to service", sessionRef);
            return sessionPtr;
        }
    }

    LE_DEBUG("add session %p for service%p", sessionRef, servicePtr);
    taf_DoIPSession_t* newSessionPtr = (taf_DoIPSession_t *)le_mem_ForceAlloc(sessPool);

    newSessionPtr->sessionRef = sessionRef;
    newSessionPtr->link = LE_DLS_LINK_INIT;
    newSessionPtr->svrPtr = servicePtr;
    newSessionPtr->func = NULL;
    newSessionPtr->ctxPtr = NULL;
    newSessionPtr->safeRef = le_ref_CreateRef(sessRefMap, newSessionPtr);
    le_dls_Queue(&servicePtr->sessionList, &(newSessionPtr->link));

    return newSessionPtr;
}

le_result_t taf_DiagDoIPSvr::RemoveSessionFromService
(
    taf_DoIPSVC_t* servicePtr,
    le_msg_SessionRef_t sessionRef
)
{
    le_dls_Link_t* linkPtr = NULL;

    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "service is NULL");

    linkPtr = le_dls_Peek(&(servicePtr->sessionList));
    while (linkPtr)
    {
        taf_DoIPSession_t* sessionPtr = CONTAINER_OF(linkPtr, taf_DoIPSession_t, link);
        linkPtr = le_dls_PeekNext(&(servicePtr->sessionList), linkPtr);

        if (sessionPtr->sessionRef == sessionRef)
        {
            LE_DEBUG("remove ref(%p) from service%p", sessionRef, servicePtr);
            le_dls_Remove(&(servicePtr->sessionList), &(sessionPtr->link));
            le_ref_DeleteRef(sessRefMap, sessionPtr->safeRef);
            le_mem_Release(sessionPtr);
            return LE_OK;
        }
    }

    LE_DEBUG("Cannot found session with ref(%p) from service%p",
        sessionRef, servicePtr);

    return LE_NOT_FOUND;
}

le_result_t taf_DiagDoIPSvr::SetSessionEventHandler
(
    taf_DoIPSession_t *sessionPtr,
    taf_diagDoIP_EventHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    if (sessionPtr->func != NULL)
    {
        LE_ERROR("Session event handler has been set");
        return LE_DUPLICATE;
    }

    sessionPtr->func = handlerPtr;
    sessionPtr->ctxPtr = contextPtr;

    return LE_OK;
}