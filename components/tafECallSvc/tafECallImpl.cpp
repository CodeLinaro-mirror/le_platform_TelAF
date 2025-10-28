/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafECall.hpp"

using namespace telux::tel;
using namespace telux::common;
using namespace tafsvc;

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
    auto &eCall = taf_ecall::GetInstance();
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        LE_INFO("Call is successful ");
        if (call)
        {
            int32_t callIndex = call->getCallIndex();;
            int8_t phoneId = call->getPhoneId();
            eCall.SetCallIndex(callIndex);
            eCall.SetCallPhoneId(phoneId);
            LE_INFO("makeCallResponse %d, %d", callIndex, phoneId);
        }
    } else {
        LE_ERROR("Call failed with error code: %d ", (static_cast<int>(errorCode)));
    }
    eCall.makeEcallProm.set_value(errorCode);
}

void tafCallCommandCallback::makeECallResponse(telux::common::ErrorCode errorCode,
                                                      std::shared_ptr<telux::tel::ICall> call) {
    auto &eCall = taf_ecall::GetInstance();
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        LE_INFO("Call is successful ");
        if (call)
        {
            int32_t callIndex = call->getCallIndex();;
            int8_t phoneId = call->getPhoneId();
            eCall.SetCallIndex(callIndex);
            eCall.SetCallPhoneId(phoneId);
            LE_INFO("makeCallResponse %d, %d", callIndex, phoneId);
        }
    } else {
        LE_ERROR("Call failed with error code: %d ", (static_cast<int>(errorCode)));
    }
    eCall.makeEcallProm.set_value(errorCode);
}

void tafPrieCallCommandCallback::makeECallResponse(telux::common::ErrorCode errorCode,
                                                      std::shared_ptr<telux::tel::ICall> call) {
    auto &eCall = taf_ecall::GetInstance();
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        LE_INFO("Call is successful ");
        if (call)
        {
            int32_t callIndex = call->getCallIndex();;
            int8_t phoneId = call->getPhoneId();
            eCall.SetCallIndex(callIndex);
            eCall.SetCallPhoneId(phoneId);
            LE_INFO("makeCallResponse %d, %d", callIndex, phoneId);
        }

    } else {
        LE_ERROR("Call failed with error code: %d ", (static_cast<int>(errorCode)));
    }
    eCall.makePrieCallProm.set_value(errorCode);
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
   auto &eCall = taf_ecall::GetInstance();
   eCall.hangupProm.set_value(errorCode);
}

void tafRejectCommandCallback::commandResponse(telux::common::ErrorCode errorCode) {
   if(errorCode == telux::common::ErrorCode::SUCCESS) {
      LE_INFO("Call reject is successful ");
   } else {
      LE_ERROR("Call reject failed with error code: %d ", (static_cast<int>(errorCode)));
   }
   auto &eCall = taf_ecall::GetInstance();
   eCall.rejectProm.set_value(errorCode);
}

void tafAnswerCommandCallback::commandResponse(telux::common::ErrorCode errorCode) {
   if(errorCode == telux::common::ErrorCode::SUCCESS) {
      LE_INFO("Call answer is successful ");
   } else {
      LE_ERROR("Call answer failed with error code: %d ", (static_cast<int>(errorCode)));
   }
   auto &eCall = taf_ecall::GetInstance();
   eCall.answerProm.set_value(errorCode);
}

void tafConfigRedialCallback::configureRedialResponse(
    telux::common::ErrorCode error) {
    auto &eCall = taf_ecall::GetInstance();
    if (error == telux::common::ErrorCode::SUCCESS) {
        LE_INFO("Configure redial executed successfully");
    } else {
        LE_ERROR( "Configure redial failed error = %d", (int)error);
    }
    eCall.configRedialProm.set_value(error);
}

void tafResumeHlapTimerCallback::resumeHlapTimerResponse(telux::common::ErrorCode error) {
    auto &eCall = taf_ecall::GetInstance();
    if (error == telux::common::ErrorCode::SUCCESS) {
        LE_INFO("Resume hlap timer executed successfully");
    } else {
        LE_ERROR( "Restart hlap timer failed error = %d", (int)error);
    }
    eCall.resumeHlapTimerProm.set_value(error);
}

void tafECallListener::onIncomingCall(std::shared_ptr<telux::tel::ICall> call) {
    TAF_ERROR_IF_RET_NIL(call == nullptr, "call is nullptr!");

    auto &eCall = taf_ecall::GetInstance();
    int8_t phoneId = call->getPhoneId(); 
    std::promise<telux::common::ErrorCode> p;
        std::promise<int> q;
        std::promise<ECallHlapTimerStatus> r;
        telux::tel::ECallHlapTimerStatusCallback cb =
            [&p, &q, &r](telux::common::ErrorCode error, int phoneId, ECallHlapTimerStatus hlapTimerStatus) {
            p.set_value(error);
            q.set_value(phoneId);
            r.set_value(hlapTimerStatus);
        };

    if (eCall.CallManager->requestECallHlapTimerStatus(phoneId, cb) == Status::SUCCESS)
    {
        if (p.get_future().get() == ErrorCode::SUCCESS) {
            if (phoneId == q.get_future().get() &&
                telux::tel::HlapTimerStatus::ACTIVE == r.get_future().get().t9)
            {
                if (telux::tel::CallState::CALL_INCOMING == call->getCallState())
                {
                    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(eCall.ECallPtrRefMap, eCall.GetECallReference());
                    if (eCallPtr != NULL)
                    {
                        eCallPtr->iCall= call;
                        taf_ecall_State_t state = TAF_ECALL_STATE_UNKNOWN;
                        tafECallSession_t sessionState = ECALL_INIT;
                        state = TAF_ECALL_STATE_INCOMING;
                        eCall.SetECallState(state);
                        sessionState = ECALL_INCOMING;
                        eCall.SetSessionState(sessionState);
                        eCall.SetCallIndex(call->getCallIndex());
                        eCall.SetCallPhoneId(call->getPhoneId());

                        StateChangeEvent_t stateEvent = { 0 };
                        le_utf8_Copy(stateEvent.dest, call->getRemotePartyNumber().c_str(), MAX_DESTINATION_LEN, NULL);
                        stateEvent.eCallRef = eCall.GetECallReference();
                        stateEvent.state = state;
                        stateEvent.phoneId = phoneId;
                        le_event_Report(eCall.StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));
                    } else {
                        LE_ERROR("eCallPtr is nullPtr");
                    }
                }
            } else {
                LE_ERROR("T9 is not in active state when receives the incoming call event");
            }
        } else {
            LE_ERROR("Get eCall hlap timer failed with error code = %d", static_cast<int>(p.get_future().get()));
        }
    } else {
        LE_ERROR("Get eCall hlap timer failed");
    }
}

void tafECallListener::onCallInfoChange(std::shared_ptr<telux::tel::ICall> call) {
    LE_INFO("onCallInfoChange");
    taf_ecall_State_t state = TAF_ECALL_STATE_UNKNOWN;
    tafECallSession_t sessionState = ECALL_INIT;

    auto &eCall = taf_ecall::GetInstance();
    CallState callState = call->getCallState();
    int8_t phoneId = call->getPhoneId();
    int32_t index = call->getCallIndex();

    bool isCallStateSet = false;

    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(eCall.ECallPtrRefMap, eCall.GetECallReference());
    TAF_ERROR_IF_RET_NIL(eCallPtr == NULL, "cannot get callptr");
    LE_INFO("onCallInfoChange index %d %d, phoneId  %d, %d, state %d", eCallPtr->callIndex, index, eCallPtr->phoneId, phoneId, (int)callState);
    if (((eCallPtr->callIndex != index) && (eCallPtr->phoneId == phoneId)) ||
        (eCallPtr->phoneId != phoneId))
    {
        LE_ERROR("Cannot match the index or phoneId");
        return;
    }

    eCall.CallEndError = telux::tel::CallEndCause::NORMAL;
    LE_INFO("Call state: %d", (int) callState);

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
        if ((call->getCallDirection() == CallDirection::INCOMING) ||
            ((eCallPtr->type != TAF_ECALL_TYPE_TEST) &&
             (eCallPtr->type != TAF_ECALL_TYPE_AUTO) &&
             (eCallPtr->type != TAF_ECALL_TYPE_MANUAL) &&
             (call->getCallDirection() == CallDirection::OUTGOING))
        )
        {
            state = TAF_ECALL_STATE_ENDED;
            isCallStateSet = true;
            eCall.SetCallIndex(-1);
            eCall.SetCallPhoneId(-1);
        }

        eCallPtr->waitForALACKPos = false;
        sessionState = ECALL_ENDED;
        eCall.CallEndError = call->getCallEndCause();
        LE_INFO("ECall ENDed terminate reason = %d", (int) eCall.CallEndError);
    }
    eCall.SetSessionState(sessionState);
    eCall.SetECallState(state);
    if (isCallStateSet)
    {
        eCallPtr->iCall= call;
        StateChangeEvent_t stateEvent;
        stateEvent.eCallRef = eCall.GetECallReference();
        stateEvent.state = state;
        stateEvent.phoneId = phoneId;
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
    auto &eCall = taf_ecall::GetInstance();

    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(eCall.ECallPtrRefMap, eCall.GetECallReference());
    if (eCallPtr == nullptr)
    {
        LE_ERROR("eCallPtr is nullptr.");
        return TAF_ECALL_STATE_UNKNOWN;
    }

    LE_DEBUG("eCallMsdTransmissionStatusToState status = %d", (int)status);

    taf_ecall_State_t state = TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED;
    StateChangeEvent_t stateEvent;
    stateEvent.eCallRef = eCall.GetECallReference();
    switch(status) {
        case telux::tel::ECallMsdTransmissionStatus::SUCCESS:
            if (eCallPtr->waitForALACKPos)
            {
                stateEvent.state = TAF_ECALL_STATE_ALACK_RECEIVED_POSITIVE;
                le_event_Report(eCall.StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));
                eCallPtr->waitForALACKPos = false;
            }
            state = TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS;
            break;
        case telux::tel::ECallMsdTransmissionStatus::FAILURE:
            state = TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED;
            break;
        case telux::tel::ECallMsdTransmissionStatus::MSD_TRANSMISSION_STARTED:
            state = TAF_ECALL_STATE_MSD_TRANSMISSION_STARTED;
            eCallPtr->waitForALACKPos = false;
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
            eCallPtr->waitForALACKPos = true;
            break;
        case telux::tel::ECallMsdTransmissionStatus::MSD_AL_ACK_CLEARDOWN:
            state = TAF_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN;
            eCallPtr->waitForALACKPos = false;
            break;
        case telux::tel::ECallMsdTransmissionStatus::LL_NACK_DUE_TO_T7_EXPIRY:
            state = TAF_ECALL_STATE_LL_NACK_DUE_TO_T7_EXPIRY;
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

    stateEvent.state = state;

    le_event_Report(eCall.StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));

    return state;
}

void tafECallListener::onECallMsdTransmissionStatus(
        int phoneId, telux::tel::ECallMsdTransmissionStatus msdTransmissionStatus) {
    eCallMsdTransmissionStatusToState(msdTransmissionStatus);
}

void tafECallListener::onECallHlapTimerEvent(int phoneId, ECallHlapTimerEvents timerEvents) {
    LE_DEBUG("onECallHlapTimerEvent t2: %d, t5: %d, t6: %d, t7:  %d, t9: %d, t10: %d",
        static_cast<int>(timerEvents.t2), static_cast<int>(timerEvents.t5), static_cast<int>(timerEvents.t6),
        static_cast<int>(timerEvents.t7), static_cast<int>(timerEvents.t9), static_cast<int>(timerEvents.t10));

    taf_ecall_State_t state = TAF_ECALL_STATE_UNKNOWN;
    auto &eCall = taf_ecall::GetInstance();
    StateChangeEvent_t stateEvent;
    stateEvent.eCallRef = eCall.GetECallReference();

    if ((timerEvents.t2 != HlapTimerEvent::UNCHANGED)
        && (timerEvents.t2 != HlapTimerEvent::UNKNOWN)) {
        if(timerEvents.t2 == HlapTimerEvent::EXPIRED) {
            state = TAF_ECALL_STATE_T2_EXPIRED;
            eCall.t2StartTimeSet = false;
        }
        if(timerEvents.t2 == HlapTimerEvent::STARTED) {
            state = TAF_ECALL_STATE_T2_STARTED;
            eCall.t2StartTime = std::chrono::steady_clock::now();
            eCall.t2StartTimeSet = true;
        }
        if(timerEvents.t2 == HlapTimerEvent::STOPPED) {
            state = TAF_ECALL_STATE_T2_STOPPED;
            eCall.t2StartTimeSet = false;
        }

        if (state != TAF_ECALL_STATE_UNKNOWN) {
            stateEvent.state = state;
            le_event_Report(eCall.StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));
        }
    }

    if ((timerEvents.t5 != HlapTimerEvent::UNCHANGED)
        && (timerEvents.t5 != HlapTimerEvent::UNKNOWN)) {
        if(timerEvents.t5 == HlapTimerEvent::EXPIRED) {
            state = TAF_ECALL_STATE_T5_EXPIRED;
        }
        if(timerEvents.t5 == HlapTimerEvent::STARTED) {
            state = TAF_ECALL_STATE_T5_STARTED;
        }
        if(timerEvents.t5 == HlapTimerEvent::STOPPED) {
            state = TAF_ECALL_STATE_T5_STOPPED;
        }

        if (state != TAF_ECALL_STATE_UNKNOWN) {
            stateEvent.state = state;
            le_event_Report(eCall.StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));
        }
    }

    if ((timerEvents.t6 != HlapTimerEvent::UNCHANGED)
        && (timerEvents.t6 != HlapTimerEvent::UNKNOWN)) {
        if(timerEvents.t6 == HlapTimerEvent::EXPIRED) {
            state = TAF_ECALL_STATE_T6_EXPIRED;
        }
        if(timerEvents.t6 == HlapTimerEvent::STARTED) {
            state = TAF_ECALL_STATE_T6_STARTED;
        }
        if(timerEvents.t6 == HlapTimerEvent::STOPPED) {
            state = TAF_ECALL_STATE_T6_STOPPED;
        }

        if (state != TAF_ECALL_STATE_UNKNOWN) {
            stateEvent.state = state;
            le_event_Report(eCall.StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));
        }
    }

    if ((timerEvents.t7 != HlapTimerEvent::UNCHANGED)
        && (timerEvents.t7 != HlapTimerEvent::UNKNOWN)) {
        if(timerEvents.t7 == HlapTimerEvent::EXPIRED) {
            state = TAF_ECALL_STATE_T7_EXPIRED;
        }
        if(timerEvents.t7 == HlapTimerEvent::STARTED) {
            state = TAF_ECALL_STATE_T7_STARTED;
        }
        if(timerEvents.t7 == HlapTimerEvent::STOPPED) {
            state = TAF_ECALL_STATE_T7_STOPPED;
        }

        if (state != TAF_ECALL_STATE_UNKNOWN) {
            stateEvent.state = state;
            le_event_Report(eCall.StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));
        }
    }

    if ((timerEvents.t9 != HlapTimerEvent::UNCHANGED)
        && (timerEvents.t9 != HlapTimerEvent::UNKNOWN)) {
        if(timerEvents.t9 == HlapTimerEvent::EXPIRED) {
            state = TAF_ECALL_STATE_T9_EXPIRED;
            eCall.t9StartTimeSet = false;
        }
        if(timerEvents.t9 == HlapTimerEvent::STARTED) {
            state = TAF_ECALL_STATE_T9_STARTED;
            eCall.t9StartTime = std::chrono::steady_clock::now();
            eCall.t9StartTimeSet = true;
            eCall.ElapsedTimeT9 = 0;
        }
        if(timerEvents.t9 == HlapTimerEvent::STOPPED) {
            state = TAF_ECALL_STATE_T9_STOPPED;
            eCall.t9StartTimeSet = false;
        }
        if(timerEvents.t9 == HlapTimerEvent::RESUMED) {
            state = TAF_ECALL_STATE_T9_RESUMED;
            eCall.t9StartTime = std::chrono::steady_clock::now();
            eCall.t9StartTimeSet = true;
        }

        if (state != TAF_ECALL_STATE_UNKNOWN) {
            stateEvent.state = state;
            le_event_Report(eCall.StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));

            ResumeHlapTimerEvent_t resumeEvent;
            resumeEvent.event  = EVENT_SAVE_HLAP_TIMER_ELAPSED;
            resumeEvent.hlapTimerType  = HLAP_TIMER_TYPE_T9;
            resumeEvent.hlapTimerEventType = eCall.ConvertHlapTimerEvent(timerEvents.t9);
            le_event_Report(eCall.ResumeHlapTimerEventId, &resumeEvent, sizeof(ResumeHlapTimerEvent_t));
        }
    }

    if ((timerEvents.t10 != HlapTimerEvent::UNCHANGED)
        && (timerEvents.t10 != HlapTimerEvent::UNKNOWN)) {
        if(timerEvents.t10 == HlapTimerEvent::EXPIRED) {
            state = TAF_ECALL_STATE_T10_EXPIRED;
            eCall.t10StartTimeSet = false;
        }
        if(timerEvents.t10 == HlapTimerEvent::STARTED) {
            state = TAF_ECALL_STATE_T10_STARTED;
            eCall.t10StartTime = std::chrono::steady_clock::now();
            eCall.t10StartTimeSet = true;
        }
        if(timerEvents.t10 == HlapTimerEvent::STOPPED) {
            state = TAF_ECALL_STATE_T10_STOPPED;
            eCall.t10StartTimeSet = false;
        }

        if (state != TAF_ECALL_STATE_UNKNOWN) {
            stateEvent.state = state;
            le_event_Report(eCall.StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));
        }
    }
}

void tafECallListener::OnMsdUpdateRequest(int phoneId) {
    LE_DEBUG("OnMsdUpdateRequest");
    taf_ecall_State_t state = TAF_ECALL_STATE_UNKNOWN;

    state = TAF_ECALL_STATE_MSD_UPDATE_REQ;
    if (state != TAF_ECALL_STATE_UNKNOWN) {
        auto &eCall = taf_ecall::GetInstance();
        StateChangeEvent_t stateEvent;
        stateEvent.eCallRef = eCall.GetECallReference();
        stateEvent.state = state;
        le_event_Report(eCall.StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));
    }
}

void tafECallListener::onECallRedial(int phoneId, ECallRedialInfo info) {
    LE_DEBUG("onECallRedial");
    auto &eCall = taf_ecall::GetInstance();
    taf_ecall_State_t state = TAF_ECALL_STATE_UNKNOWN;
    StateChangeEvent_t stateEvent;

    if (info.willECallRedial == true)
    {
        state = TAF_ECALL_STATE_END_OF_REDIAL_PERIOD;
    } else {
        state = TAF_ECALL_STATE_ENDED;
        eCall.SetCallIndex(-1);
        eCall.SetCallPhoneId(-1);
    }

    eCall.SetECallState(state);
    stateEvent.eCallRef = eCall.GetECallReference();
    stateEvent.state = state;
    le_event_Report(eCall.StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));
}

void tafECallPhoneListener::onECallOperatingModeChange(int phoneId, telux::tel::ECallModeInfo info) {
    LE_INFO("onECallOperatingModeChange operation mode is %d", (int)info.mode);
    auto &eCall = taf_ecall::GetInstance();
    ResumeHlapTimerEvent_t resumeEvent;
    resumeEvent.event  = EVENT_ECALL_MODE_CHANGE;
    resumeEvent.phoneId  = phoneId;
    resumeEvent.eCallMode = info.mode;
    le_event_Report(eCall.ResumeHlapTimerEventId, &resumeEvent, sizeof(ResumeHlapTimerEvent_t));
}

void tafECallModemEvtListener::onStateChange(telux::common::SubsystemInfo subsystemInfo,
                telux::common::OperationalStatus newOperationalStatus) {
    LE_INFO("onStateChange Location %d, Subsystem %d, New status %d",
        static_cast<int>(subsystemInfo.location), static_cast<int>(subsystemInfo.subsystems), static_cast<int>(newOperationalStatus));

    auto &eCall = taf_ecall::GetInstance();
    if(newOperationalStatus == telux::common::OperationalStatus::UNAVAILABLE)
    {
        if (eCall.t9StartTimeSet == true)
        {
            eCall.ElapsedTimeT9 = eCall.ElapsedTimeT9 + eCall.ConvertElapsedTime(eCall.t9StartTime);
            LE_INFO("ElapsedTimeT9 is %d when operation status is unavailable", eCall.ElapsedTimeT9);
            eCall.t9StartTimeSet = false;
        }
    } else if(newOperationalStatus == telux::common::OperationalStatus::OPERATIONAL) {
        ResumeHlapTimerEvent_t resumeEvent;
        resumeEvent.event  = EVENT_MODEM_REBOOT;
        le_event_Report(eCall.ResumeHlapTimerEventId, &resumeEvent, sizeof(ResumeHlapTimerEvent_t));
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
#if defined(LE_CONFIG_ENABLE_ECALL_MSD_V3)
    ECallObject.msd.msdVersion = MSD_VERSION_TWO;
#endif
    ECallObject.msd.messageIdentifier = ReadMsdMsgIdFromConfigTree();
    if (ECallObject.msd.messageIdentifier < MIN_MSD_MESSAGE_IDENTIFIER || ECallObject.msd.messageIdentifier > MAX_MSD_MESSAGE_IDENTIFIER)
    {
        ECallObject.msd.messageIdentifier = 0;
        LE_WARN("Out-of-range messageIdentifier recovered to default");
    }
    LE_DEBUG("MSD messageIdentifier is %d", ECallObject.msd.messageIdentifier);

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

    uint32_t timeStamp = 0;
    if (!ReadMsdTimeStampFromConfigTree(CFG_NODE_MSDTIMESTAMPSET, &timeStamp))
    {
        LE_DEBUG("Failed to read the MSD timeStamp from config tree msdTimeStampSet.");
        if (!ReadMsdTimeStampFromConfigTree(CFG_NODE_MSDTIMESTAMPSYSTEM, &timeStamp))
        {
            LE_DEBUG("Failed to read the MSD timeStamp from config tree msdTimeStampSystem.");
        }
    }
    ECallObject.msd.timestamp = timeStamp;
    LE_DEBUG("InitializeECallPtr timestamp = %d", ECallObject.msd.timestamp);

    ECallObject.msd.vehicleLocation.positionLatitude = 0;
    ECallObject.msd.vehicleLocation.positionLongitude = 0;

    ECallObject.msd.vehicleDirection = 0;

    ECallObject.msd.recentVehicleLocationN1.latitudeDelta = 0;
    ECallObject.msd.recentVehicleLocationN1.longitudeDelta = 0;
    ECallObject.msd.recentVehicleLocationN2.latitudeDelta = 0;
    ECallObject.msd.recentVehicleLocationN2.longitudeDelta = 0;

    ECallObject.msd.numberOfPassengers = 0;
#if defined(LE_CONFIG_ENABLE_ECALL_MSD_OPTIONAL_DATA)
    ECallObject.euroNCAPData.locationOfImpact = TAF_ECALL_LOI_UNKNOWN;
    ECallObject.euroNCAPData.rolloverDetectedPresent = false;
    ECallObject.euroNCAPData.rangeLimit = MSD_EURONCAP_OAD_DELTAV_INVALD;
    ECallObject.euroNCAPData.deltaVX = MSD_EURONCAP_OAD_DELTAV_INVALD;
    ECallObject.euroNCAPData.deltaVY = MSD_EURONCAP_OAD_DELTAV_INVALD;
#endif
    //ECallObject.msd.optionalPdu.eCallDefaultOptions.objId. =;
    ECallObject.msd.optionalPdu.eCallDefaultOptions.optionalData = '\0';

    ECallObject.iCall = nullptr;
    ECallObject.reference = (taf_ecall_CallRef_t)le_ref_CreateRef(ECallPtrRefMap, &ECallObject);

    ECallObject.msdTxMode = TAF_ECALL_MSD_TX_MODE_PUSH;

    ECallObject.eCallSession = ECALL_INIT;
    ECallObject.state = TAF_ECALL_STATE_UNKNOWN;

    ECallObject.isMsdUpdated = false;

    ECallObject.isPrieCallOngoing = false;
    ECallObject.type = TAF_ECALL_TYPE_UNKNOWN;

    ECallObject.dialRedial.dialAttempts = 0;
    memset(ECallObject.dialRedial.dialInterval, 0, sizeof(ECallObject.dialRedial.dialInterval));
    if(!CallManager) {
        LE_ERROR("Can't get call manager");
    } else {
        std::vector<int> initFailPara = {};
        std::vector<int> callDropPara = {};
        telux::common::ErrorCode errorCode = CallManager->getECallRedialConfig(initFailPara, callDropPara);
        if(errorCode == telux::common::ErrorCode::SUCCESS) {
            if (initFailPara.size() > TAF_ECALL_MAX_DIAL_ATTEMPTS_LENGTH)
            {
                initFailPara.resize(TAF_ECALL_MAX_DIAL_ATTEMPTS_LENGTH);
                if ( LE_OK != ConfigureInitialDialRedial(initFailPara))
                {
                    LE_ERROR("Failed to configureInitialDialRedial values");
                } else {
                    ECallObject.dialRedial.dialAttempts = TAF_ECALL_MAX_DIAL_ATTEMPTS_LENGTH;
                    for (size_t i = 0; i < ECallObject.dialRedial.dialAttempts; ++i) {
                        ECallObject.dialRedial.dialInterval[i] = initFailPara[i] / 1000;
                    }
                }
            } else {
                ECallObject.dialRedial.dialAttempts = initFailPara.size();
                for (size_t i = 0; i < ECallObject.dialRedial.dialAttempts; ++i) {
                    ECallObject.dialRedial.dialInterval[i] = initFailPara[i] / 1000;
                }
            }
        } else {
            LE_ERROR("Failed to get eCall redial configuration parameters");
        }
    }

    ECallObject.callIndex = -1;
    ECallObject.phoneId = -1;
    ECallObject.waitForALACKPos = false;
    UpdateMsd();
}

void taf_ecall::Init(void)
{
    //  Get the PhoneFactory and PhoneManager instances.
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    std::promise<telux::common::ServiceStatus> prom;

    CallManager = phoneFactory.getCallManager([&](telux::common::ServiceStatus status) {
        LE_INFO("Getting status: %d from call manager", (int)status);
        // If the status is SERVICE_UNAVAILABLE, the call manager will also update the status through initCB
        if (status != telux::common::ServiceStatus::SERVICE_UNAVAILABLE)
        {
            prom.set_value(status);
        }
    });
    if (!CallManager)
    {
        LE_FATAL("Can't get call manager");
    }

    std::future<telux::common::ServiceStatus> initFuture = prom.get_future();
    std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(MAX_INIT_TIMEOUT));
    telux::common::ServiceStatus serviceStatus;
    if (std::future_status::timeout == waitStatus)
    {
        LE_FATAL ("Timeout waiting for susbsytem");
    }
    else
    {
        serviceStatus = initFuture.get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
        {
            LE_FATAL(" *** ERROR - Unable to initialize call subsystem");
        }
    }

    std::promise<telux::common::ServiceStatus> phoneMgrprom;
    PhoneManager = PhoneFactory::getInstance().getPhoneManager([&] (telux::common::ServiceStatus status) {
        LE_INFO("Getting status: %d from phone manager", (int)status);
        // If the status is SERVICE_UNAVAILABLE, the call manager will also update the status through initCB
        if (status != telux::common::ServiceStatus::SERVICE_UNAVAILABLE)
        {
            phoneMgrprom.set_value(status);
        }
    });
    if (!PhoneManager)
    {
        LE_FATAL("Can't get phone manager");
    }

    telux::common::ServiceStatus phoneMgrStatus = PhoneManager->getServiceStatus();
    if (phoneMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LE_INFO("telephony subsystem is not ready, wait for it to be ready");
        std::future<telux::common::ServiceStatus> initFuture = phoneMgrprom.get_future();
        std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(MAX_INIT_TIMEOUT));
        if (std::future_status::timeout == waitStatus)
        {
            LE_FATAL("Timeout waiting for susbsytem");
        }
        else
        {
            serviceStatus = initFuture.get();
            if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
            {
                LE_FATAL(" *** ERROR - Unable to initialize phone subsystem");
            }
        }
    }
    //  Exit the service, if SDK is unable to initialize telephony subsystems
    std::vector<int> phoneIds;
    telux::common::Status status = PhoneManager->getPhoneIds(phoneIds);
    if (status == telux::common::Status::SUCCESS)
    {
        for (auto index = 1; index <= (int)phoneIds.size(); index++)
        {
            auto phone = PhoneManager->getPhone(index);
            if (phone != nullptr)
            {
                Phones.emplace_back(phone);
            }
        }
    }

    auto &subsystemFact = telux::platform::SubsystemFactory::getInstance();
    std::promise<telux::common::ServiceStatus> subsystemMgrprom{};

    subsystemMgr = subsystemFact.getSubsystemManager(
            [&subsystemMgrprom](telux::common::ServiceStatus srvStatus) {
        subsystemMgrprom.set_value(srvStatus);
    });

    if (!subsystemMgr) {
        LE_FATAL("Couldn't get the subsystemMgr");
    }

    initFuture = subsystemMgrprom.get_future();
    waitStatus = initFuture.wait_for(std::chrono::seconds(MAX_INIT_TIMEOUT));
    if (std::future_status::timeout == waitStatus)
    {
        LE_FATAL("Timeout waiting for subsystem");
    }
    else
    {
        serviceStatus = initFuture.get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LE_ERROR("*** ERROR - Unable to initialize subsystem");
        } else {
            telux::common::SubsystemInfo subsysInfo{};
            std::vector<telux::common::SubsystemInfo> listOfSubsystems;
            stateListener = std::make_shared<tafECallModemEvtListener>();

            subsysInfo.location = telux::common::ProcType::LOCAL_PROC;
            subsysInfo.subsystems = telux::common::Subsystem::MPSS;
            listOfSubsystems.push_back(subsysInfo);
            telux::common::ErrorCode ec = subsystemMgr->registerListener(stateListener, listOfSubsystems);
            if (ec!= telux::common::ErrorCode::SUCCESS) {
                LE_ERROR("Can't register listener for modem event!\n");
            }
        }
    }

    InitializeECallPtr();

    ECallListener =  std::make_shared<tafECallListener>();
    Status ret = CallManager->registerListener(ECallListener);
    if(ret!= Status::SUCCESS)
    {
        LE_CRIT("Cannot register Listener for ecall event!\n");
    }

    phoneListener =  std::make_shared<tafECallPhoneListener>();
    ret = PhoneManager->registerListener(phoneListener);
    if(ret!= Status::SUCCESS)
    {
        LE_CRIT("Cannot register Listener for phone event!\n");
    }

    CallCommandCb = std::make_shared<tafCallCommandCallback>();
    UpdateMsdCb = std::make_shared<tafUpdateMsdCommandCallback>();
    HangupCb = std::make_shared<tafHangupCommandCallback>();
    RejectCb = std::make_shared<tafRejectCommandCallback>();
    AnswerCb = std::make_shared<tafAnswerCommandCallback>();

    StateChangeEventId = le_event_CreateId("NewStateEventId", sizeof(StateChangeEvent_t));

    le_cfg_AddChangeHandler(CFG_MODEMSERVICE_ECALL_PATH, ConfigChangeHandler, NULL);

    ResumeHlapTimerEventId = le_event_CreateId("ResumeHlapTimerEventId", sizeof(ResumeHlapTimerEvent_t));
    le_event_AddHandler("Resume Hlap Timer Event Handler", ResumeHlapTimerEventId, ResumeHlapTimerEventHandler);

    elapsedTimeT9Ref = le_timer_Create("elapsedTimeT9");
    le_timer_SetMsInterval(elapsedTimeT9Ref, 60000);
    le_timer_SetHandler(elapsedTimeT9Ref, T9TimerExpiryHandler);
    le_timer_SetRepeat(elapsedTimeT9Ref, 0);
    le_timer_SetWakeup(elapsedTimeT9Ref, false);

    uint16_t minNwRegTime = 0;
    bool needToResumeT9 = false;
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn( CFG_ECALL_HLAPTIMERELAPSED_PATH );
    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T9))
    {
        ElapsedTimeT9 = le_cfg_GetInt(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T9, 0);
        LE_INFO("ElapsedTimeT9 is %d when tafECallSvc is initiated", ElapsedTimeT9);
        if (LE_OK == GetNadMinNetworkRegistrationTime(&minNwRegTime))
        {
            if (ElapsedTimeT9 < minNwRegTime*60)
            {
                needToResumeT9 = true;
            }
        } else {
            LE_ERROR("GetNadMinNetworkRegistrationTime failed");
        }
    } else {
        LE_INFO("CFG_NODE_HLAPTIMERELAPSED_T9 node not exists");
    }
    le_cfg_CancelTxn(iteratorRef);

    if (needToResumeT9 == true)
    {
        taf_ecall_OpMode_t opMode;
        int phoneId = PhoneManager->getPhoneIdFromSlotId((int)taf_sim_GetSelectedCard());
        LE_INFO("phoneId is %d", phoneId);
        if (LE_OK != GetECallOperatingMode(phoneId, &opMode))
        {
            LE_INFO("Get eCall operating mode failed");
            pendingToResumeHlapTimer = true;
        }
        else if (opMode == TAF_ECALL_MODE_NORMAL)
        {
            if (LE_OK != ResumeHlapTimer(TAF_ECALL_TIMER_TYPE_T9))
            {
                LE_CRIT("Resume T9 error");
            }
        } else {
            LE_INFO("eCall operating mode is not normal mode");
        }
    }
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
    eCallConfig.overriddenNum = psapNumber;
    eCallConfig.configValidityMask.set(ECALL_CONFIG_NUM_TYPE);
    eCallConfig.numType = ECallNumType::OVERRIDDEN;
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
            setOpModeProm = std::promise<telux::common::ErrorCode>();
            if(eCallMode == TAF_ECALL_MODE_NORMAL  || eCallMode == TAF_ECALL_MODE_ECALL) {
                auto ret = phone->setECallOperatingMode(
                        static_cast<telux::tel::ECallMode>(eCallMode),
                        tafECallOperatingModeCallback::setECallOperatingModeResponse);
                if(ret == telux::common::Status::SUCCESS) {
                    LE_INFO("Set eCall operating mode %d request sent successfully in phoneId: %d\n",
                            (int) eCallMode, phoneId);
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
        return LE_BAD_PARAMETER;
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
                if (telux::tel::ECallMode::NORMAL == mode)
                {
                    *opMode = TAF_ECALL_MODE_NORMAL;
                    return LE_OK;
                } else if (telux::tel::ECallMode::ECALL_ONLY == mode) {
                    *opMode = TAF_ECALL_MODE_ECALL;
                    return LE_OK;
                } else {
                    LE_ERROR("Invalid mode");
                }
            } else {
                LE_ERROR("Get eCall Operating mode request failed in phoneId: %d\n", phoneId);
            }
        }
    } else {
        LE_ERROR("No phone found corresponding to phoneId:  %d\n", phoneId);
        return LE_BAD_PARAMETER;
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

    uint32_t timeStamp = 0;
    if (!ReadMsdTimeStampFromConfigTree(CFG_NODE_MSDTIMESTAMPSET, &timeStamp))
    {
        timeStamp = (uint32_t)time(NULL);
        char timeStampStr[16];
        snprintf(timeStampStr, sizeof(timeStampStr), "%" PRIu32, timeStamp);
        WriteMsdTimeStampToConfigTree(CFG_NODE_MSDTIMESTAMPSYSTEM, timeStampStr);
    }
    ECallObject.msd.timestamp = timeStamp;
    LE_DEBUG("StartECall timestamp = %d", ECallObject.msd.timestamp);

    Status ret;
    EcallConfig eCallConfig = {};
    if (CallManager == nullptr)
    {
        LE_ERROR("CallManager is nullptr");
        return LE_FAULT;
    }
    ret = CallManager->getECallConfig(eCallConfig);
    if (ret == Status::SUCCESS) {
        LE_INFO("Get eCall configuration successfully.");
    }

    if (eCallVariant == ECallVariant::ECALL_TEST)
    {
        ECallObject.msd.control.automaticActivation = false;
        ECallObject.msd.control.testCall = true;
    } 
    else if (( emergencyCategory == ECallCategory::VOICE_EMER_CAT_AUTO_ECALL) ||
             ( emergencyCategory == ECallCategory::VOICE_EMER_CAT_MANUAL)) 
    {
        ECallObject.msd.control.testCall = false;
        if (eCallConfig.overriddenNum.empty())
        {
            eCallConfig.configValidityMask.set(ECALL_CONFIG_OVERRIDDEN_NUM);
            eCallConfig.overriddenNum = "112";
            eCallConfig.configValidityMask.set(ECALL_CONFIG_NUM_TYPE);
            eCallConfig.numType = ECallNumType::DEFAULT;
            ret = CallManager->setECallConfig(eCallConfig);
            if (ret == Status::SUCCESS)
            {
                LE_INFO("Set eCall configuration with number 112 successfully");
            }
        }
        else if ((0 == strncmp(eCallConfig.overriddenNum.c_str(), "112", 3)) ||
                 (0 == strncmp(eCallConfig.overriddenNum.c_str(), "911", 3)) ||
                 (0 == strncmp(eCallConfig.overriddenNum.c_str(), "999", 3)))
        {
            if (eCallConfig.numType != ECallNumType::DEFAULT)
            {
                eCallConfig.configValidityMask.set(ECALL_CONFIG_NUM_TYPE);
                eCallConfig.numType = ECallNumType::DEFAULT;
                ret = CallManager->setECallConfig(eCallConfig);
                if (ret == Status::SUCCESS)
                {
                    LE_INFO("Set eCall configuration with default number type successfully");
                }
            }
        }

        if ( emergencyCategory == ECallCategory::VOICE_EMER_CAT_AUTO_ECALL)
        {
            ECallObject.msd.control.automaticActivation = true;
        }
        else
        {
            ECallObject.msd.control.automaticActivation = false;
        }
    }

    LE_INFO("ECall Variant: %d, phoneId: %d, isMsdUpdated: %d\n",
            (int) eCallVariant, phoneId, (int) ECallObject.isMsdUpdated);

    //Check msd imported or not to send msd in pdu format or not
    if (ECallObject.isMsdUpdated)
    {
        TAF_ERROR_IF_RET_VAL(eCallPtr->pduMsdSize > MAX_EU_MSD_LENGTH, LE_BAD_PARAMETER, "MSD pdu length exceeds 140 bytes");
        std::vector< uint8_t > eCallMsdData = {};
        for (int i = 0; i < (int)(eCallPtr->pduMsdSize); i++)
        {
            eCallMsdData.push_back(eCallPtr->msdPdu[i]);
        }

        ret = CallManager->makeECall(phoneId, eCallMsdData, (int)emergencyCategory,
                (int)eCallVariant, tafCallCommandCallback::makeECallResponse);
    }
    else
    {
        if (LE_OK != UpdateMsdInformation(ecallRef))
        {
            LE_ERROR("Unable to update the msd information via VHAL");
        }
        eCallPtr->msd.messageIdentifier = 1;
        WriteMsdMsgIdToConfigTree(eCallPtr->msd.messageIdentifier);
        ECallMsdData eCallMsdData = (ECallMsdData) eCallPtr->msd;


        ret = CallManager->makeECall(phoneId, eCallMsdData, (int)emergencyCategory,
                (int)eCallVariant, CallCommandCb);
    }

    if(ret == Status::SUCCESS)
    {
        telux::common::ErrorCode error = makeEcallProm.get_future().get();
        if (error == ErrorCode::SUCCESS) {
            LE_DEBUG("Start ECall request sent successfully");
            ECallObject.eCallSession = ECALL_REQUEST;
            if (eCallVariant == ECallVariant::ECALL_TEST)
            {
                ECallObject.type = TAF_ECALL_TYPE_TEST;
            }
            else if ( emergencyCategory == ECallCategory::VOICE_EMER_CAT_AUTO_ECALL)
            {
                ECallObject.type = TAF_ECALL_TYPE_AUTO;
            }
            else if ( emergencyCategory == ECallCategory::VOICE_EMER_CAT_MANUAL)
            {
                ECallObject.type = TAF_ECALL_TYPE_MANUAL;
            }
            if (false == ECallObject.isMsdUpdated)
            {
                memset(eCallPtr->msdPdu, 0, TAF_ECALL_MAX_MSD_LENGTH);
                if (LE_OK != RetrieveEncodedMsdPdu((ECallMsdData) eCallPtr->msd, eCallPtr->msdPdu, &(eCallPtr->pduMsdSize)))
                {
                    return LE_FAULT;
                }
            }
            return LE_OK;
        }
    }
    return LE_FAULT;
}

le_result_t taf_ecall::StartPrivate(taf_ecall_CallRef_t ecallRef,
                 const char * psapNumber, const char * contentType, const char * acceptInfo) {
#if defined(LE_CONFIG_ENABLE_PRIVATE_ECALL)
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

    CustomSipHeader header;
    if (contentType != NULL){
        header.contentType = contentType;
        LE_INFO("Set content type as %s", contentType);
    } else {
        header.contentType = telux::tel::CONTENT_HEADER;
    }

    if (acceptInfo != NULL) {
        header.acceptInfo = acceptInfo;
        LE_INFO("Set accept info as %s", acceptInfo);
    } else {
        header.acceptInfo = "";
    }

    Status ret;
    EcallConfig eCallConfig = {};
    ret = CallManager->getECallConfig(eCallConfig);
    if (ret == Status::SUCCESS) {
        LE_INFO("get eCall configuration successfully.");
    }

    //Check msd imported or not to send msd in pdu format or not
    if (ECallObject.isMsdUpdated)
    {
        LE_INFO("MSD updated.");
        makePrieCallProm = std::promise<telux::common::ErrorCode>();
        std::vector< uint8_t > eCallMsdData = {};
        for (int i = 0; i < (int)(eCallPtr->pduMsdSize); i++)
        {
            eCallMsdData.push_back(eCallPtr->msdPdu[i]);
        }
        ret = CallManager->makeECall(phoneId, psapNumber, eCallMsdData, header, tafPrieCallCommandCallback::makeECallResponse);
        if(ret == Status::SUCCESS)
        {
            telux::common::ErrorCode error = makePrieCallProm.get_future().get();
            if (error == ErrorCode::SUCCESS) {
                LE_DEBUG("Start private eCall request sent successfully");
                ECallObject.eCallSession = ECALL_REQUEST;
                ECallObject.isPrieCallOngoing = true;
                ECallObject.type = TAF_ECALL_TYPE_PRIVATE;
                return LE_OK;
            }
        }
        ECallObject.isMsdUpdated = false;
        return LE_FAULT;
    } else {
        LE_INFO("No MSD updated.");
        return LE_FAULT;
    }
#else
    return LE_UNSUPPORTED;
#endif
}

le_result_t taf_ecall::StopECall(taf_ecall_CallRef_t ecallRef) {
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    std::shared_ptr<ICall> iCall = eCallPtr->iCall;
    TAF_ERROR_IF_RET_VAL(iCall == nullptr, LE_NOT_FOUND, "iCall is null on eCallPtr(%p)", eCallPtr);

    ClearPduMsd();

    if(iCall->getCallState() == telux::tel::CallState::CALL_INCOMING)
    {
        rejectProm = std::promise<telux::common::ErrorCode>();
        Status status = iCall->reject(RejectCb);
        if (status == Status::SUCCESS) {
            telux::common::ErrorCode error = rejectProm.get_future().get();
            if (error == ErrorCode::SUCCESS) {
                return LE_OK;
            }
        }
    } else {
        hangupProm = std::promise<telux::common::ErrorCode>();
        Status status = iCall->hangup(HangupCb);
        if (status == Status::SUCCESS) {
            telux::common::ErrorCode error = hangupProm.get_future().get();
            if (error == ErrorCode::SUCCESS) {
                return LE_OK;
            }
        }
    }

    return LE_FAULT;
}

le_result_t taf_ecall::AnswerECall(taf_ecall_CallRef_t ecallRef)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);
    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    std::shared_ptr<ICall> iCall = eCallPtr->iCall;
    TAF_ERROR_IF_RET_VAL(iCall == nullptr, LE_NOT_FOUND, "iCall is null on eCallPtr(%p)", eCallPtr);

    answerProm = std::promise<telux::common::ErrorCode>();
    Status status = iCall->answer(AnswerCb);
    if (status == Status::SUCCESS) {
        telux::common::ErrorCode error = answerProm.get_future().get();
        if (error == ErrorCode::SUCCESS) {
            return LE_OK;
        }
    }

    return LE_FAULT;
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
    if (direction < 0 || direction > 179)
    {
        LE_ERROR("Invalid direction value");
        direction=0xFF;
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
        LE_ERROR("MSD passengers count is set by importing MSD");
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
#if defined(LE_CONFIG_ENABLE_ECALL_MSD_V3)
    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_MSDVERSION))
    {
        ECallObject.msd.msdVersion = le_cfg_GetInt(iteratorRef, CFG_NODE_MSDVERSION, 0);
    }
#endif
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

le_result_t taf_ecall::SetMsdAdditionalData(taf_ecall_CallRef_t ecallRef, const char* oid, const uint8_t* data, size_t dataLength)
{
    taf_ECall_t* eCallPtr =(taf_ECall_t*) le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    if (eCallPtr->isMsdUpdated)
    {
        LE_ERROR("MSD optional data is set by importing MSD");
        return LE_DUPLICATE;
    }
#if defined(LE_CONFIG_ENABLE_ECALL_MSD_OPTIONAL_DATA)
    eCallPtr->msd.optionals.optionalDataPresent = true;
    eCallPtr->msd.optionalPdu.oid = oid;
    memcpy(eCallPtr->oadData, data, dataLength);
    eCallPtr->oadDataSize = dataLength;
    string oadDataString;
    for (int i = 0; i < (int)dataLength ; i++)
    {
        char s1 = char(eCallPtr->oadData[i] >> 4);
        char s2 = char(eCallPtr->oadData[i] & 0xf);
        s1 > 9 ? s1 += 55 : s1 += 48;
        s2 > 9 ? s2 += 55 : s2 += 48;
        oadDataString.append(1,s1);
        oadDataString.append(1,s2);
    }
    LE_INFO("Euro NCAP MSD OAD data = %s", oadDataString.c_str());
    std::vector<uint8_t> oadData(oadDataString.begin(), oadDataString.end());
    eCallPtr->msd.optionalPdu.data = oadData;
#endif
    return LE_OK;
}

le_result_t taf_ecall::ResetMsdAdditionalData(taf_ecall_CallRef_t ecallRef)
{
    taf_ECall_t* eCallPtr =(taf_ECall_t*) le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    if (eCallPtr->isMsdUpdated)
    {
        LE_ERROR("MSD optional data is set by importing MSD");
        return LE_DUPLICATE;
    }
#if defined(LE_CONFIG_ENABLE_ECALL_MSD_OPTIONAL_DATA)
    eCallPtr->msd.optionals.optionalDataPresent = false;
    memset(eCallPtr->oadData, 0, sizeof(eCallPtr->oadData));
    eCallPtr->oadDataSize = 0;
#endif
    return LE_OK;
}

le_result_t taf_ecall::SetMsdEuroNCAPLocationOfImpact(taf_ecall_CallRef_t ecallRef, taf_ecall_IILocations_t iiLocations)
{
    taf_ECall_t* eCallPtr =(taf_ECall_t*) le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    if (eCallPtr->isMsdUpdated)
    {
        LE_ERROR("MSD optional data is set by importing MSD");
        return LE_DUPLICATE;
    }

    if ((iiLocations < TAF_ECALL_LOI_UNKNOWN) || (iiLocations > TAF_ECALL_LOI_OTHER))
    {
        LE_ERROR("Invalid location of impact");
        iiLocations = TAF_ECALL_LOI_UNKNOWN;
    }
#if defined(LE_CONFIG_ENABLE_ECALL_MSD_OPTIONAL_DATA)
    ECallObject.euroNCAPData.locationOfImpact = iiLocations;
    memset(eCallPtr->oadData, 0, sizeof(eCallPtr->oadData));
    eCallPtr->oadDataSize = (size_t)msd_EncodeOptionalDataForEuroNCAP(&ECallObject.euroNCAPData, eCallPtr->oadData);
    SetMsdAdditionalData(ecallRef, "8.1", eCallPtr->oadData, eCallPtr->oadDataSize);
#endif
    return LE_OK;
}

le_result_t taf_ecall::SetMsdEuroNCAPRolloverDetected(taf_ecall_CallRef_t ecallRef, bool rolloverDetected)
{
    taf_ECall_t* eCallPtr =(taf_ECall_t*) le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    if (eCallPtr->isMsdUpdated)
    {
        LE_ERROR("MSD optional data is set by importing MSD");
        return LE_DUPLICATE;
    }

#if defined(LE_CONFIG_ENABLE_ECALL_MSD_OPTIONAL_DATA)
    ECallObject.euroNCAPData.rolloverDetectedPresent = true;
    ECallObject.euroNCAPData.rolloverDetected = rolloverDetected;
    memset(eCallPtr->oadData, 0, sizeof(eCallPtr->oadData));
    eCallPtr->oadDataSize = (size_t)msd_EncodeOptionalDataForEuroNCAP(&ECallObject.euroNCAPData, eCallPtr->oadData);
    SetMsdAdditionalData(ecallRef, "8.1", eCallPtr->oadData, eCallPtr->oadDataSize);
#endif
    return LE_OK;
}

le_result_t taf_ecall::ResetMsdEuroNCAPRolloverDetected(taf_ecall_CallRef_t ecallRef)
{
    taf_ECall_t* eCallPtr =(taf_ECall_t*) le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    if (eCallPtr->isMsdUpdated)
    {
        LE_ERROR("MSD optional data is set by importing MSD");
        return LE_DUPLICATE;
    }
#if defined(LE_CONFIG_ENABLE_ECALL_MSD_OPTIONAL_DATA)
    ECallObject.euroNCAPData.rolloverDetectedPresent = false;
    memset(eCallPtr->oadData, 0, sizeof(eCallPtr->oadData));
    eCallPtr->oadDataSize = (size_t)msd_EncodeOptionalDataForEuroNCAP(&ECallObject.euroNCAPData, eCallPtr->oadData);
    SetMsdAdditionalData(ecallRef, "8.1", eCallPtr->oadData, eCallPtr->oadDataSize);
#endif
    return LE_OK;
}

le_result_t taf_ecall::SetMsdEuroNCAPIIDeltaV(taf_ecall_CallRef_t ecallRef, uint8_t rangeLimit, int16_t deltaVX, int16_t deltaVY)
{
    taf_ECall_t* eCallPtr =(taf_ECall_t*) le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    if (eCallPtr->isMsdUpdated)
    {
        LE_ERROR("MSD optional data is set by importing MSD");
        return LE_DUPLICATE;
    }

    if ((rangeLimit < MSD_EURONCAP_OAD_RANGELIMIT_MIN) || (rangeLimit > MSD_EURONCAP_OAD_RANGELIMIT_MAX))
    {
        LE_ERROR("Invalid rangeLimit value");
        rangeLimit = MSD_EURONCAP_OAD_DELTAV_INVALD;
    }

    if ((deltaVX < MSD_EURONCAP_OAD_DELTAVX_MIN) || (deltaVX > MSD_EURONCAP_OAD_DELTAVX_MAX))
    {
        LE_ERROR("Invalid deltaVX value");
        deltaVX = MSD_EURONCAP_OAD_DELTAV_INVALD;
    }

    if ((deltaVY < MSD_EURONCAP_OAD_DELTAVY_MIN) || (deltaVY > MSD_EURONCAP_OAD_DELTAVY_MAX))
    {
        LE_ERROR("Invalid deltaVY value");
        deltaVY = MSD_EURONCAP_OAD_DELTAV_INVALD;
    }

#if defined(LE_CONFIG_ENABLE_ECALL_MSD_OPTIONAL_DATA)
    ECallObject.euroNCAPData.rangeLimit = rangeLimit;
    ECallObject.euroNCAPData.deltaVX = deltaVX;
    ECallObject.euroNCAPData.deltaVY = deltaVY;
    memset(eCallPtr->oadData, 0, sizeof(eCallPtr->oadData));
    eCallPtr->oadDataSize = (size_t)msd_EncodeOptionalDataForEuroNCAP(&ECallObject.euroNCAPData, eCallPtr->oadData);
    SetMsdAdditionalData(ecallRef, "8.1", eCallPtr->oadData, eCallPtr->oadDataSize);
#endif
    return LE_OK;
}

uint16_t taf_ecall::PutBits(uint16_t msgOffset, uint16_t elmtLen, uint8_t* elmtPtr, uint8_t* msgPtr)
{
    std::vector<uint8_t> bitMask({0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01});
    uint8_t msgPos = msgOffset & 0x07;
    uint8_t  elmtPos  = 8 - ((elmtLen & 0x07) ? elmtLen & 0x07 : (elmtLen & 0x07) + 8);
    msgPtr += msgOffset >> 3;
    for (uint16_t i = 0; i < elmtLen; i++) {
        uint8_t val = (*elmtPtr) & bitMask[elmtPos];
        uint8_t mask = bitMask[elmtPos];
        int8_t shift = msgPos - elmtPos;
        if (shift >= 0) {
            val >>= shift;
            mask >>= shift;
        } else {
            val <<= (-shift);
            mask <<= (-shift);
        }

        *msgPtr &= ~mask;
        *msgPtr |= val;
        elmtPos++;
        msgPos++;
        if (elmtPos > 7)
        {
            elmtPtr++;
            elmtPos = 0;
        }
        if (msgPos > 7)
        {
            msgPtr++;
            msgPos = 0;
        }
    }
    return msgOffset + elmtLen;
}

uint16_t taf_ecall::PutTwoBytes(uint16_t  msgOffset, uint16_t elmtLen, uint16_t* elmtPtr, uint8_t* msgPtr)
{
    uint16_t  msgOffsetCurr = msgOffset;
    std::vector<uint16_t> bitMask({0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
		0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000});

    for (uint16_t i = 0; i < elmtLen; i++) {
        if ((*elmtPtr & bitMask[elmtLen-i-1]) != 0) {
            msgPtr[msgOffsetCurr >> 3] |= 0x01 << (7 - (msgOffsetCurr & 0x07));
        } else {
            msgPtr[msgOffsetCurr >> 3] &= ~(0x01 << (7 - (msgOffsetCurr & 0x07)));
        }
        msgOffsetCurr++;
    }
    return msgOffset + elmtLen;
}

int32_t taf_ecall::msd_EncodeOptionalDataForEuroNCAP(taf_EuroNCAPData_t* euroNCAPDataPtr, uint8_t* outDataPtr)
{
    uint8_t extendFlag=0;
    int offset=0;
    uint16_t msdMsgLen=0;

    if (outDataPtr)
    {
        offset = PutBits(offset, 1, &extendFlag, outDataPtr);
        offset = PutBits(offset, 1,(uint8_t*)&euroNCAPDataPtr->rolloverDetectedPresent
                        , outDataPtr);
        offset = PutBits(offset, 1,(uint8_t*)&extendFlag
                        , outDataPtr);
        offset = PutBits(offset, 3,(uint8_t*)&euroNCAPDataPtr->locationOfImpact
                        , outDataPtr);

        if (euroNCAPDataPtr->rolloverDetectedPresent)
        {
               offset = PutBits(offset, 1
                        , (uint8_t*)&euroNCAPDataPtr->rolloverDetected
                        , outDataPtr);
        }

        uint8_t rangeLimitTmp = euroNCAPDataPtr->rangeLimit - 100;
        int16_t deltaVXTmp = euroNCAPDataPtr->deltaVX + 255;
        int16_t deltaVYTmp = euroNCAPDataPtr->deltaVY + 255;
        LE_INFO("rangeLimit = %d, deltaVX  = %d, deltaVY = %d", rangeLimitTmp, deltaVXTmp, deltaVYTmp);

        offset = PutBits(offset, 1, &extendFlag, outDataPtr);

        offset = PutBits(offset, 8
                         , (uint8_t*)&rangeLimitTmp
                         , outDataPtr);
        offset = PutTwoBytes(offset, 9
                         , (uint16_t*)&deltaVXTmp
                         , outDataPtr);
        offset = PutTwoBytes(offset, 9
                         , (uint16_t*)&deltaVYTmp
                         , outDataPtr);

        if (offset % 8)
        {
            msdMsgLen = (offset/8)+1;
        }
        else
        {
            msdMsgLen = (offset/8);
        }
    }

    LE_INFO("MSD optional additional data length %d Bytes for %d bits", msdMsgLen, offset);
    return msdMsgLen;
}

bool taf_ecall::ReadMsdTimeStampFromConfigTree(const char* nodeName, uint32_t* outTimeStamp)
{
    if (!nodeName || !outTimeStamp)
    {
        LE_ERROR("nodeName or outTimeStamp is nullptr");
        return false;
    }

    char timeStampStr[16];
    le_cfg_IteratorRef_t readTxn = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_ECALL_PATH);
    if (le_cfg_NodeExists(readTxn, nodeName))
    {
        le_cfg_GetString(readTxn, nodeName, timeStampStr, sizeof(timeStampStr), MSD_TIMESTAMP_STR_INVALID);
    } else {
        LE_WARN("No timeStamp found; using default: %d", 0);
        le_cfg_CancelTxn(readTxn);
        *outTimeStamp = 0;
        return false;
    }
    le_cfg_CancelTxn(readTxn);

    if (strcmp(timeStampStr, MSD_TIMESTAMP_STR_INVALID) == 0)
    {
        *outTimeStamp = 0;
        return false;
    }

    *outTimeStamp = (uint32_t)strtoul(timeStampStr, NULL, 10);
    return true;
}

void taf_ecall::WriteMsdTimeStampToConfigTree(const char* nodeName, const char* timestampStr)
{
    if (!nodeName || !timestampStr)
    {
        LE_ERROR("nodeName or timestampStr is nullptr");
        return;
    }

    le_cfg_IteratorRef_t writeTxn = le_cfg_CreateWriteTxn(CFG_MODEMSERVICE_ECALL_PATH);
    le_cfg_SetString(writeTxn, nodeName, timestampStr);
    le_cfg_CommitTxn(writeTxn);

    return;
}

le_result_t taf_ecall::SetMsdTimeStamp( taf_ecall_CallRef_t ecallRef, uint32_t timeStamp)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    if (eCallPtr->isMsdUpdated)
    {
        LE_ERROR("MSD timeStamp is set by importing MSD");
        return LE_DUPLICATE;
    }

    eCallPtr->msd.timestamp = timeStamp;

    char timeStampStr[16];
    snprintf(timeStampStr, sizeof(timeStampStr), "%" PRIu32, timeStamp);
    WriteMsdTimeStampToConfigTree(CFG_NODE_MSDTIMESTAMPSET, timeStampStr);

    return LE_OK;
}

le_result_t taf_ecall::ResetMsdTimeStamp( taf_ecall_CallRef_t ecallRef)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    if (eCallPtr->isMsdUpdated)
    {
        LE_ERROR("MSD timeStamp is set by importing MSD");
        return LE_DUPLICATE;
    }

    uint32_t timeStamp = 0;
    if (!ReadMsdTimeStampFromConfigTree(CFG_NODE_MSDTIMESTAMPSYSTEM, &timeStamp))
    {
        LE_INFO("Failed to read the MSD timeStamp from config tree msdTimeStampSystem.");
    }
    eCallPtr->msd.timestamp = timeStamp;
    LE_INFO("ResetMsdTimeStamp timestamp = %d", eCallPtr->msd.timestamp);

    WriteMsdTimeStampToConfigTree(CFG_NODE_MSDTIMESTAMPSET, MSD_TIMESTAMP_STR_INVALID);
    return LE_OK;
}

void taf_ecall::WriteMsdMsgIdToConfigTree(uint32_t msgId)
{
    le_cfg_IteratorRef_t writeTxn = le_cfg_CreateWriteTxn(CFG_MODEMSERVICE_ECALL_PATH);
    le_cfg_SetInt(writeTxn, CFG_NODE_MSDMESSAGEIDENTIFIER, msgId);
    le_cfg_CommitTxn(writeTxn);
}

uint32_t taf_ecall::ReadMsdMsgIdFromConfigTree()
{
    le_cfg_IteratorRef_t readTxn = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_ECALL_PATH);
    uint32_t messageIdentifier = 0;

    if (le_cfg_NodeExists(readTxn, CFG_NODE_MSDMESSAGEIDENTIFIER))
    {
        messageIdentifier = le_cfg_GetInt(readTxn, CFG_NODE_MSDMESSAGEIDENTIFIER, 0);
    }
    else
    {
        LE_WARN("No messageIdentifier found; using default: %d", MIN_MSD_MESSAGE_IDENTIFIER);
    }
    le_cfg_CancelTxn(readTxn);
    return messageIdentifier;
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

    if (eCallPtr->pduMsdSize == 0)
    {
        pdumsd = nullptr;
        *msdLength = 0;
        return LE_NOT_FOUND;
    } else {
        memcpy(pdumsd, eCallPtr->msdPdu, eCallPtr->pduMsdSize);
        *msdLength = eCallPtr->pduMsdSize;
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

    if ((eCallPtr->msd.messageIdentifier >= MIN_MSD_MESSAGE_IDENTIFIER) && (eCallPtr->msd.messageIdentifier < MAX_MSD_MESSAGE_IDENTIFIER))
    {
        eCallPtr->msd.messageIdentifier++;
    } else {
        eCallPtr->msd.messageIdentifier = MIN_MSD_MESSAGE_IDENTIFIER;
    }

    WriteMsdMsgIdToConfigTree(eCallPtr->msd.messageIdentifier);
    LE_DEBUG("SendMsd message identifier = %d", eCallPtr->msd.messageIdentifier);
    LE_DEBUG("SendMsd timestamp = %d", eCallPtr->msd.timestamp);

    LE_INFO("Send msd in phoneId: %d, isMsdUpdated: %d\n", phoneId, (int)eCallPtr->isMsdUpdated);

    if (eCallPtr->isMsdUpdated)
    {
        std::vector< uint8_t > eCallMsdData;
        for (int i = 0; i < (int)(eCallPtr->pduMsdSize); i++)
        {
            eCallMsdData.push_back(eCallPtr->msdPdu[i]);
        }
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
        if (LE_OK != UpdateMsdInformation(ecallRef))
        {
            LE_ERROR("Unable to update the msd information via VHAL");
        }

        updateMsdProm = std::promise<telux::common::ErrorCode>();
        status = CallManager->updateECallMsd(phoneId, eCallPtr->msd, UpdateMsdCb);
        if (status == Status::SUCCESS) {
            telux::common::ErrorCode error = updateMsdProm.get_future().get();
            if (error == ErrorCode::SUCCESS) {
                memset(eCallPtr->msdPdu, 0, sizeof(eCallPtr->msdPdu));
                if (LE_OK == RetrieveEncodedMsdPdu((ECallMsdData) eCallPtr->msd, eCallPtr->msdPdu, &(eCallPtr->pduMsdSize)))
                {
                    return LE_OK;
                }
            }
        }
    }
    return LE_FAULT;
}

le_result_t taf_ecall::RetrieveEncodedMsdPdu(ECallMsdData eCallMsdData, uint8_t* pduMsd, size_t *msdLength)
{
    std::vector<uint8_t> eCallMsdPdu = {};
    telux::common::ErrorCode errorCode = CallManager->encodeECallMsd(eCallMsdData, eCallMsdPdu);
    if (errorCode == telux::common::ErrorCode::SUCCESS)
    {
         if (eCallMsdPdu.size() < MAX_EU_MSD_LENGTH) {
             *msdLength = eCallMsdPdu.size();
             for ( size_t i = 0; i < *msdLength; ++i )
             {
                 pduMsd[i] = eCallMsdPdu[i];
                 LE_DEBUG("RetrieveEncodedMsdPdu pduMsd=%02X", pduMsd[i]);
             }
             return LE_OK;
         } else {
             LE_ERROR("Failed to retrieve the encoded eCall MSD PDU as it exceeded the max MSD length");
         }
    } else {
         LE_ERROR("Failed to retrieve the encoded eCall MSD PDU with error code: %d", (static_cast<int>(errorCode)));
    }

    pduMsd = nullptr;
    *msdLength = 0;
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

void taf_ecall::SetCallPhoneId(int8_t phoneId)
{
    ECallObject.phoneId = phoneId;
}

void taf_ecall::SetSessionState(tafECallSession_t session)
{
    ECallObject.eCallSession = session;

}

void taf_ecall::SetECallState(taf_ecall_State_t state)
{
    ECallObject.state = state;
    if ((state == TAF_ECALL_STATE_ENDED) && (ECallObject.isPrieCallOngoing == true))
    {
        ECallObject.isMsdUpdated = false;
        ECallObject.isPrieCallOngoing = false;
    }
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

taf_ecall_Type_t taf_ecall::GetType ( taf_ecall_CallRef_t ecallRef)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, TAF_ECALL_TYPE_UNKNOWN, "Invalid eCall reference");

    return eCallPtr->type;
}

le_result_t taf_ecall::UseUSimNumbers()
{
    EcallConfig eCallConfig;
    eCallConfig.muteRxAudio = 0;
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

    if (GetHlapTimerStatus(TAF_ECALL_TIMER_TYPE_T10) != TAF_ECALL_TIMER_STATUS_INACTIVE)
    {
        LE_ERROR("Error: Deregistration timer is running");
        return LE_BUSY;
    }

    uint16_t minNwRegTime = 0;
    if ((LE_OK != GetNadMinNetworkRegistrationTime(&minNwRegTime)) ||
        (deregTime < minNwRegTime))
    {
        LE_ERROR("Error: dereg timer should not less than minNwRegTime");
        return LE_FAULT;
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

    if (GetHlapTimerStatus(TAF_ECALL_TIMER_TYPE_T2) != TAF_ECALL_TIMER_STATUS_INACTIVE)
    {
        LE_ERROR("Error: clear down fallback timer is running");
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

    if (GetHlapTimerStatus(TAF_ECALL_TIMER_TYPE_T9) != TAF_ECALL_TIMER_STATUS_INACTIVE)
    {
        LE_ERROR("Error: min network registration timer is running");
        return LE_BUSY;
    }

    if (minNwRegTime < 1 || minNwRegTime > 720) {
        LE_ERROR("Error: min network registration time %d min is not allowed [Range 1:720].", minNwRegTime);
        return LE_FAULT;
    }

    uint16_t deregTime = 0;
    if ((LE_OK != GetNadDeregistrationTime(&deregTime)) ||
        (deregTime < minNwRegTime))
    {
        LE_ERROR("Error: dereg timer should not less than minNwRegTime");
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

le_result_t taf_ecall::GetHlapTimerState(taf_ecall_HlapTimerType_t timerType, taf_ecall_HlapTimerStatus_t* timerStatus, uint16_t* elapsedTime)
{
    if ((timerStatus == NULL) || (elapsedTime == NULL))
    {
        LE_ERROR("timerStatus or elapsedTime is null.");
        return LE_FAULT;
    }

    uint16_t ccftTime = 0;
    uint16_t minNwRegTime = 0;
    uint16_t deregTime = 0;
    uint16_t t2ElapsedTime = 0;
    uint16_t t9ElapsedTime = 0;
    uint16_t t10ElapsedTime = 0;
    *timerStatus = GetHlapTimerStatus(timerType);
    if (*timerStatus == TAF_ECALL_TIMER_STATUS_ACTIVE)
    {
        switch (timerType)
        {
            case TAF_ECALL_TIMER_TYPE_T2:
                if (LE_OK != GetNadClearDownFallbackTime(&ccftTime))
                {
                    LE_ERROR("GetNadClearDownFallbackTime wrong.");
                    return LE_FAULT;
                }

                if (t2StartTimeSet == true)
                {
                    t2ElapsedTime = ConvertElapsedTime(t2StartTime);
                } else {
                    LE_ERROR("Get hlap timer T2 is active, but start time is not set.");
                    return LE_FAULT;
                }

                if (ccftTime*60 >= t2ElapsedTime)
                {
                    *elapsedTime = t2ElapsedTime;
                } else {
                    LE_ERROR("Get hlap timer T2 state wrong as elapsed time is out of range.");
                    return LE_FAULT;
                }
                break;
            case TAF_ECALL_TIMER_TYPE_T9:
                if (LE_OK != GetNadMinNetworkRegistrationTime(&minNwRegTime))
                {
                    LE_ERROR("GetNadMinNetworkRegistrationTime wrong as elapsed time is out of range.");
                    return LE_FAULT;
                }

                if (t9StartTimeSet == true)
                {
                    t9ElapsedTime = ConvertElapsedTime(t9StartTime) + ElapsedTimeT9;
                } else {
                    LE_ERROR("Get hlap timer T9 is active, but start time is not set.");
                    return LE_FAULT;
                }

                if (minNwRegTime*60 >= t9ElapsedTime)
                {
                    *elapsedTime = t9ElapsedTime;
                } else {
                    LE_ERROR("Get hlap timer T9 state wrong as elapsed time is out of range.");
                    return LE_FAULT;
                }
                break;
            case TAF_ECALL_TIMER_TYPE_T10:
                if (LE_OK != GetNadDeregistrationTime(&deregTime))
                {
                     LE_ERROR("GetNadDeregistrationTime wrong.");
                     return LE_FAULT;
                }

                if (t10StartTimeSet == true)
                {
                    t10ElapsedTime = ConvertElapsedTime(t10StartTime);
                } else {
                    LE_ERROR("Get hlap timer T10 is active, but start time is not set.");
                    return LE_FAULT;
                }

                if (deregTime*60 >= t10ElapsedTime)
                {
                    *elapsedTime = t10ElapsedTime;
                } else {
                    LE_ERROR("Get hlap timer T10 state wrong as elapsed time is out of range.");
                    return LE_FAULT;
                }
                break;
            case TAF_ECALL_TIMER_TYPE_UNKNOWN:
            default:
                LE_ERROR("Wrong hlap timer type.");
                return LE_BAD_PARAMETER;
            }
    }
    else if (*timerStatus == TAF_ECALL_TIMER_STATUS_UNKNOWN)
    {
       LE_ERROR("Wrong hlap timer type or unable to get the timer status.");
       return LE_FAULT;
    }
    else
    {
       *elapsedTime = 0;
    }
    LE_INFO("Get eCall hlap timer status as: %d, elapsedTime as: %d", *timerStatus, *elapsedTime);
    return LE_OK;
}

taf_ecall_HlapTimerStatus_t taf_ecall::GetHlapTimerStatus(taf_ecall_HlapTimerType_t timerType) {
    taf_ecall_HlapTimerStatus_t timerStatus;
    std::promise<telux::common::ErrorCode> p;
    std::promise<int> q;
    std::promise<ECallHlapTimerStatus> r;
    int phone_id = PhoneManager->getPhoneIdFromSlotId((int)taf_sim_GetSelectedCard());
    telux::tel::ECallHlapTimerStatusCallback cb =
        [&p, &q, &r](telux::common::ErrorCode error, int phoneId, ECallHlapTimerStatus hlapTimerStatus) {
        p.set_value(error);
        q.set_value(phoneId);
        r.set_value(hlapTimerStatus);
    };

    Status status = CallManager->requestECallHlapTimerStatus(phone_id, cb);
    if (status == Status::SUCCESS) {
        LE_INFO("Get eCall hlap timer successfully.");
        telux::common::ErrorCode error = p.get_future().get();
        LE_INFO("Get eCall hlap timer state: error code %d", (int) error);

        if (error == ErrorCode::SUCCESS) {
            if (phone_id != q.get_future().get())
            {
                LE_ERROR("Wrong phoneId.");
                return TAF_ECALL_TIMER_STATUS_UNKNOWN;
            }
            switch (timerType)
            {
                case TAF_ECALL_TIMER_TYPE_T2:
                    timerStatus = ConvertHlapTimerStatus(r.get_future().get().t2);
                    break;
                case TAF_ECALL_TIMER_TYPE_T9:
                    timerStatus = ConvertHlapTimerStatus(r.get_future().get().t9);
                    break;
                case TAF_ECALL_TIMER_TYPE_T10:
                    timerStatus = ConvertHlapTimerStatus(r.get_future().get().t10);
                    break;
                case TAF_ECALL_TIMER_TYPE_UNKNOWN:
                default:
                    LE_ERROR("Wrong hlap timer type.");
                    return TAF_ECALL_TIMER_STATUS_UNKNOWN;
            }
            return timerStatus;
        }
    }
    return TAF_ECALL_TIMER_STATUS_UNKNOWN;
}

taf_ecall_HlapTimerStatus_t taf_ecall::ConvertHlapTimerStatus(telux::tel::HlapTimerStatus status) {
    switch(status) {
        case telux::tel::HlapTimerStatus::INACTIVE:
            return TAF_ECALL_TIMER_STATUS_INACTIVE;
        case telux::tel::HlapTimerStatus::ACTIVE:
            return TAF_ECALL_TIMER_STATUS_ACTIVE;
        case telux::tel::HlapTimerStatus::UNKNOWN:
            return TAF_ECALL_TIMER_STATUS_UNKNOWN;
        default:
            return TAF_ECALL_TIMER_STATUS_UNKNOWN;
    }
}

uint16_t taf_ecall::ConvertElapsedTime(std::chrono::time_point<std::chrono::steady_clock> startTime)
{
    std::chrono::duration<double> duration = std::chrono::steady_clock::now() - startTime;
    uint16_t elapsedTime = static_cast<uint16_t>(duration.count());
    LE_DEBUG("ElapsedTime is %d when ConvertElapsedTime", elapsedTime);
    return elapsedTime;
}

void taf_ecall::T9TimerExpiryHandler(le_timer_Ref_t timerRef)
{
    auto &eCall = taf_ecall::GetInstance();
    taf_ecall_HlapTimerStatus_t timerStatus = TAF_ECALL_TIMER_STATUS_UNKNOWN;
    uint16_t elapsedTime = 0;
    uint16_t minNwRegTime = 0;
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_ECALL_HLAPTIMERELAPSED_PATH );

    if (LE_OK == eCall.GetNadMinNetworkRegistrationTime(&minNwRegTime))
    {
        if (elapsedTime <= minNwRegTime*60)
        {
            if ((LE_OK == eCall.GetHlapTimerState(TAF_ECALL_TIMER_TYPE_T9, &timerStatus, &elapsedTime)) &&
                (timerStatus == TAF_ECALL_TIMER_STATUS_ACTIVE))
            {
                le_cfg_SetInt(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T9, elapsedTime);
            }
        } else {
            le_timer_Stop(eCall.elapsedTimeT9Ref);
            le_cfg_SetInt(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T9, minNwRegTime*60);
        }
    } else {
        le_cfg_SetInt(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T9, -1);
    }
    LE_INFO("T9 timer status: %d, elapsed timer: %d, configuration timer: %d", (int)timerStatus, elapsedTime, minNwRegTime);
    le_cfg_CommitTxn(iteratorRef);
}

void* taf_ecall::StartHlapElapsedTimer(HlapTimerType_t type, HlapTimerEventType_t event)
{
    LE_INFO("SaveHlapTimerElapsedInfo, starting timer");
    auto &eCall = taf_ecall::GetInstance();
    taf_ecall_OpMode_t opMode;
    int phoneId = eCall.PhoneManager->getPhoneIdFromSlotId((int)taf_sim_GetSelectedCard());

    if (type == HLAP_TIMER_TYPE_T9)
    {
        LE_INFO("Timer expired, elapsedTime %d", ElapsedTimeT9);
        bool shouldStopTimer = (event == HLAP_TIMER_EVENT_TYPE_EXPIRED) ||
                               (event == HLAP_TIMER_EVENT_TYPE_STOPPED) ||
                               (event == HLAP_TIMER_EVENT_TYPE_UNKNOWN) ||
                               ((LE_OK == eCall.GetECallOperatingMode(phoneId, &opMode)) &&
                                (opMode == TAF_ECALL_MODE_ECALL));

        if (shouldStopTimer)
        {
            le_timer_Stop(eCall.elapsedTimeT9Ref);
        }
        else if ((event == HLAP_TIMER_EVENT_TYPE_STARTED) ||
                 (event == HLAP_TIMER_EVENT_TYPE_RESUMED))
        {
            le_timer_Start(eCall.elapsedTimeT9Ref);
        }

        if (event != HLAP_TIMER_EVENT_TYPE_RESUMED)
        {
            le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn(CFG_ECALL_HLAPTIMERELAPSED_PATH);
            if (event == HLAP_TIMER_EVENT_TYPE_STARTED)
            {
                le_cfg_SetInt(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T9, 0);
            } else {
                le_cfg_DeleteNode(iteratorRef, "");
            }
            le_cfg_CommitTxn(iteratorRef);
        }
    }

    return NULL;
}

HlapTimerEventType_t taf_ecall::ConvertHlapTimerEvent(HlapTimerEvent event) {
    switch (event) {
        case HlapTimerEvent::STARTED:
            return HLAP_TIMER_EVENT_TYPE_STARTED;
        case HlapTimerEvent::STOPPED:
            return HLAP_TIMER_EVENT_TYPE_STOPPED;
        case HlapTimerEvent::EXPIRED:
            return HLAP_TIMER_EVENT_TYPE_EXPIRED;
        case HlapTimerEvent::RESUMED:
            return HLAP_TIMER_EVENT_TYPE_RESUMED;
        default:
            return HLAP_TIMER_EVENT_TYPE_UNKNOWN;
    }
}

le_result_t taf_ecall::ResumeHlapTimer(taf_ecall_HlapTimerType_t timerType) {
    int phoneId = PhoneManager->getPhoneIdFromSlotId((int)taf_sim_GetSelectedCard());
    EcallHlapTimerId timerId = EcallHlapTimerId::UNKNOWN;;
    uint16_t minNwRegTime = 0;
    int duration = 0;

    if (timerType == TAF_ECALL_TIMER_TYPE_T9)
    {
        if (LE_OK == GetNadMinNetworkRegistrationTime(&minNwRegTime))
        {
            duration = minNwRegTime*60 - ElapsedTimeT9;
            timerId = EcallHlapTimerId::T9;
            LE_INFO("RestartHlapTimer duration = %d, %d", minNwRegTime, ElapsedTimeT9);
        } else {
            LE_ERROR("GetNadMinNetworkRegistrationTime error.");
            return LE_FAULT;
        }
    } else {
        LE_ERROR("Wrong hlap timer type.");
        return LE_FAULT;
    }

    LE_INFO("Resume the hlap timer with the value = %d", duration);
    if (duration > 0) {
        if (CallManager) {
            resumeHlapTimerProm = std::promise<telux::common::ErrorCode>();
            Status status = CallManager->restartECallHlapTimer(phoneId, timerId, duration, tafResumeHlapTimerCallback::resumeHlapTimerResponse);
            if (status == Status::SUCCESS) {
                LE_INFO("restartECallHlapTimer success");
                telux::common::ErrorCode error = resumeHlapTimerProm.get_future().get();
                if (error != ErrorCode::SUCCESS) {
                    LE_ERROR("Restarting eCall HLAP timer failed");
                    return LE_FAULT;
                }
            } else {
                LE_ERROR("Restarting eCall HLAP timer failed");
                return LE_FAULT;
            }
        } else {
            LE_ERROR("CallManager is nullptr");
            return LE_FAULT;
        }
    } else {
        LE_ERROR("The duration is incorrect");
        return LE_FAULT;
    }

    return LE_OK;
}

void taf_ecall::ResumeHlapTimerEventHandler(void* reqPtr)
{
    ResumeHlapTimerEvent_t* eventReq = (ResumeHlapTimerEvent_t*)reqPtr;
    auto &eCall = taf_ecall::GetInstance();
    le_result_t result = LE_FAULT;

    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    std::promise<telux::common::ServiceStatus> prom;
    telux::common::ServiceStatus serviceStatus;
    std::future<telux::common::ServiceStatus> initFuture;
    std::future_status waitStatus;
    std::promise<telux::common::ServiceStatus> phoneMgrprom;
    telux::common::ServiceStatus phoneMgrStatus;
    std::vector<int> phoneIds;
    telux::common::Status status;
    taf_ecall_OpMode_t opMode;
    int phoneId;
    telux::tel::ECallMode eCallMode;

    if(eventReq == NULL)
    {
        LE_ERROR ("Invalid Parameters");
        return;
    }

    switch (eventReq->event) {
        case EVENT_MODEM_REBOOT:
            LE_INFO("Resume hlap timer when modem reboots");
            eCall.CallManager = phoneFactory.getCallManager([&](telux::common::ServiceStatus status) {
            LE_INFO("Getting status: %d from call manager %p", (int)status, eCall.CallManager.get());
            if (status != telux::common::ServiceStatus::SERVICE_UNAVAILABLE)
            {
                prom.set_value(status);
            }
            });
            if (!eCall.CallManager)
            {
                LE_ERROR("Can't get call manager");
            }

            initFuture = prom.get_future();
            waitStatus = initFuture.wait_for(std::chrono::seconds(MAX_INIT_TIMEOUT));
            if (std::future_status::timeout == waitStatus)
            {
                LE_ERROR ("Timeout waiting for susbsytem");
            }
            else
            {
                serviceStatus = initFuture.get();
                if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    LE_ERROR(" *** ERROR - Unable to initialize call subsystem");
                }
            }

            eCall.PhoneManager = PhoneFactory::getInstance().getPhoneManager([&] (telux::common::ServiceStatus status) {
                LE_INFO("Getting status: %d from phone manager", (int)status);
                if (status != telux::common::ServiceStatus::SERVICE_UNAVAILABLE)
                {
                    phoneMgrprom.set_value(status);
                }
            });
            if (!eCall.PhoneManager)
            {
                LE_ERROR("Can't get phone manager");
            }

            phoneMgrStatus = eCall.PhoneManager->getServiceStatus();
            if (phoneMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                LE_INFO("telephony subsystem is not ready, wait for it to be ready");
                initFuture = phoneMgrprom.get_future();
                waitStatus = initFuture.wait_for(std::chrono::seconds(MAX_INIT_TIMEOUT));
                if (std::future_status::timeout == waitStatus)
                {
                    LE_ERROR("Timeout waiting for susbsytem");
                }
                else
                {
                    serviceStatus = initFuture.get();
                    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
                    {
                        LE_ERROR(" *** ERROR - Unable to initialize phone subsystem");
                    }
                }
            }

            status = eCall.PhoneManager->getPhoneIds(phoneIds);
            if (status == telux::common::Status::SUCCESS)
            {
                for (auto index = 1; index <= (int)phoneIds.size(); index++)
                {
                    auto phone = eCall.PhoneManager->getPhone(index);
                    if (phone != nullptr)
                    {
                        eCall.Phones.emplace_back(phone);
                    }
                }
            }

            if(eCall.ElapsedTimeT9 == 0)
            {
                LE_INFO("No need to resume T9 timer");
                break;
            }
            phoneId = eCall.PhoneManager->getPhoneIdFromSlotId((int)taf_sim_GetSelectedCard());
            LE_INFO("phoneId is %d", phoneId);
            if (LE_OK != eCall.GetECallOperatingMode(phoneId, &opMode))
            {
                 eCall.pendingToResumeHlapTimer = true;
            }
            else if (opMode == TAF_ECALL_MODE_NORMAL)
            {
                result = eCall.ResumeHlapTimer(TAF_ECALL_TIMER_TYPE_T9);
                if (result != LE_OK)
                {
                    LE_CRIT("ResumeECallHlapTimer T9 failed");
                }
            } else {
                LE_INFO("eCall operating mode is not normal");
            }
            break;

        case EVENT_SAVE_HLAP_TIMER_ELAPSED:
            LE_INFO("Update the hlap timer with elapsed value to config tree");
            eCall.StartHlapElapsedTimer(eventReq->hlapTimerType, eventReq->hlapTimerEventType);
            break;

        case EVENT_ECALL_MODE_CHANGE:
            LE_INFO("eCall mode changed");
            eCallMode = eventReq->eCallMode;
            phoneId = eventReq->phoneId;
            if (phoneId == eCall.PhoneManager->getPhoneIdFromSlotId((int)taf_sim_GetSelectedCard()))
            {
                LE_ERROR("phoneId is different with the select one %d", phoneId);
                break;
            }

            if ((eCallMode == telux::tel::ECallMode::NORMAL) && (eCall.pendingToResumeHlapTimer == true))
            {
                result = eCall.ResumeHlapTimer(TAF_ECALL_TIMER_TYPE_T9);
                if (result != LE_OK)
                {
                    LE_CRIT("ResumeECallHlapTimer T9 failed");
                }
                eCall.pendingToResumeHlapTimer = false;
            }
            break;

        default:
            LE_ERROR("Undefined event received.");
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The Vehicle Identification Number is defined by iso 3833 as a 17 character
 * alphanumeric code, which includes the letters (F"A".."H"|"J".."N"|"P"|"R".."Z")
 * and the digit ("0".."9")
 */
//--------------------------------------------------------------------------------------------------
int taf_ecall::CheckVIN
(
    char *vin
)
{
    int ret = 0;
    char c;

    while ( (*vin) && (!ret) )
    {
        c= (char)(*vin);
        if (( (c >= 'A') && (c <= 'H') ) ||
            ( (c >= 'J') && (c <= 'N') ) ||
            ( c == 'P' ) ||
            ( (c >= 'R') && (c <= 'Z') ) ||
            ( (c >= '0') && (c <= '9') ) )
        {
            vin++;
        }
        else
        {
            ret = -1;
            LE_ERROR("%c is not allowed", *vin);
        }
    }

    return ret;
}

le_result_t taf_ecall::UpdateMsdVehicleInfo()
{
    if (isDrvPresent == true)
    {
        LE_INFO("Update Msd VehicleInfo via VHAL");
        if((*(eCallInf->getVehicleInfo)) == NULL)
        {
            LE_ERROR("getVehicleInfo VHAL not initialized");
            return LE_FAULT;
        }

        le_result_t result = LE_FAULT;
        taf_hal_eCall_VehicleInfo vehInfo;
        result = (*(eCallInf->getVehicleInfo))(&vehInfo);
        if(result != LE_OK)
        {
            LE_ERROR("Unable to get vehicleInfo via VHAL");
            return LE_FAULT;
        }

        if ((vehInfo.propulsionType & ECALL_HAL_BITMASK_PROP_TYPE_GASOLINE_TANK) == ECALL_HAL_BITMASK_PROP_TYPE_GASOLINE_TANK)
        {
            ECallObject.msd.vehiclePropulsionStorage.gasolineTankPresent = true;
        } else {
            ECallObject.msd.vehiclePropulsionStorage.gasolineTankPresent = false;
        }

        if ((vehInfo.propulsionType & ECALL_HAL_BITMASK_PROP_TYPE_DIESEL_TANK) == ECALL_HAL_BITMASK_PROP_TYPE_DIESEL_TANK)
        {
            ECallObject.msd.vehiclePropulsionStorage.dieselTankPresent = true;
        } else {
            ECallObject.msd.vehiclePropulsionStorage.dieselTankPresent = false;
        }

        if ((vehInfo.propulsionType & ECALL_HAL_BITMASK_PROP_TYPE_GASOLINE_TANK) == ECALL_HAL_BITMASK_PROP_TYPE_COMPRESSED_NATURALGAS)
        {
            ECallObject.msd.vehiclePropulsionStorage.compressedNaturalGas = true;
        } else {
            ECallObject.msd.vehiclePropulsionStorage.compressedNaturalGas = false;
        }

        if ((vehInfo.propulsionType & ECALL_HAL_BITMASK_PROP_TYPE_DIESEL_TANK) == ECALL_HAL_BITMASK_PROP_TYPE_PROPANE_GAS)
        {
            ECallObject.msd.vehiclePropulsionStorage.liquidPropaneGas = true;
        } else {
            ECallObject.msd.vehiclePropulsionStorage.liquidPropaneGas = false;
        }

        if ((vehInfo.propulsionType & ECALL_HAL_BITMASK_PROP_TYPE_DIESEL_TANK) == ECALL_HAL_BITMASK_PROP_TYPE_ELECTRIC)
        {
            ECallObject.msd.vehiclePropulsionStorage.electricEnergyStorage = true;
        } else {
            ECallObject.msd.vehiclePropulsionStorage.electricEnergyStorage = false;
        }

        if ((vehInfo.propulsionType & ECALL_HAL_BITMASK_PROP_TYPE_GASOLINE_TANK) == ECALL_HAL_BITMASK_PROP_TYPE_HYDROGEN)
        {
            ECallObject.msd.vehiclePropulsionStorage.hydrogenStorage = true;
        } else {
            ECallObject.msd.vehiclePropulsionStorage.hydrogenStorage = false;
        }

        if ((vehInfo.propulsionType & ECALL_HAL_BITMASK_PROP_TYPE_DIESEL_TANK) == ECALL_HAL_BITMASK_PROP_TYPE_OTHER)
        {
            ECallObject.msd.vehiclePropulsionStorage.otherStorage = true;
        } else {
            ECallObject.msd.vehiclePropulsionStorage.liquidPropaneGas = false;
        }

        if (CheckVIN((char *)vehInfo.vin))
        {
            LE_ERROR("VIN is wrong %s", vehInfo.vin);
            ECallObject.msd.vehicleIdentificationNumber.isowmi = "000";
            ECallObject.msd.vehicleIdentificationNumber.isovds = "000000";
            ECallObject.msd.vehicleIdentificationNumber.isovisModelyear = "0";
            ECallObject.msd.vehicleIdentificationNumber.isovisSeqPlant = "0000000";
        } else {
            std::string vinStr = vehInfo.vin;
            ECallObject.msd.vehicleIdentificationNumber.isowmi = vinStr.substr(ISOWMI_START, ISOWMI_LENGTH );
            ECallObject.msd.vehicleIdentificationNumber.isovds = vinStr.substr(ISOVDS_START, ISOVDS_LENGTH);
            ECallObject.msd.vehicleIdentificationNumber.isovisModelyear =
                                 vinStr.substr(ISOVIS_MODEL_YEAR_START, ISOVIS_MODEL_YEAR_LENGTH);
            ECallObject.msd.vehicleIdentificationNumber.isovisSeqPlant =
                                 vinStr.substr(ISOVIS_SEQ_PLANT_START, ISOVIS_SEQ_PLANT_LENGTH);
        }

        ECallObject.msd.control.vehicleType = (ECallVehicleType)vehInfo.vehiType;

        return LE_OK;
    } else {
        return LE_FAULT;
    }
}

le_result_t taf_ecall::UpdateMsdInformation(taf_ecall_CallRef_t ecallRef)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    if (isDrvPresent == true)
    {
        LE_INFO("Update Msd Information via Hal");
        le_result_t result = LE_FAULT;

        taf_hal_eCall_VehicleType maxVehicleType = ECALL_HAL_VEHITYPE_MOTOR_CYCLES_CLASS_L7E;
#if defined(LE_CONFIG_ENABLE_ECALL_MSD_V3)
        if ( ECallObject.msd.msdVersion == MSD_VERSION_THREE)
        {
            maxVehicleType = ECALL_HAL_VEHITYPE_OTHER_VEHICLE_CLASS;
        }
#endif

        if ((ECallObject.msd.control.vehicleType < (ECallVehicleType)ECALL_HAL_VEHITYPE_PASSENGER_VEHICLE_CLASS_M1) ||
            (ECallObject.msd.control.vehicleType > (ECallVehicleType)maxVehicleType))
        {
            LE_ERROR("VehicleType is wrong %d", ECallObject.msd.control.vehicleType);
            ECallObject.msd.control.vehicleType = (ECallVehicleType)ECALL_HAL_VEHITYPE_PASSENGER_VEHICLE_CLASS_M1;
        }

        taf_hal_eCall_ActivateType actType = ECALL_HAL_ACTTYPE_AUTOMATIC;

        ECallObject.msd.control.automaticActivation = false;
        eCallPtr->msd.optionals.numberOfPassengersPresent = false;

        if((*(eCallInf->getActivateType)) == NULL)
        {
            LE_ERROR("getActivateType VHAL not initialized");
        } else {
            result = (*(eCallInf->getActivateType))(&actType);
            if(result != LE_OK)
            {
                LE_ERROR("Unable to get activate type via VHAL");
            } else {
                if (actType == ECALL_HAL_ACTTYPE_AUTOMATIC)
                {
                    ECallObject.msd.control.automaticActivation = true;
                } else {
                    ECallObject.msd.control.automaticActivation = false;
                }
            }
        }

        if((*(eCallInf->getPassengerCount)) == NULL)
        {
            LE_ERROR("getPassengerCount VHAL not initialized");
        } else {

            uint8_t passCount = 0;
            result = (*(eCallInf->getPassengerCount))(&passCount);
            if(result != LE_OK)
            {
                LE_ERROR("Unable to get passenger count via VHAL");
            } else {
                eCallPtr->msd.optionals.numberOfPassengersPresent = true;
                eCallPtr->msd.numberOfPassengers = passCount;
            }
        }

        if (actType != ECALL_HAL_ACTTYPE_AUTOMATIC)
        {
            if (LE_OK != ResetMsdAdditionalData(ecallRef))
            {
                LE_ERROR("Reset Msd additionalData failed");
                return LE_FAULT;
            }
            return LE_OK;
        } else {
#if defined(LE_CONFIG_ENABLE_ECALL_MSD_OPTIONAL_DATA)
            ECallObject.euroNCAPData.locationOfImpact = (taf_ecall_IILocations_t)ECALL_HAL_LOI_UNKNOWN;
            ECallObject.euroNCAPData.rolloverDetectedPresent = false;
            ECallObject.euroNCAPData.rangeLimit = MSD_EURONCAP_OAD_DELTAV_INVALD;
            ECallObject.euroNCAPData.deltaVX = MSD_EURONCAP_OAD_DELTAV_INVALD;
            ECallObject.euroNCAPData.deltaVY = MSD_EURONCAP_OAD_DELTAV_INVALD;
#endif
        }

        if((*(eCallInf->getIILocations)) == NULL)
        {
            LE_ERROR("getIILocations VHAL not initialized");
        } else {
            taf_hal_eCall_IILocations iILocations;
            result = (*(eCallInf->getIILocations))(&iILocations);
            if((result != LE_OK) ||
               ((iILocations < ECALL_HAL_LOI_UNKNOWN) || (iILocations > ECALL_HAL_LOI_OTHER)))
            {
                LE_ERROR("Unable to get IILocations information via VHAL");
            } else {
#if defined(LE_CONFIG_ENABLE_ECALL_MSD_OPTIONAL_DATA)
                ECallObject.euroNCAPData.locationOfImpact = (taf_ecall_IILocations_t)iILocations;
#endif
            }
        }

        if((*(eCallInf->getRolloverDetected)) == NULL)
        {
            LE_ERROR("getRolloverDetected VHAL not initialized");
        } else {
            taf_hal_eCall_RolloverDetected rollDetected;
            result = (*(eCallInf->getRolloverDetected))(&rollDetected);
            if(result != LE_OK)
            {
                LE_ERROR("Unable to get rolloverDetected information via VHAL");
            } else {
#if defined(LE_CONFIG_ENABLE_ECALL_MSD_OPTIONAL_DATA)
                ECallObject.euroNCAPData.rolloverDetectedPresent = rollDetected.rolloverDetectedPresent;
                ECallObject.euroNCAPData.rolloverDetected = rollDetected.rolloverDetected;
#endif
            }
        }

        if((*(eCallInf->getDeltaV)) == NULL)
        {
            LE_ERROR("getDeltaV VHAL not initialized");
        } else {
            taf_hal_eCall_DeltaV deltaV;
            result = (*(eCallInf->getDeltaV))(&deltaV);
            if(result != LE_OK)
            {
                LE_ERROR("Unable to get DeltaVHAL via VHAL");
            } else {
#if defined(LE_CONFIG_ENABLE_ECALL_MSD_OPTIONAL_DATA)
                if ((deltaV.rangeLimit < MSD_EURONCAP_OAD_RANGELIMIT_MIN) || (deltaV.rangeLimit > MSD_EURONCAP_OAD_RANGELIMIT_MAX) ||
                    (deltaV.deltaVX < MSD_EURONCAP_OAD_DELTAVX_MIN) || (deltaV.deltaVX > MSD_EURONCAP_OAD_DELTAVX_MAX) ||
                    (deltaV.deltaVY < MSD_EURONCAP_OAD_DELTAVY_MIN) || (deltaV.deltaVY > MSD_EURONCAP_OAD_DELTAVY_MAX))
                {
                     LE_ERROR("Invalid deltaV information");
                } else{
                    ECallObject.euroNCAPData.rangeLimit = deltaV.rangeLimit;
                    ECallObject.euroNCAPData.deltaVX = deltaV.deltaVX;
                    ECallObject.euroNCAPData.deltaVY = deltaV.deltaVY;
                }
#endif
            }
        }

#if defined(LE_CONFIG_ENABLE_ECALL_MSD_OPTIONAL_DATA)
        memset(eCallPtr->oadData, 0, sizeof(eCallPtr->oadData));
        eCallPtr->oadDataSize = (size_t)msd_EncodeOptionalDataForEuroNCAP(&ECallObject.euroNCAPData, eCallPtr->oadData);
        SetMsdAdditionalData(ecallRef, "8.1", eCallPtr->oadData, eCallPtr->oadDataSize);
#endif
        return LE_OK;
    } else {
        return LE_FAULT;
    }
}

le_result_t taf_ecall::IsInProgress(taf_ecall_CallRef_t ecallRef, bool* isInProgress)
{
    TAF_ERROR_IF_RET_VAL(isInProgress == NULL, LE_BAD_PARAMETER, "Invalid parameter");
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    if (eCallPtr == NULL)
    {
        LE_ERROR("Invalid eCall reference");
        return LE_BAD_PARAMETER;
    }

    std::shared_ptr<telux::tel::ICall> spCall = nullptr;
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
            *isInProgress = true;
        } else {
            *isInProgress = false;
        }
    } else {
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_ecall::ConfigureInitialDialRedial(std::vector<int> redialPara)
{
    if (CallManager) {
        for (size_t i = 0; i < redialPara.size(); i++)
        {
            LE_DEBUG("ConfigureInitialDialRedial redialPara = %d", redialPara[i]);
        }
        configRedialProm = std::promise<telux::common::ErrorCode>();
        Status status = CallManager->configureECallRedial(RedialConfigType::CALL_ORIG, redialPara, tafConfigRedialCallback::configureRedialResponse);
        if (status == Status::SUCCESS) {
            telux::common::ErrorCode error = configRedialProm.get_future().get();
            if (error == ErrorCode::SUCCESS) {
                return LE_OK;
            }
        }
    } else {
        LE_ERROR("CallManager is null");
        return LE_FAULT;
    }
    return LE_FAULT;
}

le_result_t taf_ecall::SetInitialDialAttempts(uint8_t attempts)
{
    size_t count = 0;
    for (size_t i = 0; i < TAF_ECALL_MAX_DIAL_ATTEMPTS_LENGTH; ++i) {
        if (ECallObject.dialRedial.dialInterval[i] == 0) {
            break;
        }
        ++count;
    }

    if (attempts > count)
    {
        LE_ERROR("attempts should be set smaller than the the length of dialInterval");
        return LE_FAULT;
    }

    std::vector<int> redialPara;
    for (uint i = 0; i < attempts; i++)
    {
        redialPara.push_back(ECallObject.dialRedial.dialInterval[i] * 1000);
    }

    if ( LE_OK == ConfigureInitialDialRedial(redialPara))
    {
        ECallObject.dialRedial.dialAttempts = attempts;
        return LE_OK;
    }
    return LE_FAULT;
}

le_result_t taf_ecall::SetInitialDialIntervalBetweenDialAttempts(const uint16_t* interval, size_t intervalLength)
{
    std::vector<int> redialPara;
    size_t i = 0;

    if (intervalLength < ECallObject.dialRedial.dialAttempts)
    {
        for (; i < intervalLength; i++)
        {
            redialPara.push_back(interval[i]*1000);
        }
        for (; i < ECallObject.dialRedial.dialAttempts; i++)
        {
            redialPara.push_back(ECallObject.dialRedial.dialInterval[i]*1000);
        }
    } else {
        for (; i < ECallObject.dialRedial.dialAttempts; i++)
        {
            redialPara.push_back(interval[i]*1000);
        }
    }

    if ( LE_OK == ConfigureInitialDialRedial(redialPara))
    {
        for (size_t i = 0; i < intervalLength; i++)
        {
            ECallObject.dialRedial.dialInterval[i] = interval[i];
        }
        return LE_OK;
    }
    return LE_FAULT;
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
