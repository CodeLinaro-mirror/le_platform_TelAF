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

#include "radioAdaptor.h"

/**
 * Initialize a legato thread and connect with taf_radio service.
 */
void RadioAdaptor::Connect()
{
    // Set the TelAF thread context for connection
    le_thread_InitLegatoThreadData("telaf_thread");
    taf_radio_ConnectService();
}

/**
 * Enter the event loop of TelAF.
 */
void RadioAdaptor::RegisterEventLoop(void)
{
    // Enter the event loop to make sure telaf events can be handled properly
    le_event_RunLoop();
}

/**
 * Check whether the radio power status is on.
 */
bool RadioAdaptor::IsRadioPowerOn(void){
    bool ret = false;
    le_result_t res;
    le_onoff_t state;

    res = taf_radio_GetRadioPower(&state, RADIO_DEFAULT_PHONE_ID);

    if (res != LE_OK)
    {
        return ret;
    }

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
            ret = false;
    }

    return ret;
}