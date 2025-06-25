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
#include "telux/tel/PhoneFactory.hpp"
#include "tafSimCard.hpp"
#include <unistd.h>

using namespace telux::tel;
using namespace telux::common;
using namespace tafsvc;
using namespace std;

static taf_sim_info_t simList[TAF_SIM_ID_MAX];
static int fplmnListIndex = 0;
static taf_sim_FPLMNListRef_t fplmnListRefs = nullptr;

void tafCardListener:: onCardInfoChanged(int slotId)
{
    LE_INFO("Card info changed for slot: %d", slotId);
    auto &sim = taf_sim::GetInstance();
    sim_event_t simEvent;
    auto slotWithCard = slotId;
    if(sim.cards[slotWithCard] == nullptr && slotWithCard == DEFAULT_SLOT_ID){
        // Map sdk logical slot to physical slot.
        slotWithCard = SLOT_ID_2;
    }
    simEvent.simId = (taf_sim_Id_t)slotWithCard;
    simEvent.state =  sim.getState((taf_sim_Id_t)slotWithCard);
    if (simEvent.state == TAF_SIM_ABSENT) {
        sim.InitializeSimInfo(nullptr, (taf_sim_Id_t)slotWithCard);
    }
    le_event_Report(sim.NewStateEventId, &simEvent, sizeof(simEvent));
    if(simEvent.state == TAF_SIM_PRESENT) {
        sim.CheckAndSendRefreshEvent((taf_sim_Id_t)simEvent.simId);
    }
}

void tafSubscriptionListener:: onSubscriptionInfoChanged
                         (std::shared_ptr<telux::tel::ISubscription> subscription) {
    LE_INFO("onSubscriptionInfoChanged");
    auto &sim = taf_sim::GetInstance();
    taf_sim_info_t* simPtr = NULL;
    telux::common::Status status;
    if(subscription) {
        auto slotWithCard = subscription->getSlotId();
        if(sim.cards[slotWithCard] == nullptr && slotWithCard == DEFAULT_SLOT_ID){
            slotWithCard = SLOT_ID_2;
            auto card = sim.cardManager->getCard(DEFAULT_SLOT_ID, &status);
            LE_INFO("Update cards[%d] as %s", slotWithCard, card == nullptr ? "null" : "non-null");
            sim.cards[slotWithCard] = card;
        }
        sim.InitializeSimInfo(subscription,(taf_sim_Id_t)slotWithCard);
        simPtr = sim.GetSimContext((taf_sim_Id_t)slotWithCard);
    } else {
        LE_INFO("Subscription is empty");
        sim.InitializeSimInfo(nullptr, (taf_sim_Id_t)sim.slot);
        simPtr = sim.GetSimContext((taf_sim_Id_t)sim.slot);
    }
    sim_iccid_event_t simIccidEvent;
    simIccidEvent.simId = (taf_sim_Id_t)simPtr->simId;
    simIccidEvent.ICCID = simPtr->ICCID;
    le_event_Report(sim.IccidChangeEventId, &simIccidEvent, sizeof(simIccidEvent));
    if(!sim.isPsEventInProgress) {

        sim.isPsEventInProgress = true;
        sim.CheckAndSendProfileSwitchEvent();
    }
}

void tafMultiSimListener:: onSlotStatusChanged(std::map<SlotId, telux::tel::SlotStatus> slotStatus) {
    LE_INFO("onSlotStatusChanged: %" PRIuS, slotStatus.size());
    auto &sim = taf_sim::GetInstance();
    int activeSlotCount = 0;
    telux::common::Status status;
    int activeSlots = 0;

    for(auto it = slotStatus.begin(); it != slotStatus.end(); ++it) {
        auto slotStatus = it->second;
        if (slotStatus.slotState == telux::tel::SlotState::ACTIVE) {
            activeSlots++;
        }
    }

    sim.cardManager->getSlotCount(activeSlotCount);
    if(activeSlotCount == 1){
        sim.isSingleActive = true;
    }

    LE_INFO("activeSlotCount: %d, isSingleActive: %d", activeSlots, (int) sim.isSingleActive);

    for(auto it = slotStatus.begin(); it != slotStatus.end(); ++it) {
        auto slotId = it->first;
        auto slotStatus = it->second;

        LE_INFO("Slot: %d, slotState: %d, cardState: %d, cardError: %d", slotId,
                (int) slotStatus.slotState, (int) slotStatus.cardState,
                (int) slotStatus.cardError);

        if(activeSlots == 1) { //Single Active slot
            if (slotStatus.slotState == telux::tel::SlotState::ACTIVE) {
                sim.slot = slotId;
            }
            if (slotStatus.cardState != telux::tel::CardState::CARDSTATE_UNKNOWN
                    && slotStatus.cardState != telux::tel::CardState::CARDSTATE_ABSENT) {
                LE_INFO("Find card for single active in slot: %d", slotId);
                auto card = sim.cardManager->getCard(DEFAULT_SLOT_ID, &status);
                sim.cards[slotId] = card;
                LE_INFO("Put card as %s in cards[%d]", card == nullptr ? "null" : "non-null", slotId);
            } else {
                sim.cards[slotId] = nullptr;
                LE_INFO("Update card as null in cards[%d]", slotId);
            }
        } else if((slotStatus.slotState == telux::tel::SlotState::ACTIVE)
                && (activeSlots == 2)){
            LE_INFO("Find card for dual active in slot: %d", slotId);
            auto card = sim.cardManager->getCard(slotId, &status);
            sim.cards[slotId] = card;
        } else {
            LE_INFO("Find no card for slot: %d", slotId);
            sim.cards[slotId] = nullptr;
        }
    }
}

void tafMultiSimCallback::requestsSlotsStatusResponse(std::map<SlotId,
         telux::tel::SlotStatus> slotStatus, telux::common::ErrorCode error) {
    auto &sim = taf_sim::GetInstance();
    if(error == telux::common::ErrorCode::SUCCESS) {
        sim.slotCount = slotStatus.size();
        LE_INFO("requestsSlotsStatusResponse: slotCount: %d", sim.slotCount);
        telux::common::Status status;
        int activeSlots = 0;

        for(auto it = slotStatus.begin(); it != slotStatus.end(); ++it) {
            auto slotStatus = it->second;
            if (slotStatus.slotState == telux::tel::SlotState::ACTIVE) {
                activeSlots++;
            }
        }
        int activeSlotCount = 0;
        sim.cardManager->getSlotCount(activeSlotCount);
        if(activeSlotCount == 1){
            sim.isSingleActive = true;
        }

        LE_INFO("activeSlotCount: %d, isSingleActive: %d", activeSlots, (int) sim.isSingleActive);

        for(auto it = slotStatus.begin(); it != slotStatus.end(); ++it) {
            auto slotId = it->first;
            auto slotStatus = it->second;
            LE_INFO("Slot: %d, slotState: %d, cardState: %d, cardError: %d", slotId,
                    (int) slotStatus.slotState, (int) slotStatus.cardState,
                    (int) slotStatus.cardError);
            if(activeSlots == 1) { //Single Active slot
                if (slotStatus.slotState == telux::tel::SlotState::ACTIVE) {
                    sim.slot = slotId;
                }
                if (slotStatus.cardState != telux::tel::CardState::CARDSTATE_UNKNOWN
                        && slotStatus.cardState != telux::tel::CardState::CARDSTATE_ABSENT) {
                    LE_INFO("Find card for single active in slot: %d", slotId);
                    auto card = sim.cardManager->getCard(DEFAULT_SLOT_ID, &status);
                    sim.cards[slotId] = card;
                    LE_INFO("Put card as %s in cards[%d]",
                            card == nullptr ? "null" : "non-null", slotId);
                } else {
                    sim.cards[slotId] = nullptr;
                    LE_INFO("Update card as null in cards[%d]", slotId);
                }
            } else if((slotStatus.slotState == telux::tel::SlotState::ACTIVE)
                    && (activeSlots == 2)){
                LE_INFO("Find card for dual active in slot: %d", slotId);
                auto card = sim.cardManager->getCard(slotId, &status);
                sim.cards.emplace(slotId, card);
            } else {
                LE_INFO("Find no card for slot: %d", slotId);
                sim.cards.emplace(slotId, nullptr);
            }
        }
     } else {
         LE_INFO("Request slot status failed with error %d", static_cast<int>(error));
     }
     sim.slotStatusCbPromise.set_value(error);
}

void tafAuthenticationResponseCallback:: ChangeCardPinResponseCb(int retryCount, telux::common::ErrorCode error) {
    auto &sim = taf_sim::GetInstance();
    taf_sim_info_t* simPtr = sim.GetSimContext((taf_sim_Id_t)sim.slot);
    sim_response_event_t simResponsePtr;
    simResponsePtr.simId = (taf_sim_Id_t) sim.slot;
    simResponsePtr.responseType = TAF_SIM_CHANGE_PIN;
    if(error != telux::common::ErrorCode::SUCCESS) {
        LE_INFO("Change Card Pin Request failed with errorCode: %d",(int) error);
        LE_INFO("Change Card Pin Request failed retryCount:%d",retryCount);
        simPtr->pinTryCount = retryCount;
        simResponsePtr.result = LE_FAULT ;
    } else {
        LE_INFO("Change Card Pin Request successful retryCount:%d",retryCount);
        simPtr->pinTryCount = retryCount;
        simResponsePtr.result = LE_OK;
    }
    le_event_Report(sim.ResponseEventId, &simResponsePtr,sizeof(simResponsePtr));
}

void tafAuthenticationResponseCallback:: unlockCardByPukResponseCb(int retryCount, telux::common::ErrorCode error) {
    auto &sim = taf_sim::GetInstance();
    taf_sim_info_t* simPtr = sim.GetSimContext((taf_sim_Id_t)sim.slot);
    sim_response_event_t simResponsePtr;
    simResponsePtr.simId = (taf_sim_Id_t) sim.slot;
    simResponsePtr.responseType = TAF_SIM_UNLOCK_BY_PUK;

    if(error != telux::common::ErrorCode::SUCCESS) {
        LE_INFO("Unlock Card By Puk Request failed with errorCode:%d ",(int)error);
        LE_INFO("Unlock Card By Puk request failed retryCount:%d",retryCount);
        simPtr->pukTryCount = retryCount;
        simResponsePtr.result = LE_FAULT ;
    } else {
        LE_INFO("Unlock Card By Puk request successful retryCount:%d",retryCount);
        simPtr->pinTryCount = 3;
        simPtr->pukTryCount = 10;
        simResponsePtr.result = LE_OK;
    }
    le_event_Report(sim.ResponseEventId, &simResponsePtr,sizeof(simResponsePtr));
}

void tafAuthenticationResponseCallback:: unlockCardByPinResponseCb(int retryCount, telux::common::ErrorCode error) {
    auto &sim = taf_sim::GetInstance();
    taf_sim_info_t* simPtr = sim.GetSimContext((taf_sim_Id_t)sim.slot);
    sim_response_event_t simResponsePtr;
    simResponsePtr.simId = (taf_sim_Id_t) sim.slot;
    simResponsePtr.responseType = TAF_SIM_UNLOCK_BY_PIN;
    if(error != telux::common::ErrorCode::SUCCESS) {
        simPtr->pinTryCount = retryCount;
        LE_INFO("Unlock Card By Pin Request failed with errorCode: %d ",(int)error);
        LE_INFO( "Unlock Card By Pin Request failed retryCount: %d",retryCount);
        simResponsePtr.result = LE_FAULT ;
    } else {
        simPtr->pinTryCount = retryCount;
        simResponsePtr.result = LE_OK;
        LE_INFO( "Unlock Card By Pin Request successful retryCount: %d",retryCount);
    }
    le_event_Report(sim.ResponseEventId, &simResponsePtr,sizeof(simResponsePtr));
}

void tafAuthenticationResponseCallback::setCardLockResponseCb(int retryCount, telux::common::ErrorCode error) {
    auto &sim = taf_sim::GetInstance();
    sim_response_event_t simResponsePtr;
    simResponsePtr.simId = (taf_sim_Id_t) sim.slot;
    simResponsePtr.responseType = TAF_SIM_SET_LOCK;
    taf_sim_info_t* simPtr = sim.GetSimContext((taf_sim_Id_t)sim.slot);
    if(error != telux::common::ErrorCode::SUCCESS) {
        LE_INFO("Set card lock Request failed with errorCode: %d ",(int)error);
        LE_INFO( "Set card lock Request failed retryCount: %d",retryCount);
        simPtr->pinTryCount = retryCount;
        simResponsePtr.result = LE_FAULT ;
    } else {
        LE_INFO( "Set card lock Request successful retryCount: %d",retryCount);
        simPtr->pinTryCount = retryCount;
        simResponsePtr.result = LE_OK;
    }
    le_event_Report(sim.ResponseEventId, &simResponsePtr,sizeof(simResponsePtr));
}

void tafOpenLogicalChannelCallback::onChannelResponse(int channel, IccResult result,
                                                     ErrorCode error) {
    auto &sim = taf_sim::GetInstance();
   std::unique_lock<std::mutex> lock(sim.eventMutex);
   sim.errorCode = error;
   sim.openChannel = (uint8_t)channel;
   if(sim.cardEventExpected == CardEvent::OPEN_LOGICAL_CHANNEL) {
       LE_INFO("OpenLogicalChannel callback response sw1: %d, sw2: %d", (uint8_t)result.sw1, (uint8_t)result.sw2);
       if(error == telux::common::ErrorCode::SUCCESS && (uint8_t)result.sw1 == 0x90 && (uint8_t)result.sw2 == 0x00) {
           LE_INFO("OpenLogicalChannel successful channel = %d", channel);
       } else {
           sim.errorCode = telux::common::ErrorCode::SIM_BUSY;
           LE_INFO("OpenLogicalChannel failed");
       }
       LE_INFO("Card Event OPEN_LOGICAL_CHANNEL found with code : %d", int(error));
       sim.eventCV.notify_one();
   }
}


void tafCloseLogicalChannelCallback::commandResponse(telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      LE_INFO("onCloseLogicalChannel successful.");
   } else {
      LE_INFO( "onCloseLogicalChannel failed\n error: %d ", static_cast<int>(error));
   }
   auto &sim = taf_sim::GetInstance();
   std::unique_lock<std::mutex> lock(sim.eventMutex);
   sim.errorCode = error;
   if(sim.cardEventExpected == CardEvent::CLOSE_LOGICAL_CHANNEL) {
      LE_INFO("Card Event CLOSE_LOGICAL_CHANNEL found with code : %d", int(error));
      sim.eventCV.notify_one();
   }
}

void tafTransmitApduResponseCallback::onResponse(IccResult result, ErrorCode error) {
   LE_INFO("onResponse, error: %d ",(int)error);
   auto &sim = taf_sim::GetInstance();
   std::unique_lock<std::mutex> lock(sim.eventMutex);
   sim.errorCode = error;
   sim.apduResponse = result;
   LE_INFO("onResponse: %s " , result.toString().c_str());
   if(sim.cardEventExpected == CardEvent::TRANSMIT_APDU_CHANNEL) {
      LE_INFO("Card Event TRANSMIT_APDU_CHANNEL found with code : %d", int(error));
      sim.eventCV.notify_one();
   }
}

//--------------------------------------------------------------------------------------------------
/**
 * Register listeners.
 */
//--------------------------------------------------------------------------------------------------
void RegisterListeners()
{
    auto &sim = taf_sim::GetInstance();
    telux::common::Status status;

    LE_DEBUG("RegisterListeners");
    if ((sim.multiSimMgr != nullptr) && (sim.multiSimListener != nullptr))
    {
        status = sim.multiSimMgr->registerListener(sim.multiSimListener);
        if (status != telux::common::Status::SUCCESS)
        {
            LE_ERROR("Fail to register multi sim listener.");
        }
    }

    if ((sim.cardManager != nullptr) && (sim.cardListener != nullptr))
    {
        status = sim.cardManager->registerListener(sim.cardListener);
        if (status != telux::common::Status::SUCCESS)
        {
            LE_ERROR("Fail to register card listener.");
        }
    }

    if ((sim.subMgr != nullptr) && (sim.subscriptionListener != nullptr))
    {
        status = sim.subMgr->registerListener(sim.subscriptionListener);
        if (status != telux::common::Status::SUCCESS)
        {
            LE_ERROR("Fail to register subscription listener.");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Deregister listeners.
 */
//--------------------------------------------------------------------------------------------------
void DeregisterListeners()
{
    auto &sim = taf_sim::GetInstance();
    telux::common::Status status;

    LE_DEBUG("RegisterListeners");
    if ((sim.multiSimMgr != nullptr) && (sim.multiSimListener != nullptr))
    {
        status = sim.multiSimMgr->deregisterListener(sim.multiSimListener);
        if (status != telux::common::Status::SUCCESS)
        {
            LE_ERROR("Fail to deregister multi sim listener.");
        }
    }

    if ((sim.cardManager != nullptr) && (sim.cardListener != nullptr))
    {
        status = sim.cardManager->removeListener(sim.cardListener);
        if (status != telux::common::Status::SUCCESS)
        {
            LE_ERROR("Fail to deregister card delistener.");
        }
    }

    if ((sim.subMgr != nullptr) && (sim.subscriptionListener != nullptr))
    {
        status = sim.subMgr->removeListener(sim.subscriptionListener);
        if (status != telux::common::Status::SUCCESS)
        {
            LE_ERROR("Fail to deregister subscription listener.");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for power state changes
 */
//--------------------------------------------------------------------------------------------------
void PowerStateChangeHandler(taf_pm_State_t state, void* contextPtr)
{
    if (state == TAF_PM_STATE_RESUME)
    {
        LE_INFO("Power state change to RESUME");
        RegisterListeners();
    }
    else if (state == TAF_PM_STATE_SUSPEND)
    {
        LE_INFO("Power state change to SUSPEND");
        DeregisterListeners();
    }
}

void taf_sim::Init(void)
{
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    subMgr = phoneFactory.getSubscriptionManager();
    if (!subMgr)
    {
        LE_FATAL("Failed to get subscription manager.");
    }
    else
    {
        telux::common::ServiceStatus subMgrStatus = subMgr->getServiceStatus();
        if (subMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
        {
            LE_INFO("Subscription subsystem is not ready, waiting for it to be ready...");
            std::promise<telux::common::ServiceStatus> subMgrProm;
            subMgr = phoneFactory.getSubscriptionManager([&](telux::common::ServiceStatus status) {
                LE_INFO("Getting status:%d from subscription manager", (int)status);
                if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    subMgrProm.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                }
                else {
                    subMgrProm.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
                }
            });
            std::future<telux::common::ServiceStatus> initFuture = subMgrProm.get_future();
            std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                TAF_SIM_SUBSYSTEM_TIMEOUT));
            if (std::future_status::timeout == waitStatus)
            {
                LE_FATAL("Timeout waiting for subscription susbsytem");
            }
            else
            {
                subMgrStatus = initFuture.get();
            }
        }
        if (subMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
        {
            LE_INFO("Subscription subsystem is ready.");
        }
        else
        {
            LE_FATAL("Fail to init subscription subsystem");
        }
    }
    cardManager = phoneFactory.getCardManager();
    if (!cardManager)
    {
        LE_FATAL("Failed to get card manager.");
    }
    else
    {
        telux::common::ServiceStatus cardMgrStatus = cardManager->getServiceStatus();
        if (cardMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
        {
            LE_INFO("Card subsystem is not ready, waiting for it to be ready...");
            std::promise<telux::common::ServiceStatus> cardMgrProm;
            cardManager = phoneFactory.getCardManager([&](telux::common::ServiceStatus status) {
                LE_INFO("Getting status:%d from card manager", (int)status);
                if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    cardMgrProm.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                } else {
                    cardMgrProm.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
                }
            });
            std::future<telux::common::ServiceStatus> initFuture = cardMgrProm.get_future();
            std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                TAF_SIM_SUBSYSTEM_TIMEOUT));
            if (std::future_status::timeout == waitStatus)
            {
                LE_FATAL ("Timeout waiting for card susbsytem");
            }
            else
            {
                cardMgrStatus = initFuture.get();
            }
        }
        if (cardMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
        {
            LE_INFO("Card subsystem is ready.");
        }
        else
        {
            LE_FATAL("Fail to init card subsystem");
        }
    }
    simProfileManager = phoneFactory.getSimProfileManager();
    if (!simProfileManager)
    {
        LE_FATAL("Failed to get sim profile manager.");
    }
    else
    {
        telux::common::ServiceStatus simProfileMgrStatus = simProfileManager->getServiceStatus();
        if (simProfileMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
        {
            LE_INFO("Sim profile subsystem is not ready, waiting for it to be ready...");
            std::promise<telux::common::ServiceStatus> simProfileMgrProm;
            simProfileManager = phoneFactory.getSimProfileManager([&](telux::common::ServiceStatus status) {
                LE_INFO("Getting status:%d from sim profile manager", (int)status);
                if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    simProfileMgrProm.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                }
                else {
                    simProfileMgrProm.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
                }
            });
            std::future<telux::common::ServiceStatus> initFuture = simProfileMgrProm.get_future();
            std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                TAF_SIM_SUBSYSTEM_TIMEOUT));
            if (std::future_status::timeout == waitStatus)
            {
                LE_FATAL ("Timeout waiting for sim profile susbsytem");
            }
            else
            {
                simProfileMgrStatus = initFuture.get();
            }
        }
        if (simProfileMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
        {
            LE_INFO("Sim profile subsystem is ready.");
        }
        else
        {
            LE_FATAL("Fail to init sim profile subsystem");
        }
    }
    multiSimMgr = phoneFactory.getMultiSimManager();
    if (!multiSimMgr)
    {
        LE_FATAL("Failed to get multi sim manager.");
    }
    else
    {
        telux::common::ServiceStatus multiSimMgrStatus = multiSimMgr->getServiceStatus();
        if (multiSimMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
        {
            LE_INFO("Multi sim subsystem is not ready, waiting for it to be ready...");
            std::promise<telux::common::ServiceStatus> multiSimMgrProm;
            multiSimMgr = phoneFactory.getMultiSimManager([&](telux::common::ServiceStatus status) {
                LE_INFO("Getting status:%d from multi sim manager", (int)status);
                if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    multiSimMgrProm.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                } else {
                    multiSimMgrProm.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
                }
            });
            std::future<telux::common::ServiceStatus> initFuture = multiSimMgrProm.get_future();
            std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                TAF_SIM_SUBSYSTEM_TIMEOUT));
            if (std::future_status::timeout == waitStatus)
            {
                LE_FATAL ("Timeout waiting for multi sim susbsytem");
            }
            else
            {
                multiSimMgrStatus = initFuture.get();
            }
        }
        if (multiSimMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
        {
            LE_INFO("Multi sim subsystem is ready.");
            slotStatusCbPromise = std::promise<telux::common::ErrorCode>();
            auto ret = multiSimMgr->requestSlotStatus(tafMultiSimCallback::requestsSlotsStatusResponse);
            if(ret != telux::common::Status::SUCCESS){
                LE_FATAL("Request slot status failed with error: %d", (int)ret);
            }
            telux::common::ErrorCode errorStatus = slotStatusCbPromise.get_future().get();
            if(errorStatus == telux::common::ErrorCode::SUCCESS){
                LE_INFO("Initialize slot card map successfully, default selected slot: %d", (int)slot);
            }
            if(errorStatus != telux::common::ErrorCode::SUCCESS){
                LE_FATAL("Initialize slot card map failed with error: %d", (int)errorStatus);
            }
        }
        else
        {
            LE_FATAL("Fail to init multi sim subsystem");
        }
    }

    FPLMNNodePool = le_mem_CreatePool("FPLMNNodePool", sizeof(FPLMNNode_t));
    le_mem_ExpandPool(FPLMNNodePool, TAF_SIM_FPLMN_MAX_LISTS * TAF_SIM_FPLMN_MAX_OPERATORS_PER_LIST);
    FPLMNListPool = le_mem_CreatePool("FPLMNListPool", sizeof(taf_sim_FPLMNList_t));
    le_mem_ExpandPool(FPLMNListPool, TAF_SIM_FPLMN_MAX_LISTS);
    FPLMNListRefMap = le_ref_CreateMap("FPLMNListRefMap", TAF_SIM_FPLMN_MAX_LISTS);

    SessionPool = le_mem_CreatePool("SessionPool", sizeof(taf_sim_Session_t));
    le_mem_ExpandPool(SessionPool, 12);
    SessionRefMap = le_ref_CreateMap("SessionRefMapRefMap", 10);
    fplmnListIndex = 0;

    subscriptionListener = std::make_shared<tafSubscriptionListener>();
    cardListener = std::make_shared<tafCardListener>();
    multiSimListener = std::make_shared<tafMultiSimListener>();

    NewStateEventId = le_event_CreateId("NewStateEventId", sizeof(sim_event_t));
    ResponseEventId = le_event_CreateId("ResponseEventId", sizeof(sim_response_event_t));
    IccidChangeEventId = le_event_CreateId("IccidChangeEventId", sizeof(sim_iccid_event_t));

    for (auto i = 0; i < TAF_SIM_ID_MAX; i++)
    {
        simList[i].simId = (taf_sim_Id_t)(i + 1);
        simList[i].ICCID[0] = '\0';
        simList[i].IMSI[0] = '\0';
        simList[i].phoneNumber[0] = '\0';
        simList[i].pinTryCount = 3;
        simList[i].pukTryCount = 10;
    }

    taf_pm_AddStateChangeHandler(PowerStateChangeHandler, NULL);
    if (taf_pm_GetPowerState() != TAF_PM_STATE_SUSPEND)
    {
        RegisterListeners();
    }
}

taf_sim &taf_sim::GetInstance()
{
    static taf_sim instance;
    return instance;
}

taf_sim_States_t taf_sim::getState(taf_sim_Id_t simId) {
    LE_INFO("Input sim Id: %d, cards size: %" PRIuS, (int)simId, cards.size());

    if (simId >= TAF_SIM_ID_MAX || simId <= 0) {
        LE_INFO("Invalid sim Id");
        return TAF_SIM_STATE_UNKNOWN;
    }

    if (simId != TAF_SIM_UNSPECIFIED) {
        if (simId > cards.size()) {
            return TAF_SIM_STATE_UNKNOWN;
        }
    }
    if(simId == TAF_SIM_UNSPECIFIED) {
        LE_INFO("Sim Id as Unknown");
        simId = taf_sim_GetSelectedCard();
    }
    auto card = cards[simId];
    telux::tel::CardState cardState = telux::tel::CardState::CARDSTATE_UNKNOWN;
    if(card != nullptr) {
        card->getState(cardState);
        LE_INFO( "CardState : %s\n ",  cardStateToString(cardState)) ;
        if(cardState == telux::tel::CardState::CARDSTATE_PRESENT) {
            std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
            applications = card->getApplications();
            if(applications.size() != 0)  {
                for(auto cardApp : applications) {
                    if(cardApp->getAppType() == telux::tel::AppType::APPTYPE_USIM) {
                        auto appState = cardApp->getAppState();
                        if (appState == telux::tel::AppState::APPSTATE_READY) {
                            return TAF_SIM_READY;
                        } else if (appState == telux::tel::AppState::APPSTATE_ILLEGAL) {
                            return TAF_SIM_ERROR;
                        }
                    }
                }
            }
            return TAF_SIM_PRESENT;
        }
    } else {
        return TAF_SIM_ABSENT;
    }
    return cardStateToTafSimStates(cardState);
}

const char* taf_sim::statusToString(telux::common::Status status) {
    const char *statusString;
    switch(status) {
        case telux::common::Status::SUCCESS:
            statusString = "SUCCESS";
            break;
        case telux::common::Status::FAILED:
            statusString = "FAILED";
            break;
        case telux::common::Status::NOCONNECTION:
            statusString = "NOCONNECTION";
            break;
        case telux::common::Status::NOSUBSCRIPTION:
            statusString = "NOSUBSCRIPTION";
            break;
        case telux::common::Status::INVALIDPARAM:
            statusString = "INVALIDPARAM";
            break;
        case telux::common::Status::INVALIDSTATE:
            statusString = "INVALIDSTATE";
            break;
        case telux::common::Status::NOTREADY:
            statusString = "NOTREADY";
            break;
        case telux::common::Status::NOTALLOWED:
            statusString = "NOTALLOWED";
            break;
        case telux::common::Status::NOTIMPLEMENTED:
            statusString = "NOTIMPLEMENTED";
            break;
        case telux::common::Status::CONNECTIONLOST:
            statusString = "CONNECTIONLOST";
            break;
        case telux::common::Status::EXPIRED:
            statusString = "EXPIRED ";
            break;
        case telux::common::Status::ALREADY:
            statusString = "ALREADY";
            break;
        case telux::common::Status::NOSUCH:
            statusString = "NOSUCH";
            break;
        case telux::common::Status::NOTSUPPORTED:
            statusString = "NOTSUPPORTED";
            break;
        case telux::common::Status::NOMEMORY:
            statusString = "NOMEMORY";
            break;
        default:
            statusString = "Unknown State";
            break;
    }
    return statusString;
}

const char* taf_sim::cardStateToString(CardState state) {
    const char *cardState;
    switch(state) {
        case telux::tel::CardState::CARDSTATE_ABSENT:
            cardState = "Absent";
            break;
        case telux::tel::CardState::CARDSTATE_PRESENT:
            cardState = "Present";
            break;
        case telux::tel::CardState::CARDSTATE_ERROR:
            cardState = "Either error or absent";
            break;
        case telux::tel::CardState::CARDSTATE_RESTRICTED:
            cardState = "Restricted";
            break;
        default:
            cardState = "Unknown card state";
            break;
    }
    return cardState;
}

taf_sim_States_t taf_sim::cardStateToTafSimStates(CardState state) {
    taf_sim_States_t simState;
    switch(state) {
        case telux::tel::CardState::CARDSTATE_ABSENT:
            simState = TAF_SIM_ABSENT;
            break;
        case telux::tel::CardState::CARDSTATE_ERROR:
            simState = TAF_SIM_ERROR;
            break;
        case telux::tel::CardState::CARDSTATE_RESTRICTED:
            simState = TAF_SIM_RESTRICTED;
            break;
        default:
            simState = TAF_SIM_STATE_UNKNOWN;
            break;
    }
    return simState;
}

taf_sim_NewStateHandlerRef_t taf_sim::AddStateHandler(taf_sim_NewStateHandlerFunc_t handlerPtr,
        void* contextPtr){

    le_event_HandlerRef_t handlerRef;
    LE_INFO("Add New State Handler");

    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler pointer is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("NewStateHandler", NewStateEventId,
            FirstLayerNewSimStateHandler, (void*)handlerPtr);

    return (taf_sim_NewStateHandlerRef_t)(handlerRef);

}

void taf_sim::RemoveStateHandler(taf_sim_NewStateHandlerRef_t handlerRef) {
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

void taf_sim::FirstLayerNewSimStateHandler(void* reportPtr,
        void* secondLayerHandlerFunc)
{
    sim_event_t* simEventPtr = (sim_event_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(simEventPtr == NULL,"simEventPtr is NULL");

    taf_sim_NewStateHandlerFunc_t clientHandlerFunc =
        (taf_sim_NewStateHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(simEventPtr->simId, simEventPtr->state, le_event_GetContextPtr());
}

taf_sim_IccidChangeHandlerRef_t taf_sim:: AddIccidChangeHandler(taf_sim_IccidChangeHandlerFunc_t handlerPtr) {
    le_event_HandlerRef_t handlerRef;
    LE_INFO("Add Iccid Change handler");
    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler pointer is NULL");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("IccidChangeHandler", IccidChangeEventId,
            FirstLayerIccidChangeHandler, (void*)handlerPtr);

    return (taf_sim_IccidChangeHandlerRef_t)(handlerRef);

}

void taf_sim::RemoveIccidChangeHandler(taf_sim_IccidChangeHandlerRef_t handlerRef){
        le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

void taf_sim::FirstLayerIccidChangeHandler(void* reportPtr,
        void* secondLayerHandlerFunc){
    sim_iccid_event_t* simEventPtr = (sim_iccid_event_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(simEventPtr == NULL, "simEventPtr is NULL");

    taf_sim_IccidChangeHandlerFunc_t clientHandlerFunc =
        (taf_sim_IccidChangeHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(simEventPtr->simId, (simEventPtr->ICCID).c_str(), le_event_GetContextPtr());
}

taf_sim_info_t* taf_sim::GetSimContext(taf_sim_Id_t simId) {
    simId = (TAF_SIM_UNSPECIFIED == simId) ? (taf_sim_Id_t)slot : simId;
    return simId > 0 ? &simList[simId - 1] : &simList[0];
}

bool taf_sim::isValidSimId(taf_sim_Id_t simId) {
    LE_INFO("isValidSimId: slot count: %d, input simId: %d", slotCount, (int)simId);
    if ((simId > 0 && simId <= slotCount) || simId == TAF_SIM_UNSPECIFIED ) {
        return true;
    }
    return false;
}

void onRefreshEvent(taf_pa_sim_RefreshChangeInd_t* ind, void* contextPtr) {
   auto &sim = taf_sim::GetInstance();

   LE_INFO("onRefreshEvent: contextPtr: %p", contextPtr);

   sim.NotifyRefreshEvent(ind, contextPtr);
}

taf_pa_sim_SessionType_t taf_sim::ConvertTafSessionTypeToPaSessionType(taf_sim_SessionType_t sessionType) {
    switch (sessionType)
    {
        case TAF_SIM_SESSION_TYPE_PRI_GW_PROV:
            return TAF_PA_SIM_SESSION_TYPE_PRI_GW_PROV;
        case TAF_SIM_SESSION_TYPE_SEC_GW_PROV:
            return TAF_PA_SIM_SESSION_TYPE_SEC_GW_PROV;
        default:
            LE_WARN("Unknown refresh session type %d.", sessionType);
    }

    return TAF_PA_SIM_SESSION_TYPE_PRI_GW_PROV;
}

taf_sim_RefreshStatus_t taf_sim::ConvertPaRefreshStageToTafRefreshStatus
(
    taf_pa_sim_RefreshStage_t refreshStage
)
{
    switch (refreshStage)
    {
        case TAF_PA_SIM_REFRESH_STAGE_END_WITH_SUCCESS:
            return TAF_SIM_REFRESH_STATUS_SUCCESS;
        case TAF_PA_SIM_REFRESH_STAGE_END_WITH_FAILURE:
            return TAF_SIM_REFRESH_STATUS_FAILURE;
        default:
            LE_WARN("Unknown refresh stage %d.", refreshStage);
    }

    return TAF_SIM_REFRESH_STATUS_FAILURE;
}

void taf_sim::CheckAndSendProfileSwitchEvent() {
    auto &sim = taf_sim::GetInstance();

    char iccid1[TAF_SIM_ICCID_BYTES];
    char iccid2[TAF_SIM_ICCID_BYTES];

    sim_refresh_event_t simRefreshEvent;
    simRefreshEvent.refreshStatus = 0;

    LE_DEBUG("ICCID change and profile swap");

    le_result_t result = LE_FAULT;
    string iccId = "";
    memset(iccid1, 0, TAF_SIM_ICCID_BYTES);
    memset(iccid2, 0, TAF_SIM_ICCID_BYTES);

    auto subscription = getSubscription((taf_sim_Id_t) DEFAULT_SLOT_ID);

    if (!subscription) {
        LE_ERROR("subscription is null for slot1");
    } else {
        iccId = subscription->getIccId();
        le_utf8_Copy(iccid1, iccId.c_str(), TAF_SIM_ICCID_BYTES, NULL);
    }

    iccId = "";

    subscription = getSubscription((taf_sim_Id_t) (DEFAULT_SLOT_ID+1));

    if (!subscription) {
        LE_ERROR("subscription is null for slot2");
    } else {
        iccId = subscription->getIccId();
        le_utf8_Copy(iccid2, iccId.c_str(), TAF_SIM_ICCID_BYTES, NULL);
    }
    std::unique_lock<std::mutex> lock(sim.eventMutex);
    le_ref_IterRef_t iterRef = le_ref_GetIterator(sim.SessionRefMap);
    result = le_ref_NextNode(iterRef);

    while (LE_OK == result)
    {
        taf_sim_Session_t* sessionPtr = (taf_sim_Session_t*) le_ref_GetValue(iterRef);
        if(sessionPtr == NULL) {
            LE_INFO("CheckAndSendProfileSwitchEvent sessionPtr null!");
            continue;
        }

        LE_DEBUG("ClientSessionRef %p, refreshResetStart: %d", sessionPtr->clientSessionRef, (int) sessionPtr->refreshResetStart);

        if (sessionPtr->refreshResetStart)
        {
            LE_INFO("Notify: current iccid1: %s, previous iccid1: %s and result: %s", iccid1, sessionPtr->simProfileIccid1, LE_RESULT_TXT(result));
            LE_INFO("Notify: current iccid2: %s, previous iccid2: %s", iccid2, sessionPtr->simProfileIccid2);

            //Send profile switch notification.
            if (sessionPtr->sessionType == TAF_SIM_SESSION_TYPE_PRI_GW_PROV)
            {
                if (strncmp(iccid1, sessionPtr->simProfileIccid1, TAF_SIM_ICCID_BYTES) != 0)
                {
                    simRefreshEvent.refreshStatus = TAF_SIM_REFRESH_STATUS_PROFILE_SWITCH;
                    le_utf8_Copy(sessionPtr->simProfileIccid1, iccid1, TAF_SIM_ICCID_BYTES, NULL);
                    le_event_Report(sessionPtr->RefreshChangeEventId, &simRefreshEvent, sizeof(simRefreshEvent));
                }
            }
            else if (sessionPtr->sessionType == TAF_SIM_SESSION_TYPE_SEC_GW_PROV)
            {
                if (strncmp(iccid2, sessionPtr->simProfileIccid2, TAF_SIM_ICCID_BYTES) != 0)
                {
                    simRefreshEvent.refreshStatus = TAF_SIM_REFRESH_STATUS_PROFILE_SWITCH;
                    le_utf8_Copy(sessionPtr->simProfileIccid2, iccid2, TAF_SIM_ICCID_BYTES, NULL);
                    le_event_Report(sessionPtr->RefreshChangeEventId, &simRefreshEvent, sizeof(simRefreshEvent));
                }
            }
        }

        if (strncmp(iccid1, sessionPtr->simProfileIccid1, TAF_SIM_ICCID_BYTES) != 0)
        {
            le_utf8_Copy(sessionPtr->simProfileIccid1, iccid1, TAF_SIM_ICCID_BYTES, NULL);
        }
        if (strncmp(iccid2, sessionPtr->simProfileIccid2, TAF_SIM_ICCID_BYTES) != 0)
        {
            le_utf8_Copy(sessionPtr->simProfileIccid2, iccid2, TAF_SIM_ICCID_BYTES, NULL);
        }

        result = le_ref_NextNode(iterRef);
    }
    sim.isPsEventInProgress = false;

}

void taf_sim::CheckAndSendRefreshEvent(taf_sim_Id_t SimId) {
    auto &sim = taf_sim::GetInstance();
    le_result_t result = LE_FAULT;
    std::unique_lock<std::mutex> lock(sim.eventMutex);
    le_ref_IterRef_t iterRef = le_ref_GetIterator(sim.SessionRefMap);
    result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        taf_sim_Session_t* sessionPtr = (taf_sim_Session_t*) le_ref_GetValue(iterRef);
        if(sessionPtr == NULL) {
            LE_INFO("CheckAndSendRefreshEvent sessionPtr null!");
            continue;
        }
        if((sessionPtr->sessionType == TAF_SIM_SESSION_TYPE_PRI_GW_PROV && SimId == TAF_SIM_SLOT_ID_1) ||
            (sessionPtr->sessionType == TAF_SIM_SESSION_TYPE_SEC_GW_PROV && SimId == TAF_SIM_SLOT_ID_2))
        {
            if (sessionPtr->refreshResetStart )
            {
                LE_INFO("Notify RefreshEvent for SimId : %d,SessionType : %d", SimId,sessionPtr->sessionType);
                le_sem_Post(sessionPtr->semaphore);
            }
        }
        result = le_ref_NextNode(iterRef);
    }
}

void taf_sim::NotifyRefreshEvent(taf_pa_sim_RefreshChangeInd_t* ind, void* contextPtr) {
    LE_INFO("RefreshEvent: sessionType = %d, refreshMode = %d, refreshStage = %d", ind->sessionType, ind->refreshMode, ind->refreshStage);
    le_result_t res = LE_FAULT;
    taf_sim_Session_t* clientRequestPtr = NULL;
    sim_refresh_event_t simRefreshEvent;
    simRefreshEvent.refreshStatus = 0;
    clientRequestPtr = DiscoverSessionRef((taf_sim_RefreshRef_t) contextPtr);

    LE_INFO("NotifyRefreshEvent contextPtr: %p, clientRequestPtr: %p", contextPtr, clientRequestPtr);

    TAF_ERROR_IF_RET_NIL(NULL == clientRequestPtr, "clientRequestPtr is NULL");

    if(ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_WAIT_FOR_OK) {
        if (clientRequestPtr != NULL && ((1 << ind->refreshMode) & clientRequestPtr->refreshMode) != 0) {
            LE_INFO("Request Refresh_ok with refreshAllow: %d", (int) clientRequestPtr->refreshAllow);
            res = taf_pa_sim_RefreshOk(ind->sessionType, clientRequestPtr->refreshAllow);
        } else {
            res = taf_pa_sim_RefreshOk(ind->sessionType, true);
        }
        LE_INFO("Refresh_ok: result: %s", LE_RESULT_TXT(res));

        return;
    } else if(ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_START && ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_FCN) {
        res = taf_pa_sim_RefreshComplete(ind->sessionType);

        LE_INFO("RefreshComplete: result: %s", LE_RESULT_TXT(res));

        return;
    } else if(ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_START && ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_RESET) {
        LE_INFO("RefreshStart for reset mode");
        clientRequestPtr->refreshResetStart = true;
        le_clk_Time_t timeToWait = {5, 0};
        res = le_sem_WaitWithTimeOut(clientRequestPtr->semaphore, timeToWait);
        if (res == LE_OK )
        {
           simRefreshEvent.refreshStatus |= TAF_SIM_REFRESH_STATUS_SUCCESS;
        }
        else
        {
           simRefreshEvent.refreshStatus |= TAF_SIM_REFRESH_STATUS_FAILURE;
        }

        le_event_Report(clientRequestPtr->RefreshChangeEventId, &simRefreshEvent, sizeof(simRefreshEvent));
        clientRequestPtr->refreshResetStart = false;
        LE_INFO("Notify simRefreshEvent:refreshStatus : %d", simRefreshEvent.refreshStatus);
        return;
    }

    bool notifyClient = (ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_END_WITH_SUCCESS)
            || (ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_END_WITH_FAILURE);

    if (notifyClient) {

        bool isFileChanged = (ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_FCN)
                || (ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_INIT_FULL_FCN)
                || (ind->refreshMode == TAF_PA_SIM_REFRESH_MODE_INIT_FCN);

        LE_INFO("refreshResetStart: %d and isFileChanged: %d", (int) clientRequestPtr->refreshResetStart, (int) isFileChanged);

        if (ind->refreshStage == TAF_PA_SIM_REFRESH_STAGE_END_WITH_FAILURE) {
            simRefreshEvent.refreshStatus = ConvertPaRefreshStageToTafRefreshStatus(ind->refreshStage);
        } else {
            //Add the refresh success.
            simRefreshEvent.refreshStatus |= TAF_SIM_REFRESH_STATUS_SUCCESS;
            if (isFileChanged) {
                simRefreshEvent.refreshStatus |= TAF_SIM_REFRESH_STATUS_FILE_CHANGE;
            }
        }

        if (clientRequestPtr != NULL) {
            le_event_Report(clientRequestPtr->RefreshChangeEventId, &simRefreshEvent, sizeof(simRefreshEvent));
        }
    }
}

void taf_sim::FirstLayerNewRefreshChangeHandler(void* reportPtr, void* secondLayerHandlerFunc) {
    sim_refresh_event_t* simRefreshPtr = (sim_refresh_event_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(simRefreshPtr == NULL, "simRefreshPtr is NULL");

    taf_sim_RefreshChangeHandlerFunc_t clientHandlerFunc =
        (taf_sim_RefreshChangeHandlerFunc_t)secondLayerHandlerFunc;
    clientHandlerFunc(simRefreshPtr->refreshStatus, le_event_GetContextPtr());
}

taf_sim_RefreshChangeHandlerRef_t taf_sim::AddRefreshChangeHandler(taf_sim_RefreshChangeHandlerFunc_t handlerPtr, void* contextPtr) {
    le_event_HandlerRef_t handlerRef;
    LE_INFO("Add Refresh Change handler");
    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler pointer is NULL");
        return NULL;
    }

    taf_sim_Session_t* clientRequestPtr = NULL;
    taf_sim_RefreshRef_t sessionRef = (taf_sim_RefreshRef_t) taf_sim_GetClientSessionRef();

    clientRequestPtr = DiscoverSessionRef(sessionRef);

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, NULL, "clientRequestPtr is NULL");

    handlerRef = le_event_AddLayeredHandler("RefreshChangeHandler", clientRequestPtr->RefreshChangeEventId,
            FirstLayerNewRefreshChangeHandler, (void*)handlerPtr);

    clientRequestPtr->paHandlerRef = taf_pa_sim_AddRefreshChangeHandler((taf_pa_sim_RefreshChangeHandlerFunc_t)&onRefreshEvent, sessionRef);

    LE_INFO("taf_pa_sim_AddRefreshChangeHandler done. paHandlerRef: %p, handlerRef: %p", clientRequestPtr->paHandlerRef, handlerRef);

    return (taf_sim_RefreshChangeHandlerRef_t)(handlerRef);
}

void taf_sim::RemoveRefreshChangeHandler(taf_sim_RefreshChangeHandlerRef_t handlerRef) {
    taf_sim_Session_t* clientRequestPtr = NULL;
    taf_sim_RefreshRef_t sessionRef = (taf_sim_RefreshRef_t) taf_sim_GetClientSessionRef();

    clientRequestPtr = DiscoverSessionRef(sessionRef);

    TAF_ERROR_IF_RET_NIL( NULL == clientRequestPtr, "clientRequestPtr is NULL");
    taf_pa_sim_RemoveRefreshChangeHandler(clientRequestPtr->paHandlerRef);
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    le_ref_DeleteRef(SessionRefMap, clientRequestPtr->ref);
    mClientRefCount--;
}

taf_sim_Session_t* taf_sim::DiscoverSessionRef
(
    taf_sim_RefreshRef_t sessionRef
)
{
    auto &sim = taf_sim::GetInstance();
    std::unique_lock<std::mutex> lock(sim.eventMutex);
    le_ref_IterRef_t iterRef = le_ref_GetIterator(sim.SessionRefMap);
    le_result_t result = le_ref_NextNode(iterRef);

    while (LE_OK == result)
    {
        taf_sim_Session_t* sessionPtr = (taf_sim_Session_t*) le_ref_GetValue(iterRef);
        if(sessionPtr == NULL) {
            LE_INFO("DiscoverSessionRef sessionPtr null!");
            return NULL;
        }

        LE_DEBUG("SessionRef %p, clientSessionRef %p", sessionRef, sessionPtr->clientSessionRef);

        if (sessionRef == sessionPtr->ref || sessionRef == (taf_sim_RefreshRef_t) sessionPtr->clientSessionRef)
        {
             return sessionPtr;
        }
        result = le_ref_NextNode(iterRef);
    }
    LE_INFO("DiscoverSessionRef sessionPtr is null!");
    return NULL;
}

le_result_t taf_sim::CreateSession(taf_sim_SessionType_t sessionType, taf_sim_RefreshRef_t* refreshSessionRef) {
    taf_sim_Session_t* clientRequestPtr = NULL;
    taf_sim_RefreshRef_t sessionRef = (taf_sim_RefreshRef_t) taf_sim_GetClientSessionRef();

    LE_INFO("CreateSession client session ref %p", sessionRef);

    clientRequestPtr = DiscoverSessionRef(sessionRef);

    LE_INFO("After DiscoverSessionRef client session ref %p", clientRequestPtr);

    if (clientRequestPtr != nullptr) {
        LE_INFO("CreateSession: Already created the refresh Session, so use the existing one.");

        *refreshSessionRef = (taf_sim_RefreshRef_t) clientRequestPtr->ref;
        clientRequestPtr->sessionType = sessionType;

        return LE_OK;
    }

    taf_sim_Session_t* res  = (taf_sim_Session_t* )le_mem_ForceAlloc(SessionPool);
    if(res == NULL) {
        LE_INFO("Create SessionPool failed!");
        return LE_FAULT;
    }
    LE_INFO("Create new Session");
    res->link = LE_DLS_LIST_INIT;
    res->ref = (taf_sim_RefreshRef_t)le_ref_CreateRef(SessionRefMap, res);

    res->clientSessionRef = (le_msg_SessionRef_t) sessionRef;

    *refreshSessionRef = (taf_sim_RefreshRef_t)(res->ref);
    res->sessionType = sessionType;
    res->refreshAllow = true;
    res->refreshMode = (taf_sim_RefreshMode_t) 0xffff;
    res->refreshResetStart = false;
    res->refreshRegFilesSize = 0;
    res->RefreshChangeEventId = le_event_CreateId("ClientRefreshEventId", sizeof(sim_refresh_event_t));
    res->semaphore = le_sem_Create("IccidCheckSem", 0);

    LE_INFO("res->sessionRef %p, *reference %p", res->ref, *refreshSessionRef);

    string iccId = "";
    memset(res->simProfileIccid1, 0, TAF_SIM_ICCID_BYTES);
    memset(res->simProfileIccid2, 0, TAF_SIM_ICCID_BYTES);

    auto subscription = getSubscription((taf_sim_Id_t) DEFAULT_SLOT_ID);

    if (!subscription) {
        LE_ERROR("subscription is null for slot1");
    } else {
        iccId = subscription->getIccId();
        le_utf8_Copy(res->simProfileIccid1, iccId.c_str(), TAF_SIM_ICCID_BYTES, NULL);
    }

    iccId = "";

    subscription = getSubscription((taf_sim_Id_t) (DEFAULT_SLOT_ID+1));

    if (!subscription) {
        LE_ERROR("subscription is null for slot2");
    } else {
        iccId = subscription->getIccId();
        le_utf8_Copy(res->simProfileIccid2, iccId.c_str(), TAF_SIM_ICCID_BYTES, NULL);
    }

    LE_INFO("Refresh create session done: iccid1: %s, iccid2: %s", res->simProfileIccid1, res->simProfileIccid2);

    if (sessionRef!=nullptr) {
        //External client increase Client ref count
        mClientRefCount++;
    }

    LE_INFO("SessionRef %p was not found, Created new Client session, total count %d", sessionRef, mClientRefCount);

    return LE_OK;
}

le_result_t taf_sim::SetRefreshRegisterFiles(taf_sim_RefreshRef_t refreshSessionRef, const taf_sim_RefreshRegFile_t* filesPtr, size_t filesSize) {
    taf_sim_Session_t* clientRequestPtr = NULL;

    clientRequestPtr = DiscoverSessionRef(refreshSessionRef);

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    for (int i = 0; i < (int) filesSize; i++) {
        clientRequestPtr->refreshRegFiles[i].file_id = filesPtr[i].file_id;
        le_utf8_Copy((char*) clientRequestPtr->refreshRegFiles[i].path, (char*) filesPtr[i].path, sizeof(filesPtr[i].path), NULL);

        LE_INFO("File_id: %d and path: %s", clientRequestPtr->refreshRegFiles[i].file_id, clientRequestPtr->refreshRegFiles[i].path);
    }

    clientRequestPtr->refreshRegFilesSize = filesSize;

    return LE_OK;
}

le_result_t taf_sim::SetRefreshMode(taf_sim_RefreshRef_t refreshSessionRef, taf_sim_RefreshMode_t refreshMode) {
    taf_sim_Session_t* clientRequestPtr = NULL;

    clientRequestPtr = DiscoverSessionRef(refreshSessionRef);

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    clientRequestPtr->refreshMode = refreshMode;

    return LE_OK;
}

le_result_t taf_sim::SetRefreshAllow(taf_sim_RefreshRef_t refreshSessionRef, bool isRefreshAllowed) {
    taf_sim_Session_t* clientRequestPtr = NULL;

    clientRequestPtr = DiscoverSessionRef(refreshSessionRef);

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    taf_pa_sim_RefreshFile_t refreshPAFiles[clientRequestPtr->refreshRegFilesSize];

    for (int i = 0; i < (int) clientRequestPtr->refreshRegFilesSize; i++) {
        int pathStrLen = 0;
        refreshPAFiles[i].file_id = clientRequestPtr->refreshRegFiles[i].file_id;
        if(clientRequestPtr->refreshRegFiles[i].path != NULL) {
            pathStrLen = strlen(clientRequestPtr->refreshRegFiles[i].path);
            refreshPAFiles[i].path_len = pathStrLen/2;
        } else {
            refreshPAFiles[i].path_len = 0;
        }

        //If input clientRequestPtr->refreshRegFiles[i].path is 3F007FFF.
        //Then refreshPAFiles.path[0] = 0(00), refreshPAFiles.path[1]=63(3F), refreshPAFiles.path[2] = 255(FF) and refreshPAFiles.path[3] = 127(7F)

        uint32_t pathValue =  std::stoul(clientRequestPtr->refreshRegFiles[i].path, nullptr, 16);

        LE_INFO("pathValue string: %s, in hex: %x and input path len: %d", clientRequestPtr->refreshRegFiles[i].path, pathValue, pathStrLen);

        if (refreshPAFiles[i].path_len == 2) {
            refreshPAFiles[i].path[0] = (pathValue & 0x000000ff);
            refreshPAFiles[i].path[1] = (pathValue & 0x0000ff00) >> 8;
        } else if (refreshPAFiles[i].path_len == 4) {
            refreshPAFiles[i].path[0] = (pathValue & 0x00ff0000) >> 16;
            refreshPAFiles[i].path[1] = (pathValue & 0xff000000) >> 24;
            refreshPAFiles[i].path[2] = (pathValue & 0x000000ff);
            refreshPAFiles[i].path[3] = (pathValue & 0x0000ff00) >> 8;
        }
        LE_INFO("PA file path0 ~ path3 in hex: %x %x %x %x", refreshPAFiles[i].path[0], refreshPAFiles[i].path[1], refreshPAFiles[i].path[2], refreshPAFiles[i].path[3]);

        LE_INFO("PA file path0 ~ path3 in dec: %u %u %u %u", refreshPAFiles[i].path[0], refreshPAFiles[i].path[1], refreshPAFiles[i].path[2], refreshPAFiles[i].path[3]);

        LE_INFO("PA File_id: %d and path_len: %d", refreshPAFiles[i].file_id, refreshPAFiles[i].path_len);
    }

    le_result_t res = taf_pa_sim_RefreshRegister(ConvertTafSessionTypeToPaSessionType(clientRequestPtr->sessionType),
            clientRequestPtr->refreshRegFilesSize,
            refreshPAFiles);

    LE_INFO("Refresh register done: result: %s", LE_RESULT_TXT(res));

    clientRequestPtr->refreshAllow = isRefreshAllowed;
    return res;
}

le_result_t taf_sim::selectSimSlot(taf_sim_Id_t simId) {
    auto ret = LE_OK;
    auto isInputValid = isValidSimId(simId);
    LE_INFO("Switch slot to %d, current slot: %d", (int)simId, (int)slot);
    if (isInputValid
                && (slot != simId)
                && (simId != TAF_SIM_UNSPECIFIED)
                && isSingleActive) {
        auto cbPromise = std::promise<telux::common::ErrorCode>();
        multiSimMgr->switchActiveSlot(SlotId((int)simId), [&](telux::common::ErrorCode error){
            cbPromise.set_value(error);
        });
        telux::common::ErrorCode errorStatus = cbPromise.get_future().get();
        if (errorStatus == telux::common::ErrorCode::SUCCESS
                || errorStatus == telux::common::ErrorCode::NO_EFFECT){
            LE_INFO("Select slot: %d successfully", (int)simId);
            slot = simId;
        } else {
            LE_ERROR("Active slot: %d failed with error: %d", (int)simId, (int)errorStatus);
            ret = LE_FAULT;
        }
    }
    if (isInputValid
                && (slot != simId)
                && (simId != TAF_SIM_UNSPECIFIED)
                && !isSingleActive) {
        slot = simId;
    }
    if(!isInputValid){
        LE_WARN("Invalid simId: %d", (int)simId);
        ret = LE_FAULT;
    }
    return ret;
}

void taf_sim::InitializeSimInfo(std::shared_ptr<telux::tel::ISubscription> subscription,
                             taf_sim_Id_t simId) {
    LE_INFO("InitializeSimInfo for simId: %d", (int)simId);
    taf_sim_info_t* simPtr = NULL;
    simPtr = GetSimContext(simId);
    if (subscription) {
        simPtr->simId = simId;
         le_utf8_Copy(simPtr->ICCID, subscription->getIccId().c_str() ,TAF_SIM_ICCID_BYTES, NULL);
         le_utf8_Copy(simPtr->IMSI, subscription->getImsi().c_str() ,TAF_SIM_IMSI_BYTES, NULL);
         le_utf8_Copy(simPtr->phoneNumber, subscription->getPhoneNumber().c_str() ,TAF_SIM_PHONE_NUM_MAX_BYTES, NULL);
    } else {
        simPtr->simId = simId;
        simPtr->ICCID[0] = '\0';
        simPtr->IMSI[0] = '\0';
        simPtr->phoneNumber[0] = '\0';
        simPtr->pinTryCount = 3;
        simPtr->pukTryCount = 10;
    }
}

std::shared_ptr<telux::tel::ISubscription> taf_sim::getSubscription(taf_sim_Id_t simId){
    telux::common::Status status;
    if(isSingleActive && cards[(int)simId] != nullptr){
        LE_INFO("Single active, card found for simId: %d", (int)simId);
        // In the case of single active sim slot, the logical slot Id for querying subscription is always DEFAULT_SLOT_ID
        auto subscription = subMgr->getSubscription(DEFAULT_SLOT_ID, &status);
        return subscription;
    }
    if(!isSingleActive && cards[simId] != nullptr){
        auto subscription = subMgr->getSubscription((int)simId, &status);
        return subscription;
    }

    return nullptr;
}

le_result_t taf_sim::getICCID(taf_sim_Id_t simId, char *iccid, int length ) {
    string iccId = "";
    taf_sim_info_t* simPtr = NULL;
    if (selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    simPtr = GetSimContext(simId);
    if (simPtr->ICCID[0] != 0) {
        return le_utf8_Copy(iccid, simPtr->ICCID, length, NULL);
    }
    auto subscription = getSubscription(simId);
    if (!subscription) {
        LE_ERROR("subscription is null");
        return LE_NOT_FOUND;
    }

    iccId = subscription->getIccId();

    le_utf8_Copy(simPtr->ICCID, iccId.c_str(), length, NULL);
    return le_utf8_Copy(iccid, iccId.c_str(), length, NULL);
}

le_result_t taf_sim::getSubscriberPhoneNumber(taf_sim_Id_t simId, char *phoneNumber, int length) {
    string phoneNumberString = "";
    taf_sim_info_t* simPtr = NULL;
    if (selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }

    simPtr = GetSimContext(simId);
    if (simPtr->phoneNumber[0] != 0) {
        return le_utf8_Copy(phoneNumber, simPtr->phoneNumber, length, NULL);
    }

    auto subscription = getSubscription(simId);
    if (!subscription) {
        LE_ERROR("subscription is null");
        return LE_NOT_FOUND;
    }

    phoneNumberString = subscription->getPhoneNumber();

    le_utf8_Copy(simPtr->phoneNumber, phoneNumberString.c_str(), length, NULL);
    return le_utf8_Copy(phoneNumber, phoneNumberString.c_str(), length, NULL);
}

le_result_t taf_sim::getIMSI(taf_sim_Id_t simId, char *imsi, int length) {
    string imsiString = "";
    taf_sim_info_t* simPtr = NULL;
    if (selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    simPtr = GetSimContext(simId);
    if (simPtr->IMSI[0] != 0) {
        return le_utf8_Copy(imsi, simPtr->IMSI, length, NULL);
    }
    auto subscription = getSubscription(simId);
    if (!subscription) {
        LE_ERROR("subscription is null");
        return LE_NOT_FOUND;
    }

    imsiString = subscription->getImsi();

    le_utf8_Copy(simPtr->IMSI, imsiString.c_str(), length, NULL);
    return le_utf8_Copy(imsi, imsiString.c_str(), length, NULL);
}

le_result_t taf_sim::getHomeNetworkOperator(taf_sim_Id_t simId, char *name, int length) {
    taf_sim_info_t* simPtr = NULL;
    string nameString = "";
    if (selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    simPtr = GetSimContext(simId);
    if(simPtr != NULL)
    {
        auto subscription = getSubscription(simPtr->simId);
        if (!subscription) {
            LE_ERROR("subscription is null");
            return LE_NOT_FOUND;
        }
        nameString = subscription->getCarrierName();
    }
    else
    {
        return LE_BAD_PARAMETER;
    }
    return le_utf8_Copy(name, nameString.c_str(), length, NULL);
}

le_result_t taf_sim::getHomeNetworkMccMnc(taf_sim_Id_t simId, char *mccPtr,
        int mccPtrSize, char *mncPtr, int mncPtrSize) {
    taf_sim_info_t* simPtr = NULL;
    int mcc = 0;
    int mnc = 0;
    if (selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }

    simPtr = GetSimContext(simId);
    if(simPtr != NULL)
    {
        auto subscription = getSubscription(simPtr->simId);
        if (!subscription) {
            LE_ERROR("subscription is null");
            return LE_NOT_FOUND;
        }
        mcc = subscription->getMcc();
        mnc = subscription->getMnc();
    }
    else
    {
        return LE_BAD_PARAMETER;
    }
    le_utf8_Copy(mccPtr, to_string(mcc).c_str(), mccPtrSize, NULL);
    le_utf8_Copy(mncPtr, to_string(mnc).c_str(), mncPtrSize, NULL);
    return LE_OK;
}

le_result_t taf_sim::UnlockCardByPin(taf_sim_Id_t  simId,
             taf_sim_LockType_t lockType, const char* pinPtr) {
    if(selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    auto card = cards[slot];
    string newPin = (string) pinPtr;
    telux::tel::CardLockType cardLockType;

    if(!card) {
        LE_ERROR( "ERROR: Unable to get card instance");
        return LE_NOT_FOUND;
    }

    if(lockType == TAF_SIM_PIN1 || lockType == TAF_SIM_PIN2) {
        cardLockType = (telux::tel::CardLockType)lockType;
    } else {
        cardLockType = telux::tel::CardLockType::PIN1;
    }

    std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
    applications = card->getApplications();
    if(applications.size() != 0)  {
        for(auto cardApp : applications) {
            if(cardApp->getAppType() == telux::tel::AppType::APPTYPE_USIM
                    && cardApp->getAppState() == telux::tel::AppState::APPSTATE_PIN) {
                auto ret = cardApp->unlockCardByPin(cardLockType, newPin,
                        tafAuthenticationResponseCallback::unlockCardByPinResponseCb);
                if(ret == telux::common::Status::SUCCESS) {
                    LE_INFO("Unlock card by pin request sent successfully\n");
                    return LE_OK;
                } else {
                    LE_INFO("Unlock card by pin request failed\n");
                    return LE_FAULT;
                }
            }
        }
    } else {
        LE_INFO("Unlock card by PIN request failed\n");
        return LE_FAULT;
    }
    return LE_FAULT;
}

le_result_t taf_sim::ChangeCardPin( taf_sim_Id_t simId, taf_sim_LockType_t lockType,
        const char* oldpinPtr, const char* newpinPtr) {
    if(selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }

    auto card = cards[slot];

    telux::tel::CardLockType cardLockType;

    if(!card) {
        LE_INFO( "ERROR: Unable to get card instance");
        return LE_NOT_FOUND;
    }

    if(lockType == TAF_SIM_PIN1 || lockType == TAF_SIM_PIN2) {
        cardLockType = (telux::tel::CardLockType)lockType;
    } else {
        cardLockType = telux::tel::CardLockType::PIN1;
    }

    std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
    applications = card->getApplications();
    if(applications.size() != 0)  {
        for(auto cardApp : applications) {
            if((cardApp->getAppType() == telux::tel::AppType::APPTYPE_USIM)
                    && (cardApp->getAppState() == telux::tel::AppState::APPSTATE_READY)) {
                auto ret
                    = cardApp->changeCardPassword(cardLockType, (string)oldpinPtr, (string)newpinPtr,
                            tafAuthenticationResponseCallback::ChangeCardPinResponseCb);
                if(ret == telux::common::Status::SUCCESS) {
                    LE_INFO( "Change card PIN request sent successfully\n");
                    return LE_OK;
                } else {
                    LE_INFO( "Change card PIN request failed\n");
                    return LE_FAULT;
                }
            }
        }
    } else {
        LE_INFO("Change card PIN request failed");
        return LE_FAULT;
    }
    return LE_FAULT;
}

le_result_t taf_sim::UnlockCardByPuk(taf_sim_Id_t  simId, taf_sim_LockType_t lockType,
        const char* pukPtr, const char* newpinPtr) {
    if(selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    auto card = cards[slot];
    telux::tel::CardLockType cardLockType;

    if(!card) {
        LE_ERROR( "ERROR: Unable to get card instance");
        return LE_NOT_FOUND;
    }
    if(lockType == TAF_SIM_PUK1 || lockType == TAF_SIM_PUK2) {
        cardLockType = (telux::tel::CardLockType)lockType;
    } else {
        cardLockType = telux::tel::CardLockType::PUK1;
    }

    std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
    applications = card->getApplications();
    if(applications.size() != 0)  {
        for(auto cardApp : applications) {
            if(cardApp->getAppType() == telux::tel::AppType::APPTYPE_USIM) {
                if (cardApp->getAppState() == telux::tel::AppState::APPSTATE_PUK) {
                    auto ret = cardApp->unlockCardByPuk(cardLockType,(string) pukPtr, newpinPtr,
                            tafAuthenticationResponseCallback::unlockCardByPukResponseCb);
                    if(ret == telux::common::Status::SUCCESS) {
                        LE_INFO("Unlock card by PUK request sent successfully\n");
                        return LE_OK;
                    } else {
                        LE_INFO("Unlock card by PUK request failed\n");
                        return LE_FAULT;
                    }
                }else {
                    LE_INFO("Unlock card by PUK request failed\n");
                    return LE_FAULT;
                }
            }
        }
    } else {
        LE_INFO("Unlock card by PUK request failed\n");
        return LE_FAULT;
    }
    return LE_FAULT;
}


le_result_t taf_sim::SetCardLock(taf_sim_Id_t  simId, taf_sim_LockType_t lockType,
        const char* pinPtr, bool lockEnable) {
    if(selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    auto card = cards[slot];
    telux::tel::CardLockType cardLockType;

    if(!card) {
        LE_ERROR( "ERROR: Unable to get card instance");
        return LE_NOT_FOUND;
    }
    if(lockType == TAF_SIM_PIN1 || lockType == TAF_SIM_FDN) {
        cardLockType = (telux::tel::CardLockType)lockType;
    } else {
        cardLockType = telux::tel::CardLockType::PIN1;
    }

    std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
    applications = card->getApplications();
    if(applications.size() != 0)  {
        for(auto cardApp : applications) {
            if(cardApp->getAppType() == telux::tel::AppType::APPTYPE_USIM){
                auto ret = cardApp->setCardLock(cardLockType, pinPtr, lockEnable,
                        tafAuthenticationResponseCallback::setCardLockResponseCb);
                if(ret == telux::common::Status::SUCCESS) {
                    LE_INFO("Set card lock request sent successfully\n");
                    return LE_OK;
                } else {
                    LE_INFO("Set card lock request failed\n");
                    return LE_FAULT;
                }
            }
        }
    } else {
        LE_INFO("Set card lock request failed\n");
        return LE_FAULT;
    }
    return LE_FAULT;
}

int32_t taf_sim::GetRemainingPINTries(taf_sim_Id_t simId) {
    taf_sim_info_t* simPtr = NULL;
    if(selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    simPtr = GetSimContext(simId );
    return simPtr->pinTryCount;
}

le_result_t taf_sim::GetRemainingPukTries(taf_sim_Id_t simId, uint32_t* remainingPukTriesPtr) {
    taf_sim_info_t* simPtr = NULL;
    if(selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    simPtr = GetSimContext(simId);
    *remainingPukTriesPtr = simPtr->pukTryCount;
    return LE_OK;
}

taf_sim_AuthenticationResponseHandlerRef_t taf_sim::AddAuthenticationResponseHandler(
        taf_sim_AuthenticationResponseHandlerFunc_t handlerPtr, void* contextPtr){

    le_event_HandlerRef_t handlerRef;
    LE_INFO("Add AuthenticationResponseHandler");

    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("AuthenticationResponseHandler",
            ResponseEventId,
            FirstLayerAuthenticationResponseHandler,
            (void*)handlerPtr);

    return (taf_sim_AuthenticationResponseHandlerRef_t)(handlerRef);

}

void taf_sim::RemoveAuthenticationResponseHandler(taf_sim_AuthenticationResponseHandlerRef_t handlerRef) {
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

void taf_sim::FirstLayerAuthenticationResponseHandler(void* reportPtr,
        void* secondLayerHandlerFunc)
{
    sim_response_event_t* simResponsePtr = (sim_response_event_t*)reportPtr;

    LE_INFO("FirstLayerNewSimStateHandler simId = %d", simResponsePtr->simId);
    if (!simResponsePtr)
    {
        LE_ERROR("Null pointer provided!");
        return;
    }

    taf_sim_AuthenticationResponseHandlerFunc_t clientHandlerFunc =
        (taf_sim_AuthenticationResponseHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(simResponsePtr->simId, simResponsePtr->responseType,
            simResponsePtr->result, le_event_GetContextPtr());
}

le_result_t  taf_sim::GetEID( taf_sim_Id_t slotId, char* eidPtr, size_t eidLen) {
    return LE_UNSUPPORTED;
}

le_result_t taf_sim::SetAutomaticSelection( bool enable) {
    EnableAutoSelection = enable;
    return LE_OK;
}

le_result_t taf_sim::GetAutomaticSelection( bool* enablePtr) {
    *enablePtr = EnableAutoSelection;
    return LE_OK;
}

// We are making a synchronized APDU card requests. So added wait logic
// std::condition_variable
bool taf_sim::waitForCardEvent(CardEvent cardEvent, int timeout) {
   std::unique_lock<std::mutex> lock(eventMutex);
   cardEventExpected = cardEvent;
   auto cvStatus = eventCV.wait_for(lock, std::chrono::seconds(DEFAULT_TIMEOUT_IN_SECONDS));
   if(cvStatus == std::cv_status::timeout) {
      LE_INFO("Event: %d not found with in %d second(s)",  (int)cardEvent, DEFAULT_TIMEOUT_IN_SECONDS);
   }
   cardEventExpected = (CardEvent)0;  // reset message id to avoid further notifications
   if(cvStatus != std::cv_status::timeout) {
      if(cardEvent == CardEvent::OPEN_LOGICAL_CHANNEL
         || cardEvent == CardEvent::CLOSE_LOGICAL_CHANNEL
         || cardEvent == CardEvent::TRANSMIT_APDU_CHANNEL) {

         if(errorCode == ErrorCode::SUCCESS)
            return true;
      }
   } else {
      LE_INFO("Unable to get the events, so timing out");
      return false;
   }
   return false;
}

le_result_t taf_sim::GetAppTypes(taf_sim_Id_t slotId, taf_sim_AppType_t* appTypePtr, size_t* appTypeNumElementsPtr) {
    *appTypeNumElementsPtr = 0;
    if (selectSimSlot(slotId) != LE_OK) {
        LE_INFO("Selecting sim slot failed");
        return LE_NOT_FOUND;
    }

    auto card = cards[slot];

    if(card) {
        std::vector<std::shared_ptr<ICardApp>> applications;
        applications = card->getApplications();
        LE_INFO("Card found with given simId. num of cardApps: %d", (int) applications.size());
        int i = 0;
        for(auto cardApp : applications) {
            if (i < TAF_SIM_MAX_APP_TYPE) {
                appTypePtr[i] = (taf_sim_AppType_t) cardApp->getAppType();
                LE_DEBUG("Card Application type: %d", (int) appTypePtr[i]);
                i++;
            }
        }
        *appTypeNumElementsPtr = i;
    } else {
        LE_ERROR("No Card. Error to get app types!");
        return LE_FAULT;
    }

    return LE_OK;
}

le_result_t taf_sim::OpenLogicalChannel( taf_sim_Id_t simId, taf_sim_AppType_t appType, uint8_t* channelPtr) {
    if (selectSimSlot(simId) != LE_OK) {
        LE_INFO("Selecting sim slot failed");
        return LE_NOT_FOUND;
    }
    auto card = cards[slot];
    std::vector<std::shared_ptr<ICardApp>> applications;
    auto openLogicalCb = std::make_shared<tafOpenLogicalChannelCallback>();
    std::string aid;
    if(!card) {
        LE_INFO("Card not found!");
        return LE_BAD_PARAMETER;
    }
    if(card) {
        LE_INFO("card found with given simId");
        applications = card->getApplications();
        for(auto cardApp : applications) {
            LE_INFO("Applications exist for given card");
            if(cardApp->getAppType() == (AppType) appType) {
                aid = cardApp->getAppId();
                break;
            }
        }
    }
    if (aid.empty()) {
        LE_INFO("Getting app id failed");
        return LE_BAD_PARAMETER;
    }
    card->openLogicalChannel(aid, openLogicalCb);
    if(!waitForCardEvent(CardEvent::OPEN_LOGICAL_CHANNEL)) {
        LE_INFO("Opening Logical Channel failed ");
        return LE_FAULT;
    }
    LE_INFO("Open Logical channel done channel = %d", openChannel);
    *channelPtr = openChannel;
    return LE_OK;
}

le_result_t taf_sim::OpenLogicalChannelByAid( taf_sim_Id_t simId, const char* aid, uint8_t* channelPtr) {
    if (selectSimSlot(simId) != LE_OK) {
        LE_INFO("Selecting sim slot failed");
        return LE_NOT_FOUND;
    }
    auto card = cards[slot];

    auto openLogicalCb = std::make_shared<tafOpenLogicalChannelCallback>();

    if(!card) {
        LE_INFO("Card not found!");
        return LE_BAD_PARAMETER;
    }

    card->openLogicalChannel(aid, openLogicalCb);
    if(!waitForCardEvent(CardEvent::OPEN_LOGICAL_CHANNEL)) {
        LE_INFO("Opening Logical Channel by AID failed!");
        return LE_FAULT;
    }
    LE_INFO("Open Logical channel by AID success channel = %d", openChannel);
    *channelPtr = openChannel;
    return LE_OK;
}

le_result_t taf_sim::CloseLogicalChannel( taf_sim_Id_t simId, uint8_t channel) {
    if (selectSimSlot(simId) != LE_OK) {
        return LE_NOT_FOUND;
    }
    auto closeLogicalChannelCb = std::make_shared<tafCloseLogicalChannelCallback>();
    auto card = cards[slot];
    if(card) {
        auto ret = card->closeLogicalChannel(channel, closeLogicalChannelCb);
        if(ret != telux::common::Status::SUCCESS) {
            return LE_FAULT;
        }
        if(!waitForCardEvent(CardEvent::CLOSE_LOGICAL_CHANNEL)) {
            LE_INFO("Closing Logical Channel failed ");
            return LE_FAULT;
        }
        return LE_OK;
    }  else {
        return LE_FAULT;
    }
}

le_result_t taf_sim::SendApduOnChannel( taf_sim_Id_t simId, uint8_t channel,
            const uint8_t* commandApduPtr, size_t commandApduNumElements,
             uint8_t* responseApduPtr,size_t* responseApduNumElementsPtr){
    uint8_t cla, instruction, p1, p2, p3;
    std::vector<uint8_t> data;
    auto tafTransmitApduCb = std::make_shared<tafTransmitApduResponseCallback>();
    cla = commandApduPtr[0];
    instruction = commandApduPtr[1];
    p1 = commandApduPtr[2];
    p2 = commandApduPtr[3];
    p3 = commandApduPtr[4];

    LE_DEBUG("SendApduOnChannel: Data size(p3) = %d and commandApduNumElements: %d", (int)p3, (int)commandApduNumElements);
    if (commandApduNumElements > 5) {
        for(int i = 0; i < p3; i++) {
          data.emplace_back(commandApduPtr[i+ 5]);
        }
    }

    if (selectSimSlot(simId) != LE_OK) {
        return LE_NOT_FOUND;
    }
    LE_DEBUG("SendApduOnChannel: channel id: %d", channel);
    auto card = cards[slot];

    if (card == nullptr) {
        LE_ERROR("Card not found so SendApduOnChannel failed!");
        return LE_NOT_FOUND;
    }

    auto ret = card->transmitApduLogicalChannel(channel, cla, instruction,
                                                   p1, p2, p3, data,
                                                       tafTransmitApduCb);
    if (ret != Status::SUCCESS) {
        return LE_FAULT;
    }

    if(!waitForCardEvent(CardEvent::TRANSMIT_APDU_CHANNEL)) {
        LE_INFO("Transmit APDU failed ");
        return LE_FAULT;
    }
    responseApduPtr[0] = (uint8_t)apduResponse.sw1;
    responseApduPtr[1] = (uint8_t)apduResponse.sw2;
    LE_DEBUG("sw1: %d, sw2: %d, payload: %s", (uint8_t)apduResponse.sw1, (uint8_t)apduResponse.sw2, apduResponse.payload.c_str());
    int index = 2;
    for(auto &i : data) {
        responseApduPtr[index] = (uint8_t) i;
        index++;
    }

    return LE_OK;
}

le_result_t taf_sim::SendApdu( taf_sim_Id_t simId,const uint8_t* commandApduPtr, size_t commandApduNumElements,
             uint8_t* responseApduPtr,size_t* responseApduNumElementsPtr){
    uint8_t cla, instruction, p1, p2, p3;
    std::vector<uint8_t> data;
    auto tafTransmitApduCb = std::make_shared<tafTransmitApduResponseCallback>();
    cla = commandApduPtr[0];
    instruction = commandApduPtr[1];
    p1 = commandApduPtr[2];
    p2 = commandApduPtr[3];
    p3 = commandApduPtr[4];
    if (commandApduNumElements > 5) {
        for(int i = 0; i < p3; i++) {
          data.emplace_back(commandApduPtr[i+ 5]);
        }
    }
    if (selectSimSlot(simId) != LE_OK) {
        return LE_NOT_FOUND;
    }
    auto card = cards[slot];

    if (card == nullptr) {
        LE_ERROR("Card not found so SendApdu failed!");
        return LE_NOT_FOUND;
    }

    auto ret = card->transmitApduBasicChannel(cla, instruction,
                                                   p1, p2, p3, data,
                                                       tafTransmitApduCb);
    if (ret != Status::SUCCESS) {
        return LE_FAULT;
    }
    if(!waitForCardEvent(CardEvent::TRANSMIT_APDU_CHANNEL)) {
        LE_INFO("Transmit APDU failed failed ");
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_sim::SendCommand(
    taf_sim_Id_t simId, taf_sim_Command_t command,
    const char* fileIdentifierPtr,
    uint8_t *p1, uint8_t *p2,
    uint8_t *p3,     const uint8_t* dataPtr,
    size_t dataNumElements,const char* pathPtr,
    uint8_t *sw1,uint8_t *sw2,
    uint8_t* responsePtr, size_t* responseNumElementsPtr
)
{
    if (selectSimSlot(simId) != LE_OK) {
        LE_INFO("Issue with simId");
        return LE_NOT_FOUND;
    }
    char* fileId_end=(char*)fileIdentifierPtr+4;
    uint16_t field = strtol(fileIdentifierPtr, &fileId_end , 16);
    LE_INFO("field: %d", field);

    auto card = cards[slot];
    std::string aid;
    if (card == nullptr) {
        LE_ERROR("Card not found so SendCommand failed!");
        return LE_NOT_FOUND;
    }
    if (card)
    {
        LE_INFO("card found with given simId");
        std::vector<std::shared_ptr<ICardApp>> applications;
        applications = card->getApplications();
        for (auto cardApp : applications)
        {
            LE_INFO("Applications exist for given card");
            if (cardApp->getAppType() == (AppType)TAF_SIM_APPTYPE_USIM)
            {
                aid = cardApp->getAppId();
                break;
            }
        }
    }
    if(aid.empty())
    {
        LE_ERROR("AID is NULL");
        return LE_FAULT;

    }
    string filePath = std::string(pathPtr);
    std::vector<uint8_t> data(dataPtr, dataPtr+dataNumElements);
    auto tafTransmitApduCb = std::make_shared<tafTransmitApduResponseCallback>();
    auto returnStatus = card->exchangeSimIO(field,
                                            command,
                                            *p1,
                                            *p2,
                                            *p3,
                                            filePath,
                                            data,
                                            "",
                                            aid,
                                            tafTransmitApduCb);
    if(returnStatus != Status::SUCCESS){
        return LE_FAULT;
    }
    if(!waitForCardEvent(CardEvent::TRANSMIT_APDU_CHANNEL)){
        LE_INFO("Command SIM IO failed");;
        return LE_FAULT;
    }
    *sw1 = (uint8_t)apduResponse.sw1;
    *sw2 = (uint8_t)apduResponse.sw2;
    int index = 0;
    for(auto &i : data){
        responsePtr[index] = (uint8_t) i;
        index++;
    }
    return LE_OK;
}

le_result_t taf_sim::SetPower(taf_sim_Id_t simId, le_onoff_t powerState)
{
    if (selectSimSlot(simId) != LE_OK)
    {
        LE_INFO("Sim ID %d Invalid", simId);
        return LE_BAD_PARAMETER;
    }
    if(!(powerState==LE_OFF || powerState==LE_ON))
    {
        LE_INFO("Invalid powerState given %d", powerState);
        return LE_BAD_PARAMETER;
    }
    telux::common::Status status;
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    auto ICard = cards[slot];

    if (ICard == nullptr) {
        LE_ERROR("Card not found so set power failed!");
        return LE_NOT_FOUND;
    }

    SlotId slotId_for_card = SlotId(ICard->getSlotId());
    std::promise<telux::common::ErrorCode> p;
    telux::common::ResponseCallback setPowerResponseCb = [&p]
            (telux::common::ErrorCode error) {p.set_value(error);};
    status = (powerState==LE_OFF)?cardManager->cardPowerDown(slotId_for_card, setPowerResponseCb):
            cardManager->cardPowerUp(slotId_for_card, setPowerResponseCb);
    if(status == Status::SUCCESS)
    {
        telux::common::ErrorCode error = p.get_future().get();
        if(error == ErrorCode::SUCCESS)
        {
            return LE_OK;
        }
    }
#endif
#ifdef TARGET_SA415M
    status = Status::NOTSUPPORTED;
#endif
    LE_INFO("Set Power operation failed, with status %s , simId %d , powerState %d",
            statusToString(status), simId, powerState);
    return LE_FAULT;
}

le_result_t taf_sim::Reset(taf_sim_Id_t simId)
{
    LE_INFO("Resetting sim card");
    le_result_t r=SetPower(simId, LE_OFF);
    if(r!=LE_OK){
        LE_INFO("Powering off while resetting failed");
        return LE_FAULT;
    }
    r = SetPower(simId, LE_ON);
    if(r!=LE_OK){
        LE_INFO("Powering on while resetting failed");
        return LE_FAULT;
    }
    LE_INFO("SIM Reset successful");
    return LE_OK;
}

le_result_t taf_sim::IsEmergencyCallSubscriptionSelected(taf_sim_Id_t simId, bool* isEs){
    *isEs = false;
    std::chrono::seconds span(DEFAULT_TIMEOUT_IN_SECONDS);
    if(selectSimSlot(simId) != LE_OK){
        LE_INFO("Invalid sim identifier given");
        return LE_BAD_PARAMETER;
    }

    ProfileSyncPromise = std::promise<le_result_t>();

    std::shared_ptr<tafSimProfileCallback> profileListCb = std::make_shared<tafSimProfileCallback>();

    auto responseCb = std::bind(&tafSimProfileCallback::profileListCallBack, profileListCb, std::placeholders::_1, std::placeholders::_2);

    telux::common::Status status = simProfileManager->requestProfileList((SlotId)slot, responseCb);

    if(status == telux::common::Status::SUCCESS){
        LE_INFO("Request profile list sent successfully");
        std::future<le_result_t> futureResult = ProfileSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);
        if(std::future_status::timeout == waitStatus){
            LE_INFO("Unable to read profile list");
            return LE_NOT_FOUND;
        }
        le_result_t futureResultVal = futureResult.get();
        if(futureResultVal == LE_OK){
            *isEs = isEcs;
            return LE_OK;
        }
        if(futureResultVal == LE_NOT_FOUND)
        {
            LE_INFO("Unable to determine active profile");
            return LE_NOT_FOUND;
        }
    }
    LE_INFO("isEmergencyProfileSelected failed ");
    return LE_FAULT;
}

void tafSimProfileCallback::profileListCallBack(const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles, telux::common::ErrorCode errorCode)
{
    auto &sim = taf_sim::GetInstance();
    if(errorCode == telux::common::ErrorCode::SUCCESS){
        if(profiles.size() == 0){
            LE_INFO("Profiles List is empty");
            sim.ProfileSyncPromise.set_value(LE_FAULT);
            return;
        }
        bool active=false;
        for(auto &profile:profiles){
            if(profile){
                active |= profile->isActive();
                if(profile->getType() == telux::tel::ProfileType::EMERGENCY){
                    sim.isEcs=profile->isActive();
                    sim.ProfileSyncPromise.set_value(LE_OK);
                    return;
                }
            }
        }
        if(!active){
            sim.ProfileSyncPromise.set_value(LE_NOT_FOUND);
            return;
        }
        LE_INFO("telux::tel::ProfileType::EMERGENCY not found");
        sim.ProfileSyncPromise.set_value(LE_FAULT);
        return;
    }
    LE_INFO("profile list retrieval failed with errorCode : %d", static_cast<int>(errorCode));
    sim.ProfileSyncPromise.set_value(LE_FAULT);
}

le_result_t taf_sim::profileListCallbackEm
(
    const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
    telux::common::ErrorCode error,
    SlotId simId
)
{
    if(error != telux::common::ErrorCode::SUCCESS){
        LE_ERROR("Failed to retrieve profile list with error %d", (int)error);
    }
    LE_INFO("Retrieving Profile list successful");
    int emergencyProfileId = -1;
    for(auto profile: profiles){
        if(profile->getType() == telux::tel::ProfileType::EMERGENCY){
            emergencyProfileId = profile->getProfileId();
        }
    }
    if(emergencyProfileId == -1){
        LE_INFO("EMERGENCY profile not found");
        return LE_FAULT;
    }
    LE_INFO("EMERGENCY Profile found");
    std::promise<telux::common::ErrorCode> p;
    auto swapResponseCb = [&p](telux::common::ErrorCode error){p.set_value(error);};
    auto status = simProfileManager->setProfile((SlotId)slot,
            emergencyProfileId,
            true,
            swapResponseCb
            );
    if(status == Status::SUCCESS){
        telux::common::ErrorCode error = p.get_future().get();
        LE_INFO("error: %d", (int)error);
        if(error == ErrorCode::SUCCESS) {
            return LE_OK;
        }
    }
    LE_INFO("status : %d", (int)status);
    return LE_FAULT;
}

le_result_t taf_sim::LocalSwapToEmergencyCallSubscription
(
    taf_sim_Id_t simId,
    taf_sim_Manufacturer_t manufacturer
)
{
    std::promise<telux::common::ErrorCode> q;
    le_result_t r;

    if (manufacturer == TAF_SIM_MORPHO || manufacturer == TAF_SIM_VALID
            || manufacturer >= TAF_SIM_MANUFACTURER_MAX) {
        LE_ERROR("The manufacturer MORPHO and VALID are not supported.");
        return LE_UNSUPPORTED;
    }

    if(selectSimSlot(simId)!=LE_OK){
        return LE_BAD_PARAMETER;
    }
    auto respCb = [&](const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
                      telux::common::ErrorCode errorcode){
                      r=profileListCallbackEm(profiles,
                                              errorcode,
                                              (SlotId)slot);};
    auto status = simProfileManager->requestProfileList((SlotId)slot, respCb);
    if(status != Status::SUCCESS){
        return LE_FAULT;
    }
    le_result_t r_rs=SetPower(simId, LE_OFF);
    if(r_rs != LE_OK)
        return LE_FAULT;
    r_rs = SetPower(simId, LE_ON);
    if(r_rs != LE_OK)
        return LE_FAULT;
    return r;
}

le_result_t taf_sim::profileListCallbackCo
(
    const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
    telux::common::ErrorCode error, SlotId simId
)
{
    if(error != telux::common::ErrorCode::SUCCESS){
        LE_ERROR("Failed to retrieve profile list with error %d", (int)error);
    }
    LE_INFO("Retrieving Profile list successful");
    int regularProfileId = -1;
    for(auto profile: profiles){
        if(profile->getType() == telux::tel::ProfileType::REGULAR){
            regularProfileId = profile->getProfileId();
        }
    }
    if(regularProfileId == -1){
        LE_INFO("REGULAR profile not found");
        return LE_FAULT;
    }
    LE_INFO("REGULAR Profile found");
    std::promise<telux::common::ErrorCode> p;
    auto swapResponseCb = [&p](telux::common::ErrorCode error){p.set_value(error);};
    auto status = simProfileManager->setProfile((SlotId)slot,
                                                regularProfileId,
                                                true,
                                                swapResponseCb);
    if(status == Status::SUCCESS){
        telux::common::ErrorCode error = p.get_future().get();
        LE_INFO("error: %d", (int)error);
        if(error == ErrorCode::SUCCESS) {
            return LE_OK;
        }
    }
    LE_INFO("status : %d", (int)status);
    return LE_FAULT;
}

le_result_t taf_sim::LocalSwapToCommercialCallSubscription
(
    taf_sim_Id_t simId,
    taf_sim_Manufacturer_t manufacturer
)
{
    std::promise<telux::common::ErrorCode> q;
    le_result_t r;
    if (manufacturer == TAF_SIM_MORPHO || manufacturer == TAF_SIM_VALID
            || manufacturer >= TAF_SIM_MANUFACTURER_MAX) {
        LE_ERROR("The manufacturer MORPHO and VALID are not supported.");
        return LE_UNSUPPORTED;
    }
    if(selectSimSlot(simId)!=LE_OK){
        return LE_BAD_PARAMETER;
    }
    auto respCb = [&](const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
                      telux::common::ErrorCode errorcode)
                    {r=profileListCallbackCo(profiles, errorcode, (SlotId)slot);};
    auto status = simProfileManager->requestProfileList((SlotId)slot, respCb);
    if(status != Status::SUCCESS){
        return LE_FAULT;
    }
    le_result_t r_rs=SetPower(simId, LE_OFF);
    if(r_rs != LE_OK)
        return LE_FAULT;
    r_rs = SetPower(simId, LE_ON);
    if(r_rs != LE_OK)
        return LE_FAULT;
    return r;
}

taf_sim_FPLMNListRef_t taf_sim::CreateInternalFPLMNList
(
)
{
    taf_sim_FPLMNList_t* res = (taf_sim_FPLMNList_t* )le_mem_ForceAlloc(FPLMNListPool);
    if(res == NULL) {
        LE_INFO("CreateInternalFPLMNList failed!");
        return NULL;
    }
    res->link = LE_DLS_LIST_INIT;
    res->ref = (taf_sim_FPLMNListRef_t)le_ref_CreateRef(FPLMNListRefMap, res);
    return (taf_sim_FPLMNListRef_t)(res->ref);
}

taf_sim_FPLMNListRef_t taf_sim::CreateFPLMNList
(
)
{
    if (fplmnListRefs != nullptr) {
        taf_sim_FPLMNList_t* ListReference = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, fplmnListRefs);
        if (!ListReference) {
            return NULL;
        }
        LE_INFO("CreateFPLMNList: Already created fplmnListRefs, so use the existing one.");
        return (taf_sim_FPLMNListRef_t)(ListReference->ref);
    }

    return CreateInternalFPLMNList();
}

le_result_t taf_sim::AddFPLMNOperator
(
    taf_sim_FPLMNListRef_t FPLMNListRef,
    char* mccPtr,
    char* mncPtr
)
{
    fplmnListRefs = FPLMNListRef;

    return AddFPLMNOperatorInternal(FPLMNListRef, mccPtr, mncPtr);
}

le_result_t taf_sim::AddFPLMNOperatorInternal
(
    taf_sim_FPLMNListRef_t FPLMNListRef,
    char* mccPtr,
    char* mncPtr
)
{
    taf_sim_FPLMNList_t* ListReference = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if((ListReference == NULL) || (mccPtr == NULL) || (mncPtr == NULL)) {
        LE_INFO("Issue with ListReference or mcc or mnc");
        return LE_OVERFLOW;
    }
    FPLMNNode_t* nodeFPLMN = (FPLMNNode_t*)le_mem_ForceAlloc(FPLMNNodePool);
    if (nodeFPLMN == NULL) {
        return LE_FAULT;
    }

    le_utf8_Copy(nodeFPLMN->mcc, mccPtr, sizeof(nodeFPLMN->mcc), NULL);
    le_utf8_Copy(nodeFPLMN->mnc, mncPtr, sizeof(nodeFPLMN->mnc), NULL);
    nodeFPLMN->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&(ListReference->link), &(nodeFPLMN->link));
    return LE_OK;
}

le_result_t taf_sim::GetFirstFPLMNOperator
(
    taf_sim_FPLMNListRef_t FPLMNListRef,
    char* mccPtr,
    size_t mccLen,
    char* mncPtr,
    size_t mncLen
)
{
    taf_sim_FPLMNList_t* ListReference = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if((ListReference == NULL) | (mccPtr == NULL) | (mncPtr == NULL)) {
        LE_INFO("Issue with ListReference or mcc or mnc");
        return LE_OVERFLOW;
    }
    le_dls_Link_t* linkPtr = le_dls_Peek(&(ListReference->link));
    if (linkPtr == NULL) {
        return LE_FAULT;
    }
    FPLMNNode_t* node = CONTAINER_OF(linkPtr, FPLMNNode_t, link);
    if (node == NULL) {
        return LE_FAULT;
    }
    le_utf8_Copy(mccPtr, node->mcc, mccLen, &mccLen);
    le_utf8_Copy(mncPtr, node->mnc, mncLen, &mncLen);
    fplmnListIndex = 1;
    return LE_OK;
}

le_result_t taf_sim::GetNextFPLMNOperator
(
    taf_sim_FPLMNListRef_t FPLMNListRef,
    char* mccPtr,
    size_t mccLen,
    char* mncPtr,
    size_t mncLen
)
{
    taf_sim_FPLMNList_t* ListReference = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if((ListReference == NULL) | (mccPtr == NULL) | (mncPtr == NULL)) {
        LE_INFO("Issue with ListReference or mcc or mnc");
        return LE_OVERFLOW;
    }
    le_dls_Link_t* linkPtr = le_dls_Peek(&(ListReference->link));
    if (linkPtr == NULL) {
        return LE_FAULT;
    }
    FPLMNNode_t* node;

    for (int i = 0; i < fplmnListIndex && linkPtr!=NULL; i++) {
        linkPtr = le_dls_PeekNext(&(ListReference->link), (linkPtr));
    }

    if(linkPtr == NULL) {
        return LE_FAULT;
    }
    node = CONTAINER_OF(linkPtr, FPLMNNode_t, link);
    if (node == NULL) {
        return LE_FAULT;
    }
    le_utf8_Copy(mccPtr, node->mcc, mccLen, &mccLen);
    le_utf8_Copy(mncPtr, node->mnc, mncLen, &mncLen);
    fplmnListIndex++;
    return LE_OK;
}

taf_sim_FPLMNListRef_t taf_sim::ReadFPLMNList(
    taf_sim_Id_t simId
)
{
    //First select the file using APDU commands
    //Then read from it in binary form and use payload to get the response as a hex string
    uint8_t selectFPLMNApdu[] = {0x00, 0xA4, 0x08, 0x04, 0x04, 0x7F, 0xFF, 0x6F, 0x7B};
    uint8_t responseAPDU[100];
    size_t responseLength = 100;
    uint8_t channel = 0;
    LE_INFO("Entered here");
    if((selectSimSlot(simId))!=LE_OK) {
        return NULL;
    }
    LE_INFO("Sim slot selected");
    le_result_t res = OpenLogicalChannel((taf_sim_Id_t)slot, TAF_SIM_APPTYPE_USIM, &channel);
    if(res != LE_OK) {
        return NULL;
    }
    LE_INFO("Logical channel opened channel id: %d", channel);

    res = SendApduOnChannel((taf_sim_Id_t)slot, channel, selectFPLMNApdu, sizeof(selectFPLMNApdu), responseAPDU, &responseLength);

    if(res != LE_OK || (uint8_t)apduResponse.sw1 != 0x61) {
        res = CloseLogicalChannel((taf_sim_Id_t)slot, channel);
        LE_INFO("ReadFplmnList: CloseLogicalChannel channel_id: %d res: %d", channel, res);
        return NULL;
    }
    LE_DEBUG("ReadFplmnList: After selectFPLMNApdu channel id: %d", channel);
    LE_INFO("selectFPLMNApdu sw1: %d, sw2: %d", (uint8_t)apduResponse.sw1, (uint8_t)apduResponse.sw2);

    uint8_t readBinaryFPLMNApdu[] = {0x00, 0xB0, 0x00, 0x00, 0x00};
    res = SendApduOnChannel((taf_sim_Id_t)slot, channel, readBinaryFPLMNApdu, sizeof(readBinaryFPLMNApdu), responseAPDU, &responseLength);

    if(res != LE_OK || (uint8_t)apduResponse.sw1 != 0x90 || (uint8_t)apduResponse.sw2 != 0x00) {
        res = CloseLogicalChannel((taf_sim_Id_t)slot, channel);
        LE_INFO("ReadFplmnList: CloseLogicalChannel channel_id: %d res: %d", channel, res);
        return NULL;
    }
    LE_INFO("ReadFplmnList: After readFPLMNApdu channel id: %d", channel);
    res = CloseLogicalChannel((taf_sim_Id_t)slot, channel);
    if(res != LE_OK) {
        return NULL;
    }

    taf_sim_FPLMNListRef_t listRef = CreateInternalFPLMNList();
    LE_INFO("ReadFPLMNList: apduResponse = %s", apduResponse.toString().c_str());
    int size = apduResponse.payload.length();
    LE_INFO("ReadFPLMNList: size of payload: %d, payload: %s", size, apduResponse.payload.c_str());

    for(int i=0; i<size/6; i++) {
        int k=6*i;
        char mcc[4], mnc[4];

        //1st byte:mcc[1] mcc[0]
        //2th byte:mnc[2] mcc[2]
        //3th byte:mnc[1] mnc[0]
        //42 F6 18 = mcc:246 mnc:81
        //60 f5 34 = mcc:65 mnc:43
        //e.g. mcc:246 mnc:81 = 42 F6 18. Here mcc[0] = 2, mcc[1] = 4, mcc[2] = 6 and mnc[0] = 8, mnc[1] = 1
        //e.g. mcc:65 mnc:43 = 40 65 91. Here mcc[0] = 0, mcc[1] = 6, mcc[2] = 5 and mnc[0] = 4, mnc[1] = 3

        mcc[0]=apduResponse.payload[k+1];
        mcc[1]=apduResponse.payload[k];
        mcc[2]=apduResponse.payload[k+3];
        if(apduResponse.payload[k+2]=='F' || apduResponse.payload[k+2]=='f') {
            mnc[0]=apduResponse.payload[k+5];
            mnc[1]=apduResponse.payload[k+4];
            mnc[2] = '\0';
        } else {
            mnc[0]=apduResponse.payload[k+5];
            mnc[1]=apduResponse.payload[k+4];
            mnc[2]=apduResponse.payload[k+2];
        }
        mcc[3] = '\0';
        mnc[3] = '\0';
        if (strncmp(mcc, "FFF", 3) != 0) {
            res = AddFPLMNOperatorInternal(listRef, mcc, mnc);
            if(res!=LE_OK) {
                DeleteFPLMNList(listRef);
                return NULL;
            }
            LE_INFO("FPLMN #%d - MCC:%s MNC:%s", i+1, mcc, mnc);
        }
    }

    return listRef;
}

void taf_sim::DeleteFPLMNList
(
    taf_sim_FPLMNListRef_t FPLMNListRef
)
{
    taf_sim_FPLMNList_t* ListReference = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if(ListReference == NULL) {
        return;
    }
    while(!le_dls_IsEmpty(&(ListReference->link))) {
        auto l=le_dls_Peek(&(ListReference->link));
        if (l != NULL) {
            le_dls_Remove(&(ListReference->link), l);
        }
    }
    fplmnListIndex = 0;
    le_mem_Release(ListReference);
}

le_result_t taf_sim::WriteFPLMNList
(
    taf_sim_Id_t simId,
    taf_sim_FPLMNListRef_t FPLMNListRef
)
{
    //Select the EF
    uint8_t selectFPLMNApdu[] = {0x00, 0xA4, 0x08, 0x04, 0x04, 0x7F, 0xFF, 0x6F, 0x7B};
    uint8_t responseAPDU[100];
    size_t responseLength = 100;
    uint8_t channel = 0;
    if(selectSimSlot(simId) != LE_OK) {
        return LE_FAULT;
    }
    le_result_t res = OpenLogicalChannel((taf_sim_Id_t)slot, TAF_SIM_APPTYPE_USIM, &channel);
    if(res != LE_OK) {
        return LE_FAULT;
    }
    LE_INFO("WriteFPLMNList: OpenLogicalChannel channel id: %d", channel);
    const uint8_t channel_id = channel;
    res = SendApduOnChannel((taf_sim_Id_t)slot, channel, selectFPLMNApdu, sizeof(selectFPLMNApdu), responseAPDU, &responseLength);
    if(res != LE_OK || (uint8_t)apduResponse.sw1 != 0x61) {
        res = CloseLogicalChannel((taf_sim_Id_t)slot, channel_id);
        LE_INFO("WriteFPLMNList: CloseLogicalChannel channel_id: %d res: %d", channel_id, res);
        return LE_FAULT;
    }
    LE_DEBUG("WriteFPLMNList: After selectFPLMNApdu channel id: %d and channel_id: %d", channel, channel_id);
    LE_INFO("selectFPLMNApdu sw1: %d, sw2: %d", (uint8_t)apduResponse.sw1, (uint8_t)apduResponse.sw2);

    taf_sim_FPLMNList_t* ListReference = (taf_sim_FPLMNList_t*)le_ref_Lookup(FPLMNListRefMap, FPLMNListRef);
    if(ListReference == NULL) {
        res = CloseLogicalChannel((taf_sim_Id_t)slot, channel_id);
        LE_INFO("WriteFPLMNList: CloseLogicalChannel channel_id: %d res: %d", channel_id, res);
        return LE_FAULT;
    }

    std::vector<uint8_t> writeFPLMNListApdu = {0x00, 0xD6, 0x00, 0x00, 0x00};

    auto nodeLink=le_dls_Peek(&(ListReference->link));

    while (nodeLink != NULL) {
        FPLMNNode_t* node=CONTAINER_OF(nodeLink, FPLMNNode_t, link);
        LE_INFO("WriteFPLMNList: node->mcc:%s, node->mnc:%s", node->mcc, node->mnc);

        //1st byte:mcc[1] mcc[0]
        //2th byte:mnc[2] mcc[2]
        //3th byte:mnc[1] mnc[0]
        //mcc:246 mnc:81 = 42 F6 18
        //mcc:65 mnc:43 = 60 f5 34
        //e.g. mcc:246 mnc:81 = 42 F6 18. Here mcc[0] = 2, mcc[1] = 4, mcc[2] = 6 and mnc[0] = 8, mnc[1] = 1
        //e.g. mcc:65 mnc:43 = 40 65 91. Here mcc[0] = 0, mcc[1] = 6, mcc[2] = 5 and mnc[0] = 4, mnc[1] = 3

        int mccInt = atoi(node->mcc);
        int mncInt = atoi(node->mnc);
        uint8_t mcc0 = (int) mccInt/100;
        uint8_t mcc1 = (int) (mccInt%100)/10;
        uint8_t mcc2 = (int) mccInt%10;
        uint8_t mnc0, mnc1, mnc2;
        if (mncInt > 99) {
            mnc0 = (int) mncInt/100;
            mnc1 = (int) (mncInt%100)/10;
            mnc2 = (int) mncInt%10;
        } else {
            mnc0 = (int) mncInt/10;
            mnc1 = (int) mncInt%10;
            mnc2 = 0x0F;
        }

        LE_DEBUG("WriteFPLMNList: mcc0:%d, mcc1:%d, mcc2:%d and mnc0:%d, mnc1:%d, mnc2:%d", (int) mcc0, (int) mcc1,
                (int) mcc2, (int) mnc0, (int) mnc1, (int) mnc2);

        writeFPLMNListApdu.emplace_back(mcc1*16+mcc0);
        writeFPLMNListApdu.emplace_back(mnc2*16+mcc2);
        writeFPLMNListApdu.emplace_back(mnc1*16+mnc0);

        nodeLink = le_dls_PeekNext(&(ListReference->link), nodeLink);
    }
    uint8_t sizeOfwriteFPLMNListApdu = writeFPLMNListApdu.size();
    LE_INFO("WriteFPLMNList: Total no of data(p3): %d", sizeOfwriteFPLMNListApdu);
    writeFPLMNListApdu.at(4) = sizeOfwriteFPLMNListApdu - 5;

    res = SendApduOnChannel((taf_sim_Id_t)slot, channel_id, writeFPLMNListApdu.data(), sizeOfwriteFPLMNListApdu, responseAPDU, &responseLength);
    if(res != LE_OK || (uint8_t)apduResponse.sw1 != 0x90 || (uint8_t)apduResponse.sw2 != 0x00) {
        res = CloseLogicalChannel((taf_sim_Id_t)slot, channel_id);
        LE_INFO("WriteFPLMNList: CloseLogicalChannel channel_id: %d res: %d", channel_id, res);
        return LE_FAULT;
    }

    res = CloseLogicalChannel((taf_sim_Id_t)slot, channel_id);
    LE_INFO("WriteFPLMNList: CloseLogicalChannel channel_id: %d res: %d", channel_id, res);
    if(res != LE_OK) {
        return LE_FAULT;
    }

    return LE_OK;
}

le_result_t taf_sim::getSlotCount(int *count) {
    bool isReady = multiSimMgr->isSubsystemReady();
    LE_INFO("getSlotCount: is multi SIM subSystem ready: %d", isReady);
    if (count == NULL) {
        LE_ERROR("GetSlotCount failed! as count is NULL");
        return LE_FAULT;
    }
    *count = 1; //Single SIM by default
    if (isReady) {
        int slotCount;
        if (telux::common::Status::SUCCESS == multiSimMgr->getSlotCount(slotCount)) {
            *count = slotCount;
            LE_INFO("getSlotCount: success, Slot Count: %d", slotCount);
            return LE_OK;
        } else {
            LE_ERROR("GetSlotCount failed!!!");
            return LE_FAULT;
        }
    }

    LE_ERROR("GetSlotCount failed because multi sim sub system is not ready");
    return LE_FAULT;
}
