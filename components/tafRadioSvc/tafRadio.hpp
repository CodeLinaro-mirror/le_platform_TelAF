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
 * @file       tafRadio.hpp
 * @brief      Internal interface for Radio Service object. The functions
 *             in this file are impletmented internally.
 */

#ifndef TAFRADIO_HPP
#define TAFRADIO_HPP

#include "legato.h"
#include "interfaces.h"

#include <vector>
#include <string>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/Phone.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/PhoneListener.hpp>

#include "tafSvcIF.hpp"

namespace telux {
namespace tafsvc {
    /*
     * @brief The class of functions used in Radio Services interfaces.
     */
    class taf_RadioFunctions {
    public:
        /*
         * This function is called when checking MCC and MNC from Client.
         *
         * @param [in] mccPtr    The mobile country code string.
         * @param [in] mncPtr    The mobile network code string.
         * @returns    Check result, LE_BAD_PARAMETER or LE_OK.
         */
        static le_result_t taf_radio_CheckMccMnc(const char* mccPtr, const char* mncPtr);
    };

    /*
     * @brief The phone listener is registered for the radio state updates.
     */
    class taf_RadioPhoneListener : public telux::tel::IPhoneListener {
    public:
        ~taf_RadioPhoneListener() {};
        /*
         * This function is called when radio state changes.
         *
         * @param [in] phoneId       The phone id.
         * @param [in] radioState    The radio state of phone.
         */
        void onRadioStateChanged(int phoneId, telux::tel::RadioState radioState) override;
    };

    /*
     * @brief The network listener is registered for the network selection mode updates.
     */
    class taf_RadioNetworkSelectionListener : public telux::tel::INetworkSelectionListener {
    public:
        /*
         * This function is called when network selection mode changes.
         *
         * @param [in] mode    The network selection mode of phone.
         */
        void onSelectionModeChanged(telux::tel::NetworkSelectionMode mode) override;
    };

    /*
     * @brief The sbscription listener is registered for the sbscriptions updates.
     */
    class taf_RadioSubscriptionListener : public telux::tel::ISubscriptionListener {
    public:
        /*
         * This function is called when subscription infomation changes.
         *
         * @param [in] subscription    A subscription pointer with infomation of phone.
         */
        void onSubscriptionInfoChanged(std::shared_ptr<telux::tel::ISubscription> subscription) override;
        /*
         * This function is called when subscription infomation changes.
         *
         * @param [in] count    The number of subscription.
         */
        void onNumberOfSubscriptionsChanged(int count) override;
    };

    /*
     * @brief A radio power callback class must be provided when configuring the radio power.
     */
    class taf_RadioPowerCallback : public telux::common::ICommandResponseCallback {
    public:
        /*
         * This function is called after configuration of radio power.
         *
         * @param [in] error    The error code of radio power configuration.
         */
        void commandResponse(telux::common::ErrorCode error);
    };

    /*
     * @brief A radio network selection callback class must be provided when configuring the network selection mode.
     */
    class taf_RadioNetworkResponsecallback {
    public:
        /*
         * This function is called after configuration of network selection mode.
         *
         * @param [in] error    The error code of network selection mode configuration.
         */
        static void setNetworkSelectionModeResponseCb(telux::common::ErrorCode error);
    };

    /*
     * @brief A radio network selection callback class must be provided when getting the network selection mode.
     */
    class taf_RadioSelectionModeResponseCallback {
    public:
        static bool isRegModeMannual;
        static le_sem_Ref_t semaphore;
        /*
         * This function is called after configuration of network selection mode.
         *
         * @param [in] networkSelectionMode    Automatic or mannual mode.
         * @param [in] error                   The error code of network selection mode configuration.
         */
        static void selectionModeResponse(telux::tel::NetworkSelectionMode networkSelectionMode,
            telux::common::ErrorCode error);
    };

    /*
     * @brief The Radio Service class defined as a middleware between interfaces and implementation.
     */
    class taf_Radio : public ITafSvc {
    public:
        taf_Radio() {};
        ~taf_Radio() {};

        /*
         * This function is used for visiting the members of instance.
         *
         * @returns    Static reference of instance.
         */
        static taf_Radio &GetInstance();
        /*
         * The initialization function of the Radio Service.
         */
        void Init(void);

        std::shared_ptr<taf_RadioPowerCallback> radioPowerCb;
        std::vector<std::shared_ptr<telux::tel::IPhone>> phones;
        std::vector<std::shared_ptr<telux::tel::INetworkSelectionManager>> networkManagers;
        std::shared_ptr<telux::tel::ISubscriptionManager> subscriptionManager;

    private:
        std::shared_ptr<telux::tel::IPhoneManager> phoneManager;
        std::shared_ptr<telux::tel::IPhoneListener> phoneListener;
        std::shared_ptr<telux::tel::INetworkSelectionListener> networkListener;
        std::shared_ptr<taf_RadioSubscriptionListener> subscriptionListener;
    };
}
}

#endif /* #ifndef TAFRADIO_H */
