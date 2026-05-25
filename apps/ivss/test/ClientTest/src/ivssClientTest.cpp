/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <string>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <CommonAPI/CommonAPI.hpp>
#include <v2/com/qualcomm/qti/telephony/RadioSvcProxy.hpp>
#include <v1/com/qualcomm/qti/telephony/SimSvcProxy.hpp>
#include <v1/com/qualcomm/qti/telephony/InfoSvcProxy.hpp>
#include <v3/com/qualcomm/qti/telephony/MngdConnSvcProxy.hpp>
#include <v1/com/qualcomm/qti/telephony/LocationSvcProxy.hpp>
#include <v1/com/qualcomm/qti/telephony/SensorSvcProxy.hpp>

#define IVSS_TEST_SVC_RADIO_MASK 0x1
#define IVSS_TEST_SVC_SIM_MASK 0x10
#define IVSS_TEST_SVC_INFO_MASK 0x100
#define IVSS_TEST_SVC_MNGDCONN_MASK 0x1000
#define IVSS_TEST_SVC_LOCATION_MASK 0x10000
#define IVSS_TEST_SVC_SENSOR_MASK 0x100000
#define IVSS_TEST_SVC_MASK_ALL 0xFFFFFFFF

namespace RadioSvc = v2::com::qualcomm::qti::telephony;
namespace SimSvc = v1::com::qualcomm::qti::telephony;
namespace InfoSvc = v1::com::qualcomm::qti::telephony;
namespace MngdConnSvc = v3::com::qualcomm::qti::telephony;
namespace LocationSvc = v1::com::qualcomm::qti::telephony;
namespace SensorSvc = v1::com::qualcomm::qti::telephony;
using RadioSvcTypes = RadioSvc::RadioSvcTypes;
using SimSvcTypes = SimSvc::SimSvcTypes;
using InfoSvcTypes = InfoSvc::InfoSvcTypes;
using MngdConnSvcTypes = MngdConnSvc::MngdConnSvcTypes;
using LocationSvcTypes = LocationSvc::LocationSvcTypes;
using SensorSvcTypes = SensorSvc::SensorSvcTypes;
//--------------------------------------------------------------------------------------------------
/**
 * Print Radio Access Technology.
 */
//--------------------------------------------------------------------------------------------------

#define CHECK_RETURN_VALUE(expression, message, value) \
    do { \
        if (!(expression)) { \
            std::cerr << "Error: " << (message) << "Value: " << static_cast<int>(value) \
                << std::endl; \
            return false; \
        } \
    } while(0);

std::string RatToString
(
    RadioSvcTypes::RadioRatT rat ///< [IN] Radio Access Technology enum.
)
{
    std::string ratString;

    switch (rat)
    {
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_GSM:
            ratString = "GSM";
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_UMTS:
            ratString = "UMTS";
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_TDSCDMA:
            ratString = "TDSCDMA";
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_LTE:
            ratString = "LTE";
            break;
        case RadioSvcTypes::RadioRatT::RADIO_RAT_T_NR5G:
            ratString = "NR5G";
            break;
        default:
            ratString = "Unsupported";
            break;
    }

    return ratString;
}

std::string RadioStatesToString
(
    RadioSvcTypes::RadioStateT radioState ///< [IN] Radio state.
)
{
    std::string statesString;
    switch (radioState)
    {
        case RadioSvcTypes::RadioStateT::RADIO_STATE_T_ON:
            statesString = "ON";
            break;
        case RadioSvcTypes::RadioStateT::RADIO_STATE_T_OFF:
            statesString = "OFF";
            break;
        case RadioSvcTypes::RadioStateT::RADIO_STATE_T_UNAVAILABLE:
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
    RadioSvcTypes::RadioCellInfoStatusT status ///< [IN] Cell Info.
)
{
    std::string statesString;
    switch (status)
    {
        case RadioSvcTypes::RadioCellInfoStatusT::RADIO_CELL_INFO_STATUS_T_SERVING_CHANGED:
            statesString = "CELL_SERVING_CHANGED";
            break;
        case RadioSvcTypes::RadioCellInfoStatusT::RADIO_CELL_INFO_STATUS_T_NEIGHBOR_CHANGED:
            statesString = "CELL_NEIGHBOR_CHANGED";
            break;
        case RadioSvcTypes::RadioSvcTypes::RadioCellInfoStatusT::
            RADIO_CELL_INFO_STATUS_T_SERVING_AND_NEIGHBOR_CHANGED:
            statesString = "CELL_SERVING_AND_NEIGHBOR_CHANGED";
            break;
        default:
            statesString = "Unsupported";
            break;
    }

    return statesString;
}

std::string StateToString
(
    SimSvcTypes::SimStateT simState ///< [IN] SIM card State.
)
{
    std::string stateString;

    switch (simState)
    {
        case SimSvcTypes::SimStateT::SIM_STATE_T_PRESENT:
            stateString = "PRESENT";
            break;
        case SimSvcTypes::SimStateT::SIM_STATE_T_ABSENT:
            stateString = "ABSENT";
            break;
        case SimSvcTypes::SimStateT::SIM_STATE_T_READY:
            stateString = "READY";
            break;
        case SimSvcTypes::SimStateT::SIM_STATE_T_RESTRICTED:
            stateString = "RESTRICTED";
            break;
        case SimSvcTypes::SimStateT::SIM_STATE_T_ERROR:
            stateString = "ERROR";
            break;
        case SimSvcTypes::SimStateT::SIM_STATE_T_UNKNOWN:
            stateString = "STATE_UNKNOWN";
            break;
        default:
            stateString = "Unsupporteds";
            break;
    }

    return stateString;
}

std::string DataStateToString
(
    MngdConnSvcTypes::MngdConnDataStateT dataState ///< [IN] The data state.
)
{
    std::string statesString;
    switch (dataState)
    {
        case MngdConnSvcTypes::MngdConnDataStateT::MNGD_CONN_T_DATA_DISCONNECTED:
            statesString = "DATA_DISCONNECTED";
            break;
        case MngdConnSvcTypes::MngdConnDataStateT::MNGD_CONN_T_DATA_CONNECTED:
            statesString = "DATA_CONNECTED";
            break;
        case MngdConnSvcTypes::MngdConnDataStateT::MNGD_CONN_T_DATA_CONNECTION_FAILED:
            statesString = "DATA_CONNECTION_FAILED";
            break;
        case MngdConnSvcTypes::MngdConnDataStateT::MNGD_CONN_T_DATA_CONNECTION_STALLED:
            statesString = "DATA_CONNECTION_STALLED";
            break;
        default:
            statesString = "Unsupported";
            break;
    }

    return statesString;
}

#include <iostream>
#include <cstdint>

void DisplayCapabilities(uint64_t capabilityMask) {
    std::cout << "\n************* Capabilities Information *************\n";
    std::cout << "Loc Capabilities bit mask: 0x" << std::hex << capabilityMask << std::dec << "\n";

    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_TIME_BASED_TRACKING)
    {
        std::cout << "Time based tracking\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_DISTANCE_BASED_TRACKING)
    {
        std::cout << "Distance based tracking\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_GNSS_MEASUREMENTS)
    {
        std::cout << "GNSS Measurement\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_CONSTELLATION_ENABLEMENT)
    {
        std::cout << "Constellation enablement\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_CARRIER_PHASE)
    {
        std::cout << "Carrier phase\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_GNSS_SINGLE_FREQUENCY)
    {
        std::cout << "QWES GNSS single frequency\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_GNSS_MULTI_FREQUENCY)
    {
        std::cout << "QWES GNSS multi frequency\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_VPE)
    {
        std::cout << "QWES VPE\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_CV2X_LOCATION_BASIC)
    {
        std::cout << "QWES CV2X location basic\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_CV2X_LOCATION_PREMIUM)
    {
        std::cout << "QWES CV2X location premium\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_PPE)
    {
        std::cout << "QWES PPE\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_QDR2)
    {
        std::cout << "QWES QDR2\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_QWES_QDR3)
    {
        std::cout << "QWES QDR3\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_TIME_BASED_BATCHING)
    {
        std::cout << "Time based batching\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_DISTANCE_BASED_BATCHING)
    {
        std::cout << "Distance based batching\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_GEOFENCE)
    {
        std::cout << "Geofence\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_OUTDOOR_TRIP_BATCHING)
    {
        std::cout << "Outdoor trip batching\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_SV_POLYNOMIAL)
    {
        std::cout << "SV polynomial\n";
    }
    if (capabilityMask &
        LocationSvcTypes::LocationCapabilityBitMaskT::LOC_CAP_BIT_MASK_T_NLOS_ML20)
    {
        std::cout << "NLOS ML20\n";
    }

    std::cout << "****************************************************\n";
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

    std::shared_ptr<RadioSvc::RadioSvcProxy<>> radioProxyKeep;
    std::shared_ptr<SimSvc::SimSvcProxy<>> simProxyKeep;
    std::shared_ptr<InfoSvc::InfoSvcProxy<>> infoProxyKeep;
    std::shared_ptr<MngdConnSvc::MngdConnSvcProxy<>> mngdConnProxyKeep;
    std::shared_ptr<LocationSvc::LocationSvcProxy<>> locationProxyKeep;
    std::shared_ptr<SensorSvc::SensorSvcProxy<>> sensorProxyKeep;

    if (svcMask & IVSS_TEST_SVC_RADIO_MASK)
    {
        std::shared_ptr<RadioSvc::RadioSvcProxy<>> radioProxy = runtime->buildProxy
            <RadioSvc::RadioSvcProxy>("local", "telephony.RadioSvc", "clientTest");
        std::cout << "Checking availability!" << std::endl;
        while (!radioProxy->isAvailable())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        std::cout << "radioProxy Available..." << std::endl;
        radioProxyKeep = radioProxy;
    }

    if (svcMask & IVSS_TEST_SVC_SIM_MASK)
    {
        std::shared_ptr<SimSvc::SimSvcProxy<>> simProxy = runtime->buildProxy
            <SimSvc::SimSvcProxy>("local", "telephony.SimSvc", "clientTest");
        std::cout << "Checking availability!" << std::endl;
        while (!simProxy->isAvailable())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        std::cout << "simProxy Available..." << std::endl;
        simProxyKeep = simProxy;
    }

    if (svcMask & IVSS_TEST_SVC_INFO_MASK)
    {
        std::shared_ptr<InfoSvc::InfoSvcProxy<>> infoProxy = runtime->buildProxy
            <InfoSvc::InfoSvcProxy>("local", "telephony.InfoSvc", "clientTest");
        std::cout << "Checking availability!" << std::endl;
        while (!infoProxy->isAvailable())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        std::cout << "infoProxy Available..." << std::endl;
        infoProxyKeep = infoProxy;
    }

    if (svcMask & IVSS_TEST_SVC_MNGDCONN_MASK)
    {
        std::shared_ptr<MngdConnSvc::MngdConnSvcProxy<>> mngdConnProxy = runtime->buildProxy
            <MngdConnSvc::MngdConnSvcProxy>("local", "telephony.MngdConnSvc", "clientTest");
        std::cout << "Checking availability!" << std::endl;
        while (!mngdConnProxy->isAvailable())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        std::cout << "mngdConnProxy Available..." << std::endl;
        mngdConnProxyKeep = mngdConnProxy;
    }

    if (svcMask & IVSS_TEST_SVC_LOCATION_MASK)
    {
        std::shared_ptr<LocationSvc::LocationSvcProxy<>> locationProxy = runtime->buildProxy
            <LocationSvc::LocationSvcProxy>("local", "telephony.LocationSvc", "clientTest");
        std::cout << "Checking availability!" << std::endl;
        while (!locationProxy->isAvailable())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        std::cout << "locationProxy Available..." << std::endl;
        locationProxyKeep = locationProxy;
    }

    if (svcMask & IVSS_TEST_SVC_SENSOR_MASK)
    {
        // Instance ID must match SensorSvc.fdepl InstanceId
        const char* sensorInstanceId = "com.qualcomm.qti.sensor.SensorInterface";
        std::shared_ptr<SensorSvc::SensorSvcProxy<>> sensorProxy = runtime->buildProxy
            <SensorSvc::SensorSvcProxy>("local", sensorInstanceId, "clientTest");
        std::cout << "Checking availability!" << std::endl;
        while (!sensorProxy->isAvailable())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        std::cout << "sensorProxy Available..." << std::endl;
        sensorProxyKeep = sensorProxy;
    }

    // api test
    if (svcMask & IVSS_TEST_SVC_RADIO_MASK)
    {
        // Subscribe to broadcast
        radioProxyKeep->getSignalStrengthEvent().subscribe
        ([&]
            (
                const RadioSvcTypes::ValueState& valueState, const RadioSvcTypes::PhoneIdT& phoneId,
                const RadioSvcTypes::RadioRatT& rat, const int32_t& ss, const int32_t& rsrp
            )
            {
                std::cout << "======== SignalStrengthEvent Test ========" << std::endl;
                std::cout << "ValueState: " << static_cast<unsigned int>(valueState) << std::endl;
                std::cout << "Rat is :" << RatToString(rat) << std::endl;
                std::cout << "phoneId = " << static_cast<unsigned int>(phoneId) << "  ss = " << ss
                    << "  rsrp = " << rsrp << std::endl<< std::endl;
            }
        );

        radioProxyKeep->getRadioStateEvent().subscribe
        ([&]
            (
                const RadioSvcTypes::ValueState& valueState,
                const RadioSvcTypes::RadioStateT& radioState
            )
            {
                std::cout << "======== RadioStateEvent Test ========" << std::endl;
                std::cout << "ValueState: " << static_cast<unsigned int>(valueState) << std::endl;
                std::cout << "Radio State change to :" << RadioStatesToString(radioState)
                    << std::endl << std::endl;
            }
        );

        radioProxyKeep->getCellInfoEvent().subscribe
        ([&]
            (
                const RadioSvcTypes::ValueState& valueState, const RadioSvcTypes::PhoneIdT& phoneId,
                const RadioSvcTypes::RadioCellInfoStatusT& status
            )
            {
                std::cout << "======== CellInfoEvent Test ========" << std::endl;
                std::cout << "ValueState: " << static_cast<unsigned int>(valueState) << std::endl;
                std::cout << "phoneId = " << static_cast<unsigned int>(phoneId)
                    << std::endl << std::endl;
                std::cout << "Cell Info change to :" << CellInfoStatusToString(status)
                    << std::endl << std::endl;
            }
        );

        radioProxyKeep->getRadioRatEvent().subscribe
        ([&]
            (
                const RadioSvcTypes::ValueState& valueState, const RadioSvcTypes::PhoneIdT& phoneId,
                const RadioSvcTypes::RadioRatT& radioRat
            )
            {
                std::cout << "======== RadioRatTEvent Test ========" << std::endl;
                std::cout << "ValueState: " << static_cast<unsigned int>(valueState) << std::endl;
                std::cout << "phoneId = " << static_cast<unsigned int>(phoneId)
                    << std::endl << std::endl;
                std::cout << "Radio Rat change to :" << RatToString(radioRat)
                    << std::endl << std::endl;
            }
        );

        // Request method
        RadioSvcTypes::TelephonyResultT radioResult =
            RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNKNOWN;
        RadioSvcTypes::PhoneIdT phoneId = RadioSvcTypes::PhoneIdT::PHONE_ID_T_1;

        std::cout << "======== get GSM signal strength Test ========" << "\n";
        int32_t rssi = 0;
        uint32_t ber = 0;
        radioProxyKeep->GetGsmSignalMetrics(phoneId, callStatus, rssi, ber, radioResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(radioResult == RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "radioResult!", radioResult)
        std::cout << "get GSM signal strength: rssi=" << rssi << "(dBm), ber=" << ber
            << "(Bit error rate)" << std::endl << std::endl;

        std::cout << "======== get UMTS signal strength Test ========" << "\n";
        int32_t ss = 0;
        ber = 0;
        int32_t rscp = 0;
        radioProxyKeep->GetUmtsSignalMetrics(phoneId, callStatus, ss, ber, rscp, radioResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(radioResult == RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "radioResult!", radioResult)
        std::cout << "get UMTS signal strength: ss=" << ss << "(dBm), ber=" << ber
            << "(Bit error rate), rscp" << rscp << "(dBm)" << std::endl << std::endl;

        std::cout << "======== get LTE signal strength Test ========" << "\n";
        ss = 0;
        int32_t rsrq = 0;
        int32_t rsrp = 0;
        int32_t snr = 0;
        radioProxyKeep->GetLteSignalMetrics(phoneId, callStatus, ss, rsrq, rsrp, snr, radioResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(radioResult == RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "radioResult!", radioResult)
        std::cout << "get LTE signal strength: ss=" << ss << "(dBm), rsrq=" << rsrq << "(dB), rsrp"
            << rsrp << "(dBm), snr=" << snr << "(0.1 dB)" << std::endl << std::endl;

        std::cout << "======== get NR5G signal strength Test ========" << "\n";
        rsrq = 0;
        rsrp = 0;
        snr = 0;
        radioProxyKeep->GetNr5gSignalMetrics(phoneId, callStatus, rsrq, rsrp, snr, radioResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(radioResult == RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "radioResult!", radioResult)
        std::cout << "get NR5G signal strength: rsrq=" << rsrq << "(dB), rsrp" << rsrp
            << "(dBm), snr=" << snr << "(0.1 dB)" << std::endl << std::endl;

        std::cout << "======== get Register Mode Test ========" << "\n";
        bool isManual;
        std::string mcc;
        std::string mnc;
        radioProxyKeep->GetRegisterMode(phoneId, callStatus, isManual, mcc, mnc, radioResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(radioResult == RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "radioResult!", radioResult)
        std::cout << "get Register Mode: isManual=" << isManual << " , mcc=" << mcc << " mnc="
            << mnc << std::endl << std::endl;

        std::cout << "======== set Automatic Register Mode Test ========" << "\n";
        radioProxyKeep->SetAutomaticRegisterMode(phoneId, callStatus, radioResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(radioResult == RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "radioResult!", radioResult)
        std::cout << "set Automatic Register Mode success" << std::endl << std::endl;

        std::cout << "======== Get Hardware Config Test ========" << "\n";
        uint8_t totalSimCount = 0;
        uint8_t maxActiveSims = 0;
        uint32_t deviceRatCapMask = 0x0;
        uint32_t simRatCapMask = 0x0;
        radioProxyKeep->GetHardwareConfig(phoneId, callStatus, totalSimCount, maxActiveSims,
            deviceRatCapMask, simRatCapMask, radioResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(radioResult == RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "radioResult!", radioResult)
        std::cout << "GetHardwareConfig: totalSimCount "
            << static_cast<unsigned int>(totalSimCount) << "\n"
            << "maxActiveSims " << static_cast<unsigned int>(maxActiveSims) << "\n"
            << "deviceRatCapMask " << std::hex << "0x" << deviceRatCapMask << std::dec << "\n"
            << "simRatCapMask " << std::hex << "0x" << simRatCapMask << std::dec << "\n"
            << std::endl << std::endl;

        std::cout << "======== Get Rat Preferences Test ========" << "\n";
        uint32_t ratMask = 0x0;
        radioProxyKeep->GetRatPreferences(phoneId, callStatus, ratMask, radioResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(radioResult == RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "radioResult!", radioResult)
        std::cout << "GetRatPreferences: ratMask " << std::hex << "0x" << ratMask << std::dec
            << std::endl << std::endl;

        std::cout << "======== Get Current Network Name Test ========" << "\n";
        std::string longName;
        std::string shortName;
        radioProxyKeep->GetCurrentNetworkName(phoneId, callStatus, longName, shortName,
            radioResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(radioResult == RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "radioResult!", radioResult)
        std::cout << "GetCurrentNetworkName: longName " << longName << " shortName " << shortName
            << std::endl << std::endl;

        std::cout << "======== Get NetRegState Test ========" << "\n";
        RadioSvcTypes::RadioRatT getRat = RadioSvcTypes::RadioRatT::RADIO_RAT_T_UNKNOWN;
        uint32_t cellId;
        std::string getMcc;
        std::string getMnc;
        RadioSvcTypes::RadioNetRegStateT netReg =
            RadioSvcTypes::RadioNetRegStateT::RADIO_NET_REG_STATE_T_UNKNOWN;
        radioProxyKeep->GetNetRegState(phoneId, callStatus, getRat, cellId, getMcc, getMnc, netReg,
            radioResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(radioResult == RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "radioResult!", radioResult)
        std::cout << "GetNetRegState: getRat= " << RatToString(getRat) << " , cellId=" << cellId
            << " , getMcc=" << getMcc << " , getMnc=" << getMnc << " , netReg="
            << static_cast<unsigned int>(netReg) << std::endl << std::endl;

        std::cout << "======== Get NrDualConnectivityStatus Test ========" << "\n";
        RadioSvcTypes::RadioNrDcnrRestrictionT statusDcnr =
            RadioSvcTypes::RadioNrDcnrRestrictionT::RADIO_NR_DCNR_RESTRICTION_T_UNKNOWN;
        radioProxyKeep->GetNrDualConnectivityStatus(phoneId, callStatus, statusDcnr, radioResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(radioResult == RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "radioResult!", radioResult)
        std::cout << "GetNrDualConnectivityStatus: statusDcnr "
            << static_cast<unsigned int>(statusDcnr) << std::endl << std::endl;

        std::cout << "======== Set SignalStrengthReportingCriteria  Test ========" << std::endl;
        RadioSvcTypes::RadioSigTypeT sigType =
            RadioSvcTypes::RadioSigTypeT::RADIO_SIG_TYPE_T_LTE_RSRP;
        RadioSvcTypes::RadioSigStrengthIndicationT ind = RadioSvcTypes::RadioSigStrengthIndicationT{
            RadioSvcTypes::RadioSigIndicationTypeT::RADIO_SIG_INDICATION_TYPE_T_THRESHOLD, -1400,
            -440, 0};
        RadioSvcTypes::RadioSigStrengthHysteresisT hyst =
            RadioSvcTypes::RadioSigStrengthHysteresisT{true, 50, true, 5000};
        radioProxyKeep->SetSignalStrengthReportingCriteria(phoneId, sigType, ind, hyst, callStatus,
            radioResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(radioResult == RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "radioResult!", radioResult)
        std::cout << "SetSignalStrengthReportingCriteria SUCCESS " << std::endl << std::endl;

        std::cout << "======== Get PacketSwitchedState Test ========" << "\n";
        RadioSvcTypes::RadioNetRegStateT netState =
            RadioSvcTypes::RadioNetRegStateT::RADIO_NET_REG_STATE_T_UNKNOWN;
        radioProxyKeep->GetPacketSwitchedState(phoneId, callStatus, netState, radioResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(radioResult == RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "radioResult!", radioResult)
        std::cout << "GetPacketSwitchedState: netState=" << static_cast<unsigned int>(netState)
            << std::endl << std::endl;

        std::cout << "======== Get RadioState Test ========" << "\n";
        RadioSvcTypes::RadioStateT radioState =
            RadioSvcTypes::RadioStateT::RADIO_STATE_T_UNKNOWN;
        radioProxyKeep->GetRadioState(phoneId, callStatus, radioState, radioResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(radioResult == RadioSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "radioResult!", radioResult)
        std::cout << "GetRadioState: radioState=" << RadioStatesToString(radioState)
            << std::endl << std::endl;
    }

    if (svcMask & IVSS_TEST_SVC_SIM_MASK)
    {
        // Subscribe to broadcast
        simProxyKeep->getSimStateEvent().subscribe
        ([&]
            (
                const SimSvcTypes::ValueState& valueState, const SimSvcTypes::PhoneIdT& phoneId,
                const SimSvcTypes::SimStateT& simState
            )
            {
                std::cout << "======== getSimStateEvent Test ========" << std::endl;
                std::cout << "ValueState: " << static_cast<unsigned int>(valueState) << std::endl;
                std::cout << "SimState change to :" << StateToString(simState) << std::endl;
                std::cout << "phoneId = " << static_cast<unsigned int>(phoneId)
                    << std::endl << std::endl;
            }
        );

        // Request method
        SimSvcTypes::TelephonyResultT simResult =
            SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNKNOWN;
        SimSvcTypes::PhoneIdT simPhoneId = SimSvcTypes::PhoneIdT::PHONE_ID_T_1;

        std::cout << "======== get Sim Imsi Test ========" << "\n";
        std::string imsi;
        simProxyKeep->GetImsi(simPhoneId, callStatus, imsi, simResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(simResult == SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "simResult!", simResult)
        std::cout << "get Imsi: " << imsi << std::endl << std::endl;

        std::cout << "======== get SimState Test ========" << "\n";
        SimSvcTypes::SimStateT simState = SimSvcTypes::SimStateT::SIM_STATE_T_UNKNOWN;
        simProxyKeep->GetState(simPhoneId, callStatus, simState, simResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(simResult == SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "simResult!", simResult)
        std::cout << "get Status: " << StateToString(simState) << std::endl << std::endl;

        std::string iccid;
        std::cout << "======== Sim GetIccid Test ========" << "\n";
        simProxyKeep->GetIccid(simPhoneId, callStatus, iccid, simResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(simResult == SimSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "simResult!", simResult)
        std::cout << "get ICCID: " << iccid << std::endl << std::endl;
    }

    if (svcMask & IVSS_TEST_SVC_INFO_MASK)
    {
        // Request method
        InfoSvcTypes::TelephonyResultT infoResult =
            InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNKNOWN;

        std::string imei;
        std::cout << "======== get Imei Test ========" << "\n";
        infoProxyKeep->GetImei(callStatus, imei, infoResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(infoResult == InfoSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "infoResult!", infoResult)
        std::cout << "get Imei: " << imei << std::endl << std::endl;
    }

    if (svcMask & IVSS_TEST_SVC_MNGDCONN_MASK)
    {
        // Subscribe to broadcast
        mngdConnProxyKeep->getDataStateEvent().subscribe
        ([&]
            (
                const MngdConnSvcTypes::ValueState& valueState, const std::string& name,
                const MngdConnSvcTypes::MngdConnDataStateT& dataState
            )
            {
                std::cout << "======== getDataStateEvent Test ========" << std::endl;
                std::cout << "DataState name :" << name << std::endl;
                std::cout << "dataState change to :" << DataStateToString(dataState) << std::endl;
            }
        );

        // Request method
        MngdConnSvcTypes::TelephonyResultT mngdConnResult =
            MngdConnSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNKNOWN;

        std::cout << "======== StartData1 Test ========" << "\n";
        std::string name1 = "Data1";
        mngdConnProxyKeep->StartData(name1, callStatus, mngdConnResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(mngdConnResult == MngdConnSvcTypes::TelephonyResultT::
            TELEPHONY_RESULT_T_OK, "mngdConnResult!", mngdConnResult)
        std::cout << "StartData SUCCESS: " << name1 << std::endl << std::endl;

        std::cout << "======== StartData2 Test ========" << "\n";
        std::string name2 = "Data2";
        mngdConnProxyKeep->StartData(name2, callStatus, mngdConnResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(mngdConnResult == MngdConnSvcTypes::TelephonyResultT::
            TELEPHONY_RESULT_T_OK, "mngdConnResult!", mngdConnResult)
        std::cout << "StartData SUCCESS: " << name2 << std::endl << std::endl;

        std::cout << "======== GetDataList Test ========" << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        uint8_t dataNum = 0;
        std::vector<std::string> nameList = {};
        std::vector<MngdConnSvcTypes::MngdConnDataStateT> dataState = {};
        mngdConnProxyKeep->GetDataList(callStatus, dataNum, nameList, dataState, mngdConnResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(mngdConnResult == MngdConnSvcTypes::TelephonyResultT::
            TELEPHONY_RESULT_T_OK, "mngdConnResult!", mngdConnResult)
        std::cout << "GetDataList number: " << static_cast<int>(dataNum) << "\n";
        for (int i = 0; i < dataNum; i++)
        {
            std::cout << "name" << i << "=" << nameList[i] << ", dataState="
                << DataStateToString(dataState[i]) << "\n";
        }

        std::cout << "======== Get Data1 Ipv4Info Test ========" << "\n";
        std::string ifName1;
        MngdConnSvcTypes::MngdConnDataIpInfoT ipv4Info1;
        mngdConnProxyKeep->GetDataIpv4Info(name1, callStatus, ifName1, ipv4Info1, mngdConnResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(mngdConnResult == MngdConnSvcTypes::TelephonyResultT::
            TELEPHONY_RESULT_T_OK, "mngdConnResult!", mngdConnResult)
        std::cout << "ifName: " << ifName1 << "\n";
        std::cout << "Ipv4" << "\n";
        std::cout << "IP: " << ipv4Info1.getIpAddr() << "\n";
        std::cout << "Gateway: " << ipv4Info1.getGatewayAddr() << "\n";
        std::cout << "Dns1: " << ipv4Info1.getDns1Addr() << "\n";
        std::cout << "Dns2: " << ipv4Info1.getDns2Addr() << "\n";
        std::cout << "Mask: 0x" << std::hex << ipv4Info1.getIpMask() << "\n";
        std::cout << "Mtu: " << std::dec << ipv4Info1.getMtu() << "\n";

        std::cout << "======== Get Data2 Ipv4Info Test ========" << "\n";
        std::string ifName2;
        MngdConnSvcTypes::MngdConnDataIpInfoT ipv4Info2;
        mngdConnProxyKeep->GetDataIpv4Info(name2, callStatus, ifName2, ipv4Info2, mngdConnResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(mngdConnResult == MngdConnSvcTypes::TelephonyResultT::
            TELEPHONY_RESULT_T_OK, "mngdConnResult!", mngdConnResult)
        std::cout << "ifName: " << ifName2 << "\n";
        std::cout << "Ipv4" << "\n";
        std::cout << "IP: " << ipv4Info2.getIpAddr() << "\n";
        std::cout << "Gateway: " << ipv4Info2.getGatewayAddr() << "\n";
        std::cout << "Dns1: " << ipv4Info2.getDns1Addr() << "\n";
        std::cout << "Dns2: " << ipv4Info2.getDns2Addr() << "\n";
        std::cout << "Mask: 0x" << std::hex << ipv4Info2.getIpMask() << "\n";
        std::cout << "Mtu: " << std::dec << ipv4Info2.getMtu() << "\n";

        std::cout << "======== StopData1 Test ========" << "\n";
        mngdConnProxyKeep->StopData(name1, callStatus, mngdConnResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(mngdConnResult == MngdConnSvcTypes::TelephonyResultT::
            TELEPHONY_RESULT_T_OK, "mngdConnResult!", mngdConnResult)
        std::cout << "StopData SUCCESS: " << name1 << std::endl << std::endl;

        std::cout << "======== StopData2 Test ========" << "\n";
        mngdConnProxyKeep->StopData(name2, callStatus, mngdConnResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(mngdConnResult == MngdConnSvcTypes::TelephonyResultT::
            TELEPHONY_RESULT_T_OK, "mngdConnResult!", mngdConnResult)
        std::cout << "StopData SUCCESS: " << name2 << std::endl << std::endl;
    }

    if (svcMask & IVSS_TEST_SVC_LOCATION_MASK)
    {
        // Request method
        LocationSvcTypes::TelephonyResultT locationResult =
            LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_UNKNOWN;

        uint64_t capabilitiesMask = 0;
        std::cout << "======== get Capabilities Test ========" << "\n";
        locationProxyKeep->GetCapabilities(callStatus, capabilitiesMask, locationResult);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS, "Remote call failed!",
            callStatus)
        CHECK_RETURN_VALUE(
            locationResult == LocationSvcTypes::TelephonyResultT::TELEPHONY_RESULT_T_OK,
            "locationResult!", locationResult)
        DisplayCapabilities(capabilitiesMask);
    }

    if (svcMask & IVSS_TEST_SVC_SENSOR_MASK)
    {
        // ---- Sensor service: interactive test (ported from sensor_idl_client) ----
        // Sensor state for interactive commands
        std::vector<SensorSvcTypes::SensorInfoT> sensorList;
        int32_t sensorCount = 0;
        struct SensorTracking { float samplingRate = 0; int32_t batchCount = 0;
            SensorSvcTypes::SensorStateT state =
                SensorSvcTypes::SensorStateT::SENSOR_STATE_T_DISABLE; };
        std::vector<SensorTracking> sensorTracking;

        // Per-sensor GPTP tracking for latency/ODR output
        uint64_t accPrevGptp = 0, gyroPrevGptp = 0, headPrevGptp = 0;
        int accCount = 0, gyroCount = 0, headCount = 0;

        auto getCurTimeMs = []() -> uint64_t {
            struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
            return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
        };

        auto printSensorList = [](
            const std::vector<SensorSvcTypes::SensorInfoT>& sensors, int32_t cnt) {
            for (int32_t i = 0; i < cnt; i++) {
                const auto& s = sensors[i];
                printf("Name:%s\n", s.getName().c_str());
                printf("\tvendor: %s\n", s.getVendor().c_str());
                printf("\tversion: %d\n", s.getSensorVersion());
                printf("\tresolution: %f\n", s.getResolution());
                printf("\tmaxRange %f\n", s.getMaxRange());
                printf("\tsensor_id: %d\n", s.getSensorId());
                printf("\ttype: %d\n", s.getSensorType());
                printf("\tmaxSamplingRate: %f\n", s.getMaxSamplingRate());
                printf("\tminBatchCount: %d\n", s.getMinBatchCount());
                printf("\tmaxBatchCount: %d\n", s.getMaxBatchCount());
                const auto& odr = s.getOdr();
                printf("\todr rate: ");
                for (size_t j = 0; j < 6; j++) {
                    float v = (j < odr.size()) ? odr[j] : 0.0f;
                    printf("%fHZ ", v);
                }
                printf("\n");
            }
        };

        // Subscribe to broadcasts
        sensorProxyKeep->getSensorCapabilitiesEvent().subscribe(
            [](const SensorSvcTypes::SensorServiceStateMaskT& mask) {
                printf("[SensorCapabilities] mask=%d (%s)\n", (int)mask,
                    (mask == SensorSvcTypes::SensorServiceStateMaskT::
                        SENSOR_SERVICE_STATE_MASK_T_READY)
                    ? "READY" : "NOT_READY");
            });

        sensorProxyKeep->getSensorConfigUpdateEvent().subscribe(
            [](const int32_t& sensorId, const float& samplingRate, const int32_t& batchCount) {
                printf("[SensorConfigUpdate] sensorId=%d samplingRate=%f batchCount=%d\n",
                    sensorId, samplingRate, batchCount);
            });

        sensorProxyKeep->getSensorImuDataReadEvent().subscribe(
            [&](const std::vector<SensorSvcTypes::SensorImuEventT>& events, const int32_t& cnt) {
                if (events.empty() || cnt == 0) return;
                int32_t sid = events[0].getSensorId();
                int32_t type = events[0].getType();
                bool isAccel = (type == 35); bool isGyro = (type == 16);
                uint64_t& prevGptp = isAccel ? accPrevGptp : gyroPrevGptp;
                int& evtCnt = isAccel ? accCount : gyroCount;
                uint64_t curGptp = events[0].getGptpTimestamp();
                uint64_t curMs = getCurTimeMs();
                uint64_t batchDelta = (prevGptp > 0) ?
                    (curGptp - prevGptp)/1000000ULL : curGptp/1000000ULL;
                printf("<<--Received SensorImuDataRead sensor_id: %d\n", sid);
                printf("Sensor %s Live: sensor_id %d: count %d"
                    " batch_delta(ms)=%llu cur_gptp_ts %llu latency(ms)=%llu\n",
                    isAccel ? "ACC" : (isGyro ? "GYRO" : "IMU"), sid, cnt,
                    (unsigned long long)batchDelta, (unsigned long long)curGptp,
                    (unsigned long long)(curMs - curGptp/1000000ULL));
                uint64_t prevEvtGptp = prevGptp;
                for (int32_t i = 0; i < cnt && i < (int32_t)events.size(); i++) {
                    const auto& e = events[i]; const auto& d = e.getData();
                    uint64_t ts = e.getTimestamp(), gptp = e.getGptpTimestamp();
                    uint64_t lat = (curMs > gptp/1000000ULL) ? (curMs - gptp/1000000ULL) : 0;
                    uint64_t odr = (prevEvtGptp > 0) ?
                        (gptp - prevEvtGptp)/1000000ULL : gptp/1000000ULL;
                    printf("%s Live event:%d xyz_raw<%f %f %f> xyz_bias<%f %f %f>"
                        " timestamp<boot,gptp> %llu %llu latency(ms)=%llu odr(ms)=%llu\n",
                        isAccel ? "Accel" : (isGyro ? "Gyro" : "IMU"), evtCnt++,
                        d.getXUncalib(), d.getYUncalib(), d.getZUncalib(),
                        d.getXBias(), d.getYBias(), d.getZBias(),
                        (unsigned long long)ts, (unsigned long long)gptp,
                        (unsigned long long)lat, (unsigned long long)odr);
                    prevEvtGptp = gptp;
                }
                printf("<<-------\n");
                prevGptp = events[cnt-1].getGptpTimestamp();
            });

        sensorProxyKeep->getSensorHeadingDataReadEvent().subscribe(
            [&](const std::vector<SensorSvcTypes::SensorHeadEventT>& events, const int32_t& cnt) {
                if (events.empty() || cnt == 0) return;
                int32_t sid = events[0].getSensorId();
                uint64_t curGptp = events[0].getGptpTimestamp();
                uint64_t curMs = getCurTimeMs();
                uint64_t batchDelta = (headPrevGptp > 0) ?
                    (curGptp - headPrevGptp)/1000000ULL : curGptp/1000000ULL;
                printf("<<--Received SensorHeadingDataRead sensor_id: %d\n", sid);
                printf("Sensor HEAD Live: sensor_id %d: count %d"
                    " batch_delta(ms)=%llu cur_gptp_ts %llu latency(ms)=%llu\n",
                    sid, cnt, (unsigned long long)batchDelta, (unsigned long long)curGptp,
                    (unsigned long long)(curMs - curGptp/1000000ULL));
                uint64_t prevEvtGptp = headPrevGptp;
                for (int32_t i = 0; i < cnt && i < (int32_t)events.size(); i++) {
                    const auto& e = events[i]; const auto& d = e.getData();
                    uint64_t ts = e.getTimestamp(), gptp = e.getGptpTimestamp();
                    uint64_t lat = (curMs > gptp/1000000ULL) ? (curMs - gptp/1000000ULL) : 0;
                    uint64_t odr = (prevEvtGptp > 0) ?
                        (gptp - prevEvtGptp)/1000000ULL : gptp/1000000ULL;
                    printf("Head Live event:%d heading and accuracy<%f %f>"
                        " timestamp<boot,gptp> %llu %llu latency(ms)=%llu odr(ms)=%llu\n",
                        headCount++, d.getHeading(), d.getAccuracy(),
                        (unsigned long long)ts, (unsigned long long)gptp,
                        (unsigned long long)lat, (unsigned long long)odr);
                    prevEvtGptp = gptp;
                }
                printf("<<-------\n");
                headPrevGptp = events[cnt-1].getGptpTimestamp();
            });

        // Register client
        SensorSvcTypes::SensorReturnT sensorResp;
        printf("\n==== Calling RegisterSensorClientReq =====>>\n");
        sensorProxyKeep->RegisterSensorClientReq(callStatus, sensorResp);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS,
            "RegisterSensorClientReq failed!", callStatus)
        printf("RegisterSensorClientReq result: %d (%s)\n", (int)sensorResp,
            (sensorResp == SensorSvcTypes::SensorReturnT::SENSOR_RETURN_SUCCESS) ?
            "SUCCESS" : "FAILED");

        // Initial sensor list
        printf("\n==== Calling GetSensorListReq =====>>\n");
        sensorProxyKeep->GetSensorListReq(callStatus, sensorList, sensorCount);
        CHECK_RETURN_VALUE(callStatus == CommonAPI::CallStatus::SUCCESS,
            "GetSensorListReq failed!", callStatus)
        sensorTracking.resize(sensorCount);
        printf("sensor_count: %d\n", sensorCount);
        printSensorList(sensorList, sensorCount);

        // Interactive loop for sensor commands
        printf("\nCommands: g=GetSensorList  c=Config  a=Control  q=Quit  (Abort with CTRL+C)\n");
        while (true) {
            printf("\n> ");
            fflush(stdout);
            char cmd[4] = {};
            if (fgets(cmd, sizeof(cmd), stdin) == NULL) break;
            if (cmd[0] == 'q') break;

            if (cmd[0] == 'g') {
                printf("\n==== Calling GetSensorListReq =====>>\n");
                sensorProxyKeep->GetSensorListReq(callStatus, sensorList, sensorCount);
                if (callStatus != CommonAPI::CallStatus::SUCCESS) {
                    printf("GetSensorListReq failed\n"); continue;
                }
                sensorTracking.resize(sensorCount);
                printf("sensor_count: %d\n", sensorCount);
                printSensorList(sensorList, sensorCount);
            }
            else if (cmd[0] == 'c') {
                if (sensorCount == 0) { printf("No sensors. Call 'g' first.\n"); continue; }
                printf("\n==== Calling SensorConfigReq =====>>\n");
                for (int32_t i = 0; i < sensorCount; i++) {
                    const auto& s = sensorList[i];
                    const auto& odr = s.getOdr();
                    printf("Configure sensor id %d (%s)\nEnter ODR index: ",
                        s.getSensorId(), s.getName().c_str());
                    for (size_t j = 0; j < 6; j++) {
                        float v = (j < odr.size()) ? odr[j] : 0.0f;
                        printf("%zu:%fHZ ", j, v);
                    }
                    printf("\n");
                    char buf[64] = {}; if (fgets(buf, sizeof(buf), stdin) == NULL) continue;
                    int odrIdx = (int)strtol(buf, NULL, 10);
                    float sr = (odrIdx >= 0 && odrIdx < (int)odr.size()) ? odr[odrIdx] : 0.0f;
                    printf("Enter batch count: "); if (fgets(buf, sizeof(buf), stdin) == NULL) continue;
                    int32_t bc = (int32_t)strtol(buf, NULL, 10);
                    sensorProxyKeep->SensorConfigReq(s.getSensorId(), sr, bc, callStatus, sensorResp);
                    printf("SensorConfigReq sensor_id=%d result=%d\n", s.getSensorId(), (int)sensorResp);
                    if (sensorResp == SensorSvcTypes::SensorReturnT::SENSOR_RETURN_SUCCESS)
                        { sensorTracking[i].samplingRate = sr; sensorTracking[i].batchCount = bc; }
                }
            }
            else if (cmd[0] == 'a') {
                if (sensorCount == 0) { printf("No sensors. Call 'g' first.\n"); continue; }
                printf("\n==== Calling SensorControlReq =====>>\n");
                for (int32_t i = 0; i < sensorCount; i++) {
                    const auto& s = sensorList[i];
                    printf("Enable/Disable sensor id %d (%s)\nEnter 1=Enable, 0=Disable: ", s.getSensorId(), s.getName().c_str());
                    char buf[64] = {}; if (fgets(buf, sizeof(buf), stdin) == NULL) continue;
                    int en = (int)strtol(buf, NULL, 10);
                    auto state = (en == 1) ? SensorSvcTypes::SensorStateT::SENSOR_STATE_T_ENABLE
                                           : SensorSvcTypes::SensorStateT::SENSOR_STATE_T_DISABLE;
                    sensorProxyKeep->SensorControlReq(s.getSensorId(), state, callStatus, sensorResp);
                    printf("SensorControlReq sensor_id=%d result=%d\n", s.getSensorId(), (int)sensorResp);
                    if (sensorResp == SensorSvcTypes::SensorReturnT::SENSOR_RETURN_SUCCESS)
                        sensorTracking[i].state = state;
                }
            }
            else if (cmd[0] != '\n') {
                printf("Unknown command '%c'. Use: g c a q\n", cmd[0]);
            }
        }

        // Deregister sensor client on exit
        printf("\n==== Calling DeRegisterSensorClientReq =====>>\n");
        sensorProxyKeep->DeRegisterSensorClientReq(callStatus, sensorResp);
        printf("DeRegisterSensorClientReq result: %d\n", (int)sensorResp);
    }
    else
    {
        // Non-sensor services: wait for events
        while (true) {
            std::cout << "Waiting for event... (Abort with CTRL+C)" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(20));
        }
    }

    return 0;
}
