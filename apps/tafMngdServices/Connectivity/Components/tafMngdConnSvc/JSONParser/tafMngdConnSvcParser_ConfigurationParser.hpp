/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//-----------------------------------------------------------

/*! \page tafMngdConnSvcParser_Configuration Managed Connectivity Service Configuration Parser
 * This module parses configuration JSON files and  updates relevant configuration objects and
 * structures for use by other components.
 *
*/
/**
 * \file tafMngdConnSvcParser_ConfigurationParser.hpp
 * tafMngdConnSvcParser_ConfigurationParser.hpp provides configuration parsing module.
 *
 */
//-----------------------------------------------------------
#pragma once

#include <stdint.h>
#include "legato.h"
#include "tafMngdConn_Common.hpp"
#include "tafMngdSvcJSONParser_Helper.hpp"
#include <map>

namespace telux {
namespace tafsvc {
    typedef struct
    {
        mcs_Yes_No_t Enable; //Yes=1, No=0
        uint8_t RetryCount;
    } taf_mngdConn_Policy_DataStartRetry_t;

    typedef struct
    {
        char URL[MCS_MAX_CONNECTION_URL_LEN];
        char IPv4[MCS_MAX_IPV4_LEN];
        char IPv6[MCS_MAX_IPV6_LEN];
    } taf_mngdConn_Configuration_Data_DataStartConnectionTest_t;

    typedef struct
    {
        uint8_t Interval;
        uint8_t RetryCount;
        char URL[MCS_MAX_CONNECTION_URL_LEN];
    } taf_mngdConn_Configuration_Data_PeriodicConnectivityCheck_t;

    typedef struct
    {
        uint8_t ProfileNumber;
        char ProfileName[MCS_MAX_PROFILE_NAME_LEN];
        char APN[MCS_MAX_APN_LEN];
    } taf_mngdConn_Configuration_Data_Profile_t;

    typedef struct
    {
        uint8_t ID;
        uint8_t Use_Network_ID;
        taf_mngdConn_Configuration_Data_Profile_t Profile;
        mcs_Yes_No_t AutoStart; //Yes=1, No=0
        taf_mngdConn_Policy_DataStartRetry_t DataStartRetry;
        taf_mngdConn_Configuration_Data_DataStartConnectionTest_t DataStartConnectionTest;
        taf_mngdConn_Configuration_Data_PeriodicConnectivityCheck_t PeriodicConnectivityCheck;
    } taf_mngdConn_Configuration_Data_t;

    typedef struct
    {
        uint8_t ID;
        char Name[MCS_MAX_NAME_LEN];
        uint8_t Use_Sim_ID;
        uint8_t PhoneID;
        mcs_NW_Registration_Type_t Registration;
    } taf_mngdConn_Configuration_Network_t;

    typedef struct
    {
        uint8_t ID;
        uint8_t SlotNumber;
        char Name[MCS_MAX_NAME_LEN];
    } taf_mngdConn_Configuration_Sim_t;

    typedef struct
    {
        mcs_JSON_Version_t Version;
        char Name[MCS_MAX_NAME_LEN];
        uint8_t SimCount;
        taf_mngdConn_Configuration_Sim_t Sim[MCS_MAX_SIM_OBJECT_COUNT];
        uint8_t NetworkCount;
        taf_mngdConn_Configuration_Network_t Network[MCS_MAX_NETWORK_OBJECT_COUNT];
        uint8_t DataCount;
        taf_mngdConn_Configuration_Data_t Data[MCS_MAX_DATA_OBJECT_COUNT];
    } taf_mngdConn_Configuration_t;

    // Class is declared here and defined later
    class tafMngdConnSvc_ConfigurationParser;
}
}

class telux::tafsvc::tafMngdConnSvc_ConfigurationParser
{
private:
    // Private constructor
    tafMngdConnSvc_ConfigurationParser(){};

    // Used to validate if the proprety values conform to expected types
    typedef bool (*ConfigurationValidationFunction_t)(taf_mngdConn_Configuration_t &Configuration,
                                                            std::string Value,
                                                            int Index);

    // Validate received values via callback
    // Map of properties and validation function pointers
    std::map<std::string, ConfigurationValidationFunction_t> ConfigurationValidationFuncMap;
    void UpdateValidConfigurationFuncMap(void);

    // MCSC = ManagedConnectivityServiceConfiguration
    static bool Validate_MCSC_Name(taf_mngdConn_Configuration_t &Configuration,
                                      std::string Value,
                                      int Index);
    // Sim
    static bool Validate_MCSC_Sim_ID(taf_mngdConn_Configuration_t &Configuration,
                                      std::string Value,
                                      int Index);
    static bool Validate_MCSC_Sim_Name(taf_mngdConn_Configuration_t &Configuration,
                                   std::string Value,
                                   int Index);
    static bool Validate_MCSC_Sim_SlotNumber(taf_mngdConn_Configuration_t &Configuration,
                                   std::string Value,
                                   int Index);
    // Network
    static bool Validate_MCSC_Network_Name(taf_mngdConn_Configuration_t &Configuration,
                                   std::string Value,
                                   int Index);
    static bool Validate_MCSC_Network_ID(taf_mngdConn_Configuration_t &Configuration,
                                   std::string Value,
                                   int Index);
    static bool Validate_MCSC_Network_Use_SIM_ID(taf_mngdConn_Configuration_t &Configuration,
                                       std::string Value,
                                       int Index);
    static bool Validate_MCSC_Network_PhoneID(taf_mngdConn_Configuration_t &Configuration,
                                       std::string Value,
                                       int Index);
    static bool Validate_MCSC_Network_Registration(taf_mngdConn_Configuration_t &Configuration,
                                       std::string Value,
                                       int Index);

    //Data
    static bool Validate_MCSC_Data_ID(taf_mngdConn_Configuration_t &Configuration,
                                       std::string Value,
                                       int Index);
    static bool Validate_MCSC_Data_Use_Network_ID(taf_mngdConn_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);
    static bool Validate_MCSC_Data_Name(taf_mngdConn_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);
    static bool Validate_MCSC_Data_Profile_Number(taf_mngdConn_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);
    static bool Validate_MCSC_Data_Profile_APN(taf_mngdConn_Configuration_t &Configuration,
                                               std::string Value,
                                               int Index);
    static bool Validate_MCSC_Data_AutoStart(taf_mngdConn_Configuration_t &Configuration,
                                               std::string Value,
                                               int Index);
    static bool Validate_MCSC_Data_DataStartConnectionTest_URL(
                                    taf_mngdConn_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);
    static bool Validate_MCSC_Data_DataStartConnectionTest_IPv4(
                                    taf_mngdConn_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);
    static bool Validate_MCSC_Data_DataStartConnectionTest_IPv6(
                                    taf_mngdConn_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);
    // DSR = DataStartRetry
    static bool Validate_MCSC_Data_DSR_Enable (taf_mngdConn_Configuration_t &Policy,
                                                        std::string Value,
                                                        int Index);
    static bool Validate_MCSC_Data_DSR_RetryCount (taf_mngdConn_Configuration_t &Policy,
                                                        std::string Value,
                                                        int Index);
    static bool Validate_MCSC_Data_PeriodicConnectivityCheck_Interval(
                                    taf_mngdConn_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);
    static bool Validate_MCSC_Data_PeriodicConnectivityCheck_URL(
                                    taf_mngdConn_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);

    static bool Validate_MCSC_Data_PeriodicConnectivityCheck_RetryCount(
                                    taf_mngdConn_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);

    bool ValidateValue(taf_mngdConn_Configuration_t &Configuration,
                       std::string property,
                       std::string Value,
                       int Index);

    bool ParseAndUpdateConfigurationJSON(taf_mngdConn_Configuration_t &Configuration,
                                                                        std::string filename);

public:
    // Delete copy constructor.
    tafMngdConnSvc_ConfigurationParser(tafMngdConnSvc_ConfigurationParser const &) = delete;
    tafMngdConnSvc_ConfigurationParser &operator=(tafMngdConnSvc_ConfigurationParser const &)
                                                = delete;

    static tafMngdConnSvc_ConfigurationParser &getInstance();

    /**
     * \brief Reset the Configuration structure
     *
     */
    void ResetConfigurationStructure(taf_mngdConn_Configuration_t &Configuration);
    /**
     * \brief Return a pointer to the Configuration structure
     *
     */
    bool GetConfiguration(taf_mngdConn_Configuration_t &Configuration,
                                                std::string ConfigurationFileName);
};
