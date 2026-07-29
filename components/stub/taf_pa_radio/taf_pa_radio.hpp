/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAF_PA_RADIO_HPP
#define TAF_PA_RADIO_HPP

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * LTE cphy aggregated indication
 */
//--------------------------------------------------------------------------------------------------
#define TAF_PA_RADIO_LTE_CA_IND_BIT_MASK_SCELL_INFO 0x1
#define TAF_PA_RADIO_LTE_CA_IND_BIT_MASK_PCELL_INFO 0x2
#define TAF_PA_RADIO_MAX_RAT_SRV_STATUS_COUNT 5
typedef uint64_t taf_pa_radio_LteCaIndBitMask_t;

//--------------------------------------------------------------------------------------------------
/**
 * Bitmask to control system info indication reporting behavior.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_RADIO_SYS_INFO_IND_LIMIT_NONE = 0,
    TAF_PA_RADIO_SYS_INFO_IND_LIMIT_BY_STATE_TOGGLE = (1 << 0),
    TAF_PA_RADIO_SYS_INFO_IND_LIMIT_BY_SRV_STATUS = (1 << 1)
} taf_pa_radio_SysInfoIndLimitMask_t;

//--------------------------------------------------------------------------------------------------
/**
 * Indication modes to disable.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_RADIO_DISABLE_IND_MODE_ALL,
    TAF_PA_RADIO_DISABLE_IND_MODE_SKIP_NAS_SYS_INFO_IND
} taf_pa_radio_DisableIndicationMode_t;

//--------------------------------------------------------------------------------------------------
/**
 * RF bandwidth type.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_RADIO_RF_BANDWIDTH_INVALID,
    TAF_PA_RADIO_RF_BANDWIDTH_LTE_BW_1_4,
    TAF_PA_RADIO_RF_BANDWIDTH_LTE_BW_3,
    TAF_PA_RADIO_RF_BANDWIDTH_LTE_BW_5,
    TAF_PA_RADIO_RF_BANDWIDTH_LTE_BW_10,
    TAF_PA_RADIO_RF_BANDWIDTH_LTE_BW_15,
    TAF_PA_RADIO_RF_BANDWIDTH_LTE_BW_20
} taf_pa_radio_RFBandWidth_t;

//--------------------------------------------------------------------------------------------------
/**
 * Scell state.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_RADIO_SCELL_STATE_UNKNOWN,
    TAF_PA_RADIO_SCELL_STATE_DECONFIGURED,
    TAF_PA_RADIO_SCELL_STATE_CONFIGURED_DEACTIVATED,
    TAF_PA_RADIO_SCELL_STATE_CONFIGURED_ACTIVATED
} taf_pa_radio_ScellState_t;

//--------------------------------------------------------------------------------------------------
/**
 * ENDC status.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_RADIO_ENDC_STATUS_UNKNOWN,
    TAF_PA_RADIO_ENDC_STATUS_AVAILABLE,
    TAF_PA_RADIO_ENDC_STATUS_UNAVAILABLE
} taf_pa_radio_EndcStatus_t;


//--------------------------------------------------------------------------------------------------
/**
 * RAT type.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_RADIO_RAT_UNKNOWN,
    TAF_PA_RADIO_RAT_GSM,
    TAF_PA_RADIO_RAT_WCDMA,
    TAF_PA_RADIO_RAT_LTE,
    TAF_PA_RADIO_RAT_NR5G,
} taf_pa_radio_Rat_t;

//--------------------------------------------------------------------------------------------------
/**
 * Service status for the current radio technology.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PA_RADIO_SRV_STATUS_UNKNOWN,
    TAF_PA_RADIO_SRV_STATUS_NO_SRV,
    TAF_PA_RADIO_SRV_STATUS_LIMITED,
    TAF_PA_RADIO_SRV_STATUS_SRV,
    TAF_PA_RADIO_SRV_STATUS_LIMITED_REGIONAL,
    TAF_PA_RADIO_SRV_STATUS_PWR_SAVE,
} taf_pa_radio_ServiceStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * LTE physical carrier aggregation information reference.
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_pa_radio_LteCphyCaInfoRef* taf_pa_radio_LteCphyCaInfoRef_t;

typedef void (*taf_pa_radio_LteCaHdlrFunc_t)
(
    uint8_t phone,
    taf_pa_radio_LteCaIndBitMask_t bitmask,
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    void* contextPtr
);

typedef void (*taf_pa_radio_EndcStatusHdlrFunc_t)
(
    uint8_t phone,
    taf_pa_radio_EndcStatus_t status,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Service status handler function type.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*taf_pa_radio_ServiceStatusChangeHandlerFunc_t)
(
    uint8_t phoneId,
    taf_pa_radio_Rat_t rat,
    taf_pa_radio_ServiceStatus_t serviceStatus,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 *  Reference for service status change handler.
 */
//--------------------------------------------------------------------------------------------------
typedef void* taf_pa_radio_ServiceStatusChangeHandlerRef_t;
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
    taf_pa_radio_DisableIndicationMode_t mode
);

//--------------------------------------------------------------------------------------------------
/**
 * Set system info indication filtering.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_SetSysInfoIndLimit
(
    uint8_t phoneId,
    taf_pa_radio_SysInfoIndLimitMask_t limitMask
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
 *  Get RAT service status and serving rat.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t  taf_pa_radio_GetServiceStatus
(
    uint8_t phoneId,
    taf_pa_radio_Rat_t* servingRat,
    taf_pa_radio_ServiceStatus_t* status
);

//--------------------------------------------------------------------------------------------------
/**
 *  Get limit set for sys info indication.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetSysInfoIndLimit
(
    uint8_t phoneId,
    taf_pa_radio_SysInfoIndLimitMask_t* limitMask
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
 * Add handler for service status change.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_pa_radio_ServiceStatusChangeHandlerRef_t taf_pa_radio_AddServiceStatusChangeHandler
(
    taf_pa_radio_ServiceStatusChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for service status change.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_radio_RemoveServiceStatusChangeHandler
(
    taf_pa_radio_ServiceStatusChangeHandlerRef_t handlerRef
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

//--------------------------------------------------------------------------------------------------
/**
 * Get LTE physical carrier aggregation information.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetLteCphyCaInformation
(
    uint8_t phoneId,
    taf_pa_radio_LteCphyCaInfoRef_t* infoRefPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the valid status of pcell information.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_IsPcellInfoValid
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    bool* isValid
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the physical cell ID of pcell.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetPcellPci
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t* pciPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the absolute radio frequency channel number of pcell.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetPcellFreq
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t* freqPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the downlink bandwidth of pcell.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetPcellDownlinkBandwidth
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    taf_pa_radio_RFBandWidth_t* bandwidthPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the active band of pcell.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetPcellActiveBand
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t* bandPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the count of scell.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetScellCount
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t* countPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the physical cell ID of scell.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetScellPci
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t index,
    uint32_t* pciPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the absolute radio frequency channel number of scell.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetScellFreq
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t index,
    uint32_t* freqPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the downlink bandwidth of scell.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetScellDownlinkBandwidth
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t index,
    taf_pa_radio_RFBandWidth_t* bandwidthPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the active band of scell.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetScellActiveBand
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t index,
    uint32_t* bandPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the state of scell.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetScellState
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t index,
    taf_pa_radio_ScellState_t* statePtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets the state of scell.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetScellIndex
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t index,
    uint32_t* indexPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets if the carrier aggregation is uplink configured.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetScellUplinkConfigured
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    uint32_t index,
    bool* isCongfigured
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete LTE physical carrier aggregation information.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_DeleteLteCphyCaInformation
(
    taf_pa_radio_LteCphyCaInfoRef_t infoRef
);

//--------------------------------------------------------------------------------------------------
/**
 *  Gets ENDC status.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetEndcStatus
(
    uint8_t phoneId,                     ///< [IN] Phone id.
    taf_pa_radio_EndcStatus_t* statusPtr ///< [OUT] ENDC status.

);

//--------------------------------------------------------------------------------------------------
/**
 *  Sets Lte CA information handler.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_radio_SetLteCaHandler
(
    taf_pa_radio_LteCaHdlrFunc_t handlerFuncPtr,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 *  Sets ENDC status handler.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_radio_SetEndcStatusHandler
(
    taf_pa_radio_EndcStatusHdlrFunc_t handlerFuncPtr,
    void* contextPtr
);

#endif /* TAF_PA_RADIO_HPP */
