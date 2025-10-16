/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *
 * @file tafWlanSTAApSignalStrengthImpl.cpp
 *
 * @brief Implementation of the following signal strength monitoring related classes and their
 * functions.
 *
 * 1. taf_WlanStaConnectedApSignalStrengthMonitor
 *
 */

#include <exception>
#include <stdexcept>
#include <errno.h>
#include <numeric>
#include "tafWlanSTA.hpp"

namespace tafsvc
{

/**************************************************************************************************/
/**************************************************************************************************/
/**
 * taf_WlanStaConnectedApSignalStrengthMonitor class implementation.
 */
/**************************************************************************************************/
/**************************************************************************************************/

// Constructor
taf_WlanStaConnectedApSignalStrengthMonitor::taf_WlanStaConnectedApSignalStrengthMonitor
(
    const taf_wlan_STAid_t staId
) : staId_(staId)
{
    LE_INFO("Constructor for sta: %d(%s)", staId_, intfNameStr_.c_str());
    state_ = State_e::STOPPED;
    mode_  = Mode_e::PASSIVE;
    intfNameStr_.clear();
    connectedApSignalStrengths_.clear();
    sigStrengthTimerRef_ = nullptr;

    // Wil be updated when monitoring is started.
    monitorThreadParams_.startPromisePtr     = nullptr;
    monitorThreadParams_.stopPromisePtr      = nullptr;
}

// Destructor
taf_WlanStaConnectedApSignalStrengthMonitor::~taf_WlanStaConnectedApSignalStrengthMonitor()
{
    LE_INFO("Destructor for sta: %d", staId_);
    // Stop running thread. Timer will be cleaned up in the thread destructor.
    if (sigStrengthThreadRef_)
    {
        le_thread_Cancel(sigStrengthThreadRef_);
        LE_DEBUG("Signal strength monitor thread cancelled.");
        sigStrengthThreadRef_ = nullptr;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The signal strength timer handler. This is called by the Legato framework in the context of the
 * signal monitoring thread (apSigStrengthThreadRef_).
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanStaConnectedApSignalStrengthMonitor::apSigStrengthTimerHandler(le_timer_Ref_t timerRef)
{
    // Retrieve the taf_WlanStaConnectedApSignalStrengthMonitor object pointer from the timer's
    // context.
    taf_WlanStaConnectedApSignalStrengthMonitor *objPtr =
       static_cast<taf_WlanStaConnectedApSignalStrengthMonitor *>(le_timer_GetContextPtr(timerRef));
    TAF_ERROR_IF_RET_NIL(!objPtr, "Timer handler context is null");

    // Now access staId_ through the instance pointer
    int16_t signalStrength = 0xFFFF;
    le_result_t result = objPtr->wpaSuppIntfRefMap_[objPtr->staId_]->GetConnectedApRSSI(
                                                                                    signalStrength);
    if (LE_OK == result)
    {
        objPtr->insertSignalStrength(signalStrength);
        LE_DEBUG("Signal strength added for sta: %d", objPtr->staId_);
        return;
    }

    // Handle errors
    if (LE_FAULT == result)
    {
        LE_WARN("Get RSSI failed. Try again.");
    }
    else if (LE_COMM_ERROR == result)
    {
        LE_ERROR("Communications error. Send command to stop link monitoring.");
        auto &wlanSta = taf_WlanSTASvcImpl::GetInstance();
        StaCtx_t *staCtxPtr = wlanSta.GetStaCtx(objPtr->staId_);
        StaCmd_t staCmd = {staCtxPtr, CMD_WPA_STOP_LINK_MONITORING, nullptr};
        le_event_Report(wlanSta.GetWlanStaInternalCmdEventId(), &staCmd, sizeof(StaCmd_t));
    }
    else
    {
        LE_WARN("Unhandled error: %d", result);
    }
    LE_WARN("Signal strength not added for sta: %d", objPtr->staId_);
}

//--------------------------------------------------------------------------------------------------
/**
 * The signal strength monitoring thread destructor. This is called by the Legato framework.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanStaConnectedApSignalStrengthMonitor::apSigStrengthThreadDestructor(void *context)
{
    TAF_ERROR_IF_RET_NIL(nullptr == context, "Thread destructor context is null");
    taf_WlanStaConnectedApSignalStrengthMonitor *objPtr =
                                static_cast<taf_WlanStaConnectedApSignalStrengthMonitor *>(context);
    LE_DEBUG("Thread destructor called for sta: %d", (objPtr->staId_));

    // Stop any running timer
    if (objPtr->sigStrengthTimerRef_)
    {
        if (le_timer_IsRunning(objPtr->sigStrengthTimerRef_))
        {
            le_timer_Stop(objPtr->sigStrengthTimerRef_);
            le_timer_Delete(objPtr->sigStrengthTimerRef_);
        }
        objPtr->sigStrengthTimerRef_ = nullptr;
        LE_DEBUG("Signal strength monitor timer stopped and deleted.");
    }
    else
    {
        LE_DEBUG("Signal strength monitor timer reference is NULL!");
    }

    // Get the WPA supplicant interface object.
    if (objPtr->wpaSuppIntfRefMap_.find(objPtr->staId_) != objPtr->wpaSuppIntfRefMap_.end())
    {
        LE_INFO("Erase WPA supplicant interface for STA: %d", objPtr->staId_);
        objPtr->wpaSuppIntfRefMap_.erase(objPtr->staId_);
    }

    if (objPtr->monitorThreadParams_.stopPromisePtr)
    {
        // Signal the main thread. The main thread will timeout in case of an exception.
        try
        {
            LE_DEBUG("Stop promise set value.");
            objPtr->monitorThreadParams_.stopPromisePtr->set_value(true); // Set the result
        }
        catch (const std::exception &ex)
        {
            LE_ERROR("stopPromisePtr->set_value exception: %s", ex.what());
        }
        catch (...)
        {
            // Retrieve the exception pointer
            std::exception_ptr exPtr = std::current_exception();
            try {
                if (exPtr)
                {
                    std::rethrow_exception(exPtr);
                }
            } catch (const std::exception& e) {
                // If it's a standard exception, log its what() message
                LE_ERROR("stopPromisePtr->set_value unknown exception: %s", e.what());
            } catch (...) {
                // If it's not a std::exception or derived, log a generic unknown exception
                LE_ERROR("stopPromisePtr->set_value unknown exception");
            }
        }
    }
    else
    {
        LE_WARN("Stop promise not found!");
    }

    LE_DEBUG("Cleaned up.");

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * The signal strength monitoring thread handler. This is called by the Legato framework.
 */
//--------------------------------------------------------------------------------------------------
void *taf_WlanStaConnectedApSignalStrengthMonitor::apSigStrengthThreadHandler(void *context)
{
    TAF_ERROR_IF_RET_VAL(!context, nullptr, "Thread handler context is null");

    // Retrieve the taf_WlanStaConnectedApSignalStrengthMonitor object pointer from the timer's
    // context.
    taf_WlanStaConnectedApSignalStrengthMonitor *objPtr =
                                static_cast<taf_WlanStaConnectedApSignalStrengthMonitor *>(context);
    le_result_t result;

    // Now access staId_ through the instance pointer
    LE_DEBUG("Thread handler for sta: %d", objPtr->staId_);

    if ( !objPtr->sigStrengthTimerRef_)
    {
        // Create the timer. Don't start it.
        char nameStr[24] = {0};
        memset(nameStr, 0, 24);
        snprintf(nameStr, 23, "staId-%d sigMonTmr", objPtr->staId_);
        objPtr->sigStrengthTimerRef_ = le_timer_Create(nameStr);
        LE_DEBUG("Signal monitoring timer %p created", objPtr->sigStrengthTimerRef_);
    }
    else
    {
        LE_WARN("Signal monitoring timer %p already exists", objPtr->sigStrengthTimerRef_);
    }

    // Don't wake up NAD if it's in SUSPEND state
    le_timer_SetWakeup(objPtr->sigStrengthTimerRef_, false);
    // Run until stopped
    le_timer_SetRepeat(objPtr->sigStrengthTimerRef_, 0);
    // Pass object context to the thread handler.
    le_timer_SetContextPtr(objPtr->sigStrengthTimerRef_, objPtr);

    // Register the timer handler
    le_timer_SetHandler(objPtr->sigStrengthTimerRef_, apSigStrengthTimerHandler);

    // Set the appropriate interval based on number of clients registered.
    auto &myWlanSta = taf_WlanSTASvcImpl::GetInstance();
    int numClients = myWlanSta.GetNumSignalStrengthHandlersRefForSta(objPtr->staId_);
    LE_DEBUG("%d Clients are monitoring signal strength for STA[%d].", numClients, objPtr->staId_);
    if (numClients > 0)
    {
        // This should not fail. But if it fails, then the timer would run with a very short
        // interval and would hog CPU. So stop monitoring.
        result = objPtr->SetActiveMode();
        TAF_ERROR_IF_RET_VAL(LE_OK != result, nullptr, "SetActiveMode failed. Monitoring stopped");
        LE_DEBUG("Signal monitoring interval: ACTIVE");
    }
    else
    {
        // This should not fail. But if it fails, then the timer would run with a very short
        // interval and would hog CPU. So stop monitoring.
        result = objPtr->SetPassiveMode();
        TAF_ERROR_IF_RET_VAL(LE_OK != result, nullptr, "SetPassiveMode failed. Monitoring stopped");
        LE_DEBUG("Signal monitoring interval: PASSIVE");
    }

    // start the timer
    le_timer_Start(objPtr->sigStrengthTimerRef_);
    LE_DEBUG("Signal monitoring timer started.");

    // Signal the main thread. The main thread will timeout in case of an exception.
    try
    {
        objPtr->monitorThreadParams_.startPromisePtr->set_value(true); // Set the result
    }
    catch (const std::exception &ex)
    {
        LE_ERROR("startPromisePtr->set_value exception: %s", ex.what());
    }
    catch (...)
    {
        // Retrieve the exception pointer
        std::exception_ptr p = std::current_exception();
        try {
            if (p) {
                std::rethrow_exception(p);
            }
        } catch (const std::exception& e) {
            // If it's a standard exception, log its what() message
            LE_ERROR("startPromisePtr->set_value unknown exception: %s", e.what());
        } catch (...) {
            // If it's not a std::exception or derived, log a generic unknown exception
            LE_ERROR("startPromisePtr->set_value unknown exception");
        }
    }

    LE_DEBUG("Runing event loop...");

    // Run the event loop
    le_event_RunLoop();

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start signal strength measurements with provided STA interface.
 */
//--------------------------------------------------------------------------------------------------
bool taf_WlanStaConnectedApSignalStrengthMonitor::Start()
{
    TAF_ERROR_IF_RET_VAL(State_e::RUNNING == state_, false,
                                                    "Signal strength monitoring is already active");

    // Allocate the start promise
    monitorThreadParams_.startPromisePtr = new (std::nothrow) std::promise<bool>();
    TAF_ERROR_IF_RET_VAL(!monitorThreadParams_.startPromisePtr, false,
                                                                "Failed to allocate start promise");

    // Allocated the stop promise
    monitorThreadParams_.stopPromisePtr  = new (std::nothrow) std::promise<bool>();
    if (!monitorThreadParams_.stopPromisePtr)
    {
        LE_ERROR("Failed to allocate stop promise");
        delete monitorThreadParams_.startPromisePtr;
        monitorThreadParams_.startPromisePtr = nullptr;
        return false;
    }

    // Get the WPA supplicant interface object.
    if (wpaSuppIntfRefMap_.find(staId_) == wpaSuppIntfRefMap_.end())
    {
        LE_INFO("WPA supplicant interface not found for STA: %d", staId_);
        wpaSuppIntfRefMap_[staId_] =
            std::make_shared<taf_WlanStaWpaSupplicantIntf>(taf_WlanStaWpaSupplicantIntf(staId_));
    }

    // Set the WPA supplicant path
    std::string wpaSupplicantPath(WPA_SUPPLICANT_LOCATION_PATH);
    wpaSupplicantPath = wpaSupplicantPath + intfNameStr_;
    LE_INFO("Supplicant path: %s", wpaSupplicantPath.c_str());
    wpaSuppIntfRefMap_[staId_]->SetWpaSupplicantPath(wpaSupplicantPath);

    // Open the WPA control socket
    if (!wpaSuppIntfRefMap_[staId_]->OpenControlSocket())
    {
        LE_ERROR("Open control socket failed.");
        wpaSuppIntfRefMap_.erase(staId_);
        delete monitorThreadParams_.startPromisePtr;
        monitorThreadParams_.startPromisePtr = nullptr;
        delete monitorThreadParams_.stopPromisePtr;
        monitorThreadParams_.stopPromisePtr = nullptr;
        return false;
    }

    // Check the connection
    if (!wpaSuppIntfRefMap_[staId_]->IsControlSocketActive())
    {
        LE_ERROR("Open control socket failed.");
        wpaSuppIntfRefMap_.erase(staId_);
        delete monitorThreadParams_.startPromisePtr;
        monitorThreadParams_.startPromisePtr = nullptr;
        delete monitorThreadParams_.stopPromisePtr;
        monitorThreadParams_.stopPromisePtr = nullptr;
        return false;
    }

    // Get a future for start promise
    std::future<bool>
        startFuture = monitorThreadParams_.startPromisePtr->get_future();

    // Create the signal monitoring thread. Pass `this` pointer to the handler via context
    char nameStr[24] = {0};
    snprintf(nameStr, 23, "sta%d SigMonThread", staId_);
    sigStrengthThreadRef_ = le_thread_Create(nameStr, apSigStrengthThreadHandler, this);

    // Add a destructor to this thread
    le_thread_AddChildDestructor(sigStrengthThreadRef_, apSigStrengthThreadDestructor, this);

    // Start the thread
    le_thread_Start(sigStrengthThreadRef_);

    // Wait for result with timeout
    bool bResult = false;
    if (startFuture.wait_for(std::chrono::seconds(START_TIMEOUT)) == std::future_status::ready)
    {
        try
        {
            bResult = startFuture.get(); // May throw if producer threw
            LE_DEBUG("bResult: %d", bResult);
        }
        catch (const std::exception &ex)
        {
            LE_WARN ("Exception caught: %s", ex.what());
        }
    }
    else
    {
        LE_WARN ("Timeout: Result not ready.");
    }

    if (!bResult)
    {
        LE_ERROR("Signal strength monitoring start failed");
        // Clean up and exit
        wpaSuppIntfRefMap_.erase(staId_);
        delete monitorThreadParams_.startPromisePtr;
        monitorThreadParams_.startPromisePtr = nullptr;
        delete monitorThreadParams_.stopPromisePtr;
        monitorThreadParams_.stopPromisePtr = nullptr;

        le_thread_Cancel(sigStrengthThreadRef_);
        sigStrengthThreadRef_ = nullptr;
        return false;
    }

    // Update the state once the thread starts.
    state_ = State_e::RUNNING;
    LE_INFO("Started signal strength monitoring for sta: %d on interface: %s", staId_,
                                                                            intfNameStr_.c_str());

    // Read once and insert
    int16_t rssi;
    le_result_t result = wpaSuppIntfRefMap_[staId_]->GetConnectedApRSSI(rssi);
    if (LE_OK == result)
    {
        LE_INFO("RSSI: %d dBm", rssi);
        // Add to connectedApSignalStrengths_ deque.
        insertSignalStrength(rssi);
    }
    else
    {
        // Not fatal. The WPA supplicant interface class should handle errors.
        LE_WARN("GetConnectedApRSSI failed: %d", result);
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop signal strength measurements.
 */
//--------------------------------------------------------------------------------------------------
bool taf_WlanStaConnectedApSignalStrengthMonitor::Stop()
{
    TAF_ERROR_IF_RET_VAL(State_e::STOPPED == state_, false,
                                                       "Signal strength monitoring is not active.");

    if (!monitorThreadParams_.stopPromisePtr)
    {
        LE_WARN("Stop promise not allocated.");
        // Simply call thread cancel and exit.
        le_thread_Cancel(sigStrengthThreadRef_);
        state_ = State_e::STOPPED;
        mode_  = Mode_e::PASSIVE;
        connectedApSignalStrengths_.clear();
        sigStrengthThreadRef_ = nullptr;
        LE_INFO("Stopped signal strength monitoring for sta(no wait): %d", staId_);
        return true;
    }

    // Get a future to stop promise
    std::future<bool> stopFuture = monitorThreadParams_.stopPromisePtr->get_future();

    // Stop the signal monitoring thread
    le_thread_Cancel(sigStrengthThreadRef_);

    LE_DEBUG("Signal strength monitoring thread cancelled. Waiting...");
    // Wait for thread to exit
    bool bResult = false;
    if (stopFuture.wait_for(std::chrono::seconds(STOP_TIMEOUT)) == std::future_status::ready)
    {
        try
        {
            bResult = stopFuture.get();
            LE_DEBUG("stopPromisePtr future bResult: %d", bResult);
        }
        catch (const std::exception &ex)
        {
            LE_WARN("stopPromisePtr future exception caught: %s", ex.what());
        }
    }
    else
    {
        LE_WARN("stopPromisePtr future timeout");
    }

    // Delete the promises
    if (monitorThreadParams_.startPromisePtr)
    {
        // Delete the start promise
        delete monitorThreadParams_.startPromisePtr;
        monitorThreadParams_.startPromisePtr = nullptr;
        LE_DEBUG("startPromisePtr deleted.");
    }

    if (monitorThreadParams_.stopPromisePtr)
    {
        // Delete the stop promise
        delete monitorThreadParams_.stopPromisePtr;
        monitorThreadParams_.stopPromisePtr = nullptr;
        LE_DEBUG("stopPromisePtr deleted.");
    }

    state_ = State_e::STOPPED;
    mode_  = Mode_e::PASSIVE;
    connectedApSignalStrengths_.clear();
    sigStrengthThreadRef_ = nullptr;

    LE_INFO("Stopped signal strength monitoring for sta: %d", staId_);
    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get interface name for which signal monitoring is done.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanStaConnectedApSignalStrengthMonitor::SetInterfaceName(const char *intfNameStr)
{
    if (intfNameStr_.empty())
    {
        intfNameStr_ = std::string(intfNameStr);
        LE_DEBUG("UPDATED: Interface name for sta %d: %s", staId_, intfNameStr_.c_str());
    }
    else
    {
        LE_DEBUG("NO CHANGE: Interface name for sta %d: %s", staId_, intfNameStr_.c_str());
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Get interface name for which signal monitoring is done.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanStaConnectedApSignalStrengthMonitor::GetInterfaceName(std::string &intfNameStr) const
{
    intfNameStr = intfNameStr_;
    LE_DEBUG("Interface name for sta %d: %s", staId_, intfNameStr_.c_str());
}

//--------------------------------------------------------------------------------------------------
/**
 * Get status.
 */
//--------------------------------------------------------------------------------------------------
bool taf_WlanStaConnectedApSignalStrengthMonitor::IsRunning() const
{
    LE_DEBUG("State for sta %d: %s", staId_, (State_e::RUNNING == state_) ? "true" : "false");
    return (State_e::RUNNING == state_);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get active mode state
 */
//--------------------------------------------------------------------------------------------------
bool taf_WlanStaConnectedApSignalStrengthMonitor::IsActiveMode() const
{
    LE_DEBUG("Is mode ActiveMode for %d: %s", staId_, (Mode_e::ACTIVE == mode_) ? "true" : "false");
    return (Mode_e::ACTIVE == mode_);
}

//--------------------------------------------------------------------------------------------------
/**
 * Queue the API to set the signal strength timer interval to sigStrengthThreadRef_
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanStaConnectedApSignalStrengthMonitor::queueSetSignalStrengthTimerInterval
(
    void *param1Ptr,
    void *param2Ptr
)
{
    TAF_ERROR_IF_RET_NIL(nullptr == param1Ptr, "param1Ptr is NULL!");
    TAF_ERROR_IF_RET_NIL(nullptr == param2Ptr, "param2Ptr is NULL!");

    le_timer_Ref_t timerRef = static_cast<le_timer_Ref_t>(param1Ptr);
    Mode_e mode = *(static_cast<Mode_e *>(param2Ptr));
    LE_DEBUG("Timer Ref: %p, Mode: %s", timerRef, (Mode_e::ACTIVE == mode) ? "active" : "passive");
    le_result_t result;
    if (Mode_e::ACTIVE == mode)
    {
        result = le_timer_SetMsInterval(timerRef, SIGNAL_STRENGTH_MONITOR_ACTIVE_INTERVAL);
    }
    else
    {
        result = le_timer_SetMsInterval(timerRef, SIGNAL_STRENGTH_MONITOR_PASSIVE_INTERVAL);
    }
    TAF_ERROR_IF_RET_NIL(LE_OK != result, "le_timer_SetMsInterval failed: %d", result);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set active mode
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanStaConnectedApSignalStrengthMonitor::SetActiveMode()
{
    TAF_ERROR_IF_RET_VAL(!sigStrengthTimerRef_, LE_FAULT, "sigStrengthTimerRef_ is NULL!");

    mode_ = Mode_e::ACTIVE;
    // Ensure timer is updated from sigStrengthThreadRef_ context
    if (sigStrengthThreadRef_ == le_thread_GetCurrent())
    {
        le_result_t result = le_timer_SetMsInterval(sigStrengthTimerRef_,
                                                    SIGNAL_STRENGTH_MONITOR_ACTIVE_INTERVAL);
        LE_DEBUG("le_timer_SetMsInterval result: %d", result);
    }
    else
    {
        // Queue the thread update to the signal strength monitoring thread, sigStrengthThreadRef_
        LE_DEBUG("Queue to sigStrengthThreadRef_");
        le_event_QueueFunctionToThread(sigStrengthThreadRef_, queueSetSignalStrengthTimerInterval,
                                                                   sigStrengthTimerRef_, &mode_);
    }

    LE_DEBUG("Mode: ACTIVE");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set passive mode
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanStaConnectedApSignalStrengthMonitor::SetPassiveMode()
{
    TAF_ERROR_IF_RET_VAL(!sigStrengthTimerRef_, LE_FAULT, "sigStrengthTimerRef_ is NULL!");

    mode_ = Mode_e::PASSIVE;
    // Ensure timer is updated from sigStrengthThreadRef_ context
    if (sigStrengthThreadRef_ == le_thread_GetCurrent())
    {
        le_result_t result = le_timer_SetMsInterval(sigStrengthTimerRef_,
                                                    SIGNAL_STRENGTH_MONITOR_PASSIVE_INTERVAL);
        LE_DEBUG("le_timer_SetMsInterval result: %d", result);
    }
    else
    {
        // Queue the thread update to the signal strength monitoring thread, sigStrengthThreadRef_
        LE_DEBUG("Queue to sigStrengthThreadRef_");
        le_event_QueueFunctionToThread(sigStrengthThreadRef_, queueSetSignalStrengthTimerInterval,
                                                                   sigStrengthTimerRef_, &mode_);
    }

    LE_DEBUG("Mode: PASSIVE");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Insert measure signal strength into the map.
 */
//--------------------------------------------------------------------------------------------------
void taf_WlanStaConnectedApSignalStrengthMonitor::insertSignalStrength(const int16_t sigStrength)
{
    std::unique_lock<std::shared_mutex> lock(connectedApSignalStrengthsMutex_);
    connectedApSignalStrengths_.push_back(sigStrength);
    if (connectedApSignalStrengths_.size() > MAX_SIGNAL_STRENGTH_SAMPLES)
    {
        connectedApSignalStrengths_.pop_front();
    }
    LE_DEBUG("Latest signal strength value: %d", sigStrength);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the last measured signal strength.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanStaConnectedApSignalStrengthMonitor::GetLastSignalStrength
(
    int16_t &signalStrength
) const
{
    TAF_ERROR_IF_RET_VAL(State_e::RUNNING != state_, LE_NOT_POSSIBLE,
                                                    "Signal strength monitoring is not active");
    TAF_ERROR_IF_RET_VAL(connectedApSignalStrengths_.empty(), LE_UNAVAILABLE,
                                                "Signal strength measurements not available.");
    std::shared_lock<std::shared_mutex> lock(connectedApSignalStrengthsMutex_);
    signalStrength = connectedApSignalStrengths_.back();
    LE_DEBUG("Latest signal strength value: %d", signalStrength);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the average signal strength over the specified window.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_WlanStaConnectedApSignalStrengthMonitor::GetAverageSignalStrength
(
    const uint16_t windowSize,
    int16_t &avgSigStrength
) const
{
    TAF_ERROR_IF_RET_VAL(State_e::RUNNING != state_, LE_NOT_POSSIBLE,
                                                    "Signal strength monitoring is not active");
    TAF_ERROR_IF_RET_VAL(connectedApSignalStrengths_.empty(), LE_UNAVAILABLE,
                                                "Signal strength measurements not available.");

    size_t actual_windowSize = 0;
    std::shared_lock<std::shared_mutex> lock(connectedApSignalStrengthsMutex_);
    actual_windowSize = std::min((size_t)windowSize, connectedApSignalStrengths_.size());
    // Calculate the average over the specified window starting from the latest sample.
    avgSigStrength = std::accumulate(
                                        connectedApSignalStrengths_.end() - actual_windowSize,
                                        connectedApSignalStrengths_.end(),
                                        0.0
                                    ) / actual_windowSize;
    LE_DEBUG("Average signal strength value over window %zus: %d dBm",
                                                                actual_windowSize, avgSigStrength);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Find the event id for signal strength event based on provided key. Return nullptr if not found.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t taf_WlanStaConnectedApSignalStrengthMonitor::GetSignalStrengthEventId
(
    const ConnectedApSigStrengthEventIdKey_t &key
)
{
    {
        std::shared_lock lock(sigStrengthEventsMapMutex_); // Multiple readers allowed
        auto it = sigStrengthEventsMap_.find(key);
        if (it != sigStrengthEventsMap_.end())
        {
            LE_DEBUG("Event ID found: %p", it->second);
            return it->second;
        }
    }
    LE_DEBUG("Event ID not found.");
    return createSignalStrengthEventId(key);
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a event id for signal strength event based on provided key.
 * Function won't return on failure.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t taf_WlanStaConnectedApSignalStrengthMonitor::createSignalStrengthEventId
(
    const ConnectedApSigStrengthEventIdKey_t &key
)
{
    char nameStr[24] = {0};
    snprintf(nameStr, sizeof(nameStr), "SigEvt_%d_%d_%d_%d", staId_, key.threshold, key.frequency,
             key.bAverage);
    le_event_Id_t eventId = le_event_CreateId(nameStr, sizeof(StaConnectedApSignalStrengthEvt_t));

    // Add this key to the map. Ensure that the event id does not already exist for this key as it
    // will be overwritten.
    std::unique_lock lock(sigStrengthEventsMapMutex_); // Exclusive access
    sigStrengthEventsMap_[key] = eventId;
    LE_DEBUG("Event ID created: %p", eventId);
    return eventId;
}

} // namespace tafsvc
