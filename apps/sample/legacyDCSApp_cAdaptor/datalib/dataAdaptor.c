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

#include "dataAdaptor.h"

/**
 * Dump the available profiles.
 */
void dataAdaptor_DumpDataProfile
(
    void
)
{
    taf_dcs_ProfileInfo_t profilesInfoPtr[TAF_DCS_PROFILE_LIST_MAX_ENTRY];
    size_t listSize;
    le_result_t result;

    result = taf_dcs_GetProfileList(profilesInfoPtr, &listSize);
    LE_ASSERT(result == LE_OK);
    LE_INFO("got profile list, num: %d, result: %d", listSize, result);
    LE_INFO("%-6s""%-6s""%-12s", "Index", "type", "Name");
    for (int i = 0; i < listSize; i++)
    {
        const taf_dcs_ProfileInfo_t *profileInfoPtr = &profilesInfoPtr[i];
        LE_INFO("%-6d""%-6d""%-12s",
                profileInfoPtr->index, profileInfoPtr->tech, profileInfoPtr->name);
    }
}

/**
 * Start a data call with the default profile on given ip type.
 */
le_result_t dataAdaptor_StartDataCallOnDefaultProfile(taf_dcs_Pdp_t ipType)
{
    uint32_t profileId;
    le_result_t result;
    taf_dcs_SessionStateHandlerRef_t sessionStateRef = NULL;
    taf_dcs_ProfileRef_t profileRef = NULL;

    profileId = taf_dcs_GetDefaultProfileIndex();
    profileRef = taf_dcs_GetProfile(profileId);

    // Register the data call event hander to process the event notified.
    sessionStateRef = taf_dcs_AddSessionStateHandler(profileRef,
                      (taf_dcs_SessionStateHandlerFunc_t)DataCallEventHandler, &ipType);

    result = taf_dcs_StartSession(profileRef);

    return result;
}

/**
 * Initialize a legato thread and connect with taf_dcs service.
 */
void dataAdaptor_Connect(pthread_once_t *legatoThreadOnceKey)
{
    pthread_once(legatoThreadOnceKey, legatoContextInitialization);
    taf_dcs_ConnectService();
}

/**
 * Enter the event loop of TelAF.
 */
void dataAdaptor_RegisterEventLoop(void)
{
    // Enter the event loop to make sure telaf events can be handled properly
    le_event_RunLoop();
}
/**
 * Call back handler for received data call event.
 */
static void DataCallEventHandler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_ConState_t callEvent,
    const taf_dcs_StateInfo_t *infoPtr,
    void* contextPtr
)
{
    char interfaceName[64];
    taf_dcs_Pdp_t expectIpType = *(taf_dcs_Pdp_t *)contextPtr;

    LE_INFO("get data handler event. profile ref: %p, callEvent: %s, ip: %d, expect ip: %d\n",
        profileRef, CallEventToString(callEvent), infoPtr->ipType, expectIpType);

    if ((callEvent == TAF_DCS_CONNECTED) && (infoPtr->ipType == expectIpType))
    {
        taf_dcs_GetInterfaceName(profileRef, interfaceName, sizeof(interfaceName));
        LE_INFO("data call connected, interface : %s", interfaceName);
    }
    else if ((callEvent == TAF_DCS_DISCONNECTED) && (infoPtr->ipType == expectIpType))
    {
        taf_dcs_GetInterfaceName(profileRef, interfaceName, sizeof(interfaceName));
        LE_INFO("data call disconnected");
    }

}

/**
 * Convert data call event to readable string.
 */
static char *CallEventToString
(
    taf_dcs_ConState_t callEvent
)
{
    switch (callEvent)
    {
        case TAF_DCS_DISCONNECTED:
            return "disconnected";
        case TAF_DCS_CONNECTING:
            return "connecting";
        case TAF_DCS_CONNECTED:
            return "connected";
        case TAF_DCS_DISCONNECTING:
            return "disconnecting";
        default:
            LE_ERROR("unknown status: %d", callEvent);
            return "unknow status";
    }
    return "unknow status";
}

static void legatoContextInitialization(){
    // Set the TelAF thread context for connection with radio service
    le_thread_InitLegatoThreadData("telaf_data_thread");
}