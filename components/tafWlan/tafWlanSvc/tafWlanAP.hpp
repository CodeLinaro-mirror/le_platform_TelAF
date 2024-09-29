/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
        typedef struct
        {
            le_dls_Link_t           link;
            taf_wlanAp_WlanAPRef_t  wlanAPRef;
            taf_wlan_APid_t         id;
            char                    interfaceName[TAF_NET_INTERFACE_NAME_MAX_LEN + 1];
        } taf_wlan_AP_Ctx_t;

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
            le_result_t Start(taf_wlanAp_WlanAPRef_t apRef);
            le_result_t Stop(taf_wlanAp_WlanAPRef_t apRef);
            le_result_t Restart(taf_wlanAp_WlanAPRef_t apRef);
            le_result_t SetConfig(taf_wlanAp_WlanAPRef_t apRef,
                const taf_wlanAp_WlanAPConfig_t *wlanAPConfigPtr);
            le_result_t GetConfig(taf_wlanAp_WlanAPRef_t apRef,
                taf_wlanAp_WlanAPConfig_t *wlanAPConfigPtr);
            le_result_t SetSecurityConfig(taf_wlanAp_WlanAPRef_t apRef,
                const taf_wlanAp_WlanAPSecurityConfig_t *wlanAPSecCfgPtr);
            le_result_t GetSecurityConfig(taf_wlanAp_WlanAPRef_t apRef,
                taf_wlanAp_WlanAPSecurityConfig_t *wlanAPSecCfgPtr);
            le_result_t GetStatus(taf_wlanAp_WlanAPRef_t wlanAPRef,
                taf_wlanAp_WlanAPStatus_t *wlanAPStatusPtr);
            le_result_t GetConnectedDevices(taf_wlanAp_WlanAPRef_t wlanAPRef,
                uint16_t *numDevicesPtr, taf_wlanAp_WlanAPConnectedDeviceInfo_t *DevInfoPtr,
                size_t *DevInfoSizePtr);
            taf_wlanAp_WlanAPRef_t GetWlanAPReference(taf_wlan_APid_t apID,
                const char *LE_NONNULL apIntfName);
            taf_wlan_AP_Ctx_t* GetWlanAPCtx(taf_wlan_APid_t apID);

        private:
            // The WLAN AP Manager
            std::shared_ptr<telux::wlan::IApInterfaceManager> wlanAPMgr;

            // The WLAN AP Listener class object
            std::shared_ptr<telux::tafsvc::taf_WlanAPListener> wlanAPListener;

            // AP Reference map
            le_ref_MapRef_t APRefMap = nullptr;

            // Mutex for AP context list
            le_mutex_Ref_t APCtxMutex = nullptr;

            // Memory pool for STA context(s)
            le_mem_PoolRef_t APCtxPool = nullptr;

            // List of STA context(s)
            le_dls_List_t APCtxList = LE_DLS_LIST_INIT;
        };
    } // namespace tafsvc
} // namespace telux
