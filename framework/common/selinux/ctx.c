/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//--------------------------------------------------------------------------------------------------
/**
 * @file ctx.c
 *
 * Common part of the SELinux context migration and restore: kernel log, context backup file,
 * directory tree walk with retries, and restore of the saved contexts. No libselinux dependency.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "ctx.h"

#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>


//--------------------------------------------------------------------------------------------------
/**
 * Extended attribute holding the SELinux context of a file.
 */
//--------------------------------------------------------------------------------------------------
#define SELINUX_XATTR "security.selinux"


//--------------------------------------------------------------------------------------------------
/**
 * Tag of the messages written to the kernel log.
 */
//--------------------------------------------------------------------------------------------------
static const char* LogTagPtr = "selinuxCtx";


//--------------------------------------------------------------------------------------------------
/**
 * Growable list of strings.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char**  itemPtrArray;   ///< Strings (each allocated with strdup()).
    size_t  count;          ///< Number of strings.
    size_t  capacity;       ///< Number of slots of itemPtrArray.
}
StrList_t;


//--------------------------------------------------------------------------------------------------
/**
 * Set the tag of the messages written to the kernel log.
 */
//--------------------------------------------------------------------------------------------------
void selinuxCtx_SetLogTag
(
    const char* tagPtr
)
{
    if (tagPtr != NULL)
    {
        LogTagPtr = tagPtr;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write a message with the given syslog level to the kernel log (/dev/kmsg).
 * New lines inside the message are replaced, since each write to /dev/kmsg is one record.
 */
//--------------------------------------------------------------------------------------------------
void selinuxCtx_LogKmsg
(
    int level,
    const char* formatPtr,
    ...
)
{
    char msg[512];
    int len = snprintf(msg, sizeof(msg), "<%d>%s: ", level, LogTagPtr);
    if ((len < 0) || ((size_t)len >= sizeof(msg)))
    {
        return;
    }

    va_list args;
    va_start(args, formatPtr);
    vsnprintf(msg + len, sizeof(msg) - len, formatPtr, args);
    va_end(args);

    // A new line inside the message (e.g. in a file name) would end the kmsg record and the rest
    // of the message would be lost.
    for (char* charPtr = msg + len; *charPtr != '\0'; charPtr++)
    {
        if ((*charPtr == '\n') || (*charPtr == '\r'))
        {
            *charPtr = '?';
        }
    }

    size_t msgLen = strlen(msg);
    if (msgLen < sizeof(msg) - 1)
    {
        msg[msgLen++] = '\n';
        msg[msgLen] = '\0';
    }

    int fd = open("/dev/kmsg", O_WRONLY | O_NOCTTY | O_CLOEXEC);
    if (fd >= 0)
    {
        if (write(fd, msg, msgLen) < 0)
        {
            // Nothing more can be done; the message is also in the supervisor log.
        }
        close(fd);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a copy of a string to a list.
 *
 * @return LE_OK on success, LE_NO_MEMORY otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AddToStrList
(
    StrList_t* listPtr,     ///< [IN/OUT] List.
    const char* strPtr      ///< [IN] String.
)
{
    if (listPtr->count == listPtr->capacity)
    {
        size_t newCapacity = (listPtr->capacity == 0) ? 64 : (listPtr->capacity * 2);
        char** newArrayPtr = realloc(listPtr->itemPtrArray, newCapacity * sizeof(char*));
        if (newArrayPtr == NULL)
        {
            return LE_NO_MEMORY;
        }
        listPtr->itemPtrArray = newArrayPtr;
        listPtr->capacity = newCapacity;
    }

    char* copyPtr = strdup(strPtr);
    if (copyPtr == NULL)
    {
        return LE_NO_MEMORY;
    }
    listPtr->itemPtrArray[listPtr->count++] = copyPtr;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Release the memory of a list and empty it.
 */
//--------------------------------------------------------------------------------------------------
static void ClearStrList
(
    StrList_t* listPtr      ///< [IN/OUT] List.
)
{
    for (size_t i = 0; i < listPtr->count; i++)
    {
        free(listPtr->itemPtrArray[i]);
    }
    free(listPtr->itemPtrArray);
    listPtr->itemPtrArray = NULL;
    listPtr->count = 0;
    listPtr->capacity = 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a copy of an entry to a context backup.
 */
//--------------------------------------------------------------------------------------------------
le_result_t selinuxCtx_AddEntry
(
    selinuxCtx_Backup_t* backupPtr,
    const char* ctxPtr,
    const char* pathPtr
)
{
    if (backupPtr->count == backupPtr->capacity)
    {
        size_t newCapacity = (backupPtr->capacity == 0) ? 64 : (backupPtr->capacity * 2);
        selinuxCtx_Entry_t* newArrayPtr = realloc(backupPtr->entryArray,
                                                  newCapacity * sizeof(selinuxCtx_Entry_t));
        if (newArrayPtr == NULL)
        {
            return LE_NO_MEMORY;
        }
        backupPtr->entryArray = newArrayPtr;
        backupPtr->capacity = newCapacity;
    }

    char* pathCopyPtr = strdup(pathPtr);
    char* ctxCopyPtr = strdup(ctxPtr);
    if ((pathCopyPtr == NULL) || (ctxCopyPtr == NULL))
    {
        free(pathCopyPtr);
        free(ctxCopyPtr);
        return LE_NO_MEMORY;
    }

    backupPtr->entryArray[backupPtr->count].pathPtr = pathCopyPtr;
    backupPtr->entryArray[backupPtr->count].ctxPtr = ctxCopyPtr;
    backupPtr->count++;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Compare two entries by path (for qsort() and bsearch()).
 */
//--------------------------------------------------------------------------------------------------
static int CompareEntry
(
    const void* aPtr,
    const void* bPtr
)
{
    return strcmp(((const selinuxCtx_Entry_t*)aPtr)->pathPtr,
                  ((const selinuxCtx_Entry_t*)bPtr)->pathPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stable merge sort of entries by path.
 */
//--------------------------------------------------------------------------------------------------
static void MergeSortEntries
(
    selinuxCtx_Entry_t* arrayPtr,   ///< [IN/OUT] Entries.
    selinuxCtx_Entry_t* tmpPtr,     ///< [IN] Scratch area of at least count entries.
    size_t count                    ///< [IN] Number of entries.
)
{
    if (count < 2)
    {
        return;
    }

    size_t mid = count / 2;
    MergeSortEntries(arrayPtr, tmpPtr, mid);
    MergeSortEntries(arrayPtr + mid, tmpPtr, count - mid);

    size_t i = 0;
    size_t j = mid;
    size_t k = 0;
    while ((i < mid) && (j < count))
    {
        if (CompareEntry(&arrayPtr[j], &arrayPtr[i]) < 0)
        {
            tmpPtr[k++] = arrayPtr[j++];
        }
        else
        {
            tmpPtr[k++] = arrayPtr[i++];
        }
    }
    while (i < mid)
    {
        tmpPtr[k++] = arrayPtr[i++];
    }
    while (j < count)
    {
        tmpPtr[k++] = arrayPtr[j++];
    }
    memcpy(arrayPtr, tmpPtr, count * sizeof(selinuxCtx_Entry_t));
}


//--------------------------------------------------------------------------------------------------
/**
 * Sort a context backup by path. If a path appears more than once, the last entry is kept.
 */
//--------------------------------------------------------------------------------------------------
void selinuxCtx_SortBackup
(
    selinuxCtx_Backup_t* backupPtr
)
{
    if (backupPtr->count < 2)
    {
        return;
    }

    selinuxCtx_Entry_t* tmpPtr = malloc(backupPtr->count * sizeof(selinuxCtx_Entry_t));
    if (tmpPtr != NULL)
    {
        MergeSortEntries(backupPtr->entryArray, tmpPtr, backupPtr->count);
        free(tmpPtr);
    }
    else
    {
        // Not stable: for a duplicated path, any of its entries may be kept.
        qsort(backupPtr->entryArray, backupPtr->count, sizeof(selinuxCtx_Entry_t), CompareEntry);
    }

    // Remove duplicated paths, keeping the last entry (the sort is stable).
    size_t outCount = 0;
    for (size_t i = 0; i < backupPtr->count; i++)
    {
        selinuxCtx_Entry_t* entryPtr = &backupPtr->entryArray[i];
        if ((outCount > 0) && (CompareEntry(&backupPtr->entryArray[outCount - 1], entryPtr) == 0))
        {
            free(backupPtr->entryArray[outCount - 1].pathPtr);
            free(backupPtr->entryArray[outCount - 1].ctxPtr);
            backupPtr->entryArray[outCount - 1] = *entryPtr;
        }
        else
        {
            backupPtr->entryArray[outCount++] = *entryPtr;
        }
    }
    backupPtr->count = outCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find the entry of a path in a sorted context backup.
 */
//--------------------------------------------------------------------------------------------------
const selinuxCtx_Entry_t* selinuxCtx_FindEntry
(
    const selinuxCtx_Backup_t* backupPtr,
    const char* pathPtr
)
{
    if (backupPtr->count == 0)
    {
        return NULL;
    }

    selinuxCtx_Entry_t key = { (char*)pathPtr, NULL };
    return bsearch(&key, backupPtr->entryArray, backupPtr->count, sizeof(selinuxCtx_Entry_t),
                   CompareEntry);
}


//--------------------------------------------------------------------------------------------------
/**
 * Release the memory of a context backup and empty it.
 */
//--------------------------------------------------------------------------------------------------
void selinuxCtx_ClearBackup
(
    selinuxCtx_Backup_t* backupPtr
)
{
    for (size_t i = 0; i < backupPtr->count; i++)
    {
        free(backupPtr->entryArray[i].pathPtr);
        free(backupPtr->entryArray[i].ctxPtr);
    }
    free(backupPtr->entryArray);
    backupPtr->entryArray = NULL;
    backupPtr->count = 0;
    backupPtr->capacity = 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Load the context backup file.
 */
//--------------------------------------------------------------------------------------------------
le_result_t selinuxCtx_LoadBackup
(
    selinuxCtx_Backup_t* backupPtr
)
{
    selinuxCtx_ClearBackup(backupPtr);

    FILE* fp = fopen(SELINUXCTX_BACKUP_PATH, "r");
    if (fp == NULL)
    {
        if (errno == ENOENT)
        {
            return LE_NOT_FOUND;
        }
        SELINUXCTX_ERROR("Could not open context backup '%s': %s", SELINUXCTX_BACKUP_PATH,
                         strerror(errno));
        return LE_FAULT;
    }

    le_result_t result = LE_OK;
    char* linePtr = NULL;
    size_t lineSize = 0;
    size_t lineNum = 0;
    ssize_t len;
    while ((len = getline(&linePtr, &lineSize, fp)) >= 0)
    {
        lineNum++;
        if ((len > 0) && (linePtr[len - 1] == '\n'))
        {
            linePtr[--len] = '\0';
        }

        if (strspn(linePtr, " \t\r") == (size_t)len)
        {
            // Blank line, ignored.
            continue;
        }

        char* tabPtr = strchr(linePtr, '\t');
        if ((tabPtr == NULL) || (tabPtr == linePtr) || (tabPtr[1] == '\0'))
        {
            SELINUXCTX_ERROR("Context backup '%s' is corrupted at line %zu.",
                             SELINUXCTX_BACKUP_PATH, lineNum);
            result = LE_FAULT;
            break;
        }

        *tabPtr = '\0';
        if (selinuxCtx_AddEntry(backupPtr, linePtr, tabPtr + 1) != LE_OK)
        {
            SELINUXCTX_ERROR("Could not load '%s': out of memory.", SELINUXCTX_BACKUP_PATH);
            result = LE_FAULT;
            break;
        }
    }

    if ((result == LE_OK) && ferror(fp))
    {
        SELINUXCTX_ERROR("Could not read context backup '%s' (stopped at line %zu).",
                         SELINUXCTX_BACKUP_PATH, lineNum);
        result = LE_FAULT;
    }

    free(linePtr);
    fclose(fp);

    if (result == LE_OK)
    {
        selinuxCtx_SortBackup(backupPtr);

        // The supervisor always records /data/le_fs itself. A backup without it is corrupted.
        if ((backupPtr->count > 0)
            && (selinuxCtx_FindEntry(backupPtr, SELINUXCTX_LE_FS_PATH) == NULL))
        {
            SELINUXCTX_ERROR("Context backup '%s' has no entry for '%s'.", SELINUXCTX_BACKUP_PATH,
                             SELINUXCTX_LE_FS_PATH);
            result = LE_FAULT;
        }
    }

    if (result != LE_OK)
    {
        selinuxCtx_ClearBackup(backupPtr);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Flush a directory, so that a rename or a removal done in it survives a power loss.
 *
 * @return LE_OK on success, LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SyncDir
(
    const char* dirPathPtr      ///< [IN] Directory.
)
{
    int fd = open(dirPathPtr, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0)
    {
        SELINUXCTX_ERROR("Could not open '%s': %s", dirPathPtr, strerror(errno));
        return LE_FAULT;
    }

    le_result_t result = LE_OK;
    if (fsync(fd) != 0)
    {
        SELINUXCTX_ERROR("Could not flush '%s': %s", dirPathPtr, strerror(errno));
        result = LE_FAULT;
    }

    close(fd);
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Write a context backup atomically.
 */
//--------------------------------------------------------------------------------------------------
le_result_t selinuxCtx_WriteBackup
(
    const selinuxCtx_Backup_t* backupPtr
)
{
    int fd = open(SELINUXCTX_BACKUP_TMP_PATH, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW,
                  SELINUXCTX_BACKUP_FILE_MODE);
    if (fd < 0)
    {
        SELINUXCTX_ERROR("Could not create '%s': %s", SELINUXCTX_BACKUP_TMP_PATH, strerror(errno));
        return LE_FAULT;
    }

    // The temporary file may be left by an interrupted run with another mode.
    if (fchmod(fd, SELINUXCTX_BACKUP_FILE_MODE) != 0)
    {
        SELINUXCTX_ERROR("Could not set the mode of '%s': %s", SELINUXCTX_BACKUP_TMP_PATH,
                         strerror(errno));
        close(fd);
        (void)unlink(SELINUXCTX_BACKUP_TMP_PATH);
        return LE_FAULT;
    }

    FILE* fp = fdopen(fd, "w");
    if (fp == NULL)
    {
        SELINUXCTX_ERROR("Could not open '%s': %s", SELINUXCTX_BACKUP_TMP_PATH, strerror(errno));
        close(fd);
        (void)unlink(SELINUXCTX_BACKUP_TMP_PATH);
        return LE_FAULT;
    }

    le_result_t result = LE_OK;
    for (size_t i = 0; (result == LE_OK) && (i < backupPtr->count); i++)
    {
        if (fprintf(fp, "%s\t%s\n", backupPtr->entryArray[i].ctxPtr,
                    backupPtr->entryArray[i].pathPtr) < 0)
        {
            result = LE_FAULT;
        }
    }
    if (result != LE_OK)
    {
        SELINUXCTX_ERROR("Could not write '%s': %s", SELINUXCTX_BACKUP_TMP_PATH, strerror(errno));
    }

    if ((fflush(fp) != 0) || (fsync(fileno(fp)) != 0))
    {
        SELINUXCTX_ERROR("Could not flush '%s': %s", SELINUXCTX_BACKUP_TMP_PATH, strerror(errno));
        result = LE_FAULT;
    }

    if (fclose(fp) != 0)
    {
        SELINUXCTX_ERROR("Could not close '%s': %s", SELINUXCTX_BACKUP_TMP_PATH, strerror(errno));
        result = LE_FAULT;
    }

    if ((result == LE_OK) && (rename(SELINUXCTX_BACKUP_TMP_PATH, SELINUXCTX_BACKUP_PATH) != 0))
    {
        SELINUXCTX_ERROR("Could not rename '%s' to '%s': %s", SELINUXCTX_BACKUP_TMP_PATH,
                         SELINUXCTX_BACKUP_PATH, strerror(errno));
        result = LE_FAULT;
    }

    if (result != LE_OK)
    {
        (void)unlink(SELINUXCTX_BACKUP_TMP_PATH);
        return LE_FAULT;
    }

    return SyncDir(SELINUXCTX_BACKUP_DIR_PATH);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove the context backup file and flush its directory.
 */
//--------------------------------------------------------------------------------------------------
le_result_t selinuxCtx_RemoveBackup
(
    const char* reasonPtr
)
{
    if (unlink(SELINUXCTX_BACKUP_PATH) == 0)
    {
        LE_INFO("%s: removed saved context list '%s'.", reasonPtr, SELINUXCTX_BACKUP_PATH);
    }
    else if (errno != ENOENT)
    {
        LE_ERROR("%s: failed to remove '%s': %m", reasonPtr, SELINUXCTX_BACKUP_PATH);
        return LE_FAULT;
    }

    int fd = open(SELINUXCTX_BACKUP_DIR_PATH, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0)
    {
        LE_ERROR("%s: could not open '%s': %m", reasonPtr, SELINUXCTX_BACKUP_DIR_PATH);
        return LE_FAULT;
    }

    le_result_t result = LE_OK;
    if (fsync(fd) != 0)
    {
        LE_ERROR("%s: could not flush '%s': %m", reasonPtr, SELINUXCTX_BACKUP_DIR_PATH);
        result = LE_FAULT;
    }
    close(fd);
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Tell whether an error of an entry is not an error: the entry was removed during the walk, or it
 * is on a file system without per-file SELinux labels (restorecon skips it too).
 */
//--------------------------------------------------------------------------------------------------
static bool IsIgnoredError
(
    int err     ///< [IN] errno value.
)
{
    return (err == ENOENT) || (err == ENOTSUP);
}


//--------------------------------------------------------------------------------------------------
/**
 * Walk a directory tree and process every entry. The paths of the entries that fail are added to
 * failedPtr. A root that does not exist is not an error.
 *
 * @return LE_OK if the whole tree was walked, LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WalkTree
(
    const char* rootPathPtr,            ///< [IN] Root of the directory tree.
    selinuxCtx_EntryFunc_t func,        ///< [IN] Processing of an entry.
    void* contextPtr,                   ///< [IN/OUT] Caller state.
    StrList_t* failedPtr                ///< [IN/OUT] Paths of the entries that failed.
)
{
    if (access(rootPathPtr, F_OK) != 0)
    {
        if (errno == ENOENT)
        {
            return LE_OK;
        }
        SELINUXCTX_ERROR("Could not access '%s': %s", rootPathPtr, strerror(errno));
        return LE_FAULT;
    }

    char* pathArrayPtr[] = { (char*)rootPathPtr, NULL };
    FTS* ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
    if (ftsPtr == NULL)
    {
        SELINUXCTX_ERROR("Could not open '%s': %s", rootPathPtr, strerror(errno));
        return LE_FAULT;
    }

    le_result_t result = LE_OK;
    for (;;)
    {
        errno = 0;
        FTSENT* entPtr = fts_read(ftsPtr);
        if (entPtr == NULL)
        {
            if (errno != 0)
            {
                SELINUXCTX_ERROR("Could not walk '%s': %s", rootPathPtr, strerror(errno));
                result = LE_FAULT;
            }
            break;
        }

        int err = 0;
        bool ok = true;
        switch (entPtr->fts_info)
        {
            case FTS_DP:
                // Directory visited in post-order; it was processed in pre-order.
                continue;

            case FTS_ERR:
            case FTS_NS:
                err = entPtr->fts_errno;
                ok = false;
                break;

            case FTS_DNR:
                if (entPtr->fts_errno != ENOENT)
                {
                    SELINUXCTX_ERROR("Could not read directory '%s', its entries are skipped: %s",
                                     entPtr->fts_path, strerror(entPtr->fts_errno));
                }
                ok = func(contextPtr, entPtr->fts_path, entPtr->fts_statp->st_mode, &err);
                break;

            default:
                ok = func(contextPtr, entPtr->fts_path, entPtr->fts_statp->st_mode, &err);
                break;
        }

        if (!ok && !IsIgnoredError(err))
        {
            if (AddToStrList(failedPtr, entPtr->fts_path) != LE_OK)
            {
                SELINUXCTX_ERROR("Could not record failed entry '%s': out of memory.",
                                 entPtr->fts_path);
                result = LE_FAULT;
                break;
            }
        }
    }

    fts_close(ftsPtr);
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Retry the entries that failed, in rounds: every SELINUXCTX_RETRY_INTERVAL_MS ms, up to
 * SELINUXCTX_RETRY_ROUNDS times. The entries that still fail are skipped.
 *
 * @return Number of skipped entries.
 */
//--------------------------------------------------------------------------------------------------
static size_t RetryFailed
(
    selinuxCtx_EntryFunc_t func,        ///< [IN] Processing of an entry.
    void* contextPtr,                   ///< [IN/OUT] Caller state.
    const char* passNamePtr,            ///< [IN] Name of the pass (for logs).
    StrList_t* failedPtr                ///< [IN/OUT] Paths of the entries that failed.
)
{
    for (int round = 1; (round <= SELINUXCTX_RETRY_ROUNDS) && (failedPtr->count > 0); round++)
    {
        usleep(SELINUXCTX_RETRY_INTERVAL_MS * 1000);

        StrList_t stillFailed = { NULL, 0, 0 };
        for (size_t i = 0; i < failedPtr->count; i++)
        {
            const char* pathPtr = failedPtr->itemPtrArray[i];
            struct stat st;
            int err = 0;
            bool ok;

            if (lstat(pathPtr, &st) != 0)
            {
                err = errno;
                ok = false;
            }
            else
            {
                ok = func(contextPtr, pathPtr, st.st_mode, &err);
            }

            if (ok || IsIgnoredError(err))
            {
                continue;
            }

            if (round == SELINUXCTX_RETRY_ROUNDS)
            {
                LE_ERROR("SELinux context %s of '%s' failed after %d retries, skipped: %s",
                         passNamePtr, pathPtr, SELINUXCTX_RETRY_ROUNDS, strerror(err));
            }

            if (AddToStrList(&stillFailed, pathPtr) != LE_OK)
            {
                LE_ERROR("SELinux context %s of '%s' skipped: out of memory.", passNamePtr,
                         pathPtr);
            }
        }

        ClearStrList(failedPtr);
        *failedPtr = stillFailed;
    }

    return failedPtr->count;
}


//--------------------------------------------------------------------------------------------------
/**
 * Run one pass over a list of directory trees (see selinuxCtx_RunPass()).
 *
 * @return LE_OK if the pass was done, LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RunPassOnRoots
(
    const char* const* rootPathArray,   ///< [IN] Roots of the trees.
    size_t rootCount,                   ///< [IN] Number of roots.
    selinuxCtx_EntryFunc_t func,        ///< [IN] Processing of an entry.
    selinuxCtx_ResetFunc_t resetFunc,   ///< [IN] Reset of the caller state, or NULL.
    void* contextPtr,                   ///< [IN/OUT] Caller state.
    const char* passNamePtr,            ///< [IN] Name of the pass (for logs).
    size_t* skippedCountPtr             ///< [OUT] Number of entries skipped on error.
)
{
    *skippedCountPtr = 0;

    for (int attempt = 0; attempt <= SELINUXCTX_RETRY_ROUNDS; attempt++)
    {
        if (attempt > 0)
        {
            SELINUXCTX_ERROR("SELinux context %s of '%s' and '%s' failed, retry in %d ms "
                             "(%d/%d).", passNamePtr, SELINUXCTX_LE_FS_PATH, SELINUXCTX_DIAG_PATH,
                             SELINUXCTX_RETRY_INTERVAL_MS, attempt, SELINUXCTX_RETRY_ROUNDS);
            usleep(SELINUXCTX_RETRY_INTERVAL_MS * 1000);
        }

        if (resetFunc != NULL)
        {
            resetFunc(contextPtr);
        }

        StrList_t failed = { NULL, 0, 0 };
        le_result_t result = LE_OK;
        for (size_t i = 0; (result == LE_OK) && (i < rootCount); i++)
        {
            result = WalkTree(rootPathArray[i], func, contextPtr, &failed);
        }

        if (result == LE_OK)
        {
            *skippedCountPtr = RetryFailed(func, contextPtr, passNamePtr, &failed);
            ClearStrList(&failed);
            return LE_OK;
        }

        ClearStrList(&failed);
    }

    SELINUXCTX_ERROR("SELinux context %s of '%s' and '%s' failed after %d retries.", passNamePtr,
                     SELINUXCTX_LE_FS_PATH, SELINUXCTX_DIAG_PATH, SELINUXCTX_RETRY_ROUNDS);
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Run one pass over the /data/le_fs and /data/diag trees.
 */
//--------------------------------------------------------------------------------------------------
le_result_t selinuxCtx_RunPass
(
    selinuxCtx_EntryFunc_t func,
    selinuxCtx_ResetFunc_t resetFunc,
    void* contextPtr,
    const char* passNamePtr,
    size_t* skippedCountPtr
)
{
    static const char* const rootPathArray[] = { SELINUXCTX_LE_FS_PATH, SELINUXCTX_DIAG_PATH };

    return RunPassOnRoots(rootPathArray, NUM_ARRAY_MEMBERS(rootPathArray), func, resetFunc,
                          contextPtr, passNamePtr, skippedCountPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * State of the SELinux context restore.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    const selinuxCtx_Backup_t* backupPtr;   ///< Sorted context backup.
    const char* rootPathArray[2];           ///< Roots of the trees to restore.
    const char* rootCtxArray[2];            ///< Saved contexts of the roots.
    size_t rootCount;                       ///< Number of trees to restore.
    size_t restoredCount;                   ///< Entries set to their saved context.
    size_t defaultCount;                    ///< Entries set to the context of their root.
}
Restore_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reset the counters of the restore at the start of every attempt.
 */
//--------------------------------------------------------------------------------------------------
static void ResetRestore
(
    void* contextPtr
)
{
    Restore_t* restorePtr = contextPtr;
    restorePtr->restoredCount = 0;
    restorePtr->defaultCount = 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Restore pass: set the saved context of an entry, or the saved context of its tree root if the
 * entry is not in the context backup (created after the upgrade).
 */
//--------------------------------------------------------------------------------------------------
static bool RestoreEntry
(
    void* contextPtr,
    const char* pathPtr,
    mode_t mode,
    int* errPtr
)
{
    (void)mode;
    Restore_t* restorePtr = contextPtr;

    const char* ctxPtr = NULL;
    bool isSaved = false;

    const selinuxCtx_Entry_t* entryPtr = selinuxCtx_FindEntry(restorePtr->backupPtr, pathPtr);
    if (entryPtr != NULL)
    {
        ctxPtr = entryPtr->ctxPtr;
        isSaved = true;
    }
    else
    {
        for (size_t i = 0; i < restorePtr->rootCount; i++)
        {
            size_t len = strlen(restorePtr->rootPathArray[i]);
            if ((strncmp(pathPtr, restorePtr->rootPathArray[i], len) == 0)
                && ((pathPtr[len] == '\0') || (pathPtr[len] == '/')))
            {
                ctxPtr = restorePtr->rootCtxArray[i];
                break;
            }
        }
    }

    if (ctxPtr == NULL)
    {
        // Not under a restored tree; nothing to do.
        return true;
    }

    if (lsetxattr(pathPtr, SELINUX_XATTR, ctxPtr, strlen(ctxPtr) + 1, 0) != 0)
    {
        *errPtr = errno;
        return false;
    }

    if (isSaved)
    {
        restorePtr->restoredCount++;
    }
    else
    {
        restorePtr->defaultCount++;
    }
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Restore the SELinux contexts saved in the context backup.
 */
//--------------------------------------------------------------------------------------------------
le_result_t selinuxCtx_Restore
(
    void
)
{
    selinuxCtx_Backup_t backup = SELINUXCTX_BACKUP_INIT;
    le_result_t result = LE_FAULT;

    for (int attempt = 0; attempt <= SELINUXCTX_RETRY_ROUNDS; attempt++)
    {
        if (attempt > 0)
        {
            usleep(SELINUXCTX_RETRY_INTERVAL_MS * 1000);
        }
        result = selinuxCtx_LoadBackup(&backup);
        if (result != LE_FAULT)
        {
            break;
        }
    }

    if (result == LE_NOT_FOUND)
    {
        LE_INFO("No saved context list '%s', nothing to restore.", SELINUXCTX_BACKUP_PATH);
        return LE_OK;
    }
    if (result != LE_OK)
    {
        SELINUXCTX_ERROR("Saved context list '%s' cannot be used after %d retries.",
                         SELINUXCTX_BACKUP_PATH, SELINUXCTX_RETRY_ROUNDS);
        return LE_FAULT;
    }
    if (backup.count == 0)
    {
        SELINUXCTX_ERROR("Saved context list '%s' is empty, nothing to restore.",
                         SELINUXCTX_BACKUP_PATH);
        return LE_OK;
    }

    LE_INFO("Loaded %zu saved contexts from '%s'.", backup.count, SELINUXCTX_BACKUP_PATH);

    Restore_t restore;
    memset(&restore, 0, sizeof(restore));
    restore.backupPtr = &backup;

    static const char* const rootPathArray[] = { SELINUXCTX_LE_FS_PATH, SELINUXCTX_DIAG_PATH };
    for (size_t i = 0; i < NUM_ARRAY_MEMBERS(rootPathArray); i++)
    {
        if ((access(rootPathArray[i], F_OK) != 0) && (errno == ENOENT))
        {
            LE_INFO("'%s' does not exist, no context to restore.", rootPathArray[i]);
            continue;
        }

        const selinuxCtx_Entry_t* rootEntryPtr = selinuxCtx_FindEntry(&backup, rootPathArray[i]);
        if (rootEntryPtr == NULL)
        {
            LE_INFO("No saved context for '%s', skip it.", rootPathArray[i]);
            continue;
        }

        restore.rootPathArray[restore.rootCount] = rootPathArray[i];
        restore.rootCtxArray[restore.rootCount] = rootEntryPtr->ctxPtr;
        restore.rootCount++;
    }

    size_t skippedCount = 0;
    result = RunPassOnRoots(restore.rootPathArray, restore.rootCount, RestoreEntry, ResetRestore,
                            &restore, "restore", &skippedCount);
    if (result != LE_OK)
    {
        SELINUXCTX_ERROR("Failed to walk '%s' and '%s' after %d retries.", SELINUXCTX_LE_FS_PATH,
                         SELINUXCTX_DIAG_PATH, SELINUXCTX_RETRY_ROUNDS);
        selinuxCtx_ClearBackup(&backup);
        return LE_FAULT;
    }

    sync();

    LE_INFO("SELinux contexts of '%s' and '%s' restored: %zu restored, %zu set to the root "
            "context, %zu skipped on error.", SELINUXCTX_LE_FS_PATH, SELINUXCTX_DIAG_PATH,
            restore.restoredCount, restore.defaultCount, skippedCount);
    if (skippedCount > 0)
    {
        SELINUXCTX_ERROR("%zu entries of '%s' and '%s' were skipped on error.", skippedCount,
                         SELINUXCTX_LE_FS_PATH, SELINUXCTX_DIAG_PATH);
    }

    selinuxCtx_ClearBackup(&backup);
    return LE_OK;
}