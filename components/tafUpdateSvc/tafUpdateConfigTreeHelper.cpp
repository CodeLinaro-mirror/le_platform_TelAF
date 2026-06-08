/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafUpdateConfigTreeHelper.hpp"

#define TXN_NAME "tafUpdateConfigTreeHelper"

//--------------------------------------------------------------------------------------------------
/**
 * Get a boolean value from the config tree.
 *
 * @return
 *  - The boolean value stored at the given key, or false if the key is empty or not found.
 */
//--------------------------------------------------------------------------------------------------
bool tafsvc::tafUpdate_ConfigTree_GetBool
(
    const std::string key ///< [IN] Config tree key.
)
{
    bool defaultValue = false;
    if (key.empty())
    {
        LE_WARN("Invalid Parameter");
        return defaultValue;
    }

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(TXN_NAME);
    LE_DEBUG("key %s Found", key.c_str());
    bool ret = le_cfg_GetBool(iterRef, key.c_str(), defaultValue);
    le_cfg_CancelTxn(iterRef);
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set a boolean value in the config tree.
 */
//--------------------------------------------------------------------------------------------------
void tafsvc::tafUpdate_ConfigTree_SetBool
(
    const std::string key, ///< [IN] Config tree key.
    const bool value       ///< [IN] Boolean value to set.
)
{
    if (key.empty())
    {
        LE_WARN("Invalid Parameter");
        return;
    }
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(TXN_NAME);
    le_cfg_GoToNode(iterRef, key.c_str());
    le_cfg_SetBool(iterRef, "", value);
    le_cfg_CommitTxn(iterRef);
    LE_DEBUG("Setting value of %s as %s", key.c_str(),
        value ? "true" : "false");
}

//--------------------------------------------------------------------------------------------------
/**
 * Get an integer value from the config tree.
 *
 * @return
 *  - The integer value stored at the given key, or 0 if the key is empty or not found.
 */
//--------------------------------------------------------------------------------------------------
uint32_t tafsvc::tafUpdate_ConfigTree_GetInt
(
    const std::string key ///< [IN] Config tree key.
)
{
    int32_t defaultValue = 0;
    if (key.empty())
    {
        LE_WARN("Invalid Parameter");
        return defaultValue;
    }

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(TXN_NAME);
    LE_DEBUG("key %s Found", key.c_str());
    int32_t ret = le_cfg_GetInt(iterRef, key.c_str(), defaultValue);
    le_cfg_CancelTxn(iterRef);
    return static_cast<uint32_t>(ret);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set an integer value in the config tree.
 */
//--------------------------------------------------------------------------------------------------
void tafsvc::tafUpdate_ConfigTree_SetInt
(
    const std::string key,  ///< [IN] Config tree key.
    const uint32_t value    ///< [IN] Integer value to set.
)
{
    if (key.empty())
    {
        LE_WARN("Invalid Parameter");
        return;
    }
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(TXN_NAME);
    le_cfg_GoToNode(iterRef, key.c_str());
    le_cfg_SetInt(iterRef, "", static_cast<int32_t>(value));
    le_cfg_CommitTxn(iterRef);
    LE_DEBUG("Setting value of %s as %u", key.c_str(), value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a string value from the config tree.
 *
 * @return
 *  - LE_OK             The string value was retrieved successfully.
 *  - LE_BAD_PARAMETER  key is empty.
 *  - LE_NOT_FOUND      The key does not exist in the config tree.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafsvc::tafUpdate_ConfigTree_GetString
(
    const std::string key, ///< [IN] Config tree key.
    std::string &value     ///< [OUT] String value retrieved from the config tree.
)
{
    le_result_t leRet = LE_OK;
    if (key.empty())
    {
        LE_DEBUG("Invalid Parameter");
        return LE_BAD_PARAMETER;
    }

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(TXN_NAME);
    if (!le_cfg_NodeExists(iterRef, key.c_str()))
    {
        LE_DEBUG("key %s not found.", key.c_str());
        leRet = LE_NOT_FOUND;
    }
    else
    {
        LE_DEBUG("key %s Found", key.c_str());
        char buff[kMaxBufSize] = {};
        leRet = le_cfg_GetString(iterRef, key.c_str(), buff, kMaxBufSize, "");
        if (LE_OK == leRet)
        {
            value = std::string(buff);
            LE_DEBUG("value of %s is %s", key.c_str(), value.c_str());
        }
        else
        {
            LE_DEBUG("key %s read failed: %d", key.c_str(), leRet);
        }
    }
    le_cfg_CancelTxn(iterRef);
    return leRet;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set a string value in the config tree.
 *
 * @return
 *  - LE_OK             The string value was set successfully.
 *  - LE_BAD_PARAMETER  key or value is empty.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tafsvc::tafUpdate_ConfigTree_SetString
(
    const std::string key,   ///< [IN] Config tree key.
    const std::string value  ///< [IN] String value to set.
)
{
    if (key.empty() || value.empty())
    {
        LE_WARN("Invalid Parameter");
        return LE_BAD_PARAMETER;
    }
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(TXN_NAME);
    le_cfg_GoToNode(iterRef, key.c_str());
    le_cfg_SetString(iterRef, "", value.c_str());
    le_cfg_CommitTxn(iterRef);
    LE_DEBUG("Setting value of %s as %s", key.c_str(), value.c_str());
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear the entire config tree.
 */
//--------------------------------------------------------------------------------------------------
void tafsvc::tafUpdate_ConfigTree_ClearTree
(
    void
)
{
    LE_DEBUG("Clearing entire config tree");
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(TXN_NAME);
    le_cfg_DeleteNode(iterRef, "");
    le_cfg_CommitTxn(iterRef);
}
