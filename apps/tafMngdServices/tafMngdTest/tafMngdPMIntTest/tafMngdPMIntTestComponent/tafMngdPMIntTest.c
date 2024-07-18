/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * @file       tafMngdPmIntTest.c
 * @brief      This file includes integration test functions of Managed Connectivity Service.
 */

#include "legato.h"
#include "interfaces.h"
taf_pm_WakeupSourceRef_t ws = NULL;
static le_sem_Ref_t tafMpmAppSem;
le_clk_Time_t Timeout = { 5 , 0 };
#define WAKELOCK_WITHOUT_REF 0
#define NODE_ID 0
int status = EXIT_SUCCESS;
const char* vHalTag = "vehichle_on";
#define VEHICHLE_WAKEUP_REASON_DEFAULT 0

static void PrintUsage ()
{
    puts("\n"
        "app start tafMngdPMIntTest\n"
        "--------To know Usage--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- help \n"
        "--------To Restart the System --------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- RestartSystem \n"
        "--------To KeepAwakeThenRestartSystem the System --------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- KeepAwakeThenRestartSystem \n"
        "--------To trigger the Forced System Shutdown--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ForcedSystemShutdown \n"
        "--------To triggger the Graceful shutdown with the wake lock acquired--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysShutdownWakeLock\n"
        "--------To trigger the Graceful shutdown without wake lock acquired--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysShutdown\n"
        "--------To triggger the Graceful suspend with the wake lock acquired--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysSuspendWakeLock\n"
        "--------To trigger the Graceful suspend without wake lock acquired--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysSuspend\n"
        "------------To set the modem wakeuptypes-----------\n"
        "--------1 -> For SMS wakeuptype------------\n"
        "--------2 -> For VOICE_CALL wakeuptype------------\n"
        "--------3 -> For SMS and VOICE_CALL wakeuptype------\n"
        "--------4 -> For MCU_VHAL wakeuptype------------\n"
        "--------5 -> For SMS and MCU_VHAL wakeuptype------\n"
        "--------6 -> For VOICE_CALL and MCU_VHAL wakeuptype------\n"
        "--------7 -> For SMS, VOICE_CALL and MCU_VHAL wakeuptype------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- SetModemWakeupSource <wakeuptype>\n"
        "------------To suspend the system with wakeuptypes-----------\n"
        "--------0 -> For APP_STAYAWAKE wakeuptype------------\n"
        "--------1 -> For SMS wakeuptype------------\n"
        "--------2 -> For VOICE_CALL wakeuptype------------\n"
        "--------3 -> For MCU_VHAL wakeuptype------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- SuspendSystem <wakeuptype>\n"
        "------------To resume the system with wakeuptypes-----------\n"
        "--------0 -> For APP_STAYAWAKE wakeuptype------------\n"
        "--------1 -> For SMS wakeuptype------------\n"
        "--------2 -> For VOICE_CALL wakeuptype------------\n"
        "--------3 -> For MCU_VHAL wakeuptype------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ResumeSystem <wakeuptype>\n"
        "\n"
        "------------To restart the particular node with node ID-----------\n"
        "--------0 -> For NAD ------------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- RestartNode <NODE_ID>\n"
        "\n"
        "------------To Shutdown the particular node with node ID-----------\n"
        "--------0 -> For NAD ------------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ShutdownNode <NODE_ID>\n"
        "------------To WakeupVehicle-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- WakeupVehicle\n"
        "------------To GetInfoReport-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GetInfoReport\n"
        "------------To AddInfoReportHandler-----------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- AddInfoReportHandler\n");
}

void NodePowerStateChangeHandlerCB(
     uint8_t pmNodeId,
     taf_mngdPm_nodePowerStateRef_t nodePowerStateRef,
     taf_mngdPm_NodePowerState_t state,
	 void *contextPtr)
{
    LE_INFO("NodePowerStateChangeHandlerFunc callback");
    le_result_t res = LE_FAULT;
    res = taf_mngdPm_SendNodePowerStateChangeAck(pmNodeId, nodePowerStateRef,state, TAF_MNGDPM_CLIENT_READY);
    if(res == LE_OK)
    {
        LE_INFO("SendNodePowerStateChangeAck is success");
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}

void AddNodePowerStateChangeHandler
(
    const char* NodePowerStateChangeBitMask
)
{
    LE_INFO("taf_mngdPm_AddNodePowerStateChangeHandler");
    uint8_t pmNodeId = 0;
    if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE") == 0)
    {
        taf_mngdPm_NodePowerStateChangeBitMask_t stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE");
        }
    }
    else if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE") == 0)
    {
        taf_mngdPm_NodePowerStateChangeBitMask_t stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE");
        }
    }
    else if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE") == 0)
    {
        taf_mngdPm_NodePowerStateChangeBitMask_t stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE");
        }
    }
    else if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME") == 0)
    {
        taf_mngdPm_NodePowerStateChangeBitMask_t stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME");
        }
     }
}

void RestartCallback(taf_mngdPm_RestartMode_t mode, taf_mngdPm_ResponseMode_t rspmode ,
        void* contextPtr)
{
    LE_INFO("RestartCallback response mode is %d", rspmode);
}

static void RestartSystem()
{
    LE_INFO("----Restart System test----" );
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE");
    le_result_t res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON,
            RestartCallback, NULL);

    if(res == LE_OK)
    {
        LE_INFO("----RestartSystem success----");
    }
    else
    {
        LE_ERROR("RestartSystem request failed");
    }
}

void ForcedSystemShutdownCallBack(taf_mngdPm_ShutdownMode_t mode,
    taf_mngdPm_ResponseMode_t ResponseMode, void* contextPtr)
{
    LE_INFO("ForcedSystemShutdownCallBack response mode is %d", ResponseMode);
}

static void ForcedSystemShutdown()
{
    LE_INFO("----ForcedSystemShutdown test----");
    le_result_t result;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE");
    result = taf_mngdPm_ShutdownReqAsync(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
            ForcedSystemShutdownCallBack, NULL);

    if(result == LE_OK)
    {
        LE_INFO("----ForcedSystemShutdown success----");
    }
    else
    {
        LE_ERROR("ForcedSystemShutdown request failed");
    }
}

void GracefulSysShutdownWakeLock()
{
    LE_INFO("----GracefulSysShutdownWakeLock test----");
    le_result_t result =  LE_FAULT;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE");
    // Create and acquire a wakelock to get notified on last wakeup source release.
    if(ws == NULL)
        ws = taf_pm_NewWakeupSource(0, "mpms");

    if (ws != NULL) {
        result = taf_pm_StayAwake(ws);
        if(result == LE_OK) {
            LE_INFO("Wake source acquired successfully");

            result = taf_mngdPm_SetNodeTargetedPowerMode(WAKELOCK_WITHOUT_REF,
                    TAF_MNGDPM_SHUTDOWN);
            if(result == LE_OK)
                LE_INFO("GracefulSysShutdownWakeLock triggered successfully");

            tafMpmAppSem = le_sem_Create("tafMpmAppSem", 0);
            le_sem_WaitWithTimeOut(tafMpmAppSem, Timeout);
            LE_INFO("wake lock timer expired");
            le_sem_Delete(tafMpmAppSem);

            result = taf_pm_Relax(ws);
            if(result == LE_OK)
                LE_INFO("Wake source released successfully");
        }
        else {
            LE_INFO("Failed to acquire Wake source");
        }
    }
    else {
        LE_ERROR("Failed to create wakeup source!");
    }

    if(result == LE_OK)
    {
        LE_INFO("----GracefulSysShutdownWakeLock success----");
    }
    else
    {
        LE_ERROR("GracefulSysShutdownWakeLock request failed");
    }
}
void GracefulSysSuspendWakeLock()
{
    LE_INFO("----GracefulSysSuspendWakeLock test----");
    le_result_t result =  LE_FAULT;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE");
    // Create and acquire a wakelock to get notified on last wakeup source release.
    if(ws == NULL)
        ws = taf_pm_NewWakeupSource(0, "mpms");

    if (ws != NULL) {
        result = taf_pm_StayAwake(ws);
        if(result == LE_OK) {
            LE_INFO("Wake source acquired successfully");

            result = taf_mngdPm_SetNodeTargetedPowerMode(WAKELOCK_WITHOUT_REF,
                    TAF_MNGDPM_SUSPEND);
            if(result == LE_OK)
                LE_INFO("GracefulSysSuspendWakeLock triggered successfully");

            tafMpmAppSem = le_sem_Create("tafMpmAppSem", 0);
            le_sem_WaitWithTimeOut(tafMpmAppSem, Timeout);
            LE_INFO("wake lock timer expired");
            le_sem_Delete(tafMpmAppSem);

            result = taf_pm_Relax(ws);
            if(result == LE_OK)
                LE_INFO("Wake source released successfully");
        }
        else {
            LE_INFO("Failed to acquire Wake source");
        }
    }
    else {
        LE_ERROR("Failed to create wakeup source!");
    }

    if(result == LE_OK)
    {
        LE_INFO("----GracefulSysSuspendWakeLock success----");
    }
    else
    {
        LE_ERROR("GracefulSysSuspendWakeLock request failed");
    }
}

void GracefulSysShutdown()
{
    LE_INFO("----GracefulSysShutdown test " );
    le_result_t result =  LE_FAULT;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE");
    LE_INFO("GracefulSysShutdown without wake source");
    result = taf_mngdPm_SetNodeTargetedPowerMode(WAKELOCK_WITHOUT_REF,
            TAF_MNGDPM_SHUTDOWN);

    if(result == LE_OK)
    {
        LE_INFO("----GracefulSysShutdown success----");
    }
    else
    {
        LE_ERROR("GracefulSysShutdown request failed");
    }

}

void GracefulSysSuspend()
{
    LE_INFO("----GracefulSysSuspend test " );
    le_result_t result =  LE_FAULT;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE");
    LE_INFO("GracefulSysSuspend without wake source");
    result = taf_mngdPm_SetNodeTargetedPowerMode(WAKELOCK_WITHOUT_REF,
            TAF_MNGDPM_SUSPEND);
    if(result == LE_OK)
    {
        LE_INFO("----GracefulSysSuspend success----");
    }
    else
    {
        LE_ERROR("GracefulSysSuspend request failed");
    }

}

static int SetModemWakeupSource(const char* wakeupSource)
{

    // Convert string to uint32_t
    uint32_t uintResult = (uint32_t)strtoul(wakeupSource, NULL, 10);
    // Check for conversion errors
    if (uintResult > UINT32_MAX) {
        fprintf(stderr, "Value out of range.\n");
        exit(EXIT_FAILURE);
    }
    // Print the result
    printf("String: %s\nConverted to uint32_t: %u\n", wakeupSource, uintResult);

    le_result_t res = LE_FAULT;
    res = taf_mngdPm_SetModemWakeupSource(uintResult);
    if(res == LE_OK) {
        return EXIT_SUCCESS;
    }
    else
    {
        LE_ERROR("SetModemWakeupSource request failed");
        return EXIT_FAILURE;
    }
}

void SuspendSystem(const char* wakeuptype)
{
    le_result_t res = LE_FAULT;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE");
    taf_mngdPm_wsRef_t wsRef = NULL;
    if(strcmp(wakeuptype, "0") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is APP_STAYAWAKE");
        wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_APP_STAYAWAKE, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for APP_STAYAWAKE");
            res = taf_mngdPm_RelaxNode(wsRef);
            if(res == LE_OK)
                LE_INFO("suspended sysytem with wakeuptype APP_STAYAWAKE");
        }
    }
    else if(strcmp(wakeuptype, "1") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is SMS");
        wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_SMS, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for SMS");
            res = taf_mngdPm_RelaxNode(wsRef);
            if(res == LE_OK)
                LE_INFO("suspended sysytem with wakeuptype SMS");
        }
    }
    else if(strcmp(wakeuptype, "2") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is VOICE_CALL");
        wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_VOICE_CALL, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for VOICE_CALL");
            res = taf_mngdPm_RelaxNode(wsRef);
            if(res == LE_OK)
                LE_INFO("suspended sysytem with wakeuptype VOICE_CALL");
        }
    }
    else if(strcmp(wakeuptype, "3") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is SMS");
        wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_MCU_VHAL, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for MCU_VHAL");
            res = taf_mngdPm_RelaxNode(wsRef);
            if(res == LE_OK)
                LE_INFO("suspended sysytem with wakeuptype MCU_VHAL");
        }
    }
    else {
        LE_ERROR("SuspendSystem failed");
    }
}

void ResumeSystem(const char* wakeuptype)
{
    le_result_t res = LE_FAULT;
    taf_mngdPm_wsRef_t wsRef = NULL;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME");
    if(strcmp(wakeuptype, "0") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is APP_STAYAWAKE");
        wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_APP_STAYAWAKE, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for APP_STAYAWAKE");
            res = taf_mngdPm_StayAwakeNode(wsRef);
            if(res == LE_OK) {
                LE_INFO("Resumed sysytem with wakeuptype APP_STAYAWAKE");
             }
        }
    }
    else if(strcmp(wakeuptype, "1") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is SMS");
        wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_SMS, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for SMS");
            res = taf_mngdPm_StayAwakeNode(wsRef);
            if(res == LE_OK) {
                LE_INFO("Resumed sysytem with wakeuptype SMS");
             }
        }
    }
    else if(strcmp(wakeuptype, "2") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is VOICE_CALL");
        wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_VOICE_CALL, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for VOICE_CALL");
            res = taf_mngdPm_StayAwakeNode(wsRef);
            if(res == LE_OK) {
                LE_INFO("Resumed sysytem with wakeuptype VOICE_CALL");
             }
        }
    }
    else if(strcmp(wakeuptype, "3") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is SMS");
        wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_MCU_VHAL, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for MCU_VHAL");
            res = taf_mngdPm_StayAwakeNode(wsRef);
            if(res == LE_OK) {
                LE_INFO("Resumed sysytem with wakeuptype MCU_VHAL");
             }
        }
    }
    else {
        LE_ERROR("ResumeSystem failed");
   }
}

static int RestartNode(const char* node_id)
{
    LE_INFO("RestartNode");
    le_result_t res = LE_FAULT;
    uint8_t Node = atoi(node_id);
    LE_INFO("RestartNode for %d", Node);
    res = taf_mngdPm_RestartNode(Node);
    if(res == LE_OK)
    {
        LE_INFO("restarted the node");
        return EXIT_SUCCESS;
    }
    else {
        LE_ERROR("RestartNode failed");
        return EXIT_FAILURE;
    }
}

static int ShutdownNode(const char* node_id)
{
    LE_INFO("ShutdownNode");
    le_result_t res = LE_FAULT;
    uint8_t Node = atoi(node_id);
    LE_INFO("ShutdownNode for NAD %d", Node);
    res = taf_mngdPm_ShutdownNode(Node);
    if(res == LE_OK)
    {
        LE_INFO("ShutdownNode is successfull");
        return EXIT_SUCCESS;
    }
    else {
        LE_ERROR("ShutdownNode failed");
        return EXIT_FAILURE;
    }
}

void WakeupVehicleback(int32_t reason, int32_t rspmode ,
        void* contextPtr)
{
    LE_INFO("WakeupVehicleback response is %d", rspmode);
    exit(status);
}

static int WakeupVehicle()
{
    LE_INFO("WakeupVehicle");
    le_result_t res = taf_mngdPm_WakeupVehicleReqAsync(VEHICHLE_WAKEUP_REASON_DEFAULT,
            WakeupVehicleback, NULL);

    if(res == LE_OK)
    {
        LE_INFO("----WakeupVehicle success----");
        status = EXIT_SUCCESS;
    }
    else
    {
        LE_ERROR("WakeupVehicle request failed, Ensure device is in resume state");
        status = EXIT_FAILURE;
    }
    return status;
}

static int GetInfoReport()
{
    LE_INFO("GetInfoReport");
    int32_t status;
    le_result_t res = taf_mngdPm_GetInfoReport(TAF_MNGDPM_INFO_REPORT_BUB, &status);
    if(res == LE_OK)
    {
        if(status == TAF_MNGDPM_BUB_STATUS_IN_USE)
        {
            printf("Bub Status is TAF_MNGDPM_BUB_STATUS_IN_USE");
        }
        else if(status == TAF_MNGDPM_BUB_STATUS_NOT_IN_USE)
        {
            printf("Bub Status is TAF_MNGDPM_BUB_STATUS_NOT_IN_USE");
        }
        else if(status == TAF_MNGDPM_BUB_STATUS_UNKNOWN)
        {
            printf("Bub Status is TAF_MNGDPM_BUB_STATUS_UNKNOWN");
        }
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

void BubCallBack( int32_t status, void *contextptr)
{
    LE_INFO("BubCallBack is:%d", status);
    if(status == TAF_MNGDPM_BUB_STATUS_IN_USE)
    {
        printf("Bub is TAF_MNGDPM_BUB_STATUS_IN_USE \n");
    }
    else if(status == TAF_MNGDPM_BUB_STATUS_NOT_IN_USE)
    {
        printf("Bub is TAF_MNGDPM_BUB_STATUS_NOT_IN_USE\n");
    }
    else if(status == TAF_MNGDPM_BUB_STATUS_UNKNOWN)
    {
        printf("Bub is TAF_MNGDPM_BUB_STATUS_UNKNOWN\n");
    }
    else
    {
        printf("Error status returned");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

void AddInfoReportHandler()
{
    LE_INFO("AddInfoReportHandler");
    taf_mngdPm_InfoReportHandlerRef_t handlerRef;
    handlerRef = taf_mngdPm_AddInfoReportHandler((taf_mngdPm_InfoReportBitMask_t)1, BubCallBack, NULL);
    if(handlerRef)
    {
         LE_INFO("AddInfoReportHandler is success");
    }
}

static int KeepAwakeThenRestartSystem()
{
    LE_INFO("KeepAwakeThenRestartSystem");
    le_result_t res = LE_FAULT;
    taf_mngdPm_wsRef_t wsRef = NULL;
    LE_INFO("NewNodeWakeupSource wakeuptype is APP_STAYAWAKE");
    wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_APP_STAYAWAKE, vHalTag);
    if(wsRef != NULL) {
        LE_INFO("NewNodeWakeupSource ref is created for APP_STAYAWAKE");
        res = taf_mngdPm_StayAwakeNode(wsRef);
        if(res == LE_OK) {
            LE_INFO("Wake up sysytem with wakeuptype APP_STAYAWAKE");
        }
    }

    taf_mngdPm_wsRef_t wsRefSms0 = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_SMS, vHalTag);
    if (wsRefSms0 != NULL)
    {
        LE_INFO("NewNodeWakeupSource ref is created for SMS 0");
        res = taf_mngdPm_StayAwakeNode(wsRefSms0);
        if(res == LE_OK) {
            LE_INFO("Wake up sysytem with wakeuptype SMS 0");
        }
        res = taf_mngdPm_RelaxNode(wsRefSms0);
        if(res == LE_OK) {
            LE_INFO("Relax sysytem with wakeuptype SMS 0");
        }
    }
    taf_mngdPm_wsRef_t wsRefSms1 = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_SMS, vHalTag);
    if (wsRefSms1 != NULL)
    {
        LE_INFO("NewNodeWakeupSource ref is created for SMS 1");
        res = taf_mngdPm_StayAwakeNode(wsRefSms1);
        if(res == LE_OK) {
            LE_INFO("Wake up sysytem with wakeuptype SMS 1");
        }
    }

    RestartSystem();
    return EXIT_SUCCESS;
}

COMPONENT_INIT
{
    const char* testType = "";
    const char* testPar = "";
    if (le_arg_NumArgs() == 0 )
    {
        PrintUsage();
    }
    if (le_arg_NumArgs() >= 1)
    {
        testType = le_arg_GetArg(0);
        LE_INFO("arg0=%s ",testType);
        if (NULL == testType) {
            LE_ERROR("testType is NULL");
            exit(EXIT_FAILURE);
        }
        testPar = le_arg_GetArg(1);
        LE_INFO("arg1=%s ",testPar);
        if (NULL == testPar) {
            LE_ERROR("testPar is 0");
        }
        if (testType!= NULL && strncmp(testType,"help", 4) == 0)
        {
            PrintUsage();
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(testType, "RestartSystem") == 0)
        {
            RestartSystem();
        }
        else if(strcmp(testType, "KeepAwakeThenRestartSystem") == 0)
        {
            status = SetModemWakeupSource("1"); // whitelist SMS wakeup type
            status = KeepAwakeThenRestartSystem();
            exit(status);
        }
        else if(strcmp(testType, "ForcedSystemShutdown") == 0)
        {
            ForcedSystemShutdown();
        }
        else if(strcmp(testType, "GracefulSysShutdownWakeLock") == 0)
        {
            GracefulSysShutdownWakeLock();
        }
        else if(strcmp(testType, "GracefulSysShutdown") == 0)
        {
            GracefulSysShutdown();
        }
        else if(strcmp(testType, "GracefulSysSuspendWakeLock") == 0)
        {
            GracefulSysSuspendWakeLock();
        }
        else if(strcmp(testType, "GracefulSysSuspend") == 0)
        {
            GracefulSysSuspend();
        }
        else if(strcmp(testType, "SetModemWakeupSource") == 0)
        {
            status = SetModemWakeupSource(testPar);
            exit(status);
        }
        else if(strcmp(testType, "SuspendSystem") == 0)
        {
            SuspendSystem(testPar);
        }
        else if(strcmp(testType, "ResumeSystem") == 0)
        {
            ResumeSystem(testPar);
        }
        else if(strcmp(testType, "RestartNode") == 0)
        {
            status = RestartNode(testPar);
            exit(status);
        }
        else if(strcmp(testType, "ShutdownNode") == 0)
        {
            status = ShutdownNode(testPar);
            exit(status);
        }
        else if(strcmp(testType, "WakeupVehicle") == 0)
        {
            status = WakeupVehicle();
            exit(status);
        }
        else if(strcmp(testType, "GetInfoReport") == 0)
        {
            status = GetInfoReport();
            exit(status);
        }
        else if(strcmp(testType, "AddInfoReportHandler") == 0)
        {
            AddInfoReportHandler();
        }
        else
        {
            LE_ERROR("Error command");
            exit(EXIT_FAILURE);
        }
    }
}
