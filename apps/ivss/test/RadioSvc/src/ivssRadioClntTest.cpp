/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <iostream>
#include <string>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <CommonAPI/CommonAPI.hpp>
#include <v0/com/qualcomm/qti/modem/RadioSvcProxy.hpp>


using namespace v0::com::qualcomm::qti::modem;

//--------------------------------------------------------------------------------------------------
/**
 * Print Radio Access Technology.
 */
//--------------------------------------------------------------------------------------------------

std::string RatToString
(
    RadioSvc::Rat rat ///< [IN] Radio Access Technology enum.
)
{
    std::string ratString;

    switch (rat)
    {
        case RadioSvc::Rat::RAT_GSM:
            ratString = "GSM";
            break;
        case RadioSvc::Rat::RAT_UMTS:
            ratString = "UMTS";
            break;
        case RadioSvc::Rat::RAT_TDSCDMA:
            ratString = "TDSCDMA";
            break;
        case RadioSvc::Rat::RAT_LTE:
            ratString = "LTE";
            break;
        case RadioSvc::Rat::RAT_NR5G:
            ratString = "NR5G";
            break;
        default:
            ratString = "Unsupported";
            break;
    }

    return ratString;
}

int main()
{
    //LE_TEST_PLAN(LE_TEST_NO_PLAN);

    // commonapi proxy init
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    std::string domain = "local";
    std::string instance = "modem.RadioSvc";
    std::string connection = "radioSvcTest";
    std::shared_ptr<RadioSvcProxy<>> myProxy = runtime->buildProxy < RadioSvcProxy > (domain,
        instance, connection);

    std::cout << "Checking availability!" << std::endl;
    while (!myProxy->isAvailable())
    {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    std::cout << "Available..." << std::endl;

    // Subscribe to broadcast
    myProxy->getRadioRatEvent().subscribe([&](const RadioSvc::Rat& rat,
        const CommonTypes::PhoneId& phoneId)
        {
            std::cout << "======== GetRadioRatEvent Test ========" << "\n";
            std::cout << "Rat change to :" << RatToString(rat) << "\n";
            std::cout << "phoneId = " << static_cast<unsigned int>(phoneId) << "\n";
            std::cout << std::endl;
        }
    );

    myProxy->getSignalStrengthEvent().subscribe([&](const RadioSvc::Rat& rat, const int32_t& ss,
        const int32_t& rsrp, const CommonTypes::PhoneId& phoneId)
        {
            std::cout << "======== GetSignalStrengthEvent Test ========" << "\n";
            std::cout << "Rat is :" << RatToString(rat) << "\n";
            std::cout << "phoneId = " << static_cast<unsigned int>(phoneId) << "  ss = " << ss 
                    << "  rsrp = " << rsrp << "\n";
        }
    );

    //
    CommonAPI::CallStatus callStatus;
    CommonTypes::Result methodError;
    CommonTypes::OnOffType power;
    CommonTypes::PhoneId phoneId = CommonTypes::PhoneId::PHONE_ID_1;
    RadioSvc::Rat rat;
    int32_t ss;
    int32_t rsrp;


    std::cout << "======== setPower OFF Test ========" << "\n";
    power = CommonTypes::OnOffType::OFF;
    myProxy->SetRadioPower(phoneId, power, callStatus, methodError);
    if (callStatus != CommonAPI::CallStatus::SUCCESS) {
        std::cerr << "Remote call failed!\n";
        return -1;
    }
    std::cout << "SetRadioPower SUCCESS " << "\n";
    std::cout << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));


    std::cout << "======== getPower Test ========" << "'\n";
    myProxy->GetRadioPower(phoneId, callStatus, methodError, power);
    if (callStatus != CommonAPI::CallStatus::SUCCESS) {
        std::cerr << "Remote call failed!\n";
        return -1;
    }
    std::cout << "GetRadioPower: '" << (power == CommonTypes::OnOffType::ON ? "ON'" : "OFF'")
              << "'\n";
    std::cout << std::endl;


    std::cout << "======== setPower ON Test ========" << "\n";
    power = CommonTypes::OnOffType::ON;
    myProxy->SetRadioPower(phoneId, power, callStatus, methodError);
    if (callStatus != CommonAPI::CallStatus::SUCCESS) {
        std::cerr << "Remote call failed!\n";
        return -1;
    }
    std::cout << "SetRadioPower SUCCESS " << "\n";
    std::cout << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));


    std::cout << "======== getPower Test ========" << "'\n";
    myProxy->GetRadioPower(phoneId, callStatus, methodError, power);
    if (callStatus != CommonAPI::CallStatus::SUCCESS) {
        std::cerr << "Remote call failed!\n";
        return -1;
    }
    std::cout << "GetRadioPower: '" << (power == CommonTypes::OnOffType::ON ? "ON'" : "OFF'")
              << "'\n";
    std::cout << std::endl;


    std::cout << "======== get LTE signal strength Test ========" << "'\n";
    rat = RadioSvc::Rat::RAT_LTE;
    myProxy->GetSignalStrength(phoneId, rat, callStatus, methodError, ss, rsrp);
    if (callStatus != CommonAPI::CallStatus::SUCCESS) {
        std::cerr << "Remote call failed!\n";
        return -1;
    }
    std::cout << "get NR5G signal strength: ss=" << ss << " dBm, rsrp=" << rsrp << " dBm" << "'\n";
    std::cout << std::endl;


    while (true) {
        std::cout << "Waiting for event... (Abort with CTRL+C)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }

    return 0;
}
