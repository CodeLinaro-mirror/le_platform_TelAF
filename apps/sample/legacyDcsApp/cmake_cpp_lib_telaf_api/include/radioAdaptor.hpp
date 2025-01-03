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

#ifndef RADIOADAPTOR_H
#define RADIOADAPTOR_H

#include <stdio.h>
#include <future>

extern "C" {
#include "legato.h"
#include "taf_radio_interface.h"
}

using namespace std;

#define RADIO_DEFAULT_PHONE_ID 0
#define RADIO_API_TIMEOUT      5  // 5s

typedef void (*RadioOpModeChangeCb)(taf_radio_OpMode_t mode, void * ctx);

typedef struct
{
    void * objPtr;
}GetRadioPowerParm_t;

typedef struct
{
    le_result_t result;
    bool        status;
}GetRadioPowerResult_t;

typedef struct
{
    void * objPtr;
    bool   status;
}SetRadioPowerParm_t;

typedef struct
{
    void *              objPtr;
    RadioOpModeChangeCb callback;
    void *              ctx;
}AddOpModeChangeNotifyParm_t;

typedef struct
{
    AddOpModeChangeNotifyParm_t        parm;
    taf_radio_OpModeChangeHandlerRef_t ref;
}AddOpModeChangeNotifyCtx_t;

typedef struct
{
    le_result_t                 result;
    AddOpModeChangeNotifyCtx_t  *notifyCtx;
}AddOpModeChangeNotifyResult_t;

class RadioAdaptor
{
    public:
        RadioAdaptor(){};
        ~RadioAdaptor(){};
        le_result_t Connect();
        le_result_t GetRadioPower(bool *pwr_on);
        le_result_t SetRadioPower(bool pwr_on);
        le_result_t AddOpModeChangeNotify(RadioOpModeChangeCb, void *);

    private:
        pthread_t     radioTid;
        std::promise<le_result_t> RadioThreadPromise;

        le_event_Id_t GetRadioPowerEventId;
        std::promise<GetRadioPowerResult_t> GetRadioPowerPromise;

        le_event_Id_t SetRadioPowerEventId;
        std::promise<le_result_t> SetRadioPowerPromise;

        le_event_Id_t AddOpModeChangeNotifyEventId;
        std::promise<AddOpModeChangeNotifyResult_t> AddOpModeChangeNotifyPromise;

        void   AddEventReport();

        static void * TelafRadioTask(void *arg);
        static void   OnGetRadioPowerHandler(void* reportPtr);
        static void   OnSetRadioPowerHandler(void* reportPtr);

        static void   OnAddOpModeChangeNotifyHandler(void* reportPtr);
        static void   OnAddOpModeChangeNotifyCb(taf_radio_OpMode_t mode, void * ctx);
};
#endif
