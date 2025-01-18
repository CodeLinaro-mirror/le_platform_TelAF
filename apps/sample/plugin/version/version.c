/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#include "version.h"
#include "tafPiVersion.h"

extern version_InfoTab_t TAF_HAL_INFO_TAB;

//--------------------------------------------------------------------------------------------------
/**
 * Get version plugin information table.
 *
 * @return
 * - NULL   -- Failed.
 * - Others -- Version module information.
 */
//--------------------------------------------------------------------------------------------------
void* taf_hal_GetModInf
(
    void
)
{
    LE_INFO("Get version module information table.");

    return &(TAF_HAL_INFO_TAB.versionInf);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize version plugin.
 */
//--------------------------------------------------------------------------------------------------
void taf_piVersion_Init
(
    void
)
{
    LE_INFO("Version Plug-In Init...");

    LE_INFO("Version Plug-In Ready...");
}

//--------------------------------------------------------------------------------------------------
/**
 * Get version format.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_piVersion_GetFormat
(
    void
)
{
    return TAF_PI_VERSION_FORMAT_MAJOR_MINOR_PATCH;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get boot version.
 */
//--------------------------------------------------------------------------------------------------
int taf_piVersion_GetBootVersion
(
    taf_pi_version_Tier_t tier, ///< [IN] Version tier.
    char* version,              ///< [OUT] Version string.
    size_t versionSize          ///< [IN] Size of the version string.
)
{
    FILE *fp = fopen(BOOT_VERSION_FILE, "r");
    if (fp == NULL)
    {
        LE_ERROR("Can not open file %s.", BOOT_VERSION_FILE);
        return -1;
    }

    char line[MAX_LINE_LEN];
    if (fgets(line, sizeof(line), fp) == NULL)
    {
        LE_ERROR("Can not read file %s.", BOOT_VERSION_FILE);
        fclose(fp);
        return -1;
    }

    char* majorPtr = strstr(line, BOOT_VERSION_PREFIX);
    if (majorPtr == NULL)
    {
        LE_ERROR("Invalid boot version : %s", line);
        fclose(fp);
        return -1;
    }

    majorPtr += strlen(BOOT_VERSION_PREFIX);
    char* minorPtr = strstr(majorPtr, ".");
    char* patchPtr = NULL;
    char* endPtr = NULL;
    if (minorPtr != NULL)
    {
        patchPtr = strstr(minorPtr + 1, ".");
        if (patchPtr != NULL)
        {
            endPtr = strstr(patchPtr + 1, "-");
        }
    }

    if (tier == TAF_PI_VERSION_MAJOR)
    {
        if (minorPtr != NULL)
        {
            versionSize = minorPtr - majorPtr + 1;
        }
        le_utf8_Copy(version, majorPtr, versionSize, NULL);
    }
    else if (tier == TAF_PI_VERSION_MINOR && minorPtr != NULL)
    {
        minorPtr++;
        if (patchPtr != NULL)
        {
            versionSize = patchPtr - minorPtr + 1;
        }
        le_utf8_Copy(version, minorPtr, versionSize, NULL);
    }
    else if (tier == TAF_PI_VERSION_PATCH && patchPtr != NULL)
    {
        patchPtr++;
        if (endPtr != NULL)
        {
            versionSize = endPtr - patchPtr + 1;
        }
        le_utf8_Copy(version, patchPtr, versionSize, NULL);
    }
    else
    {
        LE_ERROR("Invalid tier %d.", tier);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get RootFS version.
 */
//--------------------------------------------------------------------------------------------------
int taf_piVersion_GetRootfsVersion
(
    taf_pi_version_Tier_t tier, ///< [IN] Version tier.
    char* version,              ///< [OUT] Version string.
    size_t versionSize          ///< [IN] Size of the version string.
)
{
    FILE *fp = fopen(ROOTFS_VERSION_FILE, "r");
    if (fp == NULL)
    {
        LE_ERROR("Can not open file %s.", ROOTFS_VERSION_FILE);
        return -1;
    }

    char line[MAX_LINE_LEN];
    if (fgets(line, sizeof(line), fp) == NULL)
    {
        LE_ERROR("Can not read file %s.", ROOTFS_VERSION_FILE);
        fclose(fp);
        return -1;
    }

    bool isAuPrefix = true;
    char* majorPtr = strstr(line, ROOTFS_VERSION_AU_PREFIX);
    if (majorPtr == NULL)
    {
        majorPtr = strstr(line, ROOTFS_VERSION_LE_PREFIX);
        if (majorPtr == NULL)
        {
            LE_ERROR("Invalid rootfs version : %s", line);
            fclose(fp);
            return -1;
        }
        isAuPrefix = false;
        majorPtr += strlen(ROOTFS_VERSION_LE_PREFIX);
    }
    else
    {
        majorPtr += strlen(ROOTFS_VERSION_AU_PREFIX);
    }

    char* minorPtr = strstr(majorPtr, ".");
    char* patchPtr = NULL;
    char* endPtr = NULL;
    if (minorPtr != NULL)
    {
        patchPtr = strstr(minorPtr + 1, ".");
        if (patchPtr != NULL)
        {
            if (isAuPrefix)
            {
                endPtr = strstr(patchPtr + 1, "-");
            }
            else
            {
                endPtr = strstr(patchPtr + 1, ".");
            }
        }
    }

    if (tier == TAF_PI_VERSION_MAJOR)
    {
        if (minorPtr != NULL)
        {
            versionSize = minorPtr - majorPtr + 1;
        }
        le_utf8_Copy(version, majorPtr, versionSize, NULL);
    }
    else if (tier == TAF_PI_VERSION_MINOR && minorPtr != NULL)
    {
        minorPtr++;
        if (patchPtr != NULL)
        {
            versionSize = patchPtr - minorPtr + 1;
        }
        le_utf8_Copy(version, minorPtr, versionSize, NULL);
    }
    else if (tier == TAF_PI_VERSION_PATCH && patchPtr != NULL)
    {
        patchPtr++;
        if (endPtr != NULL)
        {
            versionSize = endPtr - patchPtr + 1;
        }
        le_utf8_Copy(version, patchPtr, versionSize, NULL);
    }
    else
    {
        LE_ERROR("Invalid tier %d.", tier);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get firmware version.
 */
//--------------------------------------------------------------------------------------------------
int taf_piVersion_GetFirmwareVersion
(
    taf_pi_version_Tier_t tier, ///< [IN] Version tier.
    char* version,              ///< [OUT] Version string.
    size_t versionSize          ///< [IN] Size of the version string.
)
{
    FILE *fp = fopen(FIRMWARE_VERSION_FILE, "r");
    if (fp == NULL)
    {
        LE_ERROR("Can not open file %s.", FIRMWARE_VERSION_FILE);
        return -1;
    }

    char line[MAX_LINE_LEN];
    char* majorPtr = NULL;
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        majorPtr = strstr(line, FIRMWARE_VERSION_PREFIX);
        if (majorPtr != NULL)
        {
            break;
        }
    }

    if (majorPtr == NULL)
    {
        LE_ERROR("Invalid firmware version : %s", line);
        fclose(fp);
        return -1;
    }

    majorPtr += strlen(FIRMWARE_VERSION_PREFIX);
    char* minorPtr = strstr(majorPtr, ".");
    char* patchPtr = NULL;
    char* endPtr = NULL;
    if (minorPtr != NULL)
    {
        patchPtr = strstr(minorPtr + 1, "-");
        if (patchPtr != NULL)
        {
            endPtr = strstr(patchPtr + 1, ".");
        }
    }

    if (tier == TAF_PI_VERSION_MAJOR)
    {
        if (minorPtr != NULL)
        {
            versionSize = minorPtr - majorPtr + 1;
        }
        le_utf8_Copy(version, majorPtr, versionSize, NULL);
    }
    else if (tier == TAF_PI_VERSION_MINOR && minorPtr != NULL)
    {
        minorPtr++;
        if (patchPtr != NULL)
        {
            versionSize = patchPtr - minorPtr + 1;
        }
        le_utf8_Copy(version, minorPtr, versionSize, NULL);
    }
    else if (tier == TAF_PI_VERSION_PATCH && patchPtr != NULL)
    {
        patchPtr++;
        if (endPtr != NULL)
        {
            versionSize = endPtr - patchPtr + 1;
        }
        le_utf8_Copy(version, patchPtr, versionSize, NULL);
    }
    else
    {
        LE_ERROR("Invalid tier %d.", tier);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get TZ version.
 */
//--------------------------------------------------------------------------------------------------
int taf_piVersion_GetTzVersion
(
    taf_pi_version_Tier_t tier, ///< [IN] Version tier.
    char* version,              ///< [OUT] Version string.
    size_t versionSize          ///< [IN] Size of the version string.
)
{
    FILE *fp = fopen(FIRMWARE_VERSION_FILE, "r");
    if (fp == NULL)
    {
        LE_ERROR("Can not open file %s.", FIRMWARE_VERSION_FILE);
        return -1;
    }

    char line[MAX_LINE_LEN];
    char* majorPtr = NULL;
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        majorPtr = strstr(line, TZ_VERSION_PREFIX);
        if (majorPtr != NULL)
        {
            break;
        }
    }

    if (majorPtr == NULL)
    {
        LE_ERROR("Invalid tz version : %s", line);
        fclose(fp);
        return -1;
    }

    majorPtr += strlen(TZ_VERSION_PREFIX);
    char* minorPtr = strstr(majorPtr, ".");
    char* patchPtr = NULL;
    char* endPtr = NULL;
    if (minorPtr != NULL)
    {
        patchPtr = strstr(minorPtr + 1, "-");
        if (patchPtr != NULL)
        {
            endPtr = strstr(patchPtr + 1, ".");
        }
    }

    if (tier == TAF_PI_VERSION_MAJOR)
    {
        if (minorPtr != NULL)
        {
            versionSize = minorPtr - majorPtr + 1;
        }
        le_utf8_Copy(version, majorPtr, versionSize, NULL);
    }
    else if (tier == TAF_PI_VERSION_MINOR && minorPtr != NULL)
    {
        minorPtr++;
        if (patchPtr != NULL)
        {
            versionSize = patchPtr - minorPtr + 1;
        }
        le_utf8_Copy(version, minorPtr, versionSize, NULL);
    }
    else if (tier == TAF_PI_VERSION_PATCH && patchPtr != NULL)
    {
        patchPtr++;
        if (endPtr != NULL)
        {
            versionSize = endPtr - patchPtr + 1;
        }
        le_utf8_Copy(version, patchPtr, versionSize, NULL);
    }
    else
    {
        LE_ERROR("Invalid tier %d.", tier);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get TelAF version.
 */
//--------------------------------------------------------------------------------------------------
int taf_piVersion_GetTelafVersion
(
    taf_pi_version_Tier_t tier, ///< [IN] Version tier.
    char* version,              ///< [OUT] Version string.
    size_t versionSize          ///< [IN] Size of the version string.
)
{
    FILE *fp = fopen(TELAF_VERSION_FILE, "r");
    if (fp == NULL)
    {
        LE_ERROR("Can not open file %s.", TELAF_VERSION_FILE);
        return -1;
    }

    char line[MAX_LINE_LEN];
    if (fgets(line, sizeof(line), fp) == NULL)
    {
        LE_ERROR("Can not read file %s.", TELAF_VERSION_FILE);
        fclose(fp);
        return -1;
    }

    char* majorPtr = strstr(line, TELAF_VERSION_PREFIX);
    if (majorPtr == NULL)
    {
        LE_ERROR("Invalid telaf version : %s", line);
        fclose(fp);
        return -1;
    }

    majorPtr += strlen(TELAF_VERSION_PREFIX);
    char* minorPtr = majorPtr + TELAF_VERSION_MAJOR_LEN;
    char* patchPtr = minorPtr + TELAF_VERSION_MINOR_LEN;

    if (tier == TAF_PI_VERSION_MAJOR)
    {
        le_utf8_Copy(version, majorPtr, TELAF_VERSION_MAJOR_LEN + 1, NULL);
    }
    else if (tier == TAF_PI_VERSION_MINOR)
    {
        le_utf8_Copy(version, minorPtr, TELAF_VERSION_MINOR_LEN + 1, NULL);
    }
    else if (tier == TAF_PI_VERSION_PATCH)
    {
        le_utf8_Copy(version, patchPtr, TELAF_VERSION_PATCH_LEN + 1, NULL);
    }
    else
    {
        LE_ERROR("Invalid tier %d.", tier);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get LXC version.
 */
//--------------------------------------------------------------------------------------------------
int taf_piVersion_GetLXCVersion
(
    taf_pi_version_Tier_t tier, ///< [IN] Version tier.
    char* version,              ///< [OUT] Version string.
    size_t versionSize          ///< [IN] Size of the version string.
)
{
    FILE *fp = fopen(LXC_VERSION_FILE, "r");
    if (fp == NULL)
    {
        LE_ERROR("Can not open file %s.", LXC_VERSION_FILE);
        return -1;
    }

    char line[MAX_LINE_LEN];
    if (fgets(line, sizeof(line), fp) == NULL)
    {
        LE_ERROR("Can not read file %s.", LXC_VERSION_FILE);
        fclose(fp);
        return -1;
    }

    bool isAuPrefix = true;
    char* majorPtr = strstr(line, LXC_VERSION_AU_PREFIX);
    if (majorPtr == NULL)
    {
        majorPtr = strstr(line, LXC_VERSION_LE_PREFIX);
        if (majorPtr == NULL)
        {
            LE_ERROR("Invalid lxc version : %s", line);
            fclose(fp);
            return -1;
        }
        isAuPrefix = false;
        majorPtr += strlen(LXC_VERSION_LE_PREFIX);
    }
    else
    {
        majorPtr += strlen(LXC_VERSION_AU_PREFIX);
    }

    char* minorPtr = strstr(majorPtr, ".");
    char* patchPtr = NULL;
    char* endPtr = NULL;
    if (minorPtr != NULL)
    {
        patchPtr = strstr(minorPtr + 1, ".");
        if (patchPtr != NULL)
        {
            if (isAuPrefix)
            {
                endPtr = strstr(patchPtr + 1, "-");
            }
            else
            {
                endPtr = strstr(patchPtr + 1, ".");
            }
        }
    }

    if (tier == TAF_PI_VERSION_MAJOR)
    {
        if (minorPtr != NULL)
        {
            versionSize = minorPtr - majorPtr + 1;
        }
        le_utf8_Copy(version, majorPtr, versionSize, NULL);
    }
    else if (tier == TAF_PI_VERSION_MINOR && minorPtr != NULL)
    {
        minorPtr++;
        if (patchPtr != NULL)
        {
            versionSize = patchPtr - minorPtr + 1;
        }
        le_utf8_Copy(version, minorPtr, versionSize, NULL);
    }
    else if (tier == TAF_PI_VERSION_PATCH && patchPtr != NULL)
    {
        patchPtr++;
        if (endPtr != NULL)
        {
            versionSize = endPtr - patchPtr + 1;
        }
        le_utf8_Copy(version, patchPtr, versionSize, NULL);
    }
    else
    {
        LE_ERROR("Invalid tier %d.", tier);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get major version.
 */
//--------------------------------------------------------------------------------------------------
int taf_piVersion_GetMajor
(
    taf_pi_version_Comp_t component, ///< [IN] Component.
    char* versionPtr,                ///< [OUT] Major version string.
    size_t versionSize               ///< [IN] Size of the version string.
)
{
    switch (component)
    {
        case TAF_PI_VERSION_COMP_BOOT:
            return taf_piVersion_GetBootVersion(TAF_PI_VERSION_MAJOR, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_ROOTFS:
            return taf_piVersion_GetRootfsVersion(TAF_PI_VERSION_MAJOR, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_FIRMWARE:
            return taf_piVersion_GetFirmwareVersion(TAF_PI_VERSION_MAJOR, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_TZ:
            return taf_piVersion_GetTzVersion(TAF_PI_VERSION_MAJOR, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_TELAF:
            return taf_piVersion_GetTelafVersion(TAF_PI_VERSION_MAJOR, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_LXC:
            return taf_piVersion_GetLXCVersion(TAF_PI_VERSION_MAJOR, versionPtr, versionSize);
        default:
            LE_ERROR("Invalid component : %d", component);
    }
    return -1;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get minor version.
 */
//--------------------------------------------------------------------------------------------------
int taf_piVersion_GetMinor
(
    taf_pi_version_Comp_t component, ///< [IN] Component.
    char* versionPtr,                ///< [OUT] Minor version string.
    size_t versionSize               ///< [IN] Size of the version string.
)
{
    switch (component)
    {
        case TAF_PI_VERSION_COMP_BOOT:
            return taf_piVersion_GetBootVersion(TAF_PI_VERSION_MINOR, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_ROOTFS:
            return taf_piVersion_GetRootfsVersion(TAF_PI_VERSION_MINOR, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_FIRMWARE:
            return taf_piVersion_GetFirmwareVersion(TAF_PI_VERSION_MINOR, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_TZ:
            return taf_piVersion_GetTzVersion(TAF_PI_VERSION_MINOR, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_TELAF:
            return taf_piVersion_GetTelafVersion(TAF_PI_VERSION_MINOR, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_LXC:
            return taf_piVersion_GetLXCVersion(TAF_PI_VERSION_MINOR, versionPtr, versionSize);
        default:
            LE_ERROR("Invalid component : %d", component);
    }
    return -1;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get patch version.
 */
//--------------------------------------------------------------------------------------------------
int taf_piVersion_GetPatch
(
    taf_pi_version_Comp_t component, ///< [IN] Component.
    char* versionPtr,                ///< [OUT] Patch version string.
    size_t versionSize               ///< [IN] Size of the version string.
)
{
    switch (component)
    {
        case TAF_PI_VERSION_COMP_BOOT:
            return taf_piVersion_GetBootVersion(TAF_PI_VERSION_PATCH, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_ROOTFS:
            return taf_piVersion_GetRootfsVersion(TAF_PI_VERSION_PATCH, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_FIRMWARE:
            return taf_piVersion_GetFirmwareVersion(TAF_PI_VERSION_PATCH, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_TZ:
            return taf_piVersion_GetTzVersion(TAF_PI_VERSION_PATCH, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_TELAF:
            return taf_piVersion_GetTelafVersion(TAF_PI_VERSION_PATCH, versionPtr, versionSize);
        case TAF_PI_VERSION_COMP_LXC:
            return taf_piVersion_GetLXCVersion(TAF_PI_VERSION_PATCH, versionPtr, versionSize);
        default:
            LE_ERROR("Invalid component : %d", component);
    }
    return -1;
}

//--------------------------------------------------------------------------------------------------
/**
 * Version information table.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED version_InfoTab_t TAF_HAL_INFO_TAB =
{
    .mgrInf =
    {
        .name = TAF_VERSION_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .moduleType = TAF_MODULETYPE_PLUG_IN,
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    },

    .versionInf =
    {
        .init = taf_piVersion_Init,
        .getFormat = taf_piVersion_GetFormat,
        .getMajor = taf_piVersion_GetMajor,
        .getMinor = taf_piVersion_GetMinor,
        .getPatch = taf_piVersion_GetPatch,
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