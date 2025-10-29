/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafIvssSensorSvc.hpp"

using namespace std;

//--------------------------------------------------------------------------------------------------
/**
 * The initialization of TelAF IVSS Sensor service component.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start tafIvssSensorSvc Registered!");

    // Initialize the ivss sensor service.
    std::shared_ptr<CommonAPI::Runtime> sensorRuntime = CommonAPI::Runtime::get();
    auto ivssSensor = tafIvssSensorSvc::GetInstance();
    if (true != sensorRuntime->registerService("local", "telephony.SensorSvc", ivssSensor,
        "ivssSensorSvc"))
    {
        LE_FATAL("tafIvssSensorSvc Register Service failed.");
    }
    ivssSensor->Init();

    LE_INFO("Start tafIvssSensorSvc successfully! ");
}
