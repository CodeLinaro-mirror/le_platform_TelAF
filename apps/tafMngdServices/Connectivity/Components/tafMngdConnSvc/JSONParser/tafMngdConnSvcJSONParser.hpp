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

/*! \page tafMngdConnSvcJSONParser Managed Connectivity Service JSON Parser
 * The JSON parser component parses the provided policy and component JSONs and fills in the
 * relevant structures for use by other components.
 *
*/
/**
 * \file tafMngdConnSvcJSONParser.hpp
 * tafMngdConnSvcJSONParser.hpp provides interfaces for parsing policy and configuration JSONs.
 *
 */
//-----------------------------------------------------------
#pragma once

#include "legato.h"
#include "tafMngdConnSvcParser_PolicyParser.hpp"
#include "tafMngdConnSvcParser_ConfigurationParser.hpp"
#include "tafMngdSvcJSONParser_Helper.hpp"
namespace telux {
namespace tafsvc {

    /*
        * This function will take Managed Connectivity JSON filename along with references to
        * taf_mngdConn_Policy_t and taf_mngdConn_Configuration_t. The function will
        * parse and validate the JSON. If the JSON is valid, then the references will be
        * updated.
        *
        * @param [in] taf_mngdConn_Policy_t        Refernce to Policy
        * @param [in] taf_mngdConn_Configuration_t Refernce to Configuration
        * @param [in] ConfigurationFileName         Configuration JSON file name
        *
        * @return true on success, false on failure
        */
    bool tafMngdConnSvc_GetPolicyAndConfiguration(taf_mngdConn_Policy_t &PolicyRef,
                                    taf_mngdConn_Configuration_t &ConfigurationRef,
                                    std::string ConfigurationFileName);

    // Validate received values via callback
    void UpdateValidConnectivityFuncMap(void);

    // Used to validate if the proprety values conform to expected types

    typedef bool (*ConnectivityValidationFunction_t)(taf_mngdConn_Policy_t& Policy,
                                                taf_mngdConn_Configuration_t &Configuration,
                                                std::string Value,
                                                int Index);
}
}