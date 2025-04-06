/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#include "jansson.h"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

#include "tafFlashAccess.hpp"
#include "hash.h"

extern hash_InfoTab_t TAF_HAL_INFO_TAB;

//--------------------------------------------------------------------------------------------------
/**
 * Get hash plugin information table.
 *
 * @return
 * - NULL   -- Failed.
 * - Others -- Hash module information.
 */
//--------------------------------------------------------------------------------------------------
void* taf_hal_GetModInf
(
    void
)
{
    LE_INFO("Get hash module information table.");

    return &(TAF_HAL_INFO_TAB.hashInf);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize hash plugin.
 */
//--------------------------------------------------------------------------------------------------
void taf_piHash_Init
(
    void
)
{
    LE_INFO("Hash Plug-In Init...");

    LE_INFO("Hash Plug-In Ready...");
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse component string.
 *
 * @return
 *     - 0 -- On success.
 *     - Others -- On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_piHash_ParseComponent
(
    const char* compStr,           ///< [IN] Component string.
    taf_pi_hash_Comp_t* component ///< [OUT] Component.
)
{
    if (strncmp(compStr, "boot", strlen("boot")) == 0)
    {
        *component = TAF_PI_HASH_COMP_BOOT;
    }
    else if (strncmp(compStr, "telaf", strlen("telaf")) == 0)
    {
        *component = TAF_PI_HASH_COMP_TELAF;
    }
    else if (strncmp(compStr, "rootfs", strlen("rootfs")) == 0)
    {
        *component = TAF_PI_HASH_COMP_ROOTFS;
    }
    else if (strncmp(compStr, "firmware", strlen("firmware")) == 0)
    {
        *component = TAF_PI_HASH_COMP_FIRMWARE;
    }
    else if (strncmp(compStr, "lxc", strlen("lxc")) == 0)
    {
        *component = TAF_PI_HASH_COMP_LXC;
    }
    else
    {
        LE_ERROR("Invalid component : %s", compStr);
        return -1;
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse hash algorithm.
 *
 * @return
 *     - taf_pi_hash_Algorithm_t -- Hash algorithm.
 */
//--------------------------------------------------------------------------------------------------
taf_pi_hash_Algorithm_t taf_piHash_ParseHashAlg
(
    const char* algStr ///< [IN] Hash algorithm string.
)
{
    if (strncmp(algStr, "md5", strlen("md5")) == 0)
    {
        return TAF_PI_HASH_ALG_MD5;
    }
    else if (strncmp(algStr, "sha1", strlen("sha1")) == 0)
    {
        return TAF_PI_HASH_ALG_SHA1;
    }
    else if (strncmp(algStr, "sha256", strlen("sha256")) == 0)
    {
        return TAF_PI_HASH_ALG_SHA256;
    }

    return TAF_PI_HASH_ALG_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse hash type.
 *
 * @return
 *     - taf_pi_hash_Type_t -- Hash type.
 */
//--------------------------------------------------------------------------------------------------
taf_pi_hash_Type_t taf_piHash_ParseHashAype
(
    const char* typeStr ///< [IN] Hash type string.
)
{
    if (strncmp(typeStr, "buildtime", strlen("buildtime")) == 0)
    {
        return TAF_PI_BUILDTIME_HASH;
    }
    else if (strncmp(typeStr, "runtime", strlen("runtime")) == 0)
    {
        return TAF_PI_RUNTIME_HASH;
    }

    return TAF_PI_HASH_TYPE_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse configurations.
 *
 * @return
 *     - 0 -- On success.
 *     - Others -- On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_piHash_ParseConfig
(
    taf_piHash_Config_t* cfgPtr ///< [INOUT] Hash configurations.
)
{
    if (access(HASH_CONFIG_FILE, 0) == 0)
    {
        json_t *root;
        json_error_t error;

        // Load entire JSON file.
        root = json_load_file(HASH_CONFIG_FILE, 0, &error);
        if (root == NULL)
        {
            LE_ERROR("JSON file error: line: %d, column: %d, position: %d, source: '%s', error: %s",
                error.line, error.column, error.position, error.source, error.text);
            return -1;
        }

        // Check if "root" is an object.
        if (!json_is_object(root))
        {
            LE_ERROR("root is not an object.");
            json_decref(root);
            return -1;
        }

        // Load the 'components' array.
        json_t* js_components = json_object_get(root, "components");
        if ((!json_is_array(js_components)) || (json_array_size(js_components) == 0))
        {
            LE_ERROR("components array is not set in JSON file %s.", HASH_CONFIG_FILE);
            return -1;
        }

        size_t index = 0;
        json_t *js_component = NULL;
        json_t *js_compName = NULL;
        json_t *js_compHashType = NULL;
        json_t *js_compHashAlg = NULL;
        json_t *js_compImageSize = NULL;
        // Get each component entry in the array.
        json_array_foreach(js_components, index, js_component)
        {
            js_compName = json_object_get(js_component, "name");
            js_compHashType = json_object_get(js_component, "hash_type");
            js_compHashAlg = json_object_get(js_component, "hash_alg");
            js_compImageSize = json_object_get(js_component, "image_size");
            if (json_is_string(js_compName) && json_is_string(js_compHashType) &&
                json_is_string(js_compHashAlg) && json_is_string(js_compImageSize))
            {
                taf_pi_hash_Comp_t comp;
                if (taf_piHash_ParseComponent(json_string_value(js_compName), &comp))
                {
                    LE_ERROR("Fail to parse component.");
                    return -1;
                }
                else if (comp == cfgPtr->component)
                {
                    cfgPtr->hashType = taf_piHash_ParseHashAype(json_string_value(js_compHashType));
                    cfgPtr->hashAlg = taf_piHash_ParseHashAlg(json_string_value(js_compHashAlg));
                    char *endptr;
                    cfgPtr->imageSize = (uint32_t)strtoul(
                        json_string_value(js_compImageSize), &endptr, 10);
                }
            }
        }
    }
    else
    {
        // Default configuration.
        cfgPtr->hashType = TAF_PI_RUNTIME_HASH;
        cfgPtr->hashAlg = TAF_PI_HASH_ALG_SHA1;
        cfgPtr->imageSize = 0;
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get boot bank.
 *
 * @return
 *     - 0 -- On success.
 *     - Others -- On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_piHash_GetBootBank
(
    taf_pi_hash_Bank_t* bank ///< [IN] Bank.
)
{
    char slot[BOOT_SLOT_BYTES];
    FILE* fp = popen("/usr/bin/nad-abctl --boot_slot", "r");
    if (fp == NULL)
    {
        LE_ERROR("popen failed.");
        return -1;
    }

    char* p = fgets(slot, sizeof(slot), fp);
    if (p == NULL)
    {
        LE_ERROR("Fail to read pipe result.");
        return -1;
    }
    pclose(fp);

    if (strncmp(slot, "_a", strlen("_a")) == 0)
    {
        *bank = TAF_PI_HASH_BANK_A;
    }
    else if (strncmp(slot, "_b", strlen("_b")) == 0)
    {
        *bank = TAF_PI_HASH_BANK_B;
    }
    else
    {
        *bank = TAF_PI_HASH_NOT_DUAL_BANK;
        LE_ERROR("Inavlid boot bank.");
        return -1;
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get hash type.
 */
//--------------------------------------------------------------------------------------------------
int taf_piHash_GetType
(
    taf_pi_hash_Comp_t component, ///< [IN] Component
    taf_pi_hash_Type_t* type      ///< [OUT] Hash type.
)
{
    taf_piHash_Config_t cfg;
    cfg.component = component;
    if (taf_piHash_ParseConfig(&cfg))
    {
        *type = TAF_PI_HASH_TYPE_UNKNOWN;
        return -1;
    }
    else
    {
        *type = cfg.hashType;
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get hash algorithm.
 */
//--------------------------------------------------------------------------------------------------
int taf_piHash_GetAlg
(
    taf_pi_hash_Comp_t component, ///< [IN] Component
    taf_pi_hash_Algorithm_t* alg  ///< [OUT] Hash algorithm.
)
{
    taf_piHash_Config_t cfg;
    cfg.component = component;
    if (taf_piHash_ParseConfig(&cfg))
    {
        *alg = TAF_PI_HASH_ALG_UNKNOWN;
        return -1;
    }
    else
    {
        *alg = cfg.hashAlg;
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert string to hash.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_piHash_StringToHash
(
    char* strPtr,       ///< [IN] Character string.
    uint8_t* hashPtr,   ///< [OUT] Hash string.
    size_t* hashSizePtr ///< [OUT] Size of the hash string.
)
{
    size_t hashIndex = 0;
    uint8_t hex = 0;
    for (size_t i = 0; i < strlen(strPtr); i++)
    {
        if (strPtr[i] >= '0' && strPtr[i] <= '9')
        {
            hex = strPtr[i] - '0';
        }
        else if (strPtr[i] >= 'a' && strPtr[i] <= 'f')
        {
            hex = strPtr[i] - 'a' + 10;
        }
        else
        {
            break;
        }

        if (i % 2)
        {
            hashPtr[hashIndex] = hashPtr[hashIndex] * 16 + hex;
            hashIndex++;
        }
        else
        {
            hashPtr[hashIndex] = hex;
        }
    }

    *hashSizePtr = hashIndex;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get buildtime hash.
 *
 * @return
 *     - 0 -- On success.
 *     - Others -- On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_piHash_GetBuildTimeHash
(
    taf_pi_hash_Comp_t component, ///< [IN] Component.
    uint8_t* hash,                ///< [OUT] Hash string.
    size_t* hashSize              ///< [OUT] Size of the hash string.
)
{
    char version[VERSION_BYTES];

    if (component == TAF_PI_HASH_COMP_TELAF)
    {
        FILE* fp = fopen(TELAF_VERSION_FILE, "r");
        if (fp != NULL)
        {
            char* p = fgets(version, VERSION_BYTES, fp);
            if (p == NULL)
            {
                LE_ERROR("Fail to read file %s.", TELAF_VERSION_FILE);
                fclose(fp);
               return -1;
            }
            fclose(fp);

            size_t i;
            for (i = 0; i < strlen(version); i++)
            {
                if (version[i] == '_')
                    break;
            }

            return taf_piHash_StringToHash(version + i + 1, hash, hashSize);
        }
    }
    else if (component == TAF_PI_HASH_COMP_LXC)
    {
        FILE* fp = fopen(LXC_HASH_FILE, "r");
        if (fp != NULL)
        {
            char* p = fgets(version, VERSION_BYTES, fp);
            if (p == NULL)
            {
                LE_ERROR("Fail to read file %s.", LXC_HASH_FILE);
                fclose(fp);
               return -1;
            }
            fclose(fp);

            return taf_piHash_StringToHash(version, hash, hashSize);
        }
    }

    LE_ERROR("No buildtime hash ia available for the component.");
    return -1;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get partition name.
 *
 * @return
 *     - 0 -- On success.
 *     - Others -- On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_piHash_GetPartitionName
(
    taf_pi_hash_Comp_t component, ///< [IN] Component.
    taf_pi_hash_Bank_t bank,      ///< [IN] Boot bank.
    char* name,                   ///< [OUT] Partition name.
    size_t nameSize               ///< [IN] Size of partition name.
)
{
    if (bank == TAF_PI_HASH_BANK_A)
    {
		switch (component)
        {
           case TAF_PI_HASH_COMP_BOOT:
               le_utf8_Copy(name, "boot", nameSize, NULL);
               break;
           case TAF_PI_HASH_COMP_ROOTFS:
               le_utf8_Copy(name, "rootfs_a", nameSize, NULL);
               break;
           case TAF_PI_HASH_COMP_FIRMWARE:
               le_utf8_Copy(name, "firmware_a", nameSize, NULL);
               break;
           case TAF_PI_HASH_COMP_TELAF:
               le_utf8_Copy(name, "telaf_a", nameSize, NULL);
               break;
           case TAF_PI_HASH_COMP_LXC:
               le_utf8_Copy(name, "lxcrootfs_a", nameSize, NULL);
               break;
           default:
               LE_ERROR("Unknown component.");
               return -1;
		}
    }
    else if (bank == TAF_PI_HASH_BANK_B)
    {
		switch (component)
        {
           case TAF_PI_HASH_COMP_BOOT:
               le_utf8_Copy(name, "boot_b", nameSize, NULL);
               break;
           case TAF_PI_HASH_COMP_ROOTFS:
               le_utf8_Copy(name, "rootfs_b", nameSize, NULL);
               break;
           case TAF_PI_HASH_COMP_FIRMWARE:
               le_utf8_Copy(name, "firmware_b", nameSize, NULL);
               break;
           case TAF_PI_HASH_COMP_TELAF:
               le_utf8_Copy(name, "telaf_b", nameSize, NULL);
               break;
           case TAF_PI_HASH_COMP_LXC:
               le_utf8_Copy(name, "lxcrootfs_b", nameSize, NULL);
               break;
           default:
               LE_ERROR("Unknown component.");
               return -1;
		}
    }
    else
    {
        LE_ERROR("Unknown bank.");
        return -1;
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Find partition.
 *
 * @return
 *     - 0 -- On success.
 *     - Others -- On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_piHash_FindPartition
(
    taf_piHash_Config_t* cfgPtr,            ///< [INOUT] Configurations.
    taf_lib_flash_PartitionList_t* listPtr, ///< [IN] Partition list.
    uint32_t* index                         ///< [OUT] Partition index.
)
{
    // 2. Get partition name.
    char partitionName[TAF_LIB_FLASH_PARTITION_NAME_MAX_LEN];
    int ret = taf_piHash_GetPartitionName(cfgPtr->component, cfgPtr->bank,
        partitionName, TAF_LIB_FLASH_PARTITION_NAME_MAX_LEN);
    if (ret)
    {
        LE_ERROR("Fail to get partition name.");
        return ret;
    }

    // 3. Search partition in list.
    uint32_t i = 0;
    for (i = 0; i < listPtr->number; i++)
    {
        if (strncmp(partitionName, listPtr->partition[i].name, strlen(partitionName)) == 0)
        {
            if (cfgPtr->bank == TAF_PI_HASH_BANK_A && listPtr->partition[i].bank == DUAL_BANK_A)
                break;

            if (cfgPtr->bank == TAF_PI_HASH_BANK_B && listPtr->partition[i].bank == DUAL_BANK_B)
                break;
        }
    }

    if (i == listPtr->number)
    {
        LE_ERROR("Component not found.");
        return -1;
    }

    *index = i;

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get runtime hash.
 *
 * @return
 *     - 0 -- On success.
 *     - Others -- On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_piHash_GetRunTimeHash
(
    taf_piHash_Config_t* cfgPtr,             ///< [IN] Configurations.
    taf_lib_flash_Partition_t* partitionPtr, ///< [IN] Partition.
    uint8_t* hash,                           ///< [OUT] Hash string.
    size_t* hashSize                         ///< [OUT] Size of the hash string.
)
{
    // 1. Open partition.
    int errCode = 0;
    le_result_t result = taf_lib_flash_OpenPartition(partitionPtr, O_RDONLY, &errCode);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to open partition, error: %s", strerror(errCode));
        return -1;
    }

    // 2. Initiate message digest context with hash algorithm.
    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (cfgPtr->hashAlg == TAF_PI_HASH_ALG_MD5)
    {
        const EVP_MD *md = EVP_md5();
        if (EVP_DigestInit_ex(md_ctx, md, NULL) != 1)
        {
            LE_ERROR("Fail to initiate md5 context.");
            EVP_MD_CTX_free(md_ctx);
            return -1;
        }
    }
    else if (cfgPtr->hashAlg == TAF_PI_HASH_ALG_SHA1)
    {
        const EVP_MD *md = EVP_sha1();
        if (EVP_DigestInit_ex(md_ctx, md, NULL) != 1)
        {
            LE_ERROR("Fail to initiate sha1 context.");
            EVP_MD_CTX_free(md_ctx);
            return -1;
        }
    }
    else if (cfgPtr->hashAlg == TAF_PI_HASH_ALG_SHA256)
    {
        const EVP_MD *md = EVP_sha256();
        if (EVP_DigestInit_ex(md_ctx, md, NULL) != 1)
        {
            LE_ERROR("Fail to initiate sha256 context.");
            EVP_MD_CTX_free(md_ctx);
            return -1;
        }
    }

    // 3. Update partiton size for default case.
    if (cfgPtr->imageSize == 0)
    {
        if (partitionPtr->eraseSize == TAF_LIB_FLASH_MTD_BLOCK_SIZE)
        {
            result = taf_lib_flash_GetMtdSize(partitionPtr, &cfgPtr->imageSize, &errCode);
            if (result != LE_OK)
            {
                LE_ERROR("Fail to get mtd partition size, error: %s", strerror(errCode));
                EVP_MD_CTX_free(md_ctx);
                return -1;
            }
        }
        else
        {
            result = taf_lib_flash_GetUbiVolSize(partitionPtr, &cfgPtr->imageSize);
            if (result != LE_OK)
            {
                LE_ERROR("Fail to get ubi volume size.");
                EVP_MD_CTX_free(md_ctx);
                return -1;
            }
        }
    }

    // 4. Read partition data and calculate hash.
    uint8_t content[HASH_PAGE_SIZE];
    uint32_t i = 0;
    uint32_t iteration = cfgPtr->imageSize / HASH_PAGE_SIZE;
    size_t rdSize = 0;
    for (i = 0; i < iteration; i++)
    {
        rdSize = HASH_PAGE_SIZE;
        result = taf_lib_flash_ReadPartition(partitionPtr, i * HASH_PAGE_SIZE, content, &rdSize, &errCode);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to read partition at iteration %d, error: %s", i, strerror(errCode));
            EVP_MD_CTX_free(md_ctx);
            return -1;
        }

        EVP_DigestUpdate(md_ctx, content, rdSize);
    }

    // 5. Read the rest of partition data and update hash.
    rdSize = cfgPtr->imageSize % HASH_PAGE_SIZE;
    unsigned int hashLen;
    if (rdSize != 0)
    {
        result = taf_lib_flash_ReadPartition(partitionPtr,
            iteration * HASH_PAGE_SIZE, content, &rdSize, &errCode);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to read the reset of partition, error: %s", strerror(errCode));
            EVP_MD_CTX_free(md_ctx);
            return -1;
        }

        EVP_DigestUpdate(md_ctx, content, rdSize);
    }

    // 6. Get the hash output and clean up message digest context.
    EVP_DigestFinal_ex(md_ctx, hash, &hashLen);
    *hashSize = (size_t)hashLen;
    EVP_MD_CTX_free(md_ctx);

    // 7. Close partition.
    result = taf_lib_flash_ClosePartition(partitionPtr);
    if (result != LE_OK)
    {
        LE_ERROR("Fail to close partition.");
        return -1;
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get hash.
 *
 * @return
 *     - 0 -- On success.
 *     - Others -- On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_piHash_GetHash
(
    taf_pi_hash_Comp_t component, ///< [IN] Component.
    taf_pi_hash_Bank_t bank,      ///< [IN] Bank.
    uint8_t* hashPtr,             ///< [OUT] Hash string.
    size_t* hashSizePtr           ///< [OUT] Size of the hash string.
)
{
    // 1. Parse configurations.
    taf_piHash_Config_t cfg;
    cfg.component = component;
    cfg.bank = bank;
    int ret = taf_piHash_ParseConfig(&cfg);
    if (ret)
    {
        LE_ERROR("Fail to parse configurations.");
        return ret;
    }

    // 2. Check if buildtime hash is available.
    taf_pi_hash_Bank_t bootBank;
    ret = taf_piHash_GetBootBank(&bootBank);
    if (ret)
    {
        LE_ERROR("Fail to get boot bank.");
        return ret;
    }
    if (bootBank == bank && cfg.hashType == TAF_PI_BUILDTIME_HASH)
    {
        return taf_piHash_GetBuildTimeHash(component, hashPtr, hashSizePtr);
    }

    // 3. Get partition list.
    taf_lib_flash_PartitionList_t list;
    le_result_t result = taf_lib_flash_GetPartitionList(&list);
    if (result != LE_OK)
    {
        LE_ERROR("Get partition list failed.");
        return -1;
    }

    // 4. Find the partition.
    uint32_t index = 0;
    ret = taf_piHash_FindPartition(&cfg, &list, &index);
    if (ret)
    {
        LE_ERROR("Fail to find partition.");
        return ret;
    }

    // 5. Calculate runtime hash.
    return taf_piHash_GetRunTimeHash(&cfg, &list.partition[index], hashPtr, hashSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Hash information table.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED hash_InfoTab_t TAF_HAL_INFO_TAB =
{
    .mgrInf =
    {
        .name = TAF_HASH_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .moduleType = TAF_MODULETYPE_PLUG_IN,
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    },

    .hashInf =
    {
        .init = taf_piHash_Init,
        .getType = taf_piHash_GetType,
        .getAlg = taf_piHash_GetAlg,
        .getHash = taf_piHash_GetHash,
    },
};

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}