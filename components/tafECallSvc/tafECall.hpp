/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafEcallPa.hpp"
#include "tafSvcIF.hpp"
#include <unordered_map>
#include <mutex>
#include <atomic>

// For using VHAL
#include "tafHalLib.hpp"
#include "tafHalECall.h"

using namespace tafpa::ecall;
using namespace std;

#define MAX_ECALL  1
#define CFG_NODE_ECALL "eCall"
#define CFG_MODEMSERVICE_ECALL_PATH "tafeCallSvc:/eCall"
#define CFG_NODE_MSDMODE "msdtxMode"
#define CFG_NODE_MSDVERSION "msdVersion"
#define CFG_NODE_MSDVEHTYPE "msdVehicleType"
#define CFG_NODE_MSDVIN "msdVehIdentNum"
#define CFG_NODE_MSDTIMESTAMPSYSTEM "msdTimeStampSystem"
#define CFG_NODE_MSDTIMESTAMPSET "msdTimeStampSet"
#define CFG_NODE_MSDMESSAGEIDENTIFIER "msdMessageIdentifier"
#define CFG_ECALL_PROPULSIONTYPE_PATH "tafeCallSvc:/eCall/msdPropulsionType"
#define CFG_NODE_PROPULSION_GASOLINE "Gasoline"
#define CFG_NODE_PROPULSION_DIESEL "Diesel"
#define CFG_NODE_PROPULSION_NATURALGAS "Naturalgas"
#define CFG_NODE_PROPULSION_PROPANE "Propane"
#define CFG_NODE_PROPULSION_ELECTRIC "Electric"
#define CFG_NODE_PROPULSION_HYDROGEN "Hydrogen"
#define CFG_NODE_PROPULSION_OTHER "Other"
#define CFG_ECALL_HLAPTIMERELAPSED_PATH "tafeCallSvc:/eCall/hlapTimerElapsed"
#define CFG_NODE_HLAPTIMERELAPSED_T9 "T9ElapsedTime"
#define CFG_NODE_HLAPTIMERELAPSED_T10 "T10ElapsedTime"
#define CFG_NODE_LASTECALL_PHONEID "LasteCallPhoneId"
#define ISOWMI_START 0
#define ISOWMI_LENGTH 3
#define ISOVDS_START (ISOWMI_START + ISOWMI_LENGTH)
#define ISOVDS_LENGTH 6
#define ISOVIS_MODEL_YEAR_START (ISOVDS_START + ISOVDS_LENGTH)
#define ISOVIS_MODEL_YEAR_LENGTH 1
#define ISOVIS_SEQ_PLANT_START (ISOVIS_MODEL_YEAR_START + ISOVIS_MODEL_YEAR_LENGTH)
#define ISOVIS_SEQ_PLANT_LENGTH 7
#define MSD_VERSION_TWO 2
#define MSD_VERSION_THREE 3
#define SET_PSAP_NUM_TYPE_DEFFAULT 0
#define SET_PSAP_NUM_TYPE_OVERRIDDEN 1
#define MAX_EU_MSD_LENGTH 140
#define MAX_INIT_TIMEOUT 5
#define MAX_DESTINATION_LEN 50
#define MSD_EURONCAP_OAD_DELTAV_INVALD 255
#define MSD_EURONCAP_OAD_DELTAVX_MIN -250
#define MSD_EURONCAP_OAD_DELTAVX_MAX 250
#define MSD_EURONCAP_OAD_DELTAVY_MIN -250
#define MSD_EURONCAP_OAD_DELTAVY_MAX 250
#define MSD_EURONCAP_OAD_RANGELIMIT_MIN 100
#define MSD_EURONCAP_OAD_RANGELIMIT_MAX 250
#define MAX_MSD_MESSAGE_IDENTIFIER 255
#define MIN_MSD_MESSAGE_IDENTIFIER 1
#define MSD_TIMESTAMP_STR_INVALID "INVALID"
#define MIN_PHONE_ID  1
#define MAX_PHONE_ID  2
#define RX_ECALL_EVENT_POOL_SIZE 50
#define MAX_T9_T10_ELAPSED_TIME_SEC 43200
#define NSEC_PER_SEC 1000000000L

    namespace tafsvc {

        typedef enum
        {
            ECALL_INIT,
            ECALL_REQUEST,
            ECALL_DIALING,
            ECALL_ALERTING,
            ECALL_ACTIVE,
            ECALL_COMPLETED,
            ECALL_NOT_CONNECTED,
            ECALL_ENDED,
            ECALL_INCOMING
        }
        tafECallSession_t;

        typedef struct
        {
            taf_ecall_IILocations_t locationOfImpact;
            bool rolloverDetectedPresent;
            bool rolloverDetected;
            uint8_t rangeLimit;
            int16_t deltaVX;
            int16_t deltaVY;
        }
        taf_EuroNCAPData_t;

        typedef struct
        {
            uint8_t                             dialAttempts;
            uint16_t                            dialInterval[TAF_ECALL_MAX_DIAL_ATTEMPTS_LENGTH];
        }
        taf_DialRedial_t;

        typedef struct
        {
            taf_ecall_CallRef_t                 reference;
            taf_pa_ecall_msd_data_t             msd;
            bool                                isMsdUpdated;
            taf_ecall_MsdTransmissionMode_t     msdTxMode;
            tafECallSession_t                   eCallSession;
            uint8_t                             msdPdu[TAF_ECALL_MAX_MSD_LENGTH];
            size_t                              pduMsdSize;
            int32_t                             callIndex;
            taf_ecall_State_t                   state;
            taf_EuroNCAPData_t                  euroNCAPData;
            uint8_t                             oadData[TAF_ECALL_MAX_DATA_LENGTH];
            size_t                              oadDataSize;
            bool                                isPrieCallOngoing;
            taf_ecall_Type_t                    type;
            std::shared_ptr<taf_pa_ecall_CallInfo_t>  iCall;
            taf_DialRedial_t                    dialRedial;
            bool                                waitForALACKPos;
            int8_t                              phoneId;
            taf_ecall_TerminationRedialReason_t redialReason;
        }
        taf_ECall_t;

        typedef struct
        {
            taf_ecall_CallRef_t  eCallRef;
            taf_ecall_State_t    state;
            int8_t               phoneId;
            char                 dest[MAX_DESTINATION_LEN];
        }StateChangeEvent_t;

        typedef enum
        {
            ALACK_TIMER_START,
            ALACK_TIMER_STOP
        }
        ALACKTimer_t;

        typedef struct
        {
            ALACKTimer_t         alackTimer;
        }ALACKTimerEvent_t;

        typedef enum
        {
            EVENT_MODEM_OPERATIONALSTATUS_UNAVILABLE,
            EVENT_MODEM_OPERATIONALSTATUS_OPERATIONAL,
            EVENT_SAVE_HLAP_TIMER_ELAPSED,
            EVENT_ECALL_IN_PROGRESS_MODEM_REBOOT,
            EVENT_RESUME_HLAP_TIMER_ON_MODE_CHANGE
        }
        Event_t;

        typedef enum
        {
            HLAP_TIMER_TYPE_T9,
            HLAP_TIMER_TYPE_T10
        }HlapTimerType_t;

        typedef enum
        {
            HLAP_TIMER_EVENT_TYPE_UNKNOWN,
            HLAP_TIMER_EVENT_TYPE_STARTED,
            HLAP_TIMER_EVENT_TYPE_STOPPED,
            HLAP_TIMER_EVENT_TYPE_EXPIRED,
            HLAP_TIMER_EVENT_TYPE_RESUMED
        }
        HlapTimerEventType_t;

        typedef struct
        {
            Event_t event;
            HlapTimerType_t hlapTimerType;
            HlapTimerEventType_t hlapTimerEventType;
            int8_t phoneId;
            taf_pa_ecall_mode_t eCallMode;
        }ResumeHlapTimerEvent_t;

        typedef enum {
            ECALL_EVENT_INCOMING_CALL,
            ECALL_EVENT_CALL_INFO_CHANGE,
            ECALL_EVENT_MSD_TRANSMISSION_STATUS,
            ECALL_EVENT_HLAP_TIMER,
            ECALL_EVENT_MSD_UPDATE_REQ,
            ECALL_EVENT_REDIAL,
            ECALL_EVENT_MAKECALL_RESP
        } RxECallEventType_t;

        typedef struct {
            uint64_t callToken;
            int32_t callIndex;
            taf_pa_ecall_call_status_t callState;
            char remotePartyNumber[MAX_DESTINATION_LEN];
        } RxECallIncomingCallParam_t;

        typedef struct {
            uint64_t callToken;
            int32_t callIndex;
            taf_pa_ecall_call_status_t callState;
            taf_pa_ecall_dir_t callDirection;
            taf_pa_ecall_termination_t callEndCause;
        } RxECallInfoChangeParam_t;

        typedef struct {
            taf_pa_ecall_msd_status_t msdTransmissionStatus;
        } RxECallMsdTransmissionStatusParam_t;

        typedef struct {
            taf_pa_ecall_hlap_timer_events_t timerEvents;
        } RxECallHlapTimerParam_t;

        typedef struct {
            taf_pa_ecall_redial_info_t redialInfo;
        } RxECallRedialParam_t;

        typedef struct {
            int32_t callIndex;
        } RxECallMakeCallResponse;

        typedef struct {
            RxECallEventType_t eventType;
            int phoneId;
            union {
                RxECallIncomingCallParam_t incomingCall;
                RxECallInfoChangeParam_t infoChange;
                RxECallMsdTransmissionStatusParam_t msdTransmissionStatus;
                RxECallHlapTimerParam_t hlapTimer;
                RxECallRedialParam_t redial;
                RxECallMakeCallResponse response;
            } param;
        } RxECallEvent_t;

        struct PendingECallEvent {
            RxECallEventType_t eventType;
            int phoneId;
            int32_t callIndex;
            void* eventData;
        };

        class tafCallCommandCallback{
            public:
                static void makeECallResponse(std::shared_ptr<taf_pa_ecall_CallInfo_t> callInfo,
                    pa_result_t errorCode,std::any context);
        };

        class tafPrieCallCommandCallback{
            public:
                static void makeECallResponse(std::shared_ptr<taf_pa_ecall_CallInfo_t> callInfo,
                    pa_result_t errorCode,std::any context);
        };

        class tafUpdateMsdCommandCallback{
            public:
                static void commandResponse(pa_result_t errorCode,std::any context);
        };

        class tafHangupCommandCallback{
            public:
                static void commandResponse(pa_result_t errorCode,std::any context);
        };

        class tafRejectCommandCallback{
            public:
                static void commandResponse(pa_result_t errorCode,std::any context);
        };

        class tafAnswerCommandCallback{
            public:
                static void commandResponse(pa_result_t errorCode,std::any context);
        };

        class taf_ecall :public ITafSvc {
            public:
                taf_ecall() {};
                ~taf_ecall() {};
                void Init(void);
                static taf_ecall &GetInstance();

                taf_ecall_CallRef_t CreateECallReference();
                void Delete(taf_ecall_CallRef_t ecallRef);
                le_result_t SetECallOperatingMode(uint8_t phoneId, taf_ecall_OpMode_t ecallMode);
                le_result_t GetECallOperatingMode(uint8_t phoneId, taf_ecall_OpMode_t *opMode);
                le_result_t StartECall(taf_pa_ecall_category_t emergencyCategory, taf_pa_ecall_type_t eCallvariant, taf_ecall_CallRef_t ecallRef);
                le_result_t StartPrivate(taf_ecall_CallRef_t ecallRef, const char * psapNumber, const char * contentType, const char * acceptInfo);
                le_result_t StopECall(taf_ecall_CallRef_t ecallRef);
                le_result_t AnswerECall(taf_ecall_CallRef_t ecallRef);
                le_result_t SetMsdPosition (taf_ecall_CallRef_t ecallRef, bool isTrusted, int32_t latitude,
                    int32_t longitude, int32_t direction);
                le_result_t SetMsdPositionN1 (taf_ecall_CallRef_t ecallRef,int32_t latitudeDeltaN1,int32_t longitudeDeltaN1);
                le_result_t SetMsdPositionN2 (taf_ecall_CallRef_t ecallRef,int32_t latitudeDeltaN2,int32_t longitudeDeltaN2);
                le_result_t SetMsdPassengersCount (taf_ecall_CallRef_t  ecallRef, uint32_t passengerCount);
                le_result_t SetMsdTxMode (taf_ecall_MsdTransmissionMode_t txMode);
                le_result_t GetMsdTxMode ( taf_ecall_MsdTransmissionMode_t* modePtr);
                le_result_t SetMsdAdditionalData(taf_ecall_CallRef_t ecallRef, const char* oid, const uint8_t* data, size_t dataLength);
                le_result_t ResetMsdAdditionalData(taf_ecall_CallRef_t ecallRef);
                le_result_t SetMsdEuroNCAPLocationOfImpact(taf_ecall_CallRef_t ecallRef, taf_ecall_IILocations_t iiLocations);
                le_result_t SetMsdEuroNCAPRolloverDetected(taf_ecall_CallRef_t ecallRef, bool rolloverDetected);
                le_result_t ResetMsdEuroNCAPRolloverDetected(taf_ecall_CallRef_t ecallRef);
                le_result_t SetMsdEuroNCAPIIDeltaV(taf_ecall_CallRef_t ecallRef, uint8_t rangeLimit, int16_t deltaVX, int16_t deltaVY);
                int32_t msd_EncodeOptionalDataForEuroNCAP(taf_EuroNCAPData_t* euroNCAPDataPtr, uint8_t* outDataPtr);
                static uint16_t PutBits(uint16_t msgOffset, uint16_t elmtLen, uint8_t* elmtPtr, uint8_t* msgPtr);
                static uint16_t PutTwoBytes(uint16_t  msgOffset, uint16_t  elmtLen,uint16_t* elmtPtr, uint8_t*  msgPtr);
                static bool ReadMsdTimeStampFromConfigTree(const char* nodeName, uint32_t* outTimeStamp);
                static void WriteMsdTimeStampToConfigTree(const char* nodeName, const char* timestampStr);
                le_result_t SetMsdTimeStamp(taf_ecall_CallRef_t ecallRef, uint32_t timeStamp);
                le_result_t ResetMsdTimeStamp(taf_ecall_CallRef_t ecallRef);
                static void WriteMsdMsgIdToConfigTree(uint32_t msgId);
                static uint32_t ReadMsdMsgIdFromConfigTree();
                le_result_t SetPsapNumber( const char* psapNumber );
                le_result_t GetPsapNumber( char* psapNumber, size_t psapNumLength );
                le_result_t UseUSimNumbers();
                bool isIdle();
                le_result_t SetNadDeregistrationTime(uint16_t deregTime);
                le_result_t GetNadDeregistrationTime(uint16_t* deregTime);
                le_result_t TerminateRegistration();
                le_result_t SetNadClearDownFallbackTime(uint16_t ccftTime);
                le_result_t GetNadClearDownFallbackTime(uint16_t* ccftTime);
                le_result_t SetNadMinNetworkRegistrationTime(uint16_t minNwRegTime);
                le_result_t GetNadMinNetworkRegistrationTime(uint16_t* minNwRegTime);
                taf_ecall_State_t GetState ( taf_ecall_CallRef_t ecallRef);
                taf_ecall_TerminationReason_t GetTerminationReason ( taf_ecall_CallRef_t ecallRef);
                taf_ecall_Type_t GetType ( taf_ecall_CallRef_t ecallRef);
                le_result_t GetHlapTimerState(taf_ecall_HlapTimerType_t timerType, taf_ecall_HlapTimerStatus_t* timerStatus, uint16_t* elapsedTime);
                taf_ecall_HlapTimerStatus_t GetHlapTimerStatus(taf_ecall_HlapTimerType_t timerType);
                taf_ecall_HlapTimerStatus_t ConvertHlapTimerStatus(taf_pa_ecall_hlap_timer_state_t status);
                uint16_t ConvertElapsedTime(const timespec& start);
                static void T9TimerExpiryHandler(le_timer_Ref_t timerRef);
                static void T10TimerExpiryHandler(le_timer_Ref_t timerRef);
                static void ResumeHlapTimerModeWaitHandler(le_timer_Ref_t timerRef);
                HlapTimerEventType_t ConvertHlapTimerEvent(taf_pa_ecall_hlap_event_t event);
                void ArmPendingResume(bool needResumeT9, bool needResumeT10);
                void ResumeHlapTimers(bool needResumeT9, bool needResumeT10, taf_ecall_OpMode_t opMode);
                le_result_t ResumeHlapTimer(taf_ecall_HlapTimerType_t timerType);
                void OnEventResumeHlapTimerOnModeChange(taf_pa_ecall_mode_t eCallMode);
                void OnEventModemUnavailable();
                void OnEventModemOperational();
                void OnEventSaveHlapTimerElapsed(HlapTimerType_t type, HlapTimerEventType_t event);
                void OnEventEcallInProgressModemReboot();
                static void ResumeHlapTimerEventHandler(void* reqPtr);
                le_result_t IsInProgress(taf_ecall_CallRef_t ecallRef, bool* isInProgress);
                le_result_t ConfigureInitialDialRedial(std::vector<int> redialPara);
                le_result_t SetInitialDialAttempts(uint8_t attempts);
                le_result_t SetInitialDialIntervalBetweenDialAttempts(const uint16_t* interval, size_t intervalLength);
                taf_ecall_TerminationRedialReason_t MapRedialReason(taf_pa_ecall_reason_type_t redialReson);
                le_result_t GetTerminationRedialReason(taf_ecall_CallRef_t ecallRef, taf_ecall_TerminationRedialReason_t* reason);
                uint64_t StashCall(std::shared_ptr<taf_pa_ecall_CallInfo_t> sp);
                std::shared_ptr<taf_pa_ecall_CallInfo_t> TakeCall(uint64_t token);
                void HandleIncomingCall(int phoneId, const RxECallIncomingCallParam_t& incomingCall);
                void HandleCallInfoChange(int phoneId, const RxECallInfoChangeParam_t& infoChange);
                void HandleMsdTransmissionStatus(int phoneId, taf_pa_ecall_msd_status_t status);
                void HandleHlapTimerEvent(int phoneId, taf_pa_ecall_hlap_timer_events_t timerEvents);
                void HandleMsdUpdateRequest(int phoneId);
                void HandleRedial(int phoneId, taf_pa_ecall_redial_info_t redialInfo);
                void HandleMakeCallResp(int phoneId, RxECallMakeCallResponse resp);
                static void ProcessRxECallEvent(void* msgPtr);
                void* CloneEventData(const RxECallEvent_t* eventPtr);
                void FreeEventData(RxECallEventType_t eventType, void* data);
                void ProcessPendingCallEvents(int phoneId, int callIndex);
                void HandleCallEnd(int phoneId, int callIndex);
                taf_ecall_StateChangeHandlerRef_t AddStateChangeHandler (taf_ecall_StateChangeHandlerFunc_t handlerPtr,
                                                                                        void* contextPtr);
                void RemoveStateChangeHandler (taf_ecall_StateChangeHandlerRef_t handlerRef);
                static void FirstLayerStateChangeHandler(void* reportPtr,
                                     void* secondLayerHandlerFunc);

                static void ConfigChangeHandler (void* contextPtr);
                void UpdateMsd ();
                le_result_t ImportMsd( taf_ecall_CallRef_t ecallRef, const uint8_t* pdumsd, size_t msdLength);
                le_result_t ExportMsd( taf_ecall_CallRef_t ecallRef, uint8_t* pdumsd, size_t* msdLength);
                le_result_t SendMsd( taf_ecall_CallRef_t ecallRef);
                le_result_t RetrieveEncodedMsdPdu(taf_pa_ecall_msd_data_t eCallMsdData, uint8_t* pduMsd, size_t *msdLength);
                static int CheckVIN(char *vin);
                le_result_t UpdateMsdVehicleInfo();
                le_result_t UpdateMsdInformation(taf_ecall_CallRef_t ecallRef);
                void SetSessionState(tafECallSession_t session);
                void SetStateAndReport(taf_ecall_State_t state, int phoneId, const std::string &dest);
                void ClearPduMsd();
                taf_ecall_CallRef_t GetECallReference();
                void SetCallIndex(int32_t callIndex);
                void SetCallPhoneId(int8_t phoneId);
                void SetLastCallPhoneId(int8_t phoneId);
                int8_t GetLastCallPhoneId();
                le_event_Id_t StateChangeEventId;
                le_event_Id_t RxECallEventId;
                le_mem_PoolRef_t RxECallEventPool = NULL;

                taf_pa_ecall_termination_t CallEndError = taf_pa_ecall_termination_t::NORMAL;

                eCall_Inf_t *eCallInf = nullptr;
                bool isDrvPresent = false;

                le_ref_MapRef_t ECallPtrRefMap = NULL;

                le_event_Id_t ResumeHlapTimerEventId;
                le_timer_Ref_t resumeModeWaitTimerRef;
                bool pendingResumeT9 = false;
                bool pendingResumeT10 = false;
                bool needReportT9Start = false;
                bool needReportT10Start = false;
                bool needReportCallEndOnReboot = false;
                int8_t lastCallPhoneId = -1;
            private:
                std::mutex callMtx_;
                std::unordered_map<uint64_t, std::shared_ptr<taf_pa_ecall_CallInfo_t>> callStore_;
                std::atomic<uint64_t> callNextToken_{1};
                std::vector<PendingECallEvent> pendingECallEvents;
                taf_ECall_t ECallObject;
                taf_pa_ecall_event_listener_t eventListener;

                timespec t2StartTime{};
                timespec t9StartTime{};
                timespec t10StartTime{};
                bool t2StartTimeSet = false;
                bool t9StartTimeSet = false;
                bool t10StartTimeSet = false;
                uint16_t ElapsedTimeT9 = 0;
                uint16_t ElapsedTimeT10 = 0;
                le_timer_Ref_t elapsedTimeT9Ref;
                le_timer_Ref_t elapsedTimeT10Ref;
                void InitializeECallPtr();

        };

        class Handler : public ITafSvc
        {
            public:
            void Init() {
                return;
            }
            static void onIncomingCall(std::shared_ptr<taf_pa_ecall_CallInfo_t> callInfo,
                pa_result_t errorCode,std::any context);
            static void onCallInfoChange(std::shared_ptr<taf_pa_ecall_CallInfo_t> callInfo,
                pa_result_t errorCode,std::any context);
            static void onMsdTransmissionStatus(int32_t phoneId,taf_pa_ecall_msd_status_t msdStatus,
                std::any context);
            static void onMsdUpdateRequest(int32_t phoneId,std::any context);
            static void onRedial(int32_t phoneId,
                std::shared_ptr<taf_pa_ecall_redial_info_t> redialInfo,std::any context);
            static void onHlapTimerEvent(int32_t phoneId,
                std::shared_ptr<taf_pa_ecall_hlap_timer_events_t> timerEvent,std::any context);
            static void onEcallOperatingModeChange(int32_t phoneId,
                std::shared_ptr<taf_pa_ecall_mode_info_t> modeInfo,std::any context);
            static void onStateChange(std::shared_ptr<taf_pa_ecall_subsystem_info_t> info,
                taf_pa_ecall_operational_status_t status,
                std::any context);
        };
    }

