/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
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

    LE_INFO("Tree - customer: valueSet %d, valueGet1 %d, valueGet2 %d, valueGet3 %d",
        valueSet, valueGet1, valueGet2, valueGet3);

    LE_TEST_ASSERT((ret == LE_NOT_FOUND), "Test R/W for customer tree.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test read only config tree data that imported from a .cfg file.
 * The TelAF framework will import the data for 'configTreeSample' tree before the
 * 'configTreeSample' app starts up at every boot. No changes can be made to the partition.
 *
 * Example:
 * $ config get configTreeSample:/readOnlyConfig/private/testItem81
 */
//--------------------------------------------------------------------------------------------------
static void TestNote_readOnlyConfig(void)
{
    char treePath[TREE_PATH_MAX] = "";
    char config_node[TREE_PATH_MAX] = "/readOnlyConfig/private/testItem81";

    int32_t valueSet = 1, valueGet1 = 2;

    // Read the value of testItem81.
    GetConfigIntNodeValue(treePath, config_node, &valueGet1);

    LE_INFO("Tree - configTreeSample: valueGet1 %d, valueSet %d",
            valueGet1, valueSet);

    LE_TEST_ASSERT((valueGet1 == 81), "Test reading RO cfg node: readOnlyConfig");

}


//--------------------------------------------------------------------------------------------------
/**
 * Test read only config tree 'TestConfigTree'.
 * The read only tree file is stored in a read only region: "/legato/systems/current/config", so
 * no changes can be made to the partition.
 *
 * Example:
 * $ config get TestConfigTree:/configTreeData/testItem22
 */
//--------------------------------------------------------------------------------------------------
static void TestTree_TestConfigTree(void)
{
    char treePath[TREE_PATH_MAX] = "TestConfigTree:/configTreeData";
    char config_node[TREE_PATH_MAX] = "testItem22";

    int32_t valueGet1 = 2;

    // Read the value of testItem22.
    GetConfigIntNodeValue(treePath, config_node, &valueGet1);

    LE_INFO("Tree - TestConfigTree: valueGet1 %d", valueGet1);
    LE_TEST_ASSERT((valueGet1 == 22), "Test reading RO config tree: TestConfigTree");

}


// ---------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("---------- Started running config tree sample code -------------");

    TestTree_customer();
    TestTree_TestConfigTree();
    TestNote_readOnlyConfig();

    LE_INFO("----------------- All Tests Completed ---------------------------");

    LE_TEST_EXIT;
}
