/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define TREE_PATH_MAX 128

le_result_t GetConfigIntNodeValue
(
    const char* treePath,  ///< Tree path + child node
    const char* item,      ///< Item
    int * intVolue         ///< Type for this "Item"
)
{
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn( treePath );

    char config_node[TREE_PATH_MAX] = {0};
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
    return LE_NOT_FOUND;
}

le_result_t SetConfigIntNodeValue
(
    const char* treePath,      ///< Tree path + child node
    const char* item,          ///< Item
    int32_t     value
)
{
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( treePath );

    char config_node[TREE_PATH_MAX] = {};
    snprintf(config_node, sizeof(config_node), "%s", item);

    le_cfg_SetInt(iteratorRef, config_node, value);
    le_cfg_CommitTxn(iteratorRef);

    LE_INFO("Set config node %s as %d", config_node, value);

    return LE_OK;
}

static void ClearTreeNode(char* rootNode, char* item)
{
    LE_INFO("Clear rootNode: %s, item: %s", rootNode, item);
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(rootNode);
    LE_FATAL_IF(iterRef == NULL, "Test: %s - Could not create iterator.", rootNode);

    le_cfg_DeleteNode(iterRef, item);

    le_cfg_CommitTxn(iterRef);
}

static bool IsNodeEmpty(char *rootNode, char* item)
{
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(rootNode);
    if (le_cfg_IsEmpty(iterRef, item))
    {
        LE_INFO("Tree node: %s is empty", item);
        le_cfg_CommitTxn(iterRef);
        return true;
    }
    LE_INFO("Tree node: %s is not empty", item);
    le_cfg_CommitTxn(iterRef);
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the empty config tree. If the partition supports read/write operations, the data will be
 * updated accordingly. After restarting the system, this function will read back the data, and
 * the result will match the previously set values.
 *
 * Example:
 *   $ config get customer:/dynamic/testItem0
 */
//--------------------------------------------------------------------------------------------------
static void TestTree_customer(void)
{
    char treePath[TREE_PATH_MAX] = "customer:/dynamic";
    char config_node[TREE_PATH_MAX] = "testItem0";
    le_result_t ret = LE_OK;

    int32_t valueSet = 1, valueGet1 = 0, valueGet2 = 0, valueGet3 = 0;

    LE_INFO("Test tree - 'customer': read/write/delete");

    // Read the value of testItem0.
    GetConfigIntNodeValue(treePath, config_node, &valueGet1);

    // Change the value for testItem0.
    SetConfigIntNodeValue(treePath, config_node, valueSet);

    // Read the value of the tree again.
    ret = GetConfigIntNodeValue(treePath, config_node, &valueGet2);
    LE_TEST_ASSERT((ret == LE_OK), "Test 'read' customer tree.");

    // Delete the tree both in memory and partition. But, the tree can NOT be deleted if it is
    // storing in R/O partition.
    le_cfgAdmin_DeleteTree("customer");

    // Read the value of the tree again.
    ret = GetConfigIntNodeValue(treePath, config_node, &valueGet3);

    LE_INFO("Get and set status: valueSet %d, valueGet1 %d, valueGet2 %d, valueGet3 %d",
        valueSet, valueGet1, valueGet2, valueGet3);

    LE_TEST_ASSERT((ret == LE_NOT_FOUND), "Test R/W for customer tree.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Test read only config tree data that imported from a .cfg file.
 * The TelAF framework will import the data for 'configTreeSample' tree before the
 * 'configTreeSample' app starts up at every boot. The change can NOT persist.
 *
 * Example:
 * $ config get configTreeSample:/readOnlyConfig/private/testItem81
 */
//--------------------------------------------------------------------------------------------------
static void TestNode_readOnlyConfig(void)
{
    char treePath[TREE_PATH_MAX] = "";
    char config_node1[TREE_PATH_MAX] = "";
    char config_node2[TREE_PATH_MAX] = "";
    char config_path1[TREE_PATH_MAX] = "/readOnlyConfig/private";
    char config_path2[TREE_PATH_MAX] = "/readOnlyConfig/newWrite";

    int32_t writeValue = 2, valueGet1 = 0, valueGet2 = 0, valueGet3 = 0;
    LE_INFO("Test tree - 'configTreeSample' RO node 'readOnlyConfig'");

    snprintf(config_node1, sizeof(config_node1), "%s/testItem81", config_path1);
    snprintf(config_node2, sizeof(config_node2), "%s/writeValue", config_path2);

    // Read item "configTreeSample:/readOnlyConfig/private/testItem81".
    GetConfigIntNodeValue(treePath, config_node1, &valueGet1);
    LE_TEST_ASSERT((valueGet1 == 81), "Test readOnlyConfig - read testItem81");

    // Read item "configTreeSample:/readOnlyConfig/newWrite/writeValue".
    // This item was added by the previously test and should be recovered by framework.
    GetConfigIntNodeValue(treePath, config_node2, &valueGet2);
    LE_TEST_ASSERT((valueGet2 != writeValue), "Test readOnlyConfig - read 'writeValue'");

    // Change the value of item ''writeValue' for next start up verification.
    // Before starting APP "configTreeSample", the framework will import the read-only .cfg
    // and replace all the data under "readOnlyConfig".
    SetConfigIntNodeValue(treePath, config_node2, writeValue);
    GetConfigIntNodeValue(treePath, config_node2, &valueGet3);
    LE_INFO("Get and set status: valueGet1: %d, valueGet2: %d, writeValue: %d, valueGet3: %d",
        valueGet1, valueGet2, writeValue, valueGet3);
    LE_TEST_ASSERT((writeValue == valueGet3), "Test readOnlyConfig - add item 'writeValue'");

}

//--------------------------------------------------------------------------------------------------
/**
 * Test read write config tree data that imported from a .cfg file.
 * Before starting this test APP, the framework will check whether a root node with the name
 * "readWriteConfig" is existed in the app's config tree or not. If it does, no action will betaken.
 * If not, the ".cfg" file will be imported into the config tree using its filename as the root
 * node.
 *
 * Example:
 * $ config get configTreeSample:/readWriteConfig/private/testItem01
 */
//--------------------------------------------------------------------------------------------------
static void TestNode_readWriteConfig(void)
{
    char treePath[TREE_PATH_MAX] = "";
    char config_node1[TREE_PATH_MAX] = "";
    char config_node2[TREE_PATH_MAX] = "";
    char config_path1[TREE_PATH_MAX] = "/readWriteConfig/private";
    char config_path2[TREE_PATH_MAX] = "/readWriteConfig/newWrite";

    int32_t writeValue = 2, valueGet1 = 0, valueGet2 = 0, valueGet3 = 0;
    LE_INFO("Test tree - 'configTreeSample' RW node 'readWriteConfig'");

    snprintf(config_node1, sizeof(config_node1), "%s/testItem01", config_path1);
    snprintf(config_node2, sizeof(config_node2), "%s/writeValue", config_path2);

    // Read item "configTreeSample:/readWriteConfig/readWriteConfig/private/testItem01".
    GetConfigIntNodeValue(treePath, config_node1, &valueGet1);
    LE_TEST_ASSERT((valueGet1 == 1), "Test readWriteConfig - read testItem01");

    // Read item "configTreeSample:/readWriteConfig/readWriteConfig/newWrite/writeValue".
    GetConfigIntNodeValue(treePath, config_node2, &valueGet2);
    if (valueGet2 != writeValue)
    { // run loop 1:
        // Change the value of item writeValue for this run loop.
        SetConfigIntNodeValue(treePath, config_node2, writeValue);
        GetConfigIntNodeValue(treePath, config_node2, &valueGet3);
        LE_TEST_ASSERT((writeValue == valueGet3), "Test readWriteConfig - add item 'writeValue'");
    }
    else
    { // run loop 2:
        // Clear the root node for this run loop.
        ClearTreeNode("configTreeSample:", "/readWriteConfig");

        LE_TEST_ASSERT(IsNodeEmpty("configTreeSample:", "/readWriteConfig"),
            "Test readWriteConfig - is empty node 'readWriteConfig'");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test to delete the config tree both in memory and partition.
 *
 * Example:
 * $ echo a > /tmp/tree.txt
 * $ app start configTreeSample
 */
//--------------------------------------------------------------------------------------------------
static void Test_deleteTheWholeTreeFile(void)
{
    bool deleteFileSucc = true;

    if (access("/tmp/tree.txt", F_OK) == 0)
    {
        LE_INFO("Found '/tmp/tree.txt ...");
        LE_INFO("Deleting config tree file: configTreeSample ...");

        // Delete the tree both in memory and partition. But, the tree can NOT be deleted
        // if it is storing in R/O partition.
        le_cfgAdmin_DeleteTree("configTreeSample");

        if ((access("/data/persist/telaf/config/configTreeSample.scissors", F_OK) == 0)
             ||(access("/data/persist/telaf/config/configTreeSample.paper", F_OK) == 0)
             || (access("/data/persist/telaf/config/configTreeSample.rock", F_OK) == 0)
           )
        {
            deleteFileSucc = false;
        }
        LE_TEST_ASSERT(deleteFileSucc, "Tree - configTreeSample: delete");
    }
    else
    {
        LE_INFO("File '/tmp/tree.txt not exist, skip testing delete tree file.");
    }
}

//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("---------- Started running config tree sample code -------------");

    TestTree_customer();
    TestNode_readOnlyConfig();
    TestNode_readWriteConfig();

    Test_deleteTheWholeTreeFile();

    LE_INFO("----------------- All Tests Completed ---------------------------");

    LE_TEST_EXIT;
}
