/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafIvssLocationSvc.hpp"

using namespace std;

//--------------------------------------------------------------------------------------------------
/**
 * The initialization of TelAF IVSS Location service component.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start tafIvssLocationSvc Registered!");

    // Initialize the ivss location service.
    std::shared_ptr<CommonAPI::Runtime> locationRuntime = CommonAPI::Runtime::get();
    auto ivssLocation = tafIvssLocationSvc::GetInstance();
    if (true != locationRuntime->registerService("local", "telephony.LocationSvc", ivssLocation,
        "ivssLocationSvc"))
    {
        LE_FATAL("tafIvssLocationSvc Register Service failed.");
    }
    ivssLocation->Init();

    LE_INFO("Start tafIvssLocationSvc successfully! ");
}
