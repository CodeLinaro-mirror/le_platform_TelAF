/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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

#include <future>
#include <sstream>

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafWlanPa.hpp"
#include "tafWlanAP.hpp"
#include "tafWlanSTA.hpp"

#define TAF_WLAN_MAX_SESSION_REF 20

#define TAF_WLAN_GET_DATA_SETTINGS_TIMEOUT 60
#define TAF_WLAN_CMD_TIMEOUT 30

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

// Seconds in a day
#define SECONDS_IN_A_DAY 86400

namespace tafsvc
{
    //----------------------------------------------------------------------------------------------
    /**
    * WLAN device state change event structure.
    */
    //----------------------------------------------------------------------------------------------
    typedef struct
    {
        taf_wlan_DeviceState_t wlanDeviceState;
    }
    WlanDevStateChangeEvent_t;

    //----------------------------------------------------------------------------------------------
    /**
     * Internal commands to handle band interference config get and set
     */
    //----------------------------------------------------------------------------------------------
    typedef enum
    {
        WLAN_DSCMD_BAND_INT_CFG_GET = 1,
        WLAN_DSCMD_BAND_INT_CFG_SET = 2
    }
    WlanDataSettingsCmdType_t;

    //----------------------------------------------------------------------------------------------
    /**
     * Band interference configuration structure.
     */
    //----------------------------------------------------------------------------------------------
    typedef struct
    {
        taf_wlan_BandIntState_t    state;     // The band interference state.
        taf_wlan_BandIntPriority_t prioBand;  // The band to prioritize.
        uint32_t wlanUnavailableTime;         // WLAN 5Ghz band unavailable time in seconds.
        uint32_t n79UnavailableTime;          // 5G band N79 unavailable time in seconds.
    }
    WlanBandIntCfg_t;

    //----------------------------------------------------------------------------------------------
    /**
     * Internal band interference get command.
     */
    //----------------------------------------------------------------------------------------------
    typedef struct
    {
        void *contextPtr;        // The app provided context pointer.
    } WlanGetBandIntCmd_t;

    //----------------------------------------------------------------------------------------------
    /**
     * Internal band interference get command response.
     */
    //----------------------------------------------------------------------------------------------
    typedef struct
    {
        le_result_t       result;         // The result of the command.
        WlanBandIntCfg_t  config;         // Band interference configuration.
        void             *contextPtr;     // The app provided context pointer.
    }
    WlanGetBandIntCmdRsp_t;

    //----------------------------------------------------------------------------------------------
    /**
     * Internal band interference set command.
     */
    //----------------------------------------------------------------------------------------------
    typedef struct
    {
        WlanBandIntCfg_t  config;         // Band interference configuration.
        void             *contextPtr;     // The app provided context pointer.
    }
    WlanSetBandIntCmd_t;

    //----------------------------------------------------------------------------------------------
    /**
     * Internal data settings command structure.
     */
    //----------------------------------------------------------------------------------------------
    typedef struct
    {
        WlanDataSettingsCmdType_t cmdType;
        WlanGetBandIntCmd_t       bandIntGetCmd;
        WlanSetBandIntCmd_t       bandIntSetCmd;
    }
    WlanDataSettingsCmd_t;

    //----------------------------------------------------------------------------------------------
    /**
     * The TelAF WLAN APIs implementation class.
     */
    //----------------------------------------------------------------------------------------------
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

        /* Band interference related service APIs implementation */
        le_result_t SetBandIntWaitTime (taf_wlan_BandIntPriority_t band, uint32_t waitTime);
        le_result_t GetBandIntWaitTime (taf_wlan_BandIntPriority_t band, uint32_t *waitTimePtr);
        le_result_t SetBandIntPriority (taf_wlan_BandIntPriority_t bandPriority);
        le_result_t GetBandIntPriority (taf_wlan_BandIntPriority_t *bandPriorityPtr);
        le_result_t SetBandIntState    (taf_wlan_BandIntState_t state);
        le_result_t GetBandIntState    (taf_wlan_BandIntState_t *statePtr);

        // Set/Get functions for private variables.
        void SetSubsystemState(taf::pa::wlan::ServiceState_e serviceStatus);
        void SetDeviceState(bool enable);
        le_event_Id_t GetStateChangeEventID();

    private:
        le_result_t FillIntfInfo(taf_wlan_APIntfInfo_t *APIntfinfoPtr,
                                    size_t *APIntfinfoSizePtr,
                                    taf_wlan_STAIntfInfo_t *STAIntfinfoPtr,
                                    size_t *STAIntfinfoSizePtr);

        le_event_Id_t wlanDevStateChangeEvID; // The WLAN device state change event ID
        le_mem_PoolRef_t DeviceStatusPoolRef = NULL;
        le_mutex_Ref_t wlanMutexRef = NULL;

        /**
         * This section is for data settings commands.
         */
        // Thread to handle data settings commands
        std::promise<le_result_t> promDataSettingThreadStart;
        le_thread_Ref_t dataSettingsThreadRef = NULL;
        static void *DataSettingsThreadHdlr(void *context);

        // Variable to check if the data settings manager is ready or not.
        bool bDataSettingManagerReady = false;

        // The internal data settings command event ID and handler.
        le_event_Id_t dataSettingsCmd;
        static void DataSettingsCmdHandler(void *PayloadPtrPtr);

        // Promises to handle band interference commands.
        std::promise<le_result_t>            promSetBandIntConfig;
        std::promise<WlanGetBandIntCmdRsp_t> promGetBandIntConfig;

        // Functions to handle band interference commands.
        void HandleBandIntSet(WlanSetBandIntCmd_t bandIntSet);
        void HandleBandIntGet(WlanGetBandIntCmd_t bandIntGet);

        /**
         * This section is for band interference management.
         */
        // Variables
        WlanBandIntCfg_t bandIntCfgCurrent;     // The current band interference config details.
        WlanBandIntCfg_t bandIntCfgToSet;       // The band interference config details to set.
        // Timer to get the config after service init.
        const uint32_t GetFirstBandIntConfigInterval = 500; // 500ms
        le_timer_Ref_t  getFirstBandIntConfigTimerRef = NULL;
        static void GetFirstBandIntConfigTimerHdlr(le_timer_Ref_t timerRef);

        // Functions to handle band interference management.
        // Reset the band interference config details.
        void ResetBandIntCfg(WlanBandIntCfg_t &config);
    };
} //namespace tafsvc
