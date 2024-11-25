/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "legato.h"
#include "interfaces.h"

taf_mngdStorCfg_ConfigRef_t cRef;
#define LE_CFG_STR_LEN_BYTES   512

void PrintUsage(void)
{
    puts("\n"
         "-------- To do unit test automatically --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdCfgStorageUnitTest\n"
         "\n"
         "-------- To know Usage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdCfgStorageUnitTest -- help\n"
         "\n"
         "-------- To Update Version --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdCfgStorageUnitTest -- Update <Version>\n"
         "\n"
         "-------- Revert to orignal version of configuration file in case of failure during update --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdCfgStorageUnitTest -- Rollback\n"
         "\n"
         "-------- To cancel Update process --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdCfgStorageUnitTest -- Cancel\n"
         "\n"
         "-------- To Activate by replacing the orignal config file with the updated --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdCfgStorageUnitTest -- Activate\n"
         "\n"
         "-------- To Commit data to config storage after successful firmare update --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdCfgStorageUnitTest -- Commit\n"
         "\n"
         "-------- To get version of config storage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdCfgStorageUnitTest -- GetVersion\n"
         "\n"
         "-------- To get data type for the given groupName, nodeName --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdCfgStorageUnitTest -- GetType <groupName> <nodeName>\n"
         "\n"
         "-------- To get data value for the given groupName, nodeName --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdCfgStorageUnitTest -- GetData <groupName> <nodeName>\n"
         "\n"
         "-------- To get nodeInfo for the given groupName, nodeName --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdCfgStorageUnitTest -- GetNodeInfo <groupName> <nodeName>\n"
         "\n");
}

static le_result_t GetRef()
{
    cRef = taf_mngdStorCfg_GetRef();
    if(cRef == NULL) return LE_FAULT;
    return LE_OK;
}

static le_result_t Update(const char *version){
    le_result_t result;
    result = taf_mngdStorCfg_Update(cRef,version);
    if(result == LE_OK){
        printf("Storage Updated !! Please Activate and commit to use updated storage.\n");
    }
    else{
        printf("Failed to update Storage !! Please Cancel update process and retry.\n");
    }
    return result;
}

static le_result_t Cancel(){
    le_result_t result;
    result = taf_mngdStorCfg_Cancel(cRef);
    if(result == LE_OK) printf("Update Process cancels Successfuly!! you may retry.\n");
    else{
        printf("Cancels Operation failed!! Please try again.\n");
    }
    return result;
}

static le_result_t Activate(){
    le_result_t result;
    result = taf_mngdStorCfg_Activate(cRef);
    if(result == LE_OK) printf("Storage Activated!! Please Commit to use Activated Storage.\n");
    else{
        printf("Failed to Activate Storage !! you may Rollback and try activating again.\n");
    }
    return result;
}

static le_result_t Rollback(){
    le_result_t result;
    result = taf_mngdStorCfg_Rollback(cRef);
    if(result == LE_OK) printf("Storage Rollbacked!! Please commit to use rollbacked storage.\n");
    else{
        printf("Failed to Roolback Storage!! Try again.\n");
    }
    return result;
}

static le_result_t Commit(){
    le_result_t result;
    result = taf_mngdStorCfg_Commit(cRef);
    if(result == LE_OK) printf("Storage Commited!! Ready to use.\n");
    else{
        printf("Failed to Roolback Storage!! Try again.\n");
    }
    return result;

}

static le_result_t GetVersion(){
    le_result_t result;
    uint32_t MajorVersionPtr=0;
    uint32_t MinorVersionPtr=0;
    uint32_t PatchVersionPtr=0;
    result = taf_mngdStorCfg_GetVersion(cRef,&MajorVersionPtr,&MinorVersionPtr,&PatchVersionPtr);
    if(result == LE_OK){
        printf("Vesrion is %d.%d.%d\n",MajorVersionPtr,MinorVersionPtr,PatchVersionPtr);
    }
    return result;
}

static le_result_t GetType(const char *groupName , const char *nodeName){
    le_result_t result;
    taf_mngdStorCfg_NodeType_t typePtr;
    result = taf_mngdStorCfg_GetType(cRef, groupName, nodeName, &typePtr);
    if(result == LE_OK){
        printf("Node type is %d\n",typePtr);
    }
    return result;
}
static le_result_t GetValue(const char *groupName , const char *nodeName){
    le_result_t result;
    char nodeValuePtr[LE_CFG_STR_LEN_BYTES];
    taf_mngdStorCfg_NodeType_t typePtr;
    result =
      taf_mngdStorCfg_GetValue(cRef,groupName,nodeName, &typePtr,nodeValuePtr,sizeof(nodeValuePtr));
    LE_TEST_OK(result == LE_OK, "Test taf_mngdStorCfg_GetValue");
    if(result == LE_OK){
        printf("Node type is %d\n",typePtr);
        printf("Node data is %s\n",nodeValuePtr);
    }
    return result;
}
static le_result_t GetData(const char *groupName , const char *nodeName){
    le_result_t result;
    taf_mngdStorCfg_NodeType_t typePtr;
    result = taf_mngdStorCfg_GetType(cRef, groupName, nodeName, &typePtr);
    if(result == LE_OK){
        if(typePtr == TAF_MNGDSTORCFG_TYPE_STRING){
            char nodeValue[LE_CFG_STR_LEN_BYTES];
            result = taf_mngdStorCfg_GetString(cRef, groupName, nodeName,
                nodeValue, sizeof(nodeValue));
            if(result == LE_OK){
                printf("Value for node %s is %s\n",nodeName,nodeValue);
            }
        }
        else if(typePtr == TAF_MNGDSTORCFG_TYPE_BOOL){
            int32_t nodeValuePtr = 0;
            result = taf_mngdStorCfg_GetBool(cRef, groupName, nodeName, &nodeValuePtr);
            if(result == LE_OK){
                printf("Value for node %s is %d\n",nodeName,nodeValuePtr);
            }
        }
        else if(typePtr == TAF_MNGDSTORCFG_TYPE_INT){
            int32_t nodeValuePtr = 0;
            result = taf_mngdStorCfg_GetInt(cRef, groupName, nodeName, &nodeValuePtr);
            if(result == LE_OK){
                printf("Value for node %s is %d\n",nodeName,nodeValuePtr);
            }
        }
        else if(typePtr == TAF_MNGDSTORCFG_TYPE_FLOAT){
            double nodeValuePtr = 0;
            result = taf_mngdStorCfg_GetFloat(cRef, groupName, nodeName, &nodeValuePtr);
            if(result == LE_OK){
                printf("Value for node %s is %f\n",nodeName,nodeValuePtr);
            }
        }
        else{
            printf("Type not found!!");
            result = LE_NOT_FOUND;
        }
    }else{
        printf("Notable to fetch type!!");
    }
return result;
}

static void Test_cfg_UpdateProcess(){
    le_result_t result;
    cRef = taf_mngdStorCfg_GetRef();
    LE_TEST_ASSERT(cRef != NULL, "Test taf_mngdStorCfg_GetRef");
    result = taf_mngdStorCfg_Update(cRef,"1.0.0");
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Update");
    if(result == LE_OK){
        result = taf_mngdStorCfg_Activate(cRef);
        LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Activate");
        if(result != LE_OK){
            result = taf_mngdStorCfg_Rollback(cRef);
            LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Rollback");
        }
        result = taf_mngdStorCfg_Commit(cRef);
        LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Commit");
    }
    else{
        result = taf_mngdStorCfg_Cancel(cRef);
        LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Cancel");
    }
    result = taf_mngdStorCfg_ReleaseRef(cRef);
    LE_TEST_OK(result == LE_OK, "Test taf_mngdStorCfg_ReleaseRef");
}

static void Test_cfg_GetDataProcess(){
    le_result_t result;
    char nodeValue[LE_CFG_STR_LEN_BYTES];
    taf_mngdStorCfg_NodeType_t typePtr;
    cRef = taf_mngdStorCfg_GetRef();
    LE_TEST_ASSERT(cRef != NULL, "Test taf_mngdStorCfg_GetRef");
    result = taf_mngdStorCfg_GetType(cRef, "config1", "aBoolVal", &typePtr);
    LE_TEST_ASSERT(result == LE_OK,"Test taf_mngdStorCfg_GetType");
    result = taf_mngdStorCfg_GetString(cRef, "config1", "aStringVal", nodeValue,sizeof(nodeValue));
    LE_TEST_ASSERT(result == LE_OK,"Test taf_mngdStorCfg_GetString");
    int32_t IntValuePtr = 0;
    result = taf_mngdStorCfg_GetInt(cRef,"config1","aIntVal", &IntValuePtr);
    LE_TEST_ASSERT(result == LE_OK,"Test taf_mngdStorCfg_GetInt");
    int32_t boolValuePtr = 0;
    result = taf_mngdStorCfg_GetBool(cRef, "config1", "aBoolVal", &boolValuePtr);
    LE_TEST_ASSERT(result == LE_OK,"Test taf_mngdStorCfg_GetBool");
    double doubleValuePtr = 0;
    result = taf_mngdStorCfg_GetFloat(cRef, "config1", "aFloatVal", &doubleValuePtr);
    LE_TEST_ASSERT(result == LE_OK,"Test taf_mngdStorCfg_GetFloat");
    char nodeValuePtr[LE_CFG_STR_LEN_BYTES];
    result =
       taf_mngdStorCfg_GetValue(cRef, "config1","aStringVal", &typePtr,nodeValuePtr,sizeof(nodeValuePtr));
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_GetValue");
}

static le_result_t ReleaseRef(){
    le_result_t result;
    result = taf_mngdStorCfg_ReleaseRef(cRef);
    return result;
}

static void CheckNumArgs(size_t NumArgs, size_t ExpectedNumArgs)
{
    if (NumArgs!=ExpectedNumArgs)
    {
        PrintUsage();
        LE_TEST_FATAL("Invalid number of arguments");
    }
}


COMPONENT_INIT
{
    LE_TEST_INFO("Config Storage Unit Test");
    le_result_t result = LE_FAULT;
    result = GetRef();
    LE_TEST_ASSERT(result == LE_OK,"Got Config Storage Reference");
    size_t numArgs = le_arg_NumArgs();
    if (numArgs == 0) {
        Test_cfg_UpdateProcess();
        Test_cfg_GetDataProcess();
        result  =  ReleaseRef();
        LE_TEST_ASSERT(result == LE_OK,"Reference Released");
        LE_TEST_EXIT;
    }
    const char *testType = le_arg_GetArg(0);
    if (strncmp(testType, "Update", strlen(testType)) == 0){
        LE_TEST_INFO("Update Process Test");
        CheckNumArgs(numArgs,2);
        const char *version = le_arg_GetArg(1);
        result = Update(version);
        LE_TEST_OK(result == LE_OK,"Test taf_mngdStorCfg_Update()");

    }
    else if(strncmp(testType, "Rollback", strlen(testType)) == 0){
        LE_TEST_INFO("Rollback Process Test");
        CheckNumArgs(numArgs,1);
        result = Rollback();
        LE_TEST_OK(result == LE_OK,"Test taf_mngdStorCfg_Rollback()");
    }
    else if(strncmp(testType, "Commit", strlen(testType)) == 0){
        LE_TEST_INFO("Commit Process Test");
        CheckNumArgs(numArgs,1);
        result = Commit();
        LE_TEST_OK(result == LE_OK,"Test taf_mngdStorCfg_Commit()");
    }
    else if(strncmp(testType, "Cancel", strlen(testType)) == 0){
        LE_TEST_INFO("Cancel Process Test");
        CheckNumArgs(numArgs,1);
        result = Cancel();
        LE_TEST_OK(result == LE_OK,"Test taf_mngdStorCfg_Cancel()");
    }
    else if(strncmp(testType, "Activate", strlen(testType)) == 0){
        LE_TEST_INFO("Activate Process Test");
        CheckNumArgs(numArgs,1);
        result = Activate();
        LE_TEST_OK(result == LE_OK,"Test taf_mngdStorCfg_Activate()");
    }
    else if(strncmp(testType, "GetData", strlen(testType)) == 0){
        LE_TEST_INFO("GetData Process Test");
        CheckNumArgs(numArgs,3);
        const char *groupName = le_arg_GetArg(1);
        const char *nodeName = le_arg_GetArg(2);
        result = GetData(groupName,nodeName);
        LE_TEST_OK(result == LE_OK,"Test taf_mngdStorCfg_GetData()");
    }
    else if(strncmp(testType, "GetType", strlen(testType)) == 0){
        LE_TEST_INFO("GetType Process Test");
        CheckNumArgs(numArgs,3);
        const char *groupName = le_arg_GetArg(1);
        const char *nodeName = le_arg_GetArg(2);
        result = GetType(groupName,nodeName);
        LE_TEST_OK(result == LE_OK,"Test taf_mngdStorCfg_GetType()");
    }
    else if(strncmp(testType, "GetVersion", strlen(testType)) == 0){
        LE_TEST_INFO("GetVersion Process Test");
        CheckNumArgs(numArgs,1);
        result = GetVersion();
        LE_TEST_OK(result == LE_OK,"Test taf_mngdStorCfg_GetValue()");
    }
    else if(strncmp(testType, "GetNodeInfo", strlen(testType)) == 0){
        LE_TEST_INFO("GetValuE Process Test");
        CheckNumArgs(numArgs,3);
        const char *groupName = le_arg_GetArg(1);
        const char *nodeName = le_arg_GetArg(2);
        result = GetValue(groupName,nodeName);
        LE_TEST_OK(result == LE_OK,"Test taf_mngdStorCfg_GetValue()");
    }
    else if(strncmp(testType, "help", strlen(testType)) == 0){
        PrintUsage();
    }
    else{
        PrintUsage();
        LE_TEST_FATAL("Invalid test type %s", testType);
    }
    result  =  ReleaseRef();
    LE_TEST_ASSERT(result == LE_OK,"Reference Released");
    LE_TEST_EXIT;
}