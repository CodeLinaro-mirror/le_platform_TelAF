/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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
#include "interfaces.h"
#include "tafMngdConnSvcParser_PolicyParser.hpp"
#include "tafMngdConnSvcParser_ConfigurationParser.hpp"
#include "tafMngdSvcJSONParser_Helper.hpp"
namespace tafsvc {

    /*
        * This function will take Managed Connectivity JSON filename along with references to
        * mcs_Policy_t and mcs_Configuration_t. The function will
        * parse and validate the JSON. If the JSON is valid, then the references will be
        * updated.
        *
        * @param [in] mcs_Policy_t        Refernce to Policy
        * @param [in] mcs_Configuration_t Refernce to Configuration
        * @param [in] ConfigurationFileName         Configuration JSON file name
        *
        * @return true on success, false on failure
        */
    bool tafMngdConnSvc_GetPolicyAndConfiguration(mcs_Policy_t &PolicyRef,
                                    mcs_Configuration_t &ConfigurationRef,
                                    std::string ConfigurationFileName);

    //Precheck the extension json path
    le_result_t PreCheckExtensionJson(std::string ConfigurationFileName,
                            mcs_Policy_t &PolicyStructRef,
                            mcs_Configuration_t &ConfigurationStructRef);
    //Parse the JSON file
    bool ParseJSON(std::string ConfigurationFileName,
                            mcs_Policy_t &PolicyStructRef,
                            mcs_Configuration_t &ConfigurationStructRef);

    // Validate received values via callback
    void UpdateValidConnectivityFuncMap(void);

    bool DoesFileExist(const char *path);

    // Used to validate if the proprety values conform to expected types

    typedef bool (*ConnectivityValidationFunction_t)(mcs_Policy_t& Policy,
                                                mcs_Configuration_t &Configuration,
                                                std::string Value,
                                                int Index);
}
