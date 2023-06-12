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

namespace telux
{
    namespace tafsvc
    {
        /**
         *
         * ConfigTree Node for Configuration File Name. It will contain a string.
         *
         */
        const std::string TAF_MNGD_ct_node_ConfigurationFileName = "ConfigurationFileName";

        /**
         *
         * ConfigTree Node for Policy File Name. It will contain a string.
         *
         */
        const std::string TAF_MNGD_ct_node_PolicyFileName = "PolicyFileName";

        /**
         *
         * ConfigTree Node to capture if the Configuration file name in the Policy was overridden
         * by the Configuration filename provided via taf_mngd_Conn_SetPolicyConfigurationJSONs API.
         * It will contain a string.
         *
         */
        const std::string TAF_MNGD_ct_node_ConfigurationFileNameOverride =
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
} // namespace taf_mngd