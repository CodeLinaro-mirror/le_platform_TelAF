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
 * @file       tafRadioImpl.cpp
 * @brief      This file describes the implementation method that radio
 *             service is in use.
 */

#include <cstdlib>
#include <chrono>

#include "tafRadio.hpp"

using namespace std;
using namespace telux::tafsvc;

/*======================================================================

 FUNCTION        taf_radio_CheckMccMnc

 DESCRIPTION     Check whether MCC and MNC are all numbers.

 DEPENDENCIES    None

 PARAMETERS      [IN] const char* mccPtr: The mobile country code.
                 [IN] const char* mncPtr: The mobile network code.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_OK:            Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_RadioFunctions::taf_radio_CheckMccMnc(const char* mccPtr, const char* mncPtr)
{
    TAF_ERROR_IF_RET_VAL((mccPtr == nullptr), LE_BAD_PARAMETER,
        "Null ptr(mccPtr)");

    TAF_ERROR_IF_RET_VAL((mncPtr == nullptr), LE_BAD_PARAMETER,
        "Null ptr(mncPtr)");

    size_t mccLen = strlen(mccPtr);
    TAF_ERROR_IF_RET_VAL((mccLen != TAF_RADIO_MCC_LEN), LE_BAD_PARAMETER,
        "Mcc length %d != %d", mccLen, TAF_RADIO_MCC_LEN);

    size_t mncLen = strlen(mncPtr);
    TAF_ERROR_IF_RET_VAL((mncLen < TAF_RADIO_MNC_MIN_LEN || mncLen > TAF_RADIO_MNC_MAX_LEN), LE_BAD_PARAMETER,
        "Mnc error length %d", mncLen);

    for (size_t index = 0; index < mccLen; index++) {
        TAF_ERROR_IF_RET_VAL((mccPtr[index] < '0' || mccPtr[index] > '9'), LE_BAD_PARAMETER,
            "Error char %c in mcc, index %d", mccPtr[index], index);
    }

    for (size_t index = 0; index < mncLen; index++) {
        TAF_ERROR_IF_RET_VAL((mncPtr[index] < '0' || mncPtr[index] > '9'), LE_BAD_PARAMETER,
            "Error char %c in mnc, index %d", mncPtr[index], index);
    }

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_RadioPhoneListener::onRadioStateChanged

 DESCRIPTION     This function will be called when radio state changed.

 DEPENDENCIES    Phone listener registers to phone manager

 PARAMETERS      [IN] int phoneId:                  The phone id.
                 [IN] telux::tel::RadioState state: The radio state.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioPhoneListener::onRadioStateChanged(int phoneId, telux::tel::RadioState state)
{
    LE_DEBUG("<SDK Listener> taf_RadioPhoneListener --> onRadioStateChanged");
    LE_DEBUG("phoneId = %d", phoneId);
    LE_DEBUG("RadioState = %d", (int)state);
}

/*======================================================================

 FUNCTION        taf_RadioNetworkSelectionListener::onSelectionModeChanged

 DESCRIPTION     This function will be called when seletion state changed.

 DEPENDENCIES    Network selsection listener registers to network manager.

 PARAMETERS      [IN] telux::tel::NetworkSelectionMode mode:
                          The network selsetion mode.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioNetworkSelectionListener::onSelectionModeChanged(telux::tel::NetworkSelectionMode mode)
{
    LE_DEBUG("<SDK Listener> taf_RadioNetworkSelectionListener --> onSelectionModeChanged");
    LE_DEBUG("NetworkSelectionMode = %d", (int)mode);
}

/*======================================================================

 FUNCTION        taf_RadioSubscriptionListener::onSubscriptionInfoChanged

 DESCRIPTION     This function will be called when subscription infomation
                 changed.

 DEPENDENCIES    Subscription listener registers to subscription manager.

 PARAMETERS      [IN] std::shared_ptr<telux::tel::ISubscription> subscription:
                          A pointer with subscription infomation.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioSubscriptionListener::onSubscriptionInfoChanged(std::shared_ptr<telux::tel::ISubscription> subscription)
{
    LE_DEBUG("<SDK Listener> taf_RadioSubscriptionListener --> onSubscriptionInfoChanged");
    if (!subscription) {
        LE_ERROR("subscription is null");
    }
}

/*======================================================================

 FUNCTION        taf_RadioSubscriptionListener::onNumberOfSubscriptionsChanged

 DESCRIPTION     This function will be called when the subscription count changed.

 DEPENDENCIES    Subscription listener registers to subscription manager.

 PARAMETERS      [IN] int count: The count of the subscriptions.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioSubscriptionListener::onNumberOfSubscriptionsChanged(int count)
{
    LE_DEBUG("<SDK Listener> taf_RadioSubscriptionListener --> onNumberOfSubscriptionsChanged");
    LE_DEBUG("count = %d", count);
}

/*======================================================================

 FUNCTION        taf_RadioPowerCallback::commandResponse

 DESCRIPTION     The callback function used in powering on or off the radio.

 DEPENDENCIES    Call the low level function when powering on or off the
                 radio.

 PARAMETERS      [IN] telux::common::ErrorCode error:
                          Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioPowerCallback::commandResponse(telux::common::ErrorCode error)
{
    LE_DEBUG("<SDK Callback> taf_RadioPowerCallback --> commandResponse");
    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("Error(%d)", (int)error);
    }
}

/*======================================================================

 FUNCTION        taf_RadioNetworkResponsecallback::setNetworkSelectionModeResponseCb

 DESCRIPTION     The callback function used in setting the network selection mode.

 DEPENDENCIES    Call the low level function when setting the network selection mode.

 PARAMETERS      [IN] telux::common::ErrorCode error: Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioNetworkResponsecallback::setNetworkSelectionModeResponseCb(telux::common::ErrorCode error)
{
    LE_DEBUG("<SDK Callback> taf_RadioNetworkResponsecallback --> setNetworkSelectionModeResponseCb");
    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("Error(%d)", (int)error);
    }
}

bool taf_RadioSelectionModeResponseCallback::isRegModeMannual = true;

le_sem_Ref_t taf_RadioSelectionModeResponseCallback::semaphore = NULL;

/*======================================================================

 FUNCTION        taf_RadioSelectionModeResponseCallback::selectionModeResponse

 DESCRIPTION     The callback function used in getting register
                 mode(automatic/manual) of radio.

 DEPENDENCIES    Call the low level function when getting the network selection mode.

 PARAMETERS      [IN] telux::tel::NetworkSelectionMode networkSelectionMode:
                          Register mode of radio.
                 [IN] telux::common::ErrorCode error: Error code defined in SDK.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_RadioSelectionModeResponseCallback::selectionModeResponse
(
    telux::tel::NetworkSelectionMode networkSelectionMode,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> taf_RadioSelectionModeResponseCallback --> selectionModeResponse");

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (networkSelectionMode == telux::tel::NetworkSelectionMode::AUTOMATIC) {
            isRegModeMannual = false;
        } else {
            isRegModeMannual = true;
        }
    } else {
        LE_ERROR("Error(%d)", (int)error);
    }

    le_sem_Post(semaphore);
}

/*======================================================================

 FUNCTION        taf_Radio::GetInstance

 DESCRIPTION     Get the instance of radio.

 DEPENDENCIES    The initialization of Radio.

 PARAMETERS      None

 RETURN VALUE    taf_Radio&

 SIDE EFFECTS

======================================================================*/
taf_Radio &taf_Radio::GetInstance()
{
    static taf_Radio instance;
    return instance;
}

/*======================================================================

 FUNCTION        taf_Radio::Init

 DESCRIPTION     Initialization of the Radio Service

 DEPENDENCIES    The initialization of telaf.

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Radio::Init(void)
{
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    std::chrono::duration<double> elapsedTime;
    startTime = std::chrono::system_clock::now();

    //  1. Get the PhoneFactory and PhoneManager instances
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    phoneManager = phoneFactory.getPhoneManager();

    //  2. Check if telephony subsystem is ready
    bool subSystemStatus = phoneManager->isSubsystemReady();
    if (!subSystemStatus) {
        LE_INFO("Telephony subsystem wait to be ready...");
        future<bool> f = phoneManager->onSubsystemReady();
        //  Wait until the subsystem is ready.
        subSystemStatus = f.get();
    }

    if (subSystemStatus) {
        endTime = std::chrono::system_clock::now();
        elapsedTime = endTime - startTime;
        LE_INFO("Elapsed time for telephony subsystem: %lfs", elapsedTime.count());

        //  3. Instantiate Phone
        std::vector<int> phoneIds;
        telux::common::Status status = phoneManager->getPhoneIds(phoneIds);
        if (status == telux::common::Status::SUCCESS) {
            for (size_t index = 1; index <= phoneIds.size(); index++) {
                auto phone = phoneManager->getPhone(index);
                if (phone != nullptr) {
                    phones.emplace_back(phone);
                }
                auto networkManager = telux::tel::PhoneFactory::getInstance().getNetworkSelectionManager(index);
                if (networkManager != nullptr) {
                    networkManagers.emplace_back(networkManager);
                }
            }
        }

        //  4. Instantiate RadioPhoneListener
        phoneListener = std::make_shared<taf_RadioPhoneListener>();

        //  5. Register for phone info updates
        if (phoneManager->registerListener(phoneListener) != telux::common::Status::SUCCESS) {
            LE_ERROR("Failed to register phone listener");
        }

        //  6. Instantiate RadioPowerCallback
        radioPowerCb = std::make_shared<taf_RadioPowerCallback>();

        //  7. Set the Radio power on
        for (size_t index = 0; index < phones.size(); index++) {
            if(phones[index]->getRadioState() != telux::tel::RadioState::RADIO_STATE_ON) {
                phones[index]->setRadioPower(true, radioPowerCb);
            }
        }

        for (size_t index = 0; index < networkManagers.size(); index++) {
            startTime = std::chrono::system_clock::now();

            //  8. Check if network subsystem is ready
            bool networkSystemStatus = networkManagers[index]->isSubsystemReady();
            if (!networkSystemStatus) {
                LE_INFO("Network subsystem wait to be ready...");
                std::future<bool> f = networkManagers[index]->onSubsystemReady();
                //  Wait until the subsystem is ready.
                networkSystemStatus = f.get();
            }

            if (networkSystemStatus) {
                endTime = std::chrono::system_clock::now();
                elapsedTime = endTime - startTime;
                LE_INFO("Elapsed time for %d network subsystem: %lfs", index, elapsedTime.count());
            } else {
                LE_ERROR("Fail to init %d network subsystem", index);
            }

            //  9. Instantiate NetworkListener
            networkListener = std::make_shared<taf_RadioNetworkSelectionListener>();
            if (networkManagers[index]->registerListener(networkListener) != telux::common::Status::SUCCESS) {
                LE_ERROR("Failed to register network listener");
            }
        }
    } else {
        LE_ERROR("Fail to init telephony subsystem");
    }

    //  10. Get the SubscriptionManager instances
    subscriptionManager = phoneFactory.getSubscriptionManager();

    //  11. Check if subscription subsystem is ready
    startTime = std::chrono::system_clock::now();
    subSystemStatus = subscriptionManager->isSubsystemReady();

    if (!subSystemStatus) {
        LE_INFO("Subscription subsystem wait to be ready...");
        std::future<bool> f = subscriptionManager->onSubsystemReady();
        //  Wait until the subsystem is ready.
        subSystemStatus = f.get();
    }

    if (subSystemStatus) {
        endTime = std::chrono::system_clock::now();
        elapsedTime = endTime - startTime;
        LE_INFO("Elapsed time for subscription subsystem: %lfs", elapsedTime.count());
    } else {
        LE_ERROR("Fail to init subscription subsystem");
    }

    // 12. Register listener with Subscription Manager for the notification
    subscriptionListener = std::make_shared<taf_RadioSubscriptionListener>();
    subscriptionManager->registerListener(subscriptionListener);

    taf_RadioSelectionModeResponseCallback::semaphore = le_sem_Create("taf_RadioSelModeRespCbSem", 0);
}
