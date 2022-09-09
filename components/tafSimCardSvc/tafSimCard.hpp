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
#include <telux/tel/PhoneFactory.hpp>
#include "telux/common/CommonDefines.hpp"
#include "tafSvcIF.hpp"

#define DEFAULT_TIMEOUT_IN_SECONDS 5

using namespace telux::tel;
using namespace telux::common;
using namespace std;

namespace telux {
    namespace tafsvc {

        typedef struct
        {
            taf_sim_Id_t      simId;
            taf_sim_States_t  state;
        }
        sim_event_t;

        typedef struct
        {
            taf_sim_Id_t        simId;
            string              ICCID;
        }
        sim_iccid_event_t;

        typedef struct taf_sim_Obj
        {
            taf_sim_Id_t     simId;
            char             ICCID[TAF_SIM_ICCID_BYTES];
            char             IMSI[TAF_SIM_IMSI_BYTES];
            char             phoneNumber[TAF_SIM_PHONE_NUM_MAX_BYTES];
            int32_t          pinTryCount;
            uint32_t         pukTryCount;
        }
        taf_sim_info_t ;

        typedef struct
        {
            taf_sim_Id_t           simId;
            taf_sim_LockResponse_t responseType;
            le_result_t            result;
        }
        sim_response_event_t;

        enum class CardEvent {
            OPEN_LOGICAL_CHANNEL = 1,  /**<  Open Logical channel */
            CLOSE_LOGICAL_CHANNEL = 2, /**<  Close Logical channel*/
            TRANSMIT_APDU_CHANNEL = 3, /**<  Transmit of APDU on channel*/
        };

        class tafCardListener : public telux::tel::ICardListener {
            public:
                void onCardInfoChanged(int slotId) override;
        };

        class tafSubscriptionListener : public telux::tel::ISubscriptionListener {
            public:
                void onSubscriptionInfoChanged(std::shared_ptr<telux::tel::ISubscription> subscription) override;
        };

        class tafOpenLogicalChannelCallback : public ICardChannelCallback {
            public:
                void onChannelResponse(int channel, IccResult result, ErrorCode error) override;
        };

        class tafCloseLogicalChannelCallback : public ICommandResponseCallback {
            public:
                void commandResponse(ErrorCode error) override;
        };

        class tafTransmitApduResponseCallback : public ICardCommandCallback {
            public:
                void onResponse(IccResult result, ErrorCode error) override;
        };

        class tafAuthenticationResponseCallback {
            public:
             static void unlockCardByPinResponseCb(int retryCount, telux::common::ErrorCode error);
            static void ChangeCardPinResponseCb(int retryCount, telux::common::ErrorCode error);
            static void unlockCardByPukResponseCb(int retryCount, telux::common::ErrorCode error);
            static void setCardLockResponseCb(int retryCount, telux::common::ErrorCode error);

        };

        class tafSimProfileCallback {
            public:
            void profileListCallBack(
                    const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
                    telux::common::ErrorCode errorCode);
        };

        class taf_sim :public ITafSvc {
            public:
                void Init(void);
                static taf_sim &GetInstance();
                taf_sim() {};
                ~taf_sim() {};

                std::shared_ptr<telux::tel::ICardManager> cardManager;
                std::shared_ptr<telux::tel::ICardListener> cardListener;
                std::vector<std::shared_ptr<telux::tel::ICard>> cards;
                std::shared_ptr<telux::tel::ISubscriptionManager> subMgr;
                std::shared_ptr<telux::tel::ISubscriptionListener> subscriptionListener;
                std::shared_ptr<telux::tel::ISimProfileManager> simProfileManager;
                std::promise<le_result_t> ProfileSyncPromise = std::promise<le_result_t>();

                int slot = DEFAULT_SLOT_ID;
                std::condition_variable eventCV;
                CardEvent cardEventExpected;
                std::mutex eventMutex;
                ErrorCode errorCode;
                uint8_t openChannel = 0;
                IccResult apduResponse;
                bool isEcs=false;

                le_event_Id_t NewStateEventId;
                le_event_Id_t ResponseEventId;
                le_event_Id_t ProfileListEventId;
                le_event_Id_t IccidChangeEventId;
                bool EnableAutoSelection = false;

                void RemoveStateHandler(taf_sim_NewStateHandlerRef_t handlerRef);
                taf_sim_States_t getState(taf_sim_Id_t simId);
                const char* cardStateToString(CardState state);
                const char* statusToString(telux::common::Status status);
                taf_sim_States_t cardStateToTafSimStates(CardState state);
                static void FirstLayerNewSimStateHandler(void* reportPtr, void* secondLayerHandlerFunc);
                taf_sim_NewStateHandlerRef_t AddStateHandler(taf_sim_NewStateHandlerFunc_t handlerPtr,
                        void* contextPtr);
                bool isValidSimId(taf_sim_Id_t simId);
                le_result_t selectSimSlot(taf_sim_Id_t simId);
                taf_sim_info_t* GetSimContext(taf_sim_Id_t simId);
                void InitializeSimInfo(std::shared_ptr<telux::tel::ISubscription> subscription, taf_sim_Id_t simId);
                le_result_t getICCID(taf_sim_Id_t simId, char *iccid, int length);
                le_result_t getSubscriberPhoneNumber(taf_sim_Id_t simId, char *phoneNumber, int length);
                le_result_t getIMSI(taf_sim_Id_t simId, char *imsi, int length);
                le_result_t getHomeNetworkOperator(taf_sim_Id_t simId, char *namePtr, int length);
                le_result_t getHomeNetworkMccMnc(taf_sim_Id_t simId, char *mccPtr,
                        int mccPtrSize, char *mncPtr, int mncPtrSize);

                le_result_t UnlockCardByPin(taf_sim_Id_t  simId, taf_sim_LockType_t lockType, const char* pinPtr);
                le_result_t ChangeCardPin( taf_sim_Id_t simId, taf_sim_LockType_t lockType, const char* oldpinPtr,
                        const char*   newpinPtr);
                le_result_t UnlockCardByPuk(taf_sim_Id_t  simId, taf_sim_LockType_t lockType, const char* pukPtr,
                        const char* newpinPtr);
                le_result_t SetCardLock(taf_sim_Id_t  simId, taf_sim_LockType_t lockType, const char* pinPtr,
                        bool lockEnable);
                int32_t GetRemainingPINTries(taf_sim_Id_t simId);
                le_result_t GetRemainingPukTries(taf_sim_Id_t simId, uint32_t* remainingPukTriesPtr);

                static void FirstLayerAuthenticationResponseHandler(void* reportPtr, void* secondLayerHandlerFunc);
                taf_sim_AuthenticationResponseHandlerRef_t AddAuthenticationResponseHandler(
                        taf_sim_AuthenticationResponseHandlerFunc_t handlerPtr, void* contextPtr);
                void RemoveAuthenticationResponseHandler(taf_sim_AuthenticationResponseHandlerRef_t handlerRef);

                le_result_t GetEID( taf_sim_Id_t slotId, char* eidPtr, size_t eidLen);
                le_result_t SetAutomaticSelection( bool enable);
                le_result_t GetAutomaticSelection( bool* enablePtr);
                bool waitForCardEvent(CardEvent cardEvent, int timeout = DEFAULT_TIMEOUT_IN_SECONDS);
                le_result_t OpenLogicalChannel(taf_sim_Id_t slotId, taf_sim_AppType_t appType, uint8_t* channelPtr);
                le_result_t CloseLogicalChannel( taf_sim_Id_t simId, uint8_t channel);
                le_result_t SendApduOnChannel( taf_sim_Id_t simId, uint8_t channel,
                        const uint8_t* commandApduPtr, size_t commandApduNumElements,
                        uint8_t* responseApduPtr,size_t* responseApduNumElementsPtr);
                le_result_t SendApdu( taf_sim_Id_t simId,const uint8_t* commandApduPtr, size_t commandApduNumElements,
                        uint8_t* responseApduPtr,size_t* responseApduNumElementsPtr);
                le_result_t SendCommand(taf_sim_Id_t simId, taf_sim_Command_t command,
                        const char* fileIdentifierPtr,
                        uint8_t *p1, uint8_t *p2,uint8_t *p3, const uint8_t* dataPtr,
                        size_t dataNumElements, const char* pathPtr, uint8_t* sw1,uint8_t* sw2,
                        uint8_t* responsePtr, size_t* responseNumElementsPtr);
                le_result_t SetPower( taf_sim_Id_t simId, le_onoff_t powerState);
                le_result_t Reset(taf_sim_Id_t simId);
                le_result_t IsEmergencyCallSubscriptionSelected (taf_sim_Id_t simId, bool* isEcs);
                le_result_t LocalSwapToEmergencyCallSubscription(taf_sim_Id_t simId);
                le_result_t LocalSwapToCommercialCallSubscription(taf_sim_Id_t simId);
                le_result_t profileListCallbackEm(
                        const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
                        telux::common::ErrorCode error,
                        SlotId simId);
                le_result_t profileListCallbackCo(
                        const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
                        telux::common::ErrorCode error,
                        SlotId simId);
                void RemoveIccidChangeHandler(taf_sim_IccidChangeHandlerRef_t);
                taf_sim_IccidChangeHandlerRef_t AddIccidChangeHandler(taf_sim_IccidChangeHandlerFunc_t handlerPtr);
                static void FirstLayerIccidChangeHandler(void* reportPtr, void* secondLayerHandlerFunc);
        };
    }
}
