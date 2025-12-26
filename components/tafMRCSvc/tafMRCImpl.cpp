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

static void RegisterIndication
(
    uint8_t registration
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

le_result_t Utility::Convert::Result
(
    pa_result_t result
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

taf_pa_mrc_Status_t Utility::Convert::Status
(
    taf_mrc_OtaOperationStatus_t status
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

taf_pa_mrc_Status_t Utility::Convert::Status
(
    taf_mrc_SyncStatus_t status
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

static void ProcessStatusHandler
(
    taf_pa_mrc_ProcessStatusIndication_t indication,
    void* contextPtr
)
{
    auto& mrcFactory = MRCFactory::GetInstance();
    if (indication.processValid && indication.process == TAF_PA_MRC_PROCESS_ABSYNC)
    {
        LE_INFO("AB sync proceeded by MRC.");
        le_sem_Post(mrcFactory.semaphores.abSync);
    }
}

MRCFactory& MRCFactory::GetInstance
(
    void
)
{
    static MRCFactory instance;
    return instance;
}

COMPONENT_INIT
{
    auto& mrcFactory = MRCFactory::GetInstance();

    mrcFactory.semaphores.abSync = le_sem_Create("abSync", 0);
    mrcFactory.maps.metrics = le_ref_InitStaticMap(metrics, METRICS_MAX_NUM);
    mrcFactory.pools.metrics = le_mem_InitStaticPool(metrics, METRICS_MAX_NUM, sizeof(Metrics_t));

    pa_result_t result = taf_pa_mrc_Init();
    if (result != PA_OK)
        LE_ERROR("Fail to initialize MRC platform adaptor.");
    else
    {
        RegisterIndication(ENABLE_INDICATION);

        taf_pa_mrc_AddProcessStatusHandler(ProcessStatusHandler, nullptr);

        LE_INFO("MRC platform adaptor is ready.");
    }
}