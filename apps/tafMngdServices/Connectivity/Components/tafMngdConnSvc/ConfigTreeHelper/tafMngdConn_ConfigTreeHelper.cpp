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

#include "tafMngdConn_ConfigTreeHelper.hpp"

le_result_t telux::tafsvc::tafMngd_ConfigTree_Read(const std::string &Node,
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

le_result_t telux::tafsvc::tafMngd_ConfigTree_Update(const std::string& Node,
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