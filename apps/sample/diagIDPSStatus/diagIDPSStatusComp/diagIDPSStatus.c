/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define VLAN_ID_10  10
#define VLAN_ID_110 110

static taf_diagIDPS_ServiceRef_t         IDPSSvcRef    = NULL;
static taf_diagIDPS_StatusHandlerRef_t   IDPSHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Convert SecurityLevel enum to a human-readable string.
 */
//--------------------------------------------------------------------------------------------------
static const char* SecurityLevelToStr
(
    taf_diagIDPS_SecurityLevel_t level
)
{
    switch (level)
    {
        case TAF_DIAGIDPS_LOW_LEVEL:  return "LOW_LEVEL (0x01)";
        case TAF_DIAGIDPS_HIGH_LEVEL: return "HIGH_LEVEL (0x02)";
        default:                      return "UNKNOWN";
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * IDPS status notification handler.
 * Called whenever an IDPS status event is received from tafDiagSvc.
 */
//--------------------------------------------------------------------------------------------------
static void IDPSStatusHandler
(
    taf_diagIDPS_StatusMsgRef_t statusMsgRef,   ///< IDPS status message reference.
    taf_diagIDPS_SecurityLevel_t securityLevel, ///< Security level.
    uint8_t serviceId,                          ///< Service ID (e.g. 0x27).
    uint8_t status,                             ///< 0 = success, otherwise NRC.
    void* contextPtr                            ///< User context pointer.
)
{
    le_result_t result;

    LE_INFO("===== IDPS Status Notification =====");
    LE_INFO("  Security Level : %s", SecurityLevelToStr(securityLevel));
    LE_INFO("  Service ID     : 0x%02X", serviceId);
    LE_INFO("  Status         : 0x%02X (%s)", status, (status == 0) ? "Success" : "NRC");

    // Get VLAN ID from the message
    uint16_t vlanId = 0;
    result = taf_diagIDPS_GetVlanIdFromMsg(statusMsgRef, &vlanId);
    if (result == LE_OK)
    {
        LE_INFO("  VLAN ID        : %u", vlanId);
    }
    else
    {
        LE_WARN("  Failed to get VLAN ID (result=%d)", result);
    }

    // Get logical source/target addresses
    uint16_t sourceAddr = 0;
    uint16_t targetAddr = 0;
    result = taf_diagIDPS_GetLogicalAddr(statusMsgRef, &sourceAddr, &targetAddr);
    if (result == LE_OK)
    {
        LE_INFO("  Source Addr    : 0x%04X", sourceAddr);
        LE_INFO("  Target Addr    : 0x%04X", targetAddr);
    }
    else
    {
        LE_WARN("  Failed to get logical addresses (result=%d)", result);
    }

    // Get data size
    uint16_t dataSize = 0;
    result = taf_diagIDPS_GetDataSize(statusMsgRef, &dataSize);
    if (result == LE_OK)
    {
        LE_INFO("  Data Size      : %u", dataSize);

        // Get data payload if size > 0
        if (dataSize > 0)
        {
            uint8_t data[TAF_DIAGIDPS_MAX_DATA_SIZE] = {0};
            size_t dataLen = TAF_DIAGIDPS_MAX_DATA_SIZE;
            result = taf_diagIDPS_GetData(statusMsgRef, data, &dataLen);
            if (result == LE_OK)
            {
                LE_INFO("  Data (%zu bytes):", dataLen);
                for (size_t i = 0; i < dataLen; i++)
                {
                    LE_INFO("    data[%zu] = 0x%02X", i, data[i]);
                }
            }
            else
            {
                LE_WARN("  Failed to get data (result=%d)", result);
            }
        }
    }
    else
    {
        LE_WARN("  Failed to get data size (result=%d)", result);
    }

    // Get extra data size
    uint16_t extraDataSize = 0;
    result = taf_diagIDPS_GetExtraDataSize(statusMsgRef, &extraDataSize);
    if (result == LE_OK)
    {
        LE_INFO(" Extra Data Size: %u", extraDataSize);

        // Get extra data payload if size > 0
        if (extraDataSize > 0)
        {
            uint8_t extraData[TAF_DIAGIDPS_MAX_DATA_SIZE] = {0};
            size_t extraDataLen = TAF_DIAGIDPS_MAX_DATA_SIZE;
            result = taf_diagIDPS_GetExtraData(statusMsgRef, extraData, &extraDataLen);
            if (result == LE_OK)
            {
                LE_INFO(" Extra Data (%zu bytes):", extraDataLen);
                for (size_t i = 0; i < extraDataLen; i++)
                {
                    LE_INFO("    extraData[%zu] = 0x%02X", i, extraData[i]);
                }
            }
            else
            {
                LE_WARN("  Failed to get extra data (result=%d)", result);
            }
        }
    }
    else
    {
        LE_WARN("  Failed to get extra data size (result=%d)", result);
    }

    LE_INFO("====================================");

    // Release the status message after processing
    result = taf_diagIDPS_ReleaseStatusMsg(statusMsgRef);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to release IDPS status message (result=%d)", result);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse the security level from the command-line argument string.
 *
 * Accepted values:
 *   "low"  / "1"   -> TAF_DIAGIDPS_LOW_LEVEL  (all notifications)
 *   "high" / "2"   -> TAF_DIAGIDPS_HIGH_LEVEL (security access and authentication only)
 *
 * @return true on success, false if the argument is unrecognised.
 */
//--------------------------------------------------------------------------------------------------
static bool ParseSecurityLevel
(
    const char* argStr,
    taf_diagIDPS_SecurityLevel_t* levelPtr
)
{
    if (strcmp(argStr, "low") == 0 || strcmp(argStr, "1") == 0)
    {
        *levelPtr = TAF_DIAGIDPS_LOW_LEVEL;
        return true;
    }
    else if (strcmp(argStr, "high") == 0 || strcmp(argStr, "2") == 0)
    {
        *levelPtr = TAF_DIAGIDPS_HIGH_LEVEL;
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register IDPS status handler without VLAN.
 */
//--------------------------------------------------------------------------------------------------
static void RegisterIDPSWithoutVlan
(
    taf_diagIDPS_SecurityLevel_t securityLevel
)
{
    IDPSSvcRef = taf_diagIDPS_GetService(securityLevel);
    if (IDPSSvcRef == NULL)
    {
        LE_ERROR("Failed to get IDPS service reference");
        exit(EXIT_FAILURE);
    }

    LE_INFO("Got IDPS service reference successfully");

    IDPSHandlerRef = taf_diagIDPS_AddStatusHandler(
                        IDPSSvcRef,
                        IDPSStatusHandler,
                        NULL);

    if (IDPSHandlerRef == NULL)
    {
        LE_ERROR("Failed to register IDPS status handler");
        taf_diagIDPS_RemoveSvc(IDPSSvcRef);
        exit(EXIT_FAILURE);
    }

    LE_INFO("IDPS status handler registered successfully (non-VLAN)");
    LE_INFO("Waiting for IDPS status notifications...");
}

//--------------------------------------------------------------------------------------------------
/**
 * Register IDPS status handler with VLAN 10 and VLAN 110.
 */
//--------------------------------------------------------------------------------------------------
static void RegisterIDPSWithVlan
(
    taf_diagIDPS_SecurityLevel_t securityLevel
)
{
    le_result_t result;

    IDPSSvcRef = taf_diagIDPS_GetService(securityLevel);
    if (IDPSSvcRef == NULL)
    {
        LE_ERROR("Failed to get IDPS service reference");
        exit(EXIT_FAILURE);
    }

    LE_INFO("Got IDPS service reference successfully");

    // Associate VLAN 10 to the service reference
    result = taf_diagIDPS_SetVlanId(IDPSSvcRef, VLAN_ID_10);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to set VLAN ID %d (result=%d)", VLAN_ID_10, result);
        taf_diagIDPS_RemoveSvc(IDPSSvcRef);
        exit(EXIT_FAILURE);
    }

    LE_INFO("Set VLAN ID %d successfully", VLAN_ID_10);

    // Associate VLAN 110 to the service reference
    result = taf_diagIDPS_SetVlanId(IDPSSvcRef, VLAN_ID_110);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to set VLAN ID %d (result=%d)", VLAN_ID_110, result);
        taf_diagIDPS_RemoveSvc(IDPSSvcRef);
        exit(EXIT_FAILURE);
    }

    LE_INFO("Set VLAN ID %d successfully", VLAN_ID_110);

    // Register the IDPS status handler — covers both VLANs via the same svcRef
    IDPSHandlerRef = taf_diagIDPS_AddStatusHandler(
                        IDPSSvcRef,
                        IDPSStatusHandler,
                        NULL);

    if (IDPSHandlerRef == NULL)
    {
        LE_ERROR("Failed to register IDPS status handler");
        taf_diagIDPS_RemoveSvc(IDPSSvcRef);
        exit(EXIT_FAILURE);
    }

    LE_INFO("IDPS status handler registered successfully (VLAN %d and VLAN %d)",
            VLAN_ID_10, VLAN_ID_110);
    LE_INFO("Waiting for IDPS status notifications...");
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialisation.
 *
 * Usage:
 *   app runProc tafDiagIDPSStatus --exe=tafDiagIDPSStatus -- <vlan_type> <security_level>
 *
 * <vlan_type> can be:
 *   nonVlan  -> no VLAN, use default interface
 *   vlan     -> bind to VLAN 10 and VLAN 110
 *
 * <security_level> can be:
 *   low  | 1    -> LOW_LEVEL  (all notifications)
 *   high | 2    -> HIGH_LEVEL (security access and authentication only)
 *
 * Examples:
 *   app runProc tafDiagIDPSStatus --exe=tafDiagIDPSStatus -- nonVlan high
 *   app runProc tafDiagIDPSStatus --exe=tafDiagIDPSStatus -- vlan low
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("tafDiagIDPSStatus app starting");

    int numArgs = le_arg_NumArgs();
    LE_INFO("Total argument count: %d", numArgs);

    if (numArgs != 2)
    {
        printf("Usage: app runProc tafDiagIDPSStatus --exe=tafDiagIDPSStatus -- <vlan_type> <security_level>\n");
        printf("  vlan_type     : vlan | nonVlan\n");
        printf("  security_level: low|1  high|2\n");
        printf("  eg: app runProc tafDiagIDPSStatus --exe=tafDiagIDPSStatus -- nonVlan high\n");
        printf("  eg: app runProc tafDiagIDPSStatus --exe=tafDiagIDPSStatus -- vlan low\n");
        exit(EXIT_FAILURE);
    }

    const char* vlanTypePtr = le_arg_GetArg(0);
    if (vlanTypePtr == NULL)
    {
        LE_ERROR("vlanTypePtr is NULL");
        exit(EXIT_FAILURE);
    }

    const char* secLevelPtr = le_arg_GetArg(1);
    if (secLevelPtr == NULL)
    {
        LE_ERROR("secLevelPtr is NULL");
        exit(EXIT_FAILURE);
    }

    taf_diagIDPS_SecurityLevel_t securityLevel;
    if (!ParseSecurityLevel(secLevelPtr, &securityLevel))
    {
        LE_ERROR("Invalid security level '%s'. Use: low|1  high|2", secLevelPtr);
        exit(EXIT_FAILURE);
    }

    LE_INFO("Security level: %s", SecurityLevelToStr(securityLevel));

    if (strcmp(vlanTypePtr, "nonVlan") == 0)
    {
        LE_INFO("Registering IDPS status handler without VLAN");
        RegisterIDPSWithoutVlan(securityLevel);
    }
    else if (strcmp(vlanTypePtr, "vlan") == 0)
    {
        LE_INFO("Registering IDPS status handler with VLAN %d and VLAN %d",
                VLAN_ID_10, VLAN_ID_110);
        RegisterIDPSWithVlan(securityLevel);
    }
    else
    {
        printf("\n === vlan type argument is not correct ===\n");
        LE_ERROR("Invalid vlan type '%s'. Use: vlan | nonVlan", vlanTypePtr);
        exit(EXIT_FAILURE);
    }
}
