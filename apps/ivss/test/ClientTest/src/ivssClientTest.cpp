/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <iostream>
#include <string>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <CommonAPI/CommonAPI.hpp>
#include <v0/com/qualcomm/qti/modem/RadioSvcProxy.hpp>
#include <v0/com/qualcomm/qti/modem/SimSvcProxy.hpp>
#include <v0/com/qualcomm/qti/modem/InfoSvcProxy.hpp>

#define IVSS_TEST_SVC_RADIO_MASK 0x1
#define IVSS_TEST_SVC_SIM_MASK 0x10
#define IVSS_TEST_SVC_INFO_MASK 0x100
#define IVSS_TEST_SVC_MASK_ALL 0xFFFFFFFF

using namespace v0::com::qualcomm::qti::modem;
//--------------------------------------------------------------------------------------------------
/**
 * Print Radio Access Technology.
 */
//--------------------------------------------------------------------------------------------------

#define CHECK_RETURN_VALUE(expression, message) \
    do { \
        if (!(expression)) { \
            std::cerr << "Error: " << (message) << std::endl; \
            return false; \
        } \
    } while(0);

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

std::string StateToString
(
    SimSvc::States simState ///< [IN] SIM card State.
)
{
    std::string stateString;

    switch (simState)
    {
        case SimSvc::States::PRESENT:
            stateString = "PRESENT";
            break;
        case SimSvc::States::ABSENT:
            stateString = "ABSENT";
            break;
        case SimSvc::States::READY:
            stateString = "READY";
            break;
        case SimSvc::States::RESTRICTED:
            stateString = "RESTRICTED";
            break;
        case SimSvc::States::ERROR:
            stateString = "ERROR";
            break;
        case SimSvc::States::STATE_UNKNOWN:
            stateString = "STATE_UNKNOWN";
            break;
        default:
            stateString = "Unsupporteds";
            break;
    }

    return stateString;
}
//--------------------------------------------------------------------------------------------------
/**
 * main
 */
//--------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    uint32_t svcMask = IVSS_TEST_SVC_MASK_ALL;
    if (argc >= 2) 
    {
        std::string inputMask = argv[1];
        std::istringstream iss(inputMask);

        if (inputMask.size() >= 2 && (inputMask.substr(0, 2) == "0x" || inputMask.substr(0, 2) == "0X")) {
            // Resolve to hexadecimal
            iss >> std::hex >> svcMask;
        } else {
            // Resolve to decimal
            iss >> std::dec >> svcMask;
        }
        if (iss.fail()) {
            std::cerr << "Invalid value: " << inputMask << std::endl;
            return 1;
        }

        std::cout << "Decimal value: " << svcMask << std::endl;
    }

    // commonapi proxy init
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    CommonAPI::CallStatus callStatus;
    CommonTypes::Result methodError;
    CommonTypes::OnOffType power;
    CommonTypes::PhoneId phoneId = CommonTypes::PhoneId::PHONE_ID_1;

    std::shared_ptr<RadioSvcProxy<>> radioProxyKeep;
    std::shared_ptr<SimSvcProxy<>> simProxyKeep;
    std::shared_ptr<InfoSvcProxy<>> infoProxyKeep;

    if (svcMask & IVSS_TEST_SVC_RADIO_MASK) {
        // commonapi proxy init
        std::shared_ptr<RadioSvcProxy<>> radioProxy = runtime->buildProxy < RadioSvcProxy > (
            "local", "modem.RadioSvc", "radioSvcTest");
        std::cout << "Checking availability!" << std::endl;
        while (!radioProxy->isAvailable())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        std::cout << "radioProxy Available..." << std::endl;
        radioProxyKeep = radioProxy;

        // Subscribe to broadcast
        radioProxy->getRadioRatEvent().subscribe([&](const RadioSvc::Rat& rat,
            const CommonTypes::PhoneId& phoneId)
            {
                std::cout << "======== GetRadioRatEvent Test ========" << std::endl;
                std::cout << "Rat change to :" << RatToString(rat) << std::endl;
                std::cout << "phoneId = " << static_cast<unsigned int>(phoneId)
                    << std::endl << std::endl;
            }
        );

        radioProxy->getSignalStrengthEvent().subscribe([&](const RadioSvc::Rat& rat,
            const int32_t& ss, const int32_t& rsrp, const CommonTypes::PhoneId& phoneId)
            {
                std::cout << "======== GetSignalStrengthEvent Test ========" << std::endl;
                std::cout << "Rat is :" << RatToString(rat) << std::endl;
                std::cout << "phoneId = " << static_cast<unsigned int>(phoneId) << "  ss = " << ss
                        << "  rsrp = " << rsrp << std::endl<< std::endl;
            }
        );

        std::cout << "======== setPower OFF Test ========" << std::endl;
        power = CommonTypes::OnOffType::OFF;
        radioProxy->SetRadioPower(phoneId, power, callStatus, methodError);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "SetRadioPower SUCCESS " << std::endl << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::cout << "======== getPower Test ========" << "'\n";
        radioProxy->GetRadioPower(phoneId, callStatus, methodError, power);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "GetRadioPower: " << (power == CommonTypes::OnOffType::ON ? "ON'" : "OFF'")
            << std::endl << std::endl;

        std::cout << "======== setPower ON Test ========" << std::endl;
        power = CommonTypes::OnOffType::ON;
        radioProxy->SetRadioPower(phoneId, power, callStatus, methodError);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "SetRadioPower SUCCESS " << std::endl << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::cout << "======== getPower Test ========" << "'\n";
        radioProxy->GetRadioPower(phoneId, callStatus, methodError, power);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "GetRadioPower: " << (power == CommonTypes::OnOffType::ON ? "ON'" : "OFF'")
            << std::endl << std::endl;

        RadioSvc::Rat rat;
        int32_t ss;
        int32_t rsrp;
        std::cout << "======== get LTE signal strength Test ========" << "'\n";
        rat = RadioSvc::Rat::RAT_LTE;
        radioProxy->GetSignalStrength(phoneId, rat, callStatus, methodError, ss, rsrp);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "get NR5G signal strength: ss=" << ss << " dBm, rsrp=" << rsrp << " dBm"
            << std::endl << std::endl;

        bool isManual;
        std::string mcc;
        std::string mnc;
        std::cout << "======== get Register Mode Test ========" << "'\n";
        rat = RadioSvc::Rat::RAT_LTE;
        radioProxy->GetRegisterMode(phoneId, callStatus, methodError, isManual, mcc, mnc);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "get Register Mode: isManual=" << isManual << " , mcc=" << mcc << " mnc=" << mnc
            << std::endl << std::endl;

        std::cout << "======== set Automatic Register Mode Test ========" << "'\n";
        rat = RadioSvc::Rat::RAT_LTE;
        radioProxy->SetAutomaticRegisterMode(phoneId, callStatus, methodError);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "set Automatic Register Mode success" << std::endl << std::endl;
    }

    if (svcMask & IVSS_TEST_SVC_SIM_MASK) {
        std::shared_ptr<SimSvcProxy<>> simProxy = runtime->buildProxy < SimSvcProxy > ("local",
            "modem.SimSvc", "SimSvcTest");
        std::cout << "Checking availability!" << std::endl;
        while (!simProxy->isAvailable())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        std::cout << "simProxy Available..." << std::endl;
        simProxyKeep = simProxy;

        // Subscribe to broadcast
        simProxy->getSimStateEvent().subscribe([&](const CommonTypes::PhoneId& phoneId,
            const SimSvc::States& simState)
            {
                std::cout << "======== getSimStateEvent Test ========" << std::endl;
                std::cout << "SimState change to :" << StateToString(simState) << std::endl;
                std::cout << "phoneId = " << static_cast<unsigned int>(phoneId)
                    << std::endl << std::endl;
            }
        );

        std::string imsi;
        std::cout << "======== get Sim Imsi Test ========" << "'\n";
        simProxy->GetImsi(phoneId, callStatus, methodError, imsi);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "get Imsi: " << imsi << std::endl << std::endl;

        SimSvc::States simState;
        std::cout << "======== get SimState Test ========" << "'\n";
        simProxy->GetState(phoneId, callStatus, methodError, simState);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "get Status: " << StateToString(simState) << std::endl << std::endl;
    }

    if (svcMask & IVSS_TEST_SVC_INFO_MASK) {
        std::shared_ptr<InfoSvcProxy<>> infoProxy = runtime->buildProxy < InfoSvcProxy > ("local",
            "modem.InfoSvc", "InfoSvcTest");;
        std::cout << "Checking availability!" << std::endl;
        while (!infoProxy->isAvailable())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        std::cout << "infoProxy Available..." << std::endl;
        infoProxyKeep = infoProxy;

        std::string imei;
        std::cout << "======== get Imei Test ========" << "'\n";
        infoProxy->GetImei(callStatus, methodError, imei);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "get Imei: " << imei << std::endl << std::endl;
    }

    while (true) {
        std::cout << "Waiting for event... (Abort with CTRL+C)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));   
    }

    return 0;
}
