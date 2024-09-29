/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlan.hpp
 *
 * @brief      Header file for TelAF WLAN Device Management Service.
 *
 */

#pragma once

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include <telux/wlan/WlanDefines.hpp>
#include <telux/wlan/WlanFactory.hpp>
#include <telux/wlan/WlanDeviceManager.hpp>

#include <future>
#include <sstream>

#include "tafWlanAP.hpp"
#include "tafWlanSTA.hpp"

#define TAF_WLAN_MAX_SESSION_REF 20

// ServiceSet string
#define TAF_WLAN_ESS_STR "ESS"

// SecurityMode Strings
#define TAF_WLAN_SEC_MODE_WEP_STR "WEP"
#define TAF_WLAN_SEC_MODE_WPA_STR  "WPA"
#define TAF_WLAN_SEC_MODE_WPA2_STR "WPA2"
#define TAF_WLAN_SEC_MODE_WPA3_STR "WPA3"

// SecurityAuthMethod strings
#define TAF_WLAN_SEC_AUTH_METHOD_PSK_STR "PSK"
#define TAF_WLAN_SEC_AUTH_METHOD_EAP_STR "EAP"
#define TAF_WLAN_SEC_AUTH_METHOD_SAE_STR "SAE"
// SecurityAuthMethod EAP strings
#define TAF_WLAN_SEC_AUTH_METHOD_EAP_SIM_STR  "SIM"
#define TAF_WLAN_SEC_AUTH_METHOD_EAP_AKA_STR  "AKA"
#define TAF_WLAN_SEC_AUTH_METHOD_EAP_LEAP_STR "LEAP"
#define TAF_WLAN_SEC_AUTH_METHOD_EAP_TLS_STR  "TLS"
#define TAF_WLAN_SEC_AUTH_METHOD_EAP_TTLS_STR "TTLS"
#define TAF_WLAN_SEC_AUTH_METHOD_EAP_PEAP_STR "PEAP"
#define TAF_WLAN_SEC_AUTH_METHOD_EAP_FAST_STR "FAST"
#define TAF_WLAN_SEC_AUTH_METHOD_EAP_PSK_STR  "PSK"

// SecurityEncryptionMethod strings
#define TAF_WLAN_SEC_ENCRYPT_METHOD_RC4_STR  "RC4"
#define TAF_WLAN_SEC_ENCRYPT_METHOD_TKIP_STR "TKIP"
#define TAF_WLAN_SEC_ENCRYPT_METHOD_AES_STR  "AES"
#define TAF_WLAN_SEC_ENCRYPT_METHOD_CCMP_STR "CCMP"
#define TAF_WLAN_SEC_ENCRYPT_METHOD_GCMP_STR "GCMP"

// SecurityEncryptionMethod strings
#define TAF_WLAN_WPS_STR "WPS"

namespace telux
{
    namespace tafsvc
    {
        //------------------------------------------------------------------------------------------
        /**
        * WLAN device state change event structure.
        */
        //------------------------------------------------------------------------------------------
        typedef struct
        {
            taf_wlan_DeviceState_t wlanDeviceState;
        }
        WlanDevStateChangeEvent_t;

        //------------------------------------------------------------------------------------------
        /**
        * The TelAF WLAN helper class. It provides the following:
        *   - Functions to transform TelAF to TelSDK values and vice-versa
        */
        //------------------------------------------------------------------------------------------
        class taf_WlanHelper
        {
            public:

            // String helper functions
            static std::string StrTrimEndSpace(const std::string &str);
            static std::vector<std::string> StrSplit(const std::string &str, char delim);

            // BandType conversion
            static taf_wlan_Band_t BandTypeToTAF (telux::wlan::BandType bandType);
            static telux::wlan::BandType BandTypeToTelux (taf_wlan_Band_t bandType);

            ///////////////////////////////////////
            // APType conversion
            static taf_wlan_APType_t APTypeToTAF (telux::wlan::ApType APType);
            static telux::wlan::ApType APTypeToTelux(taf_wlan_APType_t APType);

            // SecMode conversion
            static taf_wlan_SecurityMode_t SecModeToTAF(telux::wlan::SecMode SecMode);
            static telux::wlan::SecMode SecModeToTelux(taf_wlan_SecurityMode_t SecMode);

            // SecAuth conversion
            static taf_wlan_SecurityAuthMethod_t SecAuthToTAF(telux::wlan::SecAuth SecAuth);
            static telux::wlan::SecAuth SecAuthToTelux(taf_wlan_SecurityAuthMethod_t SecAuth);

            // SecEncrypt conversion
            static taf_wlan_SecurityEncryptionMethod_t
                            SecEncryptToTAF(telux::wlan::SecEncrypt SecEncrypt);
            static telux::wlan::SecEncrypt
            SecEncryptToTelux(taf_wlan_SecurityEncryptionMethod_t SecEncrypt);

            // WLAN AP ID conversion
            static taf_wlan_APid_t TeluxIdtoTAFAPId(telux::wlan::Id id);
            static telux::wlan::Id TAFAPidtoTeluxId(taf_wlan_APid_t id);

            ////////////////////////////////////////////////
            // Station Mode(Bridge/Router) conversion
            static taf_wlanSta_Mode_t
            StaModeToTAF(telux::wlan::StaBridgeMode Mode);
            static telux::wlan::StaBridgeMode StaModeToTelux(taf_wlanSta_Mode_t Mode);

            // Station IP Type conversion
            static taf_wlanSta_IPType_t StaIPTypeToTAF(telux::wlan::StaIpConfig IPMode);
            static telux::wlan::StaIpConfig StaIPTypeToTelux(taf_wlanSta_IPType_t IPMode);

            // Station state conversion
            static taf_wlanSta_State_t StaIntfStatusToTAF(telux::wlan::StaInterfaceStatus State);

            // WLAN STA ID conversion
            static taf_wlan_STAid_t TeluxIdtoTAFSTAId(telux::wlan::Id id);
            static telux::wlan::Id TAFSTAidtoTeluxId(taf_wlan_STAid_t id);
        };

        //------------------------------------------------------------------------------------------
        /**
        * The TelAF WLAN listener class for TelSDK notifications.
        */
        //------------------------------------------------------------------------------------------
        class taf_WlanListener: public telux::wlan::IWlanListener
        {
            public:
                // Subsystem state change handler
                void onServiceStatusChange (telux::common::ServiceStatus status);
                // Device enalbe/disable handler
                void onEnableChanged (bool enable);
                bool getEnableStatus();
                void resetPromise();
            private:
                std::promise<bool> promise_;
        };

        //------------------------------------------------------------------------------------------
        /**
        * The TelAF WLAN APIs implementation class.
        */
        //------------------------------------------------------------------------------------------
        class taf_WlanSvcImpl : public ITafSvc
        {
        public:
            // Inherited functions
            void Init(void);
            taf_WlanSvcImpl() {};
            ~taf_WlanSvcImpl() {};

            static taf_WlanSvcImpl &GetInstance();

            // WLan Service Implementations
            le_result_t SetON             ( void );
            le_result_t SetOFF            ( void );
            le_result_t SetMode           ( taf_wlan_DeviceMode_t wlanMode );
            le_result_t GetMode           ( taf_wlan_DeviceMode_t* wlanModePtr );
            le_result_t GetState          ( taf_wlan_DeviceState_t* statePtr );
            le_result_t GetIntfInfo       ( taf_wlan_APIntfInfo_t* APIntfinfoPtr,
                                            size_t* APIntfinfoSizePtr,
                                            taf_wlan_STAIntfInfo_t* STAIntfinfoPtr,
                                            size_t* STAIntfinfoSizePtr);

            // Set/Get fucntions for private variables.
            void SetSubsystemState ( telux::common::ServiceStatus status );
            void SetDeviceState    ( bool enable );
            le_event_Id_t GetStateChangeEventID();

        private:
            le_result_t FillIntfInfo(taf_wlan_APIntfInfo_t *APIntfinfoPtr,
                                     size_t *APIntfinfoSizePtr,
                                     taf_wlan_STAIntfInfo_t *STAIntfinfoPtr,
                                     size_t *STAIntfinfoSizePtr);
            // The WLAN Device Manager
            std::shared_ptr<telux::wlan::IWlanDeviceManager> wlanDevMgr = nullptr;
            // The WLAN Listener class object
            std::shared_ptr<telux::tafsvc::taf_WlanListener> wlanListener;
            // WLAN Subsystem status
            telux::common::ServiceStatus wlanSubSystemState =
                                                telux::common::ServiceStatus::SERVICE_FAILED;

            le_event_Id_t wlanDevStateChangeEvID; // The WLAN device state change event ID
            le_mem_PoolRef_t DeviceStatusPoolRef = NULL;
            le_mutex_Ref_t wlanMutexRef = NULL;
        };
    } //namespace tafsvc
} //namespace telux
