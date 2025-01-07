/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file   dcsAdaptor.hpp
 * @brief  Header file for TelAF Data Call Service adpator.
 */

#ifndef DCSSADAPTOR_H
#define DCSSADAPTOR_H

#include <future>
#include <thread>

extern "C"
{
#include "taf_dcs_interface.h"
}

// Time out for DCS API calls
#define DCS_API_TIMEOUT      5  // 5s
// The default phone ID to use
#define DCS_DEFAULT_PHONE_ID 1

// Callback types
typedef void (*SessionStateChangeCb)(
    uint32_t profileId,
    taf_dcs_ConState_t state,
    taf_dcs_Pdp_t ipType,
    void *ctx);

class DcsAdaptor
{

    // Data to send to TelAF thread for GetDefaultProfileId
    typedef struct
    {
        DcsAdaptor *objPtr; // Pointer to the DcsAdaptor object
    } GetDefaultProfileIdCmdEventParm_t;

    // The result for GetDefaultProfileIdCmdEvent
    typedef struct
    {
        le_result_t result;
        uint32_t profileId;
    } GetDefaultProfileIdCmdResult_t;

    // Data to send to TelAF thread for StartSession
    typedef struct
    {
        DcsAdaptor *objPtr; // Pointer to the DcsAdaptor object
        uint32_t profileId;
    } StartSessionCmdEventParm_t;

    // The result for StartSession
    typedef struct
    {
        le_result_t result;
    } StartSessionCmdResult_t;

    // Data to send to TelAF thread for StopSession
    typedef struct
    {
        DcsAdaptor *objPtr; // Pointer to the DcsAdaptor object
        uint32_t profileId;
    } StopSessionCmdEventParm_t;

    // The result for StopSession
    typedef struct
    {
        le_result_t result;
    } StopSessionCmdResult_t;

    // Data to send to TelAF thread for GetInterfaceName
    typedef struct
    {
        DcsAdaptor *objPtr; // Pointer to the DcsAdaptor object
        uint32_t profileId;
    } GetInterfaceNameCmdEventParm_t;

    // The result for GetInterfaceName
    typedef struct
    {
        le_result_t result;
        std::string IntfName;
    } GetInterfaceNameCmdResult_t;

    // Data to send to TelAF thread for GetIPv4Address
    typedef struct
    {
        DcsAdaptor *objPtr; // Pointer to the DcsAdaptor object
        uint32_t profileId;
    } GetIPv4AddressCmdEventParm_t;

    // The result for GetIPv4Address
    typedef struct
    {
        le_result_t result;
        std::string IPv4Address;
    } GetIPv4AddressCmdResult_t;

    // Data to send to TelAF thread for GetCallEndReason
    typedef struct
    {
        DcsAdaptor *objPtr; // Pointer to the DcsAdaptor object
        uint32_t profileId;
        taf_dcs_Pdp_t pdp;
    } GetCallEndReasonCmdEventParm_t;

    // The result for GetCallEndReason
    typedef struct
    {
        le_result_t result;
        taf_dcs_CallEndReasonType_t CallEndReasonType;
        int32_t CallEndReason;
    } GetCallEndReasonCmdResult_t;

    // Data to send to TelAF thread for AddSessionStateChangeCbk
    typedef struct
    {
        DcsAdaptor *objPtr;            // Pointer to the DcsAdaptor object
        uint32_t profileId;            // The profile id
        SessionStateChangeCb callback; // The app callback function pointer
        void *ctx;                     // The context to pass to the callback
    } AddSessionStateChangeCbkCmdEventParm_t;

    typedef struct
    {
        SessionStateChangeCb callback; // The app callback function pointer
        void *ctx;                     // The context to pass to the callback
    } AddSessionStateChangeCbkCtx_t;

    typedef struct
    {
        le_result_t result;
    } AddSessionStateChangeCbkResult_t;

    public:
        DcsAdaptor();
        ~DcsAdaptor();

        le_result_t Connect();
        // taf_dcs_GetDefaultPhoneIdAndProfileId
        le_result_t GetDefaultProfileId (uint32_t &profileIdRef);
        // taf_dcs_StartSession
        le_result_t StartSession        (uint32_t profileId);
        // taf_dcs_StopSession
        le_result_t StopSession         (uint32_t profileId);
        // taf_dcs_GetInterfaceName
        le_result_t GetInterfaceName    (uint32_t profileId, std::string &IntfName);
        // taf_dcs_GetIPv4Address
        le_result_t GetIPv4Address      (uint32_t profileId, std::string &IPv4Addr);
        // taf_dcs_GetCallEndReason
        le_result_t GetCallEndReason    (uint32_t profileId, taf_dcs_Pdp_t pdp,
                                         taf_dcs_CallEndReasonType_t &typeRef, int32_t &reasonRef);
        // taf_dcs_AddSessionStateHandler
        le_result_t AddSessionStateChangeCbk(uint32_t profileId, SessionStateChangeCb, void *ctx);

    private:

        /*Events and handlers for suppported commands*/

        // GetDefaultProfileId
        le_event_Id_t GetDefaultProfileIdEventId; // Event to send to TelAF thread
        std::promise<GetDefaultProfileIdCmdResult_t> GetDefaultProfileIdPromise;
        static void OnGetDefaultProfileIdEventHandler(void *reportPtr);

        // StartSession
        le_event_Id_t StartSessionEventId; // Event to send to TelAF thread
        std::promise<StartSessionCmdResult_t> StartSessionPromise;
        static void OnStartSessionEventIdEventHandler(void *reportPtr);

        // StopSession
        le_event_Id_t StopSessionEventId; // Event to send to TelAF thread
        std::promise<StopSessionCmdResult_t> StopSessionPromise;
        static void OnStopSessionEventHandler(void *reportPtr);

        // GetInterfaceName
        le_event_Id_t GetInterfaceNameEventId;  // Event to send to TelAF thread
        std::promise<GetInterfaceNameCmdResult_t> GetInterfaceNamePromise;
        static void OnGetInterfaceNameEventHandler(void *reportPtr);

        // GetIPv4Address
        le_event_Id_t GetIPv4AddressEventId; // Event to send to TelAF thread
        std::promise<GetIPv4AddressCmdResult_t> GetIPv4AddressPromise;
        static void OnGetIPv4AddressEventHandler(void *reportPtr);

        // GetCallEndReason
        le_event_Id_t GetCallEndReasonEventId; // Event to send to TelAF thread
        std::promise<GetCallEndReasonCmdResult_t> GetCallEndReasonPromise;
        static void OnGetCallEndReasonEventHandler(void *reportPtr);

        // AddSessionStateChangeCbk
        le_event_Id_t AddSessionStateHandlerEventId; // Event to send to TelAF thread
        std::promise<AddSessionStateChangeCbkResult_t> AddSessionStateChangeCbkPromise;
        static void OnAddSessionStateChangeCbkEventHandler(void *reportPtr);
        // The callback handler for taf_dcs_AddSessionStateHandler
        static void SessionStateChangeHandler(
            taf_dcs_ProfileRef_t profileRef,
            taf_dcs_ConState_t state,
            const taf_dcs_StateInfo_t *stateInfoPtr,
            void *contextPtr);
        // The context to pass to SessionStateChangeHandler
        AddSessionStateChangeCbkCtx_t *SessionStateChangeCbkCtxPtr;

        // Variables
        pthread_t DcsTId;
        std::promise<le_result_t> DcsThreadPromise;

        // The DCS TelAF thread handler
        static void *TelafDcsTask(void *arg);
        // Function to create command event ids and register event handlers
        void AddCmdEventsAndHandler();
};

#endif // DCSSADAPTOR_H