/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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
#include <map>
#include <unordered_map>
#include <vector>
#include <shared_mutex>
#include <deque>

#define WLANSTA_MAX_WPA_EVENT_LEN 64

#define WPA_STA_NET_NOT_ADDED "NOT_ADDED"

#define WPA_STA_MAX_NETID 1024

#define WPA_CTRL_RSP_BUF_LEN 2048

#define WPA_SUPPLICANT_LOCATION_PATH "/var/run/wpa_supplicant/"

namespace tafsvc
{
    //----------------------------------------------------------------------------------------------
    /**
     * Context to maintain for each STATION
     */
    //----------------------------------------------------------------------------------------------
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

    //----------------------------------------------------------------------------------------------
    /**
     * STA service commands
     */
    //----------------------------------------------------------------------------------------------
    typedef enum
    {
        CMD_WPA_DO_SCAN,                // Perform AP scan.
        CMD_WPA_DO_AP_CONNECT,          // Perform AP connect.
        CMD_WPA_DO_AP_DISCONNECT,       // Perform AP disconnect.
        CMD_WPA_START_LINK_MONITORING,  // Start link monitoring after connecting to an AP.
        CMD_WPA_STOP_LINK_MONITORING    // Stop link monitoring after disconnecting from an AP.
    } StaCmd_e;

    //----------------------------------------------------------------------------------------------
    /**
     * WPA events for use with promise/future in STA service
     */
    //----------------------------------------------------------------------------------------------
    typedef enum
    {
        EVT_WPA_ERROR,
        EVT_WPA_AP_SCAN_DONE,
        EVT_WPA_AP_CONNECTING,
        EVT_WPA_AP_CONNECTED,
        EVT_WPA_AP_TEMP_DISABLED,
        EVT_WPA_AP_DISCONNECTED
    } StaWpaEvt_e;

    //----------------------------------------------------------------------------------------------
    /**
     * Context to pass to supplicant monitoring thread
     */
    //----------------------------------------------------------------------------------------------
    typedef struct
    {
        struct wpa_ctrl *ctrl;                      // WPA control handle
        int fd;                                     // Control socket fd
        le_fdMonitor_Ref_t fdMonitorRef;            // FD monitor reference
        std::vector<std::string> EventsToMonitor;   // Events to monitor
        StaWpaEvt_e resultEvent;                    // Result of the event monitoring
        bool eventCompleted;                        // Flag for event processing completion
        std::mutex mutex;                           // Mutex for thread safety
        std::condition_variable eventCv;            // To notify waiter without polling
    } SuppFdCtx_t;

    //----------------------------------------------------------------------------------------------
    /**
     * StaCmd_t data structure
     */
    //----------------------------------------------------------------------------------------------
    typedef struct
    {
        StaCtx_t           *CtxPtr;    // STA context
        StaCmd_e            cmd;       // STA command.
        le_msg_SessionRef_t clientRef; // STA client reference.
    } StaCmd_t;

    //----------------------------------------------------------------------------------------------
    /**
     * Sta Events data structure
     */
    //----------------------------------------------------------------------------------------------
    typedef struct
    {
        taf_wlanSta_WlanSTARef_t staRef; // Station reference.
        taf_wlanSta_State_t      state;  // State state to report.
    } StaEvents_t;

    //----------------------------------------------------------------------------------------------
    /**
     * Sta connected AP signal strength event structure
     */
    //----------------------------------------------------------------------------------------------
    struct StaConnectedApSignalStrengthEvt_t
    {
        taf_wlanSta_WlanSTARef_t staRef;         // Station reference.
        int16_t                  signalStrength; // Signal strength in dBm
    };

    //----------------------------------------------------------------------------------------------
    /**
     * Structure to define the key for uniqueness of event IDs that are maintained.
     * Uniqueness is calculated based on the STA, threshold, frequency and bAverage values.
     */
    //----------------------------------------------------------------------------------------------
    struct ConnectedApSigStrengthEventIdKey_t
    {
        int16_t  threshold;
        uint16_t frequency;
        bool     bAverage;

        bool operator==(const ConnectedApSigStrengthEventIdKey_t &other) const
        {
            return threshold == other.threshold &&
                   frequency == other.frequency &&
                   bAverage  == other.bAverage;
        }
    };

    //----------------------------------------------------------------------------------------------
    /**
     * Custom hash for ConnectedApSigStrengthEventIdKey_t.
     */
    //----------------------------------------------------------------------------------------------
    struct ConnectedApSigStrengthEventIdKeyHash_t
    {
        size_t operator()(const ConnectedApSigStrengthEventIdKey_t &k) const
        {
            // Use std::hash for void* for ref types
            size_t h1 = std::hash<int16_t>{}(k.threshold);
            size_t h2 = std::hash<uint16_t>{}(k.frequency);
            size_t h3 = std::hash<bool>{}(k.bAverage);
            // Combine hashes
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    //----------------------------------------------------------------------------------------------
    /**
     * Context for clients that have registered for STA connected AP signal strength events.
     */
    //----------------------------------------------------------------------------------------------
    struct ConnectedApSigStrengthClientCtx_t
    {
        taf_wlan_STAid_t                                    staId;           // IN
        ConnectedApSigStrengthEventIdKey_t                  key;             // IN
        taf_wlanSta_WlanSTARef_t                            wlanSTARef;      // IN
        le_msg_SessionRef_t                                 clientRef;       // IN
        le_timer_Ref_t                                      timerRef;        // OUT
        le_event_Id_t                                       eventId; // The signal strength event Id
    };

    //----------------------------------------------------------------------------------------------
    /**
     * The TelAF WLAN STA listener class for TelSDK notifications.
     */
    //----------------------------------------------------------------------------------------------
    class taf_WlanSTAListener : public telux::wlan::IStaListener
    {
    public:
        // STA Band changed handler
        void onStationBandChanged(telux::wlan::BandType radio) override;
        // STA status changed handler
        void onStationStatusChanged(std::vector<telux::wlan::StaStatus> staStatus) override;
    };

    //----------------------------------------------------------------------------------------------
    /**
     * The TelAF WLAN Station WPA supplicant interfaces.
     */
    //----------------------------------------------------------------------------------------------
    class taf_WlanStaWpaSupplicantIntf
    {
        // The maximum number of WAP control communication failure count
        constexpr static int MAX_CONTROL_SOCKET_FAILURE_COUNT = 3; // 3 times.

    public:
        // Constructor
        taf_WlanStaWpaSupplicantIntf(const taf_wlan_STAid_t);
        // Destructor for RAII cleanup
        ~taf_WlanStaWpaSupplicantIntf();

        // Get the WPA supplicant path.
        const std::string &GetWpaSupplicantPath() const;
        // Set the WPA supplicant path.
        bool SetWpaSupplicantPath(const std::string &supplicantPath);

        // Open the WPA supplicant control socket.
        bool OpenControlSocket(void);
        // Close the WPA supplicant control socket.
        void CloseControlSocket(void);
        // Check control socket status
        bool IsControlSocketActive(void);
        // Get the current connected AP's signal strength
        le_result_t GetConnectedApRSSI(int16_t &signalStrength);

    private:
        // Station Id
        const taf_wlan_STAid_t staId_;
        // The WPA control socket.
        struct wpa_ctrl *wpaCtrlPtr_;
        // The WPA supplicant path
        std::string wpaSupplicantPathStr_;
        // The control socket communication failure count
        int16_t controlSocketFailureCount_;
    };

    //----------------------------------------------------------------------------------------------
    /**
     * The connected AP signal strength monitor class.
     */
    //----------------------------------------------------------------------------------------------
    class taf_WlanStaConnectedApSignalStrengthMonitor
    {

        // Maximum number of signal strength samples
        constexpr static uint16_t MAX_SIGNAL_STRENGTH_SAMPLES = 300;
        // Passive(client have not registered for sig strength monitoring) interval in milliseconds.
        constexpr static uint32_t SIGNAL_STRENGTH_MONITOR_PASSIVE_INTERVAL = 10000; // 10s.
        // Active(client have registered for signal strength monitoring) interval in milliseconds.
        constexpr static uint32_t SIGNAL_STRENGTH_MONITOR_ACTIVE_INTERVAL = 1000; // 1s.
        // Timeout in seconds for monitoring thread start.
        constexpr static int START_TIMEOUT = 2;  // 2 seconds
        // Timeout in seconds for monitoring thread stop.
        constexpr static int STOP_TIMEOUT  = 2;  // 2 seconds

        enum State_e
        {
            STOPPED,
            RUNNING
        };

        enum Mode_e
        {
            ACTIVE,
            PASSIVE
        };

        // The parameters that are monitoring thread specific and have thread lifetimes.
        struct MonitorThreadParams_t
        {
            std::promise<bool> *startPromisePtr;
            std::promise<bool> *stopPromisePtr;
        };

    public:

        // Constructor
        taf_WlanStaConnectedApSignalStrengthMonitor(const taf_wlan_STAid_t);
        // Destructor for RAII cleanup
        ~taf_WlanStaConnectedApSignalStrengthMonitor();

        // Delete copy constructor and copy assignment
        taf_WlanStaConnectedApSignalStrengthMonitor
                                    (const taf_WlanStaConnectedApSignalStrengthMonitor &) = delete;
        taf_WlanStaConnectedApSignalStrengthMonitor &operator=
                                    (const taf_WlanStaConnectedApSignalStrengthMonitor &) = delete;
        // Delete move constructor and move assignment
        taf_WlanStaConnectedApSignalStrengthMonitor
                                    (taf_WlanStaConnectedApSignalStrengthMonitor &&) = delete;
        taf_WlanStaConnectedApSignalStrengthMonitor &operator=
                                    (taf_WlanStaConnectedApSignalStrengthMonitor &&) = delete;

        // Start signal strength measurements with provided STA interface.
        bool Start();
        // Stop signal strength measurements.
        bool Stop();
        void SetInterfaceName(const char *intfNameStr);
        // Get interface name for which signal monitoring is done.
        void GetInterfaceName(std::string &intfNameStr) const;
        // Get status.
        bool IsRunning() const;
        // Get active mode.
        bool IsActiveMode() const;
        // Set mode
        le_result_t SetActiveMode();
        le_result_t SetPassiveMode();
        le_result_t GetLastSignalStrength(int16_t &signalStrength) const;
        le_result_t GetAverageSignalStrength(const uint16_t windowSize,
                                                                    int16_t &avgSigStrength) const;

        le_event_Id_t GetSignalStrengthEventId(const ConnectedApSigStrengthEventIdKey_t &key);

    private:
        // Station Id
        const taf_wlan_STAid_t staId_;

        // The STA interface name for which signal strength measurements are being taken.
        std::string intfNameStr_;

        // Signal strength monitoring state
        State_e state_;
        // Signal strength monitoring mode
        Mode_e mode_;

        // FIFO queue to hold max 300 connected AP signal strength measurements.
        std::deque<int16_t> connectedApSignalStrengths_;
        // Mutex for thread safety of connectedApSignalStrengths_
        // mutable as shared_lock(const method) modifies it
        mutable std::shared_mutex connectedApSignalStrengthsMutex_;

        // Map to keep track of events and the respective keys.
        std::unordered_map<ConnectedApSigStrengthEventIdKey_t, le_event_Id_t,
                                    ConnectedApSigStrengthEventIdKeyHash_t> sigStrengthEventsMap_;
        // Mutex for threadsafe access.
        mutable std::shared_mutex sigStrengthEventsMapMutex_;

        /**
         * Shared pointer to taf_WlanStaWpaSupplicantIntf object reference.
         * No need for a mutex now as only one STA is supported and there is only one
         * taf_WlanStaWpaSupplicantIntf per STA.
         */
        std::unordered_map<taf_wlan_STAid_t, std::shared_ptr<taf_WlanStaWpaSupplicantIntf>>
                                                                                wpaSuppIntfRefMap_;

        // Signal monitor thread
        le_thread_Ref_t sigStrengthThreadRef_;
        static void *apSigStrengthThreadHandler(void *context);
        static void apSigStrengthThreadDestructor(void *context);

        // Parameters used with signal monitoring
        MonitorThreadParams_t monitorThreadParams_;

        // Signal monitor timer reference.
        le_timer_Ref_t sigStrengthTimerRef_;
        static void apSigStrengthTimerHandler(le_timer_Ref_t);

        // Functions
        le_event_Id_t createSignalStrengthEventId(const ConnectedApSigStrengthEventIdKey_t &key);

        // Insert signal strength into connectedApSignalStrengths_ deque.
        void insertSignalStrength(const int16_t sigStrength);

        // Queue the API to set the signal strength timer interval to sigStrengthThreadRef_
        static void queueSetSignalStrengthTimerInterval(void *param1Ptr, void *param2Ptr);
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
        taf_WlanSTASvcImpl(const taf_WlanSTASvcImpl &) = delete;
        taf_WlanSTASvcImpl &operator=(const taf_WlanSTASvcImpl &) = delete;

        static taf_WlanSTASvcImpl &GetInstance();
        le_event_Id_t GetWlanStaInternalCmdEventId() const;

        // WLan STA Service Implementations
        taf_wlanSta_WlanSTARef_t GetWlanSTA(taf_wlan_STAid_t STAid,
                                            const char *LE_NONNULL STAIntfName);
        le_result_t Start(taf_wlanSta_WlanSTARef_t staRef);
        le_result_t Stop(taf_wlanSta_WlanSTARef_t staRef);
        le_result_t Restart(taf_wlanSta_WlanSTARef_t staRef);
        le_result_t SetMode(taf_wlanSta_WlanSTARef_t staRef, taf_wlanSta_Mode_t StaMode);
        le_result_t GetMode(taf_wlanSta_WlanSTARef_t staRef, taf_wlanSta_Mode_t *StaModePtr);
        le_result_t SetIPConfig(taf_wlanSta_WlanSTARef_t staRef, taf_wlanSta_IPType_t StaIPType,
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
        le_result_t GetAPEstimatedThroughput(taf_wlanSta_WlanSTARef_t staRef,
                        const char* BSSID, uint32_t* estimatedThroughputPtr, int32_t* agePtr);
        taf_wlanSta_EventHandlerRef_t AddEventHandler(taf_wlanSta_WlanSTARef_t staRef,
                                                        taf_wlanSta_HandlerFunc_t handlerPtr,
                                                        void *contextPtr);
        void RemoveEventHandler(taf_wlanSta_EventHandlerRef_t handlerRef);
        le_result_t SvcGetConnectedApSignalStrength(taf_wlanSta_WlanSTARef_t wlanSTARef,
                                                                                    int16_t *ssPtr);
        taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t SvcAddConnectedApSignalStrengthHandler
        (
            taf_wlanSta_WlanSTARef_t wlanSTARef,
            int16_t signalThreshold,
            uint16_t frequency,
            bool bAverage,
            taf_wlanSta_ConnectedApSignalStrengthHandlerFunc_t handlerPtr,
            void *contextPtr,
            le_msg_SessionRef_t clientRef
        );
        void SvcRemoveConnectedApSignalStrengthHandler
        (
            taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef,
            le_msg_SessionRef_t clientRef
        );

        // These functions are public for use from signal strength monitor object.

        // Get the context
        StaCtx_t *GetStaCtx(taf_wlan_STAid_t staId);
        // Gets the number of handler references for the specified STA ID.
        int GetNumSignalStrengthHandlersRefForSta(const taf_wlan_STAid_t staId) const;

        // WPA ctrl FD monitor handler
        static void WpaSupplicantFdHandler(int fd, short events);

        bool WaitForSupplicantEvent(SuppFdCtx_t *ctx, int timeoutMs, StaWpaEvt_e &outEvt);
    private:
        /**
        * BSS output parser helper class
        */
        class BSSParser
        {
        public:
            /**
            * Extract estimated throughput value from BSS output string
            *
            * @return
            * - LE_OK if throughput extracted successfully
            * - LE_FAULT if key not found
            * - LE_BAD_PARAMETER if value is empty or invalid
            */
            inline static le_result_t extractEstThroughput(const std::string& bssOutput,
                int& estTput);

            /**
            * Extract age of measurement from BSS output string
            *
            * @return
            * - LE_OK if age extracted successfully
            * - LE_FAULT if key not found
            * - LE_BAD_PARAMETER if value is empty or invalid
            */
            inline static le_result_t extractAge(const std::string& bssOutput, int& age);
        };

        friend class taf_WlanSTAListener;
        std::string mNetID;

        std::unordered_map<struct wpa_ctrl*, std::unique_ptr<SuppFdCtx_t>> suppFdContextMap;

        // Functions

        // Create fd monitor for WPA ctrl in the client-events thread context
        le_result_t SetupWpaSupplicantMonitoring(StaCtx_t *CtxPtr,
                            const std::vector<std::string> &eventsToMonitor,
                            SuppFdCtx_t **fdCtxPtrPtr);

        // Cleanup fd monitor (queued to client-events thread)
        void CleanupWpaSupplicantMonitoring(SuppFdCtx_t *fdCtxPtr);

        // {{ add: make queue helpers class statics so they can access private members }}
        static void QueueCreateWpaSupplicantMonitor(void *param1Ptr, void *param2Ptr);
        static void QueueDeleteWpaSupplicantMonitor(void *param1Ptr, void *param2Ptr);

        static void StaCmdHandler(void *StaCmdPtr);
        static void *StaCmdThreadHdlr(void *context);
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
        void registerClientsConnectDisconnectHandlers();
        void unregisterClientsConnectDisconnectHandlers();
        bool IsConnectedToSSID(StaCtx_t *CtxPtr, const char *targetSsid);

        // The handler that is called when a client connects to WLAN STA service.
        le_msg_SessionEventHandlerRef_t wlanStaClientConnectHandlerRef_;
        static void onWlanStaClientConnect(le_msg_SessionRef_t sessionRef, void *ctxPtr);
        // The handler that is called when a client disconnects from WLAN STA service.
        le_msg_SessionEventHandlerRef_t wlanStaClientDisconnectHandlerRef_;
        static void onWlanStaClientDisconnect(le_msg_SessionRef_t sessionRef, void *ctxPtr);

        // The first level handler for connected signal strength events.
        static void connectedApSignalStrengthEventFirstLayerHandler(void *reportPtr,
                                                                    void *secondLayerHandlerFunc);

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
        le_event_Id_t staCommand_;
        // STA internal commands thread reference
        le_thread_Ref_t  staCmdThreadRef_;

        /**
         * STA client events management.
         */
        // STA client events thread reference. Events to clients will be triggered from this thread.
        le_thread_Ref_t staClientEventsThreadRef_;
        static void *staClientEventsThreadHandler(void *context);

        /**
         * Shared pointer to signal strength monitor object reference.
         * No need for a mutex now as only one STA is supported and there is only one
         * signal strength monitor per STA.
         */
        std::unordered_map <taf_wlan_STAid_t,
            std::shared_ptr<taf_WlanStaConnectedApSignalStrengthMonitor>> sigStrengthMonitorRefMap_;

        /**
         * Functions and variables to manage client that are registered for signal strength
         * events
         */
        // Map of clients and their registered signal strength handler references. One client can
        // register for multiple handlers, so multimap is used.
        std::unordered_multimap<le_msg_SessionRef_t,
            taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t> sigStrengthClientsAndHandlersRefMap_;
        // Mutex for threadsafe access.
        std::shared_mutex sigStrengthClientsAndHandlersRefMapMutex_;

        // Add an entry
        void addSigStrengthClientsAndHandlersRefMapEntry
        (
            const le_msg_SessionRef_t sessionRef,
            const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef
        );
        // Remove an entry
        void removeSigStrengthClientsAndHandlersRefMapEntry
        (
            const le_msg_SessionRef_t sessionRef,
            const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef
        );
        // Clear all entries for a sessionRef
        std::vector<taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t>
            getAllSigStrengthClientsAndHandlersRefMapEntries(const le_msg_SessionRef_t sessionRef);

        /**
         * Functions and variables to track the STA ID with which a signal strength handler is
         * associated with.
         */
        std::map<taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t,
                                     taf_wlan_STAid_t> sigStrengthHandlersRefAndStaIDMap_;
        // Mutex for threadsafe access.
        mutable std::shared_mutex sigStrengthHandlersRefAndStaIDMapMutex_;
        // Add an entry
        void addSigStrengthHandlersRefAndStaIDMapEntry
        (
            const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef,
            const taf_wlan_STAid_t& staId
        );
        // Get the STA ID
        bool getSigStrengthHandlersRefAndStaIDMapEntry
        (
            const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef,
            taf_wlan_STAid_t& staId
        ) const;
        // GetNumSignalStrengthHandlersRefForSta() is public

        // Remove an entry
        void removeSigStrengthHandlersRefAndStaIDMapEntry
        (
            const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef
        );

        /**
         * Functions and variables to track a signal strength handler and its timer handler.
         * This is used to manage the timers that are started for signal strength handler events for
         * clients.
         */
        std::map<taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t, le_timer_Ref_t>
                                                            sigStrengthHandlerRefAndTimerRefMap_;
        // Mutex for threadsafe access.
        std::shared_mutex sigStrengthHandlerRefAndTimerRefMapMutex_;
        // Add an entry
        void addSigStrengthHandlerRefAndTimerRefMapEntry
        (
            const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef,
            const le_timer_Ref_t timerRef
        );
        // Get the timerRef
        le_timer_Ref_t getTimerRefFromSigStrengthHandlerRefMap
        (
            const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef
        );
        // Remove an entry
        void removeSigStrengthHandlerRefAndTimerRefMapEntry
        (
            const taf_wlanSta_ConnectedApSignalStrengthHandlerRef_t handlerRef
        );

        // Promise to sync add signal strength event handler
        std::promise<void> sigStrengthAddHandlerPromise_;
        // Promise to sync remove signal strength event handler
        std::promise<void> sigStrengthRemoveHandlerPromise_;

        // The signal strength client event handler that is called in the context of
        // staClientEventsThreadRef_
        static void apSigStrengthClientEventsTimerHandler(le_timer_Ref_t);
        // Function to add signal thread handler in the context of staClientEventsThreadRef_
        static void queueAddApSigStrengthClientEventTimer(void *param1Ptr, void *param2Ptr);
        // Function to add signal thread handler in the context of staClientEventsThreadRef_
        static void queueRemoveApSigStrengthClientEventTimer(void *param1Ptr, void *param2Ptr);
        // Function to stop and delete the client signal strength timer from the context of
        // staClientEventsThreadRef_
        static void queueCleanupApSigStrengthClientEventTimers(void *param1Ptr, void *param2Ptr);
        // Function to start client signal strength timers from the context of
        // staClientEventsThreadRef_
        static void queueStartApSigStrengthClientEventTimers(void *param1Ptr, void *param2Ptr);
        // Function to stop client signal strength timers from the context of
        // staClientEventsThreadRef_
        static void queueStopApSigStrengthClientEventTimers(void *param1Ptr, void *param2Ptr);
    };
} // namespace tafsvc
