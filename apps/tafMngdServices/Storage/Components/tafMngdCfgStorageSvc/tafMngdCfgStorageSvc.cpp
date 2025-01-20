/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "tafMngdStorageSvc.hpp"

using namespace telux::tafsvc;

/**
 * Copy the file from the paths configured in the MSS configuration JSON file to configuration
 * storage and also checks the validity of file
 */
le_result_t taf_mngdStorCfg_Update
(
    taf_mngdStorCfg_ConfigRef_t configRef,
    const char* LE_NONNULL version
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.Update(configRef,version);
}

/**
 * Replace the orignal configuration file with the updated file in config storage.
 */
le_result_t taf_mngdStorCfg_Activate
(
    taf_mngdStorCfg_ConfigRef_t configRef
){
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.Activate(configRef);
}

/**
 *  Cancel the update file campaign.
 */
le_result_t taf_mngdStorCfg_Cancel
(
    taf_mngdStorCfg_ConfigRef_t configRef
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.Cancel(configRef);
}

/**
 *  Revert to orignal version of config file.
 */
le_result_t taf_mngdStorCfg_Rollback
(
    taf_mngdStorCfg_ConfigRef_t configRef
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.Rollback(configRef);
}

/**
 *  Commit data to config storage.
 */
le_result_t taf_mngdStorCfg_Commit
(
    taf_mngdStorCfg_ConfigRef_t configRef
)
{
   auto &mss = tafMngdStorageSvc::GetInstance();

   return mss.Commit(configRef);
}

/**
 * Get the configuration storage reference.
 */
taf_mngdStorCfg_ConfigRef_t  taf_mngdStorCfg_GetRef
(
)
{
   auto &mss = tafMngdStorageSvc::GetInstance();

   return mss.GetRef();
}

/**
 * Get the major and minor version of the given configuration file reference.
 */
le_result_t taf_mngdStorCfg_GetVersion
(
    taf_mngdStorCfg_ConfigRef_t ConfigRef,
    uint32_t* MajorVersionPtr,
    uint32_t* MinorVersionPtr,
    uint32_t* PatchVersionPtr
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(MajorVersionPtr == nullptr || MinorVersionPtr == nullptr
        || PatchVersionPtr ==nullptr,LE_BAD_PARAMETER, "output pointer is NULL");
    return mss.GetVersion(ConfigRef, MajorVersionPtr,MinorVersionPtr,PatchVersionPtr);
}

/**
 * Get the value and data type of the node.
 */
le_result_t taf_mngdStorCfg_GetValue
(
    taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char* LE_NONNULL groupName,
    const char* LE_NONNULL nodeName,
    taf_mngdStorCfg_NodeType_t* typePtr,
    char* nodeValue,
    size_t nodeValueSize
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(typePtr== nullptr || nodeValue== nullptr, LE_BAD_PARAMETER,
        "output pointer is NULL");
    return mss.GetValue(ConfigRef, groupName, nodeName, typePtr,nodeValue,nodeValueSize);
}

/**
 * Get the data type of the node.
 */
le_result_t taf_mngdStorCfg_GetType
(
   taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char* LE_NONNULL groupName,
    const char* LE_NONNULL nodeName,
    taf_mngdStorCfg_NodeType_t* typePtr
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(typePtr== nullptr , LE_BAD_PARAMETER, "output pointer is NULL");
    return mss.GetType(ConfigRef, groupName, nodeName, typePtr);
}

/**
 * Get the value of the node with data type string.
 */
le_result_t taf_mngdStorCfg_GetString
(
    taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char* LE_NONNULL groupName,
    const char* LE_NONNULL nodeName,
    char* nodeValue,
    size_t nodeValueSize
)
{
    auto &mss = tafMngdStorageSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(nodeValue== nullptr , LE_BAD_PARAMETER, "output pointer is NULL");
    return mss.GetString(ConfigRef,groupName,nodeName,nodeValue,nodeValueSize);
}

/**
 * Get the value of the node with data type integer.
 */
le_result_t taf_mngdStorCfg_GetInt
(
    taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char* LE_NONNULL groupName,
    const char* LE_NONNULL nodeName,
    int32_t* nodeValuePtr
){
    auto &mss = tafMngdStorageSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(nodeValuePtr== nullptr , LE_BAD_PARAMETER, "output pointer is NULL");
    return mss.GetInt(ConfigRef, groupName, nodeName, nodeValuePtr);
}

/**
 * Get the value of the node with data type float.
 */
le_result_t taf_mngdStorCfg_GetFloat
(
    taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char* LE_NONNULL groupName,
    const char* LE_NONNULL nodeName,
    double* nodeValuePtr
){
    auto &mss = tafMngdStorageSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(nodeValuePtr== nullptr , LE_BAD_PARAMETER, "output pointer is NULL");
    return mss.GetFloat(ConfigRef, groupName, nodeName, nodeValuePtr);
}

/**
 * Get the value of the node with data type boolean.
 */
le_result_t taf_mngdStorCfg_GetBool
(
    taf_mngdStorCfg_ConfigRef_t ConfigRef,
    const char* LE_NONNULL groupName,
    const char* LE_NONNULL nodeName,
    int32_t* nodeValuePtr
){
    auto &mss = tafMngdStorageSvc::GetInstance();
    TAF_ERROR_IF_RET_VAL(nodeValuePtr== nullptr , LE_BAD_PARAMETER, "output pointer is NULL");
    return mss.GetBool(ConfigRef, groupName, nodeName, nodeValuePtr);
}

/**
 * Get the value of the node with data type boolean.
 */
le_result_t taf_mngdStorCfg_ReleaseRef
(
    taf_mngdStorCfg_ConfigRef_t ConfigRef
){
     auto &mss = tafMngdStorageSvc::GetInstance();

    return mss.ReleaseRef(ConfigRef);
}

COMPONENT_INIT
{
    LE_INFO("tafMngdStorageSvc COMPONENT init...");

    auto &mss = tafMngdStorageSvc::GetInstance();

    mss.Init();
}