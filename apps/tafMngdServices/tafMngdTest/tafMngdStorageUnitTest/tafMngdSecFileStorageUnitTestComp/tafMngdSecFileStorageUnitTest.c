/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <string.h>
#include <stdio.h>

#define FILE_TARGET "targetFile.txt"
#define FILE_SOURCE "/tmp/sourceFile.txt"
#define TEST_STORAGE_NAME "testStorage"

// Function prototypes for the test operations
void Test_Op_create(const char* storageName);
void Test_Op_Write(const char* storageName, const char* data);
void Test_Op_Read(const char* storageName);
void Test_Op_Delete(const char* storageName);
void PrintUsage();

// Test function for taf_mngdStorSecFile_CreateStorage
void Test_CreateStorage()
{
    le_result_t result;
    const char* storageName = TEST_STORAGE_NAME;
    taf_mngdStorSecFile_ManagedCapMask_t capMask = 0x01; // Example capability mask

    result = taf_mngdStorSecFile_CreateStorage(storageName, capMask);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_CreateStorage");
}

// Test function for taf_mngdStorSecFile_GetStorageRef
void Test_GetStorageRef()
{
    taf_mngdStorSecFile_StorageRef_t storageRef;
    const char* storageName = TEST_STORAGE_NAME;

    storageRef = taf_mngdStorSecFile_GetStorageRef(storageName);
    LE_TEST_ASSERT(storageRef != NULL, "Test taf_mngdStorSecFile_GetStorageRef");
}

// Test function for taf_mngdStorSecFile_UnlockStorage
void Test_UnlockStorage()
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(TEST_STORAGE_NAME);

    le_result_t result = taf_mngdStorSecFile_UnlockStorage(storageRef);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_UnlockStorage");
}

// Test function for taf_mngdStorSecFile_LockStorage
void Test_LockStorage()
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(TEST_STORAGE_NAME);

    le_result_t result = taf_mngdStorSecFile_LockStorage(storageRef);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_LockStorage");
}

// Test function for taf_mngdStorSecFile_ImportFile
void Test_ImportFile()
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(TEST_STORAGE_NAME);
    const char* sourceFilePath = FILE_SOURCE;
    const char* targetFilePath = FILE_TARGET;

    // Create a source file for testing
    FILE* sourceFile = fopen(sourceFilePath, "w");
    if (sourceFile)
    {
        fputs("This is a test file.", sourceFile);
        fclose(sourceFile);
    }

    le_result_t result = taf_mngdStorSecFile_ImportFile(storageRef, sourceFilePath, targetFilePath);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_ImportFile");
}

// Test function for taf_mngdStorSecFile_ReadFile
void Test_ReadFile()
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(TEST_STORAGE_NAME);
    const char* filePath = FILE_TARGET;
    uint8_t buffer[1024];
    size_t bufferSize = sizeof(buffer);

    le_result_t result = taf_mngdStorSecFile_ReadFile(storageRef, filePath, buffer, &bufferSize);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_ReadFile");

    // Print the content of the buffer
    printf("Read content: %.*s\n", (int)bufferSize, buffer);
}

// Test function for taf_mngdStorSecFile_DeleteFile
void Test_DeleteFile()
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(TEST_STORAGE_NAME);
    const char* filePath = FILE_TARGET;

    le_result_t result = taf_mngdStorSecFile_DeleteFile(storageRef, filePath);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_DeleteFile");
}

// Test function for taf_mngdStorSecFile_GetBasePath
void Test_GetBasePath()
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(TEST_STORAGE_NAME);
    char basePath[256];
    size_t pathSize = sizeof(basePath);

    le_result_t result = taf_mngdStorSecFile_GetBasePath(storageRef, basePath, pathSize);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_GetBasePath");
}

void PrintUsage()
{
    puts("\n"
         "-------- To do unit test automatically --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest \n"
         "\n"
         "-------- To create file storage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest -- create <storageName>\n"
         "\n"
         "-------- To lock the specified storage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest -- lock <storageName>\n"
         "\n"
         "-------- To Unlock the specified storage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest -- unlock <storageName>\n"
         "\n"
         "-------- Import a file to the storage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest -- import <storageName> <filepath>\n"
         "\n"
         "-------- To Read the file from the storage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest -- read <storageName>\n"
         "\n"
         "-------- To Read the file from the storage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest -- delete <storageName>\n"
         "\n");
}

// Implementations of the test operations
void Test_Op_create(const char* storageName)
{
    le_result_t result = taf_mngdStorSecFile_CreateStorage(storageName, 0x01); // Example capability mask
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_CreateStorage");
}

void Test_Op_Write(const char* storageName, const char* data)
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(storageName);
    LE_TEST_ASSERT(storageRef != NULL, "Test taf_mngdStorSecFile_GetStorageRef");

    // Create a source file for testing
    FILE* sourceFile = fopen(data, "w");
    if (sourceFile)
    {
        fputs("This is a test file.", sourceFile);
        fclose(sourceFile);
    }

    le_result_t result = taf_mngdStorSecFile_ImportFile(storageRef, data, FILE_TARGET);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_ImportFile");
}

void Test_Op_Read(const char* storageName)
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(storageName);
    LE_TEST_ASSERT(storageRef != NULL, "Test taf_mngdStorSecFile_GetStorageRef");

    uint8_t buffer[1024];
    size_t bufferSize = sizeof(buffer);
    le_result_t result = taf_mngdStorSecFile_ReadFile(storageRef, FILE_TARGET, buffer, &bufferSize);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_ReadFile");

    // Print the content of the buffer
    printf("Read content: %.*s\n", (int)bufferSize, buffer);
}

void Test_Op_Delete(const char* storageName)
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(storageName);
    LE_TEST_ASSERT(storageRef != NULL, "Test taf_mngdStorSecFile_GetStorageRef");

    const char* filePath = FILE_TARGET;
    le_result_t result = taf_mngdStorSecFile_DeleteFile(storageRef, filePath);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_DeleteFile");
}

void Test_Op_Lock(const char* storageName)
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(storageName);
    le_result_t result = taf_mngdStorSecFile_LockStorage(storageRef);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_LockStorage");
}

void Test_Op_Unlock(const char* storageName)
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(storageName);
    le_result_t result = taf_mngdStorSecFile_UnlockStorage(storageRef);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_UnlockStorage");
}

COMPONENT_INIT
{
    const char* filePath = FILE_SOURCE;
    FILE* targetFile = fopen(filePath, "w");
    if (targetFile)
    {
        fputs("This is a test file for subsequent operations.\n", targetFile);
        fclose(targetFile);
    }
    else
    {
        LE_ERROR("Failed to create the target file for testing.");
        exit(EXIT_FAILURE);
    }

    if (le_arg_NumArgs() > 1)
    {
        const char* operation = le_arg_GetArg(0);
        char* op_storage = NULL;
        char* op_data = NULL;

        if (NULL == operation)
        {
            LE_ERROR("operation is NULL");
            exit(EXIT_FAILURE);
        }

        const char* label = le_arg_GetArg(1);

        if(label == NULL || strlen(label) == 0 )
        {
            LE_ERROR("Invalid data label");
            exit(EXIT_FAILURE);
        }
        op_storage = (char*)label;

        if (le_arg_NumArgs() > 2)
        {
            const char* data = le_arg_GetArg(2);

            if(data ==NULL || strlen(data) == 0 )
            {
                LE_ERROR("Invalid data label");
                exit(EXIT_FAILURE);
            }
            op_data = (char*)data;
        }

        if(op_storage == NULL || strlen(op_storage) == 0)
        {
            LE_ERROR("Invalid operation");
            exit(EXIT_FAILURE);
        }

        if (strcmp(operation, "help") == 0)
        {
            PrintUsage();
        }
        else if (strcmp(operation, "create") == 0)
        {
            Test_Op_create(op_storage);
        }
        else if (strcmp(operation, "import") == 0)
        {
            if(op_data != NULL)
            {
                Test_Op_Write(op_storage, op_data);
            }
            else
            {
                LE_ERROR("Data is required for write operation");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(operation, "read") == 0)
        {
            Test_Op_Read(op_storage);
        }
        else if (strcmp(operation, "delete") == 0)
        {
            Test_Op_Delete(op_storage);
        }
        else if (strcmp(operation, "lock") == 0)
        {
            Test_Op_Lock(op_storage);
        }
        else if (strcmp(operation, "unlock") == 0)
        {
            Test_Op_Unlock(op_storage);
        }
        else
        {
            LE_ERROR("Invalid operation");
            PrintUsage();
            exit(EXIT_FAILURE);
        }
    }
    else if (le_arg_NumArgs() == 0)
    {
        LE_TEST_PLAN(LE_TEST_NO_PLAN);

        LE_TEST_INFO("=== TelAF MngdStorage unit test BEGIN ===");

        LE_TEST_INFO("=== Test secure storage management ===");
        Test_CreateStorage();
        Test_GetStorageRef();
        Test_UnlockStorage();
        Test_LockStorage();
        Test_ImportFile();
        Test_ReadFile();
        Test_DeleteFile();
        Test_GetBasePath();

        LE_TEST_INFO("=== TelAF MngdStorage unit test END ===");
    }
    else
    {
        LE_ERROR("Invalid operation");
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    LE_TEST_EXIT;
}

