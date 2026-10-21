/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//--------------------------------------------------------------------------------------------------
/**
 * @file ctx.h
 *
 * SELinux context migration and restore of the supervisor file-system data.
 *
 * After a successful TelAF installation, the supervisor saves the current SELinux contexts
 * of /data/le_fs and /data/diag to a context backup file, then relabels these trees with the
 * file_contexts of the running image (selinuxCtx_Migrate()). On rollback, tafUpdateSvc sets the
 * saved contexts back (selinuxCtx_Restore()), so that the previous release finds the contexts it
 * expects. The context backup is removed with selinuxCtx_RemoveBackup() when it is not needed
 * anymore.
 *
 * The context backup has one "<context>\t<path>" entry per line. It is created once, by the first
 * migration run after an update, and is never modified afterwards.
 *
 * Any error is retried in rounds, every SELINUXCTX_RETRY_INTERVAL_MS ms, up to
 * SELINUXCTX_RETRY_ROUNDS times. An entry that still fails is skipped.
 *
 * ctx.c has no libselinux dependency. ctxMigrate.c (selinuxCtx_Migrate()) needs
 * libselinux and LE_CONFIG_ENABLE_SELINUX.
 */
//--------------------------------------------------------------------------------------------------

#ifndef TELAF_SELINUX_CTX_H_INCLUDE_GUARD
#define TELAF_SELINUX_CTX_H_INCLUDE_GUARD

#include "legato.h"

#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Directory trees whose SELinux contexts are migrated and restored.
 */
//--------------------------------------------------------------------------------------------------
#define SELINUXCTX_LE_FS_PATH               "/data/le_fs"
#define SELINUXCTX_DIAG_PATH                "/data/diag"

//--------------------------------------------------------------------------------------------------
/**
 * Context backup: the SELinux contexts of the trees saved before relabeling.
 */
//--------------------------------------------------------------------------------------------------
#define SELINUXCTX_BACKUP_PATH              "/data/.telaf_ctx_bk"
#define SELINUXCTX_BACKUP_TMP_PATH          "/data/.telaf_ctx_bk.tmp"
#define SELINUXCTX_BACKUP_DIR_PATH          "/data"

// The context backup drives the relabel done on rollback: only its owner may write it. The mode is
// set explicitly because the supervisor runs with umask 0.
#define SELINUXCTX_BACKUP_FILE_MODE         (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

//--------------------------------------------------------------------------------------------------
/**
 * Retry of errors: in rounds, every SELINUXCTX_RETRY_INTERVAL_MS ms, up to SELINUXCTX_RETRY_ROUNDS
 * times.
 */
//--------------------------------------------------------------------------------------------------
#define SELINUXCTX_RETRY_ROUNDS             3
#define SELINUXCTX_RETRY_INTERVAL_MS        200


//--------------------------------------------------------------------------------------------------
/**
 * Set the tag of the messages written to the kernel log (e.g. "supervisor", "tafUpdateSvc").
 * The string must stay valid; a string literal is expected.
 */
//--------------------------------------------------------------------------------------------------
void selinuxCtx_SetLogTag
(
    const char* tagPtr      ///< [IN] Tag.
);

//--------------------------------------------------------------------------------------------------
/**
 * Write a message with the given syslog level (3 = err, 4 = warn, 5 = notice) to the kernel log
 * (/dev/kmsg), so that it is visible with dmesg even if the supervisor log is not available.
 */
//--------------------------------------------------------------------------------------------------
void selinuxCtx_LogKmsg
(
    int level,              ///< [IN] Syslog level.
    const char* formatPtr,  ///< [IN] printf() format.
    ...
) __attribute__((format(printf, 2, 3)));

//--------------------------------------------------------------------------------------------------
/**
 * Log a message to both the supervisor log and the kernel log, so that it is not lost when the syslog
 * ring buffer is overwritten during boot.
 */
//--------------------------------------------------------------------------------------------------
#define SELINUXCTX_ERROR(...)               \
    do                                      \
    {                                       \
        LE_ERROR(__VA_ARGS__);              \
        selinuxCtx_LogKmsg(3, __VA_ARGS__); \
    }                                       \
    while (0)

#define SELINUXCTX_WARN(...)                \
    do                                      \
    {                                       \
        LE_WARN(__VA_ARGS__);               \
        selinuxCtx_LogKmsg(4, __VA_ARGS__); \
    }                                       \
    while (0)

#define SELINUXCTX_NOTICE(...)              \
    do                                      \
    {                                       \
        LE_INFO(__VA_ARGS__);               \
        selinuxCtx_LogKmsg(5, __VA_ARGS__); \
    }                                       \
    while (0)


//--------------------------------------------------------------------------------------------------
/**
 * One entry of a context backup.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char* pathPtr;          ///< Path of the entry.
    char* ctxPtr;           ///< Saved SELinux context.
}
selinuxCtx_Entry_t;

//--------------------------------------------------------------------------------------------------
/**
 * Context backup in memory. Must be initialized with SELINUXCTX_BACKUP_INIT (or zeroed) and
 * released with selinuxCtx_ClearBackup().
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    selinuxCtx_Entry_t* entryArray;     ///< Entries.
    size_t count;                       ///< Number of entries.
    size_t capacity;                    ///< Number of slots of entryArray.
}
selinuxCtx_Backup_t;

#define SELINUXCTX_BACKUP_INIT { NULL, 0, 0 }

//--------------------------------------------------------------------------------------------------
/**
 * Add a copy of an entry to a context backup.
 *
 * @return LE_OK on success, LE_NO_MEMORY otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t selinuxCtx_AddEntry
(
    selinuxCtx_Backup_t* backupPtr,     ///< [IN/OUT] Context backup.
    const char* ctxPtr,                 ///< [IN] Context.
    const char* pathPtr                 ///< [IN] Path.
);

//--------------------------------------------------------------------------------------------------
/**
 * Sort a context backup by path, so that selinuxCtx_FindEntry() can be used.
 */
//--------------------------------------------------------------------------------------------------
void selinuxCtx_SortBackup
(
    selinuxCtx_Backup_t* backupPtr      ///< [IN/OUT] Context backup.
);

//--------------------------------------------------------------------------------------------------
/**
 * Find the entry of a path in a sorted context backup.
 *
 * @return The entry, or NULL if the path is not in the backup.
 */
//--------------------------------------------------------------------------------------------------
const selinuxCtx_Entry_t* selinuxCtx_FindEntry
(
    const selinuxCtx_Backup_t* backupPtr,   ///< [IN] Sorted context backup.
    const char* pathPtr                     ///< [IN] Path.
);

//--------------------------------------------------------------------------------------------------
/**
 * Release the memory of a context backup and empty it.
 */
//--------------------------------------------------------------------------------------------------
void selinuxCtx_ClearBackup
(
    selinuxCtx_Backup_t* backupPtr      ///< [IN/OUT] Context backup.
);

//--------------------------------------------------------------------------------------------------
/**
 * Load the context backup file SELINUXCTX_BACKUP_PATH. The loaded backup is sorted.
 *
 * Blank lines are ignored; any other line must be "<context>\t<path>". A backup that has entries
 * but no entry for SELINUXCTX_LE_FS_PATH itself is corrupted.
 *
 * @return
 *  - LE_OK         The backup was loaded (it may have no entry).
 *  - LE_NOT_FOUND  There is no context backup.
 *  - LE_FAULT      The backup cannot be read or is corrupted (*backupPtr is empty).
 */
//--------------------------------------------------------------------------------------------------
le_result_t selinuxCtx_LoadBackup
(
    selinuxCtx_Backup_t* backupPtr      ///< [OUT] Context backup.
);

//--------------------------------------------------------------------------------------------------
/**
 * Write a context backup to a temporary file, flush it, rename it to SELINUXCTX_BACKUP_PATH and
 * flush its directory.
 *
 * @return LE_OK on success, LE_FAULT otherwise (no partial file is left).
 */
//--------------------------------------------------------------------------------------------------
le_result_t selinuxCtx_WriteBackup
(
    const selinuxCtx_Backup_t* backupPtr    ///< [IN] Context backup.
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove the context backup file and flush its directory, so that the removal survives a power
 * loss.
 *
 * @return LE_OK if the backup does not exist anymore (also if it did not exist), LE_FAULT
 *         otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t selinuxCtx_RemoveBackup
(
    const char* reasonPtr   ///< [IN] Why the backup is removed (for logs).
);

//--------------------------------------------------------------------------------------------------
/**
 * Processing of one entry during a pass.
 *
 * @return true on success, false on error (*errPtr is set to the errno value).
 */
//--------------------------------------------------------------------------------------------------
typedef bool (*selinuxCtx_EntryFunc_t)
(
    void* contextPtr,       ///< [IN/OUT] Caller state.
    const char* pathPtr,    ///< [IN] Path of the entry.
    mode_t mode,            ///< [IN] File mode of the entry.
    int* errPtr             ///< [OUT] errno value on error.
);

//--------------------------------------------------------------------------------------------------
/**
 * Reset of the caller state at the start of every attempt of a pass.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*selinuxCtx_ResetFunc_t)
(
    void* contextPtr        ///< [IN/OUT] Caller state.
);

//--------------------------------------------------------------------------------------------------
/**
 * Run one pass over the SELINUXCTX_LE_FS_PATH and SELINUXCTX_DIAG_PATH trees: call func for every
 * entry. A root that does not exist is not an error. The entries that fail are retried in rounds,
 * then skipped. If a tree cannot be walked, the whole pass is retried in rounds (resetFunc, if not
 * NULL, is called at the start of every attempt).
 *
 * @return LE_OK if the pass was done, LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
le_result_t selinuxCtx_RunPass
(
    selinuxCtx_EntryFunc_t func,        ///< [IN] Processing of an entry.
    selinuxCtx_ResetFunc_t resetFunc,   ///< [IN] Reset of the caller state, or NULL.
    void* contextPtr,                   ///< [IN/OUT] Caller state, given to func and resetFunc.
    const char* passNamePtr,            ///< [IN] Name of the pass (for logs).
    size_t* skippedCountPtr             ///< [OUT] Number of entries skipped on error.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the SELinux contexts of SELINUXCTX_LE_FS_PATH and SELINUXCTX_DIAG_PATH back to the ones of
 * the context backup. Entries that are not in the backup (created after the upgrade) get the
 * saved context of their tree root. A tree whose root is not in the backup is skipped. The data
 * itself is not touched.
 *
 *  - No context backup, or an empty one: nothing to restore, success.
 *  - The backup cannot be read or is corrupted: retried in rounds, then failure.
 *  - An entry whose context cannot be set: retried in rounds, then skipped.
 *
 * The context backup is kept; the caller removes it with selinuxCtx_RemoveBackup().
 *
 * @return LE_OK on success, LE_FAULT if the context backup cannot be used.
 */
//--------------------------------------------------------------------------------------------------
le_result_t selinuxCtx_Restore
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Migrate the SELinux contexts of SELINUXCTX_LE_FS_PATH and SELINUXCTX_DIAG_PATH to the
 * file_contexts of the running image, with the same rules as "restorecon -R" (without -F). The
 * whole trees are checked; if some entries have an old context, the context backup is created if
 * there is none, the entries of the backup are relabeled, then the whole trees are checked again.
 *
 * If the file_contexts cannot be opened, a tree cannot be walked or the context backup cannot be
 * written, the migration is not completed and the caller continues normally; the migration can be
 * tried again later (e.g. on the next boot).
 *
 * Only available with LE_CONFIG_ENABLE_SELINUX (ctxMigrate.c, needs libselinux).
 */
//--------------------------------------------------------------------------------------------------
void selinuxCtx_Migrate
(
    void
);


#ifdef __cplusplus
}
#endif

#endif // TELAF_SELINUX_CTX_H_INCLUDE_GUARD