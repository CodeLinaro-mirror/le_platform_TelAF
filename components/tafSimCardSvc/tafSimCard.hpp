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
 *  Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
*/



#include "legato.h"
#include "interfaces.h"
#include <telux/tel/PhoneFactory.hpp>
#include "telux/common/CommonDefines.hpp"
#include "tafSvcIF.hpp"
#include "taf_pa_sim.hpp"

#define DEFAULT_TIMEOUT_IN_SECONDS 10
#define TAF_SIM_SUBSYSTEM_TIMEOUT 30

using namespace telux::tel;
using namespace telux::common;
using namespace std;

    namespace tafsvc {

        typedef struct FPLMNNode
        {
            char              mcc[4];
            char              mnc[4];
            le_dls_Link_t     link;
        }FPLMNNode_t;

        typedef struct taf_sim_FPLMNList
        {
            taf_sim_FPLMNListRef_t ref;
            le_dls_List_t          link;
        }taf_sim_FPLMNList_t;

        typedef struct taf_sim_Session
        {
            taf_sim_RefreshRef_t ref;
            le_dls_List_t link;
            taf_sim_SessionType_t sessionType;
            le_msg_SessionRef_t clientSessionRef;
            taf_sim_RefreshMode_t refreshMode;
            taf_pa_sim_RefreshChangeHandlerRef_t paHandlerRef;
            le_event_Id_t RefreshChangeEventId;
            bool refreshAllow;
            char simProfileIccid1[TAF_SIM_ICCID_BYTES];
            char simProfileIccid2[TAF_SIM_ICCID_BYTES];
            bool refreshResetStart;
            size_t refreshRegFilesSize;
            taf_sim_RefreshRegFile_t refreshRegFiles[TAF_SIM_MAX_SIM_REFRESH_FILES];
            le_sem_Ref_t semaphore;
        }taf_sim_Session_t;

        typedef struct
        {
            taf_sim_Id_t      simId;
            taf_sim_States_t  state;
        }
        sim_event_t;

        typedef struct
        {
            taf_sim_Id_t        simId;
            char                ICCID[TAF_SIM_ICCID_BYTES];
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

        typedef struct
        {
            taf_sim_RefreshStatus_t     refreshStatus;
        }
        sim_refresh_event_t;

        enum class CardEvent {
            OPEN_LOGICAL_CHANNEL = 1,  /**<  Open Logical channel */
            CLOSE_LOGICAL_CHANNEL = 2, /**<  Close Logical channel*/
            TRANSMIT_APDU_CHANNEL = 3, /**<  Transmit of APDU on channel*/
        };

        class Utility{
            public:
                class Convert
                {
                    public:
                        static le_result_t Result(int32_t result);
                        static taf_pa_common_LogLevel_t Level(le_log_Level_t level);
                };
        };

        class tafCardListener : public telux::tel::ICardListener {
            public:
                void onCardInfoChanged(int slotId) override;
        };

        class tafSubscriptionListener : public telux::tel::ISubscriptionListener {
            public:
                void onSubscriptionInfoChanged(std::shared_ptr<telux::tel::ISubscription> subscription) override;
        };

        class tafMultiSimListener : public telux::tel::IMultiSimListener {
            public:
                void onSlotStatusChanged(
                std::map<SlotId, telux::tel::SlotStatus> slotStatus) override;
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

        class tafMultiSimCallback {
            public:
                static void requestsSlotsStatusResponse(std::map<SlotId,
                                            telux::tel::SlotStatus> slotStatus,
                                            telux::common::ErrorCode error);
        };

        class tafAuthenticationResponseCallback {
            public:
             static void unlockCardByPinResponseCb(int retryCount, telux::common::ErrorCode error);
            static void ChangeCardPinResponseCb(int retryCount, telux::common::ErrorCode error);
            static void unlockCardByPukResponseCb(int retryCount, telux::common::ErrorCode error);
            static void setCardLockResponseCb(int retryCount, telux::common::ErrorCode error);

        };
        class taf_sim :public ITafSvc {
            private:
                le_mem_PoolRef_t FPLMNNodePool = NULL;
                le_mem_PoolRef_t FPLMNListPool = NULL;
                le_ref_MapRef_t FPLMNListRefMap;
                le_mem_PoolRef_t SessionPool = NULL;
                le_ref_MapRef_t SessionRefMap;
                le_result_t AddFPLMNOperatorInternal(taf_sim_FPLMNListRef_t FPLMNListRef, char* mccPtr, char* mncPtr);
                taf_sim_FPLMNListRef_t CreateInternalFPLMNList();
                taf_sim_RefreshStatus_t ConvertPaRefreshStageToTafRefreshStatus(taf_pa_sim_RefreshStage_t refreshStage);
                taf_pa_sim_SessionType_t ConvertTafSessionTypeToPaSessionType(taf_sim_SessionType_t sessionType);

            public:
                void Init(void);
                static taf_sim &GetInstance();
                taf_sim() {};
                ~taf_sim() {};

                std::shared_ptr<telux::tel::ICardManager> cardManager = nullptr;
                std::shared_ptr<telux::tel::ICardListener> cardListener;
                std::map<int, std::shared_ptr<telux::tel::ICard>> cards;
                std::shared_ptr<telux::tel::ISubscriptionManager> subMgr = nullptr;
                std::shared_ptr<telux::tel::ISubscriptionListener> subscriptionListener;
                std::shared_ptr<telux::tel::IMultiSimManager> multiSimMgr = nullptr;
                std::shared_ptr<telux::tel::IMultiSimListener> multiSimListener;

                int slot = DEFAULT_SLOT_ID;
                int slotCount = 0;
                bool isSingleActive = false;
                std::condition_variable eventCV;
                CardEvent cardEventExpected;
                std::mutex eventMutex;
                ErrorCode errorCode;
                uint8_t openChannel = 0;
                IccResult apduResponse;
                bool cardRespReceived = false;
                bool isEcs=false;
                int32_t mClientRefCount;
                le_event_Id_t NewStateEventId;
                le_event_Id_t ResponseEventId;
                le_event_Id_t IccidChangeEventId;
                bool EnableAutoSelection = false;
                bool IsPsEventInProgress = false;
                bool RefreshVoteSent_Slot1 = false;
                bool RefreshVoteSent_Slot2 = false;
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
                static taf_sim_Session_t* DiscoverSessionRef(taf_sim_RefreshRef_t sessionRef);
                void InitializeSimInfo(std::shared_ptr<telux::tel::ISubscription> subscription, taf_sim_Id_t simId);
                std::shared_ptr<telux::tel::ISubscription> getSubscription(taf_sim_Id_t simId);
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
                le_result_t GetAppTypes(taf_sim_Id_t slotId, taf_sim_AppType_t* appTypePtr, size_t* appTypeNumElementsPtr);
                le_result_t OpenLogicalChannel(taf_sim_Id_t slotId, taf_sim_AppType_t appType, uint8_t* channelPtr);
                le_result_t OpenLogicalChannelByAid(taf_sim_Id_t slotId, const char* aid, uint8_t* channel);
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
                void requestsSlotsStatusResponse(std::map<SlotId,telux::tel::SlotStatus> slotStatus,telux::common::ErrorCode error);
                le_result_t IsEmergencyCallSubscriptionSelected (taf_sim_Id_t simId, bool* isEcs);
                le_result_t LocalSwapToEmergencyCallSubscription(taf_sim_Id_t simId, taf_sim_Manufacturer_t manufacturer);
                le_result_t LocalSwapToCommercialCallSubscription(taf_sim_Id_t simId, taf_sim_Manufacturer_t manufacturer);
                le_result_t MapSimIdToPaSlot(taf_sim_Id_t simId, taf_pa_sim_SlotId_t* slotOutPtr);
                bool FindProfileByType(taf_pa_sim_SlotId_t paSlot,
                        taf_pa_sim_ProfileType_t wantType,
                        taf_pa_sim_ProfileInfo_t* outInfo);
                void RemoveIccidChangeHandler(taf_sim_IccidChangeHandlerRef_t);
                taf_sim_IccidChangeHandlerRef_t AddIccidChangeHandler(taf_sim_IccidChangeHandlerFunc_t handlerPtr);
                static void FirstLayerIccidChangeHandler(void* reportPtr, void* secondLayerHandlerFunc);
                taf_sim_FPLMNListRef_t CreateFPLMNList();
                taf_sim_FPLMNListRef_t ReadFPLMNList(taf_sim_Id_t simId);
                le_result_t AddFPLMNOperator(taf_sim_FPLMNListRef_t FPLMNListRef, char* mccPtr, char* mncPtr);
                le_result_t GetFirstFPLMNOperator(taf_sim_FPLMNListRef_t FPLMNListRef, char* mccPtr, size_t mccLen, char* mncPtr, size_t mncLen);
                le_result_t GetNextFPLMNOperator(taf_sim_FPLMNListRef_t FPLMNListRef, char* mccPtr, size_t mccLen, char* mncPtr, size_t mncLen);
                void DeleteFPLMNList(taf_sim_FPLMNListRef_t FPLMNListRef);
                le_result_t WriteFPLMNList(taf_sim_Id_t simId, taf_sim_FPLMNListRef_t FPLMNListRef);
                le_result_t getSlotCount(int *count);
                static void FirstLayerNewRefreshChangeHandler(void* reportPtr, void* secondLayerHandlerFunc);
                taf_sim_RefreshChangeHandlerRef_t AddRefreshChangeHandler(taf_sim_RefreshChangeHandlerFunc_t handlerPtr, void* contextPtr);
                void RemoveRefreshChangeHandler(taf_sim_RefreshChangeHandlerRef_t handlerRef);
                void NotifyRefreshEvent(taf_pa_sim_RefreshChangeInd_t* ind, void* contextPtr);
                void CheckAndSendProfileSwitchEvent();
                void CheckAndSendRefreshEvent(taf_sim_Id_t SimId);
                le_result_t CreateSession(taf_sim_SessionType_t sessionType, taf_sim_RefreshRef_t* refreshSessionRef);
                le_result_t SetRefreshRegisterFiles(taf_sim_RefreshRef_t refreshSessionRef, const taf_sim_RefreshRegFile_t* filesPtr, size_t filesSize);
                le_result_t SetRefreshMode(taf_sim_RefreshRef_t refreshSessionRef, taf_sim_RefreshMode_t refreshMode);
                le_result_t SetRefreshAllow(taf_sim_RefreshRef_t refreshSessionRef, bool isRefreshAllowed);
                le_result_t CheckRefreshAllow(taf_pa_sim_RefreshChangeInd_t* ind);
                void ResetRefreshVote(taf_sim_Session_t* sessionPtr);
                bool IsValidMCCAndMNC(const char* mccPtr, const char* mncPtr);
                le_result_t SwapSubscriptionInternal(taf_sim_Id_t simId, taf_sim_Manufacturer_t manufacturer ,bool toEmergency);
        };
    }
