/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define TREE_NAME_MAX 65

#define CFG_READ_ONLY_PATH "configUnitTest:/configTest-readOnly"
#define CFG_READ_WRITE_PATH "configUnitTest:/configTest-readWrite"

static char TestRootDir[LE_CFG_STR_LEN_BYTES] = "configTest";

static void ClearTree(const char* testRoot)
{
    LE_INFO("---- Clearing Out Current Tree -----------------------------------------------------");
    char cfgRootDir[LE_CFG_STR_LEN_BYTES] = "";
    snprintf(cfgRootDir,TREE_NAME_MAX, "configTest-%s", testRoot);
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(cfgRootDir);
    LE_FATAL_IF(iterRef == NULL, "Test: %s - Could not create iterator.", cfgRootDir);

    le_cfg_DeleteNode(iterRef, "");
    le_cfg_CommitTxn(iterRef);
}

static void TestImportJSON(const char* testRoot, bool isSuccess)
{
    LE_INFO("---- Import Export Function Test: %s -------------------------------------", testRoot);

    char primitiveRootDir[LE_CFG_STR_LEN_BYTES] = "";
    snprintf(primitiveRootDir, TREE_NAME_MAX, "configTest-%s", testRoot);
    static char pathBuffer[LE_CFG_STR_LEN_BYTES] = "";
    LE_ASSERT(snprintf(pathBuffer, LE_CFG_STR_LEN_BYTES, "/%s", primitiveRootDir)
              <= LE_CFG_STR_LEN_BYTES);

    char filePath[PATH_MAX] = "";
    snprintf(filePath,
     150, "/legato/systems/current/appsWriteable/configUnitTest/data/configTest-%s.json", testRoot);

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn("");

    LE_INFO("IMPORT TREE: %s", pathBuffer);
    LE_INFO("Import: %s", filePath);
    if(isSuccess){
        LE_TEST(le_cfgAdmin_ImportTree(iterRef, filePath, pathBuffer) == LE_OK);
        le_cfg_CommitTxn(iterRef);
    }
    else {
        LE_TEST(le_cfgAdmin_ImportTree(iterRef, filePath, pathBuffer) != LE_OK);
        le_cfg_CancelTxn(iterRef);
    }

    unlink(filePath);

}

le_result_t GetConfigIntNodeValue
(
    const char* treePath,  ///< Tree path + child node
    const char* item,      ///< Item
    int * intVolue         ///< Type for this "Item"
)
{
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn( treePath );

    char config_node[TREE_NAME_MAX] = {0};
    snprintf(config_node, sizeof(config_node), "%s", item);

    if (le_cfg_NodeExists(iteratorRef, config_node))
    {
        *intVolue = le_cfg_GetInt(iteratorRef,config_node, 0);
        LE_INFO("Get config node %s = %d", config_node, *intVolue);

        le_cfg_CancelTxn(iteratorRef);
        return LE_OK;
    }

    LE_WARN("The config node %s doesn't exist", config_node);
    le_cfg_CancelTxn(iteratorRef);
    return LE_OK;
}

le_result_t SetConfigIntNodeValue
(
    const char* treePath,      ///< Tree path + child node
    const char* item,          ///< Item
    int32_t     value
)
{
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( treePath );

    char config_node[TREE_NAME_MAX] = {};
    snprintf(config_node, sizeof(config_node), "%s", item);

    le_cfg_SetInt(iteratorRef, config_node, value);
    le_cfg_CommitTxn(iteratorRef);

    LE_INFO("Set config node %s as %d", config_node, value);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The JSON file "configTest-readOnly.json" was bundled as read only in Component.cdef, it can not
 * be changed in file system storage. Every time this JSON file was imported to config tree should
 * has the some value.
 *
 * Note, If you want to use the read only configurations, please import it from RO JSON file to
 * config tree during your APP initialization. And later you can read it through config tree APIs.
 */
//--------------------------------------------------------------------------------------------------
static void TestImportReadOnly(void)
{
    int32_t valueSet = 202412, valueGet1 = 0, valueGet2 = 0;

    // Clean all the tree data for the 'readOnly' node, if any, and import it again.
    ClearTree("readOnly");
    TestImportJSON("readOnly", true);

    // Read the value of anIntVal.
    GetConfigIntNodeValue(CFG_READ_ONLY_PATH, "anIntVal", &valueGet1);

    // Change the value for anIntVal.
    SetConfigIntNodeValue(CFG_READ_ONLY_PATH, "anIntVal", valueSet);

    // Clean all the tree data and import it again.
    ClearTree("readOnly");
    TestImportJSON("readOnly", true);

    // Read the value of anIntVal.
    GetConfigIntNodeValue(CFG_READ_ONLY_PATH, "anIntVal", &valueGet2);

    LE_INFO("readOnly: valueGet1 %d, valueSet %d, valueGet2 %d", valueGet1, valueSet, valueGet2);
    LE_TEST_OK(valueGet1 == valueGet2, "Test readOnly for %s", CFG_READ_ONLY_PATH);
}

//--------------------------------------------------------------------------------------------------
/**
 * The JSON file "configTest-readWrite.json" was bundled as read write in Component.cdef, it can be
 * changed in the file system storage.
 *
 * The config tree can also be changed through config tree API and store to file system storage as
 * tree file. And later you can mentain it through config tree APIs.
 */
//--------------------------------------------------------------------------------------------------
static void TestImportReadWrite(void)
{
    int32_t valueSet = 202412, valueGet1 = 0, valueGet2 = 0;

    // Clean all the tree data for the 'readWrite' node, if any, and import it again.
    ClearTree("readWrite");
    TestImportJSON("readWrite", true);

    // Read the value of anIntVal.
    GetConfigIntNodeValue(CFG_READ_WRITE_PATH, "anIntVal", &valueGet1);

    // Change the value for anIntVal.
    SetConfigIntNodeValue(CFG_READ_WRITE_PATH, "anIntVal", valueSet);

    // Read the value of anIntVal.
    GetConfigIntNodeValue(CFG_READ_WRITE_PATH, "anIntVal", &valueGet2);

    LE_INFO("readWrite: valueGet1 %d, valueSet %d, valueGet2 %d", valueGet1, valueSet, valueGet2);
    LE_TEST_OK(valueSet == valueGet2, "Test readWrite for %s", CFG_READ_WRITE_PATH);
}

COMPONENT_INIT
{
    LE_INFO("---------- Started testing in: %s -------------------------------------", TestRootDir);

    TestImportReadOnly();
    TestImportReadWrite();

    ClearTree("primitive");
    TestImportJSON("primitive", true);

    ClearTree("1000");
    TestImportJSON("1000", true);

    ClearTree("5000");
    TestImportJSON("5000", true);

    TestImportJSON("errorValue", false);

    TestImportJSON("errorFormat", false);

    LE_INFO("---------- All Tests Complete in: %s ----------------------------------", TestRootDir);

    LE_TEST_EXIT;
}
