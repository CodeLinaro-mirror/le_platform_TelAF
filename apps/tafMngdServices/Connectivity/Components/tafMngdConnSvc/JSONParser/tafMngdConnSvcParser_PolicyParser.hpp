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

/*! \page tafMngdConnSvcParser_Policy Managed Connectivity Service Policy Parser
 * This module parses policy JSON files and  updates relevant configuration objects and
 * structures for use by other components.
 *
*/
/**
 * \file tafMngdConnSvcParser_PolicyParser.hpp
 * tafMngdConnSvcParser_PolicyParser.hpp provides policy parsing module.
 *
 */
//-----------------------------------------------------------
#pragma once

#include <stdint.h>
#include <map>
#include "legato.h"
#include "tafMngdConn_Common.hpp"
#include "tafMngdSvcJSONParser_Helper.hpp"


namespace telux {
namespace tafsvc {
    typedef struct
    {
        uint8_t Priority;
        uint8_t Use_Data_ID;
    } taf_mngd_Conn_Policy_DataConnection_t;

    typedef struct
    {
        taf_mngd_Yes_No_t Fallback;  //Yes=1, No=0
        uint8_t dataConnectionCount;
        taf_mngd_Conn_Policy_DataConnection_t \
                            DataConnection[TAF_MNGD_CONN_MAX_DATA_CONNECION_OBJECT_COUNT];
    } taf_mngd_Conn_Policy_DataSession_t;

    typedef struct
    {
        uint8_t Version;
        char Name[TAF_MNGD_CONN_MAX_NAME_LEN];
        char ConfigurationFileName[TAF_MNGD_CONN_MAX_FILE_NAME_LEN];
        taf_mngd_Conn_Policy_DataSession_t DataSession;
    } taf_mngd_Conn_Policy_t;

    // Class is declared here and defined later
    class tafMngdConnSvc_PolicyParser;
    }
}


class telux::tafsvc::tafMngdConnSvc_PolicyParser
{
private:

    // Private constructor
    tafMngdConnSvc_PolicyParser(){};

    // Used to validate if the proprety values conform to expected types
    typedef bool (*PolicyValidationFunction_t)(taf_mngd_Conn_Policy_t& Policy,
                                                                std::string Value,
                                                                int Index);

    // Validate received values via callback
    // Map of properties and validation function pointers
    std::map< std::string, PolicyValidationFunction_t >  PolicyValidationFuncMap;
    void UpdateValidPolicyFuncMap(void);

    // MCSP = ManagedConnectivityServicePolicy
    static bool Validate_MCSP_Version (taf_mngd_Conn_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index);
    static bool Validate_MCSP_Name (taf_mngd_Conn_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index);
    static bool Validate_MCSP_ConfigFileName (taf_mngd_Conn_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index);
    // DS = DataSession
    static bool Validate_DS_Fallback (taf_mngd_Conn_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index);
    // DS_DC = DataSession/DataConnection
    static bool Validate_DS_DC_Priority (taf_mngd_Conn_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index);
    static bool Validate_DS_DC_Use_Data_ID (taf_mngd_Conn_Policy_t &Policy,
                                                        std::string Value,
                                                        int Index);

    bool ValidateValue(taf_mngd_Conn_Policy_t &Policy,
                       std::string property,
                       std::string Value,
                       int Index);
    bool ParseAndUpdatePolicyJSON(taf_mngd_Conn_Policy_t &Policy, std::string filename);

public:
    // Delete copy constructor.
    tafMngdConnSvc_PolicyParser           (tafMngdConnSvc_PolicyParser const &) = delete;
    tafMngdConnSvc_PolicyParser &operator=(tafMngdConnSvc_PolicyParser const &) = delete;

    static tafMngdConnSvc_PolicyParser& getInstance();

    /**
     * \brief Reset the Policy structure
     *
     */
    void ResetPolicyStructure(taf_mngd_Conn_Policy_t &Policy);
    /**
     * \brief Return a pointer to the Policy structure
     *
     */
    bool GetPolicy(taf_mngd_Conn_Policy_t& Policy, std::string ConfigurationFileName);
};