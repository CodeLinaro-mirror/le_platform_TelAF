/*
 *  Copyright (c) 2021-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <vector>
#include "tafSvcIF.hpp"
#include "taf_pa_voicecall.h"

namespace tafsvc {

// Maximum number of tafcall objects
constexpr int MAX_TAFCALL_OBJ = 20;

// Maximum number of sessions/clients supported simultaneously
constexpr int MAX_TAFCALL_SESSION = 5;

// Maximum voice call support, DSDA, two calls
constexpr int MAX_VOICECALL_SUPPORT = 2;

// Call request destination length
constexpr int MAX_DESTINATION_LEN = 50;
constexpr int MAX_DESTINATION_LEN_BYTE = MAX_DESTINATION_LEN + 1;

// Invalid call index
constexpr int INVALID_CALL_IDX = -1;

// Maximum phone ID
constexpr int MAX_PHONE_ID = 2;

constexpr int TIMEOUT_CALLCOMMAND_CB = 2;
constexpr int MAX_INIT_TIMEOUT = 5;

constexpr int CMD_TIMEOUT_MS = 5000;

enum class taf_voicecall_Direction_t {
    NONE = 0,
    INCOMING = 1,
    OUTGOING = 2,
};

struct taf_CallRefNode_t {
    taf_voicecall_CallRef_t callRef;
    le_dls_Link_t link;
};

struct taf_SessionRef_t {
    le_msg_SessionRef_t sessionRef;
    le_dls_Link_t link;
};

struct taf_SessionCtx_t {
    le_msg_SessionRef_t sessionRef; // Client reference
    le_dls_List_t handlerList; // Client handler list
    le_dls_List_t callRefList; // Client call reference list
    le_dls_Link_t link; // Link to SessionCtxList
};

struct taf_HandlerCtx_t {
    taf_voicecall_StateHandlerRef_t handlerRef; // Handler reference
    taf_voicecall_StateHandlerFunc_t handlerPtr; // Function pointer
    void* usrContext;
    taf_SessionCtx_t* sessionCtxPtr; // Handler's session
    le_dls_Link_t link; // Link to handler list
};

struct taf_VoiceCtrl_t {
    int8_t phoneId;
    char destId[MAX_DESTINATION_LEN_BYTE];
    taf_voicecall_Direction_t dir;

    taf_voicecall_CallRef_t callRef;

    le_dls_List_t sessionRefList; // Session list for clients
    le_dls_Link_t link; // Link to call control list

    taf_voicecall_Event_t event;
    taf_voicecall_Event_t lastEvent;
    taf_voicecall_CallEndCause_t termination;
    int32_t terminationCode;
    bool isInProgress;
};

struct CallEvent_t {
    int8_t phoneId;
    taf_voicecall_Direction_t direction;
    char dest[MAX_DESTINATION_LEN];
    taf_voicecall_CallRef_t callRef;
    taf_voicecall_Event_t event;
    taf_voicecall_CallEndCause_t termination;
};

struct CallbackContext {
    std::function<void(le_result_t)> callback;
    std::shared_ptr<void> keepAlive;
};

// Define the class to handle the call with telsdk
class VoiceCallSvc : public ITafSvc {
public:
    void Init();

    static VoiceCallSvc& GetInstance();

    VoiceCallSvc() = default;
    ~VoiceCallSvc() = default;

    // Call interfaces
    le_result_t MakeCall(taf_VoiceCtrl_t* callCtxPtr, const char* dialNumber, int phoneId);
    le_result_t AnswerCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef);
    le_result_t DeleteCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef);
    le_result_t StopCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef);
    le_result_t HoldCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef);
    le_result_t ResumeCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef);
    le_result_t SwapCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef);

    // CallCtx interfaces
    taf_VoiceCtrl_t* CreateCallCtx(int8_t phoneId, const char* destinationPtr, taf_voicecall_Direction_t dir);
    void DestructorCallCtx(void* objPtr);
    taf_VoiceCtrl_t* GetCallCtx(int8_t phoneId, const char* destinationPtr, taf_voicecall_Direction_t dir);
    taf_VoiceCtrl_t* GetCallCtx(taf_voicecall_CallRef_t reference);
    taf_SessionRef_t* GetSessionRefNodeFromCallCtx(taf_VoiceCtrl_t* callCtxPtr, le_msg_SessionRef_t sessionRef);
    le_result_t SetSessionRefToCallCtx(taf_VoiceCtrl_t* callCtxPtr, le_msg_SessionRef_t sessionRef);
    le_result_t UnsetSessionRefToCallCtx(taf_VoiceCtrl_t* callCtxPtr, le_msg_SessionRef_t sessionRef);

    // CallRef interfaces
    taf_voicecall_CallRef_t SetCallRef(taf_VoiceCtrl_t* callCtxPtr);

    // SessionCtx interfaces
    taf_SessionCtx_t* CreateSessionCtx();
    taf_SessionCtx_t* GetSessionCtx(le_msg_SessionRef_t sessionRef);

    // SessionRef interfaces
    le_result_t ReleaseSession(le_msg_SessionRef_t sessionRef, void* ctxPtr);

    // Handler interfaces
    taf_voicecall_StateHandlerRef_t CreateStateHandlerCtx(taf_SessionCtx_t* sessionCtxPtr, taf_voicecall_StateHandlerFunc_t handlerPtr, void* contextPtr);
    le_result_t RemoveStateHandlerCtx(le_msg_SessionRef_t sessionRef, taf_voicecall_StateHandlerRef_t handlerRef);
    le_result_t RemoveStateHandlerCtx(le_msg_SessionRef_t sessionRef);
    void CallHandler(CallEvent_t* eventVoicePtr);
    le_result_t SendCallEventToClient(taf_VoiceCtrl_t* callCtxPtr);
    taf_voicecall_Event_t EventConvert(taf_pa_voicecall_event_t event);
    taf_voicecall_Direction_t DirConvert(taf_pa_voicecall_dir_t paDir);
    taf_pa_voicecall_dir_t DirToPADir(taf_voicecall_Direction_t dir);
    taf_voicecall_CallEndCause_t EndCauseConvert(taf_pa_voicecall_termination_t paTerm);

    // DFX interfaces
    void ShowAll();
    static bool isEnableDebug;
    const char* EventToString(taf_voicecall_Event_t event);
    const char* TerminationToString(taf_voicecall_CallEndCause_t termination);
    const char* PaEventToString(taf_pa_voicecall_event_t event);

    // TelAF side interface
    le_mem_PoolRef_t CallCtrlPool = nullptr;
    le_mem_PoolRef_t CallRefPool = nullptr;
    le_mem_PoolRef_t SessionCtxPool = nullptr;
    le_mem_PoolRef_t SessionRefPool = nullptr;
    le_mem_PoolRef_t HandlerPool = nullptr;
    le_ref_MapRef_t CallCtrlRefMap = nullptr;
    le_ref_MapRef_t HandlerRefMap = nullptr;

    // Handle call events/states from telsdk
    le_event_Id_t CallEvent;

    // Lists for session and call
    le_dls_List_t SessionCtxList = LE_DLS_LIST_INIT;
    le_dls_List_t CallCtrlList = LE_DLS_LIST_INIT;

    static void commonCallback(taf_pa_voicecall_Ref_t reference, le_result_t result, void* contextPtr)
    {
        auto* ctx = static_cast<CallbackContext*>(contextPtr);
        if (ctx && ctx->callback) {
            try {
                LE_INFO("Get result from PA: %d", result);
                ctx->callback(result);
            } catch (const std::exception& e) {
                LE_ERROR("Exception in lambda callback: %s", e.what());
            } catch (...) {
                LE_ERROR("Unknown exception in lambda callback.");
            }
            ctx->keepAlive.reset();
        }
    }

    template<typename CallFunc>
    le_result_t CallWithAsyncCallback(
        taf_VoiceCtrl_t* callCtxPtr,
        CallFunc callFunc,
        taf_pa_voicecall_dir_t direction,
        taf_voicecall_Event_t failEvent,
        const char* actionName)
    {
        auto promisePtr = std::make_shared<std::promise<le_result_t>>();
        std::weak_ptr<std::promise<le_result_t>> weakPromise = promisePtr;
        std::future<le_result_t> futResult = promisePtr->get_future();
    
        auto cmdCtx = std::make_shared<CallbackContext>();
        cmdCtx->keepAlive = cmdCtx;
        cmdCtx->callback = [weakPromise](le_result_t result)
        {
            if (auto locked = weakPromise.lock()) {
                locked->set_value(result);
            }
        };

        taf_pa_voicecall_Ref_t callInfoRef = taf_pa_voicecall_CreateReference(
            callCtxPtr->phoneId, callCtxPtr->destId, direction);
        if (!callInfoRef)
        {
            LE_ERROR("Cannot create call reference for %s", actionName);
            return LE_FAULT;
        }

        le_result_t result = callFunc(callInfoRef, commonCallback, cmdCtx.get());
        if (result != LE_OK)
        {
            LE_ERROR("%s failed immediately: %d", actionName, result);
            taf_pa_voicecall_DeleteReference(callInfoRef);
            CallEvent_t msgCallEvent = {0, taf_voicecall_Direction_t::NONE,
                "", callCtxPtr->callRef, failEvent, TAF_VOICECALL_END_UNDEFINED};
            le_event_Report(CallEvent, &msgCallEvent, sizeof(CallEvent_t));
            return result;
        }

        if (futResult.wait_for(std::chrono::milliseconds(CMD_TIMEOUT_MS)) == std::future_status::timeout)
        {
            taf_pa_voicecall_DeleteReference(callInfoRef);
            LE_ERROR("%s timed out", actionName);
            return LE_TIMEOUT;
        }

        le_result_t asyncResult = futResult.get();
        taf_pa_voicecall_DeleteReference(callInfoRef);
    
        if (asyncResult != LE_OK)
        {
            LE_ERROR("%s failed in callback", actionName);
            CallEvent_t msgCallEvent = {0, taf_voicecall_Direction_t::NONE,
                "", callCtxPtr->callRef, failEvent, TAF_VOICECALL_END_UNDEFINED};
            le_event_Report(CallEvent, &msgCallEvent, sizeof(CallEvent_t));
        }

        return LE_OK;
    }
};

// Define handler class for telaf's callback
class Handler : public ITafSvc {
public:
    void Init() {
        return;
    }
    // Taf service handlers
    static void CloseSessHandler(le_msg_SessionRef_t sessionRef, void* ctxPtr);

    // Handler for the call request
    static void ProcessStateChanged(void* reportPtr);

    // Define the call ctrl for release handler
    static void ReleaseCallCtrlHandler(void* objPtr);

    static void PaEventListener(taf_pa_voicecall_Ref_t reference, taf_pa_voicecall_event_t event, void *contextPtr);
};


} // namespace tafsvc




