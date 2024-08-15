/*
* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <dataAdaptor.h>
#include <radioAdaptor.h>
#include <mutex>
#include <condition_variable>

// Global variable for thread communication
std::mutex mtx;
std::condition_variable cv;
bool eventReady = false;

/**
 * Main task for TelAF thread, including delegating event handler, entering the event loop etc.
 */
void *TelafTask
(
    void *arg
)
{
    LE_INFO("Enter TelafTask");
    std::lock_guard<std::mutex>lock(mtx);

    // Delegate event handler
    LE_INFO("Create data call event hanler");
    uint32_t profileId = taf_dcs_GetDefaultProfileIndex();
    taf_dcs_ProfileRef_t profileRef = taf_dcs_GetProfile(profileId);
    taf_dcs_ConState_t callEvent;
    taf_dcs_StateInfo_t info;
    taf_dcs_Pdp_t contextPtr = TAF_DCS_PDP_IPV4V6;
    da.DataCallEventHandler(profileRef, callEvent, &info, &contextPtr); // Create a data call event handler.

    da.RegisterEventLoop();

    eventReady = true;
    cv.notify_one(); // Notify main thread

    return nullptr;
}


/**
 * Main thread for the application
 */
int main(int argc, char** argv)
{
    // Variable for radio adaptor and data adaptor
    RadioAdaptor ra;
    DataAdaptor da;

    RadioAdaptor::Connect();
    DataAdaptor::Connect();

    le_result_t result;
    taf_dcs_Pdp_t ipType = TAF_DCS_PDP_IPV4V6;

    da.DumpDataProfile();

    if(ra.IsRadioPowerOn()){
        result = da.StartDataCallOnDefaultProfile(ipType);
    }
    else{
        LE_INFO("Radio power status abnormal");
    }

    int ret;
    pthread_t tid;
    ret = pthread_create(&tid, NULL, TelafTask, NULL);  // Create a separate thread, run TelafTask in the thread.

    std::unique_lock<std::mutex>lock(mtx);
    cv.wait(lock, []{return eventReady;});

    if (ret < 0)
    {
        fprintf(stdout, "pthread_create for main thread is failed, ret: %d", ret);
        return -1;
    }

    return 0;
}