/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"

#include "taf_pa_radio.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Enable indication
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_EnableIndication
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable indication
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_DisableIndication
(
    taf_pa_radio_DisableIndicationMode_t mode
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set service state toggle indication
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_SetSysInfoIndLimit
(
    uint8_t phoneId,
    taf_pa_radio_SysInfoIndLimitMask_t limitType
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Perform network scan with Pysical Cell ID
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationListRef_t taf_pa_radio_PerformPciNetworkScan
(
    taf_radio_RatBitMask_t ratMask,
    uint8_t phoneId
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the first PCI network scan information reference
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationRef_t taf_pa_radio_GetFirstPciScanInfo
(
    taf_radio_PciScanInformationListRef_t pciScanInformationListRef
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next PCI network scan information reference
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationRef_t taf_pa_radio_GetNextPciScanInfo
(
    taf_radio_PciScanInformationListRef_t pciScanInformationListRef
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the first PLMN network information reference
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PlmnInformationRef_t taf_pa_radio_GetFirstPlmnInfo
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next PLMN network information reference
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PlmnInformationRef_t taf_pa_radio_GetNextPlmnInfo
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get PCI network scan Cell ID
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_pa_radio_GetPciScanCellId
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
)
{
    return UINT16_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get PCI network scan Global Cell ID
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_pa_radio_GetPciScanGlobalCellId
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
)
{
    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get MCC and MNC of PLMN information from PCI network scan
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetPciScanMccMnc
(
    taf_radio_PlmnInformationRef_t plmnRef,
    char* mccPtr,
    size_t mccPtrSize,
    char* mncPtr,
    size_t mncPtrSize
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete PCI network scan
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_DeletePciNetworkScan
(
    taf_radio_PciScanInformationListRef_t pciScanInformationListRef
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set reference.
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_radio_SetReference
(
    uint8_t phoneId,
    taf_radio_NetStatusRef_t netStatusRef
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for network status.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NetStatusChangeHandlerRef_t taf_pa_radio_AddNetStatusChangeHandler
(
    taf_radio_NetStatusHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for network status.
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_radio_RemoveNetStatusChangeHandler
(
    taf_radio_NetStatusChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for Service status change.
 */
//--------------------------------------------------------------------------------------------------
taf_pa_radio_ServiceStatusChangeHandlerRef_t taf_pa_radio_AddServiceStatusChangeHandler
(
    taf_pa_radio_ServiceStatusChangeHandlerFunc_t  handlerFuncPtr,
    void* contextPtr
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for Service status change.
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_radio_RemoveServiceStatusChangeHandler
(
    taf_pa_radio_ServiceStatusChangeHandlerRef_t handlerRef
)
{
    return;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Get RAT service status.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetRatSvcStatus
(
    uint8_t phoneId,
    taf_radio_RatSvcStatus_t* status
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Get RAT service status and serving rat.
 */
//--------------------------------------------------------------------------------------------------
le_result_t  taf_pa_radio_GetServiceStatus
(
    uint8_t phoneId,
    taf_pa_radio_Rat_t* servingRat,
    taf_pa_radio_ServiceStatus_t* status
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Get limit set for sys info indication.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetSysInfoIndLimit
(
    uint8_t phoneId,
    taf_pa_radio_SysInfoIndLimitMask_t* limitMask
)
{
    return LE_OK;
}

le_result_t taf_pa_radio_GetServingCellRoutingAreaCode
(
    uint8_t* rac,   ///< [OUT] Routing area code.
    uint8_t phoneId ///< [IN] Phone id.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get LTE physical carrier aggregation information.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetLteCphyCaInformation
(
    uint8_t phoneId,
    taf_pa_radio_LteCphyCaInfoRef_t* infoRefPtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the valid status of pcell information.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_IsPcellInfoValid
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    bool* isValid
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the physical cell ID of pcell.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetPcellPci
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t* pciPtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the absolute radio frequency channel number of pcell.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetPcellFreq
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t* freqPtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the downlink bandwidth of pcell.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetPcellDownlinkBandwidth
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    taf_pa_radio_RFBandWidth_t* bandwidthPtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the active band of pcell.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetPcellActiveBand
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t* bandPtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the count of scell.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetScellCount
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t* countPtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the physical cell ID of scell.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetScellPci
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t index,
    uint32_t* pciPtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the absolute radio frequency channel number of scell.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetScellFreq
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t index,
    uint32_t* freqPtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the downlink bandwidth of scell.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetScellDownlinkBandwidth
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t index,
    taf_pa_radio_RFBandWidth_t* bandwidthPtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the active band of scell.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetScellActiveBand
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t index,
    uint32_t* bandPtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the state of scell.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetScellState
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t index,
    taf_pa_radio_ScellState_t* statePtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the state of scell.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetScellIndex
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t index,
    uint32_t* indexPtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets if the carrier aggregation is uplink configured.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetScellUplinkConfigured
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t index,
    bool* isCongfigured
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete LTE physical carrier aggregation information.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_DeleteLteCphyCaInformation
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets ENDC status.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetEndcStatus
(
    uint8_t phoneId,                     ///< [IN] Phone id.
    taf_pa_radio_EndcStatus_t* statusPtr ///< [OUT] ENDC status.

)
{
    return LE_OK;
}

void taf_pa_radio_SetLteCaHandler
(
    taf_pa_radio_LteCaHdlrFunc_t handlerFuncPtr,
    void* contextPtr
)
{
}

void taf_pa_radio_SetEndcStatusHandler
(
    taf_pa_radio_EndcStatusHdlrFunc_t handlerFuncPtr,
    void* contextPtr
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Init this component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("taf_pa_radio stub");
}
