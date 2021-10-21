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
 * @file       tafUpdateImpl.cpp
 * @brief      This file describes the implementation method that update
 *             service is in use.
 */

#include <cstdlib>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include "tafUpdate.hpp"

using namespace std;
using namespace telux::tafsvc;
using namespace telux::data;

taf_UpdateServingSystemListener::taf_UpdateServingSystemListener(SlotId slot) :
   slotId(slot) {
}

void taf_UpdateServingSystemListener::onServiceStateChanged(telux::data::ServiceStatus status)
{
    std::lock_guard<std::mutex> lock(cv_mutex);
    LE_DEBUG("<SDK Listener> taf_UpdateServingSystemListener --> onServiceStateChanged");

    dsStatus = status.serviceState;
    LE_DEBUG("status = %d", (int)dsStatus);
    if (dsStatus == telux::data::DataServiceState::IN_SERVICE) {
        conVar.notify_all();
    }
}

void taf_UpdateRequestServiceStatusCallback::requestServiceStatus
(
    telux::data::ServiceStatus serviceStatus,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> taf_UpdateRequestServiceStatusCallback --> requestServiceStatus");

    if (error != telux::common::ErrorCode::SUCCESS) {
        LE_ERROR("Error(%d)", (int)error);
    }

    status = serviceStatus;
    le_sem_Post(semaphore);
}

le_event_Id_t taf_Update::updateCmdEvId = nullptr;

taf_Update &taf_Update::GetInstance()
{
    static taf_Update instance;
    return instance;
}

void taf_Update::UpdateSetState(taf_update_State_t state)
{
    le_fs_FileRef_t fileRef;
    le_result_t res = le_fs_Open(TAF_UPDATE_STATE_FILE, LE_FS_CREAT | LE_FS_WRONLY, &fileRef);
    TAF_ERROR_IF_RET_NIL(res != LE_OK, "Fail to open state file.");

    res = le_fs_Write(fileRef, (uint8_t*)&state, sizeof(taf_update_State_t));
    if (res != LE_OK) {
        LE_ERROR("Fail to write state.");
    } else {
        LE_DEBUG("Set state: %d.", state);
    }
    le_fs_Close(fileRef);
}

void taf_Update::UpdateGetState(taf_update_State_t* state)
{
    le_fs_FileRef_t fileRef;
    le_result_t res = le_fs_Open(TAF_UPDATE_STATE_FILE, LE_FS_CREAT | LE_FS_RDONLY, &fileRef);
    TAF_ERROR_IF_RET_NIL(res != LE_OK, "Fail to open state file.");

    size_t bufSize = sizeof(taf_update_State_t);
    res = le_fs_Read(fileRef, (uint8_t*)state, &bufSize);
    if (res != LE_OK) {
        LE_ERROR("Fail to read state.");
    } else {
        LE_DEBUG("Get state: %d, size: %d bytes.", *state, bufSize);
    }
    le_fs_Close(fileRef);
}

void taf_Update::UpdateTimerTick(le_timer_Ref_t timerRef)
{
    taf_UpdateTimerContext* context = (taf_UpdateTimerContext*)le_timer_GetContextPtr(timerRef);
    context->tick++;
    LE_DEBUG("Tick : %d, State : %d.", context->tick, context->state);

    auto &tafUpdate = taf_Update::GetInstance();
    taf_update_StateInfo_t stateInfo;

    if (context->state == TAF_UPDATE_DOWNLOADING) {
        taf_update_pa_ProgressState_t pState;
        int percent, ret;

        if (context->tick > TAF_UPDATE_DOWNLOAD_TIME_OUT) {
            LE_ERROR("Download agent download time over 3 minutes.");
            le_timer_Stop(timerRef);
            context->state = TAF_UPDATE_DOWNLOAD_FAIL;
            stateInfo.state = context->state;
            le_event_Report(tafUpdate.updateStateEvId, &stateInfo, sizeof(taf_update_StateInfo_t));
        }

        ret = taf_update_pa_GetProgress(&pState, &percent);
        if (!ret) {
            switch (pState) {
                case TAF_UPDATE_PA_PROGRESS_INIT:
                    LE_INFO("Download agent progress state init.");
                    break;
                case TAF_UPDATE_PA_PROGRESS_DOWNLOADING:
                    LE_DEBUG("Download agent progress state downloading, percent = %d.", percent);
                    break;
                case TAF_UPDATE_PA_PROGRESS_ERROR:
                    LE_ERROR("Download agent progress state error.");
                    context->state = TAF_UPDATE_DOWNLOAD_FAIL;
                    break;
                case TAF_UPDATE_PA_PROGRESS_FINISH:
                    LE_INFO("Download agent progress state finish.");
                    context->state = TAF_UPDATE_DOWNLOAD_COMPLETE;
                    context->tick = 0;
                    UpdateSetState(context->state);
                    le_timer_Stop(timerRef);
                    LE_INFO("Stop download timer.");
                    break;
                default:
                    LE_ERROR("Unknown download agent progress state %d.", pState);
                    context->state = TAF_UPDATE_DOWNLOAD_FAIL;
                    break;
            }
        } else {
            context->state = TAF_UPDATE_DOWNLOAD_FAIL;
            LE_ERROR("Download agent progress failed, ret = %d.", ret);
        }
        stateInfo.state = context->state;
        stateInfo.percent = percent;
        le_event_Report(tafUpdate.updateStateEvId, &stateInfo, sizeof(taf_update_StateInfo_t));
    }

    if (context->state == TAF_UPDATE_PROBATION && context->tick > TAF_UPDATE_PROBATION_TIME) {
        le_timer_Stop(timerRef);
        context->state = TAF_UPDATE_REPORTING;
        context->tick = 0;
        LE_INFO("Probation stopped, reporting.");
        taf_update_pa_ReportState_t rState = TAF_UPDATE_PA_REPORT_SUCCESS;
        int ret = taf_update_pa_Report(tafUpdate.daSessionID, rState);
        if (ret) {
            LE_ERROR("Download agent report fail, ret = %d.", ret);
            return;
        }
        LE_INFO("Download agent report done, back to idle.");
        context->state = TAF_UPDATE_IDLE;
        UpdateSetState(context->state);
        stateInfo.state = context->state;
        le_event_Report(tafUpdate.updateStateEvId, &stateInfo, sizeof(taf_update_StateInfo_t));
    }

    if (context->state == TAF_UPDATE_DOWNLOAD_FAIL) {
        LE_INFO("Stop download timer.");
        le_timer_Stop(timerRef);
        context->tick = 0;
        UpdateSetState(TAF_UPDATE_IDLE);
        stateInfo.state = TAF_UPDATE_IDLE;
        le_event_Report(tafUpdate.updateStateEvId, &stateInfo, sizeof(taf_update_StateInfo_t));
    }

    le_timer_SetContextPtr(timerRef, context);
}

void taf_Update::FirstLayerStateHandler(void* reportPtr, void* secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == nullptr, "Null ptr(secondLayerHandlerFunc)");

    taf_update_StateHandlerFunc_t handlerFunc =
        (taf_update_StateHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc((taf_update_StateInfo_t*)reportPtr, le_event_GetContextPtr());
}

void taf_Update::UpdateProcCmdHandler(void* cmdReqPtr)
{
    taf_UpdateCmdReq_t* cmdReq = (taf_UpdateCmdReq_t*)cmdReqPtr;
    auto &tafUpdate = taf_Update::GetInstance();

    taf_update_StateInfo_t stateInfo;
    UpdateGetState(&stateInfo.state);
    switch (stateInfo.state) {
        case TAF_UPDATE_IDLE:
            if (cmdReq->cmdType == TAF_UPDATE_CMD_TYPE_ASYNC_DOWNLOAD) {
                stateInfo.state = TAF_UPDATE_DOWNLOADING;
                UpdateSetState(stateInfo.state);
                int ret = taf_update_pa_Download(tafUpdate.daSessionID);
                if (ret) {
                    LE_ERROR("Download agent download failed, ret = %d.", ret);
                    stateInfo.state = TAF_UPDATE_DOWNLOAD_FAIL;
                    le_event_Report(tafUpdate.updateStateEvId, &stateInfo, sizeof(taf_update_StateInfo_t));
                    UpdateSetState(TAF_UPDATE_IDLE);
                    stateInfo.state = TAF_UPDATE_IDLE;
                    le_event_Report(tafUpdate.updateStateEvId, &stateInfo, sizeof(taf_update_StateInfo_t));
                } else {
                    tafUpdate.dlTimerContext.state = stateInfo.state;
                    tafUpdate.dlTimerContext.tick = 0;
                    le_timer_SetContextPtr(tafUpdate.dlTimerRef, &tafUpdate.dlTimerContext);
                    LE_INFO("Start download timer.");
                    le_timer_Start(tafUpdate.dlTimerRef);
                }
            } else {
                LE_ERROR("Current state is idle, Cmd : %d not download.", cmdReq->cmdType);
            }
            break;
        case TAF_UPDATE_DOWNLOADING:
            LE_ERROR("Current state is downloading, Cmd : %d.", cmdReq->cmdType);
            break;
        case TAF_UPDATE_DOWNLOAD_COMPLETE:
            if (cmdReq->cmdType == TAF_UPDATE_CMD_TYPE_ASYNC_INSTALL) {
                stateInfo.state = TAF_UPDATE_INSTALLING;
                UpdateSetState(stateInfo.state);
                LE_INFO("Start install.");
                char instCmd[TAF_UPDATE_INSTALL_CMD_LEN];
                snprintf(instCmd, sizeof(instCmd), "recovery --update_package=%s", TAF_UPDATE_INSATLL_PAKCAGE);
                system(instCmd);
                ifstream fin(TAF_UPDATE_RECOVERY_LOG_FILE);
                string strline;
                stateInfo.state = TAF_UPDATE_REPORTING;
                int line = 0;
                while (getline(fin, strline)) {
                    line++;
                    if (!(strline.find("Starting recovery") == string::npos)) {
                        stateInfo.state = TAF_UPDATE_REPORTING;
                        LE_DEBUG("Found Starting recovery in line %d", line);
                    }

                    if (!(strline.find("upgrade success") == string::npos)) {
                        stateInfo.state = TAF_UPDATE_INSTALL_SUCCESS;
                        LE_DEBUG("Found upgrade success in line %d", line);
                    }
                }
                fin.close();

                if (stateInfo.state == TAF_UPDATE_REPORTING) {
                    LE_ERROR("Install failed.");
                } else {
                    LE_INFO("Install success.");
                }
                UpdateSetState(stateInfo.state);
                le_event_Report(tafUpdate.updateStateEvId, &stateInfo, sizeof(taf_update_StateInfo_t));
            } else {
                LE_ERROR("Current state is download complete, Cmd : %d not install.", cmdReq->cmdType);
            }
            break;
        case TAF_UPDATE_INSTALLING:
            LE_ERROR("Current state is installing, Cmd : %d.", cmdReq->cmdType);
            break;
        case TAF_UPDATE_INSTALL_SUCCESS:
            LE_ERROR("Current state is install success, please reboot to active, Cmd : %d.", cmdReq->cmdType);
            break;
        case TAF_UPDATE_PROBATION:
            LE_ERROR("Current state is probation, Cmd : %d.", cmdReq->cmdType);
            break;
        case TAF_UPDATE_RECOVERY:
            LE_ERROR("Current state is recovery, Cmd : %d.", cmdReq->cmdType);
            break;
        case TAF_UPDATE_REPORTING:
            LE_ERROR("Current state is reporting, Cmd : %d.", cmdReq->cmdType);
            break;
        default:
            LE_ERROR("Invalid state : %d.", stateInfo.state);
            UpdateSetState(TAF_UPDATE_IDLE);
            break;
    }
}

void* taf_Update::UpdateCmdThread(void* contextPtr)
{
    le_event_AddHandler("UpdateProcCmdHandler", updateCmdEvId, UpdateProcCmdHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return nullptr;
}

void taf_Update::onInitCompleted(telux::common::ServiceStatus status)
{
    std::lock_guard<std::mutex> lock(mtx);
    subSystemStatusUpdated = true;
    conVar.notify_all();
}

void taf_Update::Init(void)
{
    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    // 1. Set up data call.
    updateServingSystemlisteners[SlotId::DEFAULT_SLOT_ID] = std::make_shared<taf_UpdateServingSystemListener>(SlotId::DEFAULT_SLOT_ID);
    dataServingSystemListeners[SlotId::DEFAULT_SLOT_ID] = updateServingSystemlisteners[SlotId::DEFAULT_SLOT_ID];

    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated = false;
    auto initCb = std::bind(&taf_Update::onInitCompleted, this, std::placeholders::_1);
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto servingSystemMgr = dataFactory.getServingSystemManager(SlotId::DEFAULT_SLOT_ID, initCb);
    bool subSysReady = false;
    if (servingSystemMgr) {
        std::unique_lock<std::mutex> uLock(mtx);
        conVar.wait(uLock, [this]{return this->subSystemStatusUpdated;});
        subSystemStatus = servingSystemMgr->getServiceStatus();

        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LE_INFO("Serving system manager on slot %d is ready.", (int)SlotId::DEFAULT_SLOT_ID);
            subSysReady = true;
        } else {
            LE_ERROR("Serving system manager on slot %d is not ready.", (int)SlotId::DEFAULT_SLOT_ID);
            //If manager exist, deregister and remove it
            if (dataServingSystemManagers.find(SlotId::DEFAULT_SLOT_ID) != dataServingSystemManagers.end()) {
                dataServingSystemManagers[SlotId::DEFAULT_SLOT_ID]->deregisterListener(dataServingSystemListeners[SlotId::DEFAULT_SLOT_ID]);
                dataServingSystemManagers.erase(SlotId::DEFAULT_SLOT_ID);
            }
            subSysReady = false;
        }

        //If it is new manager and initialization passed
        if (subSysReady && (dataServingSystemManagers.find(SlotId::DEFAULT_SLOT_ID) == dataServingSystemManagers.end())) {
            dataServingSystemManagers.emplace(SlotId::DEFAULT_SLOT_ID, servingSystemMgr);
            dataServingSystemManagers[SlotId::DEFAULT_SLOT_ID]->registerListener(dataServingSystemListeners[SlotId::DEFAULT_SLOT_ID]);
        }
    }

    reqSvcStateCb = std::make_shared<taf_UpdateRequestServiceStatusCallback>();
    reqSvcStateCb->semaphore = le_sem_Create("taf_UpdateReqSvcStateCbSem", 0);
    auto reqSvcStateCbFunc = std::bind(&taf_UpdateRequestServiceStatusCallback::requestServiceStatus, reqSvcStateCb, std::placeholders::_1, std::placeholders::_2);

    telux::common::Status status = dataServingSystemManagers[SlotId::DEFAULT_SLOT_ID]->requestServiceStatus(reqSvcStateCbFunc);
    TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS, "Call sdk function failed.");

    le_clk_Time_t timeToWait = {1, 0};
    le_result_t res = le_sem_WaitWithTimeOut(reqSvcStateCb->semaphore, timeToWait);
    TAF_ERROR_IF_RET_NIL(res != LE_OK, "Wait semaphore timeout.");
    LE_INFO("Current data service status: %d.", (int)reqSvcStateCb->status.serviceState);
    if (reqSvcStateCb->status.serviceState != telux::data::DataServiceState::IN_SERVICE) {
        std::unique_lock<std::mutex> uLock(updateServingSystemlisteners[SlotId::DEFAULT_SLOT_ID]->cv_mutex);
        updateServingSystemlisteners[SlotId::DEFAULT_SLOT_ID]->conVar.wait_for(uLock,
            std::chrono::seconds(TAF_UPDATE_DATA_SERVICE_TIME_OUT));
        if (updateServingSystemlisteners[SlotId::DEFAULT_SLOT_ID]->dsStatus != telux::data::DataServiceState::IN_SERVICE) {
            LE_ERROR("Wait for data in service time out.");
        }
    }

    uint32_t profileId = taf_dcs_GetDefaultProfileIndex();
    taf_dcs_ProfileRef_t profileRef = taf_dcs_GetProfile(profileId);
    if (taf_dcs_StartSession(profileRef) != LE_OK) {
        LE_ERROR("Fail to set up data call.");
    }

    daSessionID = taf_update_pa_GetSession();
    if (daSessionID == nullptr) {
        LE_ERROR("Session ID is null.");
    }

    // 2. Create event to report state.
    updateStateEvId = le_event_CreateId("updateState", sizeof(taf_update_StateInfo_t));

    // 3. Check state file.
    if (!le_fs_Exists(TAF_UPDATE_STATE_FILE)) {
        UpdateSetState(TAF_UPDATE_IDLE);
    } else {
        taf_update_State_t state;
        UpdateGetState(&state);
        if (state == TAF_UPDATE_INSTALL_SUCCESS) {
            // 2.1 Probation start.
            LE_INFO("Install success after reboot.");
            state = TAF_UPDATE_PROBATION;
            UpdateSetState(state);
            prbtTimerRef = le_timer_Create("Probation Timer");
            le_timer_SetMsInterval(prbtTimerRef, TAF_UPDATE_TIME_INTERVAL);
            le_timer_SetRepeat(prbtTimerRef, 0);
            le_timer_SetHandler(prbtTimerRef, UpdateTimerTick);
            prbtTimerContext.state = state;
            prbtTimerContext.tick = 0;
            le_timer_SetContextPtr(prbtTimerRef, &prbtTimerContext);
            LE_INFO("Start probation timer.");
            le_timer_Start(prbtTimerRef);
        } else if (state != TAF_UPDATE_IDLE) {
            LE_WARN("Wrong state %d.", state);
            UpdateSetState(TAF_UPDATE_IDLE);
        }
    }

    // 4. Create command thread.
    le_sem_Ref_t semaphore = le_sem_Create("updateCmdThreadSem", 0);
    updateCmdEvId = le_event_CreateId("updateCmd", sizeof(taf_UpdateCmdReq_t));
    le_thread_Ref_t threadRef = le_thread_Create("updateCmdThread", UpdateCmdThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, TAF_UPDATE_THREAD_STACK_SIZE);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 5. Create download timer
    dlTimerRef = le_timer_Create("Download Timer");
    le_timer_SetMsInterval(dlTimerRef, TAF_UPDATE_TIME_INTERVAL);
    le_timer_SetRepeat(dlTimerRef, 0);
    le_timer_SetHandler(dlTimerRef, UpdateTimerTick);

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_INFO("Elapsed time for update service: %lfs.", elapsedTime.count());
}
