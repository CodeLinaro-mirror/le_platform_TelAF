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

/**
 * Main task for telAF thread, including connection with telAF dcs service, entering the event loop etc.
 */
void *TelafTask
(
    void *arg
)
{
    LE_INFO("Enter TelafTask");
    radioAdaptor_Connect();
    dataAdaptor_Connect();

    le_result_t result;
    taf_dcs_Pdp_t ipType = TAF_DCS_PDP_IPV4V6;

    dataAdaptor_DumpDataProfile();

    if(radioAdaptor_IsRadioPowerOn()){
        result = dataAdaptor_StartDataCallOnDefaultProfile(ipType);
    }
    else{
        LE_INFO("Radio power status abnormal");
    }

    dataAdaptor_RegisterEventLoop();
}

/**
 * Main thread for the application.
 */
int main(int argc, char** argv)
{
    int ret;

    pthread_t tid;

    ret = pthread_create(&tid, NULL, TelafTask, NULL);  // Run TelafTask in a separate thread.
    if (ret < 0)
    {
        fprintf(stdout, "pthread_create is failed, ret: %d", ret);
        return -1;
    }

    // Please overwrite the following code per your application.
     while (1)
    {
        sleep(1);
    }

    return 0;
}
