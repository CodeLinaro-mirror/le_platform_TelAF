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

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <vector>
#include <telux/tel/PhoneFactory.hpp>
#include "telux/common/CommonDefines.hpp"
#include "tafSvcIF.hpp"

using namespace telux::tel;
using namespace telux::common;

// Max tafcall objects at one time
#define    MAX_TAFCALL_OBJ    20

// Max session/client support simultaneously
#define MAX_TAFCALL_SESSION 5

// Max voice call support,DSDA, two calls
#define MAX_VOICECALL_SUPPORT 2

// Call Requst destination len
#define MAX_DESTINATION_LEN 50
#define MAX_DESTINATION_LEN_BYTE (MAX_DESTINATION_LEN+1)

// The invalid call index
#define INVALID_CALL_IDX -1

// The max phone ID is 2
#define MAX_PHONE_ID 2

#define TIMEOUT_CALLCOMMAND_CB 2

typedef struct
{
    taf_voicecall_CallRef_t  callRef;
    le_dls_Link_t            link;
} taf_CallRefNode_t;

typedef struct
{
    le_msg_SessionRef_t sessionRef;
    le_dls_Link_t       link;
} taf_SessionRef_t;

typedef struct
{
    le_msg_SessionRef_t    sessionRef;          // this client ref
    le_dls_List_t          handlerList;         // this client handler list
    le_dls_List_t          callRefList;         // this client call reference list
    le_dls_Link_t          link;                // link to SessionCtxList
} taf_SessionCtx_t;

typedef struct
{
    taf_voicecall_StateHandlerRef_t handlerRef;     // this handler ref
    taf_voicecall_StateHandlerFunc_t handlerPtr;    // this function ptr
    void *    usrContext;
    taf_SessionCtx_t*    sessionCtxPtr;             // this handler's session
    le_dls_Link_t    link;                          // link to handler list
} taf_HandlerCtx_t;

// internal cmd
typedef enum
{
    CMD_START_CALL,     // start a call
    CMD_END_CALL,       // end a call
    CMD_ANSWER_CALL,    // answer a call
    CMD_DELETE_CALL,    // delete a call
    CMD_HOLD_CALL,      // hold a call
    CMD_RESUME_CALL,    // resume a call
    CMD_SWAP_CALL,      // make a call hold and another call active
} tafCallCmd_t;

namespace telux {
namespace tafsvc {

    // define the listener for state change for telsdk
    class tafCallListener : public ICallListener {
        public:
            void onIncomingCall(std::shared_ptr<telux::tel::ICall> tafCall) override;
            void onCallInfoChange(std::shared_ptr<telux::tel::ICall> tafCall) override;
            const char * callStateToString(telux::tel::CallState state);
            const char* callDirectionToString(telux::tel::CallDirection direction);
            taf_voicecall_Event_t stateToEvent(telux::tel::CallState state);
            taf_voicecall_CallEndCause_t endCauseToTermination(telux::tel::CallEndCause endCause);
            ~tafCallListener(){
            };
    };

    // define the Callback Class for telsdk
    class tafDialCallback : public IMakeCallCallback {
        public:
            tafDialCallback() {}
            ~tafDialCallback() {}
            void makeCallResponse(telux::common::ErrorCode error, std::shared_ptr<telux::tel::ICall> call) override;
    };

    // define the other Callback Class for telsdk
    class tafCallCommandCallback : public telux::common::ICommandResponseCallback {
        public:
            tafCallCommandCallback() {}
            ~tafCallCommandCallback() {}
            void commandResponse(telux::common::ErrorCode error);
    };

    typedef struct tagVoiceCtrl
    {
        int8_t phoneId;
        char destId[MAX_DESTINATION_LEN_BYTE];
        std::shared_ptr<telux::tel::ICall> iCall;

        taf_voicecall_CallRef_t callRef;

        //std::shared_ptr<tafDialCallback>    iCallCb;  // ICall callback
        le_dls_List_t  sessionRefList;                // session list for clients
        le_dls_Link_t  link;                          // link for call ctrl list

        telux::common::Status             tafCallStatus;
        telux::common::ErrorCode          callRspErrCode;
        taf_voicecall_Event_t             event;
        taf_voicecall_Event_t             lastEvent;
        taf_voicecall_CallEndCause_t      termination;
        int32_t                           terminationCode;
        bool                              isInProgress;
    } taf_VoiceCtrl_t;

    typedef struct
    {
        bool                                inComingCall;
        int8_t                              phoneId;
        char                                dest[MAX_DESTINATION_LEN];
        std::shared_ptr<telux::tel::ICall>  iCall;
        taf_voicecall_CallRef_t             callRef;
        taf_voicecall_Event_t               event;
        taf_voicecall_CallEndCause_t        termination;
    } callEvent_t;

    typedef struct
    {
        tafCallCmd_t             cmdID;
        taf_voicecall_CallRef_t  callRef;
        le_msg_SessionRef_t      sessionRef;
        taf_VoiceCtrl_t          *callCtrlPtr;
    } callReq_t;

    // define our class to handler the call with telsdk
    class taf_VoiceCall : public ITafSvc {
    public:
        void Init(void);

        static taf_VoiceCall &GetInstance();

        taf_VoiceCall() {};
        ~taf_VoiceCall() {};

        le_result_t ChecktafCallCommandCallbackResult(void);

        // call interfaces
        le_result_t MakeCall(taf_VoiceCtrl_t *callCtxPtr, const char* dialNumber,int phoneId);
        le_result_t AnswerCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef);
        le_result_t DeleteCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef);
        le_result_t StopCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef);
        le_result_t HoldCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef);
        le_result_t ResumeCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef);
        le_result_t SwapCall(taf_voicecall_CallRef_t callRef, le_msg_SessionRef_t sessionRef);

        // callCtx interfaces
        taf_VoiceCtrl_t* CreateCallCtx(int8_t phoneId, const char* destinationPtr);
        void DestructorCallCtx(void* objPtr);
        taf_VoiceCtrl_t* GetCallCtx(int8_t phoneId, const char* destinationPtr);
        taf_VoiceCtrl_t* GetCallCtx(std::shared_ptr<telux::tel::ICall> iCall);
        taf_VoiceCtrl_t* GetCallCtx(taf_voicecall_CallRef_t reference);
        taf_SessionRef_t* GetSessionRefNodeFromCallCtx(taf_VoiceCtrl_t* callCtxPtr, le_msg_SessionRef_t sessionRef);
        le_result_t SetSessionRefToCallCtx(taf_VoiceCtrl_t* callCtxPtr, le_msg_SessionRef_t sessionRef);
        le_result_t UnsetSessionRefToCallCtx(taf_VoiceCtrl_t* callCtxPtr, le_msg_SessionRef_t sessionRef);

        // callRef interfaces
        taf_voicecall_CallRef_t SetCallRef(taf_VoiceCtrl_t* callCtxPtr);

        // sessionCtx interfaces
        taf_SessionCtx_t* CreateSessionCtx(void);
        taf_SessionCtx_t* GetSessionCtx(le_msg_SessionRef_t sessionRef);
        taf_CallRefNode_t* GetCallRefNodeFromSessionCtx(taf_VoiceCtrl_t* callCtxPtr, taf_SessionCtx_t* sessionCtxPtr);
        taf_voicecall_CallRef_t SetCallRefToSessionCtx(taf_VoiceCtrl_t* callCtxPtr, taf_SessionCtx_t* sessionCtxPtr);

        // sessionRef interfaces
        le_result_t ReleaseSession(le_msg_SessionRef_t sessionRef, void* ctxPtr);

        // handler interfaces
        taf_voicecall_StateHandlerRef_t CreateStateHandlerCtx(taf_SessionCtx_t* sessionCtxPtr, taf_voicecall_StateHandlerFunc_t handlerPtr, void* contextPtr);
        le_result_t RemoveStateHandlerCtx(le_msg_SessionRef_t sessionRef, taf_voicecall_StateHandlerRef_t handlerRef);
        le_result_t RemoveStateHandlerCtx(le_msg_SessionRef_t sessionRef);
        void CallHandler(callEvent_t *eventVoicePtr);
        le_result_t SendCallEventToClient(taf_VoiceCtrl_t *callCtxPtr, bool incomingCall);

        // dfx interfaces
        static bool isEnableDebug;
        const char * EventToString(taf_voicecall_Event_t event);
        const char * TerminationToString(taf_voicecall_CallEndCause_t termination);
        void ShowAll();

        // TelAF side interface
        le_mem_PoolRef_t CallCtrlPool = NULL;
        le_mem_PoolRef_t CallRefPool = NULL;
        le_mem_PoolRef_t SessionCtxPool = NULL;
        le_mem_PoolRef_t SessionRefPool = NULL;
        le_mem_PoolRef_t HandlerPool = NULL;
        le_ref_MapRef_t  CallCtrlRefMap = NULL;
        le_ref_MapRef_t  HandlerRefMap = NULL;

        // to handle the request from clients
        le_event_Id_t ReqEvent;

        // to handle the call event/state from telsdk
        le_event_Id_t CallEvent;

        // Lists for session and call
        le_dls_List_t    SessionCtxList = LE_DLS_LIST_INIT;
        le_dls_List_t    CallCtrlList = LE_DLS_LIST_INIT;

        // objects used by telSdk interfaces
        std::shared_ptr<tafCallListener> CallLsn;
        std::shared_ptr<telux::tel::ICallManager> CallMgr;
        std::shared_ptr<tafDialCallback>    CallCb;
        std::shared_ptr<tafCallCommandCallback>    AnswerCb;
        std::shared_ptr<tafCallCommandCallback>    HangupCb;
        std::shared_ptr<tafCallCommandCallback>    RejectCb;
        std::shared_ptr<tafCallCommandCallback>    HoldCb;
        std::shared_ptr<tafCallCommandCallback>    ResumeCb;
        std::shared_ptr<tafCallCommandCallback>    SwapCb;

        std::promise<le_result_t> CBCallCommandSynePromise;
    };

    // define handler class for telaf's call back
    class taf_Handler: public ITafSvc {
        public:
            void Init(void);

            taf_Handler();
            ~taf_Handler();

            // Taf Service handlers
            static void CloseSessHandler(le_msg_SessionRef_t sessionRef,void* ctxPtr);

            // handler for the call request
            static void ProcessReq(void* callReq);

            // handler for the call request
            static void ProcessStateChanged(void* reportPtr);

            // define the call ctrl for release handler
            static void ReleaseCallCtrlHandler(void* objPtr);

            // tafVoiceCall obj needs to be realized first
            static taf_VoiceCall *TafCallPtr;
    };
}
}

