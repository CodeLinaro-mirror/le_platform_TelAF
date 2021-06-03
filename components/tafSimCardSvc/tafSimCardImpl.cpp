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

static taf_sim_info_t simList[TAF_SIM_ID_MAX];

void tafCardListener:: onCardInfoChanged(int slotId)
{
    auto &sim = taf_sim::GetInstance();
    sim_event_t simEvent;
    simEvent.simId = (taf_sim_Id_t)slotId;
    simEvent.state =  sim.getState((taf_sim_Id_t)slotId);
    if (simEvent.state == TAF_SIM_ABSENT) {
        sim.InitializeSimInfo(nullptr, (taf_sim_Id_t)slotId);
    }
    le_event_Report(sim.NewStateEventId, &simEvent, sizeof(simEvent));
}

void tafSubscriptionListener:: onSubscriptionInfoChanged
                         (std::shared_ptr<telux::tel::ISubscription> subscription) {
    LE_INFO("onSubscriptionInfoChanged");
    if(subscription) {
        auto &sim = taf_sim::GetInstance();
        sim.InitializeSimInfo(subscription,(taf_sim_Id_t) subscription->getSlotId());
    } else {
        LE_INFO("Subscription is empty");
    }
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
        simPtr->pinTryCount -= 1;
        simResponsePtr.result = LE_FAULT ;
    } else {
        LE_INFO("Change Card Pin Request successful retryCount:%d",retryCount);
        simPtr->pinTryCount = 3;
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
        simPtr->pukTryCount -= 1;
        simResponsePtr.result = LE_FAULT ;
    } else {
        LE_INFO("Unlock Card By Puk request successful retryCount:%d",retryCount);
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
        simPtr->pinTryCount -= 1;
        LE_INFO("Unlock Card By Pin Request failed with errorCode: %d ",(int)error);
        LE_INFO( "Unlock Card By Pin Request failed retryCount: %d",retryCount);
        simResponsePtr.result = LE_FAULT ;
    } else {
        simPtr->pinTryCount = 3;
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
    if(error != telux::common::ErrorCode::SUCCESS) {
        LE_INFO("Set card lock Request failed with errorCode: %d ",(int)error);
        LE_INFO( "Set card lock Request failed retryCount: %d",retryCount);
        simResponsePtr.result = LE_FAULT ;
    } else {
        LE_INFO( "Set card lock Request successful retryCount: %d",retryCount);
        simResponsePtr.result = LE_OK;
    }
    le_event_Report(sim.ResponseEventId, &simResponsePtr,sizeof(simResponsePtr));
}

void taf_sim::Init(void)
{
    // Get the PhoneFactory and SubscriptionManager instances.
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    subMgr = phoneFactory.getSubscriptionManager();
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
        ResponseEventId = le_event_CreateId("ResponseEventId", sizeof(sim_response_event_t));
    }

    bool subscriptionSubSystemStatus = subMgr->isSubsystemReady();
    if(!subscriptionSubSystemStatus) {
        LE_INFO("Subscription subsystem is not ready" );
        LE_INFO( "wait for it to be ready " );
        std::future<bool> f = subMgr->onSubsystemReady();
        // If we want to wait unconditionally for telephony subsystem to be ready
        subscriptionSubSystemStatus = f.get();
    }
    if(subscriptionSubSystemStatus) {

        telux::common::Status status;
        // registering Listener
        subscriptionListener = std::make_shared<tafSubscriptionListener>();
        status = subMgr->registerListener(subscriptionListener);

        if(status != telux::common::Status::SUCCESS) {
            LE_INFO("Unable to registerListener");
        }
    }

    for (auto i = 0; i < TAF_SIM_ID_MAX; i++)
    {
        simList[i].simId = (taf_sim_Id_t)(i + 1);
        simList[i].ICCID[0] = '\0';
        simList[i].IMSI[0] = '\0';
        simList[i].phoneNumber[0] = '\0';
        simList[i].pinTryCount = 3;
        simList[i].pukTryCount = 10;
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

taf_sim_info_t* taf_sim::GetSimContext(taf_sim_Id_t simId) {
    simId = (TAF_SIM_UNSPECIFIED == simId) ? (taf_sim_Id_t)slot : simId;
    return &simList[simId - 1];
}

bool taf_sim::isValidSimId(taf_sim_Id_t simId) {
    int slotCount = 0;
    cardManager->getSlotCount(slotCount);
    if ((simId > 0 && simId <= slotCount) || simId == TAF_SIM_UNSPECIFIED ) {
        return true;
    }
    return false;
}

le_result_t taf_sim::selectSimSlot(taf_sim_Id_t simId) {
    if(isValidSimId(simId)) {
        if (slot != simId &&
                simId != TAF_SIM_UNSPECIFIED) {
            slot = simId;
        }
        return LE_OK;
    }
    return LE_FAULT;
}

void taf_sim::InitializeSimInfo(std::shared_ptr<telux::tel::ISubscription> subscription,
                             taf_sim_Id_t simId) {
    LE_INFO("InitializeSimInfo");
    taf_sim_info_t* simPtr = NULL;
    simPtr = GetSimContext(simId);
    if (subscription) {
        simPtr->simId = (taf_sim_Id_t)subscription->getSlotId();
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

le_result_t taf_sim::getICCID(taf_sim_Id_t simId, char *iccid, int length ) {
    string iccId = "";
    taf_sim_info_t* simPtr = NULL;
    telux::common::Status status;
    if (selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    simPtr = GetSimContext(simId);
    if (simPtr->ICCID[0] != 0) {
        return le_utf8_Copy(iccid, simPtr->ICCID, length, NULL);
    }
    auto subscription = subMgr->getSubscription(slot, &status);
    if(subscription != nullptr) {
        iccId = subscription->getIccId();
    } else {
        LE_INFO("subscription is empty");
        return LE_NOT_FOUND;
    }
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
    telux::common::Status status;
    auto subscription = subMgr->getSubscription(slot, &status);
    if(subscription != nullptr) {
        phoneNumberString = subscription->getPhoneNumber();
    } else {
        LE_INFO("subscription is empty");
        return LE_NOT_FOUND;
    }
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
    telux::common::Status status;
    auto subscription = subMgr->getSubscription(slot, &status);
    if(subscription != nullptr) {
        imsiString = subscription->getImsi();
    } else {
        return LE_NOT_FOUND;
    }
    le_utf8_Copy(simPtr->IMSI, imsiString.c_str(), length, NULL);
    return le_utf8_Copy(imsi, imsiString.c_str(), length, NULL);
}

le_result_t taf_sim::getHomeNetworkOperator(taf_sim_Id_t simId, char *name, int length) {
    string nameString = "";
    if (selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    telux::common::Status status;
    auto subscription = subMgr->getSubscription(slot, &status);
    if(subscription != nullptr) {
        nameString = subscription->getCarrierName();
    } else {
        LE_INFO("subscription is empty");
        return LE_NOT_FOUND;
    }
    return le_utf8_Copy(name, nameString.c_str(), length, NULL);
}

le_result_t taf_sim::getHomeNetworkMccMnc(taf_sim_Id_t simId, char *mccPtr,
        int mccPtrSize, char *mncPtr, int mncPtrSize) {
    int mcc = 0;
    int mnc = 0;
    if (selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    telux::common::Status status;
    auto subscription = subMgr->getSubscription(slot, &status);
    if(subscription != nullptr) {
        mcc = subscription->getMcc();
        mnc = subscription->getMnc();
    } else {
        LE_INFO("subscription is empty");
        return LE_NOT_FOUND;
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
    auto card = cards[slot - 1];
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
            if(cardApp->getAppType() == telux::tel::AppType::APPTYPE_USIM) {
                auto ret = cardApp->unlockCardByPin(cardLockType, newPin,
                        tafAuthenticationResponseCallback::unlockCardByPinResponseCb);
                if(ret == telux::common::Status::SUCCESS) {
                    LE_INFO("Unlock card by pin request sent successfully\n");
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
    return LE_OK;
}

le_result_t taf_sim::ChangeCardPin( taf_sim_Id_t simId, taf_sim_LockType_t lockType,
        const char* oldpinPtr, const char* newpinPtr) {
    if(selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }

    auto card = cards[slot - 1];

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
    return LE_OK;
}

le_result_t taf_sim::UnlockCardByPuk(taf_sim_Id_t  simId, taf_sim_LockType_t lockType,
        const char* pukPtr, const char* newpinPtr) {
    if(selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    auto card = cards[slot - 1];
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
    return LE_OK;
}


le_result_t taf_sim::SetCardLock(taf_sim_Id_t  simId, taf_sim_LockType_t lockType,
        const char* pinPtr, bool lockEnable) {
    if(selectSimSlot(simId) != LE_OK) {
        return LE_BAD_PARAMETER;
    }
    auto card = cards[slot - 1];
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
    return LE_OK;
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
    remainingPukTriesPtr = &simPtr->pukTryCount;
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

