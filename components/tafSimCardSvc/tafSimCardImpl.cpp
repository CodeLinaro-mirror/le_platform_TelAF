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
#include "telux/tel/PhoneFactory.hpp"
#include "tafSimCard.hpp"
#include <unistd.h>

using namespace telux::tel;
using namespace telux::common;
using namespace telux::tafsvc;
using namespace std;


void tafCardListener:: onCardInfoChanged(int slotId)
{
    auto &sim = taf_sim::GetInstance();
    sim_event_t simEvent;
    simEvent.simId = (taf_sim_Id_t)slotId;
    simEvent.state =  sim.getState((taf_sim_Id_t)slotId);
    le_event_Report(sim.NewStateEventId, &simEvent, sizeof(simEvent));
}

void taf_sim::Init(void)
{
    // Get the PhoneFactory and SubscriptionManager instances.
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    auto subMgr = phoneFactory.getSubscriptionManager();
    cardManager = phoneFactory.getCardManager();
    //  Check if telephony subsystem is ready
    bool subSystemStatus = cardManager->isSubsystemReady();

    // If telephony subsystem is not ready, wait for it to be ready
    if(!subSystemStatus) {
        LE_INFO("Subscription subsystem is not ready" );
        LE_INFO( "wait for it to be ready " );
        std::future<bool> f = cardManager->onSubsystemReady();
        // If we want to wait unconditionally for telephony subsystem to be ready
        subSystemStatus = f.get();
    }

    if(subSystemStatus) {
        std::vector<int> slotIds;
        telux::common::Status status = cardManager->getSlotIds(slotIds);
        if (status == telux::common::Status::SUCCESS) {
            for (auto index = 1; index <= (int)slotIds.size(); index++) {
                auto card = cardManager->getCard(index, &status);
                if (card != nullptr) {
                    cards.emplace_back(card);
                }
            }
        }

        // listener
        cardListener = std::make_shared<tafCardListener>();

        // registering Listener
        status = cardManager->registerListener(cardListener);
        if(status != telux::common::Status::SUCCESS) {
            LE_INFO("Unable to registerListener");
        }

        // Create an event Id for change in card info notification
        NewStateEventId = le_event_CreateId("NewStateEventId", sizeof(sim_event_t));
    }
}

taf_sim &taf_sim::GetInstance()
{
    static taf_sim instance;
    return instance;
}

taf_sim_States_t taf_sim::getState(taf_sim_Id_t simId) {

    int slotCount = 0;

    if (simId >= TAF_SIM_ID_MAX || simId <= 0) {
        LE_INFO("Invalid sim Id");
        return TAF_SIM_STATE_UNKNOWN;
    }
    cardManager->getSlotCount(slotCount);
    if (simId != TAF_SIM_UNSPECIFIED) {
        if (simId > slotCount) {
            return TAF_SIM_STATE_UNKNOWN;
        }
        slot = simId;
    }
    auto card = cards[slot - 1];
    telux::tel::CardState cardState;
    if(card) {
        card->getState(cardState);
        LE_INFO( "CardState : %s\n ",  cardStateToString(cardState)) ;
        if(cardState == telux::tel::CardState::CARDSTATE_PRESENT) {
            std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
            applications = card->getApplications();
            if(applications.size() != 0)  {
                for(auto cardApp : applications) {
                    if(cardApp->getAppType() == telux::tel::AppType::APPTYPE_USIM) {
                        if (cardApp->getAppState() == telux::tel::AppState::APPSTATE_READY) {
                            return TAF_SIM_READY;
                            break;
                        }
                    }
                }
            }
            return TAF_SIM_PRESENT;
        }
    }
    return cardStateToTafSimStates(cardState);
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
