/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssMngdConnSvc.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * The initialization of TelAF IVSS Data service component.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start tafIvssDataSvc Registered!");

    // Initialize the ivss MngdConn service.
    std::shared_ptr<CommonAPI::Runtime> mngdConnRuntime = CommonAPI::Runtime::get();
    auto ivssMngdConn = tafIvssMngdConnSvc::GetInstance();
    if (true != mngdConnRuntime->registerService("local", "modem.MngdConnSvc", ivssMngdConn,
        "ivssDataSvc"))
    {
        LE_FATAL("tafIvssMngdConnSvc Register Service failed.");
    }
    ivssMngdConn->Init();


    LE_INFO("Start tafIvssDataSvc successfully! ");
}
