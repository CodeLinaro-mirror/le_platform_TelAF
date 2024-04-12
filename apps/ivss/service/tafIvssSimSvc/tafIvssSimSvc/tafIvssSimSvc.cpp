/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssSimSvc.hpp"

std::shared_ptr<tafIvssSimSvcImpl> ivssSimSvc;
using namespace std;

COMPONENT_INIT
{
    LE_INFO("Start tafIvssSimSvc Registered!");

    // Initialzie the reception and sending thread.
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "modem.SimSvc";
    std::string connection = "ivssSim-service";

    std::shared_ptr<tafIvssSimSvcImpl> myService = std::make_shared<tafIvssSimSvcImpl>();

    bool successfullyRegistered = runtime->registerService(domain, instance, myService, connection);

    while (!successfullyRegistered)
    {
        LE_INFO("tafIvssSimSvc Register Service failed, trying again in 100 milliseconds...");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        successfullyRegistered = runtime->registerService(domain, instance, myService, connection);
    }

    ivssSimSvc = myService;
    myService->Init();

    LE_INFO("Start tafIvssSimSvc successfully! ");
}
