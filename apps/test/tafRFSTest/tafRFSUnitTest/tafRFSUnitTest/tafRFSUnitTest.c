/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafRFSUnitTest.c
 * @brief      This file includes test functions for tafRFSLib component
 */

#include "legato.h"
#include "interfaces.h"
#include "linux/file.h"
#include <stdio.h>
#include <stdlib.h>

#include "tafRFSLib.h"

#define TEST_FILE_PATH "/data/testRFS_File.txt"
#define TEST_APP_DEFAULT_STORAGE "/persist/rfs/backup/"
#define TEST_DATA_SIZE (1024 * 10)
#define TIMEOUT_ITEM_TEST 5
#define TEST_MAX_FILE_SIZE  10240
#define TEST_MAX_FILE_COUNT 10

#define OP_READ    "read"
#define OP_WRITE   "write"
#define OP_DELETE  "delete"
#define OP_CORRUPT "corrupt"

static le_mem_PoolRef_t TestRequestPool;
static le_thread_Ref_t TestThreadRef;
static le_sem_Ref_t sem_TestItem;

typedef enum
{
    READ,
    WRITE,
    DELETE,
    CORRUPT
}
TestOperation_t;

typedef struct
{
    TestOperation_t op;
}
TestRequest_t;

TestOperation_t testSequence[] = {WRITE, CORRUPT, READ, DELETE};
//--------------------------------------------------------------------------------------------------
/**
 * Set semaphore timeout
 */
//--------------------------------------------------------------------------------------------------

__attribute__((unused)) le_result_t WaitForSem_Timeout
(
    le_sem_Ref_t semRef,
    uint32_t seconds
)
{
    le_clk_Time_t timeToWait = {seconds, 0};

    return le_sem_WaitWithTimeOut(semRef, timeToWait);
}

// Main function of low-priority background thread.
__attribute__((unused)) static void* TestThread
(
    void* contextPtr // not used.
)
{
    le_event_RunLoop();
}

__attribute__((unused)) static void ErrCallback(taf_rfs_Error_t error, const char* filePath)
{
    LE_ERROR("error %d happened with file %s", error, filePath);
}

__attribute__((unused)) static char* GenerateTestData()
{
    char* data = malloc(TEST_DATA_SIZE + 1);
    LE_TEST_ASSERT(data != NULL, "Allocate test data");
    for(size_t i = 0; i < TEST_DATA_SIZE; ++i)
    {
        data[i] = 'A' + (i % 26); // Use cycling chars for filling data
    }
    data[TEST_DATA_SIZE] = '\0';
    return data;
}

__attribute__((unused)) static void Test_Init()
{
    le_result_t res = taf_rfs_Init(true, ErrCallback);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_rfs_Init");
}

__attribute__((unused)) static void Test_SetAppBackupStorage()
{
    le_result_t res = taf_rfs_SetBackupStorage(NULL, TEST_MAX_FILE_SIZE, TEST_MAX_FILE_COUNT);

    LE_TEST_ASSERT(res == LE_NOT_FOUND, "Test taf_rfs_SetBackupStorage");

    res = taf_rfs_SetBackupStorage(TEST_APP_DEFAULT_STORAGE, 0, 0);

    LE_TEST_ASSERT(res == LE_BAD_PARAMETER, "Test taf_rfs_SetBackupStorage");

    res = taf_rfs_SetBackupStorage(TEST_APP_DEFAULT_STORAGE,
                                    TEST_MAX_FILE_SIZE,
                                    TEST_MAX_FILE_COUNT);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_rfs_SetBackupStorage");
}

__attribute__((unused)) static void Test_Write()
{
    char* testData = GenerateTestData();
    LE_TEST_ASSERT(testData != NULL, "Allocate test data for write");

    int fd = taf_rfs_Open(TEST_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    LE_TEST_ASSERT(fd >= 0, "Test taf_rfs_Open for write");

    ssize_t bytesWritten = taf_rfs_Write(fd, (uint8_t*)testData, TEST_DATA_SIZE);
    LE_TEST_ASSERT(bytesWritten == TEST_DATA_SIZE, "Test taf_rfs_Write for write");

    LE_TEST_ASSERT(taf_rfs_Close(fd) == 0, "Test taf_rfs_Close for write");

    free(testData);
}

__attribute__((unused)) static void Test_Read()
{
    char* buffer = malloc(TEST_DATA_SIZE + 1);
    LE_TEST_ASSERT(buffer != NULL, "Allocate buffer for read");
    buffer[TEST_DATA_SIZE] = '\0';

    int fd = taf_rfs_Open(TEST_FILE_PATH, O_RDONLY, 0);
    LE_TEST_ASSERT(fd >= 0, "Test taf_rfs_Open for read");

    size_t expectedSize = TEST_DATA_SIZE;
    ssize_t bytesRead = taf_rfs_Read(fd, (uint8_t*)buffer, &expectedSize);
    LE_TEST_ASSERT(bytesRead == TEST_DATA_SIZE, "Test taf_rfs_Read for read");

    LE_TEST_ASSERT(taf_rfs_Close(fd) == 0, "Test taf_rfs_Close for read");

    char* data = malloc(TEST_DATA_SIZE + 1);
    LE_TEST_ASSERT(data != NULL, "Allocate test data");
    for(size_t i = 0; i < TEST_DATA_SIZE; ++i)
    {
        data[i] = 'A' + (i % 26); // Use cycling chars for filling data
    }

    LE_TEST_ASSERT(memcmp(buffer, data, TEST_DATA_SIZE) == 0, "Test file integrity");

    free(buffer);
}

__attribute__((unused)) static void Test_Delete()
{
    taf_rfs_Delete(TEST_FILE_PATH);
    struct stat st;
    LE_TEST_ASSERT(stat(TEST_FILE_PATH, &st) == -1, "Test taf_rfs_Delete");
}

// This function simulates corrupting file
__attribute__((unused)) static void Simulate_Corrupt() 
{
    // set up random element
    srand((unsigned) time(NULL));

    FILE* file = fopen(TEST_FILE_PATH, "r+b");
    if (file == NULL)
    {
        perror("Failed to open file");
        return;
    }

    // move to a random position in the file
    fseek(file, rand() % TEST_DATA_SIZE, SEEK_SET);

    // generate random data to overwrite original data
    for (int i = 0; i < 10; i++)
    {
        char randomByte = rand() % 256;
        fwrite(&randomByte, sizeof(char), 1, file);
        if (ferror(file))
        {
            perror("Failed to write to file");
            break;
        }
    }

    printf("File corruption simulation completed.\n");
    fclose(file);
}

static void ProcessTest
(
    void* param1Ptr, // request object pointer
    void* param2Ptr  // not used
)
{
    TestRequest_t* requestPtr = (TestRequest_t*)param1Ptr;

    switch(requestPtr->op)
    {
        case READ:
        Test_Read();
        break;
        case WRITE:
        Test_Write();
        break;
        case DELETE:
        Test_Delete();
        break;
        case CORRUPT:
        Simulate_Corrupt();
        break;
    }

    le_mem_Release(requestPtr);

    le_sem_Post(sem_TestItem);
}

/**
 * Start app : app start tafRFSUnitTest
 * Execute app : app runProc tafRFSUnitTest tafRFSUnitTest -- <operation>
 * e.g. app runProc tafRFSUnitTest tafRFSUnitTest -- write
 *
 * If no <operation> is input, test app will run write, corrupt, read and delete functions in sequence.
 */
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("=== telaf RFS test BEGIN ===");

    TestRequestPool = le_mem_CreatePool("Test Request", sizeof(TestRequest_t));

    sem_TestItem = le_sem_Create("TestItemSem", 0);

    TestThreadRef = le_thread_Create("Background Test Thread",
                                        TestThread,
                                        NULL);

    le_thread_Start(TestThreadRef);

    LE_TEST_INFO("=== Test Init ===");
    Test_Init();

    if (le_arg_NumArgs() > 0)
    {
        TestRequest_t* requestPtr = (TestRequest_t*)le_mem_ForceAlloc(TestRequestPool);

        const char* operation = le_arg_GetArg(0);
        if (NULL == operation)
        {
            LE_ERROR("operation is NULL");
            exit(EXIT_FAILURE);
        }

        if (strcmp(operation, OP_READ) == 0)
        {
            requestPtr->op = READ;
        }
        else if (strcmp(operation, OP_WRITE) == 0)
        {
            requestPtr->op = WRITE;
        }
        else if (strcmp(operation, OP_DELETE) == 0)
        {
            requestPtr->op = DELETE;
        }
        else if (strcmp(operation, OP_CORRUPT) == 0)
        {
            requestPtr->op = CORRUPT;
        }
        else
        {
            LE_ERROR("Invalid operation");
        }

        if (le_arg_NumArgs() > 1)
        {
            const char* path = le_arg_GetArg(1);

            if(strlen(path) > 0)
            {
                le_result_t res = taf_rfs_SetBackupStorage(path,
                                                        TEST_MAX_FILE_SIZE,
                                                        TEST_MAX_FILE_COUNT);

                LE_TEST_ASSERT(res == LE_OK, "Test taf_rfs_SetBackupStorage");
            }
        }

        le_event_QueueFunctionToThread(TestThreadRef, ProcessTest, requestPtr, NULL);

        LE_ASSERT_OK(WaitForSem_Timeout(sem_TestItem, TIMEOUT_ITEM_TEST));
    }
    else
    {
        LE_TEST_INFO("=== Test set storage ===");

        Test_SetAppBackupStorage();

        LE_INFO("Starting file function tests with parameterized data size...");

        for(uint i = 0; i < (sizeof(testSequence)/sizeof(testSequence[0])); i++)
        {
            TestRequest_t* requestPtr = (TestRequest_t*)le_mem_ForceAlloc(TestRequestPool);
            requestPtr->op = testSequence[i];
            le_event_QueueFunctionToThread(TestThreadRef, ProcessTest, requestPtr, NULL);

            LE_ASSERT_OK(WaitForSem_Timeout(sem_TestItem, TIMEOUT_ITEM_TEST));
        }

        LE_INFO("All tests passed with data size: %d", TEST_DATA_SIZE);
    }

    LE_TEST_EXIT;

    LE_TEST_INFO("=== telaf RFS test END ===");
}

