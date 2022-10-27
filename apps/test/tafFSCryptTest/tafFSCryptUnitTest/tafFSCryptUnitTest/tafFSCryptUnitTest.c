/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

/*
 * @file       tafKeyStoreUnitTest.c
 * @brief      This file includes test functions for tafKeyStore Service
 */

#include "legato.h"
#include "interfaces.h"
#include "linux/file.h"

#define OP_GET      "get"
#define OP_LOCK     "lock"
#define OP_UNLOCK   "unlock"
#define OP_DELETE   "delete"

typedef enum
{
    NONE,
    FSC_GET,
    FSC_LOCK,
    FSC_UNLOCK,
    FSC_DELETE
}
fs_crypt_operation;

static char DIR_TEST[TAF_FSC_MAX_STORAGE_NAME_SIZE/2] = "/app/fs-crypt";
static char FILE_TEST[TAF_FSC_MAX_STORAGE_NAME_SIZE/2] = "test.txt";

#define TEST_CONTENT_LEN 100
static char CONTENT_TEST[TEST_CONTENT_LEN] = "aaabbbcccddd";

//--------------------------------------------------------------------------------------------------
/**
 * Tests for key management APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void FSCryptProcessTest(void)
{
    le_result_t result;
    taf_fsc_StorageRef_t storageRef = taf_fsc_GetStorageRef(DIR_TEST, &result);

    LE_TEST_ASSERT(LE_OK == result, "Test taf_fsc_GetStorageRef.");
    LE_TEST_ASSERT(storageRef, "Test taf_fsc_GetStorageRef.");

    LE_TEST_ASSERT(LE_OK == taf_fsc_LockStorage(storageRef), "Test taf_fsc_LockStorage.");

    LE_TEST_ASSERT(LE_OK == taf_fsc_UnlockStorage(storageRef), "Test taf_fsc_UnlockStorage.");

    LE_TEST_ASSERT(LE_OK == taf_fsc_DeleteStorage(storageRef), "Test taf_fsc_DeleteStorage.");
}

__attribute__((unused)) static void GetStorageTest(void)
{
    le_result_t result;
    taf_fsc_StorageRef_t storageRef = taf_fsc_GetStorageRef(DIR_TEST, &result);

    LE_TEST_ASSERT(LE_OK == result, "Test taf_fsc_GetStorageRef.");
    LE_TEST_ASSERT(storageRef, "Test taf_fsc_GetStorageRef.");
}

__attribute__((unused)) static void LockStorageTest(void)
{
    le_result_t result;
    taf_fsc_StorageRef_t storageRef = taf_fsc_GetStorageRef(DIR_TEST, &result);

    LE_TEST_ASSERT(LE_OK == result, "Test taf_fsc_GetStorageRef.");
    LE_TEST_ASSERT(storageRef, "Test taf_fsc_GetStorageRef.");

    LE_TEST_ASSERT(LE_OK == taf_fsc_LockStorage(storageRef), "Test taf_fsc_LockStorage.");

    char testFilePath[TAF_FSC_MAX_STORAGE_NAME_SIZE] = {0};

    snprintf(testFilePath, sizeof(testFilePath), "%s/%s", DIR_TEST, FILE_TEST);

    int fd = open(testFilePath, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);

    LE_TEST_ASSERT(fd < 0, "Test open file after lock.");

    LE_INFO("opening %s: %s\n", testFilePath, LE_ERRNO_TXT(errno));
}

__attribute__((unused)) static void UnlockStorageTest(void)
{
    le_result_t result;
    taf_fsc_StorageRef_t storageRef = taf_fsc_GetStorageRef(DIR_TEST, &result);

    LE_TEST_ASSERT(LE_OK == result, "Test taf_fsc_GetStorageRef.");
    LE_TEST_ASSERT(storageRef, "Test taf_fsc_GetStorageRef.");

    LE_TEST_ASSERT(LE_OK == taf_fsc_UnlockStorage(storageRef), "Test taf_fsc_UnlockStorage.");

    char testFilePath[TAF_FSC_MAX_STORAGE_NAME_SIZE] = {0};

    snprintf(testFilePath, sizeof(testFilePath), "%s/%s", DIR_TEST, FILE_TEST);

    int fd = open(testFilePath, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);

    LE_TEST_ASSERT(fd != -1, "Test open file after unlock. (%s)", LE_ERRNO_TXT(errno));

    int len = le_fd_Write(fd, &CONTENT_TEST, sizeof(CONTENT_TEST));
    LE_TEST_ASSERT(len == sizeof(CONTENT_TEST), "Test write file after unlock.");

    LE_TEST_ASSERT(fd > 0, "Test write file after unlock.");

    le_fd_Close(fd);
}

__attribute__((unused)) static void DeleteStorageTest(void)
{
    le_result_t result;
    taf_fsc_StorageRef_t storageRef = taf_fsc_GetStorageRef(DIR_TEST, &result);

    LE_TEST_ASSERT(LE_OK == result, "Test taf_fsc_GetStorageRef.");
    LE_TEST_ASSERT(storageRef, "Test taf_fsc_GetStorageRef.");

    LE_TEST_ASSERT(LE_OK == taf_fsc_DeleteStorage(storageRef), "Test taf_fsc_DeleteStorage.");
}

/**
 * Start app : app start tafFSCryptUnitTest
 * Execute app : app runProc tafFSCryptUnitTest tafFSCryptUnitTest -- <operation> <dir>
 * e.g. app runProc tafFSCryptUnitTest tafFSCryptUnitTest -- get /app/test
 *
 * The default testing path is "/app/fs-crypt" if ignoring <dir> parameter.
 * If no <operation> is input, test app will run get, lock, unlock and delete functions in sequence.
 */
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("=== telaf FSCrypt test BEGIN ===");

    if (le_arg_NumArgs() > 0)
    {
        fs_crypt_operation op = NONE;
        const char* operation = le_arg_GetArg(0);
        if (NULL == operation)
        {
            LE_ERROR("operation is NULL");
            exit(EXIT_FAILURE);
        }

        if (strcmp(operation, OP_GET) == 0)
        {
            op = FSC_GET;
        }
        else if (strcmp(operation, OP_LOCK) == 0)
        {
            op = FSC_LOCK;
        }
        else if (strcmp(operation, OP_UNLOCK) == 0)
        {
            op = FSC_UNLOCK;
        }
        else if (strcmp(operation, OP_DELETE) == 0)
        {
            op = FSC_DELETE;
        }
        else
        {
            LE_ERROR("Invalid operation");
        }

        if (le_arg_NumArgs() > 1)
        {
            const char* dirpath = le_arg_GetArg(1);
            if (NULL == dirpath)
            {
                LE_ERROR("directory is NULL");
                exit(EXIT_FAILURE);
            }
            le_utf8_Copy((char*)DIR_TEST, dirpath, TAF_FSC_MAX_STORAGE_NAME_SIZE, NULL);
            LE_INFO("Test dir is %s", DIR_TEST);
        }

        switch(op)
        {
            case FSC_GET:
                GetStorageTest();
            break;

            case FSC_LOCK:
                LockStorageTest();
            break;

            case FSC_UNLOCK:
                UnlockStorageTest();
            break;

            case FSC_DELETE:
                DeleteStorageTest();
            break;

            default:
                FSCryptProcessTest(); // Basic FS-Crypt API test
            break;
        }
    }
    else
    {
        FSCryptProcessTest(); // Basic FS-Crypt API test
    }

    LE_TEST_INFO("=== telaf FSCrypt test END ===");

    LE_TEST_EXIT;
}
