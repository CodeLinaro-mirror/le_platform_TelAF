/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"
#include <time.h>
#include <pwd.h>

#define CFG_READ_WRITE_PATH "/configTreeTimeAcess"
#define TREE_NAME_MAX 65
#define CNFG_LIMIT_MAX_APP_NAME_BYTES 128

//--------------------------------------------------------------------------------------------------
/**
 * Function to read integer value from config tree.
 */
//--------------------------------------------------------------------------------------------------
static void ReadConfigIntNodeValue
(
    const char* treePath,  ///< Tree path + child node
    int * intValue         ///< value being read
)
{
    LE_TEST_INFO("GetConfigIntNodeValue");
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn(treePath);
    if (iteratorRef == NULL)
    {
        LE_TEST_INFO("Failed to create read transaction for path: %s", treePath);
        return;
    }
    *intValue = le_cfg_GetInt(iteratorRef,"int",0);
    LE_TEST_INFO("The read value is: %d", *intValue);
    le_cfg_CancelTxn(iteratorRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to write integer value into the config tree.
 */
//--------------------------------------------------------------------------------------------------
static void WriteConfigIntNodeValue
(
    const char* treePath,      ///< Tree path + child node
    int32_t     value          ///< value to be written
)
{
    LE_TEST_INFO("SetConfigIntNodeValue");
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn(treePath);
    if (iteratorRef == NULL)
    {
        LE_TEST_INFO("Failed to create write transaction");
        return;
    }
    le_cfg_SetInt(iteratorRef, "int", value);
    le_cfg_CommitTxn(iteratorRef);
    LE_TEST_INFO("Set value is: %d",value);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to calculate KPI for config tree read/write operations.
 */
//--------------------------------------------------------------------------------------------------
static void ReadWriteIntegerConfigTreeKPI
(
)
{
    int32_t valueSet = 2025;
    int32_t valueGet2 = 0;
    struct timespec startWrite, endWrite;
    struct timespec startRead, endRead;
    long long secondsWrite, nanosecondsWrite,totalNanosecondsWrite;
    double totalSecondsWrite,totalMicrosecondsWrite,totalMillisecondsWrite;
    long long secondsRead, nanosecondsRead,totalNanosecondsRead;
    double totalSecondsRead,totalMicrosecondsRead,totalMillisecondsRead;

    LE_TEST_INFO("ReadWriteIntegerConfigTreeKPI");

    //Get start time= Write
    if (clock_gettime(CLOCK_MONOTONIC, &startWrite) == -1)
    {
        LE_TEST_INFO("Failed to get start time: %s", strerror(errno));
        return;
    }

    // Write the value for int.
    WriteConfigIntNodeValue(CFG_READ_WRITE_PATH,valueSet);

    // end time -Write
    if (clock_gettime(CLOCK_MONOTONIC, &endWrite) == -1)
    {
        LE_TEST_INFO("Failed to get end time: %s", strerror(errno));
        return;
    }

    //Total elapsed time -Write
    secondsWrite = (long long) endWrite.tv_sec - startWrite.tv_sec;
    nanosecondsWrite = (long long) endWrite.tv_nsec - startWrite.tv_nsec;
    totalNanosecondsWrite = secondsWrite * 1000000000LL + nanosecondsWrite;
    totalMicrosecondsWrite = totalNanosecondsWrite / 1e3;
    totalMillisecondsWrite = totalNanosecondsWrite / 1e6;
    totalSecondsWrite =((double) totalNanosecondsWrite)/1e9;

    //Print Elapsed time in nano second,micro seconds and milli seconds
    LE_TEST_INFO("====================================================");
    LE_TEST_INFO("Write access KPI is : %lld NANO SECONDS", totalNanosecondsWrite);
    LE_TEST_INFO("Write access KPI %.3f MICRO SECONDS", totalMicrosecondsWrite);
    LE_TEST_INFO("Write access KPI is: %.3f MILLI SECONDS", totalMillisecondsWrite);
    LE_TEST_INFO("Write access KPI is: %lf SECONDS", totalSecondsWrite);
    LE_TEST_INFO("====================================================");


    //Get start time - Read
    if (clock_gettime(CLOCK_MONOTONIC, &startRead) == -1)
    {
        LE_TEST_INFO("Failed to get start time: %s", strerror(errno));
        return;
    }

    // Read the value of int.
    ReadConfigIntNodeValue(CFG_READ_WRITE_PATH,&valueGet2);
    LE_TEST_OK(valueSet == valueGet2,"***ReadWriteIntegerConfigTreeKPI***-LE_OK");

    // end time - Read
    if (clock_gettime(CLOCK_MONOTONIC, &endRead) == -1)
    {
        LE_TEST_INFO("Failed to get end time: %s", strerror(errno));
        return;
    }

    // Verify the read value matches what was written
    if (valueGet2 != valueSet)
    {
        LE_TEST_INFO("Read value %d doesn't match written value %d", valueGet2, valueSet);
    }
    else
    {
        LE_TEST_INFO("Successfully verified read value matches written value: %d", valueGet2);
    }

    //Total elapsed time -Read
    secondsRead = (long long) endRead.tv_sec - startRead.tv_sec;
    nanosecondsRead = (long long) endRead.tv_nsec - startRead.tv_nsec;
    totalNanosecondsRead = secondsRead * 1000000000LL + nanosecondsRead;
    totalMicrosecondsRead = totalNanosecondsRead / 1e3;
    totalMillisecondsRead = totalNanosecondsRead / 1e6;
    totalSecondsRead =((double) totalNanosecondsRead)/1e9;

    //Print Elapsed time in nano second,micro seconds and milli seconds
    LE_TEST_INFO("====================================================");
    LE_TEST_INFO("Read access KPI is : %lld NANO SECONDS", totalNanosecondsRead);
    LE_TEST_INFO("Read access KPI %.3f MICRO SECONDS", totalMicrosecondsRead);
    LE_TEST_INFO("Read access KPI is: %.3f MILLI SECONDS", totalMillisecondsRead);
    LE_TEST_INFO("Read access KPI is: %lf SECONDS", totalSecondsRead);
    LE_TEST_INFO("====================================================");
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to import JSON file for read/write operations.
 */
//--------------------------------------------------------------------------------------------------
static void TestImportReadWrite(const char* treePath)
{
    int32_t valueSet = 2025;
    int32_t value;

   //Writing into the imported config tree path
    LE_TEST_INFO("TestImportReadWrite");
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn(treePath);
    if (iteratorRef == NULL)
    {
        LE_TEST_INFO("Failed to create write transaction");
        return;
    }
    le_cfg_SetInt(iteratorRef, "anIntVal", valueSet);
    le_cfg_CommitTxn(iteratorRef);
    LE_TEST_INFO("Set valueSet is: %d",valueSet);

   //Reading from the imported config tree path
    iteratorRef = le_cfg_CreateReadTxn(treePath);
    if (iteratorRef == NULL)
    {
        LE_TEST_INFO("Failed to create read transaction for path: %s", treePath);
        return;
    }
    value = le_cfg_GetInt(iteratorRef,"anIntVal",0);
    LE_TEST_INFO("The read value is: %d", value);
    LE_TEST_OK(value == valueSet,"***TestImportReadWrite***-LE_OK");
    le_cfg_CancelTxn(iteratorRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to clear config tree entries.
 */
//--------------------------------------------------------------------------------------------------
static void ClearTree(const char* testRoot)
{
    LE_TEST_INFO("---- Clearing Out Current Tree -----------------------------------------------------");
    char cfgRootDir[LE_CFG_STR_LEN_BYTES] = "";
    snprintf(cfgRootDir,TREE_NAME_MAX, "configKPITest-%s", testRoot);
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(cfgRootDir);
    LE_FATAL_IF(iterRef == NULL, "Test: %s - Could not create iterator.", cfgRootDir);

    le_cfg_DeleteNode(iterRef, "");
    le_cfg_CommitTxn(iterRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to import JSON file for appdefault/root/telaf usernames.
 */
//--------------------------------------------------------------------------------------------------
static void TestImportJSON(const char* testRoot, bool isSuccess)
{
    LE_TEST_INFO("---- Import Export Function Test: %s -------------------------------------", testRoot);
    char primitiveRootDir[LE_CFG_STR_LEN_BYTES] = "";
    snprintf(primitiveRootDir, TREE_NAME_MAX, "configKPITest-%s", testRoot);
    static char pathBuffer[LE_CFG_STR_LEN_BYTES] = "";
    LE_ASSERT(snprintf(pathBuffer, LE_CFG_STR_LEN_BYTES, "/%s", primitiveRootDir)
              <= LE_CFG_STR_LEN_BYTES);
    char filePath[PATH_MAX] = "";
    uid_t myUid = getuid();
    char *userNamePath = NULL;
    struct passwd *userName = getpwuid(myUid);

    if(userName)
    {
        LE_TEST_INFO("Username is : %s",userName->pw_name);
        userNamePath = userName->pw_name;
    }
    else
    {
        LE_TEST_INFO("Username is NULL");
    }
    if((strncmp(userNamePath, "root", LE_CFG_STR_LEN_BYTES)) == 0)
    {
        snprintf(filePath,
        150, "/legato/systems/current/appsWriteable/ConfigTreeAccessKPIRootTest/data/configKPITest-%s.json", testRoot);
        LE_TEST_INFO("root username path is selected");
    }
    else if((strncmp(userNamePath, "telaf", LE_CFG_STR_LEN_BYTES)) == 0)
    {
        snprintf(filePath,
        150, "/legato/systems/current/appsWriteable/ConfigTreeAccessKPITelafTest/data/configKPITest-%s.json", testRoot);
        LE_TEST_INFO("telaf username path is selected");
    }
    else
    {
        LE_TEST_INFO("Invalid Username");
    }

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn("");

    LE_TEST_INFO("IMPORT TREE: %s", pathBuffer);
    LE_TEST_INFO("Import: %s", filePath);
    if(isSuccess){
        LE_ASSERT(le_cfgAdmin_ImportTree(iterRef, filePath, pathBuffer) == LE_OK);
        le_cfg_CommitTxn(iterRef);
    }
    else {
        LE_TEST(le_cfgAdmin_ImportTree(iterRef, filePath, pathBuffer) != LE_OK);
        le_cfg_CancelTxn(iterRef);
    }
    TestImportReadWrite(filePath);
    unlink(filePath);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to calculate KPI for importing JSON file.
 */
//--------------------------------------------------------------------------------------------------
static void TestImportKPI()
{
    LE_TEST_INFO("TestImportKPI");
    struct timespec startImport, endImport;
    long long secondsImport, nanosecondsImport,totalNanosecondsImport;
    double totalSecondsImport,totalMicrosecondsImport,totalMillisecondsImport;

    //Get start time - Import
    if (clock_gettime(CLOCK_MONOTONIC, &startImport) == -1)
    {
        LE_TEST_INFO("Failed to get start time: %s", strerror(errno));
        return;
    }

    ClearTree("100");
    TestImportJSON("100",true);

    // end time - Import
    if (clock_gettime(CLOCK_MONOTONIC, &endImport) == -1)
    {
        LE_TEST_INFO("Failed to get end time: %s", strerror(errno));
        return;
    }

    //Total elapsed time -Import
    secondsImport = (long long) endImport.tv_sec - startImport.tv_sec;
    nanosecondsImport = (long long) endImport.tv_nsec - startImport.tv_nsec;
    totalNanosecondsImport = secondsImport * 1000000000LL + nanosecondsImport;
    totalMicrosecondsImport = totalNanosecondsImport / 1e3;
    totalMillisecondsImport = totalNanosecondsImport / 1e6;
    totalSecondsImport =((double) totalNanosecondsImport)/1e9;

    //Print Elapsed time in nano second,micro seconds and milli seconds
    LE_TEST_INFO("====================================================");
    LE_TEST_INFO("Import config KPI is : %lld NANO SECONDS", totalNanosecondsImport);
    LE_TEST_INFO("Import config KPI %.3f MICRO SECONDS", totalMicrosecondsImport);
    LE_TEST_INFO("Import config KPI is: %.3f MILLI SECONDS", totalMillisecondsImport);
    LE_TEST_INFO("Import config KPI is: %lf SECONDS", totalSecondsImport);
    LE_TEST_INFO("====================================================");
}

COMPONENT_INIT
{
    LE_TEST_INFO("---------- Started testing in:---");
    ReadWriteIntegerConfigTreeKPI();
    TestImportKPI();
    LE_TEST_INFO("---------- Test Completed in: ----");
    exit(EXIT_SUCCESS);
}
