/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//--------------------------------------------------------------------------------------------------
/**
 * @file ctxMigrate.c
 *
 * SELinux context migration of /data/le_fs and /data/diag to the file_contexts of the running
 * image (same rules as "restorecon -R" without -F). Needs libselinux.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "ctx.h"

#ifdef LE_CONFIG_ENABLE_SELINUX

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <selinux/selinux.h>
#include <selinux/label.h>
#include <selinux/context.h>


//--------------------------------------------------------------------------------------------------
/**
 * State of the SELinux context migration.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    struct selabel_handle* hndPtr;  ///< file_contexts handle.
    selinuxCtx_Backup_t backup;     ///< Context backup in use, sorted.
    selinuxCtx_Backup_t newBackup;  ///< Context backup being created.
    bool      hasLeFsRoot;          ///< The context backup holds the context of the le_fs root.
    size_t    pendingCount;         ///< Entries whose context differs from the file_contexts.
    size_t    relabelCount;         ///< Entries relabeled.
    size_t    notInBackupCount;     ///< Entries not relabeled because they are not in the backup.
}
Migration_t;


//--------------------------------------------------------------------------------------------------
/**
 * Compute the context an entry must get, with the same rule as restorecon without -F: only the
 * type is taken from the file_contexts, the user, role and MLS range of the current context are
 * kept. Entries without a file_contexts entry, with a "<<none>>" spec or with a customizable type
 * are left as they are.
 *
 * @return true on success (*targetCtxPtrPtr is NULL if no change is needed, otherwise a string to
 *         be released with free()), false on error (*errPtr is set).
 */
//--------------------------------------------------------------------------------------------------
static bool GetTargetContext
(
    struct selabel_handle* hndPtr,  ///< [IN] file_contexts handle.
    const char* pathPtr,            ///< [IN] Path of the entry.
    mode_t mode,                    ///< [IN] File mode of the entry.
    const char* curCtxPtr,          ///< [IN] Current context.
    char** targetCtxPtrPtr,         ///< [OUT] New context, or NULL if no change is needed.
    int* errPtr                     ///< [OUT] errno value on error.
)
{
    *targetCtxPtrPtr = NULL;

    char* specCtxPtr = NULL;
    if (selabel_lookup_raw(hndPtr, &specCtxPtr, pathPtr, mode) < 0)
    {
        if (errno == ENOENT)
        {
            // No file_contexts entry for this path: restorecon leaves it as it is.
            return true;
        }
        *errPtr = errno;
        return false;
    }

    bool ok = true;
    if ((strcmp(specCtxPtr, "<<none>>") != 0) && (is_context_customizable(curCtxPtr) <= 0))
    {
        errno = 0;
        context_t specCtx = context_new(specCtxPtr);
        context_t curCtx = context_new(curCtxPtr);
        const char* specTypePtr = (specCtx != NULL) ? context_type_get(specCtx) : NULL;
        const char* curTypePtr = (curCtx != NULL) ? context_type_get(curCtx) : NULL;

        if ((specTypePtr == NULL) || (curTypePtr == NULL))
        {
            *errPtr = (errno != 0) ? errno : EINVAL;
            ok = false;
        }
        else if (strcmp(curTypePtr, specTypePtr) != 0)
        {
            char* strPtr = NULL;
            if ((context_type_set(curCtx, specTypePtr) != 0)
                || ((strPtr = context_str(curCtx)) == NULL)
                || ((*targetCtxPtrPtr = strdup(strPtr)) == NULL))
            {
                *errPtr = (errno != 0) ? errno : ENOMEM;
                ok = false;
            }
        }

        if (curCtx != NULL)
        {
            context_free(curCtx);
        }
        if (specCtx != NULL)
        {
            context_free(specCtx);
        }
    }

    freecon(specCtxPtr);
    return ok;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check pass: count the entries whose context differs from the file_contexts.
 */
//--------------------------------------------------------------------------------------------------
static bool CheckEntry
(
    void* contextPtr,
    const char* pathPtr,
    mode_t mode,
    int* errPtr
)
{
    Migration_t* migPtr = contextPtr;

    char* curCtxPtr = NULL;
    if (lgetfilecon_raw(pathPtr, &curCtxPtr) < 0)
    {
        *errPtr = errno;
        return false;
    }

    char* targetCtxPtr = NULL;
    bool ok = GetTargetContext(migPtr->hndPtr, pathPtr, mode, curCtxPtr, &targetCtxPtr, errPtr);
    if (ok && (targetCtxPtr != NULL))
    {
        migPtr->pendingCount++;
        if (migPtr->pendingCount == 1)
        {
            SELINUXCTX_NOTICE("First pending context change: '%s' from '%s' to '%s'.", pathPtr,
                              curCtxPtr, targetCtxPtr);
        }
    }

    free(targetCtxPtr);
    freecon(curCtxPtr);
    return ok;
}


//--------------------------------------------------------------------------------------------------
/**
 * Backup pass, only done when there is no context backup yet: record the current context of every
 * entry. Nothing was relabeled yet, so the current context of every entry is its old context.
 * An entry that is not recorded (its context cannot be read) is not relabeled.
 */
//--------------------------------------------------------------------------------------------------
static bool BackupEntry
(
    void* contextPtr,
    const char* pathPtr,
    mode_t mode,
    int* errPtr
)
{
    (void)mode;
    Migration_t* migPtr = contextPtr;

    if (strchr(pathPtr, '\n') != NULL)
    {
        // The backup has one entry per line: such a path cannot be saved, so it is not relabeled.
        LE_WARN("'%s' has a new line in its path, its context is not saved.", pathPtr);
        return true;
    }

    char* curCtxPtr = NULL;
    if (lgetfilecon_raw(pathPtr, &curCtxPtr) < 0)
    {
        *errPtr = errno;
        return false;
    }

    bool ok = (selinuxCtx_AddEntry(&migPtr->newBackup, curCtxPtr, pathPtr) == LE_OK);
    freecon(curCtxPtr);

    if (!ok)
    {
        *errPtr = ENOMEM;
        return false;
    }

    if (strcmp(pathPtr, SELINUXCTX_LE_FS_PATH) == 0)
    {
        migPtr->hasLeFsRoot = true;
    }
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Relabel pass: set the new context of the entries whose context differs from the file_contexts.
 * Only entries whose old context is in the context backup are relabeled, so that a rollback can
 * always restore it.
 */
//--------------------------------------------------------------------------------------------------
static bool RelabelEntry
(
    void* contextPtr,
    const char* pathPtr,
    mode_t mode,
    int* errPtr
)
{
    Migration_t* migPtr = contextPtr;

    if (selinuxCtx_FindEntry(&migPtr->backup, pathPtr) == NULL)
    {
        LE_WARN("'%s' is not in the context backup, not relabeled.", pathPtr);
        migPtr->notInBackupCount++;
        return true;
    }

    char* curCtxPtr = NULL;
    if (lgetfilecon_raw(pathPtr, &curCtxPtr) < 0)
    {
        *errPtr = errno;
        return false;
    }

    char* targetCtxPtr = NULL;
    bool ok = GetTargetContext(migPtr->hndPtr, pathPtr, mode, curCtxPtr, &targetCtxPtr, errPtr);
    if (ok && (targetCtxPtr != NULL))
    {
        if (lsetfilecon_raw(pathPtr, targetCtxPtr) < 0)
        {
            *errPtr = errno;
            ok = false;
        }
        else
        {
            migPtr->relabelCount++;
        }
    }

    free(targetCtxPtr);
    freecon(curCtxPtr);
    return ok;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reset the per-pass state of the migration (called at the start of every attempt of a pass).
 */
//--------------------------------------------------------------------------------------------------
static void ResetPassState
(
    void* contextPtr
)
{
    Migration_t* migPtr = contextPtr;

    migPtr->pendingCount = 0;
    migPtr->relabelCount = 0;
    migPtr->notInBackupCount = 0;
    selinuxCtx_ClearBackup(&migPtr->newBackup);
    migPtr->hasLeFsRoot = (selinuxCtx_FindEntry(&migPtr->backup, SELINUXCTX_LE_FS_PATH) != NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Run one pass over the /data/le_fs and /data/diag trees and log its summary.
 *
 * @return LE_OK if the pass was done, LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RunPass
(
    Migration_t* migPtr,                ///< [IN/OUT] Migration state.
    selinuxCtx_EntryFunc_t func,        ///< [IN] Processing of an entry.
    const char* passNamePtr,            ///< [IN] Name of the pass (for logs).
    size_t* skippedCountPtr             ///< [OUT] Number of entries skipped on error.
)
{
    if (selinuxCtx_RunPass(func, ResetPassState, migPtr, passNamePtr, skippedCountPtr) != LE_OK)
    {
        return LE_FAULT;
    }

    SELINUXCTX_NOTICE("SELinux context %s of '%s' and '%s' done: %zu pending, "
                      "%zu relabeled, %zu not in backup, %zu skipped on error.",
                      passNamePtr, SELINUXCTX_LE_FS_PATH, SELINUXCTX_DIAG_PATH,
                      migPtr->pendingCount, migPtr->relabelCount, migPtr->notInBackupCount,
                      *skippedCountPtr);
    if (*skippedCountPtr > 0)
    {
        SELINUXCTX_ERROR("%zu entries of '%s' and '%s' were skipped on error during the "
                         "SELinux context %s.", *skippedCountPtr, SELINUXCTX_LE_FS_PATH,
                         SELINUXCTX_DIAG_PATH, passNamePtr);
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Load the existing context backup, if any.
 *
 * The backup is written atomically, so it is complete when it exists. A backup that cannot be
 * read, has a malformed line or has no entry for the le_fs root is an error. It is neither removed
 * nor recreated: some entries may already have been relabeled, so their current context is not
 * their old one anymore.
 *
 * @return LE_OK on success (*existsPtr tells whether there is a backup), LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadBackup
(
    Migration_t* migPtr,        ///< [IN/OUT] Migration state.
    bool* existsPtr             ///< [OUT] true if there is a context backup.
)
{
    *existsPtr = false;

    le_result_t result = selinuxCtx_LoadBackup(&migPtr->backup);
    if (result == LE_NOT_FOUND)
    {
        return LE_OK;
    }
    if (result != LE_OK)
    {
        return LE_FAULT;
    }

    // An empty backup has no entry for the le_fs root either.
    if (migPtr->backup.count == 0)
    {
        SELINUXCTX_ERROR("Context backup '%s' has no entry for '%s'.", SELINUXCTX_BACKUP_PATH,
                         SELINUXCTX_LE_FS_PATH);
        return LE_FAULT;
    }

    *existsPtr = true;
    SELINUXCTX_NOTICE("Loaded %zu saved contexts from '%s'.", migPtr->backup.count,
                      SELINUXCTX_BACKUP_PATH);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Make sure that the context backup exists before anything is relabeled.
 *
 * - No backup: it is created with the current context of every entry. Nothing was relabeled yet,
 *   so these are the old contexts.
 * - A backup exists: it was created by a previous run after this update, which may have relabeled
 *   some entries. It is used as it is and never modified, so that an entry created or relabeled
 *   since then is never saved as an "old" context.
 *
 * Errors are retried in rounds.
 *
 * @return LE_OK if the backup is available, LE_FAULT otherwise (nothing must be relabeled).
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EnsureBackup
(
    Migration_t* migPtr         ///< [IN/OUT] Migration state.
)
{
    for (int attempt = 0; attempt <= SELINUXCTX_RETRY_ROUNDS; attempt++)
    {
        if (attempt > 0)
        {
            SELINUXCTX_ERROR("SELinux context backup failed, retry in %d ms (%d/%d).",
                             SELINUXCTX_RETRY_INTERVAL_MS, attempt, SELINUXCTX_RETRY_ROUNDS);
            usleep(SELINUXCTX_RETRY_INTERVAL_MS * 1000);
        }

        bool exists = false;
        if (LoadBackup(migPtr, &exists) != LE_OK)
        {
            continue;
        }

        if (exists)
        {
            SELINUXCTX_NOTICE("Using the existing context backup '%s' (%zu entries), it is not "
                              "modified.", SELINUXCTX_BACKUP_PATH, migPtr->backup.count);
            return LE_OK;
        }

        size_t skippedCount = 0;
        if (RunPass(migPtr, BackupEntry, "backup", &skippedCount) != LE_OK)
        {
            return LE_FAULT;
        }

        if (!migPtr->hasLeFsRoot)
        {
            SELINUXCTX_ERROR("The context of '%s' itself could not be saved.",
                             SELINUXCTX_LE_FS_PATH);
            return LE_FAULT;
        }

        if (selinuxCtx_WriteBackup(&migPtr->newBackup) != LE_OK)
        {
            // If the rename was done but the directory could not be flushed, the next attempt
            // finds the complete backup and uses it.
            continue;
        }

        // The new backup becomes the backup in use.
        selinuxCtx_ClearBackup(&migPtr->backup);
        migPtr->backup = migPtr->newBackup;
        migPtr->newBackup = (selinuxCtx_Backup_t)SELINUXCTX_BACKUP_INIT;
        selinuxCtx_SortBackup(&migPtr->backup);

        SELINUXCTX_NOTICE("Created context backup '%s' (%zu entries).", SELINUXCTX_BACKUP_PATH,
                          migPtr->backup.count);
        return LE_OK;
    }

    SELINUXCTX_ERROR("SELinux context backup '%s' could not be used after %d retries.",
                     SELINUXCTX_BACKUP_PATH, SELINUXCTX_RETRY_ROUNDS);
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check the whole trees; if some entries have an old context, create the context backup if there
 * is none, relabel the entries of the backup, then check the whole trees again.
 *
 * @return LE_OK if the migration was done (some entries may have been skipped on error), LE_FAULT
 *         if it could not be done.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RunMigration
(
    Migration_t* migPtr         ///< [IN/OUT] Migration state.
)
{
    size_t skippedCount = 0;

    if (RunPass(migPtr, CheckEntry, "check", &skippedCount) != LE_OK)
    {
        return LE_FAULT;
    }

    if (migPtr->pendingCount == 0)
    {
        SELINUXCTX_NOTICE("Contexts of '%s' and '%s' are up to date, skip relabel.",
                          SELINUXCTX_LE_FS_PATH, SELINUXCTX_DIAG_PATH);
        return LE_OK;
    }

    SELINUXCTX_NOTICE("%zu entries of '%s' and '%s' have an old context, saving the contexts and "
                      "relabeling them.", migPtr->pendingCount, SELINUXCTX_LE_FS_PATH,
                      SELINUXCTX_DIAG_PATH);

    if (EnsureBackup(migPtr) != LE_OK)
    {
        return LE_FAULT;
    }

    if (RunPass(migPtr, RelabelEntry, "relabel", &skippedCount) != LE_OK)
    {
        return LE_FAULT;
    }

    size_t relabelCount = migPtr->relabelCount;
    size_t relabelSkippedCount = skippedCount;
    size_t notInBackupCount = migPtr->notInBackupCount;

    if (RunPass(migPtr, CheckEntry, "verify", &skippedCount) != LE_OK)
    {
        return LE_FAULT;
    }

    if (migPtr->pendingCount == 0)
    {
        SELINUXCTX_NOTICE("Relabel of '%s' and '%s' done and verified (%zu relabeled).",
                          SELINUXCTX_LE_FS_PATH, SELINUXCTX_DIAG_PATH, relabelCount);
    }
    else
    {
        SELINUXCTX_ERROR("%zu entries of '%s' and '%s' still have an old context after relabel "
                         "(%zu relabeled, %zu skipped on error, %zu not in the backup); they are "
                         "tried again on the next boot.", migPtr->pendingCount,
                         SELINUXCTX_LE_FS_PATH, SELINUXCTX_DIAG_PATH, relabelCount,
                         relabelSkippedCount, notInBackupCount);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Migrate the SELinux contexts of /data/le_fs and /data/diag to the file_contexts of the running
 * image.
 */
//--------------------------------------------------------------------------------------------------
void selinuxCtx_Migrate
(
    void
)
{
    // libselinux computes is_selinux_enabled() once, in its constructor: it needs to statfs()
    // /sys/fs/selinux. If that is denied, the migration would be silently skipped: report it as
    // an error.
    if (is_selinux_enabled() <= 0)
    {
        SELINUXCTX_ERROR("SELinux is not enabled, skip the context migration of '%s' and '%s'.",
                         SELINUXCTX_LE_FS_PATH, SELINUXCTX_DIAG_PATH);
        return;
    }

    SELINUXCTX_NOTICE("SELinux context migration of '%s' and '%s' started.",
                      SELINUXCTX_LE_FS_PATH, SELINUXCTX_DIAG_PATH);

    Migration_t mig;
    memset(&mig, 0, sizeof(mig));

    for (int attempt = 0; (attempt <= SELINUXCTX_RETRY_ROUNDS) && (mig.hndPtr == NULL); attempt++)
    {
        if (attempt > 0)
        {
            usleep(SELINUXCTX_RETRY_INTERVAL_MS * 1000);
        }

        mig.hndPtr = selabel_open(SELABEL_CTX_FILE, NULL, 0);
        if (mig.hndPtr == NULL)
        {
            SELINUXCTX_ERROR("Could not open the file_contexts of the running image: %s "
                             "(attempt %d/%d).", strerror(errno), attempt + 1,
                             SELINUXCTX_RETRY_ROUNDS + 1);
        }
    }

    if ((mig.hndPtr == NULL) || (RunMigration(&mig) != LE_OK))
    {
        SELINUXCTX_ERROR("SELinux context migration of '%s' and '%s' not completed; it is tried "
                         "again on the next boot.", SELINUXCTX_LE_FS_PATH, SELINUXCTX_DIAG_PATH);
    }
    else
    {
        SELINUXCTX_NOTICE("SELinux context migration of '%s' and '%s' finished.",
                          SELINUXCTX_LE_FS_PATH, SELINUXCTX_DIAG_PATH);
    }

    if (mig.hndPtr != NULL)
    {
        selabel_close(mig.hndPtr);
    }
    selinuxCtx_ClearBackup(&mig.backup);
    selinuxCtx_ClearBackup(&mig.newBackup);
}

#endif // LE_CONFIG_ENABLE_SELINUX