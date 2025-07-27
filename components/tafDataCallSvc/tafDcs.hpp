/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file TafDcs.hpp
 * @brief TelAF Data Call Service implementation header
 *
 */
#ifndef __TAF_DCS_HPP__
#define __TAF_DCS_HPP__

#include "legato.h"
#include "interfaces.h"
#include "taf_pa_data.hpp"

namespace taf{
namespace svc{
namespace datacall{

/**
 * The default phone ID
 */
const uint8_t TAF_DCS_DEFAULT_PHONE_ID = 1;

/**
 * The internal data structure for clientDisconnectedEvtId_
 */
struct TafDcsClientDisconnectedEvent_t
{
    le_msg_SessionRef_t clientRef; ///< The client reference
};

/**
 * The internal data structure for startSessionAsyncRspEvtId_
 */
struct TafDcsSendStartSessionAsyncRsp_t
{
    le_msg_SessionRef_t  clientRef;   ///< The client reference
    taf_dcs_ProfileRef_t profileRef;  ///< The profile reference
    le_result_t          result;      ///< The result of the operation
};

/**
 * The internal data structure for stopSessionAsyncRspEvtId_
 */
struct TafDcsSendStopSessionAsyncRsp_t
{
    le_msg_SessionRef_t clientRef;   ///< The client reference
    taf_dcs_ProfileRef_t profileRef; ///< The profile reference
    le_result_t result;              ///< The result of the operation
};

class TafDcsSvc
{
public:
    TafDcsSvc(const TafDcsSvc &) = delete;
    TafDcsSvc &operator=(const TafDcsSvc &) = delete;
    static TafDcsSvc &GetInstance();
    void Init();
    void Deinit();
    taf::pa::data::InitState_e GetInitState() const;

    // Internal threads
    le_thread_Ref_t GetEventsThreadRef() const;

    // Internal events
    le_event_Id_t GetUpdateProfileEvtId() const;
    le_event_Id_t GetClientsDisconnectedEvtId() const;
    le_event_Id_t GetSessionStartEvtId() const;
    le_event_Id_t GetSessionStopEvtId() const;
    le_event_Id_t GetStartSessionAsyncRspEvtId() const;
    le_event_Id_t GetStopSessionAsyncRspEvtId() const;
    le_event_Id_t GetPaSessionStateChangeEvtId() const;
    le_event_Id_t GetPaRoamingStatusChangeEvtId() const;
    le_event_Id_t GetPaThrottledAPNsEvtId() const;
    le_event_Id_t GetPaHwAccelerationEvtId() const;
    le_event_Id_t GetPaQosTftEvtId() const;

private:

    // Private functions
    void startThreads();
    void stopThreads();
    void createDCSEvents();
    void registerClientsConnectDisconnectHandlers();
    void unregisterClientsConnectDisconnectHandlers();
    static void powerStateChangeHandler(taf_pm_State_t state, void *contextPtr);

    // The handler that is called when a client connects to DCS
    le_msg_SessionEventHandlerRef_t dcsClientConnectHandlerRef;
    static void onDCSClientConnect(le_msg_SessionRef_t sessionRef, void *ctxPtr);
    // The handler that is called when a client disconnects from DCS
    le_msg_SessionEventHandlerRef_t dcsClientDisconnectHandlerRef;
    static void onDCSClientDisconnect(le_msg_SessionRef_t sessionRef, void *ctxPtr);

    // Private variables
    taf::pa::data::InitState_e paInitState_ = taf::pa::data::InitState_e::INIT_FAILED;
    taf::pa::data::SlotCount_e slotCount_ = taf::pa::data::SlotCount_e::ONE; // 1

    // Internal events
    le_event_Id_t clientDisconnectedEvtId_;     // Client disconnected event
    le_event_Id_t startSessionAsyncRspEvtId_;   // Start session async response event
    le_event_Id_t stopSessionAsyncRspEvtId_;    // Stop session async response event
    le_event_Id_t updateProfileEvtId_;          // Profile update event
    le_event_Id_t sessionStartEvtId_;           // Session start event
    le_event_Id_t sessionStopEvtId_;            // Session stop event

    // PA events
    le_event_Id_t paSessionStateChangeEvtId_;   // Session state change event from PA
    le_event_Id_t paRoamingChangeEvtId_;        // Roaming change event from PA
    le_event_Id_t paThrottledAPNsEvtId_;        // Throttled APNs event from PA
    le_event_Id_t paHwAccelerationChangeEvtId_; // HW acceleration change events from PA
    le_event_Id_t paQosTftEvtId_;               // QoS TFT events from PA

    // Thread to handle internal events
    le_thread_Ref_t tafDcsEventsThreadRef_ = nullptr;

    // Instance
    TafDcsSvc() {};
};

} // namespace datacall
} // namespace svc
} // namespace taf

#endif //__TAF_DCS_HPP__