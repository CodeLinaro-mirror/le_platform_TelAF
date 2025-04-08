/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFUPDATECONFIG_HPP
#define TAFUPDATECONFIG_HPP

#include <string>
#include "legato.h"
#include "interfaces.h"

const size_t kMaxBufSize = 128;

    namespace tafsvc
    {
        /*
         * Reads a value from the tree as a boolean.
         *
         * @return
         *   - boolean value for the 'key'
         */
        bool tafUpdate_ConfigTree_GetBool(
            const std::string key   ///< [IN] Node to read from the config tree.
        );

        /*
         * Writes a boolean value to the config tree.
         */
        void tafUpdate_ConfigTree_SetBool(
            const std::string key,  ///< [IN] Node to read from the config tree.
            const bool value        ///< [IN] Value to write to the specified node.
        );

        /*
         * Reads an integer value from the config tree.
         *
         * @return
         *   - integer value for the 'key'
         */
        uint32_t tafUpdate_ConfigTree_GetInt(
            const std::string key   ///< [IN] Node to read from the config tree.
        );

        /*
         * Writes an integer value to the config tree.
         */
        void tafUpdate_ConfigTree_SetInt(
            const std::string key,  ///< [IN] Node to read from the config tree.
            const uint32_t value    ///< [IN] Value to write to the specified node.
        );

        /*
         * Reads a string value from the config tree.
         *
         * @return
         *   - string value for the 'key'
         */
        le_result_t tafUpdate_ConfigTree_GetString(
            const std::string key,  ///< [IN] Node to read from the config tree.
            std::string &value      ///< [OUT] Value read from the specified Node.
        );

        /*
         * Writes a string value to the config tree.
         */
        le_result_t tafUpdate_ConfigTree_SetString(
            const std::string key,  ///< [IN] Node to read from the config tree.
            const std::string value ///< [IN] Value to write to the specified node.
        );

        /*
         * Deletes the config tree.
         */
        void tafUpdate_ConfigTree_ClearTree();
    }
#endif
