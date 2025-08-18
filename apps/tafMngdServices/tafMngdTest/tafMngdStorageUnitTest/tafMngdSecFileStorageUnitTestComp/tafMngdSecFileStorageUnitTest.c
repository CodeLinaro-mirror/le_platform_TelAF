/*
*
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"
#include <string.h>
#include <stdio.h>

#define FILE_TARGET "targetFile.txt"
#define FILE_SOURCE "/tmp/sourceFile.txt"
#define TEST_STORAGE_NAME "testStorage"
#define TEST_SVC_STORAGE_NAME "testSvcStorage"
#define TEST_SVC_STORAGE_NAME_INSTANCE "SvcStorageInstance"

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

    LE_INFO("Read buffer size: %" PRIuS, bufferSize);

    LE_TEST_ASSERT(bufferSize > 0, "Test taf_mngdStorSecFile_ReadFile - bufferSize");

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

// Test function for taf_mngdStorSecFile_DeleteStorage
void Test_DeleteStorage()
{
    taf_mngdStorSecFile_StorageRef_t storageRef;
    const char* storageName = TEST_STORAGE_NAME;

    storageRef = taf_mngdStorSecFile_GetStorageRef(storageName);
    LE_TEST_ASSERT(storageRef != NULL, "Test taf_mngdStorSecFile_GetStorageRef");

    le_result_t result = taf_mngdStorSecFile_DeleteStorage(storageRef);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdtorSecFile_DeleteStorage");
}

void Test_ServiceStorage()
{
    taf_mngdStorSecFile_StorageRef_t storageRef;
    const char* storageName = TEST_SVC_STORAGE_NAME;

    storageRef = taf_mngdStorSecFile_GetStorageRef(storageName);
    LE_TEST_ASSERT(storageRef != NULL, "Test taf_mngdStorSecFile_GetStorageRef");

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

    result = taf_mngdStorSecFile_DeleteFile(storageRef, targetFilePath);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_DeleteFile");

    result = taf_mngdStorSecFile_DeleteStorage(storageRef);
    LE_TEST_ASSERT(result == LE_NOT_PERMITTED, "Test taf_mngdtorSecFile_DeleteStorage");

    storageRef = taf_mngdStorSecFile_GetStorageRef(TEST_SVC_STORAGE_NAME_INSTANCE);
    LE_TEST_ASSERT(storageRef == NULL, "Test taf_mngdStorSecFile_GetStorageRef");
}

void PrintUsage()
{
    puts("\n"
         "-------- To do unit test automatically --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest \n"
         "\n"
         "-------- To create file storage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest -- createstorage <storageName>\n"
         "\n"
         "-------- To lock the specified storage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest -- lockstorage <storageName>\n"
         "\n"
         "-------- To Unlock the specified storage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest -- unlockstorage <storageName>\n"
         "\n"
         "-------- Import a file to the storage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest -- importfile <storageName> <filepath>\n"
         "\n"
         "-------- To Read the file from the storage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest -- readfile <storageName> <filepath>\n"
         "\n"
         "-------- To delete the file from the storage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest -- deletefile <storageName> <filepath>\n"
         "\n"
         "-------- To delete the storage --------\n"
         "app runProc tafMngdStorageUnitTest --exe=tafMngdSecFileStorageUnitTest -- deletestorage <storageName>\n"
         "\n");
}

// Implementations of the test operations
void Test_Op_Create_Storage(const char* storageName)
{
    le_result_t result = taf_mngdStorSecFile_CreateStorage(storageName, 0x01); // Example capability mask
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_CreateStorage");
}

void Test_Op_Import_File(const char* storageName, const char* data, const char* target_path)
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(storageName);
    LE_TEST_ASSERT(storageRef != NULL, "Test taf_mngdStorSecFile_GetStorageRef");
    le_result_t result = taf_mngdStorSecFile_ImportFile(storageRef, data, target_path);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_ImportFile");
}

void Test_Op_Read_File(const char* storageName, const char* filePath)
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(storageName);
    LE_TEST_ASSERT(storageRef != NULL, "Test taf_mngdStorSecFile_GetStorageRef");

    uint8_t buffer[1024];
    size_t bufferSize = sizeof(buffer);
    le_result_t result = taf_mngdStorSecFile_ReadFile(storageRef, filePath, buffer, &bufferSize);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_ReadFile");

    // Print the content of the buffer
    printf("Read content: %.*s\n", (int)bufferSize, buffer);
}

void Test_Op_Delete_File(const char* storageName, const char* filePath)
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(storageName);
    LE_TEST_ASSERT(storageRef != NULL, "Test taf_mngdStorSecFile_GetStorageRef");

    le_result_t result = taf_mngdStorSecFile_DeleteFile(storageRef, filePath);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_DeleteFile");
}

void Test_Op_Lock_Storage(const char* storageName)
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(storageName);
    le_result_t result = taf_mngdStorSecFile_LockStorage(storageRef);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_LockStorage");
}

void Test_Op_Unlock_Storage(const char* storageName)
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(storageName);
    le_result_t result = taf_mngdStorSecFile_UnlockStorage(storageRef);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdStorSecFile_UnlockStorage");
}
void Test_Op_Delete_Storage(const char* storageName)
{
    taf_mngdStorSecFile_StorageRef_t storageRef = taf_mngdStorSecFile_GetStorageRef(storageName);
    le_result_t result = taf_mngdStorSecFile_DeleteStorage(storageRef);
    LE_TEST_ASSERT(result == LE_OK, "Test taf_mngdtorSecFile_DeleteStorage");
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
        char* op_target_path = NULL;

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
        if (le_arg_NumArgs() > 3)
        {
            const char* target_path = le_arg_GetArg(3);

            if(target_path == NULL || strlen(target_path) == 0 )
            {
                LE_ERROR("Invalid target path");
                exit(EXIT_FAILURE);
            }
            op_target_path = (char*)target_path;

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
        else if (strcmp(operation, "createstorage") == 0)
        {
            Test_Op_Create_Storage(op_storage);
        }
        else if (strcmp(operation, "importfile") == 0)
        {
            if(op_data != NULL && op_target_path != NULL)
            {
                Test_Op_Import_File(op_storage, op_data, op_target_path);
            }
            else
            {
                LE_ERROR("Souce and target file paths are required for importing file operation");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(operation, "readfile") == 0)
        {
            if(op_data != NULL)
            {
                Test_Op_Read_File(op_storage,op_data);
            }
            else
            {
                LE_ERROR("File is required for read file operation");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(operation, "deletefile") == 0)
        {
            if(op_data != NULL)
            {
                Test_Op_Delete_File(op_storage,op_data);
            }
             else
            {
                LE_ERROR("File is required for delete file operation");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(operation, "lockstorage") == 0)
        {
            Test_Op_Lock_Storage(op_storage);
        }
        else if (strcmp(operation, "unlockstorage") == 0)
        {
            Test_Op_Unlock_Storage(op_storage);
        }
        else if (strcmp(operation, "deletestorage") == 0)
        {
            Test_Op_Delete_Storage(op_storage);
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
        Test_ServiceStorage();
        Test_CreateStorage();
        Test_GetStorageRef();
        Test_UnlockStorage();
        Test_LockStorage();
        Test_ImportFile();
        Test_ReadFile();
        Test_DeleteFile();
        Test_GetBasePath();
        Test_DeleteStorage();
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
