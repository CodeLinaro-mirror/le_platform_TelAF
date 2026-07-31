/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EcallMgrDataLogger.hpp"

#include <fstream>
#include <iomanip>

namespace ecall
{

// ---------- EcallDataLogger ----------

EcallMgrDataLogger& EcallMgrDataLogger::GetInstance()
{
    static EcallMgrDataLogger instance;
    return instance;
}

void EcallMgrDataLogger::BeginCallRecord()
{
    currentCallData_ = {};
}

void EcallMgrDataLogger::SetMsdContent(const std::vector<uint8_t>& msd)
{
    currentCallData_.msdContent = msd;
}

void EcallMgrDataLogger::SetNetwork(const std::string& mcc, const std::string& mnc)
{
    currentCallData_.mcc = mcc;
    currentCallData_.mnc = mnc;
}

void EcallMgrDataLogger::SetSignalLevel(int32_t level)
{
    currentCallData_.rssiLevel = level;
}

void EcallMgrDataLogger::SaveCallDataToFile(const char* filename)
{
    std::ofstream outFile(filename, std::ios_base::app);
    if (!outFile.is_open())
    {
        LE_ERROR("EcallMgrDataLogger: Failed to open file for writing: %s", filename);
        return;
    }

        outFile << "--- eCall Record ---\n";

    outFile << "MSD_Content: ";
    for (uint8_t byte : currentCallData_.msdContent)
        outFile << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(byte);
    outFile << std::dec << '\n';

    outFile << "RSSI_Level/RSRP_Level: ";
    if (currentCallData_.rssiLevel.has_value())
        outFile << currentCallData_.rssiLevel.value() << " dBm";
    else
        outFile << "N/A";
    outFile << '\n';

    outFile << "MNC: " << currentCallData_.mnc << '\n';
    outFile << "MCC: " << currentCallData_.mcc << '\n';

    outFile.flush();
    if (outFile.fail())
    {
        LE_ERROR("EcallMgrDataLogger: write error on file: %s", filename);
        return;
    }

    LE_INFO("EcallMgrDataLogger: eCall data saved to %s", filename);
}

} // namespace ecall

