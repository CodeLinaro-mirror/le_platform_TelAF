/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssMngdSvc.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * The initialization of TelAF IVSS Data service component.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start tafIvssDataSvc Registered!");

    // Initialize the ivss mngd service.
    std::shared_ptr<CommonAPI::Runtime> mngdRuntime = CommonAPI::Runtime::get();
    auto ivssMngd = tafIvssMngdSvc::GetInstance();
    if (true != mngdRuntime->registerService("local", "modem.MngdSvc", ivssMngd, "ivssMngdSvc"))
    {
        LE_FATAL("tafIvssMngdSvc Register Service failed.");
    }
    ivssMngd->Init();


    LE_INFO("Start tafIvssDataSvc successfully! ");
}
