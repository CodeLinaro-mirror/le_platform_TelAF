/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#define globalState "ALL"

using namespace telux::tafsvc;
namespace pt = boost::property_tree;
static le_mem_PoolRef_t vmStatePool;
static le_hashmap_Ref_t vmStateHashmap;
le_thread_Ref_t SetStateResponseEventThreadRef = NULL;
le_thread_Ref_t StateChangeEventThreadRef = NULL;
taf_pm_StateChangeHandlerRef_t handlerRef;
taf_pm_StateChangeExHandlerRef_t handlerExRef;
taf_mngd_pm_TargetedPowerMode_t targetedPowerMode = TAF_MNGD_PM_RESUME;
taf_pm_PowerStateRef_t powerStateRef;
taf_mngdPm_RestartCb_t restartCB;
taf_mngdPm_ShutdownCb_t shutdownCB;
le_timer_Ref_t vhalAckTimerRef;
taf_pm_WakeupSourceRef_t ws = nullptr;
bool isGraceful = false;
static pm_Inf_t *pmInf = nullptr;
taf_mngdPm_RequestedState_t statePtr;
taf_mngdPm_Client_t mngdPmClientInfo;
LE_MEM_DEFINE_STATIC_POOL(SessionCtx, MAX_SESSION, sizeof(taf_mngdPm_SessionNode_t));
const char* clientWhiteList[] = {"tafMngdPMIntTest","tafMngdPMUnitTest"};

le_result_t shutdownNAD()
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

void shutdownRespCB
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
        if(shutdownCB.shutdownCallbackFunc)
        {
            shutdownCB.shutdownCallbackFunc(TAF_MNGD_PM_SYSTEM_FORCEFUL_SHUTDOWN, TAF_MNGD_PM_READY,
                    shutdownCB.shutdownCBCtxPtr);
        }
        le_result_t res = shutdownNAD();
        if(res == LE_OK)
        {
            isGraceful = false;
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

void restartRespCB
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
        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGD_PM_RESTART_SYSTEM_OFF_ON, TAF_MNGD_PM_READY,
                    restartCB.restartCBCtxPtr);
        }
        le_result_t res = shutdownNAD();
        if(res == LE_OK)
        {
            isGraceful = false;
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

void nodeStateChangeNotificationCB
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
 * To convert TafState to string
 */
const char* tafMngdPMSvc::tafStateToString(taf_mngd_pm_State_t tafState)
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
        default :
            state = "Unknown";
            break;
    }
    return state;
}

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

le_result_t taf_mngd_pm_SetNadPowerState(taf_mngd_pm_Nad_t nad, taf_mngd_pm_State_t state)
{
    LE_DEBUG("taf_mngd_pm_setNadPowerState state : %s", tafMngdPMSvc::tafStateToString(state));

    le_result_t res = tafMngdPMSvc::IsClientValid();
    if(res != LE_OK)
    {
        return res;
    }

    res = taf_pm_SetAllVMPowerState((taf_pm_State_t)state);
    if(res != LE_OK)
    {
        LE_ERROR("Failed to set the NAD power state");
    }
    else
    {
        le_hashmap_It_Ref_t hashIter = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(vmStateHashmap);
        while (LE_OK == le_hashmap_NextNode(hashIter))
        {
            taf_mngdPm_State_t *vmStatePtr = (taf_mngdPm_State_t*)le_hashmap_GetValue(hashIter);

            if(vmStatePtr) {
                vmStatePtr->state = state;
            }
        }
    }
    return res;
}

le_result_t taf_mngd_pm_SetVMPowerState(taf_mngd_pm_Nad_t nad, const char *machineName,
        taf_mngd_pm_State_t state)
{
    LE_DEBUG("taf_mngd_pm_SetVMPowerState VM : %s state : %s", machineName,
            tafMngdPMSvc::tafStateToString(state));

    le_result_t res = tafMngdPMSvc::IsClientValid();
    if(res != LE_OK)
    {
        return res;
    }

    TAF_ERROR_IF_RET_VAL(state == TAF_MNGD_PM_STATE_UNKNOWN || machineName == NULL,
            LE_BAD_PARAMETER, "state or machineName is not valid");

    char name[32] = {0};
    taf_pm_VMListRef_t vmListRef = taf_pm_GetMachineList( );

    TAF_ERROR_IF_RET_VAL(!vmListRef, LE_UNSUPPORTED, "Machine list not available");

    res = taf_pm_GetFirstMachineName(vmListRef, name, 32);
    while(res == LE_OK)
    {
        if(machineName && strncmp(machineName, name, TAF_MNGD_PM_MACHINE_NAME_LEN) == 0)
        {
            taf_mngdPm_State_t *vmStatePtr =
                    (taf_mngdPm_State_t*)le_hashmap_Get(vmStateHashmap, name);
            TAF_ERROR_IF_RET_VAL(!vmStatePtr, LE_BAD_PARAMETER, "Machine name not available");

            // return LE_BAD_PARAMETER when received request for the same current power state.
            TAF_ERROR_IF_RET_VAL(vmStatePtr->state == state, LE_BAD_PARAMETER,
                    "Requested the already existing state");

            res = taf_pm_SetVMPowerState((taf_pm_State_t)state, machineName);
            if(res != LE_OK)
            {
                LE_ERROR("Failed to trigger state change of %s VM", machineName);
                return res;
            }
            else
            {
                LE_INFO("Successfully requested state change for %s VM", machineName);
                taf_pm_DeleteMachineList(vmListRef);
                le_hashmap_It_Ref_t hashIter =
                        (le_hashmap_It_Ref_t)le_hashmap_GetIterator(vmStateHashmap);
                while (LE_OK == le_hashmap_NextNode(hashIter))
                {
                    taf_mngdPm_State_t *vmStatePtr =
                            (taf_mngdPm_State_t*)le_hashmap_GetValue(hashIter);
                    if(vmStatePtr)
                    {
                        if(strncmp(vmStatePtr->vmName, machineName,
                                TAF_MNGD_PM_MACHINE_NAME_LEN) == 0)
                        {
                            LE_DEBUG("Update %s VM state to %s", vmStatePtr->vmName,
                                    tafMngdPMSvc::tafStateToString(state));
                            vmStatePtr->state = state;
                        }
                    }
                }
                return res;
            }
        }
        res = taf_pm_GetNextMachineName(vmListRef, name, 32);
    }
    taf_pm_DeleteMachineList(vmListRef);
    LE_ERROR("machineName %s provided is not available", machineName);
    return LE_BAD_PARAMETER;
}

le_result_t taf_mngd_pm_GetVMPowerState(taf_mngd_pm_Nad_t nad, const char *machineName,
        taf_mngd_pm_State_t *state)
{
    LE_DEBUG("taf_mngd_pm_GetVMPowerState %s state", machineName);

    le_result_t res = tafMngdPMSvc::IsClientValid();
    if(res != LE_OK)
    {
        return res;
    }

    TAF_ERROR_IF_RET_VAL(state == NULL || machineName == NULL, LE_BAD_PARAMETER,
            "state or machineName is not valid");

    le_hashmap_It_Ref_t hashIter = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(vmStateHashmap);
    while (LE_OK == le_hashmap_NextNode(hashIter))
    {
        taf_mngdPm_State_t *vmStatePtr = (taf_mngdPm_State_t*)le_hashmap_GetValue(hashIter);

        if(vmStatePtr)
        {
            if(strncmp(vmStatePtr->vmName, machineName, TAF_MNGD_PM_MACHINE_NAME_LEN) == 0)
            {
                LE_DEBUG("Update %s state", vmStatePtr->vmName);
                *state = vmStatePtr->state;
                return LE_OK;
            }
        }
    }
    LE_ERROR("machineName %s provided is not available", machineName);
    return LE_BAD_PARAMETER;
}

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

le_result_t tafMngdPMSvc::IsClientValid()
{
    le_msg_SessionRef_t sessionRef = taf_mngd_pm_GetClientSessionRef();

    taf_mngdPm_SessionNode_t* sessionNodePtr =
            (taf_mngdPm_SessionNode_t*)le_hashmap_Get(mngdPmClientInfo.clients, sessionRef);

    TAF_ERROR_IF_RET_VAL(sessionNodePtr == nullptr,
                            LE_UNSUPPORTED,
                            "cannot find session %p from table", sessionRef);

    pid_t pid;

    TAF_ERROR_IF_RET_VAL(LE_OK != le_msg_GetClientProcessId(sessionRef, &pid),
                            LE_UNSUPPORTED,
                            "cannot get process id for session %p", sessionRef);

    TAF_ERROR_IF_RET_VAL(pid != sessionNodePtr->pid,
                            LE_UNSUPPORTED,
                            "pid mismatched");

    return LE_OK;
}

void tafMngdPMSvc::StateChangeHandler(taf_pm_State_t state, void* contextPtr)
{
    LE_INFO("State change triggered for %s\n", tafStateToString((taf_mngd_pm_State_t)state));

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

void tafMngdPMSvc:: StateChangeExHandler(taf_pm_PowerStateRef_t psRef,
        taf_pm_NadVm_t vm_id, taf_pm_State_t state, void* contextPtr)
{
    LE_INFO("State change triggered in StateChangeExHandler");
    powerStateRef = psRef;
    if(state != TAF_PM_STATE_ALL_WAKELOCKS_RELEASED && !pmInf)
    {
        // Send ACK if no VHAL driver is available.
        LE_DEBUG("Send ACK if there is no driver loaded");
        taf_pm_SendStateChangeAck(powerStateRef, state, vm_id, TAF_PM_READY);
        return;
    }
    if(state == TAF_PM_STATE_ALL_WAKELOCKS_RELEASED)
    {
        LE_DEBUG("Received all wakelocks released notification");
        if(targetedPowerMode == TAF_MNGD_PM_SHUTDOWN)
        {
            le_result_t res = shutdownNAD();
            if(res == LE_OK) {
                isGraceful = true;
            }
        }
    }
    else if (state == TAF_PM_STATE_ALL_ACKED)
    {
        if(isGraceful)
        {
                LE_DEBUG("Send shutdownReqAsync %d", PM_HAL_SHUTDOWN_MODE_GRACEFUL);
                (*(pmInf->shutdownReqAsync))(PM_HAL_SHUTDOWN_MODE_GRACEFUL, shutdownRespCB);
        }
        else
        {
            taf_pm_SendStateChangeAck(powerStateRef, TAF_PM_STATE_ALL_ACKED, TAF_PM_PVM,
                    TAF_PM_READY);
        }
    }
    else if(state == TAF_PM_STATE_SUSPEND)
    {
        LE_DEBUG("Send state change notification %d", PM_HAL_NODE_STATE_SUSPEND);
        (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, PM_HAL_NODE_STATE_SUSPEND,
                nodeStateChangeNotificationCB);
    }
    else if(state == TAF_PM_STATE_SHUTDOWN)
    {
        LE_DEBUG("Send state change notification %d", PM_HAL_NODE_STATE_SHUTDOWN);
        (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, PM_HAL_NODE_STATE_SHUTDOWN,
                nodeStateChangeNotificationCB);
    }
    else if(state == TAF_PM_STATE_RESUME)
    {
        LE_DEBUG("Send state change notification %d", PM_HAL_NODE_STATE_RESUME);
        (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, PM_HAL_NODE_STATE_RESUME,
                nodeStateChangeNotificationCB);
    }
}

le_result_t taf_mngd_pm_SetNodeTargetedPowerMode(uint8_t pm_node_id,
        taf_mngd_pm_TargetedPowerMode_t powerMode)
{
    LE_INFO("taf_mngd_pm_SetNodeTargetedPowerMode powerMode : %d", powerMode);

    le_result_t res = tafMngdPMSvc::IsClientValid();
    if(res != LE_OK)
    {
        return res;
    }

    targetedPowerMode = powerMode;
    isGraceful = true;

    // Create and acquire a wakelock to get notified on last wakeup source release.
    if(ws == nullptr)
        ws = taf_pm_NewWakeupSource(WAKELOCK_WITHOUT_REF, "mpms");
    if (ws != nullptr)
    {
        le_result_t res = taf_pm_StayAwake(ws);
        if(res == LE_OK)
            LE_DEBUG("Wake source acquired successfully");
        res = taf_pm_Relax(ws);
        if(res == LE_OK)
            LE_DEBUG("Wake source released successfully");
    }
    else
    {
        LE_ERROR("Failed to create wakeup source!");
    }
    return LE_OK;
}

le_result_t taf_mngd_pm_ShutdownReqAsync(taf_mngd_pm_ShutdownMode_t mode,
    taf_mngd_pm_AsyncShutdownReqHandlerFunc_t handlerPtr, void* contextPtr)
{
    le_result_t res = tafMngdPMSvc::IsClientValid();
    if(res != LE_OK)
    {
        return res;
    }

    TAF_ERROR_IF_RET_VAL(!handlerPtr, LE_BAD_PARAMETER, "invalid handlerRef");

    if(pmInf)
    {
        LE_INFO("Send shutdownReqAsync %d", PM_HAL_SHUTDOWN_MODE_FORCEFUL);
        (*(pmInf->shutdownReqAsync))(PM_HAL_SHUTDOWN_MODE_FORCEFUL, shutdownRespCB);
        taf_mngdPm_RequestedState_t statePtr = SYSTEM_FORCEFUL_SHUTDOWN;
        le_timer_SetContextPtr(vhalAckTimerRef, &statePtr);
        le_timer_Start(vhalAckTimerRef);
        shutdownCB.shutdownCallbackFunc = handlerPtr;
        shutdownCB.shutdownCBCtxPtr = contextPtr;
        shutdownCB.sessionRef = taf_mngd_pm_GetClientSessionRef();
    }
    else 
    {
        LE_INFO("Ignore VHAL response if drive is not available");
        le_result_t res = shutdownNAD();
        if(res == LE_OK)
        {
            isGraceful = false;
        }
        // Send ready incase of driver not available.
        handlerPtr(mode, TAF_MNGD_PM_READY, contextPtr);
    }

    return LE_OK;
}

le_result_t taf_mngd_pm_RestartReqAsync(taf_mngd_pm_RestartMode_t mode,
    taf_mngd_pm_AsyncRestartReqHandlerFunc_t handlerPtr, void* contextPtr)
{
    le_result_t res = tafMngdPMSvc::IsClientValid();
    if(res != LE_OK)
    {
        return res;
    }

    TAF_ERROR_IF_RET_VAL(!handlerRef, LE_BAD_PARAMETER, "invalid handlerRef");

    if(mode == TAF_MNGD_PM_RESTART_SYSTEM_OFF_ON)
    {
        if(pmInf)
        {
            LE_INFO("Send restartReqAsync %d", PM_HAL_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF);
            (*(pmInf->restartReqAsync))(PM_HAL_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF, restartRespCB);
            statePtr = RESTART_WITH_NAD_POWER_OFF_ON;
            le_timer_SetContextPtr(vhalAckTimerRef, &statePtr);
            le_timer_Start(vhalAckTimerRef);
            restartCB.restartCallbackFunc = handlerPtr;
            restartCB.restartCBCtxPtr = contextPtr;
            restartCB.sessionRef = taf_mngd_pm_GetClientSessionRef();
        }
        else
        {
            LE_INFO("Ignore VHAL response if drive is not available");
            le_result_t res = shutdownNAD();
            if(res == LE_OK)
            {
                isGraceful = false;
            }
            // Send ready incase of driver not available.
            handlerPtr(mode, TAF_MNGD_PM_READY, contextPtr);
        }
    }

    return LE_OK;
}

COMPONENT_INIT
{
    LE_INFO("tafMngdPMSvc COMPONENT init...");

    mngdPmClientInfo.SessionNodePool = le_mem_InitStaticPool(SessionCtx,
                                        MAX_SESSION,
                                        sizeof(taf_mngdPm_SessionNode_t));

    // Create table of clients
    mngdPmClientInfo.clients = le_hashmap_Create("tafMngdPMClient", MAX_SESSION,
                                             le_hashmap_HashVoidPointer,
                                             le_hashmap_EqualsVoidPointer);

    if (NULL == mngdPmClientInfo.clients)
    {
        LE_FATAL("Failed to create client hashmap");
    }

    le_msg_AddServiceOpenHandler(taf_mngd_pm_GetServiceRef(), tafMngdPMSvc::OnClientConnection,
            NULL);

    le_msg_AddServiceCloseHandler(taf_mngd_pm_GetServiceRef(), tafMngdPMSvc::OnClientDisconnection,
            NULL);

    try
    {
        le_result_t res = tafMngdPMSvc::ParseJsonConfig(TAF_MNGD_PM_CONFIG_PATH);
        if (res == LE_OK)
        {
            LE_INFO("Successfully parsed the JSON");
        }
        else
        {
            LE_ERROR("Failed to parse the JSON");
        }
    } 
    catch (const std::exception &e)
    {
        LE_ERROR("Exception while parsing the JSON");
    }

    vmStatePool = le_mem_CreatePool("VMStatePool", sizeof(taf_mngdPm_State_t));
    vmStateHashmap = le_hashmap_Create("VMStateHashMap", TAF_MNGD_PM_VM_HASH_SIZE,
            le_hashmap_HashString, le_hashmap_EqualsString);
    char name[32] = {0};
    taf_pm_VMListRef_t vmListRef = taf_pm_GetMachineList( );
    le_result_t res;
    if(vmListRef)
    {
        res = taf_pm_GetFirstMachineName(vmListRef, name, 32);
        while(res == LE_OK)
        {
            taf_mngdPm_State_t *vmStatePtr = (taf_mngdPm_State_t*)le_mem_ForceAlloc(vmStatePool);
            memset(vmStatePtr, 0, sizeof(taf_mngdPm_State_t));
            vmStatePtr->nad = TAF_MNGD_PM_NAD1;
            memset(vmStatePtr->vmName, 0, sizeof(vmStatePtr->vmName));
            le_utf8_Copy(vmStatePtr->vmName, name, TAF_MNGD_PM_MACHINE_NAME_LEN, NULL);
            vmStatePtr->state = TAF_MNGD_PM_STATE_RESUME;
            LE_INFO("Add %s machine to hashmap", vmStatePtr->vmName);
            le_hashmap_Put(vmStateHashmap, vmStatePtr->vmName, vmStatePtr);
            res = taf_pm_GetNextMachineName(vmListRef, name, 32);
        }
        taf_pm_DeleteMachineList(vmListRef);
    }

    handlerRef = taf_pm_AddStateChangeHandler(tafMngdPMSvc::StateChangeHandler, NULL);
    if (handlerRef)
        LE_INFO("Register state change handler is successfull");

    res = tafMngdPMSvc::InitVHalModule();

    handlerExRef = taf_pm_AddStateChangeExHandler(tafMngdPMSvc::StateChangeExHandler, NULL);
    if (handlerExRef)
        LE_INFO("Register Extended state change handler is successfull");

    LE_INFO("COMPONENT end init");
}
