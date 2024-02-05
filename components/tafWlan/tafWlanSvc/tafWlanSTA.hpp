/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file       tafWlanSTA.hpp
 *
 * @brief      Header file for TelAF WLAN Station Management Service.
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
         * The TelAF WLAN STA listener class for TelSDK notifications.
         */
        //------------------------------------------------------------------------------------------
        class taf_WlanSTAListener : public telux::wlan::IStaListener
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
            taf_WlanSTASvcImpl(){};
            ~taf_WlanSTASvcImpl(){};

            static taf_WlanSTASvcImpl &GetInstance();

            // WLan STA Service Implementations
            taf_wlanSta_WlanSTARef_t GetWlanSTA(taf_wlan_STAid_t STAid,
                                                const char *LE_NONNULL STAIntfName);
            le_result_t Start(void);
            le_result_t Stop(void);
            le_result_t Restart(void);
            le_result_t SetMode(taf_wlanSta_Mode_t StaMode);
            le_result_t GetMode(taf_wlanSta_Mode_t *StaModePtr);
            le_result_t SetStaticIPConfig(
                const taf_wlanSta_IPConfig_t *LE_NONNULL StaStaticIPConfigPtr);
            le_result_t GetIPConfig(taf_wlanSta_IPType_t *StaIPTypePtr,
                                    taf_wlanSta_IPConfig_t *StaStaticIPConfigPtr);
            le_result_t GetStatus(taf_wlanSta_State_t *StaSatePtr,
                                  char *IntfName,
                                  size_t IntfNameSize,
                                  char *IPv4Address,
                                  size_t IPv4AddressSize,
                                  char *IPv6Address,
                                  size_t IPv6AddressSize,
                                  char *MACAddress,
                                  size_t MACAddressSize);

        private:
            // STA ID to use
            const telux::wlan::Id wlanSTAid = telux::wlan::Id::PRIMARY;
            // The WLAN STA Manager
            std::shared_ptr<telux::wlan::IStaInterfaceManager> wlanSTAMgr;
            // The WLAN STA Listener class object
            std::shared_ptr<telux::tafsvc::taf_WlanSTAListener> wlanSTAListener;
        };
    } // namespace tafsvc
} // namespace telux