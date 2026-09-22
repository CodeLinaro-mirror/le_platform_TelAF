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

#include <mutex>
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

//--------------------------------------------------------------------------------------------------
/**
 * Checkpoint result returned to a worker by the operation controller.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_UPDATE_CP_CONTINUE,   ///< No pending request, keep running.
    TAF_UPDATE_CP_PAUSE,      ///< A pause request is pending.
    TAF_UPDATE_CP_CANCEL      ///< A cancel request is pending.
} taf_UpdateCheckpoint_t;

//--------------------------------------------------------------------------------------------------
/**
 * Description of an interruptible operation phase.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_update_State_t running;   ///< State reported while the phase is running.
    taf_update_State_t paused;    ///< State reported while the phase is paused.
    bool canPause;                ///< Pause is supported in this phase.
    bool canCancel;               ///< Cancel is supported in this phase.
    bool workerPolls;             ///< True if the worker polls pause/cancel flags at checkpoints,
                                  ///< false if every request must be posted as an event.
} taf_UpdatePhase_t;

namespace tafsvc {
    //----------------------------------------------------------------------------------------------
    /**
     * Start/Pause/Resume/Cancel controller of one session.
     *
     * Request*() are called from the IPC thread. They only validate against the in-memory state
     * and record the request, the worker owns every state transition and every report.
     *
     * Return code contract of Request*():
     *  - LE_OK            Accepted, or the same request is already pending/effective.
     *  - LE_BUSY          Start while another operation is running.
     *  - LE_UNSUPPORTED   The current phase does not support the request.
     *  - LE_NOT_PERMITTED The request is not allowed in the current state.
     */
    //----------------------------------------------------------------------------------------------
    class taf_UpdateOpCtrl {
    public:
        taf_UpdateOpCtrl();

        // IPC side.
        le_result_t RequestStart(const taf_UpdatePhase_t& phase);
        le_result_t RequestPause(taf_update_State_t running, bool* postPtr);
        le_result_t RequestResume(taf_update_State_t running, bool* postPtr);
        le_result_t RequestCancel(taf_update_State_t running, bool* postPtr);

        // Worker side.
        taf_UpdateCheckpoint_t Checkpoint();
        taf_UpdateCheckpoint_t DisableInterrupt();
        bool IsPauseRequested();
        bool IsCancelRequested();
        void ClearPause();
        void ClearCancel();
        void OnPaused();
        bool OnResume();
        void OnFinished();
        void Restore(const taf_UpdatePhase_t& phase, bool paused);

        // Both sides.
        bool IsIdle();
        bool IsPaused();
        taf_update_State_t GetRunningState();
        taf_update_State_t GetState();

    private:
        typedef enum
        {
            RUN_IDLE,
            RUN_RUNNING,
            RUN_PAUSED
        } Run_t;

        std::mutex mtx;
        Run_t run;
        taf_UpdatePhase_t phase;
        bool pauseReq;
        bool cancelReq;
        bool resumeQueued;
    };

    class taf_Update : public ITafSvc {
    public:
        taf_Update() {};
        ~taf_Update() {};

        static taf_Update &GetInstance();

        le_result_t CheckQotaHeader(const char* file);
        le_result_t RemoveQotaHeader(const char* file);

        void ReportDownloadStatus(taf_UpdateDownloadSession_t* sessPtr, taf_update_State_t state,
            taf_update_Error_t error = TAF_UPDATE_NONE);
        void ReportUpdateStatus(taf_UpdatePlugInSession_t* sessPtr, taf_update_State_t state,
            taf_update_Error_t error = TAF_UPDATE_NONE);
        static void DownloadTimerHandler(le_timer_Ref_t timerRef);
        static void UpdateTimerHandler(le_timer_Ref_t timerRef);

        static void StateLayeredHandler(void* reportPtr, void* layerHandlerFunc);

        static void DownloadHandler(void* reqPtr);
        static void UpdateHandler(void* reqPtr);

        void Init(void);

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

        taf_UpdateOpCtrl dlCtrl;   ///< Controller of download plug-in session.
        taf_UpdateOpCtrl upiCtrl;  ///< Controller of update plug-in session.

        static const taf_UpdatePhase_t DownloadPhase;
        static const taf_UpdatePhase_t PlugInInstallPhase;
    };
}

#endif
