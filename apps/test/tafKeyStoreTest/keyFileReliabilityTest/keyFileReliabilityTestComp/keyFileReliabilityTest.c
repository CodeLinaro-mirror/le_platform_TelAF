/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       keyFileReliabilityTest.c
 * @brief      This file includes test functions for key file reliability of keyStore Service.
 */

#include "legato.h"
#include "interfaces.h"
#include <sys/wait.h>
#include <dirent.h>

//--------------------------------------------------------------------------------------------------
/**
 * Key reference and crypto session reference.
 */
//--------------------------------------------------------------------------------------------------
taf_ks_KeyRef_t KeyRef = NULL;
taf_ks_CryptoSessionRef_t SessionRef = NULL;
const uint8_t Message[TAF_KS_MAX_PACKET_SIZE/2] =
    "Keystore service ECDSA Signing Verification reliability test message.";
uint8_t Signature[TAF_KS_MAX_PACKET_SIZE] = { 0 };
size_t SignatureSize = TAF_KS_MAX_PACKET_SIZE;

//--------------------------------------------------------------------------------------------------
/**
 * Test key ID and the names of the key files the keyStore service stores it in.
 *
 * Neither name is derivable here: the active copy is named after
 * SHA256(APP ID + KEY ID + SEC ID) and the backup copy follows a separate
 * scheme of the service's own, so both must be captured from the target and
 * regenerated whenever KeyId, the app name, or either scheme changes.
 */
//--------------------------------------------------------------------------------------------------
const char KeyId[] = "ECDSA_reliability_Test_Key";
const char KeyFile[] = "95326CF367BDF01071619D98201B478085E839D6E4EEF9401DEA220EB2823416.key";
const char KeyBackupFile[] = "bak_d38ce1fcd33722ffe71e9909fcdb309145e7deae";

//--------------------------------------------------------------------------------------------------
/**
 * Test action type to key file.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    DELETE_ACTIVE_KEY_FILE_RESTART_RESTORE,   ///< Delete active key file for restart restore.
    DELETE_BACKUP_KEY_FILE_RESTART_RESTORE,   ///< Delete backup key file for restart restore.
    DELETE_ACTIVE_KEY_FILE_RUNTIME_RESTORE,   ///< Delete active key file for runtime restore.
    DELETE_BACKUP_KEY_FILE_RUNTIME_RESTORE,   ///< Delete backup key file for runtime restore.
    CORRUPT_ACTIVE_KEY_FILE_RESTART_RESTORE,  ///< Corrupt active key file for restart restore.
    CORRUPT_BACKUP_KEY_FILE_RESTART_RESTORE,  ///< Corrupt backup key file for restart restore.
    CORRUPT_ACTIVE_KEY_FILE_RUNTIME_RESTORE,  ///< Corrupt active key file for runtime restore.
    CORRUPT_BACKUP_KEY_FILE_RUNTIME_RESTORE,  ///< Corrupt backup key file for runtime restore.
    CHECK_KEY_FILES_RESTORED,                 ///< Check both key files are back and identical.
}
KeyFileAction_t;

//--------------------------------------------------------------------------------------------------
/**
 * Close every open file descriptor except stdin/stdout/stderr.
 *
 * Legato's IPC sockets are created without SOCK_CLOEXEC (see unixSocket.c), so a
 * forked child inherits live service sessions and keeps them open past exec.
 * The framework treats that as something to clean up rather than rely on: the
 * Supervisor itself calls fd_CloseAllNonStd() immediately before exec-ing every
 * app process, and updateDaemon does the same before forking its unpack helper.
 *
 * Empirically, leaking those descriptors into the shell prevents the "app
 * restart tafKeyStoreSvc" below from ever returning. The exact step it blocks on
 * has not been pinned down -- note that it is *not* the service stop, which
 * app_Stop() performs with a plain SIGTERM plus a hard-kill timer and which the
 * logs show completing in well under a second.
 *
 * /proc/self/fd is walked rather than looping to sysconf(_SC_OPEN_MAX), which
 * can be over a million on this target.
 */
//--------------------------------------------------------------------------------------------------
static void CloseInheritedFds
(
    void
)
{
    DIR* fdDirPtr = opendir("/proc/self/fd");
    if (fdDirPtr == NULL)
    {
        for (int fd = 3; fd < 1024; fd++)
        {
            close(fd);
        }
        return;
    }

    int dirFd = dirfd(fdDirPtr);
    struct dirent* entryPtr;
    while ((entryPtr = readdir(fdDirPtr)) != NULL)
    {
        if (entryPtr->d_name[0] == '.')
        {
            continue;
        }

        int fd = (int)strtol(entryPtr->d_name, NULL, 10);
        if ((fd >= 3) && (fd != dirFd))
        {
            close(fd);
        }
    }

    closedir(fdDirPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Run a shell command, closing inherited descriptors in the child before exec.
 *
 * Used instead of system() because system()'s child keeps this process's open
 * Legato IPC sessions, which stalls the service restart the command performs.
 *
 * @return LE_OK if the command exited with EXIT_SUCCESS, LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RunShellCommand
(
    const char* cmdPtr
)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        LE_ERROR("fork() failed: %m");
        return LE_FAULT;
    }

    if (pid == 0)
    {
        CloseInheritedFds();
        execl("/bin/sh", "sh", "-c", cmdPtr, (char*)NULL);
        _exit(EXIT_FAILURE);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0)
    {
        if (errno != EINTR)
        {
            LE_ERROR("waitpid() failed: %m");
            return LE_FAULT;
        }
    }

    return (WIFEXITED(status) && (WEXITSTATUS(status) == EXIT_SUCCESS)) ? LE_OK : LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Perform test action to key file per as action type.
 */
//--------------------------------------------------------------------------------------------------
static void TakeActionToKeyFile
(
  KeyFileAction_t actionType
)
{
    char command[512];
    char action[64];

    switch (actionType)
    {
        case DELETE_ACTIVE_KEY_FILE_RESTART_RESTORE:
            snprintf(action, sizeof(action), "del_act_restart_restore");
            break;
        case DELETE_BACKUP_KEY_FILE_RESTART_RESTORE:
            snprintf(action, sizeof(action), "del_bak_restart_restore");
            break;
        case DELETE_ACTIVE_KEY_FILE_RUNTIME_RESTORE:
            snprintf(action, sizeof(action), "del_act_runtime_restore");
            break;
        case DELETE_BACKUP_KEY_FILE_RUNTIME_RESTORE:
            snprintf(action, sizeof(action), "del_bak_runtime_restore");
            break;
        case CORRUPT_ACTIVE_KEY_FILE_RESTART_RESTORE:
            snprintf(action, sizeof(action), "corrupt_act_restart_restore");
            break;
        case CORRUPT_BACKUP_KEY_FILE_RESTART_RESTORE:
            snprintf(action, sizeof(action), "corrupt_bak_restart_restore");
            break;
        case CORRUPT_ACTIVE_KEY_FILE_RUNTIME_RESTORE:
            snprintf(action, sizeof(action), "corrupt_act_runtime_restore");
            break;
        case CORRUPT_BACKUP_KEY_FILE_RUNTIME_RESTORE:
            snprintf(action, sizeof(action), "corrupt_bak_runtime_restore");
            break;
        case CHECK_KEY_FILES_RESTORED:
            snprintf(action, sizeof(action), "check_files_restored");
            break;
        default:
            LE_TEST_FATAL("unknown test action.");
            break;
    }

    snprintf(command, sizeof(command), "sh /legato/systems/current/appsWriteable/" \
        "keyFileReliabilityTest/bin/keyFileReliabilityTest.sh %s %s %s",
        action, KeyFile, KeyBackupFile);

    le_result_t result = LE_OK;
    if (RunShellCommand(command) != LE_OK)
    {
        result = LE_FAULT;
    }

    LE_TEST_ASSERT(result == LE_OK, "Run keyFileReliabilityTest script.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Create ECDSA key for key file reliability test.
 */
//--------------------------------------------------------------------------------------------------
static void CreateAndProvisionKey (void)
{
    LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(KeyId, TAF_KS_ECDSA_SIGN_VERIFY, &KeyRef),
                   "Create ECDSA key fore reliability test.");
    LE_TEST_ASSERT(LE_OK == taf_ks_ProvisionEcdsaKeyValue(KeyRef,
        TAF_KS_ECC_SIZE_521, TAF_KS_DIGEST_SHA2_512,NULL, 0),
        "Provision ECDSA key.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Sign text message by using the ECDSA key and keyStore APIs.
 */
//--------------------------------------------------------------------------------------------------
static void SignData (void)
{
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(KeyRef, &SessionRef),
                   "Create signing session.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(SessionRef, TAF_KS_CRYPTO_SIGN),
                   "Start signing session.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionProcess(SessionRef,
        Message, sizeof(Message), NULL, 0),
        "Message to sign(size = %"PRIuS"): %s", sizeof(Message), Message);
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionEnd(SessionRef, NULL, 0, Signature, &SignatureSize),
        "Signature size = %"PRIuS".", SignatureSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sign text message by using the ECDSA key and keyStore APIs.
 */
//--------------------------------------------------------------------------------------------------
static void VerifySignature (void)
{
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(KeyRef, &SessionRef),
                   "Create verfication session.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(SessionRef, TAF_KS_CRYPTO_VERIFY),
                   "Start verification session.");
    LE_TEST_ASSERT(LE_OK ==taf_ks_CryptoSessionProcess(SessionRef,
        Message, sizeof(Message), NULL, 0),
        "Message to verify(size = %"PRIuS"): %s", sizeof(Message), Message);
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionEnd(SessionRef, Signature, SignatureSize, NULL, 0),
        "Verify signature.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete the test key.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteKey (void)
{
    LE_TEST_ASSERT(LE_OK == taf_ks_DeleteKey(KeyRef), "Delete signing key.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test body.
 */
//--------------------------------------------------------------------------------------------------
static void RunTests
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_UNUSED(param1Ptr);
    LE_UNUSED(param2Ptr);

    LE_TEST_INFO("=== telaf keyStore key file reliability test BEGIN ===");

    taf_ks_ConnectService();
    CreateAndProvisionKey();
    SignData();
    // Disconnect before the restart cases: the session must be gone before the
    // service is torn down and brought back under it.
    taf_ks_DisconnectService();

    // Cases 1 and 5 (delete the *active* key file) are disabled: restoring from
    // a deleted active copy is not supported in TAF_26.09.00 and is planned for
    // a later release. Re-enable both once the service supports it.
    // LE_TEST_INFO("Delete active key file and test restoration after restart.");
    // TakeActionToKeyFile(DELETE_ACTIVE_KEY_FILE_RESTART_RESTORE);
    LE_TEST_INFO("Delete backup key file and test restoration after restart.");
    TakeActionToKeyFile(DELETE_BACKUP_KEY_FILE_RESTART_RESTORE);
    LE_TEST_INFO("Corrupt active key file and test restoration after restart.");
    TakeActionToKeyFile(CORRUPT_ACTIVE_KEY_FILE_RESTART_RESTORE);
    LE_TEST_INFO("Corrupt backup key file and test restoration after restart.");
    TakeActionToKeyFile(CORRUPT_BACKUP_KEY_FILE_RESTART_RESTORE);

    taf_ks_ConnectService();

    // KeyRef indexed the previous service instance's in-memory key table, and
    // the restart cases above replaced that instance. The key file is still on
    // disk, so look the key up again to get a handle valid in the new instance.
    LE_TEST_ASSERT(LE_OK == taf_ks_GetKey(KeyId, &KeyRef),
                   "Re-fetch key reference after service restart.");

    // LE_TEST_INFO("Delete active key file and test runtime restoration.");
    // TakeActionToKeyFile(DELETE_ACTIVE_KEY_FILE_RUNTIME_RESTORE);
    // VerifySignature();
    // TakeActionToKeyFile(CHECK_KEY_FILES_RESTORED);

    LE_TEST_INFO("Delete backup key file and test runtime restoration.");
    TakeActionToKeyFile(DELETE_BACKUP_KEY_FILE_RUNTIME_RESTORE);
    VerifySignature();
    TakeActionToKeyFile(CHECK_KEY_FILES_RESTORED);

    LE_TEST_INFO("Corrupt active key file and test runtime restoration.");
    TakeActionToKeyFile(CORRUPT_ACTIVE_KEY_FILE_RUNTIME_RESTORE);
    VerifySignature();
    TakeActionToKeyFile(CHECK_KEY_FILES_RESTORED);

    LE_TEST_INFO("Corrupt backup key file and test runtime restoration.");
    TakeActionToKeyFile(CORRUPT_BACKUP_KEY_FILE_RUNTIME_RESTORE);
    VerifySignature();
    TakeActionToKeyFile(CHECK_KEY_FILES_RESTORED);

    DeleteKey();
    taf_ks_DisconnectService();

    LE_TEST_INFO("=== telaf Keystore key file reliability test END ===");

    LE_TEST_EXIT;
}

COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    le_event_QueueFunction(RunTests, NULL, NULL);
}
