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

#define TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM 2
#define TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM 100

/*
 * @brief The struct of safe reference for prefered operators in network.
 */
typedef struct
{
    void* safeRef;
    le_sls_Link_t link;
} taf_RadioPrefOpSafeRef_t;

/*
 * @brief The struct of prefered operator in network.
 */
typedef struct
{
    telux::tel::PreferredNetworkInfo info;
    le_sls_Link_t link;
} taf_RadioPrefOp_t;

/*
 * @brief The struct of prefered operator list in network.
 */
typedef struct
{
    le_sls_List_t prefOpList;
    le_sls_List_t safeRefList;
    le_sls_Link_t* currPtr;
} taf_RadioPrefOpList_t;

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
     * @brief A radio tech callback class must be provided when request for the rat in use.
     */
    class taf_RadioVoiceRadioTechnologyCallback {
    public:
        le_sem_Ref_t semaphore;
        telux::tel::RadioTechnology radioTech;
        /*
         * This function is called after getting the rat in use.
         *
         * @param [in] radioTechnology    The radio technology in use.
         * @param [in] error              The error code of getting the rat in use.
         */
        void voiceRadioTechnologyResponse(telux::tel::RadioTechnology radioTechnology, telux::common::ErrorCode error);
    };

    /*
     * @brief A voice service state callback class must be provided when request for the service state.
     */
    class taf_RadioVoiceServiceStateCallback : public telux::tel::IVoiceServiceStateCallback {
    public:
        static le_sem_Ref_t semaphore;
        static telux::tel::VoiceServiceState vocSrvState;
        /*
         * This function is called after getting the service state.
         *
         * @param [in] serviceInfo    The service information.
         * @param [in] error          The error code of getting the service state.
         */
        void voiceServiceStateResponse(const std::shared_ptr<telux::tel::VoiceServiceInfo> &serviceInfo,
            telux::common::ErrorCode error) override;
};

    /*
     * @brief A signal strength callback class must be provided when request for the signal strength.
     */
    class taf_RadioSignalStrengthCallback : public telux::tel::ISignalStrengthCallback {
    public:
        le_sem_Ref_t semaphore;
        telux::tel::SignalStrengthLevel signalStrengthLevel;
        /*
         * This function is called after getting the signal strength.
         *
         * @param [in] signalStrength    The signal strength information.
         * @param [in] error             The error code of getting the signal strength.
         */
        void signalStrengthResponse(std::shared_ptr<telux::tel::SignalStrength> signalStrength,
            telux::common::ErrorCode error) override;
    };

    /*
     * @brief A radio network selection callback class must be provided when configuring the network selection mode
     *        and network preference.
     */
    class taf_RadioNetworkResponseCallback {
    public:
        static le_sem_Ref_t semaphore;
        /*
         * This function is called after configuration of network selection mode.
         *
         * @param [in] error    The error code of network selection mode configuration.
         */
        static void setNetworkSelectionModeResponseCb(telux::common::ErrorCode error);
        /*
         * This function is called after configuration of network preference.
         *
         * @param [in] error    The error code of network preference configuration.
         */
        static void setPreferredNetworksResponseCb(telux::common::ErrorCode error);
    };

    /*
     * @brief A radio preferred networks callback class must be provided when getting the network preference.
     */
    class taf_RadioPreferredNetworksResponseCallback {
    public:
        static std::vector<telux::tel::PreferredNetworkInfo> preferredNetworksInfo;
        static le_sem_Ref_t semaphore;
        /*
         * This function is called after  getting the network preference.
         *
         * @param [in] preferredNetworks3gppInfo      The non-static prefered network.
         * @param [in] staticPreferredNetworksInfo    The static prefered network.
         * @param [in] error                          The error code of radio power configuration.
         */
        static void preferredNetworksResponse(
            std::vector<telux::tel::PreferredNetworkInfo> preferredNetworks3gppInfo,
            std::vector<telux::tel::PreferredNetworkInfo> staticPreferredNetworksInfo,
            telux::common::ErrorCode error);
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
     * @brief A service domain callback class must be provided when getting the service domain preference.
     */
    class taf_RadioServiceDomainResponseCallback {
    public:
        static telux::tel::ServiceDomainPreference svcDomainPref;
        static le_sem_Ref_t semaphore;
        /*
         * This function is called after getting of the service domain preference.
         *
         * @param [in] preference    Circuit Switched only(CS), Packet Switched only(PS) or Circuit and Packet Switched.
         * @param [in] error         The error code of the service domain preference configuration.
         */
        static void serviceDomainResponse(telux::tel::ServiceDomainPreference preference,
            telux::common::ErrorCode error);
    };

    /*
     * @brief A serving system callback class must be provided when configuring rat preference.
     */
    class taf_RadioServingSystemResponseCallback {
    public:
        /*
         * This function is called after configuration of the rat preference.
         *
         * @param [in] error         The error code of the rat preference configuration.
         */
        static void servingSystemResponse(telux::common::ErrorCode error);
    };

    /*
     * @brief A serving system callback class must be provided when getting rat preference.
     */
    class taf_RadioRatPreferenceResponseCallback {
    public:
        static le_sem_Ref_t semaphore;
        static telux::tel::RatPreference ratPref;
        /*
         * This function is called after getting of the service domain preference.
         *
         * @param [in] preference    Rat preference.
         * @param [in] error         The error code of the service domain preference configuration.
         */
        static void ratPreferenceResponse(telux::tel::RatPreference preference, telux::common::ErrorCode error);
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

        static le_mem_PoolRef_t prefOpsListPool;
        static le_mem_PoolRef_t prefOpPool;
        static le_mem_PoolRef_t prefOpSafeRefPool;
        static le_ref_MapRef_t prefOpListRefMap;
        static le_ref_MapRef_t prefOpSafeRefMap;
        std::shared_ptr<taf_RadioPowerCallback> radioPowerCb;
        std::shared_ptr<taf_RadioVoiceServiceStateCallback> voiceSrvStateCb;
        std::shared_ptr<taf_RadioVoiceRadioTechnologyCallback> voiceRadioTechCb;
        std::shared_ptr<taf_RadioSignalStrengthCallback> signalStrengthCb;
        std::vector<std::shared_ptr<telux::tel::IPhone>> phones;
        std::vector<std::shared_ptr<telux::tel::INetworkSelectionManager>> networkManagers;
        std::vector<std::shared_ptr<telux::tel::IServingSystemManager>> servingSystemManagers;
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
