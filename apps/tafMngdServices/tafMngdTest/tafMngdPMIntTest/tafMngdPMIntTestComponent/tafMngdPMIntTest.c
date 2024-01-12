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
int status = EXIT_SUCCESS;

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
        "\n");
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

COMPONENT_INIT
{
    const char* testType = "";

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
        else
        {
            LE_ERROR("Error command");
            exit(EXIT_FAILURE);
        }

    }
}
