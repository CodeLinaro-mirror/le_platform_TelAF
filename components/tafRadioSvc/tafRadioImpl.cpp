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

/*  Changes from Qualcomm Innovation Center are provided under the following license:
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafRadioImpl.cpp
 * @brief      This file describes the implementation method that radio
 *             service is in use.
 */

#include <cstdlib>
#include <chrono>

#include "tafRadio.hpp"
#include "taf_pa_radio.hpp"

using namespace std;
using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Listener for network scan results.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioNetworkSelectionListener::onNetworkScanResults
(
    telux::tel::NetworkScanStatus scanStatus,           ///< [IN] Scan status.
    std::vector<telux::tel::OperatorInfo> operatorInfos ///< [IN] Operator information.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioNetworkSelectionListener --> onNetworkScanResults");
    if (scanStatus == telux::tel::NetworkScanStatus::FAILED)
    {
        LE_ERROR("Network scan failed.");
        le_sem_Post(semaphore);
    }
    else
    {
        for (auto info : operatorInfos)
        {
            opInfos.emplace_back(info);
        }

        if (scanStatus == telux::tel::NetworkScanStatus::COMPLETE)
        {
            LE_INFO("Network scan completed.");
            le_sem_Post(semaphore);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initiate listener for IMS serving system.
 */
//--------------------------------------------------------------------------------------------------
taf_RadioImsServSysListener::taf_RadioImsServSysListener
(
    SlotId slotId ///< [IN] Slot ID.
) : slot(slotId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    if (tafRadio.phoneManager != nullptr)
    {
        phone = (uint8_t)(tafRadio.phoneManager->getPhoneIdFromSlotId((int)slotId));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for IMS registration status.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsServSysListener::onImsRegStatusChange
(
    telux::tel::ImsRegistrationInfo status ///< [IN] IMS registration status
)
{
    LE_DEBUG("<SDK Listener> taf_RadioImsServSysListener --> onImsRegStatusChange");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioImsStatus_t* statusPtr =
        (taf_RadioImsStatus_t*)le_mem_ForceAlloc(tafRadio.imsStatusChangePool);
    statusPtr->phoneId = phone;
    statusPtr->status = (taf_radio_ImsRegStatus_t)status.imsRegStatus;
    statusPtr->imsRef = tafRadio.imsRefs[phone - 1];
    le_event_ReportWithRefCounting(tafRadio.imsRegStatusChangeId, (void*)statusPtr);
}

#ifdef LE_CONFIG_FEATURE_ENHANCED_IMS
//--------------------------------------------------------------------------------------------------
/**
 * Listener for IMS service information.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsServSysListener::onImsServiceInfoChange
(
    telux::tel::ImsServiceInfo service ///< [IN] IMS service information.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioImsServSysListener --> onImsServiceInfoChange");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioImsStatus_t* statusPtr =
        (taf_RadioImsStatus_t*)le_mem_ForceAlloc(tafRadio.imsStatusChangePool);
    statusPtr->bitmask = TAF_RADIO_IMS_IND_BIT_MASK_SVC_INFO;
    statusPtr->phoneId = phone;
    statusPtr->imsRef = tafRadio.imsRefs[phone - 1];
    le_event_ReportWithRefCounting(tafRadio.imsStatusChangeId, (void*)statusPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for IMS PDP status.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsServSysListener::onImsPdpStatusInfoChange
(
    telux::tel::ImsPdpStatusInfo status ///< [IN] IMS PDP status.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioImsServSysListener --> onImsPdpStatusInfoChange");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioImsStatus_t* statusPtr =
        (taf_RadioImsStatus_t*)le_mem_ForceAlloc(tafRadio.imsStatusChangePool);
    statusPtr->bitmask = TAF_RADIO_IMS_IND_BIT_MASK_PDP_ERROR;
    statusPtr->phoneId = phone;
    statusPtr->imsRef = tafRadio.imsRefs[phone - 1];
    le_event_ReportWithRefCounting(tafRadio.imsStatusChangeId, (void*)statusPtr);
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Listener for operating mode.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioPhoneListener::onOperatingModeChanged
(
    telux::tel::OperatingMode mode ///< [IN] Operating mode.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioPhoneListener --> onOperatingModeChanged");

    auto &tafRadio = taf_Radio::GetInstance();

    taf_radio_OpMode_t* modePtr =
        (taf_radio_OpMode_t*)le_mem_ForceAlloc(tafRadio.opModeChangePool);
	*modePtr = (taf_radio_OpMode_t)mode;
    le_event_ReportWithRefCounting(tafRadio.opModeChangeId, (void*)modePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for voice service state.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioPhoneListener::onVoiceServiceStateChanged
(
    int phoneId,
        ///< [IN] Phone ID.
    const std::shared_ptr<telux::tel::VoiceServiceInfo> &srvInfo
        ///< [IN] Voice service information.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioPhoneListener --> onVoiceServiceStateChanged");

    auto &tafRadio = taf_Radio::GetInstance();

    taf_radio_NetRegStateInd_t* indPtr =
        (taf_radio_NetRegStateInd_t*)le_mem_ForceAlloc(tafRadio.netRegStatePool);
    indPtr->phoneId = phoneId;
    if (srvInfo != nullptr)
    {
        switch (srvInfo->getVoiceServiceState())
        {
            case telux::tel::VoiceServiceState::NOT_REG_AND_NOT_SEARCHING:
                indPtr->state = TAF_RADIO_NET_REG_STATE_NONE;
                break;
            case telux::tel::VoiceServiceState::REG_HOME:
                indPtr->state = TAF_RADIO_NET_REG_STATE_HOME;
                break;
            case telux::tel::VoiceServiceState::NOT_REG_AND_SEARCHING:
                indPtr->state = TAF_RADIO_NET_REG_STATE_SEARCHING;
                break;
            case telux::tel::VoiceServiceState::REG_DENIED:
                indPtr->state = TAF_RADIO_NET_REG_STATE_DENIED;
                break;
            case telux::tel::VoiceServiceState::REG_ROAMING:
                indPtr->state = TAF_RADIO_NET_REG_STATE_ROAMING;
                break;
            case telux::tel::VoiceServiceState::NOT_REG_AND_EMERGENCY_AVAILABLE_AND_NOT_SEARCHING:
                indPtr->state = TAF_RADIO_NET_REG_STATE_NONE_AND_EMERGENCY_AVAILABLE;
                break;
            case telux::tel::VoiceServiceState::NOT_REG_AND_EMERGENCY_AVAILABLE_AND_SEARCHING:
                indPtr->state = TAF_RADIO_NET_REG_STATE_SEARCHING_AND_EMERGENCY_AVAILABLE;
                break;
            case telux::tel::VoiceServiceState::REG_DENIED_AND_EMERGENCY_AVAILABLE:
                indPtr->state = TAF_RADIO_NET_REG_STATE_DENIED_AND_EMERGENCY_AVAILABLE;
                break;
            case telux::tel::VoiceServiceState::UNKNOWN_AND_EMERGENCY_AVAILABLE:
                indPtr->state= TAF_RADIO_NET_REG_STATE_UNKNOWN_AND_EMERGENCY_AVAILABLE;
                break;
            default:
                indPtr->state = TAF_RADIO_NET_REG_STATE_UNKNOWN;
        }
    }
    else
    {
        indPtr->state = TAF_RADIO_NET_REG_STATE_UNKNOWN;
    }
    le_event_ReportWithRefCounting(tafRadio.netRegStateEvId, (void*)indPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for operating mode.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioPhoneListener::onSignalStrengthChanged
(
    int phoneId,                                               ///< [IN] Phone ID.
    std::shared_ptr<telux::tel::SignalStrength> signalStrength ///< [IN] Signal strength.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioPhoneListener --> onSignalStrengthChanged");

    auto &tafRadio = taf_Radio::GetInstance();

    if (signalStrength->getGsmSignalStrength() != nullptr &&
        signalStrength->getGsmSignalStrength()->getGsmSignalStrength() !=
        INVALID_SIGNAL_STRENGTH_VALUE)
    {
        taf_RadioSsInd_t* ssPtr = (taf_RadioSsInd_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
        ssPtr->phoneId = phoneId;
        ssPtr->rssi = signalStrength->getGsmSignalStrength()->getDbm();
        le_event_ReportWithRefCounting(tafRadio.gsmSsChangeEvId, (void*)ssPtr);
    }

    if (signalStrength->getCdmaSignalStrength() != nullptr &&
        signalStrength->getCdmaSignalStrength()->getDbm() != INVALID_SIGNAL_STRENGTH_VALUE)
    {
        taf_RadioSsInd_t* ssPtr = (taf_RadioSsInd_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
        ssPtr->phoneId = phoneId;
        ssPtr->rssi = signalStrength->getCdmaSignalStrength()->getDbm();
        le_event_ReportWithRefCounting(tafRadio.cdmaSsChangeEvId, (void*)ssPtr);
    }

    if (signalStrength->getWcdmaSignalStrength() != nullptr &&
        signalStrength->getWcdmaSignalStrength()->getSignalStrength() !=
        INVALID_SIGNAL_STRENGTH_VALUE)
    {
        taf_RadioSsInd_t* ssPtr = (taf_RadioSsInd_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
        ssPtr->phoneId = phoneId;
        ssPtr->rssi = signalStrength->getWcdmaSignalStrength()->getDbm();
        le_event_ReportWithRefCounting(tafRadio.umtsSsChangeEvId, (void*)ssPtr);
    }

    if (signalStrength->getLteSignalStrength() != nullptr &&
        signalStrength->getLteSignalStrength()->getLteSignalStrength() !=
        INVALID_SIGNAL_STRENGTH_VALUE)
    {
        taf_RadioSsInd_t* ssPtr = (taf_RadioSsInd_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
        ssPtr->phoneId = phoneId;
        ssPtr->rsrp = signalStrength->getLteSignalStrength()->getDbm();
        le_event_ReportWithRefCounting(tafRadio.lteSsChangeEvId, (void*)ssPtr);
    }

    if (signalStrength->getNr5gSignalStrength() != nullptr &&
        signalStrength->getNr5gSignalStrength()->getDbm() != INVALID_SIGNAL_STRENGTH_VALUE)
    {
        taf_RadioSsInd_t* ssPtr = (taf_RadioSsInd_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
        ssPtr->phoneId = phoneId;
        ssPtr->rsrp = signalStrength->getNr5gSignalStrength()->getDbm();
        le_event_ReportWithRefCounting(tafRadio.nr5gSsChangeEvId, (void*)ssPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initiate listener for Data serving system.
 */
//--------------------------------------------------------------------------------------------------
taf_RadioDataServSysListener::taf_RadioDataServSysListener
(
    SlotId slotId ///< [IN] Slot ID.
) : slot(slotId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    if (tafRadio.phoneManager != nullptr)
    {
        phone = (uint8_t)(tafRadio.phoneManager->getPhoneIdFromSlotId((int)slotId));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for Data service state.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioDataServSysListener::onServiceStateChanged
(
    telux::data::ServiceStatus status ///< [IN] Data service state.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioDataServSysListener --> onServiceStateChanged");

    auto &tafRadio = taf_Radio::GetInstance();

    taf_radio_NetRegStateInd_t* indPtr;
    taf_radio_NetRegState_t nextState = TAF_RADIO_NET_REG_STATE_UNKNOWN;

    switch (status.serviceState)
    {
        case telux::data::DataServiceState::IN_SERVICE:
            inService = true;
            if (isRoaming)
            {
                nextState = TAF_RADIO_NET_REG_STATE_ROAMING;
            }
            else
            {
                nextState = TAF_RADIO_NET_REG_STATE_HOME;
            }
            break;
        case telux::data::DataServiceState::OUT_OF_SERVICE:
            inService = false;
            nextState = TAF_RADIO_NET_REG_STATE_NONE;
            break;
        default:
            nextState = TAF_RADIO_NET_REG_STATE_UNKNOWN;
            break;
    }

    if (nextState != currState)
    {
        currState = nextState;
        indPtr = (taf_radio_NetRegStateInd_t*)le_mem_ForceAlloc(tafRadio.packSwStatePool);
        indPtr->phoneId = phone;
        indPtr->state = currState;
        le_event_ReportWithRefCounting(tafRadio.packSwStateEvId, (void*)indPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for Data roaming state.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioDataServSysListener::onRoamingStatusChanged
(
    telux::data::RoamingStatus status ///< [IN] Data roaming state.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioDataServSysListener --> onRoamingStatusChanged");

    auto &tafRadio = taf_Radio::GetInstance();

    taf_radio_NetRegStateInd_t* indPtr;
    taf_radio_NetRegState_t nextState = TAF_RADIO_NET_REG_STATE_UNKNOWN;

    isRoaming = status.isRoaming;
    if (isRoaming)
    {
        nextState = TAF_RADIO_NET_REG_STATE_ROAMING;
    }
    else
    {
        if (inService)
        {
            nextState = TAF_RADIO_NET_REG_STATE_HOME;
        }
        else
        {
            nextState = TAF_RADIO_NET_REG_STATE_NONE;
        }
    }

    if (nextState != currState)
    {
        currState = nextState;
        indPtr = (taf_radio_NetRegStateInd_t*)le_mem_ForceAlloc(tafRadio.packSwStatePool);
        indPtr->phoneId = phone;
        indPtr->state = currState;
        le_event_ReportWithRefCounting(tafRadio.packSwStateEvId, (void*)indPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for setting operating mode.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioSetOperatingModeCallback::setOperatingModeResponse
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioOperatingModeCallback --> setOperatingModeResponse");
    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting operating mode.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioGetOperatingModeCallback::operatingModeResponse
(
    telux::tel::OperatingMode operatingMode, ///< [IN] Operating mode.
    telux::common::ErrorCode error           ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioGetOperatingModeCallback --> operatingModeResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        opMode = operatingMode;
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting signal strength.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioSignalStrengthCallback::signalStrengthResponse
(
    std::shared_ptr<telux::tel::SignalStrength> signalStrength, ///< [IN] Signal strength.
    telux::common::ErrorCode error                              ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioSignalStrengthCallback --> signalStrengthResponse");

    ssMetrics.ratMask = 0x0;

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        result = LE_OK;
    }

    if (signalStrength->getGsmSignalStrength() != nullptr &&
        signalStrength->getGsmSignalStrength()->getGsmSignalStrength() !=
        INVALID_SIGNAL_STRENGTH_VALUE)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_GSM;
        ssMetrics.gsm.sslv = (int8_t)signalStrength->getGsmSignalStrength()->getLevel() + 1;
        ssMetrics.gsm.ss = signalStrength->getGsmSignalStrength()->getDbm();
        ssMetrics.gsm.ber = signalStrength->getGsmSignalStrength()->getGsmBitErrorRate();
    }

    if (signalStrength->getCdmaSignalStrength() != nullptr &&
        signalStrength->getCdmaSignalStrength()->getDbm() != INVALID_SIGNAL_STRENGTH_VALUE)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_CDMA;
        ssMetrics.cdma.sslv = (int8_t)signalStrength->getCdmaSignalStrength()->getLevel() + 1;
        ssMetrics.cdma.ss = signalStrength->getCdmaSignalStrength()->getDbm();
        ssMetrics.cdma.ecio = signalStrength->getCdmaSignalStrength()->getCdmaEcio();
        ssMetrics.cdma.io = signalStrength->getCdmaSignalStrength()->getEvdoEcio();
        ssMetrics.cdma.snr = signalStrength->getCdmaSignalStrength()->getEvdoSignalNoiseRatio();
    }

    if (signalStrength->getWcdmaSignalStrength() != nullptr &&
        signalStrength->getWcdmaSignalStrength()->getSignalStrength() !=
        INVALID_SIGNAL_STRENGTH_VALUE)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_UMTS;
        ssMetrics.umts.sslv = (int8_t)signalStrength->getWcdmaSignalStrength()->getLevel() + 1;
        ssMetrics.umts.ss = signalStrength->getWcdmaSignalStrength()->getDbm();
        ssMetrics.umts.ber = signalStrength->getWcdmaSignalStrength()->getBitErrorRate();
    }

    if (signalStrength->getTdscdmaSignalStrength() != nullptr &&
        signalStrength->getTdscdmaSignalStrength()->getRscp() != INVALID_SIGNAL_STRENGTH_VALUE)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_TDSCDMA;
        ssMetrics.tdscdma.rscp = signalStrength->getTdscdmaSignalStrength()->getRscp();
    }

    if (signalStrength->getLteSignalStrength() != nullptr &&
        signalStrength->getLteSignalStrength()->getLteSignalStrength() !=
        INVALID_SIGNAL_STRENGTH_VALUE)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_LTE;
        ssMetrics.lte.sslv = (int8_t)signalStrength->getLteSignalStrength()->getLevel() + 1;
        ssMetrics.lte.ss = signalStrength->getLteSignalStrength()->getDbm();
        ssMetrics.lte.rsrq = signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality();
        ssMetrics.lte.rsrp = signalStrength->getLteSignalStrength()->getDbm();
        ssMetrics.lte.snr = signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr();
    }

    if (signalStrength->getNr5gSignalStrength() != nullptr &&
        signalStrength->getNr5gSignalStrength()->getDbm() != INVALID_SIGNAL_STRENGTH_VALUE)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_NR5G;
        ssMetrics.nr5g.sslv = (int8_t)signalStrength->getNr5gSignalStrength()->getLevel() + 1;
        ssMetrics.nr5g.rsrq = signalStrength->getNr5gSignalStrength()->getReferenceSignalReceiveQuality();
        ssMetrics.nr5g.rsrp = signalStrength->getNr5gSignalStrength()->getDbm();
        ssMetrics.nr5g.snr = signalStrength->getNr5gSignalStrength()->getReferenceSignalSnr();
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting cellular capability.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioCellularCapsCallback::cellularCapabilityResponse
(
     telux::tel::CellularCapabilityInfo   capabilityInfo,  ///< [IN] Cellular Capability.
     telux::common::ErrorCode error                        ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioCellularCapsCallback --> cellularCapabilityResponse");

    if (error == telux::common::ErrorCode::SUCCESS)
    {
        simCount = capabilityInfo.simCount;
        maxActiveSIM = capabilityInfo.maxActiveSims;

        for (auto simCaps : capabilityInfo.simRatCapabilities)
        {
            simRatCaps.emplace_back(simCaps);
        }
        for (auto deviceCaps : capabilityInfo.deviceRatCapability)
        {
            deviceRatCaps.emplace_back(deviceCaps);
        }
        result = LE_OK;
    }
    else
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }

    le_sem_Post(semaphore);
}


//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for configuring signal strength.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioConfigureSignalStrengthCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of configuring signal strength.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioConfigureSignalStrengthCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Response for configuring signal strength.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioConfigureSignalStrengthCallback::configureSignalStrengthResponse
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioConfigureSignalStrengthCallback --> \
        configureSignalStrengthResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting voice service state.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioVoiceServiceStateCallback::voiceServiceStateResponse
(
    const std::shared_ptr<telux::tel::VoiceServiceInfo> &serviceInfo,
        ///< [IN] Voice service information.
    telux::common::ErrorCode error
        ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioVoiceServiceStateCallback --> voiceServiceStateResponse");

    if (error != telux::common::ErrorCode::SUCCESS || serviceInfo == nullptr)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        voiceSvcState = serviceInfo->getVoiceServiceState();
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for setting selection mode.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioNetworkResponseCallback::selModeSem = NULL;
le_sem_Ref_t taf_RadioNetworkResponseCallback::prefNetSem = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of setting selection mode.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioNetworkResponseCallback::selModeRes = LE_OK;
le_result_t taf_RadioNetworkResponseCallback::prefNetRes = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Response for setting selection mode.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioNetworkResponseCallback::setNetworkSelectionModeResponseCb
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioNetworkResponseCallback --> setNetworkSelectionModeResponseCb");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        selModeRes = LE_FAULT;
    }
    else
    {
        selModeRes = LE_OK;
    }

    le_sem_Post(selModeSem);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for setting preferred network.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioNetworkResponseCallback::setPreferredNetworksResponseCb
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioNetworkResponseCallback --> setPreferredNetworksResponseCb");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        prefNetRes = LE_FAULT;
    }
    else
    {
        prefNetRes = LE_OK;
    }

    le_sem_Post(prefNetSem);
}

//--------------------------------------------------------------------------------------------------
/**
 * Preferred network information.
 */
//--------------------------------------------------------------------------------------------------
std::vector<telux::tel::PreferredNetworkInfo> taf_RadioPreferredNetworksResponseCallback::preferredNetworksInfo;

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for getting preferred network information.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioPreferredNetworksResponseCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of getting preferred network information.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioPreferredNetworksResponseCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting preferred network information.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioPreferredNetworksResponseCallback::preferredNetworksResponse
(
    std::vector<telux::tel::PreferredNetworkInfo> preferredNetworks3gppInfo,
        ///< [IN] Preferred network.
    std::vector<telux::tel::PreferredNetworkInfo> staticPreferredNetworksInfo,
        ///< [IN] Static preferred network.
    telux::common::ErrorCode error
        ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioPreferredNetworksResponseCallback --> preferredNetworksResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        preferredNetworksInfo.assign(preferredNetworks3gppInfo.begin(), preferredNetworks3gppInfo.end());
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for peforming network scan.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioPerformNetworkScanCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of performing network scan.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioPerformNetworkScanCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Response for performing network scan.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioPerformNetworkScanCallback::performNetworkScanResponse
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioPerformNetworkScanCallback --> performNetworkScanResponse");
    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for setting serving system.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioServingSystemResponseCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of setting serving system.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioServingSystemResponseCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Response for setting serving system.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioServingSystemResponseCallback::servingSystemResponse
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioServingSystemResponseCallback --> servingSystemResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for getting RAT preference.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioRatPreferenceResponseCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of getting RAT preference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioRatPreferenceResponseCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * RAT preference.
 */
//--------------------------------------------------------------------------------------------------
telux::tel::RatPreference taf_RadioRatPreferenceResponseCallback::ratPref = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting RAT preference.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioRatPreferenceResponseCallback::ratPreferenceResponse
(
    telux::tel::RatPreference preference, ///< [IN] RAT preference.
    telux::common::ErrorCode error        ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioRatPreferenceResponseCallback --> ratPreferenceResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        ratPref = preference;
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for getting cell list information.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioCellInfoCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of getting cell list information.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioCellInfoCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Cell list information.
 */
//--------------------------------------------------------------------------------------------------
taf_RadioCellListInfo_t taf_RadioCellInfoCallback::cellListInfo;

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting cell list information.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioCellInfoCallback::cellInfoListResponse
(
    std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList, ///< [IN] Cell information list.
    telux::common::ErrorCode error                                   ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioCellInfoCallback --> cellInfoListResponse");

    cellListInfo.servingCell.clear();
    cellListInfo.neighborCell.clear();

    if (error == telux::common::ErrorCode::SUCCESS)
    {
        for (auto cellInfo : cellInfoList)
        {
            taf_RadioCellInfo_t cellIdInfo;
            if (cellInfo == NULL)
            {
                LE_ERROR("Cell information pointer is NULL.");
                break;
            }

            switch (cellInfo->getType())
            {
                case telux::tel::CellType::GSM:
                {
                    cellIdInfo.rat = TAF_RADIO_RAT_GSM;
                    auto gsmCellInfo = std::static_pointer_cast<telux::tel::GsmCellInfo>(cellInfo);
                    cellIdInfo.gsm.bsic = gsmCellInfo->getCellIdentity().getBaseStationIdentityCode();
                    cellIdInfo.gsm.cid = gsmCellInfo->getCellIdentity().getIdentity();
                    cellIdInfo.gsm.lac = gsmCellInfo->getCellIdentity().getLac();
                    cellIdInfo.gsm.ta = gsmCellInfo->getSignalStrengthInfo().getTimingAdvance();
                    cellIdInfo.ss = gsmCellInfo->getSignalStrengthInfo().getDbm();
                    if (gsmCellInfo->isRegistered())
                    {
                        cellListInfo.servingCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    else
                    {
                        cellListInfo.neighborCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    break;
                }
                case telux::tel::CellType::CDMA:
                {
                    cellIdInfo.rat = TAF_RADIO_RAT_CDMA;
                    auto cdmaCellInfo = std::static_pointer_cast<telux::tel::CdmaCellInfo>(cellInfo);
                    cellIdInfo.cdma.bsid = cdmaCellInfo->getCellIdentity().getBaseStationId();
                    cellIdInfo.cdma.ecio = cdmaCellInfo->getSignalStrengthInfo().getCdmaEcio();
                    cellIdInfo.ss = cdmaCellInfo->getSignalStrengthInfo().getDbm();
                    if (cdmaCellInfo->isRegistered())
                    {
                        cellListInfo.servingCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    else
                    {
                        cellListInfo.neighborCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    break;
                }
                case telux::tel::CellType::WCDMA:
                {
                    cellIdInfo.rat = TAF_RADIO_RAT_UMTS;
                    auto umtsCellInfo = std::static_pointer_cast<telux::tel::WcdmaCellInfo>(cellInfo);
                    cellIdInfo.umts.lac = umtsCellInfo->getCellIdentity().getLac();
                    cellIdInfo.umts.cid = umtsCellInfo->getCellIdentity().getIdentity();
                    cellIdInfo.umts.psc = umtsCellInfo->getCellIdentity().getPrimaryScramblingCode();
                    cellIdInfo.ss = umtsCellInfo->getSignalStrengthInfo().getDbm();
                    if (umtsCellInfo->isRegistered())
                    {
                        cellListInfo.servingCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    else
                    {
                        cellListInfo.neighborCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    break;
                }
                case telux::tel::CellType::TDSCDMA:
                {
                    cellIdInfo.rat = TAF_RADIO_RAT_TDSCDMA;
                    auto tdscdmaCellInfo = std::static_pointer_cast<telux::tel::TdscdmaCellInfo>(cellInfo);
                    cellIdInfo.tdscdma.cid = tdscdmaCellInfo->getCellIdentity().getIdentity();
                    cellIdInfo.tdscdma.lac = tdscdmaCellInfo->getCellIdentity().getLac();
                    cellIdInfo.ss = tdscdmaCellInfo->getSignalStrengthInfo().getRscp();
                    if (tdscdmaCellInfo->isRegistered())
                    {
                        cellListInfo.servingCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    else
                    {
                        cellListInfo.neighborCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    break;
                }
                case telux::tel::CellType::LTE:
                {
                    cellIdInfo.rat = TAF_RADIO_RAT_LTE;
                    auto lteCellInfo = std::static_pointer_cast<telux::tel::LteCellInfo>(cellInfo);
                    cellIdInfo.lte.cid = lteCellInfo->getCellIdentity().getIdentity();
                    cellIdInfo.lte.pcid = lteCellInfo->getCellIdentity().getPhysicalCellId();
                    cellIdInfo.lte.tac = lteCellInfo->getCellIdentity().getTrackingAreaCode();
                    cellIdInfo.lte.earfcn = lteCellInfo->getCellIdentity().getEarfcn();
                    cellIdInfo.lte.ta = lteCellInfo->getSignalStrengthInfo().getTimingAdvance();
                    cellIdInfo.ss = lteCellInfo->getSignalStrengthInfo().getDbm();
                    if (lteCellInfo->isRegistered())
                    {
                        cellListInfo.servingCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    else
                    {
                        cellListInfo.neighborCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    break;
                }
                case telux::tel::CellType::NR5G:
                {
                    cellIdInfo.rat = TAF_RADIO_RAT_NR5G;
                    auto nr5gCellInfo = std::static_pointer_cast<telux::tel::Nr5gCellInfo>(cellInfo);
                    cellIdInfo.nr5g.cid = nr5gCellInfo->getCellIdentity().getIdentity();
                    cellIdInfo.nr5g.pcid = nr5gCellInfo->getCellIdentity().getPhysicalCellId();
                    cellIdInfo.nr5g.tac = nr5gCellInfo->getCellIdentity().getTrackingAreaCode();
                    cellIdInfo.nr5g.arfcn = nr5gCellInfo->getCellIdentity().getArfcn();
                    cellIdInfo.ss = nr5gCellInfo->getSignalStrengthInfo().getDbm();
                    if (nr5gCellInfo->isRegistered())
                    {
                        cellListInfo.servingCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    else
                    {
                        cellListInfo.neighborCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    break;
                }
                default:
                {
                    LE_ERROR("Unknown RAT(%d)", (int)cellInfo->getType());
                    break;
                }
            }
        }

        result = LE_OK;
    }
    else
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for getting IMS registration information.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioImsServSysCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of getting IMS registration information.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioImsServSysCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * IMS registration information.
 */
//--------------------------------------------------------------------------------------------------
telux::tel::RegistrationStatus taf_RadioImsServSysCallback::status;

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting IMS registration information.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsServSysCallback::imsRegStateResponse
(
    telux::tel::ImsRegistrationInfo info, ///< [IN] IMS registration information.
    telux::common::ErrorCode error        ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioImsServSysCallback --> imsRegStateResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        status = info.imsRegStatus;
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

#ifdef LE_CONFIG_FEATURE_ENHANCED_IMS
//--------------------------------------------------------------------------------------------------
/**
 * IMS VoIP service status.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ImsSvcStatus_t taf_RadioImsServSysCallback::voip = TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;

//--------------------------------------------------------------------------------------------------
/**
 * IMS SMS service status.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ImsSvcStatus_t taf_RadioImsServSysCallback::sms = TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;

//--------------------------------------------------------------------------------------------------
/**
 * IMS PDP error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PdpError_t taf_RadioImsServSysCallback::pdpError = TAF_RADIO_PDP_ERROR_UNKNOWN;

//--------------------------------------------------------------------------------------------------
/**
 * Convert IMS service status.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ImsSvcStatus_t ConvertImsSvcStatus
(
    telux::tel::CellularServiceStatus status ///< [IN] IMS service status.
)
{
    switch (status)
    {
        case telux::tel::CellularServiceStatus::NO_SERVICE:
            return TAF_RADIO_IMS_SVC_STATUS_UNAVAILABLE;
        case telux::tel::CellularServiceStatus::LIMITED_SERVICE:
            return TAF_RADIO_IMS_SVC_STATUS_LIMITED;
        case telux::tel::CellularServiceStatus::FULL_SERVICE:
            return TAF_RADIO_IMS_SVC_STATUS_FULL_SERVICE;
        default:
            break;
    }

    return TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert IMS PDP error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PdpError_t ConvertImsPdpError
(
    telux::tel::PdpFailureCode error ///< [IN] IMS PDP error.
)
{
    switch (error)
    {
        case telux::tel::PdpFailureCode::OTHER_FAILURE:
            return TAF_RADIO_PDP_ERROR_GENERIC;
        case telux::tel::PdpFailureCode::OPTION_UNSUBSCRIBED:
            return TAF_RADIO_PDP_ERROR_OPTION_UNSUBSCRIBED;
        case telux::tel::PdpFailureCode::UNKNOWN_PDP:
            return TAF_RADIO_PDP_ERROR_UNKNOWN_PDP;
        case telux::tel::PdpFailureCode::REASON_NOT_SPECIFIED:
            return TAF_RADIO_PDP_ERROR_REASON_NOT_SPECIFIED;
        case telux::tel::PdpFailureCode::CONNECTION_BRINGUP_FAILURE:
            return TAF_RADIO_PDP_ERROR_CONNECTION_BRINGUP_FAILURE;
        case telux::tel::PdpFailureCode::CONNECTION_IKE_AUTH_FAILURE:
            return TAF_RADIO_PDP_ERROR_CONNECTION_IKE_AUTH_FAILURE;
        case telux::tel::PdpFailureCode::USER_AUTH_FAILED:
            return TAF_RADIO_PDP_ERROR_USER_AUTH_FAILURE;
        default:
            break;
    }

    return TAF_RADIO_PDP_ERROR_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting IMS service information.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsServSysCallback::imsSvcInfoResponse
(
    telux::tel::ImsServiceInfo info, ///< [IN] IMS service information.
    telux::common::ErrorCode error   ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioImsServSysCallback --> imsSvcInfoResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        sms = ConvertImsSvcStatus(info.sms);
        voip = ConvertImsSvcStatus(info.voice);
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting IMS PDP status.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsServSysCallback::imsPdpStatusResponse
(
    telux::tel::ImsPdpStatusInfo status,       ///< [IN] IMS PDP status.
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioImsServSysCallback --> imsPdpStatusResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        pdpError = ConvertImsPdpError(status.failureCode);
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for IMS settings.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioImsSettingCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of IMS settings.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioImsSettingCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * IMS service configurations.
 */
//--------------------------------------------------------------------------------------------------
telux::tel::ImsServiceConfig taf_RadioImsSettingCallback::config;

//--------------------------------------------------------------------------------------------------
/**
 * IMS sip user agent.
 */
//--------------------------------------------------------------------------------------------------
char taf_RadioImsSettingCallback::sipUserAgentPtr[TAF_RADIO_IMS_USER_AGENT_BYTES] = {0};

//--------------------------------------------------------------------------------------------------
/**
 * Response for setting IMS configurations.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsSettingCallback::onResponseCallback
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioImsSettingCallback --> onResponseCallback");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting IMS configurations.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsSettingCallback::onRequestImsServiceConfig
(
    SlotId slotId,                           ///< [IN] Slot ID.
    telux::tel::ImsServiceConfig configType, ///< [IN] IMS service configurations.
    telux::common::ErrorCode error           ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioImsSettingCallback --> onRequestImsServiceConfig");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        config = configType;
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting IMS sip user agent.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsSettingCallback::onRequestImsSipUserAgentConfig
(
    SlotId slotId,                 ///< [IN] Slot ID.
    std::string sipUserAgent,      ///< [IN] IMS service configurations.
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioImsSettingCallback --> onRequestImsSipUserAgentConfig");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        sipUserAgentPtr[0] = '\0';
        if (sipUserAgent.c_str() != NULL)
        {
            le_utf8_Copy(sipUserAgentPtr, sipUserAgent.c_str(),
                TAF_RADIO_IMS_USER_AGENT_BYTES, NULL);
        }
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}


LE_MEM_DEFINE_STATIC_POOL(prefOpsListPool, TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM, sizeof(taf_RadioPrefOpList_t));

LE_MEM_DEFINE_STATIC_POOL(prefOpPool, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM, sizeof(taf_RadioPrefOp_t));

LE_MEM_DEFINE_STATIC_POOL(prefOpSafeRefPool, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM, sizeof(taf_RadioPrefOpSafeRef_t));

LE_MEM_DEFINE_STATIC_POOL(scanOpsListPool, TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM, sizeof(taf_RadioScanOpList_t));

LE_MEM_DEFINE_STATIC_POOL(scanOpPool, TAF_RADIO_SCAN_OPERATORS_MAX_NUM, sizeof(taf_RadioScanOp_t));

LE_MEM_DEFINE_STATIC_POOL(scanOpSafeRefPool, TAF_RADIO_SCAN_OPERATORS_MAX_NUM, sizeof(taf_RadioScanOpSafeRef_t));

LE_MEM_DEFINE_STATIC_POOL(metricsPool, TAF_RADIO_METRICS_MAX_NUM, sizeof(taf_RadioSignalMetrics_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for IMS references.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(phonePool, TAF_RADIO_PHONE_NUM, sizeof(uint8_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for neighboring cells
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ngbrCellsPool, TAF_RADIO_NEIGHBOR_CELLS_MAX_NUM, sizeof(taf_RadioNgbrCells_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for neighboring cell information
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ngbrCellInfoPool, TAF_RADIO_NEIGHBOR_CELL_INFO_MAX_NUM,
    sizeof(taf_RadioNgbrCellInfo_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for neighboring cell information safe reference
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ngbrCellInfoSafeRefPool, TAF_RADIO_NEIGHBOR_CELL_INFO_MAX_NUM,
    sizeof(taf_RadioNgbrCellInfoSafeRef_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for IMS references.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(prefOpListRefMap, TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM);

LE_REF_DEFINE_STATIC_MAP(prefOpSafeRefMap, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM);

LE_REF_DEFINE_STATIC_MAP(scanOpListRefMap, TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM);

LE_REF_DEFINE_STATIC_MAP(scanOpSafeRefMap, TAF_RADIO_SCAN_OPERATORS_MAX_NUM);

LE_REF_DEFINE_STATIC_MAP(metricsRefMap, TAF_RADIO_METRICS_MAX_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for neighboring cells
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(ngbrCellsRefMap, TAF_RADIO_NEIGHBOR_CELLS_MAX_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for neighboring cell information safe reference
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(ngbrCellInfoSafeRefMap, TAF_RADIO_NEIGHBOR_CELL_INFO_MAX_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for IMS references.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(imsRefMap, TAF_RADIO_PHONE_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for ne references.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(netStatusRefMap, TAF_RADIO_PHONE_NUM);

le_event_Id_t taf_Radio::radioCmdEvId = nullptr;

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for IMS registration state.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerImsRegStateHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_ImsRegStatusChangeHandlerFunc_t handlerFunc =
        (taf_radio_ImsRegStatusChangeHandlerFunc_t)layerHandlerFunc;
    if (handlerFunc)
    {
        taf_RadioImsStatus_t* statusPtr = (taf_RadioImsStatus_t*)reportPtr;
        handlerFunc(statusPtr->status, statusPtr->phoneId, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for operating mode.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerOpModeHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_OpModeChangeHandlerFunc_t handlerFunc =
        (taf_radio_OpModeChangeHandlerFunc_t)layerHandlerFunc;
    if (handlerFunc)
    {
        taf_radio_OpMode_t* modePtr = (taf_radio_OpMode_t*)reportPtr;
        handlerFunc(*modePtr, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for network registration state.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerNetRegStateHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_NetRegStateHandlerFunc_t handlerFunc =
        (taf_radio_NetRegStateHandlerFunc_t)layerHandlerFunc;
    if (handlerFunc)
    {
        handlerFunc((taf_radio_NetRegStateInd_t*)reportPtr, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for IMS state.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerImsStateHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_ImsStatusChangeHandlerFunc_t handlerFunc =
        (taf_radio_ImsStatusChangeHandlerFunc_t)layerHandlerFunc;

    taf_RadioImsStatus_t* statusPtr = (taf_RadioImsStatus_t*)reportPtr;
    if (handlerFunc)
    {
        handlerFunc(statusPtr->imsRef, statusPtr->bitmask, statusPtr->phoneId,
            le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for signal strength.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerSsHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_SignalStrengthChangeHandlerFunc_t handlerFunc =
        (taf_radio_SignalStrengthChangeHandlerFunc_t)layerHandlerFunc;

    taf_RadioSsInd_t* ssPtr = (taf_RadioSsInd_t*)reportPtr;
    if (handlerFunc)
    {
        handlerFunc(ssPtr->rssi, ssPtr->rsrp, ssPtr->phoneId, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

/*======================================================================

 FUNCTION        taf_Radio::GetInstance

 DESCRIPTION     Get the instance of radio.

 DEPENDENCIES    The initialization of Radio.

 PARAMETERS      None

 RETURN VALUE    taf_Radio&

 SIDE EFFECTS

======================================================================*/
taf_Radio &taf_Radio::GetInstance()
{
    static taf_Radio instance;
    return instance;
}

/*======================================================================

 FUNCTION        taf_Radio::RadioProcCmdHandler

 DESCRIPTION     Radio command handler.

 DEPENDENCIES    The initialization of radio command thread.

 PARAMETERS      [IN] void* cmdReqPtr: Command request pointer.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Radio::RadioProcCmdHandler(void* cmdReqPtr)
{
    taf_RadioCmdReq_t* cmdReq = (taf_RadioCmdReq_t*)cmdReqPtr;
    uint8_t phoneId = cmdReq->phoneId;
    auto &tafRadio = taf_Radio::GetInstance();

    le_result_t res = LE_OK;
    le_clk_Time_t timeToWait = {1, 0};

    switch (cmdReq->cmdType)
    {
        case TAF_RADIO_CMD_TYPE_ASYNC_REG_MANUAL:
        {
            TAF_ERROR_IF_RET_NIL(phoneId > tafRadio.networkManagers.size(),
                "Invalid para(phoneId:%d > %" PRIuS ")", phoneId, tafRadio.networkManagers.size());

            auto networkManager = tafRadio.networkManagers[phoneId - 1];
            TAF_ERROR_IF_RET_NIL(networkManager == nullptr, "Invalid para(null ptr, phoneId:%d)", phoneId);

            std::string mcc(cmdReq->mccPtr);
            std::string mnc(cmdReq->mncPtr);

            if (networkManager->setNetworkSelectionMode(telux::tel::NetworkSelectionMode::MANUAL, mcc, mnc,
                &taf_RadioNetworkResponseCallback::setNetworkSelectionModeResponseCb) != telux::common::Status::SUCCESS)
            {
                LE_ERROR("setNetworkSelectionMode failed.");
                res = LE_FAULT;
            }

            res = le_sem_WaitWithTimeOut(taf_RadioNetworkResponseCallback::selModeSem, timeToWait);
            if (res != LE_OK)
            {
                LE_ERROR("Wait semaphore timeout.");
                res = LE_TIMEOUT;
            }

            if (taf_RadioNetworkResponseCallback::selModeRes != LE_OK)
            {
                LE_ERROR("Error response when setting network selection mode.");
                res = LE_FAULT;
            }

            taf_radio_ManualSelectionHandlerFunc_t handlerFunc = (taf_radio_ManualSelectionHandlerFunc_t)cmdReq->handlerFuncPtr;
            if (handlerFunc != nullptr)
            {
                LE_DEBUG("Handler function:%p, result:%d", handlerFunc, res);
                handlerFunc(res, cmdReq->contextPtr);
            }
            else
            {
                LE_WARN("No handler function, result:%d", res);
            }
            break;
        }
        case TAF_RADIO_CMD_TYPE_ASYNC_NETWORK_SCAN:
        {
            TAF_ERROR_IF_RET_NIL(phoneId > tafRadio.networkManagers.size(),
                "Invalid para(phoneId:%d > %" PRIuS ")", phoneId, tafRadio.networkManagers.size());

            auto networkManager = tafRadio.networkManagers[phoneId - 1];
            TAF_ERROR_IF_RET_NIL(networkManager == NULL,
                "Invalid network manager(null ptr, phoneId:%d)", phoneId);

            auto networkListener = tafRadio.networkListeners[phoneId - 1];
            TAF_ERROR_IF_RET_NIL(networkListener == NULL,
                "Invalid network listener(null ptr, phoneId:%d)", phoneId);
            networkListener->opInfos.clear();

            auto status = networkManager->registerListener(networkListener);
            TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS,
                "Fail to register listener with phoneId:%d)", phoneId);

            telux::tel::NetworkScanInfo info;
            info.scanType = telux::tel::NetworkScanType::ALL_RATS;
            if (networkManager->performNetworkScan(info,
                taf_RadioPerformNetworkScanCallback::performNetworkScanResponse) !=
                telux::common::Status::SUCCESS)
            {
                LE_ERROR("Call sdk function failed");
                status = networkManager->deregisterListener(networkListener);
                TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS,
                    "Fail to deregister listener with phoneId:%d)", phoneId);
            };

            res = le_sem_WaitWithTimeOut(taf_RadioPerformNetworkScanCallback::semaphore,
                timeToWait);
            if (res != LE_OK || taf_RadioPerformNetworkScanCallback::result != LE_OK)
            {
                LE_ERROR("Perform network scan failed.");
                status = networkManager->deregisterListener(networkListener);
                TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS,
                    "Fail to deregister listener with phoneId:%d)", phoneId);
            }

            le_clk_Time_t timeToScan = {TAF_RADIO_SCAN_INTERVAL, 0};
            res = le_sem_WaitWithTimeOut(networkListener->semaphore, timeToScan);
            if (res != LE_OK)
            {
                LE_ERROR("Network scan timeout");
                status = networkManager->deregisterListener(networkListener);
                TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS,
                    "Fail to deregister listener with phoneId:%d)", phoneId);
            };

            status = networkManager->deregisterListener(networkListener);
            TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS,
                "Fail to deregister listener with phoneId:%d)", phoneId);

            TAF_ERROR_IF_RET_NIL(networkListener->opInfos.size() == 0,
                "Phone%d has no operators after scanning", phoneId);

            taf_RadioScanOpList_t* opsList = (taf_RadioScanOpList_t*)le_mem_ForceAlloc(tafRadio.scanOpsListPool);
            TAF_ERROR_IF_RET_NIL(opsList == NULL, "Null ptr(opsList)");

            opsList->scanOpList = LE_SLS_LIST_INIT;
            opsList->safeRefList = LE_SLS_LIST_INIT;
            opsList->currPtr = NULL;

            taf_RadioScanOp_t* opPtr;
            for (auto info : networkListener->opInfos)
            {
                opPtr = (taf_RadioScanOp_t*)le_mem_ForceAlloc(tafRadio.scanOpPool);
                le_utf8_Copy(opPtr->name, info.getName().c_str(), TAF_RADIO_NETWORK_NAME_MAX_LEN, NULL);
                le_utf8_Copy(opPtr->mcc, info.getMcc().c_str(), TAF_RADIO_MCC_BYTES, NULL);
                le_utf8_Copy(opPtr->mnc, info.getMnc().c_str(), TAF_RADIO_MNC_BYTES, NULL);
                opPtr->status.inUse = info.getStatus().inUse;
                opPtr->status.roaming = info.getStatus().roaming;
                opPtr->status.forbidden = info.getStatus().forbidden;
                opPtr->status.preferred = info.getStatus().preferred;
                opPtr->rat = info.getRat();
                opPtr->link = LE_SLS_LINK_INIT;
                le_sls_Queue(&(opsList->scanOpList), &(opPtr->link));
            }

            taf_radio_ScanInformationListRef_t listRef =
                (taf_radio_ScanInformationListRef_t)le_ref_CreateRef(tafRadio.scanOpListRefMap, (void*)opsList);
            taf_radio_CellularNetworkScanHandlerFunc_t handlerFunc =
                (taf_radio_CellularNetworkScanHandlerFunc_t)cmdReq->handlerFuncPtr;
            if (handlerFunc != nullptr)
            {
                LE_DEBUG("Handler function:%p listRef:%p", handlerFunc, listRef);
                handlerFunc(listRef, cmdReq->contextPtr);
            }
            else
            {
                LE_WARN("No handler function");
            }
            break;
        }
        case TAF_RADIO_CMD_TYPE_ASYNC_PCI_NETWORK_SCAN:
        {
            taf_radio_PciScanInformationListRef_t listRef =
                taf_pa_radio_PerformPciNetworkScan(cmdReq->ratMask, cmdReq->phoneId);
            taf_radio_PciNetworkScanHandlerFunc_t handlerFunc =
                (taf_radio_PciNetworkScanHandlerFunc_t)cmdReq->handlerFuncPtr;
            if (handlerFunc)
            {
                LE_DEBUG("Handler function:%p", handlerFunc);
                handlerFunc(listRef, phoneId, cmdReq->contextPtr);
            }
            else
            {
                LE_WARN("No handler function.");
            }
            break;
        }
    }
}

/*======================================================================

 FUNCTION        taf_Radio::RadioCmdThread

 DESCRIPTION     Radio command thread for handling asynchronous request.

 DEPENDENCIES    The initialization of Radio.

 PARAMETERS      [IN] void* contextPtr: Context pointer.

 RETURN VALUE    void*
                     NULL: Success.

 SIDE EFFECTS

======================================================================*/
void* taf_Radio::RadioCmdThread(void* contextPtr)
{
    le_event_AddHandler("RadioProcCmdHandler", radioCmdEvId, RadioProcCmdHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register Listener.
 */
//--------------------------------------------------------------------------------------------------
void RegisterListener
(
    void
)
{
    auto &tafRadio = taf_Radio::GetInstance();

    telux::common::Status status = tafRadio.phoneManager->registerListener(tafRadio.phoneListener);
    if (status != telux::common::Status::SUCCESS)
    {
        LE_ERROR("Failed to register phone listener.");
    }

    for (size_t i = 1; i <= tafRadio.phones.size(); i++)
    {
        int slot = tafRadio.phoneManager->getSlotIdFromPhoneId(i);
        if (tafRadio.imsServingSystemMgrs[(SlotId)slot] != nullptr &&
            tafRadio.imsServSysListeners[(SlotId)slot] != nullptr)
        {
            status = tafRadio.imsServingSystemMgrs[(SlotId)slot]->registerListener(
                tafRadio.imsServSysListeners[(SlotId)slot]);
            if (status != telux::common::Status::SUCCESS)
            {
                LE_ERROR("Failed to register IMS serving system listener.");
            }
        }

        tafRadio.dataServSysManagers[(SlotId)slot]->registerListener(
            tafRadio.dataServSysListeners[(SlotId)slot]);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Deregister Listener.
 */
//--------------------------------------------------------------------------------------------------
void DeregisterListener
(
    void
)
{
    auto &tafRadio = taf_Radio::GetInstance();

    tafRadio.phoneManager->removeListener(tafRadio.phoneListener);

    for (size_t i = 1; i <= tafRadio.phones.size(); i++)
    {
        int slot = (int)tafRadio.phoneManager->getSlotIdFromPhoneId(i);
        if (tafRadio.imsServingSystemMgrs[(SlotId)slot] != nullptr &&
            tafRadio.imsServSysListeners[(SlotId)slot] != nullptr)
        {
            tafRadio.imsServingSystemMgrs[(SlotId)slot]->deregisterListener(
                tafRadio.imsServSysListeners[(SlotId)slot]);
        }

        if (tafRadio.dataServSysManagers[(SlotId)slot] != nullptr &&
            tafRadio.dataServSysListeners[(SlotId)slot] != nullptr)
        {
            tafRadio.dataServSysManagers[(SlotId)slot]->deregisterListener(
                tafRadio.dataServSysListeners[(SlotId)slot]);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for power state changes.
 */
//--------------------------------------------------------------------------------------------------
void PowerStateChangeHandler
(
    taf_pm_State_t state, ///< [IN] PM state.
    void* contextPtr      ///< [IN] Handler context.
)
{
    le_result_t result;
    if (state == TAF_PM_STATE_RESUME)
    {
        LE_INFO("Power state change to RESUME");
        RegisterListener();
        result = taf_pa_radio_EnableIndication();
        TAF_ERROR_IF_RET_NIL(result != LE_OK, "Enable indication falied.");
    }
    else if (state == TAF_PM_STATE_SUSPEND)
    {
        LE_INFO("Power state change to SUSPEND");
        DeregisterListener();
        result = taf_pa_radio_DisableIndication();
        TAF_ERROR_IF_RET_NIL(result != LE_OK, "Disable indication falied.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Subsystem completes intialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::onInitCompleted
(
    telux::common::ServiceStatus status ///< [IN] Service status.
)
{
    std::lock_guard<std::mutex> lock(mtx);
    if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE)
    {
        LE_INFO("Status : available.");
        subSystemStatusUpdated = true;
        conVar.notify_all();
    }
    else
    {
        LE_ERROR("Invalid status: %d", (int)status);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::Init(void)
{
    // 1. Initiate the semaphore
    taf_RadioNetworkResponseCallback::selModeSem = le_sem_Create("taf_RadioSelModeSem", 0);
    taf_RadioNetworkResponseCallback::prefNetSem = le_sem_Create("taf_RadioPrefNetSem", 0);
    taf_RadioPreferredNetworksResponseCallback::semaphore =
        le_sem_Create("taf_RadioPrefNetworkRespCbSem", 0);
    taf_RadioServingSystemResponseCallback::semaphore = le_sem_Create("taf_RadioSrvSysSem", 0);
    taf_RadioRatPreferenceResponseCallback::semaphore =
        le_sem_Create("taf_RadioRatPrefRespCbSem", 0);
    taf_RadioCellInfoCallback::semaphore = le_sem_Create("taf_RadioCellInfoCbSem", 0);
    taf_RadioPerformNetworkScanCallback::semaphore = le_sem_Create("taf_RadioPerfNetScanCbSem", 0);
    taf_RadioImsServSysCallback::semaphore = le_sem_Create("taf_RadioImsServSysCbSem", 0);
    taf_RadioImsSettingCallback::semaphore = le_sem_Create("taf_RadioImsSettingCbSem", 0);
    taf_RadioConfigureSignalStrengthCallback::semaphore = le_sem_Create("taf_RadioSigCfgCbSem", 0);

    imsRegStatusChangeId = le_event_CreateIdWithRefCounting("ImsRegStatus");
    opModeChangeId = le_event_CreateIdWithRefCounting("OpMode");
    netRegStateEvId = le_event_CreateIdWithRefCounting("NetRegState");
    packSwStateEvId = le_event_CreateIdWithRefCounting("PackSwState");
    imsStatusChangeId = le_event_CreateIdWithRefCounting("ImsStatus");
    gsmSsChangeEvId = le_event_CreateIdWithRefCounting("GsmSsChange");
    umtsSsChangeEvId = le_event_CreateIdWithRefCounting("UmtsSsChange");
    cdmaSsChangeEvId = le_event_CreateIdWithRefCounting("CdmaSsChange");
    lteSsChangeEvId = le_event_CreateIdWithRefCounting("LteSsChange");
    nr5gSsChangeEvId = le_event_CreateIdWithRefCounting("Nr5gSsChange");

    // 2. Initiate the memory pool
    prefOpsListPool = le_mem_InitStaticPool(prefOpsListPool,
        TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM, sizeof(taf_RadioPrefOpList_t));
    prefOpPool = le_mem_InitStaticPool(prefOpPool, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM,
        sizeof(taf_RadioPrefOp_t));
    prefOpSafeRefPool = le_mem_InitStaticPool(prefOpSafeRefPool,
        TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM, sizeof(taf_RadioPrefOpSafeRef_t));
    scanOpsListPool = le_mem_InitStaticPool(scanOpsListPool,
        TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM, sizeof(taf_RadioScanOpList_t));
    scanOpPool = le_mem_InitStaticPool(scanOpPool, TAF_RADIO_SCAN_OPERATORS_MAX_NUM,
        sizeof(taf_RadioScanOp_t));
    scanOpSafeRefPool = le_mem_InitStaticPool(scanOpSafeRefPool, TAF_RADIO_SCAN_OPERATORS_MAX_NUM,
        sizeof(taf_RadioScanOpSafeRef_t));
    metricsPool = le_mem_InitStaticPool(metricsPool, TAF_RADIO_METRICS_MAX_NUM,
        sizeof(taf_RadioSignalMetrics_t));
    ngbrCellsPool = le_mem_InitStaticPool(ngbrCellsPool, TAF_RADIO_NEIGHBOR_CELLS_MAX_NUM,
        sizeof(taf_RadioNgbrCells_t));
    ngbrCellInfoPool = le_mem_InitStaticPool(ngbrCellInfoPool, TAF_RADIO_NEIGHBOR_CELL_INFO_MAX_NUM,
        sizeof(taf_RadioNgbrCellInfo_t));
    ngbrCellInfoSafeRefPool = le_mem_InitStaticPool(ngbrCellInfoSafeRefPool,
        TAF_RADIO_NEIGHBOR_CELL_INFO_MAX_NUM, sizeof(taf_RadioNgbrCellInfoSafeRef_t));
    phonePool = le_mem_InitStaticPool(phonePool, TAF_RADIO_PHONE_NUM, sizeof(uint8_t));

    opModeChangePool = le_mem_CreatePool("opModeChangePool", sizeof(taf_radio_OpMode_t));
    netRegStatePool = le_mem_CreatePool("netRegStatePool", sizeof(taf_radio_NetRegStateInd_t));
    packSwStatePool = le_mem_CreatePool("packSwStatePool", sizeof(taf_radio_NetRegStateInd_t));
    imsStatusChangePool = le_mem_CreatePool("imsStatusChangePool", sizeof(taf_RadioImsStatus_t));
    ssChangePool = le_mem_CreatePool("ssChangePool", sizeof(taf_RadioSsInd_t));


    // 3. Initiate the reference map.
    prefOpListRefMap = le_ref_InitStaticMap(prefOpListRefMap,TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM);
    prefOpSafeRefMap = le_ref_InitStaticMap(prefOpSafeRefMap, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM);
    scanOpListRefMap = le_ref_InitStaticMap(scanOpListRefMap, TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM);
    scanOpSafeRefMap = le_ref_InitStaticMap(scanOpSafeRefMap, TAF_RADIO_SCAN_OPERATORS_MAX_NUM);
    metricsRefMap = le_ref_InitStaticMap(metricsRefMap, TAF_RADIO_METRICS_MAX_NUM);
    ngbrCellsRefMap = le_ref_InitStaticMap(ngbrCellsRefMap, TAF_RADIO_NEIGHBOR_CELLS_MAX_NUM);
    ngbrCellInfoSafeRefMap = le_ref_InitStaticMap(ngbrCellInfoSafeRefMap,
        TAF_RADIO_NEIGHBOR_CELL_INFO_MAX_NUM);
    imsRefMap = le_ref_InitStaticMap(imsRefMap, TAF_RADIO_PHONE_NUM);
    netStatusRefMap = le_ref_InitStaticMap(netStatusRefMap, TAF_RADIO_PHONE_NUM);
    for (uint8_t phoneId = 1; phoneId <= TAF_RADIO_PHONE_NUM; phoneId++)
    {
        uint8_t* phonePtr = (uint8_t*)le_mem_ForceAlloc(phonePool);
        *phonePtr = phoneId;
        imsRefs[phoneId - 1] = (taf_radio_ImsRef_t)le_ref_CreateRef(imsRefMap, (void*)phonePtr);
        netStatusRefs[phoneId - 1] =
            (taf_radio_NetStatusRef_t)le_ref_CreateRef(netStatusRefMap, (void*)phonePtr);
        taf_pa_radio_SetReference(phoneId, netStatusRefs[phoneId - 1]);
    }

    // 4. Get the PhoneFactory, dataFactory and PhoneManager instances
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    auto &dataFactory = telux::data::DataFactory::getInstance();
    std::promise<telux::common::ServiceStatus> promPhone;
    phoneManager = phoneFactory.getPhoneManager([&](telux::common::ServiceStatus svcStatus)
        {
            promPhone.set_value(svcStatus);
        });

    // 5. Check if telephony subsystem is ready
    bool isReady = false;
    if (!phoneManager)
    {
        LE_FATAL("Invalid phone manager.");
    }
    else
    {
        std::future<telux::common::ServiceStatus> initFuture = promPhone.get_future();
        std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
            TAF_RADIO_SUBSYSTEM_TIMEOUT));
        telux::common::ServiceStatus serviceStatus;
        if (std::future_status::timeout == waitStatus)
        {
            LE_FATAL("Timeout waiting for telephony susbsytem.");
        }
        else
        {
            serviceStatus = initFuture.get();
            if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
            {
                LE_FATAL("Fail to initiate telephony susbsytem.");
            }
            else
            {
                LE_INFO("Telephony subsystem is ready.");
                isReady = true;
            }
        }

        phoneListener = std::make_shared<taf_RadioPhoneListener>();

        // 6. Instantiate Phone
        std::vector<int> phoneIds;
        telux::common::Status status = phoneManager->getPhoneIds(phoneIds);
        if (status == telux::common::Status::SUCCESS)
        {
            for (size_t index = 1; index <= phoneIds.size(); index++)
            {
                auto phone = phoneManager->getPhone(index);
                if (phone != nullptr)
                {
                    phones.emplace_back(phone);
                }

                // 7. Initialize network subsystem.
                isReady = false;
                std::promise<telux::common::ServiceStatus> promNet;
                auto networkManager =
                    telux::tel::PhoneFactory::getInstance().getNetworkSelectionManager(index,
                    [&](telux::common::ServiceStatus svcStatus)
                    {
                        promNet.set_value(svcStatus);
                    });
                if (!networkManager)
                {
                    LE_FATAL("Invalid network manager.");
                }
                else
                {
                    std::future<telux::common::ServiceStatus> initFuture = promNet.get_future();
                    std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                        TAF_RADIO_SUBSYSTEM_TIMEOUT));
                    if (std::future_status::timeout == waitStatus)
                    {
                        LE_FATAL("Timeout waiting for network susbsytem.");
                    }
                    else
                    {
                        serviceStatus = initFuture.get();
                        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
                        {
                            LE_FATAL("Fail to initiate network susbsytem.");
                        }
                        else
                        {
                            isReady = true;
                            LE_INFO("%" PRIuS " network subsystem is ready.", index);
                            networkManagers.emplace_back(networkManager);
                            auto networkListener =
                                std::make_shared<taf_RadioNetworkSelectionListener>();
                            networkListener->semaphore = le_sem_Create("networkListenerSem", 0);
                            networkListeners.emplace_back(networkListener);
                        }
                    }
                }

                // 8. Initialize serving subsystem.
                isReady = false;
                std::promise<telux::common::ServiceStatus> promSrv;
                auto servingSystemManager =
                     telux::tel::PhoneFactory::getInstance().getServingSystemManager(index,
                    [&](telux::common::ServiceStatus svcStatus)
                    {
                        promSrv.set_value(svcStatus);
                    });
                if (!servingSystemManager)
                {
                    LE_FATAL("Invalid serving manager.");
                }
                if (servingSystemManager != nullptr)
                {
                    std::future<telux::common::ServiceStatus> initFuture = promSrv.get_future();
                    std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                        TAF_RADIO_SUBSYSTEM_TIMEOUT));
                    if (std::future_status::timeout == waitStatus)
                    {
                        LE_FATAL("Timeout waiting for serving susbsytem.");
                    }
                    else
                    {
                        serviceStatus = initFuture.get();
                        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
                        {
                            LE_FATAL("Fail to initiate serving susbsytem.");
                        }
                        else
                        {
                            isReady = true;
                            LE_INFO("%" PRIuS " serving subsystem is ready.", index);
                            servingSystemManagers.emplace_back(servingSystemManager);
                        }
                    }
                }
            }
        }

        // 9. Instantiate RadioCallback
        signalStrengthCb = std::make_shared<taf_RadioSignalStrengthCallback>();
        voiceSrvStateCb = std::make_shared<taf_RadioVoiceServiceStateCallback>();
        setOperatingModeCb = std::make_shared<taf_RadioSetOperatingModeCallback>();
        getOperatingModeCb = std::make_shared<taf_RadioGetOperatingModeCallback>();
        cellularCapsCb = std::make_shared<taf_RadioCellularCapsCallback>();
        voiceSrvStateCb->semaphore = le_sem_Create("taf_RadioVoiceSrvStateCbSem", 0);
        signalStrengthCb->semaphore = le_sem_Create("taf_RadioSgnStrengthCbSem", 0);
        setOperatingModeCb->semaphore = le_sem_Create("taf_RadioSetOpModeCbSem", 0);
        getOperatingModeCb->semaphore = le_sem_Create("taf_RadioGetOpModeCbSem", 0);
        cellularCapsCb->semaphore = le_sem_Create("CellCapsCbSem", 0);
        dataInfoCb.semaphore = le_sem_Create("dataInfoCbSem", 0);
        opNameCb.semaphore = le_sem_Create("OopNameCbSem", 0);
        opNameCb.longOpNamePtr[TAF_RADIO_NETWORK_NAME_MAX_LEN] = {0};
        opNameCb.shortOpNamePtr[TAF_RADIO_NETWORK_NAME_MAX_LEN] = {0};

        for (size_t index = 1; index <= phoneIds.size(); index++)
        {
            // 10. Initialize data serving subsystem.
            isReady = false;
            int slot = (int)phoneManager->getSlotIdFromPhoneId(index);
            telux::common::ServiceStatus subSystemStatus =
                telux::common::ServiceStatus::SERVICE_FAILED;
            auto initCb = std::bind(&taf_Radio::onInitCompleted, this, std::placeholders::_1);
            auto servingSystemMgr = dataFactory.getServingSystemManager((SlotId)slot, initCb);
            if (servingSystemMgr)
            {
                std::unique_lock<std::mutex> uLock(mtx);
                if (!conVar.wait_for(uLock, std::chrono::seconds(TAF_RADIO_SUBSYSTEM_TIMEOUT),
                    [this]{return this->subSystemStatusUpdated;}))
                {
                    LE_FATAL("Timeout waiting for data serving susbsytem");
                }

                subSystemStatus = servingSystemMgr->getServiceStatus();
                if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    isReady = true;
                    LE_INFO("%d data serving subsystem is ready.", slot);
                    dataServSysManagers.emplace((SlotId)slot, servingSystemMgr);
                    auto listener = std::make_shared<taf_RadioDataServSysListener>((SlotId)slot);
                    dataServSysListeners.emplace((SlotId)slot, listener);
                }
                else
                {
                    LE_FATAL("Fail to init Data Serving subsystem");
                }
            }
            else
            {
                LE_ERROR("Failed to get Data Serving System instance.");
            }

            // 11. Initialize IMS serving subsystem.
            isReady = false;
            std::promise<telux::common::ServiceStatus> prom;
            auto imsServingSystemMgr = phoneFactory.getImsServingSystemManager(
                (SlotId)slot,[&](telux::common::ServiceStatus svcStatus)
                {
                    if (svcStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                    {
                        prom.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                    }
                    else
                    {
                        prom.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
                    }
                });

            if (!imsServingSystemMgr)
            {
                LE_ERROR("Failed to get IMS Serving System instance.");
            }
            else
            {
                telux::common::ServiceStatus imsServSysMgrStatus =
                    imsServingSystemMgr->getServiceStatus();
                if (imsServSysMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    LE_INFO("IMS serving subsystem wait to be ready...");
                    std::future<telux::common::ServiceStatus> initFuture = prom.get_future();
                    std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                        TAF_RADIO_SUBSYSTEM_TIMEOUT));
                    if (std::future_status::timeout == waitStatus)
                    {
                        LE_FATAL ("Timeout waiting for IMS serving susbsytem");
                    }
                    else
                    {
                        imsServSysMgrStatus = initFuture.get();
                    }
                }
                if (imsServSysMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    isReady = true;
                    imsServingSystemMgrs.emplace((SlotId)slot, imsServingSystemMgr);
                    LE_INFO("%d IMS serving subsystem is ready.", slot);

                    auto listener = std::make_shared<taf_RadioImsServSysListener>((SlotId)slot);
                    imsServSysListeners.emplace((SlotId)slot, listener);
                }
                else
                {
                    LE_FATAL("Fail to init IMS Serving subsystem");
                }
            }

            // 12. Initialize IMS settings subsystem.
            isReady = false;
            std::promise<telux::common::ServiceStatus> promSetting;
            auto imsSettingMgr = phoneFactory.getImsSettingsManager(
                [&](telux::common::ServiceStatus svcStatus)
                {
                    if (svcStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                    {
                        promSetting.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                    }
                    else
                    {
                        promSetting.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
                    }
                });

            if (!imsSettingMgr)
            {
                LE_ERROR("Failed to get IMS Settings instance.");
            }
            else
            {
                telux::common::ServiceStatus imsSettingMgrStatus = imsSettingMgr->getServiceStatus();
                if (imsSettingMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    LE_INFO("IMS setting subsystem wait to be ready...");
                    std::future<telux::common::ServiceStatus> initFuture = promSetting.get_future();
                    std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                        TAF_RADIO_SUBSYSTEM_TIMEOUT));
                    if (std::future_status::timeout == waitStatus)
                    {
                        LE_FATAL ("Timeout waiting for IMS setting susbsytem");
                    }
                    else
                    {
                        imsSettingMgrStatus = initFuture.get();
                    }
                }
                if (imsSettingMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    isReady = true;
                    imsSettingMgrs.emplace((SlotId)slot, imsSettingMgr);
                    LE_INFO("%d IMS setting subsystem is ready.", slot);
                }
                else
                {
                    LE_FATAL("Fail to init IMS Setting subsystem");
                }
            }
        }
    }

    if (isReady)
    {
        // 13. Create and start command thread.
        le_sem_Ref_t radioCmdThreadSem = le_sem_Create("radioCmdThreadSem", 0);
        radioCmdEvId = le_event_CreateId("radioCmd", sizeof(taf_RadioCmdReq_t));
        le_thread_Ref_t radioCmdThreadRef = le_thread_Create("radioCmdThread", RadioCmdThread, (void*)radioCmdThreadSem);
        le_thread_SetStackSize(radioCmdThreadRef, TAF_RADIO_THREAD_STACK_SIZE);
        le_thread_Start(radioCmdThreadRef);
        le_sem_Wait(radioCmdThreadSem);

        // 14. Delete semaphore.
        le_sem_Delete(radioCmdThreadSem);

        // 15. Add power state change handle.
        taf_pm_AddStateChangeHandler(PowerStateChangeHandler, NULL);
        if (taf_pm_GetPowerState() != TAF_PM_STATE_SUSPEND)
        {
            RegisterListener();
            taf_pa_radio_EnableIndication();
        }
    }
}
