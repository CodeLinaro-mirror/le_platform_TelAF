/*
* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the
* disclaimer below) provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following
* disclaimer in the documentation and/or other materials provided
* with the distribution.
*
* * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
* contributors may be used to endorse or promote products derived
* from this software without specific prior written permission.
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

#include "tafECall.hpp"

using namespace telux::tel;
using namespace telux::common;
using namespace telux::tafsvc;

LE_REF_DEFINE_STATIC_MAP(ECallMap, MAX_ECALL);

char fdn[TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES];
char sdn[TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES];

void tafECallOperatingModeCallback::setECallOperatingModeResponse(
    telux::common::ErrorCode error) {
    auto &eCall = taf_ecall::GetInstance();
    if (error == telux::common::ErrorCode::SUCCESS) {
        LE_DEBUG("Set eCall operating mode request executed successfully");
    } else {
        LE_ERROR( "Set eCall operating mode request failed error = %d", (int)error);
    }
    eCall.setOpModeProm.set_value(error);
}

void tafECallOperatingModeCallback::getECallOperatingModeResponse(
    telux::tel::ECallMode eCallMode, telux::common::ErrorCode error) {

    auto &eCall = taf_ecall::GetInstance();
    if (error == telux::common::ErrorCode::SUCCESS) {
        LE_DEBUG("eCall operating mode request executed successfully");
    } else {
         LE_ERROR("Request eCall Operating Mode failed, errorCode: ");
    }
    eCall.getOpModeProm.set_value(eCallMode);
}

void tafCallCommandCallback::makeCallResponse(telux::common::ErrorCode errorCode,
        std::shared_ptr<telux::tel::ICall> call) {
    int32_t callIndex = -1;
    auto &eCall = taf_ecall::GetInstance();
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        LE_INFO("Call is successful ");
        callIndex = call->getCallIndex();

    } else {
        LE_ERROR("Call failed with error code: %d ", (static_cast<int>(errorCode)));
    }
    eCall.makeEcallProm.set_value(errorCode);
    eCall.SetCallIndex(callIndex);
}

void tafCallCommandCallback::makeECallResponse(telux::common::ErrorCode errorCode,
                                                      std::shared_ptr<telux::tel::ICall> call) {
    int32_t callIndex = -1;
    auto &eCall = taf_ecall::GetInstance();
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        LE_INFO("Call is successful ");
        callIndex = call->getCallIndex();
    } else {
        LE_ERROR("Call failed with error code: %d ", (static_cast<int>(errorCode)));
    }
    eCall.makeEcallProm.set_value(errorCode);
    eCall.SetCallIndex(callIndex);
}

void tafUpdateMsdCommandCallback::commandResponse(telux::common::ErrorCode errorCode) {
    auto &eCall = taf_ecall::GetInstance();
    eCall.updateMsdProm.set_value(errorCode);
}

void tafHangupCommandCallback::commandResponse(telux::common::ErrorCode errorCode) {
   if(errorCode == telux::common::ErrorCode::SUCCESS) {
      LE_INFO("Call hangup is successful ");
   } else {
      LE_ERROR("Call hangup failed with error code: %d ", (static_cast<int>(errorCode)));
   }
}

void tafECallListener::onIncomingCall(std::shared_ptr<telux::tel::ICall> call) {

}

void tafECallListener::onCallInfoChange(std::shared_ptr<telux::tel::ICall> call) {

    taf_ecall_State_t state = TAF_ECALL_STATE_UNKNOWN;
    tafECallSession_t sessionState = ECALL_INIT;

    auto &eCall = taf_ecall::GetInstance();
    CallState callState = call->getCallState();

    bool isCallStateSet = false;

    eCall.CallEndError = telux::tel::CallEndCause::NORMAL;
    LE_INFO("CallID = %d, State: %d", (int) call->getCallIndex(), (int) callState);

    if (callState == CallState::CALL_ACTIVE)
    {
        sessionState = ECALL_ACTIVE;
        state = TAF_ECALL_STATE_ACTIVE;
        isCallStateSet = true;

    }
    else if (callState == CallState::CALL_ALERTING)
    {
        sessionState = ECALL_ALERTING;
        state = TAF_ECALL_STATE_ALERTING;
        isCallStateSet = true;
    }
    else if (callState == CallState::CALL_DIALING)
    {
        sessionState = ECALL_DIALING;
        state = TAF_ECALL_STATE_DIALING;
        isCallStateSet = true;
    }
    else if (callState == CallState::CALL_INCOMING)
    {

    }
    else if (callState == CallState::CALL_ENDED)
    {
        sessionState = ECALL_ENDED;
        state = TAF_ECALL_STATE_ENDED;
        eCall.CallEndError = call->getCallEndCause();
        LE_INFO("ECall ENDed terminate reason = %d", (int) eCall.CallEndError);
        isCallStateSet = true;

    }
    eCall.SetSessionState(sessionState);
    eCall.SetECallState(state);
    if (isCallStateSet)
    {
        StateChangeEvent_t stateEvent;
        stateEvent.eCallRef = eCall.GetECallReference();
        stateEvent.state = state;
        le_event_Report(eCall.StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));
    }
}

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
void tafECallListener::onEmergencyNetworkScanFail(int phoneId) {

}
#endif

taf_ecall_State_t tafECallListener::eCallMsdTransmissionStatusToState(
   telux::tel::ECallMsdTransmissionStatus status)
{
    taf_ecall_State_t state = TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED;
    auto &eCall = taf_ecall::GetInstance();
    StateChangeEvent_t stateEvent;

    LE_DEBUG("eCallMsdTransmissionStatusToState status = %d", (int)status);

    switch(status) {
        case telux::tel::ECallMsdTransmissionStatus::SUCCESS:
            state = TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS;
            break;
        case telux::tel::ECallMsdTransmissionStatus::FAILURE:
            state = TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED;
            break;
        case telux::tel::ECallMsdTransmissionStatus::MSD_TRANSMISSION_STARTED:
            state = TAF_ECALL_STATE_MSD_TRANSMISSION_STARTED;
            break;
        case telux::tel::ECallMsdTransmissionStatus::NACK_OUT_OF_ORDER:
            state = TAF_ECALL_STATE_NACK_OUT_OF_ORDER;
            break;
        case telux::tel::ECallMsdTransmissionStatus::ACK_OUT_OF_ORDER:
            state = TAF_ECALL_STATE_ACK_OUT_OF_ORDER;
            break;
        case telux::tel::ECallMsdTransmissionStatus::START_RECEIVED:
            state = TAF_ECALL_STATE_PSAP_START_RECEIVED;
            break;
        case telux::tel::ECallMsdTransmissionStatus::LL_ACK_RECEIVED:
            state = TAF_ECALL_STATE_LL_ACK_RECEIVED;
            break;
        case telux::tel::ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_STARTED:
            state = TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_STARTED;
            break;
        case telux::tel::ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_SUCCESS:
            state = TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_SUCCESS;
            break;
        case telux::tel::ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_FAILURE:
            state = TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_FAILURE;
            break;
        default:
            LE_ERROR( "Unknown ECallMsdTransmissionStatus  = %d", (int)status);
    }

    stateEvent.eCallRef = eCall.GetECallReference();
    stateEvent.state = state;

    le_event_Report(eCall.StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));

    return state;
}

void tafECallListener::onECallMsdTransmissionStatus(
        int phoneId, telux::tel::ECallMsdTransmissionStatus msdTransmissionStatus) {
    eCallMsdTransmissionStatusToState(msdTransmissionStatus);
}

void tafECallListener::onECallHlapTimerEvent(int phoneId, ECallHlapTimerEvents timerEvents) {
    LE_DEBUG("onECallHlapTimerEvent ");
    taf_ecall_State_t state = TAF_ECALL_STATE_UNKNOWN;
    if(timerEvents.t2 == HlapTimerEvent::EXPIRED) {
        state = TAF_ECALL_STATE_T2_EXPIRED;
    }
    if(timerEvents.t5 == HlapTimerEvent::EXPIRED) {
        state = TAF_ECALL_STATE_T5_EXPIRED;
    }
    if(timerEvents.t6 == HlapTimerEvent::EXPIRED) {
        state = TAF_ECALL_STATE_T6_EXPIRED;
    }
    if(timerEvents.t7 == HlapTimerEvent::EXPIRED) {
        state = TAF_ECALL_STATE_T7_EXPIRED;
    }
    if(timerEvents.t9 == HlapTimerEvent::EXPIRED) {
        state = TAF_ECALL_STATE_T9_EXPIRED;
    }
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    if(timerEvents.t10 == HlapTimerEvent::EXPIRED) {
        state = TAF_ECALL_STATE_T10_EXPIRED;
    }
#endif
    if (state != TAF_ECALL_STATE_UNKNOWN) {
        auto &eCall = taf_ecall::GetInstance();
        StateChangeEvent_t stateEvent;
        stateEvent.eCallRef = eCall.GetECallReference();
        stateEvent.state = state;
        le_event_Report(eCall.StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));
    }
}

void taf_ecall::InitializeECallPtr()
{

    ECallPtrRefMap = le_ref_InitStaticMap(ECallMap, MAX_ECALL);

    ECallObject.msd.optionals.optionalDataType = ECallOptionalDataType::ECALL_DEFAULT;
    ECallObject.msd.optionals.optionalDataPresent = false;
    ECallObject.msd.optionals.recentVehicleLocationN1Present = false;
    ECallObject.msd.optionals.recentVehicleLocationN2Present = false;
    ECallObject.msd.optionals.numberOfPassengersPresent = false;

    ECallObject.msd.msdVersion = MSD_VERSION_TWO;

    ECallObject.msd.messageIdentifier = 0;

    ECallObject.msd.control.automaticActivation = false;
    ECallObject.msd.control.testCall = false;
    ECallObject.msd.control.positionCanBeTrusted = false;
    ECallObject.msd.control.vehicleType = (ECallVehicleType)TAF_ECALL_PASSENGER_VEHICLE_CLASS_M1;

    ECallObject.msd.vehicleIdentificationNumber.isowmi = '\0';
    ECallObject.msd.vehicleIdentificationNumber.isovds = '\0';
    ECallObject.msd.vehicleIdentificationNumber.isovisModelyear = '\0';
    ECallObject.msd.vehicleIdentificationNumber.isovisSeqPlant = '\0';

    ECallObject.msd.vehiclePropulsionStorage.gasolineTankPresent = false;
    ECallObject.msd.vehiclePropulsionStorage.dieselTankPresent = false;
    ECallObject.msd.vehiclePropulsionStorage.compressedNaturalGas = false;
    ECallObject.msd.vehiclePropulsionStorage.liquidPropaneGas = false;
    ECallObject.msd.vehiclePropulsionStorage.electricEnergyStorage = false;
    ECallObject.msd.vehiclePropulsionStorage.hydrogenStorage = false;
    ECallObject.msd.vehiclePropulsionStorage.otherStorage = false;

    ECallObject.msd.timestamp = 0;

    ECallObject.msd.vehicleLocation.positionLatitude = 0;
    ECallObject.msd.vehicleLocation.positionLongitude = 0;

    ECallObject.msd.vehicleDirection = 0;

    ECallObject.msd.recentVehicleLocationN1.latitudeDelta = 0;
    ECallObject.msd.recentVehicleLocationN1.longitudeDelta = 0;
    ECallObject.msd.recentVehicleLocationN2.latitudeDelta = 0;
    ECallObject.msd.recentVehicleLocationN2.longitudeDelta = 0;

    ECallObject.msd.numberOfPassengers = 0;

    //ECallObject.msd.optionalPdu.eCallDefaultOptions.objId. =;
    ECallObject.msd.optionalPdu.eCallDefaultOptions.optionalData = '\0';

    ECallObject.reference = (taf_ecall_CallRef_t)le_ref_CreateRef(ECallPtrRefMap, &ECallObject);

    ECallObject.msdTxMode = TAF_ECALL_MSD_TX_MODE_PUSH;

    ECallObject.eCallSession = ECALL_INIT;
    ECallObject.state = TAF_ECALL_STATE_UNKNOWN;

    ECallObject.isMsdUpdated = false;
}

void taf_ecall::Init(void)
{
   //  Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

   std::promise<telux::common::ServiceStatus> prom;
   CallManager = phoneFactory.getCallManager([&](telux::common::ServiceStatus status) {
       prom.set_value(status);
   });
   telux::common::ServiceStatus mgrStatus = prom.get_future().get();
   if (mgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       LE_INFO("Cannot initialize all manager, ret: %d", (int)mgrStatus);
       return;
   }

   PhoneManager = phoneFactory.getPhoneManager();
   //  Check if telephony subsystem is ready
   bool subSystemStatus = PhoneManager->isSubsystemReady();

   //  If telephony subsystem is not ready, wait for it to be ready
   if(!subSystemStatus) {
      LE_INFO("\n\nTelephony subsystem is not ready, Please wait");
      std::future<bool> f = PhoneManager->onSubsystemReady();
      // If we want to wait unconditionally for telephony subsystem to be ready
      subSystemStatus = f.get();
   }

   //  Exit the service, if SDK is unable to initialize telephony subsystems
   if(subSystemStatus) {
      std::vector<int> phoneIds;
      telux::common::Status status = PhoneManager->getPhoneIds(phoneIds);
      if (status == telux::common::Status::SUCCESS) {
          for (auto index = 1; index <= (int)phoneIds.size(); index++) {
              auto phone = PhoneManager->getPhone(index);
              if (phone != nullptr) {
                  Phones.emplace_back(phone);
              }
          }
      }
   } else {
      LE_ERROR("ERROR - Unable to initialize subsystem");
      return;
   }

   InitializeECallPtr();

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn( CFG_MODEMSERVICE_ECALL_PATH );
    int opMode = le_cfg_GetInt(iteratorRef, CFG_NODE_OPMODE, 0);
    le_cfg_CancelTxn(iteratorRef);
    if (opMode == TAF_ECALL_FORCED_PERSISTENT_ONLY_MODE) {
        uint8_t phoneId = PhoneManager->getPhoneIdFromSlotId((int)taf_sim_GetSelectedCard());
        le_result_t res = SetECallOperatingMode(phoneId, TAF_ECALL_MODE_ECALL);
        LE_INFO("Apply eCall persist only mode in phoneId: %d, result = %d\n", phoneId, res);
    }

    ECallListener =  std::make_shared<tafECallListener>();
    Status ret = CallManager->registerListener(ECallListener);
    if(ret!= Status::SUCCESS)
    {
        LE_CRIT("Cannot register Listern for ecall event!\n");
    }


   CallCommandCb = std::make_shared<tafCallCommandCallback>();
   UpdateMsdCb = std::make_shared<tafUpdateMsdCommandCallback>();
   HangupCb = std::make_shared<tafHangupCommandCallback>();

   StateChangeEventId = le_event_CreateId("NewStateEventId", sizeof(StateChangeEvent_t));

   le_cfg_AddChangeHandler(CFG_MODEMSERVICE_ECALL_PATH, ConfigChangeHandler, NULL);
}

taf_ecall &taf_ecall::GetInstance()
{
    static taf_ecall instance;
    return instance;
}

bool taf_ecall::isIdle()
{
    std::vector<std::shared_ptr<telux::tel::ICall>> callList = CallManager->getInProgressCalls();

    for(auto itr = std::begin(callList); itr != std::end(callList); ++itr) {
        telux::tel::CallState callState = (*itr)->getCallState();
        if (callState != telux::tel::CallState::CALL_ENDED &&
                callState != telux::tel::CallState::CALL_IDLE) {
            LE_INFO("isIdle: call state is %d", (int) callState);
            return false;
        }
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(ECallPtrRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_ECall_t* eCallPtr = (taf_ECall_t*) le_ref_GetValue(iterRef);
        LE_ASSERT(eCallPtr != NULL);

        //Check ECall session
        if (eCallPtr->eCallSession != ECALL_INIT && (eCallPtr->eCallSession != ECALL_ENDED)) {
            LE_INFO("isIdle: call session is %d", (int) eCallPtr->eCallSession);
            return false;
        }
    }

    return true;
}

le_result_t taf_ecall::SetPsapNumber(const char* psapNumber)
{
    TAF_ERROR_IF_RET_VAL(strlen(psapNumber) > TAF_SIM_PHONE_NUM_MAX_LEN, LE_FAULT,
            "PsapNumber length is wrong");

    LE_INFO("Set PSAP number as %s", psapNumber);

    EcallConfig eCallConfig;
    eCallConfig.configValidityMask.set(ECALL_CONFIG_OVERRIDDEN_NUM);
    eCallConfig.configValidityMask.set(ECALL_CONFIG_NUM_TYPE);
    eCallConfig.numType = ECallNumType::OVERRIDDEN;
    eCallConfig.overriddenNum = psapNumber;
    Status status = CallManager->setECallConfig(eCallConfig);

    return Status::SUCCESS == status ? LE_OK : LE_FAULT;
}

le_result_t taf_ecall::GetPsapNumber(char* psapNumber, size_t psapNumLength)
{
    TAF_ERROR_IF_RET_VAL(psapNumber == NULL, LE_BAD_PARAMETER, "PsapNumber is NULL");

    EcallConfig eCallConfig = {};
    Status status = CallManager->getECallConfig(eCallConfig);
    if (status == Status::SUCCESS && eCallConfig.configValidityMask.test(ECALL_CONFIG_OVERRIDDEN_NUM)) {
        LE_INFO("PSAP number retrieved as: %s", eCallConfig.overriddenNum.c_str());
        le_utf8_Copy(psapNumber, eCallConfig.overriddenNum.c_str(), psapNumLength, NULL);
    } else {
        LE_ERROR("Unable to get PSAP number. Error: %d", (int) status);
    }

    return Status::SUCCESS == status ? LE_OK : LE_FAULT;
}

taf_ecall_CallRef_t taf_ecall::CreateECallReference()
{
    if (ECallObject.reference == NULL)
    {
        LE_FATAL("Ecall reference not initialized");
        return NULL;
    }
    return ECallObject.reference;
}

void taf_ecall::Delete(taf_ecall_CallRef_t ecallRef)
{
    return;
}


le_result_t taf_ecall::SetECallOperatingMode(uint8_t phoneId, taf_ecall_OpMode_t eCallMode) {
    if (Phones.size() >= phoneId) {
        auto phone = Phones[phoneId - 1];
        if(phone) {
            if(eCallMode == TAF_ECALL_MODE_NORMAL  || eCallMode == TAF_ECALL_MODE_ECALL) {
                auto ret = phone->setECallOperatingMode(
                        static_cast<telux::tel::ECallMode>(eCallMode),
                        tafECallOperatingModeCallback::setECallOperatingModeResponse);
                if(ret == telux::common::Status::SUCCESS) {
                    LE_INFO("Set eCall operating mode %d request sent successfully in phoneId: %d\n",
                            (int) eCallMode, phoneId);
                    setOpModeProm = std::promise<telux::common::ErrorCode>();
                    telux::common::ErrorCode error = setOpModeProm.get_future().get();
                    if (error == telux::common::ErrorCode::SUCCESS)
                    {
                        return LE_OK;
                    }
                } else {
                    LE_ERROR("Set eCall operating mode %d failed in phoneId: %d\n", (int) eCallMode, phoneId);
                }
            } else {
                LE_ERROR("Invalid input op mode: %d phoneId: %d\n", (int) eCallMode, phoneId);
            }
        }
    } else {
        LE_ERROR("No phone found corresponding to phoneId: %d\n", phoneId);
    }
    return LE_FAULT;
}

le_result_t taf_ecall::GetECallOperatingMode(uint8_t phoneId, taf_ecall_OpMode_t *opMode) {
    TAF_ERROR_IF_RET_VAL(opMode == NULL, LE_BAD_PARAMETER, "OpMode is NULL");

    if (Phones.size() >= phoneId) {
        auto phone = Phones[phoneId - 1];
        if(phone) {
            getOpModeProm = std::promise<telux::tel::ECallMode>();
            auto ret = phone->requestECallOperatingMode(
                    tafECallOperatingModeCallback::getECallOperatingModeResponse);
            if(ret == telux::common::Status::SUCCESS) {
                LE_INFO("Get eCall op mode request sent successfully in phoneId: %d\n", phoneId);
                telux::tel::ECallMode mode = getOpModeProm.get_future().get();
                *opMode = (taf_ecall_OpMode_t)mode;
                return LE_OK;
            } else {
                LE_ERROR("Get eCall Operating mode request failed in phoneId: %d\n", phoneId);
            }
        }
    } else {
        LE_ERROR("No phone found corresponding to phoneId:  %d\n", phoneId);
    }
    return LE_FAULT;
}

le_result_t taf_ecall::StartECall(ECallCategory emergencyCategory,
                 ECallVariant eCallVariant, taf_ecall_CallRef_t ecallRef) {

    //From reference read ecall ptr object
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    //Get Selected card
    uint8_t phoneId = PhoneManager->getPhoneIdFromSlotId((int)taf_sim_GetSelectedCard());

    //Check ECall session
    if (eCallPtr->eCallSession != ECALL_INIT && (eCallPtr->eCallSession != ECALL_ENDED)) {
        LE_ERROR("Already ecall in progress");
        return LE_BUSY;
    }
    makeEcallProm = std::promise<telux::common::ErrorCode>();

    ECallObject.msd.timestamp = (uint32_t)time(NULL);

    if (eCallVariant == ECallVariant::ECALL_TEST)
    {
        ECallObject.msd.control.automaticActivation = false;
        ECallObject.msd.control.testCall = true;
    }
    else if ( emergencyCategory == ECallCategory::VOICE_EMER_CAT_AUTO_ECALL)
    {
        ECallObject.msd.control.automaticActivation = true;
        ECallObject.msd.control.testCall = false;
    }
    else if ( emergencyCategory == ECallCategory::VOICE_EMER_CAT_MANUAL)
    {
        ECallObject.msd.control.automaticActivation = false;
        ECallObject.msd.control.testCall = false;
    }

    Status ret;

    EcallConfig eCallConfig = {};
    ret = CallManager->getECallConfig(eCallConfig);
    if (ret == Status::SUCCESS) {
        LE_INFO("get eCall configuration successfully.");
    }

    LE_INFO("ECall Variant: %d, phoneId: %d, isMsdUpdated: %d\n",
            (int) eCallVariant, phoneId, (int) ECallObject.isMsdUpdated);

    //Check msd imported or not to send msd in pdu format or not
    if (ECallObject.isMsdUpdated)
    {
        const std::vector< uint8_t > eCallMsdData(begin(eCallPtr->msdPdu),end(eCallPtr->msdPdu));
        if (eCallVariant == ECallVariant::ECALL_TEST
                && eCallConfig.numType == ECallNumType::OVERRIDDEN) {
            ret = CallManager->makeECall(phoneId, eCallConfig.overriddenNum, eCallMsdData,
                    (int)emergencyCategory, tafCallCommandCallback::makeECallResponse);
        } else {
            ret = CallManager->makeECall(phoneId, eCallMsdData, (int)emergencyCategory,
                    (int)eCallVariant, tafCallCommandCallback::makeECallResponse);
        }
    }
    else
    {
        eCallPtr->msd.messageIdentifier = 1;
        ECallMsdData eCallMsdData = (ECallMsdData) eCallPtr->msd;

        if (eCallVariant == ECallVariant::ECALL_TEST
                && eCallConfig.numType == ECallNumType::OVERRIDDEN) {
            ret = CallManager->makeECall(phoneId, eCallConfig.overriddenNum, eCallMsdData,
                    (int)emergencyCategory, CallCommandCb);
        } else {
            ret = CallManager->makeECall(phoneId, eCallMsdData, (int)emergencyCategory,
                    (int)eCallVariant, CallCommandCb);
        }
    }

    if(ret == Status::SUCCESS)
    {
        LE_DEBUG("Start ECall request sent successfully");
        ECallObject.eCallSession = ECALL_REQUEST;
        telux::common::ErrorCode error = makeEcallProm.get_future().get();
        if (error == ErrorCode::SUCCESS) {
            return LE_OK;
        }
    }
    return LE_FAULT;
}

le_result_t taf_ecall::StopECall(taf_ecall_CallRef_t ecallRef) {
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    le_result_t result = LE_OK;

     try {

      std::shared_ptr<telux::tel::ICall> spCall = nullptr;
      // Iterate through the call list in the application and hangup the first Call that is
      // Active or on Hold
      if (CallManager) {
          std::vector<std::shared_ptr<telux::tel::ICall>> callList
             = CallManager->getInProgressCalls();
          for(auto callIterator = std::begin(callList); callIterator != std::end(callList);
              ++callIterator) {
             telux::tel::CallState callState = (*callIterator)->getCallState();
             if(callState != telux::tel::CallState::CALL_ENDED) {
                spCall = *callIterator;
                break;
             }
          }
          if(spCall && eCallPtr->callIndex == spCall->getCallIndex()) {
             LE_DEBUG("Sending request to hangup call ");
             spCall->hangup(HangupCb);
          } else {
             LE_ERROR("No active or on-hold call found");
             result = LE_FAULT;
          }
      } else {
          LE_ERROR("Call manager is NULL, failed to hangup the call");
             result = LE_FAULT;
      }
   } catch(const std::exception &e) {
        LE_ERROR("Exception caught");
             result = LE_FAULT;
   }

    ClearPduMsd();
    return result;
}

le_result_t taf_ecall::SetMsdPosition (taf_ecall_CallRef_t ecallRef, bool isTrusted, int32_t latitude,
            int32_t longitude, int32_t direction)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);
    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    if (eCallPtr->isMsdUpdated)
    {
        LE_ERROR("MSD position is set by importing MSD");
        return LE_DUPLICATE;
    }

    if (latitude < -324000000 || latitude > 324000000)
    {
        LE_ERROR("Invalid latitude value");
        latitude=0x7FFFFFFF;
    }
    if (longitude < -648000000 || longitude > 648000000)
    {
        LE_ERROR("Invalid longitude value");
        longitude=0x7FFFFFFF;
    }
    LE_INFO("SetMsdPosition isTrusted = %d ", isTrusted);
    LE_INFO("SetMsdPosition latitude = %d ", latitude);
    LE_INFO("SetMsdPosition longitude = %d ", longitude);
    LE_INFO("SetMsdPosition direction = %d ", direction);

    eCallPtr->msd.control.positionCanBeTrusted = isTrusted;
    eCallPtr->msd.vehicleLocation.positionLatitude = latitude;
    eCallPtr->msd.vehicleLocation.positionLongitude = longitude;
    eCallPtr->msd.vehicleDirection = direction;

    return LE_OK;
}

le_result_t taf_ecall::SetMsdPositionN1 (taf_ecall_CallRef_t ecallRef,int32_t latitudeDeltaN1,
            int32_t longitudeDeltaN1)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);
    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    if (eCallPtr->isMsdUpdated)
    {
        LE_ERROR("MSD position is set by importing MSD");
        return LE_DUPLICATE;
    }

    if (latitudeDeltaN1 < -512 || latitudeDeltaN1 > 511
        || longitudeDeltaN1 < -512 || longitudeDeltaN1 > 511)
    {
        LE_ERROR("Invalid delta value");
        return LE_FAULT;
    }

    eCallPtr->msd.optionals.recentVehicleLocationN1Present = true;
    eCallPtr->msd.recentVehicleLocationN1.latitudeDelta = latitudeDeltaN1;
    eCallPtr->msd.recentVehicleLocationN1.longitudeDelta = longitudeDeltaN1;

    return LE_OK;
}

le_result_t taf_ecall::SetMsdPositionN2 (taf_ecall_CallRef_t ecallRef,int32_t latitudeDeltaN2,
            int32_t longitudeDeltaN2)
{
    taf_ECall_t* eCallPtr =(taf_ECall_t*) le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    if (eCallPtr->isMsdUpdated)
    {
        LE_ERROR("MSD position is set by importing MSD");
        return LE_DUPLICATE;
    }

    if (latitudeDeltaN2 < -512 || latitudeDeltaN2 > 511
        || longitudeDeltaN2 < -512 || longitudeDeltaN2 > 511)
    {
        LE_ERROR("Invalid delta value");
        return LE_FAULT;
    }

    eCallPtr->msd.optionals.recentVehicleLocationN2Present = true;
    eCallPtr->msd.recentVehicleLocationN2.latitudeDelta = latitudeDeltaN2;
    eCallPtr->msd.recentVehicleLocationN2.longitudeDelta = longitudeDeltaN2;

    return LE_OK;
}

le_result_t taf_ecall::SetMsdPassengersCount (taf_ecall_CallRef_t  ecallRef, uint32_t passengerCount)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    if (eCallPtr->isMsdUpdated)
    {
        LE_ERROR("MSD position is set by importing MSD");
        return LE_DUPLICATE;
    }

    eCallPtr->msd.optionals.numberOfPassengersPresent = true;
    eCallPtr->msd.numberOfPassengers= passengerCount;

    return LE_OK;
}

void taf_ecall::ConfigChangeHandler(void* contextPtr) {

    auto &eCall = GetInstance();
    eCall.UpdateMsd();
}

void taf_ecall::UpdateMsd ()
{
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn( CFG_MODEMSERVICE_ECALL_PATH);

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_MSDVERSION))
    {
        ECallObject.msd.msdVersion = le_cfg_GetInt(iteratorRef, CFG_NODE_MSDVERSION, 0);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_MSDVIN))
    {
        char vin[TAF_ECALL_MAX_VIN_BYTES] = {0};
        le_cfg_GetString(iteratorRef, CFG_NODE_MSDVIN, vin, TAF_ECALL_MAX_VIN_BYTES, "");

        std::string vinStr = vin;

        ECallObject.msd.vehicleIdentificationNumber.isowmi = vinStr.substr(ISOWMI_START, ISOWMI_LENGTH );
        ECallObject.msd.vehicleIdentificationNumber.isovds = vinStr.substr(ISOVDS_START, ISOVDS_LENGTH);
        ECallObject.msd.vehicleIdentificationNumber.isovisModelyear =
                             vinStr.substr(ISOVIS_MODEL_YEAR_START, ISOVIS_MODEL_YEAR_LENGTH);
        ECallObject.msd.vehicleIdentificationNumber.isovisSeqPlant =
                                 vinStr.substr(ISOVIS_SEQ_PLANT_START, ISOVIS_SEQ_PLANT_LENGTH);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_MSDVEHTYPE))
    {
        ECallObject.msd.control.vehicleType = (ECallVehicleType) le_cfg_GetInt(iteratorRef, CFG_NODE_MSDVEHTYPE, 0);
    }

    le_cfg_CancelTxn(iteratorRef);

    iteratorRef = le_cfg_CreateReadTxn( CFG_ECALL_PROPULSIONTYPE_PATH);

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_GASOLINE))
    {
        ECallObject.msd.vehiclePropulsionStorage.gasolineTankPresent =
            le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_GASOLINE, false);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_DIESEL))
    {
        ECallObject.msd.vehiclePropulsionStorage.dieselTankPresent =
            le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_DIESEL, false);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_NATURALGAS))
    {
        ECallObject.msd.vehiclePropulsionStorage.compressedNaturalGas =
            le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_NATURALGAS, false);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_PROPANE))
    {
        ECallObject.msd.vehiclePropulsionStorage.liquidPropaneGas =
            le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_PROPANE, false);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_ELECTRIC))
    {
        ECallObject.msd.vehiclePropulsionStorage.electricEnergyStorage =
            le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_ELECTRIC, false);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_HYDROGEN))
    {
        ECallObject.msd.vehiclePropulsionStorage.hydrogenStorage =
            le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_HYDROGEN, false);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_OTHER))
    {
        ECallObject.msd.vehiclePropulsionStorage.otherStorage =
            le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_OTHER, false);
    }
    le_cfg_CancelTxn(iteratorRef);
}

le_result_t taf_ecall::SetMsdTxMode (taf_ecall_MsdTransmissionMode_t txMode)
{
    LE_DEBUG("Set MsdTransmission mode %d", txMode);
    ECallObject.msdTxMode = txMode;

    return LE_OK;
}

le_result_t taf_ecall::GetMsdTxMode ( taf_ecall_MsdTransmissionMode_t* modePtr)
{
    *modePtr = ECallObject.msdTxMode;
    return LE_OK;
}

le_result_t taf_ecall::ImportMsd( taf_ecall_CallRef_t ecallRef, const uint8_t* pduMsd, size_t msdLength)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    memcpy(eCallPtr->msdPdu, pduMsd, msdLength);
    eCallPtr->pduMsdSize = msdLength;
    eCallPtr->isMsdUpdated = true;
    return LE_OK;
}

le_result_t taf_ecall::ExportMsd( taf_ecall_CallRef_t ecallRef, uint8_t* pdumsd, size_t *msdLength)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");
    TAF_ERROR_IF_RET_VAL(eCallPtr->pduMsdSize > *msdLength, LE_OVERFLOW, "buffer is small");

    if (eCallPtr->isMsdUpdated)
    {
        memcpy(pdumsd, eCallPtr->msdPdu, eCallPtr->pduMsdSize);
        *msdLength = eCallPtr->pduMsdSize;
    }
    else
    {
        return LE_NOT_FOUND;
    }

    return LE_OK;
}

le_result_t taf_ecall::SendMsd( taf_ecall_CallRef_t ecallRef)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    telux::common::Status status;
    std::promise<telux::common::ErrorCode> p;
    int phoneId = PhoneManager->getPhoneIdFromSlotId((int)taf_sim_GetSelectedCard());

    eCallPtr->msd.messageIdentifier++;

    LE_INFO("Send msd in phoneId: %d, isMsdUpdated: %d\n", phoneId, (int)eCallPtr->isMsdUpdated);

    if (eCallPtr->isMsdUpdated)
    {
        const std::vector< uint8_t > eCallMsdData(begin(eCallPtr->msdPdu),end(eCallPtr->msdPdu));
        telux::common::ResponseCallback cb = [&p](telux::common::ErrorCode error) { p.set_value(error); };
        status = CallManager->updateECallMsd(phoneId, eCallMsdData, cb);
        if (status == Status::SUCCESS) {
            telux::common::ErrorCode error = p.get_future().get();
            if (error == ErrorCode::SUCCESS) {
                return LE_OK;
            }
        }
    }
    else
    {
        updateMsdProm = std::promise<telux::common::ErrorCode>();
        status = CallManager->updateECallMsd(phoneId, eCallPtr->msd, UpdateMsdCb);
        if (status == Status::SUCCESS) {
            telux::common::ErrorCode error = updateMsdProm.get_future().get();
            if (error == ErrorCode::SUCCESS) {
                return LE_OK;
            }
        }
    }
    return LE_FAULT;
}

taf_ecall_CallRef_t taf_ecall::GetECallReference()
{
    if (ECallObject.reference == NULL)
    {
        LE_FATAL("Ecall reference not initialized");
        return NULL;
    }
    return ECallObject.reference;
}

void taf_ecall::SetCallIndex(int32_t callIndex)
{
    ECallObject.callIndex = callIndex;
}

void taf_ecall::SetSessionState(tafECallSession_t session)
{
    ECallObject.eCallSession = session;

}

void taf_ecall::SetECallState(taf_ecall_State_t state)
{
    ECallObject.state = state;

}

void taf_ecall::ClearPduMsd()
{
    memset(ECallObject.msdPdu, 0, sizeof(ECallObject.msdPdu));
    ECallObject.pduMsdSize = 0;
    ECallObject.isMsdUpdated = false;
}

taf_ecall_State_t taf_ecall::GetState ( taf_ecall_CallRef_t ecallRef)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, TAF_ECALL_STATE_UNKNOWN, "Invalid eCall reference");

    return eCallPtr->state;

}

taf_ecall_TerminationReason_t taf_ecall::GetTerminationReason ( taf_ecall_CallRef_t ecallRef)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_ERROR_IF_RET_VAL(eCallPtr == NULL,
            TAF_ECALL_REASON_ERROR_UNSPECIFIED, "Invalid eCall reference");
    TAF_ERROR_IF_RET_VAL(TAF_ECALL_STATE_ENDED != taf_ecall::GetState(ecallRef),
            TAF_ECALL_REASON_NORMAL_UNSPECIFIED, "The eCall is not ENDed");

    auto &eCall = taf_ecall::GetInstance();
    // Map CDMA specific call end causes
    if (eCall.CallEndError >= telux::tel::CallEndCause::CDMA_LOCKED_UNTIL_POWER_CYCLE &&
            eCall.CallEndError <= telux::tel::CallEndCause::CDMA_ACCESS_BLOCKED) {
        eCall.CallEndError = telux::tel::CallEndCause::NORMAL_UNSPECIFIED;
    }
    LE_INFO("GetTerminationReason call end error = %d", (int) eCall.CallEndError);
    return (taf_ecall_TerminationReason_t) eCall.CallEndError;
}

le_result_t taf_ecall::UseUSimNumbers()
{
    EcallConfig eCallConfig;
    eCallConfig.configValidityMask.set(ECALL_CONFIG_NUM_TYPE);
    eCallConfig.numType = ECallNumType::DEFAULT;
    Status status = CallManager->setECallConfig(eCallConfig);
    LE_INFO("UseUSimNumbers: status %d", (int) status);
    return Status::SUCCESS == status ? LE_OK : LE_FAULT;
}

le_result_t taf_ecall::SetNadDeregistrationTime(uint16_t deregTime)
{
    if (!isIdle()) {
        LE_INFO("ECall session is in progress, try it later when session is not active");
        return LE_BUSY;
    }

    if (deregTime < 1 || deregTime > 720) {
        LE_ERROR("Error: Deregistration time %d min is not allowed [Range 1:720].", deregTime);
        return LE_FAULT;
    }

    uint32_t t10 = (uint32_t) deregTime;
    LE_INFO("Set NAD deregistration time (in minutes): %d", t10);

    std::promise<telux::common::ErrorCode> p;
    int phoneId = PhoneManager->getPhoneIdFromSlotId((int)taf_sim_GetSelectedCard());

    telux::common::ResponseCallback cb = [&p](telux::common::ErrorCode error) { p.set_value(error); };
    Status status = CallManager->updateEcallHlapTimer(phoneId, HlapTimerType::T10_TIMER, t10, cb);
    if (status == Status::SUCCESS) {
        LE_INFO("SetNadDeregistrationTime: status %d", (int) status);
        telux::common::ErrorCode error = p.get_future().get();
        LE_INFO("SetNadDeregistrationTime: error code %d", (int) error);
        if (error == ErrorCode::SUCCESS) {
            return LE_OK;
        }
    }

    return LE_FAULT;
}

le_result_t taf_ecall::GetNadDeregistrationTime(uint16_t* deregTime)
{
    if (deregTime == NULL) {
        LE_ERROR("deregTime is null.");
        return LE_FAULT;
    }

    std::promise<telux::common::ErrorCode> p;
    std::promise<uint32_t> q;
    int phoneId = PhoneManager->getPhoneIdFromSlotId((int)taf_sim_GetSelectedCard());
    telux::tel::ECallHlapTimerCallback cb =
            [&p, &q](telux::common::ErrorCode error, uint32_t timeDuration) {
                p.set_value(error);
                q.set_value(timeDuration);
            };
    Status status = CallManager->requestEcallHlapTimer(phoneId, HlapTimerType::T10_TIMER, cb);

    if (status == Status::SUCCESS) {
        LE_INFO("GetNadDeregistrationTime: status %d", (int) status);
        telux::common::ErrorCode error = p.get_future().get();
        LE_INFO("GetNadDeregistrationTime: error code %d", (int) error);
        if (error == ErrorCode::SUCCESS) {
            uint32_t t10 = q.get_future().get();
            LE_INFO("Get NAD deregistration time (T10 in minutes) fetched as: %d", t10);
            *deregTime =  (uint16_t) t10;
            LE_INFO("Get NAD deregistration time (in minutes): %d", *deregTime);
            return LE_OK;
        }
    }

    return LE_FAULT;
}

le_result_t taf_ecall::TerminateRegistration()
{
    std::promise<telux::common::ErrorCode> p;
    int phoneId = PhoneManager->getPhoneIdFromSlotId((int)taf_sim_GetSelectedCard());

    telux::common::ResponseCallback cb = [&p](telux::common::ErrorCode error) { p.set_value(error); };
    Status status = CallManager->requestNetworkDeregistration(phoneId, cb);
    LE_INFO("TerminateRegistration: status %d", (int) status);
    if (status == Status::SUCCESS) {
        telux::common::ErrorCode error = p.get_future().get();
        LE_INFO("TerminateRegistration: error code %d", (int) error);
        if (error == ErrorCode::SUCCESS) {
            return LE_OK;
        }
    }

    return LE_FAULT;
}

le_result_t taf_ecall::SetNadClearDownFallbackTime(uint16_t ccftTime)
{
    if (!isIdle()) {
        LE_INFO("ECall session is in progress, try it later when session is not active");
        return LE_BUSY;
    }

    if (ccftTime < 1 || ccftTime > 720) {
        LE_ERROR("Error: clear down fallback time %d min is not allowed [Range 1:720].", ccftTime);
        return LE_FAULT;
    }

    uint32_t t2 = (uint32_t) ccftTime*60*1000;
    LE_INFO("Set NAD clear down fallback time (in minutes): %d", ccftTime);

    EcallConfig eCallConfig;
    eCallConfig.configValidityMask.set(ECALL_CONFIG_T2_TIMER);
    eCallConfig.t2Timer = t2;
    Status status = CallManager->setECallConfig(eCallConfig);

    return Status::SUCCESS == status ? LE_OK : LE_FAULT;

}

le_result_t taf_ecall::GetNadClearDownFallbackTime(uint16_t* ccftTime)
{
    if (ccftTime == NULL) {
        LE_ERROR("ccftTime is null.");
        return LE_FAULT;
    }

    EcallConfig eCallConfig = {};
    Status status = CallManager->getECallConfig(eCallConfig);
    if (status == Status::SUCCESS && eCallConfig.configValidityMask.test(ECALL_CONFIG_T2_TIMER)) {
        LE_INFO("NAD clear down fallback time (in minutes): %d", eCallConfig.t2Timer/60000);
        *ccftTime = (uint16_t) (eCallConfig.t2Timer/60000);
    } else {
        LE_ERROR("Unable to get clear down fallback time. Error: %d", (int) status);
    }

    return Status::SUCCESS == status ? LE_OK : LE_FAULT;
}

le_result_t taf_ecall::SetNadMinNetworkRegistrationTime(uint16_t minNwRegTime)
{
    if (!isIdle()) {
        LE_INFO("ECall session is in progress, try it later when session is not active");
        return LE_BUSY;
    }

    if (minNwRegTime < 1 || minNwRegTime > 720) {
        LE_ERROR("Error: min network registration time %d min is not allowed [Range 1:720].", minNwRegTime);
        return LE_FAULT;
    }

    uint16_t minNwRegTimeGet = 0;
    le_result_t result = GetNadMinNetworkRegistrationTime(&minNwRegTimeGet);
    if ((result == LE_OK) &&
        (minNwRegTimeGet == minNwRegTime))
    {
        LE_INFO("Setting min network registration time is the same as the current value");
        return LE_OK;
    }

    uint32_t t9 = (uint32_t) minNwRegTime*60*1000;;
    LE_INFO("Set NAD min network registration time (in minutes): %d", minNwRegTime);

    EcallConfig eCallConfig;
    eCallConfig.configValidityMask.set(ECALL_CONFIG_T9_TIMER);
    eCallConfig.t9Timer = t9;
    Status status = CallManager->setECallConfig(eCallConfig);
    if (Status::SUCCESS != status)
    {
        LE_ERROR("Unable to set min network registration time. Error: %d", (int) status);
        return LE_FAULT;
    }

    uint16_t minNwRegTimeGetAfterSet = 0;
    result = GetNadMinNetworkRegistrationTime(&minNwRegTimeGetAfterSet);
    if ((result == LE_OK) &&
        (minNwRegTimeGetAfterSet != minNwRegTime))
    {
        LE_ERROR("Error: Setting min network registration time is not consistent with get.");
        t9 = (uint32_t) minNwRegTimeGet*60*1000;;
        eCallConfig.configValidityMask.set(ECALL_CONFIG_T9_TIMER);
        eCallConfig.t9Timer = t9;
        status = CallManager->setECallConfig(eCallConfig);
        if (Status::SUCCESS != status)
        {
            LE_ERROR("Unable to set the previous min network registration time. Error: %d", (int) status);
        }
        return LE_FAULT;
    }
    return result;
}

le_result_t taf_ecall::GetNadMinNetworkRegistrationTime(uint16_t* minNwRegTime)
{
    if (minNwRegTime == NULL) {
        LE_ERROR("minNwRegTime is null.");
        return LE_FAULT;
    }

    EcallConfig eCallConfig = {};
    Status status = CallManager->getECallConfig(eCallConfig);
    if (status == Status::SUCCESS && eCallConfig.configValidityMask.test(ECALL_CONFIG_T9_TIMER)) {
        LE_INFO("NAD min network registration time (in minutes): %d", eCallConfig.t9Timer/60000);
        *minNwRegTime = (uint16_t) (eCallConfig.t9Timer/60000);
    } else {
        LE_ERROR("Unable to get min network registration time. Error: %d", (int) status);
    }

    return Status::SUCCESS == status ? LE_OK : LE_FAULT;
}

taf_ecall_StateChangeHandlerRef_t taf_ecall::AddStateChangeHandler
        (taf_ecall_StateChangeHandlerFunc_t handlerPtr,
        void* contextPtr){
    le_event_HandlerRef_t handlerRef;

    TAF_KILL_CLIENT_IF_RET_VAL(handlerPtr == NULL, NULL, "Handler pointer is NULL");

    handlerRef = le_event_AddLayeredHandler("NewStateHandler", StateChangeEventId,
            FirstLayerStateChangeHandler, (void*)handlerPtr);

    return (taf_ecall_StateChangeHandlerRef_t) handlerRef;
}

void taf_ecall::RemoveStateChangeHandler (taf_ecall_StateChangeHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

void taf_ecall::FirstLayerStateChangeHandler(void* reportPtr,
        void* secondLayerHandlerFunc)
{

    StateChangeEvent_t* stateEventPtr = (StateChangeEvent_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(stateEventPtr == NULL,"stateEventPtr is NULL");

    taf_ecall_StateChangeHandlerFunc_t clientHandlerFunc =
        (taf_ecall_StateChangeHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(stateEventPtr->eCallRef, stateEventPtr->state, le_event_GetContextPtr());

}
