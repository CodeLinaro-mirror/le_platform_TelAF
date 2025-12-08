/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include <string>
#include "tafResetSvr.hpp"
#include "configuration.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of TelAF ECURestet server.
 */
//--------------------------------------------------------------------------------------------------
taf_ResetSvr &taf_ResetSvr::GetInstance()
{
    static taf_ResetSvr instance;

    return instance;
}

//-------------------------------------------------------------------------------------------------
/**
 * Create a reference for the service, or get the reference of a service if the reference already
 * exist.
 */
//-------------------------------------------------------------------------------------------------
taf_diagReset_ServiceRef_t taf_ResetSvr::GetService
(
    uint8_t resetType
)
{
    LE_DEBUG("Gets the Reset Service");

    // Reset type check. Exception if can't get node from config file.
    if (resetType != TAF_DIAGRESET_ALL_RESET)
    {
        try
        {
            cfg::Node node = cfg::top_reset_all<int>("sub_function_identifier", resetType);
            LE_INFO("Reset type 0x%x is supported", resetType);
        }
        catch (const std::exception& e)
        {
            LE_ERROR("ECU reset type 0x%x is not configured in YAML file.", resetType);
            return NULL;
        }
    }

    // Search the service.
    taf_ResetSvc_t* servicePtr = GetServiceObj(resetType, taf_diagReset_GetClientSessionRef());

    // Create a service object if it doesn't exist in the list.
    if (servicePtr == NULL)
    {
        servicePtr = (taf_ResetSvc_t *)le_mem_ForceAlloc(SvcPool);
        memset(servicePtr, 0, sizeof(taf_ResetSvc_t));

        // Init the resetType.
        servicePtr->resetType = resetType;

        // Init the service Rx Handler.
        servicePtr->handlerRef = NULL;

        // Attach the service to the client.
        servicePtr->sessionRef = taf_diagReset_GetClientSessionRef();

        // Create a Safe Reference for this service object
        servicePtr->svcRef = (taf_diagReset_ServiceRef_t)le_ref_CreateRef(SvcRefMap, servicePtr);

        servicePtr->supportedVlanList = LE_DLS_LIST_INIT;

        LE_DEBUG("svcRef %p of client %p is created for ECU reset type %x.",
                servicePtr->svcRef, servicePtr->sessionRef, resetType);
    }
    else
    {
        if (servicePtr->resetType == TAF_DIAGRESET_ALL_RESET &&
                resetType != TAF_DIAGRESET_ALL_RESET)
        {
            LE_ERROR("Service is already created to handle all type of Reset");
            return NULL;
        }
        else if(servicePtr->resetType != TAF_DIAGRESET_ALL_RESET &&
                resetType == TAF_DIAGRESET_ALL_RESET)
        {
            LE_ERROR("Service is already created for Reset type 0x%x", servicePtr->resetType);
            return NULL;
        }
    }

    return servicePtr->svcRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the service instacnce object by resetType if the service reference already created and
 * return the object pointer.
 */
//-------------------------------------------------------------------------------------------------
taf_ResetSvc_t* taf_ResetSvr::GetServiceObj
(
    uint8_t resetType,
    uint16_t vlanId
)
{
    bool isFound = false;
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_ResetSvc_t* servicePtr = (taf_ResetSvc_t *)le_ref_GetValue(iterRef);
        if (servicePtr != NULL)
        {
            if ((servicePtr->resetType == resetType) ||
                (servicePtr->resetType == TAF_DIAGRESET_ALL_RESET &&
                resetType != TAF_DIAGRESET_ALL_RESET) ||
                (servicePtr->resetType != TAF_DIAGRESET_ALL_RESET &&
                resetType == TAF_DIAGRESET_ALL_RESET))
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
                    taf_ResetVlanIdNode_t *vlan = CONTAINER_OF(linkPtr, taf_ResetVlanIdNode_t, link);
                    if (vlan != NULL && vlan->vlanId == vlanId)
                    {
                        // Match.
                        isFound = true;
                        LE_DEBUG("Service object for vlan(x%0x) found", vlanId);
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
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the service instacnce object by resetType if the service reference already created and
 * return the object pointer.
 */
//-------------------------------------------------------------------------------------------------
taf_ResetSvc_t* taf_ResetSvr::GetServiceObj
(
    uint8_t resetType,
    le_msg_SessionRef_t sessionRef
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_ResetSvc_t* servicePtr = (taf_ResetSvc_t *)le_ref_GetValue(iterRef);
        if (servicePtr != NULL)
        {
            if ((sessionRef == servicePtr->sessionRef) &&
                ((servicePtr->resetType == resetType) ||
                (servicePtr->resetType == TAF_DIAGRESET_ALL_RESET &&
                resetType != TAF_DIAGRESET_ALL_RESET) ||
                (servicePtr->resetType != TAF_DIAGRESET_ALL_RESET &&
                resetType == TAF_DIAGRESET_ALL_RESET)))
            {
                return servicePtr;
            }
        }
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * UDS stack handler function
 */
//-------------------------------------------------------------------------------------------------
void taf_ResetSvr::UDSMsgHandler
(
    const taf_uds_AddrInfo_t* addrPtr,
    uint8_t sid,
    uint8_t* msgPtr,
    size_t msgLen
)
{
    LE_DEBUG("UDSMsgHandler!");

    TAF_ERROR_IF_RET_NIL(addrPtr == NULL, "Invalid addrPtr");
    TAF_ERROR_IF_RET_NIL(msgPtr == NULL, "Invalid msgPtr");

    if (sid != reqSvcId)
    {
        LE_ERROR("The service(0x%x) is invalid", sid);
        return;
    }

#ifndef LE_CONFIG_DIAG_FEATURE_A
    uint8_t errCode = 0;
    taf_uds_AddrInfo_t addrInfo;
    memcpy(&addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));

    auto &diag = taf_DiagSvr::GetInstance();
    uint8_t resetType = msgPtr[1] & 0x7F;

    // check enable condition
    try
    {
        cfg::Node & node = cfg::top_reset_all<int>("sub_function_identifier", resetType);
        cfg::Node & enableNode = node.get_child("data_enable_condition");

        for (const auto & enable: enableNode)
        {
            // Get the defined enable operation type: "and" or "or"
            std::string enableOperation = enable.first;
            if (enableOperation == "and")
            {
                cfg::Node & optNodeList = enableNode.get_child("and");
                for (const auto & optNode: optNodeList)
                {
                    uint8_t enableId = optNode.second.get_value<uint8_t>();

                    if (!diag.GetEnableConditionStatus(enableId))
                    {
                        errCode = cfg::get_nrc_by_condition_id(enableId);
                        LE_WARN("Enable id %d condition is false, send nrc 0x%x",
                                enableId, errCode);
                        SendNRCResp(&addrInfo, errCode);
                        return;
                    }
                }
            }
            else if (enableOperation == "or")
            {
                cfg::Node & optNodeList = enableNode.get_child("or");
                bool enableStatus = false;
                uint8_t enableId = 0;

                for (const auto & optNode: optNodeList)
                {
                    enableId = optNode.second.get_value<uint8_t>();

                    if(diag.GetEnableConditionStatus(enableId))
                    {
                        enableStatus = true;
                        break;
                    }
                }

                if(!enableStatus && enableId != 0)
                {
                    errCode = cfg::get_nrc_by_condition_id(enableId);
                    LE_WARN("Enable id %d condition is false, send nrc 0x%x",
                            enableId, errCode);
                    SendNRCResp(&addrInfo, errCode);
                    return;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        LE_WARN("Enable condition does not define for reset type: 0x%x, Exception: %s",
                    resetType, e.what());
    }
#endif

    taf_ResetRxMsg_t* rxMsgPtr = NULL;

    rxMsgPtr = (taf_ResetRxMsg_t*)le_mem_ForceAlloc(RxMsgPool);
    memset(rxMsgPtr, 0, sizeof(taf_ResetRxMsg_t));

    memcpy(&rxMsgPtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
    rxMsgPtr->subFunc = msgPtr[1] & 0x7F;
    rxMsgPtr->link = LE_DLS_LINK_INIT;
    rxMsgPtr->rxMsgRef = (taf_diagReset_RxMsgRef_t)le_ref_CreateRef(RxMsgRefMap, rxMsgPtr);

    LE_DEBUG("Receive message(%p) for serviceId: 0x%x and subFunction: 0x%x)",
            rxMsgPtr->rxMsgRef, sid, rxMsgPtr->subFunc);

    // Report the request message to message handler in service layer.
    le_event_ReportWithRefCounting(ResetEvent, rxMsgPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a Rx handler for a given service.
 */
//-------------------------------------------------------------------------------------------------
taf_diagReset_RxMsgHandlerRef_t taf_ResetSvr::AddRxMsgHandler
(
    taf_diagReset_ServiceRef_t svcRef,
    taf_diagReset_RxMsgHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_DEBUG("AddRxMsgHandler");

    taf_ResetSvc_t* servicePtr = (taf_ResetSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, NULL, "Invalid service reference provided");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    if (servicePtr->handlerRef != NULL)
    {
        LE_ERROR("Rx handler is already registered for the ECUReset service type: 0x%x.",
                servicePtr->resetType);

        return NULL;
    }

    taf_ResetReqHandler_t* handlerObjPtr = NULL;
    handlerObjPtr = (taf_ResetReqHandler_t*)le_mem_ForceAlloc(ReqHandlerPool);

    // Initialize the RxHandler object.
    handlerObjPtr->svcRef     = svcRef;
    handlerObjPtr->func       = handlerPtr;
    handlerObjPtr->ctxPtr     = contextPtr;
    handlerObjPtr->handlerRef =
            (taf_diagReset_RxMsgHandlerRef_t)le_ref_CreateRef(ReqHandlerRefMap, handlerObjPtr);

    // Attach handler to service.
    servicePtr->handlerRef = handlerObjPtr->handlerRef;

    return handlerObjPtr->handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Reset event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_ResetSvr::RxReqEventHandler
(
    void* reportPtr
)
{
    LE_DEBUG("RxReqEventHandler");

    auto &reset = taf_ResetSvr::GetInstance();

    taf_ResetRxMsg_t* rxMsgPtr = (taf_ResetRxMsg_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(rxMsgPtr == NULL, "rxMsgPtr is Null");

    taf_ResetSvc_t* servicePtr = NULL;
    taf_ResetReqHandler_t* handlerObjPtr = NULL;

#ifndef LE_CONFIG_DIAG_VSTACK
    servicePtr = (taf_ResetSvc_t*)reset.GetServiceObj(rxMsgPtr->subFunc, rxMsgPtr->addrInfo.vlanId);
#else
    servicePtr = (taf_ResetSvc_t*)reset.GetServiceObj(rxMsgPtr->subFunc, (uint16_t)0);
#endif
    if (servicePtr == NULL)
    {
#ifndef LE_CONFIG_DIAG_VSTACK
        servicePtr = (taf_ResetSvc_t*)reset.GetServiceObj(TAF_DIAGRESET_ALL_RESET,
            rxMsgPtr->addrInfo.vlanId);
#else
        servicePtr = (taf_ResetSvc_t*)reset.GetServiceObj(TAF_DIAGRESET_ALL_RESET, (uint16_t)0);
#endif
        if(servicePtr == NULL)
        {
            LE_WARN("Not found registered ECU reset service type: 0x%x for this request(vlan id:0x%x)",
                    rxMsgPtr->subFunc, rxMsgPtr->addrInfo.vlanId);
            // UDS_0x11_NRC_21: service pointer is null
            reset.SendNRCResp(&(rxMsgPtr->addrInfo), TAF_DIAG_BUSY_REPEAT_REQUEST);
            le_ref_DeleteRef(reset.RxMsgRefMap, rxMsgPtr->rxMsgRef);
            le_mem_Release(rxMsgPtr);
            return;
        }
    }

    if (servicePtr->handlerRef == NULL)
    {
        LE_WARN("Did not register handler for ECU reset service type: 0x%x, send NRC %x",
                rxMsgPtr->subFunc, TAF_DIAG_BUSY_REPEAT_REQUEST);
        // UDS_0x11_NRC_21: handler is not registered
        reset.SendNRCResp(&(rxMsgPtr->addrInfo), TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(reset.RxMsgRefMap, rxMsgPtr->rxMsgRef);
        le_mem_Release(rxMsgPtr);
        return;
    }

    // Lookup message handler of this service
    handlerObjPtr =
            (taf_ResetReqHandler_t*)le_ref_Lookup(reset.ReqHandlerRefMap, servicePtr->handlerRef);
    if (handlerObjPtr == NULL || handlerObjPtr->func == NULL)
    {
        // UDS_0x11_NRC_21: handler is null
        reset.SendNRCResp(&(rxMsgPtr->addrInfo), TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(reset.RxMsgRefMap, rxMsgPtr->rxMsgRef);
        le_mem_Release(rxMsgPtr);
        return;
    }

    uint8_t resetType = rxMsgPtr->subFunc;

    // Add the message in service message list and notify to application.
    rxMsgPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->rxMsgList, &rxMsgPtr->link);

    if(servicePtr->resetType == resetType || servicePtr->resetType == TAF_DIAGRESET_ALL_RESET)
    {
        handlerObjPtr->func(rxMsgPtr->rxMsgRef, resetType, handlerObjPtr->ctxPtr);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the Rx handler for a given service.
 */
//-------------------------------------------------------------------------------------------------
void taf_ResetSvr::RemoveRxMsgHandler
(
    taf_diagReset_RxMsgHandlerRef_t handlerRef
)
{
    LE_DEBUG("RemoveRxMsgHandler");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");

    taf_ResetSvc_t* servicePtr = NULL;
    taf_ResetReqHandler_t* handlerPtr = NULL;

    handlerPtr = (taf_ResetReqHandler_t*)le_ref_Lookup(ReqHandlerRefMap, handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerPtr == NULL, "Invalid handlerPtr");

    servicePtr = (taf_ResetSvc_t*)le_ref_Lookup(SvcRefMap, handlerPtr->svcRef);
    if (servicePtr == NULL)
    {
        LE_WARN("The handler is not belong to this service.");
        le_ref_DeleteRef(ReqHandlerRefMap, handlerRef);
        le_mem_Release(handlerPtr);

        return;
    }

    // Detach the handler from service.
    servicePtr->handlerRef = NULL;

    // Clear Rx Handler resources
    handlerPtr->handlerRef = NULL;
    handlerPtr->svcRef     = NULL;
    handlerPtr->func       = NULL;
    handlerPtr->ctxPtr     = NULL;

    // Free the handler.
    le_ref_DeleteRef(ReqHandlerRefMap, handlerRef);
    le_mem_Release(handlerPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send NRC response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_ResetSvr::SendNRCResp
(
    taf_uds_AddrInfo_t*  addrInfoPtr,
    uint8_t errCode
)
{
    TAF_ERROR_IF_RET_VAL(addrInfoPtr == NULL, LE_BAD_PARAMETER, "Invalid addrInfoPtr");

    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = addrInfoPtr->ta;
    addrInfo.ta = addrInfoPtr->sa;
    addrInfo.taType = addrInfoPtr->taType;
    addrInfo.vlanId = addrInfoPtr->vlanId;
    le_utf8_Copy(addrInfo.ifName, addrInfoPtr->ifName, MAX_INTERFACE_NAME_LEN, NULL);
    backend.RespDiagNegative(reqSvcId, &addrInfo, errCode);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send ECUReset response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_ResetSvr::SendResp
(
    taf_diagReset_RxMsgRef_t rxMsgRef,
    uint8_t errCode
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    le_result_t ret;

    taf_ResetRxMsg_t* rxMsgPtr = (taf_ResetRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    TAF_ERROR_IF_RET_VAL(rxMsgPtr == NULL, LE_BAD_PARAMETER, "Invalid rxMsgPtr");

#ifndef LE_CONFIG_DIAG_VSTACK
    taf_ResetSvc_t* servicePtr = (taf_ResetSvc_t*)GetServiceObj(rxMsgPtr->subFunc, rxMsgPtr->addrInfo.vlanId);
#else
    taf_ResetSvc_t* servicePtr = (taf_ResetSvc_t*)GetServiceObj(rxMsgPtr->subFunc, (uint16_t)0);
#endif
    if (servicePtr == NULL)
    {
#ifndef LE_CONFIG_DIAG_VSTACK
        servicePtr = (taf_ResetSvc_t*)GetServiceObj(TAF_DIAGRESET_ALL_RESET, rxMsgPtr->addrInfo.vlanId);
#else
        servicePtr = (taf_ResetSvc_t*)GetServiceObj(TAF_DIAGRESET_ALL_RESET, (uint16_t)0);
#endif
        if(servicePtr == NULL)
        {
            LE_ERROR("Cannot find the service(type:0x%x, vlan id:0x%x)",
                rxMsgPtr->subFunc, rxMsgPtr->addrInfo.vlanId);
            return LE_NOT_FOUND;
        }
    }

    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = rxMsgPtr->addrInfo.ta;
    addrInfo.ta = rxMsgPtr->addrInfo.sa;
    addrInfo.taType = rxMsgPtr->addrInfo.taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = rxMsgPtr->addrInfo.vlanId;
    le_utf8_Copy(addrInfo.ifName, rxMsgPtr->addrInfo.ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif
    if (errCode == 0)
    {
        // Positive Response
        ret = backend.RespDiagPositive(reqSvcId, &addrInfo);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send routine control positive response.(%d)", ret);
            return ret;
        }
    }
    else
    {
        // Negative response.
        ret = backend.RespDiagNegative(reqSvcId, &addrInfo, errCode);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send ECUReset negative response.(%d)", ret);
            return ret;
        }
    }

    // Remove the message from service message list.
    le_dls_Remove(&(servicePtr->rxMsgList), &(rxMsgPtr->link));

    // Free the message
    le_ref_DeleteRef(RxMsgRefMap, rxMsgPtr->rxMsgRef);
    le_mem_Release(rxMsgPtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear reset message list.
 */
//-------------------------------------------------------------------------------------------------
void taf_ResetSvr::ClearResetMsgList
(
    taf_ResetSvc_t* servicePtr
)
{
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear the UDS Rx message of reset list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->rxMsgList);
    while (linkPtr != NULL)
    {
        taf_ResetRxMsg_t* msgPtr = CONTAINER_OF(linkPtr, taf_ResetRxMsg_t, link);
        if (msgPtr != NULL)
        {
            // Free the message
            le_ref_DeleteRef(RxMsgRefMap, msgPtr->rxMsgRef);
            le_mem_Release(msgPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->rxMsgList);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear Vlan list.
 */
//-------------------------------------------------------------------------------------------------
void taf_ResetSvr::ClearVlanList
(
    taf_ResetSvc_t* servicePtr
)
{
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear the vlan id list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->supportedVlanList);
    while (linkPtr != NULL)
    {
        taf_ResetVlanIdNode_t *vlanPtr = CONTAINER_OF(linkPtr, taf_ResetVlanIdNode_t, link);
        if (vlanPtr != NULL)
        {
            le_mem_Release(vlanPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->supportedVlanList);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the created service and release the alloted memory.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_ResetSvr::RemoveSvc
(
    taf_diagReset_ServiceRef_t svcRef
)
{
    LE_DEBUG("RemoveSvc");

    taf_ResetSvc_t* servicePtr = (taf_ResetSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid servicePtr");

    // Release reset message resources.
    ClearResetMsgList(servicePtr);
    ClearVlanList(servicePtr);

    // Clear the registered reset handler
    if (servicePtr->handlerRef != NULL)
    {
        RemoveRxMsgHandler(servicePtr->handlerRef);
        servicePtr->handlerRef = NULL;
    }

    // Clear service object
    le_ref_DeleteRef(SvcRefMap, (void*)servicePtr->svcRef);
    le_mem_Release(servicePtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Session close handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_ResetSvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    LE_DEBUG("OnClientDisconnection");

    auto &reset = taf_ResetSvr::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(reset.SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_ResetSvc_t* servicePtr = (taf_ResetSvc_t *)le_ref_GetValue(iterRef);
        LE_ASSERT(servicePtr != NULL);
        LE_ASSERT(servicePtr->svcRef == le_ref_GetSafeRef(iterRef));

        if (servicePtr->sessionRef == sessionRef)
        {
            reset.RemoveSvc(servicePtr->svcRef);
        }
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * VLAN ID setting.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ResetSvr::SetVlanId
(
    taf_diagReset_ServiceRef_t svcRef,
    uint16_t vlanId
)
{
    taf_ResetSvc_t* servicePtr = (taf_ResetSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid service reference");
    TAF_ERROR_IF_RET_VAL(vlanId == 0, LE_BAD_PARAMETER, "Invalid vlan Id");

#ifndef LE_CONFIG_DIAG_VSTACK

    // Check Vlan Id is valid or not.
    auto& backend = taf_DiagBackend::GetInstance();
    if (!backend.isVlanIdValid(vlanId))
    {
        LE_ERROR("VlanId is unknown");
        return LE_UNSUPPORTED;
    }

    // Check if the vlan is set.
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&servicePtr->supportedVlanList);
    while (linkPtr)
    {
        taf_ResetVlanIdNode_t *vlan = CONTAINER_OF(linkPtr, taf_ResetVlanIdNode_t, link);
        if (vlan != NULL && vlan->vlanId == vlanId)
        {
            return LE_OK;
        }
        linkPtr = le_dls_PeekNext(&servicePtr->supportedVlanList, linkPtr);
    }

    taf_ResetVlanIdNode_t *vlanPtr = (taf_ResetVlanIdNode_t *)le_mem_ForceAlloc(VlanPool);
    if (vlanPtr == NULL)
    {
        LE_DEBUG("Failed to allocate memory.");
        return LE_NO_MEMORY;
    }

    vlanPtr->vlanId = vlanId;
    vlanPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->supportedVlanList, &vlanPtr->link);

    return LE_OK;
#else
    return LE_NOT_IMPLEMENTED;
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * VLAN ID getting.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ResetSvr::GetVlanIdFromMsg
(
    taf_diagReset_RxMsgRef_t rxMsgRef,
    uint16_t* vlanIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(vlanIdPtr == NULL, LE_BAD_PARAMETER, "Invalid vlanIdPtr");

#ifndef LE_CONFIG_DIAG_VSTACK
    taf_ResetRxMsg_t* rxMsgPtr = (taf_ResetRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the rxMsg in reset service");
        return LE_NOT_FOUND;
    }

    *vlanIdPtr = rxMsgPtr->addrInfo.vlanId;

    return LE_OK;
#else
    return LE_NOT_IMPLEMENTED;
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_ResetSvr::Init()
{
    // Create memory pools.
    SvcPool = le_mem_CreatePool("ResetSvcPool", sizeof(taf_ResetSvc_t));
    RxMsgPool = le_mem_CreatePool("ResetRxMsgPool", sizeof(taf_ResetRxMsg_t));
    ReqHandlerPool = le_mem_CreatePool("ResetReqHandlerPool", sizeof(taf_ResetReqHandler_t));
    VlanPool = le_mem_CreatePool("ResetVlanPool", sizeof(taf_ResetVlanIdNode_t));

    // Create reference maps
    SvcRefMap = le_ref_CreateMap("ResetSvcRefMap", DEFAULT_SVC_REF_CNT);
    RxMsgRefMap = le_ref_CreateMap("ResetRxMsgRefMap", DEFAULT_RX_MSG_REF_CNT);
    ReqHandlerRefMap = le_ref_CreateMap("ResetReqHandlerRefMap", DEFAULT_RX_HANDLER_REF_CNT);

    // Create event and add the event handler.
    ResetEvent = le_event_CreateIdWithRefCounting("ResetEvent");
    ResetEventHandlerRef = le_event_AddHandler("ResetEventHandlerRef", ResetEvent,
            taf_ResetSvr::RxReqEventHandler);

    // Set session close handler
    le_msg_AddServiceCloseHandler(taf_diagReset_GetServiceRef(), OnClientDisconnection, NULL);

    auto& backend = taf_DiagBackend::GetInstance();
    backend.RegisterUdsService(reqSvcId, this);
    LE_DEBUG("tafResetSvr Init completed!");
}
