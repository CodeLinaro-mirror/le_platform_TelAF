/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <sys/xattr.h>
#include <errno.h>
#include <openssl/sha.h>

#include "tafRFSLib.h"

__attribute__((unused)) static void Simulate_Corrupt();

#define TEST_FILE_PATH "/data/testRFS_File.txt"
#define TEST_COPY_PATH "/data/testRFS_File_Copy.txt"
#define TEST_RENAME_PATH "/data/testRFS_File_Rename.txt"
#define TEST_APP_DEFAULT_STORAGE "/data/rfs/backup/"
#define TEST_DATA_SIZE (1024 * 10)
#define TIMEOUT_ITEM_TEST 5
#define TEST_MAX_FILE_SIZE  10240
#define TEST_MAX_FILE_COUNT 10
#define TEST_XATTR_NAME "security.md5"

#define OP_READ    "read"
#define OP_WRITE   "write"
#define OP_DELETE  "delete"
#define OP_CORRUPT "corrupt"
#define OP_COPY    "copy"
#define OP_RENAME  "rename"
#define OP_RESTORE              "restore"
#define OP_RESTORE_MISSING      "restore_missing"
#define OP_BACKUP_INVALID       "backup_invalid"
#define OP_RO_OPEN_HEALS_BACKUP "ro_open_heals_backup"
#define OP_BACKUP_MD5_MISSING_RECOVER "backup_md5_missing_recover"
#define OP_BACKUP_MD5_MISMATCH_RECOVER "backup_md5_mismatch_recover"
#define OP_CLOSE_UPDATES_BACKUP "close_updates_backup"

static le_mem_PoolRef_t TestRequestPool;
static le_thread_Ref_t TestThreadRef;
static le_sem_Ref_t sem_TestItem;

typedef enum
{
    READ,
    WRITE,
    DELETE,
    CORRUPT,
    COPY,
    RENAME,
    RESTORE,
    RESTORE_MISSING,
    BACKUP_INVALID,
    RO_OPEN_HEALS_BACKUP,
    BACKUP_MD5_MISSING_RECOVER,
    BACKUP_MD5_MISMATCH_RECOVER,
    CLOSE_UPDATES_BACKUP
}
TestOperation_t;

typedef struct
{
    TestOperation_t op;
}
TestRequest_t;

TestOperation_t testSequence[] = {
    WRITE,
    CORRUPT,
    RESTORE,
    WRITE,
    RESTORE_MISSING,
    WRITE,
    RO_OPEN_HEALS_BACKUP,
    WRITE,
    BACKUP_MD5_MISSING_RECOVER,
    WRITE,
    BACKUP_MD5_MISMATCH_RECOVER,
    BACKUP_INVALID,
    DELETE,
    WRITE,
    DELETE,
    WRITE,
    CLOSE_UPDATES_BACKUP,
    WRITE,
    COPY,
    WRITE,
    RENAME,
    DELETE
};
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
    for (size_t i = 0; i < TEST_DATA_SIZE; ++i)
    {
        data[i] = 'A' + (i % 26);
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
    le_result_t res = taf_rfs_SetBackupStorage(NULL);

    LE_TEST_ASSERT(res == LE_NOT_FOUND, "Test taf_rfs_SetBackupStorage");

    res = taf_rfs_SetBackupStorage(TEST_APP_DEFAULT_STORAGE);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_rfs_SetBackupStorage");

    res = taf_rfs_SetBackupCapacity(TEST_MAX_FILE_SIZE,
                                    TEST_MAX_FILE_COUNT);

    LE_TEST_ASSERT(res == LE_OK, "Test taf_rfs_SetBackupCapacity");
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

__attribute__((unused)) static void Verify_File(const char* filePath, const char* op)
{
    char* buffer = malloc(TEST_DATA_SIZE + 1);
    LE_TEST_ASSERT(buffer != NULL, "Allocate buffer for %s", op);
    buffer[TEST_DATA_SIZE] = '\0';

    int fd = taf_rfs_Open(filePath, O_RDONLY, 0);
    LE_TEST_ASSERT(fd >= 0, "Test taf_rfs_Open for %s", op);

    size_t expectedSize = TEST_DATA_SIZE;
    ssize_t bytesRead = taf_rfs_Read(fd, (uint8_t*)buffer, &expectedSize);
    LE_TEST_ASSERT(bytesRead == TEST_DATA_SIZE, "Test taf_rfs_Read for %s", op);

    LE_TEST_ASSERT(taf_rfs_Close(fd) == 0, "Test taf_rfs_Close for %s", op);

    char* data = malloc(TEST_DATA_SIZE + 1);
    LE_TEST_ASSERT(data != NULL, "Allocate test data");
    for (size_t i = 0; i < TEST_DATA_SIZE; ++i)
    {
        data[i] = 'A' + (i % 26);
    }

    LE_TEST_ASSERT(memcmp(buffer, data, TEST_DATA_SIZE) == 0, "Test file integrity");

    free(buffer);
    free(data);
}

__attribute__((unused)) static void Test_Read()
{
    Verify_File(TEST_FILE_PATH, OP_READ);
}

__attribute__((unused)) static void GetBackupPath(char* fullBackupPath, size_t backupPathSize)
{
    char backupPath[256] = {0};
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)TEST_FILE_PATH, strlen(TEST_FILE_PATH), hash);
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        snprintf(backupPath + strlen(backupPath),
                 sizeof(backupPath) - strlen(backupPath),
                 "%02x",
                 hash[i]);
    }

    snprintf(fullBackupPath, backupPathSize, "%s%s", TEST_APP_DEFAULT_STORAGE, backupPath);
}

__attribute__((unused)) static void RemoveBackupMd5Xattr()
{
    char fullBackupPath[512] = {0};
    GetBackupPath(fullBackupPath, sizeof(fullBackupPath));

    int ret = removexattr(fullBackupPath, TEST_XATTR_NAME);
    LE_TEST_ASSERT((ret == 0) || (errno == ENODATA), "Remove backup MD5 xattr");
}

__attribute__((unused)) static void CorruptBackupFile()
{
    char fullBackupPath[512] = {0};
    GetBackupPath(fullBackupPath, sizeof(fullBackupPath));

    FILE* file = fopen(fullBackupPath, "r+b");
    LE_TEST_ASSERT(file != NULL, "Open backup file for corruption");

    fseek(file, 0, SEEK_SET);
    for (int i = 0; i < 10; i++)
    {
        char randomByte = (char)('z' - i);
        fwrite(&randomByte, sizeof(char), 1, file);
    }
    fclose(file);
}

__attribute__((unused)) static void CorruptBackupMd5Xattr()
{
    char fullBackupPath[512] = {0};
    GetBackupPath(fullBackupPath, sizeof(fullBackupPath));

    const char* fakeMd5 = "00000000000000000000000000000000";
    int ret = setxattr(fullBackupPath, TEST_XATTR_NAME, fakeMd5, strlen(fakeMd5), 0);
    LE_TEST_ASSERT(ret == 0, "Force backup MD5 xattr mismatch");
}

__attribute__((unused)) static void Test_Restore()
{
    Simulate_Corrupt();
    Verify_File(TEST_FILE_PATH, OP_RESTORE);
}

__attribute__((unused)) static void Test_RestoreMissing()
{
    int ret = unlink(TEST_FILE_PATH);
    LE_TEST_ASSERT((ret == 0) || (errno == ENOENT),
                   "Delete only primary file for %s",
                   OP_RESTORE_MISSING);

    Verify_File(TEST_FILE_PATH, OP_RESTORE_MISSING);
}

__attribute__((unused)) static void Test_RoOpenHealsBackup()
{
    char fullBackupPath[512] = {0};
    GetBackupPath(fullBackupPath, sizeof(fullBackupPath));

    CorruptBackupMd5Xattr();

    int fd = taf_rfs_Open(TEST_FILE_PATH, O_RDONLY, 0);
    LE_TEST_ASSERT(fd >= 0, "Test taf_rfs_Open for %s", OP_RO_OPEN_HEALS_BACKUP);
    LE_TEST_ASSERT(taf_rfs_Close(fd) == 0, "Test taf_rfs_Close for %s", OP_RO_OPEN_HEALS_BACKUP);

    char backupMd5Buf[64] = {0};
    ssize_t backupLen = getxattr(fullBackupPath, TEST_XATTR_NAME, backupMd5Buf, sizeof(backupMd5Buf));
    LE_TEST_ASSERT(backupLen > 0, "Backup MD5 xattr is restored during %s", OP_RO_OPEN_HEALS_BACKUP);

    char primaryMd5Buf[64] = {0};
    ssize_t primaryLen = getxattr(TEST_FILE_PATH, TEST_XATTR_NAME, primaryMd5Buf, sizeof(primaryMd5Buf));
    LE_TEST_ASSERT(primaryLen > 0, "Primary MD5 xattr exists during %s", OP_RO_OPEN_HEALS_BACKUP);

    LE_TEST_ASSERT(strcmp(backupMd5Buf, primaryMd5Buf) == 0,
                   "Backup MD5 xattr matches primary during %s",
                   OP_RO_OPEN_HEALS_BACKUP);
}

__attribute__((unused)) static void Test_BackupMd5MissingRecover()
{
    char fullBackupPath[512] = {0};
    GetBackupPath(fullBackupPath, sizeof(fullBackupPath));

    RemoveBackupMd5Xattr();

    int fd = taf_rfs_Open(TEST_FILE_PATH, O_RDONLY, 0);
    LE_TEST_ASSERT(fd >= 0, "Test taf_rfs_Open for %s", OP_BACKUP_MD5_MISSING_RECOVER);
    LE_TEST_ASSERT(taf_rfs_Close(fd) == 0, "Test taf_rfs_Close for %s", OP_BACKUP_MD5_MISSING_RECOVER);

    char md5Buf[64] = {0};
    ssize_t len = getxattr(fullBackupPath, TEST_XATTR_NAME, md5Buf, sizeof(md5Buf));
    LE_TEST_ASSERT(len > 0, "Backup MD5 xattr is restored during %s", OP_BACKUP_MD5_MISSING_RECOVER);
}

__attribute__((unused)) static void Test_BackupMd5MismatchRecover()
{
    char fullBackupPath[512] = {0};
    GetBackupPath(fullBackupPath, sizeof(fullBackupPath));

    CorruptBackupMd5Xattr();

    int fd = taf_rfs_Open(TEST_FILE_PATH, O_RDONLY, 0);
    LE_TEST_ASSERT(fd >= 0, "Test taf_rfs_Open for %s", OP_BACKUP_MD5_MISMATCH_RECOVER);
    LE_TEST_ASSERT(taf_rfs_Close(fd) == 0, "Test taf_rfs_Close for %s", OP_BACKUP_MD5_MISMATCH_RECOVER);

    char md5Buf[64] = {0};
    ssize_t len = getxattr(fullBackupPath, TEST_XATTR_NAME, md5Buf, sizeof(md5Buf));
    LE_TEST_ASSERT(len > 0, "Backup MD5 xattr is restored during %s", OP_BACKUP_MD5_MISMATCH_RECOVER);
}

__attribute__((unused)) static void Test_BackupInvalidOpenFail()
{
    Simulate_Corrupt();
    CorruptBackupFile();

    int fd = taf_rfs_Open(TEST_FILE_PATH, O_RDONLY, 0);
    LE_TEST_ASSERT(fd < 0, "Test taf_rfs_Open fails for %s", OP_BACKUP_INVALID);
}

__attribute__((unused)) static void Test_CloseUpdatesBackup()
{
    char fullBackupPath[512] = {0};
    GetBackupPath(fullBackupPath, sizeof(fullBackupPath));

    RemoveBackupMd5Xattr();

    int fd = taf_rfs_Open(TEST_FILE_PATH, O_RDWR, 0);
    LE_TEST_ASSERT(fd >= 0, "Test taf_rfs_Open for %s", OP_CLOSE_UPDATES_BACKUP);
    LE_TEST_ASSERT(taf_rfs_Close(fd) == 0, "Test taf_rfs_Close for %s", OP_CLOSE_UPDATES_BACKUP);

    char md5Buf[64] = {0};
    ssize_t len = getxattr(fullBackupPath, TEST_XATTR_NAME, md5Buf, sizeof(md5Buf));
    LE_TEST_ASSERT(len > 0, "Backup MD5 xattr updated during %s", OP_CLOSE_UPDATES_BACKUP);
}

__attribute__((unused)) static void Test_Delete()
{
    taf_rfs_Delete(TEST_FILE_PATH);
    struct stat st;
    LE_TEST_ASSERT(stat(TEST_FILE_PATH, &st) == -1, "Test taf_rfs_Delete %s", TEST_FILE_PATH);

    taf_rfs_Delete(TEST_COPY_PATH);
    LE_TEST_ASSERT(stat(TEST_COPY_PATH, &st) == -1, "Test taf_rfs_Delete %s", TEST_COPY_PATH);

    taf_rfs_Delete(TEST_RENAME_PATH);
    LE_TEST_ASSERT(stat(TEST_RENAME_PATH, &st) == -1, "Test taf_rfs_Delete %s", TEST_RENAME_PATH);
}

// This function simulates corrupting file
__attribute__((unused)) static void Simulate_Corrupt()
{
    srand((unsigned)time(NULL));

    FILE* file = fopen(TEST_FILE_PATH, "r+b");
    if (file == NULL)
    {
        perror("Failed to open file");
        return;
    }

    fseek(file, rand() % TEST_DATA_SIZE, SEEK_SET);

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

__attribute__((unused)) static void Test_Copy()
{
    LE_TEST_ASSERT(taf_rfs_Copy(TEST_FILE_PATH, TEST_COPY_PATH) == 0, "Test taf_rfs_Copy");
    Verify_File(TEST_COPY_PATH, OP_COPY);
}

__attribute__((unused)) static void Test_Rename()
{
    LE_TEST_ASSERT(taf_rfs_Rename(TEST_FILE_PATH, TEST_RENAME_PATH) == 0, "Test taf_rfs_Rename");
    Verify_File(TEST_RENAME_PATH, OP_RENAME);
}

static void ProcessTest
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_UNUSED(param2Ptr);
    TestRequest_t* requestPtr = (TestRequest_t*)param1Ptr;

    switch (requestPtr->op)
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
        case COPY:
            Test_Copy();
            break;
        case RENAME:
            Test_Rename();
            break;
        case RESTORE:
            Test_Restore();
            break;
        case RESTORE_MISSING:
            Test_RestoreMissing();
            break;
        case BACKUP_INVALID:
            Test_BackupInvalidOpenFail();
            break;
        case RO_OPEN_HEALS_BACKUP:
            Test_RoOpenHealsBackup();
            break;
        case BACKUP_MD5_MISSING_RECOVER:
            Test_BackupMd5MissingRecover();
            break;
        case BACKUP_MD5_MISMATCH_RECOVER:
            Test_BackupMd5MismatchRecover();
            break;
        case CLOSE_UPDATES_BACKUP:
            Test_CloseUpdatesBackup();
            break;
        default:
            LE_ERROR("Unknown operation");
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
 * Useful operations:
 *   restore                : primary corrupted, valid backup restores primary
 *   restore_missing        : primary missing, valid backup restores primary
 *   backup_invalid         : primary corrupted, invalid backup makes open fail
 *   ro_open_heals_backup   : RO open + close heals backup MD5 xattr mismatch from valid primary
 *   backup_md5_missing_recover   : RO open + close heals missing backup MD5 xattr from valid primary
 *   backup_md5_mismatch_recover   : RO open + close heals backup MD5/content mismatch from valid primary
 *   close_updates_backup   : close updates backup xattr/content from primary
 *
 * If no <operation> is input, test app will run the default regression sequence.
 */
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("=== telaf RFS test BEGIN ===");

    TestRequestPool = le_mem_CreatePool("Test Request", sizeof(TestRequest_t));
    sem_TestItem = le_sem_Create("TestItemSem", 0);
    TestThreadRef = le_thread_Create("Background Test Thread", TestThread, NULL);
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
        else if (strcmp(operation, OP_COPY) == 0)
        {
            requestPtr->op = COPY;
        }
        else if (strcmp(operation, OP_RENAME) == 0)
        {
            requestPtr->op = RENAME;
        }
        else if (strcmp(operation, OP_RESTORE) == 0)
        {
            requestPtr->op = RESTORE;
        }
        else if (strcmp(operation, OP_RESTORE_MISSING) == 0)
        {
            requestPtr->op = RESTORE_MISSING;
        }
        else if (strcmp(operation, OP_BACKUP_INVALID) == 0)
        {
            requestPtr->op = BACKUP_INVALID;
        }
        else if (strcmp(operation, OP_RO_OPEN_HEALS_BACKUP) == 0)
        {
            requestPtr->op = RO_OPEN_HEALS_BACKUP;
        }
        else if (strcmp(operation, OP_BACKUP_MD5_MISSING_RECOVER) == 0)
        {
            requestPtr->op = BACKUP_MD5_MISSING_RECOVER;
        }
        else if (strcmp(operation, OP_BACKUP_MD5_MISMATCH_RECOVER) == 0)
        {
            requestPtr->op = BACKUP_MD5_MISMATCH_RECOVER;
        }
        else if (strcmp(operation, OP_CLOSE_UPDATES_BACKUP) == 0)
        {
            requestPtr->op = CLOSE_UPDATES_BACKUP;
        }
        else
        {
            LE_ERROR("Invalid operation");
        }

        if (le_arg_NumArgs() > 1)
        {
            const char* path = le_arg_GetArg(1);

            if (path != NULL && strlen(path) > 0)
            {
                le_result_t res = taf_rfs_SetBackupStorage(path);
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

        for (uint i = 0; i < (sizeof(testSequence) / sizeof(testSequence[0])); i++)
        {
            TestRequest_t* requestPtr = (TestRequest_t*)le_mem_ForceAlloc(TestRequestPool);
            requestPtr->op = testSequence[i];
            le_event_QueueFunctionToThread(TestThreadRef, ProcessTest, requestPtr, NULL);

            LE_ASSERT_OK(WaitForSem_Timeout(sem_TestItem, TIMEOUT_ITEM_TEST));
        }

        LE_INFO("All tests passed with data size: %d", TEST_DATA_SIZE);
    }

    LE_TEST_EXIT;
}