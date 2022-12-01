/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFRADIO_HPP
#define TAFRADIO_HPP

#include "legato.h"
#include "interfaces.h"

#include <string>
#include <mutex>
#include <map>

#include "taf_pa_radio.hpp"

#define INSTANCE_MAX_COUNT 2
#define DISABLE_INDICATION 0
#define ENABLE_INDICATION 1
#define BITMASK_RAT_LTE 0x1
#define BITMASK_RAT_5G_NSA 0x2
#define PLMN_SCAN_TIMEOUT 210

#define COMMON_LIST_TYPE_NUM 4
#define COMMON_LIST_MAX_COUNT (INSTANCE_MAX_COUNT * COMMON_LIST_TYPE_NUM)
#define COMMON_RERERENCE_TYPE_NUM 2
#define COMMON_RERERENCE_MAX_COUNT (INSTANCE_MAX_COUNT * COMMON_RERERENCE_TYPE_NUM)
#define CA_INFO_MAX_COUNT (INSTANCE_MAX_COUNT * 2)
#define CONN_STATUS_MAX_COUNT (INSTANCE_MAX_COUNT * 2)
#define PCI_CELL_MAX_COUNT (INSTANCE_MAX_COUNT * TAF_PA_RADIO_PCI_SCAN_CELL_MAX_COUNT)
#define PLMN_ID_MAX_COUNT (PCI_CELL_MAX_COUNT * TAF_PA_RADIO_PCI_SCAN_PLMN_ID_MAX_COUNT)
#define PLMN_INFO_MAX_COUNT (INSTANCE_MAX_COUNT * TAF_PA_RADIO_PLMN_SCAN_NETWORK_MAX_COUNT)
#define PREF_NET_MAX_COUNT (INSTANCE_MAX_COUNT * TAF_PA_RADIO_PREFERRED_NETWORK_MAX_COUNT)
#define NGBR_CELL_MAX_COUNT (INSTANCE_MAX_COUNT * TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT)
#define SAFE_REF_MAX_COUNT (COMMON_RERERENCE_MAX_COUNT + PCI_CELL_MAX_COUNT + PLMN_ID_MAX_COUNT+ PLMN_INFO_MAX_COUNT + PREF_NET_MAX_COUNT + NGBR_CELL_MAX_COUNT)

//--------------------------------------------------------------------------------------------------
/**
 * Command identifiers for asynchronous requests handled by the internal request thread.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    COMMAND_UNKNOWN = 0,                         ///< Unknown request command.
    COMMAND_SET_NETWORK_SELECTION_PREFERENCE = 1,///< Set manual network selection preference.
    COMMAND_PERFORM_PLMN_NETWORK_SCAN = 2,       ///< Perform a PLMN network scan.
    COMMAND_PERFORM_PCI_NETWORK_SCAN = 3         ///< Perform a PCI network scan.
} Command_t;

//--------------------------------------------------------------------------------------------------
/**
 * Manual network selection preference (MCC/MNC string form).
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char mcc[TAF_RADIO_MCC_BYTES]; ///< Mobile country code (ASCII digits, null terminated).
    char mnc[TAF_RADIO_MNC_BYTES]; ///< Mobile network code (ASCII digits, null terminated).
} NetworkSelectionPreference_t;

//--------------------------------------------------------------------------------------------------
/**
 * Request payload used by the asynchronous request thread.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    Command_t command;      ///< Command to execute.
    uint32_t phone;         ///< Phone ID as exposed by the public API (1-based).
    void* handlerFuncPtr;   ///< Client callback pointer.
    void* contextPtr;       ///< Client context pointer.
    union
    {
        taf_radio_RatBitMask_t rat;             ///< RAT bitmask used by PCI scans.
        NetworkSelectionPreference_t preference;///< Network selection preference for manual select.
    };
} Request_t;

//--------------------------------------------------------------------------------------------------
/**
 * Generic safe-reference holder.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    void* safeRef;      ///< Opaque pointer stored in the ref map.
    le_sls_Link_t link; ///< Link used to chain objects into an SLS list.
} SafeRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Common list container shared by multiple scan/list APIs.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sls_List_t commonList;  ///< List of objects returned to the caller.
    le_sls_List_t safeRefList; ///< List of safe references owned by this list.
    le_sls_Link_t* currPtr;    ///< Iterator cursor used by GetFirst/GetNext style APIs.
} CommonList_t;

//--------------------------------------------------------------------------------------------------
/**
 * PCI scan cell information container.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t cellId;         ///< Physical cell ID (PCI).
    uint32_t globalCellId;   ///< Global cell identifier.
    le_sls_List_t plmnIdList;///< List of PLMN IDs associated with this cell.
    le_sls_List_t safeRefList; ///< List of safe references owned by this cell.
    le_sls_Link_t* currPtr;  ///< Iterator cursor for PLMN ID list traversal.
    le_sls_Link_t link;      ///< Link used to chain into the parent CommonList_t.
} PciCell_t;

//--------------------------------------------------------------------------------------------------
/**
 * PLMN ID container used by PCI scan results.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t mcc;                ///< Mobile country code.
    uint16_t mnc;                ///< Mobile network code.
    uint8_t mncIncludesPcsDigit; ///< Indicates whether the MNC contains a PCS digit.
    le_sls_Link_t link;          ///< Link used to chain into the parent PciCell_t list.
} PlmnId_t;

//--------------------------------------------------------------------------------------------------
/**
 * PLMN scan result entry container.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_pa_radio_PlmnInformation_t plmnInfo; ///< PLMN information returned by the PA layer.
    le_sls_Link_t link;                      ///< Link used to chain into the parent CommonList_t.
} PlmnInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Preferred network entry container.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_pa_radio_PreferredNetwork_t prefNet; ///< Preferred network entry returned by the PA layer.
    le_sls_Link_t link;                      ///< Link used to chain into an SLS list.
} PrefNet_t;

//--------------------------------------------------------------------------------------------------
/**
 * Neighbor cell entry container.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_pa_radio_CellLocationInfo_t ngbrCell; ///< Neighbor cell location info returned by PA.
    le_sls_Link_t link;                       ///< Link used to chain into an SLS list.
} NgbrCell_t;

//--------------------------------------------------------------------------------------------------
/**
 * Static event identifiers owned by the component.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_event_Id_t request; ///< Event used to dispatch internal asynchronous requests.
} StaticEvent_t;

//--------------------------------------------------------------------------------------------------
/**
 * Event identifiers owned by the radio service component.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_event_Id_t networkRejection;             ///< Network rejection indications.
    le_event_Id_t ratChange;                    ///< RAT change indications.
    le_event_Id_t netRegState;                  ///< Voice registration state indications.
    le_event_Id_t packetSwitchedState;          ///< Packet switched registration state indications.
    le_event_Id_t gsmSignalStrengthInfoChange;  ///< GSM signal strength indications.
    le_event_Id_t cdmaSignalStrengthInfoChange; ///< CDMA signal strength indications.
    le_event_Id_t umtsSignalStrengthInfoChange; ///< UMTS signal strength indications.
    le_event_Id_t tdscdmaSignalStrengthInfoChange; ///< TDSCDMA signal strength indications.
    le_event_Id_t lteSignalStrengthInfoChange;  ///< LTE signal strength indications.
    le_event_Id_t nr5gSignalStrengthInfoChange; ///< NR5G signal strength indications.
    le_event_Id_t imsRegStatusChange;           ///< IMS registration status indications.
    le_event_Id_t operatingModeChange;          ///< Operating mode change indications.
    le_event_Id_t netStatusChange;              ///< Network status indications.
    le_event_Id_t imsStatusChange;              ///< IMS status indications.
    le_event_Id_t cellInfoChange;               ///< Cell information change indications.
    le_event_Id_t nrIconChange;                 ///< NR icon change indications.
    le_event_Id_t caInfoChange;                 ///< Carrier aggregation information change indications.
    le_event_Id_t connStatusChange;             ///< Connection status indications.
} Event_t;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pools backing temporary and ref-counted objects used by this component.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_mem_PoolRef_t networkRejection;           ///< Pool for network rejection indications.
    le_mem_PoolRef_t ratChange;                  ///< Pool for RAT change indications.
    le_mem_PoolRef_t netRegState;                ///< Pool for registration state indications.
    le_mem_PoolRef_t signalStrengthInfoChange;   ///< Pool for signal strength indications.
    le_mem_PoolRef_t imsRegStatusChange;         ///< Pool for IMS registration status indications.
    le_mem_PoolRef_t operatingModeChange;        ///< Pool for operating mode change indications.
    le_mem_PoolRef_t netStatusChange;            ///< Pool for network status indications.
    le_mem_PoolRef_t imsStatusChange;            ///< Pool for IMS status indications.
    le_mem_PoolRef_t cellInfoChange;             ///< Pool for cell info change indications.
    le_mem_PoolRef_t nrIconChange;               ///< Pool for NR icon change indications.
    le_mem_PoolRef_t caInfoChange;               ///< Pool for CA info change indications.
    le_mem_PoolRef_t connStatusChange;           ///< Pool for connection status change indications.
    le_mem_PoolRef_t commonList;                 ///< Pool for CommonList_t containers.
    le_mem_PoolRef_t pciCell;                    ///< Pool for PCI scan cell entries.
    le_mem_PoolRef_t plmnId;                     ///< Pool for PLMN ID entries.
    le_mem_PoolRef_t plmnInfo;                   ///< Pool for PLMN scan entries.
    le_mem_PoolRef_t prefNet;                    ///< Pool for preferred network entries.
    le_mem_PoolRef_t ngbrCell;                   ///< Pool for neighbor cell entries.
    le_mem_PoolRef_t safeRef;                    ///< Pool for generic safe reference holders.
    le_mem_PoolRef_t signalStrengthInfo;         ///< Pool for cached signal strength info.
    le_mem_PoolRef_t caInfo;                     ///< Pool for cached CA info.
    le_mem_PoolRef_t connStatus;                 ///< Pool for cached connection status info.
} Pool_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference maps used to validate and resolve opaque handles.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_ref_MapRef_t commonList;          ///< Map for CommonList_t handles.
    le_ref_MapRef_t safeRef;             ///< Map for safe reference handles.
    le_ref_MapRef_t signalStrengthInfo;  ///< Map for signal strength info handles.
    le_ref_MapRef_t caInfo;              ///< Map for CA info handles.
    le_ref_MapRef_t connStatus;          ///< Map for connection status handles.
} Map_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through layered events for signal strength indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phone; ///< Phone ID as exposed by the public API (1-based).
    int32_t rssi;  ///< RSSI value, or TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE when unavailable.
    int32_t rsrp;  ///< RSRP value, or TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE when unavailable.
} SignalStrengthInfoInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through layered events for IMS registration status indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phone;                ///< Phone ID as exposed by the public API (1-based).
    taf_radio_ImsRegStatus_t status; ///< IMS registration status.
} ImsRegStatusInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through layered events for network status indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phone;                        ///< Phone ID as exposed by the public API (1-based).
    taf_radio_NetStatusIndBitMask_t bitmask; ///< Changed fields bitmask.
    taf_radio_NetStatusRef_t reference;      ///< Cached network status reference.
} NetStatusInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through layered events for IMS status indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phone;                     ///< Phone ID as exposed by the public API (1-based).
    taf_radio_ImsIndBitMask_t bitmask; ///< Changed fields bitmask.
    taf_radio_ImsRef_t reference;      ///< Cached IMS status reference.
} ImsStatusInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through layered events for cell info change indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phone;                  ///< Phone ID as exposed by the public API (1-based).
    taf_radio_CellInfoStatus_t status; ///< Cell info change status.
} CellInfoInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through layered events for NR icon indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phone;              ///< Phone ID as exposed by the public API (1-based).
    taf_radio_NrIconType_t icon; ///< NR icon type.
} NrIconInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through layered events for carrier aggregation indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phone;                 ///< Phone ID as exposed by the public API (1-based).
    taf_radio_CAInfoRef_t reference; ///< Cached CA info reference.
} CAInfoInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through layered events for connection status indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phone;                     ///< Phone ID as exposed by the public API (1-based).
    taf_radio_ConnIndBitMask_t bitmask; ///< Changed fields bitmask.
    taf_radio_ConnStatusRef_t reference; ///< Cached connection status reference.
} ConnStatusInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Carrier aggregation information cached per instance.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_radio_CAStatus_t status; ///< CA activation status.
    uint32_t cellCount;          ///< Number of component carriers (PCell + active SCells).
} CAInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Signal strength hysteresis configuration.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t time; ///< Hysteresis time in seconds.
    std::map<taf_radio_SigType_t, uint16_t> delta; ///< Per-metric hysteresis delta values.
} HysteresisConfig_t;

//--------------------------------------------------------------------------------------------------
/**
 * Cached state maintained by the component for each instance.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t netRejectCause;                              ///< Cached network reject cause.
    taf_pa_radio_Rat_t rat[INSTANCE_MAX_COUNT];          ///< Cached RAT per instance.
    taf_pa_radio_DataServiceState_t dataServiceState[INSTANCE_MAX_COUNT]; ///< Cached data svc state.
    taf_radio_NetRegState_t packetSwitchedState[INSTANCE_MAX_COUNT]; ///< Cached PS reg state.
    HysteresisConfig_t hysteresisConfig[INSTANCE_MAX_COUNT]; ///< Cached hysteresis config.
    taf_radio_NetStatusRef_t netStatusRefs[INSTANCE_MAX_COUNT]; ///< Cached net status references.
    taf_pa_radio_RatServiceStatus_t ratSvcState[INSTANCE_MAX_COUNT]; ///< Cached RAT svc state.
    taf_pa_radio_ServiceDomain_t svcDomain[INSTANCE_MAX_COUNT]; ///< Cached service domain.
    taf_radio_ImsRef_t imsRefs[INSTANCE_MAX_COUNT];      ///< Cached IMS references.
    taf_radio_CAInfoRef_t caInfoRefs[INSTANCE_MAX_COUNT]; ///< Cached CA references.
    taf_radio_ConnStatusRef_t connStatusRefs[INSTANCE_MAX_COUNT]; ///< Cached connection refs.
    taf_radio_NetRegState_t netRegState[INSTANCE_MAX_COUNT]; ///< Cached network reg state.
    std::mutex sNetRegStateMutex[INSTANCE_MAX_COUNT];
} Cache_t;

//--------------------------------------------------------------------------------------------------
/**
 * Utility class for conversion and common helper operations.
 */
//--------------------------------------------------------------------------------------------------
class Utility
{
    public:
        class Convert
        {
            public:
                /**
                 * Converts PA result to Legato result.
                 *
                 * @return
                 *      - LE_OK if the PA layer returned 0.
                 *      - LE_FAULT if the PA layer returned -EFAULT, or any unmapped error.
                 *      - LE_TIMEOUT if the PA layer returned -ETIMEDOUT.
                 *      - LE_OUT_OF_RANGE if the PA layer returned -ERANGE.
                 *      - LE_BAD_PARAMETER if the PA layer returned -EINVAL.
                 *      - LE_UNSUPPORTED if the PA layer returned -ENOTSUP.
                 *      - LE_NOT_IMPLEMENTED if the PA layer returned -ENOSYS.
                 */
                static le_result_t Result
                (
                    pa_result_t result ///< [IN] PA result.
                );

                /**
                 * Converts an ASCII digit string to uint16_t.
                 *
                 * @return
                 *      - LE_OK if conversion succeeds.
                 *      - LE_BAD_PARAMETER if stringPtr/valuePtr is null, empty, or contains non-digits.
                 *      - LE_OUT_OF_RANGE if the value is outside [0, 999].
                 */
                static le_result_t StringToU16
                (
                    const char* stringPtr, ///< [IN] Null-terminated ASCII digit string.
                    uint16_t* valuePtr     ///< [OUT] Converted value.
                );

                /**
                 * Converts uint16_t in range [0, 999] to a null-terminated ASCII digit string.
                 *
                 * @return
                 *      - LE_OK if conversion succeeds.
                 *      - LE_BAD_PARAMETER if stringPtr is null.
                 *      - LE_OUT_OF_RANGE if value is outside [0, 999].
                 */
                static le_result_t U16ToString
                (
                    uint16_t value, ///< [IN] Value to convert.
                    char* stringPtr, ///< [OUT] Output buffer.
                    size_t length,   ///< [IN] Output buffer size in bytes.
                    bool padding     ///< [IN] If true, left-pad single digit numbers with '0'.
                );

                /**
                 * Converts public API phone ID (1-based) to internal instance index (0-based).
                 *
                 * @return
                 *      - Instance index in range [0, INSTANCE_MAX_COUNT).
                 *      - INSTANCE_MAX_COUNT when phone is invalid.
                 */
                static uint32_t PhoneToInstance
                (
                    uint8_t phone ///< [IN] Phone ID.
                );

                /**
                 * Converts internal instance index (0-based) to public API phone ID (1-based).
                 *
                 * @return
                 *      - Phone ID (1..INSTANCE_MAX_COUNT) for valid instances.
                 *      - 0 when instance is invalid.
                 */
                static uint8_t InstanceToPhone
                (
                    uint32_t instance ///< [IN] Instance index.
                );

                /**
                 * Resolves a cached network status reference to its instance index.
                 *
                 * @return
                 *      - LE_OK if found.
                 *      - LE_NOT_FOUND if reference does not match any instance.
                 */
                static le_result_t ReferenceToInstance
                (
                    taf_radio_NetStatusRef_t reference, ///< [IN] Network status reference.
                    uint32_t* instancePtr               ///< [OUT] Instance index.
                );

                /**
                 * Resolves a cached IMS status reference to its instance index.
                 *
                 * @return
                 *      - LE_OK if found.
                 *      - LE_NOT_FOUND if reference does not match any instance.
                 */
                static le_result_t ReferenceToInstance
                (
                    taf_radio_ImsRef_t reference, ///< [IN] IMS reference.
                    uint32_t* instancePtr         ///< [OUT] Instance index.
                );

                /**
                 * Converts a public RAT bitmask to the PA RAT bitmask.
                 */
                static taf_pa_radio_RatBitMask_t Rat
                (
                    taf_radio_RatBitMask_t bitmask ///< [IN] Public RAT bitmask.
                );

                /**
                 * Converts a PA RAT bitmask to the public RAT bitmask.
                 */
                static taf_radio_RatBitMask_t Rat
                (
                    taf_pa_radio_RatBitMask_t bitmask ///< [IN] PA RAT bitmask.
                );

                /**
                 * Converts PA RAT to public RAT.
                 */
                static taf_radio_Rat_t Rat
                (
                    taf_pa_radio_Rat_t rat ///< [IN] PA RAT.
                );

                /**
                 * Converts PA service domain to public service domain state.
                 */
                static taf_radio_ServiceDomainState_t ServiceDomain
                (
                    taf_pa_radio_ServiceDomain_t domain ///< [IN] PA service domain.
                );

                /**
                 * Converts PA service domain bitmask to public service domain state.
                 */
                static taf_radio_ServiceDomainState_t ServiceDomain
                (
                    taf_pa_radio_ServiceDomainBitMask_t bitmask ///< [IN] PA service domain bitmask.
                );

                /**
                 * Converts public service domain to PA service domain bitmask.
                 */
                static taf_pa_radio_ServiceDomainBitMask_t ServiceDomain
                (
                    taf_radio_ServiceDomainState_t domain ///< [IN] Public service domain.
                );

                /**
                 * Converts PA voice service info to public registration state.
                 */
                static taf_radio_NetRegState_t NetRegState
                (
                    taf_pa_radio_VoiceServiceInfo_t* infoPtr ///< [IN] PA voice service info.
                );

                /**
                 * Converts PA data service state/roaming status to public registration state.
                 */
                static taf_radio_NetRegState_t NetRegState
                (
                    taf_pa_radio_DataServiceState_t state, ///< [IN] Data service state.
                    taf_pa_radio_DataRoamingStatus_t status ///< [IN] Roaming status.
                );

                /**
                 * Converts PA signal strength level to the corresponding integer level (1..5).
                 */
                static uint32_t SignalStrengthLevel
                (
                    taf_pa_radio_SignalStrengthLevel_t level ///< [IN] PA signal strength level.
                );

                /**
                 * Builds the PA signal strength indication config based on cached hysteresis settings.
                 */
                static void SignalStrengthIndConfig
                (
                    uint32_t instance, ///< [IN] Instance index.
                    taf_radio_SigType_t metric, ///< [IN] Signal type.
                    taf_pa_radio_SignalStrengthIndConfig_t* configPtr ///< [OUT] Config to populate.
                );

                /**
                 * Converts PA band bitmask to the public band bitmask.
                 */
                static taf_radio_BandBitMask_t ToBand
                (
                    taf_pa_radio_BandBitMask_t bitmask ///< [IN] PA band bitmask.
                );

                /**
                 * Converts public band bitmask to the PA band bitmask.
                 */
                static taf_pa_radio_BandBitMask_t ToPaBand
                (
                    taf_radio_BandBitMask_t bitmask ///< [IN] Public band bitmask.
                );

                /**
                 * Converts PA bandwidth value to public RF bandwidth value.
                 */
                static taf_radio_RFBandWidth_t Bandwidth
                (
                    taf_pa_radio_Bandwidth_t bandwidth ///< [IN] PA bandwidth.
                );

                /**
                 * Converts PA IMS registration status to public IMS registration status.
                 */
                static taf_radio_ImsRegStatus_t ImsRegistrationStatus
                (
                    taf_pa_radio_ImsRegistrationStatus_t status ///< [IN] PA IMS status.
                );

                /**
                 * Converts PA operating mode to public operating mode.
                 *
                 * @return
                 *      - LE_OK if conversion succeeds.
                 *      - LE_BAD_PARAMETER for unsupported mode values.
                 */
                static le_result_t OperatingMode
                (
                    taf_pa_radio_OperatingMode_t mode, ///< [IN] PA operating mode.
                    taf_radio_OpMode_t* modePtr        ///< [OUT] Public operating mode.
                );

                /**
                 * Converts public operating mode to PA operating mode.
                 */
                static taf_pa_radio_OperatingMode_t OperatingMode
                (
                    taf_radio_OpMode_t mode ///< [IN] Public operating mode.
                );

                /**
                 * Converts PA LTE CS capability to public LTE CS capability.
                 */
                static taf_radio_CsCap_t LteCsCapability
                (
                    taf_pa_radio_LteCsCapability_t capability ///< [IN] PA capability.
                );

                /**
                 * Converts public IMS service type to PA IMS service.
                 *
                 * @return
                 *      - LE_OK if conversion succeeds.
                 *      - LE_BAD_PARAMETER for unsupported service values.
                 */
                static le_result_t ImsService
                (
                    taf_radio_ImsSvcType_t service, ///< [IN] Public service type.
                    taf_pa_radio_ImsService_t* servicePtr ///< [OUT] PA service.
                );

                /**
                 * Converts public IMS service type to PA IMS service setting bitmask.
                 */
                static taf_pa_radio_ImsServiceSettingBitMask_t ImsService
                (
                    taf_radio_ImsSvcType_t service ///< [IN] Public service type.
                );

                /**
                 * Converts PA IMS service status to public IMS service status.
                 */
                static taf_radio_ImsSvcStatus_t ImsServiceStatus
                (
                    taf_pa_radio_ImsServiceStatus_t status ///< [IN] PA service status.
                );

                /**
                 * Converts PA IMS PDP failure error code to public PDP error.
                 */
                static taf_radio_PdpError_t PdpError
                (
                    taf_pa_radio_ImsPdpFailureErrorCode_t code ///< [IN] PA error code.
                );

                /**
                 * Converts PA ENDC availability to public ENDC availability.
                 */
                static taf_radio_NREndcAvailability_t EndcAvailability
                (
                    taf_pa_radio_EndcAvailability_t availability ///< [IN] PA availability.
                );

                /**
                 * Converts PA DCNR restriction to public DCNR restriction.
                 */
                static taf_radio_NRDcnrRestriction_t DcnrRestriction
                (
                    taf_pa_radio_DcnrRestriction_t restriction ///< [IN] PA restriction.
                );

                /**
                 * Converts PA cell role bitmask to public cell info status.
                 *
                 * @return
                 *      - LE_OK if conversion succeeds.
                 *      - LE_BAD_PARAMETER if statusPtr is null or bitmask is unsupported.
                 */
                static le_result_t CellInfoStatus
                (
                    taf_pa_radio_CellRoleBitMask_t bitmask, ///< [IN] PA cell role bitmask.
                    taf_radio_CellInfoStatus_t* statusPtr   ///< [OUT] Public status.
                );

                /**
                 * Converts PA NR icon to public NR icon type.
                 */
                static taf_radio_NrIconType_t NrIcon
                (
                    taf_pa_radio_NrIcon_t icon ///< [IN] PA NR icon.
                );

                /**
                 * Converts PA RAT service status to public RAT service status.
                 */
                static taf_radio_RatSvcStatus_t RatServiceStatus
                (
                    taf_pa_radio_RatServiceStatus_t status ///< [IN] PA RAT service status.
                );

                /**
                 * Determines ENDC availability based on PA data available system status.
                 */
                static taf_radio_NREndcAvailability_t EndcStatus
                (
                    taf_pa_radio_DataAvailSysStatus_t* statusPtr ///< [IN] PA status.
                );

                /**
                 * Populates cached CA info from a PA LTE CPHY CA info structure.
                 */
                static void LteCphyCaInfo
                (
                    taf_pa_radio_LteCphyCaInfo_t* paInfoPtr, ///< [IN] PA CA info.
                    CAInfo_t* infoPtr                        ///< [OUT] Cached CA info.
                );

                static taf_radio_NetRegState_t CombineNetRegState
                (
                    taf_radio_NetRegState_t voiceState,
                    taf_radio_NetRegState_t dataState
                );
        };

        /**
         * Wrapper callbacks used by Legato layered events to bridge generic event payloads to the
         * typed public handler signatures exposed by this service.
         */
        class LayeredFunction
        {
            public:
                /**
                 * Dispatches network rejection indications and releases the ref-counted payload.
                 */
                static void NetworkRejection
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches RAT change indications and releases the ref-counted payload.
                 */
                static void RatChange
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches network registration state indications and releases the ref-counted payload.
                 */
                static void NetRegState
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches signal strength indications and releases the ref-counted payload.
                 */
                static void SignalStrengthInfoChange
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches IMS registration status indications and releases the ref-counted payload.
                 */
                static void ImsRegStatusChange
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches operating mode change indications and releases the ref-counted payload.
                 */
                static void OperatingModeChange
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches network status indications and releases the ref-counted payload.
                 */
                static void NetStatusChange
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches IMS status indications and releases the ref-counted payload.
                 */
                static void ImsStatusChange
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches cell info change indications and releases the ref-counted payload.
                 */
                static void CellInfoChange
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches NR icon indications and releases the ref-counted payload.
                 */
                static void NrIconChange
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches carrier aggregation info indications and releases the ref-counted payload.
                 */
                static void CAInfoChange
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches connection status indications and releases the ref-counted payload.
                 */
                static void ConnStatusChange
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );
        };

        /**
         * Collection of common helper functions used by the public APIs.
         */
        class Common
        {
            public:
                /**
                 * Performs a PCI network scan synchronously and returns a list reference.
                 *
                 * @return
                 *      - A valid list reference on success.
                 *      - nullptr if the scan fails.
                 */
                static taf_radio_PciScanInformationListRef_t PciNetworkScan
                (
                    uint8_t phone,              ///< [IN] Phone ID.
                    taf_radio_RatBitMask_t bitmask ///< [IN] RAT bitmask for the scan.
                );

                /**
                 * Performs a PLMN network scan synchronously and returns a list reference.
                 *
                 * @return
                 *      - A valid list reference on success.
                 *      - nullptr if the scan fails.
                 */
                static taf_radio_ScanInformationListRef_t PlmnNetworkScan
                (
                    uint8_t phone ///< [IN] Phone ID.
                );

                /**
                 * Performs manual network selection with the given MCC/MNC.
                 *
                 * @return
                 *      - LE_OK on success.
                 *      - LE_BAD_PARAMETER on invalid inputs.
                 *      - LE_OUT_OF_RANGE if MCC/MNC is outside [0, 999].
                 *      - LE_FAULT/LE_TIMEOUT/LE_UNSUPPORTED/LE_NOT_IMPLEMENTED depending on PA error.
                 */
                static le_result_t ManualNetworkSelection
                (
                    uint8_t phone,      ///< [IN] Phone ID.
                    const char* mccPtr, ///< [IN] MCC string.
                    const char* mncPtr  ///< [IN] MNC string.
                );

                /**
                 * Finds the serving cell index within a PA cell location list.
                 *
                 * @return
                 *      - Index of the serving cell.
                 *      - TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT if not found or input is null.
                 */
                static uint32_t FindServingCell
                (
                    taf_pa_radio_CellLocationListInfo_t* infoPtr ///< [IN] Cell location list.
                );
        };
};

//--------------------------------------------------------------------------------------------------
/**
 * Radio service factory class.
 */
//--------------------------------------------------------------------------------------------------
class Factory
{
    public:
        /**
         * Gets the singleton factory instance.
         *
         * @return
         *      - Reference to the singleton Factory instance.
         */
        static Factory& GetInstance
        (
            void
        );

        static StaticEvent_t staticEvents; ///< Static events used by this component.
        Cache_t cache;                     ///< Cached runtime state.
        Event_t events;                    ///< Event identifiers used to fan out indications.
        Pool_t pools;                      ///< Memory pools used by this component.
        Map_t maps;                        ///< Reference maps used by this component.
};

#endif /* #ifndef TAFRADIO_HPP */
