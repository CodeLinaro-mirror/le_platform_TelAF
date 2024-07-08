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

uint8_t TEST_TEXT_PATTERN[] = {'a','b','c','d','e','f','g'};
uint8_t TEST_NUM_PATTERN[] = {0,1,2,3,4,5,6,7,8,9};

taf_mngdSecStore_DataRef_t dataRef;

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

    res = taf_mngdSecStore_GetFreeSize(&freeSize);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_GetFreeSize");

    LE_INFO("freeSize = %u", freeSize);

    LE_TEST_ASSERT(freeSize == MSS_SECURE_STORAGE_SIZE, "Test taf_mngdSecStore_GetFreeSize");

    uint32_t usedSize = 0;

    res = taf_mngdSecStore_GetUsedSize(&usedSize);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_GetUsedSize");

    LE_TEST_ASSERT(usedSize == 0, "Test taf_mngdSecStore_GetUsedSize");
}


__attribute__((unused)) static void Test_Secure_Data_Management()
{
    le_result_t res;

    dataRef = taf_mngdSecStore_CreateData(TEST_DATA_LABEL);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdSecStore_CreateData");

    taf_mngdSecStore_DataRef_t checkedDataRef = taf_mngdSecStore_GetDataRef(TEST_DATA_LABEL);

    LE_TEST_ASSERT(checkedDataRef != NULL, "Test taf_mngdSecStore_GetDataRef");

    LE_TEST_ASSERT(checkedDataRef == dataRef, "Test taf_mngdSecStore_GetDataRef");

    uint32_t dataSize = 0;

    res = taf_mngdSecStore_GetDataSize(dataRef, &dataSize);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_GetDataSize");

    LE_TEST_ASSERT(dataSize == 0, "Test taf_mngdSecStore_GetDataSize");

    res = taf_mngdSecStore_DeleteData(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_DeleteData");
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

    dataRef = taf_mngdSecStore_CreateData(TEST_DATA_LABEL);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdSecStore_CreateData");

    res = taf_mngdSecStore_WriteDataStart(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_WriteDataStart");

    res = taf_mngdSecStore_WriteDataChunk(dataRef,
                                                TEST_TEXT_PATTERN,
                                                sizeof(TEST_TEXT_PATTERN));

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_WriteDataChunk");

    res = taf_mngdSecStore_WriteDataChunk(dataRef,
                                                TEST_NUM_PATTERN,
                                                sizeof(TEST_NUM_PATTERN));

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_WriteDataChunk");

    res = taf_mngdSecStore_WriteDataEnd(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_WriteDataEnd");

    uint32_t dataSize = 0;

    res = taf_mngdSecStore_GetDataSize(dataRef, &dataSize);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_GetDataSize");

    LE_TEST_ASSERT(dataSize > 0, "Test taf_mngdSecStore_GetDataSize");
}

__attribute__((unused)) static void Test_Secure_Data_Read()
{
    le_result_t res;

    uint8_t readBuf[TAF_MNGDSECSTORE_MAX_DATA_CHUNK_SIZE] = {0};
    size_t readLen = sizeof(readBuf);

    res = taf_mngdSecStore_ReadDataFirstChunk(dataRef, readBuf, &readLen);

    LE_INFO("readLen = %" PRIuS, readLen);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_ReadDataFirstChunk");

    LE_TEST_ASSERT(readLen > 0, "Test taf_mngdSecStore_ReadDataFirstChunk");

    LE_TEST_ASSERT(readLen == (sizeof(TEST_TEXT_PATTERN) + sizeof(TEST_NUM_PATTERN)),
                    "Test taf_mngdSecStore_ReadDataFirstChunk");

    LE_TEST_ASSERT(memcmp(readBuf, TEST_TEXT_PATTERN, sizeof(TEST_TEXT_PATTERN)) == 0,
                    "Test taf_mngdSecStore_ReadDataFirstChunk");

    LE_TEST_ASSERT(memcmp(readBuf + sizeof(TEST_TEXT_PATTERN),
                            TEST_NUM_PATTERN, sizeof(TEST_NUM_PATTERN)) == 0,
                    "Test taf_mngdSecStore_ReadDataFirstChunk");
}

__attribute__((unused)) static void Test_Secure_Data_Read_Write_Chunks()
{
    le_result_t res;

    res = taf_mngdSecStore_WriteDataStart(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_WriteDataStart");

    uint8_t* genData = GenerateTestData(TAF_MNGDSECSTORE_MAX_DATA_CHUNK_SIZE * 2);

    res = taf_mngdSecStore_WriteDataChunk(dataRef,
                                            genData,
                                            TAF_MNGDSECSTORE_MAX_DATA_CHUNK_SIZE);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_WriteDataChunk");

    res = taf_mngdSecStore_WriteDataChunk(dataRef,
                                            genData + TAF_MNGDSECSTORE_MAX_DATA_CHUNK_SIZE,
                                            TAF_MNGDSECSTORE_MAX_DATA_CHUNK_SIZE);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_WriteDataChunk");

    res = taf_mngdSecStore_WriteDataEnd(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_WriteDataEnd");

    uint8_t readBuf[TAF_MNGDSECSTORE_MAX_DATA_CHUNK_SIZE] = {0};
    size_t readLen = sizeof(readBuf);
    size_t totalReadLen = 0;

    res = taf_mngdSecStore_ReadDataFirstChunk(dataRef, readBuf, &readLen);

    LE_INFO("readLen = %" PRIuS, readLen);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_ReadDataFirstChunk");

    LE_TEST_ASSERT(readLen > 0, "Test taf_mngdSecStore_ReadDataFirstChunk");

    LE_TEST_ASSERT(memcmp(readBuf, genData, readLen) == 0,
                    "Test taf_mngdSecStore_ReadDataFirstChunk");

    totalReadLen = readLen;

    while(readLen > 0)
    {
        taf_mngdSecStore_ReadDataNextChunk(dataRef, readBuf, &readLen);

        LE_INFO("readLen = %" PRIuS, readLen);

        LE_TEST_ASSERT(memcmp(readBuf, genData + totalReadLen, readLen) == 0,
                        "Test taf_mngdSecStore_ReadDataNextChunk");

        totalReadLen += readLen;
    }

    free(genData);
}

__attribute__((unused)) static void Test_Secure_Delete_Storage()
{
    le_result_t res;

    res = taf_mngdSecStore_DeleteData(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_DeleteData");
}

__attribute__((unused)) static void Test_Op_create(const char* label)
{
    dataRef = taf_mngdSecStore_CreateData(label);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdSecStore_CreateData");

    printf("Create data label: %s succussfully\n", label);
    fflush(stdout);
}

__attribute__((unused)) static void Test_Op_Write
(
    const char* label, const char* data
)
{
    le_result_t res;

    dataRef = taf_mngdSecStore_GetDataRef(label);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdSecStore_GetDataRef");

    res = taf_mngdSecStore_WriteDataStart(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_WriteDataStart");

    res = taf_mngdSecStore_WriteDataChunk(dataRef,
                                            (uint8_t*)data,
                                            strlen(data));

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_WriteDataChunk");

    res = taf_mngdSecStore_WriteDataEnd(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_WriteDataEnd");

    printf("Write data: [%s] succussfully\n", data);
    fflush(stdout);
}

__attribute__((unused)) static void Test_Op_Read(const char* label)
{
    le_result_t res;

    dataRef = taf_mngdSecStore_GetDataRef(label);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdSecStore_GetDataRef");

    uint8_t readBuf[TAF_MNGDSECSTORE_MAX_DATA_CHUNK_SIZE] = {0};
    size_t readLen = sizeof(readBuf);

    res = taf_mngdSecStore_ReadDataFirstChunk(dataRef, readBuf, &readLen);

    LE_INFO("readLen = %" PRIuS, readLen);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_ReadDataFirstChunk");

    printf("Read data: [%s] succussfully\n", (char*)readBuf);
    fflush(stdout);
}

__attribute__((unused)) static void Test_Op_Delete(const char* label)
{
    le_result_t res;

    dataRef = taf_mngdSecStore_GetDataRef(label);

    LE_TEST_ASSERT(dataRef != NULL, "Test taf_mngdSecStore_GetDataRef");

    res = taf_mngdSecStore_DeleteData(dataRef);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_mngdSecStore_DeleteData");

    printf("Delete data label: %s succussfully\n", label);
    fflush(stdout);
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

        LE_TEST_INFO("=== TelAF MngdStorage unit test END ===");
    }
    else
    {
        LE_ERROR("Invalid operation");
        PrintUsage();
    }

    LE_TEST_EXIT;
}