/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafMngdStorageSvc.hpp"

using namespace tafsvc;

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