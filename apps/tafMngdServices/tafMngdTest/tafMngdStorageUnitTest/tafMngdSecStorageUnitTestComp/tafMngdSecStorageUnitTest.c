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
#define OP_CREATE    "create"
#define OP_READ      "read"
#define OP_WRITE     "write"
#define OP_DELETE    "delete"
#define OP_SHARE     "share"
#define OP_CANCEL    "cancel"
#define OP_GET       "get"

#define TEST_DATA_LABEL "testdata"
#define MSS_SECURE_STORAGE_SIZE 8192
#define LE_CFG_STR_LEN_BYTES   512

uint8_t TEST_TEXT_PATTERN[] = {'a','b','c','d','e','f','g'};
uint8_t TEST_NUM_PATTERN[] = {0,1,2,3,4,5,6,7,8,9};

taf_mngdStorSecData_DataRef_t dataRef;
taf_mngdStorSecData_DataStateChangeHandlerRef_t handlerRef;

__attribute__((unused)) static void PrintUsage()
{
    puts("\n"
        "-------- To do unit test automatically --------\n"
        "app start tafMngdStorageUnitTest\n"
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

    res = taf_mngdStorSecData_CreateData(TEST_DATA_LABEL);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_CreateData");

    dataRef = taf_mngdStorSecData_GetDataRef(TEST_DATA_LABEL);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdStorSecData_GetDataRef");

    taf_mngdStorSecData_DataRef_t checkedDataRef = taf_mngdStorSecData_GetDataRef(TEST_DATA_LABEL);

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

    res = taf_mngdStorSecData_CreateData(TEST_DATA_LABEL);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_CreateData");

    dataRef = taf_mngdStorSecData_GetDataRef(TEST_DATA_LABEL);

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

__attribute__((unused)) static void Test_Secure_Data_Delete()
{
    le_result_t res;

    res = taf_mngdStorSecData_DeleteData(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_DeleteData");
}

__attribute__((unused)) static void dataStateChangeHandler
(
    char dataName[TAF_MNGDSTORSECDATA_MAX_DATA_LABEL_BYTES],
    char appName[LE_LIMIT_APP_NAME_LEN + 1],
    taf_mngdStorSecData_DataState_t state,
    void* context
)
{
    LE_INFO("Data state change triggered for %s::%s, state: %u", appName, dataName, state);
}

__attribute__((unused)) static void Test_Secure_Data_Sharing()
{
    const char* sharedAppNameList[] = {"Shared_App1", "Shared_App2",
                                       "Shared_App3", "Shared_App4",
                                       "Shared_App5"};

    const taf_mngdStorSecData_DataUsage_t usageList[] = {TAF_MNGDSTORSECDATA_USAGE_READ,
                                                         TAF_MNGDSTORSECDATA_USAGE_WRITE,
                                                         TAF_MNGDSTORSECDATA_USAGE_READ,
                                                         TAF_MNGDSTORSECDATA_USAGE_WRITE,
                                                         TAF_MNGDSTORSECDATA_USAGE_READ_WRITE};

    char appName[5][LE_LIMIT_APP_NAME_LEN + 1] = { 0 };
    taf_mngdStorSecData_DataUsage_t usage[5] = { 0 };

    le_result_t res;

    res = taf_mngdStorSecData_CreateData(TEST_DATA_LABEL);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_CreateData");

    dataRef = taf_mngdStorSecData_GetDataRef(TEST_DATA_LABEL);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdStorSecData_GetDataRef");

    // Share a wrong usage
    for (int i = 0; i < NUM_ARRAY_MEMBERS(sharedAppNameList); i++)
    {
        LE_TEST_ASSERT(LE_BAD_PARAMETER ==
                        taf_mngdStorSecData_ShareData(dataRef,
                                                        sharedAppNameList[i],
                                                        TAF_MNGDSTORSECDATA_USAGE_UNKNOWN),
                        "Test taf_mngdStorSecData_ShareData");
    }

    // Share a correct usage
    for (int i = 0; i < NUM_ARRAY_MEMBERS(sharedAppNameList); i++)
    {
        LE_TEST_ASSERT(LE_OK ==
                        taf_mngdStorSecData_ShareData(dataRef,
                                                        sharedAppNameList[i],
                                                        usageList[i]),
                        "Test taf_mngdStorSecData_ShareData");
    }

    // Get sharedApp list.
    LE_TEST_ASSERT(LE_OK ==
                    taf_mngdStorSecData_GetFirstSharedApp(dataRef,
                                                            appName[0], LE_LIMIT_APP_NAME_LEN + 1,
                                                            &usage[0]),
                    "Test taf_mngdStorSecData_GetFirstSharedApp");

    uint i = 1;
    while (i < NUM_ARRAY_MEMBERS(sharedAppNameList))
    {
        LE_TEST_ASSERT(LE_OK ==
                        taf_mngdStorSecData_GetNextSharedApp(dataRef,
                                                                appName[0], LE_LIMIT_APP_NAME_LEN + 1,
                                                                &usage[0]),
                    "Test taf_mngdStorSecData_GetNextSharedApp");
        i++;
    }

    // Cancel the sharing for all apps
    for (int i = 0; i < NUM_ARRAY_MEMBERS(sharedAppNameList); i++)
    {
        LE_TEST_ASSERT(LE_OK == taf_mngdStorSecData_CancelDataSharing(dataRef,
                                                                        sharedAppNameList[i]),
                        "Test taf_mngdStorSecData_GetNextSharedApp");
    }

    LE_TEST_ASSERT(LE_NOT_PERMITTED == taf_mngdStorSecData_CancelDataSharing(dataRef,
                                                                        sharedAppNameList[0]),
                        "Test taf_mngdStorSecData_GetNextSharedApp");

    handlerRef = taf_mngdStorSecData_AddDataStateChangeHandler(
        TEST_DATA_LABEL,
        "Shared_App",
        (taf_mngdStorSecData_DataStateChangeHandlerFunc_t)dataStateChangeHandler,
        NULL);

    LE_TEST_ASSERT(handlerRef != NULL, "Test taf_mngdStorSecData_AddDataStateChangeHandler");

    taf_mngdStorSecData_RemoveDataStateChangeHandler(handlerRef);

    LE_TEST_ASSERT(true, "Test taf_mngdStorSecData_RemoveDataStateChangeHandler");

    res = taf_mngdStorSecData_DeleteData(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_DeleteData");
}

__attribute__((unused)) static void Test_Op_create(const char* label)
{
    if(label == NULL)
    {
        LE_ERROR("Bad paremeter");
        exit(1);
    }

    le_result_t res;

    res = taf_mngdStorSecData_CreateData(label);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_CreateData");

    printf("Create data label: %s succussfully\n", label);
    fflush(stdout);
}

__attribute__((unused)) static void Test_Op_Write
(
    const char* label, const char* data
)
{
    if(label == NULL || data == NULL)
    {
        LE_ERROR("Bad paremeter");
        exit(1);
    }

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
    if(label == NULL)
    {
        LE_ERROR("Bad paremeter");
        exit(1);
    }

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
    if(label == NULL)
    {
        LE_ERROR("Bad paremeter");
        exit(1);
    }

    le_result_t res;

    dataRef = taf_mngdStorSecData_GetDataRef(label);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdStorSecData_GetDataRef");

    res = taf_mngdStorSecData_DeleteData(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_DeleteData");

    printf("Delete data label: %s succussfully\n", label);
    fflush(stdout);
}

__attribute__((unused)) static void Test_Op_Share
(
    const char* label,
    const char* app,
    const char* usage
)
{
    if(label == NULL || app == NULL || usage == NULL)
    {
        LE_ERROR("Bad paremeter");
        exit(1);
    }

    le_result_t res;

    dataRef = taf_mngdStorSecData_GetDataRef(label);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdStorSecData_GetDataRef");

    int num = (int)atoi(usage);

    taf_mngdStorSecData_DataUsage_t dataUsage = (taf_mngdStorSecData_DataUsage_t)(num);

    res = taf_mngdStorSecData_ShareData(dataRef, app, dataUsage);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_ShareData");

    printf("Share data: %s to app '%s' with usage %d succussfully\n", label, app, dataUsage);
    fflush(stdout);
}

__attribute__((unused)) static void Test_Op_Cancel
(
    const char* label,
    const char* app
)
{
    if(label == NULL || app == NULL)
    {
        LE_ERROR("Bad paremeter");
        exit(1);
    }

    le_result_t res;

    dataRef = taf_mngdStorSecData_GetDataRef(label);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdStorSecData_GetDataRef");

    res = taf_mngdStorSecData_CancelDataSharing(dataRef, app);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_CancelDataSharing");

    printf("Cancel data: %s sharing to app '%s' succussfully\n", label, app);
    fflush(stdout);
}

__attribute__((unused)) static void Test_Op_Get
(
    const char* label
)
{
    if(label == NULL)
    {
        LE_ERROR("Bad paremeter");
        exit(1);
    }

    le_result_t res;

    dataRef = taf_mngdStorSecData_GetDataRef(label);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdStorSecData_GetDataRef");

    char appName[LE_LIMIT_APP_NAME_LEN + 1];

    taf_mngdStorSecData_DataUsage_t usage;

    res = taf_mngdStorSecData_GetFirstSharedApp(dataRef, appName, sizeof(appName), &usage);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdStorSecData_GetFirstSharedApp");

    printf("Get data: %s first shared app '%s' usage %d succussfully\n", label, appName, usage);

    uint i = 1;
    while (LE_OK == taf_mngdStorSecData_GetNextSharedApp(dataRef,                                                            appName, LE_LIMIT_APP_NAME_LEN + 1,
                                                            &usage))
    {
        printf("Get data: %s next shared app '%s' usage %d succussfully\n", label, appName, usage);
        i++;
    }

    fflush(stdout);
}

COMPONENT_INIT
{
    if (le_arg_NumArgs() > 1)
    {
        const char* operation = le_arg_GetArg(0);
        char* op_label = NULL;
        char* op_para1 = NULL;
        char* op_para2 = NULL;

        if (NULL == operation)
        {
            LE_ERROR("operation is NULL");
            exit(EXIT_FAILURE);
        }
        LE_INFO("operation: %s", operation);

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
                LE_ERROR("Invalid data");
            }
            op_para1 = (char*)data;
        }

        if (le_arg_NumArgs() > 3)
        {
            const char* data = le_arg_GetArg(3);

            if(strlen(data) == 0)
            {
                LE_ERROR("Invalid data");
            }
            op_para2 = (char*)data;
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
            if(op_para1 != NULL)
            {
                Test_Op_Write(op_label, op_para1);
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
        else if (strcmp(operation, OP_SHARE) == 0)
        {
            Test_Op_Share(op_label, op_para1, op_para2);
        }
        else if (strcmp(operation, OP_CANCEL) == 0)
        {
            Test_Op_Cancel(op_label, op_para1);
        }
        else if (strcmp(operation, OP_GET) == 0)
        {
            Test_Op_Get(op_label);
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

        LE_TEST_INFO("=== Test secure data delete ===");
        Test_Secure_Data_Delete();

        LE_TEST_INFO("=== Test secure data sharing ===");
        Test_Secure_Data_Sharing();
        LE_TEST_INFO("=== TelAF MngdStorage unit test END ===");
    }
    else
    {
        LE_ERROR("Invalid operation");
        PrintUsage();
    }

    LE_TEST_EXIT;
}
