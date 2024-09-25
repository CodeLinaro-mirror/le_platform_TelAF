/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
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
const char* tafMngdPMSvc::TafStateToString(taf_mngdPm_State_t tafState)
{
    const char *state;
    switch(tafState)
    {
        case TAF_MNGDPM_STATE_RESUME:
            state = "Resume";
            break;
        case TAF_MNGDPM_STATE_SUSPEND:
            state = "Suspend";
            break;
        case TAF_MNGDPM_STATE_SHUTDOWN:
            state = "Shutdown";
            break;
        case TAF_MNGDPM_STATE_RESTART:
            state = "Restart";
            break;
        case TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE:
            state = "Releasing wake source";
            break;
        case TAF_MNGDPM_STATE_SUSPENDING:
            state = "Suspending";
            break;
        case TAF_MNGDPM_STATE_RESTARTING:
            state = "Restarting";
            break;
        case TAF_MNGDPM_STATE_SHUTTING_DOWN:
            state = "Shutting down";
            break;
        case TAF_MNGDPM_STATE_WAKING_UP:
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
                                    (resumeGpioPinState == 1), TAF_MNGDPM_STATE_RESUME);
                            std::string resumeCanFrmId =
                                    config.second.get<std::string>("can.frame_id");
                            LE_INFO("resumeCanFrmId is %s", resumeCanFrmId.c_str());

                            std::string resumeCanMsg = config.second.get<std::string>("can.msg");
                            LE_INFO("resumeCanMsg is %s", resumeCanMsg.c_str());

                            frameId = std::stoul(resumeCanFrmId, nullptr, 16);
                            can.RegisterCanEvents(frameId, TAF_MNGDPM_STATE_RESUME,
                                    resumeCanMsg.c_str());

                            std::string resumeSmsMsg = config.second.get<std::string>("SMS.text");
                            LE_INFO("resumeSmsMsg is %s", resumeSmsMsg.c_str());
                            sms.RegisterSms(resumeSmsMsg.c_str(), TAF_MNGDPM_STATE_RESUME);
                        }
                        if(config.first == "SUSPEND")
                        {
                            uint8_t suspendGpioPin = config.second.get<uint8_t>("gpio.pin_num");
                            LE_INFO("suspendGpioPin is %d", suspendGpioPin);

                            uint8_t suspendGpioPinState = config.second.get<uint8_t>("gpio.state");
                            LE_INFO("suspendGpioPinState is %d", suspendGpioPinState);

                            gpio.RegisterGpioChangeCallback(suspendGpioPin,
                                    (suspendGpioPinState == 1), TAF_MNGDPM_STATE_SUSPEND);

                            std::string suspendCanFrmId =
                                    config.second.get<std::string>("can.frame_id");
                            LE_INFO("suspendCanFrmId is %s", suspendCanFrmId.c_str());

                            std::string suspendCanMsg =
                                    config.second.get<std::string>("can.msg");
                            LE_INFO("suspendCanMsg is %s", suspendCanMsg.c_str());

                            frameId = std::stoul(suspendCanFrmId, nullptr, 16);
                            can.RegisterCanEvents(frameId, TAF_MNGDPM_STATE_SUSPEND,
                                    suspendCanMsg.c_str());

                            std::string suspendSmsMsg = config.second.get<std::string>("SMS.text");
                            LE_INFO("suspendSmsMsg is %s", suspendSmsMsg.c_str());
                            sms.RegisterSms(suspendSmsMsg.c_str(),
                                    TAF_MNGDPM_STATE_SUSPEND);
                        }
                        if(config.first == "SHUTDOWN")
                        {
                            uint8_t shutdownGpioPin = config.second.get<uint8_t>("gpio.pin_num");
                            LE_INFO("shutdownGpioPin is %d", shutdownGpioPin);

                            uint8_t shutdownGpioPinState =
                                    config.second.get<uint8_t>("gpio.state");
                            LE_INFO("shutdownGpioPinState is %d", shutdownGpioPinState);

                            gpio.RegisterGpioChangeCallback(shutdownGpioPin,
                                    (shutdownGpioPinState == 1), TAF_MNGDPM_STATE_SHUTDOWN);

                            std::string shutdownCanFrmId =
                                    config.second.get<std::string>("can.frame_id");
                            LE_INFO("shutdownCanFrmId is %s", shutdownCanFrmId.c_str());

                            std::string shutdownCanMsg =
                                    config.second.get<std::string>("can.msg");
                            LE_INFO("shutdownCanMsg is %s", shutdownCanMsg.c_str());

                            frameId = std::stoul(shutdownCanFrmId, nullptr, 16);
                            can.RegisterCanEvents(frameId, TAF_MNGDPM_STATE_SHUTDOWN,
                                    shutdownCanMsg.c_str());

                            std::string shutdownSmsMsg =
                                    config.second.get<std::string>("SMS.text");
                            LE_INFO("shutdownSmsMsg is %s", shutdownSmsMsg.c_str());
                            sms.RegisterSms(shutdownSmsMsg.c_str(), TAF_MNGDPM_STATE_SHUTDOWN);
                        }
                    }
                }
            }
        }
    }
    return LE_OK;
}

/**
 * Parse JSON config file
 */
le_result_t tafMngdPMSvc::ParseJsonConfiguration(std::string configPath)
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
        configPath = TAF_MNGDPM_DEFAULT_CONF_PATH;
        std::ifstream jsonFile(configPath);
        if(!jsonFile.is_open())
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
    auto &mpms = tafMngdPMSvc::GetInstance();

    long int bootup_awake_time = root.get<int>("bootup_awake_time");
    LE_INFO("bootup_awake_time is %ld", bootup_awake_time);
    mpms.config.bootup_awake_time = bootup_awake_time;

    bool hal_enabled = root.get<bool>("hal_enabled");
    LE_INFO("hal_enabled is %d", hal_enabled);
    mpms.config.hal_enabled = hal_enabled;

    long int hal_state_prepare_timeout = root.get<int>("hal_state_prepare_timeout");
    LE_INFO("hal_state_prepare_timeout is %ld", hal_state_prepare_timeout);
    mpms.config.hal_state_prepare_timeout = hal_state_prepare_timeout;

    long int hal_wakeup_vehicle_timeout = root.get<int>("hal_wakeup_vehicle_timeout");
    LE_INFO("hal_wakeup_vehicle_timeout is %ld", hal_wakeup_vehicle_timeout);
    mpms.config.hal_wakeup_vehicle_timeout = hal_wakeup_vehicle_timeout;

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
            taf_mngdPm_vmState_t *vmStatePtr =
                    (taf_mngdPm_vmState_t*)le_hashmap_GetValue(hashIter);

            if(vmStatePtr) {
                vmStatePtr->state = TAF_MNGDPM_STATE_SHUTDOWN;
            }
        }
    }
    return res;
}

/**
 * Set Restart state to NAD
 */
le_result_t tafMngdPMSvc::RestartNAD()
{
    le_result_t res = taf_pm_SetAllVMPowerState(TAF_PM_STATE_RESTART);
    if(res != LE_OK)
    {
        LE_ERROR("Failed to restart the NAD");
    }
    else
    {
        le_hashmap_It_Ref_t hashIter =
                (le_hashmap_It_Ref_t)le_hashmap_GetIterator(vmStateHashmap);
        while (LE_OK == le_hashmap_NextNode(hashIter))
        {
            taf_mngdPm_vmState_t *vmStatePtr =
                    (taf_mngdPm_vmState_t*)le_hashmap_GetValue(hashIter);

            if(vmStatePtr) {
                vmStatePtr->state = TAF_MNGDPM_STATE_RESTART;
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
    LE_INFO("SuspendNAD");
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
            taf_mngdPm_vmState_t *vmStatePtr =
                    (taf_mngdPm_vmState_t*)le_hashmap_GetValue(hashIter);

            if(vmStatePtr) {
                vmStatePtr->state = TAF_MNGDPM_STATE_SUSPEND;
            }
        }
    }
    return res;
}

/**
 * NodeStateChange request callback function for VHAL module
 */
void tafMngdPMSvc::NodeStateChangeReqRespCB
(
    uint8_t pmNodeId,
    hal_pm_NodeState_t state,
    hal_pm_PowerMode_t mode
)
{
    LE_INFO("***** %s *****", __FUNCTION__);
    LE_INFO("pmNodeId: %d", pmNodeId);
    LE_INFO("hal_pm_NodeState_t: %d", state);
    LE_INFO("hal_pm_PowerMode_t: %d", mode);

    taf_pm_SendStateChangeAck(powerStateRef, TAF_PM_STATE_ALL_ACKED, TAF_PM_PVM,
            TAF_PM_READY);
}

/**
 * Shutdown response callback function for VHAL module
 */
void tafMngdPMSvc::ShutdownPrepareRespCB
(
    uint8_t pmNodeId,
    hal_pm_NodeState_t state,
    hal_pm_PowerMode_t mode,
    hal_pm_RspReason_t reason
)
{
    LE_INFO("***** %s *****", __FUNCTION__);
    LE_INFO("pmNodeId: %d", pmNodeId);
    LE_INFO("hal_pm_NodeState_t: %d", state);
    LE_INFO("hal_pm_PowerMode_t: %d", mode);
    LE_INFO("hal_pm_RspReason_t: %d", reason);
    if(le_timer_IsRunning(vhalAckTimerRef))
    {
        LE_DEBUG("Stop the timer");
        le_timer_Stop(vhalAckTimerRef);
    }
    if (mode == HAL_PM_SHUTDOWN_MODE_NORMAL && reason == HAL_PM_RSP_READY)
    {
        if(RequestStateChange(TAF_MNGDPM_STATE_SHUTTING_DOWN) != LE_OK)
        {
            if(shutdownCB.shutdownCallbackFunc)
            {
                shutdownCB.shutdownCallbackFunc(
                    TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
                    TAF_MNGDPM_NOT_READY,
                    shutdownCB.shutdownCBCtxPtr);
            }
            shutdownCB.shutdownCallbackFunc = nullptr;
            return;
        }

        if(shutdownCB.shutdownCallbackFunc)
        {
            shutdownCB.shutdownCallbackFunc(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL, TAF_MNGDPM_READY,
                    shutdownCB.shutdownCBCtxPtr);
        }
        le_result_t res = ShutdownNAD();
        if(res == LE_OK)
        {
            powerMode.isGraceful = false;
        }
    }
    else if (mode == HAL_PM_SHUTDOWN_MODE_NORMAL && reason == HAL_PM_RSP_NOT_READY)
    {
        tafMngdPMSvc::ProcessStateChange(stateMachine.prevState);
        powerMode.isForceful = false;
        if(shutdownCB.shutdownCallbackFunc)
        {
            shutdownCB.shutdownCallbackFunc(
                TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
                TAF_MNGDPM_NOT_READY,
                shutdownCB.shutdownCBCtxPtr);
        }
    }
    shutdownCB.shutdownCallbackFunc = nullptr;
}

/**
 * Restart response callback function for VHAL module
 */
void tafMngdPMSvc::RestartPrepareRespCB
(
    uint8_t pmNodeId,
    hal_pm_NodeState_t state,
    hal_pm_PowerMode_t mode,
    hal_pm_RspReason_t reason
)
{
    LE_INFO("***** %s *****", __FUNCTION__);
    LE_INFO("pmNodeId: %d", pmNodeId);
    LE_INFO("hal_pm_NodeState_t: %d", state);
    LE_INFO("hal_pm_PowerMode_t: %d", mode);
    LE_INFO("hal_pm_RspReason_t: %d", reason);
    if(le_timer_IsRunning(vhalAckTimerRef))
    {
        LE_INFO("vhalAckTimerRef");
        LE_DEBUG("Stop the timer");
        le_timer_Stop(vhalAckTimerRef);
    }
    if (mode == HAL_PM_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF && reason == HAL_PM_RSP_READY)
    {
        if(RequestStateChange(TAF_MNGDPM_STATE_SHUTTING_DOWN) != LE_OK)
        {
            if(restartCB.restartCallbackFunc)
            {
                restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON, TAF_MNGDPM_NOT_READY,
                        restartCB.restartCBCtxPtr);
            }
            return;
        }

        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON, TAF_MNGDPM_READY,
                    restartCB.restartCBCtxPtr);
        }
        le_result_t res = ShutdownNAD();
        if(res == LE_OK)
        {
            powerMode.isGraceful = false;
        }
    }
    else if (mode == HAL_PM_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF && reason == HAL_PM_RSP_NOT_READY)
    {
        tafMngdPMSvc::ProcessStateChange(stateMachine.prevState);
        powerMode.isShutDown = false;
        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON, TAF_MNGDPM_NOT_READY,
                    restartCB.restartCBCtxPtr);
        }
    }
    if (mode == HAL_PM_RESTART_MODE_NAD_REBOOT && reason == HAL_PM_RSP_READY)
    {
        if(RequestStateChange(TAF_MNGDPM_STATE_RESTARTING) != LE_OK)
        {
            if(restartCB.restartCallbackFunc)
            {
                restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT, TAF_MNGDPM_NOT_READY,
                        restartCB.restartCBCtxPtr);
            }
            return;
        }

        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT, TAF_MNGDPM_READY,
                    restartCB.restartCBCtxPtr);
        }
        le_result_t res = RestartNAD();
        if(res == LE_OK)
        {
            LE_INFO("RestartNAD is success");
        }
    }
    else if (mode == HAL_PM_RESTART_MODE_NAD_REBOOT && reason == HAL_PM_RSP_NOT_READY)
    {
        tafMngdPMSvc::ProcessStateChange(stateMachine.prevState);
        powerMode.isRestart = false;
        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_MODE_NAD_REBOOT, TAF_MNGDPM_NOT_READY,
                    restartCB.restartCBCtxPtr);
        }
    }
    restartCB.restartCallbackFunc = nullptr;
}

/**
 * WakeupVehicle response callback function for VHAL module
 */
void tafMngdPMSvc::WakeupVehicleCB
(
    int32_t reason,
    int32_t response
)
{
    LE_INFO("***** %s *****", __FUNCTION__);
    LE_INFO("hal_pm_WakeupVehicleReason: %d", reason);
    LE_INFO("hal_pm_RspReason_t: %d", response);
    if(le_timer_IsRunning(wakeupVehicleTimerRef))
    {
        LE_INFO("wakeupVehicleTimerRef");
        LE_DEBUG("Stop the timer");
        le_timer_Stop(wakeupVehicleTimerRef);
    }
    if (reason == VEHICHLE_WAKEUP_REASON_DEFAULT && response == HAL_PM_VEHICHLE_WAKEUP_STATUS_AWAKE)
    {
        LE_INFO("Response for the wakeupVehicleCall is HAL_PM_VEHICHLE_WAKEUP_STATUS_AWAKE");
        if(wakeupVehicleCB.wakeupVehicleCallbackFunc)
        {
            wakeupVehicleCB.wakeupVehicleCallbackFunc(VEHICHLE_WAKEUP_REASON_DEFAULT, VEHICHLE_WAKEUP_STATUS_AWAKE,
                    wakeupVehicleCB.wakeupVehicleCBCtxPtr);
        }
    }
    else if (reason == VEHICHLE_WAKEUP_REASON_DEFAULT && response == HAL_PM_VEHICHLE_WAKEUP_STATUS_INVALID_REQ)
    {
        LE_INFO("Response for the wakeupVehicleCall is HAL_PM_VEHICHLE_WAKEUP_STATUS_INVALID_REQ");
        if(wakeupVehicleCB.wakeupVehicleCallbackFunc)
        {
            wakeupVehicleCB.wakeupVehicleCallbackFunc(VEHICHLE_WAKEUP_REASON_DEFAULT, VEHICHLE_WAKEUP_STATUS_INVALID_REQ,
                    wakeupVehicleCB.wakeupVehicleCBCtxPtr);
        }
    }
    else if (reason == VEHICHLE_WAKEUP_REASON_DEFAULT && response == HAL_PM_VEHICHLE_WAKEUP_STATUS_UNKNOWN)
    {
        LE_INFO("Response for the wakeupVehicleCall is HAL_PM_VEHICHLE_WAKEUP_STATUS_UNKNOWN");
        if(wakeupVehicleCB.wakeupVehicleCallbackFunc)
        {
            wakeupVehicleCB.wakeupVehicleCallbackFunc(VEHICHLE_WAKEUP_REASON_DEFAULT, VEHICHLE_WAKEUP_STATUS_UNKNOWN,
                    wakeupVehicleCB.wakeupVehicleCBCtxPtr);
        }
    }
    wakeupVehicleCB.wakeupVehicleCallbackFunc = nullptr;
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
    LE_INFO("NodeEventCB for node %d with node info %s", pm_node_id, pm_node_event_info);
    le_result_t res = LE_FAULT;
    if(strncmp(pm_node_event_info, RELAX, strlen(RELAX)) == 0)
    {
        if(tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE) != LE_OK)
        {
            return;
        }

        res = tafMngdPMSvc::ReleaseWakeLock();
        if(res == LE_OK)
        {
            LE_INFO(" ReleaseWakeLock successfull");
            tafMngdPMSvc::ProcessStateChange(TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE);
        }
    }
    else if(strncmp(pm_node_event_info, STAYAWAKE, strlen(STAYAWAKE)) == 0)
    {
        if(tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_WAKING_UP) != LE_OK)
        {
            return;
        }

        res = tafMngdPMSvc::AcquireWakeLock();
        if(res == LE_OK)
        {
            LE_INFO(" AcquireWakeLock successfull");
            tafMngdPMSvc::ProcessStateChange(TAF_MNGDPM_STATE_WAKING_UP);
        }
    }
    else if(strncmp(pm_node_event_info, SHUTDOWN, strlen(SHUTDOWN)) == 0)
    {
        LE_INFO("SHUTDOWN from VHAL");

        if(tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_SHUTTING_DOWN) != LE_OK)
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

    if (LE_OK != le_msg_GetClientProcessId(sessionRef, &pid))
    {
        LE_ERROR("Error, Failed to get client pid.");
    }

    taf_mngdPm_SessionNode_t* sessionNodePtr = nullptr;

    sessionNodePtr =
        (taf_mngdPm_SessionNode_t*)le_mem_TryAlloc(mngdPmClientInfo.SessionNodePool);

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

    auto &mpms = tafMngdPMSvc::GetInstance();
    //clear state change registered clients
    for(auto it = mpms.regClientrecrd.begin(); it != mpms.regClientrecrd.end(); )
    {
        if(it->sessionRef == sessionRef)
        {
            LE_INFO("Client with sessionRef %p", it->sessionRef);
            it = mpms.regClientrecrd.erase(it);
        }
        else
        {
            ++it;
        }
    }
    //clear Wake Source filter
    for(auto it = mpms.wsWhiteList.begin(); it != mpms.wsWhiteList.end(); )
    {
        if(it->sessionRef == sessionRef)
        {
            LE_INFO("Client with sessionRef %p", it->sessionRef);
            it = mpms.wsWhiteList.erase(it);
        }
        else
        {
            ++it;
        }
    }

    //Clear WsReflist
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.wsRefList));

    while (linkHandlerPtr)
    {
        taf_wsRefCtx_t * wsRefCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_wsRefCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.wsRefList), linkHandlerPtr);
        if (wsRefCtxPtr && wsRefCtxPtr->sessionRef == sessionRef && wsRefCtxPtr->isAcquiredLock)
        {
            LE_INFO("Client with sessionRef %p", wsRefCtxPtr->sessionRef);
            le_result_t res = tafMngdPMSvc::ReleaseWakeLock();
            if(res == LE_OK)
            {
                LE_INFO("Released lock");
                wsRefCtxPtr->isAcquiredLock = false;
            }
            else {
                le_ref_DeleteRef(mpms.wsRefMap, wsRefCtxPtr->wsRef);
                le_dls_Remove(&(mpms.wsRefList), &wsRefCtxPtr->link);
                free((void*)wsRefCtxPtr->vhalTag);
                le_mem_Release((void*)wsRefCtxPtr);
            }
        }
    }
}

/**
 * Check whether the client session is valid or not
 */
bool tafMngdPMSvc::IsClientValid()
{
    le_msg_SessionRef_t sessionRef = taf_mngdPm_GetClientSessionRef();

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
    LE_INFO("State change triggered for %s\n", TafStateToString((taf_mngdPm_State_t)state));

    // Update the VMs data on receiving state change caused by any other sources like SMS, CAN
    le_hashmap_It_Ref_t hashIter = (le_hashmap_It_Ref_t)le_hashmap_GetIterator(vmStateHashmap);
    while (LE_OK == le_hashmap_NextNode(hashIter))
    {
        taf_mngdPm_vmState_t *vmStatePtr = (taf_mngdPm_vmState_t*)le_hashmap_GetValue(hashIter);
        if(vmStatePtr) {
            vmStatePtr->state = (taf_mngdPm_State_t)state;
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
    taf_mngdPm_NodePowerStateChange_t powerStateChange;
    if(state == TAF_PM_STATE_ALL_ACKED && !pmInf)
    {
        LE_DEBUG("Send ACK if there is no driver loaded");
        taf_pm_SendStateChangeAck(powerStateRef, state, vm_id, TAF_PM_READY);
        return;
    }
    if(state == TAF_PM_STATE_ALL_WAKELOCKS_RELEASED)
    {
        LE_DEBUG("Received all wakelocks released notification");
        le_result_t res;
        if(targetedPowerMode == TAF_MNGDPM_SHUTDOWN)
        {
            res = ShutdownNAD();
            if(res == LE_OK) {
                powerMode.isGraceful = true;
                ProcessStateChange(TAF_MNGDPM_STATE_SHUTTING_DOWN);
            }
        }
        else if(wsCount == 0)
        {
            res = SuspendNAD();
            if(res == LE_OK) {
                powerMode.isSuspend = true;
                ProcessStateChange(TAF_MNGDPM_STATE_SUSPENDING);
                LE_INFO("Suspend triggered for NAD on wakelocks released");
            }
        }
    }
    else if ((state == TAF_PM_STATE_ALL_ACKED) && (pmInf) && (pmInf->nodeStateChangeReqAsync))
    {
        if(powerMode.isGraceful)
        {
            powerMode.isGraceful = false;
            LE_DEBUG("Send shutdownReqAsync %d", HAL_PM_SHUTDOWN_MODE_GRACEFUL);
            (*(pmInf->nodeStateChangeReqAsync))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SHUTDOWN, HAL_PM_SHUTDOWN_MODE_GRACEFUL, tafMngdPMSvc::NodeStateChangeReqRespCB);
        }
        else if(powerMode.isShutDown)
        {
            powerMode.isShutDown = false;
            LE_DEBUG("Send shutdownReqAsync %d", HAL_PM_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF);
            (*(pmInf->nodeStateChangeReqAsync))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_RESTART, HAL_PM_RESTART_MODE_SYSTEM_OFF_ON_NAD_OFF, tafMngdPMSvc::NodeStateChangeReqRespCB);
        }
        else if(powerMode.isRestart)
        {
            powerMode.isRestart = false;
            LE_DEBUG("Send RestartReqAsync %d", HAL_PM_RESTART_MODE_NAD_REBOOT);
            (*(pmInf->nodeStateChangeReqAsync))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_RESTART, HAL_PM_RESTART_MODE_NAD_REBOOT, tafMngdPMSvc::NodeStateChangeReqRespCB);
        }
        else if(powerMode.isSuspend)
        {
            powerMode.isSuspend = false;
            LE_DEBUG("Send SuspendReqAsync %d", HAL_PM_SUSPEND_MODE_FULL);
            (*(pmInf->nodeStateChangeReqAsync))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SUSPEND, HAL_PM_SUSPEND_MODE_FULL, tafMngdPMSvc::NodeStateChangeReqRespCB);
        }
        else if(powerMode.isForceful)
        {
            LE_INFO("nodeStateChangeReqAsync triggered to VHAL on forceful shutdown");
            powerMode.isForceful = false;
            (*(pmInf->nodeStateChangeReqAsync))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SHUTDOWN, HAL_PM_SHUTDOWN_MODE_NORMAL, tafMngdPMSvc::NodeStateChangeReqRespCB);
        }
    }
    else if(state == TAF_PM_STATE_SUSPEND)
    {
        ProcessStateChange(TAF_MNGDPM_STATE_SUSPEND);
        powerStateChange.state = TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE;
        le_event_Report(nodePowerStateChange, &powerStateChange, sizeof(taf_mngdPm_NodePowerStateChange_t));

        if(pmInf && pmInf->nodeStateChangeNotification)
        {
            LE_DEBUG("Send state change notification %d", HAL_PM_NODE_STATE_SUSPEND);
            (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SUSPEND,
                    NULL);
        }
    }
    else if(state == TAF_PM_STATE_SHUTDOWN)
    {
        ProcessStateChange(TAF_MNGDPM_STATE_SHUTDOWN);
        if(powerMode.isShutDown)
        {
             powerStateChange.state = TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE;
             le_event_Report(nodePowerStateChange, &powerStateChange, sizeof(taf_mngdPm_NodePowerStateChange_t));
             if(pmInf && pmInf->nodeStateChangeNotification)
             {
                 LE_DEBUG("Send state change notification %d", HAL_PM_NODE_STATE_RESTART);
                 (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_RESTART,
                         NULL);
             }
        }
        else
        {
             powerStateChange.state = TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE;
             le_event_Report(nodePowerStateChange, &powerStateChange, sizeof(taf_mngdPm_NodePowerStateChange_t));
             if(pmInf && pmInf->nodeStateChangeNotification)
             {
                 LE_DEBUG("Send state change notification %d", HAL_PM_NODE_STATE_SHUTDOWN);
                 (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SHUTDOWN,
                         NULL);
             }
        }
    }
    else if(state == TAF_PM_STATE_RESTART)
    {
        ProcessStateChange(TAF_MNGDPM_STATE_RESTART);
        if(powerMode.isRestart)
        {
             powerStateChange.state = TAF_MNGDPM_NODE_STATE_RESTART_PREPARE;
             le_event_Report(nodePowerStateChange, &powerStateChange, sizeof(taf_mngdPm_NodePowerStateChange_t));
             if(pmInf && pmInf->nodeStateChangeNotification)
             {
                 LE_DEBUG("Send state change notification %d", HAL_PM_NODE_STATE_RESTART);
                 (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_RESTART,
                         NULL);
             }
        }
    }
    else if(state == TAF_PM_STATE_RESUME)
    {
        ProcessStateChange(TAF_MNGDPM_STATE_RESUME);
        powerStateChange.state = TAF_MNGDPM_NODE_STATE_RESUME;
        le_event_Report(nodePowerStateChange, &powerStateChange, sizeof(taf_mngdPm_NodePowerStateChange_t));

        if(pmInf && pmInf->nodeStateChangeNotification)
        {
            LE_DEBUG("Send state change notification %d", HAL_PM_NODE_STATE_RESUME);
            (*(pmInf->nodeStateChangeNotification))(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_RESUME,
                    NULL);
        }
    }
}

/**
 * VHAL ack timer handler
 */
void tafMngdPMSvc::VhalAckTimerHandler(le_timer_Ref_t timerRef)
{
    LE_INFO("VhalAckTimerHandler");
    auto &mpms = tafMngdPMSvc::GetInstance();
    taf_mngdPm_RequestedState_t* state =
      (taf_mngdPm_RequestedState_t*)le_timer_GetContextPtr(timerRef);
    LE_INFO("VhalAckTimer Expired after %ld msec for state %d", mpms.config.hal_state_prepare_timeout,
            *(state));
    if(*(state) == SYSTEM_NORMAL_SHUTDOWN)
    {
        LE_INFO("VhalAckTimer expire for SYSTEM_FORCEFUL_SHUTDOWN");
        if(shutdownCB.shutdownCallbackFunc)
        {
            shutdownCB.shutdownCallbackFunc(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL, TAF_MNGDPM_TIMEOUT,
                    shutdownCB.shutdownCBCtxPtr);
        }
    }
    else if (*(state) == RESTART_WITH_NAD_POWER_OFF_ON)
    {
            LE_INFO("VhalAckTimer expire for TAF_MNGDPM_RESTART_SYSTEM_OFF_ON");
        if(restartCB.restartCallbackFunc)
        {
            restartCB.restartCallbackFunc(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON, TAF_MNGDPM_TIMEOUT,
                    restartCB.restartCBCtxPtr);
        }
    }
}
/**
 * VHAL ack timer handler
 */
void tafMngdPMSvc::VehichleWakeupTimerHandler(le_timer_Ref_t timerRef)
{
    LE_INFO("VehichleWakeupTimerHandler");
    auto &mpms = tafMngdPMSvc::GetInstance();
    taf_mngdPm_RequestedWakeupVehicle_t* wakeupMode =
      (taf_mngdPm_RequestedWakeupVehicle_t*)le_timer_GetContextPtr(timerRef);

    LE_INFO("VehichleWakeupTimer Expired after %ld msec for wakeupMode %d",
            mpms.config.hal_wakeup_vehicle_timeout, *(wakeupMode));

    taf_mngdPm_RequestedWakeupVehicle_t* mode =
            (taf_mngdPm_RequestedWakeupVehicle_t*)le_timer_GetContextPtr(timerRef);

    if(*(mode) == WAKEUP_VEHICHLE_REQ_DEFAULT)
    {
        LE_INFO("VehichleWakeupTimer expire for WAKEUP_VEHICHLE_REQ_DEFAULT");
        if(wakeupVehicleCB.wakeupVehicleCallbackFunc)
        {
            wakeupVehicleCB.wakeupVehicleCallbackFunc(WAKEUP_VEHICHLE_REQ_DEFAULT, TAF_MNGDPM_TIMEOUT,
                    wakeupVehicleCB.wakeupVehicleCBCtxPtr);
        }
        wakeupVehicleCB.wakeupVehicleCallbackFunc = nullptr;
    }
}
/**
 * Timer to wait wakesource request from apps
 */
void tafMngdPMSvc::WaitWakeSourceTimer()
{
    LE_INFO("WaitWakeSourceTimer");
    auto &mpms = tafMngdPMSvc::GetInstance();
    le_result_t res;
    //timer to wait for wake source from apps
    wakeSourceTimerRef = le_timer_Create("WAKE SOURCE timer");
    le_timer_SetMsInterval(wakeSourceTimerRef, mpms.config.bootup_awake_time);
    le_timer_SetHandler(wakeSourceTimerRef, WakeSourceTimerHandler);
    //acquire wakesource
    res = AcquireWakeLock();
    if(res == LE_OK) {
        LE_INFO("acquired wake lock after init");
    }
    le_timer_Start(wakeSourceTimerRef);
    LE_INFO("Started WaitWakeSourceTimer for wakesource request from apps");
}

/**
 * Timer handler for wakesource request
 */
void tafMngdPMSvc::WakeSourceTimerHandler(le_timer_Ref_t timerRef)
{
    LE_INFO("WaitWakeSourceTimer Expired for WakeSourceTimerHandler");
    auto &mpms = tafMngdPMSvc::GetInstance();
    le_result_t res = ReleaseWakeLock();
    if(res == LE_OK)
        LE_INFO("ReleaseWakeLock after %ld msec timeout", mpms.config.bootup_awake_time);
}

/**
 * Initialize PM VHAL module
 */
le_result_t tafMngdPMSvc::InitVHalModule()
{
    // Load the driver and does not care the version
    pmInf = (hal_pm_Inf_t *)taf_devMgr_LoadDrv(TAF_PM_MODULE_NAME, nullptr);

    if(pmInf == nullptr)
    {
        LE_ERROR("Can not load the driver %s", TAF_PM_MODULE_NAME);
        return LE_FAULT;
    }
    else // successfully loaded
    {
        LE_INFO("Loaded module %s successfully", TAF_PM_MODULE_NAME);
        LE_DEBUG("Call pmInf(%p) init function", pmInf);
        auto &mpms = tafMngdPMSvc::GetInstance();
        // init first
        (*(pmInf->InitHAL))();
        vhalAckTimerRef = le_timer_Create("VHAL ACK timer");
        le_timer_SetMsInterval(vhalAckTimerRef, mpms.config.hal_state_prepare_timeout);
        le_timer_SetHandler(vhalAckTimerRef, VhalAckTimerHandler);
        //creating the timer for vehichle wakeup
        wakeupVehicleTimerRef = le_timer_Create("VEHICHLE WAKEUP timer");
        le_timer_SetMsInterval(wakeupVehicleTimerRef, mpms.config.hal_wakeup_vehicle_timeout);
        le_timer_SetHandler(wakeupVehicleTimerRef, VehichleWakeupTimerHandler);
    }

    return LE_OK;
}

/**
 * Acquire wakesource and let system stay awake
 */
le_result_t tafMngdPMSvc::AcquireWakeLock()
{
    le_result_t res = LE_FAULT;
    if(wsCount == 0) {
        if(ws == nullptr)
            ws = taf_pm_NewWakeupSource(WAKELOCK_WITHOUT_REF, "mpms");
        if (ws != nullptr && !powerMode.isWsAcquired)
        {
            res = taf_pm_StayAwake(ws);
            if(res == LE_OK) {
                powerMode.isWsAcquired = true;
                wsCount++;
                LE_INFO("Acquired WakeLock wsCount:%d", wsCount);
            }
            else
            {
                LE_INFO("failed to acquire ws");
            }
        }
        else
        {
            LE_ERROR("Failed to create wakeup source!");
        }
    }
    else
    {
        wsCount++;
        LE_INFO("Acquired WakeLock wsCount:%d", wsCount);
        res = LE_OK;
    }
    return res;
}

/**
 * Release wakesource
 */
le_result_t tafMngdPMSvc::ReleaseWakeLock()
{
    le_result_t res = LE_FAULT;
    if(wsCount > 0 )
    {
        LE_INFO("Wake source released successfully");
        wsCount--;
        LE_INFO("ReleaseWakeLock wsCount:%d", wsCount);
        res = LE_OK;
        if(wsCount == 0 )
        {
            if (ws != nullptr && powerMode.isWsAcquired)
            {
                if(tafMngdPMSvc::RequestStateChange(TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE) != LE_OK)
                {
                    return res;
                }
                res = taf_pm_Relax(ws);
                if(res == LE_OK) {
                    LE_INFO("Wake source from pms released successfully");
                    powerMode.isWsAcquired = false;
                }
            }
            else
            {
                LE_ERROR("Failed to release wakeup lock!");
                res = LE_FAULT;
            }
        }
    }
    else
    {
        LE_ERROR("No wakeup lock acquired to release !");
    }

    return res;
}

/**
 * Set modem wake up source
 */
void tafMngdPMSvc::SetModemWakeupSource(taf_mngdPm_WakeupType_t wakeupType)
{
    LE_INFO("SetModemWakeupSource");
    //check if same wakeupSource exists
    auto &mpms = tafMngdPMSvc::GetInstance();
    if(wsWhiteList.size() > 0)
    {
        for(auto &client : mpms.wsWhiteList)
        {
            if((client.wakeupType == wakeupType) && (client.sessionRef == taf_mngdPm_GetClientSessionRef()))
            {
                LE_INFO("wakeupType already in wsWhiteList");
                return;
            }
        }
    }
    LE_INFO("SetModemWakeupSource type %d", wakeupType);
    mpms.wsWhiteList.push_back({wakeupType, taf_mngdPm_GetClientSessionRef()});
}

/**
 * Request MPMS state change condition check
 */
le_result_t tafMngdPMSvc::RequestStateChange(taf_mngdPm_State_t requestedState)
{
    LE_INFO("current state %s", TafStateToString(stateMachine.currentState));
    LE_INFO("requested state %s", TafStateToString(requestedState));

    le_result_t res = LE_OK;
    if (stateMachine.currentState == requestedState)
    {
        return res;
    }
    switch(requestedState)
    {
        case TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE:
            if(stateMachine.currentState != TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE &&
               stateMachine.currentState != TAF_MNGDPM_STATE_RESUME)
            {
                res = LE_NOT_PERMITTED;
            }
            break;

        case TAF_MNGDPM_STATE_SUSPENDING:
            if(stateMachine.currentState != TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE &&
               stateMachine.currentState != TAF_MNGDPM_STATE_RESUME)
            {
                res = LE_NOT_PERMITTED;
            }
            break;

        case TAF_MNGDPM_STATE_RESTARTING:
        case TAF_MNGDPM_STATE_SHUTTING_DOWN:
            if(stateMachine.currentState != TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE &&
               stateMachine.currentState != TAF_MNGDPM_STATE_RESUME)
            {
                res = LE_NOT_PERMITTED;
            }
            break;

        case TAF_MNGDPM_STATE_WAKING_UP:
            if(stateMachine.currentState == TAF_MNGDPM_STATE_SHUTTING_DOWN || stateMachine.currentState == TAF_MNGDPM_STATE_RESTARTING)
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
void tafMngdPMSvc::ProcessStateChange(taf_mngdPm_State_t toState)
{
    LE_INFO("current state %s", TafStateToString(stateMachine.currentState));

    switch(toState)
    {
        case TAF_MNGDPM_STATE_WAKING_UP:
            if(stateMachine.currentState == TAF_MNGDPM_STATE_RESUME
               || stateMachine.currentState == TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE)
            {
                // No need to change state from RESUME and RELEASING_WAKE_SOURCE to WAKING_UP
                toState = stateMachine.currentState;
            }
            break;

        default:
            break;
    }

    if(toState != stateMachine.currentState)
    {
        stateMachine.prevState = stateMachine.currentState;
        stateMachine.currentState = toState;

        taf_mngdPm_StateInd_t stateInd;
        stateInd.state = toState;

        le_event_Report(stateChange, &stateInd, sizeof(taf_mngdPm_StateInd_t));
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

    taf_mngdPm_StateChangeHandlerFunc_t handlerFunc =
        (taf_mngdPm_StateChangeHandlerFunc_t)layerHandlerFunc;

    handlerFunc((taf_mngdPm_StateInd_t*)reportPtr, le_event_GetContextPtr());
}

void tafMngdPMSvc::DeleteNodePowerStateRefs()
{
    LE_INFO("DeleteNodePowerStateRefs");
    auto &mpms = tafMngdPMSvc::GetInstance();
    for (const auto &client : mpms.regClientrecrd ) {
            le_ref_DeleteRef(nodePowerStateRefMap, client.nodeStateRef);
    }
}

bool tafMngdPMSvc::IsSameAsCurrentState(taf_mngdPm_NodePowerState_t nodeState, taf_mngdPm_State_t tafState)
{
    LE_INFO("IsSameAsCurrentState");
    if((nodeState == TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE) && (tafState == TAF_MNGDPM_STATE_SHUTDOWN))
    {
        return true;
    }
    else if((nodeState == TAF_MNGDPM_NODE_STATE_RESTART_PREPARE) && (tafState == TAF_MNGDPM_STATE_SHUTDOWN))
    {
        return true;
    }
    else if((nodeState == TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE) && (tafState == TAF_MNGDPM_STATE_SUSPEND))
    {
        return true;
    }
    else if((nodeState == TAF_MNGDPM_NODE_STATE_RESUME) && (tafState == TAF_MNGDPM_STATE_RESUME))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool tafMngdPMSvc::IsConfiguredBitMask(taf_mngdPm_NodePowerState_t state, taf_mngdPm_NodePowerStateChangeBitMask_t stateMask)
{
    LE_INFO("IsConfiguredBitMask");
    bool isSameBitMask = false;
    if(state == TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE && (stateMask & (1)) !=0)
    {
        isSameBitMask = true;
    }
    else if(state == TAF_MNGDPM_NODE_STATE_RESTART_PREPARE && (stateMask & (1 << 1)) !=0)
    {
        isSameBitMask = true;
    }
    else if(state == TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE && (stateMask & (1 << 2)) !=0)
    {
       LE_INFO("IsConfiguredBitMask");
        isSameBitMask = true;
    }
    else if(state == TAF_MNGDPM_NODE_STATE_RESUME && (stateMask & (1 << 3)) !=0)
    {
        isSameBitMask = true;
    }
    else
    {
        LE_INFO("IsConfiguredBitMask");
        isSameBitMask = false;
    }
    return isSameBitMask;
}

void tafMngdPMSvc::SendAckToPms(taf_mngdPm_NodePowerState_t state, taf_pm_ClientAck_t ackType)
{
    LE_INFO("SendAckToPms");
    auto &mpms = tafMngdPMSvc::GetInstance();
    if (state == TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE)
    {
        LE_INFO("TAF_PM_STATE_SHUTDOWN");
        taf_pm_SendStateChangeAck(mpms.powerStateRef, TAF_PM_STATE_SHUTDOWN, TAF_PM_PVM, ackType);
    }
    else if(state== TAF_MNGDPM_NODE_STATE_RESTART_PREPARE)
    {
        LE_INFO("TAF_PM_STATE_SHUTDOWN");
        taf_pm_SendStateChangeAck(mpms.powerStateRef, TAF_PM_STATE_RESTART, TAF_PM_PVM, ackType);
    }
    else if (state == TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE)
    {
        LE_INFO("TAF_PM_STATE_SUSPEND");
        taf_pm_SendStateChangeAck(mpms.powerStateRef, TAF_PM_STATE_SUSPEND, TAF_PM_PVM, ackType);
    }
    else if (state == TAF_MNGDPM_NODE_STATE_RESUME)
    {
        LE_INFO("TAF_PM_STATE_RESUME");
        taf_pm_SendStateChangeAck(mpms.powerStateRef, TAF_PM_STATE_RESUME, TAF_PM_PVM,
                ackType);
    }
}

/**
 * To Call Clients for Extend power state change notification
 */
void tafMngdPMSvc::CallNodePowerStateHandlerFunc(taf_mngdPm_NodePowerState_t state)
{
    LE_INFO("CallNodePowerStateHandlerFunc");
    auto &mpms = tafMngdPMSvc::GetInstance();
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&(mpms.nodePowerStateHandlerList));
    //clearing the previous references for new state notification
    DeleteNodePowerStateRefs();
    mpms.regClientrecrd.clear();
    mpms.ackClientrecrdSize = 0;
    while (linkHandlerPtr)
    {
        taf_mngdPm_NodePowerStateCtxt_t * handlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_mngdPm_NodePowerStateCtxt_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&(mpms.nodePowerStateHandlerList), linkHandlerPtr);
        if (handlerCtxPtr->handlerPtr)
        {
            LE_INFO("Client found");
            if(mpms.IsConfiguredBitMask(state, handlerCtxPtr->powerStateMask))
            {
                taf_NodePowerStateRef_t* nodeStateListPtr =
                        (taf_NodePowerStateRef_t*)le_mem_ForceAlloc(mpms.nodePowerStateRefPool);
                nodeStateListPtr->nodeStateRef =
                        (taf_mngdPm_nodePowerStateRef_t)le_ref_CreateRef(mpms.nodePowerStateRefMap,
                                nodeStateListPtr);
                mpms.regClientrecrd.push_back({nodeStateListPtr->nodeStateRef,
                        handlerCtxPtr->sessionRef, state});
                handlerCtxPtr->handlerPtr(handlerCtxPtr->pmNodeId, nodeStateListPtr->nodeStateRef,
                        state, handlerCtxPtr->nodePowerStateHandlerCtxPtr);
                LE_INFO("Notified to Client");
            }
            else {
                continue;
            }
            mpms.clientSize++;
        }
    }
    if(mpms.clientSize == 0)
    {
        LE_INFO("No client registered in MPMS, ack to PMS immediately for state:%d", state);
        mpms.SendAckToPms(state, TAF_PM_READY);
    }
}

/**
 * State change layered handler function
 */
void tafMngdPMSvc::NodePowerStateChanged(void* reportPtr)
{
    LE_INFO("NodePowerStateChanged");
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");
    taf_mngdPm_NodePowerStateChange_t* powerStateChange =(taf_mngdPm_NodePowerStateChange_t*)reportPtr;
    if(powerStateChange->state == TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE)
    {
        CallNodePowerStateHandlerFunc(TAF_MNGDPM_NODE_STATE_SHUTDOWN_PREPARE);
    }
    else if(powerStateChange->state == TAF_MNGDPM_NODE_STATE_RESTART_PREPARE)
    {
        CallNodePowerStateHandlerFunc(TAF_MNGDPM_NODE_STATE_RESTART_PREPARE);
    }
    else if(powerStateChange->state == TAF_MNGDPM_NODE_STATE_SUSPEND_PREPARE)
    {
        CallNodePowerStateHandlerFunc(powerStateChange->state);
    }
    else if(powerStateChange->state == TAF_MNGDPM_NODE_STATE_RESUME)
    {
        CallNodePowerStateHandlerFunc(powerStateChange->state);
    }
    else
    {
        LE_INFO("Invalid state");
    }
}

/**
 * Call Clients for Bub Status Event notification
 */
void tafMngdPMSvc::InfoReportCB(void* reportPtr)
{
    LE_INFO("InfoReportCB");
    bubStatusEvent_t* stateEvent = (bubStatusEvent_t*)reportPtr;
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&infoReportHandlerList);
    while (linkHandlerPtr)
    {
        taf_mngdPm_InfoReportCb_t * handlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_mngdPm_InfoReportCb_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&infoReportHandlerList, linkHandlerPtr);
        if (handlerCtxPtr->handlerPtr)
        {
            LE_INFO("Notifying to clients");
            handlerCtxPtr->handlerPtr(stateEvent->status, handlerCtxPtr->infoReportHandlerCtxPtr);
        }
    }
}

/**
 * VHAL callback for Bub Status Event notification
 */
void tafMngdPMSvc::InfoReportVhalCB(int32_t* reportPtr)
{
    LE_INFO("InfoReportVhalCB");
    bubStatusEvent_t bubStatusEvent;
    int32_t bubStatus = *reportPtr;
    bubStatusEvent.status = (taf_mngdPm_BubStatus_t)bubStatus;
    le_event_Report(infoReport, &bubStatusEvent, sizeof(bubStatusEvent_t));
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

taf_mngdPm_TargetedPowerMode_t tafMngdPMSvc::targetedPowerMode = TAF_MNGDPM_RESUME;
taf_mngdPm_RestartCb_t tafMngdPMSvc::restartCB;
taf_mngdPm_ShutdownCb_t tafMngdPMSvc::shutdownCB;
std::vector<taf_mngdPm_WakeupSourceCtxt_t> tafMngdPMSvc::wsWhiteList;

uint8_t tafMngdPMSvc::wsCount = 0;
taf_powerMode_t tafMngdPMSvc::powerMode{};
taf_stateMachine_t tafMngdPMSvc::stateMachine{};

hal_pm_Inf_t* tafMngdPMSvc::pmInf = nullptr;
le_timer_Ref_t tafMngdPMSvc::vhalAckTimerRef = nullptr;
le_timer_Ref_t tafMngdPMSvc::wakeSourceTimerRef = nullptr;
le_timer_Ref_t tafMngdPMSvc::wakeupVehicleTimerRef = nullptr;
taf_mngdPm_RequestedWakeupVehicle_t tafMngdPMSvc::wakeupModePtr;
taf_mngdPm_RequestedState_t tafMngdPMSvc::statePtr;
taf_mngdPm_Client_t tafMngdPMSvc::mngdPmClientInfo;

le_event_Id_t tafMngdPMSvc::stateChange;
taf_mngdPm_WakeupVehicleCb_t tafMngdPMSvc::wakeupVehicleCB;

le_mem_PoolRef_t tafMngdPMSvc::infoReportHandlerPool;
le_dls_List_t tafMngdPMSvc::infoReportHandlerList;
le_ref_MapRef_t tafMngdPMSvc::infoReportHandlerRefMap;
le_event_Id_t tafMngdPMSvc::infoReport;
//Node Power State change handler
le_event_Id_t tafMngdPMSvc::nodePowerStateChange;
le_mem_PoolRef_t tafMngdPMSvc::nodePowerStateHandlerPool;
le_dls_List_t tafMngdPMSvc::nodePowerStateHandlerList;
le_ref_MapRef_t tafMngdPMSvc::nodePowerStateHandlerMap;
le_mem_PoolRef_t tafMngdPMSvc::nodePowerStateRefPool;
le_ref_MapRef_t tafMngdPMSvc::nodePowerStateRefMap;
int8_t tafMngdPMSvc::clientSize;
int8_t tafMngdPMSvc::ackClientrecrdSize;
taf_mngdPm_config_t tafMngdPMSvc::config;