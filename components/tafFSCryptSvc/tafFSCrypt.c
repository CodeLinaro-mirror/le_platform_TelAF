/*
 * Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "legato.h"
#include "interfaces.h"

#include "tafFSCrypt.h"

#include "limit.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/md5.h>

//--------------------------------------------------------------------------------------------------
/**
 * Key name of internal AES GCM key.
 */
//--------------------------------------------------------------------------------------------------
#define FSC_INT_KEY_NAME "fscIntAesGcmKey"

//--------------------------------------------------------------------------------------------------
/**
 * Key parameters of internal AES GCM key.
 */
//--------------------------------------------------------------------------------------------------
#define INT_KEY_NONCE_LEN 12
#define INT_KEY_AEAD_LEN  32

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for standard keys.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t StorageRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for standard keys.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t StoragePool;

//--------------------------------------------------------------------------------------------------
/**
 * Message list created with 'CreateRxMsgList' function
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_List_t        list;
}
taf_fsc_StorageList_t;

//--------------------------------------------------------------------------------------------------
/**
 * Key structure for standard keys.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_fsc_StorageRef_t        storageRef;                             ///< Reference to the FS-Crypt storage
    KeyMgt_KeyFileRef_t         keyFileRef;                             ///< Key file reference to the key file
    le_msg_SessionRef_t         clientSessionRef;                       ///< Client session reference
    char                        dirpath[TAF_FSC_MAX_STORAGE_NAME_SIZE]; ///< Directory path
    char                        descriptor[FS_KEY_DESCRIPTOR_HEX_SIZE]; ///< Descriptor is used for IO control
    bool                        keyIsAddedToKernel;                        ///< Ture if the key is added to kernel
}
taf_fsc_Storage_t;

//--------------------------------------------------------------------------------------------------
/**
 * Checks whether the specified directory is empty.
 */
//--------------------------------------------------------------------------------------------------
static bool IsDirectoryEmpty(const char *dirname)
{
    int n = 0;
    struct dirent *d;
    DIR *dir = opendir(dirname);

    if(dir == NULL)
    {
        LE_ERROR("dir == NULL");
        return false;
    }

    while ((d = readdir(dir)) != NULL)
    {
        if(++n > 2)
            break;
    }
    closedir(dir);

    if (n <= 2)
    {
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Transforms an input key descriptor into a byte array and outputs a hex string.
 */
//--------------------------------------------------------------------------------------------------
static void key_descriptor_to_hex
(
    const uint8_t bytes[FS_KEY_DESCRIPTOR_SIZE],
    char hex[FS_KEY_DESCRIPTOR_HEX_SIZE]
)
{
    LE_DEBUG("descriptor_to_hex:");
    for (uint i = 0; i < FS_KEY_DESCRIPTOR_SIZE; ++i)
    {
      snprintf(hex + 2 * i, 3, "%02x", bytes[i]);

      LE_DEBUG("%02x", bytes[i]);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The descriptor is just the first 8 bytes of a double application of SHA512
 * formatted as hex (so 16 characters).
 */
//--------------------------------------------------------------------------------------------------
static void compute_descriptor(const uint8_t key[FSC_MAX_KEY_SIZE],
                               char descriptor[FS_KEY_DESCRIPTOR_HEX_SIZE])
{
    uint8_t digest1[EVP_MAX_MD_SIZE];
    uint8_t digest2[EVP_MAX_MD_SIZE];

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    const EVP_MD* method = EVP_sha512();

    if(ctx == NULL)
    {
        LE_ERROR("ctx is NULL");
        return;
    }
    // double hash the key

    // first hash
    EVP_DigestInit_ex(ctx, method, NULL);
    EVP_DigestUpdate(ctx, key, FSC_MAX_KEY_SIZE);
    EVP_DigestFinal_ex(ctx, digest1, NULL);
    EVP_MD_CTX_free(ctx);

    // second hash
    ctx = EVP_MD_CTX_new();
    if(ctx == NULL)
    {
        LE_ERROR("ctx is NULL");
        return;
    }
    EVP_DigestInit_ex(ctx, method, NULL);
    EVP_DigestUpdate(ctx, digest1, EVP_MAX_MD_SIZE);
    EVP_DigestFinal_ex(ctx, digest2, NULL);
    EVP_MD_CTX_free(ctx);

    key_descriptor_to_hex(digest2, descriptor);

    memset(digest1, 0, EVP_MAX_MD_SIZE);
    memset(digest2, 0, EVP_MAX_MD_SIZE);
}

//--------------------------------------------------------------------------------------------------
/**
 * Inserts a key read into the kernel keyring. This has the effect of unlocking files
 * encrypted with that key.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t IoControl_add_key
(
    uint8_t *key,
    const char* descriptor,
    const char* dirpath
)
{
    LE_INFO("dirpath = %s", dirpath);

    struct fscrypt_add_key_arg *arg = calloc(sizeof(*arg) + FSC_MAX_KEY_SIZE, 1);
    if (!arg)
    {
        LE_ERROR("error: failed to allocate memory");
        return LE_FAULT;
    }

    arg->raw_size = FSC_MAX_KEY_SIZE;
    memcpy(arg->raw, key, FSC_MAX_KEY_SIZE);
    memcpy(arg->key_spec.u.descriptor, descriptor, FS_KEY_DESCRIPTOR_SIZE);

    arg->key_spec.type = FSCRYPT_KEY_SPEC_TYPE_DESCRIPTOR;

    LE_DEBUG("arg->raw_size2 = %u", arg->raw_size);

    int fd = le_fd_Open(dirpath, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
    {
        LE_ERROR("error: opening %s: %s", dirpath, LE_ERRNO_TXT(errno));

        memset(arg->raw, 0, arg->raw_size);
        free(arg);

        return LE_FAULT;
    }

    if (le_fd_Ioctl(fd, FS_IOC_ADD_ENCRYPTION_KEY, arg) != 0)
    {
        LE_ERROR("error: adding key to %s: %s", dirpath, LE_ERRNO_TXT(errno));
        le_fd_Close(fd);

        memset(arg->raw, 0, arg->raw_size);
        free(arg);

        return LE_FAULT;
    }

    le_fd_Close(fd);

#ifdef TAF_FSC_DEBUG
    char descryptor_hex[FS_KEY_DESCRIPTOR_HEX_SIZE];
    le_hex_BinaryToString(arg->key_spec.u.descriptor, FS_KEY_DESCRIPTOR_SIZE,
                descryptor_hex, FS_KEY_DESCRIPTOR_HEX_SIZE);
    LE_INFO("descriptor: %s", descryptor_hex);
#endif

    memset(key, 0, FSC_MAX_KEY_SIZE);
    memset(arg->raw, 0, arg->raw_size);

    free(arg);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns error messages according to errno values.
 */
//--------------------------------------------------------------------------------------------------
const char *txt_errno_policy(int errno_val)
{
    switch (errno_val)
    {
        case EEXIST:
            return "This storage is already encrypted";
        case EINVAL:
            return "Invalid encryption argument";
        case ENODATA:
            return "Directory is not encrypted";
        default:
            return strerror(errno_val);
    }
}

#ifndef TAF_FCS_ERRNO_TXT
#define TAF_FCS_ERRNO_TXT(v) txt_errno_policy(v)
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Applies a policy (i.e. the specified descriptor) to the specified directory.
 * The policy options defaults can be overridden by command-line options.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t IoControl_set_policy(const char* descriptor, const char *dirpath)
{
    LE_INFO("dirpath = %s", dirpath);

    struct fscrypt_policy policy =
    {
        .version = 0,
        .contents_encryption_mode = FS_ENCRYPTION_MODE_AES_256_XTS,
        .filenames_encryption_mode = FS_ENCRYPTION_MODE_AES_256_CTS,
        // Use maximum zero-padding to leak less info about filename length
        .flags = FS_POLICY_FLAGS_PAD_32
    };

    memcpy(policy.master_key_descriptor, descriptor, FS_KEY_DESCRIPTOR_SIZE);

    // Policies can only be set on directories
    int fd = le_fd_Open(dirpath, O_RDONLY | O_DIRECTORY);
    if (fd < 0)
    {
        LE_ERROR("set_policy - error: opening %s: %s", dirpath, LE_ERRNO_TXT(errno));
        return LE_FAULT;
    }

    int ret = le_fd_Ioctl(fd, FS_IOC_SET_ENCRYPTION_POLICY, &policy);

    if (ret != 0)
    {
        LE_ERROR("set_policy - error: adding key to %s: %s", dirpath, TAF_FCS_ERRNO_TXT(errno));
        le_fd_Close(fd);

        if(EEXIST == errno)
        {
            return LE_DUPLICATE;
        }

        return LE_FAULT;
    }

    le_fd_Close(fd);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the policy information from a specified file or directory with encryption enabled
 */
//--------------------------------------------------------------------------------------------------
static le_result_t IoControl_get_policy(const char *dirpath, struct fscrypt_policy *policy)
{
    LE_INFO("dirpath = %s", dirpath);

    // Policies can only be set on directories
    int fd = le_fd_Open(dirpath, O_RDONLY);
    if (fd < 0)
    {
        LE_ERROR("get_policy - error: opening %s: %s", dirpath, LE_ERRNO_TXT(errno));
        return LE_FAULT;
    }

    int ret = le_fd_Ioctl(fd, FS_IOC_GET_ENCRYPTION_POLICY, policy);

    if (ret != 0)
    {
        // Check whether the dir is already encrypted
        if(errno == ENODATA)
        {
            le_fd_Close(fd);
            LE_INFO("get_policy: get policy for %s: %s", dirpath, TAF_FCS_ERRNO_TXT(errno));
            return LE_UNAVAILABLE;
        }
        else
        {
            LE_ERROR("get_policy - error: get policy for %s: %s",
                     dirpath, TAF_FCS_ERRNO_TXT(errno));
        }
        le_fd_Close(fd);

        return LE_FAULT;
    }

    le_fd_Close(fd);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes a key from kernel keyring for the specified directory.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t IoControl_remove_key(const char* descriptor, const char *dirpath)
{
    LE_INFO("dirpath = %s", dirpath);

    struct fscrypt_remove_key_arg arg = {0};

    memcpy(arg.key_spec.u.identifier, descriptor, FS_KEY_DESCRIPTOR_SIZE);
    arg.key_spec.type = FSCRYPT_KEY_SPEC_TYPE_DESCRIPTOR;

    int fd = le_fd_Open(dirpath, O_RDONLY | O_CLOEXEC);
    LE_DEBUG("fd = %d", fd);

    if (fd < 0)
    {
        LE_ERROR("IoControl_remove_key - error: opening %s: %s", dirpath, strerror(errno));
        return LE_FAULT;
    }

    int ret = le_fd_Ioctl(fd, FS_IOC_REMOVE_ENCRYPTION_KEY, &arg);

    le_fd_Close(fd);

    if (ret != 0)
    {
        LE_ERROR("IoControl_remove_key - error: removing key: %s", strerror(errno));
        return LE_FAULT;
    }

    if (arg.removal_status_flags & FS_KEY_REMOVAL_STATUS_FLAG_OTHER_USERS)
    {
        LE_WARN("warning: other users still have this key added");
    }
    else if (arg.removal_status_flags & FS_KEY_REMOVAL_STATUS_FLAG_FILES_BUSY)
    {
        LE_WARN("warning: some files using this key are still in-use");
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes FS-Crypt key from kernel keyring to lock the directory.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Bad parameters.
 *     - LE_NOT_FOUND -- The key does not exist or is not provisioned.
 *     - LE_FAULT -- Error in process.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_fsc_LockStorage
(
    taf_fsc_StorageRef_t StorageRef
)
{
    if(StorageRef == NULL)
    {
        LE_ERROR("StorageRef is NULL");
        return LE_BAD_PARAMETER;
    }

    taf_fsc_Storage_t* storagePtr = (taf_fsc_Storage_t*)le_ref_Lookup(StorageRefMap, StorageRef);

    if(storagePtr == NULL)
    {
        LE_ERROR("storagePtr == NULL");
        return LE_FAULT;
    }

    le_result_t res = LE_OK;

    if(storagePtr->keyIsAddedToKernel)
    {
        res = IoControl_remove_key(storagePtr->descriptor, storagePtr->dirpath);
    }

    if(res == LE_OK)
    {
        storagePtr->keyIsAddedToKernel = false;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds FS-Crypt key to kernel keyring to unlock the directory.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Bad parameters.
 *     - LE_NOT_FOUND -- Key does not exist.
 *     - LE_FAULT -- Error in process.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_fsc_UnlockStorage
(
    taf_fsc_StorageRef_t StorageRef
)
{
    if(StorageRef == NULL)
    {
        LE_ERROR("StorageRef is NULL");
        return LE_BAD_PARAMETER;
    }

    KeyMgt_KeyFileRef_t keyFileRef = NULL;

    uint8_t key[FSC_MAX_KEY_SIZE] = {0};

    taf_fsc_Storage_t* storagePtr = (taf_fsc_Storage_t*)le_ref_Lookup(StorageRefMap, StorageRef);

    if(storagePtr == NULL)
    {
        LE_ERROR("storagePtr == NULL");
        return LE_FAULT;
    }

    // Process PA layer validation and get raw key
    le_result_t res = taf_pa_fsc_GetKey(taf_fsc_GetClientSessionRef(),
                                        storagePtr->dirpath,
                                        &keyFileRef,
                                        key,
                                        FSC_MAX_KEY_SIZE);

    if(keyFileRef != storagePtr->keyFileRef)
    {
        LE_ERROR("keyFileRef != storagePtr->keyFileRef");
        return LE_FAULT;
    }

    res = IoControl_add_key(key, storagePtr->descriptor, storagePtr->dirpath);

    if(res == LE_OK)
    {
        storagePtr->keyIsAddedToKernel = true;
    }

    memset(key, 0, FSC_MAX_KEY_SIZE);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Cleans up the whole FS-Crypt directory and ecryption key.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Bad parameters.
 *     - LE_NOT_FOUND -- Key does not exist.
 *     - LE_FAULT -- Error in process.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_fsc_DeleteStorage
(
    taf_fsc_StorageRef_t StorageRef
)
{
    if(StorageRef == NULL)
    {
        LE_ERROR("StorageRef is NULL");
        return LE_BAD_PARAMETER;
    }

    taf_fsc_Storage_t* storagePtr = (taf_fsc_Storage_t*)le_ref_Lookup(StorageRefMap, StorageRef);

    if(storagePtr == NULL)
    {
        LE_ERROR("storagePtr == NULL");
        return LE_FAULT;
    }

    le_result_t res = LE_OK;

    if(storagePtr->keyIsAddedToKernel)
    {
        res = IoControl_remove_key(storagePtr->descriptor, storagePtr->dirpath);
    }

    if(LE_OK == res)
    {
        storagePtr->keyIsAddedToKernel = false;

        LE_DEBUG("RemoveRecursive '%s'", storagePtr->dirpath);
        res = le_dir_RemoveRecursive(storagePtr->dirpath);
    }

    if(LE_OK == res)
    {
        res = taf_pa_fsc_DeleteKey(storagePtr->clientSessionRef, storagePtr->keyFileRef);
    }

    if(LE_OK == res)
    {
        // Free the storage object and reference
        le_ref_DeleteRef(StorageRefMap, StorageRef);
        le_mem_Release(storagePtr);
    }

    LE_DEBUG("taf_fsc_DeleteStorage: %s", LE_RESULT_TXT(res));

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Finds exsisting storage node for specifeied client input.
 * This is for the client that has been closed and start again,
 * requesting for the existing FS-Crypt storage
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FindAppStorageRef
(
    KeyMgt_KeyFileRef_t     keyFileRef,
    const char              dirPath[TAF_FSC_MAX_STORAGE_NAME_SIZE],
    char                    descriptor[FS_KEY_DESCRIPTOR_HEX_SIZE],
    taf_fsc_StorageRef_t*   storageRef
)
{
    LE_DEBUG("FindAppStorageRef");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(StorageRefMap);

    // Scan all the storage nodes
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_fsc_Storage_t* storagePtr = le_ref_GetValue(iterRef);
        if(storagePtr == NULL)
        {
            LE_ERROR("storagePtr == NULL");
            return LE_FAULT;
        }

        // Find the node that context matches to the current client and storage but session has been closed
        if ((storagePtr->keyFileRef == keyFileRef) &&
            (strcmp(storagePtr->dirpath, dirPath) == 0) &&
            (strcmp(storagePtr->descriptor, descriptor) == 0) &&
            (storagePtr->clientSessionRef == NULL))
        {
            storagePtr->clientSessionRef = taf_fsc_GetClientSessionRef();
            (*storageRef) = (void*)le_ref_GetSafeRef(iterRef);

            LE_DEBUG("Find storage '%s' for client session (%p)",
                    storagePtr->dirpath, storagePtr->clientSessionRef);

            return LE_OK;
        }
    }

    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference of FS-Crypt storage object.
 *
 * @return
 *     - Storage -- The reference of FS-Crypt storage object.
 *     - NULL -- Fail in process.
 */
//--------------------------------------------------------------------------------------------------
taf_fsc_StorageRef_t taf_fsc_GetStorageRef
(
    const char  *dirPath,                                ///< Storage name.
    le_result_t *result                                  ///< error status.
)
{
    if(result == NULL)
    {
        LE_ERROR("result is NULL");
        return NULL;
    }

    KeyMgt_KeyFileRef_t keyFileRef = NULL;

    uint8_t key[FSC_MAX_KEY_SIZE] = {0};
    bool storageAlreadyExist = false;

    // Check whether the specifid direcroty already exists
    if (le_dir_IsDir(dirPath))
    {
        *result = taf_pa_fsc_GetKey(taf_fsc_GetClientSessionRef(),
                                        dirPath,
                                        &keyFileRef,
                                        key,
                                        FSC_MAX_KEY_SIZE);

        LE_INFO("taf_pa_fsc_GetKey - %s", LE_RESULT_TXT(*result));
        if(*result == LE_NOT_FOUND)
        {
            if(IsDirectoryEmpty(dirPath) == false)
            {
                LE_ERROR("Directory: %s is not empty", dirPath);
                *result = LE_NOT_PERMITTED;
                goto exception;
            }

            struct fscrypt_policy policy;
            if(IoControl_get_policy(dirPath, &policy) == LE_OK)
            {
                LE_ERROR("Directory: %s is already encrypted", dirPath);
                *result = LE_NOT_PERMITTED;
                goto exception;
            }
        }
        else if(*result != LE_OK)
        {
            LE_ERROR("Get key for %s error !", dirPath);
            goto exception;
        }
        else
        {
            storageAlreadyExist = true;
        }
    }
    else
    {
        *result = LE_NOT_FOUND;

        LE_INFO("Try to make path: %s", dirPath);
        if(LE_OK != le_dir_MakePath(dirPath, S_IRWXU))
        {
            LE_ERROR("le_dir_MakePath(%s) failed", dirPath);
            *result = LE_FAULT;
            return NULL;
        }
    }

    LE_DEBUG("result = %s(%d)", LE_RESULT_TXT(*result), *result);

    // If key file doesn't exist or it's new directory, generate new key
    if(*result == LE_NOT_FOUND)
    {
        *result = taf_pa_fsc_GenerateAesKey(taf_fsc_GetClientSessionRef(),
                                                dirPath,
                                                &keyFileRef,
                                                key,
                                                FSC_MAX_KEY_SIZE);

        if(LE_OK != *result)
        {
            LE_ERROR("taf_pa_fsc_GenerateAesKey failed");
            goto exception;
        }
    }

    // Compute descriptor
    char descriptor[FS_KEY_DESCRIPTOR_HEX_SIZE] = {0};
    compute_descriptor(key, descriptor);

    // Add key to kernel keyring
    *result = IoControl_add_key(key, descriptor, dirPath);
    if(LE_OK != *result)
    {
        goto exception;
    }

    // Set policy to new directory to encrypt data
    if(!storageAlreadyExist)
    {
        *result = IoControl_set_policy(descriptor, dirPath);
        if(LE_OK != *result)
        {
            goto exception;
        }
    }

    taf_fsc_StorageRef_t storageRef = NULL;
    taf_fsc_Storage_t *storagePtr = NULL;

    // Find if storage context already exist, if not, create new structure
    if(FindAppStorageRef(keyFileRef, dirPath, descriptor, &storageRef) != LE_OK)
    {
        // Create a new Storage for the key.
        storagePtr = (taf_fsc_Storage_t*)le_mem_ForceAlloc(StoragePool);
        memset(storagePtr, 0, sizeof(taf_fsc_Storage_t));

        storagePtr->clientSessionRef = taf_fsc_GetClientSessionRef();
        storagePtr->keyFileRef = keyFileRef;

        memcpy(storagePtr->dirpath, dirPath, TAF_FSC_MAX_STORAGE_NAME_SIZE);
        memcpy(storagePtr->descriptor, descriptor, FS_KEY_DESCRIPTOR_HEX_SIZE);
        storagePtr->storageRef = le_ref_CreateRef(StorageRefMap, storagePtr);

        storageRef = storagePtr->storageRef;
    }
    else
    {
        storagePtr = (taf_fsc_Storage_t*)le_ref_Lookup(StorageRefMap, storageRef);
    }

    if(storagePtr != NULL)
    {
        storagePtr->keyIsAddedToKernel = true;
        return storageRef;
    }

exception:

    // Deletes key if something wrong happend
    if(!storageAlreadyExist)
    {
        taf_pa_fsc_DeleteKey(taf_fsc_GetClientSessionRef(), keyFileRef);
    }

    LE_ERROR("taf_fsc_GetStorageRef - %s", LE_RESULT_TXT(*result));
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes keys from kernel keyring.
 */
//--------------------------------------------------------------------------------------------------

static void RemoveKeysFromKernel
(
    void
)
{
    LE_INFO("trigger RemoveKeysFromKernel()");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(StorageRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_fsc_Storage_t* storagePtr = le_ref_GetValue(iterRef);
        if(storagePtr == NULL)
        {
            LE_ERROR("storagePtr == NULL");
            return;
        }

        if ((storagePtr->keyFileRef != NULL) && (storagePtr->keyIsAddedToKernel == true))
        {
            // Remove the storage reference.
            void* storageRef = (void*)le_ref_GetSafeRef(iterRef);
            LE_ASSERT(storageRef != NULL);
            le_ref_DeleteRef(StorageRefMap, storageRef);

            // Remove key from kernel keyring
            if(LE_OK == IoControl_remove_key(storagePtr->descriptor, storagePtr->dirpath))
            {
                LE_INFO("Remove key(descriptor: %s) for dir '%s' for client(%p).",
                storagePtr->descriptor, storagePtr->dirpath, storagePtr->clientSessionRef);

                // Free the storage object.
                le_mem_Release(storagePtr);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Power state change handler to trigger process during shutting down.
 */
//--------------------------------------------------------------------------------------------------

void PowerStateChangeHandler(taf_pm_State_t state, void* contextPtr)
{
    LE_INFO("Power state change to %d", state);

    if(state == TAF_PM_STATE_SHUTDOWN)
    {
        LE_INFO("System is shutting down");
        RemoveKeysFromKernel();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes storage which is created by the specified client
 */
//--------------------------------------------------------------------------------------------------
static void RemoveSessionFromStorage
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] client session reference
    void*               contextPtr   ///< [IN]
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(StorageRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_fsc_Storage_t* storagePtr = le_ref_GetValue(iterRef);
        if(storagePtr == NULL)
        {
            LE_ERROR("storagePtr == NULL");
            return;
        }

        if ((storagePtr->keyFileRef != NULL) &&
            (storagePtr->clientSessionRef == sessionRef))
        {
            LE_INFO("Remove client session (%p) from storage context '%s'",
                    storagePtr->clientSessionRef, storagePtr->dirpath);

            storagePtr->clientSessionRef = NULL;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Encrypt data by KeyStore.
 */
//--------------------------------------------------------------------------------------------------
le_result_t InternalCryptoProcess
(
    taf_ks_CryptoPurpose_t  purpose,
    Sha256_t*               fileIdPtr,
    const uint8_t*          plainTextPtr,
    size_t                  plainTextSize,
    uint8_t*                encryptedDataPtr,
    size_t*                 encryptedDataSizePtr
)
{
    uint8_t nonce[INT_KEY_NONCE_LEN] = {0};
    uint8_t aead[INT_KEY_AEAD_LEN] = {0};
    uint8_t md5Digest[EVP_MAX_MD_SIZE] = {0};

    uint md5_hash_length = 0;

    // Caculate the md5 of the file ID, later use the md5 as the nonce.
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if(ctx == NULL)
    {
        LE_ERROR("ctx is NULL");
        return LE_FAULT;
    }

    const EVP_MD* method = EVP_md5();

    EVP_DigestInit_ex(ctx, method, NULL);
    EVP_DigestUpdate(ctx, fileIdPtr->data, sizeof(fileIdPtr->data));
    EVP_DigestFinal_ex(ctx, md5Digest, &md5_hash_length);
    EVP_MD_CTX_free(ctx);

    memscpy(nonce, INT_KEY_NONCE_LEN, md5Digest, md5_hash_length);

    // Set the AEAD before data encryption/decryption.
    memscpy(aead, INT_KEY_AEAD_LEN, fileIdPtr, sizeof(fileIdPtr->data));

    // Encrypt the data using internal key
    const char keyId[] = FSC_INT_KEY_NAME;

    taf_ks_KeyRef_t keyRef;
    taf_ks_CryptoSessionRef_t sessionRef;

    size_t totalEncryptedSize = *encryptedDataSizePtr;
    size_t encSize = 0;

    le_result_t res;

    res = taf_ks_GetKey(keyId, &keyRef);

    LE_ASSERT(LE_FAULT != res);

    LE_DEBUG("Get key(%s) res = %d", FSC_INT_KEY_NAME, res);

    if(LE_NOT_FOUND == res)
    {
        res = taf_ks_CreateKey(keyId, TAF_KS_AES_ENCRYPT_DECRYPT, &keyRef);

        LE_ASSERT(LE_FAULT != res);

        res = taf_ks_ProvisionAesKeyValue(keyRef,
                                          TAF_KS_AES_SIZE_256,
                                          TAF_KS_AES_MODE_GCM,
                                          NULL, 0);

        LE_ASSERT(LE_FAULT != res);
    }
    else if(LE_OK != res)
    {
        return LE_FAULT;
    }

    LE_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(keyRef, &sessionRef));
    LE_ASSERT(LE_OK == taf_ks_CryptoSessionSetAesNonce(sessionRef, nonce, sizeof(nonce)));
    LE_ASSERT(LE_OK == taf_ks_CryptoSessionStart(sessionRef, purpose));
    LE_ASSERT(LE_OK == taf_ks_CryptoSessionProcessAead(sessionRef, aead, sizeof(aead)));
    LE_ASSERT(LE_OK == taf_ks_CryptoSessionProcess(sessionRef,
                                                    plainTextPtr,
                                                    plainTextSize,
                                                    encryptedDataPtr,
                                                    encryptedDataSizePtr));

    encSize = *encryptedDataSizePtr;
    *encryptedDataSizePtr = totalEncryptedSize - encSize;
    LE_ASSERT(LE_OK == taf_ks_CryptoSessionEnd(sessionRef,
                                                NULL, 0,
                                                encryptedDataPtr + encSize,
                                                encryptedDataSizePtr));

    *encryptedDataSizePtr += encSize;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The FS-Crypt component initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Init PA crypto function
    taf_pa_fsc_Init(&InternalCryptoProcess);

    // Create memory pools
    StoragePool = le_mem_CreatePool("StoragePool", sizeof(taf_fsc_Storage_t));

    // Create reference maps
    StorageRefMap = le_ref_CreateMap("StorageRefMap", MAX_NUM_OF_STORAGE);

    // Add power state change handler
    taf_pm_AddStateChangeHandler(PowerStateChangeHandler, NULL);

    // Set session close handlers
    le_msg_AddServiceCloseHandler(taf_fsc_GetServiceRef(), RemoveSessionFromStorage, NULL);

    LE_INFO("taf FSCrypt service Ready");
}
