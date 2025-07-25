/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Version information test.
 */
//--------------------------------------------------------------------------------------------------
static void Test_GetVersion
(
    void
)
{
    char version[TAF_VERINFO_VERSION_MAX_BYTES];
    le_result_t result = LE_OK;

    result = taf_verInfo_GetKernelVersion(version, sizeof(version));
    if(result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetKernelVersion - OK");
        LE_INFO("Kernel version : %s", version);
    }

    result = taf_verInfo_GetFirmwareVersion(version, sizeof(version));
    if(result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetFirmwareVersion - OK");
        LE_INFO("Firmware version : %s", version);
    }

    result = taf_verInfo_GetTZVersion(version, sizeof(version));
    if(result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetTZVersion - OK");
        LE_INFO("TZ Version : %s", version);
    }

    result = taf_verInfo_GetRootFSVersion(version, sizeof(version));
    if(result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetRootFSVersion - OK");
        LE_INFO("RootFS version : %s", version);
    }

    result = taf_verInfo_GetTelAFVersion(version, sizeof(version));
    if(result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetTelAFVersion - OK");
        LE_INFO("TelAF version : %s", version);
    }

    result = taf_verInfo_GetLXCVersion(version, sizeof(version));
    if(result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetLXCVersion - OK");
        LE_INFO("LXC version : %s", version);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Hash information test.
 */
//--------------------------------------------------------------------------------------------------
static void Test_GetHash
(
    void
)
{
    uint8_t hash[TAF_VERINFO_HASH_MAX_BYTES];
    size_t hashSize = TAF_VERINFO_HASH_MAX_BYTES;
    le_result_t result = LE_OK;

    result = taf_verInfo_GetTelAFHash(TAF_VERINFO_BANK_A, hash, &hashSize);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetTelAFHash - OK");
        printf("telaf_a hash : ");
        size_t i;
        for (i = 0; i < hashSize; i++)
        {
            printf("%02x", hash[i]);
        }
        printf("\n");
    }

    result = taf_verInfo_GetTelAFHash(TAF_VERINFO_BANK_B, hash, &hashSize);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetTelAFHash - OK");
        printf("telaf_b hash : ");
        size_t i;
        for (i = 0; i < hashSize; i++)
        {
            printf("%02x", hash[i]);
        }
        printf("\n");
    }

    result = taf_verInfo_GetBootHash(TAF_VERINFO_BANK_A, hash, &hashSize);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetBootHash - OK");
        printf("boot hash : ");
        size_t i;
        for (i = 0; i < hashSize; i++)
        {
            printf("%02x", hash[i]);
        }
        printf("\n");
    }

    result = taf_verInfo_GetBootHash(TAF_VERINFO_BANK_B, hash, &hashSize);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetBootHash - OK");
        printf("boot_b hash : ");
        size_t i;
        for (i = 0; i < hashSize; i++)
        {
            printf("%02x", hash[i]);
        }
        printf("\n");
    }

    result = taf_verInfo_GetRootFSHash(TAF_VERINFO_BANK_A, hash, &hashSize);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetRootFSHash - OK");
        printf("rootfs_a hash : ");
        size_t i;
        for (i = 0; i < hashSize; i++)
        {
            printf("%02x", hash[i]);
        }
        printf("\n");
    }

    result = taf_verInfo_GetRootFSHash(TAF_VERINFO_BANK_B, hash, &hashSize);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetRootFSHash - OK");
        printf("rootfs_b hash : ");
        size_t i;
        for (i = 0; i < hashSize; i++)
        {
            printf("%02x", hash[i]);
        }
        printf("\n");
    }

    result = taf_verInfo_GetFirmwareHash(TAF_VERINFO_BANK_A, hash, &hashSize);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetFirmwareHash - OK");
        printf("firmware_a hash : ");
        size_t i;
        for (i = 0; i < hashSize; i++)
        {
            printf("%02x", hash[i]);
        }
        printf("\n");
    }

    result = taf_verInfo_GetFirmwareHash(TAF_VERINFO_BANK_B, hash, &hashSize);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetFirmwareHash - OK");
        printf("firmware_b hash : ");
        size_t i;
        for (i = 0; i < hashSize; i++)
        {
            printf("%02x", hash[i]);
        }
        printf("\n");
    }

    result = taf_verInfo_GetLXCHash(TAF_VERINFO_BANK_A, hash, &hashSize);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetLXCHash - OK");
        printf("lxcrootfs_a hash : ");
        size_t i;
        for (i = 0; i < hashSize; i++)
        {
            printf("%02x", hash[i]);
        }
        printf("\n");
    }

    result = taf_verInfo_GetLXCHash(TAF_VERINFO_BANK_B, hash, &hashSize);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_verInfo_GetLXCHash - OK");
        printf("lxcrootfs_b hash : ");
        size_t i;
        for (i = 0; i < hashSize; i++)
        {
            printf("%02x", hash[i]);
        }
        printf("\n");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("======== Version Information Unit Test ========");

    LE_TEST_INFO("======== Version Test ========");
    Test_GetVersion();

    LE_TEST_INFO("======== Hash Test ========");
    Test_GetHash();

    LE_TEST_EXIT;
}
