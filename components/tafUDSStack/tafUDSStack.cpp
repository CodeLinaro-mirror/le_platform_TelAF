/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include "tafUDSStack.h"
#include "tafUDSCommunicationMgr.hpp"

using namespace taf::uds;

le_result_t taf_uds_SendDiagResp
(
    const taf_uds_AddrInfo_t*  addrInfoPtr,    ///< [IN] Logical address information pointer.
    const taf_uds_DiagMsg_t*   diagMsgPtr,     ///< [IN] Diagnostic message pointer.
    taf_uds_ServiceId_t serviceId,             ///< [IN] Service Id.
    uint8_t err                                ///< [IN] Error code.
)
{
    LE_DEBUG("taf_uds_SendDiagResp");

    if(addrInfoPtr == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_BAD_PARAMETER;
    }

    LE_DEBUG("ifName=%s", addrInfoPtr->ifName);
    auto udsCmMgr = UdsCommunicationMgr::GetInstance(addrInfoPtr->ifName);
    if(udsCmMgr == NULL)
    {
        LE_ERROR("Can't get instance by ifName %s", addrInfoPtr->ifName);
        return LE_FAULT;
    }

    memcpy(&udsCmMgr->udsRespAddrInfo, addrInfoPtr, sizeof(*addrInfoPtr));
    le_utf8_Copy(udsCmMgr->udsRespAddrInfo.ifName, addrInfoPtr->ifName, MAX_INTERFACE_NAME_LEN,
            NULL);

    if (diagMsgPtr != NULL)
    {
        return udsCmMgr->SendUDSResp(addrInfoPtr->ifName, serviceId, err,
                diagMsgPtr->dataPtr, diagMsgPtr->dataLen);
    }
    else
    {
        return udsCmMgr->SendUDSResp(addrInfoPtr->ifName, serviceId, err, NULL, 0);
    }
}

le_result_t taf_uds_SetData
(
    taf_uds_AddrInfo_t*  addrInfoPtr,             ///< [IN] Logical address information pointer.
    const taf_uds_DiagMsg_t*   diagMsgPtr,        ///< [IN] Data pointer.
    taf_uds_DataType_t dataType                   ///< [IN] Data type.
)
{
    LE_DEBUG("taf_uds_SetData");

    if(addrInfoPtr == NULL || diagMsgPtr == NULL || diagMsgPtr->dataPtr == NULL)
    {
        LE_ERROR("Null pointer");
        return LE_BAD_PARAMETER;
    }

    //If interface name is not set, get it by VLAN Id
    if(addrInfoPtr->ifName[0] == '\0')
    {
        if(UdsCommunicationMgr::GetIfNameByVlanId(addrInfoPtr->vlanId, addrInfoPtr->ifName) !=
                LE_OK)
        {
            LE_ERROR("Can't get ifName by vlanId %d", addrInfoPtr->vlanId);
            return LE_FAULT;
        }
    }

    LE_DEBUG("ifName=%s, vlanId=%d", addrInfoPtr->ifName, addrInfoPtr->vlanId);

    auto udsCmMgr = UdsCommunicationMgr::GetInstance(addrInfoPtr->ifName);
    if(udsCmMgr == NULL)
    {
        LE_ERROR("Can't get instance by ifName %s", addrInfoPtr->ifName);
        return LE_FAULT;
    }

    return udsCmMgr->SetUDSData(addrInfoPtr->ifName, (uint8_t)dataType, diagMsgPtr->dataPtr,
            diagMsgPtr->dataLen);

}

taf_uds_DiagIndicationHandlerRef_t taf_uds_AddDiagIndicationHandler
(
    taf_uds_DiagIndicationHandlerFunc_t  indicationHandlerPtr,   ///< [IN] Hander function.
    void*                                userPtr                 ///< [IN] User-defined pointer.
)
{
    LE_DEBUG("taf_uds_AddDiagIndicationHandler");

    void* handlerRef;

    if(indicationHandlerPtr == NULL)
    {
        LE_ERROR("indicationHandlerPtr is Null");
        return NULL;
    }

    if(UdsCommunicationMgr::UdsAddDiagIndicationHandler() != LE_OK)
    {
        LE_ERROR("Add doip indication");
        return NULL;
    }

    // Remove previous handler reference
    if (UdsCommunicationMgr::udsIndicationHandler.safeRef != NULL &&
        le_ref_Lookup(UdsCommunicationMgr::udsHandlerRefMap,
                UdsCommunicationMgr::udsIndicationHandler.safeRef))
    {
        le_ref_DeleteRef(UdsCommunicationMgr::udsHandlerRefMap,
                UdsCommunicationMgr::udsIndicationHandler.safeRef);
    }

    handlerRef = le_ref_CreateRef(UdsCommunicationMgr::udsHandlerRefMap,
            &UdsCommunicationMgr::udsIndicationHandler);
    UdsCommunicationMgr::udsIndicationHandler.funcPtr =
            (taf_doip_DiagIndicationHandlerFunc_t)indicationHandlerPtr;
    UdsCommunicationMgr::udsIndicationHandler.ctxPtr = userPtr;
    UdsCommunicationMgr::udsIndicationHandler.safeRef = handlerRef;

    return (taf_uds_DiagIndicationHandlerRef_t)handlerRef;
}

void taf_uds_RemoveDiagIndicationHandler
(
    taf_uds_DiagIndicationHandlerRef_t handerRef   ///< [IN] The handler reference.
)
{
    LE_DEBUG("taf_uds_RemoveDiagIndicationHandler");


    taf_UDSIndicationHandler_t* handlerPtr =
            (taf_UDSIndicationHandler_t*)le_ref_Lookup(UdsCommunicationMgr::udsHandlerRefMap,
            handerRef);

    if (handlerPtr != NULL)
    {
        le_ref_DeleteRef(UdsCommunicationMgr::udsHandlerRefMap, handlerPtr->safeRef);
        handlerPtr->safeRef = NULL;
    }

    return;
}

le_result_t taf_uds_Start
(
    const char* configPathPtr
)
{
    LE_DEBUG("taf_uds_Start");

    return UdsCommunicationMgr::UdsStart(configPathPtr);
}

le_result_t taf_uds_Stop
(
)
{
    LE_DEBUG("taf_uds_Stop");

    return UdsCommunicationMgr::UdsStop();
}

void taf_uds_GetFileXferActiveStateList
(
    le_dls_List_t* fileXferStateListPtr
)
{
    LE_DEBUG("taf_uds_GetFileXferActiveState");

    return UdsCommunicationMgr::GetFileXferActiveStateList(fileXferStateListPtr);
}

void taf_uds_GetVlanIdList
(
    le_dls_List_t* vlanIDListPtr
)
{
    LE_DEBUG("taf_uds_GetVlanIdList");

    return UdsCommunicationMgr::GetVlanIdList(vlanIDListPtr);
}

COMPONENT_INIT
{
    LE_INFO("UDS component init once start...");

    LE_INFO("UDS component init end...");
}
