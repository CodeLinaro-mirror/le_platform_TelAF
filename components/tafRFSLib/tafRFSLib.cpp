/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafRFSLib.h"

#include "legato.h"
#include "limit.h"

#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef LE_CONFIG_ENABLE_SELINUX
#include <selinux/selinux.h>
#endif
#include <sys/xattr.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/evp.h>

// Defines RFS backup file size
#ifndef RFS_MAX_BACKUP_FILE_SIZE
#define RFS_MAX_BACKUP_FILE_SIZE 10240
#endif

// Defines RFS backup file number
#ifndef RFS_MAX_BACKUP_FILE_COUNT
#define RFS_MAX_BACKUP_FILE_COUNT 100
#endif

#define RFS_COPY_BUFFER_SIZE 8192

// RFS file backup storage
#define RFS_STORAGE "/data/rfs/"
#define RFS_BACKUP_STORAGE RFS_STORAGE"backup/"
#define RFS_MAX_BACKUP_FILENAME 1024
#define RFS_FILE_EXTENDED_ATTR_MD5 "security.md5"

// backup configuration
static bool enableBackup = false;
static char appBackupStorage[LIMIT_MAX_PATH_BYTES] = {0};
static taf_rfs_ErrorHandler_t errCB = nullptr;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for RFS events
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t ErrorEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Error message structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_rfs_Error_t error;
    char filePath[LIMIT_MAX_PATH_BYTES];
}
RFS_ErrorMsg_t;

//--------------------------------------------------------------------------------------------------
/**
 * Error message structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t maxFileSize;
    uint16_t maxFileCount;
}
RFS_BackupStorageLimit_t;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for RFS events
 */
//--------------------------------------------------------------------------------------------------
static RFS_BackupStorageLimit_t BackupStorageCheck;

//--------------------------------------------------------------------------------------------------
/**
 * Count files in the backup folder
 */
//--------------------------------------------------------------------------------------------------

uint16_t CountFiles(const char *path)
{
    int32_t file_count = 0;
    struct dirent *entry;
    DIR *dp;

    dp = opendir(path);
    if (dp == NULL)
    {
        LE_ERROR("Failed to open directory \"%s\": %s\n", path, strerror(errno));
        return 0;
    }

    while ((entry = readdir(dp)) != NULL)
    {
        // check if it is a file
        if (entry->d_type == DT_REG)
        {
            file_count++;
        }
    }

    closedir(dp);
    return file_count;
}

//--------------------------------------------------------------------------------------------------
/**
 * Calculates MD5 for a specified file.
 */
//--------------------------------------------------------------------------------------------------
static void CalculateFileMD5(const char* filePath, char* md5Str, size_t md5StrSize)
{
    LE_DEBUG("%s", __FUNCTION__);

    if (md5StrSize < (MD5_DIGEST_LENGTH * 2) + 1)
    {
        LE_ERROR("Output buffer is too small for MD5 hash.\n");
        return;
    }

    FILE* file = fopen(filePath, "rb");
    if (!file) {
        LE_ERROR("Failed to open file");
        return;
    }

    EVP_MD_CTX* mdCtx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_md5();
    unsigned char mdValue[EVP_MAX_MD_SIZE];
    unsigned int mdLen, i;

    if (mdCtx == NULL || !EVP_DigestInit_ex(mdCtx, md, NULL))
    {
        LE_ERROR("Digest initialization failed.\n");
        fclose(file);
        EVP_MD_CTX_free(mdCtx);
        return;
    }

    // Read the file and update the digest
    unsigned char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        if (!EVP_DigestUpdate(mdCtx, buffer, bytesRead)) {
            LE_ERROR("Digest update failed.\n");
            fclose(file);
            EVP_MD_CTX_free(mdCtx);
            return;
        }
    }

    if (!EVP_DigestFinal_ex(mdCtx, mdValue, &mdLen))
    {
        LE_ERROR("Digest finalization failed.\n");
        fclose(file);
        EVP_MD_CTX_free(mdCtx);
        return;
    }

    // Convert the hash to a hex string
    for (i = 0; i < mdLen; i++)
    {
        snprintf(&(md5Str[i*2]), 3, "%02x", mdValue[i]);
    }

    fclose(file);
    EVP_MD_CTX_free(mdCtx);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets file MD5 to its file extended attribute
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetFileMD5ToExtendedAttr(const char* filePath)
{
    LE_DEBUG("%s", __FUNCTION__);

    char md5Str[(MD5_DIGEST_LENGTH * 2) + 1];
    CalculateFileMD5(filePath, md5Str, sizeof(md5Str));

    // write MD5 to the file extension attribute
    if (setxattr(filePath, RFS_FILE_EXTENDED_ATTR_MD5, md5Str, strlen(md5Str), 0) < 0)
    {
        LE_ERROR("Failed to set MD5 attribute: %s", strerror(errno));

        RFS_ErrorMsg_t errMsg;
        errMsg.error = RFS_ERR_SET_HASH;
        snprintf(errMsg.filePath, sizeof(errMsg.filePath), "%s", filePath);
        le_event_Report(ErrorEventId, (void*)&errMsg, sizeof(RFS_ErrorMsg_t));

        return LE_FAULT;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Calculates file path SHA1
 */
//--------------------------------------------------------------------------------------------------
static void CalculateFilePathSHA1(const char* filePath, char* outputHash)
{
    LE_DEBUG("%s", __FUNCTION__);

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)filePath, strlen(filePath), hash);
    // transfer SHA1 to a hex string
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        snprintf(outputHash + (i * 2), sizeof(outputHash), "%02x", hash[i]);
    }
    outputHash[SHA_DIGEST_LENGTH * 2] = '\0';
}

//--------------------------------------------------------------------------------------------------
/**
 * Restores file from backup storage
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReplaceFileWithBackup(const char* filePath)
{
    LE_DEBUG("%s", __FUNCTION__);

    char sha1Hash[SHA_DIGEST_LENGTH * 2 + 1];
    CalculateFilePathSHA1(filePath, sha1Hash);

    char backupPath[RFS_MAX_BACKUP_FILENAME];

    if(strlen(appBackupStorage) > 0)
    {
        snprintf(backupPath, sizeof(backupPath), "%s%s", appBackupStorage, sha1Hash);
    }
    else
    {
        snprintf(backupPath, sizeof(backupPath), "%s%s", RFS_BACKUP_STORAGE, sha1Hash);
    }

    LE_INFO("replace %s with %s", filePath, backupPath);

    int inputFd = open(backupPath, O_RDONLY);
    if (inputFd == -1)
    {
        LE_ERROR("Failed to open backup file");
        return LE_FAULT;
    }

    struct stat stat_buf;
    if (fstat(inputFd, &stat_buf) == -1)
    {
        LE_ERROR("Failed to get backup file size");
        close(inputFd);
        return LE_FAULT;
    }

    int outputFd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, stat_buf.st_mode);
    if (outputFd == -1)
    {
        LE_ERROR("Failed to open original file for writing");
        close(inputFd);
        return LE_FAULT;
    }

    off_t offset = 0;
    ssize_t result = sendfile(outputFd, inputFd, &offset, stat_buf.st_size);
    if (result == -1)
    {
        LE_ERROR("Failed to replace original file with backup");
        close(inputFd);
        close(outputFd);
        return LE_FAULT;
    }

#ifdef LE_CONFIG_ENABLE_SELINUX
    char* selinuxContext = NULL;

    // obtain and set selinux context
    if (getfilecon(backupPath, &selinuxContext) >= 0)
    {
        if (setfilecon(filePath, selinuxContext) < 0)
        {
            LE_ERROR("Failed to set SELinux context on the backup file");
        }
        freecon(selinuxContext);
    }
#endif

    close(inputFd);
    close(outputFd);

    SetFileMD5ToExtendedAttr(filePath);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Backs up file and selinux context
 */
//--------------------------------------------------------------------------------------------------
static le_result_t BackUpFileAndSELinuxContext(const char* sourcePath, const char* targetPath)
{
    LE_DEBUG("%s", __FUNCTION__);

    LE_DEBUG("sourcePath: %s, targetPath: %s", sourcePath, targetPath);
    int inputFd, outputFd;
    struct stat stat_buf;
    off_t offset = 0;
    ssize_t sent;
    #ifdef LE_CONFIG_ENABLE_SELINUX
    char* selinuxContext = NULL;
    #endif


    inputFd = open(sourcePath, O_RDONLY);
    if (inputFd < 0)
    {
        LE_ERROR("Failed to open source file for copying");
        return LE_FAULT;
    }

    if (fstat(inputFd, &stat_buf) < 0)
    {
        LE_ERROR("Failed to get file size for copying");
        close(inputFd);
        return LE_FAULT;
    }

    if ((uint64_t)stat_buf.st_size > BackupStorageCheck.maxFileSize)
    {
        LE_ERROR("Failed to get file size for copying");
        close(inputFd);

        RFS_ErrorMsg_t errMsg;
        errMsg.error = RFS_ERR_FILE_TOO_LARGE;
        snprintf(errMsg.filePath, sizeof(errMsg.filePath), "%s", sourcePath);
        le_event_Report(ErrorEventId, (void*)&errMsg, sizeof(RFS_ErrorMsg_t));

        return LE_OUT_OF_RANGE;
    }

    outputFd = open(targetPath, O_WRONLY | O_CREAT | O_TRUNC, stat_buf.st_mode);
    if (outputFd < 0)
    {
        LE_ERROR("Failed to open target file for copying");
        close(inputFd);
        return LE_FAULT;
    }

    sent = sendfile(outputFd, inputFd, &offset, stat_buf.st_size);
    if (sent < 0)
    {
        LE_ERROR("Failed to copy file");
        close(inputFd);
        close(outputFd);
        return LE_FAULT;
    }
#ifdef LE_CONFIG_ENABLE_SELINUX
    // obtain and set selinux context
    if (getfilecon(sourcePath, &selinuxContext) >= 0)
    {
        if (setfilecon(targetPath, selinuxContext) < 0)
        {
            LE_ERROR("Failed to set SELinux context on the backup file");
        }
        freecon(selinuxContext);
    }
#endif
    close(inputFd);
    close(outputFd);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Backs up file to storage
 */
//--------------------------------------------------------------------------------------------------
static le_result_t BackupFileToStorage
(
    const char* filePath
)
{
    LE_DEBUG("----- %s -----", __FUNCTION__);

    char backupDir[RFS_MAX_BACKUP_FILENAME - (SHA_DIGEST_LENGTH * 2 + 1)];

    if(strlen(appBackupStorage) > 0)
    {
        snprintf(backupDir, sizeof(backupDir), "%s", appBackupStorage);
    }
    else
    {
        snprintf(backupDir, sizeof(backupDir), "%s", RFS_BACKUP_STORAGE);
    }

    if(CountFiles(backupDir) > BackupStorageCheck.maxFileCount)
    {
        RFS_ErrorMsg_t errMsg;
        errMsg.error = RFS_ERR_NO_MEMORY;
        snprintf(errMsg.filePath, sizeof(errMsg.filePath), "%s", filePath);

        le_event_Report(ErrorEventId, (void*)&errMsg, sizeof(RFS_ErrorMsg_t));

        return LE_NO_MEMORY;
    }

    // Calculate SHA1 as the backup file name
    char sha1Hash[SHA_DIGEST_LENGTH * 2 + 1];
    CalculateFilePathSHA1(filePath, sha1Hash);

    char backupPath[RFS_MAX_BACKUP_FILENAME];
    snprintf(backupPath, sizeof(backupPath), "%s%s", backupDir, sha1Hash);

    // back up file
    if (BackUpFileAndSELinuxContext(filePath, backupPath) != LE_OK)
    {
        // trigger error event
        LE_ERROR("Failed to backup file");

        RFS_ErrorMsg_t errMsg;
        errMsg.error = RFS_ERR_BACKUP;
        snprintf(errMsg.filePath, sizeof(errMsg.filePath), "%s", filePath);

        le_event_Report(ErrorEventId, (void*)&errMsg, sizeof(RFS_ErrorMsg_t));
    }
    LE_DEBUG("----- %s finished -----", __FUNCTION__);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes backup file from storage
 */
//--------------------------------------------------------------------------------------------------
static void DeleteBackup
(
    const char* filePath
)
{
    LE_DEBUG("%s", __FUNCTION__);

    char sha1Hash[SHA_DIGEST_LENGTH * 2 + 1];
    CalculateFilePathSHA1(filePath, sha1Hash);
    char backupPath[RFS_MAX_BACKUP_FILENAME];

    if(strlen(appBackupStorage) > 0)
    {
        snprintf(backupPath, sizeof(backupPath), "%s%s", appBackupStorage, sha1Hash);
    }
    else
    {
        snprintf(backupPath, sizeof(backupPath), "%s%s", RFS_BACKUP_STORAGE, sha1Hash);
    }

    LE_DEBUG("delete backup: %s", backupPath);
    unlink(backupPath);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a error handler.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessErrorHandler
(
    void* context
)
{
    LE_INFO("%s", __FUNCTION__);

    RFS_ErrorMsg_t* ErrMsg = (RFS_ErrorMsg_t*)context;

    LE_ERROR("Error %d happened for %s", ErrMsg->error, ErrMsg->filePath);

    if(errCB != nullptr)
    {
        errCB(ErrMsg->error, ErrMsg->filePath);
    }
}

extern "C" LE_SHARED le_result_t taf_rfs_Init
(
    bool backup,
    taf_rfs_ErrorHandler_t handlerFunc
)
{
    LE_INFO("%s", __FUNCTION__);
    LE_INFO("backup = %d", backup);

    enableBackup = backup;

    if(enableBackup == false)
    {
        return LE_OK;
    }

    if(handlerFunc != nullptr)
    {
        LE_INFO("Set up error handler");
        errCB = handlerFunc;
    }
       return LE_OK;
}

extern "C" LE_SHARED le_result_t taf_rfs_SetBackupStorage
(
    const char *filePathPtr
)
{
    LE_INFO("%s: %s", __FUNCTION__, filePathPtr);

    if(filePathPtr == NULL || strlen(filePathPtr) == 0)
    {
        return LE_NOT_FOUND;
    }

    struct stat statbuf;
    if(stat(filePathPtr, &statbuf) != 0)
    {
        LE_ERROR("storage: %s doesn't exist", filePathPtr);
        return LE_BAD_PARAMETER;
    }

    if(S_ISDIR(statbuf.st_mode & S_IFMT) == false)
    {
        LE_ERROR("%s already exists but it's not a folder", filePathPtr);
        return LE_BUSY;
    }

    if((strlen(filePathPtr) + 1) > sizeof(appBackupStorage))
    {
        return LE_OVERFLOW;
    }

    snprintf(appBackupStorage, sizeof(appBackupStorage), "%s", filePathPtr);

    size_t len = strlen(appBackupStorage);

    // check if the string includes '/'
    if(appBackupStorage[len - 1] != '/')
    {
        // check if the string size is enough for adding '/'
        if((len + 2) > sizeof(appBackupStorage))
        {
            return LE_OVERFLOW;
        }
        snprintf(appBackupStorage + len, sizeof(appBackupStorage), "/");
    }

    return LE_OK;
}

extern "C" LE_SHARED le_result_t taf_rfs_SetBackupCapacity
(
    uint32_t maxFileSizeBytes,
    uint16_t maxFileCount
)
{
    if(maxFileSizeBytes == 0)
    {
        LE_ERROR("maxFileSizeBytes is 0");
        return LE_BAD_PARAMETER;
    }

    if(maxFileCount == 0)
    {
        LE_ERROR("maxFileCount is 0");
        return LE_BAD_PARAMETER;
    }

    BackupStorageCheck.maxFileSize = maxFileSizeBytes;
    BackupStorageCheck.maxFileCount = maxFileCount;

    LE_INFO("maxFileSizeBytes is %u, maxFileCount is %u", maxFileSizeBytes, maxFileCount);

    return LE_OK;
}

extern "C" LE_SHARED int taf_rfs_Open
(
    const char *filePathPtr,
    int flags,
    mode_t mode
)
{
    LE_DEBUG("%s", __FUNCTION__);
    LE_DEBUG("filePathPtr: %s", filePathPtr);

    if(enableBackup == false)
    {
        return open(filePathPtr, flags, mode);
    }

    struct stat st;
    bool needRestore = false;

    if (stat(filePathPtr, &st) == 0)
    {
        // file exists

        char storedMd5Str[(MD5_DIGEST_LENGTH * 2) + 1] = {0};
        ssize_t len = getxattr(filePathPtr, RFS_FILE_EXTENDED_ATTR_MD5, storedMd5Str, sizeof(storedMd5Str));

        if (len > 0)
        {
            char md5Str[(MD5_DIGEST_LENGTH * 2) + 1] = {0};
            CalculateFileMD5(filePathPtr, md5Str, sizeof(md5Str));

            if (strncmp(storedMd5Str, md5Str, MD5_DIGEST_LENGTH * 2) != 0)
            {
                // MD5 is not matched, restore the file from backup storage
                LE_WARN("MD5 is not mathced, will restore the file");
                needRestore = true;
            }
        }
        else
        {
            // The file doesn't have extended attribute to check hash, ignore it
            LE_DEBUG("Cannot get MD5 from extended attribute: %s", strerror(errno));
        }
    }
    else if (!(flags & O_CREAT))
    {
        LE_ERROR("File does not exist and O_CREAT not specified");
        return -1;
    }

    if (needRestore == true)
    {
        // MD5 is not matched, restore the file from backup storage
        if (ReplaceFileWithBackup(filePathPtr) != LE_OK)
        {
            LE_ERROR("Failed to replace file with backup, pleae check the file integrity");

            RFS_ErrorMsg_t errMsg;
            errMsg.error = RFS_ERR_RESTORE;
            snprintf(errMsg.filePath, sizeof(errMsg.filePath), "%s", filePathPtr);
            le_event_Report(ErrorEventId, (void*)&errMsg, sizeof(RFS_ErrorMsg_t));
        }
    }

    int fd = open(filePathPtr, flags, mode);
    return fd;
}

extern "C" LE_SHARED int taf_rfs_Close
(
    int fd
)
{
    LE_DEBUG("%s", __FUNCTION__);

    if(enableBackup == false)
    {
        return close(fd);
    }

    // use fcntl and F_GETFL to get flags of the fd
    int flags = fcntl(fd, F_GETFL);
    if (flags != -1)
    {
        // check if the fd has write permission
        if (!(flags & O_WRONLY) && !(flags & O_RDWR))
        {
            LE_DEBUG("The fd does not have write permission, close it");
            return close(fd);
        }
    }

    LE_DEBUG("The fd has write permission.");

    // Need to finish and return this function ASAP, push off uneccessary tasks to event process

    char filePath[LIMIT_MAX_PATH_BYTES];

    // construt the path to /proc/[pid]/fd/[fd]
    snprintf(filePath, sizeof(filePath), "/proc/self/fd/%d", fd);

    // get the actual path from the fd
    char actualPath[LIMIT_MAX_PATH_BYTES];
    ssize_t len = readlink(filePath, actualPath, sizeof(actualPath)-1);
    if (len != -1)
    {
        actualPath[len] = '\0'; // ensure the string end with null terminator
        LE_INFO("The file path is: %s\n", actualPath);
    }

    SetFileMD5ToExtendedAttr(actualPath);

    BackupFileToStorage(actualPath);

    return close(fd);
}

extern "C" LE_SHARED int taf_rfs_Read
(
    int fd,
    uint8_t* bufPtr,
    size_t* sizePtr
)
{
    LE_DEBUG("%s", __FUNCTION__);
    return read(fd, bufPtr, *sizePtr);
}

extern "C" LE_SHARED int taf_rfs_Write
(
    int fd,
    const uint8_t* bufPtr,
    size_t sizePtr
)
{
    LE_DEBUG("%s", __FUNCTION__);
    return write(fd, bufPtr, sizePtr);
}

extern "C" LE_SHARED void taf_rfs_Delete
(
    const char* filePathPtr
)
{
    LE_DEBUG("%s", __FUNCTION__);
    LE_DEBUG("filePathPtr: %s", filePathPtr);

    unlink(filePathPtr);

    if(enableBackup == true)
    {
        DeleteBackup(filePathPtr);
    }
}

extern "C" LE_SHARED int taf_rfs_Copy
(
    const char *sourcePath,
    const char *destPath
)
{
    int srcFd, destFd;
    ssize_t bytesRead, bytesWritten;
    uint8_t buffer[RFS_COPY_BUFFER_SIZE];

    // open source file
    srcFd = open(sourcePath, O_RDONLY);
    if (srcFd < 0)
    {
        LE_ERROR("Failed to open source file");
        return errno;
    }

    // open destination file, if it doesn't exist, create it
    destFd = taf_rfs_Open(destPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (destFd < 0)
    {
        LE_ERROR("Failed to open destination file");
        close(srcFd);
        return errno;
    }

    // read source file and write to destination file
    while ((bytesRead = read(srcFd, buffer, RFS_COPY_BUFFER_SIZE)) > 0)
    {
        bytesWritten = taf_rfs_Write(destFd, buffer, bytesRead);
        if (bytesWritten != bytesRead)
        {
            LE_ERROR("Failed to write to destination file");
            close(srcFd);
            close(destFd);
            return errno;
        }
    }

    // error check for read returned value
    if (bytesRead < 0)
    {
        LE_ERROR("Failed to read from source file");
        close(srcFd);
        taf_rfs_Close(destFd);
        return errno;
    }

    close(srcFd);
    taf_rfs_Close(destFd);

    return 0;
}

int taf_rfs_Rename
(
    const char *sourcePath,
    const char *destPath
)
{
    // Try POSIX rename()
    if (rename(sourcePath, destPath) == 0)
    {
        DeleteBackup(sourcePath);

        BackupFileToStorage(destPath);

        return 0;
    }
    else
    {
        // if the errno is EXDEV (cross-filesystem)，try to copy and delete
        if (errno == EXDEV)
        {
            LE_INFO("Cross-filesystem rename detected, attempting copy and delete.");

            int copyResult = taf_rfs_Copy(sourcePath, destPath);
            if (copyResult != 0)
            {
                return copyResult;  // Copy failed, return error
            }

            // Copy succeeded, delete the source file
            taf_rfs_Delete(sourcePath);

            return 0;
        }
        else
        {
            // Other error, return error number
            LE_ERROR("Failed to rename file");
            return errno;
        }
    }
}

COMPONENT_INIT
{
    // initializing

    LE_INFO("COMPONENT_INIT - tafRFSLib");

    //Create the storage for backup files
    struct stat sb;

    // assume RFS_STORAGE dir exists in the system
    if(stat(RFS_STORAGE, &sb) == -1)
    {
        LE_INFO("RFS storage dir does not exist, create it.");

        if(mkdir(RFS_STORAGE, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", RFS_STORAGE);
            exit(-1);
        }

        // go ahead creating the sub dir
        if(mkdir(RFS_BACKUP_STORAGE, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", RFS_BACKUP_STORAGE);
            exit(-1);
        }
    }
    else if((sb.st_mode & S_IFMT) == S_IFDIR)
    {
        // assume the sub dir was created as well. do nothing
        LE_INFO("RFS storage was found!");
    }
    else
    {
        // some other file objects. delete first
        LE_ERROR("Delete the file object, then create the storage");

        // try to delete it
        unlink(RFS_STORAGE);

        // Create the directory
        if(mkdir(RFS_BACKUP_STORAGE, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", RFS_BACKUP_STORAGE);
            exit(-1);
        }

        // go ahead creating the sub dir
        if(mkdir(RFS_BACKUP_STORAGE, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", RFS_BACKUP_STORAGE);
            exit(-1);
        }
    }

    // Create an event Id for error event.
    ErrorEventId = le_event_CreateId("ErrorEventId", sizeof(RFS_ErrorMsg_t));

    // Register handler for error events.
    le_event_AddHandler("ProcessBackupErrHandler",
                            ErrorEventId,
                            ProcessErrorHandler);

    BackupStorageCheck.maxFileSize = RFS_MAX_BACKUP_FILE_SIZE;
    BackupStorageCheck.maxFileCount = RFS_MAX_BACKUP_FILE_COUNT;
}
