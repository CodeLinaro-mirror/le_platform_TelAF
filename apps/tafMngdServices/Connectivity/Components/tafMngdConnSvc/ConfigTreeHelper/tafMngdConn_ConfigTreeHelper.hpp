/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


//-----------------------------------------------------------

/*! \page tafMngdConn_ConfigTreeHelper Managed Connectivity ConfigTree Helper
 * The ConfigTree Helper component provides APIs that will read/write/update the service's
 * ConfigTree.
 *
*/
/**
 * \file tafMngdConn_ConfigTreeHelper.hpp
 * tafMngdConn_ConfigTreeHelper.hpp provides interfaces for ConfigTree management.
 *
 */
//-----------------------------------------------------------
#pragma once
#include <string>
#include "legato.h"
#include "interfaces.h"

namespace tafsvc
{
    /**
     *
     * ConfigTree Node for Configuration File Name. It will contain a string.
     *
     */
    const std::string MCS_ct_node_ConfigurationFileName = "ConfigurationFileName";

    /**
     *
     * ConfigTree Node to capture if the Configuration file name in the Policy was overridden
     * by the Configuration filename provided via taf_mngdConn_SetPolicyConfigurationJSONs API.
     * It will contain a string.
     *
     */
    const std::string MCS_ct_node_ConfigurationFileNameOverride =
                                                "ConfigurationFileNameOverride";
    /*
     * Read from ConfigTree
     *
     * @return
     *   - LE_OK on success
     *   - le_result_t error on failure
     *
     */
    le_result_t tafMngd_ConfigTree_Read(
        const std::string &Node, ///< [IN] Node to read from the config tree.
        char *strValuePtr,       ///< [OUT] Value read from the specified Node.
        size_t ValueSize         ///< [OUT] Size of Value String.
    );

    /*
     * Write to ConfigTree
     *
     * @return
     *   - LE_OK on success
     *   - le_result_t error on failure
     *
     */
    le_result_t tafMngd_ConfigTree_Update(
        const std::string &Node, ///< [IN] Node to read from the config tree.
        const std::string &Value ///< [IN] Value to write to the specified node.
    );
} // namespace connectivity
