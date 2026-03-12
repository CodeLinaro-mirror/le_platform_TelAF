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

typedef enum
{
    COMMAND_UNKNOWN = 0,
    COMMAND_SET_NETWORK_SELECTION_PREFERENCE = 1,
    COMMAND_PERFORM_PLMN_NETWORK_SCAN = 2,
    COMMAND_PERFORM_PCI_NETWORK_SCAN = 3
} Command_t;

typedef struct
{
    char mcc[TAF_RADIO_MCC_BYTES];
    char mnc[TAF_RADIO_MNC_BYTES];
} NetworkSelectionPreference_t;

typedef struct
{
    Command_t command;
    uint32_t phone;
    void* handlerFuncPtr;
    void* contextPtr;
    union
    {
        taf_radio_RatBitMask_t rat;
        NetworkSelectionPreference_t preference;
    };
} Request_t;

typedef struct
{
    void* safeRef;
    le_sls_Link_t link;
} SafeRef_t;

typedef struct
{
    le_sls_List_t commonList;
    le_sls_List_t safeRefList;
    le_sls_Link_t* currPtr;
} CommonList_t;

typedef struct
{
    uint16_t cellId;
    uint32_t globalCellId;
    le_sls_List_t plmnIdList;
    le_sls_List_t safeRefList;
    le_sls_Link_t* currPtr;
    le_sls_Link_t link;
} PciCell_t;

typedef struct
{
    uint16_t mcc;
    uint16_t mnc;
    uint8_t mncIncludesPcsDigit;
    le_sls_Link_t link;
} PlmnId_t;

typedef struct
{
    taf_pa_radio_PlmnInformation_t plmnInfo;
    le_sls_Link_t link;
} PlmnInfo_t;

typedef struct
{
    taf_pa_radio_PreferredNetwork_t prefNet;
    le_sls_Link_t link;
} PrefNet_t;

typedef struct
{
    taf_pa_radio_CellLocationInfo_t ngbrCell;
    le_sls_Link_t link;
} NgbrCell_t;

typedef struct
{
    le_event_Id_t request;
} StaticEvent_t;

typedef struct
{
    le_event_Id_t networkRejection;
    le_event_Id_t ratChange;
    le_event_Id_t netRegState;
    le_event_Id_t packetSwitchedState;
    le_event_Id_t gsmSignalStrengthInfoChange;
    le_event_Id_t cdmaSignalStrengthInfoChange;
    le_event_Id_t umtsSignalStrengthInfoChange;
    le_event_Id_t tdscdmaSignalStrengthInfoChange;
    le_event_Id_t lteSignalStrengthInfoChange;
    le_event_Id_t nr5gSignalStrengthInfoChange;
    le_event_Id_t imsRegStatusChange;
    le_event_Id_t operatingModeChange;
    le_event_Id_t netStatusChange;
    le_event_Id_t imsStatusChange;
    le_event_Id_t cellInfoChange;
    le_event_Id_t nrIconChange;
    le_event_Id_t caInfoChange;
    le_event_Id_t connStatusChange;
} Event_t;

typedef struct
{
    le_mem_PoolRef_t networkRejection;
    le_mem_PoolRef_t ratChange;
    le_mem_PoolRef_t netRegState;
    le_mem_PoolRef_t signalStrengthInfoChange;
    le_mem_PoolRef_t imsRegStatusChange;
    le_mem_PoolRef_t operatingModeChange;
    le_mem_PoolRef_t netStatusChange;
    le_mem_PoolRef_t imsStatusChange;
    le_mem_PoolRef_t cellInfoChange;
    le_mem_PoolRef_t nrIconChange;
    le_mem_PoolRef_t caInfoChange;
    le_mem_PoolRef_t connStatusChange;
    le_mem_PoolRef_t commonList;
    le_mem_PoolRef_t pciCell;
    le_mem_PoolRef_t plmnId;
    le_mem_PoolRef_t plmnInfo;
    le_mem_PoolRef_t prefNet;
    le_mem_PoolRef_t ngbrCell;
    le_mem_PoolRef_t safeRef;
    le_mem_PoolRef_t signalStrengthInfo;
    le_mem_PoolRef_t caInfo;
    le_mem_PoolRef_t connStatus;
} Pool_t;

typedef struct
{
    le_ref_MapRef_t commonList;
    le_ref_MapRef_t safeRef;
    le_ref_MapRef_t signalStrengthInfo;
    le_ref_MapRef_t caInfo;
    le_ref_MapRef_t connStatus;
} Map_t;

typedef struct
{
    uint8_t phone;
    int32_t rssi;
    int32_t rsrp;
} SignalStrengthInfoInd_t;

typedef struct
{
    uint8_t phone;
    taf_radio_ImsRegStatus_t status;
} ImsRegStatusInd_t;

typedef struct
{
    uint8_t phone;
    taf_radio_NetStatusIndBitMask_t bitmask;
    taf_radio_NetStatusRef_t reference;
} NetStatusInd_t;

typedef struct
{
    uint8_t phone;
    taf_radio_ImsIndBitMask_t bitmask;
    taf_radio_ImsRef_t reference;
} ImsStatusInd_t;

typedef struct
{
    uint8_t phone;
    taf_radio_CellInfoStatus_t status;
} CellInfoInd_t;

typedef struct
{
    uint8_t phone;
    taf_radio_NrIconType_t icon;
} NrIconInd_t;

typedef struct
{
    uint8_t phone;
    taf_radio_CAInfoRef_t reference;
} CAInfoInd_t;

typedef struct
{
    uint8_t phone;
    taf_radio_ConnIndBitMask_t bitmask;
    taf_radio_ConnStatusRef_t reference;
} ConnStatusInd_t;

typedef struct
{
    taf_radio_CAStatus_t status;
    uint32_t cellCount;
} CAInfo_t;

typedef struct
{
    uint16_t time;
    std::map<taf_radio_SigType_t, uint16_t> delta;
} HysteresisConfig_t;

typedef struct
{
    int32_t netRejectCause;
    taf_pa_radio_Rat_t rat[INSTANCE_MAX_COUNT];
    taf_pa_radio_DataServiceState_t dataServiceState[INSTANCE_MAX_COUNT];
    taf_radio_NetRegState_t packetSwitchedState[INSTANCE_MAX_COUNT];
    HysteresisConfig_t hysteresisConfig[INSTANCE_MAX_COUNT];
    taf_radio_NetStatusRef_t netStatusRefs[INSTANCE_MAX_COUNT];
    taf_pa_radio_RatServiceStatus_t ratSvcState[INSTANCE_MAX_COUNT];
    taf_pa_radio_ServiceDomain_t svcDomain[INSTANCE_MAX_COUNT];
    taf_radio_ImsRef_t imsRefs[INSTANCE_MAX_COUNT];
    taf_radio_CAInfoRef_t caInfoRefs[INSTANCE_MAX_COUNT];
    taf_radio_ConnStatusRef_t connStatusRefs[INSTANCE_MAX_COUNT];
} Cache_t;

class Utility
{
    public:
        class Convert
        {
            public:
                static le_result_t Result
                (
                    pa_result_t result
                );

                static le_result_t StringToU16
                (
                    const char* stringPtr,
                    uint16_t* valuePtr
                );

                static le_result_t U16ToString
                (
                    uint16_t value,
                    char* stringPtr,
                    size_t length,
                    bool padding
                );

                static uint32_t PhoneToInstance
                (
                    uint8_t phone
                );

                static uint8_t InstanceToPhone
                (
                    uint32_t instance
                );

                static le_result_t ReferenceToInstance
                (
                    taf_radio_NetStatusRef_t reference,
                    uint32_t* instancePtr
                );

                static le_result_t ReferenceToInstance
                (
                    taf_radio_ImsRef_t reference,
                    uint32_t* instancePtr
                );

                static taf_pa_radio_RatBitMask_t Rat
                (
                    taf_radio_RatBitMask_t bitmask
                );

                static taf_radio_RatBitMask_t Rat
                (
                    taf_pa_radio_RatBitMask_t bitmask
                );

                static taf_radio_Rat_t Rat
                (
                    taf_pa_radio_Rat_t rat
                );

                static taf_radio_ServiceDomainState_t ServiceDomain
                (
                    taf_pa_radio_ServiceDomain_t domain
                );

                static taf_radio_ServiceDomainState_t ServiceDomain
                (
                    taf_pa_radio_ServiceDomainBitMask_t bitmask
                );

                static taf_pa_radio_ServiceDomainBitMask_t ServiceDomain
                (
                    taf_radio_ServiceDomainState_t domain
                );

                static taf_radio_NetRegState_t NetRegState
                (
                    taf_pa_radio_VoiceServiceInfo_t* infoPtr
                );

                static taf_radio_NetRegState_t NetRegState
                (
                    taf_pa_radio_DataServiceState_t state,
                    taf_pa_radio_DataRoamingStatus_t status
                );

                static uint32_t SignalStrengthLevel
                (
                    taf_pa_radio_SignalStrengthLevel_t level
                );

                static void SignalStrengthIndConfig
                (
                    uint32_t instance,
                    taf_radio_SigType_t metric,
                    taf_pa_radio_SignalStrengthIndConfig_t* configPtr
                );

                static taf_radio_BandBitMask_t ToBand
                (
                    taf_pa_radio_BandBitMask_t bitmask
                );

                static taf_pa_radio_BandBitMask_t ToPaBand
                (
                    taf_radio_BandBitMask_t bitmask
                );

                static taf_radio_RFBandWidth_t Bandwidth
                (
                    taf_pa_radio_Bandwidth_t bandwidth
                );

                static taf_radio_ImsRegStatus_t ImsRegistrationStatus
                (
                    taf_pa_radio_ImsRegistrationStatus_t status
                );

                static le_result_t OperatingMode
                (
                    taf_pa_radio_OperatingMode_t mode,
                    taf_radio_OpMode_t* modePtr
                );

                static taf_pa_radio_OperatingMode_t OperatingMode
                (
                    taf_radio_OpMode_t mode
                );

                static taf_radio_CsCap_t LteCsCapability
                (
                    taf_pa_radio_LteCsCapability_t capability
                );

                static le_result_t ImsService
                (
                    taf_radio_ImsSvcType_t service,
                    taf_pa_radio_ImsService_t* servicePtr
                );

                static taf_pa_radio_ImsServiceSettingBitMask_t ImsService
                (
                    taf_radio_ImsSvcType_t service
                );

                static taf_radio_ImsSvcStatus_t ImsServiceStatus
                (
                    taf_pa_radio_ImsServiceStatus_t status
                );

                static taf_radio_PdpError_t PdpError
                (
                    taf_pa_radio_ImsPdpFailureErrorCode_t code
                );

                static taf_radio_NREndcAvailability_t EndcAvailability
                (
                    taf_pa_radio_EndcAvailability_t availability
                );

                static taf_radio_NRDcnrRestriction_t DcnrRestriction
                (
                    taf_pa_radio_DcnrRestriction_t restriction
                );

                static le_result_t CellInfoStatus
                (
                    taf_pa_radio_CellRoleBitMask_t bitmask,
                    taf_radio_CellInfoStatus_t* statusPtr
                );

                static taf_radio_NrIconType_t NrIcon
                (
                    taf_pa_radio_NrIcon_t icon
                );

                static taf_radio_RatSvcStatus_t RatServiceStatus
                (
                    taf_pa_radio_RatServiceStatus_t status
                );

                static taf_radio_NREndcAvailability_t EndcStatus
                (
                    taf_pa_radio_DataAvailSysStatus_t* statusPtr
                );

                static void LteCphyCaInfo
                (
                    taf_pa_radio_LteCphyCaInfo_t* paInfoPtr,
                    CAInfo_t* infoPtr
                );
        };

        class LayeredFunction
        {
            public:
                static void NetworkRejection
                (
                    void* reportPtr,
                    void* handlerFuncPtr
                );

                static void RatChange
                (
                    void* reportPtr,
                    void* handlerFuncPtr
                );

                static void NetRegState
                (
                    void* reportPtr,
                    void* handlerFuncPtr
                );

                static void SignalStrengthInfoChange
                (
                    void* reportPtr,
                    void* handlerFuncPtr
                );

                static void ImsRegStatusChange
                (
                    void* reportPtr,
                    void* handlerFuncPtr
                );

                static void OperatingModeChange
                (
                    void* reportPtr,
                    void* handlerFuncPtr
                );

                static void NetStatusChange
                (
                    void* reportPtr,
                    void* handlerFuncPtr
                );

                static void ImsStatusChange
                (
                    void* reportPtr,
                    void* handlerFuncPtr
                );

                static void CellInfoChange
                (
                    void* reportPtr,
                    void* handlerFuncPtr
                );

                static void NrIconChange
                (
                    void* reportPtr,
                    void* handlerFuncPtr
                );

                static void CAInfoChange
                (
                    void* reportPtr,
                    void* handlerFuncPtr
                );

                static void ConnStatusChange
                (
                    void* reportPtr,
                    void* handlerFuncPtr
                );
        };

        class Common
        {
            public:
                static taf_radio_PciScanInformationListRef_t PciNetworkScan
                (
                    uint8_t phone,
                    taf_radio_RatBitMask_t bitmask
                );

                static taf_radio_ScanInformationListRef_t PlmnNetworkScan
                (
                    uint8_t phone
                );

                static le_result_t ManualNetworkSelection
                (
                    uint8_t phone,
                    const char* mccPtr,
                    const char* mncPtr
                );

                static uint32_t FindServingCell
                (
                    taf_pa_radio_CellLocationListInfo_t* infoPtr
                );
        };
};

class Factory
{
    public:
        static Factory& GetInstance
        (
            void
        );

        static StaticEvent_t staticEvents;
        Cache_t cache;
        Event_t events;
        Pool_t pools;
        Map_t maps;
};

#endif /* #ifndef TAFRADIO_HPP */
