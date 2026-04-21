/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafMRC.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for EFS metrics.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(metrics, METRICS_MAX_NUM, sizeof(Metrics_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static map for EFS metrics.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(metrics, METRICS_MAX_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Registers indication from PA.
 */
//--------------------------------------------------------------------------------------------------
static void RegisterIndication
(
    uint8_t registration ///< [IN] Registration mask.
)
{
    pa_result_t result = taf_pa_mrc_RegisterIndication(registration);
    switch(result)
    {
        case 0:
            if (registration == ENABLE_INDICATION)
                LE_INFO("Indication is enabled.");
            else
                LE_INFO("Indication is disabled.");
            break;
        case -ENOSYS:
        case -ENOTSUP:
            break;
        default:
            LE_ERROR("Failed to register indication.");
    }
}

//--------------------------------------------------------------------------------------------------
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
//--------------------------------------------------------------------------------------------------
le_result_t Utility::Convert::Result
(
    pa_result_t result ///< [IN] PA result.
)
{
    switch (result)
    {
        case 0:
            return LE_OK;
        case -EFAULT:
            return LE_FAULT;
        case -ETIMEDOUT:
            return LE_TIMEOUT;
        case -EINVAL:
            return LE_BAD_PARAMETER;
        case -ENOTSUP:
            return LE_UNSUPPORTED;
        case -ENOSYS:
            return LE_NOT_IMPLEMENTED;
        default:
            LE_INFO("Unknown result %d.", result);
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Converts OTA operation status to PA status.
 *
 * @return
 *      - TAF_PA_MRC_STATUS_SUCCEEDED when status is TAF_MRC_OTA_OP_STATUS_SUCCESS.
 *      - TAF_PA_MRC_STATUS_FAILED when status is TAF_MRC_OTA_OP_STATUS_FAILURE.
 *      - TAF_PA_MRC_STATUS_UNKNOWN for any unsupported OTA status value.
 */
//--------------------------------------------------------------------------------------------------
taf_pa_mrc_Status_t Utility::Convert::Status
(
    taf_mrc_OtaOperationStatus_t status ///< [IN] OTA operation status.
)
{
    switch (status)
    {
        case TAF_MRC_OTA_OP_STATUS_SUCCESS:
            return TAF_PA_MRC_STATUS_SUCCEEDED;
        case TAF_MRC_OTA_OP_STATUS_FAILURE:
            return TAF_PA_MRC_STATUS_FAILED;
        default:
            LE_INFO("Unknown status %d.", status);
    }

    return TAF_PA_MRC_STATUS_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Converts sync status to PA status.
 *
 * @return
 *      - TAF_PA_MRC_STATUS_INITIATED when status is TAF_MRC_SYNC_STATUS_INIT.
 *      - TAF_PA_MRC_STATUS_SUCCEEDED when status is TAF_MRC_SYNC_STATUS_SUCCESS.
 *      - TAF_PA_MRC_STATUS_FAILED when status is TAF_MRC_SYNC_STATUS_FAILURE.
 *      - TAF_PA_MRC_STATUS_UNKNOWN for any unsupported sync status value.
 */
//--------------------------------------------------------------------------------------------------
taf_pa_mrc_Status_t Utility::Convert::Status
(
    taf_mrc_SyncStatus_t status ///< [IN] Sync status.
)
{
    switch (status)
    {
        case TAF_MRC_SYNC_STATUS_INIT:
            return TAF_PA_MRC_STATUS_INITIATED;
        case TAF_MRC_SYNC_STATUS_SUCCESS:
            return TAF_PA_MRC_STATUS_SUCCEEDED;
        case TAF_MRC_SYNC_STATUS_FAILURE:
            return TAF_PA_MRC_STATUS_FAILED;
        default:
            LE_INFO("Unknown status %d.", status);
    }

    return TAF_PA_MRC_STATUS_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Converts a public toggle status into the integer acknowledgement value expected by
 * the platform adaptor.
 *
 * @return
 *      - LE_OK if the conversion succeeds.
 *      - LE_BAD_PARAMETER if statusPtr is null or the status is unsupported.
 */
//--------------------------------------------------------------------------------------------------
le_result_t Utility::Convert::Status
(
    taf_mrc_ToggleStatus_t status, ///< [IN] Toggle status reported by the client.
    int32_t* statusPtr             ///< [OUT] Integer value consumed by the PA layer.
)
{
    if (statusPtr == nullptr)
    {
        LE_ERROR("statusPtr is nullptr.");
        return LE_BAD_PARAMETER;
    }

    switch (status)
    {
        case TAF_MRC_TOGGLE_STATUS_SUCCEEDED:
            *statusPtr = 0;
            break;
        case TAF_MRC_TOGGLE_STATUS_FAILED:
            *statusPtr = 1;
            break;
        default:
            LE_ERROR("Unknown status %d.", status);
            return LE_BAD_PARAMETER;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Dispatches a bank-toggle indication to the registered client callback and then
 * releases the ref-counted event payload.
 */
//--------------------------------------------------------------------------------------------------
void Utility::LayeredFunction::ToggleBank
(
    void* reportPtr,     ///< [IN] Ref-counted ToggleBankInd_t payload.
    void* handlerFuncPtr ///< [IN] Client callback registered through the API.
)
{
    if (reportPtr == nullptr)
    {
        LE_ERROR("reportPtr is nullptr");
        return;
    }

    taf_mrc_ToggleBankHandlerFunc_t handlerFunc = (taf_mrc_ToggleBankHandlerFunc_t)handlerFuncPtr;
    if (handlerFunc != nullptr)
    {
        ToggleBankInd_t* indPtr = (ToggleBankInd_t*)reportPtr;
        handlerFunc(indPtr->status, indPtr->bank, indPtr->error, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for process status indication.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessStatusHandler
(
    taf_pa_mrc_ProcessStatusIndication_t indication, ///< [IN] Process status indication.
    void* contextPtr                                 ///< [IN] Context.
)
{
    auto& mrcFactory = MRCFactory::GetInstance();
    if (indication.processValid && indication.process == TAF_PA_MRC_PROCESS_ABSYNC)
    {
        LE_INFO("AB sync proceeded by MRC.");
        le_sem_Post(mrcFactory.semaphores.abSync);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for scrub status indications.
 *
 * When the platform adaptor requests a slot toggle, this function packages the request into a
 * ref-counted event payload so registered client handlers can acknowledge it asynchronously.
 */
//--------------------------------------------------------------------------------------------------
static void ScrubStatusHandler
(
    taf_pa_mrc_ScrubStatusIndication_t indication, ///< [IN] Scrub status indication.
    void* contextPtr                               ///< [IN] Context.
)
{
    auto& mrcFactory = MRCFactory::GetInstance();

    if (indication.slotToggleRequested)
    {
        ToggleBankInd_t* indPtr = (ToggleBankInd_t*)le_mem_ForceAlloc(mrcFactory.pools.toggleBank);

        indPtr->status = TAF_MRC_TOGGLE_STATUS_REQUESTED;
        indPtr->bank = TAF_MRC_BANK_UNKNOWN;
        indPtr->error = TAF_MRC_ERROR_NONE;

        le_event_ReportWithRefCounting(mrcFactory.events.toggleBank, (void*)indPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns the singleton factory instance that owns all shared runtime objects used by this
 * component, such as semaphores, events, pools, and reference maps.
 */
//--------------------------------------------------------------------------------------------------
MRCFactory& MRCFactory::GetInstance
(
    void
)
{
    static MRCFactory instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    auto& mrcFactory = MRCFactory::GetInstance();

    // Create the synchronization and dispatch primitives used while interacting with the PA layer.
    mrcFactory.semaphores.abSync = le_sem_Create("ABSync", 0);
    mrcFactory.events.toggleBank = le_event_CreateIdWithRefCounting("ToggleBank");
    mrcFactory.maps.metrics = le_ref_InitStaticMap(metrics, METRICS_MAX_NUM);
    mrcFactory.pools.metrics = le_mem_InitStaticPool(metrics, METRICS_MAX_NUM, sizeof(Metrics_t));
    mrcFactory.pools.toggleBank = le_mem_CreatePool("ToggleBank", sizeof(ToggleBankInd_t));

    pa_result_t result = taf_pa_mrc_Init();
    if (result != PA_OK)
        LE_ERROR("Fail to initialize MRC platform adaptor.");
    else
    {
        // Subscribe to asynchronous PA indications before exposing the service as ready.
        RegisterIndication(ENABLE_INDICATION);

        taf_pa_mrc_AddProcessStatusHandler(ProcessStatusHandler, nullptr);
        taf_pa_mrc_AddScrubStatusHandler(ScrubStatusHandler, nullptr);

        LE_INFO("MRC platform adaptor is ready.");
    }
}