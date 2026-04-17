/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "tafFscryptPa.h"

#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/err.h>

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
#define FS_IOC_GET_ENCRYPTION_KEY_STATUS            _IOWR('f', 26, struct fscrypt_get_key_status_arg)
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

/*
static int cmd_ioc_add_key(uint8_t *key, const char* descriptor, const char *dirpath);
static int cmd_set_policy(const char* descriptor, const char *dirpath);
static int cmd_ioc_rm_key(const char* descriptor, const char *dirpath);
*/
