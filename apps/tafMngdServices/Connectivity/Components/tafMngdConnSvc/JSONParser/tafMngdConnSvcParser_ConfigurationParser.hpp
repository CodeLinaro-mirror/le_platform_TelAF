/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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
#include "interfaces.h"
#include "tafMngdConn_Common.hpp"
#include "tafMngdSvcJSONParser_Helper.hpp"
#include <map>

namespace tafsvc {
    typedef struct
    {
        mcs_Yes_No_t Enable; //Yes=1, No=0
        uint8_t      RetryCount;
        uint16_t     BackoffInterval;
        uint8_t      BackoffIntervalStep;
    } mcs_Configuration_DataStartRetry_t;

    typedef struct
    {
        char URL[MCS_MAX_CONNECTION_URL_LEN];
        char IPv4[MCS_MAX_IPV4_LEN];
    } mcs_Configuration_Data_DataStartConnectionTest_t;

    typedef struct
    {
        uint16_t Interval;
        uint8_t  RetryCount;
        char URL[MCS_MAX_CONNECTION_URL_LEN];
    } mcs_Configuration_Data_PeriodicConnectivityCheck_t;

    typedef struct
    {
        uint8_t ProfileNumber;
        char APN[MCS_MAX_APN_LEN];
    } mcs_Configuration_Data_Profile_t;

    typedef struct
    {
        uint8_t ID;
        uint8_t Use_Network_ID;
        char DataName[MCS_MAX_NAME_LEN];
        mcs_Configuration_Data_Profile_t Profile;
        mcs_Yes_No_t AutoStart; //Yes=1, No=0
        mcs_Configuration_DataStartRetry_t DataStartRetry;
        mcs_Configuration_Data_DataStartConnectionTest_t DataStartConnectionTest;
        mcs_Configuration_Data_PeriodicConnectivityCheck_t PeriodicConnectivityCheck;
    } mcs_Configuration_Data_t;

    typedef struct
    {
        uint8_t ID;
        char Name[MCS_MAX_NAME_LEN];
        uint8_t Use_Sim_ID;
        uint8_t PhoneID;
        mcs_NW_Registration_Type_t Registration;
    } mcs_Configuration_Network_t;

    typedef struct
    {
        uint8_t ID;
        uint8_t SlotNumber;
        char Name[MCS_MAX_NAME_LEN];
    } mcs_Configuration_Sim_t;

    typedef struct
    {
        mcs_JSON_Version_t Version;
        char Name[MCS_MAX_NAME_LEN];
        uint8_t SimCount;
        mcs_Configuration_Sim_t Sim[MCS_MAX_SIM_OBJECT_COUNT];
        uint8_t NetworkCount;
        mcs_Configuration_Network_t Network[MCS_MAX_NETWORK_OBJECT_COUNT];
        uint8_t DataCount;
        mcs_Configuration_Data_t Data[MCS_MAX_DATA_OBJECT_COUNT];
    } mcs_Configuration_t;

    // Class is declared here and defined later
    class mcs_ConfigurationParser;
}

class tafsvc::mcs_ConfigurationParser
{
private:
    // Private constructor
    mcs_ConfigurationParser(){};

    // Used to validate if the proprety values conform to expected types
    typedef bool (*ConfigurationValidationFunction_t)(mcs_Configuration_t &Configuration,
                                                            std::string Value,
                                                            int Index);

    // Validate received values via callback
    // Map of properties and validation function pointers
    std::map<std::string, ConfigurationValidationFunction_t> ConfigurationValidationFuncMap;
    void UpdateValidConfigurationFuncMap(void);

    // MCSC = ManagedConnectivityServiceConfiguration
    static bool Validate_MCSC_Name(mcs_Configuration_t &Configuration,
                                      std::string Value,
                                      int Index);
    // Sim
    static bool Validate_MCSC_Sim_ID(mcs_Configuration_t &Configuration,
                                      std::string Value,
                                      int Index);
    static bool Validate_MCSC_Sim_Name(mcs_Configuration_t &Configuration,
                                   std::string Value,
                                   int Index);
    static bool Validate_MCSC_Sim_SlotNumber(mcs_Configuration_t &Configuration,
                                   std::string Value,
                                   int Index);
    // Network
    static bool Validate_MCSC_Network_Name(mcs_Configuration_t &Configuration,
                                   std::string Value,
                                   int Index);
    static bool Validate_MCSC_Network_ID(mcs_Configuration_t &Configuration,
                                   std::string Value,
                                   int Index);
    static bool Validate_MCSC_Network_Use_SIM_ID(mcs_Configuration_t &Configuration,
                                       std::string Value,
                                       int Index);
    static bool Validate_MCSC_Network_PhoneID(mcs_Configuration_t &Configuration,
                                       std::string Value,
                                       int Index);
    static bool Validate_MCSC_Network_Registration(mcs_Configuration_t &Configuration,
                                       std::string Value,
                                       int Index);

    //Data
    static bool Validate_MCSC_Data_ID(mcs_Configuration_t &Configuration,
                                       std::string Value,
                                       int Index);
    static bool Validate_MCSC_Data_Use_Network_ID(mcs_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);
    static bool Validate_MCSC_Data_Name(mcs_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);
    static bool Validate_MCSC_Data_Profile_Number(mcs_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);
    static bool Validate_MCSC_Data_Profile_APN(mcs_Configuration_t &Configuration,
                                               std::string Value,
                                               int Index);
    static bool Validate_MCSC_Data_AutoStart(mcs_Configuration_t &Configuration,
                                               std::string Value,
                                               int Index);
    static bool Validate_MCSC_Data_DataStartConnectionTest_URL(
                                    mcs_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);
    static bool Validate_MCSC_Data_DataStartConnectionTest_IPv4(
                                    mcs_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);
    // DSR = DataStartRetry
    static bool Validate_MCSC_Data_DSR_Enable (mcs_Configuration_t &Configuration,
                                                        std::string Value,
                                                        int Index);
    static bool Validate_MCSC_Data_DSR_RetryCount (mcs_Configuration_t &Configuration,
                                                        std::string Value,
                                                        int Index);
    static bool Validate_MCSC_Data_DSR_BackoffInterval (mcs_Configuration_t &Configuration,
                                                        std::string Value,
                                                        int Index);
    static bool Validate_MCSC_Data_DSR_BackoffIntervalStep (mcs_Configuration_t &Configuration,
                                                            std::string Value,
                                                            int Index);

    // Periodic Connectivity Check
    static bool Validate_MCSC_Data_PeriodicConnectivityCheck_Interval(
                                    mcs_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);
    static bool Validate_MCSC_Data_PeriodicConnectivityCheck_URL(
                                    mcs_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);

    static bool Validate_MCSC_Data_PeriodicConnectivityCheck_RetryCount(
                                    mcs_Configuration_t &Configuration,
                                    std::string Value,
                                    int Index);

    bool ValidateValue(mcs_Configuration_t &Configuration,
                       std::string property,
                       std::string Value,
                       int Index);

    bool ParseAndUpdateConfigurationJSON(mcs_Configuration_t &Configuration,
                                                                        std::string filename);

public:
    // Delete copy constructor.
    mcs_ConfigurationParser(mcs_ConfigurationParser const &) = delete;
    mcs_ConfigurationParser &operator=(mcs_ConfigurationParser const &)
                                                = delete;

    static mcs_ConfigurationParser &getInstance();

    /**
     * \brief Reset the Configuration structure
     *
     */
    void ResetConfigurationStructure(mcs_Configuration_t &Configuration);
    /**
     * \brief Return a pointer to the Configuration structure
     *
     */
    bool GetConfiguration(mcs_Configuration_t &Configuration,
                                                std::string ConfigurationFileName);
};
