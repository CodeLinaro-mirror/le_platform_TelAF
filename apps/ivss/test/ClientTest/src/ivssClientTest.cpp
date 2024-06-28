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
#include <v0/com/qualcomm/qti/modem/MngdConnSvcProxy.hpp>

#define IVSS_TEST_SVC_RADIO_MASK 0x1
#define IVSS_TEST_SVC_SIM_MASK 0x10
#define IVSS_TEST_SVC_INFO_MASK 0x100
#define IVSS_TEST_SVC_MNGDCONN_MASK 0x1000
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

std::string RadioStatesToString
(
    RadioSvc::States radioState ///< [IN] Radio state.
)
{
    std::string statesString;
    switch (radioState)
    {
        case RadioSvc::States::ON:
            statesString = "ON";
            break;
        case RadioSvc::States::OFF:
            statesString = "OFF";
            break;
        case RadioSvc::States::UNAVAILABLE:
            statesString = "UNAVAILABLE";
            break;
        default:
            statesString = "Unsupported";
            break;
    }

    return statesString;
}

std::string CellInfoStatusToString
(
    RadioSvc::CellInfoStatus status ///< [IN] Cell Info.
)
{
    std::string statesString;
    switch (status)
    {
        case RadioSvc::CellInfoStatus::CELL_SERVING_CHANGED:
            statesString = "CELL_SERVING_CHANGED";
            break;
        case RadioSvc::CellInfoStatus::CELL_NEIGHBOR_CHANGED:
            statesString = "CELL_NEIGHBOR_CHANGED";
            break;
        case RadioSvc::CellInfoStatus::CELL_SERVING_AND_NEIGHBOR_CHANGED:
            statesString = "CELL_SERVING_AND_NEIGHBOR_CHANGED";
            break;
        default:
            statesString = "Unsupported";
            break;
    }

    return statesString;
}

std::string DataStateToString
(
    MngdConnSvc::DataState dataState ///< [IN] The data state.
)
{
    std::string statesString;
    switch (dataState)
    {
        case MngdConnSvc::DataState::DATA_DISCONNECTED:
            statesString = "DATA_DISCONNECTED";
            break;
        case MngdConnSvc::DataState::DATA_CONNECTED:
            statesString = "DATA_CONNECTED";
            break;
        case MngdConnSvc::DataState::DATA_CONNECTION_FAILED:
            statesString = "DATA_CONNECTION_FAILED";
            break;
        case MngdConnSvc::DataState::DATA_CONNECTION_STALLED:
            statesString = "DATA_CONNECTION_STALLED";
            break;
        default:
            statesString = "Unsupported";
            break;
    }

    return statesString;
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

        if (inputMask.size() >= 2 && (inputMask.substr(0, 2) == "0x" ||
            inputMask.substr(0, 2) == "0X"))
        {
            // Resolve to hexadecimal
            iss >> std::hex >> svcMask;
        }
        else 
        {
            // Resolve to decimal
            iss >> std::dec >> svcMask;
        }
        if (iss.fail())
        {
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
    std::shared_ptr<MngdConnSvcProxy<>> mngdConnProxyKeep;

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
        radioProxy->getRadioRatEvent().subscribe([&](const CommonTypes::PhoneId& phoneId,
            const RadioSvc::Rat& rat)
            {
                std::cout << "======== RadioRatEvent Test ========" << std::endl;
                std::cout << "Rat change to :" << RatToString(rat) << std::endl;
                std::cout << "phoneId = " << static_cast<unsigned int>(phoneId)
                    << std::endl << std::endl;
            }
        );

        radioProxy->getSignalStrengthEvent().subscribe([&](const CommonTypes::PhoneId& phoneId,
            const RadioSvc::Rat& rat, const int32_t& ss, const int32_t& rsrp)
            {
                std::cout << "======== SignalStrengthEvent Test ========" << std::endl;
                std::cout << "Rat is :" << RatToString(rat) << std::endl;
                std::cout << "phoneId = " << static_cast<unsigned int>(phoneId) << "  ss = " << ss
                    << "  rsrp = " << rsrp << std::endl<< std::endl;
            }
        );

        radioProxy->getRadioStateEvent().subscribe([&](const RadioSvc::States& radioState)
            {
                std::cout << "======== RadioStateEvent Test ========" << std::endl;
                std::cout << "Radio State change to :" << RadioStatesToString(radioState)
                    << std::endl << std::endl;
            }
        );

        radioProxy->getCellInfoEvent().subscribe([&](const CommonTypes::PhoneId& phoneId,
            const RadioSvc::CellInfoStatus& status)
            {
                std::cout << "======== CellInfoEvent Test ========" << std::endl;
                std::cout << "phoneId = " << static_cast<unsigned int>(phoneId)
                    << std::endl << std::endl;
                std::cout << "Cell Info change to :" << CellInfoStatusToString(status)
                    << std::endl << std::endl;
            }
        );

        std::cout << "======== setPower OFF Test ========" << std::endl;
        power = CommonTypes::OnOffType::OFF;
        radioProxy->SetRadioPower(phoneId, power, callStatus, methodError);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "SetRadioPower SUCCESS " << std::endl << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

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
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::cout << "======== getPower Test ========" << "'\n";
        radioProxy->GetRadioPower(phoneId, callStatus, methodError, power);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "GetRadioPower: " << (power == CommonTypes::OnOffType::ON ? "ON'" : "OFF'")
            << std::endl << std::endl;

        RadioSvc::Rat rat;
        RadioSvc::SignalMetrics signalStrength;
        std::cout << "======== get LTE signal strength Test ========" << "'\n";
        rat = RadioSvc::Rat::RAT_LTE;
        radioProxy->GetSignalStrength(phoneId, rat, callStatus, methodError, signalStrength);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        RadioSvc::LteSignalMetrics lteMetrics = signalStrength.get<RadioSvc::LteSignalMetrics>();
        std::cout << "get LTE signal strength: ss=" << lteMetrics.getSs() << " (dBm), rsrq=" 
            << lteMetrics.getRsrq() << " (dB), rsrp" << lteMetrics.getRsrp() << " (dBm), snr"
            << lteMetrics.getSnr() << " (0.1 dB)"
            << std::endl << std::endl;

        bool isManual;
        std::string mcc;
        std::string mnc;
        std::cout << "======== get Register Mode Test ========" << "'\n";
        rat = RadioSvc::Rat::RAT_LTE;
        radioProxy->GetRegisterMode(phoneId, callStatus, methodError, isManual, mcc, mnc);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "get Register Mode: isManual=" << isManual << " , mcc=" << mcc << " mnc="
            << mnc << std::endl << std::endl;


        std::cout << "======== set Automatic Register Mode Test ========" << "'\n";
        rat = RadioSvc::Rat::RAT_LTE;
        radioProxy->SetAutomaticRegisterMode(phoneId, callStatus, methodError);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "set Automatic Register Mode success" << std::endl << std::endl;


        std::cout << "======== Get Hardware Config Test ========" << "'\n";
        uint8_t totalSimCount = 0;
        uint8_t maxActiveSims = 0;
        RadioSvc::RatBitMask deviceRatCapMask = 0x0;
        RadioSvc::RatBitMask simRatCapMask = 0x0;
        radioProxy->GetHardwareConfig(phoneId, callStatus, methodError, totalSimCount,
            maxActiveSims, deviceRatCapMask, simRatCapMask);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "GetHardwareConfig: totalSimCount "
            << static_cast<unsigned int>(totalSimCount) << "'\n"
            << "maxActiveSims " << static_cast<unsigned int>(maxActiveSims) << "'\n"
            << "deviceRatCapMask "<< static_cast<unsigned int>(deviceRatCapMask) << "'\n"
            << "simRatCapMask "<< static_cast<unsigned int>(simRatCapMask) << "'\n"
            << std::endl << std::endl;


        std::cout << "======== Get Rat Preferences Test ========" << "'\n";
        RadioSvc::RatBitMask ratMask = 0x0;
        radioProxy->GetRatPreferences(phoneId, callStatus, methodError, ratMask);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "GetRatPreferences: ratMask " << static_cast<unsigned int>(ratMask)
            << std::endl << std::endl;


        std::cout << "======== Get Current Network Name Test ========" << "'\n";
        std::string longName;
        std::string shortName;
        radioProxy->GetCurrentNetworkName(phoneId, callStatus, methodError, longName, shortName);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "GetCurrentNetworkName: longName " << longName << " shortName " << shortName
            << std::endl << std::endl;


        std::cout << "======== Get NetRegState Test ========" << "'\n";
        RadioSvc::Rat getRat;
        uint32_t cellId;
        std::string getMcc;
        std::string getMnc;
        RadioSvc::NetRegState netReg;
        radioProxy->GetNetRegState(phoneId, callStatus, methodError, getRat, cellId, getMcc, getMnc,
            netReg);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "GetNetRegState: rat= " << RatToString(rat) << " , cellId=" << cellId 
            << " , mcc=" << mcc << " , mnc=" << mnc << " , netReg=" 
            << static_cast<unsigned int>(netReg) << std::endl << std::endl;


        std::cout << "======== Get NrDualConnectivityStatus Test ========" << "'\n";
        RadioSvc::NRDcnrRestriction statusDcnr;
        radioProxy->GetNrDualConnectivityStatus(phoneId, callStatus, methodError, statusDcnr);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "GetNrDualConnectivityStatus: statusDcnr "
            << static_cast<unsigned int>(statusDcnr) << std::endl << std::endl;


        std::cout << "======== Set SignalStrengthReportingCriteria  Test ========" << std::endl;
        power = CommonTypes::OnOffType::ON;
        RadioSvc::SigType sigType = RadioSvc::SigType::SIG_TYPE_LTE_RSRP;
        RadioSvc::SigStrengthIndication ind = RadioSvc::SigStrengthIndication{
            RadioSvc::SigIndicationType::SIG_THRESHOLD, -1400, -440, 0};
        RadioSvc::SigStrengthHysteresis hyst = RadioSvc::SigStrengthHysteresis{
            true, 50, true, 5000};
        radioProxy->SetSignalStrengthReportingCriteria(phoneId, sigType, ind, hyst, callStatus,
            methodError);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "SetSignalStrengthReportingCriteria SUCCESS " << std::endl << std::endl;
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

        std::string iccid;
        std::cout << "======== Sim GetICCID Test ========" << "'\n";
        simProxy->GetICCID(phoneId, callStatus, methodError, iccid);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "get ICCID: " << iccid << std::endl << std::endl;
    }


    if (svcMask & IVSS_TEST_SVC_INFO_MASK) {
        std::shared_ptr<InfoSvcProxy<>> infoProxy = runtime->buildProxy < InfoSvcProxy > ("local",
            "modem.InfoSvc", "InfoSvcTest");
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


    if (svcMask & IVSS_TEST_SVC_MNGDCONN_MASK) {
        std::shared_ptr<MngdConnSvcProxy<>> mngdConnProxy = runtime->buildProxy < MngdConnSvcProxy >
            ("local", "modem.MngdConnSvc", "MngdConnSvcTest");
        std::cout << "Checking availability!" << std::endl;
        while (!mngdConnProxy->isAvailable())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        std::cout << "mngdConnProxy Available..." << std::endl;
        mngdConnProxyKeep = mngdConnProxy;

        // Subscribe to broadcast
        mngdConnProxy->getDataStateEvent().subscribe([&](const std::string& name,
            const MngdConnSvc::DataState& dataState)
            {
                std::cout << "======== getDataStateEvent Test ========" << std::endl;
                std::cout << "DataState name :" << name << std::endl;
                std::cout << "dataState change to :" << DataStateToString(dataState) << std::endl;
            }
        );


        std::string name = "Data1";
        std::cout << "======== StartData Test ========" << "'\n";
        mngdConnProxy->StartData(name, callStatus, methodError);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "StartData SUCCESS: " << name << std::endl << std::endl;


        uint8_t dataNum = 0;
        std::vector<std::string> nameList = {};
        std::vector<MngdConnSvc::DataState> dataState = {};
        std::cout << "======== GetDataList Test ========" << "'\n";
        mngdConnProxy->GetDataList(callStatus, methodError, dataNum, nameList, dataState);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "GetDataList number: " << dataNum << " , name=" << nameList[0] << " , dataState="
            << DataStateToString(dataState[0]) << std::endl << std::endl;

        std::cout << "======== StopData Test ========" << "'\n";
        mngdConnProxy->StopData(name, callStatus, methodError);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!")
        CHECK_RETURN_VALUE(methodError == CommonTypes::Result::OK, "methodError!")
        std::cout << "StopData SUCCESS: " << name << std::endl << std::endl;
    }


    while (true) {
        std::cout << "Waiting for event... (Abort with CTRL+C)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(20));
    }

    return 0;
}
