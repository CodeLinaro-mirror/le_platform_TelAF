/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <string>
#include "tafIDPSSvr.hpp"
#include "tafUDSStack.h"
#include "tafDiagBackend.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of TelAF IDPS server.
 */
//--------------------------------------------------------------------------------------------------
taf_IdpsSvr& taf_IdpsSvr::GetInstance
(
)
{
    static taf_IdpsSvr instance;

    return instance;
}

//-------------------------------------------------------------------------------------------------
/**
 * Create a reference for the service, or get the reference of a service if the reference already
 * exists for the given session and security level.
 */
//-------------------------------------------------------------------------------------------------
taf_diagIDPS_ServiceRef_t taf_IdpsSvr::GetService
(
    taf_diagIDPS_SecurityLevel_t securityLevel
)
{
    LE_DEBUG("Gets the IDPS service! securityLevel=%d", (int)securityLevel);

    taf_IdpsSvc_t* servicePtr = GetServiceObj(taf_diagIDPS_GetClientSessionRef());

    // Create a service object if it doesn't exist for this session.
    if (servicePtr == NULL)
    {
        servicePtr = (taf_IdpsSvc_t*)le_mem_ForceAlloc(SvcPool);
        memset(servicePtr, 0, sizeof(taf_IdpsSvc_t));

        // Init the service handler.
        servicePtr->statusHandlerRef = NULL;
        servicePtr->securityLevel = securityLevel;

        // Init message and VLAN lists.
        servicePtr->statusMsgList = LE_DLS_LIST_INIT;
        servicePtr->supportedVlanList = LE_DLS_LIST_INIT;

        // Attach the service to the client session.
        servicePtr->sessionRef = taf_diagIDPS_GetClientSessionRef();

        // Create a Safe Reference for this service object.
        servicePtr->svcRef = (taf_diagIDPS_ServiceRef_t)le_ref_CreateRef(SvcRefMap, servicePtr);
    }
    else
    {
        // Update security level if service already exists.
        servicePtr->securityLevel = securityLevel;
    }

    return servicePtr->svcRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the service instance object by session reference.
 */
//-------------------------------------------------------------------------------------------------
taf_IdpsSvc_t* taf_IdpsSvr::GetServiceObj
(
    le_msg_SessionRef_t sessionRef
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_IdpsSvc_t* servicePtr = (taf_IdpsSvc_t*)le_ref_GetValue(iterRef);
        if ((servicePtr != NULL) && (sessionRef == servicePtr->sessionRef))
        {
            return servicePtr;
        }
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the service instance object by VLAN ID and security level.
 */
//-------------------------------------------------------------------------------------------------
taf_IdpsSvc_t* taf_IdpsSvr::GetServiceObj
(
    uint16_t vlanId,
    taf_diagIDPS_SecurityLevel_t securityLevel
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        bool isFound = false;
        taf_IdpsSvc_t* servicePtr = (taf_IdpsSvc_t*)le_ref_GetValue(iterRef);
        if (servicePtr == NULL)
        {
            continue;
        }

        // Check security level match:
        // LOW_LEVEL receives all notifications, so it matches any incoming security level.
        // HIGH_LEVEL only matches HIGH_LEVEL notifications.
        bool levelMatch = (servicePtr->securityLevel == TAF_DIAGIDPS_LOW_LEVEL) ||
                          (servicePtr->securityLevel == securityLevel);
        if (!levelMatch)
        {
            continue;
        }

        // In some cases the interface may not set the VLAN.
        if (vlanId == 0 && le_dls_NumLinks(&servicePtr->supportedVlanList) == 0)
        {
            return servicePtr;
        }

        // Verify if the VLAN matches.
        le_dls_Link_t* linkPtr = le_dls_Peek(&servicePtr->supportedVlanList);
        while (linkPtr)
        {
            taf_IdpsVlanIdNode_t* vlan = CONTAINER_OF(linkPtr, taf_IdpsVlanIdNode_t, link);
            if (vlan != NULL && vlan->vlanId == vlanId)
            {
                isFound = true;
                LE_DEBUG("Service object for vlan(0x%x) found!", vlanId);
                break;
            }
            linkPtr = le_dls_PeekNext(&servicePtr->supportedVlanList, linkPtr);
        }

        if (isFound)
        {
            return servicePtr;
        }
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * IDPS status message handler function — parses the raw UDS message into a status message object
 * and reports it via the event.
 */
//-------------------------------------------------------------------------------------------------
void taf_IdpsSvr::IdpsIndicationHandler
(
    const taf_uds_IdpsAddrInfo_t*  addrInfoPtr,   ///< [IN] Logical address information pointer.
    const taf_uds_IdpsStatusInfo_t*  diagMsgPtr,  ///< [IN] IDPS status information pointer.
    void*                      userPtr            ///< [IN] User-defined pointer.
)
{
    TAF_ERROR_IF_RET_NIL(addrInfoPtr == NULL, "Invalid addrInfoPtr");
    TAF_ERROR_IF_RET_NIL(diagMsgPtr == NULL, "Invalid diagMsgPtr");

    auto& idpsIns = taf_IdpsSvr::GetInstance();
    taf_IdpsStatusMsg_t* statusMsgPtr =
            (taf_IdpsStatusMsg_t*)le_mem_ForceAlloc(idpsIns.StatusMsgPool);
    memset(statusMsgPtr, 0, sizeof(taf_IdpsStatusMsg_t));

    memcpy(&statusMsgPtr->addrInfo, addrInfoPtr, sizeof(taf_uds_IdpsAddrInfo_t));
    statusMsgPtr->link = LE_DLS_LINK_INIT;

    statusMsgPtr->payload.securityLevel = (taf_diagIDPS_SecurityLevel_t)diagMsgPtr->securityLevel;
    statusMsgPtr->payload.serviceId = diagMsgPtr->sid;
    statusMsgPtr->payload.status = diagMsgPtr->status;

    LE_DEBUG("IDPS IdpsIndicationHandler sid=0x%x, status=0x%x", statusMsgPtr->payload.serviceId,
        statusMsgPtr->payload.status);

    const uint16_t maxIdpsPayloadSize = (MAX_IDPS_DATA_LEN < TAF_DIAGIDPS_MAX_DATA_SIZE) ?
        MAX_IDPS_DATA_LEN : TAF_DIAGIDPS_MAX_DATA_SIZE;

    // Parse data field if present.
    if(diagMsgPtr->dataLen > 0)
    {
        statusMsgPtr->payload.dataLen = diagMsgPtr->dataLen;

        if (statusMsgPtr->payload.dataLen > maxIdpsPayloadSize)
        {
            LE_WARN("IDPS data size 0x%x exceeds max supported size 0x%x, truncating",
                    statusMsgPtr->payload.dataLen, maxIdpsPayloadSize);
            statusMsgPtr->payload.dataLen = maxIdpsPayloadSize;
        }

        memcpy(statusMsgPtr->payload.data, diagMsgPtr->data, statusMsgPtr->payload.dataLen);
    }
    // Parse extra data field if present.
    if(diagMsgPtr->extraDataLen > 0)
    {
        statusMsgPtr->payload.extraDataLen = diagMsgPtr->extraDataLen;

        if (statusMsgPtr->payload.extraDataLen > maxIdpsPayloadSize)
        {
            LE_WARN("IDPS extra data size 0x%x exceeds max supported size 0x%x, truncating",
                    statusMsgPtr->payload.extraDataLen, maxIdpsPayloadSize);
            statusMsgPtr->payload.extraDataLen = maxIdpsPayloadSize;
        }

        memcpy(statusMsgPtr->payload.extraData, diagMsgPtr->extraData,
            statusMsgPtr->payload.extraDataLen);
    }

    // Create a safe reference for this status message.
    statusMsgPtr->statusMsgRef = (taf_diagIDPS_StatusMsgRef_t)
            le_ref_CreateRef(idpsIns.StatusMsgRefMap, statusMsgPtr);

    // Report the IDPS status message to the event handler.
    le_event_ReportWithRefCounting(idpsIns.StatusEvent, statusMsgPtr);
}

//-------------------------------------------------------------------------------------------------
/**
 * IDPS status event handler — dispatches the status message to the registered handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_IdpsSvr::StatusEventHandler
(
    void* reportPtr
)
{
    auto& idpsIns = taf_IdpsSvr::GetInstance();

    taf_IdpsStatusMsg_t* statusMsgPtr = (taf_IdpsStatusMsg_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(statusMsgPtr == NULL, "statusMsgPtr is NULL");

    bool deliveredAny = false;

    // Iterate all services and deliver to every match (multi-recipient support).
    le_ref_IterRef_t iterRef = le_ref_GetIterator(idpsIns.SvcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_IdpsSvc_t* svcPtr = (taf_IdpsSvc_t*)le_ref_GetValue(iterRef);
        if (svcPtr == NULL)
        {
            continue;
        }

        // Security level match:
        // LOW_LEVEL receives all notifications; HIGH_LEVEL only receives HIGH_LEVEL notifications.
        bool levelMatch = (svcPtr->securityLevel == TAF_DIAGIDPS_LOW_LEVEL) ||
                          (svcPtr->securityLevel == statusMsgPtr->payload.securityLevel);
        if (!levelMatch)
        {
            continue;
        }

        // VLAN match:
        bool vlanMatched = false;
        uint16_t vlanId = statusMsgPtr->addrInfo.vlanId;

        // In non-VLAN case (vlanId == 0), match if service has no configured VLAN list.
        if (vlanId == 0 && le_dls_NumLinks(&svcPtr->supportedVlanList) == 0)
        {
            vlanMatched = true;
        }
        else
        {
            // Otherwise, verify the VLAN is in the service's supported list.
            le_dls_Link_t* linkPtr = le_dls_Peek(&svcPtr->supportedVlanList);
            while (linkPtr)
            {
                taf_IdpsVlanIdNode_t* vlan = CONTAINER_OF(linkPtr, taf_IdpsVlanIdNode_t, link);
                if (vlan != NULL && vlan->vlanId == vlanId)
                {
                    vlanMatched = true;
                    break;
                }
                linkPtr = le_dls_PeekNext(&svcPtr->supportedVlanList, linkPtr);
            }
        }

        if (!vlanMatched)
        {
            continue;
        }

        // Must have a registered handler to receive notifications.
        if (svcPtr->statusHandlerRef == NULL)
        {
            LE_WARN("Did not register handler for IDPS service (svcRef=%p)", svcPtr->svcRef);
            continue;
        }

        // Lookup the handler object.
        taf_IdpsStatusHandler_t* handlerObjPtr = (taf_IdpsStatusHandler_t*)
            le_ref_Lookup(idpsIns.StatusHandlerRefMap, svcPtr->statusHandlerRef);
        if (handlerObjPtr == NULL || handlerObjPtr->func == NULL)
        {
            LE_WARN("Cannot find IDPS status handler object.");
            continue;
        }

        // Allocate a per-client message and clone the original payload.
        taf_IdpsStatusMsg_t* cloneMsgPtr =
            (taf_IdpsStatusMsg_t*)le_mem_ForceAlloc(idpsIns.StatusMsgPool);
        memset(cloneMsgPtr, 0, sizeof(taf_IdpsStatusMsg_t));

        memcpy(&cloneMsgPtr->addrInfo, &statusMsgPtr->addrInfo, sizeof(taf_uds_IdpsAddrInfo_t));
        memcpy(&cloneMsgPtr->payload, &statusMsgPtr->payload, sizeof(taf_IdpsStatusPayload_t));

        // Create a safe reference for this cloned message.
        cloneMsgPtr->statusMsgRef = (taf_diagIDPS_StatusMsgRef_t)
                le_ref_CreateRef(idpsIns.StatusMsgRefMap, cloneMsgPtr);

        // Record the owning service so ReleaseStatusMsg() can find the correct list.
        cloneMsgPtr->ownerSvcRef = svcPtr->svcRef;

        // Queue the message in the owning service list.
        cloneMsgPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&svcPtr->statusMsgList, &cloneMsgPtr->link);

        // Dispatch to the service handler.
        handlerObjPtr->func(cloneMsgPtr->statusMsgRef,
                cloneMsgPtr->payload.securityLevel,
                cloneMsgPtr->payload.serviceId,
                cloneMsgPtr->payload.status,
                handlerObjPtr->ctxPtr);

        deliveredAny = true;
    }

    if (!deliveredAny)
    {
        LE_WARN("Not found any registered IDPS service for vlan(%d), secLevel(%d)",
                statusMsgPtr->addrInfo.vlanId, (int)statusMsgPtr->payload.securityLevel);
    }

    // Clean up the original event message payload; clones are owned by recipients.
    le_ref_DeleteRef(idpsIns.StatusMsgRefMap, statusMsgPtr->statusMsgRef);
    le_mem_Release(statusMsgPtr);

}

//-------------------------------------------------------------------------------------------------
/**
 * Add a status handler for a given IDPS service.
 */
//-------------------------------------------------------------------------------------------------
taf_diagIDPS_StatusHandlerRef_t taf_IdpsSvr::AddStatusHandler
(
    taf_diagIDPS_ServiceRef_t svcRef,
    taf_diagIDPS_StatusHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    taf_IdpsSvc_t* svcPtr = (taf_IdpsSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(svcPtr == NULL, NULL, "Invalid service reference provided");

    if (svcPtr->statusHandlerRef != NULL)
    {
        LE_ERROR("Status handler is already registered");
        return NULL;
    }

    taf_IdpsStatusHandler_t* handlerObjPtr =
            (taf_IdpsStatusHandler_t*)le_mem_ForceAlloc(StatusHandlerPool);

    // Initialize the handler object.
    handlerObjPtr->svcRef     = svcRef;
    handlerObjPtr->func       = handlerPtr;
    handlerObjPtr->ctxPtr     = contextPtr;
    handlerObjPtr->handlerRef = (taf_diagIDPS_StatusHandlerRef_t)
            le_ref_CreateRef(StatusHandlerRefMap, handlerObjPtr);

    // Attach handler to service.
    svcPtr->statusHandlerRef = handlerObjPtr->handlerRef;

    return handlerObjPtr->handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the status handler for a given IDPS service.
 */
//-------------------------------------------------------------------------------------------------
void taf_IdpsSvr::RemoveStatusHandler
(
    taf_diagIDPS_StatusHandlerRef_t handlerRef
)
{
    LE_DEBUG("IDPS RemoveStatusHandler!");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");

    taf_IdpsStatusHandler_t* handlerObjPtr = (taf_IdpsStatusHandler_t*)
            le_ref_Lookup(StatusHandlerRefMap, handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerObjPtr == NULL, "Invalid handlerObjPtr");
    TAF_ERROR_IF_RET_NIL(handlerObjPtr->handlerRef != handlerRef, "Invalid handler reference");

    taf_IdpsSvc_t* svcPtr = (taf_IdpsSvc_t*)le_ref_Lookup(SvcRefMap, handlerObjPtr->svcRef);
    if (svcPtr == NULL)
    {
        LE_WARN("The handler does not belong to this service.");
        le_ref_DeleteRef(StatusHandlerRefMap, handlerRef);
        le_mem_Release(handlerObjPtr);
        return;
    }

    // Detach the handler from service.
    svcPtr->statusHandlerRef = NULL;

    // Clear handler resources.
    handlerObjPtr->handlerRef = NULL;
    handlerObjPtr->svcRef     = NULL;
    handlerObjPtr->func       = NULL;
    handlerObjPtr->ctxPtr     = NULL;

    // Free the handler.
    le_ref_DeleteRef(StatusHandlerRefMap, handlerRef);
    le_mem_Release(handlerObjPtr);
}

//-------------------------------------------------------------------------------------------------
/**
 * Set VLAN ID to service.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_IdpsSvr::SetVlanId
(
    taf_diagIDPS_ServiceRef_t svcRef,
    uint16_t vlanId
)
{
    taf_IdpsSvc_t* servicePtr = (taf_IdpsSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid service reference");
    TAF_ERROR_IF_RET_VAL(vlanId == 0, LE_BAD_PARAMETER, "Invalid vlan Id");

    // Check VLAN ID is valid or not.
    auto& backend = taf_DiagBackend::GetInstance();
    if (!backend.isVlanIdValid(vlanId))
    {
        LE_ERROR("VlanId is unknown");
        return LE_UNSUPPORTED;
    }

    // Check if the VLAN is already set.
    le_dls_Link_t* linkPtr = le_dls_Peek(&servicePtr->supportedVlanList);
    while (linkPtr)
    {
        taf_IdpsVlanIdNode_t* vlan = CONTAINER_OF(linkPtr, taf_IdpsVlanIdNode_t, link);
        if (vlan != NULL && vlan->vlanId == vlanId)
        {
            LE_DEBUG("The Vlan id(0x%x) is already set for ref %p", vlanId, svcRef);
            return LE_OK;
        }
        linkPtr = le_dls_PeekNext(&servicePtr->supportedVlanList, linkPtr);
    }

    taf_IdpsVlanIdNode_t* vlanPtr = (taf_IdpsVlanIdNode_t*)le_mem_ForceAlloc(VlanPool);

    vlanPtr->vlanId = vlanId;
    vlanPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->supportedVlanList, &vlanPtr->link);

    return LE_OK;

}

//-------------------------------------------------------------------------------------------------
/**
 * Get VLAN ID from status message.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_IdpsSvr::GetVlanIdFromMsg
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef,
    uint16_t* vlanIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(statusMsgRef == NULL, LE_BAD_PARAMETER, "Invalid statusMsgRef");
    TAF_ERROR_IF_RET_VAL(vlanIdPtr == NULL, LE_BAD_PARAMETER, "Invalid vlanIdPtr");

    taf_IdpsStatusMsg_t* statusMsgPtr = (taf_IdpsStatusMsg_t*)
            le_ref_Lookup(StatusMsgRefMap, statusMsgRef);
    if (statusMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the statusMsg");
        return LE_NOT_FOUND;
    }

    *vlanIdPtr = statusMsgPtr->addrInfo.vlanId;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get logical source and target addresses from status message.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_IdpsSvr::GetLogicalAddr
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef,
    uint16_t* sourceAddrPtr,
    uint16_t* targetAddrPtr
)
{
    TAF_ERROR_IF_RET_VAL(statusMsgRef == NULL, LE_BAD_PARAMETER, "Invalid statusMsgRef");
    TAF_ERROR_IF_RET_VAL(sourceAddrPtr == NULL, LE_BAD_PARAMETER, "Invalid sourceAddrPtr");
    TAF_ERROR_IF_RET_VAL(targetAddrPtr == NULL, LE_BAD_PARAMETER, "Invalid targetAddrPtr");

    taf_IdpsStatusMsg_t* statusMsgPtr = (taf_IdpsStatusMsg_t*)
            le_ref_Lookup(StatusMsgRefMap, statusMsgRef);
    if (statusMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the statusMsg");
        return LE_NOT_FOUND;
    }

    *sourceAddrPtr = statusMsgPtr->addrInfo.sa;
    *targetAddrPtr = statusMsgPtr->addrInfo.ta;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get data size from status message.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_IdpsSvr::GetDataSize
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef,
    uint16_t* sizePtr
)
{
    TAF_ERROR_IF_RET_VAL(statusMsgRef == NULL, LE_BAD_PARAMETER, "Invalid statusMsgRef");
    TAF_ERROR_IF_RET_VAL(sizePtr == NULL, LE_BAD_PARAMETER, "Invalid sizePtr");

    taf_IdpsStatusMsg_t* statusMsgPtr = (taf_IdpsStatusMsg_t*)
            le_ref_Lookup(StatusMsgRefMap, statusMsgRef);
    if (statusMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the statusMsg");
        return LE_NOT_FOUND;
    }

    *sizePtr = statusMsgPtr->payload.dataLen;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get data from status message.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_IdpsSvr::GetData
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef,
    uint8_t* dataPtr,
    size_t* dataSizePtr
)
{
    TAF_ERROR_IF_RET_VAL(statusMsgRef == NULL, LE_BAD_PARAMETER, "Invalid statusMsgRef");
    TAF_ERROR_IF_RET_VAL(dataPtr == NULL, LE_BAD_PARAMETER, "Invalid dataPtr");
    TAF_ERROR_IF_RET_VAL(dataSizePtr == NULL, LE_BAD_PARAMETER, "Invalid dataSizePtr");

    taf_IdpsStatusMsg_t* statusMsgPtr = (taf_IdpsStatusMsg_t*)
            le_ref_Lookup(StatusMsgRefMap, statusMsgRef);
    if (statusMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the statusMsg");
        return LE_NOT_FOUND;
    }

    if (*dataSizePtr < statusMsgPtr->payload.dataLen)
    {
        LE_ERROR("Buffer is not enough for data. %" PRIuS "-%" PRIuS,
                *dataSizePtr, (size_t)statusMsgPtr->payload.dataLen);
        return LE_OVERFLOW;
    }

    memcpy(dataPtr, statusMsgPtr->payload.data, statusMsgPtr->payload.dataLen);
    *dataSizePtr = statusMsgPtr->payload.dataLen;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get extra data size from status message.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_IdpsSvr::GetExtraDataSize
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef,
    uint16_t* sizePtr
)
{
    TAF_ERROR_IF_RET_VAL(statusMsgRef == NULL, LE_BAD_PARAMETER, "Invalid statusMsgRef");
    TAF_ERROR_IF_RET_VAL(sizePtr == NULL, LE_BAD_PARAMETER, "Invalid sizePtr");

    taf_IdpsStatusMsg_t* statusMsgPtr = (taf_IdpsStatusMsg_t*)
            le_ref_Lookup(StatusMsgRefMap, statusMsgRef);
    if (statusMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the statusMsg");
        return LE_NOT_FOUND;
    }

    *sizePtr = statusMsgPtr->payload.extraDataLen;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get extra data from status message.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_IdpsSvr::GetExtraData
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef,
    uint8_t* dataPtr,
    size_t* dataSizePtr
)
{
    TAF_ERROR_IF_RET_VAL(statusMsgRef == NULL, LE_BAD_PARAMETER, "Invalid statusMsgRef");
    TAF_ERROR_IF_RET_VAL(dataPtr == NULL, LE_BAD_PARAMETER, "Invalid dataPtr");
    TAF_ERROR_IF_RET_VAL(dataSizePtr == NULL, LE_BAD_PARAMETER, "Invalid dataSizePtr");

    taf_IdpsStatusMsg_t* statusMsgPtr = (taf_IdpsStatusMsg_t*)
            le_ref_Lookup(StatusMsgRefMap, statusMsgRef);
    if (statusMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the statusMsg");
        return LE_NOT_FOUND;
    }

    if (*dataSizePtr < statusMsgPtr->payload.extraDataLen)
    {
        LE_ERROR("Buffer is not enough for extra data. %" PRIuS "-%" PRIuS,
                *dataSizePtr, (size_t)statusMsgPtr->payload.extraDataLen);
        return LE_OVERFLOW;
    }

    memcpy(dataPtr, statusMsgPtr->payload.extraData, statusMsgPtr->payload.extraDataLen);
    *dataSizePtr = statusMsgPtr->payload.extraDataLen;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Release an IDPS status notification message.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_IdpsSvr::ReleaseStatusMsg
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef
)
{
    TAF_ERROR_IF_RET_VAL(statusMsgRef == NULL, LE_BAD_PARAMETER, "Invalid statusMsgRef");

    taf_IdpsStatusMsg_t* statusMsgPtr = (taf_IdpsStatusMsg_t*)
            le_ref_Lookup(StatusMsgRefMap, statusMsgRef);
    if (statusMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the statusMsg");
        return LE_NOT_FOUND;
    }

    // Find the owning service using the reference recorded at queue time and remove
    // the message from its list.  Using ownerSvcRef avoids any dependency on the
    // securityLevel override applied in StatusEventHandler().
    taf_IdpsSvc_t* svcPtr =
        (taf_IdpsSvc_t*)le_ref_Lookup(SvcRefMap, statusMsgPtr->ownerSvcRef);

    if (svcPtr != NULL)
    {
        le_dls_Remove(&svcPtr->statusMsgList, &statusMsgPtr->link);
    }
    else
    {
        LE_WARN("ReleaseStatusMsg: owning service (ref=%p) no longer exists, message not in list",
                statusMsgPtr->ownerSvcRef);
    }

    // Free the message.
    le_ref_DeleteRef(StatusMsgRefMap, statusMsgPtr->statusMsgRef);
    le_mem_Release(statusMsgPtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the created service and release the allocated memory.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_IdpsSvr::RemoveSvc
(
    taf_diagIDPS_ServiceRef_t svcRef
)
{
    LE_DEBUG("IDPS RemoveSvc");

    taf_IdpsSvc_t* servicePtr = (taf_IdpsSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid servicePtr");

    // Clear the registered status handler.
    if (servicePtr->statusHandlerRef != NULL)
    {
        RemoveStatusHandler(servicePtr->statusHandlerRef);
        servicePtr->statusHandlerRef = NULL;
    }

    // Release IDPS status message resources.
    ClearMsgList(servicePtr);
    ClearVlanList(servicePtr);

    // Clear service object.
    le_ref_DeleteRef(SvcRefMap, (void*)servicePtr->svcRef);
    le_mem_Release(servicePtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear IDPS status message list.
 */
//-------------------------------------------------------------------------------------------------
void taf_IdpsSvr::ClearMsgList
(
    taf_IdpsSvc_t* servicePtr
)
{
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->statusMsgList);
    while (linkPtr != NULL)
    {
        taf_IdpsStatusMsg_t* statusMsgPtr = CONTAINER_OF(linkPtr, taf_IdpsStatusMsg_t, link);
        if (statusMsgPtr != NULL)
        {
            le_ref_DeleteRef(StatusMsgRefMap, statusMsgPtr->statusMsgRef);
            le_mem_Release(statusMsgPtr);
        }

        linkPtr = le_dls_Pop(&servicePtr->statusMsgList);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear VLAN ID list.
 */
//-------------------------------------------------------------------------------------------------
void taf_IdpsSvr::ClearVlanList
(
    taf_IdpsSvc_t* servicePtr
)
{
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->supportedVlanList);
    while (linkPtr != NULL)
    {
        taf_IdpsVlanIdNode_t* vlanPtr = CONTAINER_OF(linkPtr, taf_IdpsVlanIdNode_t, link);
        if (vlanPtr != NULL)
        {
            le_mem_Release(vlanPtr);
        }

        linkPtr = le_dls_Pop(&servicePtr->supportedVlanList);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Session close handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_IdpsSvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    auto& idpsIns = taf_IdpsSvr::GetInstance();

    taf_diagIDPS_ServiceRef_t toRemove = NULL;
    le_ref_IterRef_t iterRef = le_ref_GetIterator(idpsIns.SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_IdpsSvc_t* servicePtr = (taf_IdpsSvc_t*)le_ref_GetValue(iterRef);
        LE_ASSERT(servicePtr != NULL);
        LE_ASSERT(servicePtr->svcRef == le_ref_GetSafeRef(iterRef));

        if (servicePtr->sessionRef == sessionRef)
        {
            toRemove = servicePtr->svcRef;
            break;
        }
    }

    if (toRemove != NULL)
    {
        idpsIns.RemoveSvc(toRemove);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_IdpsSvr::Init
(
    void
)
{
    // Create memory pools.
    SvcPool = le_mem_CreatePool("IdpsSvcPool", sizeof(taf_IdpsSvc_t));
    StatusMsgPool = le_mem_CreatePool("IdpsStatusMsgPool", sizeof(taf_IdpsStatusMsg_t));
    StatusHandlerPool = le_mem_CreatePool("IdpsStatusHandlerPool",
            sizeof(taf_IdpsStatusHandler_t));
    VlanPool = le_mem_CreatePool("IdpsVlanPool", sizeof(taf_IdpsVlanIdNode_t));

    // Create reference maps.
    SvcRefMap = le_ref_CreateMap("IdpsSvcRefMap", DEFAULT_IDPS_SVC_REF_CNT);
    StatusMsgRefMap = le_ref_CreateMap("IdpsStatusMsgRefMap", DEFAULT_IDPS_STATUS_MSG_REF_CNT);
    StatusHandlerRefMap = le_ref_CreateMap("IdpsStatusHandlerRefMap",
            DEFAULT_IDPS_HANDLER_REF_CNT);

    // Create the event and add event handler for IDPS status notification.
    StatusEvent = le_event_CreateIdWithRefCounting("IdpsStatusEvent");
    StatusEventHandlerRef = le_event_AddHandler("IdpsStatusEventHandlerRef", StatusEvent,
            taf_IdpsSvr::StatusEventHandler);

    taf_diagIDPS_AdvertiseService();
    // Set session close handler.
    le_msg_AddServiceCloseHandler(taf_diagIDPS_GetServiceRef(), OnClientDisconnection, NULL);

    // Register the callback for receiving IDPS message.
    idpsIndHandlerRef = taf_uds_AddIdpsHandler(IdpsIndicationHandler, NULL);
    if (idpsIndHandlerRef == NULL)
    {
        LE_ERROR("Add diag message reception handler failure.");
        return;
    }


    LE_INFO("IDPS service init completed, Advertise IDPS service");
}
