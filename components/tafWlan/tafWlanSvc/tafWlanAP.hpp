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
#include <set>

#define AP_WPA_CTRL_RSP_BUF_LEN 2048

#define HOSTAPD_LOCATION_PATH "/var/run/hostapd/"

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
            le_event_Id_t DeviceConnectionEvent; // AP device connection event.
            bool            isAPWpaCtrlThreadRunning;
            le_thread_Ref_t APWpaCtrlThreadRef;  // AP internal wpa_ctrl thread reference
            // Clients that have requested device connected events
            std::set<le_msg_SessionRef_t> devCnxEvtClients;
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
         * Client connection event data structure
         */
        //------------------------------------------------------------------------------------------
        typedef struct
        {
            taf_wlanAp_WlanAPRef_t                 apRef;      // AP reference.
            taf_wlanAp_DeviceConnectionEvent_t     event;      // Event type.
            char    MACAddress[TAF_NET_MAC_ADDR_MAX_LEN];      // MAC address of the device.
        } DeviceCnxEvent_t;

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
            le_result_t GetStatus(taf_wlanAp_WlanAPRef_t apRef,
                                  taf_wlanAp_WlanAPStatus_t *wlanAPStatusPtr);
            le_result_t GetConnectedDevices(taf_wlanAp_WlanAPRef_t apRef,
                uint16_t *numDevicesPtr, taf_wlanAp_WlanAPConnectedDeviceInfo_t *DevInfoPtr,
                size_t *DevInfoSizePtr);
            taf_wlanAp_WlanAPRef_t GetWlanAPReference(taf_wlan_APid_t apID,
                const char *LE_NONNULL apIntfName);
            taf_wlan_AP_Ctx_t* GetWlanAPCtx(taf_wlan_APid_t apID);
            taf_wlanAp_DeviceConnectionEventHandlerRef_t AddDeviceConnectionEventHandler(
                taf_wlanAp_WlanAPRef_t apRef,
                taf_wlanAp_DeviceConnectionEventHandlerFunc_t handlerPtr,
                void *contextPtr);
            void RemoveDeviceConnectionEventHandler(
                taf_wlanAp_DeviceConnectionEventHandlerRef_t handlerRef);

        private:
            // The WLAN AP Manager
            std::shared_ptr<telux::wlan::IApInterfaceManager> wlanAPMgr;

            // The WLAN AP Listener class object
            std::shared_ptr<telux::tafsvc::taf_WlanAPListener> wlanAPListener;

            // AP Reference map
            le_ref_MapRef_t APRefMap = nullptr;

            // Mutex for AP context list
            le_mutex_Ref_t APCtxMutex = nullptr;

            // Memory pool for AP context(s)
            le_mem_PoolRef_t APCtxPoolRef = nullptr;

            // List of AP context(s)
            le_dls_List_t APCtxList = LE_DLS_LIST_INIT;

            // Memory pool ref for device connection events events
            le_mem_PoolRef_t DeviceCnxEventPoolRef = NULL;

            // WPA CTRL Thread handler and destructor
            static void *APWpaCtrlThreadHdlr(void *context);
            static void  APWpaCtrlThreadDestructor(void *context);

            // Device connecton events first layer handler
            static void DeviceCnxFirstLayerEventHandler(    void *reportPtr,
                                                            void *secondLayerHandlerFunc);

            // Private functions
            void ReportDevCnxEvents(taf_wlan_AP_Ctx_t *ApctxPtr,              // AP Context
                                    taf_wlanAp_DeviceConnectionEvent_t event, // Event type
                                    const char *MACAddressStr  // MAC address of the device
            );
            // Client connect/disconnect handlers.
            // TBD: Move to a common location in tafWlanSvcImpl
            static void OnWlanSvcClientConnect(le_msg_SessionRef_t sessionRef, void *context);
            static void OnWlanSvcClientDisconnect(le_msg_SessionRef_t sessionRef, void *context);
        };
    } // namespace tafsvc
} // namespace telux
