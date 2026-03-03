/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

static bool MpmsSvcOnline = false;
static const char* ModuleName = "mpms.tool.daemon";
static taf_mngdPm_wsRef_t WsRef = NULL;
static taf_mngdPm_NodePowerStateChangeHandlerRef_t StateChangeHandlerRef = NULL;
static le_timer_Ref_t TryConnectTimerRef = NULL;
static bool EnableAutoConnectToMpmsSvc = false;

// Local NAD device ID
#define NODE_ID 0
#define AUTO_TRY_CONNECT_INTERVAL (500)
#define ENABLE_AUTO_CONNECT_OFFON "/data/mpms.daemon.auto"

#define MASK_FOR_ALL_NOTIFICATIONS (\
TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME | \
TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE | \
TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE | \
TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE)

typedef enum {
    ACQUIRED,
    NOT_ACQUIRED
} WakeLockStatus_t;

static WakeLockStatus_t WakeLock = NOT_ACQUIRED;

#define check_MpmsSvcShouldOnline() \
do { \
    if (MpmsSvcOnline == false) \
    { \
        LE_ERROR("mpms.service is [Unavailable]"); \
        return LE_FAULT; \
    } \
} while (0)

static le_result_t AcquireWakeLock(void)
{
    le_result_t rst = taf_mngdPm_StayAwake(WsRef);

    if (rst != LE_OK)
    {
        if (rst == LE_DUPLICATE)
        {
            LE_INFO("Already acquired");

            rst = LE_OK; // Remark as LE_OK for command tool
        }
        else
        {
            LE_ERROR("Failed to acquire the wakelock");
        }
    }
    else
    {
        LE_INFO("Acquired successfully");
    }

    WakeLock = ACQUIRED;
    return rst;
}

static le_result_t ReleaseWakeLock(void)
{
    le_result_t rst = taf_mngdPm_Relax(WsRef);

    if (rst != LE_OK)
    {
        if (rst == LE_UNAVAILABLE)
        {
            LE_INFO("Never acquired");

            rst = LE_OK; // remark for command tool
        }
        else
        {
            LE_ERROR("Failed to release the wakelock");
        }
    }
    else
    {
        LE_INFO("Release successfully");
    }

    WakeLock = NOT_ACQUIRED;
    return rst;
}


static void MpmsServiceGoneHandler(void* context)
{
    LE_WARN("The mpms.service has [Gone].. to be unavailable");

    // Reset the mpms service online status
    MpmsSvcOnline = false;

    // Reset wake source & state change handler reference
    WsRef = NULL;
    StateChangeHandlerRef = NULL;

    // Reset wake lock status
    WakeLock = NOT_ACQUIRED;

    if (EnableAutoConnectToMpmsSvc == true)
    {
        LE_INFO("Monitor the mpms.service status until online..");
        le_timer_Start(TryConnectTimerRef);
    }
}

const char * string_OfState(taf_mngdPm_NodePowerState_t state)
{
    switch (state)
    {
        case TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE: return "shutdown";
        case TAF_MNGDPM_NODE_STATE_RESTART_PREPARE: return "restart";
        case TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE: return "suspend";
        case TAF_MNGDPM_NODE_STATE_RESUME: return "resume";
        default: return "";
    }
}

void PowerStateChangeHandler
(
     uint8_t pmNodeId,
     taf_mngdPm_nodePowerStateRef_t nodePowerStateRef,
     taf_mngdPm_NodePowerState_t state,
     void *contextPtr
)
{
    LE_INFO("[mpms.tool.d] node: %d, goes to state: [%s]", pmNodeId, string_OfState(state));

    le_result_t rst =
        taf_mngdPm_SendNodePowerStateChangeAck(
            pmNodeId,
            nodePowerStateRef,
            TAF_MNGDPM_CLIENT_READY);

    if(rst != LE_OK)
    {
        LE_ERROR("Failed to send ACK to mpms.service: %d", rst);
    }
    else
    {
        LE_INFO("Successfully send back ACK to mpms.service");
    }
}

le_result_t taf_mpms_tool_Ctrl_Suspend(void)
{
    le_result_t rst = LE_OK;

    check_MpmsSvcShouldOnline();

    if (WakeLock == ACQUIRED)
    {
        rst = ReleaseWakeLock();

        if (rst != LE_OK)
        {
            return LE_FAULT;
        }
    }

    LE_INFO("Successfully finish graceful /suspend");
    return LE_OK;
}

le_result_t taf_mpms_tool_Ctrl_Resume(void)
{
    le_result_t rst = LE_OK;

    check_MpmsSvcShouldOnline();

    if (WakeLock == ACQUIRED)
    {
        LE_INFO("Alread acquired wake lock");
        return LE_OK;
    }

    rst = AcquireWakeLock();

    if (rst != LE_OK)
    {
        return LE_FAULT;
    }

    LE_INFO("Successfully lock the wake source");
    return LE_OK;
}

le_result_t taf_mpms_tool_Ctrl_Shutdown(void)
{
    le_result_t rst = LE_OK;

    check_MpmsSvcShouldOnline();

    // Graceful shutdown
    rst = taf_mngdPm_SetNodeTargetedPowerMode(NODE_ID, TAF_MNGDPM_SHUTDOWN);

    if (rst != LE_OK)
    {
        LE_ERROR("Failed to taf_mngdPm_SetNodeTargetedPowerMode: %d for /%s",
                 rst, "shutdown");
    }

    if (WakeLock == ACQUIRED)
    {
        rst = ReleaseWakeLock();

        if (rst != LE_OK)
        {
            return LE_FAULT;
        }
    }

    LE_INFO("Successfully finish graceful /shutdown");
    return LE_OK;
}

le_result_t taf_mpms_tool_Ctrl_Restart(void)
{
    le_result_t rst = LE_OK;

    check_MpmsSvcShouldOnline();

    // Forceful [restart], do not care the wakelock.

    rst = taf_mngdPm_RestartNode(NODE_ID);

    if (rst != LE_OK)
    {
        LE_ERROR("Failed to taf_mngdPm_RestartNode: %d for /%s",
                 rst, "restart");
    }

    LE_INFO("Successfully finish forceful /restart");
    return LE_OK;
}

static le_result_t TryToConnectMpmsSvc(void)
{
    le_result_t rst = LE_OK;

    rst = taf_mngdPm_TryConnectService();

    if (rst != LE_OK)
    {
        MpmsSvcOnline = false;
        LE_ERROR("mpms.service is [Unavailable], needs 'selfcheck' for retry");

        goto err_no_connection;
    }

    // Monitor the service gone event, but not-exist in this case
    taf_mngdPm_SetNonExitServerDisconnectHandler(MpmsServiceGoneHandler, NULL);

    // Create one default wake source from mpms.service
    WsRef = taf_mngdPm_CreateWakeupSource(
                TAF_MNGDPM_STAY_AWAKE_REASON_NORMAL,
                TAF_MNGDPM_WS_OPT_DEFAULT,
                ModuleName);

    if (WsRef == NULL)
    {
        LE_ERROR("Failed to create wake source from mpms.service");
        rst = LE_FAULT;
        goto err;
    }
    else
    {
        LE_INFO("Successfully created wake source from mpms.service");
    }

    StateChangeHandlerRef = taf_mngdPm_AddNodePowerStateChangeHandler(
                                PowerStateChangeHandler,
                                NULL,
                                NODE_ID,
                                MASK_FOR_ALL_NOTIFICATIONS);

    if (StateChangeHandlerRef == NULL)
    {
        LE_ERROR("Failed to register the state change handler");
        rst = LE_FAULT;
        goto err;
    }
    else
    {
        LE_INFO("Successfully register the state change handler");
    }

    MpmsSvcOnline = true;
    LE_INFO("mpms.service --> [Online]");
    return rst;

err:

    if (WsRef)
    {
        taf_mngdPm_DeleteWakeupSource(WsRef);
    }

    taf_mngdPm_DisconnectService();

err_no_connection:

    if (EnableAutoConnectToMpmsSvc == true)
    {
        le_timer_Start(TryConnectTimerRef);
    }

    return rst;
}

le_result_t taf_mpms_tool_Ctrl_Selfcheck(void)
{
    LE_INFO("mpms.tool.daemon: [%s] captured", "selfcheck");

    if (MpmsSvcOnline == true)
    {
        LE_INFO("mpms.service is already [Online]");
        return LE_OK;
    }
    else
    {
        return TryToConnectMpmsSvc();
    }
}

static void TimerExpiredHandler
(
    le_timer_Ref_t timerRef
)
{
    le_result_t rst = TryToConnectMpmsSvc();

    if (rst != LE_OK)
    {
        le_timer_Start(TryConnectTimerRef);
        LE_INFO("mpms.service is [Offline], Contine monitoring ..");
    }
    else
    {
        LE_INFO("mpms.service is [Online], connected, stop timer");
    }
}


static bool EnableAutoConnection(void)
{
    const char *path = ENABLE_AUTO_CONNECT_OFFON;

    if (access(path, F_OK) == 0)
    {
        LE_INFO("[Enable] auto connect to mpms.service");
        return true;
    }
    else
    {
        LE_INFO("[Disable] auto connect to mpms.service");
        return false;
    }
}

COMPONENT_INIT
{
    LE_INFO("mpms.tool.daemon --> Starting ...");

    if (EnableAutoConnection())
    {
        EnableAutoConnectToMpmsSvc = true;

        TryConnectTimerRef = le_timer_Create(ModuleName);
        le_timer_SetWakeup(TryConnectTimerRef, false);
        le_timer_SetMsInterval(TryConnectTimerRef, AUTO_TRY_CONNECT_INTERVAL);
        le_timer_SetHandler(TryConnectTimerRef, TimerExpiredHandler);
    }

    (void) TryToConnectMpmsSvc();

    LE_INFO("mpms.tool.daemon --> Ready for mpms.tool.client");
}
