/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

#include "legato.h"
#include "interfaces.h"

namespace ecall
{

// Data structure to hold information for a single eCall session.
struct EcallLogData
{
    std::vector<uint8_t> msdContent;
    std::optional<int32_t> rssiLevel;
    std::string mnc;
    std::string mcc;
};

// EcallDataLogger class to handle data collection and file logging.
class EcallMgrDataLogger
{
public:
    static EcallMgrDataLogger& GetInstance();

    // Begin a new call record (clears previous data).
    void BeginCallRecord();

    // Populate fields.
    void SetMsdContent(const std::vector<uint8_t>& msd);
    void SetNetwork(const std::string& mcc, const std::string& mnc);
    void SetSignalLevel(int32_t level);  // RSRP or RSSI in dBm

    // Saves the collected eCall data to a log file.
    void SaveCallDataToFile(const char* filename = "/tmp/ecall_log.txt");

private:
    EcallMgrDataLogger() = default;
    ~EcallMgrDataLogger() = default;
    EcallMgrDataLogger(const EcallMgrDataLogger&) = delete;
    EcallMgrDataLogger& operator=(const EcallMgrDataLogger&) = delete;

    EcallLogData currentCallData_;
};

} // namespace ecall
