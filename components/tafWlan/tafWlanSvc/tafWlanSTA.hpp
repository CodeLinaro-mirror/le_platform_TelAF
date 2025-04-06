/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <wpa_ctrl.h>

#define WLANSTA_MAX_WPA_EVENT_LEN 64

#define WPA_STA_NET_NOT_ADDED "NOT_ADDED"

#define WPA_STA_MAX_NETID 1024

#define WPA_CTRL_RSP_BUF_LEN 2048

#define WPA_SUPPLICANT_LOCATION_PATH "/var/run/wpa_supplicant/"

    namespace tafsvc
    {
        //------------------------------------------------------------------------------------------
        /**
         * Context to maintain for each STATION
         */
        //------------------------------------------------------------------------------------------
        typedef struct
        {
            taf_wlan_STAid_t         id;
            taf_wlanSta_WlanSTARef_t staRef;    // Station reference.
            le_dls_Link_t            link;      // Link to sta context list
            le_event_Id_t            StaEvent;  // STA state event for applications.
            char IntfName[TAF_NET_INTERFACE_NAME_MAX_LEN+1];
            uint16_t numScannedAPs;             // Number of available APs
            taf_wlanSta_APInfo_t ApInfo[TAF_WLANSTA_MAX_APSCAN_RESULT_NUM];
            taf_wlanSta_APInfo_t ApInfoConnect;
        } StaCtx_t;

        //------------------------------------------------------------------------------------------
        /**
         * STA service commands
         */
        //------------------------------------------------------------------------------------------
        typedef enum
        {
            EVT_WPA_DO_SCAN,            // Perform AP scan
            EVT_WPA_DO_AP_CONNECT,      // Perform AP connect
            EVT_WPA_DO_AP_DISCONNECT    // Perform AP disconnect
        } StaCmd_e;

        //------------------------------------------------------------------------------------------
        /**
         * WPA events for use with promise/future in STA service
         */
        //------------------------------------------------------------------------------------------
        typedef enum
        {
            EVT_WPA_ERROR,
            EVT_WPA_AP_SCAN_DONE,
            EVT_WPA_AP_CONNECTING,
            EVT_WPA_AP_CONNECTED,
            EVT_WPA_AP_TEMP_DISABLED,
            EVT_WPA_AP_DISCONNECTED
        } StaWpaEvt_e;

        //------------------------------------------------------------------------------------------
        /**
         * Context to pass to supplicant monitoring thread
         */
        //------------------------------------------------------------------------------------------
        typedef struct
        {
            char IntfName[TAF_NET_INTERFACE_NAME_MAX_LEN + 1];
            std::vector<std::string> EventsToMonitor;
        } SuppThreadCtx_t;

        //------------------------------------------------------------------------------------------
        /**
         * StaCmd_t data structure
         */
        //------------------------------------------------------------------------------------------
        typedef struct
        {
            StaCtx_t  *CtxPtr;  // STA context
            StaCmd_e  cmd;      // STA command.
        } StaCmd_t;

        //------------------------------------------------------------------------------------------
        /**
         * Sta Events data structure
         */
        //------------------------------------------------------------------------------------------
        typedef struct
        {
            taf_wlanSta_WlanSTARef_t staRef; // Station reference.
            taf_wlanSta_State_t      state;  // State state to report.
        } StaEvents_t;

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
            le_result_t Start(taf_wlanSta_WlanSTARef_t staRef);
            le_result_t Stop(taf_wlanSta_WlanSTARef_t staRef);
            le_result_t Restart(taf_wlanSta_WlanSTARef_t staRef);
            le_result_t SetMode(taf_wlanSta_WlanSTARef_t staRef, taf_wlanSta_Mode_t StaMode);
            le_result_t GetMode(taf_wlanSta_WlanSTARef_t staRef, taf_wlanSta_Mode_t *StaModePtr);
            le_result_t SetStaticIPConfig(taf_wlanSta_WlanSTARef_t staRef,
                                  const taf_wlanSta_IPConfig_t *LE_NONNULL StaStaticIPConfigPtr);
            le_result_t GetIPConfig(taf_wlanSta_WlanSTARef_t staRef,
                                    taf_wlanSta_IPType_t *StaIPTypePtr,
                                    taf_wlanSta_IPConfig_t *StaStaticIPConfigPtr);
            le_result_t GetStatus(taf_wlanSta_WlanSTARef_t staRef,
                                  taf_wlanSta_State_t *StaSatePtr,
                                  char *IntfName,
                                  size_t IntfNameSize,
                                  char *IPv4Address,
                                  size_t IPv4AddressSize,
                                  char *IPv6Address,
                                  size_t IPv6AddressSize,
                                  char *MACAddress,
                                  size_t MACAddressSize);
            le_result_t APConnect(taf_wlanSta_WlanSTARef_t staRef,
                                  const taf_wlanSta_APInfo_t* LE_NONNULL ApInfo);
            le_result_t APDisconnect(taf_wlanSta_WlanSTARef_t staRef,
                                     const taf_wlanSta_APInfo_t* LE_NONNULL ApInfo);
            le_result_t DoAPScan(taf_wlanSta_WlanSTARef_t staRef);
            le_result_t GetAPScanResults(taf_wlanSta_WlanSTARef_t staRef,
                                         uint16_t *numAPPtr,
                                         taf_wlanSta_APInfo_t *ApInfoPtr,
                                         size_t *ApInfoSizePtr);
            le_result_t SetWpa2Psk(taf_wlanSta_WlanSTARef_t staRef,
                                    const taf_wlanSta_APInfo_t* LE_NONNULL ApInfo,
                                    const char* LE_NONNULL psk);
            le_result_t Connect(StaCtx_t *CtxPtr,
                                const taf_wlanSta_APInfo_t* LE_NONNULL ApInfo);
            le_result_t Disconnect(StaCtx_t *CtxPtr,
                                   const taf_wlanSta_APInfo_t* LE_NONNULL ApInfo);
            taf_wlanSta_EventHandlerRef_t AddEventHandler(taf_wlanSta_WlanSTARef_t staRef,
                                                          taf_wlanSta_HandlerFunc_t handlerPtr,
                                                          void *contextPtr);
            void RemoveEventHandler(taf_wlanSta_EventHandlerRef_t handlerRef);

        private:
            friend class taf_WlanSTAListener;
            std::string mNetID;

            // Promise/Future to synchronize WPA indications
            std::promise<StaWpaEvt_e> PromiseWPA;

            // Promise/Future to check for AP connection status
            std::promise<StaWpaEvt_e> PromiseConn;

            // Functions
            StaCtx_t *GetStaCtx(taf_wlan_STAid_t staId);
            static void StaCmdHandler(void *StaCmdPtr);
            static void *StaCmdThreadHdlr(void *context);
            static void *StaWpaSuppMonitorThreadHdlr(void *context);
            static void FirstLayerEventHandler(void *reportPtr, void *secondLayerHandlerFunc);
            void ReportStaState(StaCtx_t *StaCtxPtr, taf_wlanSta_State_t state);
            void PerformScan(StaCtx_t *CtxPtr);
            void PopulateScanResults(StaCtx_t *CtxPtr, const char *ScanResultsPtr);
            void ParseScanResultsFlags(StaCtx_t *CtxPtr, int Index, std::string FlagsStr);
            le_result_t runWPACommand(StaCtx_t *CtxPtr, const char *cmd, char *response,
                                      const size_t responseSize);
            bool CheckCommunication(StaCtx_t *CtxPtr, struct wpa_ctrl *ctrl);
            std::string CheckNetworkAdded(StaCtx_t *CtxPtr,
                                          const taf_wlanSta_APInfo_t* LE_NONNULL ApInfo);

            // The WLAN STA Manager
            std::shared_ptr<telux::wlan::IStaInterfaceManager> wlanSTAMgr;
            // The WLAN STA Listener class object
            std::shared_ptr<tafsvc::taf_WlanSTAListener> wlanSTAListener;

            // Memory pool for STA context(s)
            le_mem_PoolRef_t STACtxPoolRef = NULL;
            // List of STA context(s)
            le_dls_List_t    STACtxList  = LE_DLS_LIST_INIT;
            // Mutex for STA context list
            le_mutex_Ref_t   STACtxMutex = NULL;
            // Station Reference map
            le_ref_MapRef_t StaRefMap = NULL;

            // Memory pool ref for Sta events
            le_mem_PoolRef_t StaEventsPoolRef = NULL;
            // Mutex for STA events
            le_mutex_Ref_t STAEventsMutexRef  = NULL;

            // Internal events
            le_event_Id_t StaCommand;
            // STA internal commands thread reference
            le_thread_Ref_t  StaCmdThreadRef;
        };
    } // namespace tafsvc
