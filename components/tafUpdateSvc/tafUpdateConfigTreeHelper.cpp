/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafUpdateConfigTreeHelper.hpp"

#define TXN_NAME "tafUpdateConfigTreeHelper"

bool tafsvc::tafUpdate_ConfigTree_GetBool
(
    const std::string key
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

void tafsvc::tafUpdate_ConfigTree_SetBool
(
    const std::string key,
    const bool value
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

uint32_t tafsvc::tafUpdate_ConfigTree_GetInt
(
    const std::string key
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

void tafsvc::tafUpdate_ConfigTree_SetInt
(
    const std::string key,
    const uint32_t value
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

le_result_t tafsvc::tafUpdate_ConfigTree_GetString
(
    const std::string key,
    std::string &value
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

le_result_t tafsvc::tafUpdate_ConfigTree_SetString
(
    const std::string key,
    const std::string value
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

void tafsvc::tafUpdate_ConfigTree_ClearTree()
{
    LE_DEBUG("Clearing entire config tree");
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(TXN_NAME);
    le_cfg_DeleteNode(iterRef, "");
    le_cfg_CommitTxn(iterRef);
}
