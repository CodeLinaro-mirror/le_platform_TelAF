/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafIDPSSvc.cpp
 * @brief      This file provides the TAF IDPS service as interfaces described
 *             in taf_diagIDPS.api. The Diag IDPS service will be started automatically.
 */

#include "legato.h"
#include "interfaces.h"
#include "tafIDPSSvr.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference to an IDPS service. If there is no IDPS service for the current session,
 * a new one will be created.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to create the service.
 *
 */
//--------------------------------------------------------------------------------------------------
taf_diagIDPS_ServiceRef_t taf_diagIDPS_GetService
(
    taf_diagIDPS_SecurityLevel_t securityLevel
        ///< [IN] Security level.
)
{
    auto& idpsIns = taf_IdpsSvr::GetInstance();

    return idpsIns.GetService(securityLevel);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets VLAN ID to the service. If the VLAN ID is not found or does not exist, it will return an
 * error.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameter.
 *     - LE_UNSUPPORTED -- VLAN ID is unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagIDPS_SetVlanId
(
    taf_diagIDPS_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint16_t vlanId
        ///< [IN] VLAN ID.
)
{
    auto& idpsIns = taf_IdpsSvr::GetInstance();

    return idpsIns.SetVlanId(svcRef, vlanId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagIDPS_Status'
 *
 * This event provides information on IDPS status change.
 */
//--------------------------------------------------------------------------------------------------
taf_diagIDPS_StatusHandlerRef_t taf_diagIDPS_AddStatusHandler
(
    taf_diagIDPS_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagIDPS_StatusHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    auto& idpsIns = taf_IdpsSvr::GetInstance();

    return idpsIns.AddStatusHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagIDPS_Status'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagIDPS_RemoveStatusHandler
(
    taf_diagIDPS_StatusHandlerRef_t handlerRef
        ///< [IN]
)
{
    auto& idpsIns = taf_IdpsSvr::GetInstance();

    idpsIns.RemoveStatusHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the VLAN ID of the Rx status message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameter.
 *     - LE_NOT_FOUND -- VLAN ID not found.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagIDPS_GetVlanIdFromMsg
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef,
        ///< [IN] IDPS status message reference.
    uint16_t* vlanIdPtr
        ///< [OUT] VLAN ID.
)
{
    auto& idpsIns = taf_IdpsSvr::GetInstance();

    return idpsIns.GetVlanIdFromMsg(statusMsgRef, vlanIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the logical address of the Rx status message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameter.
 *     - LE_NOT_FOUND -- VLAN ID not found.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagIDPS_GetLogicalAddr
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef,
        ///< [IN] IDPS status message reference.
    uint16_t* sourceAddrPtr,
        ///< [OUT] Logical source address.
    uint16_t* targetAddrPtr
        ///< [OUT] Logical target address.
)
{
    auto& idpsIns = taf_IdpsSvr::GetInstance();

    return idpsIns.GetLogicalAddr(statusMsgRef, sourceAddrPtr, targetAddrPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the size of data in the Rx IDPS status message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameter.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagIDPS_GetDataSize
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef,
        ///< [IN] Received message reference.
    uint16_t* sizePtr
        ///< [OUT] The size of data.
)
{
    auto& idpsIns = taf_IdpsSvr::GetInstance();

    return idpsIns.GetDataSize(statusMsgRef, sizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data of the Rx IDPS status message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameter.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagIDPS_GetData
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef,
        ///< [IN] Received message reference.
    uint8_t* dataPtr,
        ///< [OUT] The data.
    size_t* dataSizePtr
        ///< [INOUT]
)
{
    auto& idpsIns = taf_IdpsSvr::GetInstance();

    return idpsIns.GetData(statusMsgRef, dataPtr, dataSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the size of extra data in the Rx IDPS status message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameter.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagIDPS_GetExtraDataSize
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef,
        ///< [IN] Received message reference.
    uint16_t* sizePtr
        ///< [OUT] The size of extra data.
)
{
    auto& idpsIns = taf_IdpsSvr::GetInstance();

    return idpsIns.GetExtraDataSize(statusMsgRef, sizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the extra data of the Rx IDPS status message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameter.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagIDPS_GetExtraData
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef,
        ///< [IN] Received message reference.
    uint8_t* dataPtr,
        ///< [OUT] The extra data.
    size_t* dataSizePtr
        ///< [INOUT]
)
{
    auto& idpsIns = taf_IdpsSvr::GetInstance();

    return idpsIns.GetExtraData(statusMsgRef, dataPtr, dataSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Releases an IDPS status notification message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid statusRef or invalid service of the statusRef.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagIDPS_ReleaseStatusMsg
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef
        ///< [IN] IDPS status message reference.
)
{
    auto& idpsIns = taf_IdpsSvr::GetInstance();

    return idpsIns.ReleaseStatusMsg(statusMsgRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes the IDPS service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagIDPS_RemoveSvc
(
    taf_diagIDPS_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    auto& idpsIns = taf_IdpsSvr::GetInstance();

    return idpsIns.RemoveSvc(svcRef);
}
