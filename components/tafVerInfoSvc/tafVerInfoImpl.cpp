/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafVerInfo.hpp"

using namespace std;
using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get revisions.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_verInfo::GetRevisions
(
    taf_pi_version_Comp_t component, ///< [IN] Component
    char* versionPtr,                ///< [OUT] Version string.
    size_t versionSize               ///< [IN] Size of the version string.
)
{
    if (versionInfPtr->getFormat == NULL)
    {
        LE_ERROR("Version format undefined.");
        return LE_FAULT;
    }

    uint32_t format = (*(versionInfPtr->getFormat))();
    if (format & TAF_PI_VERSION_FORMAT_MAJOR_MINOR_PATCH)
    {
        le_utf8_Append(versionPtr, "-", versionSize, NULL);
        if (versionInfPtr->getMajor != NULL && versionInfPtr->getMinor != NULL &&
            versionInfPtr->getPatch != NULL)
        {
            char major[TAF_PI_VERSION_MAJOR_MAX_BYTES] = "";
            char minor[TAF_PI_VERSION_MINOR_MAX_BYTES] = "";
            char patch[TAF_PI_VERSION_PATCH_MAX_BYTES] = "";

            int ret = (*(versionInfPtr->getMajor))(component, major, sizeof(major));
            if (!ret)
            {
                LE_INFO("Major version: %s", major);
                le_utf8_Copy(versionPtr, major, versionSize, NULL);
                le_utf8_Append(versionPtr, ".", versionSize, NULL);
            }
            else
            {
                LE_ERROR("Fail to get major version.");
                le_utf8_Copy(versionPtr, "0.0.0", versionSize, NULL);
                return LE_OK;
            }

            ret = (*(versionInfPtr->getMinor))(component, minor, sizeof(minor));
            if (!ret)
            {
                LE_INFO("Minor version: %s", minor);
                le_utf8_Append(versionPtr, minor, versionSize, NULL);
                le_utf8_Append(versionPtr, ".", versionSize, NULL);
            }
            else
            {
                LE_ERROR("Fail to get minor version.");
                le_utf8_Append(versionPtr, "0.0", versionSize, NULL);
                return LE_OK;
            }

            ret = (*(versionInfPtr->getPatch))(component, patch, sizeof(patch));
            if (!ret)
            {
                LE_INFO("Patch version: %s", patch);
                le_utf8_Append(versionPtr, patch, versionSize, NULL);
            }
            else
            {
                LE_ERROR("Fail to get patch version.");
                le_utf8_Append(versionPtr, "0", versionSize, NULL);
                return LE_OK;
            }
        }
    }

    return LE_OK;
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
le_result_t taf_verInfo::StringToHash
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
 * Get boot bank.
 *
 * @return
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_verInfo::GetBootBank
(
    taf_verInfo_Bank_t* bank ///< [OUT] Bank.
)
{
    char slot[BOOT_SLOT_BYTES];
    FILE* fp = popen("/usr/bin/nad-abctl --boot_slot", "r");
    if (fp == NULL)
    {
        LE_ERROR("popen failed.");
        return LE_FAULT;
    }

    char* p = fgets(slot, sizeof(slot), fp);
    if (p == NULL)
    {
        LE_ERROR("Fail to read pipe result.");
        return LE_FAULT;
    }
    pclose(fp);

    if (strncmp(slot, "_a", strlen("_a")) == 0)
    {
        *bank = TAF_VERINFO_BANK_A;
    }
    else if (strncmp(slot, "_b", strlen("_b")) == 0)
    {
        *bank = TAF_VERINFO_BANK_B;
    }
    else
    {
        *bank = TAF_VERINFO_BANK_UNKNOWN;
        LE_ERROR("Inavlid boot bank.");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get hash.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_verInfo::GetHash
(
    taf_pi_hash_Comp_t component, ///< [IN] Component.
    taf_pi_hash_Bank_t bank,      ///< [IN] Bank.
    uint8_t* hashPtr,             ///< [OUT] Hash string.
    size_t* hashSizePtr           ///< [IN] Size of the hash string.
)
{
    if (hashInfPtr->getType != NULL)
    {
        taf_pi_hash_Type_t type = TAF_PI_HASH_TYPE_UNKNOWN;
        int ret = (*(hashInfPtr->getType))(component, &type);
        if (!ret)
        {
            switch (type)
            {
                case TAF_PI_BUILDTIME_HASH:
                    LE_INFO("Buildtime hash.");
                    break;
                case TAF_PI_RUNTIME_HASH:
                    LE_INFO("Runtime hash.");
                    break;
                default:
                    LE_INFO("Hash generation is unknown.");
                    break;
            }
        }
    }

    if (hashInfPtr->getAlg != NULL)
    {
        taf_pi_hash_Algorithm_t alg = TAF_PI_HASH_ALG_UNKNOWN;
        int ret = (*(hashInfPtr->getAlg))(component, &alg);
        if (!ret)
        {
            switch (alg)
            {
                case TAF_PI_HASH_ALG_MD5:
                    LE_INFO("MD5.");
                    break;
                case TAF_PI_HASH_ALG_SHA1:
                    LE_INFO("SHA1.");
                    break;
                case TAF_PI_HASH_ALG_SHA256:
                    LE_INFO("SHA256.");
                    break;
                default:
                    LE_INFO("Hash algorithm is unknown.");
                    break;
            }
        }
    }

    if (hashInfPtr->getHash != NULL)
    {
        int ret = (*(hashInfPtr->getHash))(component, bank, hashPtr, hashSizePtr);
        if (ret)
        {
            LE_ERROR("Fail to get hash.");
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get instance.
 */
//--------------------------------------------------------------------------------------------------
taf_verInfo& taf_verInfo::GetInstance
(
    void
)
{
    static taf_verInfo instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_verInfo::Init
(
    void
)
{
    versionInfPtr = (version_Inf_t *)taf_devMgr_LoadDrv(TAF_VERSION_MODULE_NAME, NULL);
    if (versionInfPtr == NULL)
    {
        LE_WARN("Version PI module is not loaded.");
    }
    else
    {
        if (versionInfPtr->init != NULL)
        {
            LE_INFO("Version PI init...");
            (*(versionInfPtr->init))();
        }
    }

    hashInfPtr = (hash_Inf_t *)taf_devMgr_LoadDrv(TAF_HASH_MODULE_NAME, NULL);
    if (hashInfPtr == NULL)
    {
        LE_WARN("Hash PI module is not loaded.");
    }
    else
    {
        if (hashInfPtr->init != NULL)
        {
            LE_INFO("Hash PI init...");
            (*(hashInfPtr->init))();
        }
    }
}
