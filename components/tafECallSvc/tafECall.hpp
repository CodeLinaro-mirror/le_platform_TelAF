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
#include "legato.h"
#include "interfaces.h"
#include <telux/tel/PhoneFactory.hpp>
#include "telux/common/CommonDefines.hpp"
#include "tafSvcIF.hpp"

using namespace telux::tel;
using namespace telux::common;
using namespace std;

#define MAX_ECALL  1
#define CFG_NODE_ECALL "eCall"
#define CFG_MODEMSERVICE_ECALL_PATH "tafeCallSvc:/eCall"
#define CFG_NODE_MSDMODE "msdtxMode"
#define CFG_NODE_MSDVERSION "msdVersion"
#define CFG_NODE_MSDVEHTYPE "msdVehicleType"
#define CFG_NODE_MSDVIN "msdVehIdentNum"
#define CFG_NODE_OPMODE "operatingMode"
#define CFG_NODE_NUMTYPE "setPsapnumType"
#define CFG_ECALL_PROPULSIONTYPE_PATH "tafeCallSvc:/eCall/msdPropulsionType"
#define CFG_NODE_PROPULSION_GASOLINE "Gasoline"
#define CFG_NODE_PROPULSION_DIESEL "Diesel"
#define CFG_NODE_PROPULSION_NATURALGAS "Naturalgas"
#define CFG_NODE_PROPULSION_PROPANE "Propane"
#define CFG_NODE_PROPULSION_ELECTRIC "Electric"
#define CFG_NODE_PROPULSION_HYDROGEN "Hydrogen"
#define CFG_NODE_PROPULSION_OTHER "Other"
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

namespace telux {
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
            ECALL_ENDED
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
            taf_ecall_CallRef_t                 reference;
            telux::tel::ECallMsdData            msd;
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
        }
        taf_ECall_t;

        typedef struct
        {
            taf_ecall_CallRef_t  eCallRef;
            taf_ecall_State_t    state;
        }StateChangeEvent_t;

        class tafECallOperatingModeCallback {
            public:
                static void setECallOperatingModeResponse(telux::common::ErrorCode error);
                static void getECallOperatingModeResponse(telux::tel::ECallMode eCallMode,
                                             telux::common::ErrorCode error);
        };

        class tafCallCommandCallback : public telux::tel::IMakeCallCallback {
            public:
                void makeCallResponse(telux::common::ErrorCode errorCode,
                                                    std::shared_ptr<telux::tel::ICall> call)override;
                static void makeECallResponse(telux::common::ErrorCode errorCode,
                                                    std::shared_ptr<telux::tel::ICall> call);
        };

        class tafPrieCallCommandCallback : public telux::tel::IMakeCallCallback {
            public:
                static void makeECallResponse(telux::common::ErrorCode errorCode,
                                                    std::shared_ptr<telux::tel::ICall> call);
        };

        class tafUpdateMsdCommandCallback : public telux::common::ICommandResponseCallback {
            public:
                void commandResponse(telux::common::ErrorCode error) override;
        };

        class tafHangupCommandCallback : public telux::common::ICommandResponseCallback {
            public:
                void commandResponse(telux::common::ErrorCode error) override;
        };

        class tafECallListener : public telux::tel::ICallListener {
            void onIncomingCall(std::shared_ptr<telux::tel::ICall> call) override;
            void onCallInfoChange(std::shared_ptr<telux::tel::ICall> call) override;
            void onECallMsdTransmissionStatus(
                    int phoneId, telux::tel::ECallMsdTransmissionStatus msdTransmissionStatus) override;
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
            void onEmergencyNetworkScanFail(int phoneId) override;
#endif
            void onECallHlapTimerEvent(int phoneId, ECallHlapTimerEvents timerEvents) override;
            void OnMsdUpdateRequest(int phoneId);

             taf_ecall_State_t eCallMsdTransmissionStatusToState( ECallMsdTransmissionStatus status);
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
                le_result_t StartECall(ECallCategory emergencyCategory, ECallVariant eCallvariant, taf_ecall_CallRef_t ecallRef);
                le_result_t StartPrivate(taf_ecall_CallRef_t ecallRef, const char * psapNumber, const char * contentType, const char * acceptInfo);
                le_result_t StopECall(taf_ecall_CallRef_t ecallRef);
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
                void SetSessionState(tafECallSession_t session);
                void SetECallState(taf_ecall_State_t state);
                void ClearPduMsd();
                taf_ecall_CallRef_t GetECallReference();
                void SetCallIndex(int32_t callIndex);
                le_event_Id_t StateChangeEventId;

                std::promise<telux::tel::ECallMode> getOpModeProm;
                std::promise<telux::common::ErrorCode> setOpModeProm;
                std::promise<telux::common::ErrorCode> updateMsdProm;
                std::promise<telux::common::ErrorCode> makeEcallProm;
                std::promise<telux::common::ErrorCode> makePrieCallProm;
                CallEndCause CallEndError = telux::tel::CallEndCause::NORMAL;

            private:
                std::shared_ptr<telux::tel::IPhoneManager> PhoneManager;
                std::shared_ptr<telux::tel::ICallManager> CallManager;
                std::shared_ptr<tafECallListener> ECallListener;
                std::shared_ptr<tafCallCommandCallback> CallCommandCb;
                std::shared_ptr<tafUpdateMsdCommandCallback> UpdateMsdCb;
                std::shared_ptr<tafHangupCommandCallback> HangupCb;
                std::vector<std::shared_ptr<telux::tel::IPhone>> Phones;

                taf_ECall_t ECallObject;
                le_ref_MapRef_t ECallPtrRefMap = NULL;
                void InitializeECallPtr();
                bool isUseUSimNum = true;

        };
    }
}

