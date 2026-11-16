/*
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/*
* @file       tafHMEcallRec.cpp
* @brief      Implementation of the taf_hmsEcall API. It stores per-phone eCall recovery records
*             in memory so that the eCall service can read them back after it restarts.
*             The handlers only read and write memory and never call another service.
*/

#include "legato.h"
#include "interfaces.h"

// Phone IDs are 1 .. TAF_ECALL_REC_MAX_PHONE_ID (PHONE_ID_1 and PHONE_ID_2 in taf_types.api).
#define TAF_ECALL_REC_MAX_PHONE_ID 2
#define TAF_ECALL_REC_MAX_RECORDS  TAF_ECALL_REC_MAX_PHONE_ID

namespace
{
    struct EcallRecord_t
    {
        bool                      inUse;
        uint8_t                   phoneId;
        bool                      typeSet;
        taf_hmsEcall_Type_t       type;
        taf_hmsEcall_RecordRef_t  ref;
    };

    // One slot per phone, indexed by phoneId - 1.
    EcallRecord_t Records[TAF_ECALL_REC_MAX_RECORDS];

    // Safe reference map. It is global (not per client session) and records are never released
    // on session close, so a record survives a restart of the eCall service.
    le_ref_MapRef_t RecordRefMap = NULL;

    EcallRecord_t* LookupRecord(taf_hmsEcall_RecordRef_t recordRef)
    {
        if (RecordRefMap == NULL)
        {
            return NULL;
        }
        return (EcallRecord_t*)le_ref_Lookup(RecordRefMap, recordRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the eCall recovery record of a phone. An empty record is created if none exists.
 */
//--------------------------------------------------------------------------------------------------
taf_hmsEcall_RecordRef_t taf_hmsEcall_GetRecord
(
    uint8_t phoneId
)
{
    if ((phoneId < 1) || (phoneId > TAF_ECALL_REC_MAX_PHONE_ID))
    {
        LE_ERROR("Invalid phone ID %d", phoneId);
        return NULL;
    }

    EcallRecord_t* recPtr = &Records[phoneId - 1];

    if (RecordRefMap == NULL)
    {
        LE_ERROR("eCall record reference map is unavailable");
        return NULL;
    }

    if (!recPtr->inUse)
    {
        memset(recPtr, 0, sizeof(*recPtr));
        recPtr->phoneId = phoneId;
        recPtr->type = TAF_HMSECALL_TYPE_UNKNOWN;
        recPtr->ref = (taf_hmsEcall_RecordRef_t)le_ref_CreateRef(RecordRefMap, recPtr);
        if (recPtr->ref == NULL)
        {
            LE_ERROR("Failed to create eCall record reference for phone %d", phoneId);
            memset(recPtr, 0, sizeof(*recPtr));
            return NULL;
        }
        recPtr->inUse = true;
        LE_INFO("Created eCall record for phone %d", phoneId);
    }

    return recPtr->ref;
}

//--------------------------------------------------------------------------------------------------
/**
 * Releases an eCall recovery record.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hmsEcall_Delete
(
    taf_hmsEcall_RecordRef_t recordRef
)
{
    EcallRecord_t* recPtr = LookupRecord(recordRef);
    if (recPtr == NULL)
    {
        LE_ERROR("Invalid record reference %p", recordRef);
        return LE_BAD_PARAMETER;
    }

    LE_INFO("Deleted eCall record for phone %d", recPtr->phoneId);
    le_ref_DeleteRef(RecordRefMap, recordRef);
    memset(recPtr, 0, sizeof(*recPtr));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the eCall type of a record.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hmsEcall_SetType
(
    taf_hmsEcall_RecordRef_t recordRef,
    taf_hmsEcall_Type_t type
)
{
    EcallRecord_t* recPtr = LookupRecord(recordRef);
    if (recPtr == NULL)
    {
        LE_ERROR("Invalid record reference %p", recordRef);
        return LE_BAD_PARAMETER;
    }

    if ((type <= TAF_HMSECALL_TYPE_UNKNOWN) || (type > TAF_HMSECALL_TYPE_PRIVATE))
    {
        LE_ERROR("Invalid eCall type %d", type);
        return LE_BAD_PARAMETER;
    }

    recPtr->type = type;
    recPtr->typeSet = true;
    LE_INFO("Set eCall type %d for phone %d", type, recPtr->phoneId);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the eCall type of a record.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hmsEcall_GetType
(
    taf_hmsEcall_RecordRef_t recordRef,
    taf_hmsEcall_Type_t* typePtr
)
{
    EcallRecord_t* recPtr = LookupRecord(recordRef);
    if (recPtr == NULL)
    {
        LE_ERROR("Invalid record reference %p", recordRef);
        return LE_BAD_PARAMETER;
    }

    if (typePtr == NULL)
    {
        LE_ERROR("typePtr is NULL");
        return LE_BAD_PARAMETER;
    }

    if (!recPtr->typeSet)
    {
        return LE_NOT_FOUND;
    }

    *typePtr = recPtr->type;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the eCall recovery record store. Called from the tafHMSvc component init.
 */
//--------------------------------------------------------------------------------------------------
void taf_hmsEcallRec_Init
(
    void
)
{
    if (RecordRefMap != NULL)
    {
        // Already initialized. Creating the map again would orphan the existing references.
        LE_WARN("eCall recovery store is already initialized");
        return;
    }

    memset(Records, 0, sizeof(Records));
    RecordRefMap = le_ref_CreateMap("tafHmsEcallRecordRefMap", TAF_ECALL_REC_MAX_RECORDS);
    if (RecordRefMap == NULL)
    {
        LE_ERROR("Failed to create eCall record reference map");
        return;
    }
    LE_INFO("TelAF Health Monitor eCall recovery store init completed");
}
