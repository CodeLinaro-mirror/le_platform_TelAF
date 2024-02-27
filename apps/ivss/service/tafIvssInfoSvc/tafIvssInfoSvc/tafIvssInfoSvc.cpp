/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssInfoSvc.hpp"

std::shared_ptr<tafIvssInfoSvcStubImpl> ivssInfoSvc;
using namespace std;

COMPONENT_INIT
{
    LE_INFO("Start tafIvssInfoSvc Registered!");

    // Initialzie the reception and sending thread.
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "modem.InfoSvc";
    std::string connection = "ivssInfo-service";

    std::shared_ptr<tafIvssInfoSvcStubImpl> myService =
        std::make_shared<tafIvssInfoSvcStubImpl>();
    bool successfullyRegistered = runtime->registerService(domain, instance, myService, connection);

    while (!successfullyRegistered)
    {
        LE_INFO("tafIvssInfoSvc Register Service failed, trying again in 100 milliseconds...");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        successfullyRegistered = runtime->registerService(domain, instance, myService, connection);
    }

    ivssInfoSvc = myService;
    myService->Init();

    LE_INFO("Start tafIvssInfoSvc successfully! ");
}
