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

#include "tafDoIPStack.h"
#include "tafDoIPCommon.hpp"
#include "tafDoIPCommunicationMgr.hpp"
#include "tafDoIPVehicleMgr.hpp"
#include "tafDoIPVehDiscoveryAndParser.hpp"

using namespace taf::doip;

//--------------------------------------------------------------------------------------------------
/**
 * Component once initializer.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT_ONCE
{
    LE_INFO("tafDoip Stack Init...");

    auto &tafCmMgr = CommunicationMgr::GetInstance();
    tafCmMgr.Init();

    LE_INFO("tafDoip Stack Ready...");
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization function
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_DEBUG("DoIP component initializing");
}

//-------------------------------------------------------------------------------------------------
/**
 * Creates a DoIP entity reference with the given configuration.
 *
 * @return
 *  - Reference to the created DoIP entity.
 */
//-------------------------------------------------------------------------------------------------
taf_doip_Ref_t taf_doip_Create
(
    const char* configPathPtr   ///< [IN] DoIP configuration path pointer.
)
{
    uint16_t            logicalSrcAddr;
    taf_doipSession_t*  doipSessionPtr;
    taf_doip_Ref_t      doipSessionRef;
    char eventName[32] = {0};

    auto& vehicleMgr = VehicleManager::GetInstance();
    auto& cmMgr = CommunicationMgr::GetInstance();

    // 1. Initialize vehicle manager with the configure file.
    vehicleMgr.Init(configPathPtr);

    // 2. If this doip is created
    if (vehicleMgr.GetEntityLogicalAddr(&logicalSrcAddr) != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to get entity source address.");
        return NULL;
    }

    doipSessionPtr = cmMgr.FindDoipSession(logicalSrcAddr);
    if (doipSessionPtr != NULL)
    {
        return doipSessionPtr->doipRef;
    }

    // 3. Allocate doip object and initialize its members.
    doipSessionPtr = (taf_doipSession_t*)le_mem_ForceAlloc(cmMgr.doipSessionPool);
    memset(doipSessionPtr, 0, sizeof(taf_doipSession_t));

    doipSessionPtr->logicalSrcAddr = logicalSrcAddr;

    doipSessionRef = (taf_doip_Ref_t)le_ref_CreateRef(cmMgr.doipSessionRefMap, doipSessionPtr);
    doipSessionPtr->doipRef = doipSessionRef;


    doipSessionPtr->diagIndicationHandler.mutexRef = le_mutex_CreateNonRecursive("indication");
    doipSessionPtr->diagConfirmHandler.mutexRef = le_mutex_CreateNonRecursive("confirm");

    snprintf(eventName, sizeof(eventName)-1, "StatusEvent-%x", logicalSrcAddr);
    doipSessionPtr->statusEvtId = le_event_CreateId("DoIPStatusEvent", sizeof(taf_doip_Status_t));

    cmMgr.SessionInit();

    return doipSessionRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get a DoIP entity reference with the source address.
 *
 * @return
 *  - Reference to the DoIP entity.
 */
//-------------------------------------------------------------------------------------------------
taf_doip_Ref_t taf_doip_Get
(
    uint16_t sa     ///< [IN] DoIP entity source address.
)
{
    taf_doipSession_t*  doipSessionPtr;
    auto& cmMgr = CommunicationMgr::GetInstance();

    doipSessionPtr = cmMgr.FindDoipSession(sa);
    if (doipSessionPtr == NULL)
    {
        LE_ERROR("Failed to get entity reference with source address0x%x.", sa);
        return NULL;
    }

    return doipSessionPtr->doipRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Deletes a previously created DoIP entity and free allocated resources.
 *
 * @return
 *  - LE_OK         Function success.
 *  - LE_NOT_FOUND  Invalid reference.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_doip_Delete
(
    taf_doip_Ref_t  doipRef ///< [IN] DoIP entity reference.
)
{
    taf_doipSession_t*  doipSessionPtr;

    auto& cmMgr = CommunicationMgr::GetInstance();

    le_mutex_Lock(cmMgr.doipSessionRefMutex);
    doipSessionPtr = (taf_doipSession_t*)le_ref_Lookup(cmMgr.doipSessionRefMap, doipRef);
    if (doipSessionPtr == NULL)
    {
        le_mutex_Unlock(cmMgr.doipSessionRefMutex);
        LE_ERROR("Invalid reference!!");
        return LE_NOT_FOUND;
    }
    le_mutex_Unlock(cmMgr.doipSessionRefMutex);

    if (doipSessionPtr->diagIndicationHandler.safeRef)
    {
        le_ref_DeleteRef(cmMgr.doipHandlerRefMap,
                &doipSessionPtr->diagIndicationHandler.safeRef);
    }

    if (doipSessionPtr->diagConfirmHandler.safeRef)
    {
        le_ref_DeleteRef(cmMgr.doipHandlerRefMap,
                &doipSessionPtr->diagConfirmHandler.safeRef);
    }

    if (doipSessionPtr->userConfirmHandler.safeRef)
    {
        le_ref_DeleteRef(cmMgr.doipHandlerRefMap,
                &doipSessionPtr->userConfirmHandler.safeRef);
    }

    if (doipSessionPtr->pmQueryhandler.safeRef)
    {
        le_ref_DeleteRef(cmMgr.doipHandlerRefMap,
                &doipSessionPtr->pmQueryhandler.safeRef);
    }

    le_ref_DeleteRef(cmMgr.doipSessionRefMap, doipSessionPtr->doipRef);

    le_mutex_Delete(doipSessionPtr->diagIndicationHandler.mutexRef);
    le_mutex_Delete(doipSessionPtr->diagConfirmHandler.mutexRef);
    le_mem_Release(doipSessionPtr);
    cmMgr.SessionDeInit();

    auto& vehicleMgr = VehicleManager::GetInstance();
    vehicleMgr.DeInit();

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Triggers vehicle announcement.
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_doip_VehicleAnnouncementTrigger
(
    taf_doip_Ref_t  doipRef ///< [IN] DoIP entity reference.
)
{
    taf_doip_Result_t   ret;
    auto& vehicleDisovery = VehicleDiscovery::GetInstance();

    ret = vehicleDisovery.VehicleDiscoveryAnnounce();
    if (ret != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to trigger announcement, return:%d", ret);
        return LE_FAULT;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Starts a DoIP entity.
 *
 * @return
 *  - LE_OK         Function success.
 *  - LE_NOT_FOUND  Invalid reference.
 *  - LE_FAULT      Internal error.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_doip_Start
(
    taf_doip_Ref_t  doipRef ///< [IN] DoIP entity reference.
)
{
    taf_doip_Result_t   ret;
    taf_doipSession_t*  doipSessionPtr;
    auto& cmMgr = CommunicationMgr::GetInstance();

    le_mutex_Lock(cmMgr.doipSessionRefMutex);
    doipSessionPtr = (taf_doipSession_t*)le_ref_Lookup(cmMgr.doipSessionRefMap, doipRef);
    if (doipSessionPtr == NULL)
    {
        le_mutex_Unlock(cmMgr.doipSessionRefMutex);
        LE_ERROR("Invalid reference!!");
        return LE_NOT_FOUND;
    }
    le_mutex_Unlock(cmMgr.doipSessionRefMutex);

    ret = cmMgr.SessionStart();
    if (ret != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to start DoIP entity, return:%d", ret);
        return LE_FAULT;
    }

    if (taf_doip_VehicleAnnouncementTrigger(doipRef) != LE_OK)
    {
        LE_ERROR("Failed to announce vehicle discovery");
        return LE_FAULT;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Stops DoIP a entity.
 *
 * @return
 *  - LE_OK         Function success.
 *  - LE_NOT_FOUND  Invalid reference.
 *  - LE_FAULT      Internal error.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_doip_Stop
(
    taf_doip_Ref_t  doipRef ///< [IN] DoIP entity reference.
)
{
    taf_doip_Result_t   ret;
    taf_doipSession_t*  doipSessionPtr;
    auto& cmMgr = CommunicationMgr::GetInstance();

    le_mutex_Lock(cmMgr.doipSessionRefMutex);
    doipSessionPtr = (taf_doipSession_t*)le_ref_Lookup(cmMgr.doipSessionRefMap, doipRef);
    if (doipSessionPtr == NULL)
    {
        le_mutex_Unlock(cmMgr.doipSessionRefMutex);
        LE_ERROR("Invalid reference!!");
        return LE_NOT_FOUND;
    }
    le_mutex_Unlock(cmMgr.doipSessionRefMutex);

    ret = cmMgr.SessionStop();
    if (ret != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to stop DoIP entity, return:%d", ret);
        return LE_FAULT;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Sets the vehicle vehicle identification number(VIN).
 *
 * @note This function must be called before taf_doip_Start().
 *       VIN is 17 bytes length and shall follow ISO3779.
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 *  - LE_NOT_FOUND      Invalid reference.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_doip_SetVin
(
    const char*     vinPtr      ///< [IN] Vehicle identification number pointer.
)
{
    taf_doip_Result_t   ret;
    auto& vehicleMgr = VehicleManager::GetInstance();

    if (vinPtr == NULL || strlen(vinPtr) != TAF_DOIP_VIN_SIZE)
    {
        LE_ERROR("Invalid Vin");
        return LE_BAD_PARAMETER;
    }

    ret = vehicleMgr.SetVin(vinPtr);
    if (ret != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to set VIN, return:%d", ret);
        return LE_FAULT;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the vehicle vehicle identification number(VIN).
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 *  - LE_NOT_FOUND      Invalid reference.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_doip_GetVin
(
    char*     vinPtr,      ///< [IN] Vehicle identification number pointer.
    size_t    vinSize      ///< [IN] vinPtr buffer size.
)
{
    taf_doip_Result_t   ret;
    auto& vehicleMgr = VehicleManager::GetInstance();

    if (vinPtr == NULL || vinSize <= TAF_DOIP_VIN_SIZE)
    {
        LE_ERROR("Invalid Vin");
        return LE_BAD_PARAMETER;
    }

    ret = vehicleMgr.GetVin(vinPtr);
    if (ret != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to set VIN, return:%d", ret);
        return LE_FAULT;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Sets the vehicle group identification(GID).
 *
 * @note This function must be called before taf_doip_Start().
 *       Recommends to use MAC address as the GID. It can be defined such as 'xx:xx:xx:xx:xx:xx'
 *       or 'xxxxxxxxxxxx', x is in range of '0-F'.
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 *  - LE_NOT_FOUND      Invalid reference.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_doip_SetGid
(
    const char*     gidPtr      ///< [IN] Group identification pointer.
)
{
    taf_doip_Result_t   ret;
    auto& vehicleMgr = VehicleManager::GetInstance();

    if (gidPtr == NULL)
    {
        LE_ERROR("Invalid GID");
        return LE_BAD_PARAMETER;
    }

    ret = vehicleMgr.SetGid(gidPtr);
    if (ret != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to set GID, return:%d", ret);
        return LE_FAULT;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the vehicle group identification(GID).
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 *  - LE_NOT_FOUND      Invalid reference.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_doip_GetGid
(
    char*     gidPtr,      ///< [OUT] Group identification pointer.
    size_t    gidSize      ///< [IN] gidPtr buffer size.
)
{
    taf_doip_Result_t   ret;
    char gidBin[TAF_DOIP_GID_SIZE];

    auto& vehicleMgr = VehicleManager::GetInstance();

    if (gidPtr == NULL || gidSize <= (TAF_DOIP_GID_SIZE*2))
    {
        LE_ERROR("Invalid Vin");
        return LE_BAD_PARAMETER;
    }

    ret = vehicleMgr.GetGid(gidBin);
    if (ret != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to set GID, return:%d", ret);
        return LE_FAULT;
    }

    le_hex_BinaryToString((const uint8_t*)gidBin, TAF_DOIP_GID_SIZE, gidPtr, gidSize);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Sets the vehicle entity identification(EID).
 *
 * @note This function must be called before taf_doip_Start().
 *       Recommends to use MAC address as the EID. It can be defined such as 'xx:xx:xx:xx:xx:xx'
 *       or 'xxxxxxxxxxxx', x is in range of '0-F'.
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 *  - LE_NOT_FOUND      Invalid reference.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_doip_SetEid
(
    const char*     eidPtr      ///< [IN] Entity identification pointer.
)
{
    taf_doip_Result_t   ret;
    auto& vehicleMgr = VehicleManager::GetInstance();

    if (eidPtr == NULL)
    {
        LE_ERROR("Invalid Ein");
        return LE_BAD_PARAMETER;
    }

    ret = vehicleMgr.SetEid((char*)eidPtr);
    if (ret != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to set EID, return:%d", ret);
        return LE_FAULT;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the vehicle entity identification(EID).
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 *  - LE_NOT_FOUND      Invalid reference.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_doip_GetEid
(
    char*     eidPtr,      ///< [IN] Entity identification pointer.
    size_t    eidSize      ///< [IN] eidPtr buffer size.
)
{
    taf_doip_Result_t   ret;
    char eidBin[TAF_DOIP_EID_SIZE];

    auto& vehicleMgr = VehicleManager::GetInstance();

    if (eidPtr == NULL || eidSize <= (TAF_DOIP_EID_SIZE*2))
    {
        LE_ERROR("Invalid Vin");
        return LE_BAD_PARAMETER;
    }

    ret = vehicleMgr.GetEid(eidBin);
    if (ret != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to set GID, return:%d", ret);
        return LE_FAULT;
    }

    le_hex_BinaryToString((const uint8_t*)eidBin, TAF_DOIP_EID_SIZE, eidPtr, eidSize);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Sets the DoIP entity source address.
 *
 * @note This function must be called before taf_doip_Start().
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 *  - LE_NOT_FOUND      Invalid reference.
 *  - LE_FAULT          Internal error.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_doip_SetSourceAddr
(
    taf_doip_Ref_t  doipRef,    ///< [IN] DoIP entity reference.
    uint16_t        sa          ///< [IN] Source address.
)
{
    taf_doip_Result_t   ret;
    auto& vehicleMgr = VehicleManager::GetInstance();

    ret = vehicleMgr.SetEntityLogicalAddr(sa);
    if (ret != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to set source address, return:%d", ret);
        return LE_FAULT;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the supported interfaces of DoIP entity.
 *
 * @return
 *  - Interface list    success.
 *  - NULL              FAILURE.
 */
//-------------------------------------------------------------------------------------------------
le_dls_List_t *taf_doip_GetIfaces
(
    taf_doip_Ref_t  doipRef     ///< [IN] DoIP entity reference.
)
{
    auto& vehicleMgr = VehicleManager::GetInstance();

    return vehicleMgr.GetIfaceList();
}

//-------------------------------------------------------------------------------------------------
/**
 * Adds a handler to query power mode status.
 *
 * @note If call this function to add handler more than one times with the same reference,
 *       it will override previous handler. Only the last one takes effect.
 *
 * @return
 *  - TAF_DOIP_RESULT_OK            Function success.
 *  - TAF_DOIP_RESULT_PARAM_ERROR   Invalid parameter.
 *  - TAF_DOIP_RESULT_ERROR         Internal error.
 */
//-------------------------------------------------------------------------------------------------
taf_doip_PowerModeQueryHandlerRef_t taf_doip_AddPowerModeQueryHandler
(
    taf_doip_Ref_t                       doipRef,            ///< [IN] DoIP entity reference.
    taf_doip_PowerModeQueryHandlerFunc_t pmQueryHandlerPtr,  ///< [IN] Hander function.
    void*                                userPtr             ///< [IN] User-defined pointer.
)
{
    void*               handlerRef;
    taf_doipSession_t*  doipSessionPtr;

    auto& cmMgr = CommunicationMgr::GetInstance();

    le_mutex_Lock(cmMgr.doipSessionRefMutex);
    doipSessionPtr = (taf_doipSession_t*)le_ref_Lookup(cmMgr.doipSessionRefMap, doipRef);
    if (doipSessionPtr == NULL)
    {
        le_mutex_Unlock(cmMgr.doipSessionRefMutex);
        LE_ERROR("Invalid reference!!");
        return NULL;
    }
    le_mutex_Unlock(cmMgr.doipSessionRefMutex);

    // Remove previous handler reference
    if (le_ref_Lookup(cmMgr.doipHandlerRefMap, doipSessionPtr->pmQueryhandler.safeRef))
    {
        le_ref_DeleteRef(cmMgr.doipHandlerRefMap, doipSessionPtr->pmQueryhandler.safeRef);
    }

    handlerRef = le_ref_CreateRef(cmMgr.doipHandlerRefMap,
            &doipSessionPtr->pmQueryhandler);
    if (handlerRef == NULL)
    {
        LE_ERROR("Failed to create power query reference!!");
        return NULL;
    }

    doipSessionPtr->pmQueryhandler.funcPtr  = pmQueryHandlerPtr;
    doipSessionPtr->pmQueryhandler.ctxPtr   = userPtr;
    doipSessionPtr->pmQueryhandler.safeRef  = handlerRef;

    return (taf_doip_PowerModeQueryHandlerRef_t)handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Removes the query power mode status handler.
 *
 */
//-------------------------------------------------------------------------------------------------
void taf_doip_RemovePowerModeQueryHandler
(
    taf_doip_PowerModeQueryHandlerRef_t handerRef    ///< [IN] The handler reference.
)
{
    taf_doipPmQueryHandler_t*   handlerPtr = NULL;

    auto& cmMgr = CommunicationMgr::GetInstance();

    handlerPtr = (taf_doipPmQueryHandler_t*)le_ref_Lookup(cmMgr.doipHandlerRefMap, handerRef);
    if (handlerPtr == NULL)
    {
        LE_ERROR("Invalid reference!!");
        return;
    }

    le_ref_DeleteRef(cmMgr.doipHandlerRefMap, handlerPtr->safeRef);
    handlerPtr->safeRef = NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * Adds a handler to indicate diagnostic message or connection registered event
 *
 * @note If call this function to add handler more than one times with the same reference,
 *       it will override previous handler. Only the last one takes effect.
 *
 * @return
 *  - A handler reference   success.
 *  - NULL                  FAILURE.
 */
//-------------------------------------------------------------------------------------------------
taf_doip_DiagIndicationHandlerRef_t taf_doip_AddDiagIndicationHandler
(
    taf_doip_Ref_t                       doipRef,                ///< [IN] DoIP entity reference.
    taf_doip_DiagIndicationHandlerFunc_t indicationHandlerPtr,   ///< [IN] Hander function.
    void*                                userPtr                 ///< [IN] User-defined pointer.
)
{
    void*               handlerRef;
    taf_doipSession_t*  doipSessionPtr;

    auto& cmMgr = CommunicationMgr::GetInstance();

    le_mutex_Lock(cmMgr.doipSessionRefMutex);
    doipSessionPtr = (taf_doipSession_t*)le_ref_Lookup(cmMgr.doipSessionRefMap, doipRef);
    if (doipSessionPtr == NULL)
    {
        le_mutex_Unlock(cmMgr.doipSessionRefMutex);
        LE_ERROR("Invalid reference!!");
        return NULL;
    }
    le_mutex_Unlock(cmMgr.doipSessionRefMutex);

    le_mutex_Lock(doipSessionPtr->diagIndicationHandler.mutexRef);

    // Remove previous handler reference
    if (le_ref_Lookup(cmMgr.doipHandlerRefMap, doipSessionPtr->diagIndicationHandler.safeRef))
    {
        le_ref_DeleteRef(cmMgr.doipHandlerRefMap, doipSessionPtr->diagIndicationHandler.safeRef);
    }

    handlerRef = le_ref_CreateRef(cmMgr.doipHandlerRefMap,
            &doipSessionPtr->diagIndicationHandler);
    if (handlerRef == NULL)
    {
        LE_ERROR("Failed to create diag indication reference!!");
        le_mutex_Unlock(doipSessionPtr->diagIndicationHandler.mutexRef);
        return NULL;
    }

    doipSessionPtr->diagIndicationHandler.funcPtr  = indicationHandlerPtr;
    doipSessionPtr->diagIndicationHandler.ctxPtr   = userPtr;
    doipSessionPtr->diagIndicationHandler.safeRef  = handlerRef;

    le_mutex_Unlock(doipSessionPtr->diagIndicationHandler.mutexRef);

    return (taf_doip_DiagIndicationHandlerRef_t)handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Removes the diagnostic indication handler.
 *
 */
//-------------------------------------------------------------------------------------------------
void taf_doip_RemoveDiagIndicationHandler
(
    taf_doip_DiagIndicationHandlerRef_t handerRef   ///< [IN] The handler reference.
)
{
    taf_doipIndicationHandler_t*   handlerPtr = NULL;

    auto& cmMgr = CommunicationMgr::GetInstance();


    handlerPtr = (taf_doipIndicationHandler_t*)le_ref_Lookup(cmMgr.doipHandlerRefMap, handerRef);
    if (handlerPtr == NULL)
    {
        LE_ERROR("Invalid reference!!");
        return;
    }

    le_mutex_Lock(handlerPtr->mutexRef);
    le_ref_DeleteRef(cmMgr.doipHandlerRefMap, handlerPtr->safeRef);
    handlerPtr->safeRef = NULL;
    le_mutex_Unlock(handlerPtr->mutexRef);
}

//------------------------------------------------------------------------------------------------
/**
 * Adds a handler to confirm diagnostic message request result.
 *
 * @note If call this function to add handler more than one times with the same reference,
 *       it will override previous handler. Only the last one takes effect.
 *
 * @return
 *  - A handler reference   success.
 *  - NULL                  FAILURE.
 */
//-------------------------------------------------------------------------------------------------
taf_doip_DiagConfirmHandlerRef_t taf_doip_AddDiagConfirmHandler
(
    taf_doip_Ref_t                    doipRef,              ///< [IN] DoIP entity reference.
    taf_doip_DiagConfirmHandlerFunc_t confirmHandlerPtr,    ///< [IN] Hander function.
    void*                             userPtr               ///< [IN] User-defined pointer.
)
{
    void*               handlerRef;
    taf_doipSession_t*  doipSessionPtr;

    auto& cmMgr = CommunicationMgr::GetInstance();

    le_mutex_Lock(cmMgr.doipSessionRefMutex);
    doipSessionPtr = (taf_doipSession_t*)le_ref_Lookup(cmMgr.doipSessionRefMap, doipRef);
    if (doipSessionPtr == NULL)
    {
        le_mutex_Unlock(cmMgr.doipSessionRefMutex);
        LE_ERROR("Invalid reference!!");
        return NULL;
    }
    le_mutex_Unlock(cmMgr.doipSessionRefMutex);

    le_mutex_Lock(doipSessionPtr->diagConfirmHandler.mutexRef);

    // Remove previous handler reference
    if (le_ref_Lookup(cmMgr.doipHandlerRefMap, doipSessionPtr->diagConfirmHandler.safeRef))
    {
        le_ref_DeleteRef(cmMgr.doipHandlerRefMap, doipSessionPtr->diagConfirmHandler.safeRef);
    }

    handlerRef = le_ref_CreateRef(cmMgr.doipHandlerRefMap,
            &doipSessionPtr->diagConfirmHandler);
    if (handlerRef == NULL)
    {
        LE_ERROR("Failed to create diag confirmation reference!!");
        le_mutex_Unlock(doipSessionPtr->diagConfirmHandler.mutexRef);
        return NULL;
    }

    doipSessionPtr->diagConfirmHandler.funcPtr  = confirmHandlerPtr;
    doipSessionPtr->diagConfirmHandler.ctxPtr   = userPtr;
    doipSessionPtr->diagConfirmHandler.safeRef  = handlerRef;

    le_mutex_Unlock(doipSessionPtr->diagConfirmHandler.mutexRef);

    return (taf_doip_DiagConfirmHandlerRef_t)handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Removes the diagnostic confirmation handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_doip_RemoveDiagConfirmHandler
(
    taf_doip_DiagConfirmHandlerRef_t handerRef  ///< [IN] The handler reference.
)
{
    taf_doipDiagConfirmHandler_t*   handlerPtr = NULL;

    auto& cmMgr = CommunicationMgr::GetInstance();


    handlerPtr = (taf_doipDiagConfirmHandler_t*)le_ref_Lookup(cmMgr.doipHandlerRefMap, handerRef);
    if (handlerPtr == NULL)
    {
        LE_ERROR("Invalid reference!!");
        return;
    }

    le_mutex_Lock(handlerPtr->mutexRef);
    le_ref_DeleteRef(cmMgr.doipHandlerRefMap, handlerPtr->safeRef);
    handlerPtr->safeRef = NULL;
    le_mutex_Unlock(handlerPtr->mutexRef);
}

//-------------------------------------------------------------------------------------------------
/**
 * Adds a handler to confirm routing activation.
 *
 * @return
 *  - A handler reference   success.
 *  - NULL                  FAILURE.
 */
//-------------------------------------------------------------------------------------------------
taf_doip_UserConfirmHandlerRef_t taf_doip_AddUserConfirmHandler
(
    taf_doip_Ref_t                      doipRef,            ///< [IN] DoIP entity reference.
    taf_doip_UserConfirmHandlerFunc_t   confirmHandlerPtr,  ///< [IN] Hander function.
    void*                               userPtr             ///< [IN] User-defined pointer.
)
{
    void*               handlerRef;
    taf_doipSession_t*  doipSessionPtr;

    auto& cmMgr = CommunicationMgr::GetInstance();

    le_mutex_Lock(cmMgr.doipSessionRefMutex);
    doipSessionPtr = (taf_doipSession_t*)le_ref_Lookup(cmMgr.doipSessionRefMap, doipRef);
    if (doipSessionPtr == NULL)
    {
        le_mutex_Unlock(cmMgr.doipSessionRefMutex);
        LE_ERROR("Invalid reference!!");
        return NULL;
    }
    le_mutex_Unlock(cmMgr.doipSessionRefMutex);

    // Remove previous handler reference
    if (le_ref_Lookup(cmMgr.doipHandlerRefMap, doipSessionPtr->userConfirmHandler.safeRef))
    {
        le_ref_DeleteRef(cmMgr.doipHandlerRefMap, doipSessionPtr->userConfirmHandler.safeRef);
    }

    handlerRef = le_ref_CreateRef(cmMgr.doipHandlerRefMap,
            &doipSessionPtr->userConfirmHandler);
    if (handlerRef == NULL)
    {
        LE_ERROR("Failed to create power query reference!!");
        return NULL;
    }

    doipSessionPtr->userConfirmHandler.funcPtr  = confirmHandlerPtr;
    doipSessionPtr->userConfirmHandler.ctxPtr   = userPtr;
    doipSessionPtr->userConfirmHandler.safeRef  = handlerRef;

    return (taf_doip_UserConfirmHandlerRef_t)handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Removes the routing activation confirmation handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_doip_RemoveUserConfirmHandler
(
    taf_doip_UserConfirmHandlerRef_t    handerRef ///< [IN] Hander function.
)
{
    taf_doipUserConfirmHandler_t*   handlerPtr = NULL;

    auto& cmMgr = CommunicationMgr::GetInstance();

    handlerPtr = (taf_doipUserConfirmHandler_t*)le_ref_Lookup(cmMgr.doipHandlerRefMap, handerRef);
    if (handlerPtr == NULL)
    {
        LE_ERROR("Invalid reference!!");
        return;
    }

    le_ref_DeleteRef(cmMgr.doipHandlerRefMap, handlerPtr->safeRef);
    handlerPtr->safeRef = NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * Sends a diagnostic message.
 *
 * @note Shall be called after taf_doip_Start(). If DoIP entity is not started,
 *       it will fail.
 *
 * @return
 *  - LE_OK             Function success.
 *  - LE_BAD_PARAMETER  Invalid parameter
 *  - LE_COMM_ERROR     Sending message error.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_doip_DiagRequest
(
    const taf_doip_AddrInfo_t*  addrInfoPtr,    ///< [IN] Logical address information pointer.
    const taf_doip_DiagMsg_t*   diagMsgPtr      ///< [IN] Diagnostic message pointer.
)
{
    taf_doip_Result_t   ret;
    auto& cmMgr = CommunicationMgr::GetInstance();

    if (addrInfoPtr == NULL || addrInfoPtr == NULL)
    {
        LE_ERROR("Invalid parameter: %p, %p", addrInfoPtr, diagMsgPtr);
        return LE_BAD_PARAMETER;
    }

    ret = cmMgr.TransUdsMessage(addrInfoPtr->sa, addrInfoPtr->ta, addrInfoPtr->taType,
        (char*)diagMsgPtr->dataPtr, diagMsgPtr->dataLen);
    if (ret != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to request diag message, return code:%d", ret);
        return LE_COMM_ERROR;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get DoIP component status event.
 *
 * @return
 *  - event id.
 */
//-------------------------------------------------------------------------------------------------
le_event_Id_t taf_doip_GetStatusEvent
(
    uint16_t sa
)
{
    taf_doipSession_t*  doipSessionPtr;
    auto& cmMgr = CommunicationMgr::GetInstance();
    doipSessionPtr = cmMgr.FindDoipSession(sa);
    if (doipSessionPtr == NULL)
    {
        LE_ERROR("Failed to get doip session");
        return (le_event_Id_t)NULL;
    }

    return doipSessionPtr->statusEvtId;
}

static void FirstDoIPEventHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    taf_doip_Status_t *statusPtr = (taf_doip_Status_t*)reportPtr;
    if (statusPtr == NULL || secondLayerHandlerFunc == NULL)
    {
        LE_ERROR("Report message error");
        return;
    }

    taf_doip_EventHandlerFunc_t handlerPtr =
        (taf_doip_EventHandlerFunc_t)secondLayerHandlerFunc;

    taf_doipSession_t*  doipSessionPtr;
    auto& cmMgr = CommunicationMgr::GetInstance();

    doipSessionPtr = cmMgr.FindDoipSession(statusPtr->entityAddr);
    if (doipSessionPtr == NULL)
    {
        LE_ERROR("Failed to get entity reference with source address0x%x.",
            statusPtr->entityAddr);
        return;
    }

    taf_doip_Event_t event;
    if (statusPtr->eventStatus == TAF_DOIP_RESULT_SA_REGISTERED)
    {
        event = TAF_DOIP_EVENT_CONNECTION;
    }
    else if (statusPtr->eventStatus == TAF_DOIP_RESULT_SA_DEREGISTERED)
    {
        event = TAF_DOIP_EVENT_DISCONNECTION;
    }
    else
    {
        // No need to report other events.
        return;
    }

    handlerPtr(doipSessionPtr->doipRef, event, statusPtr->clientAddr,
            statusPtr->vlanId, le_event_GetContextPtr());
}

//-------------------------------------------------------------------------------------------------
/**
 * Adds a handler to notify DoIP event.
 *
 * @return
 *  - A handler reference   success.
 *  - NULL                  FAILURE.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED taf_doip_EventHandlerRef_t taf_doip_AddEventHandler
(
    taf_doip_Ref_t                    doipRef,              ///< [IN] DoIP entity reference.
    taf_doip_EventHandlerFunc_t       handlerPtr,           ///< [IN] Hander function.
    void*                             userPtr               ///< [IN] User-defined pointer.
)
{
    taf_doipSession_t*  doipSessionPtr;
    auto& cmMgr = CommunicationMgr::GetInstance();

    le_mutex_Lock(cmMgr.doipSessionRefMutex);
    doipSessionPtr = (taf_doipSession_t*)le_ref_Lookup(cmMgr.doipSessionRefMap, doipRef);
    if (doipSessionPtr == NULL)
    {
        le_mutex_Unlock(cmMgr.doipSessionRefMutex);
        LE_ERROR("Invalid reference!!");
        return NULL;
    }
    le_mutex_Unlock(cmMgr.doipSessionRefMutex);

    le_event_Id_t eventId = doipSessionPtr->statusEvtId;

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("DoIPEventHandler",
        eventId, FirstDoIPEventHandler, (void*)handlerPtr);
    le_event_SetContextPtr(handlerRef, userPtr);

    LE_INFO("Registered event handler to DoIP stack");

    return (taf_doip_EventHandlerRef_t)handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Removes the DoIP event handler.
 *
 * @return
 *  - A handler reference   success.
 *  - NULL                  FAILURE.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED void taf_doip_RemoveEventHandler
(
    taf_doip_EventHandlerRef_t eventHandlerRef  ///< [IN] DoIP event handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)eventHandlerRef);
}
