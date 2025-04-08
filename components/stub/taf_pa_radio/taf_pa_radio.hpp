/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAF_PA_RADIO_HPP
#define TAF_PA_RADIO_HPP

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Enable indication
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_EnableIndication
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Disable indication
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_DisableIndication
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Perform network scan with Pysical Cell ID
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_PciScanInformationListRef_t taf_pa_radio_PerformPciNetworkScan
(
    taf_radio_RatBitMask_t ratMask,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the first PCI network scan information reference
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_PciScanInformationRef_t taf_pa_radio_GetFirstPciScanInfo
(
    taf_radio_PciScanInformationListRef_t pciScanInformationListRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the next PCI network scan information reference
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_PciScanInformationRef_t taf_pa_radio_GetNextPciScanInfo
(
    taf_radio_PciScanInformationListRef_t pciScanInformationListRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the first PLMN network information reference
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_PlmnInformationRef_t taf_pa_radio_GetFirstPlmnInfo
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the next PLMN network information reference
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_PlmnInformationRef_t taf_pa_radio_GetNextPlmnInfo
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get PCI network scan Cell ID
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint16_t taf_pa_radio_GetPciScanCellId
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get PCI network scan Global Cell ID
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint32_t taf_pa_radio_GetPciScanGlobalCellId
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get MCC and MNC of PLMN information from PCI network scan
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetPciScanMccMnc
(
    taf_radio_PlmnInformationRef_t plmnRef,
    char* mccPtr,
    size_t mccPtrSize,
    char* mncPtr,
    size_t mncPtrSize
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete PCI network scan
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_DeletePciNetworkScan
(
    taf_radio_PciScanInformationListRef_t scanInformationListRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Set reference.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_radio_SetReference
(
    uint8_t phoneId,
    taf_radio_NetStatusRef_t netStatusRef
);

//--------------------------------------------------------------------------------------------------
/**
 *  Get RAT service status.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetRatSvcStatus
(
    uint8_t phoneId,
    taf_radio_RatSvcStatus_t* status
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for network status.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_NetStatusChangeHandlerRef_t taf_pa_radio_AddNetStatusChangeHandler
(
    taf_radio_NetStatusHandlerFunc_t handlerFuncPtr,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for network status.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_radio_RemoveNetStatusChangeHandler
(
    taf_radio_NetStatusChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
);

//--------------------------------------------------------------------------------------------------
/**
 *  Gets routing area code.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetServingCellRoutingAreaCode
(
    uint8_t* rac,   ///< [OUT] Routing area code.
    uint8_t phoneId ///< [IN] Phone id.
);

#endif /* TAF_PA_RADIO_HPP */
