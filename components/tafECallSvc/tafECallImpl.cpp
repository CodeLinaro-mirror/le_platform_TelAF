/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafECall.hpp"
#include <future>

using namespace tafsvc;

static le_result_t ConvertPaToLeResult(pa_result_t paResult)
{
    return (paResult == PA_OK) ? LE_OK : LE_FAULT;
}
static bool WaitResult(pa_result_t paResult,
                       std::future<le_result_t>& fut,
                         const char* caller)
{
    if (paResult != PA_OK)
    {
        LE_ERROR("%s: PA did not accept request, paResult=%d", caller, (int)paResult);
        return false;
    }

    try
    {
        return (fut.get() == LE_OK);
    }
    catch (const std::future_error& e)
    {
        LE_ERROR("%s: future_error on get(): %s", caller, e.what());
        return false;
    }
    catch (const std::exception& e)
    {
        LE_ERROR("%s: exception on get(): %s", caller, e.what());
        return false;
    }
}

LE_REF_DEFINE_STATIC_MAP(ECallMap, MAX_ECALL);

void tafCallCommandCallback::makeECallResponse(std::shared_ptr<taf_pa_ecall_CallInfo_t> callInfo,
        pa_result_t errorCode,std::any context) {
    std::shared_ptr<std::promise<le_result_t>> promPtr;
    try
    {
        promPtr = std::any_cast<std::shared_ptr<std::promise<le_result_t>>>(context);
        if (!promPtr) {
            LE_ERROR("makeECallResponse: null promise pointer");
            return;
        }
        if(errorCode == PA_OK) {
            LE_DEBUG("Call is successful.");
            if (callInfo)
            {
                auto &eCall = taf_ecall::GetInstance();
                int32_t callIndex = callInfo->callIndex;
                int8_t phoneId = callInfo->phoneId;
                LE_DEBUG("makeCallResponse %d, %d", callIndex, phoneId);

                RxECallEvent_t* eventPtr = (RxECallEvent_t*)le_mem_ForceAlloc(eCall.RxECallEventPool);
                if (eventPtr == nullptr) {
                    LE_ERROR("Failed to allocate memory for RxECallEvent_t");
                    promPtr->set_value(LE_FAULT);
                    return;
                }

                eventPtr->eventType = ECALL_EVENT_MAKECALL_RESP;
                eventPtr->phoneId = phoneId;
                eventPtr->param.response.callIndex = callIndex;

                le_event_ReportWithRefCounting(eCall.RxECallEventId, eventPtr);
            }
            else
            {
                LE_ERROR("makeECallResponse: PA_OK but callInfo is null");
                promPtr->set_value(LE_FAULT);
                return;
            }
        } else {
            LE_ERROR("Call failed with error code: %d ", (static_cast<int>(errorCode)));
        }
        promPtr->set_value(ConvertPaToLeResult(errorCode));
    }
    catch (const std::exception &e)
    {
       LE_ERROR("Exception: %s", e.what());
        if (promPtr) try { promPtr->set_value(LE_FAULT); } catch (...) {}
    }
    catch (...)
    {
        LE_ERROR("Unknown error in callback.");
        if (promPtr) try { promPtr->set_value(LE_FAULT); } catch (...) {}
    }
}

void tafPrieCallCommandCallback::makeECallResponse(std::shared_ptr<taf_pa_ecall_CallInfo_t> callInfo,
        pa_result_t errorCode,std::any context) {
    std::shared_ptr<std::promise<le_result_t>> promPtr;
    try
    {
        promPtr = std::any_cast<std::shared_ptr<std::promise<le_result_t>>>(context);
        if (!promPtr) {
            LE_ERROR("makeECallResponse(Prie): null promise pointer");
            return;
        }
        if(errorCode == PA_OK) {
            LE_DEBUG("Call is successful.");
            if (callInfo)
            {
                auto &eCall = taf_ecall::GetInstance();
                int32_t callIndex = callInfo->callIndex;
                int8_t phoneId = callInfo->phoneId;
                LE_DEBUG("makeCallResponse %d, %d", callIndex, phoneId);

                RxECallEvent_t* eventPtr = (RxECallEvent_t*)le_mem_ForceAlloc(eCall.RxECallEventPool);
                if (eventPtr == nullptr) {
                    LE_ERROR("Failed to allocate memory for RxECallEvent_t");
                    promPtr->set_value(LE_FAULT);
                    return;
                }

                eventPtr->eventType = ECALL_EVENT_MAKECALL_RESP;
                eventPtr->phoneId = phoneId;
                eventPtr->param.response.callIndex = callIndex;

                le_event_ReportWithRefCounting(eCall.RxECallEventId, eventPtr);
            }
            else
            {
                LE_ERROR("makeECallResponse: PA_OK but callInfo is null");
                promPtr->set_value(LE_FAULT);
                return;
            }
        } else {
            LE_ERROR("Call failed with error code: %d ", (static_cast<int>(errorCode)));
        }
        promPtr->set_value(ConvertPaToLeResult(errorCode));
    }
    catch (const std::exception &e)
    {
       LE_ERROR("Exception: %s", e.what());
        if (promPtr) try { promPtr->set_value(LE_FAULT); } catch (...) {}
    }
    catch (...)
    {
        LE_ERROR("Unknown error in callback.");
        if (promPtr) try { promPtr->set_value(LE_FAULT); } catch (...) {}
    }
}

void tafUpdateMsdCommandCallback::commandResponse(pa_result_t errorCode,std::any context) {
    std::shared_ptr<std::promise<le_result_t>> promPtr;
    try
    {
        promPtr = std::any_cast<std::shared_ptr<std::promise<le_result_t>>>(context);
        if (!promPtr) { LE_ERROR("commandResponse(UpdateMsd): null promise"); return; }
        if(errorCode == PA_OK) {
            LE_INFO("Update MSD is successful ");
        } else {
            LE_ERROR("Update MSD failed with error code: %d ", (static_cast<int>(errorCode)));
        }
        promPtr->set_value(ConvertPaToLeResult(errorCode));
    }
    catch (const std::exception &e)
    {
       LE_ERROR("Exception: %s", e.what());
        if (promPtr) try { promPtr->set_value(LE_FAULT); } catch (...) {}
    }
    catch (...)
    {
        LE_ERROR("Unknown error in callback.");
        if (promPtr) try { promPtr->set_value(LE_FAULT); } catch (...) {}
    }
}

void tafHangupCommandCallback::commandResponse(pa_result_t errorCode,std::any context) {
    std::shared_ptr<std::promise<le_result_t>> promPtr;
    try
    {
        promPtr = std::any_cast<std::shared_ptr<std::promise<le_result_t>>>(context);
        if (!promPtr) { LE_ERROR("commandResponse(Hangup): null promise"); return; }
        if(errorCode == PA_OK) {
            LE_INFO("Call hangup is successful ");
        } else {
            LE_ERROR("Call hangup failed with error code: %d ", (static_cast<int>(errorCode)));
        }
        promPtr->set_value(ConvertPaToLeResult(errorCode));
    }
    catch (const std::exception &e)
    {
       LE_ERROR("Exception: %s", e.what());
        if (promPtr) try { promPtr->set_value(LE_FAULT); } catch (...) {}
    }
    catch (...)
    {
        LE_ERROR("Unknown error in callback.");
        if (promPtr) try { promPtr->set_value(LE_FAULT); } catch (...) {}
    }
}

void tafRejectCommandCallback::commandResponse(pa_result_t errorCode,std::any context) {
    std::shared_ptr<std::promise<le_result_t>> promPtr;
    try
    {
        promPtr = std::any_cast<std::shared_ptr<std::promise<le_result_t>>>(context);
        if (!promPtr) { LE_ERROR("commandResponse(Reject): null promise"); return; }
        if(errorCode == PA_OK) {
            LE_INFO("Call reject is successful ");
        } else {
            LE_ERROR("Call reject failed with error code: %d ", (static_cast<int>(errorCode)));
        }
        promPtr->set_value(ConvertPaToLeResult(errorCode));
    }
    catch (const std::exception &e)
    {
       LE_ERROR("Exception: %s", e.what());
        if (promPtr) try { promPtr->set_value(LE_FAULT); } catch (...) {}
    }
    catch (...)
    {
        LE_ERROR("Unknown error in callback.");
        if (promPtr) try { promPtr->set_value(LE_FAULT); } catch (...) {}
    }
}

void tafAnswerCommandCallback::commandResponse(pa_result_t errorCode,std::any context) {
    std::shared_ptr<std::promise<le_result_t>> promPtr;
    try
    {
        promPtr = std::any_cast<std::shared_ptr<std::promise<le_result_t>>>(context);
        if (!promPtr) { LE_ERROR("commandResponse(Answer): null promise"); return; }
        if(errorCode == PA_OK) {
            LE_INFO("Call answer is successful ");
        } else {
            LE_ERROR("Call answer failed with error code: %d ", (static_cast<int>(errorCode)));
        }
        promPtr->set_value(ConvertPaToLeResult(errorCode));
    }
    catch (const std::exception &e)
    {
       LE_ERROR("Exception: %s", e.what());
        if (promPtr) try { promPtr->set_value(LE_FAULT); } catch (...) {}
    }
    catch (...)
    {
        LE_ERROR("Unknown error in callback.");
        if (promPtr) try { promPtr->set_value(LE_FAULT); } catch (...) {}
    }
}

void Handler::onIncomingCall(std::shared_ptr<taf_pa_ecall_CallInfo_t> callInfo,
    pa_result_t errorCode,std::any context)
{
    LE_DEBUG("Received onIncomingCall");
    if (callInfo == nullptr) {
        LE_ERROR("Received null call object in onIncomingCall");
        return;
    }
    auto &eCall = taf_ecall::GetInstance();
    RxECallEvent_t* eventPtr = (RxECallEvent_t*)le_mem_ForceAlloc(eCall.RxECallEventPool);
    if (eventPtr == nullptr) {
        LE_ERROR("Failed to allocate memory for RxECallEvent_t");
        return;
    }
    uint64_t token = eCall.StashCall(callInfo);
    eventPtr->eventType = ECALL_EVENT_INCOMING_CALL;
    eventPtr->phoneId = callInfo->phoneId;
    eventPtr->param.incomingCall.callToken = token;
    eventPtr->param.incomingCall.callIndex = callInfo->callIndex;
    eventPtr->param.incomingCall.callState = callInfo->callState;
    const std::string& number = callInfo->remotePartyNumber;
    LE_DEBUG("Incoming call remotePartyNumber: %s", number.c_str());
    if (!number.empty()) {
        le_utf8_Copy(eventPtr->param.incomingCall.remotePartyNumber, number.c_str(), MAX_DESTINATION_LEN, nullptr);
    } else {
        LE_WARN("Remote party number is empty");
        eventPtr->param.incomingCall.remotePartyNumber[0] = '\0';
    }
    le_event_ReportWithRefCounting(eCall.RxECallEventId, eventPtr);
}
void taf_ecall::HandleIncomingCall(int phoneId, const RxECallIncomingCallParam_t& incomingCall)
{
    LE_DEBUG("HandleIncomingCall");
    auto spCall = TakeCall(incomingCall.callToken);
    if (!spCall) {
        LE_ERROR("Incoming call token invalid or already consumed");
        return;
    }
    auto &eCall = taf_ecall::GetInstance();
    int32_t callIndex = incomingCall.callIndex;
    int8_t phone_Id = phoneId;
    taf_pa_ecall_call_status_t callState = incomingCall.callState;
    auto promisePtr = std::make_shared<std::promise<le_result_t>>();
    auto cb = [promisePtr, phone_Id](pa_result_t errorCode, int8_t phoneId,
        std::shared_ptr<const taf_pa_ecall_hlap_timer_status_t> hlapStatus,
        std::any context) {
        try
        {
            if((errorCode == PA_OK) && (phone_Id == phoneId) &&
               (hlapStatus->t9 == taf_pa_ecall_hlap_timer_state_t::ACTIVE))
            {
                promisePtr->set_value(LE_OK);
            }
            else
            {
                LE_ERROR("requestECallHlapTimerStatus failed errorCode: %d ", int(errorCode));
                promisePtr->set_value(LE_FAULT);
            }
        }
        catch (const std::future_error& e)
        {
            LE_ERROR("Future error in callback: %s", e.what());
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in callback: %s", e.what());
            try { promisePtr->set_value(LE_FAULT); } catch (...) {}
        }
        catch (...)
        {
            LE_ERROR("Unknown error in callback.");
            try { promisePtr->set_value(LE_FAULT); } catch (...) {}
        }
    };

    std::future<le_result_t> futResult = promisePtr->get_future();
    pa_result_t result = taf_pa_ecall_RequestHlapTimerStatus(phone_Id, cb,{});
    if (WaitResult(result, futResult, "HandleIncomingCall")) {
        if (taf_pa_ecall_call_status_t::INCOMING == callState)
        {
            taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(eCall.ECallPtrRefMap,
                eCall.GetECallReference());
            if (eCallPtr != NULL)
            {
                eCallPtr->iCall= spCall;
                tafECallSession_t sessionState = ECALL_INCOMING;
                eCall.SetSessionState(sessionState);
                eCall.SetCallIndex(callIndex);
                eCall.SetCallPhoneId(phone_Id);

                taf_ecall_State_t state = TAF_ECALL_STATE_INCOMING;
                eCall.SetStateAndReport(state, phone_Id, incomingCall.remotePartyNumber);
            } else {
                LE_ERROR("eCallPtr is nullPtr");
            }
        }
    } else {
        LE_ERROR("Get eCall hlap timer failed with status: %d", static_cast<int>(result));
    }
}

void Handler::onCallInfoChange(std::shared_ptr<taf_pa_ecall_CallInfo_t> callInfo,
    pa_result_t errorCode,std::any context)
{
    LE_DEBUG("Received onCallInfoChange");
    if (callInfo == nullptr) {
        LE_ERROR("Received null call object in onCallInfoChange");
        return;
    }

    auto &eCall = taf_ecall::GetInstance();
    RxECallEvent_t* eventPtr = (RxECallEvent_t*)le_mem_ForceAlloc(eCall.RxECallEventPool);
    if (eventPtr == nullptr) {
        LE_ERROR("Failed to allocate memory for RxECallEvent_t");
        return;
    }
    uint64_t token = eCall.StashCall(callInfo);
    eventPtr->eventType = ECALL_EVENT_CALL_INFO_CHANGE;
    eventPtr->phoneId = callInfo->phoneId;
    eventPtr->param.infoChange.callToken = token;
    eventPtr->param.infoChange.callIndex = callInfo->callIndex;
    eventPtr->param.infoChange.callState = callInfo->callState;
    eventPtr->param.infoChange.callDirection = callInfo->dir;
    eventPtr->param.infoChange.callEndCause = callInfo->endCause;

    le_event_ReportWithRefCounting(eCall.RxECallEventId, eventPtr);
}

void taf_ecall::HandleCallInfoChange(int phoneId, const RxECallInfoChangeParam_t& infoChange)
{
    LE_DEBUG("onCallInfoChange");
    auto spCall = TakeCall(infoChange.callToken);
    if (!spCall) {
        LE_ERROR("HandleCallInfoChange: token invalid or already consumed");
        return;
    }

    taf_ecall_State_t state = TAF_ECALL_STATE_UNKNOWN;
    tafECallSession_t sessionState = ECALL_INIT;

    auto &eCall = taf_ecall::GetInstance();
    taf_pa_ecall_call_status_t callState = infoChange.callState;
    int8_t CallPhoneId = phoneId;
    int32_t index = infoChange.callIndex;

    bool isCallStateSet = false;

    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(eCall.ECallPtrRefMap, eCall.GetECallReference());
    TAF_ERROR_IF_RET_NIL(eCallPtr == NULL, "cannot get callptr");
    LE_DEBUG("onCallInfoChange index %d %d, phoneId  %d, %d, state %d", eCallPtr->callIndex, index, eCallPtr->phoneId, CallPhoneId, (int)callState);
    if (((eCallPtr->callIndex != index) && (eCallPtr->phoneId == CallPhoneId)) ||
        (eCallPtr->phoneId != CallPhoneId))
    {
        LE_ERROR("Cannot match the index or phoneId");
        return;
    }

    eCall.CallEndError = taf_pa_ecall_termination_t::NORMAL;
    LE_DEBUG("Call state: %d", (int) callState);

    if (callState == taf_pa_ecall_call_status_t::ACTIVE)
    {
        sessionState = ECALL_ACTIVE;
        state = TAF_ECALL_STATE_ACTIVE;
        isCallStateSet = true;

    }
    else if (callState == taf_pa_ecall_call_status_t::ALERTING)
    {
        sessionState = ECALL_ALERTING;
        state = TAF_ECALL_STATE_ALERTING;
        isCallStateSet = true;
    }
    else if (callState == taf_pa_ecall_call_status_t::DIALING)
    {
        sessionState = ECALL_DIALING;
        state = TAF_ECALL_STATE_DIALING;
        isCallStateSet = true;
    }
    else if (callState == taf_pa_ecall_call_status_t::INCOMING)
    {

    }
    else if (callState == taf_pa_ecall_call_status_t::ENDED)
    {
        if ((infoChange.callDirection == taf_pa_ecall_dir_t::INCOMING) ||
            ((eCallPtr->type != TAF_ECALL_TYPE_TEST) &&
             (eCallPtr->type != TAF_ECALL_TYPE_AUTO) &&
             (eCallPtr->type != TAF_ECALL_TYPE_MANUAL) &&
             (infoChange.callDirection == taf_pa_ecall_dir_t::OUTGOING)) ||
             (eCall.needReportCallEndOnReboot == true)
        )
        {
            state = TAF_ECALL_STATE_ENDED;
            isCallStateSet = true;
            eCall.SetCallIndex(-1);
            eCall.SetCallPhoneId(-1);
            eCallPtr->redialReason = TAF_ECALL_TERMINATION_REDIAL_REASON_NONE;
        }

        eCallPtr->waitForALACKPos = false;
        sessionState = ECALL_ENDED;
        eCall.CallEndError = infoChange.callEndCause;
        LE_DEBUG("ECall ENDed terminate reason = %d", (int) eCall.CallEndError);
    }

    eCall.SetSessionState(sessionState);
    if (isCallStateSet)
    {
        eCallPtr->iCall= spCall;
        eCall.SetStateAndReport(state, phoneId, "");

        if (eCall.needReportCallEndOnReboot == true)
        {
            eCall.needReportCallEndOnReboot = false;
            ResumeHlapTimerEvent_t resumeEvent;
            resumeEvent.event  = EVENT_ECALL_IN_PROGRESS_MODEM_REBOOT;
            le_event_Report(eCall.ResumeHlapTimerEventId, &resumeEvent, sizeof(ResumeHlapTimerEvent_t));
        }
    }
}

void taf_ecall::HandleMsdTransmissionStatus(int phoneId, taf_pa_ecall_msd_status_t status)
{
    LE_DEBUG("MSD Transmission Status: phoneId=%d, status=%d", phoneId, (int)status);
    auto &eCall = taf_ecall::GetInstance();

    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(eCall.ECallPtrRefMap, eCall.GetECallReference());
    if (eCallPtr == nullptr)
    {
        LE_ERROR("eCallPtr is nullptr.");
        return;
    }

    LE_DEBUG("eCallMsdTransmissionStatusToState status = %d", (int)status);

    taf_ecall_State_t state = TAF_ECALL_STATE_UNKNOWN;
    switch(status) {
        case taf_pa_ecall_msd_status_t::SUCCESS:
            if (eCallPtr->waitForALACKPos)
            {
                state = TAF_ECALL_STATE_ALACK_RECEIVED_POSITIVE;
                eCall.SetStateAndReport(state, phoneId, "");
                eCallPtr->waitForALACKPos = false;
            }
            state = TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS;
            break;
        case taf_pa_ecall_msd_status_t::FAILURE:
            state = TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED;
            break;
        case taf_pa_ecall_msd_status_t::TRANSMISSION_STARTED:
            state = TAF_ECALL_STATE_MSD_TRANSMISSION_STARTED;
            eCallPtr->waitForALACKPos = false;
            break;
        case taf_pa_ecall_msd_status_t::NACK_OUT_OF_ORDER:
            state = TAF_ECALL_STATE_NACK_OUT_OF_ORDER;
            break;
        case taf_pa_ecall_msd_status_t::ACK_OUT_OF_ORDER:
            state = TAF_ECALL_STATE_ACK_OUT_OF_ORDER;
            break;
        case taf_pa_ecall_msd_status_t::START_RECEIVED:
            state = TAF_ECALL_STATE_PSAP_START_RECEIVED;
            break;
        case taf_pa_ecall_msd_status_t::LL_ACK_RECEIVED:
            state = TAF_ECALL_STATE_LL_ACK_RECEIVED;
            eCallPtr->waitForALACKPos = true;
            break;
        case taf_pa_ecall_msd_status_t::MSD_AL_ACK_CLEARDOWN:
            state = TAF_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN;
            eCallPtr->waitForALACKPos = false;
            break;
        case taf_pa_ecall_msd_status_t::LL_NACK_DUE_TO_T7_EXPIRY:
            state = TAF_ECALL_STATE_LL_NACK_DUE_TO_T7_EXPIRY;
            break;
        case taf_pa_ecall_msd_status_t::OUTBAND_MSD_TRANSMISSION_STARTED:
            state = TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_STARTED;
            break;
        case taf_pa_ecall_msd_status_t::OUTBAND_MSD_TRANSMISSION_SUCCESS:
            state = TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_SUCCESS;
            break;
        case taf_pa_ecall_msd_status_t::OUTBAND_MSD_TRANSMISSION_FAILURE:
            state = TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_FAILURE;
            break;
        default:
            LE_ERROR( "Unknown ECallMsdTransmissionStatus  = %d", (int)status);
    }

    if (state != TAF_ECALL_STATE_UNKNOWN)
    {
        eCall.SetStateAndReport(state, phoneId, "");
    }
}

void Handler::onMsdTransmissionStatus(int32_t phoneId,taf_pa_ecall_msd_status_t msdStatus,
    std::any context)
{
    LE_DEBUG("Received onECallMsdTransmissionStatus phoneId = %d, status = %d", phoneId, (int)msdStatus);

    auto &eCall = taf_ecall::GetInstance();
    RxECallEvent_t* eventPtr = (RxECallEvent_t*)le_mem_ForceAlloc(eCall.RxECallEventPool);
    if (eventPtr == nullptr) {
        LE_ERROR("Failed to allocate memory for RxECallEvent_t");
        return;
    }
    eventPtr->eventType = ECALL_EVENT_MSD_TRANSMISSION_STATUS;
    eventPtr->phoneId = phoneId;
    eventPtr->param.msdTransmissionStatus.msdTransmissionStatus = msdStatus;

    le_event_ReportWithRefCounting(eCall.RxECallEventId, eventPtr);
}

void Handler::onMsdUpdateRequest(int32_t phoneId,std::any context)
{
    LE_DEBUG("Received OnMsdUpdateRequest");

    auto &eCall = taf_ecall::GetInstance();
    RxECallEvent_t* eventPtr = (RxECallEvent_t*)le_mem_ForceAlloc(eCall.RxECallEventPool);
    if (eventPtr == nullptr) {
        LE_ERROR("Failed to allocate memory for RxECallEvent_t");
        return;
    }

    eventPtr->eventType = ECALL_EVENT_MSD_UPDATE_REQ;
    eventPtr->phoneId = phoneId;

    le_event_ReportWithRefCounting(eCall.RxECallEventId, eventPtr);
}

void taf_ecall::HandleMsdUpdateRequest(int phoneId)
{
    LE_DEBUG("RequestMsdUpdate: phoneId=%d", phoneId);

    taf_ecall_State_t state = TAF_ECALL_STATE_MSD_UPDATE_REQ;
    SetStateAndReport(state, phoneId, "");
}

void Handler::onRedial(int32_t phoneId,
    std::shared_ptr<taf_pa_ecall_redial_info_t> redialInfo,std::any context)
{
    LE_DEBUG("Received onECallRedial");
    if (redialInfo == nullptr) {
        LE_ERROR("Received null redialInfo in onRedial");
        return;
    }

    auto &eCall = taf_ecall::GetInstance();
    RxECallEvent_t* eventPtr = (RxECallEvent_t*)le_mem_ForceAlloc(eCall.RxECallEventPool);
    if (eventPtr == nullptr) {
        LE_ERROR("Failed to allocate memory for RxECallEvent_t");
        return;
    }
    eventPtr->eventType = ECALL_EVENT_REDIAL;
    eventPtr->phoneId = phoneId;
    eventPtr->param.redial.redialInfo = *redialInfo;

    le_event_ReportWithRefCounting(eCall.RxECallEventId, eventPtr);
}

void taf_ecall::HandleRedial(int phoneId, taf_pa_ecall_redial_info_t redialInfo)
{
    LE_DEBUG("Redial Event: phoneId=%d, willRedial=%d, redialReason=%d", phoneId,
                redialInfo.willEcallRedial, (int)redialInfo.reason);

    auto &eCall = taf_ecall::GetInstance();
    taf_ecall_CallRef_t callRef = eCall.GetECallReference();
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(eCall.ECallPtrRefMap, callRef);
    if (!eCallPtr)
    {
        LE_ERROR("HandleRedial: invalid eCallRef");
        return;
    }
    eCallPtr->redialReason = eCall.MapRedialReason(redialInfo.reason);

    taf_ecall_State_t state = TAF_ECALL_STATE_UNKNOWN;

    if (redialInfo.willEcallRedial == true)
    {
        state = TAF_ECALL_STATE_END_OF_REDIAL_PERIOD;
    } else {
        state = TAF_ECALL_STATE_ENDED;
        eCall.SetCallIndex(-1);
        eCall.SetCallPhoneId(-1);
    }

    eCall.SetStateAndReport(state, phoneId, "");
}

void Handler::onHlapTimerEvent(int32_t phoneId,
    std::shared_ptr<taf_pa_ecall_hlap_timer_events_t> timerEvent,std::any context)
{
    if (timerEvent == nullptr) {
        LE_ERROR("Received null timerEvent in onHlapTimerEvent");
        return;
    }

    LE_DEBUG("Received onECallHlapTimerEvent t2: %d, t5: %d, t6: %d, t7:  %d, t9: %d, t10: %d",
        static_cast<int>(timerEvent->t2), static_cast<int>(timerEvent->t5), static_cast<int>(timerEvent->t6),
        static_cast<int>(timerEvent->t7), static_cast<int>(timerEvent->t9), static_cast<int>(timerEvent->t10));

    auto &eCall = taf_ecall::GetInstance();
    RxECallEvent_t* eventPtr = (RxECallEvent_t*)le_mem_ForceAlloc(eCall.RxECallEventPool);
    if (eventPtr == nullptr) {
        LE_ERROR("Failed to allocate memory for RxECallEvent_t");
        return;
    }

    eventPtr->eventType = ECALL_EVENT_HLAP_TIMER;
    eventPtr->phoneId = phoneId;
    eventPtr->param.hlapTimer.timerEvents = *timerEvent;

    le_event_ReportWithRefCounting(eCall.RxECallEventId, eventPtr);
}

void taf_ecall::HandleHlapTimerEvent(int phoneId, taf_pa_ecall_hlap_timer_events_t timerEvent)
{
    LE_DEBUG("HLAP Timer Event: phoneId=%d", phoneId);

    taf_ecall_State_t state = TAF_ECALL_STATE_UNKNOWN;
    auto &eCall = taf_ecall::GetInstance();

    if ((timerEvent.t2 != taf_pa_ecall_hlap_event_t::UNCHANGED)
        && (timerEvent.t2 != taf_pa_ecall_hlap_event_t::UNKNOWN)) {
        if(timerEvent.t2 == taf_pa_ecall_hlap_event_t::EXPIRED) {
            state = TAF_ECALL_STATE_T2_EXPIRED;
            eCall.t2StartTimeSet = false;
        }
        if(timerEvent.t2 == taf_pa_ecall_hlap_event_t::STARTED) {
            state = TAF_ECALL_STATE_T2_STARTED;
            if (clock_gettime(CLOCK_BOOTTIME, &eCall.t2StartTime) != 0) {
                LE_ERROR("Failed to get CLOCK_BOOTTIME for T2, errno=%d", errno);
                eCall.t2StartTime = {0, 0};
                eCall.t2StartTimeSet = false;
            } else {
                eCall.t2StartTimeSet = true;
            }
        }
        if(timerEvent.t2 == taf_pa_ecall_hlap_event_t::STOPPED) {
            state = TAF_ECALL_STATE_T2_STOPPED;
            eCall.t2StartTimeSet = false;
        }

        if (state != TAF_ECALL_STATE_UNKNOWN) {
            eCall.SetStateAndReport(state, phoneId, "");
        }
    }

    if ((timerEvent.t5 != taf_pa_ecall_hlap_event_t::UNCHANGED)
        && (timerEvent.t5!= taf_pa_ecall_hlap_event_t::UNKNOWN)) {
        if(timerEvent.t5 == taf_pa_ecall_hlap_event_t::EXPIRED) {
            state = TAF_ECALL_STATE_T5_EXPIRED;
        }
        if(timerEvent.t5 == taf_pa_ecall_hlap_event_t::STARTED) {
            state = TAF_ECALL_STATE_T5_STARTED;
        }
        if(timerEvent.t5 == taf_pa_ecall_hlap_event_t::STOPPED) {
            state = TAF_ECALL_STATE_T5_STOPPED;
        }

        if (state != TAF_ECALL_STATE_UNKNOWN) {
            eCall.SetStateAndReport(state, phoneId, "");
        }
    }

    if ((timerEvent.t6 != taf_pa_ecall_hlap_event_t::UNCHANGED)
        && (timerEvent.t6!= taf_pa_ecall_hlap_event_t::UNKNOWN)) {
        if(timerEvent.t6 == taf_pa_ecall_hlap_event_t::EXPIRED) {
            state = TAF_ECALL_STATE_T6_EXPIRED;
        }
        if(timerEvent.t6== taf_pa_ecall_hlap_event_t::STARTED) {
            state = TAF_ECALL_STATE_T6_STARTED;
        }
        if(timerEvent.t6 == taf_pa_ecall_hlap_event_t::STOPPED) {
            state = TAF_ECALL_STATE_T6_STOPPED;
        }

        if (state != TAF_ECALL_STATE_UNKNOWN) {
            eCall.SetStateAndReport(state, phoneId, "");
        }
    }

    if ((timerEvent.t7 != taf_pa_ecall_hlap_event_t::UNCHANGED)
        && (timerEvent.t7 != taf_pa_ecall_hlap_event_t::UNKNOWN)) {
        if(timerEvent.t7 == taf_pa_ecall_hlap_event_t::EXPIRED) {
            state = TAF_ECALL_STATE_T7_EXPIRED;
        }
        if(timerEvent.t7 == taf_pa_ecall_hlap_event_t::STARTED) {
            state = TAF_ECALL_STATE_T7_STARTED;
        }
        if(timerEvent.t7 == taf_pa_ecall_hlap_event_t::STOPPED) {
            state = TAF_ECALL_STATE_T7_STOPPED;
        }

        if (state != TAF_ECALL_STATE_UNKNOWN) {
            eCall.SetStateAndReport(state, phoneId, "");
        }
    }

    if ((timerEvent.t9 != taf_pa_ecall_hlap_event_t::UNCHANGED)
        && (timerEvent.t9 != taf_pa_ecall_hlap_event_t::UNKNOWN)) {
        if(timerEvent.t9 == taf_pa_ecall_hlap_event_t::EXPIRED) {
            state = TAF_ECALL_STATE_T9_EXPIRED;
            eCall.t9StartTimeSet = false;
        }
        if(timerEvent.t9 == taf_pa_ecall_hlap_event_t::STARTED) {
            state = TAF_ECALL_STATE_T9_STARTED;
            if (clock_gettime(CLOCK_BOOTTIME, &eCall.t9StartTime) != 0) {
                LE_ERROR("Failed to get CLOCK_BOOTTIME for T9, errno=%d", errno);
                eCall.t9StartTime = {0, 0};
                eCall.t9StartTimeSet = false;
            } else {
                eCall.t9StartTimeSet = true;
            }
            eCall.ElapsedTimeT9 = 0;
        }
        if(timerEvent.t9 == taf_pa_ecall_hlap_event_t::STOPPED) {
            state = TAF_ECALL_STATE_T9_STOPPED;
            eCall.t9StartTimeSet = false;
        }
        if(timerEvent.t9 == taf_pa_ecall_hlap_event_t::RESUMED) {
            if (eCall.needReportT9Start)
            {
                LE_DEBUG("T9 time started");
                state = TAF_ECALL_STATE_T9_STARTED;
                eCall.needReportT9Start = false;
            } else {
            state = TAF_ECALL_STATE_T9_RESUMED;
            }
            if (clock_gettime(CLOCK_BOOTTIME, &eCall.t9StartTime) != 0) {
                LE_ERROR("Failed to get CLOCK_BOOTTIME for T9, errno=%d", errno);
                eCall.t9StartTime = {0, 0};
                eCall.t9StartTimeSet = false;
            } else {
                eCall.t9StartTimeSet = true;
            }
        }

        if (state != TAF_ECALL_STATE_UNKNOWN) {
            eCall.SetStateAndReport(state, phoneId, "");

            ResumeHlapTimerEvent_t resumeEvent;
            resumeEvent.event  = EVENT_SAVE_HLAP_TIMER_ELAPSED;
            resumeEvent.hlapTimerType  = HLAP_TIMER_TYPE_T9;
            resumeEvent.hlapTimerEventType = eCall.ConvertHlapTimerEvent(timerEvent.t9);
            le_event_Report(eCall.ResumeHlapTimerEventId, &resumeEvent,
                sizeof(ResumeHlapTimerEvent_t));
        }
    }

    if ((timerEvent.t10 != taf_pa_ecall_hlap_event_t::UNCHANGED)
        && (timerEvent.t10 != taf_pa_ecall_hlap_event_t::UNKNOWN)) {
        if(timerEvent.t10 == taf_pa_ecall_hlap_event_t::EXPIRED) {
            state = TAF_ECALL_STATE_T10_EXPIRED;
            eCall.t10StartTimeSet = false;
        }
        if(timerEvent.t10 == taf_pa_ecall_hlap_event_t::STARTED) {
            state = TAF_ECALL_STATE_T10_STARTED;
            if (clock_gettime(CLOCK_BOOTTIME, &eCall.t10StartTime) != 0) {
                LE_ERROR("Failed to get CLOCK_BOOTTIME for T10, errno=%d", errno);
                eCall.t10StartTime = {0, 0};
                eCall.t10StartTimeSet = false;
            } else {
                eCall.t10StartTimeSet = true;
            }
            eCall.ElapsedTimeT10 = 0;
        }
        if(timerEvent.t10 == taf_pa_ecall_hlap_event_t::STOPPED) {
            state = TAF_ECALL_STATE_T10_STOPPED;
            eCall.t10StartTimeSet = false;
        }
        if(timerEvent.t10 == taf_pa_ecall_hlap_event_t::RESUMED) {
            if (eCall.needReportT10Start)
            {
                LE_DEBUG("T10 time started");
                state = TAF_ECALL_STATE_T10_STARTED;
                eCall.needReportT10Start = false;
            } else {
                state = TAF_ECALL_STATE_T10_RESUMED;
            }
            if (clock_gettime(CLOCK_BOOTTIME, &eCall.t10StartTime) != 0) {
                LE_ERROR("Failed to get CLOCK_BOOTTIME for T10, errno=%d", errno);
                eCall.t10StartTime = {0, 0};
                eCall.t10StartTimeSet = false;
            } else {
                eCall.t10StartTimeSet = true;
            }
        }

        if (state != TAF_ECALL_STATE_UNKNOWN) {
            eCall.SetStateAndReport(state, phoneId, "");

            ResumeHlapTimerEvent_t resumeEvent;
            resumeEvent.event  = EVENT_SAVE_HLAP_TIMER_ELAPSED;
            resumeEvent.hlapTimerType  = HLAP_TIMER_TYPE_T10;
            resumeEvent.hlapTimerEventType = eCall.ConvertHlapTimerEvent(timerEvent.t10);
            le_event_Report(eCall.ResumeHlapTimerEventId, &resumeEvent,
                sizeof(ResumeHlapTimerEvent_t));
        }
    }
}

void Handler::onEcallOperatingModeChange(int32_t phoneId,
    std::shared_ptr<taf_pa_ecall_mode_info_t> modeInfo,std::any context)
{
    LE_DEBUG("onECallOperatingModeChange operation mode is %d", (int)modeInfo->mode);
    auto &eCall = taf_ecall::GetInstance();

    if ((eCall.pendingResumeT9 || eCall.pendingResumeT10) &&
        (phoneId == eCall.lastCallPhoneId))
        {
        ResumeHlapTimerEvent_t resumeEvent;
        resumeEvent.event = EVENT_RESUME_HLAP_TIMER_ON_MODE_CHANGE;
        resumeEvent.phoneId = phoneId;
        resumeEvent.eCallMode = modeInfo->mode;
        le_event_Report(eCall.ResumeHlapTimerEventId, &resumeEvent, sizeof(ResumeHlapTimerEvent_t));
    }
}

void Handler::onStateChange(std::shared_ptr<taf_pa_ecall_subsystem_info_t> info,
    taf_pa_ecall_operational_status_t status,
    std::any context)
{
    LE_DEBUG("onStateChange Location %d, Subsystem %d, New status %d",
        static_cast<int>(info->location), static_cast<int>(info->subsystems),
        static_cast<int>(status));

    auto &eCall = taf_ecall::GetInstance();
    if(status == taf_pa_ecall_operational_status_t::UNAVAILABLE)
    {
        ResumeHlapTimerEvent_t resumeEvent;
        resumeEvent.event  = EVENT_MODEM_OPERATIONALSTATUS_UNAVILABLE;
        le_event_Report(eCall.ResumeHlapTimerEventId, &resumeEvent, sizeof(ResumeHlapTimerEvent_t));
    } else if(status == taf_pa_ecall_operational_status_t::OPERATIONAL) {
        ResumeHlapTimerEvent_t resumeEvent;
        resumeEvent.event  = EVENT_MODEM_OPERATIONALSTATUS_OPERATIONAL;
        le_event_Report(eCall.ResumeHlapTimerEventId, &resumeEvent, sizeof(ResumeHlapTimerEvent_t));
    }
}

uint64_t taf_ecall::StashCall(std::shared_ptr<taf_pa_ecall_CallInfo_t> sp) {
    uint64_t t = callNextToken_.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lk(callMtx_);
        callStore_[t] = std::move(sp);
    }
    return t;
}

std::shared_ptr<taf_pa_ecall_CallInfo_t> taf_ecall::TakeCall(uint64_t token) {
    std::lock_guard<std::mutex> lk(callMtx_);
    auto it = callStore_.find(token);
    if (it == callStore_.end()) return {};
    auto sp = std::move(it->second);
    callStore_.erase(it);
    return sp;
}

void taf_ecall::HandleMakeCallResp(int phoneId, RxECallMakeCallResponse resp)
{
    LE_DEBUG("MakeCall response Event: phoneId=%d, callIndex=%d", phoneId, resp.callIndex);

    SetCallIndex(resp.callIndex);
    SetCallPhoneId(phoneId);
    ProcessPendingCallEvents(phoneId, resp.callIndex);
}

void taf_ecall::ProcessRxECallEvent(void* msgPtr)
{
    if (msgPtr == nullptr) {
        LE_WARN("RxECallEvent msgPtr is NULL.");
        return;
    }

    taf_ecall& eCall = taf_ecall::GetInstance();
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(eCall.ECallPtrRefMap, eCall.GetECallReference());
    TAF_ERROR_IF_RET_NIL(eCallPtr == NULL, "cannot get callptr");

    int32_t currentCallIndex = eCallPtr->callIndex;
    int8_t currentPhoneId = eCallPtr->phoneId;
    LE_INFO("ProcessRxECallEvent index %d, phoneId  %d", eCallPtr->callIndex, eCallPtr->phoneId);
    bool makeCallRespReceived = (currentCallIndex >= 0 && currentPhoneId >= 0);
    RxECallEvent_t* eventPtr = (RxECallEvent_t*)msgPtr;
    bool needCache = false;
    LE_INFO("RxECallEventType: %d", eventPtr->eventType);
    switch (eventPtr->eventType)
    {
        case ECALL_EVENT_CALL_INFO_CHANGE:
        {
            taf_pa_ecall_call_status_t eventCallState = eventPtr->param.infoChange.callState;
            int32_t eventCallIndex = eventPtr->param.infoChange.callIndex;
            int8_t eventPhoneId = eventPtr->phoneId;
            LE_INFO("Call state = %d", (int)eventCallState);
            if (!makeCallRespReceived) {
                if (eventCallState == taf_pa_ecall_call_status_t::ENDED) {
                    LE_INFO("CALL_INFO_CHANGE with CALL_ENDED received before makeCallResp, cleaning up directly.");
                    eCall.HandleCallEnd(eventPhoneId, eventCallIndex);
                } else {
                    eCall.pendingECallEvents.push_back({
                        eventPtr->eventType,
                        eventPhoneId,
                        eventCallIndex,
                        eCall.CloneEventData(eventPtr)
                    });
                }
                needCache = true;
            }
            break;
        }
        case ECALL_EVENT_REDIAL:
        case ECALL_EVENT_MSD_TRANSMISSION_STATUS:
        case ECALL_EVENT_HLAP_TIMER:
        case ECALL_EVENT_MSD_UPDATE_REQ:
        {
            LE_INFO("eCall session = %d", eCallPtr->eCallSession);
            if (!makeCallRespReceived && (eCallPtr->eCallSession != ECALL_INIT) && (eCallPtr->eCallSession != ECALL_ENDED)) {
                eCall.pendingECallEvents.push_back({
                    eventPtr->eventType,
                    eventPtr->phoneId,
                    -1,
                    eCall.CloneEventData(eventPtr)
                });
                needCache = true;
            }
            break;
        }
        default:
            break;
    }
    if (!needCache) {
    switch (eventPtr->eventType)
    {
        case ECALL_EVENT_INCOMING_CALL:
            eCall.HandleIncomingCall(eventPtr->phoneId, eventPtr->param.incomingCall);
            break;
        case ECALL_EVENT_CALL_INFO_CHANGE:
            eCall.HandleCallInfoChange(eventPtr->phoneId, eventPtr->param.infoChange);
            break;
        case ECALL_EVENT_MSD_TRANSMISSION_STATUS:
            eCall.HandleMsdTransmissionStatus(eventPtr->phoneId, eventPtr->param.msdTransmissionStatus.msdTransmissionStatus);
            break;
        case ECALL_EVENT_HLAP_TIMER:
            eCall.HandleHlapTimerEvent(eventPtr->phoneId, eventPtr->param.hlapTimer.timerEvents);
            break;
        case ECALL_EVENT_MSD_UPDATE_REQ:
            eCall.HandleMsdUpdateRequest(eventPtr->phoneId);
            break;
        case ECALL_EVENT_REDIAL:
            eCall.HandleRedial(eventPtr->phoneId, eventPtr->param.redial.redialInfo);
            break;
        case ECALL_EVENT_MAKECALL_RESP:
            eCall.HandleMakeCallResp(eventPtr->phoneId, eventPtr->param.response);
            break;
        default:
            LE_WARN("Unknown RxECallEventType: %d", eventPtr->eventType);
            break;
    }
    }

    le_mem_Release(eventPtr);
}

void* taf_ecall::CloneEventData(const RxECallEvent_t* eventPtr) {
    switch (eventPtr->eventType) {
        case ECALL_EVENT_CALL_INFO_CHANGE: {
            RxECallInfoChangeParam_t* infoChange = new RxECallInfoChangeParam_t(eventPtr->param.infoChange);
            return infoChange;
        }
        case ECALL_EVENT_REDIAL: {
            taf_pa_ecall_redial_info_t* info = new taf_pa_ecall_redial_info_t(eventPtr->param.redial.redialInfo);
            return info;
        }
        case ECALL_EVENT_MSD_TRANSMISSION_STATUS: {
            auto* status = new taf_pa_ecall_msd_status_t(eventPtr->param.msdTransmissionStatus.msdTransmissionStatus);
            return status;
        }
        case ECALL_EVENT_HLAP_TIMER: {
            taf_pa_ecall_hlap_timer_events_t* timer = new taf_pa_ecall_hlap_timer_events_t(eventPtr->param.hlapTimer.timerEvents);
            return timer;
        }
        case ECALL_EVENT_MSD_UPDATE_REQ: {
            return nullptr;
        }
        default:
            return nullptr;
    }
}

void taf_ecall::FreeEventData(RxECallEventType_t eventType, void* data) {
    switch (eventType) {
        case ECALL_EVENT_CALL_INFO_CHANGE:
            delete (RxECallInfoChangeParam_t*)data;
            break;
        case ECALL_EVENT_REDIAL:
            delete (taf_pa_ecall_redial_info_t*)data;
            break;
        case ECALL_EVENT_MSD_TRANSMISSION_STATUS:
            delete (taf_pa_ecall_msd_status_t*)data;
            break;
        case ECALL_EVENT_HLAP_TIMER:
            delete (taf_pa_ecall_hlap_timer_events_t*)data;
            break;
        case ECALL_EVENT_MSD_UPDATE_REQ:
            break;
        default:
            break;
    }
}

void taf_ecall::HandleCallEnd(int phoneId, int callIndex)
{
    LE_INFO("CallEnd Event: phoneId=%d, callIndex=%d, clearing cache...", phoneId, callIndex);
    auto it = pendingECallEvents.begin();
    while (it != pendingECallEvents.end()) {
        bool match = false;
        if (it->eventType == ECALL_EVENT_CALL_INFO_CHANGE) {
            match = (it->callIndex == callIndex && it->phoneId == phoneId);
        } else {
            match = (it->phoneId == phoneId);
        }
        if (match) {
            LE_INFO("Clearing pending event: type=%d, callIndex=%d, phoneId=%d", it->eventType, it->callIndex, it->phoneId);
            FreeEventData(it->eventType, it->eventData);
            it = pendingECallEvents.erase(it);
        } else {
            ++it;
        }
    }
}

void taf_ecall::ProcessPendingCallEvents(int phoneId, int callIndex)
{
    LE_INFO("ProcessPendingCalls index %d, phoneId  %d", callIndex, phoneId);
    auto it = pendingECallEvents.begin();
    while (it != pendingECallEvents.end()) {
        bool match = false;
        if (it->eventType == ECALL_EVENT_CALL_INFO_CHANGE) {
                match = (it->callIndex == callIndex && it->phoneId == phoneId);
        } else {
                match = (it->phoneId == phoneId);
            }
        LE_INFO("PendingEvent: type=%d, callIndex=%d, phoneId=%d, match=%d",
            it->eventType, it->callIndex, it->phoneId, match);
        if (match) {
            switch (it->eventType) {
                case ECALL_EVENT_CALL_INFO_CHANGE:
                    HandleCallInfoChange(phoneId, *(RxECallInfoChangeParam_t*)(it->eventData));
                    break;
                case ECALL_EVENT_REDIAL:
                    HandleRedial(phoneId, *(taf_pa_ecall_redial_info_t*)(it->eventData));
                    break;
                case ECALL_EVENT_MSD_TRANSMISSION_STATUS:
                    HandleMsdTransmissionStatus(phoneId, *(taf_pa_ecall_msd_status_t*)(it->eventData));
                    break;
                case ECALL_EVENT_HLAP_TIMER:
                    HandleHlapTimerEvent(phoneId, *(taf_pa_ecall_hlap_timer_events_t*)(it->eventData));
                    break;
                case ECALL_EVENT_MSD_UPDATE_REQ:
                    HandleMsdUpdateRequest(phoneId);
                    break;
                default:
                    break;
            }
        }
        FreeEventData(it->eventType, it->eventData);
        it = pendingECallEvents.erase(it);
    }
}

void taf_ecall::InitializeECallPtr()
{

    ECallPtrRefMap = le_ref_InitStaticMap(ECallMap, MAX_ECALL);

    ECallObject.msd.optionalData.optionalDataType = taf_pa_ecall_optional_data_type_t::DEFAULT;
    ECallObject.msd.optionalData.isMsdOptionalDataPresent = false;
    ECallObject.msd.optionalData.recentVehicleLocationN1Present = false;
    ECallObject.msd.optionalData.recentVehicleLocationN2Present = false;
    ECallObject.msd.optionalData.numberOfPassengersPresent = false;
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
    ECallObject.msd.control.vehicleType = (taf_pa_ecall_vehicle_type_t)TAF_ECALL_PASSENGER_VEHICLE_CLASS_M1;

    ECallObject.msd.vehicleIdentification.isowmi = '\0';
    ECallObject.msd.vehicleIdentification.isovds = '\0';
    ECallObject.msd.vehicleIdentification.isovisModelyear = '\0';
    ECallObject.msd.vehicleIdentification.isovisSeqPlant = '\0';

    ECallObject.msd.propulsionType.gasolineTankPresent = false;
    ECallObject.msd.propulsionType.dieselTankPresent = false;
    ECallObject.msd.propulsionType.compressedNaturalGas = false;
    ECallObject.msd.propulsionType.liquidPropaneGas = false;
    ECallObject.msd.propulsionType.electricEnergyStorage = false;
    ECallObject.msd.propulsionType.hydrogenStorage = false;
    ECallObject.msd.propulsionType.otherStorage = false;

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

    ECallObject.msd.recentVehicleLocationN1.positionLatitude = 0;
    ECallObject.msd.recentVehicleLocationN1.positionLongitude = 0;
    ECallObject.msd.recentVehicleLocationN2.positionLatitude = 0;
    ECallObject.msd.recentVehicleLocationN2.positionLongitude = 0;

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
    std::vector<int> initFailPara = {};
    std::vector<int> callDropPara = {};
    pa_result_t res = taf_pa_ecall_GetEcallRedial(initFailPara, callDropPara);
    if(res == PA_OK) {
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

    ECallObject.callIndex = -1;
    ECallObject.phoneId = -1;
    ECallObject.waitForALACKPos = false;
    ECallObject.redialReason = TAF_ECALL_TERMINATION_REDIAL_REASON_NONE;
    UpdateMsd();
}

void taf_ecall::Init(void)
{
    //Intialize Platform Adaptor
    pa_result_t initRes = taf_pa_ecall_Init();
    if (initRes != PA_OK) {
        LE_FATAL("Unable to Initialize ecall platoform Adaptor");
    }

    //Register Listener for Platform Adaptor
    eventListener.onIncomingCall = &Handler::onIncomingCall;
    eventListener.onCallInfoChange = &Handler::onCallInfoChange;
    eventListener.onMsdTransmissionStatus = &Handler::onMsdTransmissionStatus;
    eventListener.onMsdUpdateRequest = &Handler::onMsdUpdateRequest;
    eventListener.onRedial = &Handler::onRedial;
    eventListener.onHlapTimerEvent = &Handler::onHlapTimerEvent;
    eventListener.onEcallOperatingModeChange = &Handler::onEcallOperatingModeChange;
    eventListener.onStateChange = &Handler::onStateChange;

    pa_result_t regRes = taf_pa_ecall_RegisterListener(&eventListener,{});
    if (regRes != PA_OK) {
        LE_FATAL("Unable to register listener to ecall platoform Adaptor");
    }

    InitializeECallPtr();


    StateChangeEventId = le_event_CreateId("NewStateEventId", sizeof(StateChangeEvent_t));

    le_cfg_AddChangeHandler(CFG_MODEMSERVICE_ECALL_PATH, ConfigChangeHandler, NULL);

    RxECallEventPool = le_mem_CreatePool("RxECallEventPool", sizeof(RxECallEvent_t));
    if (RxECallEventPool == NULL)
    {
       LE_FATAL("Fail to create RxECallEventPool.");
    }
    le_mem_ExpandPool(RxECallEventPool, RX_ECALL_EVENT_POOL_SIZE);
    RxECallEventId = le_event_CreateIdWithRefCounting("Received eCall Event");
    le_event_AddHandler("Received eCall Event Handler", RxECallEventId, ProcessRxECallEvent);
    ResumeHlapTimerEventId = le_event_CreateId("ResumeHlapTimerEventId", sizeof(ResumeHlapTimerEvent_t));
    le_event_AddHandler("Resume Hlap Timer Event Handler", ResumeHlapTimerEventId, ResumeHlapTimerEventHandler);

    elapsedTimeT9Ref = le_timer_Create("elapsedTimeT9");
    le_timer_SetMsInterval(elapsedTimeT9Ref, 60000);
    le_timer_SetHandler(elapsedTimeT9Ref, T9TimerExpiryHandler);
    le_timer_SetRepeat(elapsedTimeT9Ref, 0);
    le_timer_SetWakeup(elapsedTimeT9Ref, false);

    elapsedTimeT10Ref = le_timer_Create("elapsedTimeT10");
    le_timer_SetMsInterval(elapsedTimeT10Ref, 60000);
    le_timer_SetHandler(elapsedTimeT10Ref, T10TimerExpiryHandler);
    le_timer_SetRepeat(elapsedTimeT10Ref, 0);
    le_timer_SetWakeup(elapsedTimeT10Ref, false);

    resumeModeWaitTimerRef = le_timer_Create("resumeModeWait");
    le_timer_SetMsInterval(resumeModeWaitTimerRef, MAX_INIT_TIMEOUT*4*1000);
    le_timer_SetHandler(resumeModeWaitTimerRef, ResumeHlapTimerModeWaitHandler);
    le_timer_SetRepeat(resumeModeWaitTimerRef, 1);
    le_timer_SetWakeup(resumeModeWaitTimerRef, false);
    lastCallPhoneId = GetLastCallPhoneId();
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

    uint16_t deRegTime = 0;
    bool needToResumeT10 = false;
    iteratorRef = le_cfg_CreateReadTxn( CFG_ECALL_HLAPTIMERELAPSED_PATH );
    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T10))
    {
        ElapsedTimeT10 = le_cfg_GetInt(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T10, 0);
        LE_INFO("ElapsedTimeT10 is %d when tafECallSvc is initiated", ElapsedTimeT10);
        if (LE_OK == GetNadDeregistrationTime(&deRegTime))
        {
            if (ElapsedTimeT10 < deRegTime*60)
        {
                needToResumeT10 = true;
        }
        } else {
            LE_ERROR("GetNadDeregistrationTime failed");
            }
        } else {
        LE_INFO("CFG_NODE_HLAPTIMERELAPSED_T10 node not exists");
        }
    le_cfg_CancelTxn(iteratorRef);
    if ((needToResumeT9 == true) || (needToResumeT10 == true))
    {
        ArmPendingResume(needToResumeT9, needToResumeT10);
    }
}

taf_ecall &taf_ecall::GetInstance()
{
    static taf_ecall instance;
    return instance;
}

bool taf_ecall::isIdle()
{
    std::vector<std::shared_ptr<taf_pa_ecall_CallInfo_t>> callList;
    pa_result_t callListRes = taf_pa_ecall_GetInProgressCalls(&callList);
    if (callListRes != PA_OK)
    {
        LE_ERROR("taf_pa_ecall_GetInProgressCalls failed: %d", (int)callListRes);
        return false;
    }

    for(auto itr = std::begin(callList); itr != std::end(callList); ++itr) {
        taf_pa_ecall_call_status_t callState = (*itr)->callState;
        if (callState != taf_pa_ecall_call_status_t::ENDED &&
                callState != taf_pa_ecall_call_status_t::IDLE) {
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

    taf_pa_ecall_config_t eCallConfig;
    eCallConfig.validityMask.set(OVERRIDDEN_NUM);
    eCallConfig.overriddenNum = psapNumber;
    eCallConfig.validityMask.set(NUM_TYPE);
    eCallConfig.numtype = taf_pa_ecall_num_type_t::OVERRIDDEN;
    return taf_pa_ecall_SetConfig(eCallConfig) == PA_OK ? LE_OK : LE_FAULT;
}

le_result_t taf_ecall::GetPsapNumber(char* psapNumber, size_t psapNumLength)
{
    TAF_ERROR_IF_RET_VAL(psapNumber == NULL, LE_BAD_PARAMETER, "PsapNumber is NULL");

    taf_pa_ecall_config_t eCallConfig = {};
    pa_result_t res = taf_pa_ecall_GetConfig(eCallConfig);
    if (res == PA_OK && eCallConfig.validityMask.test(OVERRIDDEN_NUM)) {
        LE_INFO("PSAP number retrieved as: %s", eCallConfig.overriddenNum.c_str());
        le_utf8_Copy(psapNumber, eCallConfig.overriddenNum.c_str(), psapNumLength, NULL);
    } else {
        LE_ERROR("Unable to get PSAP number. Error: %d", (int) res);
    }

    return res== PA_OK ? LE_OK : LE_FAULT;
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
    if (phoneId < MIN_PHONE_ID || phoneId > MAX_PHONE_ID) {
        LE_ERROR("Invalid phoneId: %d, must be %d~%d",
                 phoneId, MIN_PHONE_ID, MAX_PHONE_ID);
        return LE_BAD_PARAMETER;
    }

    if(eCallMode == TAF_ECALL_MODE_NORMAL  || eCallMode == TAF_ECALL_MODE_ECALL) {
        auto promisePtr = std::make_shared<std::promise<le_result_t>>();
        std::future<le_result_t> futResult = promisePtr->get_future();
        auto cb = [promisePtr](pa_result_t errorCode,std::any context)
        {
            try
            {
                if (errorCode == PA_OK)
                {
                    LE_INFO("Set eCall operating mode successfully done");
                    promisePtr->set_value(LE_OK);
                }
                else
                {
                    LE_INFO("Set eCall operating mode failed, errorCode: %d", static_cast<int>(errorCode));
                    promisePtr->set_value(LE_FAULT);
                }
            }
            catch (const std::future_error& e)
            {
                LE_ERROR("Future error in callback: %s", e.what());
            }
            catch (const std::exception& e)
            {
                LE_ERROR("Exception in callback: %s", e.what());
                try { promisePtr->set_value(LE_FAULT); } catch (...) {}
            }
            catch (...)
            {
                LE_ERROR("Unknown error in callback.");
                try { promisePtr->set_value(LE_FAULT); } catch (...) {}
            }
        };
        pa_result_t result = taf_pa_ecall_SetOpMode(phoneId,
                static_cast<taf_pa_ecall_mode_t>(eCallMode), cb,{});
        if (WaitResult(result, futResult, "SetECallOperatingMode")) {
            LE_INFO("Set eCall operating mode %d successfully in phoneId: %d",
                    (int) eCallMode, phoneId);
                return LE_OK;
        } else {
            LE_ERROR("Set eCall operating mode %d failed in phoneId: %d\n", (int) eCallMode, phoneId);
        }
    } else {
        LE_ERROR("Invalid input op mode: %d phoneId: %d\n", (int) eCallMode, phoneId);
    }
    return LE_FAULT;
}

le_result_t taf_ecall::GetECallOperatingMode(uint8_t phoneId, taf_ecall_OpMode_t *opMode) {
    TAF_ERROR_IF_RET_VAL(opMode == NULL, LE_BAD_PARAMETER, "OpMode is NULL");

    if (phoneId < MIN_PHONE_ID || phoneId > MAX_PHONE_ID) {
        LE_ERROR("Invalid phoneId: %d, must be %d~%d",
                 phoneId, MIN_PHONE_ID, MAX_PHONE_ID);
        return LE_BAD_PARAMETER;
    }

    auto promisePtr = std::make_shared<std::promise<le_result_t>>();
    auto resultMode  = std::make_shared<taf_pa_ecall_mode_t>(taf_pa_ecall_mode_t::NORMAL);
    std::future<le_result_t> futResult = promisePtr->get_future();
    auto cb = [promisePtr, resultMode](taf_pa_ecall_mode_t mode, pa_result_t errorCode,
    std::any context)
    {
        try
        {
            if(errorCode == PA_OK)
            {
                *resultMode = mode;
                promisePtr->set_value(LE_OK);
            }
            else
            {
                LE_ERROR("GetECallOperatingMode callback failed errorCode: %d", int(errorCode));
                promisePtr->set_value(LE_FAULT);
            }
        }
        catch (const std::future_error& e)
        {
            LE_ERROR("Future error in callback: %s", e.what());
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in callback: %s", e.what());
            try { promisePtr->set_value(LE_FAULT); } catch (...) {}
        }
        catch (...)
        {
            LE_ERROR("Unknown error in callback.");
            try { promisePtr->set_value(LE_FAULT); } catch (...) {}
        }
    };
    pa_result_t result = taf_pa_ecall_GetOpMode(phoneId,cb,{});
    if (WaitResult(result, futResult, "GetECallOperatingMode")) {
        LE_INFO("Get eCall op mode successfully in phoneId: %d", phoneId);
            if (taf_pa_ecall_mode_t::NORMAL == *resultMode)
            {
                *opMode = TAF_ECALL_MODE_NORMAL;
                return LE_OK;
            } else if (taf_pa_ecall_mode_t::ONLY == *resultMode) {
                *opMode = TAF_ECALL_MODE_ECALL;
                return LE_OK;
            } else {
                LE_ERROR("Invalid mode");
        }
    } else {
        LE_ERROR("Get eCall Operating mode request failed in phoneId: %d\n", phoneId);
    }
    return LE_FAULT;
}

le_result_t taf_ecall::StartECall(taf_pa_ecall_category_t emergencyCategory,
                taf_pa_ecall_type_t eCallVariant, taf_ecall_CallRef_t ecallRef) {

    //From reference read ecall ptr object
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    //Get Selected card
    int8_t phoneId = -1;
    pa_result_t phoneIdRes = taf_pa_ecall_GetPhoneIdFromSlotId(
        static_cast<int8_t>(taf_sim_GetSelectedCard()), &phoneId);
    if (phoneIdRes != PA_OK)
    {
        LE_ERROR("taf_pa_ecall_GetPhoneIdFromSlotId failed: %d", (int)phoneIdRes);
    }

    //Check ECall session
    if (eCallPtr->eCallSession != ECALL_INIT && (eCallPtr->eCallSession != ECALL_ENDED)) {
        LE_ERROR("Already ecall in progress");
        return LE_BUSY;
    }
    auto makeEcallPromPtr = std::make_shared<std::promise<le_result_t>>();
    std::future<le_result_t> makeEcallFut = makeEcallPromPtr->get_future();

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

    taf_pa_ecall_config_t eCallConfig = {};
    pa_result_t ret = taf_pa_ecall_GetConfig(eCallConfig);
    if (ret == PA_OK) {
        LE_INFO("Get eCall configuration successfully.");
    }

    if (eCallVariant == taf_pa_ecall_type_t::TEST)
    {
        ECallObject.msd.control.automaticActivation = false;
        ECallObject.msd.control.testCall = true;
    }
    else if (( emergencyCategory == taf_pa_ecall_category_t::AUTO) ||
             ( emergencyCategory == taf_pa_ecall_category_t::MANUAL))
    {
        ECallObject.msd.control.testCall = false;
        if (eCallConfig.overriddenNum.empty())
        {
            eCallConfig.validityMask.set(OVERRIDDEN_NUM);
            eCallConfig.overriddenNum = "112";
            eCallConfig.validityMask.set(NUM_TYPE);
            eCallConfig.numtype = taf_pa_ecall_num_type_t::DEFAULT;
            ret = taf_pa_ecall_SetConfig(eCallConfig);
            if (ret == PA_OK)
            {
                LE_INFO("Set eCall configuration with number 112 successfully");
            }
        }
        else if ((0 == strncmp(eCallConfig.overriddenNum.c_str(), "112", 3)) ||
                 (0 == strncmp(eCallConfig.overriddenNum.c_str(), "911", 3)) ||
                 (0 == strncmp(eCallConfig.overriddenNum.c_str(), "999", 3)))
        {
            if (eCallConfig.numtype != taf_pa_ecall_num_type_t::DEFAULT)
            {
                eCallConfig.validityMask.set(NUM_TYPE);
                eCallConfig.numtype = taf_pa_ecall_num_type_t::DEFAULT;
                ret = taf_pa_ecall_SetConfig(eCallConfig);
                if (ret == PA_OK)
                {
                    LE_INFO("Set eCall configuration with default number type successfully");
                }
            }
        }

        if ( emergencyCategory == taf_pa_ecall_category_t::AUTO)
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

        ret =   taf_pa_ecall_MakeECall(phoneId, eCallMsdData, emergencyCategory,
                eCallVariant, tafCallCommandCallback::makeECallResponse, makeEcallPromPtr);
    }
    else
    {
        if (LE_OK != UpdateMsdInformation(ecallRef))
        {
            LE_ERROR("Unable to update the msd information via VHAL");
        }
        eCallPtr->msd.messageIdentifier = 1;
        WriteMsdMsgIdToConfigTree(eCallPtr->msd.messageIdentifier);
        taf_pa_ecall_msd_data_t eCallMsdData = (taf_pa_ecall_msd_data_t) eCallPtr->msd;


        ret = taf_pa_ecall_MakeECall(phoneId, eCallMsdData, emergencyCategory,
                eCallVariant, tafCallCommandCallback::makeECallResponse, makeEcallPromPtr);
    }

    if (WaitResult(ret, makeEcallFut, "StartECall"))
    {
        LE_DEBUG("Start ECall request sent successfully");
        SetLastCallPhoneId(phoneId);
        ECallObject.eCallSession = ECALL_REQUEST;
        if (eCallVariant == taf_pa_ecall_type_t::TEST)
        {
            ECallObject.type = TAF_ECALL_TYPE_TEST;
        }
        else if ( emergencyCategory == taf_pa_ecall_category_t::AUTO)
        {
            ECallObject.type = TAF_ECALL_TYPE_AUTO;
        }
        else if ( emergencyCategory == taf_pa_ecall_category_t::MANUAL)
        {
            ECallObject.type = TAF_ECALL_TYPE_MANUAL;
        }
        if (false == ECallObject.isMsdUpdated)
        {
            memset(eCallPtr->msdPdu, 0, TAF_ECALL_MAX_MSD_LENGTH);
            if (LE_OK != RetrieveEncodedMsdPdu((taf_pa_ecall_msd_data_t) eCallPtr->msd, eCallPtr->msdPdu, &(eCallPtr->pduMsdSize)))
            {
                return LE_FAULT;
            }
        }
        return LE_OK;
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
    int8_t phoneId = -1;
    pa_result_t phoneIdRes = taf_pa_ecall_GetPhoneIdFromSlotId(
        static_cast<int8_t>(taf_sim_GetSelectedCard()), &phoneId);
    if (phoneIdRes != PA_OK)
    {
        LE_ERROR("taf_pa_ecall_GetPhoneIdFromSlotId failed: %d", (int)phoneIdRes);
    }

    //Check ECall session
    if (eCallPtr->eCallSession != ECALL_INIT && (eCallPtr->eCallSession != ECALL_ENDED)) {
        LE_ERROR("Already ecall in progress");
        return LE_BUSY;
    }

    taf_pa_ecall_custom_sip_header_t header;
    if (contentType != NULL){
        header.contentType = contentType;
        LE_INFO("Set content type as %s", contentType);
    }

    if (acceptInfo != NULL) {
        header.acceptInfo = acceptInfo;
        LE_INFO("Set accept info as %s", acceptInfo);
    } else {
        header.acceptInfo = "";
    }

    pa_result_t ret;
    taf_pa_ecall_config_t eCallConfig = {};
    ret = taf_pa_ecall_GetConfig(eCallConfig);
    if (ret == PA_OK) {
        LE_INFO("get eCall configuration successfully.");
    }

    //Check msd imported or not to send msd in pdu format or not
    if (ECallObject.isMsdUpdated)
    {
        LE_INFO("MSD updated.");
        auto prieCallPromPtr = std::make_shared<std::promise<le_result_t>>();
        std::future<le_result_t> prieCallFut = prieCallPromPtr->get_future();
        std::vector< uint8_t > eCallMsdData = {};
        for (int i = 0; i < (int)(eCallPtr->pduMsdSize); i++)
        {
            eCallMsdData.push_back(eCallPtr->msdPdu[i]);
        }
        ret = taf_pa_ecall_MakeECall(phoneId, psapNumber, header, eCallMsdData,
                tafPrieCallCommandCallback::makeECallResponse, prieCallPromPtr);
        if (WaitResult(ret, prieCallFut, "StartPrivate"))
        {
            LE_DEBUG("Start private eCall request sent successfully");
            ECallObject.eCallSession = ECALL_REQUEST;
            ECallObject.isPrieCallOngoing = true;
            ECallObject.type = TAF_ECALL_TYPE_PRIVATE;
            return LE_OK;
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

    std::shared_ptr<taf_pa_ecall_CallInfo_t> iCall = eCallPtr->iCall;
    TAF_ERROR_IF_RET_VAL(iCall == nullptr, LE_NOT_FOUND, "iCall is null on eCallPtr(%p)", eCallPtr);

    ClearPduMsd();

    if(iCall->callState == taf_pa_ecall_call_status_t::INCOMING)
    {
        auto rejectPromPtr = std::make_shared<std::promise<le_result_t>>();
        std::future<le_result_t> rejectFut = rejectPromPtr->get_future();
        pa_result_t result = taf_pa_ecall_Reject(*iCall, tafRejectCommandCallback::commandResponse,
                                                 rejectPromPtr);
        if (WaitResult(result, rejectFut, "StopECall(Reject)")) {
                return LE_OK;
        }
    } else {
        auto hangupPromPtr = std::make_shared<std::promise<le_result_t>>();
        std::future<le_result_t> hangupFut = hangupPromPtr->get_future();
        pa_result_t result = taf_pa_ecall_Hangup(*iCall, tafHangupCommandCallback::commandResponse,
                                                 hangupPromPtr);
        if (WaitResult(result, hangupFut, "StopECall(Hangup)")) {
                return LE_OK;
        }
    }
    return LE_FAULT;
}

le_result_t taf_ecall::AnswerECall(taf_ecall_CallRef_t ecallRef)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);
    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");

    std::shared_ptr<taf_pa_ecall_CallInfo_t> iCall = eCallPtr->iCall;
    TAF_ERROR_IF_RET_VAL(iCall == nullptr, LE_NOT_FOUND, "iCall is null on eCallPtr(%p)", eCallPtr);

    auto answerPromPtr = std::make_shared<std::promise<le_result_t>>();
    std::future<le_result_t> answerFut = answerPromPtr->get_future();
    pa_result_t result = taf_pa_ecall_Answer(*iCall, tafAnswerCommandCallback::commandResponse,
                                             answerPromPtr);
    if (WaitResult(result, answerFut, "AnswerECall")) {
            return LE_OK;
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

    eCallPtr->msd.optionalData.recentVehicleLocationN1Present = true;
    eCallPtr->msd.recentVehicleLocationN1.positionLatitude = latitudeDeltaN1;
    eCallPtr->msd.recentVehicleLocationN1.positionLongitude = longitudeDeltaN1;

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

    eCallPtr->msd.optionalData.recentVehicleLocationN2Present = true;
    eCallPtr->msd.recentVehicleLocationN2.positionLatitude = latitudeDeltaN2;
    eCallPtr->msd.recentVehicleLocationN2.positionLongitude = longitudeDeltaN2;

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

    eCallPtr->msd.optionalData.numberOfPassengersPresent = true;
    eCallPtr->msd.numberOfPassengers = passengerCount;

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

        ECallObject.msd.vehicleIdentification.isowmi = vinStr.substr(ISOWMI_START, ISOWMI_LENGTH );
        ECallObject.msd.vehicleIdentification.isovds = vinStr.substr(ISOVDS_START, ISOVDS_LENGTH);
        ECallObject.msd.vehicleIdentification.isovisModelyear =
                             vinStr.substr(ISOVIS_MODEL_YEAR_START, ISOVIS_MODEL_YEAR_LENGTH);
        ECallObject.msd.vehicleIdentification.isovisSeqPlant =
                             vinStr.substr(ISOVIS_SEQ_PLANT_START, ISOVIS_SEQ_PLANT_LENGTH);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_MSDVEHTYPE))
    {
        ECallObject.msd.control.vehicleType =
            (taf_pa_ecall_vehicle_type_t) le_cfg_GetInt(iteratorRef, CFG_NODE_MSDVEHTYPE, 0);
    }

    le_cfg_CancelTxn(iteratorRef);

    iteratorRef = le_cfg_CreateReadTxn( CFG_ECALL_PROPULSIONTYPE_PATH);

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_GASOLINE))
    {
        ECallObject.msd.propulsionType.gasolineTankPresent =
            le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_GASOLINE, false);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_DIESEL))
    {
        ECallObject.msd.propulsionType.dieselTankPresent =
            le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_DIESEL, false);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_NATURALGAS))
    {
        ECallObject.msd.propulsionType.compressedNaturalGas =
            le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_NATURALGAS, false);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_PROPANE))
    {
        ECallObject.msd.propulsionType.liquidPropaneGas =
            le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_PROPANE, false);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_ELECTRIC))
    {
        ECallObject.msd.propulsionType.electricEnergyStorage =
            le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_ELECTRIC, false);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_HYDROGEN))
    {
        ECallObject.msd.propulsionType.hydrogenStorage =
            le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_HYDROGEN, false);
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_OTHER))
    {
        ECallObject.msd.propulsionType.otherStorage =
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
    eCallPtr->msd.optionalData.isMsdOptionalDataPresent = true;
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
    eCallPtr->msd.optionalData.isMsdOptionalDataPresent = false;
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

    if (!isIdle()) {
        LE_INFO("ECall session is in progress, try it later when session is not active");
        return LE_BUSY;
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

    if (!isIdle()) {
        LE_INFO("ECall session is in progress, try it later when session is not active");
        return LE_BUSY;
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

    pa_result_t result;
    int8_t phoneId = -1;
    pa_result_t phoneIdRes = taf_pa_ecall_GetPhoneIdFromSlotId(
        static_cast<int8_t>(taf_sim_GetSelectedCard()), &phoneId);
    if (phoneIdRes != PA_OK)
    {
        LE_ERROR("taf_pa_ecall_GetPhoneIdFromSlotId failed: %d", (int)phoneIdRes);
    }

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
        auto promisePtr = std::make_shared<std::promise<le_result_t>>();
        auto cb = [promisePtr](pa_result_t error,std::any context)
        {
            try
            {
                if (error == PA_OK)
                {
                    LE_INFO("Send eCall MSD successfully done");
                    promisePtr->set_value(LE_OK);
                }
                else
                {
                    LE_INFO("Send eCall MSD failed, errorCode: %d", static_cast<int>(error));
                    promisePtr->set_value(LE_FAULT);
            }
            }
            catch (const std::future_error& e)
            {
                LE_ERROR("Future error in callback: %s", e.what());
            }
            catch (const std::exception& e)
            {
                LE_ERROR("Exception in callback: %s", e.what());
                try { promisePtr->set_value(LE_FAULT); } catch (...) {}
            }
            catch (...)
            {
                LE_ERROR("Unknown error in callback.");
                try { promisePtr->set_value(LE_FAULT); } catch (...) {}
            }
        };

        std::future<le_result_t> futResult = promisePtr->get_future();
        result = taf_pa_ecall_UpdateMsd(phoneId, eCallMsdData, cb,{});
        if (WaitResult(result, futResult, "SendMsd(pdu)")) {
            {
                LE_INFO("Send eCall MSD successfully done");
                return LE_OK;
            }
        } else {
            LE_ERROR("Send eCall MSD failed");
        }
    }
    else
    {
        if (LE_OK != UpdateMsdInformation(ecallRef))
        {
            LE_ERROR("Unable to update the msd information via VHAL");
        }

        auto updateMsdPromPtr = std::make_shared<std::promise<le_result_t>>();
        std::future<le_result_t> updateMsdFut = updateMsdPromPtr->get_future();
        pa_result_t status = taf_pa_ecall_UpdateMsd(phoneId, eCallPtr->msd,
                                                    tafUpdateMsdCommandCallback::commandResponse,
                                                    updateMsdPromPtr);
        if (WaitResult(status, updateMsdFut, "SendMsd(UpdateMsd)")) {
            {
                memset(eCallPtr->msdPdu, 0, sizeof(eCallPtr->msdPdu));
                if (LE_OK == RetrieveEncodedMsdPdu((taf_pa_ecall_msd_data_t) eCallPtr->msd, eCallPtr->msdPdu, &(eCallPtr->pduMsdSize)))
                {
                    return LE_OK;
                }
            }
        }
    }
    return LE_FAULT;
}

le_result_t taf_ecall::RetrieveEncodedMsdPdu(taf_pa_ecall_msd_data_t eCallMsdData, uint8_t* pduMsd, size_t *msdLength)
{
    std::vector<uint8_t> eCallMsdPdu = {};
    pa_result_t result = taf_pa_ecall_EncodeMsd(eCallMsdData, eCallMsdPdu);
    if (result == PA_OK)
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
         LE_ERROR("Failed to retrieve the encoded eCall MSD PDU with error code: %d", (static_cast<int>(result)));
    }

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

void taf_ecall::SetStateAndReport(taf_ecall_State_t state, int phoneId, const std::string &dest)
{
    ECallObject.state = state;
    StateChangeEvent_t stateEvent = { 0 };
    le_utf8_Copy(stateEvent.dest, dest.c_str(), MAX_DESTINATION_LEN, NULL);
    stateEvent.eCallRef = GetECallReference();
    stateEvent.state = state;
    stateEvent.phoneId = phoneId;
    le_event_Report(StateChangeEventId, &stateEvent, sizeof(StateChangeEvent_t));
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
    TAF_ERROR_IF_RET_VAL(ECALL_ENDED != eCallPtr->eCallSession,
            TAF_ECALL_REASON_NORMAL_UNSPECIFIED, "The eCall is not ENDed");

    auto &eCall = taf_ecall::GetInstance();
    // Map CDMA specific call end causes
    if (eCall.CallEndError >= taf_pa_ecall_termination_t::CDMA_LOCKED_UNTIL_POWER_CYCLE &&
            eCall.CallEndError <= taf_pa_ecall_termination_t::CDMA_ACCESS_BLOCKED) {
        eCall.CallEndError = taf_pa_ecall_termination_t::NORMAL_UNSPECIFIED;
    }
    LE_INFO("GetTerminationReason call end error = %d", (int) eCall.CallEndError);
    return (taf_ecall_TerminationReason_t) eCall.CallEndError;
}

taf_ecall_TerminationRedialReason_t taf_ecall::MapRedialReason(taf_pa_ecall_reason_type_t redialReason)
{
    switch (redialReason)
    {
        case taf_pa_ecall_reason_type_t::NONE:
            return TAF_ECALL_TERMINATION_REDIAL_REASON_NONE;
        case taf_pa_ecall_reason_type_t::ORIG_FAILURE:
            return TAF_ECALL_TERMINATION_REDIAL_REASON_CALL_ORIG_FAILURE;
        case taf_pa_ecall_reason_type_t::DROP:
            return TAF_ECALL_TERMINATION_REDIAL_REASON_CALL_DROP;
        case taf_pa_ecall_reason_type_t::MAX_REDIAL_ATTEMPTED:
            return TAF_ECALL_TERMINATION_REDIAL_REASON_MAX_REDIAL_ATTEMPTED;
        case taf_pa_ecall_reason_type_t::CONNECTED:
            return TAF_ECALL_TERMINATION_REDIAL_REASON_CALL_CONNECTED;
        default:
            return TAF_ECALL_TERMINATION_REDIAL_REASON_NONE;
    }
}
le_result_t taf_ecall::GetTerminationRedialReason( taf_ecall_CallRef_t ecallRef, taf_ecall_TerminationRedialReason_t* reason)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);
    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, LE_BAD_PARAMETER, "Invalid eCall reference");
    TAF_ERROR_IF_RET_VAL(reason == NULL, LE_BAD_PARAMETER, "Invalid parameter");
    TAF_ERROR_IF_RET_VAL(ECALL_ENDED != eCallPtr->eCallSession,
            LE_FAULT, "The eCall is not ENDed");
    *reason = eCallPtr->redialReason;
    return LE_OK;
}
taf_ecall_Type_t taf_ecall::GetType ( taf_ecall_CallRef_t ecallRef)
{
    taf_ECall_t* eCallPtr = (taf_ECall_t*)le_ref_Lookup(ECallPtrRefMap, ecallRef);

    TAF_KILL_CLIENT_IF_RET_VAL(eCallPtr == NULL, TAF_ECALL_TYPE_UNKNOWN, "Invalid eCall reference");

    return eCallPtr->type;
}

le_result_t taf_ecall::UseUSimNumbers()
{
    taf_pa_ecall_config_t eCallConfig;
    eCallConfig.validityMask.set(NUM_TYPE);
    eCallConfig.numtype = taf_pa_ecall_num_type_t::DEFAULT;
    pa_result_t result = taf_pa_ecall_SetConfig(eCallConfig);
    LE_INFO("UseUSimNumbers: status %d", (int) result);

    return result == PA_OK ? LE_OK : LE_FAULT;
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
    LE_INFO("Set eCall NAD deregistration time (in minutes): %d", t10);

    int8_t phoneId = -1;
    pa_result_t phoneIdRes = taf_pa_ecall_GetPhoneIdFromSlotId(
        static_cast<int8_t>(taf_sim_GetSelectedCard()), &phoneId);
    if (phoneIdRes != PA_OK)
    {
        LE_ERROR("taf_pa_ecall_GetPhoneIdFromSlotId failed: %d", (int)phoneIdRes);
    }

    auto promisePtr = std::make_shared<std::promise<le_result_t>>();
    auto cb = [promisePtr](pa_result_t error,std::any context)
    {
        try
        {
            if (error == PA_OK)
            {
                LE_INFO("Set eCall NAD deregistration time successfully done");
                promisePtr->set_value(LE_OK);
            }
            else
            {
                LE_INFO("Send eCall NAD deregistration time failed, errorCode: %d", static_cast<int>(error));
                promisePtr->set_value(LE_FAULT);
            }
        }
        catch (const std::future_error& e)
        {
            LE_ERROR("Future error in callback: %s", e.what());
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in callback: %s", e.what());
            try { promisePtr->set_value(LE_FAULT); } catch (...) {}
        }
        catch (...)
        {
            LE_ERROR("Unknown error in callback.");
            try { promisePtr->set_value(LE_FAULT); } catch (...) {}
        }
    };

    std::future<le_result_t> futResult = promisePtr->get_future();
    pa_result_t result = taf_pa_ecall_UpdateHlapTimer(phoneId, taf_pa_ecall_hlap_timer_type_t::T10, t10, cb,{});
    if (WaitResult(result, futResult, "SetNadDeregistrationTime")) {
        {
            LE_INFO("Set eCall NAD deregistration time successfully.");
            return LE_OK;
        }
    } else {
        LE_ERROR("eCall NAD deregistration time failed");
    }

    return LE_FAULT;
}

le_result_t taf_ecall::GetNadDeregistrationTime(uint16_t* deregTime)
{
    if (deregTime == NULL) {
        LE_ERROR("deregTime is null.");
        return LE_FAULT;
    }
    int8_t phoneId = -1;
    pa_result_t phoneIdRes = taf_pa_ecall_GetPhoneIdFromSlotId(
        static_cast<int8_t>(taf_sim_GetSelectedCard()), &phoneId);
    if (phoneIdRes != PA_OK)
    {
        LE_ERROR("taf_pa_ecall_GetPhoneIdFromSlotId failed: %d", (int)phoneIdRes);
    }

    auto promisePtr = std::make_shared<std::promise<le_result_t>>();
    auto resultTime = std::make_shared<uint32_t>(0);
    std::future<le_result_t> futResult = promisePtr->get_future();
    auto cb = [promisePtr, resultTime](pa_result_t error, uint32_t timeDuration, std::any context)
    {
        try
        {
            if(error == PA_OK)
            {
                LE_INFO("Get NAD deregistration time (T10 in minutes) fetched as: %d", timeDuration);
                *resultTime = timeDuration;
                promisePtr->set_value(LE_OK);
            }
            else
            {
                LE_ERROR("Get eCall hlap timer status failed errorCode: %d ", int(error));
                promisePtr->set_value(LE_FAULT);
            }
        }
        catch (const std::future_error& e)
        {
            LE_ERROR("Future error in callback: %s", e.what());
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in callback: %s", e.what());
            try { promisePtr->set_value(LE_FAULT); } catch (...) {}
        }
        catch (...)
        {
            LE_ERROR("Unknown error in callback.");
            try { promisePtr->set_value(LE_FAULT); } catch (...) {}
        }
    };

    pa_result_t result = taf_pa_ecall_RequestHlapTimer(phoneId, taf_pa_ecall_hlap_timer_type_t::T10, cb,{});

    if (WaitResult(result, futResult, "GetNadDeregistrationTime")) {
        {
            *deregTime = (uint16_t)(*resultTime);
            return LE_OK;
        }
    } else {
        LE_ERROR("GetNadDeregistrationTime: status %d", (int) result);
    }

    return LE_FAULT;
}

le_result_t taf_ecall::TerminateRegistration()
{
    int8_t phoneId = -1;
    pa_result_t phoneIdRes = taf_pa_ecall_GetPhoneIdFromSlotId(
        static_cast<int8_t>(taf_sim_GetSelectedCard()), &phoneId);
    if (phoneIdRes != PA_OK)
    {
        LE_ERROR("taf_pa_ecall_GetPhoneIdFromSlotId failed: %d", (int)phoneIdRes);
    }

    auto promisePtr = std::make_shared<std::promise<le_result_t>>();
    auto cb = [promisePtr](pa_result_t error,std::any context)
    {
        try
        {
            if (error == PA_OK)
            {
                LE_INFO("Terminate registration successfully done");
                promisePtr->set_value(LE_OK);
            }
            else
            {
                LE_INFO("Terminate registration failed, errorCode: %d", static_cast<int>(error));
                promisePtr->set_value(LE_FAULT);
            }
        }
        catch (const std::future_error& e)
        {
            LE_ERROR("Future error in callback: %s", e.what());
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in callback: %s", e.what());
            try { promisePtr->set_value(LE_FAULT); } catch (...) {}
        }
        catch (...)
        {
            LE_ERROR("Unknown error in callback.");
            try { promisePtr->set_value(LE_FAULT); } catch (...) {}
        }
    };

    std::future<le_result_t> futResult = promisePtr->get_future();
    pa_result_t result = taf_pa_ecall_RequestNetworkDeregistration(phoneId, cb,{});
    if (WaitResult(result, futResult, "TerminateRegistration")) {
        {
            LE_INFO("Terminate registration successfully.");
            return LE_OK;
        }
    } else {
        LE_ERROR("Terminate registration failed");
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

    taf_pa_ecall_config_t eCallConfig;
    eCallConfig.validityMask.set(T2_TIMER);
    eCallConfig.t2Timer = t2;
    pa_result_t result = taf_pa_ecall_SetConfig(eCallConfig);

    return result == PA_OK ? LE_OK : LE_FAULT;

}

le_result_t taf_ecall::GetNadClearDownFallbackTime(uint16_t* ccftTime)
{
    if (ccftTime == NULL) {
        LE_ERROR("ccftTime is null.");
        return LE_FAULT;
    }

    taf_pa_ecall_config_t eCallConfig = {};
    pa_result_t result = taf_pa_ecall_GetConfig(eCallConfig);
    if (result == PA_OK && eCallConfig.validityMask.test(T2_TIMER)) {
        LE_INFO("NAD clear down fallback time (in minutes): %d", eCallConfig.t2Timer/60000);
        *ccftTime = (uint16_t) (eCallConfig.t2Timer/60000);
    } else {
        LE_ERROR("Unable to get clear down fallback time. Error: %d", (int) result);
    }

    return result == PA_OK ? LE_OK : LE_FAULT;
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

    taf_pa_ecall_config_t eCallConfig;
    eCallConfig.validityMask.set(T9_TIMER);
    eCallConfig.t9Timer = t9;
    pa_result_t res = taf_pa_ecall_SetConfig(eCallConfig);
    if (res != PA_OK)
    {
        LE_ERROR("Unable to set min network registration time. Error: %d", (int) res);
        return LE_FAULT;
    }

    uint16_t minNwRegTimeGetAfterSet = 0;
    result = GetNadMinNetworkRegistrationTime(&minNwRegTimeGetAfterSet);
    if ((result == LE_OK) &&
        (minNwRegTimeGetAfterSet != minNwRegTime))
    {
        LE_ERROR("Error: Setting min network registration time is not consistent with get.");
        t9 = (uint32_t) minNwRegTimeGet*60*1000;;
        eCallConfig.validityMask.set(T9_TIMER);
        eCallConfig.t9Timer = t9;
        res = taf_pa_ecall_SetConfig(eCallConfig);
        if (res != PA_OK)
        {
            LE_ERROR("Unable to set the previous min network registration time. Error: %d", (int) res);
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

    taf_pa_ecall_config_t eCallConfig = {};
    pa_result_t result = taf_pa_ecall_GetConfig(eCallConfig);
    if (result == PA_OK && eCallConfig.validityMask.test(T9_TIMER)) {
        LE_INFO("NAD min network registration time (in minutes): %d", eCallConfig.t9Timer/60000);
        *minNwRegTime = (uint16_t) (eCallConfig.t9Timer/60000);
    } else {
        LE_ERROR("Unable to get min network registration time. Error: %d", (int) result);
    }

    return result == PA_OK  ? LE_OK : LE_FAULT;
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
                    t10ElapsedTime = ConvertElapsedTime(t10StartTime) + ElapsedTimeT10;
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
    taf_ecall_HlapTimerStatus_t timerStatus = TAF_ECALL_TIMER_STATUS_UNKNOWN;
    int8_t phone_id = -1;
    pa_result_t phoneIdRes = taf_pa_ecall_GetPhoneIdFromSlotId(
        static_cast<int8_t>(taf_sim_GetSelectedCard()), &phone_id);
    if (phoneIdRes != PA_OK)
    {
        LE_ERROR("taf_pa_ecall_GetPhoneIdFromSlotId failed: %d", (int)phoneIdRes);
    }

    auto promisePtr = std::make_shared<std::promise<le_result_t>>();
    auto receivedTimerStatusPtr = std::make_shared<taf_pa_ecall_hlap_timer_status_t>();
    auto cb = [promisePtr, phone_id, receivedTimerStatusPtr](pa_result_t errorCode,
        int8_t phoneId,
        std::shared_ptr<const taf_pa_ecall_hlap_timer_status_t> hlapStatus,
        std::any context) {
        try
        {
            if((errorCode == PA_OK) && (phone_id == phoneId))
            {
                *receivedTimerStatusPtr = *hlapStatus;
                promisePtr->set_value(LE_OK);
            }
            else
            {
                LE_ERROR("Get eCall hlap timer status failed errorCode: %d ", int(errorCode));
                promisePtr->set_value(LE_FAULT);
            }
        }
        catch (const std::future_error& e)
        {
            LE_ERROR("Future error in callback: %s", e.what());
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in callback: %s", e.what());
            try { promisePtr->set_value(LE_FAULT); } catch (...) {}
        }
        catch (...)
        {
            LE_ERROR("Unknown error in callback.");
            try { promisePtr->set_value(LE_FAULT); } catch (...) {}
        }
    };

    std::future<le_result_t> futResult = promisePtr->get_future();
    pa_result_t result = taf_pa_ecall_RequestHlapTimerStatus(phone_id, cb,{});
    if (WaitResult(result, futResult, "GetHlapTimerState")) {
        LE_INFO("Get eCall hlap timer successfully.");

        {
            switch (timerType)
            {
                case TAF_ECALL_TIMER_TYPE_T2:
                    timerStatus = ConvertHlapTimerStatus(receivedTimerStatusPtr->t2);
                    break;
                case TAF_ECALL_TIMER_TYPE_T9:
                    timerStatus = ConvertHlapTimerStatus(receivedTimerStatusPtr->t9);
                    break;
                case TAF_ECALL_TIMER_TYPE_T10:
                    timerStatus = ConvertHlapTimerStatus(receivedTimerStatusPtr->t10);
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

taf_ecall_HlapTimerStatus_t taf_ecall::ConvertHlapTimerStatus(taf_pa_ecall_hlap_timer_state_t status) {
    switch(status) {
        case taf_pa_ecall_hlap_timer_state_t::INACTIVE:
            return TAF_ECALL_TIMER_STATUS_INACTIVE;
        case taf_pa_ecall_hlap_timer_state_t::ACTIVE:
            return TAF_ECALL_TIMER_STATUS_ACTIVE;
        case taf_pa_ecall_hlap_timer_state_t::UNKNOWN:
            return TAF_ECALL_TIMER_STATUS_UNKNOWN;
        default:
            return TAF_ECALL_TIMER_STATUS_UNKNOWN;
    }
}

uint16_t taf_ecall::ConvertElapsedTime(const timespec& start)
{
    timespec now{};
    uint16_t elapseTime = MAX_T9_T10_ELAPSED_TIME_SEC;

    if ((start.tv_nsec < 0) || (start.tv_nsec >= NSEC_PER_SEC)) {
        LE_ERROR("ConvertElapsedTime: invalid start.tv_nsec=%ld", start.tv_nsec);
        return elapseTime;
    }

    int ret = clock_gettime(CLOCK_BOOTTIME, &now);
    if (ret != 0) {
        LE_ERROR("ConvertElapsedTime: clock_gettime failed, errno=%d", errno);
        return elapseTime;
    }

    if ((now.tv_nsec < 0) || (now.tv_nsec >= NSEC_PER_SEC)) {
        LE_ERROR("ConvertElapsedTime: invalid now.tv_nsec=%ld", now.tv_nsec);
        return elapseTime;
    }

    long   diff_sec  = now.tv_sec  - start.tv_sec;
    long   diff_nsec = now.tv_nsec - start.tv_nsec;

    if (diff_nsec < 0) {
        if (diff_nsec < -NSEC_PER_SEC) {
            LE_ERROR("ConvertElapsedTime: diff_nsec too small=%ld", diff_nsec);
            return elapseTime;
        }

        diff_sec  -= 1;
        diff_nsec += NSEC_PER_SEC;
    }

    if (diff_sec < 0) {
        LE_ERROR("ConvertElapsedTime: start is in the future, diff_sec=%ld", diff_sec);
        return elapseTime;
    }

    uint64_t diff_sec_u = static_cast<uint64_t>(diff_sec);

    if (diff_sec_u > MAX_T9_T10_ELAPSED_TIME_SEC) {
        diff_sec_u = MAX_T9_T10_ELAPSED_TIME_SEC;
    }

    elapseTime = static_cast<uint16_t>(diff_sec_u);
    LE_DEBUG("ElapsedTime is %u when ConvertElapsedTime", elapseTime);
    return elapseTime;
}

void taf_ecall::T9TimerExpiryHandler(le_timer_Ref_t timerRef)
{
    auto &eCall = taf_ecall::GetInstance();
    taf_ecall_HlapTimerStatus_t timerStatus = TAF_ECALL_TIMER_STATUS_UNKNOWN;
    uint16_t elapsedTime = 0;
    uint16_t minNwRegTime = 0;
    bool minNwRegTimeValid = (LE_OK == eCall.GetNadMinNetworkRegistrationTime(&minNwRegTime));
    bool timerActive = false;
    if (minNwRegTimeValid && (eCall.ElapsedTimeT9 <= minNwRegTime*60))
    {
        timerActive = ((LE_OK == eCall.GetHlapTimerState(TAF_ECALL_TIMER_TYPE_T9, &timerStatus, &elapsedTime)) &&
                       (timerStatus == TAF_ECALL_TIMER_STATUS_ACTIVE));
    }
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_ECALL_HLAPTIMERELAPSED_PATH );

    if (minNwRegTimeValid)
    {
        if (eCall.ElapsedTimeT9 <= minNwRegTime*60)
        {
            if (timerActive)
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

void taf_ecall::T10TimerExpiryHandler(le_timer_Ref_t timerRef)
{
    auto &eCall = taf_ecall::GetInstance();
    taf_ecall_HlapTimerStatus_t timerStatus = TAF_ECALL_TIMER_STATUS_UNKNOWN;
    uint16_t elapsedTime = 0;
    uint16_t deRegTime = 0;
    bool deRegTimeValid = (LE_OK == eCall.GetNadDeregistrationTime(&deRegTime));
    bool timerActive = false;
    if (deRegTimeValid && (eCall.ElapsedTimeT10 <= deRegTime*60))
    {
        timerActive = ((LE_OK == eCall.GetHlapTimerState(TAF_ECALL_TIMER_TYPE_T10, &timerStatus, &elapsedTime)) &&
                       (timerStatus == TAF_ECALL_TIMER_STATUS_ACTIVE));
    }
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_ECALL_HLAPTIMERELAPSED_PATH );

    if (deRegTimeValid)
    {
        if (eCall.ElapsedTimeT10 <= deRegTime*60)
        {
            if (timerActive)

        {
                le_cfg_SetInt(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T10, elapsedTime);
            }
        } else {
            le_timer_Stop(eCall.elapsedTimeT10Ref);
            le_cfg_SetInt(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T10, deRegTime*60);
        }
    } else {
        le_cfg_SetInt(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T10, -1);
    }
    LE_INFO("T10 timer status: %d, elapsed timer: %d, configuration timer: %d", (int)timerStatus, elapsedTime, deRegTime);
    le_cfg_CommitTxn(iteratorRef);
}
void taf_ecall::SetLastCallPhoneId(int8_t phoneId)
        {
    lastCallPhoneId = phoneId;
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_MODEMSERVICE_ECALL_PATH );
    le_cfg_SetInt(iteratorRef, CFG_NODE_LASTECALL_PHONEID, phoneId);
    le_cfg_CommitTxn(iteratorRef);
}

int8_t taf_ecall::GetLastCallPhoneId()
{
    int8_t phoneId = -1;
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn( CFG_MODEMSERVICE_ECALL_PATH );

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_LASTECALL_PHONEID))
            {
        phoneId = le_cfg_GetInt(iteratorRef, CFG_NODE_LASTECALL_PHONEID, -1);
    }
    le_cfg_CancelTxn(iteratorRef);
    return phoneId;
}

void taf_ecall::ResumeHlapTimers(bool needResumeT9, bool needResumeT10, taf_ecall_OpMode_t opMode)
{
    le_result_t result = LE_FAULT;

    LE_DEBUG("ResumeHlapTimers phoneId %d, opMode %d, T9 %d, T10 %d",
        lastCallPhoneId, (int)opMode, needResumeT9, needResumeT10);


    if (needResumeT9)
    {
        result = ResumeHlapTimer(TAF_ECALL_TIMER_TYPE_T9);
        if (result != LE_OK) {
            LE_CRIT("ResumeECallHlapTimer T9 failed");
            needReportT9Start = false;
        }
    }

    if ((opMode == TAF_ECALL_MODE_ECALL) && needResumeT10) {
        result = ResumeHlapTimer(TAF_ECALL_TIMER_TYPE_T10);
        if (result != LE_OK) {
            LE_CRIT("ResumeECallHlapTimer T10 failed");
            needReportT10Start = false;
        }
    }
}
void taf_ecall::ArmPendingResume(bool needResumeT9, bool needResumeT10)
{
    if (!needResumeT9 && !needResumeT10)
    {
        pendingResumeT9 = false;
        pendingResumeT10 = false;
        return;
    }

    taf_ecall_OpMode_t opMode = TAF_ECALL_MODE_NORMAL;
    if (LE_OK == GetECallOperatingMode(lastCallPhoneId, &opMode))
    {
        pendingResumeT9 = false;
        pendingResumeT10 = false;
        ResumeHlapTimers(needResumeT9, needResumeT10, opMode);
        return;
    }

    pendingResumeT9 = needResumeT9;
    pendingResumeT10 = needResumeT10;
    le_timer_Restart(resumeModeWaitTimerRef);
}
void taf_ecall::ResumeHlapTimerModeWaitHandler(le_timer_Ref_t timerRef)
{
    auto &eCall = taf_ecall::GetInstance();
    if (!eCall.pendingResumeT9 && !eCall.pendingResumeT10)
    {
        return;
    }
    LE_INFO("Operating mode change not received in time, resume via one-shot query");
    bool needResumeT9 = eCall.pendingResumeT9;
    bool needResumeT10 = eCall.pendingResumeT10;
    eCall.pendingResumeT9 = false;
    eCall.pendingResumeT10 = false;
    taf_ecall_OpMode_t opMode = TAF_ECALL_MODE_NORMAL;
    if (LE_OK != eCall.GetECallOperatingMode(eCall.lastCallPhoneId, &opMode))
    {
        LE_CRIT("ResumeECallHlapTimer failed: cannot get operating mode on fallback");
        eCall.needReportT9Start = false;
        eCall.needReportT10Start = false;
        return;
    }
    eCall.ResumeHlapTimers(needResumeT9, needResumeT10, opMode);
}
void taf_ecall::OnEventResumeHlapTimerOnModeChange(taf_pa_ecall_mode_t eCallMode)
{
    if (!pendingResumeT9 && !pendingResumeT10)
    {
        return;
    }
    le_timer_Stop(resumeModeWaitTimerRef);

    taf_ecall_OpMode_t opMode =
        (eCallMode == taf_pa_ecall_mode_t::ONLY) ? TAF_ECALL_MODE_ECALL : TAF_ECALL_MODE_NORMAL;
    bool needResumeT9 = pendingResumeT9;
    bool needResumeT10 = pendingResumeT10;
    pendingResumeT9 = false;
    pendingResumeT10 = false;

    ResumeHlapTimers(needResumeT9, needResumeT10, opMode);
}

HlapTimerEventType_t taf_ecall::ConvertHlapTimerEvent(taf_pa_ecall_hlap_event_t event) {
    switch (event) {
        case taf_pa_ecall_hlap_event_t::STARTED:
            return HLAP_TIMER_EVENT_TYPE_STARTED;
        case taf_pa_ecall_hlap_event_t::STOPPED:
            return HLAP_TIMER_EVENT_TYPE_STOPPED;
        case taf_pa_ecall_hlap_event_t::EXPIRED:
            return HLAP_TIMER_EVENT_TYPE_EXPIRED;
        case taf_pa_ecall_hlap_event_t::RESUMED:
            return HLAP_TIMER_EVENT_TYPE_RESUMED;
        default:
            return HLAP_TIMER_EVENT_TYPE_UNKNOWN;
    }
}

le_result_t taf_ecall::ResumeHlapTimer(taf_ecall_HlapTimerType_t timerType) {
    int phoneId = lastCallPhoneId;
    taf_pa_ecall_hlap_timer_id_t timerId = taf_pa_ecall_hlap_timer_id_t::UNKNOWN;
    uint16_t minNwRegTime = 0;
    uint16_t deRegTime = 0;
    int duration = 0;

    if (timerType == TAF_ECALL_TIMER_TYPE_T9)
    {
        if (LE_OK == GetNadMinNetworkRegistrationTime(&minNwRegTime))
        {
            duration = minNwRegTime*60 - ElapsedTimeT9;
            timerId = taf_pa_ecall_hlap_timer_id_t::T9;
            LE_DEBUG("RestartHlapTimer duration = %d, %d", minNwRegTime, ElapsedTimeT9);
        } else {
            LE_ERROR("GetNadMinNetworkRegistrationTime error.");
            return LE_FAULT;
        }
    } else if (timerType == TAF_ECALL_TIMER_TYPE_T10) {
        if (LE_OK == GetNadDeregistrationTime(&deRegTime))
        {
            duration = deRegTime*60 - ElapsedTimeT10;
            timerId = taf_pa_ecall_hlap_timer_id_t::T10;
            LE_DEBUG("RestartHlapTimer duration = %d, %d", deRegTime, ElapsedTimeT10);
        } else {
            LE_ERROR("GetNadDeregistrationTime error.");
            return LE_FAULT;
        }
    } else {
        LE_ERROR("Wrong hlap timer type.");
        return LE_FAULT;
    }

    LE_DEBUG("Resume the hlap timer with the value = %d", duration);
    if (duration > 0) {
        auto promisePtr = std::make_shared<std::promise<le_result_t>>();
        auto cb = [promisePtr](pa_result_t error,std::any context)
        {
            try
            {
                if (error == PA_OK)
                {
                    LE_DEBUG("Resume the hlap timer successfully done");
                    promisePtr->set_value(LE_OK);
                }
                else
                {
                    LE_ERROR("Resume the hlap timer failed, errorCode: %d", static_cast<int>(error));
                    promisePtr->set_value(LE_FAULT);
                }
            }
            catch (const std::future_error& e)
            {
                LE_ERROR("Future error in callback: %s", e.what());
            }
            catch (const std::exception& e)
            {
                LE_ERROR("Exception in callback: %s", e.what());
                try { promisePtr->set_value(LE_FAULT); } catch (...) {}
            }
            catch (...)
            {
                LE_ERROR("Unknown error in callback.");
                try { promisePtr->set_value(LE_FAULT); } catch (...) {}
            }
        };
        std::future<le_result_t> futResult = promisePtr->get_future();
        pa_result_t result = taf_pa_ecall_RestartHlapTimer(phoneId, timerId, duration, cb,{});
        if (WaitResult(result, futResult, "ResumeHlapTimer")) {
            {
                LE_DEBUG("Resume the hlap timer successfully done");
                return LE_OK;
            }
        } else {
            LE_ERROR("Restarting eCall HLAP timer failed");
        }
    } else {
        LE_ERROR("The duration is incorrect");
    }

    return LE_FAULT;
}

void taf_ecall::OnEventModemUnavailable()
{
    if (t9StartTimeSet == true)
    {
        ElapsedTimeT9 += ConvertElapsedTime(t9StartTime);
        LE_DEBUG("ElapsedTimeT9 is %d when operation status is unavailable", ElapsedTimeT9);
        t9StartTimeSet = false;
    }

    if (t10StartTimeSet == true)
    {
        ElapsedTimeT10 += ConvertElapsedTime(t10StartTime);
        LE_DEBUG("ElapsedTimeT10 is %d when operation status is unavailable", ElapsedTimeT10);
        t10StartTimeSet = false;
    }

    if (t2StartTimeSet == true)
    {
        t2StartTimeSet = false;
    }

    if (!isIdle())
    {
        taf_ecall_State_t state = TAF_ECALL_STATE_FAILED;
        SetStateAndReport(state, -1, "");
    }
}

void taf_ecall::OnEventModemOperational()
{
    bool needToResumeT9Local = true;
    bool needToResumeT10Local = true;

    if (!isIdle())
    {
        needReportCallEndOnReboot = true;
        LE_INFO("Call ended due to modem crash/reboot = %d", ElapsedTimeT9);
        return;
    }

    if(ElapsedTimeT9 == 0)
    {
        LE_DEBUG("No need to resume T9 timer");
        needToResumeT9Local = false;
        if(ElapsedTimeT10 == 0)
        {
            LE_DEBUG("No need to resume T10 timer");
            needToResumeT10Local = false;
        }
    }

    ArmPendingResume(needToResumeT9Local, needToResumeT10Local);
}

void taf_ecall::OnEventSaveHlapTimerElapsed(HlapTimerType_t type, HlapTimerEventType_t event)
{
    LE_DEBUG("OnEventSaveHlapTimerElapsed");
    auto &eCall = taf_ecall::GetInstance();

    if ((type == HLAP_TIMER_TYPE_T9) || (type == HLAP_TIMER_TYPE_T10))
    {
        LE_DEBUG("T9 elapsedTime %d, T10 elapsedTime %d", ElapsedTimeT9, ElapsedTimeT10);
        bool shouldStopTimer = (event == HLAP_TIMER_EVENT_TYPE_EXPIRED) ||
                               (event == HLAP_TIMER_EVENT_TYPE_STOPPED) ||
                               (event == HLAP_TIMER_EVENT_TYPE_UNKNOWN);

        if (shouldStopTimer)
        {
            if (type == HLAP_TIMER_TYPE_T9)
            {
                le_timer_Stop(eCall.elapsedTimeT9Ref);
            } else {
                le_timer_Stop(eCall.elapsedTimeT10Ref);
            }
        }
        else if ((event == HLAP_TIMER_EVENT_TYPE_STARTED) ||
                 (event == HLAP_TIMER_EVENT_TYPE_RESUMED))
        {
            if (type == HLAP_TIMER_TYPE_T9)
            {
                le_timer_Start(eCall.elapsedTimeT9Ref);
            } else {
                le_timer_Start(eCall.elapsedTimeT10Ref);
            }
        }

        le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn(CFG_ECALL_HLAPTIMERELAPSED_PATH);
        if (event != HLAP_TIMER_EVENT_TYPE_RESUMED)
        {
            if (event == HLAP_TIMER_EVENT_TYPE_STARTED)
            {
                if (type == HLAP_TIMER_TYPE_T9)
                {
                    le_cfg_SetInt(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T9, 0);
                } else {
                    le_cfg_SetInt(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T10, 0);
                }
            } else {
                if (type == HLAP_TIMER_TYPE_T9)
                {
                    le_cfg_DeleteNode(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T9);
                } else {
                    le_cfg_DeleteNode(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T10);
                }
            }
        } else {
            if (type == HLAP_TIMER_TYPE_T9)
            {
                le_cfg_SetInt(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T9, ElapsedTimeT9);
            } else {
                le_cfg_SetInt(iteratorRef, CFG_NODE_HLAPTIMERELAPSED_T10, ElapsedTimeT10);
            }
        }
        le_cfg_CommitTxn(iteratorRef);
    }
}

void taf_ecall::OnEventEcallInProgressModemReboot()
{
    uint16_t minNwRegTime = 0;
    uint16_t deRegTime = 0;
    bool needToResumeT9Local = false;
    bool needToResumeT10Local = false;

    if (LE_OK == GetNadMinNetworkRegistrationTime(&minNwRegTime))
    {
        if ((ElapsedTimeT9 < minNwRegTime*60) && (ElapsedTimeT9 > 0))
        {
            needReportT9Start = false;
        } else {
            needReportT9Start = true;
            ElapsedTimeT9 = 0;
        }

        needToResumeT9Local = true;
    }

    if (LE_OK == GetNadDeregistrationTime(&deRegTime))
    {
        if ((ElapsedTimeT10 < deRegTime*60) && (ElapsedTimeT10 > 0))
        {
            needReportT10Start = false;
        } else {
            needReportT10Start = true;
            ElapsedTimeT10 = 0;
        }

        needToResumeT10Local = true;
    }

    ArmPendingResume(needToResumeT9Local, needToResumeT10Local);
}

void taf_ecall::ResumeHlapTimerEventHandler(void* reqPtr)
{
    ResumeHlapTimerEvent_t* eventReq = (ResumeHlapTimerEvent_t*)reqPtr;
    auto &eCall = taf_ecall::GetInstance();

    if(eventReq == NULL)
    {
        LE_ERROR ("Invalid Parameters");
        return;
    }

    switch (eventReq->event) {
        case EVENT_MODEM_OPERATIONALSTATUS_UNAVILABLE:
            eCall.OnEventModemUnavailable();
            break;
        case EVENT_MODEM_OPERATIONALSTATUS_OPERATIONAL:
            LE_DEBUG("Resume hlap timer when modem available");
            eCall.OnEventModemOperational();
            break;
        case EVENT_SAVE_HLAP_TIMER_ELAPSED:
            LE_DEBUG("Update the hlap timer with elapsed value to config tree");
            eCall.OnEventSaveHlapTimerElapsed(eventReq->hlapTimerType, eventReq->hlapTimerEventType);
            break;
        case EVENT_ECALL_IN_PROGRESS_MODEM_REBOOT:
            eCall.OnEventEcallInProgressModemReboot();
            break;
        case EVENT_RESUME_HLAP_TIMER_ON_MODE_CHANGE:
            LE_DEBUG("Resume hlap timer on operating mode change");
            eCall.OnEventResumeHlapTimerOnModeChange(eventReq->eCallMode);
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
            ECallObject.msd.propulsionType.gasolineTankPresent = true;
        } else {
            ECallObject.msd.propulsionType.gasolineTankPresent = false;
        }

        if ((vehInfo.propulsionType & ECALL_HAL_BITMASK_PROP_TYPE_DIESEL_TANK) == ECALL_HAL_BITMASK_PROP_TYPE_DIESEL_TANK)
        {
            ECallObject.msd.propulsionType.dieselTankPresent = true;
        } else {
            ECallObject.msd.propulsionType.dieselTankPresent = false;
        }

        if ((vehInfo.propulsionType & ECALL_HAL_BITMASK_PROP_TYPE_COMPRESSED_NATURALGAS) == ECALL_HAL_BITMASK_PROP_TYPE_COMPRESSED_NATURALGAS)
        {
            ECallObject.msd.propulsionType.compressedNaturalGas = true;
        } else {
            ECallObject.msd.propulsionType.compressedNaturalGas = false;
        }

        if ((vehInfo.propulsionType & ECALL_HAL_BITMASK_PROP_TYPE_PROPANE_GAS) == ECALL_HAL_BITMASK_PROP_TYPE_PROPANE_GAS)
        {
            ECallObject.msd.propulsionType.liquidPropaneGas = true;
        } else {
            ECallObject.msd.propulsionType.liquidPropaneGas = false;
        }

        if ((vehInfo.propulsionType & ECALL_HAL_BITMASK_PROP_TYPE_ELECTRIC) == ECALL_HAL_BITMASK_PROP_TYPE_ELECTRIC)
        {
            ECallObject.msd.propulsionType.electricEnergyStorage = true;
        } else {
            ECallObject.msd.propulsionType.electricEnergyStorage = false;
        }

        if ((vehInfo.propulsionType & ECALL_HAL_BITMASK_PROP_TYPE_HYDROGEN) == ECALL_HAL_BITMASK_PROP_TYPE_HYDROGEN)
        {
            ECallObject.msd.propulsionType.hydrogenStorage = true;
        } else {
            ECallObject.msd.propulsionType.hydrogenStorage = false;
        }

        if ((vehInfo.propulsionType & ECALL_HAL_BITMASK_PROP_TYPE_OTHER) == ECALL_HAL_BITMASK_PROP_TYPE_OTHER)
        {
            ECallObject.msd.propulsionType.otherStorage = true;
        } else {
            ECallObject.msd.propulsionType.otherStorage = false;
        }

        if (CheckVIN((char *)vehInfo.vin))
        {
            LE_ERROR("VIN is wrong %s", vehInfo.vin);
            ECallObject.msd.vehicleIdentification.isowmi = "000";
            ECallObject.msd.vehicleIdentification.isovds = "000000";
            ECallObject.msd.vehicleIdentification.isovisModelyear = "0";
            ECallObject.msd.vehicleIdentification.isovisSeqPlant = "0000000";
        } else {
            std::string vinStr = vehInfo.vin;
            ECallObject.msd.vehicleIdentification.isowmi = vinStr.substr(ISOWMI_START, ISOWMI_LENGTH );
            ECallObject.msd.vehicleIdentification.isovds = vinStr.substr(ISOVDS_START, ISOVDS_LENGTH);
            ECallObject.msd.vehicleIdentification.isovisModelyear =
                                 vinStr.substr(ISOVIS_MODEL_YEAR_START, ISOVIS_MODEL_YEAR_LENGTH);
            ECallObject.msd.vehicleIdentification.isovisSeqPlant =
                                 vinStr.substr(ISOVIS_SEQ_PLANT_START, ISOVIS_SEQ_PLANT_LENGTH);
        }

        ECallObject.msd.control.vehicleType = (taf_pa_ecall_vehicle_type_t)vehInfo.vehiType;

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

        if ((ECallObject.msd.control.vehicleType < (taf_pa_ecall_vehicle_type_t)ECALL_HAL_VEHITYPE_PASSENGER_VEHICLE_CLASS_M1) ||
            (ECallObject.msd.control.vehicleType > (taf_pa_ecall_vehicle_type_t)maxVehicleType))
        {
            LE_ERROR("VehicleType is wrong %d", (int)ECallObject.msd.control.vehicleType);
            ECallObject.msd.control.vehicleType = (taf_pa_ecall_vehicle_type_t)ECALL_HAL_VEHITYPE_PASSENGER_VEHICLE_CLASS_M1;
        }

        taf_hal_eCall_ActivateType actType = ECALL_HAL_ACTTYPE_AUTOMATIC;

        ECallObject.msd.control.automaticActivation = false;
        eCallPtr->msd.optionalData.numberOfPassengersPresent = false;

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
                eCallPtr->msd.optionalData.numberOfPassengersPresent = true;
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

    std::shared_ptr<taf_pa_ecall_CallInfo_t> spCall = nullptr;
    std::vector<std::shared_ptr<taf_pa_ecall_CallInfo_t>> callList;
    pa_result_t callListRes = taf_pa_ecall_GetInProgressCalls(&callList);
    if (callListRes != PA_OK)
    {
        LE_ERROR("taf_pa_ecall_GetInProgressCalls failed: %d", (int)callListRes);
        return LE_FAULT;
    }
    for(auto callIterator = std::begin(callList); callIterator != std::end(callList);
        ++callIterator) {
        taf_pa_ecall_call_status_t callState = (*callIterator)->callState;
        if(callState != taf_pa_ecall_call_status_t::ENDED) {
            spCall = *callIterator;
            break;
        }
    }
    if(spCall && eCallPtr->callIndex == spCall->callIndex) {
         *isInProgress = true;
    } else {
        *isInProgress = false;
    }
    return LE_OK;
}

le_result_t taf_ecall::ConfigureInitialDialRedial(std::vector<int> redialPara)
{

    for (size_t i = 0; i < redialPara.size(); i++)
    {
        LE_DEBUG("ConfigureInitialDialRedial redialPara = %d", redialPara[i]);
    }
    auto promisePtr = std::make_shared<std::promise<le_result_t>>();
    auto cb = [promisePtr](pa_result_t error,std::any context)
    {
        try
        {
            if (error == PA_OK)
            {
                LE_INFO("Set eCall redial parameter successfully done");
                promisePtr->set_value(LE_OK);
            }
            else
            {
                LE_INFO("Send eCall redial parameter failed, errorCode: %d", static_cast<int>(error));
                promisePtr->set_value(LE_FAULT);
            }
        }
        catch (const std::future_error& e)
        {
            LE_ERROR("Future error in callback: %s", e.what());
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in callback: %s", e.what());
            try { promisePtr->set_value(LE_FAULT); } catch (...) {}
        }
        catch (...)
        {
            LE_ERROR("Unknown error in callback.");
            try { promisePtr->set_value(LE_FAULT); } catch (...) {}
        }
    };

    std::future<le_result_t> futResult = promisePtr->get_future();
    pa_result_t result = taf_pa_ecall_SetEcallRedial(redialPara, cb,{});
    if (WaitResult(result, futResult, "ConfigureInitialDialRedial")) {
        {
            LE_INFO("Set eCall redial parameter successfully.");
            return LE_OK;
        }
    } else {
        LE_ERROR("Set eCall redial parameter failed");
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
