/*
* Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "radioAdaptor.hpp"

using namespace std;

void * RadioAdaptor::TelafRadioTask(void *arg)
 {
     RadioAdaptor* objPtr = (RadioAdaptor*)(arg);

     LE_INFO("TelafRadioTask start");

     le_thread_InitLegatoThreadData("TelafRadioTask-thread");

     if (objPtr == NULL) return NULL;

     taf_radio_ConnectService();

     objPtr->AddEventReport();

     LE_INFO("TelafRadioTask fwk run event loop");
     objPtr->RadioThreadPromise.set_value(LE_OK);

     le_event_RunLoop();

     return nullptr;
}

void RadioAdaptor::AddEventReport()
{
    LE_INFO("libradio AddEventReport\n");

    // GetRadioPower API
    GetRadioPowerEventId = le_event_CreateId("GetRadioPowerEvent", sizeof(GetRadioPowerParm_t));
    le_event_AddHandler("GetRadioPowerHandler",
                         GetRadioPowerEventId,
                         RadioAdaptor::OnGetRadioPowerHandler);

    // SetRadioPower API
    SetRadioPowerEventId = le_event_CreateId("SetRadioPowerEvent", sizeof(SetRadioPowerParm_t));
    le_event_AddHandler("SetRadioPowerHandler",
                         SetRadioPowerEventId,
                         RadioAdaptor::OnSetRadioPowerHandler);

    // AddOpModeChangeNotify API
    AddOpModeChangeNotifyEventId = le_event_CreateId("AddOpModeChangeNotifyEvent",
                                                      sizeof(AddOpModeChangeNotifyParm_t));
    le_event_AddHandler("AddOpModeChangeNotifyHandler",
                         AddOpModeChangeNotifyEventId,
                         RadioAdaptor::OnAddOpModeChangeNotifyHandler);
}

void RadioAdaptor::OnGetRadioPowerHandler(void* reportPtr)
{
    GetRadioPowerParm_t *evtPtr = (GetRadioPowerParm_t*)reportPtr;
    GetRadioPowerResult_t sResult;

    if (evtPtr == NULL)
    {
        LE_ERROR("OnGetRadioPowerHandler called with NULL report");
    }
    else
    {
        bool ret = false;
        le_result_t res;
        le_onoff_t state;

        res = taf_radio_GetRadioPower(&state, RADIO_DEFAULT_PHONE_ID);

        if (res == LE_OK)
        {
            switch (state)
            {
                case LE_OFF:
                    LE_INFO("Power OFF");
                    break;
                case LE_ON:
                    LE_INFO("Power ON");
                    ret = true;
                    break;
                default:
                    LE_ERROR("Power Invalid state :%d", (int)state);
                    ret = false;
            }
        }

        if ((evtPtr->objPtr) != NULL)
        {
            sResult.result = res;
            sResult.status = ret;
            ((RadioAdaptor*)evtPtr->objPtr)->GetRadioPowerPromise.set_value(sResult);
        }
        else
        {
            LE_ERROR("NULL object");
        }
    }
}

void RadioAdaptor::OnSetRadioPowerHandler(void* reportPtr)
{
    SetRadioPowerParm_t *evtPtr = (SetRadioPowerParm_t*)reportPtr;
    le_result_t res;


    if (evtPtr == NULL)
    {
        LE_ERROR("OnSetRadioPowerHandler called with NULL report");
    }
    else
    {
        le_onoff_t radio_pwr;

        radio_pwr = (evtPtr->status? LE_ON : LE_OFF);

        switch (radio_pwr)
        {
            case LE_OFF:
                LE_INFO("Power OFF");
                break;
            case LE_ON:
                LE_INFO("Power ON");
                break;
            default:
                LE_ERROR("Power Invalid state :%d", (int)radio_pwr);
        }

        res = taf_radio_SetRadioPower(radio_pwr, RADIO_DEFAULT_PHONE_ID);

        if ((evtPtr->objPtr) != NULL)
        {
            ((RadioAdaptor*)evtPtr->objPtr)->SetRadioPowerPromise.set_value(res);
        }
        else
        {
            LE_ERROR("NULL object");
        }
    }
}

void RadioAdaptor::OnAddOpModeChangeNotifyHandler(void* reportPtr)
{
    AddOpModeChangeNotifyParm_t *evtPtr = (AddOpModeChangeNotifyParm_t*)reportPtr;
    taf_radio_OpModeChangeHandlerRef_t ref;
    AddOpModeChangeNotifyCtx_t * eventCtx;
    AddOpModeChangeNotifyResult_t sResult;

    if (evtPtr == NULL)
    {
        LE_ERROR("OnAddOpModeChangeNotifyHandler called with NULL report");
    }
    else
    {
        if ((evtPtr->objPtr) == NULL)
        {
            LE_ERROR("NULL object");
            return;
        }

        // Evaluating change to use new API in future release
        eventCtx = (AddOpModeChangeNotifyCtx_t*)malloc(sizeof(AddOpModeChangeNotifyCtx_t));

        if (eventCtx == NULL)
        {
            LE_ERROR("OnAddOpModeChangeNotifyHandler eventCtx == NULL");

            sResult.result = LE_FAULT;
            ((RadioAdaptor*)evtPtr->objPtr)->AddOpModeChangeNotifyPromise.set_value(sResult);
            return;
        }

        eventCtx->parm.objPtr   = evtPtr->objPtr;
        eventCtx->parm.callback = evtPtr->callback;
        eventCtx->parm.ctx      = evtPtr->ctx;

        ref = taf_radio_AddOpModeChangeHandler(&RadioAdaptor::OnAddOpModeChangeNotifyCb,
                                               (void*)eventCtx);

        if (ref == NULL)
        {
            LE_ERROR("OnAddOpModeChangeNotifyHandler ref == NULL");
            sResult.result = LE_FAULT;
            ((RadioAdaptor*)evtPtr->objPtr)->AddOpModeChangeNotifyPromise.set_value(sResult);
            free(eventCtx);
            return;
        }

        eventCtx->ref     = ref;
        sResult.notifyCtx = eventCtx;
        sResult.result    = LE_OK;
        ((RadioAdaptor*)evtPtr->objPtr)->AddOpModeChangeNotifyPromise.set_value(sResult);
    }
}

void RadioAdaptor::OnAddOpModeChangeNotifyCb(taf_radio_OpMode_t mode, void * ctx)
{
    AddOpModeChangeNotifyCtx_t * eventCtx = (AddOpModeChangeNotifyCtx_t *)ctx;

    LE_INFO("libradio OnAddOpModeChangeNotifyCb");

    if (eventCtx != NULL)
    {
        if (eventCtx->parm.callback != NULL)
        {
            eventCtx->parm.callback(mode, (void*)eventCtx->parm.ctx);
        }
    }
}

le_result_t  RadioAdaptor::Connect()
{
    int ret;
    le_result_t result = LE_OK;
    std::chrono::seconds span(RADIO_API_TIMEOUT);

    LE_INFO("libradio Connect");

    RadioThreadPromise = std::promise<le_result_t>();

    // Run TelafTask in a separate thread.
    ret = pthread_create(&radioTid, NULL, &RadioAdaptor::TelafRadioTask, this);
    if (ret < 0)
    {
        fprintf(stdout, "pthread_create is failed, ret: %d", ret);
        return LE_FAULT;
    }

    // blocking here to get response
    std::future<le_result_t> futResult = RadioThreadPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("libradio Connect waiting promise timeout");
        return LE_TIMEOUT;
    }
    result = futResult.get();

    return result;
}

/**
 * Check whether the radio power status is on.
 */
le_result_t RadioAdaptor::GetRadioPower(bool *status)
{
    le_result_t result = LE_OK;
    GetRadioPowerParm_t evt;
    std::chrono::seconds span(RADIO_API_TIMEOUT);

    GetRadioPowerResult_t sResult;

    LE_INFO("libradio GetRadioPower called\n");

    evt.objPtr = (void*)this;

    GetRadioPowerPromise = std::promise<GetRadioPowerResult_t>();

    le_event_Report(GetRadioPowerEventId, &evt, sizeof(GetRadioPowerParm_t));

    // blocking here to get response
    std::future<GetRadioPowerResult_t> futResult = GetRadioPowerPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("GetRadioPower waiting promise timeout");
        return LE_TIMEOUT;
    }

    sResult    = futResult.get();
    result     = sResult.result;
    *status = sResult.status;

    return result;
}

le_result_t RadioAdaptor::SetRadioPower(bool pwr_on)
{
    le_result_t result = LE_OK;
    SetRadioPowerParm_t evt;
    std::chrono::seconds span(RADIO_API_TIMEOUT);

    GetRadioPowerResult_t sResult;

    LE_INFO("libradio SetRadioPower called\n");

    evt.objPtr = (void*)this;
    evt.status = pwr_on;

    SetRadioPowerPromise = std::promise<le_result_t>();

    le_event_Report(SetRadioPowerEventId, &evt, sizeof(SetRadioPowerParm_t));

    // blocking here to get response
    std::future<le_result_t> futResult = SetRadioPowerPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("GetRadioPower waiting promise timeout");
        return LE_TIMEOUT;
    }

    result     = futResult.get();
    return result;
}

le_result_t RadioAdaptor::AddOpModeChangeNotify(RadioOpModeChangeCb callback, void * ctx)
{
    le_result_t result = LE_OK;
    AddOpModeChangeNotifyParm_t evt;
    std::chrono::seconds span(RADIO_API_TIMEOUT);

    AddOpModeChangeNotifyResult_t sResult;

    LE_INFO("libradio AddOpModeChangeNotify called");

    evt.objPtr   = (void*)this;
    evt.callback = callback;
    evt.ctx      = ctx;

    AddOpModeChangeNotifyPromise = std::promise<AddOpModeChangeNotifyResult_t>();

    le_event_Report(AddOpModeChangeNotifyEventId, &evt, sizeof(AddOpModeChangeNotifyParm_t));

    // blocking here to get response
    std::future<AddOpModeChangeNotifyResult_t> futResult \
                                               = AddOpModeChangeNotifyPromise.get_future();
    std::future_status waitStatus = futResult.wait_for(span);
    if (std::future_status::timeout == waitStatus)
    {
        LE_ERROR("AddOpModeChangeNotify waiting promise timeout");
        return LE_TIMEOUT;
    }

    sResult    = futResult.get();
    result     = sResult.result;

    return result;
}

