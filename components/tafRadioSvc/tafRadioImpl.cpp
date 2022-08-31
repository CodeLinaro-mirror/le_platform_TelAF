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
 * @file       tafRadioImpl.cpp
 * @brief      This file describes the implementation method that radio
 *             service is in use.
 */

#include <cstdlib>
#include <chrono>

#include "tafRadio.hpp"

using namespace std;
using namespace telux::tafsvc;

/*======================================================================

 FUNCTION        taf_radio_CheckMccMnc

 DESCRIPTION     Check whether MCC and MNC are all numbers.

 DEPENDENCIES    None

 PARAMETERS      [IN] const char* mccPtr: The mobile country code.
                 [IN] const char* mncPtr: The mobile network code.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_RadioFunctions::taf_radio_CheckMccMnc(const char* mccPtr, const char* mncPtr)
{
    TAF_ERROR_IF_RET_VAL((mccPtr == nullptr), LE_BAD_PARAMETER,
        "Null ptr(mccPtr)");

    TAF_ERROR_IF_RET_VAL((mncPtr == nullptr), LE_BAD_PARAMETER,
        "Null ptr(mncPtr)");

    size_t mccLen = strlen(mccPtr);
    TAF_ERROR_IF_RET_VAL((mccLen != TAF_RADIO_MCC_LEN), LE_BAD_PARAMETER,
        "Mcc length %d != %d", mccLen, TAF_RADIO_MCC_LEN);

    size_t mncLen = strlen(mncPtr);
    TAF_ERROR_IF_RET_VAL((mncLen < TAF_RADIO_MNC_MIN_LEN || mncLen > TAF_RADIO_MNC_MAX_LEN), LE_BAD_PARAMETER,
        "Mnc error length %d", mncLen);

    for (size_t index = 0; index < mccLen; index++) {
        TAF_ERROR_IF_RET_VAL((mccPtr[index] < '0' || mccPtr[index] > '9'), LE_BAD_PARAMETER,
            "Error char %c in mcc, index %d", mccPtr[index], index);
    }

    for (size_t index = 0; index < mncLen; index++) {
        TAF_ERROR_IF_RET_VAL((mncPtr[index] < '0' || mncPtr[index] > '9'), LE_BAD_PARAMETER,
            "Error char %c in mnc, index %d", mncPtr[index], index);
    }

    return LE_OK;
}

telux::tel::VoiceServiceState taf_RadioPhoneListener::vocSrvState = telux::tel::VoiceServiceState::UNKNOWN;

/*======================================================================

 FUNCTION        taf_RadioPhoneListener::onRadioStateChanged

 DESCRIPTION     This function will be called when radio state changed.

 DEPENDENCIES    Phone listener registers to phone manager

 PARAMETERS      [IN] int phoneId:                  The phone id.
                 [IN] telux::tel::RadioState state: The radio state.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioPhoneListener::onRadioStateChanged(int phoneId, telux::tel::RadioState state)
{
    LE_DEBUG("<SDK Listener> taf_RadioPhoneListener --> onRadioStateChanged");
    LE_DEBUG("phoneId = %d", phoneId);
    LE_DEBUG("RadioState = %d", (int)state);
}

/*======================================================================

 FUNCTION        taf_RadioPhoneListener::onVoiceRadioTechnologyChanged

 DESCRIPTION     This function will be called when RAT changed.

 DEPENDENCIES    Phone listener registers to phone manager

 PARAMETERS      [IN] int phoneId:                                 The phone id.
                 [IN] telux::tel::RadioTechnology radioTechnology: The radio technology.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioPhoneListener::onVoiceRadioTechnologyChanged(int phoneId, telux::tel::RadioTechnology radioTechnology)
{
    LE_DEBUG("<SDK Listener> taf_RadioPhoneListener --> onVoiceRadioTechnologyChanged");
    LE_DEBUG("phoneId = %d", phoneId);
    LE_DEBUG("radioTechnology = %d", (int)radioTechnology);

    auto &tafRadio = taf_Radio::GetInstance();
    taf_radio_RatChangeInd_t* reportPtr = (taf_radio_RatChangeInd_t*)le_mem_ForceAlloc(tafRadio.ratChangePool);
    reportPtr->rat = (taf_radio_Rat_t)radioTechnology;
    reportPtr->phoneId = phoneId;
    le_event_ReportWithRefCounting(tafRadio.ratChangeEvId, (void*)reportPtr);
}

/*======================================================================

 FUNCTION        taf_RadioPhoneListener::onVoiceServiceStateChanged

 DESCRIPTION     This function will be called when network registration state changed.

 DEPENDENCIES    Phone listener registers to phone manager

 PARAMETERS      [IN] int phoneId: The phone id.
                 [IN] const std::shared_ptr<telux::tel::VoiceServiceInfo> &srvInfo:
                          A pointer with network service information.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioPhoneListener::onVoiceServiceStateChanged
(
    int phoneId,
    const std::shared_ptr<telux::tel::VoiceServiceInfo> &srvInfo
)
{
    LE_DEBUG("<SDK Listener> taf_RadioPhoneListener --> onVoiceServiceStateChanged");
    LE_DEBUG("phoneId = %d", phoneId);
    if (srvInfo != nullptr) {
        auto &tafRadio = taf_Radio::GetInstance();
        telux::tel::VoiceServiceState state = srvInfo->getVoiceServiceState();
        if (state != vocSrvState) {
            taf_radio_NetRegStateInd_t* netRegStateIndPtr =
                (taf_radio_NetRegStateInd_t*)le_mem_ForceAlloc(tafRadio.netRegStatePool);
            netRegStateIndPtr->state = (taf_radio_NetRegState_t)state;
            netRegStateIndPtr->phoneId = phoneId;
            vocSrvState = state;
            le_event_ReportWithRefCounting(tafRadio.netRegStateEvId, (void*)netRegStateIndPtr);
        }

        telux::tel::VoiceServiceDenialCause cause = srvInfo->getVoiceServiceDenialCause();
        if (cause != telux::tel::VoiceServiceDenialCause::GENERAL) {
            taf_radio_NetRegRejInd_t* netRegRejIndPtr =
                (taf_radio_NetRegRejInd_t*)le_mem_ForceAlloc(tafRadio.netRegRejectPool);
            netRegRejIndPtr->cause = (taf_radio_NetRejCause_t)cause;
            netRegRejIndPtr->phoneId = phoneId;
            le_event_ReportWithRefCounting(tafRadio.netRegRejectEvId, (void*)netRegRejIndPtr);
        }
    }
}

/*======================================================================

 FUNCTION        taf_RadioPhoneListener::onSignalStrengthChanged

 DESCRIPTION     This function will be called when signal strength changed.

 DEPENDENCIES    Phone listener registers to phone manager

 PARAMETERS      [IN] int phoneId: The phone id.
                 [IN] std::shared_ptr<telux::tel::SignalStrength> signalStrength:
                          A pointer with signal strength information.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioPhoneListener::onSignalStrengthChanged
(
    int phoneId,
    std::shared_ptr<telux::tel::SignalStrength> signalStrength
)
{
    LE_DEBUG("<SDK Listener> taf_RadioPhoneListener --> onSignalStrengthChanged");
    LE_DEBUG("phoneId = %d", phoneId);
    if (signalStrength != nullptr) {
        auto &tafRadio = taf_Radio::GetInstance();
        if (signalStrength->getGsmSignalStrength() != nullptr) {
            taf_RadioSignalStrengthInfo_t* gsmSsPtr =
                (taf_RadioSignalStrengthInfo_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
            gsmSsPtr->strength = signalStrength->getGsmSignalStrength()->getGsmSignalStrength();
            gsmSsPtr->ber = signalStrength->getGsmSignalStrength()->getGsmBitErrorRate();
            gsmSsPtr->dbm = signalStrength->getGsmSignalStrength()->getDbm();
            gsmSsPtr->ta = signalStrength->getGsmSignalStrength()->getTimingAdvance();
            gsmSsPtr->level = signalStrength->getGsmSignalStrength()->getLevel();
            le_event_ReportWithRefCounting(tafRadio.gsmSsChangeEvId, (void*)gsmSsPtr);
        }

        if (signalStrength->getCdmaSignalStrength() != nullptr) {
            taf_RadioSignalStrengthInfo_t* cdmaSsPtr =
                (taf_RadioSignalStrengthInfo_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
            cdmaSsPtr->dbm = signalStrength->getCdmaSignalStrength()->getDbm();
            cdmaSsPtr->cdma.cdmaEcio = signalStrength->getCdmaSignalStrength()->getCdmaEcio();
            cdmaSsPtr->cdma.evdoEcio = signalStrength->getCdmaSignalStrength()->getEvdoEcio();
            cdmaSsPtr->snr = signalStrength->getCdmaSignalStrength()->getEvdoSignalNoiseRatio();
            cdmaSsPtr->level = signalStrength->getCdmaSignalStrength()->getLevel();
            le_event_ReportWithRefCounting(tafRadio.cdmaSsChangeEvId, (void*)cdmaSsPtr);
        }

        if (signalStrength->getLteSignalStrength() != nullptr) {
            taf_RadioSignalStrengthInfo_t* lteSsPtr =
                (taf_RadioSignalStrengthInfo_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
            lteSsPtr->strength = signalStrength->getLteSignalStrength()->getLteSignalStrength();
            lteSsPtr->dbm = signalStrength->getLteSignalStrength()->getDbm();
            lteSsPtr->lte.rsrq = signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality();
            lteSsPtr->snr = signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr() * TAF_RADIO_LTE_SNR_RATIO;
            lteSsPtr->lte.cqi = signalStrength->getLteSignalStrength()->getLteChannelQualityIndicator();
            lteSsPtr->ta = signalStrength->getLteSignalStrength()->getTimingAdvance();
            lteSsPtr->level = signalStrength->getLteSignalStrength()->getLevel();
            le_event_ReportWithRefCounting(tafRadio.lteSsChangeEvId, (void*)lteSsPtr);
        }

        if (signalStrength->getWcdmaSignalStrength() != nullptr) {
            taf_RadioSignalStrengthInfo_t* wcdmaPtr =
                (taf_RadioSignalStrengthInfo_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
            wcdmaPtr->strength = signalStrength->getWcdmaSignalStrength()->getSignalStrength();
            wcdmaPtr->dbm = signalStrength->getWcdmaSignalStrength()->getDbm();
            wcdmaPtr->ber = signalStrength->getWcdmaSignalStrength()->getBitErrorRate();
            wcdmaPtr->level = signalStrength->getWcdmaSignalStrength()->getLevel();
            le_event_ReportWithRefCounting(tafRadio.wcdmaSsChangeEvId, (void*)wcdmaPtr);
        }

        if (signalStrength->getTdscdmaSignalStrength() != nullptr) {
            taf_RadioSignalStrengthInfo_t* tdscdmaPtr =
                (taf_RadioSignalStrengthInfo_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
            tdscdmaPtr->dbm = signalStrength->getTdscdmaSignalStrength()->getRscp();
            tdscdmaPtr->tdscdma.rscp = signalStrength->getTdscdmaSignalStrength()->getRscp();
            le_event_ReportWithRefCounting(tafRadio.tdscdmaSsChangeEvId, (void*)tdscdmaPtr);
        }
    }
}

/*======================================================================

 FUNCTION        taf_RadioNetworkSelectionListener::onSelectionModeChanged

 DESCRIPTION     This function will be called when seletion state changed.

 DEPENDENCIES    Network selsection listener registers to network manager.

 PARAMETERS      [IN] telux::tel::NetworkSelectionMode mode:
                          The network selsetion mode.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioNetworkSelectionListener::onSelectionModeChanged(telux::tel::NetworkSelectionMode mode)
{
    LE_DEBUG("<SDK Listener> taf_RadioNetworkSelectionListener --> onSelectionModeChanged");
    LE_DEBUG("NetworkSelectionMode = %d", (int)mode);
}

/*======================================================================

 FUNCTION        taf_RadioServingSystemListener::onServiceDomainPreferenceChanged

 DESCRIPTION     This function will be called when service domian preference changed.

 DEPENDENCIES    Serving system listener registers to serving system manager.

 PARAMETERS      [IN] telux::tel::ServiceDomainPreference preference:
                          The service domian preference.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioServingSystemListener::onServiceDomainPreferenceChanged(telux::tel::ServiceDomainPreference preference)
{
    LE_DEBUG("<SDK Listener> taf_RadioServingSystemListener --> onServiceDomainPreferenceChanged");
    LE_DEBUG("ServiceDomainPreference = %d", (int)preference);

    auto &tafRadio = taf_Radio::GetInstance();
    taf_radio_ServiceDomainState_t* statePtr =
        (taf_radio_ServiceDomainState_t*)le_mem_ForceAlloc(tafRadio.packetSwChangePool);
    *statePtr = (taf_radio_ServiceDomainState_t)preference;
    le_event_ReportWithRefCounting(tafRadio.packetSwChangeEvId, (void*)statePtr);
}

/*======================================================================

 FUNCTION        taf_RadioSubscriptionListener::onSubscriptionInfoChanged

 DESCRIPTION     This function will be called when subscription infomation
                 changed.

 DEPENDENCIES    Subscription listener registers to subscription manager.

 PARAMETERS      [IN] std::shared_ptr<telux::tel::ISubscription> subscription:
                          A pointer with subscription infomation.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioSubscriptionListener::onSubscriptionInfoChanged(std::shared_ptr<telux::tel::ISubscription> subscription)
{
    LE_DEBUG("<SDK Listener> taf_RadioSubscriptionListener --> onSubscriptionInfoChanged");
    if (!subscription) {
        LE_ERROR("subscription is null");
    }
}

/*======================================================================

 FUNCTION        taf_RadioSubscriptionListener::onNumberOfSubscriptionsChanged

 DESCRIPTION     This function will be called when the subscription count changed.

 DEPENDENCIES    Subscription listener registers to subscription manager.

 PARAMETERS      [IN] int count: The count of the subscriptions.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioSubscriptionListener::onNumberOfSubscriptionsChanged(int count)
{
    LE_DEBUG("<SDK Listener> taf_RadioSubscriptionListener --> onNumberOfSubscriptionsChanged");
    LE_DEBUG("count = %d", count);
}

void taf_RadioSetOperatingModeCallback::setOperatingModeResponse(telux::common::ErrorCode error)
{
    LE_DEBUG("<SDK Callback> taf_RadioOperatingModeCallback --> setOperatingModeResponse");
    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("Error(%d)", (int)error);
    }

    le_sem_Post(semaphore);
}

/*======================================================================

 FUNCTION        taf_RadioVoiceRadioTechnologyCallback::voiceRadioTechnologyResponse

 DESCRIPTION     The callback function used in getting rat in use.

 DEPENDENCIES    Call the low level function when getting rat in use.

 PARAMETERS      [IN] telux::tel::RadioTechnology radioTechnology:
                          Rat in use.
                 [IN] telux::common::ErrorCode error: Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioVoiceRadioTechnologyCallback::voiceRadioTechnologyResponse
(
    telux::tel::RadioTechnology radioTechnology,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> taf_RadioVoiceRadioTechnologyCallback --> voiceRadioTechnologyResponse");
    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("Error(%d)", (int)error);
    }
    radioTech = radioTechnology;
    le_sem_Post(semaphore);
}

le_sem_Ref_t taf_RadioVoiceServiceStateCallback::semaphore = nullptr;

telux::tel::VoiceServiceState taf_RadioVoiceServiceStateCallback::vocSrvState = telux::tel::VoiceServiceState::UNKNOWN;

/*======================================================================

 FUNCTION        taf_RadioVoiceServiceStateCallback::voiceServiceStateResponse

 DESCRIPTION     The callback function used in getting the service state.

 DEPENDENCIES    Call the low level function when getting the service state.

 PARAMETERS      [IN] std::shared_ptr<telux::tel::VoiceServiceInfo> &serviceInfo:
                          Service state information.
                 [IN] telux::common::ErrorCode error: Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioVoiceServiceStateCallback::voiceServiceStateResponse
(
    const std::shared_ptr<telux::tel::VoiceServiceInfo> &serviceInfo,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> taf_RadioVoiceServiceStateCallback --> voiceServiceStateResponse");
    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("Error(%d)", (int)error);
    }

    vocSrvState = serviceInfo->getVoiceServiceState();

    le_sem_Post(semaphore);
}

/*======================================================================

 FUNCTION        taf_RadioSignalStrengthCallback::signalStrengthResponse

 DESCRIPTION     The callback function used in getting the signal strength.

 DEPENDENCIES    Call the low level function when getting the signal strength.

 PARAMETERS      [IN] std::shared_ptr<telux::tel::SignalStrength> signalStrength:
                          Signal strength information.
                 [IN] telux::common::ErrorCode error: Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioSignalStrengthCallback::signalStrengthResponse
(
    std::shared_ptr<telux::tel::SignalStrength> signalStrength,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> taf_RadioSignalStrengthCallback --> signalStrengthResponse");
    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("Error(%d)", (int)error);
    }

    signalStrengthLevel = telux::tel::SignalStrengthLevel::LEVEL_UNKNOWN;
    telux::tel::SignalStrengthLevel signalLevel = telux::tel::SignalStrengthLevel::LEVEL_UNKNOWN;

    if (signalStrength->getGsmSignalStrength() != nullptr) {
        signalLevel = signalStrength->getGsmSignalStrength()->getLevel();
        signalStrengthLevel = (signalStrengthLevel > signalLevel ? signalStrengthLevel : signalLevel);
        LE_DEBUG("GSM Signal Level: %d", int(signalLevel));
    }

    if (signalStrength->getCdmaSignalStrength() != nullptr) {
        signalLevel = signalStrength->getCdmaSignalStrength()->getLevel();
        signalStrengthLevel = (signalStrengthLevel > signalLevel ? signalStrengthLevel : signalLevel);
        LE_DEBUG("CDMA Signal Level: %d", int(signalLevel));
    }

    if (signalStrength->getLteSignalStrength() != nullptr) {
        signalLevel = signalStrength->getLteSignalStrength()->getLevel();
        signalStrengthLevel = (signalStrengthLevel > signalLevel ? signalStrengthLevel : signalLevel);
        LE_DEBUG("LTE Signal Level: %d", int(signalLevel));
    }

    if (signalStrength->getWcdmaSignalStrength() != nullptr) {
        signalLevel = signalStrength->getWcdmaSignalStrength()->getLevel();
        signalStrengthLevel = (signalStrengthLevel > signalLevel ? signalStrengthLevel : signalLevel);
        LE_DEBUG("WCDMA Signal Level: %d", int(signalLevel));
    }

    if (signalStrength->getNr5gSignalStrength() != nullptr) {
        signalLevel = signalStrength->getNr5gSignalStrength()->getLevel();
        signalStrengthLevel = (signalStrengthLevel > signalLevel ? signalStrengthLevel : signalLevel);
        LE_DEBUG("NR5G Signal Level: %d", int(signalLevel));
    }

    le_sem_Post(semaphore);
}

le_sem_Ref_t taf_RadioNetworkResponseCallback::semNetSelModeRespCb = nullptr;

le_sem_Ref_t taf_RadioNetworkResponseCallback::semPrefNetRespCb = nullptr;

int32_t taf_RadioNetworkResponseCallback::errCode = 0;

telux::common::ErrorCode taf_RadioNetworkResponseCallback::errorCode = telux::common::ErrorCode::SUCCESS;

/*======================================================================

 FUNCTION        taf_RadioNetworkResponseCallback::setNetworkSelectionModeResponseCb

 DESCRIPTION     The callback function used in setting the network selection mode.

 DEPENDENCIES    Call the low level function when setting the network selection mode.

 PARAMETERS      [IN] telux::common::ErrorCode error: Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioNetworkResponseCallback::setNetworkSelectionModeResponseCb(telux::common::ErrorCode error)
{
    LE_DEBUG("<SDK Callback> taf_RadioNetworkResponseCallback --> setNetworkSelectionModeResponseCb");
    errCode = (int32_t)error;
    le_sem_Post(semNetSelModeRespCb);
}

/*======================================================================

 FUNCTION        taf_RadioNetworkResponseCallback::setPreferredNetworksResponseCb

 DESCRIPTION     The callback function used in setting the network preference.

 DEPENDENCIES    Call the low level function when setting the network preference.

 PARAMETERS      [IN] telux::common::ErrorCode error: Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioNetworkResponseCallback::setPreferredNetworksResponseCb(telux::common::ErrorCode error)
{
    LE_DEBUG("<SDK Callback> taf_RadioNetworkResponseCallback --> setPreferredNetworksResponseCb");
    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("Error(%d)", (int)error);
    }

    errorCode = error;

    le_sem_Post(semPrefNetRespCb);
}

bool taf_RadioSelectionModeResponseCallback::isRegModeMannual = true;

le_sem_Ref_t taf_RadioSelectionModeResponseCallback::semaphore = nullptr;

/*======================================================================

 FUNCTION        taf_RadioSelectionModeResponseCallback::selectionModeResponse

 DESCRIPTION     The callback function used in getting register
                 mode(automatic/manual) of radio.

 DEPENDENCIES    Call the low level function when getting the network selection mode.

 PARAMETERS      [IN] telux::tel::NetworkSelectionMode networkSelectionMode:
                          Register mode of radio.
                 [IN] telux::common::ErrorCode error: Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioSelectionModeResponseCallback::selectionModeResponse
(
    telux::tel::NetworkSelectionMode networkSelectionMode,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> taf_RadioSelectionModeResponseCallback --> selectionModeResponse");

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (networkSelectionMode == telux::tel::NetworkSelectionMode::AUTOMATIC) {
            isRegModeMannual = false;
        } else {
            isRegModeMannual = true;
        }
    } else {
        LE_ERROR("Error(%d)", (int)error);
    }

    le_sem_Post(semaphore);
}

std::vector<telux::tel::PreferredNetworkInfo> taf_RadioPreferredNetworksResponseCallback::preferredNetworksInfo;

le_sem_Ref_t taf_RadioPreferredNetworksResponseCallback::semaphore = nullptr;

/*======================================================================

 FUNCTION        taf_RadioPreferredNetworksResponseCallback::preferredNetworksResponse

 DESCRIPTION     The callback function used in getting the network preference.

 DEPENDENCIES    Call the low level function when getting the network selection mode.

 PARAMETERS      [IN] std::vector<telux::tel::PreferredNetworkInfo> preferredNetworks3gppInfo:
                          Non-static network preference infomation.
                 [IN] std::vector<telux::tel::PreferredNetworkInfo> staticPreferredNetworksInfo:
                          Static network preference infomation.
                 [IN] telux::common::ErrorCode error: Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioPreferredNetworksResponseCallback::preferredNetworksResponse
(
    std::vector<telux::tel::PreferredNetworkInfo> preferredNetworks3gppInfo,
    std::vector<telux::tel::PreferredNetworkInfo> staticPreferredNetworksInfo,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> taf_RadioPreferredNetworksResponseCallback --> preferredNetworksResponse");

    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("Error(%d)", (int)error);
    }

    preferredNetworksInfo.assign(preferredNetworks3gppInfo.begin(), preferredNetworks3gppInfo.end());

    le_sem_Post(semaphore);
}

le_sem_Ref_t taf_RadioPerformNetworkScanCallback::semaphore = nullptr;

std::vector<telux::tel::OperatorInfo> taf_RadioPerformNetworkScanCallback::opInfos;

/*======================================================================

 FUNCTION        taf_RadioPerformNetworkScanCallback::performNetworkScanResponse

 DESCRIPTION     The callback function used in getting the network scan information.

 DEPENDENCIES    Call the low level function when getting the network scan information.

 PARAMETERS      [IN] std::vector<telux::tel::OperatorInfo> operatorInfos:
                          Network scan infomation.
                 [IN] telux::common::ErrorCode error: Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioPerformNetworkScanCallback::performNetworkScanResponse
(
    std::vector<telux::tel::OperatorInfo> operatorInfos,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> taf_RadioPerformNetworkScanCallback --> performNetworkScanResponse");

    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("Error(%d)", (int)error);
    }

    opInfos.assign(operatorInfos.begin(), operatorInfos.end());

    le_sem_Post(semaphore);
}

le_sem_Ref_t taf_RadioServiceDomainResponseCallback::semaphore = nullptr;

telux::tel::ServiceDomainPreference taf_RadioServiceDomainResponseCallback::svcDomainPref = telux::tel::ServiceDomainPreference::UNKNOWN;

/*======================================================================

 FUNCTION        taf_RadioServiceDomainResponseCallback::serviceDomainResponse

 DESCRIPTION     The callback function used in getting the service domain preference.

 DEPENDENCIES    Call the low level function when getting the service domain preference.

 PARAMETERS      [IN] telux::tel::ServiceDomainPreference preference:
                          The service domain preference.
                 [IN] telux::common::ErrorCode error: Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioServiceDomainResponseCallback::serviceDomainResponse
(
    telux::tel::ServiceDomainPreference preference,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> taf_RadioServiceDomainResponseCallback --> serviceDomainResponse");

    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("Error(%d)", (int)error);
    }

    svcDomainPref = preference;

    le_sem_Post(semaphore);
}

/*======================================================================

 FUNCTION        taf_RadioServingSystemResponseCallback::servingSystemResponse

 DESCRIPTION     The callback function used in configuring rat preference.

 DEPENDENCIES    Call the low level function when configuring rat preference.

 PARAMETERS      [IN] telux::common::ErrorCode error: Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioServingSystemResponseCallback::servingSystemResponse(telux::common::ErrorCode error)
{
    LE_DEBUG("<SDK Callback> taf_RadioServingSystemResponseCallback --> servingSystemResponse");
    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("Error(%d)", (int)error);
    }
}

le_sem_Ref_t taf_RadioRatPreferenceResponseCallback::semaphore = nullptr;

telux::tel::RatPreference taf_RadioRatPreferenceResponseCallback::ratPref = 0;

/*======================================================================

 FUNCTION        taf_RadioRatPreferenceResponseCallback::ratPreferenceResponse

 DESCRIPTION     The callback function used in getting rat preference.

 DEPENDENCIES    Call the low level function when getting rat preference.

 PARAMETERS      [IN] telux::tel::RatPreference preference:
                          Rat preference.
                 [IN] telux::common::ErrorCode error: Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioRatPreferenceResponseCallback::ratPreferenceResponse
(
    telux::tel::RatPreference preference,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> taf_RadioRatPreferenceResponseCallback --> ratPreferenceResponse");
    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("Error(%d)", (int)error);
    }

    ratPref = preference;

    le_sem_Post(semaphore);
}

le_sem_Ref_t taf_RadioCellInfoCallback::semaphore = nullptr;

taf_RadioCellMetrics_t taf_RadioCellInfoCallback::cellMetrics;

/*======================================================================

 FUNCTION        taf_RadioCellInfoCallback::cellInfoListResponse

 DESCRIPTION     The callback function used in getting cell information list.

 DEPENDENCIES    Call the low level function when getting cell information list.

 PARAMETERS      [IN] std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList:
                          Poniter of the cell information list.
                 [IN] telux::common::ErrorCode error: Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioCellInfoCallback::cellInfoListResponse
(
    std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> taf_RadioCellInfoCallback --> cellInfoListResponse");
    if (error == telux::common::ErrorCode::SUCCESS) {
        cellMetrics.cellRatMask = 0;
        taf_RadioSignalMetrics_t* metricsPtr;
        for (auto cellinfo : cellInfoList) {
            if (cellinfo->getType() == telux::tel::CellType::GSM) {
                cellMetrics.cellRatMask |= TAF_RADIO_CELL_RAT_MASK_GSM;
                auto gsmCellInfo = std::static_pointer_cast<telux::tel::GsmCellInfo>(cellinfo);
                metricsPtr = &cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_GSM];
                metricsPtr->isRegistered = gsmCellInfo->isRegistered();
                metricsPtr->cellId.mcc = gsmCellInfo->getCellIdentity().getMcc();
                metricsPtr->cellId.mnc = gsmCellInfo->getCellIdentity().getMnc();
                metricsPtr->cellId.lac = gsmCellInfo->getCellIdentity().getLac();
                metricsPtr->cellId.cid = gsmCellInfo->getCellIdentity().getIdentity();
                metricsPtr->cellId.arfcn = gsmCellInfo->getCellIdentity().getArfcn();
                metricsPtr->cellId.gsm.bsic = gsmCellInfo->getCellIdentity().getBaseStationIdentityCode();
                metricsPtr->signalStrength.strength = gsmCellInfo->getSignalStrengthInfo().getGsmSignalStrength();
                metricsPtr->signalStrength.ber = gsmCellInfo->getSignalStrengthInfo().getGsmBitErrorRate();
                metricsPtr->signalStrength.dbm = gsmCellInfo->getSignalStrengthInfo().getDbm();
                metricsPtr->signalStrength.ta = gsmCellInfo->getSignalStrengthInfo().getTimingAdvance();
                metricsPtr->signalStrength.level = gsmCellInfo->getSignalStrengthInfo().getLevel();
            } else if (cellinfo->getType() == telux::tel::CellType::CDMA) {
                cellMetrics.cellRatMask |= TAF_RADIO_CELL_RAT_MASK_CDMA;
                auto cdmaCellInfo = std::static_pointer_cast<telux::tel::CdmaCellInfo>(cellinfo);
                metricsPtr = &cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_CDMA];
                metricsPtr->isRegistered = cdmaCellInfo->isRegistered();
                metricsPtr->cellId.cmda.nid = cdmaCellInfo->getCellIdentity().getNid();
                metricsPtr->cellId.cmda.sid = cdmaCellInfo->getCellIdentity().getSid();
                metricsPtr->cellId.cmda.bsid = cdmaCellInfo->getCellIdentity().getBaseStationId();
                metricsPtr->cellId.cmda.longitude = cdmaCellInfo->getCellIdentity().getLongitude();
                metricsPtr->cellId.cmda.latitude = cdmaCellInfo->getCellIdentity().getLatitude();
                metricsPtr->signalStrength.dbm = cdmaCellInfo->getSignalStrengthInfo().getDbm();
                metricsPtr->signalStrength.snr = cdmaCellInfo->getSignalStrengthInfo().getEvdoSignalNoiseRatio();
                metricsPtr->signalStrength.level = cdmaCellInfo->getSignalStrengthInfo().getLevel();
                metricsPtr->signalStrength.cdma.cdmaEcio = cdmaCellInfo->getSignalStrengthInfo().getCdmaEcio();
                metricsPtr->signalStrength.cdma.evdoEcio = cdmaCellInfo->getSignalStrengthInfo().getEvdoEcio();
            } else if (cellinfo->getType() == telux::tel::CellType::LTE) {
                cellMetrics.cellRatMask |= TAF_RADIO_CELL_RAT_MASK_LTE;
                auto lteCellInfo = std::static_pointer_cast<telux::tel::LteCellInfo>(cellinfo);
                metricsPtr = &cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_LTE];
                metricsPtr->isRegistered = lteCellInfo->isRegistered();
                metricsPtr->cellId.mcc = lteCellInfo->getCellIdentity().getMcc();
                metricsPtr->cellId.mnc = lteCellInfo->getCellIdentity().getMnc();
                metricsPtr->cellId.cid = lteCellInfo->getCellIdentity().getIdentity();
                metricsPtr->cellId.arfcn = lteCellInfo->getCellIdentity().getEarfcn();
                metricsPtr->cellId.lte.pid = lteCellInfo->getCellIdentity().getPhysicalCellId();
                metricsPtr->cellId.lte.tac = lteCellInfo->getCellIdentity().getTrackingAreaCode();
                metricsPtr->signalStrength.strength = lteCellInfo->getSignalStrengthInfo().getLteSignalStrength();
                metricsPtr->signalStrength.dbm = lteCellInfo->getSignalStrengthInfo().getDbm();
                metricsPtr->signalStrength.snr = lteCellInfo->getSignalStrengthInfo().getLteReferenceSignalSnr() * TAF_RADIO_LTE_SNR_RATIO;
                metricsPtr->signalStrength.ta = lteCellInfo->getSignalStrengthInfo().getTimingAdvance();
                metricsPtr->signalStrength.level = lteCellInfo->getSignalStrengthInfo().getLevel();
                metricsPtr->signalStrength.lte.rsrq = lteCellInfo->getSignalStrengthInfo().getLteReferenceSignalReceiveQuality();
                metricsPtr->signalStrength.lte.cqi = lteCellInfo->getSignalStrengthInfo().getLteChannelQualityIndicator();
            } else if (cellinfo->getType() == telux::tel::CellType::WCDMA) {
                cellMetrics.cellRatMask |= TAF_RADIO_CELL_RAT_MASK_WCDMA;
                auto wcdmaCellInfo = std::static_pointer_cast<telux::tel::WcdmaCellInfo>(cellinfo);
                metricsPtr = &cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_WCDMA];
                metricsPtr->isRegistered = wcdmaCellInfo->isRegistered();
                metricsPtr->cellId.mcc = wcdmaCellInfo->getCellIdentity().getMcc();
                metricsPtr->cellId.mnc = wcdmaCellInfo->getCellIdentity().getMnc();
                metricsPtr->cellId.lac = wcdmaCellInfo->getCellIdentity().getLac();
                metricsPtr->cellId.cid = wcdmaCellInfo->getCellIdentity().getIdentity();
                metricsPtr->cellId.arfcn = wcdmaCellInfo->getCellIdentity().getUarfcn();
                metricsPtr->cellId.wcdma.psc = wcdmaCellInfo->getCellIdentity().getPrimaryScramblingCode();
                metricsPtr->signalStrength.strength = wcdmaCellInfo->getSignalStrengthInfo().getSignalStrength();
                metricsPtr->signalStrength.dbm = wcdmaCellInfo->getSignalStrengthInfo().getDbm();
                metricsPtr->signalStrength.ber = wcdmaCellInfo->getSignalStrengthInfo().getBitErrorRate();
                metricsPtr->signalStrength.level = wcdmaCellInfo->getSignalStrengthInfo().getLevel();
                LE_DEBUG("WCDMA Signal Strength: %d", metricsPtr->signalStrength.strength);
                LE_DEBUG("WCDMA Signal Bit Error Rate: %d", metricsPtr->signalStrength.ber);
            } else if (cellinfo->getType() == telux::tel::CellType::TDSCDMA) {
                cellMetrics.cellRatMask |= TAF_RADIO_CELL_RAT_MASK_TDSCDMA;
                auto tdsCdmaCellInfo = std::static_pointer_cast<telux::tel::TdscdmaCellInfo>(cellinfo);
                metricsPtr = &cellMetrics.signalMetrics[TAF_RADIO_CELL_INFO_TYPE_TDSCDMA];
                metricsPtr->isRegistered = tdsCdmaCellInfo->isRegistered();
                metricsPtr->cellId.mcc = tdsCdmaCellInfo->getCellIdentity().getMcc();
                metricsPtr->cellId.mnc = tdsCdmaCellInfo->getCellIdentity().getMnc();
                metricsPtr->cellId.lac = tdsCdmaCellInfo->getCellIdentity().getLac();
                metricsPtr->cellId.cid = tdsCdmaCellInfo->getCellIdentity().getIdentity();
                metricsPtr->cellId.tdscdma.cpid = tdsCdmaCellInfo->getCellIdentity().getParametersId();
                metricsPtr->signalStrength.tdscdma.rscp = tdsCdmaCellInfo->getSignalStrengthInfo().getRscp();
            }
        }
    } else {
        LE_ERROR("Error(%d)", (int)error);
    }

    le_sem_Post(semaphore);
}

LE_MEM_DEFINE_STATIC_POOL(prefOpsListPool, TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM, sizeof(taf_RadioPrefOpList_t));

LE_MEM_DEFINE_STATIC_POOL(prefOpPool, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM, sizeof(taf_RadioPrefOp_t));

LE_MEM_DEFINE_STATIC_POOL(prefOpSafeRefPool, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM, sizeof(taf_RadioPrefOpSafeRef_t));

LE_MEM_DEFINE_STATIC_POOL(scanOpsListPool, TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM, sizeof(taf_RadioScanOpList_t));

LE_MEM_DEFINE_STATIC_POOL(scanOpPool, TAF_RADIO_SCAN_OPERATORS_MAX_NUM, sizeof(taf_RadioScanOp_t));

LE_MEM_DEFINE_STATIC_POOL(scanOpSafeRefPool, TAF_RADIO_SCAN_OPERATORS_MAX_NUM, sizeof(taf_RadioScanOpSafeRef_t));

LE_MEM_DEFINE_STATIC_POOL(cellMetricsPool, TAF_RADIO_CELL_METRICS_MAX_NUM, sizeof(taf_RadioCellMetrics_t));

LE_REF_DEFINE_STATIC_MAP(prefOpListRefMap, TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM);

LE_REF_DEFINE_STATIC_MAP(prefOpSafeRefMap, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM);

LE_REF_DEFINE_STATIC_MAP(scanOpListRefMap, TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM);

LE_REF_DEFINE_STATIC_MAP(scanOpSafeRefMap, TAF_RADIO_SCAN_OPERATORS_MAX_NUM);

LE_REF_DEFINE_STATIC_MAP(metricsRefMap, TAF_RADIO_METRICS_MAX_NUM);

le_event_Id_t taf_Radio::radioCmdEvId = nullptr;

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

 FUNCTION        taf_Radio::FirstLayerNetRegRejectHandler

 DESCRIPTION     The first layer handler function for network registration rejection.

 DEPENDENCIES    The initialization of Radio.

 PARAMETERS      [IN] void* reportPtr: Pointer to the report details.
                 [IN] void* secondLayerHandlerFunc:
                          The second layer handler function for network registration rejection.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Radio::FirstLayerNetRegRejectHandler(void* reportPtr, void* secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == nullptr, "Null ptr(secondLayerHandlerFunc)");

    taf_radio_NetRegRejectHandlerFunc_t handlerFunc = (taf_radio_NetRegRejectHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc((taf_radio_NetRegRejInd_t*)reportPtr, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

/*======================================================================

 FUNCTION        taf_Radio::FirstLayerRatChangeHandler

 DESCRIPTION     The first layer handler function for network registration rejection.

 DEPENDENCIES    The initialization of Radio.

 PARAMETERS      [IN] void* reportPtr: Pointer to the report details.
                 [IN] void* secondLayerHandlerFunc:
                          The second layer handler function for network registration rejection.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Radio::FirstLayerRatChangeHandler(void* reportPtr, void* secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == nullptr, "Null ptr(secondLayerHandlerFunc)");

    taf_radio_RatChangeHandlerFunc_t handlerFunc = (taf_radio_RatChangeHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc((taf_radio_RatChangeInd_t*)reportPtr, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

/*======================================================================

 FUNCTION        taf_Radio::FirstLayerNetRegStateEventHandler

 DESCRIPTION     The first layer handler function for network registration state.

 DEPENDENCIES    The initialization of Radio.

 PARAMETERS      [IN] void* reportPtr: Pointer to the report details.
                 [IN] void* secondLayerHandlerFunc:
                          The second layer handler function for network registration state.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Radio::FirstLayerNetRegStateEventHandler(void* reportPtr, void* secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == nullptr, "Null ptr(secondLayerHandlerFunc)");

    taf_radio_NetRegStateHandlerFunc_t handlerFunc = (taf_radio_NetRegStateHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc((taf_radio_NetRegStateInd_t*)reportPtr, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

/*======================================================================

 FUNCTION        taf_Radio::FirstLayerPacketSwChangeHandler

 DESCRIPTION     The first layer handler function for service domain state.

 DEPENDENCIES    The initialization of Radio.

 PARAMETERS      [IN] void* reportPtr: Pointer to the report details.
                 [IN] void* secondLayerHandlerFunc:
                          The second layer handler function for service domain state.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Radio::FirstLayerPacketSwChangeHandler(void* reportPtr, void* secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == nullptr, "Null ptr(secondLayerHandlerFunc)");

    taf_radio_PacketSwitchedChangeHandlerFunc_t handlerFunc =
        (taf_radio_PacketSwitchedChangeHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc(*(taf_radio_ServiceDomainState_t*)reportPtr, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

/*======================================================================

 FUNCTION        taf_Radio::FirstLayerSsChangeHandler

 DESCRIPTION     The first layer handler function for signal strength change.

 DEPENDENCIES    The initialization of Radio.

 PARAMETERS      [IN] void* reportPtr: Pointer to the report details.
                 [IN] void* secondLayerHandlerFunc:
                          The second layer handler function for signal strength change.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Radio::FirstLayerSsChangeHandler(void* reportPtr, void* secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == nullptr, "Null ptr(secondLayerHandlerFunc)");

    taf_radio_SignalStrengthChangeHandlerFunc_t handlerFunc =
        (taf_radio_SignalStrengthChangeHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc(((taf_RadioSignalStrengthInfo_t*)reportPtr)->dbm, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
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
    TAF_ERROR_IF_RET_NIL(phoneId >= tafRadio.networkManagers.size(),
        "Invalid para(phoneId:%d >= %d)", phoneId, tafRadio.networkManagers.size());

    auto networkManager = tafRadio.networkManagers[phoneId];
    TAF_ERROR_IF_RET_NIL(networkManager == nullptr,
        "Invalid para(null ptr, phoneId:%d)", phoneId);
    std::chrono::time_point<std::chrono::system_clock> startTime,endTime;
    std::chrono::duration<double> elapsedTime;
    le_result_t res;

    if (cmdReq->cmdType == TAF_RADIO_CMD_TYPE_ASYNC_REG_MANUAL) {
        std::string mcc(cmdReq->mccPtr);
        std::string mnc(cmdReq->mncPtr);

        startTime = std::chrono::system_clock::now();
        TAF_ERROR_IF_RET_NIL(networkManager->setNetworkSelectionMode(telux::tel::NetworkSelectionMode::MANUAL, mcc, mnc,
            &taf_RadioNetworkResponseCallback::setNetworkSelectionModeResponseCb) != telux::common::Status::SUCCESS,
            "Call sdk function failed");

        le_clk_Time_t timeToWait = {1, 0};
        res = le_sem_WaitWithTimeOut(taf_RadioNetworkResponseCallback::semNetSelModeRespCb, timeToWait);
        TAF_ERROR_IF_RET_NIL(res != LE_OK, "Wait semaphore timeout\n");

        endTime = std::chrono::system_clock::now();
        elapsedTime = endTime - startTime;
        LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

        taf_radio_ManualSelectionHandlerFunc_t handlerFunc = (taf_radio_ManualSelectionHandlerFunc_t)cmdReq->handlerFuncPtr;
        if (handlerFunc != nullptr) {
            LE_DEBUG("Handler function:%p, result:%d", handlerFunc, res);
            handlerFunc(res, cmdReq->contextPtr);
        } else {
            LE_WARN("No handler function, result:%d", res);
        }
    } else if (cmdReq->cmdType == TAF_RADIO_CMD_TYPE_ASYNC_NETWORK_SCAN) {
        startTime = std::chrono::system_clock::now();
        TAF_ERROR_IF_RET_NIL(networkManager->performNetworkScan(
            taf_RadioPerformNetworkScanCallback::performNetworkScanResponse) != telux::common::Status::SUCCESS,
            "Call sdk function failed");

        le_clk_Time_t timeToWait = {TAF_RADIO_SCAN_INTERVAL, 0};
        res = le_sem_WaitWithTimeOut(taf_RadioPerformNetworkScanCallback::semaphore, timeToWait);
            TAF_ERROR_IF_RET_NIL(res != LE_OK, "Wait semaphore timeout\n");

        endTime = std::chrono::system_clock::now();
        elapsedTime = endTime - startTime;
        LE_DEBUG("Elapsed time: %lfs\n", elapsedTime.count());

        TAF_ERROR_IF_RET_NIL(taf_RadioPerformNetworkScanCallback::opInfos.size() == 0, "Phone%d has no operators after scanning", phoneId);

        taf_RadioScanOpList_t* opsList = (taf_RadioScanOpList_t*)le_mem_ForceAlloc(tafRadio.scanOpsListPool);
        TAF_ERROR_IF_RET_NIL(opsList == nullptr, "Null ptr(opsList)");

        opsList->scanOpList = LE_SLS_LIST_INIT;
        opsList->safeRefList = LE_SLS_LIST_INIT;
        opsList->currPtr = NULL;
        opsList->num = taf_RadioPerformNetworkScanCallback::opInfos.size();

        taf_RadioScanOp_t* opPtr;
        for (auto info : taf_RadioPerformNetworkScanCallback::opInfos) {
            opPtr = (taf_RadioScanOp_t*)le_mem_ForceAlloc(tafRadio.scanOpPool);
            le_utf8_Copy(opPtr->name, info.getName().c_str(), TAF_RADIO_NETWORK_NAME_MAX_LEN, NULL);
            le_utf8_Copy(opPtr->mcc, info.getMcc().c_str(), TAF_RADIO_MCC_BYTES, NULL);
            le_utf8_Copy(opPtr->mnc, info.getMnc().c_str(), TAF_RADIO_MNC_BYTES, NULL);
            opPtr->status.inUse = info.getStatus().inUse;
            opPtr->status.roaming = info.getStatus().roaming;
            opPtr->status.forbidden = info.getStatus().forbidden;
            opPtr->status.preferred = info.getStatus().preferred;
            opPtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(opsList->scanOpList), &(opPtr->link));
        }

        taf_radio_ScanInformationListRef_t listRef = (taf_radio_ScanInformationListRef_t)le_ref_CreateRef(tafRadio.scanOpListRefMap, (void*)opsList);
        taf_radio_CellularNetworkScanHandlerFunc_t handlerFunc = (taf_radio_CellularNetworkScanHandlerFunc_t)cmdReq->handlerFuncPtr;
        if (handlerFunc != nullptr) {
            LE_DEBUG("Handler function:%p listRef:%p", handlerFunc, listRef);
            handlerFunc(listRef, cmdReq->contextPtr);
        } else {
            LE_WARN("No handler function");
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

/*======================================================================

 FUNCTION        taf_Radio::Init

 DESCRIPTION     Initialization of the Radio Service

 DEPENDENCIES    The initialization of telaf.

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Radio::Init(void)
{
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    std::chrono::duration<double> elapsedTime;

    // 1. Initiate the semaphore
    taf_RadioSelectionModeResponseCallback::semaphore = le_sem_Create("taf_RadioSelModeRespCbSem", 0);
    taf_RadioPreferredNetworksResponseCallback::semaphore = le_sem_Create("taf_RadioPrefNetworkRespCbSem", 0);
    taf_RadioNetworkResponseCallback::semPrefNetRespCb = le_sem_Create("taf_RadioPrefNetRespCbSem", 0);
    taf_RadioNetworkResponseCallback::semNetSelModeRespCb = le_sem_Create("taf_RadioNetSelModeRespCbSem", 0);
    taf_RadioPerformNetworkScanCallback::semaphore = le_sem_Create("taf_RadioPerfNetScanCbSem", 0);
    taf_RadioVoiceServiceStateCallback::semaphore = le_sem_Create("taf_RadioVocSvcStateCbSem", 0);
    taf_RadioRatPreferenceResponseCallback::semaphore = le_sem_Create("taf_RadioRatPrefRespCbSem", 0);
    taf_RadioServiceDomainResponseCallback::semaphore = le_sem_Create("taf_RadioSvcDomainRespCbSem", 0);
    taf_RadioCellInfoCallback::semaphore = le_sem_Create("taf_RadioCellInfoCbSem", 0);

    // 2. Create event id.
    netRegRejectEvId = le_event_CreateIdWithRefCounting("NetRegReject");
    ratChangeEvId = le_event_CreateIdWithRefCounting("RatChange");
    netRegStateEvId = le_event_CreateIdWithRefCounting("NetRegState");
    packetSwChangeEvId = le_event_CreateIdWithRefCounting("PacketSwState");
    gsmSsChangeEvId = le_event_CreateIdWithRefCounting("GsmSsChange");
    cdmaSsChangeEvId = le_event_CreateIdWithRefCounting("CdmaSsChange");
    lteSsChangeEvId = le_event_CreateIdWithRefCounting("LteSsChange");
    wcdmaSsChangeEvId = le_event_CreateIdWithRefCounting("WcdmaSsChange");
    tdscdmaSsChangeEvId = le_event_CreateIdWithRefCounting("TdscdmaSsChange");

    // 3. Initiate the memory pool
    prefOpsListPool = le_mem_InitStaticPool(prefOpsListPool,
        TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM, sizeof(taf_RadioPrefOpList_t));
    prefOpPool = le_mem_InitStaticPool(prefOpPool, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM,
        sizeof(taf_RadioPrefOp_t));
    prefOpSafeRefPool = le_mem_InitStaticPool(prefOpSafeRefPool, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM,
        sizeof(taf_RadioPrefOpSafeRef_t));
    scanOpsListPool = le_mem_InitStaticPool(scanOpsListPool,
        TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM, sizeof(taf_RadioScanOpList_t));
    scanOpPool = le_mem_InitStaticPool(scanOpPool, TAF_RADIO_SCAN_OPERATORS_MAX_NUM,
        sizeof(taf_RadioScanOp_t));
    scanOpSafeRefPool = le_mem_InitStaticPool(scanOpSafeRefPool, TAF_RADIO_SCAN_OPERATORS_MAX_NUM,
        sizeof(taf_RadioScanOpSafeRef_t));
    cellMetricsPool = le_mem_InitStaticPool(cellMetricsPool, TAF_RADIO_CELL_METRICS_MAX_NUM,
        sizeof(taf_RadioCellMetrics_t));

    netRegRejectPool = le_mem_CreatePool("netRegRejectPool", sizeof(taf_radio_NetRegRejInd_t));
    ratChangePool = le_mem_CreatePool("ratChangePool", sizeof(taf_radio_RatChangeInd_t));
    netRegStatePool = le_mem_CreatePool("netRegStatePool", sizeof(taf_radio_NetRegStateInd_t));
    packetSwChangePool = le_mem_CreatePool("packetSwChangePool", sizeof(taf_radio_ServiceDomainState_t*));
    ssChangePool = le_mem_CreatePool("ssChangePool", sizeof(taf_RadioSignalStrengthInfo_t) * TAF_RADIO_CELL_INFO_TYPE_MAX);

    // 4. Initiate the reference map.
    prefOpListRefMap = le_ref_InitStaticMap(prefOpListRefMap, TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM);
    prefOpSafeRefMap = le_ref_InitStaticMap(prefOpSafeRefMap, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM);
    scanOpListRefMap = le_ref_InitStaticMap(scanOpListRefMap, TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM);
    scanOpSafeRefMap = le_ref_InitStaticMap(scanOpSafeRefMap, TAF_RADIO_SCAN_OPERATORS_MAX_NUM);
    metricsRefMap = le_ref_InitStaticMap(metricsRefMap, TAF_RADIO_METRICS_MAX_NUM);

    startTime = std::chrono::system_clock::now();

    // 5. Get the PhoneFactory and PhoneManager instances
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    phoneManager = phoneFactory.getPhoneManager();

    // 6. Check if telephony subsystem is ready
    bool subSystemStatus = phoneManager->isSubsystemReady();
    if (!subSystemStatus) {
        LE_INFO("Telephony subsystem wait to be ready...");
        future<bool> f = phoneManager->onSubsystemReady();
        //  Wait until the subsystem is ready.
        subSystemStatus = f.get();
    }

    if (subSystemStatus) {
        endTime = std::chrono::system_clock::now();
        elapsedTime = endTime - startTime;
        LE_INFO("Elapsed time for telephony subsystem: %lfs", elapsedTime.count());

        // 7. Instantiate Phone
        std::vector<int> phoneIds;
        telux::common::Status status = phoneManager->getPhoneIds(phoneIds);
        if (status == telux::common::Status::SUCCESS) {
            for (size_t index = 1; index <= phoneIds.size(); index++) {
                auto phone = phoneManager->getPhone(index);
                if (phone != nullptr) {
                    phones.emplace_back(phone);
                }
                auto networkManager = telux::tel::PhoneFactory::getInstance().getNetworkSelectionManager(index);
                if (networkManager != nullptr) {
                    networkManagers.emplace_back(networkManager);
                }
                auto servingSystemManager = telux::tel::PhoneFactory::getInstance().getServingSystemManager(index);
                if (servingSystemManager != nullptr) {
                    servingSystemManagers.emplace_back(servingSystemManager);
                }
            }
        }

        // 8. Instantiate RadioPhoneListener
        phoneListener = std::make_shared<taf_RadioPhoneListener>();

        // 9. Register for phone info updates
        if (phoneManager->registerListener(phoneListener) != telux::common::Status::SUCCESS) {
            LE_ERROR("Failed to register phone listener");
        }

        // 10. Instantiate RadioCallback
        voiceSrvStateCb = std::make_shared<taf_RadioVoiceServiceStateCallback>();
        voiceRadioTechCb = std::make_shared<taf_RadioVoiceRadioTechnologyCallback>();
        signalStrengthCb = std::make_shared<taf_RadioSignalStrengthCallback>();
        setOperatingModeCb = std::make_shared<taf_RadioSetOperatingModeCallback>();
        voiceRadioTechCb->semaphore = le_sem_Create("taf_RadioVocRATCbSem", 0);
        signalStrengthCb->semaphore = le_sem_Create("taf_RadioSgnStrengthCbSem", 0);
        setOperatingModeCb->semaphore = le_sem_Create("taf_RadioSetOpModeCbSem", 0);

        for (size_t index = 0; index < networkManagers.size(); index++) {
            startTime = std::chrono::system_clock::now();

            // 12. Check if network subsystem is ready
            bool networkSystemStatus = networkManagers[index]->isSubsystemReady();
            if (!networkSystemStatus) {
                LE_INFO("Network subsystem wait to be ready...");
                std::future<bool> f = networkManagers[index]->onSubsystemReady();
                //  Wait until the subsystem is ready.
                networkSystemStatus = f.get();
            }

            if (networkSystemStatus) {
                endTime = std::chrono::system_clock::now();
                elapsedTime = endTime - startTime;
                LE_INFO("Elapsed time for %d network subsystem: %lfs", index, elapsedTime.count());
            } else {
                LE_ERROR("Fail to init %d network subsystem", index);
            }

            // 13. Instantiate RadioNetworkSelectionListener
            networkListener = std::make_shared<taf_RadioNetworkSelectionListener>();
            if (networkManagers[index]->registerListener(networkListener) != telux::common::Status::SUCCESS) {
                LE_ERROR("Failed to register network listener");
            }
        }

        for (size_t index = 0; index < servingSystemManagers.size(); index++) {
            startTime = std::chrono::system_clock::now();

            // 14. Check if serving subsystem is ready
            bool servingSystemStatus = servingSystemManagers[index]->isSubsystemReady();
            if (!servingSystemStatus) {
                LE_INFO("Serving subsystem wait to be ready...");
                std::future<bool> f = servingSystemManagers[index]->onSubsystemReady();
                //  Wait until the subsystem is ready.
                servingSystemStatus = f.get();
            }

            if (servingSystemStatus) {
                endTime = std::chrono::system_clock::now();
                elapsedTime = endTime - startTime;
                LE_INFO("Elapsed time for %d serving subsystem: %lfs", index, elapsedTime.count());
            } else {
                LE_ERROR("Fail to init %d serving subsystem", index);
            }

            // 15. Instantiate RadioServingSystemListener
            servingSystemListener = std::make_shared<taf_RadioServingSystemListener>();
            if (servingSystemManagers[index]->registerListener(servingSystemListener) != telux::common::Status::SUCCESS) {
                LE_ERROR("Failed to register serving system listener");
            }
        }
    } else {
        LE_ERROR("Fail to init telephony subsystem");
    }

    // 16. Get the SubscriptionManager instances
    subscriptionManager = phoneFactory.getSubscriptionManager();

    // 17. Check if subscription subsystem is ready
    startTime = std::chrono::system_clock::now();
    subSystemStatus = subscriptionManager->isSubsystemReady();

    if (!subSystemStatus) {
        LE_INFO("Subscription subsystem wait to be ready...");
        std::future<bool> f = subscriptionManager->onSubsystemReady();
        //  Wait until the subsystem is ready.
        subSystemStatus = f.get();
    }

    if (subSystemStatus) {
        endTime = std::chrono::system_clock::now();
        elapsedTime = endTime - startTime;
        LE_INFO("Elapsed time for subscription subsystem: %lfs", elapsedTime.count());
    } else {
        LE_ERROR("Fail to init subscription subsystem");
    }

    // 18. Register listener with Subscription Manager for the notification
    subscriptionListener = std::make_shared<taf_RadioSubscriptionListener>();
    subscriptionManager->registerListener(subscriptionListener);

    // 19. Create and start command thread.
    le_sem_Ref_t radioCmdThreadSem = le_sem_Create("radioCmdThreadSem", 0);
    radioCmdEvId = le_event_CreateId("radioCmd", sizeof(taf_RadioCmdReq_t));
    le_thread_Ref_t radioCmdThreadRef = le_thread_Create("radioCmdThread", RadioCmdThread, (void*)radioCmdThreadSem);
    le_thread_SetStackSize(radioCmdThreadRef, TAF_RADIO_THREAD_STACK_SIZE);
    le_thread_Start(radioCmdThreadRef);
    le_sem_Wait(radioCmdThreadSem);

    // 20. Delete semaphore.
    le_sem_Delete(radioCmdThreadSem);
}
