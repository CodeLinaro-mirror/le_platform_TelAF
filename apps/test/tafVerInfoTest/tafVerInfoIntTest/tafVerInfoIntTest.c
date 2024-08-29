/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Print usage.
 */
//--------------------------------------------------------------------------------------------------
void PrintUsage
(
    void
)
{
    puts(
        "NAME:\n"
        "app runProc tafVerInfoIntTest tafVerInfoIntTest - Version Information Service "
        "Integration Test.\n"
        "\n"
        "SYNOPSIS:\n"
         "app runProc tafVerInfoIntTest tafVerInfoIntTest -- help\n"
         "app runProc tafVerInfoIntTest tafVerInfoIntTest -- version\n"
         "app runProc tafVerInfoIntTest tafVerInfoIntTest -- hash\n"
         "\n"
        "DESCRIPTION:\n"
        "    app runProc tafVerInfoIntTest tafVerInfoIntTest -- help\n"
        "       Display this help and exit.\n"
        "    app runProc tafVerInfoIntTest tafVerInfoIntTest -- version\n"
        "       Display version information.\n"
        "    app runProc tafVerInfoIntTest tafVerInfoIntTest -- hash\n"
        "       Display hash information.\n"
        "\n");
}

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
    LE_TEST_OK(result == LE_OK, "taf_verInfo_GetKernelVersion - OK");
    LE_INFO("Kernel version : %s", version);

    result = taf_verInfo_GetFirmwareVersion(version, sizeof(version));
    LE_TEST_OK(result == LE_OK, "taf_verInfo_GetFirmwareVersion - OK");
    LE_INFO("Firmware version : %s", version);

    result = taf_verInfo_GetTZVersion(version, sizeof(version));
    LE_TEST_OK(result == LE_OK, "taf_verInfo_GetTZVersion - OK");
    LE_INFO("TZ Version : %s", version);

    result = taf_verInfo_GetRootFSVersion(version, sizeof(version));
    LE_TEST_OK(result == LE_OK, "taf_verInfo_GetRootFSVersion - OK");
    LE_INFO("RootFS version : %s", version);

    result = taf_verInfo_GetTelAFVersion(version, sizeof(version));
    LE_TEST_OK(result == LE_OK, "taf_verInfo_GetTelAFVersion - OK");
    LE_INFO("TelAF version : %s", version);
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
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("======== Version Information Integration Test ========");

    const char *option = le_arg_GetArg(0);
    if (option == NULL)
    {
        LE_TEST_INFO("No options.");
        PrintUsage();
    }
    else if (strncmp(option, "version", strlen(option)) == 0)
    {
        LE_TEST_INFO("======== Version Test ========");
        Test_GetVersion();
    }
    else if (strncmp(option, "hash", strlen(option)) == 0)
    {
        LE_TEST_INFO("======== Hash Test ========");
        Test_GetHash();
    }
    else
    {
        PrintUsage();
    }

    LE_TEST_EXIT;
}
