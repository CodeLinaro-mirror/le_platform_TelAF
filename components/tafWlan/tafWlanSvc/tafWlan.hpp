/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <future>
#include <telux/wlan/WlanDefines.hpp>
#include <telux/wlan/WlanFactory.hpp>
#include <telux/wlan/WlanDeviceManager.hpp>

#define TAF_WLAN_MAX_SESSION_REF 20

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
            // BandType conversion
            static taf_wlan_Band_t BandTypeToTAF (telux::wlan::BandType bandType);
            static telux::wlan::BandType BandTypeToTelux (taf_wlan_Band_t bandType);

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

            // Station Mode(Bridge/Router) conversion
            static taf_wlanSta_Mode_t StaModeToTAF(telux::wlan::StaBridgeMode Mode);
            static telux::wlan::StaBridgeMode StaModeToTelux(taf_wlanSta_Mode_t Mode);

            // Station IP Type conversion
            static taf_wlanSta_IPType_t StaIPTypeToTAF(telux::wlan::StaIpConfig IPMode);
            static telux::wlan::StaIpConfig StaIPTypeToTelux(taf_wlanSta_IPType_t IPMode);

            // Station state conversion
            static taf_wlanSta_State_t StaIntfStatusToTAF(telux::wlan::StaInterfaceStatus State);
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
            le_result_t SetON                      ( void );
            le_result_t SetOFF                     ( void );
            le_result_t SetMode                    ( taf_wlan_DeviceMode_t wlanMode );
            le_result_t GetMode                    ( taf_wlan_DeviceMode_t* wlanModePtr );
            le_result_t GetState                   ( taf_wlan_DeviceState_t* statePtr );

            // Set/Get fucntions for private variables.
            void SetSubsystemState ( telux::common::ServiceStatus status );
            void SetDeviceState    ( bool enable );
            le_event_Id_t GetStateChangeEventID();

        private:
            // The WLAN Device Manager
            std::shared_ptr<telux::wlan::IWlanDeviceManager> wlanDevMgr = nullptr;
            // The WLAN Listener class object
            std::shared_ptr<telux::tafsvc::taf_WlanListener> wlanListener;
            // WLAN Subsystem status
            telux::common::ServiceStatus wlanSubSystemState =
                                                telux::common::ServiceStatus::SERVICE_FAILED;

            le_event_Id_t wlanDevStateChangeEvID; // The WLAN device state change event ID
            le_mem_PoolRef_t DeviceStatusPool;
            le_mutex_Ref_t wlanMutexRef = NULL;
        };


        //------------------------------------------------------------------------------------------
        /**
        * The TelAF WLAN AP listener class for TelSDK notifications.
        */
        //------------------------------------------------------------------------------------------
        class taf_WlanAPListener: public telux::wlan::IApListener
        {
            public:
                // AP Config changed handler
                void onApConfigChanged (telux::wlan::Id apId) override;
                // AP Band changed handler
                void onApBandChanged(telux::wlan::BandType radio) override;

                void onApDeviceStatusChanged(
                    telux::wlan::ApDeviceConnectionEvent event,
                    std::vector<telux::wlan::DeviceIndInfo> info) override;
        };

        //------------------------------------------------------------------------------------------
        /**
        * The TelAF WLAN Access Point APIs implementation class.
        */
        //------------------------------------------------------------------------------------------
        class taf_WlanAPSvcImpl : public ITafSvc
        {
        public:
            // Inherited functions
            void Init(void);
            taf_WlanAPSvcImpl() {};
            ~taf_WlanAPSvcImpl() {};

            static taf_WlanAPSvcImpl &GetInstance();

            // WLan AP Service Implementations
            le_result_t Start              ( void );
            le_result_t Stop               ( void );
            le_result_t Restart            ( void );
            le_result_t SetConfig          (const taf_wlanAp_WlanAPConfig_t *wlanAPConfigPtr);
            le_result_t GetConfig          (taf_wlanAp_WlanAPConfig_t *wlanAPConfigPtr);
            le_result_t SetSecurityConfig
                                     ( const taf_wlanAp_WlanAPSecurityConfig_t* wlanAPSecCfgPtr );
            le_result_t GetSecurityConfig  ( taf_wlanAp_WlanAPSecurityConfig_t* wlanAPSecCfgPtr );
            le_result_t GetStatus          ( taf_wlanAp_WlanAPStatus_t* wlanAPStatusPtr );
            le_result_t GetConnectedDevices( uint16_t *numDevicesPtr,
                                             taf_wlanAp_WlanAPConnectedDeviceInfo_t *DevInfoPtr,
                                             size_t *DevInfoSizePtr
                                           );

        private:
            // AP ID to use
            const telux::wlan::Id wlanAPID = telux::wlan::Id::PRIMARY;
            // The WLAN AP Manager
            std::shared_ptr<telux::wlan::IApInterfaceManager> wlanAPMgr;
            // The WLAN AP Listener class object
            std::shared_ptr<telux::tafsvc::taf_WlanAPListener> wlanAPListener;
        };

        //------------------------------------------------------------------------------------------
        /**
        * The TelAF WLAN STA listener class for TelSDK notifications.
        */
        //------------------------------------------------------------------------------------------
        class taf_WlanSTAListener: public telux::wlan::IStaListener
        {
            public:
                // STA Band changed handler
                void onStationBandChanged(telux::wlan::BandType radio) override;
                // STA status changed handler
                void onStationStatusChanged(std::vector<telux::wlan::StaStatus> staStatus) override;
        };

        //------------------------------------------------------------------------------------------
        /**
        * The TelAF WLAN Station APIs implementation class.
        */
        //------------------------------------------------------------------------------------------
        class taf_WlanSTASvcImpl : public ITafSvc
        {
        public:
            // Inherited functions
            void Init(void);
            taf_WlanSTASvcImpl() {};
            ~taf_WlanSTASvcImpl() {};

            static taf_WlanSTASvcImpl &GetInstance();

            // WLan STA Service Implementations
            le_result_t Start        ( void );
            le_result_t Stop         ( void );
            le_result_t Restart      ( void );
            le_result_t SetMode      (taf_wlanSta_Mode_t StaMode);
            le_result_t GetMode      (taf_wlanSta_Mode_t* StaModePtr);
            le_result_t SetStaticIPConfig  (
                             const taf_wlanSta_IPConfig_t * LE_NONNULL StaStaticIPConfigPtr );
            le_result_t GetIPConfig  ( taf_wlanSta_IPType_t* StaIPTypePtr,
                                       taf_wlanSta_IPConfig_t * StaStaticIPConfigPtr);
            le_result_t GetStatus          ( taf_wlanSta_State_t* StaSatePtr,
                                            char* IntfName,
                                            size_t IntfNameSize,
                                            char* IPv4Address,
                                            size_t IPv4AddressSize,
                                            char* IPv6Address,
                                            size_t IPv6AddressSize,
                                            char* MACAddress,
                                            size_t MACAddressSize
                                           );

        private:
            // STA ID to use
            const telux::wlan::Id wlanSTAid = telux::wlan::Id::PRIMARY;
            // The WLAN STA Manager
            std::shared_ptr<telux::wlan::IStaInterfaceManager> wlanSTAMgr;
            // The WLAN STA Listener class object
            std::shared_ptr<telux::tafsvc::taf_WlanSTAListener> wlanSTAListener;
        };
    } //namespace tafsvc
} //namespace telux