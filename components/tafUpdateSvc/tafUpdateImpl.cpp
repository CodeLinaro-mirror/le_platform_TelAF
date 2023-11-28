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
 * @brief      This file implements update component.
 */

#include <chrono>
#include <fstream>

#include "tafUpdate.hpp"
#include "tafAppMgmt.hpp"
#include "tafFwUpdate.hpp"

using namespace std;
using namespace telux::tafsvc;

le_event_Id_t taf_Update::requestEvId = nullptr;

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
 FUNCTION        taf_Update::ParseHeader
 DESCRIPTION     Parse QOTA header of a file
 PARAMETERS      [IN] file: File path
 RETURN VALUE    le_result_t: Result of parsing header
======================================================================*/
le_result_t taf_Update::ParseHeader(const char* file, taf_update_OTA_t* ota)
{
    int i;
    for (i = 0; i < TAF_UPDATE_TIME_TO_ACCESS_FILE; i++) {
        if (access(file, 0) == 0) {
            break;
        }
        le_thread_Sleep(1);
    }

    TAF_ERROR_IF_RET_VAL(i == TAF_UPDATE_TIME_TO_ACCESS_FILE, LE_NOT_FOUND,
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

    char* buffer = new char[TAF_UPDATE_QOTA_HEADER_SIZE];
    infile.read (buffer, TAF_UPDATE_QOTA_HEADER_SIZE);

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
        *ota = TAF_UPDATE_NON_QOTA;
        LE_ERROR("%s in not a QOTA package.", file);
    }

    // 4. Check payload type.
    if (tafUpdate.CheckHeader(tafUpdate.qotaHeader["payload_type"], qotaSota, sizeof(qotaSota))) {
        *ota = TAF_UPDATE_SOTA;
        LE_INFO("%s is a package for application update.", file);
    } else if (tafUpdate.CheckHeader(tafUpdate.qotaHeader["payload_type"], qotaFota, sizeof(qotaFota))) {
        *ota = TAF_UPDATE_FOTA;
        LE_INFO("%s is a package for firmware update.", file);
    } else {
        *ota = TAF_UPDATE_NON_QOTA;
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
    int fd = open(file, O_RDWR);
    TAF_ERROR_IF_RET_VAL(fd < 0, LE_FAULT, "Open %s failed.", file);

    long size = lseek(fd, 0, SEEK_END);

    char* buffer = new char[TAF_UPDATE_RW_BUFFER_SIZE];
    TAF_ERROR_IF_RET_VAL(buffer == nullptr, LE_FAULT, "Alloc buffer failed.");

    long pos = TAF_UPDATE_QOTA_HEADER_SIZE;
    long rdSize;

    le_result_t ret = LE_OK;
    while (pos < size) {
        lseek(fd, pos, SEEK_SET);
        rdSize = read(fd, buffer, TAF_UPDATE_RW_BUFFER_SIZE);
        if (rdSize <= 0) {
            LE_ERROR("Read buffer error, return size: %ld.", rdSize);
            ret = LE_FAULT;
            break;
        }
        lseek(fd, pos - TAF_UPDATE_QOTA_HEADER_SIZE, SEEK_SET);
        write(fd, buffer, rdSize);
        pos += rdSize;
    }

    if (ret == LE_OK) {
        ftruncate(fd, size - TAF_UPDATE_QOTA_HEADER_SIZE);
        LE_INFO("QOTA header removed.");
    }

    delete[] buffer;
    close(fd);

    return ret;
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

    le_utf8_Copy(tafUpdate.dlAppName, le_json_GetString(), TAF_APPMGMT_APP_NAME_BYTES, NULL);
    LE_INFO("The app name is %s.", tafUpdate.dlAppName);
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
    auto &tafUpdate = taf_Update::GetInstance();
    char path[PATH_MAX] = {0};

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
            LE_INFO("Cleaning up json session.");
            sessRef = le_json_GetSession();
            if (sessRef != NULL)
                le_json_Cleanup(sessRef);
            close(tafUpdate.dlAppFd);

            LE_INFO("Restoring SOTA package.");
            snprintf(path, sizeof(path), TAF_UPDATE_SOTA_PAKCAGE_FILE_PATH, tafUpdate.dlAppName);
            if (rename(TAF_UPDATE_PAKCAGE_FILE_PATH, path)) {
                LE_ERROR("Restoring SOTA package with error.");
                tafUpdate.NotifyDownloadFail();
            } else {
                taf_update_StateInd_t stateInd;
                stateInd.ota = TAF_UPDATE_SOTA;
                stateInd.state = TAF_UPDATE_DOWNLOAD_SUCCESS;
                le_utf8_Copy(stateInd.name, tafUpdate.dlAppName, TAF_APPMGMT_APP_NAME_BYTES, NULL);
                le_event_Report(tafUpdate.stateEvId, &stateInd, sizeof(taf_update_StateInd_t));
                LE_INFO("SOTA package downloaded successfully.");
                tafUpdate.downloadState = TAF_UPDATE_IDLE;
            }
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
    auto &tafUpdate = taf_Update::GetInstance();
    tafUpdate.dlAppFd = open(file, O_RDONLY);
    TAF_ERROR_IF_RET_VAL(tafUpdate.dlAppFd < 0, LE_FAULT, "Open %s failed.", file);

    (void)le_json_Parse(tafUpdate.dlAppFd, JsonEventHandler, JsonErrorHandler, NULL);

    return LE_OK;
}

/*======================================================================
 FUNCTION        taf_Update::ParsePackage
 DESCRIPTION     Parse package
 PARAMETERS      [IN] file: Package file path
 RETURN VALUE    le_result_t: Result of parsing
======================================================================*/
le_result_t taf_Update::ParsePackage(const char* file)
{
    le_result_t result =LE_OK;
    taf_update_OTA_t ota = TAF_UPDATE_NON_QOTA;
    auto &tafUpdate = taf_Update::GetInstance();

    LE_INFO("Parsing header.");
    result = tafUpdate.ParseHeader(file, &ota);
    if ((result != LE_OK) || (ota == TAF_UPDATE_NON_QOTA)) {
        LE_ERROR("Parse header with error, result = %d, ota = %d.", result, ota);
        return LE_FAULT;
    }

    LE_INFO("Removing QOTA header.");
    TAF_ERROR_IF_RET_VAL(tafUpdate.RemoveHeader(file) != LE_OK, LE_FAULT,
        "Remove header with error.");

    if (ota == TAF_UPDATE_FOTA) {
        LE_INFO("Restoring FOTA package.");
        TAF_ERROR_IF_RET_VAL(rename(file, TAF_UPDATE_FOTA_PAKCAGE_FILE_PATH), LE_FAULT,
            "Restoring FOTA package with error.");

        taf_update_StateInd_t stateInd;
        stateInd.ota = TAF_UPDATE_FOTA;
        stateInd.state = TAF_UPDATE_DOWNLOAD_SUCCESS;
        le_utf8_Copy(stateInd.name, TAF_UPDATE_FOTA_PAKCAGE_FILE_PATH,
            TAF_UPDATE_MAX_PKG_NAME_LEN, NULL);
        le_event_Report(tafUpdate.stateEvId, &stateInd, sizeof(taf_update_StateInd_t));

        LE_INFO("FOTA package downloaded successfully.");
        tafUpdate.downloadState = TAF_UPDATE_IDLE;
    } else {
        LE_INFO("Parsing app bundle.");
        TAF_ERROR_IF_RET_VAL(tafUpdate.ParseBundle(file) != LE_OK, LE_FAULT,
            "Parsing app bundle with error.");
    }

    return LE_OK;
}

/*======================================================================
 FUNCTION        taf_Update::StateLayeredHandler
 DESCRIPTION     Layered handler for update state
 PARAMETERS      [IN] reportPtr: Report content
                 [IN] layerHandlerFunc: Layered function of handler
 RETURN VALUE    void
======================================================================*/
void taf_Update::StateLayeredHandler(void* reportPtr, void* layerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(layerHandlerFunc == nullptr, "Null ptr(layerHandlerFunc)");

    taf_update_StateHandlerFunc_t handlerFunc =
        (taf_update_StateHandlerFunc_t)layerHandlerFunc;

    handlerFunc((taf_update_StateInd_t*)reportPtr, le_event_GetContextPtr());
}

/*======================================================================
 FUNCTION        taf_Update::NotifyDownloadFail
 DESCRIPTION     Notify download fail
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void taf_Update::NotifyDownloadFail()
{
    taf_update_StateInd_t stateInd;
    auto &tafUpdate = taf_Update::GetInstance();

    stateInd.state = TAF_UPDATE_DOWNLOAD_FAIL;
    le_event_Report(tafUpdate.stateEvId, &stateInd, sizeof(taf_update_StateInd_t));

    tafUpdate.downloadState = TAF_UPDATE_IDLE;
}

/*======================================================================
 FUNCTION        taf_Update::DownloadTimerHandler
 DESCRIPTION     Download timer handler for getting progress
 PARAMETERS      [IN] timerRef: Timer reference
 RETURN VALUE    void
======================================================================*/
void taf_Update::DownloadTimerHandler(le_timer_Ref_t timerRef)
{
    taf_update_StateInd_t stateInd;
    taf_update_ProgressState_t pState = TAF_UPDATE_PROGRESS_ERROR;
    int percent = 0, ret;
    auto &tafUpdate = taf_Update::GetInstance();

    ret = taf_pa_update_GetProgress(&pState, &percent);
    if (ret) {
        LE_ERROR("Download agent progress failed, ret = %d.", ret);
        le_timer_Stop(timerRef);
        tafUpdate.NotifyDownloadFail();
    } else {
        switch (pState) {
            case TAF_UPDATE_PROGRESS_INIT:
                LE_INFO("Download agent progress state init.");
                break;
            case TAF_UPDATE_PROGRESS_DOWNLOADING:
                LE_INFO("Download agent progress state downloading, percent = %d.", percent);
                stateInd.state = TAF_UPDATE_DOWNLOADING;
                stateInd.percent = percent;
                le_event_Report(tafUpdate.stateEvId, &stateInd, sizeof(taf_update_StateInd_t));
                break;
            case TAF_UPDATE_PROGRESS_FINISH:
                LE_INFO("Download agent progress state finish.");

                LE_INFO("Stop download timer.");
                le_timer_Stop(timerRef);

                if (tafUpdate.ParsePackage(TAF_UPDATE_PAKCAGE_FILE_PATH) != LE_OK) {
                    LE_ERROR("Internal error when parsing download package.");
                    tafUpdate.NotifyDownloadFail();
                }
                break;
            case TAF_UPDATE_PROGRESS_ERROR:
            default:
                LE_ERROR("Download agent progress state = %d error.", pState);
                le_timer_Stop(timerRef);
                tafUpdate.NotifyDownloadFail();
        }
    }
}

/*======================================================================
 FUNCTION        taf_Update::DownloadHandler
 DESCRIPTION     Download handler
 PARAMETERS      [IN] reqPtr: Download request
 RETURN VALUE    void
======================================================================*/
void taf_Update::DownloadHandler(void* reqPtr)
{
    taf_UpdateDlReq_t* dlReq = (taf_UpdateDlReq_t*)reqPtr;
    auto &tafUpdate = taf_Update::GetInstance();

    switch (tafUpdate.downloadState) {
        case TAF_UPDATE_IDLE:
            if (dlReq->event == TAF_UPDATE_DL_START) {
                LE_INFO("Start to download.");
                tafUpdate.downloadState = TAF_UPDATE_DOWNLOADING;
                int ret = taf_pa_update_Download(tafUpdate.daSessionID);
                if (ret) {
                    LE_ERROR("Download agent download failed, ret = %d.", ret);
                    tafUpdate.NotifyDownloadFail();
                } else {
                    le_thread_Sleep(5);
                    LE_INFO("Start download timer to get progress.");
                    le_timer_Start(tafUpdate.dlTimerRef);
                }
            } else {
                LE_ERROR("Invalid operation (%d) for idle state.", dlReq->event);
            }
            break;
        case TAF_UPDATE_DOWNLOADING:
            if (dlReq->event == TAF_UPDATE_DL_PAUSED) {
                LE_WARN("Download paused, only supported in streaming update.");
            } else {
                LE_ERROR("Invalid operation (%d) for downloading state.", dlReq->event);
            }
            break;
        case TAF_UPDATE_DOWNLOAD_PAUSED:
            if (dlReq->event == TAF_UPDATE_DL_RESUME) {
                LE_WARN("Download resume, only supported in streaming update.");
            } else {
                LE_ERROR("Invalid operation (%d) for download paused state.", dlReq->event);
            }
            break;
        default:
            LE_ERROR("Unknown download state = %d.", tafUpdate.downloadState);
    }
}

/*======================================================================
 FUNCTION        taf_Update::RequestHandler
 DESCRIPTION     Request handler
 PARAMETERS      [IN] reqPtr: User request
 RETURN VALUE    void
======================================================================*/
void taf_Update::RequestHandler(void* reqPtr)
{
    taf_UpdateUsrReq_t* usrReq = (taf_UpdateUsrReq_t*)reqPtr;
    auto &tafUpdate = taf_Update::GetInstance();
    taf_UpdateDlReq_t dlReq;

    switch (usrReq->event) {
        case TAF_UPDATE_REQ_DOWNLOAD:
            LE_INFO("Download request received.");
            TAF_ERROR_IF_RET_NIL(tafUpdate.daSessionID == nullptr, "Session ID is null.");
            dlReq.event = TAF_UPDATE_DL_START;
            le_event_Report(tafUpdate.downloadEvId, &dlReq, sizeof(taf_UpdateDlReq_t));
            break;
        case TAF_UPDATE_REQ_INSTALL:
            if (usrReq->ota == TAF_UPDATE_FOTA) {
                LE_INFO("Install firmware request received.");
                taf_FwUpdateReq_t updateReq;
                updateReq.event = TAF_FWUPDATE_EV_INSTALL;
                le_utf8_Copy(updateReq.name, usrReq->name, TAF_UPDATE_MAX_PKG_NAME_LEN, NULL);
                le_event_Report(taf_FwUpdate::fwUpdateEvId, &updateReq, sizeof(taf_FwUpdateReq_t));
            } else if (usrReq->ota == TAF_UPDATE_SOTA) {
                LE_INFO("Install app request received.");
                taf_AppMgmtUpdateReq_t updateReq;
                updateReq.event = TAF_APPMGMT_EV_INSTALL;
                le_utf8_Copy(updateReq.name, usrReq->name, TAF_UPDATE_MAX_PKG_NAME_LEN, NULL);
                le_event_Report(taf_AppMgmt::appUpdateEvId, &updateReq, sizeof(taf_AppMgmtUpdateReq_t));
            } else {
                LE_ERROR("Not FOTA or SOTA installation.");
            }
            break;
        default:
            LE_ERROR("Undefined request %d received.", usrReq->event);
    }
}

/*======================================================================
 FUNCTION        taf_Update::RequestThread
 DESCRIPTION     Thread for handling user request
 PARAMETERS      [IN] contextPtr: Context of the calling thread
 RETURN VALUE    void*: NULL
======================================================================*/
void* taf_Update::RequestThread(void* contextPtr)
{
    le_event_AddHandler("RequestHandler", requestEvId, RequestHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return NULL;
}

/*======================================================================
 FUNCTION        taf_Update::Init
 DESCRIPTION     Initialization of update component
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void taf_Update::Init(void)
{
    chrono::time_point<chrono::system_clock> startTime = chrono::system_clock::now();

    // 1. Get dowload session.
    daSessionID = taf_pa_update_GetSession();
    if (daSessionID == nullptr) {
        LE_ERROR("Session ID is null.");
    }

    // 2. Create events.
    stateEvId = le_event_CreateId("stateEvId", sizeof(taf_update_StateInd_t));
    requestEvId = le_event_CreateId("requestEvId", sizeof(taf_UpdateUsrReq_t));
    downloadEvId = le_event_CreateId("downloadEvId", sizeof(taf_UpdateDlReq_t));

    // 3. Register event handler.
    le_event_AddHandler("DownloadHandler", downloadEvId, DownloadHandler);

    // 4. Create thread for user request.
    le_sem_Ref_t semaphore = le_sem_Create("requestThreadSem", 0);
    le_thread_Ref_t threadRef = le_thread_Create("RequestThread", RequestThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, TAF_UPDATE_THREAD_STACK_SIZE);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);

    // 5. Create download timer.
    dlTimerRef = le_timer_Create("Download Timer");
    le_timer_SetMsInterval(dlTimerRef, TAF_UPDATE_DOWNLOAD_TIME_INTERVAL);
    le_timer_SetRepeat(dlTimerRef, 0);
    le_timer_SetHandler(dlTimerRef, DownloadTimerHandler);

    chrono::time_point<chrono::system_clock> endTime = chrono::system_clock::now();
    chrono::duration<double> elapsedTime = endTime - startTime;
    LE_INFO("Elapsed time for tafUpdate component: %lfs.", elapsedTime.count());
}
