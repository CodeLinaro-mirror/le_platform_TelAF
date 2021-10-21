/*
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       tafUpdate.cpp
 * @brief      This file includes test functions of the Update Service.
 */

#include "legato.h"
#include "interfaces.h"

#define TAF_UPDATE_TIME_FOR_REBOOT 10
#define TAF_UPDATE_TIME_FOR_EXIT 600

le_sem_Ref_t semaphore;

taf_update_StateHandlerRef_t handlerRef;

void UpdateStateHandlerFunc(taf_update_StateInfo_t* stateInfo, void* contextPtr)
{
    LE_INFO("**** OTA Mangager Handler (Begin)****");

    switch (stateInfo->state) {
        case TAF_UPDATE_DOWNLOADING:
            LE_INFO("OTA state: downloading percent = %d%% .", stateInfo->percent);
            break;
        case TAF_UPDATE_DOWNLOAD_COMPLETE:
            LE_INFO("OTA state: download complete");
            LE_INFO("OTA Manager start install.");
            LE_ASSERT(taf_update_Install() == LE_OK);
            break;
        case TAF_UPDATE_INSTALLING:
            LE_INFO("OTA state: installing");
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            LE_INFO("OTA state: install success");
            LE_INFO("OTA Manager will start reboot after 10s.");
            le_thread_Sleep(TAF_UPDATE_TIME_FOR_REBOOT);
            LE_ASSERT(taf_update_RebootToActive() == LE_OK);
            break;
        case TAF_UPDATE_REPORTING:
            LE_INFO("OTA state: reporting");
            break;
        case TAF_UPDATE_IDLE:
            LE_INFO("OTA state: idle");
            break;
        default:
            break;
    }
    LE_INFO("**** OTA Mangager Handler (End)****");
}

void* UpdateStateThread(void* contextPtr)
{
    //  connect service in thread.
    taf_update_ConnectService();

    handlerRef = taf_update_AddStateHandler(
        (taf_update_StateHandlerFunc_t)UpdateStateHandlerFunc, NULL);
    LE_ASSERT(handlerRef != NULL);
    le_sem_Post(semaphore);
    le_event_RunLoop();

    return NULL;
}

COMPONENT_INIT
{
    semaphore = le_sem_Create("tafUpdateSem", 0);
    le_thread_Ref_t threadRef = le_thread_Create("UpdateStateThread",
        UpdateStateThread, NULL);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);

    char version[TAF_UPDATE_MAX_VERS_LEN] = {0};
    taf_update_GetFirmwareVersion(version, TAF_UPDATE_MAX_VERS_LEN);
    LE_INFO("OTA Manager get firmware version: %s", version);
    LE_INFO("OTA Manager start download.");
    taf_update_Download(TAF_UPDATE_FIRMWARE);

    le_thread_Sleep(TAF_UPDATE_TIME_FOR_EXIT);

    exit(EXIT_SUCCESS);
}
