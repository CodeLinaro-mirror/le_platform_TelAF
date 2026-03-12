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

#include "tafRadio.hpp"

using namespace std;

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for common lists
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(commonList, COMMON_LIST_MAX_COUNT, sizeof(CommonList_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for safe references.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(safeRef, SAFE_REF_MAX_COUNT, sizeof(SafeRef_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for PCI cells.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(pciCell, PCI_CELL_MAX_COUNT, sizeof(PciCell_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for PLMN IDs.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(plmnId, PLMN_ID_MAX_COUNT, sizeof(PlmnId_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for PLMN infomation.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(plmnInfo, PLMN_INFO_MAX_COUNT, sizeof(PlmnInfo_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for preferred networks.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(prefNet, PREF_NET_MAX_COUNT, sizeof(PrefNet_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for neighbor cells.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ngbrCell, NGBR_CELL_MAX_COUNT, sizeof(NgbrCell_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for signal strength information.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(signalStrengthInfo, INSTANCE_MAX_COUNT,
    sizeof(taf_pa_radio_SignalStrengthInfo_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for CA information.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(caInfo, CA_INFO_MAX_COUNT, sizeof(CAInfo_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for connection status.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(connStatus, CONN_STATUS_MAX_COUNT,
    sizeof(taf_radio_NREndcAvailability_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static map for common lists.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(commonList, COMMON_LIST_MAX_COUNT);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for safe references.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(safeRef, SAFE_REF_MAX_COUNT);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for signal strength information.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(signalStrengthInfo, INSTANCE_MAX_COUNT);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for CA information.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(caInfo, CA_INFO_MAX_COUNT);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for connection status.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(connStatus, CA_INFO_MAX_COUNT);

static void RegisterIndication
(
    uint8_t registration
)
{
    for (uint32_t i = 0; i < INSTANCE_MAX_COUNT; i++)
    {
        pa_result_t result = taf_pa_radio_RegisterIndication(i, registration);
        switch(result)
        {
            case 0:
                if (registration == ENABLE_INDICATION)
                    LE_INFO("Indication is enabled for instance %d.", i);
                else
                    LE_INFO("Indication is disabled for instance %d.", i);
                break;
            case -ENOTSUP:
            case -ENOSYS:
                break;
            default:
                LE_ERROR("Failed to register indication for instance %d.", i);
        }
    }
}

static void PowerStateChangeHandler
(
    taf_pm_State_t state,
    void* contextPtr
)
{
    if (state == TAF_PM_STATE_RESUME)
    {
        LE_INFO("Power state change to RESUME");
        RegisterIndication(ENABLE_INDICATION);
    }
    else if (state == TAF_PM_STATE_SUSPEND)
    {
        LE_INFO("Power state change to SUSPEND");
        RegisterIndication(DISABLE_INDICATION);
    }
}

static void DataAvailSysStatusHandler
(
    uint32_t instance,
    taf_pa_radio_DataAvailSysStatusIndication_t indication,
    void* contextPtr
)
{
    taf_radio_NREndcAvailability_t availability = TAF_RADIO_NR_ENDC_UNAVAILABLE;
    if (indication.availSysValid)
        availability = Utility::Convert::EndcStatus(&indication.availSys);

    auto& factory = Factory::GetInstance();
    if (instance < INSTANCE_MAX_COUNT)
    {
        taf_radio_NREndcAvailability_t* availabilityPtr =
           (taf_radio_NREndcAvailability_t*)le_ref_Lookup(factory.maps.connStatus,
            factory.cache.connStatusRefs[instance]);
        if (availability != *availabilityPtr)
        {
            *availabilityPtr = availability;

            ConnStatusInd_t* indPtr = (ConnStatusInd_t*)le_mem_ForceAlloc(
                factory.pools.connStatusChange);
            indPtr->phone = Utility::Convert::InstanceToPhone(instance);
            indPtr->bitmask = TAF_RADIO_CONN_IND_BIT_MASK_ENDC;
            indPtr->reference = factory.cache.connStatusRefs[instance];
            le_event_ReportWithRefCounting(factory.events.connStatusChange, (void*)indPtr);
        }
    }
}

static void NetworkRejectHandler
(
    uint32_t instance,
    taf_pa_radio_NetworkRejectIndication_t indication,
    void* contextPtr
)
{
    auto& factory = Factory::GetInstance();
    taf_radio_NetRegRejInd_t* indPtr = (taf_radio_NetRegRejInd_t*)le_mem_ForceAlloc(
        factory.pools.networkRejection);

    factory.cache.netRejectCause = indication.cause;

    le_result_t result = Utility::Convert::U16ToString(indication.plmnId.mcc, indPtr->mcc,
        TAF_RADIO_MCC_BYTES, false);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert MCC %d.", indication.plmnId.mcc);
        le_mem_Release((void*)indPtr);
        return;
    }

    result = Utility::Convert::U16ToString(indication.plmnId.mnc, indPtr->mnc, TAF_RADIO_MNC_BYTES,
        true);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert MNC %d.", indication.plmnId.mnc);
        le_mem_Release((void*)indPtr);
        return;
    }

    indPtr->phoneId = Utility::Convert::InstanceToPhone(instance);
    indPtr->rat = Utility::Convert::Rat(indication.rat);
    indPtr->domain = Utility::Convert::ServiceDomain(indication.domain);
    indPtr->cause = static_cast<taf_radio_NetRejCause_t>(indication.cause);

    le_event_ReportWithRefCounting(factory.events.networkRejection, (void*)indPtr);
}

static void RatChangeHandler
(
    uint32_t instance,
    taf_pa_radio_RatChangeIndication_t indication,
    void* contextPtr
)
{
    auto& factory = Factory::GetInstance();
    if (instance < INSTANCE_MAX_COUNT && indication.rat != factory.cache.rat[instance])
    {
        taf_radio_RatChangeInd_t* indPtr = (taf_radio_RatChangeInd_t*)le_mem_ForceAlloc(
            factory.pools.ratChange);

        indPtr->phoneId = Utility::Convert::InstanceToPhone(instance);
        indPtr->rat = Utility::Convert::Rat(indication.rat);

        le_event_ReportWithRefCounting(factory.events.ratChange, (void*)indPtr);
    }
}

static void VoiceServiceInfoHandler
(
    uint32_t instance,
    taf_pa_radio_VoiceServiceInfoIndication_t indication,
    void* contextPtr
)
{
    auto& factory = Factory::GetInstance();

    taf_radio_NetRegStateInd_t* indPtr = (taf_radio_NetRegStateInd_t*)le_mem_ForceAlloc(
        factory.pools.netRegState);

    indPtr->phoneId = Utility::Convert::InstanceToPhone(instance);
    indPtr->state = Utility::Convert::NetRegState(&indication.info);

    le_event_ReportWithRefCounting(factory.events.netRegState, (void*)indPtr);
}

static void DataServiceStatusHandler
(
    uint32_t instance,
    taf_pa_radio_DataServiceStatusIndication_t indication,
    void* contextPtr
)
{
    if (instance >= INSTANCE_MAX_COUNT)
    {
        LE_ERROR("Invalid instance %d.", instance);
        return;
    }

    auto& factory = Factory::GetInstance();
    factory.cache.dataServiceState[instance] = indication.state;
    taf_radio_NetRegState_t state = TAF_RADIO_NET_REG_STATE_UNKNOWN;
    switch (indication.state)
    {
        case TAF_PA_RADIO_DATA_SERVICE_STATE_IN_SERVICE:
        {
            taf_pa_radio_DataRoamingStatus_t status = TAF_PA_RADIO_DATA_ROAMING_STATUS_UNKNOWN;
            pa_result_t result = taf_pa_radio_GetDataCurrRoamingStatus(instance, &status);
            if (result == 0 && status == TAF_PA_RADIO_DATA_ROAMING_STATUS_ON)
                state = TAF_RADIO_NET_REG_STATE_ROAMING;
            else
                state = TAF_RADIO_NET_REG_STATE_HOME;

            break;
        }
        case TAF_PA_RADIO_DATA_SERVICE_STATE_OUT_OF_SERVICE:
            state = TAF_RADIO_NET_REG_STATE_NONE;
            break;
        default:
            state = TAF_RADIO_NET_REG_STATE_UNKNOWN;
    }

    if (state != factory.cache.packetSwitchedState[instance])
    {
        factory.cache.packetSwitchedState[instance] = state;

        taf_radio_NetRegStateInd_t* indPtr = (taf_radio_NetRegStateInd_t*)le_mem_ForceAlloc(
            factory.pools.netRegState);
        indPtr->phoneId = Utility::Convert::InstanceToPhone(instance);
        indPtr->state = state;
        le_event_ReportWithRefCounting(factory.events.packetSwitchedState, (void*)indPtr);
    }
}

static void DataRoamingStatusHandler
(
    uint32_t instance,
    taf_pa_radio_DataRoamingStatusIndication_t indication,
    void* contextPtr
)
{
    if (instance >= INSTANCE_MAX_COUNT)
    {
        LE_ERROR("Invalid instance %d.", instance);
        return;
    }

    auto& factory = Factory::GetInstance();
    taf_radio_NetRegState_t state = TAF_RADIO_NET_REG_STATE_UNKNOWN;
    if (indication.status == TAF_PA_RADIO_DATA_ROAMING_STATUS_ON)
        state = TAF_RADIO_NET_REG_STATE_ROAMING;
    else if (factory.cache.dataServiceState[instance] ==
        TAF_PA_RADIO_DATA_SERVICE_STATE_IN_SERVICE)
        state = TAF_RADIO_NET_REG_STATE_HOME;
    else
        state = TAF_RADIO_NET_REG_STATE_NONE;

    if (state != factory.cache.packetSwitchedState[instance])
    {
        factory.cache.packetSwitchedState[instance] = state;

        taf_radio_NetRegStateInd_t* indPtr = (taf_radio_NetRegStateInd_t*)le_mem_ForceAlloc(
            factory.pools.netRegState);
        indPtr->phoneId = Utility::Convert::InstanceToPhone(instance);
        indPtr->state = state;
        le_event_ReportWithRefCounting(factory.events.packetSwitchedState, (void*)indPtr);
    }
}

static void SignalStrengthInfoChangeHandler
(
    uint32_t instance,
    taf_pa_radio_SignalStrengthInfoChangeIndication_t indication,
    void* contextPtr
)
{
    auto& factory = Factory::GetInstance();

    if (indication.info.bitmask & TAF_PA_RADIO_BITMASK_RAT_GSM)
    {
        SignalStrengthInfoInd_t* indPtr = (SignalStrengthInfoInd_t*)le_mem_ForceAlloc(
            factory.pools.signalStrengthInfoChange);

        indPtr->phone = Utility::Convert::InstanceToPhone(instance);
        indPtr->rssi = indication.info.gsmInfo.rssi;
        indPtr->rsrp = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        le_event_ReportWithRefCounting(factory.events.gsmSignalStrengthInfoChange, (void*)indPtr);
    }

    if (indication.info.bitmask & TAF_PA_RADIO_BITMASK_RAT_CDMA)
    {
        SignalStrengthInfoInd_t* indPtr = (SignalStrengthInfoInd_t*)le_mem_ForceAlloc(
            factory.pools.signalStrengthInfoChange);

        indPtr->phone = Utility::Convert::InstanceToPhone(instance);
        indPtr->rssi = indication.info.cdmaInfo.ss;
        indPtr->rsrp = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        le_event_ReportWithRefCounting(factory.events.cdmaSignalStrengthInfoChange, (void*)indPtr);
    }

    if (indication.info.bitmask & TAF_PA_RADIO_BITMASK_RAT_UMTS)
    {
        SignalStrengthInfoInd_t* indPtr = (SignalStrengthInfoInd_t*)le_mem_ForceAlloc(
            factory.pools.signalStrengthInfoChange);

        indPtr->phone = Utility::Convert::InstanceToPhone(instance);
        indPtr->rssi = indication.info.umtsInfo.ss;
        indPtr->rsrp = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        le_event_ReportWithRefCounting(factory.events.umtsSignalStrengthInfoChange, (void*)indPtr);
    }

    if (indication.info.bitmask & TAF_PA_RADIO_BITMASK_RAT_TDSCDMA)
    {
        SignalStrengthInfoInd_t* indPtr = (SignalStrengthInfoInd_t*)le_mem_ForceAlloc(
            factory.pools.signalStrengthInfoChange);

        indPtr->phone = Utility::Convert::InstanceToPhone(instance);
        indPtr->rssi = indication.info.tdscdmaInfo.rssi;
        indPtr->rsrp = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        le_event_ReportWithRefCounting(factory.events.tdscdmaSignalStrengthInfoChange,
            (void*)indPtr);
    }

    if (indication.info.bitmask & TAF_PA_RADIO_BITMASK_RAT_LTE)
    {
        SignalStrengthInfoInd_t* indPtr = (SignalStrengthInfoInd_t*)le_mem_ForceAlloc(
            factory.pools.signalStrengthInfoChange);

        indPtr->phone = Utility::Convert::InstanceToPhone(instance);
        indPtr->rssi = indication.info.lteInfo.rssi;
        indPtr->rsrp = indication.info.lteInfo.rsrp;
        le_event_ReportWithRefCounting(factory.events.lteSignalStrengthInfoChange, (void*)indPtr);
    }

    if (indication.info.bitmask & TAF_PA_RADIO_BITMASK_RAT_NR5G)
    {
        SignalStrengthInfoInd_t* indPtr = (SignalStrengthInfoInd_t*)le_mem_ForceAlloc(
            factory.pools.signalStrengthInfoChange);

        indPtr->phone = Utility::Convert::InstanceToPhone(instance);
        indPtr->rssi = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        indPtr->rsrp = indication.info.nr5gInfo.rsrp;
        le_event_ReportWithRefCounting(factory.events.nr5gSignalStrengthInfoChange, (void*)indPtr);
    }
}

static void ImsRegStatusChangeHandler
(
    uint32_t instance,
    taf_pa_radio_ImsRegStatusChangeIndication_t indication,
    void* contextPtr
)
{
    auto& factory = Factory::GetInstance();

    ImsRegStatusInd_t* indPtr = (ImsRegStatusInd_t*)le_mem_ForceAlloc(
        factory.pools.imsRegStatusChange);

    indPtr->phone = Utility::Convert::InstanceToPhone(instance);
    indPtr->status = Utility::Convert::ImsRegistrationStatus(indication.status);

    le_event_ReportWithRefCounting(factory.events.imsRegStatusChange, (void*)indPtr);
}

static void OperatingModeChangeHandler
(
    uint32_t instance,
    taf_pa_radio_OperatingModeChangeIndication_t indication,
    void* contextPtr
)
{
    auto& factory = Factory::GetInstance();
    taf_radio_OpMode_t mode = TAF_RADIO_OP_MODE_ONLINE;
    le_result_t result = Utility::Convert::OperatingMode(indication.mode, &mode);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert operating mode %d.", indication.mode);
        return;
    }

    taf_radio_OpMode_t* modePtr = (taf_radio_OpMode_t*)le_mem_ForceAlloc(
        factory.pools.operatingModeChange);
    *modePtr = mode;
    le_event_ReportWithRefCounting(factory.events.operatingModeChange, (void*)modePtr);
}

static void RatSvcStatusHandler
(
    uint32_t instance,
    taf_pa_radio_RatSvcStatusIndication_t indication,
    void* contextPtr
)
{
    taf_pa_radio_RatServiceStatus_t status = TAF_PA_RADIO_RAT_SERVICE_STATUS_UNKNOWN;

    if (indication.gsmSvcStatusValid && indication.gsmSvcStatus > status)
        status = indication.gsmSvcStatus;

    if (indication.cdmaSvcStatusValid && indication.cdmaSvcStatus > status)
        status = indication.cdmaSvcStatus;

    if (indication.umtsSvcStatusValid && indication.umtsSvcStatus > status)
        status = indication.umtsSvcStatus;

    if (indication.tdscdmaSvcStatusValid && indication.tdscdmaSvcStatus > status)
        status = indication.tdscdmaSvcStatus;

    if (indication.lteSvcStatusValid && indication.lteSvcStatus > status)
        status = indication.lteSvcStatus;

    if (indication.nr5gSvcStatusValid && indication.nr5gSvcStatus > status)
        status = indication.nr5gSvcStatus;

    auto& factory = Factory::GetInstance();

    if (instance < INSTANCE_MAX_COUNT && status != factory.cache.ratSvcState[instance])
    {
        factory.cache.ratSvcState[instance] = status;
        NetStatusInd_t* indPtr = (NetStatusInd_t*)le_mem_ForceAlloc(factory.pools.netStatusChange);
        indPtr->phone = Utility::Convert::InstanceToPhone(instance);
        indPtr->bitmask = TAF_RADIO_NET_STATUS_IND_BIT_MASK_RAT_SVC_STATUS;
        indPtr->reference = factory.cache.netStatusRefs[instance];
        le_event_ReportWithRefCounting(factory.events.netStatusChange, (void*)indPtr);
    }
}

static void ServiceDomainHandler
(
    uint32_t instance,
    taf_pa_radio_ServiceDomainIndication_t indication,
    void* contextPtr
)
{
    auto& factory = Factory::GetInstance();

    if (instance < INSTANCE_MAX_COUNT && indication.domain != factory.cache.svcDomain[instance])
    {
        factory.cache.svcDomain[instance] = indication.domain;
        NetStatusInd_t* indPtr = (NetStatusInd_t*)le_mem_ForceAlloc(factory.pools.netStatusChange);
        indPtr->phone = Utility::Convert::InstanceToPhone(instance);
        indPtr->bitmask = TAF_RADIO_NET_STATUS_IND_BIT_MASK_SVC_DOMAIN;
        indPtr->reference = factory.cache.netStatusRefs[instance];
        le_event_ReportWithRefCounting(factory.events.netStatusChange, (void*)indPtr);
    }
}

static void LteCsCapabilityHandler
(
    uint32_t instance,
    taf_pa_radio_LteCsCapabilityIndication_t indication,
    void* contextPtr
)
{
    auto& factory = Factory::GetInstance();

    if (instance < INSTANCE_MAX_COUNT)
    {
        NetStatusInd_t* indPtr = (NetStatusInd_t*)le_mem_ForceAlloc(factory.pools.netStatusChange);
        indPtr->phone = Utility::Convert::InstanceToPhone(instance);
        indPtr->bitmask = TAF_RADIO_NET_STATUS_IND_BIT_MASK_LTE_CS_CAP;
        indPtr->reference = factory.cache.netStatusRefs[instance];
        le_event_ReportWithRefCounting(factory.events.netStatusChange, (void*)indPtr);
    }
}

static void ImsServiceInfoHandler
(
    uint32_t instance,
    taf_pa_radio_ImsServiceInfoIndication_t indication,
    void* contextPtr
)
{
    auto& factory = Factory::GetInstance();

    if (instance < INSTANCE_MAX_COUNT && (indication.voipServiceStatusValid ||
        indication.smsServiceStatusValid))
    {
        ImsStatusInd_t* indPtr = (ImsStatusInd_t*)le_mem_ForceAlloc(factory.pools.imsStatusChange);
        indPtr->phone = Utility::Convert::InstanceToPhone(instance);
        indPtr->bitmask = TAF_RADIO_IMS_IND_BIT_MASK_SVC_INFO;
        indPtr->reference = factory.cache.imsRefs[instance];
        le_event_ReportWithRefCounting(factory.events.imsStatusChange, (void*)indPtr);
    }
}

static void ImsPdpErrorHandler
(
    uint32_t instance,
    taf_pa_radio_ImsPdpErrorIndication_t indication,
    void* contextPtr
)
{
    auto& factory = Factory::GetInstance();

    if (instance < INSTANCE_MAX_COUNT && indication.failureErrorCodeValid)
    {
        ImsStatusInd_t* indPtr = (ImsStatusInd_t*)le_mem_ForceAlloc(factory.pools.imsStatusChange);
        indPtr->phone = Utility::Convert::InstanceToPhone(instance);
        indPtr->bitmask = TAF_RADIO_IMS_IND_BIT_MASK_PDP_ERROR;
        indPtr->reference = factory.cache.imsRefs[instance];
        le_event_ReportWithRefCounting(factory.events.imsStatusChange, (void*)indPtr);
    }
}

static void CellInfoChangeHandler
(
    uint32_t instance,
    taf_pa_radio_CellInfoChangeIndication_t indication,
    void* contextPtr
)
{
    auto& factory = Factory::GetInstance();

    taf_radio_CellInfoStatus_t status = TAF_RADIO_CELL_SERVING_CHANGED;
    le_result_t result = Utility::Convert::CellInfoStatus(indication.cellRole, &status);
    if (instance < INSTANCE_MAX_COUNT && indication.cellRoleValid && result == LE_OK)
    {
        CellInfoInd_t* indPtr = (CellInfoInd_t*)le_mem_ForceAlloc(factory.pools.cellInfoChange);
        indPtr->phone = Utility::Convert::InstanceToPhone(instance);
        indPtr->status = status;
        le_event_ReportWithRefCounting(factory.events.cellInfoChange, (void*)indPtr);
    }
}

static void NrIconChangeHandler
(
    uint32_t instance,
    taf_pa_radio_NrIconChangeIndication_t indication,
    void* contextPtr
)
{
    auto& factory = Factory::GetInstance();
    NrIconInd_t* indPtr = (NrIconInd_t*)le_mem_ForceAlloc(factory.pools.nrIconChange);
    indPtr->phone = Utility::Convert::InstanceToPhone(instance);
    indPtr->icon = Utility::Convert::NrIcon(indication.icon);
    le_event_ReportWithRefCounting(factory.events.nrIconChange, (void*)indPtr);
}

static void LteCphyCaHandler
(
    uint32_t instance,
    taf_pa_radio_LteCphyCaIndication_t indication,
    void* contextPtr
)
{
    uint32_t count = 0;
    taf_radio_CAStatus_t status = TAF_RADIO_CA_STATUS_DEACTIVATED;

    if (indication.pcellInfoValid)
        count++;

    if (indication.scellInfoValid)
    {
        for (uint32_t i = 0; i < indication.scellInfoCount &&
            i < TAF_PA_RADIO_LTE_CPHY_SCELL_INFO_MAX_COUNT; i++)
        {
            if (indication.scellInfo[i].scellState ==
                TAF_PA_RADIO_LTE_CPHY_SCELL_STATE_CONFIGURED_ACTIVATED)
            {
                status = TAF_RADIO_CA_STATUS_ACTIVATED;
                count++;
            }
        }
    }

    auto& factory = Factory::GetInstance();
    if (instance < INSTANCE_MAX_COUNT)
    {
        CAInfo_t* infoPtr = (CAInfo_t*)le_ref_Lookup(factory.maps.caInfo,
            factory.cache.caInfoRefs[instance]);
        if (count != infoPtr->cellCount || infoPtr->status != status)
        {
            infoPtr->status = status;
            infoPtr->cellCount = count;

            CAInfoInd_t* indPtr = (CAInfoInd_t*)le_mem_ForceAlloc(factory.pools.caInfoChange);
            indPtr->phone = Utility::Convert::InstanceToPhone(instance);
            indPtr->reference = factory.cache.caInfoRefs[instance];
            le_event_ReportWithRefCounting(factory.events.caInfoChange, (void*)indPtr);
        }
    }
}

le_result_t Utility::Convert::Result
(
    pa_result_t result
)
{
    switch (result)
    {
        case 0:
            return LE_OK;
        case -EFAULT:
            return LE_FAULT;
        case -ETIMEDOUT:
            return LE_TIMEOUT;
        case -ERANGE:
            return LE_OUT_OF_RANGE;
        case -EINVAL:
            return LE_BAD_PARAMETER;
        case -ENOTSUP:
            return LE_UNSUPPORTED;
        case -ENOSYS:
            return LE_NOT_IMPLEMENTED;
        default:
            LE_INFO("Unknown result %d.", result);
    }

    return LE_FAULT;
}

le_result_t Utility::Convert::StringToU16
(
    const char* stringPtr,
    uint16_t* valuePtr
)
{
    if (stringPtr == nullptr)
    {
        LE_ERROR("stringPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (valuePtr == nullptr)
    {
        LE_ERROR("valuePtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    string str(stringPtr);
    if (str.empty())
    {
        LE_ERROR("str is empty.");
        return LE_BAD_PARAMETER;
    }

    for (unsigned char c : str)
    {
        if (!std::isdigit(c))
        {
            LE_ERROR("str contains non-digit %d.", static_cast<int>(c));
            return LE_BAD_PARAMETER;
        }
    }

    long value = stoi(stringPtr, nullptr, 0);
    if (value < 0 || value > 999)
    {
        LE_ERROR("value %ld is out of range [0, 999]", value);
        return LE_OUT_OF_RANGE;
    }

    *valuePtr = static_cast<uint16_t>(value);

    return LE_OK;
}

le_result_t Utility::Convert::U16ToString
(
    uint16_t value,
    char* stringPtr,
    size_t length,
    bool padding
)
{
    if (value > 999)
    {
        LE_ERROR("value %d is out of range [0, 999]", value);
        return LE_OUT_OF_RANGE;
    }

    if (stringPtr == nullptr)
    {
        LE_ERROR("stringPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    string s = to_string(value);
    size_t offset = 0;
    if (padding && value < 10)
    {
        *stringPtr = '0';
        offset = 1;
    }

    le_utf8_Copy(stringPtr + offset, s.c_str(), length, nullptr);

    return LE_OK;
}

uint32_t Utility::Convert::PhoneToInstance
(
    uint8_t phone
)
{
    switch (phone)
    {
        case 1:
            return 0;
        case 2:
            return 1;
        default:
            LE_INFO("Invalid phone %d.", phone);
    }

    return INSTANCE_MAX_COUNT;
}

uint8_t Utility::Convert::InstanceToPhone
(
    uint32_t instance
)
{
    switch (instance)
    {
        case 0:
            return 1;
        case 1:
            return 2;
        default:
            LE_INFO("Invalid instance %d.", instance);
    }

    return 0;
}

le_result_t Utility::Convert::ReferenceToInstance
(
     taf_radio_NetStatusRef_t reference,
     uint32_t* instancePtr
)
{
    auto& factory = Factory::GetInstance();

    for (uint32_t i = 0; i < INSTANCE_MAX_COUNT; i++)
    {
        if (reference == factory.cache.netStatusRefs[i])
        {
            *instancePtr = i;
            return LE_OK;
        }
    }

    return LE_NOT_FOUND;
}

le_result_t Utility::Convert::ReferenceToInstance
(
     taf_radio_ImsRef_t reference,
     uint32_t* instancePtr
)
{
    auto& factory = Factory::GetInstance();

    for (uint32_t i = 0; i < INSTANCE_MAX_COUNT; i++)
    {
        if (reference == factory.cache.imsRefs[i])
        {
            *instancePtr = i;
            return LE_OK;
        }
    }

    return LE_NOT_FOUND;
}


taf_pa_radio_RatBitMask_t Utility::Convert::Rat
(
    taf_radio_RatBitMask_t bitmask
)
{
    if (bitmask & TAF_RADIO_RAT_BIT_MASK_ALL)
        return TAF_PA_RADIO_BITMASK_RAT_GSM | TAF_PA_RADIO_BITMASK_RAT_CDMA |
            TAF_PA_RADIO_BITMASK_RAT_UMTS | TAF_PA_RADIO_BITMASK_RAT_TDSCDMA |
            TAF_PA_RADIO_BITMASK_RAT_LTE | TAF_PA_RADIO_BITMASK_RAT_NR5G;

    taf_pa_radio_RatBitMask_t result = 0x0;

    if (bitmask & TAF_RADIO_RAT_BIT_MASK_GSM)
        result |= TAF_PA_RADIO_BITMASK_RAT_GSM;

    if (bitmask & TAF_RADIO_RAT_BIT_MASK_CDMA)
        result |= TAF_PA_RADIO_BITMASK_RAT_CDMA;

    if (bitmask & TAF_RADIO_RAT_BIT_MASK_UMTS)
        result |= TAF_PA_RADIO_BITMASK_RAT_UMTS;

    if (bitmask & TAF_RADIO_RAT_BIT_MASK_TDSCDMA)
        result |= TAF_PA_RADIO_BITMASK_RAT_TDSCDMA;

    if (bitmask & TAF_RADIO_RAT_BIT_MASK_LTE)
        result |= TAF_PA_RADIO_BITMASK_RAT_LTE;

    if (bitmask & TAF_RADIO_RAT_BIT_MASK_NR5G)
        result |= TAF_PA_RADIO_BITMASK_RAT_NR5G;

    return result;
}

taf_radio_RatBitMask_t Utility::Convert::Rat
(
    taf_pa_radio_RatBitMask_t bitmask
)
{
    taf_radio_RatBitMask_t result = 0x0;

    if (bitmask & TAF_PA_RADIO_BITMASK_RAT_GSM)
        result |= TAF_RADIO_RAT_BIT_MASK_GSM;

    if (bitmask & TAF_PA_RADIO_BITMASK_RAT_CDMA)
        result |= TAF_RADIO_RAT_BIT_MASK_CDMA;

    if (bitmask & TAF_PA_RADIO_BITMASK_RAT_UMTS)
        result |= TAF_RADIO_RAT_BIT_MASK_UMTS;

    if (bitmask & TAF_PA_RADIO_BITMASK_RAT_TDSCDMA)
        result |= TAF_RADIO_RAT_BIT_MASK_TDSCDMA;

    if (bitmask & TAF_PA_RADIO_BITMASK_RAT_LTE)
        result |= TAF_RADIO_RAT_BIT_MASK_LTE;

    if (bitmask & TAF_PA_RADIO_BITMASK_RAT_NR5G)
        result |= TAF_RADIO_RAT_BIT_MASK_NR5G;

    return result;
}

taf_radio_Rat_t Utility::Convert::Rat
(
    taf_pa_radio_Rat_t rat
)
{
    switch (rat)
    {
        case TAF_PA_RADIO_RAT_GSM:
            return TAF_RADIO_RAT_GSM;
        case TAF_PA_RADIO_RAT_CDMA:
            return TAF_RADIO_RAT_CDMA;
        case TAF_PA_RADIO_RAT_UMTS:
            return TAF_RADIO_RAT_UMTS;
        case TAF_PA_RADIO_RAT_TDSCDMA:
            return TAF_RADIO_RAT_TDSCDMA;
        case TAF_PA_RADIO_RAT_LTE:
            return TAF_RADIO_RAT_LTE;
        case TAF_PA_RADIO_RAT_NR5G:
            return TAF_RADIO_RAT_NR5G;
        default:
            LE_INFO("Unknown RAT %d.", rat);
    }

    return TAF_RADIO_RAT_UNKNOWN;
}

taf_radio_ServiceDomainState_t Utility::Convert::ServiceDomain
(
    taf_pa_radio_ServiceDomain_t domain
)
{
    switch (domain)
    {
        case TAF_PA_RADIO_SERVICE_DOMAIN_NO_SERVICE:
            return TAF_RADIO_SERVICE_DOMAIN_STATE_NO_SVC;
        case TAF_PA_RADIO_SERVICE_DOMAIN_CS_ONLY:
            return TAF_RADIO_SERVICE_DOMAIN_STATE_CS_ONLY;
        case TAF_PA_RADIO_SERVICE_DOMAIN_PS_ONLY:
            return TAF_RADIO_SERVICE_DOMAIN_STATE_PS_ONLY;
        case TAF_PA_RADIO_SERVICE_DOMAIN_CS_AND_PS:
            return TAF_RADIO_SERVICE_DOMAIN_STATE_CS_AND_PS;
        case TAF_PA_RADIO_SERVICE_DOMAIN_CAMPED:
            return TAF_RADIO_SERVICE_DOMAIN_STATE_CAMPED;
        default:
            LE_INFO("Unknown service domain %d.", domain);
    }

    return TAF_RADIO_SERVICE_DOMAIN_STATE_UNKNOWN;
}

taf_radio_ServiceDomainState_t Utility::Convert::ServiceDomain
(
    taf_pa_radio_ServiceDomainBitMask_t bitmask
)
{
    if (bitmask & TAF_PA_RADIO_BITMASK_SERVICE_DOMAIN_CS_ONLY)
        return TAF_RADIO_SERVICE_DOMAIN_STATE_CS_ONLY;

    if (bitmask & TAF_PA_RADIO_BITMASK_SERVICE_DOMAIN_PS_ONLY)
        return TAF_RADIO_SERVICE_DOMAIN_STATE_PS_ONLY;

    if (bitmask & TAF_PA_RADIO_BITMASK_SERVICE_DOMAIN_CS_AND_PS)
        return TAF_RADIO_SERVICE_DOMAIN_STATE_CS_AND_PS;

    return TAF_RADIO_SERVICE_DOMAIN_STATE_UNKNOWN;
}

taf_pa_radio_ServiceDomainBitMask_t Utility::Convert::ServiceDomain
(
    taf_radio_ServiceDomainState_t domain
)
{
    switch (domain)
    {
        case TAF_RADIO_SERVICE_DOMAIN_STATE_CS_ONLY:
            return TAF_PA_RADIO_BITMASK_SERVICE_DOMAIN_CS_ONLY;
        case TAF_RADIO_SERVICE_DOMAIN_STATE_PS_ONLY:
            return TAF_PA_RADIO_BITMASK_SERVICE_DOMAIN_PS_ONLY;
        case TAF_RADIO_SERVICE_DOMAIN_STATE_CS_AND_PS:
            return TAF_PA_RADIO_BITMASK_SERVICE_DOMAIN_CS_AND_PS;
        default:
            LE_ERROR("Unknown service domain %d.", domain);
    }

    return 0;
}

taf_radio_NetRegState_t Utility::Convert::NetRegState
(
    taf_pa_radio_VoiceServiceInfo_t* infoPtr
)
{
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return TAF_RADIO_NET_REG_STATE_UNKNOWN;
    }

    switch (infoPtr->regState)
    {
        case TAF_PA_RADIO_REGISTRATION_STATE_NOT_REGISTERED:
            if (infoPtr->emergModeValid && infoPtr->emergMode == TAF_PA_RADIO_EMERGENCY_MODE_ON)
                return TAF_RADIO_NET_REG_STATE_NONE_AND_EMERGENCY_AVAILABLE;
            else
                return TAF_RADIO_NET_REG_STATE_NONE;
        case TAF_PA_RADIO_REGISTRATION_STATE_REGISTERED:
            if (infoPtr->roamingIndicatorValid && infoPtr->roamingIndicator ==
                TAF_PA_RADIO_ROAMING_INDICATOR_ON)
                return TAF_RADIO_NET_REG_STATE_ROAMING;
            else
                return TAF_RADIO_NET_REG_STATE_HOME;
        case TAF_PA_RADIO_REGISTRATION_STATE_NOT_REGISTERED_SEARCHING:
            if (infoPtr->emergModeValid && infoPtr->emergMode == TAF_PA_RADIO_EMERGENCY_MODE_ON)
                return TAF_RADIO_NET_REG_STATE_SEARCHING_AND_EMERGENCY_AVAILABLE;
            else
                return TAF_RADIO_NET_REG_STATE_SEARCHING;
        case TAF_PA_RADIO_REGISTRATION_STATE_DENIED:
            if (infoPtr->emergModeValid && infoPtr->emergMode == TAF_PA_RADIO_EMERGENCY_MODE_ON)
                return TAF_RADIO_NET_REG_STATE_DENIED_AND_EMERGENCY_AVAILABLE;
            else
                return TAF_RADIO_NET_REG_STATE_DENIED;
        case TAF_PA_RADIO_REGISTRATION_STATE_UNKNOWN:
            if (infoPtr->emergModeValid && infoPtr->emergMode == TAF_PA_RADIO_EMERGENCY_MODE_ON)
                return TAF_RADIO_NET_REG_STATE_UNKNOWN_AND_EMERGENCY_AVAILABLE;
            break;
        default:
            LE_ERROR("Unknown registration state %d.", infoPtr->regState);
    }

    return TAF_RADIO_NET_REG_STATE_UNKNOWN;
}

taf_radio_NetRegState_t Utility::Convert::NetRegState
(
    taf_pa_radio_DataServiceState_t state,
    taf_pa_radio_DataRoamingStatus_t status
)
{
    switch (state)
    {
        case TAF_PA_RADIO_DATA_SERVICE_STATE_IN_SERVICE:
            if (status == TAF_PA_RADIO_DATA_ROAMING_STATUS_ON)
                return TAF_RADIO_NET_REG_STATE_ROAMING;
            else
                return TAF_RADIO_NET_REG_STATE_HOME;
        case TAF_PA_RADIO_DATA_SERVICE_STATE_OUT_OF_SERVICE:
            return TAF_RADIO_NET_REG_STATE_NONE;
        default:
            LE_INFO("Unknown data service state %d.", state);
    }

    return TAF_RADIO_NET_REG_STATE_UNKNOWN;
}

uint32_t Utility::Convert::SignalStrengthLevel
(
    taf_pa_radio_SignalStrengthLevel_t level
)
{
    switch (level)
    {
        case TAF_PA_RADIO_SIGNAL_STRENGTH_LEVEL_1:
            return 1;
        case TAF_PA_RADIO_SIGNAL_STRENGTH_LEVEL_2:
            return 2;
        case TAF_PA_RADIO_SIGNAL_STRENGTH_LEVEL_3:
            return 3;
        case TAF_PA_RADIO_SIGNAL_STRENGTH_LEVEL_4:
            return 4;
        case TAF_PA_RADIO_SIGNAL_STRENGTH_LEVEL_5:
            return 5;
        default:
            LE_DEBUG("Unknown signal strength level %d.", level);
    }

    return 0;
}

void Utility::Convert::SignalStrengthIndConfig
(
    uint32_t instance,
    taf_radio_SigType_t metric,
    taf_pa_radio_SignalStrengthIndConfig_t* configPtr
)
{
    if (configPtr == nullptr)
    {
        LE_ERROR("configPtr is nullptr.");
        return;
    }

    if (instance >= INSTANCE_MAX_COUNT)
    {
        LE_ERROR("Invalid instance %d.", instance);
        return;
    }

    switch (metric)
    {
        case TAF_RADIO_SIG_TYPE_GSM_RSSI:
            configPtr->rat = TAF_PA_RADIO_RAT_GSM;
            configPtr->metric = TAF_PA_RADIO_SIGNAL_METRIC_RSSI;
            break;
        case TAF_RADIO_SIG_TYPE_CDMA_RSSI:
            configPtr->rat = TAF_PA_RADIO_RAT_CDMA;
            configPtr->metric = TAF_PA_RADIO_SIGNAL_METRIC_RSSI;
            break;
        case TAF_RADIO_SIG_TYPE_UMTS_RSSI:
            configPtr->rat = TAF_PA_RADIO_RAT_UMTS;
            configPtr->metric = TAF_PA_RADIO_SIGNAL_METRIC_RSSI;
            break;
        case TAF_RADIO_SIG_TYPE_TDSCDMA_RSSI:
            configPtr->rat = TAF_PA_RADIO_RAT_TDSCDMA;
            configPtr->metric = TAF_PA_RADIO_SIGNAL_METRIC_RSSI;
            break;
        case TAF_RADIO_SIG_TYPE_LTE_RSSI:
            configPtr->rat = TAF_PA_RADIO_RAT_LTE;
            configPtr->metric = TAF_PA_RADIO_SIGNAL_METRIC_RSSI;
            break;
        case TAF_RADIO_SIG_TYPE_LTE_RSRP:
            configPtr->rat = TAF_PA_RADIO_RAT_LTE;
            configPtr->metric = TAF_PA_RADIO_SIGNAL_METRIC_RSRP;
            break;
        case TAF_RADIO_SIG_TYPE_NR5G_RSRP:
            configPtr->rat = TAF_PA_RADIO_RAT_NR5G;
            configPtr->metric = TAF_PA_RADIO_SIGNAL_METRIC_RSRP;
            break;
        default:
            LE_ERROR("Invalid metric %d.", metric);
            return;
    }

    auto& factory = Factory::GetInstance();
    configPtr->hysteresisTimeValid = 1;
    configPtr->hysteresisTime = factory.cache.hysteresisConfig[instance].time;

    auto it = factory.cache.hysteresisConfig[instance].delta.find(metric);
    if (it != factory.cache.hysteresisConfig[instance].delta.end())
	{
        configPtr->hysteresisDeltaValid = 1;
        configPtr->hysteresisDelta = it->second;
    }
}

taf_radio_BandBitMask_t Utility::Convert::ToBand
(
    taf_pa_radio_BandBitMask_t bitmask
)
{
    taf_radio_BandBitMask_t result = 0x0;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_GSM_BAND_450)
        result |= TAF_RADIO_BAND_BIT_MASK_GSM_BAND_450;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_GSM_BAND_480)
        result |= TAF_RADIO_BAND_BIT_MASK_GSM_BAND_480;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_GSM_BAND_750)
        result |= TAF_RADIO_BAND_BIT_MASK_GSM_BAND_750;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_GSM_BAND_850)
        result |= TAF_RADIO_BAND_BIT_MASK_GSM_BAND_850;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_CLASS_E_GSM_900_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_CLASS_E_GSM_900_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_CLASS_P_GSM_900_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_CLASS_P_GSM_900_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_GSM_BAND_RAILWAYS_900_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_GSM_BAND_RAILWAYS_900_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_CLASS_GSM_DCS_1800_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_CLASS_GSM_DCS_1800_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_GSM_PCS_1900_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_GSM_PCS_1900_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_WCDMA_EU_J_CH_IMT_2100_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_J_CH_IMT_2100_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_WCDMA_US_PCS_1900_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_WCDMA_US_PCS_1900_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_WCDMA_EU_CH_DCS_1800_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_EU_CH_DCS_1800_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_WCDMA_US_1700_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_WCDMA_US_1700_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_WCDMA_US_850_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_WCDMA_US_850_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_WCDMA_JAPAN_800_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_WCDMA_JAPAN_800_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_WCDMA_EU_2600_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_2600_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_WCDMA_EU_J_900_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_J_900_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_WCDMA_EU_J_1700_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_J_1700_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_WCDMA_JAPAN_1500_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_WCDMA_JAPAN_1500_BAND;

    if (bitmask & TAF_PA_RADIO_BITMASK_BAND_WCDMA_JAPAN_850_BAND)
        result |= TAF_RADIO_BAND_BIT_MASK_WCDMA_JAPAN_850_BAND;

    return result;
}

taf_pa_radio_BandBitMask_t Utility::Convert::ToPaBand
(
    taf_radio_BandBitMask_t bitmask
)
{
    taf_pa_radio_BandBitMask_t result = 0x0;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_GSM_BAND_450)
        result |= TAF_PA_RADIO_BITMASK_BAND_GSM_BAND_450;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_GSM_BAND_480)
        result |= TAF_PA_RADIO_BITMASK_BAND_GSM_BAND_480;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_GSM_BAND_750)
        result |= TAF_PA_RADIO_BITMASK_BAND_GSM_BAND_750;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_GSM_BAND_850)
        result |= TAF_PA_RADIO_BITMASK_BAND_GSM_BAND_850;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_CLASS_E_GSM_900_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_CLASS_E_GSM_900_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_CLASS_P_GSM_900_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_CLASS_P_GSM_900_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_GSM_BAND_RAILWAYS_900_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_GSM_BAND_RAILWAYS_900_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_CLASS_GSM_DCS_1800_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_CLASS_GSM_DCS_1800_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_GSM_PCS_1900_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_GSM_PCS_1900_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_J_CH_IMT_2100_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_WCDMA_EU_J_CH_IMT_2100_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_WCDMA_US_PCS_1900_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_WCDMA_US_PCS_1900_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_EU_CH_DCS_1800_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_WCDMA_EU_CH_DCS_1800_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_WCDMA_US_1700_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_WCDMA_US_1700_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_WCDMA_US_850_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_WCDMA_US_850_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_WCDMA_JAPAN_800_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_WCDMA_JAPAN_800_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_2600_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_WCDMA_EU_2600_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_J_900_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_WCDMA_EU_J_900_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_J_1700_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_WCDMA_EU_J_1700_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_WCDMA_JAPAN_1500_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_WCDMA_JAPAN_1500_BAND;

    if (bitmask & TAF_RADIO_BAND_BIT_MASK_WCDMA_JAPAN_850_BAND)
        result |= TAF_PA_RADIO_BITMASK_BAND_WCDMA_JAPAN_850_BAND;

    return result;
}

taf_radio_RFBandWidth_t Utility::Convert::Bandwidth
(
    taf_pa_radio_Bandwidth_t bandwidth
)
{
    switch (bandwidth)
    {
        case TAF_PA_RADIO_BANDWIDTH_GSM_BW_NRB_2:
            return TAF_RADIO_RF_BANDWIDTH_GSM_BW_0_2;
        case TAF_PA_RADIO_BANDWIDTH_WCDMA_BW_NRB_5:
            return TAF_RADIO_RF_BANDWIDTH_WCDMA_BW_5;
        case TAF_PA_RADIO_BANDWIDTH_WCDMA_BW_NRB_10:
            return TAF_RADIO_RF_BANDWIDTH_WCDMA_BW_10;
        case TAF_PA_RADIO_BANDWIDTH_TDSCDMA_BW_NRB_2:
            return TAF_RADIO_RF_BANDWIDTH_TDSCDMA_BW_1_6;
        case TAF_PA_RADIO_BANDWIDTH_LTE_BW_NRB_6:
            return TAF_RADIO_RF_BANDWIDTH_LTE_BW_1_4;
        case TAF_PA_RADIO_BANDWIDTH_LTE_BW_NRB_15:
            return TAF_RADIO_RF_BANDWIDTH_LTE_BW_3;
        case TAF_PA_RADIO_BANDWIDTH_LTE_BW_NRB_25:
            return TAF_RADIO_RF_BANDWIDTH_LTE_BW_5;
        case TAF_PA_RADIO_BANDWIDTH_LTE_BW_NRB_50:
            return TAF_RADIO_RF_BANDWIDTH_LTE_BW_10;
        case TAF_PA_RADIO_BANDWIDTH_LTE_BW_NRB_75:
            return TAF_RADIO_RF_BANDWIDTH_LTE_BW_15;
        case TAF_PA_RADIO_BANDWIDTH_LTE_BW_NRB_100:
            return TAF_RADIO_RF_BANDWIDTH_LTE_BW_20;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_5:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_5;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_10:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_10;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_15:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_15;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_20:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_20;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_25:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_25;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_30:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_30;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_40:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_40;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_50:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_50;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_60:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_60;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_70:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_70;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_80:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_80;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_90:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_90;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_100:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_100;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_200:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_200;
        case TAF_PA_RADIO_BANDWIDTH_NR5G_BW_NRB_400:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_400;
        default:
            LE_ERROR("Unknown bandwidth %d.", bandwidth);
    }

    return TAF_RADIO_RF_BANDWIDTH_INVALID;
}

taf_radio_ImsRegStatus_t Utility::Convert::ImsRegistrationStatus
(
     taf_pa_radio_ImsRegistrationStatus_t status
)
{
    switch (status)
    {
        case TAF_PA_RADIO_IMS_REGISTRATION_STATUS_NOT_REGISTERED:
            return TAF_RADIO_IMS_REG_STATUS_NOT_REGISTERED;
        case TAF_PA_RADIO_IMS_REGISTRATION_STATUS_REGISTRERING:
            return TAF_RADIO_IMS_REG_STATUS_REGISTRERING;
        case TAF_PA_RADIO_IMS_REGISTRATION_STATUS_REGISTERED:
            return TAF_RADIO_IMS_REG_STATUS_REGISTERED;
        case TAF_PA_RADIO_IMS_REGISTRATION_STATUS_LIMITED_REGISTERED:
            return TAF_RADIO_IMS_REG_STATUS_LIMITED_REGISTERED;
        default:
            LE_INFO("Unknown IMS registration status %d.", status);
    }

    return TAF_RADIO_IMS_REG_STATUS_UNKNOWN ;
}

le_result_t Utility::Convert::OperatingMode
(
    taf_pa_radio_OperatingMode_t mode,
    taf_radio_OpMode_t* modePtr
)
{
    switch (mode)
    {
        case TAF_PA_RADIO_OPERATING_MODE_ONLINE:
            *modePtr = TAF_RADIO_OP_MODE_ONLINE;
            return LE_OK;
        case TAF_PA_RADIO_OPERATING_MODE_LOW_POWER:
            *modePtr = TAF_RADIO_OP_MODE_AIRPLANE;
            return LE_OK;
        case TAF_PA_RADIO_OPERATING_MODE_FACTORY_TEST_MODE:
            *modePtr = TAF_RADIO_OP_MODE_FACTORY_TEST;
            return LE_OK;
        case TAF_PA_RADIO_OPERATING_MODE_OFFLINE:
            *modePtr = TAF_RADIO_OP_MODE_OFFLINE;
            return LE_OK;
        case TAF_PA_RADIO_OPERATING_MODE_RESETTING:
            *modePtr = TAF_RADIO_OP_MODE_RESETTING;
            return LE_OK;
        case TAF_PA_RADIO_OPERATING_MODE_SHUTTING_DOWN:
            *modePtr = TAF_RADIO_OP_MODE_SHUTTING_DOWN;
            return LE_OK;
        case TAF_PA_RADIO_OPERATING_MODE_PERSISTENT_LOW_POWER:
            *modePtr = TAF_RADIO_OP_MODE_PERSISTENT_LOW_POWER;
            return LE_OK;
        default:
            LE_ERROR("Unknown operating mode %d.", mode);
            return LE_BAD_PARAMETER;
    }
}

taf_pa_radio_OperatingMode_t Utility::Convert::OperatingMode
(
    taf_radio_OpMode_t mode
)
{
    switch (mode)
    {
        case TAF_RADIO_OP_MODE_ONLINE :
            return TAF_PA_RADIO_OPERATING_MODE_ONLINE;
        case TAF_RADIO_OP_MODE_AIRPLANE:
            return TAF_PA_RADIO_OPERATING_MODE_LOW_POWER;
        case TAF_RADIO_OP_MODE_FACTORY_TEST:
            return TAF_PA_RADIO_OPERATING_MODE_FACTORY_TEST_MODE;
        case TAF_RADIO_OP_MODE_OFFLINE:
            return TAF_PA_RADIO_OPERATING_MODE_OFFLINE;
        case TAF_RADIO_OP_MODE_RESETTING:
            return TAF_PA_RADIO_OPERATING_MODE_RESETTING;
        case TAF_RADIO_OP_MODE_SHUTTING_DOWN:
            return TAF_PA_RADIO_OPERATING_MODE_SHUTTING_DOWN;
        case TAF_RADIO_OP_MODE_PERSISTENT_LOW_POWER:
            return TAF_PA_RADIO_OPERATING_MODE_PERSISTENT_LOW_POWER;
        default:
            LE_ERROR("Unknown operating mode %d.", mode);
    }

    return TAF_PA_RADIO_OPERATING_MODE_UNKNOWN;
}

taf_radio_CsCap_t Utility::Convert::LteCsCapability
(
    taf_pa_radio_LteCsCapability_t capability
)
{
    switch (capability)
    {
        case TAF_PA_RADIO_LTE_CS_CAPABILITY_FULL_SERVICE:
            return TAF_RADIO_CS_CAP_FULL_SERVICE;
        case TAF_PA_RADIO_LTE_CS_CAPABILITY_CSFB_NOT_PREFERRED:
            return TAF_RADIO_CS_CAP_CSFB_NOT_PREFERRED;
        case TAF_PA_RADIO_LTE_CS_CAPABILITY_SMS_ONLY:
            return TAF_RADIO_CS_CAP_SMS_ONLY;
        case TAF_PA_RADIO_LTE_CS_CAPABILITY_LIMITED:
            return TAF_RADIO_CS_CAP_LIMITED;
        case TAF_PA_RADIO_LTE_CS_CAPABILITY_BARRED:
            return TAF_RADIO_CS_CAP_BARRED;
        default:
            LE_INFO("Unknown LTE CS capability %d.", capability);
    }

    return TAF_RADIO_CS_CAP_UNKNOWN;
}

le_result_t Utility::Convert::ImsService
(
    taf_radio_ImsSvcType_t service,
    taf_pa_radio_ImsService_t* servicePtr
)
{
    switch (service)
    {
        case TAF_RADIO_IMS_SVC_TYPE_IMS_REG:
            *servicePtr = TAF_PA_RADIO_IMS_SERVICE_REGISTRATION;
            return LE_OK;
        case TAF_RADIO_IMS_SVC_TYPE_SMS:
            *servicePtr = TAF_PA_RADIO_IMS_SERVICE_SMS;
            return LE_OK;
        case TAF_RADIO_IMS_SVC_TYPE_VOIP:
            *servicePtr = TAF_PA_RADIO_IMS_SERVICE_VOIP;
            return LE_OK;
        case TAF_RADIO_IMS_SVC_TYPE_RTT:
            *servicePtr = TAF_PA_RADIO_IMS_SERVICE_RTT;
            return LE_OK;
        case TAF_RADIO_IMS_SVC_TYPE_VONR:
            *servicePtr = TAF_PA_RADIO_IMS_SERVICE_VONR;
            return LE_OK;
        default:
            LE_ERROR("Unknown IMS service %d.", service);
    }

    return LE_BAD_PARAMETER;
}

taf_radio_ImsSvcStatus_t Utility::Convert::ImsServiceStatus
(
    taf_pa_radio_ImsServiceStatus_t status
)
{
    switch (status)
    {
        case TAF_PA_RADIO_IMS_SERVICE_STATUS_NO_SERVICE:
            return TAF_RADIO_IMS_SVC_STATUS_UNAVAILABLE;
        case TAF_PA_RADIO_IMS_SERVICE_STATUS_LIMITED_SERVICE:
            return TAF_RADIO_IMS_SVC_STATUS_LIMITED;
        case TAF_PA_RADIO_IMS_SERVICE_STATUS_FULL_SERVICE:
            return TAF_RADIO_IMS_SVC_STATUS_FULL_SERVICE;
        default:
            LE_INFO("Unknown IMS service status %d.", status);
    }

    return TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;
}

taf_pa_radio_ImsServiceSettingBitMask_t Utility::Convert::ImsService
(
    taf_radio_ImsSvcType_t service
)
{
    switch (service)
    {
        case TAF_RADIO_IMS_SVC_TYPE_IMS_REG:
            return TAF_PA_RADIO_BITMASK_IMS_SERVICE_SETTING_IMS_SERVICE;
        case TAF_RADIO_IMS_SVC_TYPE_SMS:
            return TAF_PA_RADIO_BITMASK_IMS_SERVICE_SETTING_SMS;
        case TAF_RADIO_IMS_SVC_TYPE_VOIP:
            return TAF_PA_RADIO_BITMASK_IMS_SERVICE_SETTING_VOLTE;
        case TAF_RADIO_IMS_SVC_TYPE_RTT:
            return TAF_PA_RADIO_BITMASK_IMS_SERVICE_SETTING_RTT;
        case TAF_RADIO_IMS_SVC_TYPE_VONR:
            return TAF_PA_RADIO_BITMASK_IMS_SERVICE_SETTING_VONR;
        default:
            LE_ERROR("Unknown IMS service %d.", service);
    }

    return 0x0;
}

taf_radio_PdpError_t Utility::Convert::PdpError
(
    taf_pa_radio_ImsPdpFailureErrorCode_t code
)
{
    switch (code)
    {
        case TAF_PA_RADIO_IMS_PDP_FAILURE_ERROR_CODE_OTHER_FAILURE:
            return TAF_RADIO_PDP_ERROR_GENERIC;
        case TAF_PA_RADIO_IMS_PDP_FAILURE_ERROR_CODE_OPTION_UNSUBSCRIBED:
            return TAF_RADIO_PDP_ERROR_OPTION_UNSUBSCRIBED;
        case TAF_PA_RADIO_IMS_PDP_FAILURE_ERROR_CODE_UNKNOWN_PDP:
            return TAF_RADIO_PDP_ERROR_UNKNOWN_PDP;
        case TAF_PA_RADIO_IMS_PDP_FAILURE_ERROR_CODE_REASON_NOT_SPECIFIED:
            return TAF_RADIO_PDP_ERROR_REASON_NOT_SPECIFIED;
        case TAF_PA_RADIO_IMS_PDP_FAILURE_ERROR_CODE_CONNECTION_BRINGUP_FAILURE:
            return TAF_RADIO_PDP_ERROR_CONNECTION_BRINGUP_FAILURE;
        case TAF_PA_RADIO_IMS_PDP_FAILURE_ERROR_CODE_CONNECTION_IKE_AUTH_FAILURE:
            return TAF_RADIO_PDP_ERROR_CONNECTION_IKE_AUTH_FAILURE;
        case TAF_PA_RADIO_IMS_PDP_FAILURE_ERROR_CODE_USER_AUTH_FAILED:
            return TAF_RADIO_PDP_ERROR_USER_AUTH_FAILURE;
        default:
            LE_INFO("Unknown IMS PDP failure error code %d.", code);
    }

    return TAF_RADIO_PDP_ERROR_UNKNOWN;
}

taf_radio_NREndcAvailability_t Utility::Convert::EndcAvailability
(
    taf_pa_radio_EndcAvailability_t availability
)
{
    switch (availability)
    {
        case TAF_PA_RADIO_ENDC_AVAILABILITY_AVAILABLE:
            return TAF_RADIO_NR_ENDC_AVAILABLE;
        case TAF_PA_RADIO_ENDC_AVAILABILITY_UNAVAILABLE:
            return TAF_RADIO_NR_ENDC_UNAVAILABLE;
        default:
            LE_INFO("Unknown ENDC availability.");
    }

    return TAF_RADIO_NR_ENDC_UNKNOWN;
}

taf_radio_NRDcnrRestriction_t Utility::Convert::DcnrRestriction
(
    taf_pa_radio_DcnrRestriction_t restriction
)
{
    switch (restriction)
    {
        case TAF_PA_RADIO_DCNR_RESTRICTION_RESTRICTED:
            return TAF_RADIO_NR_DCNR_RESTRICTED;
        case TAF_PA_RADIO_DCNR_RESTRICTION_NOT_RESTRICTED:
            return TAF_RADIO_NR_DCNR_UNRESTRICTED;
        default:
            LE_INFO("Unknown DCNR restriction.");
    }

    return TAF_RADIO_NR_DCNR_UNKNOWN;
}

le_result_t Utility::Convert::CellInfoStatus
(
    taf_pa_radio_CellRoleBitMask_t bitmask,
    taf_radio_CellInfoStatus_t* statusPtr
)
{
    if (statusPtr == nullptr)
    {
        LE_ERROR("statusPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    taf_pa_radio_CellRoleBitMask_t all = TAF_PA_RADIO_BITMASK_CELL_ROLE_SERVING |
        TAF_PA_RADIO_BITMASK_CELL_ROLE_NEIGHBOR;
    if ((bitmask & all) == all)
    {
        *statusPtr = TAF_RADIO_CELL_SERVING_AND_NEIGHBOR_CHANGED;
        return LE_OK;
    }

    if (bitmask & TAF_PA_RADIO_BITMASK_CELL_ROLE_SERVING)
    {
        *statusPtr = TAF_RADIO_CELL_SERVING_CHANGED;
        return LE_OK;
    }

    if (bitmask & TAF_PA_RADIO_BITMASK_CELL_ROLE_NEIGHBOR)
    {
        *statusPtr = TAF_RADIO_CELL_NEIGHBOR_CHANGED;
        return LE_OK;
    }

    return LE_BAD_PARAMETER;
}

taf_radio_NrIconType_t Utility::Convert::NrIcon
(
    taf_pa_radio_NrIcon_t icon
)
{
    switch (icon)
    {
        case TAF_PA_RADIO_NR_ICON_BASIC:
        case TAF_PA_RADIO_NR_ICON_UWB:
            return TAF_RADIO_NR_ICON_5G;
        default:
            break;
    }

    return TAF_RADIO_NR_ICON_TYPE_NONE;
}

taf_radio_RatSvcStatus_t Utility::Convert::RatServiceStatus
(
    taf_pa_radio_RatServiceStatus_t status
)
{
    switch (status)
    {
        case TAF_PA_RADIO_RAT_SERVICE_STATUS_NO_SERVICE:
            return TAF_RADIO_RAT_SVC_STATUS_NO_SERVICE;
        case TAF_PA_RADIO_RAT_SERVICE_STATUS_LIMITED:
            return TAF_RADIO_RAT_SVC_STATUS_LIMITED;
        case TAF_PA_RADIO_RAT_SERVICE_STATUS_SERVICE:
            return TAF_RADIO_RAT_SVC_STATUS_SERVICE;
        case TAF_PA_RADIO_RAT_SERVICE_STATUS_LIMITED_REGIONAL:
            return TAF_RADIO_RAT_SVC_STATUS_LIMITED_REGIONAL;
        case TAF_PA_RADIO_RAT_SERVICE_STATUS_POWER_SAVE:
            return TAF_RADIO_RAT_SVC_STATUS_POWER_SAVE;
        default:
            LE_INFO("Unknown RAT service status.");
    }

    return TAF_RADIO_RAT_SVC_STATUS_UNKNOWN;
}

taf_radio_NREndcAvailability_t Utility::Convert::EndcStatus
(
    taf_pa_radio_DataAvailSysStatus_t* statusPtr
)
{
    if (statusPtr == nullptr)
    {
        LE_ERROR("statusPtr is nullptr.");
        return TAF_RADIO_NR_ENDC_UNKNOWN;
    }

    uint64_t bitmask = 0x0;
    for (uint32_t i = 0; i < statusPtr->availSysCount && i < TAF_PA_RADIO_DATA_AVAIL_SYS_MAX_COUNT;
        i++)
    {
        if (statusPtr->availSysStatusInfo[i].rat == TAF_PA_RADIO_RAT_LTE)
            bitmask |= BITMASK_RAT_LTE;
        else if (statusPtr->availSysStatusInfo[i].rat == TAF_PA_RADIO_RAT_NR5G &&
            statusPtr->availSysStatusInfo[i].soMask & TAF_PA_RADIO_BITMASK_SO_5G_NSA)
            bitmask |= BITMASK_RAT_5G_NSA;
    }

    if (bitmask == (BITMASK_RAT_LTE | BITMASK_RAT_5G_NSA))
        return TAF_RADIO_NR_ENDC_AVAILABLE;

    return TAF_RADIO_NR_ENDC_UNAVAILABLE;
}

void Utility::Convert::LteCphyCaInfo
(
    taf_pa_radio_LteCphyCaInfo_t* paInfoPtr,
    CAInfo_t* infoPtr
)
{
    if (paInfoPtr == nullptr)
    {
        LE_ERROR("paInfoPtr is nullptr.");
        return;
    }

    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return;
    }

    infoPtr->cellCount = 1;
    infoPtr->status = TAF_RADIO_CA_STATUS_DEACTIVATED;

    for (uint32_t i = 0; i < paInfoPtr->scellInfoCount &&
        i < TAF_PA_RADIO_LTE_CPHY_SCELL_INFO_MAX_COUNT; i++)
    {
        if (paInfoPtr->scellInfo[i].scellState ==
            TAF_PA_RADIO_LTE_CPHY_SCELL_STATE_CONFIGURED_ACTIVATED)
        {
            infoPtr->status = TAF_RADIO_CA_STATUS_ACTIVATED;
            infoPtr->cellCount++;
        }
    }
}

void Utility::LayeredFunction::NetworkRejection
(
    void* reportPtr,
    void* handlerFuncPtr
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    taf_radio_NetRegRejectHandlerFunc_t handlerFunc =
        (taf_radio_NetRegRejectHandlerFunc_t)handlerFuncPtr;
    if (handlerFunc != nullptr)
        handlerFunc((taf_radio_NetRegRejInd_t*)reportPtr, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

void Utility::LayeredFunction::RatChange
(
    void* reportPtr,
    void* handlerFuncPtr
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    taf_radio_RatChangeHandlerFunc_t handlerFunc =
        (taf_radio_RatChangeHandlerFunc_t)handlerFuncPtr;
    if (handlerFunc != nullptr)
        handlerFunc((taf_radio_RatChangeInd_t*)reportPtr, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

void Utility::LayeredFunction::NetRegState
(
    void* reportPtr,
    void* handlerFuncPtr
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    taf_radio_NetRegStateHandlerFunc_t handlerFunc =
        (taf_radio_NetRegStateHandlerFunc_t)handlerFuncPtr;
    if (handlerFunc != nullptr)
        handlerFunc((taf_radio_NetRegStateInd_t*)reportPtr, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

void Utility::LayeredFunction::SignalStrengthInfoChange
(
    void* reportPtr,
    void* handlerFuncPtr
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    taf_radio_SignalStrengthChangeHandlerFunc_t handlerFunc =
        (taf_radio_SignalStrengthChangeHandlerFunc_t)handlerFuncPtr;
    if (handlerFunc != nullptr)
    {
        SignalStrengthInfoInd_t* infoPtr = (SignalStrengthInfoInd_t*)reportPtr;
        handlerFunc(infoPtr->rssi, infoPtr->rsrp, infoPtr->phone, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

void Utility::LayeredFunction::ImsRegStatusChange
(
    void* reportPtr,
    void* handlerFuncPtr
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    taf_radio_ImsRegStatusChangeHandlerFunc_t handlerFunc =
        (taf_radio_ImsRegStatusChangeHandlerFunc_t)handlerFuncPtr;
    if (handlerFunc != nullptr)
    {
        ImsRegStatusInd_t* indPtr = (ImsRegStatusInd_t*)reportPtr;
        handlerFunc(indPtr->status, indPtr->phone, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

void Utility::LayeredFunction::OperatingModeChange
(
    void* reportPtr,
    void* handlerFuncPtr
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    taf_radio_OpModeChangeHandlerFunc_t handlerFunc =
        (taf_radio_OpModeChangeHandlerFunc_t)handlerFuncPtr;
    if (handlerFunc != nullptr)
    {
        taf_radio_OpMode_t* modePtr = (taf_radio_OpMode_t*)reportPtr;
        handlerFunc(*modePtr, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

void Utility::LayeredFunction::NetStatusChange
(
    void* reportPtr,
    void* handlerFuncPtr
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    taf_radio_NetStatusHandlerFunc_t handlerFunc =
        (taf_radio_NetStatusHandlerFunc_t)handlerFuncPtr;
    if (handlerFunc != nullptr)
    {
        NetStatusInd_t* indPtr = (NetStatusInd_t*)reportPtr;
        handlerFunc(indPtr->reference, indPtr->bitmask, indPtr->phone, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

void Utility::LayeredFunction::ImsStatusChange
(
    void* reportPtr,
    void* handlerFuncPtr
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    taf_radio_ImsStatusChangeHandlerFunc_t handlerFunc =
        (taf_radio_ImsStatusChangeHandlerFunc_t)handlerFuncPtr;
    if (handlerFunc != nullptr)
    {
        ImsStatusInd_t* indPtr = (ImsStatusInd_t*)reportPtr;
        handlerFunc(indPtr->reference, indPtr->bitmask, indPtr->phone, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

void Utility::LayeredFunction::CellInfoChange
(
    void* reportPtr,
    void* handlerFuncPtr
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    taf_radio_CellInfoChangeHandlerFunc_t handlerFunc =
        (taf_radio_CellInfoChangeHandlerFunc_t)handlerFuncPtr;
    if (handlerFunc != nullptr)
    {
        CellInfoInd_t* indPtr = (CellInfoInd_t*)reportPtr;
        handlerFunc(indPtr->status, indPtr->phone, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

void Utility::LayeredFunction::NrIconChange
(
    void* reportPtr,
    void* handlerFuncPtr
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    taf_radio_NrIconTypeHandlerFunc_t handlerFunc =
        (taf_radio_NrIconTypeHandlerFunc_t)handlerFuncPtr;
    if (handlerFunc != nullptr)
    {
        NrIconInd_t* indPtr = (NrIconInd_t*)reportPtr;
        handlerFunc(indPtr->icon, indPtr->phone, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

void Utility::LayeredFunction::CAInfoChange
(
    void* reportPtr,
    void* handlerFuncPtr
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    taf_radio_CAInfoHandlerFunc_t handlerFunc = (taf_radio_CAInfoHandlerFunc_t)handlerFuncPtr;
    if (handlerFunc != nullptr)
    {
        CAInfoInd_t* indPtr = (CAInfoInd_t*)reportPtr;
        handlerFunc(indPtr->phone, indPtr->reference, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

void Utility::LayeredFunction::ConnStatusChange
(
    void* reportPtr,
    void* handlerFuncPtr
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    taf_radio_ConnectionStatusHandlerFunc_t handlerFunc =
        (taf_radio_ConnectionStatusHandlerFunc_t)handlerFuncPtr;
    if (handlerFunc != nullptr)
    {
        ConnStatusInd_t* indPtr = (ConnStatusInd_t*)reportPtr;
        handlerFunc(indPtr->phone, indPtr->bitmask, indPtr->reference, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

taf_radio_PciScanInformationListRef_t Utility::Common::PciNetworkScan
(
    uint8_t phone,
    taf_radio_RatBitMask_t bitmask
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_RatBitMask_t rat = Utility::Convert::Rat(bitmask);

    taf_pa_radio_PciScanInformation_t information;
    pa_result_t result = taf_pa_radio_PerformPciNetworkScan(instance, rat, &information);
    if (result != 0)
    {
        LE_ERROR("Failed to perform PCI network scan.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_mem_ForceAlloc(factory.pools.commonList);
    listPtr->commonList = LE_SLS_LIST_INIT;
    listPtr->safeRefList = LE_SLS_LIST_INIT;
    listPtr->currPtr = nullptr;

    PciCell_t* cellPtr = nullptr;
    PlmnId_t* plmnIdPtr = nullptr;
    for (uint32_t i = 0; i < information.pciCellCount && i < TAF_PA_RADIO_PCI_SCAN_CELL_MAX_COUNT;
        i++)
    {
        cellPtr = (PciCell_t*)le_mem_ForceAlloc(factory.pools.pciCell);

        cellPtr->cellId = information.pciCellInfo[i].cellId;
        cellPtr->globalCellId = information.pciCellInfo[i].globalCellId;

        for (uint32_t j = 0; j < information.pciCellInfo[i].plmnCount &&
            j < TAF_PA_RADIO_PCI_SCAN_PLMN_ID_MAX_COUNT; j++)
        {
            plmnIdPtr = (PlmnId_t*)le_mem_ForceAlloc(factory.pools.plmnId);

            plmnIdPtr->mcc = information.pciCellInfo[i].plmnId[j].mcc;
            plmnIdPtr->mnc = information.pciCellInfo[i].plmnId[j].mnc;
            plmnIdPtr->mncIncludesPcsDigit =
                information.pciCellInfo[i].plmnId[j].mncIncludesPcsDigit;

            plmnIdPtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(cellPtr->plmnIdList), &(plmnIdPtr->link));
        }

        cellPtr->link = LE_SLS_LINK_INIT;
        le_sls_Queue(&(listPtr->commonList), &(cellPtr->link));
    }

    return (taf_radio_PciScanInformationListRef_t)le_ref_CreateRef(factory.maps.commonList,
        (void*)listPtr);
}

taf_radio_ScanInformationListRef_t Utility::Common::PlmnNetworkScan
(
    uint8_t phone
)
{
    uint32_t instance = Utility::Convert::PhoneToInstance(phone);
    taf_pa_radio_PlmnNetworkScanConfig_t config;
    config.bitmask = TAF_PA_RADIO_BITMASK_RAT_GSM | TAF_PA_RADIO_BITMASK_RAT_UMTS |
        TAF_PA_RADIO_BITMASK_RAT_LTE | TAF_PA_RADIO_BITMASK_RAT_NR5G;
    config.timeout = PLMN_SCAN_TIMEOUT;
    taf_pa_radio_PlmnScanInformation_t information;
    pa_result_t result = taf_pa_radio_PerformPlmnNetworkScan(instance, &config, &information);
    if (result != 0)
    {
        LE_ERROR("Failed to perform PLMN network scan.");
        return nullptr;
    }

    auto& factory = Factory::GetInstance();
    CommonList_t* listPtr = (CommonList_t*)le_mem_ForceAlloc(factory.pools.commonList);
    listPtr->commonList = LE_SLS_LIST_INIT;
    listPtr->safeRefList = LE_SLS_LIST_INIT;
    listPtr->currPtr = nullptr;

    PlmnInfo_t* infoPtr = nullptr;
    for (uint32_t i = 0; i < information.plmnCount && i < TAF_PA_RADIO_PLMN_SCAN_NETWORK_MAX_COUNT;
        i++)
    {
        infoPtr = (PlmnInfo_t*)le_mem_ForceAlloc(factory.pools.plmnInfo);
        infoPtr->plmnInfo = information.plmnInfo[i];

        infoPtr->link = LE_SLS_LINK_INIT;
        le_sls_Queue(&(listPtr->commonList), &(infoPtr->link));
    }

    return (taf_radio_ScanInformationListRef_t)le_ref_CreateRef(factory.maps.commonList, (void*)listPtr);
}

le_result_t Utility::Common::ManualNetworkSelection
(
    uint8_t phone,
    const char* mccPtr,
    const char* mncPtr
)
{
    if (mccPtr == nullptr)
    {
        LE_ERROR("mccPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    if (mncPtr == nullptr)
    {
        LE_ERROR("mncPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    uint32_t instance = Utility::Convert::PhoneToInstance(phone);

    taf_pa_radio_NetworkSelectionPreference_t preference;
    preference.mode = TAF_PA_RADIO_NETWORK_SELECTION_MODE_MANUAL;

    le_result_t result = Utility::Convert::StringToU16(mccPtr, &preference.mcc);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert MCC %s.", mccPtr);
        return result;
    }

    result = Utility::Convert::StringToU16(mncPtr, &preference.mnc);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to convert MNC %s.", mncPtr);
        return result;
    }

    pa_result_t paResult = taf_pa_radio_SetNetworkSelectionPreference(instance, &preference);

    return Utility::Convert::Result(paResult);
}

uint32_t Utility::Common::FindServingCell
(
    taf_pa_radio_CellLocationListInfo_t* infoPtr
)
{
    if (infoPtr == nullptr)
    {
        LE_ERROR("infoPtr is nullptr.");
        return TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT;
    }

    bool found = false;
    uint32_t i = 0;
    while (i < infoPtr->cellLocInfoCount && i < TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT)
    {
        if (infoPtr->cellLocInfo[i].location == TAF_PA_RADIO_CELL_LOCATION_SERVING)
        {
            found = true;
            break;
        }

        i++;
    }

    if (found)
        return i;

    return TAF_PA_RADIO_CELL_LOCATION_MAX_COUNT;
}

StaticEvent_t Factory::staticEvents = 
{
    .request = nullptr
};

Factory& Factory::GetInstance
(
    void
)
{
    static Factory instance;
    return instance;
}

static void RequestHandler
(
    void* contextPtr
)
{
    Request_t* requestPtr = (Request_t*)contextPtr;
    if (requestPtr == nullptr)
    {
        LE_ERROR("requestPtr is nullptr.");
        return;
    }

    switch (requestPtr->command)
    {
        case COMMAND_SET_NETWORK_SELECTION_PREFERENCE:
        {
            le_result_t result = Utility::Common::ManualNetworkSelection(requestPtr->phone,
                requestPtr->preference.mcc, requestPtr->preference.mnc);

            taf_radio_ManualSelectionHandlerFunc_t handlerFunc =
                (taf_radio_ManualSelectionHandlerFunc_t)requestPtr->handlerFuncPtr;
            if (handlerFunc != nullptr)
                handlerFunc(result, requestPtr->contextPtr);
            else
                LE_WARN("Handler function for setting network selection preference is null.");

            break;
        }
        case COMMAND_PERFORM_PLMN_NETWORK_SCAN:
        {
            taf_radio_ScanInformationListRef_t listRef =
                Utility::Common::PlmnNetworkScan(requestPtr->phone);

            taf_radio_CellularNetworkScanHandlerFunc_t handlerFunc =
                (taf_radio_CellularNetworkScanHandlerFunc_t)requestPtr->handlerFuncPtr;
            if (handlerFunc != nullptr)
                handlerFunc(listRef, requestPtr->contextPtr);
            else
                LE_WARN("Handler function for performing PLMN network scan is null.");

            break;
        }
        case COMMAND_PERFORM_PCI_NETWORK_SCAN:
        {
            taf_radio_PciScanInformationListRef_t listRef =
                Utility::Common::PciNetworkScan(requestPtr->phone, requestPtr->rat);

            taf_radio_PciNetworkScanHandlerFunc_t handlerFunc =
                (taf_radio_PciNetworkScanHandlerFunc_t)requestPtr->handlerFuncPtr;
            if (handlerFunc != nullptr)
                handlerFunc(listRef, requestPtr->phone, requestPtr->contextPtr);
            else
                LE_WARN("Handler function for performing PCI network scan is null.");

            break;
        }
        default:
            LE_ERROR("Invalid command %d.", requestPtr->command);
    }
}

static void* RequestThread
(
    void* contextPtr
)
{
    le_event_AddHandler("RequestHandler", Factory::staticEvents.request, RequestHandler);

    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();

    return nullptr;
}

COMPONENT_INIT
{
    Factory::staticEvents.request = le_event_CreateId("request", sizeof(Request_t));

    auto& factory = Factory::GetInstance();

    factory.events.networkRejection = le_event_CreateIdWithRefCounting("NetworkRejection");
    factory.events.ratChange = le_event_CreateIdWithRefCounting("RatChange");
    factory.events.netRegState = le_event_CreateIdWithRefCounting("NetRegState");
    factory.events.packetSwitchedState = le_event_CreateIdWithRefCounting("PacketSwitchedState");
    factory.events.gsmSignalStrengthInfoChange = le_event_CreateIdWithRefCounting(
        "GsmSignalStrengthInfoChange");
    factory.events.cdmaSignalStrengthInfoChange = le_event_CreateIdWithRefCounting(
        "CdmaSignalStrengthInfoChange");
    factory.events.umtsSignalStrengthInfoChange = le_event_CreateIdWithRefCounting(
        "UmtsSignalStrengthInfoChange");
    factory.events.tdscdmaSignalStrengthInfoChange = le_event_CreateIdWithRefCounting(
        "TdscdmaSignalStrengthInfoChange");
    factory.events.lteSignalStrengthInfoChange = le_event_CreateIdWithRefCounting(
        "LteSignalStrengthInfoChange");
    factory.events.nr5gSignalStrengthInfoChange = le_event_CreateIdWithRefCounting(
        "Nr5gSignalStrengthInfoChange");
    factory.events.imsRegStatusChange = le_event_CreateIdWithRefCounting("ImsRegStatusChange");
    factory.events.operatingModeChange = le_event_CreateIdWithRefCounting("OperatingModeChange");
    factory.events.netStatusChange = le_event_CreateIdWithRefCounting("NetStatusChange");
    factory.events.imsStatusChange = le_event_CreateIdWithRefCounting("ImsStatusChange");
    factory.events.cellInfoChange = le_event_CreateIdWithRefCounting("CellInfoChange");
    factory.events.nrIconChange = le_event_CreateIdWithRefCounting("NrIconChange");
    factory.events.caInfoChange = le_event_CreateIdWithRefCounting("CAInfoChange");
    factory.events.connStatusChange = le_event_CreateIdWithRefCounting("ConnStatusChange");

    factory.pools.networkRejection = le_mem_CreatePool("NetworkRejection",
        sizeof(taf_radio_NetRegRejInd_t));
    factory.pools.ratChange = le_mem_CreatePool("RatChange", sizeof(taf_radio_RatChangeInd_t));
    factory.pools.netRegState = le_mem_CreatePool("NetRegState",
        sizeof(taf_radio_NetRegStateInd_t));
    factory.pools.signalStrengthInfoChange = le_mem_CreatePool("SignalStrengthInfoChange",
        sizeof(SignalStrengthInfoInd_t));
    factory.pools.imsRegStatusChange = le_mem_CreatePool("ImsRegStatusChange",
        sizeof(ImsRegStatusInd_t));
    factory.pools.operatingModeChange = le_mem_CreatePool("OperatingModeChange",
        sizeof(taf_radio_OpMode_t));
    factory.pools.netStatusChange = le_mem_CreatePool("NetStatusChange", sizeof(NetStatusInd_t));
    factory.pools.imsStatusChange = le_mem_CreatePool("ImsStatusChange", sizeof(ImsStatusInd_t));
    factory.pools.cellInfoChange = le_mem_CreatePool("CellInfoChange", sizeof(CellInfoInd_t));
	factory.pools.nrIconChange = le_mem_CreatePool("NrIconChange", sizeof(NrIconInd_t));
	factory.pools.caInfoChange = le_mem_CreatePool("CaInfoChange", sizeof(CAInfoInd_t));
	factory.pools.connStatusChange = le_mem_CreatePool("ConnStatusChange", sizeof(ConnStatusInd_t));

    factory.pools.commonList = le_mem_InitStaticPool(commonList, COMMON_LIST_MAX_COUNT,
        sizeof(CommonList_t));
    factory.pools.pciCell = le_mem_InitStaticPool(pciCell, PCI_CELL_MAX_COUNT, sizeof(PciCell_t));
    factory.pools.plmnId = le_mem_InitStaticPool(plmnId, PLMN_ID_MAX_COUNT, sizeof(PlmnId_t));
    factory.pools.plmnInfo = le_mem_InitStaticPool(plmnInfo, PLMN_INFO_MAX_COUNT,
        sizeof(PlmnInfo_t));
    factory.pools.prefNet = le_mem_InitStaticPool(prefNet, PREF_NET_MAX_COUNT, sizeof(PrefNet_t));
	factory.pools.ngbrCell = le_mem_InitStaticPool(ngbrCell, NGBR_CELL_MAX_COUNT,
         sizeof(NgbrCell_t));
    factory.pools.safeRef = le_mem_InitStaticPool(safeRef, SAFE_REF_MAX_COUNT, sizeof(SafeRef_t));
    factory.pools.signalStrengthInfo = le_mem_InitStaticPool(signalStrengthInfo,
        INSTANCE_MAX_COUNT, sizeof(taf_pa_radio_SignalStrengthInfo_t));
    factory.pools.caInfo = le_mem_InitStaticPool(caInfo, CA_INFO_MAX_COUNT, sizeof(CAInfo_t));
    factory.pools.connStatus = le_mem_InitStaticPool(connStatus, CONN_STATUS_MAX_COUNT,
        sizeof(taf_radio_NREndcAvailability_t));

    factory.maps.commonList = le_ref_InitStaticMap(commonList, COMMON_LIST_MAX_COUNT);
    factory.maps.safeRef = le_ref_InitStaticMap(safeRef, SAFE_REF_MAX_COUNT);
    factory.maps.signalStrengthInfo = le_ref_InitStaticMap(signalStrengthInfo, INSTANCE_MAX_COUNT);
    factory.maps.caInfo = le_ref_InitStaticMap(caInfo, CA_INFO_MAX_COUNT);
    factory.maps.connStatus = le_ref_InitStaticMap(connStatus, CONN_STATUS_MAX_COUNT);
    for (uint32_t i = 0; i < INSTANCE_MAX_COUNT; i++)
    {
        SafeRef_t* netRefPtr = (SafeRef_t*)le_mem_ForceAlloc(factory.pools.safeRef);
        factory.cache.netStatusRefs[i] = (taf_radio_NetStatusRef_t)le_ref_CreateRef(
            factory.maps.safeRef, (void*)netRefPtr);

        SafeRef_t* imsRefPtr = (SafeRef_t*)le_mem_ForceAlloc(factory.pools.safeRef);
        factory.cache.imsRefs[i] = (taf_radio_ImsRef_t)le_ref_CreateRef(
            factory.maps.safeRef, (void*)imsRefPtr);

        CAInfo_t* caInfoPtr = (CAInfo_t*)le_mem_ForceAlloc(factory.pools.caInfo);
        factory.cache.caInfoRefs[i] = (taf_radio_CAInfoRef_t)le_ref_CreateRef(
            factory.maps.caInfo, (void*)caInfoPtr);

        taf_radio_NREndcAvailability_t* availabilityPtr =
            (taf_radio_NREndcAvailability_t*)le_mem_ForceAlloc(factory.pools.connStatus);
        factory.cache.connStatusRefs[i] = (taf_radio_ConnStatusRef_t)le_ref_CreateRef(
            factory.maps.connStatus, (void*)availabilityPtr);
    }
    
    le_sem_Ref_t semaphore = le_sem_Create("semaphore", 0);
    le_thread_Ref_t thread = le_thread_Create("RequestThread", RequestThread, (void*)semaphore);
    le_thread_Start(thread);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    pa_result_t result = taf_pa_radio_Init();
    if (result != 0)
    {
        LE_ERROR("Failed to initialize platform adaptor.");
        return;
    }

    taf_pa_radio_AddNetworkRejectHandler(0, NetworkRejectHandler, nullptr);
    taf_pa_radio_AddRatChangeHandler(0, RatChangeHandler, nullptr);
    taf_pa_radio_AddVoiceServiceInfoHandler(0, VoiceServiceInfoHandler, nullptr);
    taf_pa_radio_AddDataServiceStatusHandler(0, DataServiceStatusHandler, nullptr);
    taf_pa_radio_AddDataRoamingStatusHandler(0, DataRoamingStatusHandler, nullptr);
    taf_pa_radio_AddSignalStrengthInfoChangeHandler(0, SignalStrengthInfoChangeHandler, nullptr);
    taf_pa_radio_AddImsRegStatusChangeHandler(0, ImsRegStatusChangeHandler, nullptr);
    taf_pa_radio_AddOperatingModeChangeHandler(0, OperatingModeChangeHandler, nullptr);
    taf_pa_radio_AddRatSvcStatusHandler(0, RatSvcStatusHandler, nullptr);
    taf_pa_radio_AddServiceDomainHandler(0, ServiceDomainHandler, nullptr);
    taf_pa_radio_AddLteCsCapabilityHandler(0, LteCsCapabilityHandler, nullptr);
    taf_pa_radio_AddImsServiceInfoHandler(0, ImsServiceInfoHandler, nullptr);
    taf_pa_radio_AddImsPdpErrorHandler(0, ImsPdpErrorHandler, nullptr);
    taf_pa_radio_AddCellInfoChangeHandler(0, CellInfoChangeHandler, nullptr);
    taf_pa_radio_AddNrIconChangeHandler(0, NrIconChangeHandler, nullptr);
    taf_pa_radio_AddLteCphyCaHandler(0, LteCphyCaHandler, nullptr);
    taf_pa_radio_AddDataAvailSysStatusHandler(0, DataAvailSysStatusHandler, nullptr);

    taf_pm_AddStateChangeHandler(PowerStateChangeHandler, nullptr);
    if (taf_pm_GetPowerState() != TAF_PM_STATE_SUSPEND)
        RegisterIndication(ENABLE_INDICATION);

    LE_INFO("Radio service is ready.");
}