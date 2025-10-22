/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include <chrono>

#include "tafMrc.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for EFS metrics.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(metricsPool, TAF_MRC_METRICS_MAX_NUM, sizeof(taf_MrcEfsMetrics_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static map for EFS metrics.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(metricsRefMap, TAF_MRC_METRICS_MAX_NUM);

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

static void ProcessStatusHandler
(
    taf_pa_mrc_ProcessStatusIndication_t indication,
    void* contextPtr
)
{
    auto& mrc = taf_Mrc::GetInstance();
    if (indication.processValid && indication.process == TAF_PA_MRC_PROCESS_ABSYNC)
    {
        LE_INFO("ABSYNC proceeded by MRC.");
        le_sem_Post(mrc.syncSem);
    }
}

taf_Mrc &taf_Mrc::GetInstance()
{
    static taf_Mrc instance;
    return instance;
}

void taf_MrcOtaOperationsListener::onServiceStatusChange(telux::common::ServiceStatus serviceStatus)
{
    LE_DEBUG("<SDK Listener> taf_MrcOtaOperationsListener --> onServiceStatusChange");
    switch (serviceStatus) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            LE_DEBUG("Ota operation service status: Available.");
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            LE_INFO("Ota operation service status: Unavailable.");
            break;
        case telux::common::ServiceStatus::SERVICE_FAILED:
            LE_ERROR("Ota operation service status: Failed.");
            break;
        default:
            LE_ERROR("Ota operation service status: Unknown.");
            break;
    }
}

le_result_t taf_Mrc::SendOtaMsg(taf_MrcOtaMsgType_t type)
{
    telux::platform::OtaOperation op;
    telux::platform::OperationStatus opStatus;
    telux::common::Status status;

    auto promisePtr = std::make_shared<std::promise<telux::common::ErrorCode>>();
    auto cb = [promisePtr](telux::common::ErrorCode err) {
        try {
            promisePtr->set_value(err);
        }
        catch (const std::future_error& e) {
            LE_ERROR("Future error in callback: %s", e.what());
        }
        catch (const std::exception& e) {
            LE_ERROR("Exception in callback: %s", e.what());
        }
        catch (...) {
            LE_ERROR("Unknown error in callback.");
        }
    };

    auto &tafMrc = taf_Mrc::GetInstance();

    std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
    switch (type) {
        case TAF_MRC_OTA_MSG_TYPE_START:
             op = telux::platform::OtaOperation::START;
             status = tafMrc.fsManager->prepareForOta(op, cb);
             break;
        case TAF_MRC_OTA_MSG_TYPE_RESUME:
             op = telux::platform::OtaOperation::RESUME;
             status = tafMrc.fsManager->prepareForOta(op, cb);
             break;
        case TAF_MRC_OTA_MSG_TYPE_END_SUCCESS:
             opStatus = telux::platform::OperationStatus::SUCCESS;
             status = tafMrc.fsManager->otaCompleted(opStatus, cb);
             break;
        case TAF_MRC_OTA_MSG_TYPE_END_FAILURE:
             opStatus = telux::platform::OperationStatus::FAILURE;
             status = tafMrc.fsManager->otaCompleted(opStatus, cb);
             break;
        case TAF_MRC_OTA_MSG_TYPE_ABSYNC:
             status = tafMrc.fsManager->startAbSync(cb);
             break;
        default:
             LE_ERROR("Error message type: %d.", (int)type);
             return LE_BAD_PARAMETER;
    }

    TAF_ERROR_IF_RET_VAL(status != telux::common::Status::SUCCESS, LE_FAULT,
        "Send OTA message failed(status = %d type = %d)", (int)status, (int)type);

    telux::common::ErrorCode error = promisePtr->get_future().get();
    TAF_ERROR_IF_RET_VAL(error != telux::common::ErrorCode::SUCCESS, LE_FAULT,
        "Callback with error(error = %d)", (int)error);

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs.", elapsedTime.count());

    return LE_OK;
}

void taf_Mrc::Init(void)
{
    metricsRefMap = le_ref_InitStaticMap(metricsRefMap, TAF_MRC_METRICS_MAX_NUM);
    metricsPool = le_mem_InitStaticPool(metricsPool, TAF_MRC_METRICS_MAX_NUM,
        sizeof(taf_MrcEfsMetrics_t));

    // 1. Get platform factory.
    auto &platformFactory = telux::platform::PlatformFactory::getInstance();

    // 2. Prepare a callback that is invoked when the filesystem sub-system initialization is complete.
    auto promisePtr = std::make_shared<std::promise<telux::common::ServiceStatus>>();
    auto initCb = [promisePtr](telux::common::ServiceStatus status) {
        try {
            LE_INFO("Received service status: %d", (int)status);
            promisePtr->set_value(status);
        }
        catch (const std::future_error& e) {
            LE_ERROR("Future error in callback: %s", e.what());
        }
        catch (const std::exception& e) {
            LE_ERROR("Exception in callback: %s", e.what());
        }
        catch (...) {
            LE_ERROR("Unknown error in callback.");
        }
    };

    // 3. Get the filesystem manager.
    fsManager = platformFactory.getFsManager(initCb);
    TAF_ERROR_IF_RET_NIL(fsManager == nullptr, "Null ptr(fsManager)");
    LE_INFO("Obtained filesystem manager.");

    // 4. Wait until initialization is complete.
    promisePtr->get_future().get();
    TAF_ERROR_IF_RET_NIL(fsManager->getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE,
        "Filesystem service not available.");
    LE_INFO("Filesystem service is now available.");

    // 5. Create the listener object and register as a listener.
    otaOperationsListener = std::make_shared<taf_MrcOtaOperationsListener>();
    fsManager->registerListener(otaOperationsListener);

    syncSem = le_sem_Create("syncSem", 0);
    pa_result_t result = taf_pa_mrc_Init();
    if (result != PA_OK)
    {
        paReady = false;
        LE_WARN("Fail to initialize MRC platform adaptor.");
    }
    else
    {
        RegisterIndication(ENABLE_INDICATION);
        taf_pa_mrc_AddProcessStatusHandler(ProcessStatusHandler, nullptr);

        paReady = true;
        LE_INFO("MRC platform adaptor is ready.");
    }
}
