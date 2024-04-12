/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssRadioSvc.hpp"

std::shared_ptr<tafIvssRadioSvcStubImpl> ivssRadioSvc;
using namespace std;

COMPONENT_INIT
{
    LE_INFO("Start tafIvssRadioSvc Registered!");

    // Initialzie the reception and sending thread.
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "modem.RadioSvc";
    std::string connection = "ivssRadio-service";

    std::shared_ptr<tafIvssRadioSvcStubImpl> myService =
        std::make_shared<tafIvssRadioSvcStubImpl>();
    bool successfullyRegistered = runtime->registerService(domain, instance, myService, connection);

    while (!successfullyRegistered)
    {
        LE_INFO("tafIvssRadioSvc Register Service failed, trying again in 100 milliseconds...");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        successfullyRegistered = runtime->registerService(domain, instance, myService, connection);
    }

    ivssRadioSvc = myService;
    myService->Init();

    LE_INFO("Start tafIvssRadioSvc successfully! ");
}
