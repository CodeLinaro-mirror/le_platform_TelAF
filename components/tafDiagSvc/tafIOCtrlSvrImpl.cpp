/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <string>
#include "tafIOCtrlSvr.hpp"
#include "configuration.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of TelAF IOCtrl server.
 */
//--------------------------------------------------------------------------------------------------
taf_IOCtrlSvr &taf_IOCtrlSvr::GetInstance
(
)
{
    static taf_IOCtrlSvr instance;

    return instance;
}

//-------------------------------------------------------------------------------------------------
/**
 * Create a reference for the service, or get the reference of a service if the reference already
 * exist.
 */
//-------------------------------------------------------------------------------------------------
taf_diagIOCtrl_ServiceRef_t taf_IOCtrlSvr::GetService
(
    uint16_t dataID
)
{
    LE_DEBUG("Gets the IOCtrl service for DataID 0x%x", dataID);

    try
    {
        // If get_io_entry() cannot find the dataID, it will throw std::out_of_range,
        cfg::get_io_entry(dataID);
    }
    catch (const std::exception& e)
    {
        LE_ERROR("DataId 0x%x is not configured in YAML file. Exception: %s",
                 dataID, e.what());
        return NULL;
    }

    // Check if a service object already exists for this client session.
    taf_IOCtrlSvc_t* servicePtr = GetServiceObj(dataID, taf_diagIOCtrl_GetClientSessionRef());

    // Create a service object if it doesn't exist in the list.
    if (servicePtr == NULL)
    {
        servicePtr = (taf_IOCtrlSvc_t*)le_mem_ForceAlloc(SvcPool);
        if (servicePtr == nullptr) {
            LE_CRIT("Memory allocation failed for new IO Control service object!");
            return NULL;
        }
        memset(servicePtr, 0, sizeof(taf_IOCtrlSvc_t));

        // Init the service Rx Handler.
        servicePtr->handlerRef = NULL;
        servicePtr->dataID = dataID;

        // Init message list.
        servicePtr->reqMsgList  = LE_DLS_LIST_INIT;
        servicePtr->supportedVlanList = LE_DLS_LIST_INIT;

        // Attach the service to the client.
        servicePtr->sessionRef = taf_diagIOCtrl_GetClientSessionRef();

        // Create a Safe Reference for this service object
        servicePtr->svcRef = (taf_diagIOCtrl_ServiceRef_t)le_ref_CreateRef(SvcRefMap,
                servicePtr);
        LE_DEBUG("svcRef %p of client %p is created for DataId 0x%x.",
                servicePtr->svcRef, servicePtr->sessionRef, servicePtr->dataID);
    }

    return servicePtr->svcRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the service instacnce object, if the service reference already created and return the
 * object pointer.
 */
//-------------------------------------------------------------------------------------------------
taf_IOCtrlSvc_t* taf_IOCtrlSvr::GetServiceObj
(
    uint16_t dataID,
    le_msg_SessionRef_t sessionRef
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_IOCtrlSvc_t* servicePtr = (taf_IOCtrlSvc_t *)le_ref_GetValue(iterRef);
        if ((servicePtr != NULL) && (servicePtr->dataID == dataID)
            && (sessionRef == servicePtr->sessionRef))
        {
            return servicePtr;
        }
    }

    return NULL;
}

taf_IOCtrlSvc_t* taf_IOCtrlSvr::GetServiceObj
(
    uint16_t dataID,
    uint16_t vlanId
)
{
    bool isFound = false;
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_IOCtrlSvc_t* servicePtr = (taf_IOCtrlSvc_t *)le_ref_GetValue(iterRef);
        if ((servicePtr != NULL) && (servicePtr->dataID == dataID))
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
                taf_IOCtrlVlanIdNode_t *vlan = CONTAINER_OF(linkPtr,
                    taf_IOCtrlVlanIdNode_t, link);
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

    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * UDS stack handler function
 */
//-------------------------------------------------------------------------------------------------
void taf_IOCtrlSvr::UDSMsgHandler
(
    const taf_uds_AddrInfo_t* addrPtr,
    uint8_t sid,
    uint8_t* msgPtr,
    size_t msgLen
)
{
    TAF_ERROR_IF_RET_NIL(addrPtr == NULL, "Invalid addrPtr");
    TAF_ERROR_IF_RET_NIL(msgPtr == NULL, "Invalid msgPtr");

    uint8_t msgPos = 1;  // Skip sid
    uint8_t errCode = 0;

    if (sid == reqIOCtrlSvcId)
    {
        LE_DEBUG("UDSMsgHandler, IO control service");

        // check enable condition
#ifndef LE_CONFIG_DIAG_FEATURE_A
        // Diag instance
        auto &diag = taf_DiagSvr::GetInstance();
        uint16_t dataId = (msgPtr[msgPos] << 8) + msgPtr[msgPos + 1];

        try
        {
            const EnableConditionData& cond = cfg::get_event_enable_conditions(dataId);

            if (!cond.and_conditions.empty())
            {
                LE_DEBUG("Check IO Ctrl enable condition status based on AND operation");
                for (uint8_t enableId : cond.and_conditions)
                {
                    LE_DEBUG("Enable condition id = 0x%x", enableId);
                    if (!diag.GetEnableConditionStatus(enableId))
                    {
                        errCode = cfg::get_nrc_by_condition_id(enableId);
                        LE_WARN("Enable id %d condition is false, send nrc 0x%x",
                            enableId, errCode);
                        SendNRCResp(sid, addrPtr, errCode);
                        return;
                    }
                }
            }
            else if (!cond.or_conditions.empty())
            {
                LE_DEBUG("Check IO ctrl enable condition status based on OR operation");
                bool enableStatus = false;
                uint8_t failingId = 0;

                for (uint8_t enableId : cond.or_conditions)
                {
                    if (diag.GetEnableConditionStatus(enableId))
                    {
                        enableStatus = true;
                        break;
                    }
                    failingId = enableId;   // remember the last id for error reporting
                }

                if (!enableStatus && failingId != 0)
                {
                    errCode = cfg::get_nrc_by_condition_id(failingId);
                    LE_WARN("Enable id %d condition is false, send nrc 0x%x",
                        failingId, errCode);
                    SendNRCResp(sid, addrPtr, errCode);
                    return;
                }
            }
        }
        catch (const std::exception& e)
        {
            LE_WARN("Enable condition does not define for IO ctrl ID: 0x%x, Exception: %s",
                dataId, e.what());
        }
#endif

        taf_IOCtrlRxMsg_t* rxIOCtrlMsgPtr = NULL;

        rxIOCtrlMsgPtr = (taf_IOCtrlRxMsg_t*)le_mem_ForceAlloc(RxMsgPool);
        memset(rxIOCtrlMsgPtr, 0, sizeof(taf_IOCtrlRxMsg_t));

        memcpy(&rxIOCtrlMsgPtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
        // ServiceId
        rxIOCtrlMsgPtr->serviceId = sid;

        // dataIdentifier
        rxIOCtrlMsgPtr->dataID = (msgPtr[msgPos] << 8) + msgPtr[msgPos + 1];
        msgPos += 2;

        // inputOutputControlParameter
        rxIOCtrlMsgPtr->ioCtrlParameter = msgPtr[msgPos];
        msgPos += 1;

        // This parameter record contains control state.
        if ((msgLen - msgPos) > TAF_DIAGIOCTRL_MAX_CONTROL_RECORD_SIZE)
        {
            LE_WARN("Message length(%" PRIuS ") is not in correct format, send NRC %x",
                    msgLen - msgPos, TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
            errCode = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            SendNRCResp(sid, addrPtr, errCode);
            le_mem_Release(rxIOCtrlMsgPtr);
            return;
        }

        // controlState
        if(rxIOCtrlMsgPtr->ioCtrlParameter == 0x00 || rxIOCtrlMsgPtr->ioCtrlParameter == 0x01
                || rxIOCtrlMsgPtr->ioCtrlParameter == 0x02)
        {
            memset(rxIOCtrlMsgPtr->controlState, 0, TAF_DIAGIOCTRL_MAX_CONTROL_RECORD_SIZE);
            rxIOCtrlMsgPtr->controlStateSize = 0;
        }
        else if (rxIOCtrlMsgPtr->ioCtrlParameter == 0x03)
        {
            // Get the controlState size from config module
            try
            {
                uint32_t byteSize = cfg::get_io_control_state_size(rxIOCtrlMsgPtr->dataID);
                rxIOCtrlMsgPtr->controlStateSize = byteSize;
            }
            catch (const std::exception& e)
            {
                LE_WARN("Exception: %s, NRC %x", e.what(), TAF_DIAG_REQUEST_OUT_OF_RANGE);
                errCode = TAF_DIAG_REQUEST_OUT_OF_RANGE;
                SendNRCResp(sid, addrPtr, errCode);
                le_mem_Release(rxIOCtrlMsgPtr);
                return;
            }

            if ((rxIOCtrlMsgPtr->controlStateSize + msgPos) > msgLen)
            {
                LE_WARN("Received msg length(%" PRIuS ") is not in correct format, send NRC %x",
                        msgLen, TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
                errCode = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
                SendNRCResp(sid, addrPtr, errCode);
                le_mem_Release(rxIOCtrlMsgPtr);
                return;
            }

            for (int i = 0; i<(rxIOCtrlMsgPtr->controlStateSize); i++)
            {
                rxIOCtrlMsgPtr->controlState[i] = msgPtr[msgPos];
                msgPos+= 1;
            }
        }
        else
        {
            LE_WARN("controlState is for ISOReserved inputOutputControlParameter, send NRC %x",
                    TAF_DIAG_REQUEST_OUT_OF_RANGE);
            errCode = TAF_DIAG_REQUEST_OUT_OF_RANGE;
            SendNRCResp(sid, addrPtr, errCode);
            le_mem_Release(rxIOCtrlMsgPtr);
            return;
        }

        // controlEnableMaskRecord.
        rxIOCtrlMsgPtr->maskRecordSize = (msgLen - msgPos)/sizeof(uint8_t);
        for (size_t i = 0; i < (rxIOCtrlMsgPtr->maskRecordSize); i++)
        {
            rxIOCtrlMsgPtr->maskRecord[i] = msgPtr[msgPos];
            msgPos += 1;
        }

        rxIOCtrlMsgPtr->link = LE_DLS_LINK_INIT;
        rxIOCtrlMsgPtr->rxMsgRef = (taf_diagIOCtrl_RxMsgRef_t)le_ref_CreateRef(
                RxMsgRefMap, rxIOCtrlMsgPtr);

        // Report the IOCtrl request message to message handler in service layer.
        le_event_ReportWithRefCounting(ReqEvent, rxIOCtrlMsgPtr);
    }
    else
    {
        LE_WARN("Service(0x%x) is invalid, send NRC %x", sid, TAF_DIAG_SERVICE_NOT_SUPPORTED);
        errCode = TAF_DIAG_SERVICE_NOT_SUPPORTED; // ServiceNotSupported
        SendNRCResp(sid, addrPtr, errCode);
        return;
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a Rx handler for a given service (IOCtrl).
 */
//-------------------------------------------------------------------------------------------------
taf_diagIOCtrl_RxMsgHandlerRef_t taf_IOCtrlSvr::AddRxMsgHandler
(
    taf_diagIOCtrl_ServiceRef_t svcRef,
    taf_diagIOCtrl_RxMsgHandlerFunc_t handlerPtr,
    void* contextPtr
)
{

    taf_IOCtrlSvc_t* servicePtr = (taf_IOCtrlSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, NULL, "Invalid service reference provided");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    if (servicePtr->handlerRef != NULL)
    {
        LE_ERROR("Rx IOCtrl handler is already registered");

        return NULL;
    }

    taf_IOCtrlHandler_t* handlerObjPtr = NULL;
    handlerObjPtr = (taf_IOCtrlHandler_t*)le_mem_ForceAlloc(ReqHandlerPool);
    memset(handlerObjPtr, 0, sizeof(taf_IOCtrlHandler_t));

    // Initialize the RxHandler object.
    handlerObjPtr->svcRef     = svcRef;
    handlerObjPtr->func       = handlerPtr;
    handlerObjPtr->ctxPtr     = contextPtr;
    handlerObjPtr->handlerRef = (taf_diagIOCtrl_RxMsgHandlerRef_t)le_ref_CreateRef(
            ReqHandlerRefMap, handlerObjPtr);

    // Attach handler to service.
    servicePtr->handlerRef = handlerObjPtr->handlerRef;

    LE_DEBUG("IOCtrl: Registered Rx Handler for data ID 0x%x", servicePtr->dataID);

    return handlerObjPtr->handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * IOCtrl event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_IOCtrlSvr::RxIOCtrlEventHandler
(
    void* reportPtr
)
{
    auto &ioCtrl = taf_IOCtrlSvr::GetInstance();

    taf_IOCtrlRxMsg_t* rxIOCtrlMsgPtr = (taf_IOCtrlRxMsg_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(rxIOCtrlMsgPtr == NULL, "rxIOCtrlMsgPtr is Null");

    taf_IOCtrlSvc_t* servicePtr = NULL;
    taf_IOCtrlHandler_t* handlerObjPtr = NULL;

#ifndef LE_CONFIG_DIAG_VSTACK
    servicePtr = (taf_IOCtrlSvc_t*)ioCtrl.GetServiceObj(rxIOCtrlMsgPtr->dataID,
        rxIOCtrlMsgPtr->addrInfo.vlanId);
#else
    servicePtr = (taf_IOCtrlSvc_t*)ioCtrl.GetServiceObj(rxIOCtrlMsgPtr->dataID, (uint16_t)0);
#endif
    if (servicePtr == NULL)
    {
        LE_WARN("Not found registered IOCtrl service for this request, send NRC %x",
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        // UDS_0x2F_NRC_21: service pointer is null
        ioCtrl.SendNRCResp(rxIOCtrlMsgPtr->serviceId, &(rxIOCtrlMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(ioCtrl.RxMsgRefMap, rxIOCtrlMsgPtr->rxMsgRef);
        le_mem_Release(rxIOCtrlMsgPtr);
        return;
    }

    if (servicePtr->handlerRef == NULL)
    {
        LE_WARN("Did not register handler for IOCtrl service, send NRC %x",
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        // UDS_0x2F_NRC_21: handler is not registered
        ioCtrl.SendNRCResp(rxIOCtrlMsgPtr->serviceId, &(rxIOCtrlMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(ioCtrl.RxMsgRefMap, rxIOCtrlMsgPtr->rxMsgRef);
        le_mem_Release(rxIOCtrlMsgPtr);
        return;
    }

    // Lookup message handler of this service
    handlerObjPtr =
            (taf_IOCtrlHandler_t*)le_ref_Lookup(ioCtrl.ReqHandlerRefMap,
                    servicePtr->handlerRef);
    if (handlerObjPtr == NULL || handlerObjPtr->func == NULL)
    {
        LE_ERROR("Can not find IO control handler object, send NRC %x",
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        // UDS_0x2F_NRC_21: handler is null
        ioCtrl.SendNRCResp(rxIOCtrlMsgPtr->serviceId, &(rxIOCtrlMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(ioCtrl.RxMsgRefMap, rxIOCtrlMsgPtr->rxMsgRef);
        le_mem_Release(rxIOCtrlMsgPtr);
        return;
    }

    // Add the message in service message list and notify to application.
    rxIOCtrlMsgPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->reqMsgList, &rxIOCtrlMsgPtr->link);
    handlerObjPtr->func(rxIOCtrlMsgPtr->rxMsgRef, rxIOCtrlMsgPtr->dataID,
            rxIOCtrlMsgPtr->ioCtrlParameter, handlerObjPtr->ctxPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the Rx handler for a given service (IOctrl).
 */
//-------------------------------------------------------------------------------------------------
void taf_IOCtrlSvr::RemoveRxMsgHandler
(
    taf_diagIOCtrl_RxMsgHandlerRef_t handlerRef
)
{
    LE_DEBUG("RemoveRxMsgHandler!");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");

    taf_IOCtrlSvc_t* servicePtr = NULL;
    taf_IOCtrlHandler_t* handlerObjPtr = NULL;

    handlerObjPtr = (taf_IOCtrlHandler_t*)le_ref_Lookup(ReqHandlerRefMap,
            handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerObjPtr == NULL, "Invalid handlerObjPtr");

    servicePtr = (taf_IOCtrlSvc_t*)le_ref_Lookup(SvcRefMap, handlerObjPtr->svcRef);
    if (servicePtr == NULL)
    {
        LE_WARN("The handler is not belong to this service.");
        le_ref_DeleteRef(ReqHandlerRefMap, handlerRef);
        le_mem_Release(handlerObjPtr);

        return;
    }

    // Detach the handler from service.
    servicePtr->handlerRef = NULL;

    // Clear Rx Handler resources
    handlerObjPtr->handlerRef = NULL;
    handlerObjPtr->svcRef     = NULL;
    handlerObjPtr->func       = NULL;
    handlerObjPtr->ctxPtr     = NULL;

    // Free the handler.
    le_ref_DeleteRef(ReqHandlerRefMap, handlerRef);
    le_mem_Release(handlerObjPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the controlState of the Rx IOCtrl message.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_IOCtrlSvr::GetCtrlState
(
    taf_diagIOCtrl_RxMsgRef_t rxMsgRef,
    uint8_t* controlStatePtr,
    size_t* controlStateSizePtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(controlStatePtr == NULL, LE_BAD_PARAMETER, "Invalid controlStatePtr");
    TAF_ERROR_IF_RET_VAL(controlStateSizePtr == NULL, LE_BAD_PARAMETER,
            "Invalid controlStateSizePtr");

    taf_IOCtrlRxMsg_t* rxIOCtrlMsgPtr =
            (taf_IOCtrlRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxIOCtrlMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    memcpy(controlStatePtr, rxIOCtrlMsgPtr->controlState, rxIOCtrlMsgPtr->controlStateSize);
    *controlStateSizePtr = rxIOCtrlMsgPtr->controlStateSize;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the controlEnableMaskRecord of the Rx IOCtrl message.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_IOCtrlSvr::GetCtrlEnableMaskRecd
(
    taf_diagIOCtrl_RxMsgRef_t rxMsgRef,
    uint8_t* maskRecordPtr,
    size_t* maskRecordSizePtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(maskRecordPtr == NULL, LE_BAD_PARAMETER, "Invalid maskRecordPtr");
    TAF_ERROR_IF_RET_VAL(maskRecordSizePtr == NULL, LE_BAD_PARAMETER,
            "Invalid maskRecordSizePtr");

    taf_IOCtrlRxMsg_t* rxIOCtrlMsgPtr =
            (taf_IOCtrlRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxIOCtrlMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    memcpy(maskRecordPtr, rxIOCtrlMsgPtr->maskRecord, rxIOCtrlMsgPtr->maskRecordSize);
    *maskRecordSizePtr = rxIOCtrlMsgPtr->maskRecordSize;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send IOCtrl response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_IOCtrlSvr::SendResp
(
    taf_diagIOCtrl_RxMsgRef_t rxMsgRef,
    uint8_t errCode,
    const uint8_t* dataPtr,
    size_t dataSize
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    le_result_t ret = LE_OK;

    taf_IOCtrlRxMsg_t* rxIOCtrlMsgPtr =
            (taf_IOCtrlRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxIOCtrlMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    taf_IOCtrlSvc_t* servicePtr = NULL;

#ifndef LE_CONFIG_DIAG_VSTACK
    servicePtr = (taf_IOCtrlSvc_t*)GetServiceObj(rxIOCtrlMsgPtr->dataID,
        rxIOCtrlMsgPtr->addrInfo.vlanId);
#else
    servicePtr = (taf_IOCtrlSvc_t*)GetServiceObj(rxIOCtrlMsgPtr->dataID, (uint16_t)0);
#endif
    if (servicePtr == NULL)
    {
        LE_ERROR("Not found registered IOCtrl service for this request!");
        return LE_NOT_FOUND;
    }

    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = rxIOCtrlMsgPtr->addrInfo.ta;
    addrInfo.ta = rxIOCtrlMsgPtr->addrInfo.sa;
    addrInfo.taType = rxIOCtrlMsgPtr->addrInfo.taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = rxIOCtrlMsgPtr->addrInfo.vlanId;
    le_utf8_Copy(addrInfo.ifName, rxIOCtrlMsgPtr->addrInfo.ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif

    if (errCode == 0)
    {
        // Positive response.
        if (dataPtr == NULL || dataSize == 0)
        {
            ret = backend.RespDiagPositive(reqIOCtrlSvcId, &addrInfo);
            if (ret != LE_OK)
            {
                LE_ERROR("Failed to send I/O Ctrl positive response.(%d)", ret);
                return ret;
            }
        }
        else
        {
            ret = backend.RespDiagPositive(reqIOCtrlSvcId, &addrInfo, dataPtr, dataSize);
            if (ret != LE_OK)
            {
                LE_ERROR("Failed to send I/O Ctrl positive response.(%d)", ret);
                return ret;
            }
        }
    }
    else
    {
        // Negative response.
        ret = backend.RespDiagNegative(reqIOCtrlSvcId, &addrInfo, errCode);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send IOCtrl negative response.(%d)", ret);
        }
    }

    // Remove the message from service message list.
    le_dls_Remove(&(servicePtr->reqMsgList), &(rxIOCtrlMsgPtr->link));

    // Free the message
    le_ref_DeleteRef(RxMsgRefMap, rxIOCtrlMsgPtr->rxMsgRef);
    le_mem_Release(rxIOCtrlMsgPtr);

    return ret;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send NRC response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_IOCtrlSvr::SendNRCResp
(
    uint8_t sid,
    const taf_uds_AddrInfo_t* addrInfoPtr,
    uint8_t errCode
)
{
    TAF_ERROR_IF_RET_VAL(addrInfoPtr == NULL, LE_BAD_PARAMETER, "Invalid addrInfoPtr");

    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = addrInfoPtr->ta;
    addrInfo.ta = addrInfoPtr->sa;
    addrInfo.taType = addrInfoPtr->taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = addrInfoPtr->vlanId;
    le_utf8_Copy(addrInfo.ifName, addrInfoPtr->ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif

    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();
    backend.RespDiagNegative(sid, &addrInfo, errCode);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear IOCtrl message list.
 */
//-------------------------------------------------------------------------------------------------
void taf_IOCtrlSvr::ClearMsgList
(
    taf_IOCtrlSvc_t* servicePtr
)
{
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear the UDS Rx message of IOCtrl list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->reqMsgList);
    while (linkPtr != NULL)
    {
        taf_IOCtrlRxMsg_t* rxIOCtrlMsgPtr = CONTAINER_OF(linkPtr, taf_IOCtrlRxMsg_t, link);
        if (rxIOCtrlMsgPtr != NULL)
        {
            // Free the message
            le_ref_DeleteRef(RxMsgRefMap, rxIOCtrlMsgPtr->rxMsgRef);
            le_mem_Release(rxIOCtrlMsgPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->reqMsgList);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear vlan list.
 */
//-------------------------------------------------------------------------------------------------
void taf_IOCtrlSvr::ClearVlanList
(
    taf_IOCtrlSvc_t* servicePtr
)
{
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear the vlan id list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->supportedVlanList);
    while (linkPtr != NULL)
    {
        taf_IOCtrlVlanIdNode_t *vlanPtr = CONTAINER_OF(linkPtr, taf_IOCtrlVlanIdNode_t, link);
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
le_result_t taf_IOCtrlSvr::RemoveSvc
(
    taf_diagIOCtrl_ServiceRef_t svcRef
)
{
    taf_IOCtrlSvc_t* servicePtr = (taf_IOCtrlSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid servicePtr");

    // Release IOCtrl message resources.
    ClearMsgList(servicePtr);
    ClearVlanList(servicePtr);

    // Clear the registered IOCtrl handler
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
void taf_IOCtrlSvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    LE_DEBUG("OnClientDisconnection");

    auto &ioCtrl = taf_IOCtrlSvr::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ioCtrl.SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_IOCtrlSvc_t* servicePtr = (taf_IOCtrlSvc_t *)le_ref_GetValue(iterRef);
        LE_ASSERT(servicePtr != NULL);
        LE_ASSERT(servicePtr->svcRef == le_ref_GetSafeRef(iterRef));

        if (servicePtr->sessionRef == sessionRef)
        {
            ioCtrl.RemoveSvc(servicePtr->svcRef);
        }
    }

    return;
}

le_result_t taf_IOCtrlSvr::SetVlanId
(
    taf_diagIOCtrl_ServiceRef_t svcRef,
    uint16_t vlanId
)
{
    taf_IOCtrlSvc_t* servicePtr = (taf_IOCtrlSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
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
        taf_IOCtrlVlanIdNode_t *vlan = CONTAINER_OF(linkPtr,
            taf_IOCtrlVlanIdNode_t, link);
        if (vlan != NULL && vlan->vlanId == vlanId)
        {
            return LE_OK;
        }
        linkPtr = le_dls_PeekNext(&servicePtr->supportedVlanList, linkPtr);
    }

    taf_IOCtrlVlanIdNode_t *vlanPtr = (taf_IOCtrlVlanIdNode_t *)
        le_mem_ForceAlloc(vlanPool);
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

le_result_t taf_IOCtrlSvr::GetVlanIdFromMsg
(
    taf_diagIOCtrl_RxMsgRef_t rxMsgRef,
    uint16_t* vlanIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(vlanIdPtr == NULL, LE_BAD_PARAMETER, "Invalid vlanIdPtr");

#ifndef LE_CONFIG_DIAG_VSTACK
    taf_IOCtrlRxMsg_t* rxMsgPtr = (taf_IOCtrlRxMsg_t*)
        le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the rxMsg in IO ctrl service");
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
void taf_IOCtrlSvr::Init
(
    void
)
{
    // Create memory pools.
    SvcPool = le_mem_CreatePool("IOCtrlSvcPool", sizeof(taf_IOCtrlSvc_t));
    RxMsgPool = le_mem_CreatePool("IOCtrlRxMsgPool", sizeof(taf_IOCtrlRxMsg_t));
    ReqHandlerPool = le_mem_CreatePool("IOCtrlReqHandlerPool",
            sizeof(taf_IOCtrlHandler_t));
    vlanPool = le_mem_CreatePool("IOCtrlVlanPool", sizeof(taf_IOCtrlVlanIdNode_t));

    // Create reference maps
    SvcRefMap = le_ref_CreateMap("IOCtrlSvcRefMap", DEFAULT_SVC_REF_CNT);
    RxMsgRefMap = le_ref_CreateMap("IOCtrlRxMsgRefMap", DEFAULT_RX_MSG_REF_CNT);
    ReqHandlerRefMap = le_ref_CreateMap("IOCtrlReqHandlerRefMap",
            DEFAULT_RX_HANDLER_REF_CNT);

    // Create the event and add event handler for IOCtrl (0x2F).
    ReqEvent = le_event_CreateIdWithRefCounting("IOCtrlEvent");
    ReqEventHandlerRef = le_event_AddHandler("IOCtrlEventHandlerRef", ReqEvent,
            taf_IOCtrlSvr::RxIOCtrlEventHandler);

    // Set session close handler
    le_msg_AddServiceCloseHandler(taf_diagIOCtrl_GetServiceRef(), OnClientDisconnection, NULL);

    auto& backend = taf_DiagBackend::GetInstance();
    backend.RegisterUdsService(reqIOCtrlSvcId, this);

    LE_DEBUG("taf_IOCtrlSvr Init completed!");
}
