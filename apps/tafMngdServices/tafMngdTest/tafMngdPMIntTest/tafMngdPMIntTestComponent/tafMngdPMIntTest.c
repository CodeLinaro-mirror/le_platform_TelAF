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
        "--------To trigger the Forced System Shutdown--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- ForcedSystemShutdown \n"
        "--------To triggger the Graceful shutdown with the wake lock acquired--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysShutdownWakeLock\n"
        "--------To trigger the Graceful shutdown without wake lock acquired--------\n"
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- GracefulSysShutdown\n"
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
        "app runProc tafMngdPMIntTest --exe=tafMngdPMIntTest -- WakeupVehicle\n");
}

void RestartCallback(taf_mngd_pm_RestartMode_t mode, taf_mngd_pm_ResponseMode_t rspmode ,
        void* contextPtr)
{
    LE_INFO("RestartCallback response mode is %d", rspmode);
    exit(status);
}

static void RestartSystem()
{
    LE_INFO("----Restart System test----" );

    le_result_t res = taf_mngd_pm_RestartReqAsync(TAF_MNGD_PM_RESTART_SYSTEM_OFF_ON,
            RestartCallback, NULL);

    if(res == LE_OK)
    {
        LE_INFO("----RestartSystem success----");
        status = EXIT_SUCCESS;
    }
    else
    {
        LE_ERROR("RestartSystem request failed");
        status = EXIT_FAILURE;
    }
    return ;
}

void ForcedSystemShutdownCallBack(taf_mngd_pm_ShutdownMode_t mode,
    taf_mngd_pm_ResponseMode_t ResponseMode, void* contextPtr)
{
    LE_INFO("ForcedSystemShutdownCallBack response mode is %d", ResponseMode);
    exit(status);
}

static void ForcedSystemShutdown()
{
    LE_INFO("----ForcedSystemShutdown test----");
    le_result_t result;
    result = taf_mngd_pm_ShutdownReqAsync(TAF_MNGD_PM_SYSTEM_FORCEFUL_SHUTDOWN,
            ForcedSystemShutdownCallBack, NULL);

    if(result == LE_OK)
    {
        LE_INFO("----ForcedSystemShutdown success----");
        status = EXIT_SUCCESS;
    }
    else
    {
        LE_ERROR("ForcedSystemShutdown request failed");
        status = EXIT_FAILURE;
    }
    return;
}

static int GracefulSysShutdownWakeLock()
{
    LE_INFO("----GracefulSysShutdownWakeLock test----");
    le_result_t result =  LE_FAULT;

    // Create and acquire a wakelock to get notified on last wakeup source release.
    if(ws == NULL)
        ws = taf_pm_NewWakeupSource(0, "mpms");

    if (ws != NULL) {
        result = taf_pm_StayAwake(ws);
        if(result == LE_OK) {
            LE_INFO("Wake source acquired successfully");

            result = taf_mngd_pm_SetNodeTargetedPowerMode(WAKELOCK_WITHOUT_REF,
                    TAF_MNGD_PM_SHUTDOWN);
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
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int GracefulSysShutdown()
{
    LE_INFO("----GracefulSysShutdown test " );
    le_result_t result =  LE_FAULT;

    LE_INFO("GracefulSysShutdown without wake source");
    result = taf_mngd_pm_SetNodeTargetedPowerMode(WAKELOCK_WITHOUT_REF,
            TAF_MNGD_PM_SHUTDOWN);

    if(result == LE_OK)
    {
        LE_INFO("----GracefulSysShutdown success----");
    }
    else
    {
        LE_ERROR("GracefulSysShutdown request failed");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
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
    res = taf_mngd_pm_SetModemWakeupSource(uintResult);
    if(res == LE_OK) {
        return EXIT_SUCCESS;
    }
    else
    {
        LE_ERROR("SetModemWakeupSource request failed");
        return EXIT_FAILURE;
    }
}

static int SuspendSystem(const char* wakeuptype)
{
    le_result_t res = LE_FAULT;
    taf_mngd_pm_wsRef_t wsRef = NULL;
    if(strcmp(wakeuptype, "0") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is APP_STAYAWAKE");
        wsRef = taf_mngd_pm_NewNodeWakeupSource(NODE_ID, TAF_MNGD_PM_APP_STAYAWAKE, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for APP_STAYAWAKE");
            res = taf_mngd_pm_RelaxNode(wsRef);
            if(res == LE_OK)
                LE_INFO("suspended sysytem with wakeuptype APP_STAYAWAKE");
        }
    }
    else if(strcmp(wakeuptype, "1") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is SMS");
        wsRef = taf_mngd_pm_NewNodeWakeupSource(NODE_ID, TAF_MNGD_PM_SMS, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for SMS");
            res = taf_mngd_pm_RelaxNode(wsRef);
            if(res == LE_OK)
                LE_INFO("suspended sysytem with wakeuptype SMS");
        }
    }
    else if(strcmp(wakeuptype, "2") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is VOICE_CALL");
        wsRef = taf_mngd_pm_NewNodeWakeupSource(NODE_ID, TAF_MNGD_PM_VOICE_CALL, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for VOICE_CALL");
            res = taf_mngd_pm_RelaxNode(wsRef);
            if(res == LE_OK)
                LE_INFO("suspended sysytem with wakeuptype VOICE_CALL");
        }
    }
    else if(strcmp(wakeuptype, "3") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is SMS");
        wsRef = taf_mngd_pm_NewNodeWakeupSource(NODE_ID, TAF_MNGD_PM_MCU_VHAL, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for MCU_VHAL");
            res = taf_mngd_pm_RelaxNode(wsRef);
            if(res == LE_OK)
                LE_INFO("suspended sysytem with wakeuptype MCU_VHAL");
        }
    }
    else {
        LE_ERROR("SuspendSystem failed");
        return EXIT_FAILURE;
    }
    return res;
}

static int ResumeSystem(const char* wakeuptype)
{
    le_result_t res = LE_FAULT;
    taf_mngd_pm_wsRef_t wsRef = NULL;
    if(strcmp(wakeuptype, "0") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is APP_STAYAWAKE");
        wsRef = taf_mngd_pm_NewNodeWakeupSource(NODE_ID, TAF_MNGD_PM_APP_STAYAWAKE, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for APP_STAYAWAKE");
            res = taf_mngd_pm_StayAwakeNode(wsRef);
            if(res == LE_OK) {
                LE_INFO("Resumed sysytem with wakeuptype APP_STAYAWAKE");
                return EXIT_SUCCESS;
             }
        }
    }
    else if(strcmp(wakeuptype, "1") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is SMS");
        wsRef = taf_mngd_pm_NewNodeWakeupSource(NODE_ID, TAF_MNGD_PM_SMS, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for SMS");
            res = taf_mngd_pm_StayAwakeNode(wsRef);
            if(res == LE_OK) {
                LE_INFO("Resumed sysytem with wakeuptype SMS");
                return EXIT_SUCCESS;
             }
        }
    }
    else if(strcmp(wakeuptype, "2") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is VOICE_CALL");
        wsRef = taf_mngd_pm_NewNodeWakeupSource(NODE_ID, TAF_MNGD_PM_VOICE_CALL, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for VOICE_CALL");
            res = taf_mngd_pm_StayAwakeNode(wsRef);
            if(res == LE_OK) {
                LE_INFO("Resumed sysytem with wakeuptype VOICE_CALL");
                return EXIT_SUCCESS;
             }
        }
    }
    else if(strcmp(wakeuptype, "3") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is SMS");
        wsRef = taf_mngd_pm_NewNodeWakeupSource(NODE_ID, TAF_MNGD_PM_MCU_VHAL, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for MCU_VHAL");
            res = taf_mngd_pm_StayAwakeNode(wsRef);
            if(res == LE_OK) {
                LE_INFO("Resumed sysytem with wakeuptype MCU_VHAL");
                return EXIT_SUCCESS;
             }
        }
    }
    else {
        LE_ERROR("ResumeSystem failed");
        return EXIT_FAILURE;
   }
   return res;
}

static int RestartNode(const char* node_id)
{
    LE_INFO("RestartNode");
    le_result_t res = LE_FAULT;
    uint8_t Node = atoi(node_id);
    LE_INFO("RestartNode for %d", Node);
    res = taf_mngd_pm_RestartNode(Node);
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
    res = taf_mngd_pm_ShutdownNode(Node);
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
    le_result_t res = taf_mngd_pm_WakeupVehicleReqAsync(VEHICHLE_WAKEUP_REASON_DEFAULT,
            WakeupVehicleback, NULL);

    if(res == LE_OK)
    {
        LE_INFO("----WakeupVehicle success----");
        status = EXIT_SUCCESS;
    }
    else
    {
        LE_ERROR("WakeupVehicle request failed");
        status = EXIT_FAILURE;
    }
    return status;
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
        }
        else if(strcmp(testType, "RestartSystem") == 0)
        {
            RestartSystem();
        }
        else if(strcmp(testType, "ForcedSystemShutdown") == 0)
        {
            ForcedSystemShutdown();
        }
        else if(strcmp(testType, "GracefulSysShutdownWakeLock") == 0)
        {
            status = GracefulSysShutdownWakeLock();
            exit(status);
        }
        else if(strcmp(testType, "GracefulSysShutdown") == 0)
        {
            status = GracefulSysShutdown();
            exit(status);
        }
        else if(strcmp(testType, "SetModemWakeupSource") == 0)
        {
            status = SetModemWakeupSource(testPar);
            exit(status);
        }
        else if(strcmp(testType, "SuspendSystem") == 0)
        {
            status = SuspendSystem(testPar);
            exit(status);
        }
        else if(strcmp(testType, "ResumeSystem") == 0)
        {
            status = ResumeSystem(testPar);
            exit(status);
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
        else
        {
            LE_ERROR("Error command");
            exit(EXIT_FAILURE);
        }
    }
}
