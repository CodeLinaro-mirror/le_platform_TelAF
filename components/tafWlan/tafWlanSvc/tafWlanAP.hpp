/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanAP.hpp
 *
 * @brief      Header file for TelAF WLAN Access Point Management Service.
 *
 */
#pragma once
#include "tafWlan.hpp"

namespace telux
{
    namespace tafsvc
    {

        //------------------------------------------------------------------------------------------
        /**
         * The TelAF WLAN AP listener class for TelSDK notifications.
         */
        //------------------------------------------------------------------------------------------
        class taf_WlanAPListener : public telux::wlan::IApListener
        {
        public:
            // AP Config changed handler
            void onApConfigChanged(telux::wlan::Id apId) override;
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
            taf_WlanAPSvcImpl(){};
            ~taf_WlanAPSvcImpl(){};

            static taf_WlanAPSvcImpl &GetInstance();

            // WLan AP Service Implementations
            taf_wlanAp_WlanAPRef_t GetWlanAP(taf_wlan_APid_t APid,
                                             const char *LE_NONNULL APIntfName);
            le_result_t Start(void);
            le_result_t Stop(void);
            le_result_t Restart(void);
            le_result_t SetConfig(const taf_wlanAp_WlanAPConfig_t *wlanAPConfigPtr);
            le_result_t GetConfig(taf_wlanAp_WlanAPConfig_t *wlanAPConfigPtr);
            le_result_t SetSecurityConfig(const taf_wlanAp_WlanAPSecurityConfig_t *wlanAPSecCfgPtr);
            le_result_t GetSecurityConfig(taf_wlanAp_WlanAPSecurityConfig_t *wlanAPSecCfgPtr);
            le_result_t GetStatus(taf_wlanAp_WlanAPStatus_t *wlanAPStatusPtr);
            le_result_t GetConnectedDevices(uint16_t *numDevicesPtr,
                                            taf_wlanAp_WlanAPConnectedDeviceInfo_t *DevInfoPtr,
                                            size_t *DevInfoSizePtr);

        private:
            // AP ID to use
            const telux::wlan::Id wlanAPID = telux::wlan::Id::PRIMARY;
            // The WLAN AP Manager
            std::shared_ptr<telux::wlan::IApInterfaceManager> wlanAPMgr;
            // The WLAN AP Listener class object
            std::shared_ptr<telux::tafsvc::taf_WlanAPListener> wlanAPListener;
        };
    } // namespace tafsvc
} // namespace telux
