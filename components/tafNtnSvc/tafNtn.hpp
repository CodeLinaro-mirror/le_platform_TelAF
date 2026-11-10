/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFNTN_HPP
#define TAFNTN_HPP

#include "legato.h"
#include "interfaces.h"

#include <string>

#include "tafNtnPa.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Number of NTN operation contexts (instances) managed by this service. The first NTN target
 * (NB-IoT/NB1) exposes a single default context.
 */
//--------------------------------------------------------------------------------------------------
#define INSTANCE_MAX_COUNT 2

//--------------------------------------------------------------------------------------------------
/**
 * Upper bounds for the safe-reference and memory pools owned by this component. These are sized
 * generously to accommodate concurrent client-owned objects.
 */
//--------------------------------------------------------------------------------------------------
#define SESSION_MAX_COUNT 8
#define DATA_PACKET_MAX_COUNT 32
#define LOCATION_FIX_MAX_COUNT 8
#define SYSTEM_SELECTION_LIST_MAX_COUNT 8
#define SYSTEM_SELECTION_SPEC_MAX_COUNT 64
#define SAFE_REF_MAX_COUNT (SYSTEM_SELECTION_SPEC_MAX_COUNT * 2)

//--------------------------------------------------------------------------------------------------
/**
 * NTN session context object referenced by a taf_ntn_SessionRef_t.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t instance; ///< Instance index bound to this session.
    uint8_t phoneId;   ///< Phone ID that created this session.
} Session_t;

//--------------------------------------------------------------------------------------------------
/**
 * Incoming NTN data packet object referenced by a taf_ntn_DataPacketRef_t.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t size;                       ///< Number of valid bytes in data[].
    uint8_t data[TAF_NTN_MAX_DATA_BYTES];///< Received packet content.
} DataPacket_t;

//--------------------------------------------------------------------------------------------------
/**
 * Location fix builder object referenced by a taf_ntn_LocationFixRef_t. Populated incrementally by
 * the SetLocation* APIs and flushed to the PA layer by taf_ntn_UpdateLocationFix().
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t instance;                ///< Instance index bound to this location fix.
    taf_pa_ntn_LocationFix_t fix;     ///< Accumulated location fix passed to the PA layer.
} LocationFix_t;

//--------------------------------------------------------------------------------------------------
/**
 * System selection specifier object referenced by a taf_ntn_SystemSelectionSpecifierRef_t. A
 * specifier holds at most one PLMN, an NTN band bitmask and zero or more EARFCNs.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool plmnValid;                        ///< True if mcc/mnc carry a valid PLMN.
    char mcc[TAF_NTN_MCC_MAX_LEN + 1];     ///< Mobile Country Code (null terminated).
    char mnc[TAF_NTN_MNC_MAX_LEN + 1];     ///< Mobile Network Code (null terminated).
    uint64_t bandMask[TAF_NTN_NTN_BAND_GROUP_NUM]; ///< NTN band bitmask.
    uint64_t earfcns[TAF_NTN_MAX_SYSTEM_SELECTION_EARFCNS]; ///< EARFCN list.
    uint32_t earfcnCount;                  ///< Number of valid entries in earfcns[].
    le_sls_Link_t link;                    ///< Link used to chain into the parent list.
    taf_ntn_SystemSelectionSpecifierRef_t ref; ///< Reference to this specifier (managed by factory).
} SystemSelectionSpecifier_t;

//--------------------------------------------------------------------------------------------------
/**
 * System selection list object referenced by a taf_ntn_SystemSelectionListRef_t. Owns its
 * specifiers and the safe references handed out by the Add/GetFirst/GetNext APIs.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t instance;         ///< Instance index bound to this list.
    le_sls_List_t specifierList;///< List of SystemSelectionSpecifier_t owned by this list.
    le_sls_List_t safeRefList; ///< List of safe references owned by this list.
    le_sls_Link_t* currPtr;    ///< Iterator cursor used by GetFirst/GetNext.
} SystemSelectionList_t;

//--------------------------------------------------------------------------------------------------
/**
 * Generic safe-reference holder used to track references handed out for list traversal so they can
 * be released when the owning list is deleted.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    void* safeRef;      ///< Opaque reference stored in the ref map.
    le_sls_Link_t link; ///< Link used to chain into an SLS list.
} SafeRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through a layered event for NTN state change indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_ntn_SessionRef_t sessionRef; ///< NTN session reference.
    taf_ntn_State_t state;           ///< NTN state.
} StateChangeInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through a layered event for NTN capability change indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_ntn_SessionRef_t sessionRef; ///< NTN session reference.
} CapabilitiesChangeInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through a layered event for NTN data acknowledgement indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_ntn_SessionRef_t sessionRef;  ///< NTN session reference.
    taf_ntn_DataAckResult_t result;   ///< Data acknowledgement result.
    uint64_t transactionId;           ///< Transaction ID returned by taf_ntn_SendData().
} DataAckInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through a layered event for incoming NTN data indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_ntn_SessionRef_t sessionRef;  ///< NTN session reference.
    taf_ntn_DataPacketRef_t packetRef;///< Incoming data packet reference.
} IncomingDataInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through a layered event for terrestrial cellular coverage indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_ntn_SessionRef_t sessionRef; ///< NTN session reference.
    bool available;                  ///< True if terrestrial cellular coverage is available.
} CellularCoverageInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through a layered event for location fix request indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_ntn_SessionRef_t sessionRef;             ///< NTN session reference.
    taf_ntn_LocationFixRequestReason_t reason;   ///< Location fix request reason.
} LocationFixRequestInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Payload forwarded through a layered event for NTN band update indications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_ntn_SessionRef_t sessionRef; ///< NTN session reference.
    uint64_t band;                   ///< NTN operating band.
} BandUpdateInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Event identifiers owned by the NTN service component.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_event_Id_t stateChange;          ///< NTN state change indications.
    le_event_Id_t capabilitiesChange;   ///< NTN capability change indications.
    le_event_Id_t dataAck;              ///< NTN data acknowledgement indications.
    le_event_Id_t incomingData;         ///< Incoming NTN data indications.
    le_event_Id_t internalIncomingData; ///< Internal event for deferred data ref creation (main thread).
    le_event_Id_t cellularCoverage;     ///< Terrestrial cellular coverage indications.
    le_event_Id_t locationFixRequest;   ///< Location fix request indications.
    le_event_Id_t bandUpdate;           ///< NTN band update indications.
} Event_t;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pools backing ref-counted objects used by this component.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_mem_PoolRef_t session;             ///< Pool for Session_t objects.
    le_mem_PoolRef_t dataPacket;          ///< Pool for DataPacket_t objects.
    le_mem_PoolRef_t locationFix;         ///< Pool for LocationFix_t objects.
    le_mem_PoolRef_t systemSelectionList; ///< Pool for SystemSelectionList_t objects.
    le_mem_PoolRef_t specifier;           ///< Pool for SystemSelectionSpecifier_t objects.
    le_mem_PoolRef_t safeRef;             ///< Pool for generic safe-reference holders.
    le_mem_PoolRef_t stateChange;         ///< Pool for StateChangeInd_t payloads.
    le_mem_PoolRef_t capabilitiesChange;  ///< Pool for CapabilitiesChangeInd_t payloads.
    le_mem_PoolRef_t dataAck;             ///< Pool for DataAckInd_t payloads.
    le_mem_PoolRef_t incomingData;        ///< Pool for IncomingDataInd_t payloads.
    le_mem_PoolRef_t cellularCoverage;    ///< Pool for CellularCoverageInd_t payloads.
    le_mem_PoolRef_t locationFixRequest;  ///< Pool for LocationFixRequestInd_t payloads.
    le_mem_PoolRef_t bandUpdate;          ///< Pool for BandUpdateInd_t payloads.
} Pool_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference maps used to validate and resolve opaque handles.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_ref_MapRef_t session;             ///< Map for Session_t handles.
    le_ref_MapRef_t dataPacket;          ///< Map for DataPacket_t handles.
    le_ref_MapRef_t locationFix;         ///< Map for LocationFix_t handles.
    le_ref_MapRef_t systemSelectionList; ///< Map for SystemSelectionList_t handles.
    le_ref_MapRef_t specifier;           ///< Map for SystemSelectionSpecifier_t handles.
    le_ref_MapRef_t safeRef;             ///< Map for safe-reference handles.
} Map_t;

//--------------------------------------------------------------------------------------------------
/**
 * Cached runtime state maintained per instance.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    /// Representative session reference used to tag indications for the single NTN context.
    taf_ntn_SessionRef_t activeSessionRef[INSTANCE_MAX_COUNT];
    uint32_t activeInstance;
} Cache_t;

//--------------------------------------------------------------------------------------------------
/**
 * Utility class for conversion and common helper operations.
 */
//--------------------------------------------------------------------------------------------------
class Utility
{
    public:
        class Convert
        {
            public:
                /**
                 * Converts a PA result to a Legato result.
                 *
                 * @return
                 *      - LE_OK if the PA layer returned 0.
                 *      - LE_FAULT if the PA layer returned -EFAULT, or any unmapped error.
                 *      - LE_TIMEOUT if the PA layer returned -ETIMEDOUT.
                 *      - LE_OUT_OF_RANGE if the PA layer returned -ERANGE.
                 *      - LE_BAD_PARAMETER if the PA layer returned TAF_PA_BAD_PARAMETER.
                 *      - LE_CAP_UNSUPPORTED if the PA layer returned TAF_PA_UNSUPPORTED.
                 *      - LE_NOT_PERMITTED if the PA layer returned TAF_PA_NOT_PERMITTED.
                 *      - LE_CAP_NOT_IMPLEMENTED if the PA layer returned TAF_PA_NOT_IMPLEMENTED.
                 *      - LE_UNAVAILABLE if the PA layer returned TAF_PA_UNAVAILABLE.
                 */
                static le_result_t Result
                (
                    taf_pa_result_t result ///< [IN] PA result.
                );

                /**
                 * Converts a public API phone ID to an internal instance index.
                 *
                 * @return
                 *      - Instance index in range [0, INSTANCE_MAX_COUNT).
                 *      - INSTANCE_MAX_COUNT when phoneId is invalid.
                 */
                static uint32_t PhoneToInstance
                (
                    uint8_t phoneId ///< [IN] Phone ID.
                );

                /**
                 * Converts a PA NTN state to the public NTN state.
                 */
                static taf_ntn_State_t State
                (
                    taf_pa_ntn_State_t state ///< [IN] PA NTN state.
                );

                /**
                 * Converts a PA data acknowledgement error code to the public data ack result.
                 */
                static taf_ntn_DataAckResult_t DataAckResult
                (
                    taf_pa_result_t errorCode ///< [IN] PA acknowledgement error code.
                );

                /**
                 * Converts a PA location fix request reason to the public reason.
                 */
                static taf_ntn_LocationFixRequestReason_t LocationFixRequestReason
                (
                    taf_pa_ntn_LocationFixRequestReason_t reason ///< [IN] PA request reason.
                );

                /**
                 * Converts a public location status to the PA location status.
                 */
                static taf_pa_ntn_LocationStatus_t LocationStatus
                (
                    taf_ntn_LocationStatus_t status ///< [IN] Public location status.
                );
        };

        /**
         * Wrapper callbacks used by Legato layered events to bridge generic event payloads to the
         * typed public handler signatures exposed by this service.
         */
        class LayeredFunction
        {
            public:
                /**
                 * Dispatches NTN state change indications and releases the ref-counted payload.
                 */
                static void StateChange
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches NTN capability change indications and releases the ref-counted payload.
                 */
                static void CapabilitiesChange
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches NTN data acknowledgement indications and releases the payload.
                 */
                static void DataAck
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches incoming NTN data indications and releases the ref-counted payload.
                 */
                static void IncomingData
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches terrestrial cellular coverage indications and releases the payload.
                 */
                static void CellularCoverage
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches location fix request indications and releases the ref-counted payload.
                 */
                static void LocationFixRequest
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );

                /**
                 * Dispatches NTN band update indications and releases the ref-counted payload.
                 */
                static void BandUpdate
                (
                    void* reportPtr,     ///< [IN] Ref-counted payload.
                    void* handlerFuncPtr ///< [IN] Client callback.
                );
        };
};

//--------------------------------------------------------------------------------------------------
/**
 * NTN service factory class. Owns the singleton runtime state (events, pools, maps, cache).
 */
//--------------------------------------------------------------------------------------------------
class Factory
{
    public:
        /**
         * Gets the singleton factory instance.
         *
         * @return
         *      - Reference to the singleton Factory instance.
         */
        static Factory& GetInstance
        (
            void
        );

        Event_t events; ///< Event identifiers used to fan out indications.
        Pool_t pools;   ///< Memory pools used by this component.
        Map_t maps;     ///< Reference maps used by this component.
        Cache_t cache;  ///< Cached runtime state.
};

#endif /* #ifndef TAFNTN_HPP */
