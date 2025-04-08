/*
 *  Copyright (c) 2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafMngdConn_ConfigTreeHelper.hpp"

le_result_t tafsvc::tafMngd_ConfigTree_Read(const std::string &Node,
                                                    char *strValuePtr,
                                                    size_t ValueSize)
{

    le_result_t leRet = LE_OK;
    if (Node.empty() || NULL == strValuePtr || ValueSize <= 0)
    {
        LE_WARN("Invalid Parameter");
        return LE_BAD_PARAMETER;
    }

    // Open up a read transaction on the Config Tree.
    LE_DEBUG("Calling le_cfg_CreateReadTxn() and le_cfg_CancelTxn()");
    le_cfg_IteratorRef_t iteratorRef_r = le_cfg_CreateReadTxn("tafMngdConnSvc");
    if (le_cfg_NodeExists(iteratorRef_r, Node.c_str()) == false)
    {
        LE_WARN("Node \"%s\" not found.", Node.c_str());
        leRet = LE_NOT_FOUND;
    }
    else
    {
        LE_DEBUG("Node \"%s\" Found", Node.c_str());
        leRet = le_cfg_GetString(iteratorRef_r, Node.c_str(),
                                             strValuePtr, ValueSize,
                                             "");
        if (LE_OK == leRet)
        {
            LE_DEBUG("Value of \"%s\" is %s", Node.c_str(), strValuePtr);
            // Set return value to true
        }
        else
        {
            LE_WARN("Node \"%s\" read failed: %d", Node.c_str(), leRet);
        }
    }
    le_cfg_CancelTxn(iteratorRef_r);

    return leRet;
}

le_result_t tafsvc::tafMngd_ConfigTree_Update(const std::string& Node,
                                                      const std::string& Value)
{
    if (Node.empty() || Value.empty())
    {
        LE_WARN("Invalid Parameter");
        return LE_BAD_PARAMETER;
    }

    le_cfg_IteratorRef_t iteratorRef_r = le_cfg_CreateWriteTxn("tafMngdConnSvc");
    // Change the location of iterator. The target node does not need to exist.
    // Writing a value to a non-existent node will automatically create that node.
    le_cfg_GoToNode (iteratorRef_r, Node.c_str());
    // Pass an empty path. The iterator's current node will be set
    le_cfg_SetString(iteratorRef_r, "", Value.c_str());
    le_cfg_CommitTxn(iteratorRef_r);
    return LE_OK;
}
