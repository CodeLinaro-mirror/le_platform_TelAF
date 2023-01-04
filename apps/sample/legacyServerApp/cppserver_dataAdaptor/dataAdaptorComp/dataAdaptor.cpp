/**
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "dataAdaptor.h"

/**
 * Dump the available profiles.
 */
void DataAdaptor::DumpDataProfile
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
 * Convert data call event to readable string.
 */
const char* DataAdaptor::CallEventToString
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

/**
 * Start a data call with the default profile on given ip type.
 */
le_result_t DataAdaptor::StartDataCallOnDefaultProfile(taf_dcs_Pdp_t ipType)
{
    uint32_t profileId;
    le_result_t result;
    taf_dcs_SessionStateHandlerRef_t sessionStateRef = NULL;
    taf_dcs_ProfileRef_t profileRef = NULL;

    profileId = taf_dcs_GetDefaultProfileIndex();
    profileRef = taf_dcs_GetProfile(profileId);

    // Register the data call event hander to process the event notified.
    sessionStateRef = taf_dcs_AddSessionStateHandler(profileRef, DataCallEventHandler, &ipType);

    result = taf_dcs_StartSession(profileRef);

    return result;
}

/**
 * Call back handler for received data call event.
 */
void DataAdaptor::DataCallEventHandler
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
 * Initialize a legato thread and connect with taf_dcs service.
 */
void DataAdaptor::Connect()
{
    //le_thread_InitLegatoThreadData("telaf_data_thread");
    taf_dcs_ConnectService();
}

/**
 * Enter the event loop of TelAF.
 */
void DataAdaptor::RegisterEventLoop(void)
{
    // Enter the event loop to make sure telaf events can be handled properly
    le_event_RunLoop();
}

DataAdaptor &DataAdaptor::GetInstance()
{
    static DataAdaptor instance;
    return instance;
}

void DataAdaptor::OnClientSessionDisconnected
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    // When the client session closed, this event handler will be triggered.
    LE_INFO("OnClientSessionDisconnected");
}

void DataAdaptor::OnClientSessionConnected
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    // When the client session connected, this event handler will be triggered.
    LE_INFO("OnClientSessionConnected");

    le_msg_SessionRef_t clientSessionRef = sessionRef;
    pid_t clientPid;
    if (LE_OK != le_msg_GetClientProcessId(sessionRef, &clientPid))
    {
        LE_FATAL("Error, Failed to get pClient procId.");
    }
    LE_INFO("Client connected with PID: %d", clientPid);
}