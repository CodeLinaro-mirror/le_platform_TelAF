/*
 *  Copyright (c) 2022, 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include <chrono>

#include "tafMrc.hpp"

using namespace tafsvc;

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
    std::promise<telux::common::ErrorCode> p;
    telux::common::ResponseCallback cb = [&p](telux::common::ErrorCode error) { p.set_value(error); };
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

    telux::common::ErrorCode error = p.get_future().get();
    TAF_ERROR_IF_RET_VAL(error != telux::common::ErrorCode::SUCCESS, LE_FAULT,
        "Callback with error(error = %d)", (int)error);

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    LE_DEBUG("Elapsed time: %lfs.", elapsedTime.count());

    return LE_OK;
}

void taf_Mrc::Init(void)
{
    // 1. Get platform factory.
    auto &platformFactory = telux::platform::PlatformFactory::getInstance();

    // 2. Prepare a callback that is invoked when the filesystem sub-system initialization is complete.
    std::promise<telux::common::ServiceStatus> p;
    auto initCb = [&p](telux::common::ServiceStatus status) {
        LE_INFO("Received service status: %d.", (int)status);
        p.set_value(status);
    };

    // 3. Get the filesystem manager.
    fsManager = platformFactory.getFsManager(initCb);
    TAF_ERROR_IF_RET_NIL(fsManager == nullptr, "Null ptr(fsManager)");
    LE_INFO("Obtained filesystem manager.");

    // 4. Wait until initialization is complete.
    p.get_future().get();
    TAF_ERROR_IF_RET_NIL(fsManager->getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE,
        "Filesystem service not available.");
    LE_INFO("Filesystem service is now available.");

    // 5. Create the listener object and register as a listener.
    otaOperationsListener = std::make_shared<taf_MrcOtaOperationsListener>();
    fsManager->registerListener(otaOperationsListener);
}
