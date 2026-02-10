/*
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "etsStubImpl.hpp"

COMPONENT_INIT
{
    LE_INFO("-- ETS svc initialize --");
    CommonAPI::Runtime::setProperty("LogContext", "ETS01S");
    CommonAPI::Runtime::setProperty("LibraryBase", "ETS");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "someip.testability.ETS";
    std::string connection = "ets_default_service";

    std::shared_ptr<etsStubImpl> etsService = etsStubImpl::getEtsServiceInstance();
    ETS::VersionType version(ETS::getInterfaceVersion().Major, ETS::getInterfaceVersion().Minor);
    etsService->setETSInterfaceVersionAttribute(version);
    bool successfullyRegistered = runtime->registerService(domain, instance, etsService, connection);

    while (!successfullyRegistered) {
        std::cout << "Register Service failed, trying again in 100 milliseconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        successfullyRegistered = runtime->registerService(domain, instance, etsService, connection);
    }

    std::cout << "Successfully Registered ETS Service!" << std::endl;

    while (true) {
        std::cout << "Waiting for calls... (Abort with CTRL+C)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}
