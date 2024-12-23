/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssRadioSvc.hpp"
#include "tafIvssSimSvc.hpp"
#include "tafIvssInfoSvc.hpp"

using namespace std;

//--------------------------------------------------------------------------------------------------
/**
 * The initialization of TelAF IVSS Telephony service component.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start tafIvssTelephonySvc Registered!");

    // Initialize the ivss radio service.
    std::shared_ptr<CommonAPI::Runtime> radioRuntime = CommonAPI::Runtime::get();
    auto ivssRadio = tafIvssRadioSvc::GetInstance();
    if (true != radioRuntime->registerService("local", "telephony.RadioSvc", ivssRadio,
        "ivssTelephonySvc"))
    {
        LE_FATAL("tafIvssRadioSvc Register Service failed.");
    }
    ivssRadio->Init();

    // Initialize the ivss sim service.
    std::shared_ptr<CommonAPI::Runtime> simRuntime = CommonAPI::Runtime::get();
    auto ivssSim = tafIvssSimSvc::GetInstance();
    if (true != simRuntime->registerService("local", "telephony.SimSvc", ivssSim,
        "ivssTelephonySvc"))
    {
        LE_FATAL("tafIvssSimSvc Register Service failed.");
    }
    ivssSim->Init();

    // Initialize the ivss info service.
    std::shared_ptr<CommonAPI::Runtime> infoRuntime = CommonAPI::Runtime::get();
    auto ivssInfo = tafIvssInfoSvc::GetInstance();
    if (true != infoRuntime->registerService("local", "telephony.InfoSvc", ivssInfo,
        "ivssTelephonySvc"))
    {
        LE_FATAL("tafIvssInfoSvc Register Service failed.");
    }
    ivssInfo->Init();

    LE_INFO("Start tafIvssTelephonySvc successfully! ");
}
