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

/*======================================================================
 FUNCTION        taf_UpdateServingSystemListener::taf_UpdateServingSystemListener
 DESCRIPTION     Class constructor of taf_UpdateServingSystemListener
 PARAMETERS      [IN] slot: Slot index
 RETURN VALUE    void
======================================================================*/
taf_UpdateServingSystemListener::taf_UpdateServingSystemListener(SlotId slot) :
   slotId(slot) {
}

/*======================================================================
 FUNCTION        taf_UpdateServingSystemListener::onServiceStateChanged
 DESCRIPTION     Listener for data service state
 PARAMETERS      [IN] status: Data service status
 RETURN VALUE    void
======================================================================*/
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

/*======================================================================
 FUNCTION        taf_UpdateRequestServiceStatusCallback::requestServiceStatus
 DESCRIPTION     Call back function for requesting data service status
 PARAMETERS      [IN] serviceStatus: Data service status
                 [IN] error: Error of requesting data service status
 RETURN VALUE    void
======================================================================*/
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

/*======================================================================
 FUNCTION        taf_Update::GetInstance
 DESCRIPTION     Get the instance of taf_Update
 PARAMETERS      void
 RETURN VALUE    taf_Update: Instance reference
======================================================================*/
taf_Update &taf_Update::GetInstance()
{
    static taf_Update instance;
    return instance;
}

/*======================================================================
 FUNCTION        taf_Update::CheckHeader
 DESCRIPTION     Check each byte of header
 PARAMETERS      [IN] src: Source string
                 [IN] dst: Target string
                 [IN] n: Byte number for comparation
 RETURN VALUE    bool: If src have same n bytes as dst
======================================================================*/
bool taf_Update::CheckHeader(const char* src, const char* dst, int n)
{
    if (src == NULL || dst == NULL || n <=0 ) {
        return false;
    }

    for (int i = 0; i < n; i++) {
        if (src[i] != dst[i])
            return false;
    }

    return true;
}

/*======================================================================
 FUNCTION        taf_Update::ParsePackage
 DESCRIPTION     Parse QOTA header of a file
 PARAMETERS      [IN] file: File path
 RETURN VALUE    le_result_t: Result of parsing header
======================================================================*/
le_result_t taf_Update::ParsePackage(const char* file)
{
    TAF_ERROR_IF_RET_VAL(access(file, 0), LE_NOT_FOUND,
        "%s not found", file);

    // 1. Get the file size.
    ifstream infile(file);
    infile.seekg (0, ios::end);
    long size = infile.tellg();
    infile.seekg (0);

    LE_INFO("%s is %ld bytes.", file, size);

    TAF_ERROR_IF_RET_VAL(size <= TAF_UPDATE_QOTA_HEADER_SIZE, LE_FAULT,
        "%s less than %d bytes.", file, TAF_UPDATE_QOTA_HEADER_SIZE);

    // 2. Parse and restore QOTA header.
    taf_UpdateQotaHeaderSeg_t qotaHeader[TAF_UPDATE_QOTA_HEADER_SEG_NUM] = {
        {"magic", 4},
        {"version", 2},
        {"vendor", 8},
        {"license", 8},
        {"compat", 2},
        {"payload_type", 2},
        {"payload_length", 4},
        {"auth_type", 2},
        {"auth_offset", 4},
        {"encryption", 2},
        {"res1", 2},
        {"res2", 4},
        {"crc", 4},
    };
    const char qotaMagic[] = {0x00, 0x01, 0x02, 0x03};
    const char qotaSota[] = {0x00, 0x01};
    const char qotaFota[] = {0x00, 0x02};

    char* buffer = new char[size];
    infile.read (buffer, size);

    auto &tafUpdate = taf_Update::GetInstance();
    tafUpdate.qotaHeader.clear();

    size_t offset = 0;
    for (int i = 0; i < TAF_UPDATE_QOTA_HEADER_SEG_NUM; i++) {
        if (tafUpdate.qotaHeader[qotaHeader[i].name] != NULL)
            free(tafUpdate.qotaHeader[qotaHeader[i].name]);
        tafUpdate.qotaHeader[qotaHeader[i].name] = (char*)malloc(qotaHeader[i].size);
        memcpy(tafUpdate.qotaHeader[qotaHeader[i].name], buffer + offset, qotaHeader[i].size);
        offset += qotaHeader[i].size;
    }

    // 3. Check magic.
    if(!tafUpdate.CheckHeader(tafUpdate.qotaHeader["magic"], qotaMagic, sizeof(qotaMagic))) {
        tafUpdate.pkgType = TAF_UPDATE_PACKAGE_NON_QOTA;
        LE_ERROR("%s in not a QOTA package.", file);
    }

    // 4. Check payload type.
    if (tafUpdate.CheckHeader(tafUpdate.qotaHeader["payload_type"], qotaSota, sizeof(qotaSota))) {
        tafUpdate.pkgType = TAF_UPDATE_PACKAGE_SOTA;
        LE_INFO("%s is a package for application update.", file);
    } else if (tafUpdate.CheckHeader(tafUpdate.qotaHeader["payload_type"], qotaFota, sizeof(qotaFota))) {
        LE_INFO("%s is a package for firmware update.", file);
        tafUpdate.pkgType = TAF_UPDATE_PACKAGE_FOTA;
    } else {
        tafUpdate.pkgType = TAF_UPDATE_PACKAGE_NON_QOTA;
        LE_ERROR("%s is not for SOTA or FOTA.", file);
    }

    delete[] buffer;
    infile.close();

    return LE_OK;
}

/*======================================================================
 FUNCTION        taf_Update::RemoveHeader
 DESCRIPTION     Remove QOTA header of a file
 PARAMETERS      [IN] file: File path
 RETURN VALUE    le_result_t: Result of removing header
======================================================================*/
le_result_t taf_Update::RemoveHeader(const char* file)
{
    TAF_ERROR_IF_RET_VAL(access(file, 0), LE_NOT_FOUND,
        "%s not found", file);

    int fd = open(file, O_RDWR);
    TAF_ERROR_IF_RET_VAL(fd < 0, LE_FAULT, "Open %s failed.", file);

    long size = lseek(fd, 0, SEEK_END);
    LE_INFO("%s is %ld bytes.", file, size);

    char* buffer = new char[TAF_UPDATE_RW_BUFFER_SIZE];
    TAF_ERROR_IF_RET_VAL(buffer == nullptr, LE_FAULT, "Alloc buffer failed.");

    long pos = TAF_UPDATE_QOTA_HEADER_SIZE;
    long rdSize;

    while (pos < size) {
        lseek(fd, pos, SEEK_SET);
        rdSize = read(fd, buffer, TAF_UPDATE_RW_BUFFER_SIZE);
        TAF_ERROR_IF_RET_VAL(rdSize <= 0, LE_FAULT, "Read buffer error, return size: %ld.", rdSize);
        lseek(fd, pos - TAF_UPDATE_QOTA_HEADER_SIZE, SEEK_SET);
        write(fd, buffer, rdSize);
        pos += rdSize;
    }

    ftruncate(fd, size - TAF_UPDATE_QOTA_HEADER_SIZE);

    delete[] buffer;
    close(fd);

    LE_INFO("QOTA header removed.");
    return LE_OK;
}

/*======================================================================
 FUNCTION        taf_Update::NameEventHandler
 DESCRIPTION     Handler for json event when parsing the name string
 PARAMETERS      [IN] event: Json parsing event
 RETURN VALUE    void
======================================================================*/
void taf_Update::NameEventHandler(le_json_Event_t event)
{
    TAF_ERROR_IF_RET_NIL(event != LE_JSON_STRING, "Not a string.");
    auto &tafUpdate = taf_Update::GetInstance();
    le_utf8_Copy(tafUpdate.sotaAppName, le_json_GetString(), TAF_APPMGMT_APP_NAME_BYTES, NULL);
    LE_INFO("App name is %s.", tafUpdate.sotaAppName);
}

/*======================================================================
 FUNCTION        taf_Update::JsonEventHandler
 DESCRIPTION     Handler for json parsing event
 PARAMETERS      [IN] event: Json parsing event
 RETURN VALUE    void
======================================================================*/
void taf_Update::JsonEventHandler(le_json_Event_t event)
{
    const char* nameStr;
    le_json_ParsingSessionRef_t sessRef;
    taf_update_StateInd_t stateInd;
    auto &tafUpdate = taf_Update::GetInstance();

    switch (event) {
        case LE_JSON_OBJECT_START:
            LE_INFO("Start json object parsing.");
            break;
        case LE_JSON_OBJECT_END:
            LE_INFO("Complete json object parsing.");
            break;
        case LE_JSON_DOC_END:
            LE_INFO("Complete json doc parsing.");

            // Clean up json parsing session.
            sessRef = le_json_GetSession();
            le_json_Cleanup(sessRef);

            tafUpdate.UpdateSetState(TAF_UPDATE_DOWNLOAD_SUCCESS);
            stateInd.pkgType = TAF_UPDATE_PACKAGE_SOTA;
            stateInd.state = TAF_UPDATE_DOWNLOAD_SUCCESS;
            le_utf8_Copy(stateInd.pkgName, tafUpdate.sotaAppName, TAF_APPMGMT_APP_NAME_BYTES, NULL);
            le_event_Report(tafUpdate.updateStateEvId, &stateInd, sizeof(taf_update_StateInd_t));
            close(tafUpdate.sotaFd);
            break;
        case LE_JSON_OBJECT_MEMBER:
            nameStr = le_json_GetString();
            if (strcmp(nameStr, "name") == 0)
                le_json_SetEventHandler(NameEventHandler);
            break;
        case LE_JSON_NUMBER:
        case LE_JSON_STRING:
        case LE_JSON_ARRAY_START:
        case LE_JSON_ARRAY_END:
        case LE_JSON_TRUE:
        case LE_JSON_FALSE:
        case LE_JSON_NULL:
        default:
            LE_WARN("Warning event : %s.", le_json_GetEventName(event));
            break;
    }
}

/*======================================================================
 FUNCTION        taf_Update::JsonErrorHandler
 DESCRIPTION     Handler for json parsing error
 PARAMETERS      [IN] error: Json parsing error
                 [IN] msg: Message of error
 RETURN VALUE    void
======================================================================*/
void taf_Update::JsonErrorHandler(le_json_Error_t error, const char* msg)
{
    LE_ERROR("Json error: %d.", (int)error);
    if ((error == LE_JSON_SYNTAX_ERROR) || (error == LE_JSON_READ_ERROR)) {
        LE_ERROR("Json error messgae: %s.", msg);
    }
}

/*======================================================================
 FUNCTION        taf_Update::ParseBundle
 DESCRIPTION     Parse app bundle
 PARAMETERS      [IN] file: App bundle file path
 RETURN VALUE    le_result_t: Result of parsing
======================================================================*/
le_result_t taf_Update::ParseBundle(const char* file)
{
    TAF_ERROR_IF_RET_VAL(access(file, 0), LE_NOT_FOUND,
        "%s not found", file);

    auto &tafUpdate = taf_Update::GetInstance();
    tafUpdate.sotaFd = open(file, O_RDONLY);
    TAF_ERROR_IF_RET_VAL(tafUpdate.sotaFd < 0, LE_FAULT, "Open %s failed.", file);

    (void)le_json_Parse(tafUpdate.sotaFd, JsonEventHandler, JsonErrorHandler, NULL);

    return LE_OK;
}

/*======================================================================
 FUNCTION        taf_Update::UpdateSetState
 DESCRIPTION     Set current update state
 PARAMETERS      [IN] state: Update state
 RETURN VALUE    void
======================================================================*/
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

/*======================================================================
 FUNCTION        taf_Update::UpdateGetState
 DESCRIPTION     Get current update state
 PARAMETERS      [OUT] state: Update state
 RETURN VALUE    void
======================================================================*/
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

/*======================================================================
 FUNCTION        taf_Update::DownloadTimerTick
 DESCRIPTION     Handler for download period
 PARAMETERS      [IN] timerRef: Timer reference
 RETURN VALUE    void
======================================================================*/
void taf_Update::DownloadTimerTick(le_timer_Ref_t timerRef)
{
    taf_UpdateTimerContext* context = (taf_UpdateTimerContext*)le_timer_GetContextPtr(timerRef);
    context->tick++;
    LE_DEBUG("Tick : %d, State : %d.", context->tick, context->state);

    auto &tafUpdate = taf_Update::GetInstance();
    taf_update_StateInd_t stateInd;
    taf_update_pa_ProgressState_t pState;
    int percent, ret;

    stateInd.state = context->state;
    switch (context->state) {
        case TAF_UPDATE_DOWNLOADING:
            ret = taf_update_pa_GetProgress(&pState, &percent);
            if (ret) {
                LE_ERROR("Download agent progress failed, ret = %d.", ret);
                context->state = TAF_UPDATE_DOWNLOAD_FAIL;
            } else {
                switch (pState) {
                     case TAF_UPDATE_PA_PROGRESS_INIT:
                         LE_INFO("Download agent progress state init.");
                         break;
                     case TAF_UPDATE_PA_PROGRESS_DOWNLOADING:
                         LE_INFO("Download agent progress state downloading, percent = %d.", percent);
                         break;
                     case TAF_UPDATE_PA_PROGRESS_ERROR:
                         LE_ERROR("Download agent progress state error.");
                         context->state = TAF_UPDATE_DOWNLOAD_FAIL;
                         break;
                     case TAF_UPDATE_PA_PROGRESS_FINISH:
                         LE_INFO("Download agent progress state finish.");
                         context->state = TAF_UPDATE_DOWNLOAD_SUCCESS;
                         break;
                     default:
                         LE_ERROR("Unknown download agent progress state %d.", pState);
                         context->state = TAF_UPDATE_DOWNLOAD_FAIL;
                         break;
                }
            }
            break;
        case TAF_UPDATE_DOWNLOAD_SUCCESS:
            LE_INFO("Stop timer with download success.");
            le_timer_Stop(timerRef);
            LE_INFO("Parsing download package.");
            TAF_ERROR_IF_RET_NIL(tafUpdate.ParsePackage(TAF_UPDATE_PAKCAGE_FILE_PATH) != LE_OK, "Fail to parse download package.");
            TAF_ERROR_IF_RET_NIL(tafUpdate.RemoveHeader(TAF_UPDATE_PAKCAGE_FILE_PATH) != LE_OK, "Fail to remove QOTA header.");
            stateInd.pkgType = tafUpdate.pkgType;
            if (stateInd.pkgType == TAF_UPDATE_PACKAGE_SOTA) {
                TAF_ERROR_IF_RET_NIL(tafUpdate.ParseBundle(TAF_UPDATE_PAKCAGE_FILE_PATH) != LE_OK, "Fail to parse app bundle.");
            } else {
                tafUpdate.UpdateSetState(TAF_UPDATE_DOWNLOAD_SUCCESS);
                le_event_Report(tafUpdate.updateStateEvId, &stateInd, sizeof(taf_update_StateInd_t));
            }
            break;
        case TAF_UPDATE_DOWNLOAD_FAIL:
        default:
            LE_ERROR("Stop timer with download failure.");
            le_timer_Stop(timerRef);
            tafUpdate.UpdateSetState(TAF_UPDATE_IDLE);
            break;
    }

    le_timer_SetContextPtr(timerRef, context);
    if (context->state != TAF_UPDATE_DOWNLOAD_SUCCESS) {
        stateInd.percent = percent;
        le_event_Report(tafUpdate.updateStateEvId, &stateInd, sizeof(taf_update_StateInd_t));
    }
}

/*======================================================================
 FUNCTION        taf_Update::ProbationTimerTick
 DESCRIPTION     Handler for probation period
 PARAMETERS      [IN] timerRef: Timer reference
 RETURN VALUE    void
======================================================================*/
void taf_Update::ProbationTimerTick(le_timer_Ref_t timerRef)
{
    taf_UpdateTimerContext* context = (taf_UpdateTimerContext*)le_timer_GetContextPtr(timerRef);
    context->tick++;
    LE_DEBUG("Tick : %d, State : %d.", context->tick, context->state);

    if (context->tick > TAF_UPDATE_PROBATION_TIME) {
        LE_INFO("Probation timer stopped, reporting.");
        le_timer_Stop(timerRef);
        if (context->pkgType == TAF_UPDATE_PACKAGE_SOTA) {
            LE_INFO("Mark good for application.");
            (void)le_updateCtrl_MarkGood(true);
        } else {
#ifdef TARGET_SA515M
            TAF_ERROR_IF_RET_NIL(taf_mrc_SendOtaAbsyncMsg() != LE_OK,
                "Fail to send OTA AB Sync message to MRC daemon.");
#endif
        }
        auto &tafUpdate = taf_Update::GetInstance();
        taf_update_pa_ReportState_t rState = TAF_UPDATE_PA_REPORT_SUCCESS;
        int ret = taf_update_pa_Report(tafUpdate.daSessionID, rState);
        TAF_ERROR_IF_RET_NIL(ret, "Download agent report fail, ret = %d.", ret);
        LE_INFO("Report done, back to idle.");

        tafUpdate.UpdateSetState(TAF_UPDATE_IDLE);
        taf_update_StateInd_t stateInd;
        stateInd.state = TAF_UPDATE_IDLE;
        le_event_Report(tafUpdate.updateStateEvId, &stateInd, sizeof(taf_update_StateInd_t));
    }
    le_timer_SetContextPtr(timerRef, context);
}

/*======================================================================
 FUNCTION        taf_Update::AppInstallHandler
 DESCRIPTION     Layered handler for update state
 PARAMETERS      [IN] state: State of installation
                 [IN] percent: Percent of the installation
                 [IN] contextPtr: Context of installation
 RETURN VALUE    void
======================================================================*/
void taf_Update::AppInstallHandler(le_update_State_t state, uint percent, void* contextPtr)
{
    auto &tafUpdate = taf_Update::GetInstance();
    taf_update_StateInd_t stateInd;
    stateInd.pkgType = TAF_UPDATE_PACKAGE_SOTA;

    switch (state) {
        case LE_UPDATE_STATE_DOWNLOAD_SUCCESS:
            LE_INFO("Install init.");
            le_update_Install();
            stateInd.state = TAF_UPDATE_INSTALLING;
            stateInd.percent = percent / 4 + 25;
            break;
        case LE_UPDATE_STATE_UNPACKING:
            LE_INFO("Unpaking %d%%...", percent);
            stateInd.state = TAF_UPDATE_INSTALLING;
            stateInd.percent = percent / 4;
            break;
        case LE_UPDATE_STATE_APPLYING:
            LE_INFO("Applying ...");
            stateInd.state = TAF_UPDATE_INSTALLING;
            stateInd.percent = percent / 4 + 50;
            break;
        case LE_UPDATE_STATE_SUCCESS:
            LE_INFO("Install success.");
            le_update_End();
            stateInd.percent = 100;
            stateInd.state = TAF_UPDATE_INSTALL_SUCCESS;
            tafUpdate.UpdateSetState(TAF_UPDATE_INSTALL_SUCCESS);
            break;
        case LE_UPDATE_STATE_FAILED:
        default:
            LE_ERROR("Install failed.");
            le_update_End();
            stateInd.percent = 0;
            stateInd.state = TAF_UPDATE_INSTALL_FAIL;
            stateInd.error = (taf_update_InstallError_t)le_update_GetErrorCode();
            tafUpdate.UpdateSetState(TAF_UPDATE_IDLE);
            break;
    }

    le_utf8_Copy(stateInd.pkgName, tafUpdate.sotaAppName, TAF_APPMGMT_APP_NAME_BYTES, NULL);
    le_event_Report(tafUpdate.updateStateEvId, &stateInd, sizeof(taf_update_StateInd_t));
}

/*======================================================================
 FUNCTION        taf_Update::UpdateStateLayeredHandler
 DESCRIPTION     Layered handler for update state
 PARAMETERS      [IN] reportPtr: Report content
                 [IN] secondLayerHandlerFunc: Layered function of handler
 RETURN VALUE    void
======================================================================*/
void taf_Update::UpdateStateLayeredHandler(void* reportPtr, void* secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == nullptr, "Null ptr(secondLayerHandlerFunc)");

    taf_update_StateHandlerFunc_t handlerFunc =
        (taf_update_StateHandlerFunc_t)secondLayerHandlerFunc;

    handlerFunc((taf_update_StateInd_t*)reportPtr, le_event_GetContextPtr());
}

/*======================================================================
 FUNCTION        taf_Update::UpdateProcCmdHandler
 DESCRIPTION     Handler of update command thread
 PARAMETERS      [IN] cmdReqPtr: Update command
 RETURN VALUE    void
======================================================================*/
void taf_Update::UpdateProcCmdHandler(void* cmdReqPtr)
{
    taf_UpdateCmdReq_t* cmdReq = (taf_UpdateCmdReq_t*)cmdReqPtr;
    taf_update_StateInd_t stateInd;
    auto &tafUpdate = taf_Update::GetInstance();
    tafUpdate.UpdateGetState(&stateInd.state);

    switch (cmdReq->cmdType) {
        case TAF_UPDATE_CMD_TYPE_DOWNLOAD:
            if (stateInd.state == TAF_UPDATE_IDLE) {
                int ret = taf_update_pa_Download(tafUpdate.daSessionID);
                if (ret) {
                    LE_ERROR("Download agent download failed, ret = %d.", ret);
                    stateInd.state = TAF_UPDATE_DOWNLOAD_FAIL;
                    tafUpdate.UpdateSetState(TAF_UPDATE_IDLE);
                    le_event_Report(tafUpdate.updateStateEvId, &stateInd, sizeof(taf_update_StateInd_t));
                } else {
                    tafUpdate.UpdateSetState(TAF_UPDATE_DOWNLOADING);
                    tafUpdate.dlTimerContext.state = TAF_UPDATE_DOWNLOADING;
                    tafUpdate.dlTimerContext.tick = 0;
                    le_timer_SetContextPtr(tafUpdate.dlTimerRef, &tafUpdate.dlTimerContext);
                    LE_INFO("Start download timer.");
                    le_timer_Start(tafUpdate.dlTimerRef);
                }
            } else {
                LE_ERROR("Can not download, current state : %d.", stateInd.state);
            }
            break;
        case TAF_UPDATE_CMD_TYPE_INSTALL:
            if (stateInd.state == TAF_UPDATE_DOWNLOAD_SUCCESS) {
                stateInd.state = TAF_UPDATE_INSTALLING;
                tafUpdate.UpdateSetState(stateInd.state);
                if (tafUpdate.pkgType == TAF_UPDATE_PACKAGE_FOTA) {
                    auto &tafFwUpdate = taf_FwUpdate::GetInstance();
                    if (tafFwUpdate.Install(cmdReq->pkgName) != TAF_FWUPDATE_ERROR_NONE) {
                        stateInd.state = TAF_UPDATE_INSTALL_FAIL;
                        tafUpdate.UpdateSetState(TAF_UPDATE_IDLE);
                    } else {
                        stateInd.state = TAF_UPDATE_INSTALL_SUCCESS;
                        tafUpdate.UpdateSetState(TAF_UPDATE_INSTALL_SUCCESS);
                    }
                    stateInd.pkgType = TAF_UPDATE_PACKAGE_FOTA;
                    le_event_Report(tafUpdate.updateStateEvId, &stateInd, sizeof(taf_update_StateInd_t));
                } else {
                    int fd = open(cmdReq->pkgName, O_RDONLY);
                    le_result_t result = le_update_Start(fd);
                    if (result != LE_OK) {
                        le_update_End();
                        stateInd.state = TAF_UPDATE_INSTALL_FAIL;
                        tafUpdate.UpdateSetState(TAF_UPDATE_IDLE);
                        le_event_Report(tafUpdate.updateStateEvId, &stateInd, sizeof(taf_update_StateInd_t));
                    }
                }
            } else {
                LE_ERROR("Can not install, current state : %d.", stateInd.state);
            }
            break;
        default:
            LE_ERROR("Unknown cmd %d.", cmdReq->cmdType);
    }
}

/*======================================================================
 FUNCTION        taf_Update::UpdateCmdThread
 DESCRIPTION     A thread for handling aync update command
 PARAMETERS      [IN] contextPtr: Context of the calling thread
 RETURN VALUE    void*: NULL
======================================================================*/
void* taf_Update::UpdateCmdThread(void* contextPtr)
{
#ifdef TARGET_SA515M
    taf_mrc_ConnectService();
#endif
    le_update_ConnectService();

    // Regster app install handler.
    le_update_AddProgressHandler(AppInstallHandler, NULL);

    le_event_AddHandler("UpdateProcCmdHandler", updateCmdEvId, UpdateProcCmdHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return NULL;
}

/*======================================================================
 FUNCTION        taf_Update::onInitCompleted
 DESCRIPTION     Call back function for data serving system initialization
 PARAMETERS      [IN] status: Data call service status
 RETURN VALUE    void
======================================================================*/
void taf_Update::onInitCompleted(telux::common::ServiceStatus status)
{
    std::lock_guard<std::mutex> lock(mtx);
    subSystemStatusUpdated = true;
    conVar.notify_all();
}

/*======================================================================
 FUNCTION        taf_Update::Init
 DESCRIPTION     Initialization of update service
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void taf_Update::Init(void)
{
    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();

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
    updateStateEvId = le_event_CreateId("updateState", sizeof(taf_update_StateInd_t));

    // 3. Create command thread.
    le_sem_Ref_t semaphore = le_sem_Create("updateCmdThreadSem", 0);
    updateCmdEvId = le_event_CreateId("updateCmd", sizeof(taf_UpdateCmdReq_t));
    le_thread_Ref_t threadRef = le_thread_Create("updateCmdThread", UpdateCmdThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, TAF_UPDATE_THREAD_STACK_SIZE);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 4. Create download timer.
    dlTimerRef = le_timer_Create("Download Timer");
    le_timer_SetMsInterval(dlTimerRef, TAF_UPDATE_TIME_INTERVAL);
    le_timer_SetRepeat(dlTimerRef, 0);
    le_timer_SetHandler(dlTimerRef, DownloadTimerTick);

    // 5. Create probation timer.
    prbtTimerRef = le_timer_Create("Probation Timer");
    le_timer_SetMsInterval(prbtTimerRef, TAF_UPDATE_TIME_INTERVAL);
    le_timer_SetRepeat(prbtTimerRef, 0);
    le_timer_SetHandler(prbtTimerRef, ProbationTimerTick);

    if (!le_fs_Exists(TAF_UPDATE_STATE_FILE)) {
        UpdateSetState(TAF_UPDATE_IDLE);
    } else {
        taf_update_State_t state;
        UpdateGetState(&state);
        if (state == TAF_UPDATE_INSTALL_SUCCESS) {
            // Probation start.
            LE_INFO("Install success after reboot.");
            state = TAF_UPDATE_PROBATION;
            UpdateSetState(state);
            prbtTimerContext.tick = 0;
            prbtTimerContext.pkgType = TAF_UPDATE_PACKAGE_FOTA;
            le_timer_SetContextPtr(prbtTimerRef, &prbtTimerContext);
            LE_INFO("Start probation timer for firmware.");
            le_timer_Start(prbtTimerRef);
        } else if (state != TAF_UPDATE_IDLE) {
            LE_WARN("Wrong state %d.", state);
            UpdateSetState(TAF_UPDATE_IDLE);
        }
    }

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_INFO("Elapsed time for update service: %lfs.", elapsedTime.count());
}
