/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string>
#include <fstream>

#include "tafVerInfo.hpp"

using namespace std;
using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("tafVerInfo Service Init...\n");
    auto& tafVerInfo = taf_verInfo::GetInstance();
    tafVerInfo.Init();
    LE_INFO("tafVerInfo Service Ready...\n");
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the kernel version.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_verInfo_GetKernelVersion
(
    char* versionPtr,  ///< [OUT] Kernel version.
    size_t versionSize ///< [IN] Kernel version size.
)
{
    TAF_ERROR_IF_RET_VAL(versionPtr == NULL, LE_BAD_PARAMETER, "Null ptr(versionPtr)");

    le_result_t result = LE_OK;
    ifstream ifs(KERNEL_VERSION_FILE);
    string version;

    getline(ifs, version);
    ifs.close();
    version = version.substr(0, version.find("-"));

    le_utf8_Copy(versionPtr, version.c_str(), versionSize, NULL);
    size_t baseSize = strlen(versionPtr);
    auto& tafVerInfo = taf_verInfo::GetInstance();
    if (tafVerInfo.versionInfPtr != NULL && baseSize < versionSize)
    {
        LE_INFO("Baseline version: %s", versionPtr);
        le_utf8_Append(versionPtr, "<", versionSize, NULL);
        result = tafVerInfo.GetRevisions(TAF_PI_VERSION_COMP_BOOT, versionPtr + baseSize + 1,
            versionSize - baseSize - 1);
        le_utf8_Append(versionPtr, ">", versionSize, NULL);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the firmware version.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_verInfo_GetFirmwareVersion
(
    char* versionPtr,  ///< [OUT] Firmware version.
    size_t versionSize ///< [IN] Firmware version size.
)
{
    TAF_ERROR_IF_RET_VAL(versionPtr == NULL, LE_BAD_PARAMETER, "Null ptr(versionPtr)");

    le_result_t result = LE_OK;
    ifstream ifs(FIRMWARE_VERSION_FILE);
    string version;

    size_t start = string::npos;
    while (getline(ifs, version))
    {
        start = version.find("MPSS");
        if (start != string::npos)
            break;
    }
    ifs.close();

    size_t end = version.find(",");
    if (start != string::npos && end != string::npos)
    {
        version = version.substr(start, end - start - 1);
    }
    else
    {
        LE_ERROR("Invalid character in current firmware version.");
        return LE_FAULT;
    }

    le_utf8_Copy(versionPtr, version.c_str(), versionSize, NULL);

    size_t baseSize = strlen(versionPtr);
    auto& tafVerInfo = taf_verInfo::GetInstance();
    if (tafVerInfo.versionInfPtr != NULL && baseSize < versionSize)
    {
        LE_INFO("Baseline version: %s", versionPtr);
        le_utf8_Append(versionPtr, "<", versionSize, NULL);
        result = tafVerInfo.GetRevisions(TAF_PI_VERSION_COMP_FIRMWARE, versionPtr + baseSize + 1,
            versionSize - baseSize - 1);
        le_utf8_Append(versionPtr, ">", versionSize, NULL);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the TZ version.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_verInfo_GetTZVersion
(
    char* versionPtr,  ///< [OUT] TZ version.
    size_t versionSize ///< [IN] TZ version size.
)
{
    TAF_ERROR_IF_RET_VAL(versionPtr == NULL, LE_BAD_PARAMETER, "Null ptr(versionPtr)");

    le_result_t result = LE_OK;
    ifstream ifs(FIRMWARE_VERSION_FILE);
    string version;

    size_t start = string::npos;
    while (getline(ifs, version))
    {
        start = version.find("TZ");
        if (start != string::npos)
            break;
    }
    ifs.close();

    size_t end = version.find(",");
    if (start != string::npos && end != string::npos)
    {
        version = version.substr(start, end - start - 1);
    }
    else
    {
        LE_ERROR("Invalid character in current TZ version.");
        return LE_FAULT;
    }

    le_utf8_Copy(versionPtr, version.c_str(), versionSize, NULL);

    size_t baseSize = strlen(versionPtr);
    auto& tafVerInfo = taf_verInfo::GetInstance();
    if (tafVerInfo.versionInfPtr != NULL && baseSize < versionSize)
    {
        LE_INFO("Baseline version: %s", versionPtr);
        le_utf8_Append(versionPtr, "<", versionSize, NULL);
        result = tafVerInfo.GetRevisions(TAF_PI_VERSION_COMP_TZ, versionPtr + baseSize + 1,
            versionSize - baseSize - 1);
        le_utf8_Append(versionPtr, ">", versionSize, NULL);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the TelAF version.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_verInfo_GetTelAFVersion
(
    char* versionPtr,  ///< [OUT] TelAF version.
    size_t versionSize ///< [IN] TelAF version size.
)
{
    TAF_ERROR_IF_RET_VAL(versionPtr == NULL, LE_BAD_PARAMETER, "Null ptr(versionPtr)");

    le_result_t result = LE_OK;
    ifstream ifs(TELAF_VERSION_FILE);
    string version;

    getline(ifs, version);
    ifs.close();

    le_utf8_Copy(versionPtr, version.c_str(), versionSize, NULL);
    size_t baseSize = strlen(versionPtr);
    auto& tafVerInfo = taf_verInfo::GetInstance();
    if (tafVerInfo.versionInfPtr != NULL && baseSize < versionSize)
    {
        LE_INFO("Baseline version: %s", versionPtr);
        le_utf8_Append(versionPtr, "<", versionSize, NULL);
        result = tafVerInfo.GetRevisions(TAF_PI_VERSION_COMP_TELAF, versionPtr + baseSize + 1,
            versionSize - baseSize - 1);
        le_utf8_Append(versionPtr, ">", versionSize, NULL);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the rootFS version.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_verInfo_GetRootFSVersion
(
    char* versionPtr,  ///< [OUT] RootFS version.
    size_t versionSize ///< [IN] RootFS version size.
)
{
    TAF_ERROR_IF_RET_VAL(versionPtr == NULL, LE_BAD_PARAMETER, "Null ptr(versionPtr)");

    le_result_t result = LE_OK;
    ifstream ifs(ROOTFS_VERSION_FILE);
    string version;

    getline(ifs, version);
    ifs.close();

    le_utf8_Copy(versionPtr, version.c_str(), versionSize, NULL);
    size_t baseSize = strlen(versionPtr);
    auto& tafVerInfo = taf_verInfo::GetInstance();
    if (tafVerInfo.versionInfPtr != NULL && baseSize < versionSize)
    {
        LE_INFO("Baseline version: %s", versionPtr);
        le_utf8_Append(versionPtr, "<", versionSize, NULL);
        result = tafVerInfo.GetRevisions(TAF_PI_VERSION_COMP_ROOTFS, versionPtr + baseSize + 1,
            versionSize - baseSize - 1);
        le_utf8_Append(versionPtr, ">", versionSize, NULL);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the LXC version.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_verInfo_GetLXCVersion
(
    char* versionPtr,  ///< [OUT] LXC version.
    size_t versionSize ///< [IN] LXC version size.
)
{
    TAF_ERROR_IF_RET_VAL(versionPtr == NULL, LE_BAD_PARAMETER, "Null ptr(versionPtr)");

    le_result_t result = LE_OK;
    ifstream ifs(LXC_VERSION_FILE);
    string version;

    getline(ifs, version);
    ifs.close();

    le_utf8_Copy(versionPtr, version.c_str(), versionSize, NULL);
    size_t baseSize = strlen(versionPtr);
    auto& tafVerInfo = taf_verInfo::GetInstance();
    if (tafVerInfo.versionInfPtr != NULL && baseSize < versionSize)
    {
        LE_INFO("Baseline version: %s", versionPtr);
        le_utf8_Append(versionPtr, "<", versionSize, NULL);
        result = tafVerInfo.GetRevisions(TAF_PI_VERSION_COMP_LXC, versionPtr + baseSize + 1,
            versionSize - baseSize - 1);
        le_utf8_Append(versionPtr, ">", versionSize, NULL);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the TelAF hash.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_UNSUPPORTED -- Not supported.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_verInfo_GetTelAFHash
(
    taf_verInfo_Bank_t bank, ///< [IN] Bank.
    uint8_t* hashPtr,        ///< [OUT] TelAF hash.
    size_t* hashSizePtr      ///< [OUT] TelAF hash size.
)
{
    TAF_ERROR_IF_RET_VAL(hashPtr == NULL, LE_BAD_PARAMETER, "Null ptr(hashPtr)");

    TAF_ERROR_IF_RET_VAL(hashSizePtr == NULL, LE_BAD_PARAMETER, "Null ptr(hashSizePtr)");

    le_result_t result = LE_OK;
    auto& tafVerInfo = taf_verInfo::GetInstance();

    if (tafVerInfo.hashInfPtr != NULL)
    {
        if (bank == TAF_VERINFO_BANK_A)
        {
            result = tafVerInfo.GetHash(TAF_PI_HASH_COMP_TELAF, TAF_PI_HASH_BANK_A,
                hashPtr, hashSizePtr);
        }
        else if (bank == TAF_VERINFO_BANK_B)
        {
            result = tafVerInfo.GetHash(TAF_PI_HASH_COMP_TELAF, TAF_PI_HASH_BANK_B,
                hashPtr, hashSizePtr);
        }
        else
        {
            LE_ERROR("Invalid bank for TelAF.");
            return LE_BAD_PARAMETER;
        }
    }
    else
    {
        taf_verInfo_Bank_t bootBank;
        result = tafVerInfo.GetBootBank(&bootBank);
        TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get the boot bank.");

        TAF_ERROR_IF_RET_VAL(bank != bootBank, LE_UNSUPPORTED,
            "TelAF hash is only available for boot bank.");

        char version[TAF_VERINFO_VERSION_MAX_BYTES] = { 0 };
        FILE* fp = fopen(TELAF_VERSION_FILE, "r");
        if (fp != NULL)
        {
            char* p = fgets(version, TAF_VERINFO_VERSION_MAX_BYTES, fp);
            fclose(fp);
            TAF_ERROR_IF_RET_VAL(p == NULL, LE_FAULT, "Fail to read version file.");
            

            size_t i;
            for (i = 0; i < strlen(version); i++)
            {
                if (version[i] == '_')
                    break;
            }

            if (i + 1 < strlen(version))
            {
                result = tafVerInfo.StringToHash(version + i + 1, hashPtr, hashSizePtr);
            }
            else
            {
                LE_ERROR("Fail to find hash in %s.", version);
                return LE_FAULT;
            }
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the boot hash.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_UNSUPPORTED -- Not supported.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_verInfo_GetBootHash
(
    taf_verInfo_Bank_t bank, ///< [IN] Bank.
    uint8_t* hashPtr,        ///< [OUT] Boot hash.
    size_t* hashSizePtr      ///< [OUT] Boot hash size.
)
{
    TAF_ERROR_IF_RET_VAL(hashPtr == NULL, LE_BAD_PARAMETER, "Null ptr(hashPtr)");

    TAF_ERROR_IF_RET_VAL(hashSizePtr == NULL, LE_BAD_PARAMETER, "Null ptr(hashSizePtr)");

    le_result_t result = LE_OK;
    auto& tafVerInfo = taf_verInfo::GetInstance();

    TAF_ERROR_IF_RET_VAL(tafVerInfo.hashInfPtr == NULL, LE_UNSUPPORTED,
        "Boot hash is not available without plug-in.");

    if (bank == TAF_VERINFO_BANK_A)
    {
        result = tafVerInfo.GetHash(TAF_PI_HASH_COMP_BOOT, TAF_PI_HASH_BANK_A,
            hashPtr, hashSizePtr);
    }
    else if (bank == TAF_VERINFO_BANK_B)
    {
        result = tafVerInfo.GetHash(TAF_PI_HASH_COMP_BOOT, TAF_PI_HASH_BANK_B,
            hashPtr, hashSizePtr);
    }
    else
    {
        LE_ERROR("Invalid bank for boot.");
        return LE_BAD_PARAMETER;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the rootFS hash.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_UNSUPPORTED -- Not supported.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_verInfo_GetRootFSHash
(
    taf_verInfo_Bank_t bank, ///< [IN] Bank.
    uint8_t* hashPtr,        ///< [OUT] RootFS hash.
    size_t* hashSizePtr      ///< [OUT] RootFS hash size.
)
{
    TAF_ERROR_IF_RET_VAL(hashPtr == NULL, LE_BAD_PARAMETER, "Null ptr(hashPtr)");

    TAF_ERROR_IF_RET_VAL(hashSizePtr == NULL, LE_BAD_PARAMETER, "Null ptr(hashSizePtr)");

    le_result_t result = LE_OK;
    auto& tafVerInfo = taf_verInfo::GetInstance();

    TAF_ERROR_IF_RET_VAL(tafVerInfo.hashInfPtr == NULL, LE_UNSUPPORTED,
        "RootFS hash is not available without plug-in.");

    if (bank == TAF_VERINFO_BANK_A)
    {
        result = tafVerInfo.GetHash(TAF_PI_HASH_COMP_ROOTFS, TAF_PI_HASH_BANK_A,
            hashPtr, hashSizePtr);
    }
    else if (bank == TAF_VERINFO_BANK_B)
    {
        result = tafVerInfo.GetHash(TAF_PI_HASH_COMP_ROOTFS, TAF_PI_HASH_BANK_B,
            hashPtr, hashSizePtr);
    }
    else
    {
        LE_ERROR("Invalid bank for rootFS.");
        return LE_BAD_PARAMETER;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the firmware hash.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_UNSUPPORTED -- Not supported.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_verInfo_GetFirmwareHash
(
    taf_verInfo_Bank_t bank, ///< [IN] Bank.
    uint8_t* hashPtr,        ///< [OUT] Firmware hash.
    size_t* hashSizePtr      ///< [OUT] Firmware hash size.
)
{
    TAF_ERROR_IF_RET_VAL(hashPtr == NULL, LE_BAD_PARAMETER, "Null ptr(hashPtr)");

    TAF_ERROR_IF_RET_VAL(hashSizePtr == NULL, LE_BAD_PARAMETER, "Null ptr(hashSizePtr)");

    le_result_t result = LE_OK;
    auto& tafVerInfo = taf_verInfo::GetInstance();

    TAF_ERROR_IF_RET_VAL(tafVerInfo.hashInfPtr == NULL, LE_UNSUPPORTED,
        "Firmware hash is not available without plug-in.");

    if (bank == TAF_VERINFO_BANK_A)
    {
        result = tafVerInfo.GetHash(TAF_PI_HASH_COMP_FIRMWARE, TAF_PI_HASH_BANK_A,
            hashPtr, hashSizePtr);
    }
    else if (bank == TAF_VERINFO_BANK_B)
    {
        result = tafVerInfo.GetHash(TAF_PI_HASH_COMP_FIRMWARE, TAF_PI_HASH_BANK_B,
            hashPtr, hashSizePtr);
    }
    else
    {
        LE_ERROR("Invalid bank for firmware.");
        return LE_BAD_PARAMETER;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Gets the LXC hash.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_UNSUPPORTED -- Not supported.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_verInfo_GetLXCHash
(
    taf_verInfo_Bank_t bank, ///< [IN] Bank.
    uint8_t* hashPtr,        ///< [OUT] LXC hash.
    size_t* hashSizePtr      ///< [OUT] LXC hash size.
)
{
    TAF_ERROR_IF_RET_VAL(hashPtr == NULL, LE_BAD_PARAMETER, "Null ptr(hashPtr)");

    TAF_ERROR_IF_RET_VAL(hashSizePtr == NULL, LE_BAD_PARAMETER, "Null ptr(hashSizePtr)");

    le_result_t result = LE_OK;
    auto& tafVerInfo = taf_verInfo::GetInstance();

    struct stat buffer;
    if(stat(LXC_VERSION_FILE, &buffer) != 0)
    {
        LE_ERROR("LXC not supported");
        return LE_UNSUPPORTED;
    }

    if (tafVerInfo.hashInfPtr != NULL)
    {
        if (bank == TAF_VERINFO_BANK_A)
        {
            result = tafVerInfo.GetHash(TAF_PI_HASH_COMP_LXC, TAF_PI_HASH_BANK_A,
                hashPtr, hashSizePtr);
        }
        else if (bank == TAF_VERINFO_BANK_B)
        {
            result = tafVerInfo.GetHash(TAF_PI_HASH_COMP_LXC, TAF_PI_HASH_BANK_B,
                hashPtr, hashSizePtr);
        }
        else
        {
            LE_ERROR("Invalid bank for LXC.");
            return LE_BAD_PARAMETER;
        }
    }
    else
    {
        taf_verInfo_Bank_t bootBank;
        result = tafVerInfo.GetBootBank(&bootBank);
        TAF_ERROR_IF_RET_VAL(result != LE_OK, LE_FAULT, "Fail to get the boot bank.");

        TAF_ERROR_IF_RET_VAL(bank != bootBank, LE_UNSUPPORTED,
            "TelAF hash is only available for boot bank.");

        char hashArray[TAF_VERINFO_VERSION_MAX_BYTES];
        FILE* fp = fopen(LXC_HASH_FILE, "r");
        if (fp != NULL)
        {
            char* p = fgets(hashArray, TAF_VERINFO_VERSION_MAX_BYTES, fp);
            fclose(fp);
            TAF_ERROR_IF_RET_VAL(p == NULL, LE_FAULT, "Fail to read hash file.")

            result = tafVerInfo.StringToHash(hashArray, hashPtr, hashSizePtr);
        }
    }

    return result;
}