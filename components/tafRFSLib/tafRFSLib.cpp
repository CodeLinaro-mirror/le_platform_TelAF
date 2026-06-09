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

static le_result_t CalculateFileMD5(const char* filePath, char* md5Str, size_t md5StrSize);
static void CalculateFilePathSHA1(const char* filePath, char* outputHash, size_t outputHashSize);
static le_result_t BackupFileToStorage(const char* filePath);
static le_result_t GetFileMD5FromExtendedAttr(const char* filePath, char* md5Str, size_t md5StrSize);
static le_result_t SetFileMD5ToExtendedAttrValue(const char* filePath, const char* md5Str);
static le_result_t RefreshBackupIfNeeded(const char* filePath, const char* primaryMd5Str);

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
static le_result_t CalculateFileMD5(const char* filePath, char* md5Str, size_t md5StrSize)
{
    LE_DEBUG("%s", __FUNCTION__);

    if (md5StrSize < (MD5_DIGEST_LENGTH * 2) + 1)
    {
        LE_ERROR("Output buffer is too small for MD5 hash.\n");
        return LE_OVERFLOW;
    }

    FILE* file = fopen(filePath, "rb");
    if (!file)
    {
        LE_ERROR("Failed to open file");
        return LE_FAULT;
    }

    EVP_MD_CTX* mdCtx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_md5();
    unsigned char mdValue[EVP_MAX_MD_SIZE];
    unsigned int mdLen = 0;

    if (mdCtx == NULL || !EVP_DigestInit_ex(mdCtx, md, NULL))
    {
        LE_ERROR("Digest initialization failed.\n");
        fclose(file);
        EVP_MD_CTX_free(mdCtx);
        return LE_FAULT;
    }

    unsigned char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        if (!EVP_DigestUpdate(mdCtx, buffer, bytesRead))
        {
            LE_ERROR("Digest update failed.\n");
            fclose(file);
            EVP_MD_CTX_free(mdCtx);
            return LE_FAULT;
        }
    }

    if (ferror(file))
    {
        LE_ERROR("Failed while reading file for MD5 calculation.\n");
        fclose(file);
        EVP_MD_CTX_free(mdCtx);
        return LE_FAULT;
    }

    if (!EVP_DigestFinal_ex(mdCtx, mdValue, &mdLen))
    {
        LE_ERROR("Digest finalization failed.\n");
        fclose(file);
        EVP_MD_CTX_free(mdCtx);
        return LE_FAULT;
    }

    for (unsigned int i = 0; i < mdLen; i++)
    {
        snprintf(&(md5Str[i * 2]), 3, "%02x", mdValue[i]);
    }

    md5Str[mdLen * 2] = '\0';
    fclose(file);
    EVP_MD_CTX_free(mdCtx);
    return LE_OK;
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
    if (CalculateFileMD5(filePath, md5Str, sizeof(md5Str)) != LE_OK)
    {
        LE_ERROR("Failed to calculate MD5 for %s", filePath);
        return LE_FAULT;
    }

    return SetFileMD5ToExtendedAttrValue(filePath, md5Str);
}

static le_result_t GetFileMD5FromExtendedAttr(const char* filePath, char* md5Str, size_t md5StrSize)
{
    ssize_t len = getxattr(filePath, RFS_FILE_EXTENDED_ATTR_MD5, md5Str, md5StrSize);

    if (len <= 0)
    {
        if (errno == ENODATA)
        {
            LE_WARN("MD5 xattr missing for %s", filePath);
        }
        else
        {
            LE_ERROR("Failed to read MD5 xattr for %s: %s", filePath, strerror(errno));
        }
        return LE_FAULT;
    }

    if ((size_t)len >= md5StrSize)
    {
        LE_ERROR("MD5 xattr buffer too small for %s", filePath);
        return LE_FAULT;
    }

    md5Str[len] = '\0';
    return LE_OK;
}

static le_result_t SetFileMD5ToExtendedAttrValue(const char* filePath, const char* md5Str)
{
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
 * Durably persists a single file: its data and metadata (including the MD5 xattr) are flushed
 * to stable storage. This is scoped to the given file only (no global sync), to keep the
 * performance impact minimal while guaranteeing that the content and its integrity attribute
 * are never lost across an unexpected power loss / reboot / suspend.
 *
 * @return LE_OK on success, LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PersistFile(const char* filePath)
{
    int fd = open(filePath, O_RDONLY);
    if (fd < 0)
    {
        LE_ERROR("Persist: failed to open %s: %s", filePath, strerror(errno));
        return LE_FAULT;
    }

    // fsync flushes the file content AND this file's inode metadata, which includes
    // the extended attribute (security.md5). It is scoped to this file only.
    if (fsync(fd) != 0)
    {
        LE_ERROR("Persist: fsync failed for %s: %s", filePath, strerror(errno));
        close(fd);
        return LE_FAULT;
    }

    close(fd);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Durably persists the directory entry of a file (e.g. after create/rename/unlink), so the
 * file name -> inode mapping survives a power loss / reboot. Scoped to the parent directory only.
 *
 * @return LE_OK on success, LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PersistParentDir(const char* filePath)
{
    char dirBuf[LIMIT_MAX_PATH_BYTES] = {0};
    snprintf(dirBuf, sizeof(dirBuf), "%s", filePath);

    char* lastSlash = strrchr(dirBuf, '/');
    if (lastSlash == NULL)
    {
        return LE_OK;
    }

    if (lastSlash == dirBuf)
    {
        // file in root directory
        lastSlash[1] = '\0';
    }
    else
    {
        *lastSlash = '\0';
    }

    int dfd = open(dirBuf, O_RDONLY | O_DIRECTORY);
    if (dfd < 0)
    {
        LE_ERROR("Persist: failed to open dir %s: %s", dirBuf, strerror(errno));
        return LE_FAULT;
    }

    if (fsync(dfd) != 0)
    {
        LE_ERROR("Persist: fsync failed for dir %s: %s", dirBuf, strerror(errno));
        close(dfd);
        return LE_FAULT;
    }

    close(dfd);
    return LE_OK;
}

static le_result_t ValidateFileMD5WithExtendedAttr(const char* filePath)
{
    char storedMd5Str[(MD5_DIGEST_LENGTH * 2) + 1] = {0};

    if (GetFileMD5FromExtendedAttr(filePath, storedMd5Str, sizeof(storedMd5Str)) != LE_OK)
    {
        if (errno == ENODATA)
        {
            LE_WARN("MD5 xattr missing for %s", filePath);
        }
        else
        {
            LE_WARN("MD5 xattr missing or unreadable for %s", filePath);
        }
        return LE_FAULT;
    }

    char md5Str[(MD5_DIGEST_LENGTH * 2) + 1] = {0};
    if (CalculateFileMD5(filePath, md5Str, sizeof(md5Str)) != LE_OK)
    {
        LE_WARN("Failed to calculate MD5 for %s", filePath);
        return LE_FAULT;
    }

    if (strncmp(storedMd5Str, md5Str, MD5_DIGEST_LENGTH * 2) != 0)
    {
        LE_WARN("MD5 xattr/content mismatch for %s", filePath);
        return LE_FAULT;
    }

    return LE_OK;
}

static le_result_t GetBackupPath(const char* filePath, char* backupPath, size_t backupPathSize)
{
    char sha1Hash[SHA_DIGEST_LENGTH * 2 + 1];
    CalculateFilePathSHA1(filePath, sha1Hash, sizeof(sha1Hash));

    if (strlen(appBackupStorage) > 0)
    {
        snprintf(backupPath, backupPathSize, "%s%s", appBackupStorage, sha1Hash);
    }
    else
    {
        snprintf(backupPath, backupPathSize, "%s%s", RFS_BACKUP_STORAGE, sha1Hash);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Calculates file path SHA1
 */
//--------------------------------------------------------------------------------------------------
static void CalculateFilePathSHA1(const char* filePath, char* outputHash, size_t outputHashSize)
{
    LE_DEBUG("%s", __FUNCTION__);

    if (outputHashSize < (SHA_DIGEST_LENGTH * 2) + 1)
    {
        LE_ERROR("Output buffer is too small for SHA1 hash string.");
        if (outputHashSize > 0)
        {
            outputHash[0] = '\0';
        }
        return;
    }

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)filePath, strlen(filePath), hash);
    // transfer SHA1 to a hex string
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        snprintf(outputHash + (i * 2), 3, "%02x", hash[i]);
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

    char backupPath[RFS_MAX_BACKUP_FILENAME];
    GetBackupPath(filePath, backupPath, sizeof(backupPath));

    if (ValidateFileMD5WithExtendedAttr(backupPath) != LE_OK)
    {
        LE_ERROR("Backup file MD5 validation failed for %s", backupPath);
        return LE_FAULT;
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

    // Strip execute bits: secStorage holds inherently non-executable data files,
    // so never propagate any execute permission inherited from the source (CR 4551772).
    mode_t destMode = stat_buf.st_mode & ~(S_IXUSR | S_IXGRP | S_IXOTH);
    int outputFd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, destMode);
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

    // Update the primary file's xattr with its MD5 after restore
    if (SetFileMD5ToExtendedAttr(filePath) != LE_OK)
    {
        LE_ERROR("Failed to set MD5 for restored primary file %s", filePath);
        return LE_FAULT;
    }

    // Durably persist the restored primary file (content + MD5 xattr) and its directory entry,
    // so the restore result survives an unexpected power loss / reboot. Scoped to this file/dir.
    if (PersistFile(filePath) != LE_OK)
    {
        LE_ERROR("Failed to persist restored primary file %s", filePath);
        return LE_FAULT;
    }
    PersistParentDir(filePath);

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

    int inputFd = open(sourcePath, O_RDONLY);
    if (inputFd < 0)
    {
        LE_ERROR("Failed to open source file for copying");
        return LE_FAULT;
    }

    struct stat stat_buf;
    if (fstat(inputFd, &stat_buf) < 0)
    {
        LE_ERROR("Failed to get file size for copying");
        close(inputFd);
        return LE_FAULT;
    }

    // Strip execute bits: the backup of a secStorage data file must not carry
    // execute permission inherited from the source (CR 4551772).
    mode_t destMode = stat_buf.st_mode & ~(S_IXUSR | S_IXGRP | S_IXOTH);
    int outputFd = open(targetPath, O_WRONLY | O_CREAT | O_TRUNC, destMode);
    if (outputFd < 0)
    {
        LE_ERROR("Failed to open target file for copying");
        close(inputFd);
        return LE_FAULT;
    }

    off_t offset = 0;
    ssize_t sent = sendfile(outputFd, inputFd, &offset, stat_buf.st_size);
    if (sent < 0)
    {
        LE_ERROR("Failed to copy file");
        close(inputFd);
        close(outputFd);
        return LE_FAULT;
    }
#ifdef LE_CONFIG_ENABLE_SELINUX
    char* selinuxContext = NULL;
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

    // Durably persist the freshly written backup file and its directory entry, so the backup
    // (and later its MD5 xattr) cannot be lost on an unexpected power loss / reboot.
    if (PersistFile(targetPath) != LE_OK)
    {
        LE_ERROR("Failed to persist backup file %s", targetPath);
        return LE_FAULT;
    }
    PersistParentDir(targetPath);

    return LE_OK;
}

static le_result_t RefreshBackupIfNeeded(const char* filePath, const char* primaryMd5Str)
{
    char backupPath[RFS_MAX_BACKUP_FILENAME];
    char backupMd5Str[(MD5_DIGEST_LENGTH * 2) + 1] = {0};

    if (primaryMd5Str == NULL || primaryMd5Str[0] == '\0')
    {
        LE_ERROR("Primary MD5 is invalid for %s", filePath);
        return LE_BAD_PARAMETER;
    }

    GetBackupPath(filePath, backupPath, sizeof(backupPath));

    if (GetFileMD5FromExtendedAttr(backupPath, backupMd5Str, sizeof(backupMd5Str)) != LE_OK)
    {
        if (errno == ENODATA)
        {
            LE_WARN("Backup MD5 missing for %s, rebuilding from primary %s",
                    backupPath,
                    filePath);
        }
        else
        {
            LE_WARN("Backup MD5 unreadable for %s, rebuilding from primary %s",
                    backupPath,
                    filePath);
        }
        return BackupFileToStorage(filePath);
    }

    if (ValidateFileMD5WithExtendedAttr(backupPath) != LE_OK)
    {
        LE_WARN("Backup MD5/content mismatch for %s, rebuilding from primary %s",
                backupPath,
                filePath);
        return BackupFileToStorage(filePath);
    }

    if (strncmp(primaryMd5Str, backupMd5Str, MD5_DIGEST_LENGTH * 2) != 0)
    {
        LE_INFO("Backup is stale for %s, refreshing from primary %s", backupPath, filePath);
        return BackupFileToStorage(filePath);
    }

    LE_DEBUG("Backup is already valid and up-to-date for %s", filePath);
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

    if(CountFiles(backupDir) >= BackupStorageCheck.maxFileCount)
    {
        RFS_ErrorMsg_t errMsg;
        errMsg.error = RFS_ERR_NO_MEMORY;
        snprintf(errMsg.filePath, sizeof(errMsg.filePath), "%s", filePath);

        le_event_Report(ErrorEventId, (void*)&errMsg, sizeof(RFS_ErrorMsg_t));

        return LE_NO_MEMORY;
    }

    char sha1Hash[SHA_DIGEST_LENGTH * 2 + 1];
    CalculateFilePathSHA1(filePath, sha1Hash, sizeof(sha1Hash));

    char backupPath[RFS_MAX_BACKUP_FILENAME];
    snprintf(backupPath, sizeof(backupPath), "%s%s", backupDir, sha1Hash);

    if (BackUpFileAndSELinuxContext(filePath, backupPath) != LE_OK)
    {
        LE_ERROR("Failed to backup file");

        RFS_ErrorMsg_t errMsg;
        errMsg.error = RFS_ERR_BACKUP;
        snprintf(errMsg.filePath, sizeof(errMsg.filePath), "%s", filePath);

        le_event_Report(ErrorEventId, (void*)&errMsg, sizeof(RFS_ErrorMsg_t));
        return LE_FAULT;
    }

    char primaryMd5Str[(MD5_DIGEST_LENGTH * 2) + 1] = {0};
    if (GetFileMD5FromExtendedAttr(filePath, primaryMd5Str, sizeof(primaryMd5Str)) != LE_OK)
    {
        LE_ERROR("Failed to get MD5 from primary file %s", filePath);
        return LE_FAULT;
    }

    if (SetFileMD5ToExtendedAttrValue(backupPath, primaryMd5Str) != LE_OK)
    {
        LE_ERROR("Failed to set MD5 for backup file %s", backupPath);
        return LE_FAULT;
    }

    // Persist the backup's MD5 xattr to stable storage. This is the critical step that
    // prevents the "backup content present but MD5 xattr missing after reboot" failure.
    if (PersistFile(backupPath) != LE_OK)
    {
        LE_ERROR("Failed to persist backup MD5 xattr for %s", backupPath);
        return LE_FAULT;
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
    CalculateFilePathSHA1(filePath, sha1Hash, sizeof(sha1Hash));
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
    int savedErrno = 0;

    if (stat(filePathPtr, &st) == 0)
    {
        // file exists
        if (ValidateFileMD5WithExtendedAttr(filePathPtr) != LE_OK)
        {
            LE_WARN("Primary file validation failed, will restore from backup");
            needRestore = true;
        }
    }
    else
    {
        savedErrno = errno;
        if (!(flags & O_CREAT) && savedErrno == ENOENT)
        {
            LE_WARN("Primary file missing (ENOENT), will restore from backup");
            needRestore = true;
        }
        else if (!(flags & O_CREAT))
        {
            LE_ERROR("stat failed for %s: %s", filePathPtr, strerror(savedErrno));
            return -1;
        }
    }

    if (needRestore == true)
    {
        // Restore primary before returning fd to caller.
        if (ReplaceFileWithBackup(filePathPtr) != LE_OK)
        {
            LE_ERROR("Failed to replace file with backup, please check the file integrity");

            RFS_ErrorMsg_t errMsg;
            errMsg.error = RFS_ERR_RESTORE;
            snprintf(errMsg.filePath, sizeof(errMsg.filePath), "%s", filePathPtr);
            le_event_Report(ErrorEventId, (void*)&errMsg, sizeof(RFS_ErrorMsg_t));
            return -1;
        }

        // Re-validate restored primary before proceeding.
        if (ValidateFileMD5WithExtendedAttr(filePathPtr) != LE_OK)
        {
            LE_ERROR("Primary validation failed after restore for %s", filePathPtr);

            RFS_ErrorMsg_t errMsg;
            errMsg.error = RFS_ERR_RESTORE;
            snprintf(errMsg.filePath, sizeof(errMsg.filePath), "%s", filePathPtr);
            le_event_Report(ErrorEventId, (void*)&errMsg, sizeof(RFS_ErrorMsg_t));
            return -1;
        }
    }

    char primaryMd5Str[(MD5_DIGEST_LENGTH * 2) + 1] = {0};
    if (GetFileMD5FromExtendedAttr(filePathPtr, primaryMd5Str, sizeof(primaryMd5Str)) == LE_OK)
    {
        if (RefreshBackupIfNeeded(filePathPtr, primaryMd5Str) != LE_OK)
        {
            LE_WARN("Backup refresh/heal failed during open for %s", filePathPtr);
        }
    }
    else
    {
        LE_WARN("Primary MD5 xattr missing or unreadable during open for %s", filePathPtr);
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

    char filePath[LIMIT_MAX_PATH_BYTES];

    // construt the path to /proc/[pid]/fd/[fd]
    snprintf(filePath, sizeof(filePath), "/proc/self/fd/%d", fd);

    // get the actual path from the fd
    char actualPath[LIMIT_MAX_PATH_BYTES] = {0};
    ssize_t len = readlink(filePath, actualPath, sizeof(actualPath)-1);
    if (len == -1)
    {
        LE_ERROR("Failed to get file path from fd %d", fd);
        return close(fd);
    }

    actualPath[len] = '\0'; // ensure the string end with null terminator
    LE_DEBUG("The file path is: %s\n", actualPath);

    // use fcntl and F_GETFL to get flags of the fd before close
    int flags = fcntl(fd, F_GETFL);
    bool hasWritePermission = false;
    if (flags != -1)
    {
        hasWritePermission = ((flags & O_WRONLY) || (flags & O_RDWR));
    }

    int closeResult = close(fd);
    if (closeResult != 0)
    {
        LE_ERROR("Failed to close fd %d for %s", fd, actualPath);
        return closeResult;
    }

    if (hasWritePermission)
    {
        char primaryMd5Str[(MD5_DIGEST_LENGTH * 2) + 1] = {0};

        if (CalculateFileMD5(actualPath, primaryMd5Str, sizeof(primaryMd5Str)) != LE_OK)
        {
            LE_ERROR("Failed to calculate MD5 for primary file %s", actualPath);
            return -1;
        }

        if (SetFileMD5ToExtendedAttrValue(actualPath, primaryMd5Str) != LE_OK)
        {
            LE_ERROR("Failed to update MD5 for primary file %s", actualPath);
            return -1;
        }

        // Persist the primary file's content + MD5 xattr to stable storage immediately, so a
        // power loss / reboot right after this write cannot leave "content present, xattr lost".
        if (PersistFile(actualPath) != LE_OK)
        {
            LE_ERROR("Failed to persist primary file %s", actualPath);
            return -1;
        }

        if (RefreshBackupIfNeeded(actualPath, primaryMd5Str) != LE_OK)
        {
            LE_ERROR("Backup maintenance failed for %s; will retry on next close/open path", actualPath);
        }
    }
    else if (ValidateFileMD5WithExtendedAttr(actualPath) != LE_OK)
    {
        char backupPath[RFS_MAX_BACKUP_FILENAME];
        GetBackupPath(actualPath, backupPath, sizeof(backupPath));

        if (ValidateFileMD5WithExtendedAttr(backupPath) == LE_OK)
        {
            LE_ERROR("Primary is invalid while backup is valid for %s; defer primary restore to next open",
                     actualPath);
        }
        else
        {
            LE_ERROR("Primary and backup are both invalid for %s", actualPath);
        }
    }

    return 0;
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
        // Persist the new directory entry (the renamed file name) so it survives a power loss.
        PersistParentDir(destPath);

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
