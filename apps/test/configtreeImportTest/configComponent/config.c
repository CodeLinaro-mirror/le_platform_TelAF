/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define TREE_NAME_MAX 65

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

COMPONENT_INIT
{
    LE_INFO("---------- Started testing in: %s -------------------------------------", TestRootDir);

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
