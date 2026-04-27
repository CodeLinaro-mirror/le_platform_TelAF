/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/*
 * @file       tafUpdate.hpp
 * @brief      Internal interface for Update Service object. The functions
 *             in this file are impletmented internally.
 */

#ifndef TAFUPDATE_HPP
#define TAFUPDATE_HPP

#include <string>

#include "legato.h"
#include "interfaces.h"

#include "tafSvcIF.hpp"
#include "tafHalLib.hpp"
#include "tafPiDA.h"
#include "tafPiUA.h"

#define TAF_UPDATE_SOTA_PAKCAGE_FILE_PATH "/data/images/app_%s"

#define TAF_UPDATE_THREAD_STACK_SIZE 0x20000

#define TAF_UPDATE_SESSION_NUM 5

#define TAF_UPDATE_QOTA_HEADER_SIZE 48
#define TAF_UPDATE_QOTA_MAGIC_SIZE 4

#define TAF_UPDATE_RW_BUFFER_SIZE 4096

#define TAF_UPDATE_TIMER_INTERVAL 1000

//--------------------------------------------------------------------------------------------------
/**
 * Download event enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum {
    TAF_UPDATE_DL_START,
    TAF_UPDATE_DL_PAUSE,
    TAF_UPDATE_DL_RESUME,
    TAF_UPDATE_DL_CANCEL
} taf_UpdateDlEvent_t;

//--------------------------------------------------------------------------------------------------
/**
 * Update event enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_UPDATE_INST_START,
    TAF_UPDATE_INST_PAUSE,
    TAF_UPDATE_INST_RESUME,
    TAF_UPDATE_INST_CANCEL
} taf_UpdateEvent_t;

//--------------------------------------------------------------------------------------------------
/**
 * Update session type enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_UPDATE_SESSION_TYPE_PLUGIN_DOWNLOAD,
    TAF_UPDATE_SESSION_TYPE_PLUGIN_UPDATE,
    TAF_UPDATE_SESSION_TYPE_QOTA_PARSE,
    TAF_UPDATE_SESSION_TYPE_FW_UPDATE,
    TAF_UPDATE_SESSION_TYPE_APP_UPDATE
} taf_UpdateSessionType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Download session structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_timer_Ref_t timerRef;
    taf_pi_da_SessionRef_t sessRef;
    taf_update_State_t state;
    int percent;
    int error;
} taf_UpdateDownloadSession_t;

//--------------------------------------------------------------------------------------------------
/**
 * Update Plug-In session structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_timer_Ref_t timerRef;
    taf_pi_ua_SessionRef_t sessRef;
    taf_update_State_t state;
    int percent;
    int error;
} taf_UpdatePlugInSession_t;

//--------------------------------------------------------------------------------------------------
/**
 * Firmware install session structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char filePath[TAF_UPDATE_FILE_PATH_LEN];
} taf_UpdateFwInstallSession_t;

//--------------------------------------------------------------------------------------------------
/**
 * Download request structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_UpdateDlEvent_t event;
    taf_UpdateDownloadSession_t* sessPtr;
    char filePath[TAF_UPDATE_FILE_PATH_LEN];
} taf_UpdateDlReq_t;

//--------------------------------------------------------------------------------------------------
/**
 * Update request structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_UpdateEvent_t event;
    taf_UpdatePlugInSession_t* sessPtr;
    char filePath[TAF_UPDATE_FILE_PATH_LEN];
} taf_UpdateReq_t;

//--------------------------------------------------------------------------------------------------
/**
 * Update session structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_UpdateSessionType_t sessType;
    union
    {
        taf_UpdateDownloadSession_t dlSess;
        taf_UpdatePlugInSession_t upiSess;
        taf_UpdateFwInstallSession_t fwSess;
    };
} taf_UpdateSession_t;

namespace tafsvc {
    class taf_Update : public ITafSvc {
    public:
        taf_Update() {};
        ~taf_Update() {};

        /**
         * Returns the singleton instance of taf_Update.
         *
         * @return
         *  - Reference to the singleton taf_Update instance.
         */
        static taf_Update &GetInstance
        (
            void
        );

        /**
         * Checks whether a package starts with a valid QOTA header.
         *
         * @return
         *  - LE_OK    -- The file exists, is large enough, and starts with the expected QOTA magic.
         *  - LE_FAULT -- The file could not be opened, is smaller than the QOTA header size, or its
         *                leading bytes do not match the expected QOTA magic.
         */
        le_result_t CheckQotaHeader
        (
            const char* file ///< [IN] Path to the package file.
        );

        /**
         * Removes the QOTA header from a package file in place.
         *
         * @return
         *  - LE_OK    -- The QOTA header was removed successfully.
         *  - LE_FAULT -- The file could not be opened, the temporary buffer could not be allocated, or a
         *                read error occurred while shifting the payload to overwrite the header.
         */
        le_result_t RemoveQotaHeader
        (
            const char* file ///< [IN] Path to the package file.
        );

        /**
         * Reports download progress and emits a state indication.
         *
         * @note This helper has no return value.
         */
        void ReportDownloadStatus
        (
            taf_UpdateDownloadSession_t* sessPtr, ///< [IN] Download session pointer.
            taf_update_State_t           state    ///< [IN] State to report.
        );

        /**
         * Reports update progress and emits a state indication.
         *
         * @note This helper has no return value.
         */
        void ReportUpdateStatus
        (
            taf_UpdatePlugInSession_t* sessPtr, ///< [IN] Update session pointer.
            taf_update_State_t         state    ///< [IN] State to report.
        );

        /**
         * Handles download timer expiry events.
         *
         * @note This helper has no return value.
         */
        static void DownloadTimerHandler
        (
            le_timer_Ref_t timerRef ///< [IN] Download timer reference.
        );

        /**
         * Handles update timer expiry events.
         *
         * @note This helper has no return value.
         */
        static void UpdateTimerHandler
        (
            le_timer_Ref_t timerRef ///< [IN] Update timer reference.
        );

        /**
         * Dispatches state reports to layered handlers.
         *
         * @note This helper has no return value.
         */
        static void StateLayeredHandler
        (
            void* reportPtr,       ///< [IN] State report pointer.
            void* layerHandlerFunc ///< [IN] Layer handler function pointer.
        );

        /**
         * Handles download requests in the event loop.
         *
         * @note This helper has no return value.
         */
        static void DownloadHandler
        (
            void* reqPtr ///< [IN] Download request pointer.
        );

        /**
         * Handles update requests in the event loop.
         *
         * @note This helper has no return value.
         */
        static void UpdateHandler
        (
            void* reqPtr ///< [IN] Update request pointer.
        );

        /**
         * Initializes the update service.
         *
         * @note This helper has no return value.
         */
        void Init
        (
            void
        );

        da_Inf_t* daInfPtr;
        ua_Inf_t* uaInfPtr;

        le_mem_PoolRef_t sessionPool;
        le_ref_MapRef_t sessionMap;

        taf_update_SessionRef_t dlSessRef;
        taf_update_SessionRef_t upiSessRef;
        taf_update_SessionRef_t qotaSessRef;
        taf_update_SessionRef_t fwSessRef;
        taf_update_SessionRef_t appSessRef;

        le_event_Id_t stateEvId;
        le_event_Id_t downloadEvId;
        le_event_Id_t updatePiEvId;
    };
}

#endif