/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the
* disclaimer below) provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following
* disclaimer in the documentation and/or other materials provided
* with the distribution.
*
* * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
* contributors may be used to endorse or promote products derived
* from this software without specific prior written permission.
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

#include "mpmsAdaptor.hpp"

using namespace std;

void * MpmsAdaptor::TelafMpmsTask(void *arg)
 {
     MpmsAdaptor* objPtr = (MpmsAdaptor*)(arg);

     LE_INFO("TelafMpmsTask start");

     le_thread_InitLegatoThreadData("TelafMpmsTask-thread");

     if (objPtr == NULL) return NULL;

     taf_mngdPm_ConnectService();

     objPtr->AddEventReport();

     LE_INFO("TelafMpmsTask fwk run event loop");
     objPtr->MpmsThreadPromise.set_value(LE_OK);

     le_event_RunLoop();

     return nullptr;
}

void MpmsAdaptor::AddEventReport()
{
    LE_INFO("libmpms AddEventReport\n");

    // TcuWakeupVehicleReqAsync API
    TcuWakeupVehicleReqAsyncEventId = le_event_CreateId("TcuWakeupVehicleReqAsyncEvent",
                                                         sizeof(TcuWakeupVehicleReqAsyncParm_t));
    le_event_AddHandler("TcuWakeupVehicleReqAsyncHandler",
                         TcuWakeupVehicleReqAsyncEventId,
                         MpmsAdaptor::OnTcuWakeupVehicleReqAsyncHandler);
}

void MpmsAdaptor::OnTcuWakeupVehicleReqAsyncHandler(void* reportPtr)
{
    TcuWakeupVehicleReqAsyncParm_t *evtPtr = (TcuWakeupVehicleReqAsyncParm_t*)reportPtr;
    TcuWakeupVehicleReqAsyncCtx_t * eventCtx;
    le_result_t res;

    if (evtPtr == NULL)
    {
        LE_ERROR("OnTcuWakeupVehicleReqAsyncHandler called with NULL report");
    }
    else
    {
        if ((evtPtr->objPtr) == NULL)
        {
            LE_ERROR("NULL object");
            return;
        }

        // Evaluating change to use new API in future release
        eventCtx = (TcuWakeupVehicleReqAsyncCtx_t*)malloc(sizeof(TcuWakeupVehicleReqAsyncCtx_t));

        if (eventCtx == NULL)
        {
            LE_ERROR("OnTcuWakeupVehicleReqAsyncHandler eventCtx == NULL");
            ((MpmsAdaptor*)evtPtr->objPtr)->TcuWakeupVehicleReqAsyncPromise.set_value(LE_FAULT);
            return;
        }

        eventCtx->parm.objPtr   = evtPtr->objPtr;
        eventCtx->parm.callback = evtPtr->callback;
        eventCtx->parm.ctx      = evtPtr->ctx;

        res = taf_mngdPm_WakeupVehicleReqAsync(evtPtr->reason,
                                               &MpmsAdaptor::OnTcuWakeupVehicleReqAsyncCb,
                                               (void*)eventCtx);
        ((MpmsAdaptor*)evtPtr->objPtr)->TcuWakeupVehicleReqAsyncPromise.set_value(res);
    }
}

void MpmsAdaptor::OnTcuWakeupVehicleReqAsyncCb(int32_t reason, int32_t response, void * ctx)
{
    TcuWakeupVehicleReqAsyncCtx_t * eventCtx = (TcuWakeupVehicleReqAsyncCtx_t *)ctx;

    LE_INFO("libmpms OnTcuWakeupVehicleReqAsyncCb");

    if (eventCtx != NULL)
    {
        if (eventCtx->parm.callback != NULL)
        {
            eventCtx->parm.callback(reason, response, (void*)eventCtx->parm.ctx);
        }

        // Async CB one time. Evaluating change to use delete API in future release
        free(eventCtx);
    }
}

le_result_t  MpmsAdaptor::Connect()
{
    int ret;
    le_result_t result = LE_OK;
    std::chrono::seconds span(MPMS_API_TIMEOUT);

    LE_INFO("libmpms Connect");

    MpmsThreadPromise = std::promise<le_result_t>();

    // Run TelafTask in a separate thread.
    ret = pthread_create(&MpmsTid, NULL, &MpmsAdaptor::TelafMpmsTask, this);
    if (ret < 0)
    {
        fprintf(stdout, "pthread_create is failed, ret: %d", ret);
        return LE_FAULT;
    }

    // blocking here to get response
    std::future<le_result_t> futResult = MpmsThreadPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("libmpms Connect waiting promise timeout");
        return LE_TIMEOUT;
    }
    result = futResult.get();

    return result;
}

le_result_t MpmsAdaptor::TcuWakeupVehicleReqAsync(int32_t reason,
                                                  TcuWakeupVehicleReqCb callback,
                                                  void *ctx)
{
    le_result_t result = LE_OK;
    TcuWakeupVehicleReqAsyncParm_t evt;
    std::chrono::seconds span(MPMS_API_TIMEOUT);

    LE_INFO("libmpms TcuWakeupVehicleReqAsync called");

    evt.objPtr      = (void*)this;
    evt.callback    = callback;
    evt.ctx         = ctx;
    evt.reason      = reason;

    TcuWakeupVehicleReqAsyncPromise = std::promise<le_result_t>();

    le_event_Report(TcuWakeupVehicleReqAsyncEventId, &evt, sizeof(TcuWakeupVehicleReqAsyncParm_t));

    // blocking here to get response
    std::future<le_result_t> futResult = TcuWakeupVehicleReqAsyncPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("TcuWakeupVehicleReqAsync waiting promise timeout");
        return LE_TIMEOUT;
    }

    result    = futResult.get();
    return result;
}

