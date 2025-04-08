/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDiagBackend.hpp"

#ifndef LE_CONFIG_DIAG_VSTACK
#include "tafUDSStack.h"
#endif

using namespace tafsvc;
using namespace std;

//--------------------------------------------------------------------------------------------------
/**
 * Get the single instance of TelAF DiagUpdate server.
 */
//--------------------------------------------------------------------------------------------------
taf_DiagBackend& taf_DiagBackend::GetInstance()
{
    static taf_DiagBackend instance;
    return instance;
}

#ifndef LE_CONFIG_DIAG_VSTACK
void taf_DiagBackend::UdsIndicationHanler
(
    const taf_uds_AddrInfo_t*  addrInfoPtr,
    const taf_uds_DiagMsg_t* diagMsgPtr,
    le_result_t result,
    void* userPtr
)
{
    uint8_t sid = 0;
    taf_uds_AddrInfo_t addrInfo;
    taf_DiagBackend& backend = taf_DiagBackend::GetInstance();

    LE_DEBUG("Enter UdsIndicationHanler");

    if (addrInfoPtr == NULL || diagMsgPtr == NULL)
    {
        LE_ERROR("Bad parameter, invalid indication.");
        return;
    }

    if (result != LE_OK)
    {
        LE_ERROR("The result of this message is incorrect.(%d)", result);
        return;
    }

    if (diagMsgPtr->dataLen == 0)
    {
        LE_ERROR("Message length%" PRIuS " is insufficient.", diagMsgPtr->dataLen);
        addrInfo.sa = addrInfoPtr->ta;
        addrInfo.ta = addrInfoPtr->sa;
        addrInfo.taType = addrInfoPtr->taType;
        addrInfo.vlanId = addrInfoPtr->vlanId;
        le_utf8_Copy(addrInfo.ifName, addrInfoPtr->ifName, MAX_INTERFACE_NAME_LEN, NULL);
        backend.RespDiagNegative(sid, &addrInfo, TAF_DIAG_SERVICE_NOT_SUPPORTED);
        return;
    }

    // Get the Service Id and indicate the message to correct service.
    sid = diagMsgPtr->dataPtr[0];

    auto elem = backend.svcMap.find(sid);
    if (elem == backend.svcMap.end())
    {
        LE_ERROR("This service(%d) handler is unregistered", sid);
        addrInfo.sa = addrInfoPtr->ta;
        addrInfo.ta = addrInfoPtr->sa;
        addrInfo.taType = addrInfoPtr->taType;
        addrInfo.vlanId = addrInfoPtr->vlanId;
        le_utf8_Copy(addrInfo.ifName, addrInfoPtr->ifName, MAX_INTERFACE_NAME_LEN, NULL);
        backend.RespDiagNegative(sid, &addrInfo, TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT);
        return;
    }

    taf_UDSInterface* inf = elem->second;
    if (inf != NULL)
    {
        inf->UDSMsgHandler(addrInfoPtr, sid, diagMsgPtr->dataPtr, diagMsgPtr->dataLen);
    }

    return;
}
#endif

#ifdef LE_CONFIG_DIAG_VSTACK
void taf_DiagBackend::IntIndicationHandler
(
    taf_diagBackend_DiagInfRef_t ref,
    const taf_diagBackend_AddrInfo_t *addrInfoPtr,
    uint8_t serviceID,
    const uint8_t* msgPtr,
    size_t msgSize,
    void* contextPtr
)
{
    taf_DiagBackend& backend = taf_DiagBackend::GetInstance();

    LE_DEBUG("Enter IntegrationIndicationHanler");

    if (addrInfoPtr == NULL || msgPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return;
    }

    if (msgPtr == NULL)
    {
        LE_ERROR("MessagePtr is NULL");
        return;
    }

    auto elem = backend.svcMap.find(serviceID);
    if (elem == backend.svcMap.end())
    {
        LE_ERROR("This service(%d) handler is unregistered", serviceID);
        return;
    }

    backend.refMap.insert(make_pair(serviceID, ref));

    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = addrInfoPtr->sa;
    addrInfo.ta = addrInfoPtr->ta;
    addrInfo.taType = (taf_uds_TaType_t)addrInfoPtr->taType;

    taf_UDSInterface* inf = elem->second;
    if (inf != NULL)
    {
        inf->UDSMsgHandler(&addrInfo, serviceID, const_cast<uint8_t*>(msgPtr), msgSize);
    }
}
#endif

le_result_t taf_DiagBackend::RegisterUdsService
(
    uint8_t sid,
    taf_UDSInterface* service
)
{
    if (service == NULL)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    if (regstFlag == 0)
    {
        // Bring up UDS stack
        if (InitUdsStack() != LE_OK)
        {
            LE_ERROR("UDS stack can not start.");
            return LE_COMM_ERROR;
        }
        regstFlag = 1;
    }

    svcMap.insert(make_pair(sid, service));

    return LE_OK;
}

void taf_DiagBackend::UnregisterUdsService
(
    uint8_t sid
)
{
    svcMap.erase(sid);
#ifdef LE_CONFIG_DIAG_VSTACK
    refMap.erase(sid);
#endif

#ifdef LE_CONFIG_DIAG_VSTACK
    if (svcMap.empty() && refMap.empty())
#else
    if (svcMap.empty())
#endif
    {
        DeInitUdsStack();
    }
}

le_result_t taf_DiagBackend::InitUdsStack
(
)
{
    LE_DEBUG("InitUdsStack");
#ifndef LE_CONFIG_DIAG_VSTACK
    le_result_t ret;

    ret = taf_uds_Start(TAF_STACK_CONFIG_PATH);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to start uds stack.(%d)", ret);
        return ret;
    }

    // Create memory pools.
    VlanAndSesTypeMemPool = le_mem_CreatePool("VlanAndSesTypeMemPool",
            sizeof(taf_RxVlanCurrentSesType_t));

    taf_RxVlanCurrentSesType_t* VlanAndSesTypePtr = NULL;

    le_dls_List_t VlanIdList = LE_DLS_LIST_INIT;
    taf_uds_GetVlanIdList(&VlanIdList);

    // Check enable id already present.
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&VlanIdList);
    while (linkPtr)
    {
        taf_uds_VlanId_t* vlanIdPtr = CONTAINER_OF(linkPtr, taf_uds_VlanId_t,
                link);
        linkPtr = le_dls_PeekNext(&VlanIdList, linkPtr);

        VlanAndSesTypePtr = (taf_RxVlanCurrentSesType_t *)le_mem_ForceAlloc(VlanAndSesTypeMemPool);
        VlanAndSesTypePtr->vlanId = vlanIdPtr->vlanId;
        VlanAndSesTypePtr->currentSesType = 0x01; // Set Default value on starting
        LE_DEBUG("vlanId: %x", VlanAndSesTypePtr->vlanId);
        VlanAndSesTypePtr->link = LE_DLS_LINK_INIT;

        // add this event context to list
        le_dls_Queue(&VlanAndSesTypeList, &VlanAndSesTypePtr->link);
    }

    // Release UDS Vlan List
    ClearUDSVlanList(&VlanIdList);

    // Register the callback for receiving uds message.
    udsIndHandlerRef = taf_uds_AddDiagIndicationHandler(UdsIndicationHanler, NULL);
    if (udsIndHandlerRef == NULL)
    {
        LE_ERROR("Add diag message reception handler failure.");
        return LE_FAULT;
    }
#else
    // Register the callback for receiving uds message from KPIT.
    integrationIndHandlerRef = taf_diagBackend_AddDiagEventHandler(IntIndicationHandler, NULL);
    if (integrationIndHandlerRef == NULL)
    {
        LE_ERROR("Add diag message reception handler failure.");
        return LE_FAULT;
    }
#endif
    return LE_OK;
}

/*
 * Clear UDS vlan list.
*/
void taf_DiagBackend::ClearUDSVlanList
(
    le_dls_List_t* vlanIdListPtr
)
{
    LE_DEBUG("ClearUDSVlanList");

    le_dls_Link_t* linkPtr = NULL;

    TAF_ERROR_IF_RET_NIL(vlanIdListPtr == NULL, "vlanListPtr is null");

    linkPtr = le_dls_Pop(vlanIdListPtr);
    while (linkPtr)
    {
        taf_uds_VlanId_t* vlanIdPtr = CONTAINER_OF(linkPtr, taf_uds_VlanId_t,
                link);

        if (vlanIdPtr != NULL)
        {
             //Release memory in the list
            le_mem_Release(vlanIdPtr);
        }

        // Removes and returns the link at the head of the list.
        linkPtr = le_dls_Pop(vlanIdListPtr);
    }

    return;
}

/*
 * Check VlanId is valid or not
*/
bool taf_DiagBackend::isVlanIdValid
(
    uint16_t vlanId
)
{
    LE_INFO("isVlanIdValid");
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&VlanAndSesTypeList);
    while (linkPtr)
    {
        taf_RxVlanCurrentSesType_t* vlanSesTypePtr = CONTAINER_OF(linkPtr,
                taf_RxVlanCurrentSesType_t, link);
        linkPtr = le_dls_PeekNext(&VlanAndSesTypeList, linkPtr);

        if (vlanSesTypePtr->vlanId == vlanId)
        {
            LE_DEBUG("VlanId is valid %d", vlanId);
            return true;
        }
    }

    return false;
}

/*
 * Get current session type for request vlanId, if VlanId is valid.
*/
le_result_t taf_DiagBackend::GetCurrentSesType
(
    uint16_t vlanId,
    uint8_t* currentSesTypePtr
)
{
    LE_DEBUG("GetCurrentSesType");

    // Check session type for respective VLAN ID
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&VlanAndSesTypeList);
    while (linkPtr)
    {
        taf_RxVlanCurrentSesType_t* vlanSesTypePtr = CONTAINER_OF(linkPtr,
                taf_RxVlanCurrentSesType_t, link);
        linkPtr = le_dls_PeekNext(&VlanAndSesTypeList, linkPtr);

        if (vlanSesTypePtr->vlanId == vlanId)
        {
            LE_DEBUG("Current session type is %d for vlanId %d", vlanSesTypePtr->currentSesType,
                    vlanId);
            *currentSesTypePtr = vlanSesTypePtr->currentSesType;
            return LE_OK;
        }
    }

    LE_ERROR("Vlan ID is not valid: %d", vlanId);
    return LE_NOT_FOUND;
}

void taf_DiagBackend::DeInitUdsStack
(
)
{
#ifndef LE_CONFIG_DIAG_VSTACK
    if (udsIndHandlerRef != NULL)
    {
        taf_uds_RemoveDiagIndicationHandler(udsIndHandlerRef);
        regstFlag = 0;
    }
#else
    if (integrationIndHandlerRef != NULL)
    {
        taf_diagBackend_RemoveDiagEventHandler(integrationIndHandlerRef);
        regstFlag = 0;
    }
#endif
}

le_result_t taf_DiagBackend::RespDiagPositive
(
    uint8_t sid,
    const taf_uds_AddrInfo_t* addrInfoPtr,
    const uint8_t* dataPtr,
    size_t dataLen
)
{
    if (addrInfoPtr == NULL || dataPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }
#ifndef LE_CONFIG_DIAG_VSTACK
    taf_uds_DiagMsg_t msg;
    msg.dataPtr = (uint8_t*)dataPtr;
    msg.dataLen = dataLen;

    return taf_uds_SendDiagResp(addrInfoPtr, &msg, (taf_uds_ServiceId_t)sid, 0);
#else
    taf_diagBackend_AddrInfo_t addrInfo;
    addrInfo.sa = addrInfoPtr->sa;
    addrInfo.ta = addrInfoPtr->ta;
    addrInfo.taType = (taf_diagBackend_TaType_t)addrInfoPtr->taType;

    taf_DiagBackend& backend = taf_DiagBackend::GetInstance();
    auto elem = backend.refMap.find(sid);
    taf_diagBackend_DiagInfRef_t ref = elem->second;

    return taf_diagBackend_SendResp(ref, &addrInfo, sid, 0, dataPtr, dataLen);
#endif
}

le_result_t taf_DiagBackend::RespDiagPositive
(
    uint8_t sid,
    const taf_uds_AddrInfo_t* addrInfoPtr
)
{
    if (addrInfoPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }
#ifndef LE_CONFIG_DIAG_VSTACK
    return taf_uds_SendDiagResp(addrInfoPtr, NULL, (taf_uds_ServiceId_t)sid, 0);
#else
    taf_diagBackend_AddrInfo_t addrInfo;
    addrInfo.sa = addrInfoPtr->sa;
    addrInfo.ta = addrInfoPtr->ta;
    addrInfo.taType = (taf_diagBackend_TaType_t)addrInfoPtr->taType;

    taf_DiagBackend& backend = taf_DiagBackend::GetInstance();
    auto elem = backend.refMap.find(sid);
    taf_diagBackend_DiagInfRef_t ref = elem->second;

    return taf_diagBackend_SendResp(ref, &addrInfo, sid, 0, NULL, 0);
#endif
}

le_result_t taf_DiagBackend::RespDiagNegative
(
    uint8_t sid,
    const taf_uds_AddrInfo_t* addrInfoPtr,
    uint8_t nrc
)
{
    if (addrInfoPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }
#ifndef LE_CONFIG_DIAG_VSTACK
    return taf_uds_SendDiagResp(addrInfoPtr, NULL, (taf_uds_ServiceId_t)sid, nrc);
#else
    taf_diagBackend_AddrInfo_t addrInfo;
    addrInfo.sa = addrInfoPtr->sa;
    addrInfo.ta = addrInfoPtr->ta;
    addrInfo.taType = (taf_diagBackend_TaType_t)addrInfoPtr->taType;

    taf_DiagBackend& backend = taf_DiagBackend::GetInstance();
    auto elem = backend.refMap.find(sid);
    taf_diagBackend_DiagInfRef_t ref = elem->second;

    return taf_diagBackend_SendResp(ref, &addrInfo, sid, nrc, NULL, 0);
#endif
}

void taf_DiagBackend::Init
(
    void
)
{
    if (regstFlag == 0) {
        if (InitUdsStack() == LE_OK)
        {
            regstFlag = 1;
        }
    }

}
