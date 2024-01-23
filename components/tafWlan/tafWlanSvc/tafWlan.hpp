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
        * The TelAF WLAN listener class for TelSDK notifications.
        */
        //------------------------------------------------------------------------------------------
        class taf_WlanListener: public telux::wlan::IWlanListener {
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
    }
}