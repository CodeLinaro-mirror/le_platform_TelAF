/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFFSCRYPT_HPP
#define TAFFSCRYPT_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

extern "C" {
#include "tafFscryptPa.h"
}

#include "tafPaResultConvert.hpp"

#include "limit.h"

#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/err.h>
#include <openssl/evp.h>

/*
 * v1 policy keys are specified by an arbitrary 8-byte key "descriptor",
 * matching fscrypt_policy_v1::master_key_descriptor.
 */

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Message List objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_OF_STORAGE    100

//--------------------------------------------------------------------------------------------------
/**
 * FS-Crypt key definitions
 */
//--------------------------------------------------------------------------------------------------
#define FS_KEY_DESCRIPTOR_SIZE      8
#define FS_KEY_DESCRIPTOR_HEX_SIZE  ((2 * FS_KEY_DESCRIPTOR_SIZE) + 1)
#define FS_KEY_IDENTIFIER_SIZE      16

//--------------------------------------------------------------------------------------------------
/**
 * FS-Crypt v1 policy keys are specified by an arbitrary 8-byte key "descriptor",
 * matching fscrypt_policy_v1::master_key_descriptor.
 */
//--------------------------------------------------------------------------------------------------
#define FSCRYPT_KEY_SPEC_TYPE_DESCRIPTOR 1

//--------------------------------------------------------------------------------------------------
/**
 * FS-Crypt IO control code definitions
 */
//--------------------------------------------------------------------------------------------------
#define FS_IOC_SET_ENCRYPTION_POLICY                _IOR('f', 19, struct fscrypt_policy_v1)
#define FS_IOC_GET_ENCRYPTION_PWSALT                _IOW('f', 20, uint8_t[16])
#define FS_IOC_GET_ENCRYPTION_POLICY                _IOW('f', 21, struct fscrypt_policy_v1)
#define FS_IOC_GET_ENCRYPTION_POLICY_EX             _IOWR('f', 22, uint8_t[9]) /* size + version */
#define FS_IOC_ADD_ENCRYPTION_KEY                   _IOWR('f', 23, struct fscrypt_add_key_arg)
#define FS_IOC_REMOVE_ENCRYPTION_KEY                _IOWR('f', 24, struct fscrypt_remove_key_arg)
#define FS_IOC_REMOVE_ENCRYPTION_KEY_ALL_USERS      _IOWR('f', 25, struct fscrypt_remove_key_arg)
#define FS_IOC_GET_ENCRYPTION_KEY_STATUS           _IOWR('f', 26, struct fscrypt_get_key_status_arg)
#define FS_IOC_GET_ENCRYPTION_NONCE                 _IOR('f', 27, uint8_t[16])

//--------------------------------------------------------------------------------------------------
/**
 * Amount of padding
 */
//--------------------------------------------------------------------------------------------------
#define FS_POLICY_FLAGS_PAD_4     0x00
#define FS_POLICY_FLAGS_PAD_8     0x01
#define FS_POLICY_FLAGS_PAD_16    0x02
#define FS_POLICY_FLAGS_PAD_32    0x03
#define FS_POLICY_FLAGS_PAD_MASK  0x03

//--------------------------------------------------------------------------------------------------
/**
 * Encryption algorithms
 */
//--------------------------------------------------------------------------------------------------
#define FS_ENCRYPTION_MODE_INVALID     0
#define FS_ENCRYPTION_MODE_AES_256_XTS 1
#define FS_ENCRYPTION_MODE_AES_256_GCM 2
#define FS_ENCRYPTION_MODE_AES_256_CBC 3
#define FS_ENCRYPTION_MODE_AES_256_CTS 4
#define FS_ENCRYPTION_MODE_AES_128_CBC 5
#define FS_ENCRYPTION_MODE_AES_128_CTS 6

//--------------------------------------------------------------------------------------------------
/**
 * Key removal status
 */
//--------------------------------------------------------------------------------------------------
#define FS_KEY_REMOVAL_STATUS_FLAG_FILES_BUSY    0x00000001
#define FS_KEY_REMOVAL_STATUS_FLAG_OTHER_USERS   0x00000002

//--------------------------------------------------------------------------------------------------
/**
 * Key status
 */
//--------------------------------------------------------------------------------------------------
#define FS_KEY_STATUS_ABSENT                    1
#define FS_KEY_STATUS_PRESENT                   2
#define FS_KEY_STATUS_INCOMPLETELY_REMOVED      3

//--------------------------------------------------------------------------------------------------
/**
 * Key status flag
 */
//--------------------------------------------------------------------------------------------------
#define FS_KEY_STATUS_FLAG_ADDED_BY_SELF   0x00000001

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
#define INT_KEY_AEAD_LEN  MD5_DIGEST_LENGTH

//--------------------------------------------------------------------------------------------------
// Data structures
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Specifies a key, either for v1 or v2 policies.  This doesn't contain the
 * actual key itself; this is just the "name" of the key.
 */
//--------------------------------------------------------------------------------------------------
struct fscrypt_key_specifier
{
    uint32_t type;    /* one of FSCRYPT_KEY_SPEC_TYPE_* */
    uint32_t __reserved;
    union
    {
        uint8_t __reserved[32]; /* reserve some extra space */
        uint8_t descriptor[FS_KEY_DESCRIPTOR_SIZE];
        uint8_t identifier[FS_KEY_IDENTIFIER_SIZE];
    } u;
};

//--------------------------------------------------------------------------------------------------
/**
 * FS-Crypt policy
 */
//--------------------------------------------------------------------------------------------------
struct fscrypt_policy_v1
{
    uint8_t version;
    uint8_t contents_encryption_mode;
    uint8_t filenames_encryption_mode;
    uint8_t flags;
    uint8_t master_key_descriptor[FS_KEY_DESCRIPTOR_SIZE];
};

//--------------------------------------------------------------------------------------------------
/**
 * Struct passed to FS_IOC_ADD_ENCRYPTION_KEY
 */
//--------------------------------------------------------------------------------------------------
struct fscrypt_add_key_arg
{
    struct fscrypt_key_specifier key_spec;
    uint32_t raw_size;
    uint32_t key_id;
    uint32_t __reserved[8];
    uint8_t raw[];
};

//--------------------------------------------------------------------------------------------------
/**
 * Struct passed to FS_IOC_REMOVE_ENCRYPTION_KEY
 */
//--------------------------------------------------------------------------------------------------
struct fscrypt_remove_key_arg
{
    struct fscrypt_key_specifier key_spec;

    uint32_t removal_status_flags;    /* output */
    uint32_t __reserved[5];
};

//--------------------------------------------------------------------------------------------------
/**
 * Struct passed to FS_IOC_GET_ENCRYPTION_KEY_STATUS
 */
//--------------------------------------------------------------------------------------------------
struct fscrypt_get_key_status_arg
{
    /* input */
    struct fscrypt_key_specifier key_spec;
    uint32_t __reserved[6];

    /* output */
    uint32_t status;
    uint32_t status_flags;
    uint32_t user_count;
    uint32_t __out_reserved[13];
};

//--------------------------------------------------------------------------------------------------
/**
 * Policy provided via an ioctl on the topmost directory
 */
//--------------------------------------------------------------------------------------------------
struct fscrypt_policy
{
  uint8_t version;
  uint8_t contents_encryption_mode;
  uint8_t filenames_encryption_mode;
  uint8_t flags;
  uint8_t master_key_descriptor[FS_KEY_DESCRIPTOR_SIZE];
} __attribute__((packed));

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
    taf_fsc_StorageRef_t storageRef;              ///< Reference to the FS-Crypt storage
    KeyMgt_KeyFileRef_t  keyFileRef;               ///< Key file reference to the key file
    le_msg_SessionRef_t  clientSessionRef;         ///< Client session reference
    char                 dirpath[TAF_FSC_MAX_STORAGE_NAME_SIZE]; ///< Directory path
    char                 descriptor[FS_KEY_DESCRIPTOR_HEX_SIZE]; ///< Descriptor used for IO control
    bool                 keyIsAddedToKernel;       ///< True if the key is added to kernel
}
taf_fsc_Storage_t;

//--------------------------------------------------------------------------------------------------
/**
 * TelAF FS-Crypt Service Class
 */
//--------------------------------------------------------------------------------------------------
namespace tafsvc
{
    class taf_FSCrypt : public ITafSvc
    {
        public:
            taf_FSCrypt() {};
            ~taf_FSCrypt() {};
            void Init();

            // Static member functions.
            static taf_FSCrypt &GetInstance();
            static void RemoveSessionFromStorage(le_msg_SessionRef_t sessionRef, void* contextPtr);
            static le_result_t InternalCryptoProcess(
                taf_ks_CryptoPurpose_t  purpose,
                const uint8_t*          md5DataPtr,
                size_t                  md5DataSize,
                const uint8_t*          plainTextPtr,
                size_t                  plainTextSize,
                uint8_t*                encryptedDataPtr,
                size_t*                 encryptedDataSizePtr);

            // Public methods (correspond to the taf_fsc IPC API entries).
            taf_fsc_StorageRef_t GetStorageRef(const char* dirPath, le_result_t* result);
            le_result_t LockStorage(taf_fsc_StorageRef_t StorageRef);
            le_result_t UnlockStorage(taf_fsc_StorageRef_t StorageRef);
            le_result_t DeleteStorage(taf_fsc_StorageRef_t StorageRef);

        private:
            // Internal helpers.
            le_result_t FindAppStorageRef(
                KeyMgt_KeyFileRef_t     keyFileRef,
                const char              dirPath[TAF_FSC_MAX_STORAGE_NAME_SIZE],
                char                    descriptor[FS_KEY_DESCRIPTOR_HEX_SIZE],
                taf_fsc_StorageRef_t*   storageRef);

            // Storage object.
            le_ref_MapRef_t StorageRefMap_;
            le_mem_PoolRef_t StoragePool_;
    };
}
#endif /* #ifndef TAFFSCRYPT_HPP */
