/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#include "tafMngdPMSvc.hpp"
#include "tafMngdPMCommon.hpp"
#include "can/tafMngdPMCan.hpp"
#include "sms/tafMngdPMSms.hpp"
#include "gpio/tafMngdPMGpio.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "limit.h"

using namespace telux::tafsvc;
namespace pt = boost::property_tree;

/**
 * To convert TafState to string
 */
const char* tafMngdPMSvc::TafStateToString(taf_mngd_pm_State_t tafState)
{
    const char *state;
    switch(tafState)
    {
        case TAF_MNGD_PM_STATE_RESUME:
            state = "Resume";
            break;
        case TAF_MNGD_PM_STATE_SUSPEND:
            state = "Suspend";
            break;
        case TAF_MNGD_PM_STATE_SHUTDOWN:
            state = "Shutdown";
            break;
        case TAF_MNGD_PM_STATE_RELEASING_WAKE_SOURCE:
            state = "Releasing wake source";
            break;
        case TAF_MNGD_PM_STATE_SUSPENDING:
            state = "Suspending";
            break;
        case TAF_MNGD_PM_STATE_SHUTTING_DOWN:
            state = "Shutting down";
            break;
        case TAF_MNGD_PM_STATE_WAKING_UP:
            state = "Waking up";
            break;
        default :
            state = "Unknown";
            break;
    }
    return state;
}

/**
 * Parse JSON config file
 */
le_result_t tafMngdPMSvc::ParseJsonConfig(std::string configPath)
{
    LE_DEBUG("ParseJsonConfig %s", configPath.c_str());

    if (configPath.empty())
    {
        LE_ERROR("configPath is empty!");
        return LE_FAULT;
    }

    std::ifstream jsonFile(configPath);
    if (!jsonFile.is_open())
    {
        LE_WARN ("Unable to open %s", configPath.c_str());
        return LE_FAULT;
    }

    // Create a root
    pt::ptree root;
    // Load the json file in this ptree
    try
    {
        pt::read_json(configPath, root);
    }
    catch (const std::exception &e)
    {
        LE_WARN ("read_json exception: %s. Check validity of JSON.", e.what());
        return LE_FAULT;
    }

    auto can = tafMngdPMCan::GetInstance();
    auto sms = tafMngdPMSms::GetInstance();
    auto gpio = tafMngdPMGpio::GetInstance();
    for(auto & element : root)
    {
        //Product
        if ("Product" == element.first )
        {
            // Get the elements within "Product"
            if (element.second.get_value < std::string > () != "TelAF")
            {
                LE_WARN("Invalid JSON_Property Value");
                return LE_FAULT;
            }
        }

        //Name
        if ("Name" == element.first )
        {

            // Get the elements within "Product"
            if (element.second.get_value < std::string > ()
             != "Managed PM Configuration")
            {
                LE_WARN("Invalid JSON_Property Value");
                return LE_FAULT;
            }
        }
        if ("ManagedPMService" == element.first )
        {
            for (auto & property: element.second)
            {
                if ("Configuration" == property.first)
                {
                    for (auto & config: property.second)
                    {
                        uint32_t frameId;
                        if(config.first == "RESUME")
                        {
                            uint8_t resumeGpioPin = config.second.get<uint8_t>("gpio.pin_num");
                            LE_INFO("resumeGpioPin is %d", resumeGpioPin);

                            uint8_t resumeGpioPinState = config.second.get<uint8_t>("gpio.state");
                            LE_INFO("resumeGpioPinState is %d", resumeGpioPinState);

                            gpio.RegisterGpioChangeCallback(resumeGpioPin,
                                    (resumeGpioPinState == 1), TAF_MNGD_PM_STATE_RESUME);
                            std::string resumeCanFrmId =
                                    config.second.get<std::string>("can.frame_id");
                            LE_INFO("resumeCanFrmId is %s", resumeCanFrmId.c_str());

                            std::string resumeCanMsg = config.second.get<std::string>("can.msg");
                            LE_INFO("resumeCanMsg is %s", resumeCanMsg.c_str());

                            frameId = std::stoul(resumeCanFrmId, nullptr, 16);
                            can.RegisterCanEvents(frameId, TAF_MNGD_PM_STATE_RESUME,
                                    resumeCanMsg.c_str());

                            std::string resumeSmsMsg = config.second.get<std::string>("SMS.text");
                            LE_INFO("resumeSmsMsg is %s", resumeSmsMsg.c_str());
                            sms.RegisterSms(resumeSmsMsg.c_str(), TAF_MNGD_PM_STATE_RESUME);
                        }
                        if(config.first == "SUSPEND")
                        {
                            uint8_t suspendGpioPin = config.second.get<uint8_t>("gpio.pin_num");
                            LE_INFO("suspendGpioPin is %d", suspendGpioPin);

                            uint8_t suspendGpioPinState = config.second.get<uint8_t>("gpio.state");
                            LE_INFO("suspendGpioPinState is %d", suspendGpioPinState);

                            gpio.RegisterGpioChangeCallback(suspendGpioPin,
                                    (suspendGpioPinState == 1), TAF_MNGD_PM_STATE_SUSPEND);

                            std::string suspendCanFrmId =
                                    config.second.get<std::string>("can.frame_id");
                            LE_INFO("suspendCanFrmId is %s", suspendCanFrmId.c_str());

                            std::string suspendCanMsg =
                                    config.second.get<std::string>("can.msg");
                            LE_INFO("suspendCanMsg is %s", suspendCanMsg.c_str());

                            frameId = std::stoul(suspendCanFrmId, nullptr, 16);
                            can.RegisterCanEvents(frameId, TAF_MNGD_PM_STATE_SUSPEND,
                                    suspendCanMsg.c_str());

                            std::string suspendSmsMsg = config.second.get<std::string>("SMS.text");
                            LE_INFO("suspendSmsMsg is %s", suspendSmsMsg.c_str());
                            sms.RegisterSms(suspendSmsMsg.c_str(),
                                    TAF_MNGD_PM_STATE_SUSPEND);
                        }
                        if(config.first == "SHUTDOWN")
                        {
                            uint8_t shutdownGpioPin = config.second.get<uint8_t>("gpio.pin_num");
                            LE_INFO("shutdownGpioPin is %d", shutdownGpioPin);

                            uint8_t shutdownGpioPinState =
                                    config.second.get<uint8_t>("gpio.state");
                            LE_INFO("shutdownGpioPinState is %d", shutdownGpioPinState);

                            gpio.RegisterGpioChangeCallback(shutdownGpioPin,
                                    (shutdownGpioPinState == 1), TAF_MNGD_PM_STATE_SHUTDOWN);

                            std::string shutdownCanFrmId =
                                    config.second.get<std::string>("can.frame_id");
                            LE_INFO("shutdownCanFrmId is %s", shutdownCanFrmId.c_str());

                            std::string shutdownCanMsg =
                                    config.second.get<std::string>("can.msg");
                            LE_INFO("shutdownCanMsg is %s", shutdownCanMsg.c_str());

                            frameId = std::stoul(shutdownCanFrmId, nullptr, 16);
                            can.RegisterCanEvents(frameId, TAF_MNGD_PM_STATE_SHUTDOWN,
                                    shutdownCanMsg.c_str());

                            std::string shutdownSmsMsg =
                                    config.second.get<std::string>("SMS.text");
                            LE_INFO("shutdownSmsMsg is %s", shutdownSmsMsg.c_str());
                            sms.RegisterSms(shutdownSmsMsg.c_str(), TAF_MNGD_PM_STATE_SHUTDOWN);
                        }
                    }
                }
            }
        }
    }
    return LE_OK;
}

/**
 * Set shutdown state to NAD
 */
le_result_t tafMngdPMSvc::ShutdownNAD()
{
    le_result_t res = taf_pm_SetAllVMPowerState(TAF_PM_STATE_SHUTDOWN);
    if(res != LE_OK)
    {
        LE_ERROR("Failed to shutdown the NAD");
    }
    else
    {
        le_hashmap_It_Ref_t hashIter =
                (le_hashmap_It_Ref_t)le_hashmap_GetIterator(vmStateHashmap);
        while (LE_OK == le_hashmap_NextNode(hashIter))
        {
            taf_mngdPm_State_t *vmStatePtr =
                    (taf_mngdPm_State_t*)le_hashmap_GetValue(hashIter);

            if(vmStatePtr) {
                vmStatePtr->state = TAF_MNGD_PM_STATE_SHUTDOWN;
            }
        }
    }
    return res;
}

/**
 * Set suspend state to NAD
 */
le_result_t tafMngdPMSvc::SuspendNAD()
{
    le_result_t res = taf_pm_SetAllVMPowerState(TAF_PM_STATE_SUSPEND);
    if(res != LE_OK)
    {
        LE_ERROR("Failed to suspend the NAD");
    }
    else
    {
        le_hashmap_It_Ref_t hashIter =
                (le_hashmap_It_Ref_t)le_hashmap_GetIterator(vmStateHashmap);
        while (LE_OK == le_hashmap_NextNode(hashIter))
        {
            taf_mngdPm_State_t *vmStatePtr =
                    (taf_mngdPm_State_t*)le_hashmap_GetValue(hashIter);

            if(vmStatePtr) {
                vmStatePtr->state = TAF_MNGD_PM_STATE_SUSPEND;
            }
        }
    }
    return res;
}

/**
 * Shutdown response callback function for VHAL module
 */
void tafMngdPMSvc::ShutdownRespCB
(
    taf_hal_pm_ShutdownMode mode,
    taf_hal_pm_RspReason reason
)
{
    LE_INFO("***** %s *****", __FUNCTION__);
    LE_INFO("taf_hal_pm_ShutdownMode: %d", mode);
    LE_INFO("taf_hal_pm_RspReason: %d", reason);
    if(le_timer_IsRunning(vhalAckTimerRef))
    {
        LE_DEBUG("Stop the timer");
        le_timer_Stop(vhalAckTimerRef);
    }
    if (mode == PM_HAL_SHUTDOWN_MODE_GRACEFUL && reason == PM_HAL_RSP_READY)
    {
        taf_pm_SendStateChangeAck(powerStateRef, TAF_PM_STATE_ALL_ACKED, TAF_PM_PVM, TAF_PM_READY);
    }
    else if (mode == PM_HAL_SHUTDOWN_MODE_GRACEFUL && reason == PM_HAL_RSP_NOT_READY)
    {
        taf_pm_SendStateChangeAck(powerStateRef, TAF_PM_STATE_ALL_ACKED, TAF_PM_PVM,
                TAF_PM_NOT_READY);
    }
    else if (mode == PM_HAL_SHUTDOWN_MODE_FORCEFUL && reason == PM_HAL_RSP_READY)
    {
        if(RequestStateChange(TAF_MNGD_PM_STATE_SHUTTING_DOWN) != LE_OK)
        {
            return;
        }

        if(shutdownCB.shutdownCallbackFunc)
        {
            shutdownCB.shutdownCallbackFunc(TAF_MNGD_PM_SYSTEM_FORCEFUL_SHUTDOWN, TAF_MNGD_PM_READY,
                    shutdownCB.shutdownCBCtxPtr);
        }
        le_result_t res = ShutdownNAD();
        if(res == LE_OK)
        {
            powerMode.isGraceful = false;
        }
    }
    else if (mode == PM_HAL_SHUTDOWN_MODE_FORCEFUL && reason == PM_HAL_RSP_NOT_READY)
    {
        if(shutdownCB.shutdownCallbackFunc)
        {
            shutdownCB.shutdownCallbackFunc(
                TAF_MNGD_PM_SYSTEM_FORCEFUL_SHUTDOWN,
                TAF_MNGD_PM_NOT_READY,
                shutdownCB.shutdownCBCtxPtr);
        }
    }
    shutdownCB.shutdownCallbackFunc = nullptr;
}

/**
 * Shutdown last response callback function from VHAL
 */
void tafMngdPMSvc::ShutdownCmdCB
(
    taf_hal_pm_ShutdownMode mode,
    taf_hal_pm_RspReason reason
)
{
    LE_INFO("***** %s *****", __FUNCTION__);
    LE_INFO("taf_hal_pm_ShutdownMode: %d", mode);
    LE_INFO("taf_hal_pm_RspReason: %d", reason);
    powerMode.isRestart = false;
    if (mode == PM_HAL_SHUTDOWN_MODE_FORCEFUL && reason == PM_HAL_RSP_READY)
    {
        taf_pm_SendStateChangeAck(powerStateRef, TAF_PM_STATE_ALL_ACKED, TAF_PM_PVM,
                TAF_PM_READY);
    }
    else if (mode == PM_HAL_SHUTDOWN_MODE_FORCEFUL && reason == PM_HAL_RSP_NOT_READY)
    {
        taf_pm_SendStateChangeAck(powerStateRef, TAF_PM_STATE_ALL_ACKED, TAF_PM_PVM,
                TAF_PM_NOT_READY);
    }
}

/**
 * Suspend response callback function for VHAL module
 */
void tafMngdPMSvc::SuspendRespCB
(
    taf_hal_pm_SuspendMode mode,
    taf_hal_pm_RspReason reason
)
{
    LE_INFO("***** %s *****", __FUNCTION__);
    LE_INFO("taf_hal_pm_SuspendMode: %d", mode);
    LE_INFO("taf_hal_pm_RspReason: %d", reason);
    powerMode.isSuspend = false;
    if (mode == PM_HAL_SUSPEND_MODE_FULL && reason == PM_HAL_RSP_READY)
    {
        taf_pm_SendStateChangeAck(powerStateRef, TAF_PM_STATE_ALL_ACKED, TAF_PM_PVM,
                TAF_PM_READY);
    }
    else if (mode == PM_HAL_SUSPEND_MODE_FULL && reason == PM_HAL_RSP_NOT_READY)
    {
        taf_pm_SendStateChangeAck(powerStateRef, TAF_PM_STATE_ALL_ACKED, TAF_PM_PVM,
                TAF_PM_NOT_READY);
    }
}

/**
 * Restart response callback function for VHAL module
 */
void tafMngdPMSvc::RestartRespCB
(
    taf_hal_pm_RestartMode mode,
    taf_hal_pm_RspReason reason
)
{
    LE_INFO("***** %s *****", __FUNCTION__);
    LE_INFO("taf_hal_pm_RestartMode: %d", mode);
    LE_INFO("taf_hal_pm_RspReason: %d", reason);
    if(le_timer_IsRunning(vhalAckTimerRef))
    {
        LE_INFO("vhalAckTimerRef");
        LE_DEBUG("Stop the timer");
        le_timer_Stop(vhalAckTimerRef);
    }
    if (mode == PM_HAL_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF && reason == PM_HAL_RSP_READY)
    {
        if(RequestStateChange(TAF_MNGD_PM_STATE_SHUTTING_DOWN) != LE_OK)
        {
            if(restartCB.restartCallbackFunc)
            {
                restartCB.restartCallbackFunc(TAF_MNGD_PM_RESTART_SYSTEM_OFF_ON, TAF_MNGD_PM_NOT_READY,
                        restartCB.restartCBCtxPtr);
            }
            return;
        }

        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGD_PM_RESTART_SYSTEM_OFF_ON, TAF_MNGD_PM_READY,
                    restartCB.restartCBCtxPtr);
        }
        le_result_t res = ShutdownNAD();
        if(res == LE_OK)
        {
            powerMode.isGraceful = false;
        }
    }
    else if (mode == PM_HAL_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF && reason == PM_HAL_RSP_NOT_READY)
    {
        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGD_PM_RESTART_SYSTEM_OFF_ON, TAF_MNGD_PM_NOT_READY,
                    restartCB.restartCBCtxPtr);
        }
    }
    restartCB.restartCallbackFunc = nullptr;
}

/**
 * Node state change callback function for VHAL module
 */
void tafMngdPMSvc::NodeStateChangeNotificationCB
(
    uint8_t pm_node_id,
    taf_hal_pm_NodeState state,
    taf_hal_pm_ConfirmStatus status
)
{
    LE_INFO("***** %s *****", __FUNCTION__);
    LE_INFO("pm_node_id: %d", pm_node_id);
    LE_INFO("taf_hal_pm_NodeState: %d", state);
    LE_INFO("taf_hal_pm_ConfirmStatus: %d", status);
    if (state == PM_HAL_NODE_STATE_SHUTDOWN && status == PM_HAL_NODE_STATUS_READY)
    {
        taf_pm_SendStateChangeAck(powerStateRef, TAF_PM_STATE_SHUTDOWN, TAF_PM_PVM, TAF_PM_READY);
    }
    else if (state == PM_HAL_NODE_STATE_SHUTDOWN && status == PM_HAL_NODE_STATUS_NOT_READY)
    {
        taf_pm_SendStateChangeAck(powerStateRef, TAF_PM_STATE_SHUTDOWN, TAF_PM_PVM,
                TAF_PM_NOT_READY);
    }
    else if (state == PM_HAL_NODE_STATE_SUSPEND && status == PM_HAL_NODE_STATUS_READY)
    {
        taf_pm_SendStateChangeAck(powerStateRef, TAF_PM_STATE_SUSPEND, TAF_PM_PVM, TAF_PM_READY);
    }
    else if (state == PM_HAL_NODE_STATE_SUSPEND && status == PM_HAL_NODE_STATUS_NOT_READY)
    {
        taf_pm_SendStateChangeAck(powerStateRef, TAF_PM_STATE_SUSPEND, TAF_PM_PVM,
                TAF_PM_NOT_READY);
    }
}

/**
 * Node event callback function for VHAL module
 */
void tafMngdPMSvc::NodeEventCB
(
    uint8_t pm_node_id,
    const char* pm_node_event_info
)
{
    auto &mpms = tafMngdPMSvc::GetInstance();

    LE_INFO("NodeEventCB for node %d with node info %s", pm_node_id, pm_node_event_info);
    le_result_t res = LE_FAULT;
    if(strncmp(pm_node_event_info, RELAX, strlen(RELAX)) == 0)
    {
        if(tafMngdPMSvc::RequestStateChange(TAF_MNGD_PM_STATE_RELEASING_WAKE_SOURCE) != LE_OK)
        {
            return;
        }

        if(mpms.wsCount > 0) {
            mpms.wsCount--;
            res = tafMngdPMSvc::ReleaseWakeLock();
            if(res == LE_OK)
            {
                LE_INFO(" ReleaseWakeLock successfull");
                tafMngdPMSvc::ProcessStateChange(TAF_MNGD_PM_STATE_RELEASING_WAKE_SOURCE);
            }
        }
    }
    else if(strncmp(pm_node_event_info, STAYAWAKE, strlen(STAYAWAKE)) == 0)
    {
        if(tafMngdPMSvc::RequestStateChange(TAF_MNGD_PM_STATE_WAKING_UP) != LE_OK)
        {
            return;
        }

        mpms.wsCount++;
        res = tafMngdPMSvc::AcquireWakeLock();
        if(res == LE_OK)
        {
            LE_INFO(" AcquireWakeLock successfull");
            tafMngdPMSvc::ProcessStateChange(TAF_MNGD_PM_STATE_WAKING_UP);
        }
    }
    else if(strncmp(pm_node_event_info, SHUTDOWN, strlen(SHUTDOWN)) == 0)
    {
        LE_INFO("SHUTDOWN from VHAL");

        if(tafMngdPMSvc::RequestStateChange(TAF_MNGD_PM_STATE_SHUTTING_DOWN) != LE_OK)
        {
            return;
        }
    }
    else
    {
        LE_INFO("Received not supported mode from VHAL");
    }
}

/**
 * Client connection callback function
 */
void tafMngdPMSvc::OnClientConnection(le_msg_SessionRef_t sessionRef, void *ctxPtr)
{
    LE_DEBUG("OnClientConnection");

    pid_t pid;
    char appName[LIMIT_MAX_PATH_BYTES] = {0};

    if (LE_OK != le_msg_GetClientProcessId(sessionRef, &pid))
    {
        LE_ERROR("Error, Failed to get client pid.");
    }

    bool inTheWhiteList = false;
    if (le_appInfo_GetName(pid, appName, sizeof(appName)) == LE_OK)
    {
        LE_INFO("client appName: %s", appName);

        for(uint i = 0; i < sizeof(clientWhiteList) / sizeof(clientWhiteList[0]); i++)
        {
            if(strcmp(appName, clientWhiteList[i]) == 0)
            {
                LE_INFO("app is in the client white list");
                inTheWhiteList = true;
                break;
            }
        }
    }

    taf_mngdPm_SessionNode_t* sessionNodePtr = nullptr;

    if(inTheWhiteList)
    {
        sessionNodePtr =
            (taf_mngdPm_SessionNode_t*)le_mem_ForceAlloc(mngdPmClientInfo.SessionNodePool);
    }
    else
    {
        sessionNodePtr =
            (taf_mngdPm_SessionNode_t*)le_mem_TryAlloc(mngdPmClientInfo.SessionNodePool);
    }

    TAF_ERROR_IF_RET_NIL(sessionNodePtr == nullptr, "Cannot allocate sessionNode");

    sessionNodePtr->sessionRef = sessionRef;

    if (LE_OK != le_msg_GetClientProcessId(sessionRef, &sessionNodePtr->pid))
    {
        LE_ERROR("Error, Failed to get client pid.");
    }

    // update client record in table
    if (le_hashmap_Put(mngdPmClientInfo.clients, sessionRef, sessionNodePtr))
    {
        LE_ERROR("Failed to add client record for session %p.", sessionRef);
    }

    LE_INFO("Session %p (process %d) connected", sessionRef, sessionNodePtr->pid);
}

/**
 * Client disconnection callback function
 */
void tafMngdPMSvc::OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *ctxPtr)
{
    LE_DEBUG("OnClientDisconnection");

    taf_mngdPm_SessionNode_t* sessionNodePtr =
            (taf_mngdPm_SessionNode_t*)le_hashmap_Remove(mngdPmClientInfo.clients, sessionRef);

    TAF_ERROR_IF_RET_NIL(sessionNodePtr == nullptr, "Failed to remove sessionRef %p from table",
                         sessionRef);
    if(restartCB.sessionRef == sessionRef)
         restartCB.restartCallbackFunc = nullptr;
    if(shutdownCB.sessionRef == sessionRef)
        shutdownCB.shutdownCallbackFunc = nullptr;
    LE_INFO("Client with sessionRef %p (process %d) disconnected", sessionRef, sessionNodePtr->pid);

    le_mem_Release(sessionNodePtr);
}

/**
 * Check whether the client session is valid or not
 */
bool tafMngdPMSvc::IsClientValid()
{
    le_msg_SessionRef_t sessionRef = taf_mngd_pm_GetClientSessionRef();

    taf_mngdPm_SessionNode_t* sessionNodePtr =
            (taf_mngdPm_SessionNode_t*)le_hashmap_Get(mngdPmClientInfo.clients, sessionRef);

    TAF_ERROR_IF_RET_VAL(sessionNodePtr == nullptr,
                            false,
                            "cannot find session %p from table", sessionRef);

    pid_t pid;

    TAF_ERROR_IF_RET_VAL(LE_OK != le_msg_GetClientProcessId(sessionRef, &pid),
                            false,
                            "cannot get process id for session %p", sessionRef);

    TAF_ERROR_IF_RET_VAL(pid != sessionNodePtr->pid,
                            false,
                            "pid mismatched");

    return true;
}

/**
 * State change callback function for PM service
 */
void tafMngdPMSvc::StateChangeHandler(taf_pm_State_t state, void* contextPtr)
{
    LE_INFO("State change triggered for %s\n", TafStateToString((taf_mngd_pm_State_t)state));

    // Update the VMs data on receiving state change caused by any other sources like SMS, CAN
    le_hashmap_It_Ref_t hashIter = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(vmStateHashmap);
    while (LE_OK == le_hashmap_NextNode(hashIter))
    {
        taf_mngdPm_State_t *vmStatePtr = (taf_mngdPm_State_t*)le_hashmap_GetValue(hashIter);
        if(vmStatePtr) {
            vmStatePtr->state = (taf_mngd_pm_State_t)state;
        }
    }
}

/**
 * Ex State change callback function for PM service
 */
void tafMngdPMSvc::StateChangeExHandler(taf_pm_PowerStateRef_t psRef,
        taf_pm_NadVm_t vm_id, taf_pm_State_t state, void* contextPtr)
{
    LE_INFO("State change triggered in StateChangeExHandler");
    powerStateRef = psRef;
    if(state != TAF_PM_STATE_ALL_WAKELOCKS_RELEASED && !pmInf)
    {
        if(state == TAF_PM_STATE_SUSPEND)
        {
            ProcessStateChange(TAF_MNGD_PM_STATE_SUSPEND);
        }
        else if(state == TAF_PM_STATE_SHUTDOWN)
        {
            ProcessStateChange(TAF_MNGD_PM_STATE_SHUTDOWN);
        }
        else if(state == TAF_PM_STATE_RESUME)
        {
            ProcessStateChange(TAF_MNGD_PM_STATE_RESUME);
        }

        // Send ACK if no VHAL driver is available.
        LE_DEBUG("Send ACK if there is no driver loaded");
        taf_pm_SendStateChangeAck(powerStateRef, state, vm_id, TAF_PM_READY);
        return;
    }
    if(state == TAF_PM_STATE_ALL_WAKELOCKS_RELEASED)
    {
        LE_DEBUG("Received all wakelocks released notification");
        le_result_t res;
        if(targetedPowerMode == TAF_MNGD_PM_SHUTDOWN)
        {
            res = ShutdownNAD();
            if(res == LE_OK) {
                powerMode.isGraceful = true;
                ProcessStateChange(TAF_MNGD_PM_STATE_SHUTTING_DOWN);
            }
        }
        else if(wsCount == 0)
        {
            res = SuspendNAD();
            if(res == LE_OK) {
                powerMode.isSuspend = true;
                ProcessStateChange(TAF_MNGD_PM_STATE_SUSPENDING);
                LE_INFO("Suspend triggered for NAD on wakelocks released");
            }
        }
    }
    else if (state == TAF_PM_STATE_ALL_ACKED)
    {
        if(powerMode.isGraceful)
        {
            LE_DEBUG("Send shutdownReqAsync %d", PM_HAL_SHUTDOWN_MODE_GRACEFUL);
            (*(pmInf->shutdownReqAsync))(PM_HAL_SHUTDOWN_MODE_GRACEFUL, ShutdownRespCB);
        }
        else if(powerMode.isRestart)
        {
            LE_DEBUG("Send shutdownReqAsync %d", PM_HAL_SHUTDOWN_MODE_FORCEFUL);
            (*(pmInf->shutdownReqAsync))(PM_HAL_SHUTDOWN_MODE_FORCEFUL, ShutdownCmdCB);
        }
        else if(powerMode.isSuspend)
        {
            LE_DEBUG("Send SuspendReqAsync %d", PM_HAL_SUSPEND_MODE_FULL);
            (*(pmInf->suspendReqAsync))(PM_HAL_SUSPEND_MODE_FULL, SuspendRespCB);
        }
        else
        {
            taf_pm_SendStateChangeAck(powerStateRef, TAF_PM_STATE_ALL_ACKED, TAF_PM_PVM,
                    TAF_PM_READY);
        }
    }
    else if(state == TAF_PM_STATE_SUSPEND)
    {
        ProcessStateChange(TAF_MNGD_PM_STATE_SUSPEND);

        LE_DEBUG("Send state change notification %d", PM_HAL_NODE_STATE_SUSPEND);
        (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, PM_HAL_NODE_STATE_SUSPEND,
                NodeStateChangeNotificationCB);
    }
    else if(state == TAF_PM_STATE_SHUTDOWN)
    {
        ProcessStateChange(TAF_MNGD_PM_STATE_SHUTDOWN);

        LE_DEBUG("Send state change notification %d", PM_HAL_NODE_STATE_SHUTDOWN);
        (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, PM_HAL_NODE_STATE_SHUTDOWN,
                NodeStateChangeNotificationCB);
    }
    else if(state == TAF_PM_STATE_RESUME)
    {
        ProcessStateChange(TAF_MNGD_PM_STATE_RESUME);

        LE_DEBUG("Send state change notification %d", PM_HAL_NODE_STATE_RESUME);
        (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, PM_HAL_NODE_STATE_RESUME,
                NodeStateChangeNotificationCB);
    }
}

/**
 * VHAL ack timer handler
 */
void tafMngdPMSvc::VhalAckTimerHandler(le_timer_Ref_t timerRef)
{
    taf_mngdPm_RequestedState_t* state =
      (taf_mngdPm_RequestedState_t*)le_timer_GetContextPtr(timerRef);
    LE_INFO("Timer Expired state is %d", *(state));

    if(*(state) == SYSTEM_FORCEFUL_SHUTDOWN)
    {
        LE_INFO("Timer expire for SYSTEM_FORCEFUL_SHUTDOWN");
        if(shutdownCB.shutdownCallbackFunc)
        {
            shutdownCB.shutdownCallbackFunc(TAF_MNGD_PM_SYSTEM_FORCEFUL_SHUTDOWN, TAF_MNGD_PM_TIMEOUT,
                    shutdownCB.shutdownCBCtxPtr);
        }
    }
    else if (*(state) == RESTART_WITH_NAD_POWER_OFF_ON)
    {
            LE_INFO("Timer expire for TAF_MNGD_PM_RESTART_SYSTEM_OFF_ON");
        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGD_PM_RESTART_SYSTEM_OFF_ON, TAF_MNGD_PM_TIMEOUT,
                    restartCB.restartCBCtxPtr);
        }
    }
}

/**
 * Timer to wait wakesource request from apps
 */
void tafMngdPMSvc::WaitWakeSourceTimer()
{
    LE_INFO("WaitWakeSourceTimer");
    le_result_t res;
    //timer to wait for wake source from apps
    wakeSourceTimerRef = le_timer_Create("WAKE SOURCE timer");
    le_timer_SetMsInterval(wakeSourceTimerRef, VHAL_WAKESOURCE_TIMEOUT);
    le_timer_SetHandler(wakeSourceTimerRef, WakeSourceTimerHandler);
    //acquire wakesource
    wsCount++;
    res = AcquireWakeLock();
    if(res == LE_OK) {
        LE_INFO("acquired wake lock after init");
    }
    le_timer_Start(wakeSourceTimerRef);
    LE_INFO("Started timer for wakesource request from apps");
}

/**
 * Timer handler for wakesource request
 */
void tafMngdPMSvc::WakeSourceTimerHandler(le_timer_Ref_t timerRef)
{
    LE_INFO("Timer Expired for WakeSourceTimerHandler");
    wsCount--;
    if(wsCount == 0)
    {
        le_result_t res = ReleaseWakeLock();
        if(res == LE_OK)
            LE_INFO("Triggered suspend after 10sec wait to acquire wakesource from apps");
    }
}

/**
 * Initialize PM VHAL module
 */
le_result_t tafMngdPMSvc::InitVHalModule()
{
    // Load the driver and does not care the version
    pmInf = (pm_Inf_t *)taf_devMgr_LoadDrv(TAF_PM_MODULE_NAME, nullptr);

    if(pmInf == nullptr)
    {
        LE_ERROR("Can not load the driver %s", TAF_PM_MODULE_NAME);
        return LE_FAULT;
    }
    else // successfully loaded
    {
        LE_INFO("Loaded module %s successfully", TAF_PM_MODULE_NAME);
        LE_DEBUG("Call pmInf(%p) init function", pmInf);

        // init first
        (*(pmInf->InitHAL))();
        vhalAckTimerRef = le_timer_Create("VHAL ACK timer");
        le_timer_SetMsInterval(vhalAckTimerRef, VHAL_ACK_TIMEOUT);
        le_timer_SetHandler(vhalAckTimerRef, VhalAckTimerHandler);
    }

    return LE_OK;
}

/**
 * Acquire wakesource and let system stay awake
 */
le_result_t tafMngdPMSvc::AcquireWakeLock()
{
    LE_INFO("AcquireWakeLock wsCount:%d", wsCount);
    if(le_timer_IsRunning(wakeSourceTimerRef))
    {
        LE_INFO("acquired wake source when timer is running");
        return LE_OK;
    }
    le_result_t res;
    if(wsCount > 0 ) {
        if(ws == nullptr)
            ws = taf_pm_NewWakeupSource(WAKELOCK_WITHOUT_REF, "mpms");
        if (ws != nullptr && !powerMode.isWsAcquired)
        {
            res = taf_pm_StayAwake(ws);
            if(res == LE_OK) {
                LE_INFO("Wake source from PM acquired successfully");
                powerMode.isWsAcquired = true;
                return res;
            }
            else
                LE_INFO("failed to acquire ws");
        }
        else
        {
            LE_ERROR("Failed to create wakeup source!");
        }
    }
    return LE_FAULT;
}

/**
 * Release wakesource
 */
le_result_t tafMngdPMSvc::ReleaseWakeLock()
{
    LE_INFO("ReleaseWakeLock wsCount:%d", wsCount);

    le_result_t res = LE_FAULT;
    if(wsCount == 0 ) {
        if (ws != nullptr && powerMode.isWsAcquired)
        {
            res = taf_pm_Relax(ws);
            if(res == LE_OK) {
                LE_INFO("Wake source released successfully");
                powerMode.isWsAcquired = false;
            }
        }
        else
        {
            LE_ERROR("Failed to release wakeup lock!");
        }
    }
    return res;
}

/**
 * Set modem wake up source
 */
void tafMngdPMSvc::SetModemWakeupSource(taf_mngd_pm_WakeupType_t wakeupType)
{
    LE_INFO("SetModemWakeupSource");
    //check if same wakeupSource exists
    if(wsWhiteList.size() > 0)
    {
        for (auto it = wsWhiteList.begin(); it != wsWhiteList.end(); ++it )
        {
            if (*it == wakeupType)
            {
                LE_INFO("wakeupType already in wsWhiteList");
                return ;
            }
        }
    }
    LE_INFO("SetModemWakeupSource type %d", wakeupType);
    wsWhiteList.push_back(wakeupType);
}

/**
 * Request MPMS state change condition check
 */
le_result_t tafMngdPMSvc::RequestStateChange(taf_mngd_pm_State_t requestedState)
{
    LE_INFO("current state %s", TafStateToString(stateMachine.currentState));
    LE_INFO("requested state %s", TafStateToString(requestedState));

    le_result_t res = LE_OK;

    switch(requestedState)
    {
        case TAF_MNGD_PM_STATE_RELEASING_WAKE_SOURCE:
            if(stateMachine.currentState != TAF_MNGD_PM_STATE_RELEASING_WAKE_SOURCE &&
               stateMachine.currentState != TAF_MNGD_PM_STATE_RESUME)
            {
                res = LE_NOT_PERMITTED;
            }
            break;

        case TAF_MNGD_PM_STATE_SUSPENDING:
            if(stateMachine.currentState != TAF_MNGD_PM_STATE_RELEASING_WAKE_SOURCE &&
               stateMachine.currentState != TAF_MNGD_PM_STATE_RESUME)
            {
                res = LE_NOT_PERMITTED;
            }
            break;

        case TAF_MNGD_PM_STATE_SHUTTING_DOWN:
            if(stateMachine.currentState != TAF_MNGD_PM_STATE_RELEASING_WAKE_SOURCE &&
               stateMachine.currentState != TAF_MNGD_PM_STATE_RESUME)
            {
                res = LE_NOT_PERMITTED;
            }
            break;

        case TAF_MNGD_PM_STATE_WAKING_UP:
            if(stateMachine.currentState == TAF_MNGD_PM_STATE_SUSPENDING &&
               stateMachine.currentState == TAF_MNGD_PM_STATE_SHUTTING_DOWN)
            {
                res = LE_NOT_PERMITTED;
            }
            break;

        default:
            res = LE_NOT_PERMITTED;
            break;
    }

    if(res == LE_NOT_PERMITTED)
    {
        LE_INFO("RequestStateChange NOT PERMITTED");
        return res;
    }

    LE_INFO("RequestStateChange OK");
    return LE_OK;
}

/**
 * Process MPMS state change
 */
void tafMngdPMSvc::ProcessStateChange(taf_mngd_pm_State_t toState)
{
    LE_INFO("current state %s", TafStateToString(stateMachine.currentState));

    switch(toState)
    {
        case TAF_MNGD_PM_STATE_WAKING_UP:
            if(stateMachine.currentState == TAF_MNGD_PM_STATE_RESUME)
            {
                // No need to change state from RESUME to WAKING_UP
                toState = TAF_MNGD_PM_STATE_RESUME;
            }
            break;

        default:
            break;
    }

    if(toState != stateMachine.currentState)
    {
        stateMachine.currentState = toState;

        taf_mngd_pm_StateInd_t stateInd;
        stateInd.state = toState;

        le_event_Report(stateChange, &stateInd, sizeof(taf_mngd_pm_StateInd_t));
    }

    LE_INFO("change to state %s", TafStateToString(toState));
}

/**
 * State change layered handler function
 */
void tafMngdPMSvc::StateLayeredHandler(void* reportPtr, void* layerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(layerHandlerFunc == nullptr, "Null ptr(layerHandlerFunc)");

    taf_mngd_pm_StateChangeHandlerFunc_t handlerFunc =
        (taf_mngd_pm_StateChangeHandlerFunc_t)layerHandlerFunc;

    handlerFunc((taf_mngd_pm_StateInd_t*)reportPtr, le_event_GetContextPtr());
}

/**
 * Get MPMS instance
 */
tafMngdPMSvc &tafMngdPMSvc::GetInstance()
{
   static tafMngdPMSvc instance;
   return instance;
}

/**
 * tafMngdPMSvc initialization
 */
void tafMngdPMSvc::Init(void)
{
}

/**
 * initialize static variables
 */
le_mem_PoolRef_t tafMngdPMSvc::vmStatePool;
le_hashmap_Ref_t tafMngdPMSvc::vmStateHashmap;
le_mem_PoolRef_t tafMngdPMSvc::wsRefPool;
le_dls_List_t tafMngdPMSvc::wsRefList;
le_ref_MapRef_t tafMngdPMSvc::wsRefMap;

taf_pm_StateChangeHandlerRef_t tafMngdPMSvc::handlerRef = nullptr;
taf_pm_StateChangeExHandlerRef_t tafMngdPMSvc::handlerExRef = nullptr;
taf_pm_PowerStateRef_t tafMngdPMSvc::powerStateRef = nullptr;
taf_pm_WakeupSourceRef_t tafMngdPMSvc::ws = nullptr;

taf_mngd_pm_TargetedPowerMode_t tafMngdPMSvc::targetedPowerMode = TAF_MNGD_PM_RESUME;
taf_mngdPm_RestartCb_t tafMngdPMSvc::restartCB;
taf_mngdPm_ShutdownCb_t tafMngdPMSvc::shutdownCB;
std::vector<taf_mngd_pm_WakeupType_t> tafMngdPMSvc::wsWhiteList;

uint8_t tafMngdPMSvc::wsCount = 0;
taf_powerMode_t tafMngdPMSvc::powerMode{};
taf_stateMachine_t tafMngdPMSvc::stateMachine{};

pm_Inf_t* tafMngdPMSvc::pmInf = nullptr;
le_timer_Ref_t tafMngdPMSvc::vhalAckTimerRef = nullptr;
le_timer_Ref_t tafMngdPMSvc::wakeSourceTimerRef = nullptr;

taf_mngdPm_RequestedState_t tafMngdPMSvc::statePtr;

taf_mngdPm_Client_t tafMngdPMSvc::mngdPmClientInfo;
const char* tafMngdPMSvc::clientWhiteList[] = {"tafMngdPMIntTest","tafMngdPMUnitTest"};

le_event_Id_t tafMngdPMSvc::stateChange;
