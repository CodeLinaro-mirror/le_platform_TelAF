/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 #include "legato.h"
#include "interfaces.h"
#include "tafDiagDoIPSvr.hpp"
#include "tafDoIPStack.h"

using namespace tafsvc;

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
    vlanPool = le_mem_CreatePool("DoIPVlanPool", sizeof(taf_VlanCallback_t));

    // Create reference maps
    svcRefMap = le_ref_CreateMap("DoIPSvcRefMap", DEFAULT_SVC_REF_CNT);
    vlanRefMap = le_ref_CreateMap("DoIPVlanRefMap", DEFAULT_VLAN_REF_CNT);

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
    uint16_t vlanId,
    taf_diagDoIP_EventHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    le_result_t ret;

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

    ret = SetSessionEventHandler(sessionPtr, vlanId, handlerPtr, contextPtr);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set event handler");
        return NULL;
    }

    LE_INFO("Registered event handler to DoIP service");
    taf_VlanCallback_t* vlanPtr = GetVlanInfoWithVlanId(sessionPtr, vlanId);
    if (vlanPtr == NULL)
    {
        // Enter here means the vlan count reaches max.
        LE_ERROR("Failed to get vlan info with vlan id(0x%x)", vlanId);
        SetSessionEventHandler(sessionPtr, vlanId, NULL, NULL);
        return NULL;
    }

    return (taf_diagDoIP_EventHandlerRef_t)vlanPtr->safeRef;
}

void taf_DiagDoIPSvr::RemoveEventHandler
(
    taf_diagDoIP_EventHandlerRef_t handlerRef
)
{
    taf_VlanCallback_t* vlanPtr;

    vlanPtr = (taf_VlanCallback_t*)le_ref_Lookup(vlanRefMap, (void*)handlerRef);
    if (vlanPtr == NULL || vlanPtr->sessPtr == NULL)
    {
        LE_ERROR("Invalid reference");
        return;
    }

    if (vlanPtr->sessPtr->sessionRef != taf_diagDoIP_GetClientSessionRef())
    {
        LE_WARN("Remove handler in another session");
    }

    SetSessionEventHandler(vlanPtr->sessPtr, vlanPtr->vlanId, NULL, NULL);
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
    uint16_t fromVlanId,
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
    LE_DEBUG("Report a event from DoIP");
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

                taf_VlanCallback_t* vlanPtr = doipSvr.GetVlanInfoWithVlanId(sessionPtr, fromVlanId);
                if (vlanPtr != NULL && vlanPtr->func != NULL)
                {
                    // App regitstered callback for the fromVlanId from this session.
                    vlanPtr->func(svcPtr->ref,
                                  eventType,
                                  remoteAddr,
                                  fromVlanId,
                                  vlanPtr->ctxPtr
                                  );
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

    LE_DEBUG("add session ref%p for service%p", sessionRef, servicePtr);
    taf_DoIPSession_t* newSessionPtr = (taf_DoIPSession_t *)le_mem_ForceAlloc(sessPool);

    newSessionPtr->sessionRef = sessionRef;
    newSessionPtr->link = LE_DLS_LINK_INIT;
    newSessionPtr->svrPtr = servicePtr;
    newSessionPtr->vlanInfoList = LE_DLS_LIST_INIT;
    le_dls_Queue(&servicePtr->sessionList, &(newSessionPtr->link));
    LE_DEBUG("Allocate a session class%p", newSessionPtr);

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
            LE_DEBUG("remove session ref(%p) from service%p", sessionRef, servicePtr);
            le_dls_Remove(&(servicePtr->sessionList), &(sessionPtr->link));
            RemoveAllVlanFromSession(sessionPtr);

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
    uint16_t vlanId,
    taf_diagDoIP_EventHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    le_dls_Link_t* linkPtr;
    taf_VlanCallback_t* vlanPtr;

    if (handlerPtr == NULL)
    {
        // Remove event handler.
        linkPtr = le_dls_Peek(&(sessionPtr->vlanInfoList));
        while (linkPtr != NULL)
        {
            vlanPtr = CONTAINER_OF(linkPtr, taf_VlanCallback_t, link);
            if (vlanPtr->vlanId == vlanId)
            {
                // Remove the element and release resource.
                le_dls_Remove(&(sessionPtr->vlanInfoList), &(vlanPtr->link));
                le_ref_DeleteRef(vlanRefMap, vlanPtr->safeRef);
                le_mem_Release(vlanPtr);
                LE_DEBUG("Release resources for vlanId:%d", vlanId);
                return LE_OK;
            }
            linkPtr = le_dls_PeekNext(&(sessionPtr->vlanInfoList), linkPtr);
        }
    }
    else
    {
        // Add event handler.
        linkPtr = le_dls_Peek(&(sessionPtr->vlanInfoList));
        while (linkPtr != NULL)
        {
            // Check if the vlan id is registered.
            vlanPtr = CONTAINER_OF(linkPtr, taf_VlanCallback_t, link);
            if (vlanPtr->vlanId == vlanId)
            {
                LE_ERROR("Session event handler for vlan0x%x has been set", vlanId);
                return LE_DUPLICATE;
            }
            linkPtr = le_dls_PeekNext(&(sessionPtr->vlanInfoList), linkPtr);
        }

        vlanPtr = (taf_VlanCallback_t *)le_mem_ForceAlloc(vlanPool);
        vlanPtr->vlanId = vlanId;
        vlanPtr->func = handlerPtr;
        vlanPtr->ctxPtr = contextPtr;
        vlanPtr->link = LE_DLS_LINK_INIT;
        vlanPtr->safeRef = le_ref_CreateRef(vlanRefMap, vlanPtr);
        vlanPtr->sessPtr = sessionPtr;
        le_dls_Queue(&sessionPtr->vlanInfoList, &(vlanPtr->link));
        LE_DEBUG("Add vlan(0x%x) handler to session successful.", vlanId);
    }

    return LE_OK;
}
void taf_DiagDoIPSvr::RemoveAllVlanFromSession
(
    taf_DoIPSession_t* sessionPtr
)
{
    if (sessionPtr == NULL)
    {
        return;
    }

    le_dls_Link_t* linkPtr = le_dls_Pop(&sessionPtr->vlanInfoList);
    while (linkPtr != NULL)
    {
        taf_VlanCallback_t* vlanPtr = CONTAINER_OF(linkPtr, taf_VlanCallback_t, link);
        le_mem_Release(vlanPtr);

        linkPtr = le_dls_Pop(&sessionPtr->vlanInfoList);
    }
}

taf_VlanCallback_t* taf_DiagDoIPSvr::GetVlanInfoWithVlanId
(
    taf_DoIPSession_t* sessionPtr,
    uint16_t vlanId
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&(sessionPtr->vlanInfoList));
    while (linkPtr != NULL)
    {
        taf_VlanCallback_t* vlanPtr = CONTAINER_OF(linkPtr, taf_VlanCallback_t, link);
        if (vlanPtr->vlanId == vlanId)
        {
            return vlanPtr;
        }
        linkPtr = le_dls_PeekNext(&(sessionPtr->vlanInfoList), linkPtr);
    }

    return NULL;
}
