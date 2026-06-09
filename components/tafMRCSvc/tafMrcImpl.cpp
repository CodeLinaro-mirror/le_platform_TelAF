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

//--------------------------------------------------------------------------------------------------
/**
 * Handler preference for operation status.
 */
//--------------------------------------------------------------------------------------------------
taf_pa_mrc_OpStatusHandlerRef_t taf_Mrc::opStatusHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Platform event thread.
 */
//--------------------------------------------------------------------------------------------------
void* taf_Mrc::PAEventThread
(
    void* contextPtr ///< [IN] Context
)
{
    opStatusHandlerRef = taf_pa_mrc_AddOpStatusHandler(OpStatusHandler, NULL);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for operation status.
 */
//--------------------------------------------------------------------------------------------------
void taf_Mrc::OpStatusHandler
(
    taf_pa_mrc_OperationIndication_t* indPtr, ///< [IN] Indication for operation status.
    void* contextPtr                          ///< [IN] Context.
)
{
    auto &tafMrc = taf_Mrc::GetInstance();

    switch (indPtr->status)
    {
        case TAF_PA_MRC_OP_STATUS_SUCCESS:
            LE_INFO("MRC indicates operation is Successful.");
            break;
        case TAF_PA_MRC_OP_STATUS_FAILURE:
            LE_ERROR("MRC indicates operation is Failed.");
            break;
        default:
            LE_WARN("MRC indicates operation is Unknown.");
            break;
    }

    switch (indPtr->operation)
    {
        case TAF_PA_MRC_OP_OTA_START:
            LE_INFO("MRC indicates OTA Started.");
            break;
        case TAF_PA_MRC_OP_OTA_RESUME:
            LE_INFO("MRC indicates OTA resumed.");
            break;
        case TAF_PA_MRC_OP_OTA_END:
            LE_INFO("MRC indicates OTA ended.");
            break;
        case TAF_PA_MRC_OP_ABSYNC:
            LE_INFO("MRC indicates AB sync.");
            sem_post(&tafMrc.syncSem);
            break;
        default:
            LE_WARN("MRC indicates Unknown operation.");
            break;
    }
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
    std::future<telux::common::ServiceStatus> initFuture = promisePtr->get_future();
    std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
        TAF_MRC_SVC_READY_TIMEOUT));
    if (std::future_status::timeout == waitStatus)
        LE_FATAL("Timeout waiting for filysystem.");

    telux::common::ServiceStatus serviceStatus = initFuture.get();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
        LE_FATAL("Fail to initiate Filesystem.");

    LE_INFO("Filesystem service is now available.");

    // 5. Create the listener object and register as a listener.
    otaOperationsListener = std::make_shared<taf_MrcOtaOperationsListener>();
    fsManager->registerListener(otaOperationsListener);

    sem_init(&syncSem, 0, 0);
    le_result_t result = taf_pa_mrc_Initialize(TAF_MRC_SVC_READY_TIMEOUT, TAF_MRC_MSG_RESP_TIMEOUT);
    if (result != LE_OK)
    {
        paReady = false;
        LE_WARN("Fail to initialize MRC platform adaptor.");
    }
    else
    {
        result = taf_pa_mrc_IndicationRegistration(TAF_PA_MRC_IND_BIT_MASK_OTA_ABSYNC_STATUS
            | TAF_PA_MRC_IND_BIT_MASK_IMMINENT, 1);
        if (result != LE_OK)
        {
            paReady = false;
            LE_ERROR("Fail to register MRC indications.");
        }
        else
        {
            // Create thread for platform adaptor event.
            le_sem_Ref_t semaphore = le_sem_Create("PAEventThreadSem", 0);
            le_thread_Ref_t threadRef = le_thread_Create("PAEventThread", PAEventThread, (void*)semaphore);
            le_thread_Start(threadRef);
            le_sem_Wait(semaphore);
            le_sem_Delete(semaphore);

            paReady = true;
            LE_INFO("MRC platform adaptor is ready.");
        }
    }

    result = taf_prop_hms_Initialize(TAF_MRC_SVC_READY_TIMEOUT, TAF_MRC_MSG_RESP_TIMEOUT);
    if (result == LE_UNSUPPORTED)
    {
        LE_WARN("HMS platform adaptor is not implemented.");
    }
    else if (result != LE_OK)
    {
        paReady = false;
        LE_WARN("Fail to initialize HMS platform adaptor.");
    }
    else
        LE_INFO("HMS platform adaptor is ready.");
}
