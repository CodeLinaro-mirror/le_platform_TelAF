/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafNtn.hpp"

using namespace std;

//--------------------------------------------------------------------------------------------------
/**
 * Static memory pools backing the ref-counted objects owned by this component.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(session, SESSION_MAX_COUNT, sizeof(Session_t));
LE_MEM_DEFINE_STATIC_POOL(dataPacket, DATA_PACKET_MAX_COUNT, sizeof(DataPacket_t));
LE_MEM_DEFINE_STATIC_POOL(locationFix, LOCATION_FIX_MAX_COUNT, sizeof(LocationFix_t));
LE_MEM_DEFINE_STATIC_POOL(systemSelectionList, SYSTEM_SELECTION_LIST_MAX_COUNT,
    sizeof(SystemSelectionList_t));
LE_MEM_DEFINE_STATIC_POOL(specifier, SYSTEM_SELECTION_SPEC_MAX_COUNT,
    sizeof(SystemSelectionSpecifier_t));
LE_MEM_DEFINE_STATIC_POOL(safeRef, SAFE_REF_MAX_COUNT, sizeof(SafeRef_t));

//--------------------------------------------------------------------------------------------------
// Utility::Convert
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Converts a PA result to a Legato result.
 */
//--------------------------------------------------------------------------------------------------
le_result_t Utility::Convert::Result
(
    taf_pa_result_t result ///< [IN] PA result.
)
{
    switch (result)
    {
        case TAF_PA_OK:
            return LE_OK;
        case TAF_PA_FAULT:
            return LE_FAULT;
        case TAF_PA_TIMEOUT:
            return LE_TIMEOUT;
        case TAF_PA_OUT_OF_RANGE:
            return LE_OUT_OF_RANGE;
        case TAF_PA_BAD_PARAMETER:
            return LE_BAD_PARAMETER;
        case TAF_PA_UNSUPPORTED:
            return LE_CAP_UNSUPPORTED;
        case TAF_PA_NOT_PERMITTED:
            return LE_NOT_PERMITTED;
        case TAF_PA_NOT_IMPLEMENTED:
            return LE_CAP_NOT_IMPLEMENTED;
        case TAF_PA_UNAVAILABLE:
            return LE_UNAVAILABLE;
        default:
            LE_INFO("Unknown result %d.", result);
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Converts public API phone ID (1-based) to internal instance index (0-based).
 *
 * @return
 *      - Instance index in range [0, INSTANCE_MAX_COUNT).
 *      - INSTANCE_MAX_COUNT when phone is invalid.
 */
//--------------------------------------------------------------------------------------------------
uint32_t Utility::Convert::PhoneToInstance
(
    uint8_t phone ///< [IN] Phone ID.
)
{
    switch (phone)
    {
        case 1:
            return 0;
        case 2:
            return 1;
        default:
            LE_INFO("Invalid phone %d.", phone);
    }

    return INSTANCE_MAX_COUNT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Converts a PA NTN state to the public NTN state.
 */
//--------------------------------------------------------------------------------------------------
taf_ntn_State_t Utility::Convert::State
(
    taf_pa_ntn_State_t state ///< [IN] PA NTN state.
)
{
    switch (state)
    {
        case TAF_PA_NTN_STATE_DISABLED:
            return TAF_NTN_STATE_DISABLED;
        case TAF_PA_NTN_STATE_OUT_OF_SERVICE:
            return TAF_NTN_STATE_OUT_OF_SERVICE;
        case TAF_PA_NTN_STATE_IN_SERVICE:
            return TAF_NTN_STATE_IN_SERVICE;
        case TAF_PA_NTN_STATE_UNKNOWN:
        default:
            LE_ERROR("Unknown PA NTN state %d.", state);
            return TAF_NTN_STATE_DISABLED;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Converts a PA data acknowledgement error code to the public data ack result.
 */
//--------------------------------------------------------------------------------------------------
taf_ntn_DataAckResult_t Utility::Convert::DataAckResult
(
    taf_pa_result_t errorCode ///< [IN] PA acknowledgement error code.
)
{
    switch (errorCode)
    {
        case TAF_PA_OK:
            return TAF_NTN_DATA_ACK_RESULT_SUCCESS;
        case TAF_PA_TIMEOUT:
            return TAF_NTN_DATA_ACK_RESULT_TIMEOUT;
        default:
            return TAF_NTN_DATA_ACK_RESULT_FAILED;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Converts a PA location fix request reason to the public reason.
 */
//--------------------------------------------------------------------------------------------------
taf_ntn_LocationFixRequestReason_t Utility::Convert::LocationFixRequestReason
(
    taf_pa_ntn_LocationFixRequestReason_t reason ///< [IN] PA request reason.
)
{
    switch (reason)
    {
        case TAF_PA_NTN_LOCATION_FIX_REQUEST_REASON_NORMAL:
            return TAF_NTN_LOCATION_FIX_REQUEST_REASON_NORMAL;
        case TAF_PA_NTN_LOCATION_FIX_REQUEST_REASON_VALIDITY_TIMER_EXPIRED:
            return TAF_NTN_LOCATION_FIX_REQUEST_REASON_VALIDITY_TIMER_EXPIRED;
        case TAF_PA_NTN_LOCATION_FIX_REQUEST_REASON_UNKNOWN:
        default:
            return TAF_NTN_LOCATION_FIX_REQUEST_REASON_UNKNOWN;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Converts a public location status to the PA location status.
 */
//--------------------------------------------------------------------------------------------------
taf_pa_ntn_LocationStatus_t Utility::Convert::LocationStatus
(
    taf_ntn_LocationStatus_t status ///< [IN] Public location status.
)
{
    switch (status)
    {
        case TAF_NTN_LOCATION_STATUS_SUCCESS:
            return TAF_PA_NTN_LOCATION_STATUS_SUCCESS;
        case TAF_NTN_LOCATION_STATUS_INVALID_ARG:
            return TAF_PA_NTN_LOCATION_STATUS_INVALID_ARG;
        case TAF_NTN_LOCATION_STATUS_INTERNAL_ERR:
            return TAF_PA_NTN_LOCATION_STATUS_INTERNAL_ERR;
        case TAF_NTN_LOCATION_STATUS_NOT_SUPPORTED:
            return TAF_PA_NTN_LOCATION_STATUS_NOT_SUPPORTED;
        case TAF_NTN_LOCATION_STATUS_RETRY:
            return TAF_PA_NTN_LOCATION_STATUS_RETRY;
        case TAF_NTN_LOCATION_STATUS_FAILED:
            return TAF_PA_NTN_LOCATION_STATUS_FAILED;
        case TAF_NTN_LOCATION_STATUS_INVALID:
        default:
            return TAF_PA_NTN_LOCATION_STATUS_INVALID;
    }
}

//--------------------------------------------------------------------------------------------------
// Utility::LayeredFunction
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Dispatches NTN state change indications and releases the ref-counted payload.
 */
//--------------------------------------------------------------------------------------------------
void Utility::LayeredFunction::StateChange
(
    void* reportPtr,     ///< [IN] Ref-counted payload.
    void* handlerFuncPtr ///< [IN] Client callback.
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    StateChangeInd_t* indPtr = (StateChangeInd_t*)reportPtr;
    taf_ntn_StateChangeHandlerFunc_t handlerFunc =
        (taf_ntn_StateChangeHandlerFunc_t)handlerFuncPtr;
    LE_INFO("Dispatch StateChange to client: handlerFunc=%p sessionRef=%p state=%d",
        (void*)handlerFunc, (void*)indPtr->sessionRef, indPtr->state);
    if (handlerFunc != nullptr)
        handlerFunc(indPtr->sessionRef, indPtr->state, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Dispatches NTN capability change indications and releases the ref-counted payload.
 */
//--------------------------------------------------------------------------------------------------
void Utility::LayeredFunction::CapabilitiesChange
(
    void* reportPtr,     ///< [IN] Ref-counted payload.
    void* handlerFuncPtr ///< [IN] Client callback.
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    CapabilitiesChangeInd_t* indPtr = (CapabilitiesChangeInd_t*)reportPtr;
    taf_ntn_CapabilitiesChangeHandlerFunc_t handlerFunc =
        (taf_ntn_CapabilitiesChangeHandlerFunc_t)handlerFuncPtr;
    LE_INFO("Dispatch CapabilitiesChange to client: handlerFunc=%p sessionRef=%p",
        (void*)handlerFunc, (void*)indPtr->sessionRef);
    if (handlerFunc != nullptr)
        handlerFunc(indPtr->sessionRef, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Dispatches NTN data acknowledgement indications and releases the ref-counted payload.
 */
//--------------------------------------------------------------------------------------------------
void Utility::LayeredFunction::DataAck
(
    void* reportPtr,     ///< [IN] Ref-counted payload.
    void* handlerFuncPtr ///< [IN] Client callback.
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    DataAckInd_t* indPtr = (DataAckInd_t*)reportPtr;
    taf_ntn_DataAckHandlerFunc_t handlerFunc = (taf_ntn_DataAckHandlerFunc_t)handlerFuncPtr;
    LE_INFO("Dispatch DataAck to client: handlerFunc=%p sessionRef=%p result=%d transactionId=%"
        PRIu64, (void*)handlerFunc, (void*)indPtr->sessionRef, indPtr->result,
        indPtr->transactionId);
    if (handlerFunc != nullptr)
        handlerFunc(indPtr->sessionRef, indPtr->result, indPtr->transactionId,
            le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Dispatches incoming NTN data indications and releases the ref-counted payload.
 */
//--------------------------------------------------------------------------------------------------
void Utility::LayeredFunction::IncomingData
(
    void* reportPtr,     ///< [IN] Ref-counted payload.
    void* handlerFuncPtr ///< [IN] Client callback.
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    IncomingDataInd_t* indPtr = (IncomingDataInd_t*)reportPtr;
    taf_ntn_IncomingDataHandlerFunc_t handlerFunc =
        (taf_ntn_IncomingDataHandlerFunc_t)handlerFuncPtr;
    LE_INFO("Dispatch IncomingData to client: handlerFunc=%p sessionRef=%p packetRef=%p",
        (void*)handlerFunc, (void*)indPtr->sessionRef, (void*)indPtr->packetRef);
    if (handlerFunc != nullptr)
        handlerFunc(indPtr->sessionRef, indPtr->packetRef, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Dispatches terrestrial cellular coverage indications and releases the ref-counted payload.
 */
//--------------------------------------------------------------------------------------------------
void Utility::LayeredFunction::CellularCoverage
(
    void* reportPtr,     ///< [IN] Ref-counted payload.
    void* handlerFuncPtr ///< [IN] Client callback.
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    CellularCoverageInd_t* indPtr = (CellularCoverageInd_t*)reportPtr;
    taf_ntn_CellularCoverageHandlerFunc_t handlerFunc =
        (taf_ntn_CellularCoverageHandlerFunc_t)handlerFuncPtr;
    LE_INFO("Dispatch CellularCoverage to client: handlerFunc=%p sessionRef=%p available=%d",
        (void*)handlerFunc, (void*)indPtr->sessionRef, indPtr->available);
    if (handlerFunc != nullptr)
        handlerFunc(indPtr->sessionRef, indPtr->available, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Dispatches location fix request indications and releases the ref-counted payload.
 */
//--------------------------------------------------------------------------------------------------
void Utility::LayeredFunction::LocationFixRequest
(
    void* reportPtr,     ///< [IN] Ref-counted payload.
    void* handlerFuncPtr ///< [IN] Client callback.
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    LocationFixRequestInd_t* indPtr = (LocationFixRequestInd_t*)reportPtr;
    taf_ntn_LocationFixRequestHandlerFunc_t handlerFunc =
        (taf_ntn_LocationFixRequestHandlerFunc_t)handlerFuncPtr;
    LE_INFO("Dispatch LocationFixRequest to client: handlerFunc=%p sessionRef=%p reason=%d",
        (void*)handlerFunc, (void*)indPtr->sessionRef, indPtr->reason);
    if (handlerFunc != nullptr)
        handlerFunc(indPtr->sessionRef, indPtr->reason, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Dispatches NTN band update indications and releases the ref-counted payload.
 */
//--------------------------------------------------------------------------------------------------
void Utility::LayeredFunction::BandUpdate
(
    void* reportPtr,     ///< [IN] Ref-counted payload.
    void* handlerFuncPtr ///< [IN] Client callback.
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    BandUpdateInd_t* indPtr = (BandUpdateInd_t*)reportPtr;
    taf_ntn_BandUpdateHandlerFunc_t handlerFunc = (taf_ntn_BandUpdateHandlerFunc_t)handlerFuncPtr;
    LE_INFO("Dispatch BandUpdate to client: handlerFunc=%p sessionRef=%p band=%" PRIu64,
        (void*)handlerFunc, (void*)indPtr->sessionRef, indPtr->band);
    if (handlerFunc != nullptr)
        handlerFunc(indPtr->sessionRef, indPtr->band, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
// PA indication callbacks
//
// These run on the PA notification context. Each allocates a ref-counted payload, tags it with the
// active session reference for the (single) NTN context, and forwards it to the service event loop.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * PA callback for NTN state change indications.
 */
//--------------------------------------------------------------------------------------------------
static void NtnStateChangeHandler
(
    taf_pa_ntn_NtnStateChangeIndication_t indication, ///< [IN] Indication payload.
    void* contextPtr                                  ///< [IN] Context.
)
{
    auto& factory = Factory::GetInstance();

    StateChangeInd_t* indPtr = (StateChangeInd_t*)le_mem_ForceAlloc(factory.pools.stateChange);
    indPtr->sessionRef = factory.cache.activeSessionRef[factory.cache.activeInstance];
    indPtr->state = Utility::Convert::State(indication.ntnState);

    LE_INFO("PA NtnStateChangeHandler: reporting event, activeSessionRef[%u]=%p state=%d",
        factory.cache.activeInstance, (void*)indPtr->sessionRef, indPtr->state);

    le_event_ReportWithRefCounting(factory.events.stateChange, (void*)indPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * PA callback for NTN capability change indications.
 */
//--------------------------------------------------------------------------------------------------
static void CapabilitiesChangeHandler
(
    taf_pa_ntn_CapabilitiesChangeIndication_t indication, ///< [IN] Indication payload.
    void* contextPtr                                      ///< [IN] Context.
)
{
    auto& factory = Factory::GetInstance();

    CapabilitiesChangeInd_t* indPtr =
        (CapabilitiesChangeInd_t*)le_mem_ForceAlloc(factory.pools.capabilitiesChange);
    indPtr->sessionRef = factory.cache.activeSessionRef[factory.cache.activeInstance];

    LE_INFO("PA CapabilitiesChangeHandler: reporting event, activeSessionRef[%u]=%p",
        factory.cache.activeInstance, (void*)indPtr->sessionRef);

    le_event_ReportWithRefCounting(factory.events.capabilitiesChange, (void*)indPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * PA callback for NTN data acknowledgement indications.
 */
//--------------------------------------------------------------------------------------------------
static void DataAckHandler
(
    taf_pa_ntn_DataAckIndication_t indication, ///< [IN] Indication payload.
    void* contextPtr                           ///< [IN] Context.
)
{
    auto& factory = Factory::GetInstance();

    DataAckInd_t* indPtr = (DataAckInd_t*)le_mem_ForceAlloc(factory.pools.dataAck);
    indPtr->sessionRef = factory.cache.activeSessionRef[factory.cache.activeInstance];
    indPtr->result = Utility::Convert::DataAckResult(indication.errorCode);
    indPtr->transactionId = indication.transactionId;

    LE_INFO("PA DataAckHandler: reporting event, activeSessionRef[%u]=%p result=%d transactionId=%"
        PRIu64, factory.cache.activeInstance, (void*)indPtr->sessionRef, indPtr->result,
        indPtr->transactionId);

    le_event_ReportWithRefCounting(factory.events.dataAck, (void*)indPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * PA callback for incoming NTN data indications. Creates a client-owned DataPacket reference from
 * the received bytes and forwards it to the client.
 */
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * Internal handler (main thread) to create data packet ref and report to clients.
 * This runs on the main thread to safely manage le_ref_CreateRef.
 */
//--------------------------------------------------------------------------------------------------
static void IncomingDataInternalHandler
(
    void* reportPtr ///< [IN] Points to the DataPacket_t* being processed.
)
{
    auto& factory = Factory::GetInstance();

    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr in IncomingDataInternalHandler.");
        return;
    }

    DataPacket_t* packetPtr = *(DataPacket_t**)reportPtr;
    if (packetPtr == nullptr)
    {
        LE_ERROR("packetPtr is nullptr in IncomingDataInternalHandler.");
        return;
    }

    taf_ntn_DataPacketRef_t packetRef =
        (taf_ntn_DataPacketRef_t)le_ref_CreateRef(factory.maps.dataPacket, (void*)packetPtr);

    IncomingDataInd_t* indPtr = (IncomingDataInd_t*)le_mem_ForceAlloc(factory.pools.incomingData);
    indPtr->sessionRef = factory.cache.activeSessionRef[factory.cache.activeInstance];
    indPtr->packetRef = packetRef;

    LE_INFO("IncomingDataInternalHandler: created ref, activeSessionRef[%u]=%p packetRef=%p size=%"
        PRIu32, factory.cache.activeInstance, (void*)indPtr->sessionRef, (void*)packetRef,
        packetPtr->size);

    le_event_ReportWithRefCounting(factory.events.incomingData, (void*)indPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * PA callback for incoming data indications.
 * Copy data only; defer ref creation to internal handler (main thread).
 */
//--------------------------------------------------------------------------------------------------
static void IncomingDataHandler
(
    taf_pa_ntn_IncomingDataIndication_t indication, ///< [IN] Indication payload.
    void* contextPtr                                ///< [IN] Context.
)
{
    auto& factory = Factory::GetInstance();

    if (indication.data == nullptr || indication.size == 0)
    {
        LE_ERROR("Invalid incoming data indication.");
        return;
    }

    uint32_t size = indication.size;
    if (size > TAF_NTN_MAX_DATA_BYTES)
    {
        LE_WARN("Incoming data size %" PRIu32 " exceeds %d, truncating.", size,
            TAF_NTN_MAX_DATA_BYTES);
        size = TAF_NTN_MAX_DATA_BYTES;
    }

    DataPacket_t* packetPtr = (DataPacket_t*)le_mem_ForceAlloc(factory.pools.dataPacket);
    packetPtr->size = size;
    memcpy(packetPtr->data, indication.data, size);

    LE_INFO("PA IncomingDataHandler: data copied, size=%" PRIu32, size);

    le_event_ReportWithRefCounting(factory.events.internalIncomingData, (void*)packetPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * PA callback for terrestrial cellular coverage indications.
 */
//--------------------------------------------------------------------------------------------------
static void CellularCoverageAvailableHandler
(
    taf_pa_ntn_CellularCoverageAvailableIndication_t indication, ///< [IN] Indication payload.
    void* contextPtr                                             ///< [IN] Context.
)
{
    auto& factory = Factory::GetInstance();

    CellularCoverageInd_t* indPtr =
        (CellularCoverageInd_t*)le_mem_ForceAlloc(factory.pools.cellularCoverage);
    indPtr->sessionRef = factory.cache.activeSessionRef[factory.cache.activeInstance];
    indPtr->available = indication.isCellularCoverageAvailable;

    LE_INFO("PA CellularCoverageAvailableHandler: reporting event, activeSessionRef[%u]=%p "
        "available=%d", factory.cache.activeInstance, (void*)indPtr->sessionRef, indPtr->available);

    le_event_ReportWithRefCounting(factory.events.cellularCoverage, (void*)indPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * PA callback for location fix request indications.
 */
//--------------------------------------------------------------------------------------------------
static void LocationFixRequestHandler
(
    taf_pa_ntn_LocationFixRequestIndication_t indication, ///< [IN] Indication payload.
    void* contextPtr                                      ///< [IN] Context.
)
{
    auto& factory = Factory::GetInstance();

    LocationFixRequestInd_t* indPtr =
        (LocationFixRequestInd_t*)le_mem_ForceAlloc(factory.pools.locationFixRequest);
    indPtr->sessionRef = factory.cache.activeSessionRef[factory.cache.activeInstance];
    indPtr->reason = Utility::Convert::LocationFixRequestReason(indication.reason);

    LE_INFO("PA LocationFixRequestHandler: reporting event, activeSessionRef[%u]=%p reason=%d",
        factory.cache.activeInstance, (void*)indPtr->sessionRef, indPtr->reason);

    le_event_ReportWithRefCounting(factory.events.locationFixRequest, (void*)indPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * PA callback for NTN band update indications.
 */
//--------------------------------------------------------------------------------------------------
static void NtnBandUpdateHandler
(
    taf_pa_ntn_NtnBandUpdateIndication_t indication, ///< [IN] Indication payload.
    void* contextPtr                                 ///< [IN] Context.
)
{
    auto& factory = Factory::GetInstance();

    BandUpdateInd_t* indPtr = (BandUpdateInd_t*)le_mem_ForceAlloc(factory.pools.bandUpdate);
    indPtr->sessionRef = factory.cache.activeSessionRef[factory.cache.activeInstance];
    indPtr->band = indication.bandValue;

    LE_INFO("PA NtnBandUpdateHandler: reporting event, activeSessionRef[%u]=%p band=%" PRIu64,
        factory.cache.activeInstance, (void*)indPtr->sessionRef, indPtr->band);

    le_event_ReportWithRefCounting(factory.events.bandUpdate, (void*)indPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * SIGTERM handler. Deinitializes the PA layer and exits cleanly.
 */
//--------------------------------------------------------------------------------------------------
static void SigTermEventHandler
(
    int sigNum ///< [IN] Signal number received (expected: SIGTERM).
)
{
    LE_INFO("SigTermEventHandler signal : %d", sigNum);

    taf_pa_result_t result = taf_pa_ntn_Deinit();
    if (result != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ntn_Deinit reason=%d", result);
    }
    else
    {
        LE_INFO("NTN platform adaptor shutdown complete.");
    }

    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
// Factory
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Gets the singleton factory instance.
 */
//--------------------------------------------------------------------------------------------------
Factory& Factory::GetInstance
(
    void
)
{
    static Factory instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization: creates events, pools and maps, registers PA indication handlers and
 * arms the SIGTERM handler.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    auto& factory = Factory::GetInstance();

    // Events fanned out to clients.
    factory.events.stateChange = le_event_CreateIdWithRefCounting("StateChange");
    factory.events.capabilitiesChange = le_event_CreateIdWithRefCounting("CapabilitiesChange");
    factory.events.dataAck = le_event_CreateIdWithRefCounting("DataAck");
    factory.events.incomingData = le_event_CreateIdWithRefCounting("IncomingData");
    factory.events.internalIncomingData = le_event_CreateIdWithRefCounting("InternalIncomingData");
    factory.events.cellularCoverage = le_event_CreateIdWithRefCounting("CellularCoverage");
    factory.events.locationFixRequest = le_event_CreateIdWithRefCounting("LocationFixRequest");
    factory.events.bandUpdate = le_event_CreateIdWithRefCounting("BandUpdate");

    // Memory pools.
    factory.pools.session = le_mem_InitStaticPool(session, SESSION_MAX_COUNT, sizeof(Session_t));
    factory.pools.dataPacket = le_mem_InitStaticPool(dataPacket, DATA_PACKET_MAX_COUNT,
        sizeof(DataPacket_t));
    factory.pools.locationFix = le_mem_InitStaticPool(locationFix, LOCATION_FIX_MAX_COUNT,
        sizeof(LocationFix_t));
    factory.pools.systemSelectionList = le_mem_InitStaticPool(systemSelectionList,
        SYSTEM_SELECTION_LIST_MAX_COUNT, sizeof(SystemSelectionList_t));
    factory.pools.specifier = le_mem_InitStaticPool(specifier, SYSTEM_SELECTION_SPEC_MAX_COUNT,
        sizeof(SystemSelectionSpecifier_t));
    factory.pools.safeRef = le_mem_InitStaticPool(safeRef, SAFE_REF_MAX_COUNT, sizeof(SafeRef_t));

    factory.pools.stateChange = le_mem_CreatePool("StateChange", sizeof(StateChangeInd_t));
    factory.pools.capabilitiesChange = le_mem_CreatePool("CapabilitiesChange",
        sizeof(CapabilitiesChangeInd_t));
    factory.pools.dataAck = le_mem_CreatePool("DataAck", sizeof(DataAckInd_t));
    factory.pools.incomingData = le_mem_CreatePool("IncomingData", sizeof(IncomingDataInd_t));
    factory.pools.cellularCoverage = le_mem_CreatePool("CellularCoverage",
        sizeof(CellularCoverageInd_t));
    factory.pools.locationFixRequest = le_mem_CreatePool("LocationFixRequest",
        sizeof(LocationFixRequestInd_t));
    factory.pools.bandUpdate = le_mem_CreatePool("BandUpdate", sizeof(BandUpdateInd_t));

    // Reference maps.
    factory.maps.session = le_ref_CreateMap("Session", SESSION_MAX_COUNT);
    factory.maps.dataPacket = le_ref_CreateMap("DataPacket", DATA_PACKET_MAX_COUNT);
    factory.maps.locationFix = le_ref_CreateMap("LocationFix", LOCATION_FIX_MAX_COUNT);
    factory.maps.systemSelectionList = le_ref_CreateMap("SystemSelectionList",
        SYSTEM_SELECTION_LIST_MAX_COUNT);
    factory.maps.specifier = le_ref_CreateMap("SystemSelectionSpecifier",
        SYSTEM_SELECTION_SPEC_MAX_COUNT);
    factory.maps.safeRef = le_ref_CreateMap("NtnSafeRef", SAFE_REF_MAX_COUNT);

    for (uint32_t i = 0; i < INSTANCE_MAX_COUNT; i++)
        factory.cache.activeSessionRef[i] = nullptr;

    taf_pa_result_t result = taf_pa_ntn_Init();
    if (result != TAF_PA_OK)
    {
        LE_WARN("[PA_CALL_FAILED] api=taf_pa_ntn_Init reason=%d", result);
    }

    taf_pa_ntn_NtnStateChangeHandlerRef_t stateChangeRef = nullptr;
    taf_pa_ntn_CapabilitiesChangeHandlerRef_t capabilitiesChangeRef = nullptr;
    taf_pa_ntn_DataAckHandlerRef_t dataAckRef = nullptr;
    taf_pa_ntn_IncomingDataHandlerRef_t incomingDataRef = nullptr;
    taf_pa_ntn_CellularCoverageAvailableHandlerRef_t cellularCoverageRef = nullptr;
    taf_pa_ntn_LocationFixRequestHandlerRef_t locationFixRequestRef = nullptr;
    taf_pa_ntn_NtnBandUpdateHandlerRef_t bandUpdateRef = nullptr;

#define ADD_PA_NTN_HANDLER(addFunc, handlerFunc, handlerRef) \
    do \
    { \
        taf_pa_result_t _addRes = addFunc(0, handlerFunc, nullptr, &(handlerRef)); \
        if (_addRes != TAF_PA_OK) \
        { \
            LE_ERROR("[PA_CALL_FAILED] api=" #addFunc " reason=%d", _addRes); \
        } \
    } while (0)

    ADD_PA_NTN_HANDLER(taf_pa_ntn_AddNtnStateChangeHandler, NtnStateChangeHandler,
        stateChangeRef);
    ADD_PA_NTN_HANDLER(taf_pa_ntn_AddCapabilitiesChangeHandler, CapabilitiesChangeHandler,
        capabilitiesChangeRef);
    ADD_PA_NTN_HANDLER(taf_pa_ntn_AddDataAckHandler, DataAckHandler, dataAckRef);
    ADD_PA_NTN_HANDLER(taf_pa_ntn_AddIncomingDataHandler, IncomingDataHandler, incomingDataRef);
    ADD_PA_NTN_HANDLER(taf_pa_ntn_AddCellularCoverageAvailableHandler,
        CellularCoverageAvailableHandler, cellularCoverageRef);
    ADD_PA_NTN_HANDLER(taf_pa_ntn_AddLocationFixRequestHandler, LocationFixRequestHandler,
        locationFixRequestRef);
    ADD_PA_NTN_HANDLER(taf_pa_ntn_AddNtnBandUpdateHandler, NtnBandUpdateHandler, bandUpdateRef);

#undef ADD_PA_NTN_HANDLER

    // Register internal event handler
    le_event_AddHandler("IncomingDataInternalHandler", factory.events.internalIncomingData, IncomingDataInternalHandler);

    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, SigTermEventHandler);

    LE_INFO("NTN service is ready.");
}
