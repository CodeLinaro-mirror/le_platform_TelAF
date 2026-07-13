/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAFMRC_HPP
#define TAFMRC_HPP

#include "legato.h"
#include "interfaces.h"

#include "tafMrcPa.hpp"

#define DISABLE_INDICATION 0
#define ENABLE_INDICATION 1

#define METRICS_MAX_NUM 1
#define RESP_TIMEOUT 180

//--------------------------------------------------------------------------------------------------
/**
 * EFS metrics structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t maxCount;      ///< Max P/E count
    uint32_t minCount;      ///< Min P/E count
    uint32_t avgCount;      ///< Average P/E count
    uint32_t sdValue;       ///< Standard deviation of P/E count
    uint32_t badBlockCount; ///< Bad block count
    uint32_t blockCount;    ///< Number of valid per-block P/E counts
    uint32_t peCount[TAF_PA_MRC_EFS_PARTITION_BLOCKS]; ///< Per-block P/E counts
} Metrics_t;

//--------------------------------------------------------------------------------------------------
/**
 * Semaphores structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sem_Ref_t abSync; ///< Semaphore for AB sync
} Semaphore_t;

/**
 * Event identifiers owned by the MRC service component.
 */
typedef struct
{
    le_event_Id_t toggleBank;  ///< Event used to dispatch bank toggle indications to clients.
} Event_t;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pools structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_mem_PoolRef_t metrics;    ///< Pool for EFS metrics
    le_mem_PoolRef_t toggleBank; ///< Pool for toggling bank
} Pool_t;

//--------------------------------------------------------------------------------------------------
/**
 * Reference maps structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_ref_MapRef_t metrics; ///< Map for EFS metrics
} Map_t;

/**
 * Payload forwarded through the layered Legato event when a bank toggle indication is received.
 */
typedef struct
{
    taf_mrc_ToggleStatus_t status; ///< Toggle operation status reported to the client handler.
    taf_mrc_Bank_t bank;           ///< Target bank associated with the indication, if available.
    taf_mrc_Error_t error;         ///< Error code describing why the toggle failed, if applicable.
} ToggleBankInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Utility class for conversion.
 */
//--------------------------------------------------------------------------------------------------
class Utility
{
    public:
        class Convert
        {
            public:
                /**
                 * Converts PA result to Le result.
                 *
                 * @return
                 *      - LE_OK if the PA layer returned 0.
                 *      - LE_FAULT if the PA layer returned -EFAULT, or any unmapped error.
                 *      - LE_TIMEOUT if the PA layer returned -ETIMEDOUT.
                 *      - LE_BAD_PARAMETER if the PA layer returned -EINVAL.
                 *      - LE_UNSUPPORTED if the PA layer returned -ENOTSUP.
                 *      - LE_NOT_IMPLEMENTED if the PA layer returned -ENOSYS.
                 */
                static le_result_t Result
                (
                    pa_result_t result ///< [IN] PA result.
                );

                /**
                 * Converts OTA operation status to PA status.
                 *
                 * @return
                 *      - TAF_PA_MRC_STATUS_SUCCEEDED when status is TAF_MRC_OTA_OP_STATUS_SUCCESS.
                 *      - TAF_PA_MRC_STATUS_FAILED when status is TAF_MRC_OTA_OP_STATUS_FAILURE.
                 *      - TAF_PA_MRC_STATUS_UNKNOWN for any unsupported OTA status value.
                 */
                static taf_pa_mrc_Status_t Status
                (
                    taf_mrc_OtaOperationStatus_t status ///< [IN] OTA operation status.
                );

                /**
                 * Converts sync status to PA status.
                 *
                 * @return
                 *      - TAF_PA_MRC_STATUS_INITIATED when status is TAF_MRC_SYNC_STATUS_INIT.
                 *      - TAF_PA_MRC_STATUS_SUCCEEDED when status is TAF_MRC_SYNC_STATUS_SUCCESS.
                 *      - TAF_PA_MRC_STATUS_FAILED when status is TAF_MRC_SYNC_STATUS_FAILURE.
                 *      - TAF_PA_MRC_STATUS_UNKNOWN for any unsupported sync status value.
                 */
                static taf_pa_mrc_Status_t Status
                (
                    taf_mrc_SyncStatus_t status ///< [IN] Sync status.
                );

                /**
                 * Converts a public toggle status into the integer acknowledgement value expected 
                 * by the platform adaptor.
                 *
                 * @return
                 *      - LE_OK if the conversion succeeds.
                 *      - LE_BAD_PARAMETER if statusPtr is null or the status is unsupported.
                 */
                static le_result_t Status
                (
                    taf_mrc_ToggleStatus_t status, ///< [IN] Toggle status reported by the client.
                    int32_t* statusPtr             ///< [OUT] Integer value consumed by the PA layer.
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
                 * Dispatches a bank-toggle indication to the registered client callback and then
                 * releases the ref-counted event payload.
                 */
                static void ToggleBank
                (
                    void* reportPtr,      ///< [IN] Ref-counted ToggleBankInd_t payload.
                    void* handlerFuncPtr  ///< [IN] Client callback registered through the API.
                );
        };
};


//--------------------------------------------------------------------------------------------------
/**
 * MRC factory class.
 */
//--------------------------------------------------------------------------------------------------
class MRCFactory
{
    public:
        /**
         * Gets the instance of MRC factory.
         *
         * @return
         *      - Reference to the singleton MRC factory instance.
         */
        static MRCFactory& GetInstance
        (
            void
        );

        Semaphore_t semaphores; ///< Semaphores used to synchronize asynchronous PA responses.
        Event_t events;         ///< Event identifiers used to fan out indications to API clients.
        Pool_t pools;           ///< Memory pools backing temporary and ref-counted objects.
        Map_t maps;             ///< Reference maps used to validate and resolve opaque handles.
};

#endif
