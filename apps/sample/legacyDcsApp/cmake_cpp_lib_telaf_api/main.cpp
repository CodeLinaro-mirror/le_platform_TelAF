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

#include <iostream>
#include <radioAdaptor.hpp>
#include <mpmsAdaptor.hpp>

const char RadioOpMode[7][32] =
{
    "OP_MODE_ONLINE",              ///< Online mode.
    "OP_MODE_AIRPLANE",            ///< Low Power mode, temporarily disabled RF.
    "OP_MODE_FACTORY_TEST",        ///< Special mode for manufacturer use.
    "OP_MODE_OFFLINE",             ///< Device has deactivated RF and partially shutdown.
    "OP_MODE_RESETTING",           ///< Device is in process of power cycling.
    "OP_MODE_SHUTTING_DOWN",       ///< Device is in process of shutting down.
    "OP_MODE_PERSISTENT_LOW_POWER" ///< Persistent low power mode.
};

void AppRadioOpModeChangeCb(taf_radio_OpMode_t mode, void * ctx)
{
    if ((int)mode <= TAF_RADIO_OP_MODE_PERSISTENT_LOW_POWER)
    {
        LE_INFO("-- SampleApp AppRadioOpModeChangeCb mode: %s", RadioOpMode[(int)mode]);
    }
    else
    {
        LE_INFO("-- SampleApp AppRadioOpModeChangeCb mode: UNKNOWN");
    }
}

void AppMpmsWakeupVehicleRspCb(int32_t reason, int32_t response, void * ctx)
{
    LE_INFO("-- SampleApp AppMpmsWakeupVehicleRspCb reason: %d, response: %d", reason, response);
}

static void   *AppTask(void *arg)
{
    le_result_t  result = LE_OK;
    RadioAdaptor ra;
    bool         status = false;
    MpmsAdaptor  pm;

    LE_INFO(" SampleApp AppTask start");

    result = ra.Connect();
    if (result != LE_OK)
    {
        LE_ERROR("SampleApp ra.Connect fail, result: %d", result);
        return nullptr;
    }

    // Invoke TelAF event call back
    LE_INFO("++ SampleApp AppTask calls AddOpModeChangeNotify");
    result = ra.AddOpModeChangeNotify(AppRadioOpModeChangeCb, NULL);
    if (result != LE_OK)
    {
        LE_ERROR("SampleApp ra.AddOpModeChangeNotify fail, result: %d", result);
        return nullptr;
    }

    // Invoke TelAF sync API call back
    LE_INFO("++ SampleApp AppTask calls GetRadioPower");
    result = ra.GetRadioPower(&status);
    LE_INFO("SampleApp AppTask GetRadioPower result: %d, status: %d", result, status);
    if (result == LE_OK)
    {
      LE_INFO("++ SampleApp AppTask calls SetRadioPower");
      result = ra.SetRadioPower(!status);
      LE_INFO("SampleApp AppTask SetRadioPower result: %d", result);
    }

    result = pm.Connect();
    if (result != LE_OK)
    {
        LE_ERROR("SampleApp pm.Connect fail, result: %d", result);
        return nullptr;
    }

    // Invoke TelAF async API call with call back
    LE_INFO("** SampleApp AppTask calls TcuWakeupVehicleReqAsync");
    pm.TcuWakeupVehicleReqAsync(0, AppMpmsWakeupVehicleRspCb, NULL);

    // Illustrating systemd based scheduler
    while(1)
    {
        sleep(1);
    }
    return nullptr;
}

/**
 * Main thread for the application
 */
int main(int argc, char** argv)
{
    RadioAdaptor ra;
    bool status = false;
    le_result_t result = LE_OK;
    int ret;
    pthread_t tid;

    ra.Connect();
    LE_INFO("++ SampleApp AppMain calls GetRadioPower");
    result = ra.GetRadioPower(&status);
    LE_INFO("SampleApp AppMain GetRadioPower result: %d, status: %d", result, status);

    ret = pthread_create(&tid, NULL, AppTask, NULL);  // Run TelafTask in a separate thread.
    if (ret < 0)
    {
        fprintf(stdout, "pthread_create is failed, ret: %d", ret);
        return 0;
    }

    // Illustrating systemd based scheduler
    while (1)
    {
        sleep(1);
    }

    return 0;
}
