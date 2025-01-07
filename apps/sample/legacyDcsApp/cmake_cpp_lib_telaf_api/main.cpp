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

#include <iostream>
#include <radioAdaptor.hpp>
#include <mpmsAdaptor.hpp>
#include <dcsAdaptor.hpp>

// Future to any event that the app is waiting on
static std::promise<void> registrationPromise;
static std::promise<void> dataConnectedPromise;
static std::promise<void> dataDisconnectedPromise;
// Flag to check if future should set value
static bool bWaitingForEvent = false;

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

static std::string NetRegStateToString(taf_radio_NetRegState_t NetRegState)
{
    switch (NetRegState)
    {
    case TAF_RADIO_NET_REG_STATE_NONE:
        return "Not registered and not searching.";
    case TAF_RADIO_NET_REG_STATE_HOME:
        return "Home registered.";
    case TAF_RADIO_NET_REG_STATE_SEARCHING:
        return "Not registered and searching.";
    case TAF_RADIO_NET_REG_STATE_DENIED:
        return "Registration denied.";
    case TAF_RADIO_NET_REG_STATE_ROAMING:
        return "Roaming registered.";
    case TAF_RADIO_NET_REG_STATE_UNKNOWN:
        return "Unknown registration state.";
    case TAF_RADIO_NET_REG_STATE_NONE_AND_EMERGENCY_AVAILABLE:
        return "Not registered and not searching, but emergency calls available.";
    case TAF_RADIO_NET_REG_STATE_SEARCHING_AND_EMERGENCY_AVAILABLE:
        return "Not registered and searching, but emergency calls available.";
    case TAF_RADIO_NET_REG_STATE_DENIED_AND_EMERGENCY_AVAILABLE:
        return "Registration denied, but emergency calls available.";
    case TAF_RADIO_NET_REG_STATE_UNKNOWN_AND_EMERGENCY_AVAILABLE:
        return "Unknown registration state, but emergency calls available.";
    default:
        return "Invalid state.";
    }
}

static void AppRegistrationStateChangeCb(const taf_radio_NetRegStateInd_t *netRegStateIndPtr, void *ctx)
{
    LE_INFO ("Phone Id                : %d", netRegStateIndPtr->phoneId);
    LE_INFO ("Packet Switched Nw State: %d(%s)", netRegStateIndPtr->state,
                                NetRegStateToString(netRegStateIndPtr->state).c_str());
    if (TAF_RADIO_NET_REG_STATE_HOME == netRegStateIndPtr->state ||
        TAF_RADIO_NET_REG_STATE_ROAMING == netRegStateIndPtr->state)
    {
        // Set the promise if the NAD registration state is either ROAMING or HOME
        if (bWaitingForEvent)
            registrationPromise.set_value();
    }
}

static std::string ConStateToString (taf_dcs_ConState_t state)
{
    switch (state)
    {
    case TAF_DCS_DISCONNECTED:
        return "TAF_DCS_DISCONNECTED";
    case TAF_DCS_CONNECTING:
        return "TAF_DCS_CONNECTING";
    case TAF_DCS_CONNECTED:
        return "TAF_DCS_CONNECTED";
    case TAF_DCS_DISCONNECTING:
        return "TAF_DCS_DISCONNECTING";
    default:
        return "Unknown";
    }
}

static std::string IpTypeToString(taf_dcs_Pdp_t pdp)
{
    switch (pdp)
    {
    case TAF_DCS_PDP_UNKNOWN:
        return "TAF_DCS_PDP_UNKNOWN";
    case TAF_DCS_PDP_IPV4:
        return "TAF_DCS_PDP_IPV4";
    case TAF_DCS_PDP_IPV6:
        return "TAF_DCS_PDP_IPV6";
    case TAF_DCS_PDP_IPV4V6:
        return "TAF_DCS_PDP_IPV4V6";
    default:
        return "Unknown";
    }
}

static void DataSessionStateChangeCb ( uint32_t profileId,
                                taf_dcs_ConState_t state,
                                taf_dcs_Pdp_t ipType,
                                void *ctx)
{
    LE_INFO("Profile Id: %d", profileId);
    LE_INFO("State: %d(%s)", state, ConStateToString(state).c_str());
    LE_INFO("IP Type: %d(%s)", ipType, IpTypeToString(ipType).c_str());
    if (TAF_DCS_PDP_IPV4V6 == ipType && TAF_DCS_CONNECTED == state)
    {
        // Set the promise if app is waiting
        if (bWaitingForEvent)
            dataConnectedPromise.set_value();
    }
    // For disconnected event, get the call end reason
    if (TAF_DCS_PDP_IPV4V6 == ipType && TAF_DCS_DISCONNECTED == state)
    {
        // Set the promise if app is waiting
        if (bWaitingForEvent)
            dataDisconnectedPromise.set_value();
    }
}

void AppMpmsWakeupVehicleRspCb(int32_t reason, int32_t response, le_result_t result, void * ctx)
{
    LE_INFO("-- SampleApp AppMpmsWakeupVehicleRspCb reason: %d, response: %d, result: %d",
              reason, response, (int)result);
}

static void   *AppTask(void *arg)
{
    le_result_t  result = LE_OK;
    RadioAdaptor ra;
    bool         status = false;
    MpmsAdaptor  pm;
    DcsAdaptor   dcs;
    bool         bSkipNetworkRegistrationCheck = false;

    LE_INFO(" SampleApp AppTask start");

    result = ra.Connect();
    if (result != LE_OK)
    {
        LE_ERROR("SampleApp ra.Connect fail, result: %d", result);
        return nullptr;
    }

    result = dcs.Connect();
    if (result != LE_OK)
    {
        LE_ERROR("SampleApp dcs.Connect failed, result: %d", result);
        return nullptr;
    }

    // Invoke TelAF radio event call back
    LE_INFO("++ SampleApp AppTask calls AddOpModeChangeNotify");
    result = ra.AddOpModeChangeNotify(AppRadioOpModeChangeCb, NULL);
    if (result != LE_OK)
    {
        LE_ERROR("SampleApp ra.AddOpModeChangeNotify fail, result: %d", result);
        return nullptr;
    }

    // Register for registration state events
    LE_INFO("++ SampleApp AppTask calls AddRegistrationStateChangeNotify");
    result = ra.AddRegistrationStateChangeNotify(AppRegistrationStateChangeCb, NULL);
    if (result != LE_OK)
    {
        LE_ERROR("SampleApp ra.AddRegistrationStateChangeNotify fail, result: %d(%s)", result,
                                                                            LE_RESULT_TXT(result));
        return nullptr;
    }

    // Invoke TelAF sync API call back
    LE_INFO("++ SampleApp AppTask calls GetRadioPower");
    result = ra.GetRadioPower(&status);
    LE_INFO("SampleApp AppTask GetRadioPower result: %d, status: %d", result, status);
    if (result == LE_OK)
    {
        LE_INFO("Radio State: %s", status?"ON":"OFF");
        // Turn on radio if it's off.
        if (false == status)
        {
            LE_INFO("++ SampleApp AppTask calls SetRadioPower to turn radio ON");
            result = ra.SetRadioPower(true);
            LE_INFO("SampleApp AppTask SetRadioPower result: %d", result);
            if (result != LE_OK)
            {
                LE_ERROR("SetRadioPower failed: %d(%s)", result, LE_RESULT_TXT(result));
                return nullptr;
            }

            // Wait for data call service to notify app that NAD has registered
            std::future<void> pktReg_future = registrationPromise.get_future();
            // Wait for packet network registered event
            bWaitingForEvent = true;
            pktReg_future.wait();
            // Mark that the app is not waiting for an event anymore.
            bWaitingForEvent = false;
            // Mark that network registration check can be skipped
            bSkipNetworkRegistrationCheck = true;
            LE_INFO ("NAD is registered");
            std::cout << "NAD is registered" << std::endl;
        }
    }
    else
    {
        LE_ERROR("GetRadioPower failed: %d(%s)", result, LE_RESULT_TXT(result));
        // Handle errors
        return nullptr;
    }

    // Check if NAD  is registered, if needed
    if (!bSkipNetworkRegistrationCheck)
    {
        // Check if NAD is registered or not
        taf_radio_NetRegState_t netRegState;
        result = ra.GetRegistrationState(&netRegState);
        if (result != LE_OK)
        {
            LE_ERROR("SampleApp ra.GetRegistrationState failed, result: %d(%s)", result,
                                                                            LE_RESULT_TXT(result));
            return nullptr;
        }
        if (TAF_RADIO_NET_REG_STATE_HOME != netRegState &&
            TAF_RADIO_NET_REG_STATE_ROAMING != netRegState)
        {
            LE_ERROR("NAD is not registered: %d(%s)", netRegState,
                                                        NetRegStateToString(netRegState).c_str());
            return nullptr;
        }
        LE_INFO ("NAD is registered");
        std::cout << "NAD is registered" << std::endl;
    }

    // Get the default profile ID
    uint32_t profileId = 0;
    result = dcs.GetDefaultProfileId(profileId);
    if (result != LE_OK)
    {
        LE_ERROR("SampleApp dcs.GetDefaultProfileId failed, result: %d(%s)", result,
                                                                    LE_RESULT_TXT(result));
        return nullptr;
    }
    LE_INFO("Default profile Id: %d", profileId);
    std::cout << "Default profile Id: " << profileId << std::endl;

    // Register for data state notifications with the dcs object pointer
    result = dcs.AddSessionStateChangeCbk(profileId, DataSessionStateChangeCb, NULL);
    if (result != LE_OK)
    {
        LE_ERROR("SampleApp dcs.AddSessionStateChangeCbk failed, result: %d(%s)", result,
                                                                    LE_RESULT_TXT(result));
        return nullptr;
    }

    // Start data session
    result = dcs.StartSession(profileId);
    if (result != LE_OK)
    {
        taf_dcs_CallEndReasonType_t CallEndType;
        int32_t CallEndReason;

        LE_ERROR("SampleApp dcs.StartSession failed, result: %d(%s)", result,
                                                                    LE_RESULT_TXT(result));
        // Get the failure reason
        result = dcs.GetCallEndReason(profileId, TAF_DCS_PDP_IPV4,
                                        CallEndType, CallEndReason);
        if (result != LE_OK)
        {
            LE_WARN("SampleApp dcs.GetCallEndReason failed, result: %d(%s)", result,
                    LE_RESULT_TXT(result));
        }
        LE_INFO("Call End - Type: %d, Reason: %d", CallEndType, CallEndReason);
        std::cout << "Call End - Type: " << CallEndType << ", Reason: " << CallEndReason <<
                                                                                        std::endl;
    }
    LE_INFO("Data started");
    std::cout << "Data started" << std::endl;

    // Wait for IPv4v6 connected event
    std::future<void> ipv4v6Connected_future = dataConnectedPromise.get_future();
    // Wait for packet network registered event
    bWaitingForEvent = true;
    ipv4v6Connected_future.wait();
    // Mark that the app is not waiting for an event anymore.
    bWaitingForEvent = false;

    // Get the IPv4 address
    std::string ipv4Address;
    result = dcs.GetIPv4Address(profileId, ipv4Address);
    if (result != LE_OK)
    {
        LE_ERROR("SampleApp dcs.GetIPv4Address failed, result: %d(%s)", result,
                                                                    LE_RESULT_TXT(result));
        return nullptr;
    }
    LE_INFO("IPv4 address: %s", ipv4Address.c_str());
    std::cout << "IPv4 address: " << ipv4Address << std::endl;

    //Get the interface name
    std::string intfName;
    result = dcs.GetInterfaceName(profileId, intfName);
    if (result != LE_OK)
    {
        LE_ERROR("SampleApp dcs.GetInterfaceName failed, result: %d(%s)", result,
                 LE_RESULT_TXT(result));
        return nullptr;
    }
    LE_INFO("Interface name: %s", intfName.c_str());
    std::cout << "Interface name: " << intfName << std::endl;

    //Stop data session
    result = dcs.StopSession(profileId);
    if (result != LE_OK)
    {
        LE_ERROR("SampleApp dcs.StopSession failed, result: %d(%s)", result,
                 LE_RESULT_TXT(result));
        return nullptr;
    }

    LE_INFO("Data stopped");
    std::cout << "Data stopped" << std::endl;

    // Wait for IPv4v6 disconnected event
    std::future<void> ipv4v6Disconnected_future = dataDisconnectedPromise.get_future();
    // Wait for packet network registered event
    bWaitingForEvent = true;
    ipv4v6Disconnected_future.wait();
    // Mark that the app is not waiting for an event anymore.
    bWaitingForEvent = false;

    taf_dcs_CallEndReasonType_t CallEndType;
    int32_t CallEndReason;
    // Get the failure reason
    result = dcs.GetCallEndReason(profileId, TAF_DCS_PDP_IPV4,CallEndType, CallEndReason);
    if (result != LE_OK)
    {
        LE_WARN("SampleApp dcs.GetCallEndReason failed, result: %d(%s)", result,
                                                                LE_RESULT_TXT(result));
    }
    LE_INFO("Call End - Type: %d, Reason: %d", CallEndType, CallEndReason);
    std::cout << "Call End - Type: " << CallEndType << ", Reason: " << CallEndReason << std::endl;

    result = pm.Connect();
    if (result != LE_OK)
    {
        LE_ERROR("SampleApp pm.Connect fail, result: %d", result);
        return nullptr;
    }

    // Invoke TelAF async API call with call back
    LE_INFO("** SampleApp AppTask calls TcuWakeupVehicleReqAsync");
    pm.TcuWakeupVehicleReqAsync(0, AppMpmsWakeupVehicleRspCb, NULL);

    std::cout << "Use CTRL-C to stop application" << std::endl;
    // Illustrating systemd based scheduler
    while (1)
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
