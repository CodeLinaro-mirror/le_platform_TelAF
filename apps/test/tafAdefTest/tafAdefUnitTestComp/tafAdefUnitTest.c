/*Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

#define CNFG_TEST_NAME_SIZE 20
#define CNFG_TREE_NAME_MAX 65
#define CNFG_SMALL_STRING_SIZE 5
#define CNFG_INT_VALUE            42
#define CNFG_INT_DEFAULT_VALUE    56

// Test values
#define CNFG_BOOL_VALUE         true
#define BOOL_DEFAULT_VALUE false
#define CNFG_STRING_VALUE         "TelAF"
#define CNFG_STRING_DEFAULT_VALUE "username"
#define CNFG_FLOAT_VALUE          3.14f
#define CNFG_FLOAT_DEFAULT_VALUE  137.036f
#define CNFG_FLOAT_EPSILON        0.001f

// Root path of config tree to test
#define TEST_ROOT_NODE "/testConfigTree"

static char ConfigTreeTestRootDir[LE_CFG_STR_LEN_BYTES] = "";

static char ConfigTreeTestAppRootDir[LE_CFG_STR_LEN_BYTES] = "";

//--------------------------------------------------------------------------------------------------
/**
 * Maximum string length and byte storage size of application names.
 */
//--------------------------------------------------------------------------------------------------
#define CNFG_LIMIT_MAX_APP_NAME_LEN                  128
#define CNFG_LIMIT_MAX_APP_NAME_BYTES                (CNFG_LIMIT_MAX_APP_NAME_LEN + 1)

#define CNFG_LIMIT_MAX_NUM_CAPABILITIES              40

// 35 bytes (4 more than the segment size + null)
#define CNFG_TEST_PATTERN_LARGE_STRING   "1234567890123456789012345678901234"
// 2 bytes + null ; small string
#define CNFG_TEST_PATTERN_SMALL_STRING   "12"

// Long string - 511 bytes
#define CNFG_TEST_PATTERN_MAX_SIZE_STRING    "09876543210987654321098765432109876543210987654321"\
                                        "09876543210987654321098765432109876543210987654321"\
                                        "09876543210987654321098765432109876543210987654321"\
                                        "09876543210987654321098765432109876543210987654321"\
                                        "09876543210987654321098765432109876543210987654321"\
                                        "09876543210987654321098765432109876543210987654321"\
                                        "09876543210987654321098765432109876543210987654321"\
                                        "09876543210987654321098765432109876543210987654321"\
                                        "09876543210987654321098765432109876543210987654321"\
                                        "09876543210987654321098765432109876543210987654321"\
                                        "12345678901"

// ConfigTreeTestRootDir array size (512 bytes) + file name size (100 bytes)
#define CNFG_NAMETEMPLATESIZE            612

static const char* ConfigTreeNodeTypeStr
(
    le_cfg_IteratorRef_t iterRef
)
{
    le_cfg_nodeType_t type = le_cfg_GetNodeType(iterRef, "");

    // Add detailed descriptions for each node type
    switch (type)
    {
        case LE_CFG_TYPE_STRING:
            return "This node contains a string value.";

        case LE_CFG_TYPE_EMPTY:
            return "This is an empty node with no value.";

        case LE_CFG_TYPE_BOOL:
            return "This node represents a boolean value (true or false).";

        case LE_CFG_TYPE_INT:
            return "This node contains an integer value.";

        case LE_CFG_TYPE_FLOAT:
            return "This node contains a floating-point number.";

        case LE_CFG_TYPE_STEM:
            return "This is a stem node that can have child nodes.";

        case LE_CFG_TYPE_DOESNT_EXIST:
            return "This node does not exist in the configuration.";
    }

    return "This is an unknown or unsupported node type.";
}



static void ConfigTreeDumpTree(le_cfg_IteratorRef_t iterRef, size_t indent)
{
    if (le_arg_NumArgs() == 1)
    {
        return;
    }

    le_result_t result;
    static char ConfigTreestrConfigTreeBuffer[LE_CFG_STR_LEN_BYTES] = "";

    do
    {
        size_t i;

        for (i = 0; i < indent; i++)
        {
            printf(" ");
        }

        le_cfg_GetNodeName(iterRef, "", ConfigTreestrConfigTreeBuffer, LE_CFG_STR_LEN_BYTES);
        le_cfg_nodeType_t type = le_cfg_GetNodeType(iterRef, "");

        // Skip printing empty nodes
        if (type == LE_CFG_TYPE_STEM)
        {
            printf("%s/\n", ConfigTreestrConfigTreeBuffer);
            result = le_cfg_GoToFirstChild(iterRef);
            LE_FATAL_IF(result != LE_OK,
                        "Test: stem %s has no child.", ConfigTreestrConfigTreeBuffer);
            ConfigTreeDumpTree(iterRef, indent + 2);
            le_cfg_GoToParent(iterRef);
        }
        else if (type != LE_CFG_TYPE_EMPTY)
        {
            printf("%s<%s> == ", ConfigTreestrConfigTreeBuffer, ConfigTreeNodeTypeStr(iterRef));
            le_cfg_GetString(iterRef, "", ConfigTreestrConfigTreeBuffer, LE_CFG_STR_LEN_BYTES, "");
            printf("%s\n", ConfigTreestrConfigTreeBuffer);

        }
    }
    while (le_cfg_GoToNextSibling(iterRef) == LE_OK);
}




static void ConfigTreeQuickFunctionTest()
{
    le_result_t result;
    char pathBuffer[LE_CFG_STR_LEN_BYTES] = "";

    LE_TEST_INFO("---- Quick Function Test ------------------------------------------------------");

    // Quick Boolean Test
    LE_ASSERT( snprintf(pathBuffer, LE_CFG_STR_LEN_BYTES, "%s/quickFunctions/boolVal",
                    ConfigTreeTestRootDir)
                                    <= LE_CFG_STR_LEN_BYTES);
    bool value = le_cfg_QuickGetBool(pathBuffer, false);
    LE_DEBUG("Quick Boolean Test - Initial Value: %d", value);

    le_cfg_QuickSetBool(pathBuffer, true);
    value = le_cfg_QuickGetBool(pathBuffer, false);
    LE_DEBUG("Quick Boolean Test - Updated Value: %d", value);

    // Quick Float Test
    LE_ASSERT(snprintf(pathBuffer, LE_CFG_STR_LEN_BYTES, "%s/quickFunctions/floatVal",
                   ConfigTreeTestRootDir)
                                    <= LE_CFG_STR_LEN_BYTES);
    double floatValue = le_cfg_QuickGetFloat(pathBuffer, 0.0);
    LE_DEBUG("Quick Float Test - Initial Value: %f", floatValue);

    le_cfg_QuickSetFloat(pathBuffer, 123.45);
    floatValue = le_cfg_QuickGetFloat(pathBuffer, 0.0);
    LE_DEBUG("Quick Float Test - Updated Value: %f", floatValue);

    // Quick Binary Test
    LE_TEST_INFO("---- Quick Binary Test --------------------------------------------------------");
    LE_ASSERT(snprintf(pathBuffer, LE_CFG_STR_LEN_BYTES, "tafAdefUnitTest:/quickFunctions/binVal")
                                    <= LE_CFG_STR_LEN_BYTES);
    uint8_t writeBuf[LE_CFG_BINARY_LEN] = {0};
    uint8_t readBuf[LE_CFG_BINARY_LEN] = {0};

    for (int i = 0; i < sizeof(writeBuf); i++)
    {
        writeBuf[i] = (uint8_t)i;
    }

    size_t len = sizeof(readBuf);
    result = le_cfg_QuickGetBinary(pathBuffer, readBuf, &len, (uint8_t*)"no_good", 7);
    LE_FATAL_IF(result != LE_OK, "Quick Binary Test - Initial Get: %s", LE_RESULT_TXT(result));
    le_cfg_QuickSetBinary(pathBuffer, writeBuf, sizeof(writeBuf));
    len = sizeof(readBuf);
    result = le_cfg_QuickGetBinary(pathBuffer, readBuf, &len, (uint8_t*)"no_good", 7);
    LE_FATAL_IF(result != LE_OK, "Quick Binary Test - Updated Get: %s", LE_RESULT_TXT(result));
    LE_TEST(len == sizeof(writeBuf) && memcmp(writeBuf, readBuf, len) == 0);

    // Quick Integer Test
    LE_ASSERT(snprintf(pathBuffer, LE_CFG_STR_LEN_BYTES, "%s/quickFunctions/intVal",
                    ConfigTreeTestRootDir)
                                    <= LE_CFG_STR_LEN_BYTES);
    int intValue = le_cfg_QuickGetInt(pathBuffer, 0);
    LE_DEBUG("Quick Integer Test - Initial Value: %d", intValue);

    le_cfg_QuickSetInt(pathBuffer, 5678);
    intValue = le_cfg_QuickGetInt(pathBuffer, 0);
    LE_DEBUG("Quick Integer Test - Updated Value: %d", intValue);
}


static void TestAppUsername
(
)
{
    le_result_t result;

    char ConfigTreepathConfigTreeBuffer[LE_CFG_STR_LEN_BYTES] = "";

    LE_TEST_INFO("---- Username Test ------------------------------------------------------");
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn("system:/apps");

    if (le_cfg_GoToFirstChild(iterRef) == LE_NOT_FOUND)
    {
        LE_DEBUG("There are no installed apps.");
    }

    do
    {
        char appName[CNFG_LIMIT_MAX_APP_NAME_BYTES];
        LE_FATAL_IF(le_cfg_GetNodeName(iterRef, "", appName, sizeof(appName)) != LE_OK,
                    "Application name in config is too long.");

        // Construct the full config tree path
        LE_ASSERT(snprintf(ConfigTreepathConfigTreeBuffer, LE_CFG_STR_LEN_BYTES,
                         "system:/apps/%s/username", appName)
                 <= LE_CFG_STR_LEN_BYTES);
        LE_TEST_INFO("User name path found for the TelAF app: '%s'.",
                    ConfigTreepathConfigTreeBuffer);

        char ConfigTreestrConfigTreeBuffer[513] = "";
        result = le_cfg_QuickGetString(ConfigTreepathConfigTreeBuffer,
                                     ConfigTreestrConfigTreeBuffer, 513, "");
        LE_FATAL_IF(result != LE_OK,
                   "Test: system:/apps/%s/username - Test failure, result == %s.",
                   appName,
                   LE_RESULT_TXT(result));
        LE_DEBUG("<<< Get USERNAME STRING <%s> from path <%s>",
                ConfigTreestrConfigTreeBuffer, ConfigTreepathConfigTreeBuffer);

        if(strncmp(ConfigTreestrConfigTreeBuffer, "telaf", LE_CFG_STR_LEN_BYTES) != 0)
        {
            LE_TEST_INFO("Test: system:/apps/%s/username - Expected '%s' but got '%s' instead.",
                        appName,
                        "telaf",
                        ConfigTreestrConfigTreeBuffer);
        }
        else
        {
            LE_TEST_INFO("User name found for the TelAF app: '%s'--- OK ---: '%s'.",
                        ConfigTreepathConfigTreeBuffer, ConfigTreestrConfigTreeBuffer);
        }
    }
    while (le_cfg_GoToNextSibling(iterRef) == LE_OK);
    le_cfg_CancelTxn(iterRef);
}

static void TestAppCapability()
{
    char appConfigTreepathConfigTreeBuffer[LE_CFG_STR_LEN_BYTES] = "";
    LE_TEST_INFO("---- Capability Test ------------------------------------------------------");
    // Get an iterator to the capability list in the config
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn("system:/apps");

    if (le_cfg_GoToFirstChild(iterRef) == LE_NOT_FOUND)
    {
        LE_DEBUG("There are no installed apps.");
    }
    do
    {
        size_t i;
        char appName[CNFG_LIMIT_MAX_APP_NAME_BYTES];

        LE_FATAL_IF(le_cfg_GetNodeName(iterRef, "", appName, sizeof(appName)) != LE_OK,
                        "Application name in config is too long.");
        LE_TEST_INFO("Application name for the TelAF app: '%s'.", appName);
        LE_ASSERT(snprintf(appConfigTreepathConfigTreeBuffer, LE_CFG_STR_LEN_BYTES,
                    "%s%s%s%s",ConfigTreeTestAppRootDir,"/",appName,"/capability/")
                      <= LE_CFG_STR_LEN_BYTES);
        LE_TEST_INFO("Capability path found for the TelAF app: '%s'.",
                        appConfigTreepathConfigTreeBuffer);

        le_cfg_GoToNode(iterRef, appConfigTreepathConfigTreeBuffer);

        size_t numOfCapabilities = 0;
        if (le_cfg_GoToFirstChild(iterRef) != LE_OK)
        {
            LE_DEBUG("No capabilities setting for app '%s'.",appName);
            le_cfg_CancelTxn(iterRef);
            return;
        }

        for (i = 0; i < CNFG_LIMIT_MAX_NUM_CAPABILITIES; i++)
        {
            // Read the capability name from the configTree.
            char capabilityName[CNFG_LIMIT_MAX_APP_NAME_BYTES];

            if (le_cfg_GetNodeName(iterRef, "", capabilityName, sizeof(capabilityName)) != LE_OK)
            {
                LE_ERROR("Could not read capability for '%s'.",appName);
                le_cfg_CancelTxn(iterRef);
                return;
            }

            LE_TEST_INFO("Capability name: %s",capabilityName);

            // Go to the next capability.
            if (le_cfg_GoToNextSibling(iterRef) != LE_OK)
            {
                break;
            }
            else if (i >= CNFG_LIMIT_MAX_NUM_CAPABILITIES - 1)
            {
                LE_ERROR("Too many capabilities for app '%s'.",appName);
                le_cfg_CancelTxn(iterRef);
                return;
            }
        }
        numOfCapabilities = i + 1;

        LE_TEST_INFO("Capability list size for '%s': %" PRIuS "", appName, numOfCapabilities);

    }
    while (le_cfg_GoToNextSibling(iterRef) == LE_OK);
    le_cfg_CancelTxn(iterRef);
}


static void ConfigTreeStringOverwriteTest()
{
    // This custom test focuses on string overwriting with different strings.
    static char pathBuffer[LE_CFG_STR_LEN_BYTES] = "";

    LE_ASSERT(snprintf(pathBuffer, LE_CFG_STR_LEN_BYTES, "%s/custom/string/overwrite",
                    ConfigTreeTestRootDir) <= LE_CFG_STR_LEN_BYTES);

    // Create a write transaction and set an initial string.
    le_cfg_IteratorRef_t iterRefWrite = le_cfg_CreateWriteTxn(pathBuffer);
    le_cfg_SetString(iterRefWrite, pathBuffer, "Original String");
    le_cfg_CommitTxn(iterRefWrite);

    // Create a read transaction and verify the original string.
    le_cfg_IteratorRef_t iterRefRead = le_cfg_CreateReadTxn(pathBuffer);
    static char readBuffer[LE_CFG_STR_LEN_BYTES];
    le_result_t result = le_cfg_GetString(iterRefRead, pathBuffer, readBuffer,
                    LE_CFG_STR_LEN_BYTES, "");
    le_cfg_CancelTxn(iterRefRead);

    LE_FATAL_IF(result != LE_OK, "Test: Failed to read the original string");

    LE_FATAL_IF(strcmp(readBuffer, "Original String") != 0,
                    "Test: Unexpected original string value");

    // Overwrite the string with a new value.
    iterRefWrite = le_cfg_CreateWriteTxn(pathBuffer);
    le_cfg_SetString(iterRefWrite, pathBuffer, "New Value");
    le_cfg_CommitTxn(iterRefWrite);

    // Create a read transaction and verify the updated string.
    iterRefRead = le_cfg_CreateReadTxn(pathBuffer);
    result = le_cfg_GetString(iterRefRead, pathBuffer, readBuffer, LE_CFG_STR_LEN_BYTES, "");
    le_cfg_CancelTxn(iterRefRead);
    LE_FATAL_IF(result != LE_OK, "Test: Failed to read the updated string");
    LE_FATAL_IF(strcmp(readBuffer, "New Value") != 0, "Test: Unexpected updated string value");
}


static void ConfigTreeExistAndEmptyTest()
{
    // This custom test checks the existence of a Config Tree path and its emptiness.
    static char pathBuffer[LE_CFG_STR_LEN_BYTES] = "";

    LE_ASSERT(snprintf(pathBuffer, LE_CFG_STR_LEN_BYTES, "%s/custom/exist/empty",
                 ConfigTreeTestRootDir) <= LE_CFG_STR_LEN_BYTES);

    // Create a write transaction and commit it.
    le_cfg_IteratorRef_t iterWriteRef = le_cfg_CreateWriteTxn(pathBuffer);
    le_cfg_CommitTxn(iterWriteRef);

    // Create a read transaction and display the tree structure.
    le_cfg_IteratorRef_t iterReadRef = le_cfg_CreateReadTxn("");
    ConfigTreeDumpTree(iterReadRef, 0);
    le_cfg_CancelTxn(iterReadRef);
}

le_cfg_ChangeHandlerRef_t HandlerRef = NULL;
le_cfg_ChangeHandlerRef_t RootHandlerRef = NULL;
static le_sem_Ref_t CallbackSemaphore;
static le_sem_Ref_t RootCallbackSemaphore;

static void ConfigTreeIncrementTest()
{
    // This custom test increments a counter stored in the Config Tree.
    static char pathBuffer[LE_CFG_STR_LEN_BYTES] = "";
    LE_ASSERT(snprintf(pathBuffer, LE_CFG_STR_LEN_BYTES, "%s/custom/increment/count",
                ConfigTreeTestRootDir) <= LE_CFG_STR_LEN_BYTES);
    // Create a write transaction and increment the counter.
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(pathBuffer);

    int currentCount = le_cfg_GetInt(iterRef, "", 0);
    currentCount++;
    le_cfg_SetInt(iterRef, "", currentCount);

    le_cfg_CommitTxn(iterRef);
}

static le_sem_Ref_t SetEmptySemaphore;

COMPONENT_INIT
{
    char ConfigTreeTestRootDir[LE_CFG_STR_LEN_BYTES];
    char ConfigTreeTestAppRootDir[LE_CFG_STR_LEN_BYTES];
    const char* rootDir = "/configTest";
    const char* appRootDir = "/apps";
    memcpy(ConfigTreeTestRootDir, rootDir, strlen(rootDir) + 1);
    memcpy(ConfigTreeTestAppRootDir, appRootDir, strlen(appRootDir) + 1);
    SetEmptySemaphore = le_sem_Create("SetEmptySemaphore", 0);
    CallbackSemaphore = le_sem_Create("CallbackSemaphore", 0);
    RootCallbackSemaphore = le_sem_Create("RootCallbackSemaphore", 0);

    if (le_arg_NumArgs() == 1)
    {
        const char* name = le_arg_GetArg(0);

        if (name != NULL)
        {
            snprintf(ConfigTreeTestRootDir, LE_CFG_STR_LEN_BYTES, "/configTest_%s", name);
        }
    }

    LE_TEST_INFO("---------- Started testing in: %s -------------------------------------",
                   ConfigTreeTestRootDir);

    ConfigTreeQuickFunctionTest();
    TestAppUsername();
    TestAppCapability();
    ConfigTreeExistAndEmptyTest();
     // overwrite a large string with a small string and vice-versa
    ConfigTreeStringOverwriteTest();
    if (le_arg_NumArgs() == 1)
    {
        ConfigTreeIncrementTest();
    }

    LE_TEST_INFO("---------- All Tests Complete in: %s ----------------------------------",
                    ConfigTreeTestRootDir);
    exit(EXIT_SUCCESS);
}
