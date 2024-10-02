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
#include <string.h>

#define OP_HELP "help"
#define OP_CREATE "create"
#define OP_READ    "read"
#define OP_WRITE   "write"
#define OP_DELETE  "delete"

#define TEST_DATA_LABEL "testdata"
#define MSS_SECURE_STORAGE_SIZE 8192
#define LE_CFG_STR_LEN_BYTES   512

uint8_t TEST_TEXT_PATTERN[] = {'a','b','c','d','e','f','g'};
uint8_t TEST_NUM_PATTERN[] = {0,1,2,3,4,5,6,7,8,9};

taf_mngdStorSecData_DataRef_t dataRef;
taf_mngdStorCfg_ConfigRef_t cRef;

__attribute__((unused)) static void PrintUsage()
{
    puts("\n"
        "-------- To do unit test automatically --------\n"
        "app start tafMngdPMIntTest\n"
        "\n"
        "-------- To know Usage --------\n"
        "app runProc tafMngdStorageUnitTest tafMngdStorageUnitTest -- help \n"
        "\n"
        "-------- To create data label --------\n"
        "app runProc tafMngdStorageUnitTest tafMngdStorageUnitTest -- create <label>\n"
        "\n"
        "-------- To write data to label --------\n"
        "app runProc tafMngdStorageUnitTest tafMngdStorageUnitTest -- write <label> <data>\n"
        "\n"
        "-------- To read data from label --------\n"
        "app runProc tafMngdStorageUnitTest tafMngdStorageUnitTest -- read <label>\n"
        "\n"
        "-------- To delete data label --------\n"
        "app runProc tafMngdStorageUnitTest tafMngdStorageUnitTest -- delete <label>\n");
}

__attribute__((unused)) static void Test_Secure_Storage_Management()
{
    le_result_t res;

    uint32_t freeSize = 0;

    res = taf_mngdStorSecData_GetFreeSize(&freeSize);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_GetFreeSize");

    LE_INFO("freeSize = %u", freeSize);

    LE_TEST_ASSERT(freeSize == MSS_SECURE_STORAGE_SIZE, "Test taf_mngdStorSecData_GetFreeSize");

    uint32_t usedSize = 0;

    res = taf_mngdStorSecData_GetUsedSize(&usedSize);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_GetUsedSize");

    LE_TEST_ASSERT(usedSize == 0, "Test taf_mngdStorSecData_GetUsedSize");
}


__attribute__((unused)) static void Test_Secure_Data_Management()
{
    le_result_t res;

    dataRef = taf_mngdStorSecData_CreateData(TEST_DATA_LABEL);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdStorSecData_CreateData");

    taf_mngdStorSecData_DataRef_t checkedDataRef = taf_mngdStorSecData_GetDataRef(TEST_DATA_LABEL);

    LE_TEST_ASSERT(checkedDataRef != NULL, "Test taf_mngdStorSecData_GetDataRef");

    LE_TEST_ASSERT(checkedDataRef == dataRef, "Test taf_mngdStorSecData_GetDataRef");

    uint32_t dataSize = 0;

    res = taf_mngdStorSecData_GetDataSize(dataRef, &dataSize);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_GetDataSize");

    LE_TEST_ASSERT(dataSize == 0, "Test taf_mngdStorSecData_GetDataSize");

    res = taf_mngdStorSecData_DeleteData(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_DeleteData");
}

__attribute__((unused)) static uint8_t* GenerateTestData(uint32_t size)
{
    uint8_t* data = malloc(size);
    LE_TEST_ASSERT(data != NULL, "Allocate test data");
    for(size_t i = 0; i < size; ++i)
    {
        data[i] = 'A' + (i % 26); // Use cycling chars for filling data
    }
    return data;
}

__attribute__((unused)) static void Test_Secure_Data_Write()
{
    le_result_t res;

    dataRef = taf_mngdStorSecData_CreateData(TEST_DATA_LABEL);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdStorSecData_CreateData");

    res = taf_mngdStorSecData_WriteDataStart(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_WriteDataStart");

    res = taf_mngdStorSecData_WriteDataChunk(dataRef,
                                                TEST_TEXT_PATTERN,
                                                sizeof(TEST_TEXT_PATTERN));

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_WriteDataChunk");

    res = taf_mngdStorSecData_WriteDataChunk(dataRef,
                                                TEST_NUM_PATTERN,
                                                sizeof(TEST_NUM_PATTERN));

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_WriteDataChunk");

    res = taf_mngdStorSecData_WriteDataEnd(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_WriteDataEnd");

    uint32_t dataSize = 0;

    res = taf_mngdStorSecData_GetDataSize(dataRef, &dataSize);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_GetDataSize");

    LE_TEST_ASSERT(dataSize > 0, "Test taf_mngdStorSecData_GetDataSize");
}

__attribute__((unused)) static void Test_Secure_Data_Read()
{
    le_result_t res;

    uint8_t readBuf[TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE] = {0};
    size_t readLen = sizeof(readBuf);

    res = taf_mngdStorSecData_ReadDataFirstChunk(dataRef, readBuf, &readLen);

    LE_INFO("readLen = %" PRIuS, readLen);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_ReadDataFirstChunk");

    LE_TEST_ASSERT(readLen > 0, "Test taf_mngdStorSecData_ReadDataFirstChunk");

    LE_TEST_ASSERT(readLen == (sizeof(TEST_TEXT_PATTERN) + sizeof(TEST_NUM_PATTERN)),
                    "Test taf_mngdStorSecData_ReadDataFirstChunk");

    LE_TEST_ASSERT(memcmp(readBuf, TEST_TEXT_PATTERN, sizeof(TEST_TEXT_PATTERN)) == 0,
                    "Test taf_mngdStorSecData_ReadDataFirstChunk");

    LE_TEST_ASSERT(memcmp(readBuf + sizeof(TEST_TEXT_PATTERN),
                            TEST_NUM_PATTERN, sizeof(TEST_NUM_PATTERN)) == 0,
                    "Test taf_mngdStorSecData_ReadDataFirstChunk");
}

__attribute__((unused)) static void Test_Secure_Data_Read_Write_Chunks()
{
    le_result_t res;

    res = taf_mngdStorSecData_WriteDataStart(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_WriteDataStart");

    uint8_t* genData = GenerateTestData(TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE * 2);

    res = taf_mngdStorSecData_WriteDataChunk(dataRef,
                                            genData,
                                            TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_WriteDataChunk");

    res = taf_mngdStorSecData_WriteDataChunk(dataRef,
                                            genData + TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE,
                                            TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_WriteDataChunk");

    res = taf_mngdStorSecData_WriteDataEnd(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_WriteDataEnd");

    uint8_t readBuf[TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE] = {0};
    size_t readLen = sizeof(readBuf);
    size_t totalReadLen = 0;

    res = taf_mngdStorSecData_ReadDataFirstChunk(dataRef, readBuf, &readLen);

    LE_INFO("readLen = %" PRIuS, readLen);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_ReadDataFirstChunk");

    LE_TEST_ASSERT(readLen > 0, "Test taf_mngdStorSecData_ReadDataFirstChunk");

    LE_TEST_ASSERT(memcmp(readBuf, genData, readLen) == 0,
                    "Test taf_mngdStorSecData_ReadDataFirstChunk");

    totalReadLen = readLen;

    while(readLen > 0)
    {
        readLen = sizeof(readBuf);

        taf_mngdStorSecData_ReadDataNextChunk(dataRef, readBuf, &readLen);

        LE_INFO("readLen = %" PRIuS, readLen);

        LE_TEST_ASSERT(memcmp(readBuf, genData + totalReadLen, readLen) == 0,
                        "Test taf_mngdStorSecData_ReadDataNextChunk");

        totalReadLen += readLen;
    }

    free(genData);
}

__attribute__((unused)) static void Test_Secure_Delete_Storage()
{
    le_result_t res;

    res = taf_mngdStorSecData_DeleteData(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_DeleteData");
}

__attribute__((unused)) static void Test_Op_create(const char* label)
{
    dataRef = taf_mngdStorSecData_CreateData(label);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdStorSecData_CreateData");

    printf("Create data label: %s succussfully\n", label);
    fflush(stdout);
}

__attribute__((unused)) static void Test_Op_Write
(
    const char* label, const char* data
)
{
    le_result_t res;

    dataRef = taf_mngdStorSecData_GetDataRef(label);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdStorSecData_GetDataRef");

    res = taf_mngdStorSecData_WriteDataStart(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_WriteDataStart");

    res = taf_mngdStorSecData_WriteDataChunk(dataRef,
                                            (uint8_t*)data,
                                            strlen(data));

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_WriteDataChunk");

    res = taf_mngdStorSecData_WriteDataEnd(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_WriteDataEnd");

    printf("Write data: [%s] succussfully\n", data);
    fflush(stdout);
}

__attribute__((unused)) static void Test_Op_Read(const char* label)
{
    le_result_t res;

    dataRef = taf_mngdStorSecData_GetDataRef(label);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdStorSecData_GetDataRef");

    uint8_t readBuf[TAF_MNGDSTORSECDATA_MAX_DATA_CHUNK_SIZE] = {0};
    size_t readLen = sizeof(readBuf);

    res = taf_mngdStorSecData_ReadDataFirstChunk(dataRef, readBuf, &readLen);

    LE_INFO("readLen = %" PRIuS, readLen);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_ReadDataFirstChunk");

    printf("Read data: [%s] succussfully\n", (char*)readBuf);
    fflush(stdout);
}

__attribute__((unused)) static void Test_Op_Delete(const char* label)
{
    le_result_t res;

    dataRef = taf_mngdStorSecData_GetDataRef(label);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdStorSecData_GetDataRef");

    res = taf_mngdStorSecData_DeleteData(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_DeleteData");

    printf("Delete data label: %s succussfully\n", label);
    fflush(stdout);
}

__attribute__((unused)) static void Test_cfg_GetRef(){
    cRef = taf_mngdStorCfg_GetRef();
    LE_TEST_ASSERT(cRef != NULL, "Test taf_mngdStorCfg_GetRef");
}

__attribute__((unused)) static void Test_cfg_UpdateProcess(){
    le_result_t result;
    result = taf_mngdStorCfg_Update(cRef,"0.0.0");
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_UpdateFile");
    if(result == LE_OK){
        result = taf_mngdStorCfg_Activate(cRef);
        LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Sync");
        if(result == LE_OK){
            result = taf_mngdStorCfg_Commit(cRef);
            LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Commit");
        }
        else{
            result = taf_mngdStorCfg_Rollback(cRef);
            LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Rollback");
        }
    }
    else{
        result = taf_mngdStorCfg_Cancel(cRef);
        LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Cancel");
    }
}

__attribute__((unused)) static void Test_cfg_RollbackProcess(){
    le_result_t result;
    result = taf_mngdStorCfg_Update(cRef,"0.0.0");
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_UpdateFile");
    if(result == LE_OK){
        result = taf_mngdStorCfg_Activate(cRef);
        LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Sync");
        if(result == LE_OK){
            result = taf_mngdStorCfg_Rollback(cRef);
            LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Rollback");
            if(result == LE_OK){
                result = taf_mngdStorCfg_Commit(cRef);
                LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Commit");
            }
        }
    }
    else{
        result = taf_mngdStorCfg_Cancel(cRef);
        LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Cancel");
    }
}

__attribute__((unused)) static void Test_cfg_CancelProcess(){
    le_result_t result;
    result = taf_mngdStorCfg_Update(cRef,"0.0.0");
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Update");
    result = taf_mngdStorCfg_Cancel(cRef);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_Cancel");
}

__attribute__((unused)) static void Test_cfg_GetVersion(){
    le_result_t result;
    uint32_t MajorVersionPtr=0;
    uint32_t MinorVersionPtr=0;
    uint32_t PatchVersionPtr=0;
    result = taf_mngdStorCfg_GetVersion(cRef,&MajorVersionPtr,&MinorVersionPtr,&PatchVersionPtr);
    LE_TEST_OK(result == LE_OK, "Test taf_mngdStorCfg_GetVersion");
    LE_INFO("Vesrion is %d.%d.%d",MajorVersionPtr,MinorVersionPtr,PatchVersionPtr);
}

__attribute__((unused)) static void Test_cfg_GetType()
{
    le_result_t result;
    const char *LE_NONNULL groupName = "config1/";
    const char *LE_NONNULL nodeName =  "aBoolVal";
    taf_mngdStorCfg_NodeType_t typePtr;
    result = taf_mngdStorCfg_GetType(cRef, groupName, nodeName, &typePtr);
    LE_TEST_OK(result == LE_OK, "Test taf_mngdStorCfg_GetType");
    LE_INFO("Node type is %d",typePtr);
}
__attribute__((unused)) static void Test_cfg_GetString()
{
    le_result_t result;
    const char *LE_NONNULL groupName = "config1/";
    const char *LE_NONNULL nodeName = "aStringVal";
    char nodeValue[LE_CFG_STR_LEN_BYTES];
    result = taf_mngdStorCfg_GetString(cRef, groupName, nodeName, nodeValue, sizeof(nodeValue));
    LE_TEST_OK(result == LE_OK, "Test taf_mngdStorCfg_GetString");
    LE_INFO("Value for node %s is %s",nodeName,nodeValue);
}

__attribute__((unused)) static void Test_cfg_GetInt()
{
    le_result_t result;
    const char *LE_NONNULL groupName = "config1/";
    const char *LE_NONNULL nodeName = "aIntVal";
    int32_t nodeValuePtr = 0;
    result = taf_mngdStorCfg_GetInt(cRef, groupName, nodeName, &nodeValuePtr);
    LE_TEST_OK(result == LE_OK, "Test taf_mngdStorCfg_GetInt");
    LE_INFO("Value for node %s is %d",nodeName,nodeValuePtr);
}
__attribute__((unused)) static void Test_cfg_GetFloat()
{
    le_result_t result;
    const char *LE_NONNULL groupName = "config1/";
    const char *LE_NONNULL nodeName = "aFloatVal";
    double nodeValuePtr = 0.08597;
    result = taf_mngdStorCfg_GetFloat(cRef, groupName, nodeName, &nodeValuePtr);
    LE_TEST_OK(result == LE_OK, "Test taf_mngdStorCfg_GetFloat");
    LE_INFO("Value for node %s is %f",nodeName,nodeValuePtr);
}
__attribute__((unused)) static void Test_cfg_GetBool()
{
    le_result_t result;
    const char *LE_NONNULL groupName = "config1/";
    const char *LE_NONNULL nodeName = "aBoolVal";
    int32_t nodeValuePtr = 0;
    result = taf_mngdStorCfg_GetBool(cRef, groupName, nodeName, &nodeValuePtr);
    LE_TEST_OK(result == LE_OK, "Test taf_mngdStorCfg_GetBool");
    LE_INFO("Value for node %s is %d",nodeName,nodeValuePtr);
}


__attribute__((unused)) static void Test_cfg_ReleaseRef(){
    le_result_t result;
    result = taf_mngdStorCfg_ReleaseRef(cRef);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorCfg_ReleaseRef");
}

COMPONENT_INIT
{
    if (le_arg_NumArgs() > 1)
    {
        const char* operation = le_arg_GetArg(0);
        char* op_label = NULL;
        char* op_data = NULL;

        if (NULL == operation)
        {
            LE_ERROR("operation is NULL");
            exit(EXIT_FAILURE);
        }

        const char* label = le_arg_GetArg(1);

        if(strlen(label) == 0)
        {
            LE_ERROR("Invalid data label");
        }
        op_label = (char*)label;

        if (le_arg_NumArgs() > 2)
        {
            const char* data = le_arg_GetArg(2);

            if(strlen(data) == 0)
            {
                LE_ERROR("Invalid data label");
            }
            op_data = (char*)data;
        }

        if(op_label == NULL || strlen(op_label) == 0)
        {
            LE_ERROR("Invalid operation");
        }

        if (strcmp(operation, OP_HELP) == 0)
        {
            PrintUsage();
        }
        else if (strcmp(operation, OP_CREATE) == 0)
        {
            Test_Op_create(op_label);
        }
        else if (strcmp(operation, OP_WRITE) == 0)
        {
            if(op_data != NULL)
            {
                Test_Op_Write(op_label, op_data);
            }
        }
        else if (strcmp(operation, OP_READ) == 0)
        {
            Test_Op_Read(op_label);
        }
        else if (strcmp(operation, OP_DELETE) == 0)
        {
            Test_Op_Delete(op_label);
        }
        else
        {
            LE_ERROR("Invalid operation");
        }
    }
    else if (le_arg_NumArgs() == 0)
    {
        LE_TEST_PLAN(LE_TEST_NO_PLAN);

        LE_TEST_INFO("=== TelAF MngdStorage unit test BEGIN ===");

        LE_TEST_INFO("=== Test secure storage management ===");
        Test_Secure_Storage_Management();

        LE_TEST_INFO("=== Test secure data management ===");
        Test_Secure_Data_Management();

        LE_TEST_INFO("=== Test secure data write ===");
        Test_Secure_Data_Write();

        LE_TEST_INFO("=== Test secure data read ===");
        Test_Secure_Data_Read();

        LE_TEST_INFO("=== Test secure data read write chunks ===");
        Test_Secure_Data_Read_Write_Chunks();

        LE_TEST_INFO("=== Test delete storage ===");
        Test_Secure_Delete_Storage();

        LE_TEST_INFO("=== Test GetRef Process ConfigStorage");
        Test_cfg_GetRef();

        LE_TEST_INFO("=== Test update Process configStorage ===");
        Test_cfg_UpdateProcess();

        LE_TEST_INFO("=== Test Rollback Process configStorage ===");
        Test_cfg_RollbackProcess();

        LE_TEST_INFO("=== Test Cancel Process configStorage ===");
        Test_cfg_CancelProcess();

        LE_TEST_INFO("=== Test GetVersion ===");
        Test_cfg_GetVersion();

        LE_TEST_INFO("=== Test GetType ===");
        Test_cfg_GetType();

        LE_TEST_INFO("=== Test GetString ===");
        Test_cfg_GetString();

        LE_TEST_INFO("=== Test GetInt ===");
        Test_cfg_GetInt();

        LE_TEST_INFO("=== Test GetFloat ===");
        Test_cfg_GetFloat();

        LE_TEST_INFO("=== Test GetBool ===");
        Test_cfg_GetBool();

        LE_TEST_INFO("=== Test Release configStorage ===");
        Test_cfg_ReleaseRef();

        LE_TEST_INFO("=== TelAF MngdStorage unit test END ===");
    }
    else
    {
        LE_ERROR("Invalid operation");
        PrintUsage();
    }

    LE_TEST_EXIT;
}
