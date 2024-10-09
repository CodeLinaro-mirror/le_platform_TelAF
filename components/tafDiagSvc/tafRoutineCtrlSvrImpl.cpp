/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include "tafRoutineCtrlSvr.hpp"
#include "tafDiagBackend.hpp"
#include <arpa/inet.h>
#include "configuration.hpp"


using namespace telux::tafsvc;

taf_RoutinCtrlSvr &taf_RoutinCtrlSvr::GetInstance()
{
    static taf_RoutinCtrlSvr instance;

    return instance;
}

void taf_RoutinCtrlSvr::UDSMsgHandler
(
    const taf_uds_AddrInfo_t* addrPtr,
    uint8_t sid,
    uint8_t* msgPtr,
    size_t msgLen
)
{
    size_t copyLen;
    taf_RoutineCtrlReqMsg_t* rcMsgPtr;

    if (sid != svcId)
    {
        LE_ERROR("The service(0x%x) is invalid", sid);
        return;
    }

    rcMsgPtr = (taf_RoutineCtrlReqMsg_t*)le_mem_ForceAlloc(reqMsgPool);
    memset(rcMsgPtr, 0, sizeof(taf_RoutineCtrlReqMsg_t));

    rcMsgPtr->subFunc = msgPtr[1] & 0x7F;
    rcMsgPtr->routineId = ntohs(*((uint16_t*)(msgPtr + 2)));

    copyLen = msgLen - 4 > TAF_DIAG_ROUTINE_CTRL_RECORD_LEN
        ? TAF_DIAG_ROUTINE_CTRL_RECORD_LEN : msgLen - 4;

    memcpy(rcMsgPtr->recordData, msgPtr + 4, copyLen);
    rcMsgPtr->recordSize = copyLen;
    memcpy(&rcMsgPtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
    rcMsgPtr->link = LE_DLS_LINK_INIT;
    rcMsgPtr->ref = (taf_diagRoutineCtrl_RxMsgRef_t)le_ref_CreateRef(reqMsgRefMap, rcMsgPtr);

    LE_DEBUG("Receive message(%p) for service(identifier:0x%x, subFunction:0x%x from 0x%x)",
        rcMsgPtr->ref, rcMsgPtr->routineId, rcMsgPtr->subFunc, sid);

    // Report the request message to message handler in service layer.
    taf_UdsEvent_t event;
    event.type = 0;
    event.ref = rcMsgPtr->ref;
    le_event_Report(rxReqEvent, &event, sizeof(event));

    return;
}

void taf_RoutinCtrlSvr::ServiceObjDestructor
(
    void* objPtr
)
{
    taf_RoutinCtrlSvr& rc = taf_RoutinCtrlSvr::GetInstance();
    LE_DEBUG("Enter ServiceObjDestructor, thread%p", le_thread_GetCurrent());

    if (objPtr == NULL)
    {
        return;
    }

    // Release routine control service resource.
    taf_RoutineCtrlSvc_t* servicePtr = (taf_RoutineCtrlSvc_t*)objPtr;

    // Clear the UDS Rx message list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->reqMsgList);
    while (linkPtr != NULL)
    {
        taf_RoutineCtrlReqMsg_t* msgPtr = CONTAINER_OF(linkPtr, taf_RoutineCtrlReqMsg_t, link);
        if (msgPtr != NULL)
        {
            LE_INFO("Release ReqMsg(ref=%p, subFunc=0x%x, routineID=0x%x)",
                msgPtr->ref, msgPtr->subFunc, msgPtr->routineId);
            le_ref_DeleteRef(rc.reqMsgRefMap, msgPtr->ref);
            le_mem_Release(msgPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->reqMsgList);
    }

    // Clear the registered handler
    if (servicePtr->rxHandlerRef != NULL)
    {
        rc.RemoveRxReqMsgHandler(servicePtr->rxHandlerRef);
    }
}

void taf_RoutinCtrlSvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    auto& rc = taf_RoutinCtrlSvr::GetInstance();

    le_ref_IterRef_t iterRef = le_ref_GetIterator(rc.svcRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_RoutineCtrlSvc_t* svcPtr = (taf_RoutineCtrlSvc_t*)le_ref_GetValue(iterRef);
        if (svcPtr != NULL && svcPtr->sessionRef == sessionRef)
        {
            rc.RemoveRoutineCtrlSvc(svcPtr->ref);
        }
    }
}

void taf_RoutinCtrlSvr::RxReqEventHandler
(
    void* reportPtr
)
{
    taf_RoutineCtrlSvc_t* servicePtr;
    taf_RoutineCtrlReqMsg_t* msgPtr;
    taf_RoutineCtrlReqHandler_t* handlerObjPtr;
    taf_diagRoutineCtrl_Type_t type;

    if (reportPtr == NULL)
    {
        return;
    }

    taf_RoutinCtrlSvr& rc = taf_RoutinCtrlSvr::GetInstance();
    taf_UdsEvent_t *eventPtr = (taf_UdsEvent_t*)reportPtr;
    msgPtr = (taf_RoutineCtrlReqMsg_t*)le_ref_Lookup(rc.reqMsgRefMap, eventPtr->ref);
    if (msgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return;
    }

    servicePtr = rc.GetServiceObj(msgPtr->routineId, msgPtr->addrInfo.vlanId);
    if (servicePtr == NULL)
    {
        LE_WARN("Not found registered service(identifier:0x%x) for this request(vlan id:0x%x)",
            msgPtr->routineId, msgPtr->addrInfo.vlanId);
        // UDS_0x31_NRC_21: service pointer is null
        rc.SendNRCResp(rc.svcId, &(msgPtr->addrInfo), TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(rc.reqMsgRefMap, msgPtr->ref);
        le_mem_Release(msgPtr);
        return;
    }

    if (servicePtr->rxHandlerRef == NULL)
    {
        LE_WARN("Did not register handler for service(identifier:0x%x)",
            msgPtr->routineId);
        // UDS_0x31_NRC_21: handler is not registered
        rc.SendNRCResp(rc.svcId, &(msgPtr->addrInfo), TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(rc.reqMsgRefMap, msgPtr->ref);
        le_mem_Release(msgPtr);
        return;
    }

    // Loopup message handler of this service
    handlerObjPtr = (taf_RoutineCtrlReqHandler_t*)
        le_ref_Lookup(rc.reqHandlerRefMap, servicePtr->rxHandlerRef);
    if (handlerObjPtr == NULL || handlerObjPtr->func == NULL)
    {
        LE_ERROR("Can not find routine control handler object!");
        // UDS_0x31_NRC_21: handler is null
        rc.SendNRCResp(rc.svcId, &(msgPtr->addrInfo), TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(rc.reqMsgRefMap, msgPtr->ref);
        le_mem_Release(msgPtr);
        return;
    }

    if (msgPtr->subFunc == 0x01)
    {
        type = TAF_DIAGROUTINECTRL_START_ROUTINE;
    }
    else if (msgPtr->subFunc == 0x02)
    {
        type = TAF_DIAGROUTINECTRL_STOP_ROUTINE;
    }
    else if (msgPtr->subFunc == 0x03)
    {
        type = TAF_DIAGROUTINECTRL_REQUEST_ROUTINE_RESULTS;
    }
    else
    {
        LE_ERROR("Unknow SubFunction(0x%x)", msgPtr->subFunc);
        rc.SendNRCResp(rc.svcId, &(msgPtr->addrInfo), TAF_DIAG_SUBFUNCTION_NOT_SUPPORTED);
        le_ref_DeleteRef(rc.reqMsgRefMap, msgPtr->ref);
        le_mem_Release(msgPtr);
        return;
    }

    // Add the message in service message list and notify to application.
    msgPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->reqMsgList, &msgPtr->link);

    handlerObjPtr->func(msgPtr->ref, type, msgPtr->routineId, handlerObjPtr->ctxPtr);
}

taf_RoutineCtrlSvc_t* taf_RoutinCtrlSvr::GetServiceObj
(
    uint16_t identifier,
    le_msg_SessionRef_t sessionRef
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(svcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_RoutineCtrlSvc_t* servicePtr = (taf_RoutineCtrlSvc_t*)le_ref_GetValue(iterRef);
        if ((servicePtr != NULL) && (servicePtr->identifier == identifier)
            && (sessionRef == servicePtr->sessionRef))
        {
            return servicePtr;
        }
    }

    return NULL;
}

taf_RoutineCtrlSvc_t* taf_RoutinCtrlSvr::GetServiceObj
(
    uint16_t identifier,
    uint16_t vlanId
)
{
    bool isFound = false;
    le_ref_IterRef_t iterRef = le_ref_GetIterator(svcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_RoutineCtrlSvc_t* servicePtr = (taf_RoutineCtrlSvc_t *)le_ref_GetValue(iterRef);
        if ((servicePtr != NULL) && (servicePtr->identifier == identifier))
        {
            // In some cases. the interface may not set the vlan.
            if (vlanId == 0 && le_dls_NumLinks(&servicePtr->supportedVlanList) == 0)
            {
                return servicePtr;
            }

            // Verify if the vlan is match.
            le_dls_Link_t* linkPtr = NULL;
            linkPtr = le_dls_Peek(&servicePtr->supportedVlanList);
            while (linkPtr)
            {
                taf_RoutineCtrlVlanIdNode_t *vlan = CONTAINER_OF(linkPtr,
                    taf_RoutineCtrlVlanIdNode_t, link);
                if (vlan != NULL && vlan->vlanId == vlanId)
                {
                    // Match.
                    isFound = true;
                    break;
                }
                linkPtr = le_dls_PeekNext(&servicePtr->supportedVlanList, linkPtr);
            }

            if (isFound)
            {
                return servicePtr;
            }
        }
    }

    return NULL;
}

le_result_t taf_RoutinCtrlSvr::GetRoutineCtrlRec
(
    taf_diagRoutineCtrl_RxMsgRef_t reqMsgRef,
    uint8_t* optionRecPtr,
    size_t* optionRecSizePtr
)
{
    taf_RoutineCtrlReqMsg_t* reqMsgPtr;

    if (reqMsgRef == NULL || optionRecPtr == NULL || optionRecSizePtr == NULL)
    {
        LE_ERROR("Bad parameters");
        return LE_BAD_PARAMETER;
    }

    reqMsgPtr = (taf_RoutineCtrlReqMsg_t*)le_ref_Lookup(reqMsgRefMap, reqMsgRef);
    if (reqMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    size_t copySize = reqMsgPtr->recordSize > *optionRecSizePtr
        ? *optionRecSizePtr : reqMsgPtr->recordSize;

    memcpy(optionRecPtr, reqMsgPtr->recordData, copySize);
    *optionRecSizePtr = copySize;

    return LE_OK;
}

le_result_t taf_RoutinCtrlSvr::SendRoutineCtrlResp
(
    taf_diagRoutineCtrl_RxMsgRef_t reqMsgRef,
    uint8_t nrc,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    le_result_t ret;
    taf_RoutineCtrlSvc_t* servicePtr;
    taf_RoutineCtrlReqMsg_t* reqMsgPtr;

    if (reqMsgRef == NULL)
    {
        LE_ERROR("Bad parameters");
        return LE_BAD_PARAMETER;
    }

    reqMsgPtr = (taf_RoutineCtrlReqMsg_t*)le_ref_Lookup(reqMsgRefMap, reqMsgRef);
    if (reqMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    servicePtr = GetServiceObj(reqMsgPtr->routineId, reqMsgPtr->addrInfo.vlanId);
    if (servicePtr == NULL)
    {
        LE_ERROR("Cannot find the service(identifier:0x%x, vlan id:0x%x)",
            reqMsgPtr->routineId, reqMsgPtr->addrInfo.vlanId);
        return LE_NOT_FOUND;
    }

    LE_ASSERT(reqMsgPtr->ref == reqMsgRef);

    // Call UDS function to send the response message.
    auto& backend = taf_DiagBackend::GetInstance();
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = reqMsgPtr->addrInfo.ta;
    addrInfo.ta = reqMsgPtr->addrInfo.sa;
    addrInfo.taType = reqMsgPtr->addrInfo.taType;
    addrInfo.vlanId = reqMsgPtr->addrInfo.vlanId;
    le_utf8_Copy(addrInfo.ifName, reqMsgPtr->addrInfo.ifName, MAX_INTERFACE_NAME_LEN, NULL);

    if (nrc != 0)
    {
        // Negative response.
        ret = backend.RespDiagNegative(svcId, &addrInfo, nrc);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send routine control negative response.(%d)", ret);
            return ret;
        }
    }
    else
    {
        // Positive response.
        if (dataPtr == NULL || dataSize == 0)
        {
            ret = backend.RespDiagPositive(svcId, &addrInfo);
            if (ret != LE_OK)
            {
                LE_ERROR("Failed to send routine control positive response.(%d)", ret);
                return ret;
            }
        }
        else
        {
            ret = backend.RespDiagPositive(svcId, &addrInfo, dataPtr, dataSize);
            if (ret != LE_OK)
            {
                LE_ERROR("Failed to send routine control positive response.(%d)", ret);
                return ret;
            }
        }
    }

    LE_DEBUG("Sent response message with NRC0x%x", nrc);

    // Remove the message from service message list.
    le_dls_Remove(&servicePtr->reqMsgList, &reqMsgPtr->link);

    // Free the message
    le_ref_DeleteRef(reqMsgRefMap, reqMsgPtr->ref);
    le_mem_Release(reqMsgPtr);

    LE_DEBUG("Release reqMsg(%p) resource", reqMsgRef);

    return LE_OK;
}

le_result_t taf_RoutinCtrlSvr::SendNRCResp
(
    uint8_t sid,
    const taf_uds_AddrInfo_t*  addrInfoPtr,
    uint8_t errCode
)
{
    LE_DEBUG("SendNRCResp");

    TAF_ERROR_IF_RET_VAL(addrInfoPtr == NULL, LE_BAD_PARAMETER, "Invalid addrInfoPtr");

    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();

    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = addrInfoPtr->ta;
    addrInfo.ta = addrInfoPtr->sa;
    addrInfo.taType = addrInfoPtr->taType;
    addrInfo.vlanId = addrInfoPtr->vlanId;
    le_utf8_Copy(addrInfo.ifName, addrInfoPtr->ifName, MAX_INTERFACE_NAME_LEN, NULL);

    return backend.RespDiagNegative(sid, &addrInfo, errCode);
}

le_result_t taf_RoutinCtrlSvr::ReleaseRoutineCtrlMsg
(
    taf_diagRoutineCtrl_RxMsgRef_t reqMsgRef
)
{
    taf_RoutineCtrlSvc_t* servicePtr;
    taf_RoutineCtrlReqMsg_t* reqMsgPtr;

    if (reqMsgRef == NULL)
    {
        LE_ERROR("Bad parameters");
        return LE_BAD_PARAMETER;
    }

    reqMsgPtr = (taf_RoutineCtrlReqMsg_t*)le_ref_Lookup(reqMsgRefMap, reqMsgRef);
    if (reqMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    servicePtr = GetServiceObj(reqMsgPtr->routineId, reqMsgPtr->addrInfo.vlanId);
    if (servicePtr == NULL)
    {
        LE_ERROR("Cannot find the service(identifier:0x%x, vlan id:0x%x)",
            reqMsgPtr->routineId, reqMsgPtr->addrInfo.vlanId);
        return LE_NOT_FOUND;
    }

    // Remove the message from service message list.
    le_dls_Remove(&servicePtr->reqMsgList, &reqMsgPtr->link);

    // Free the message
    le_ref_DeleteRef(reqMsgRefMap, reqMsgPtr->ref);
    le_mem_Release(reqMsgPtr);

    LE_DEBUG("Release reqMsg(%p) resource", reqMsgRef);

    return LE_OK;
}

taf_diagRoutineCtrl_ServiceRef_t taf_RoutinCtrlSvr::FindOrCreateService
(
    uint16_t identifier
)
{
    LE_DEBUG("Enter routine control FindOrCreateService");

    // Verify the identifier which is defined in configuration file.
    try
    {
        // Check if RID supported in active session.
        cfg::top_routines_all<uint16_t>("identifier", identifier);
    }
    catch (const std::exception& e)
    {
        LE_ERROR("The RID(0x%x) is not defined in configuration file", identifier);
        return NULL;
    }

    taf_RoutineCtrlSvc_t* servicePtr = GetServiceObj(identifier,
        taf_diagRoutineCtrl_GetClientSessionRef());
    if (servicePtr != NULL)
    {
        return servicePtr->ref;
    }

    servicePtr = (taf_RoutineCtrlSvc_t*)le_mem_ForceAlloc(svcPool);
    memset(servicePtr, 0, sizeof(taf_RoutineCtrlSvc_t));

    // Initialize service object.
    servicePtr->identifier  = identifier;
    servicePtr->reqMsgList  = LE_DLS_LIST_INIT;
    servicePtr->sessionRef  = taf_diagRoutineCtrl_GetClientSessionRef();
    servicePtr->ref = (taf_diagRoutineCtrl_ServiceRef_t)le_ref_CreateRef(svcRefMap, servicePtr);
    servicePtr->supportedVlanList = LE_DLS_LIST_INIT;

    LE_INFO("Routine control: serviceRef%p of client%p is created for identifier0x%x",
        servicePtr->ref, servicePtr->sessionRef, servicePtr->identifier);

    return servicePtr->ref;
}

le_result_t taf_RoutinCtrlSvr::RemoveRoutineCtrlSvc
(
    taf_diagRoutineCtrl_ServiceRef_t svcRef
)
{
    taf_RoutineCtrlSvc_t* servicePtr = (taf_RoutineCtrlSvc_t*)le_ref_Lookup(svcRefMap, svcRef);
    if (servicePtr == NULL)
    {
        LE_ERROR("Bad parameters");
        return LE_BAD_PARAMETER;
    }

    // Clear the UDS Rx message list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->reqMsgList);
    while (linkPtr != NULL)
    {
        taf_RoutineCtrlReqMsg_t* msgPtr = CONTAINER_OF(linkPtr, taf_RoutineCtrlReqMsg_t, link);
        if (msgPtr != NULL)
        {
            LE_INFO("Release ReqMsg(ref=%p, subFunc=0x%x, routineID=0x%x)",
                msgPtr->ref, msgPtr->subFunc, msgPtr->routineId);
            le_ref_DeleteRef(reqMsgRefMap, msgPtr->ref);
            le_mem_Release(msgPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->reqMsgList);
    }

    // Clear the registered handler
    if (servicePtr->rxHandlerRef != NULL)
    {
        taf_RoutineCtrlReqHandler_t* handlerObjPtr;

        handlerObjPtr = (taf_RoutineCtrlReqHandler_t*)
            le_ref_Lookup(reqHandlerRefMap, servicePtr->rxHandlerRef);
        if (handlerObjPtr != NULL)
        {
            le_ref_DeleteRef(reqHandlerRefMap, handlerObjPtr->safeRef);
            le_mem_Release(handlerObjPtr);
        }

        servicePtr->rxHandlerRef = NULL;
    }

    // Clear service object
    le_ref_DeleteRef(svcRefMap, (void*)servicePtr->ref);
    le_mem_Release(servicePtr);

    return LE_OK;
}

taf_diagRoutineCtrl_RxMsgHandlerRef_t taf_RoutinCtrlSvr::AddRxReqMsgHandler
(
    taf_diagRoutineCtrl_ServiceRef_t svcRef,
    taf_diagRoutineCtrl_RxMsgHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    taf_RoutineCtrlReqHandler_t* handlerObjPtr;

    if (handlerPtr == NULL)
    {
        LE_ERROR("Bad parameters");
        return NULL;
    }

    taf_RoutineCtrlSvc_t* servicePtr = (taf_RoutineCtrlSvc_t*)le_ref_Lookup(svcRefMap, svcRef);
    if (servicePtr == NULL)
    {
        LE_ERROR("Bad parameters");
        return NULL;
    }

    if (servicePtr->rxHandlerRef != NULL)
    {
        LE_ERROR("Rx handler is already registered for the routine control service(id:0x%x).",
            servicePtr->identifier);
        return NULL;
    }

    handlerObjPtr = (taf_RoutineCtrlReqHandler_t*)le_mem_ForceAlloc(reqHandlerPool);

    void* handlerRef = le_ref_CreateRef(reqHandlerRefMap, handlerObjPtr);
    if (handlerRef == NULL)
    {
        LE_ERROR("Failed to create RX Handler reference for the routine control service(id:0x%x)!",
            servicePtr->identifier);
        return NULL;
    }

    // Initialize the RxHandler object.
    handlerObjPtr->svcRef    = svcRef;
    handlerObjPtr->safeRef   = handlerRef;
    handlerObjPtr->func      = handlerPtr;
    handlerObjPtr->ctxPtr    = contextPtr;

    // Attach handler to service.
    servicePtr->rxHandlerRef = (taf_diagRoutineCtrl_RxMsgHandlerRef_t)handlerObjPtr->safeRef;

    LE_INFO("Routine control: Registered Rx Handler for identifier0x%x",
        servicePtr->identifier);

    return (taf_diagRoutineCtrl_RxMsgHandlerRef_t)handlerObjPtr->safeRef;
}

void taf_RoutinCtrlSvr::RemoveRxReqMsgHandler
(
    taf_diagRoutineCtrl_RxMsgHandlerRef_t handlerRef
)
{
    taf_RoutineCtrlSvc_t* servicePtr;
    taf_RoutineCtrlReqHandler_t* handlerObjPtr;

    handlerObjPtr = (taf_RoutineCtrlReqHandler_t*)le_ref_Lookup(reqHandlerRefMap, handlerRef);
    if (handlerObjPtr == NULL)
    {
        LE_ERROR("Invalid reference");
        return;
    }

    servicePtr = (taf_RoutineCtrlSvc_t*)le_ref_Lookup(svcRefMap, handlerObjPtr->svcRef);
    if (servicePtr == NULL)
    {
        LE_WARN("The handler is not belong to any routine control service.");
        le_ref_DeleteRef(reqHandlerRefMap, (void*)handlerRef);
        le_mem_Release(handlerObjPtr);
        return;
    }

    // Detach the handler from service.
    servicePtr->rxHandlerRef = NULL;

    // Clear Rx Handler resources
    handlerObjPtr->ctxPtr     = NULL;
    handlerObjPtr->func       = NULL;
    handlerObjPtr->safeRef    = NULL;
    handlerObjPtr->svcRef     = NULL;

    le_ref_DeleteRef(reqHandlerRefMap, (void*)handlerRef);
    le_mem_Release(handlerObjPtr);
}

le_result_t taf_RoutinCtrlSvr::SetVlanId
(
    taf_diagRoutineCtrl_ServiceRef_t svcRef,
    uint16_t vlanId
)
{
    taf_RoutineCtrlSvc_t* servicePtr = (taf_RoutineCtrlSvc_t*)le_ref_Lookup(svcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid service reference");

    // Check if the vlan is set.
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&servicePtr->supportedVlanList);
    while (linkPtr)
    {
        taf_RoutineCtrlVlanIdNode_t *vlan = CONTAINER_OF(linkPtr,
            taf_RoutineCtrlVlanIdNode_t, link);
        if (vlan != NULL && vlan->vlanId == vlanId)
        {
            LE_INFO("The Vlan id(0x%x) is set for ref%p", vlanId, svcRef);
            return LE_OK;
        }
        linkPtr = le_dls_PeekNext(&servicePtr->supportedVlanList, linkPtr);
    }

    taf_RoutineCtrlVlanIdNode_t *vlanPtr = (taf_RoutineCtrlVlanIdNode_t *)
        le_mem_ForceAlloc(vlanPool);
    if (vlanPtr == NULL)
    {
        LE_INFO("Failed to allocate memory.");
        return LE_NO_MEMORY;
    }

    vlanPtr->vlanId = vlanId;
    vlanPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->supportedVlanList, &vlanPtr->link);

    return LE_OK;
}

le_result_t taf_RoutinCtrlSvr::GetVlanIdFromMsg
(
    taf_diagRoutineCtrl_RxMsgRef_t reqMsgRef,
    uint16_t* vlanIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(vlanIdPtr == NULL, LE_BAD_PARAMETER, "Invalid vlanIdPtr");

    taf_RoutineCtrlReqMsg_t* reqMsgPtr = (taf_RoutineCtrlReqMsg_t*)
        le_ref_Lookup(reqMsgRefMap, reqMsgRef);
    if (reqMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the rxMsg in routine ctrl service");
        return LE_NOT_FOUND;
    }

    *vlanIdPtr = reqMsgPtr->addrInfo.vlanId;

    return LE_OK;
}

void taf_RoutinCtrlSvr::Init()
{
    // Create memory pools.
    svcPool = le_mem_CreatePool("RoutineCtrlSvcPool", sizeof(taf_RoutineCtrlSvc_t));
    reqMsgPool = le_mem_CreatePool("RoutineReqMsgPool", sizeof(taf_RoutineCtrlReqMsg_t));
    reqHandlerPool = le_mem_CreatePool("RoutinereqHdlPool", sizeof(taf_RoutineCtrlReqHandler_t));
    vlanPool = le_mem_CreatePool("RoutineVlanPool", sizeof(taf_RoutineCtrlVlanIdNode_t));

    // Set memory pool destructor.
    //le_mem_SetDestructor(svcPool, taf_RoutinCtrlSvr::ServiceObjDestructor);

    // Create reference maps
    svcRefMap = le_ref_CreateMap("RoutineCtrlSvcRefMap", TAF_DIAG_ROUTINE_CTRL_SVC_CNT);
    reqHandlerRefMap = le_ref_CreateMap("RoutineCtrlReqHandlerRefMap",
        TAF_DIAG_ROUTINE_CTRL_REQHANDLER_CNT);
    reqMsgRefMap = le_ref_CreateMap("RoutineCtrlReqMsgRefMap", TAF_DIAG_ROUTINE_CTRL_REQMSG_CNT);

    // Create event and add the event handler.
    rxReqEvent = le_event_CreateId("RoutineCtrlReqEvent", sizeof(taf_UdsEvent_t));
    rxReqEventHandlerRef = le_event_AddHandler("RoutineCtrlReqEventHandler", rxReqEvent,
        taf_RoutinCtrlSvr::RxReqEventHandler);

    // Create client session close hander.
    le_msg_AddServiceCloseHandler(taf_diagRoutineCtrl_GetServiceRef(),
                                  taf_RoutinCtrlSvr::OnClientDisconnection, NULL);

    auto& backend = taf_DiagBackend::GetInstance();

    backend.RegisterUdsService(svcId, this);

    LE_INFO("Routine control service initialization successful in thread%p!", le_thread_GetCurrent());
}
