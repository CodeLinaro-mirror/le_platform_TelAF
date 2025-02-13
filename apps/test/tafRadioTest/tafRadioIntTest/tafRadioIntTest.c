/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"

#include "tafRadioIntTest.h"

taf_radio_NetRegStateEventHandlerRef_t netRegStateHandlerRef;
taf_radio_PacketSwitchedChangeHandlerRef_t packSwStateHandlerRef;
taf_radio_NetRegRejectHandlerRef_t netRegRejHandlerRef;
taf_radio_RatChangeHandlerRef_t ratChangeHandlerRef;
taf_radio_SignalStrengthChangeHandlerRef_t gsmSsChangeHandlerRef;
taf_radio_SignalStrengthChangeHandlerRef_t umtsSsChangeHandlerRef;
taf_radio_SignalStrengthChangeHandlerRef_t cdmaSsChangeHandlerRef;
taf_radio_SignalStrengthChangeHandlerRef_t lteSsChangeHandlerRef;
taf_radio_SignalStrengthChangeHandlerRef_t nr5gSsChangeHandlerRef;
taf_radio_ImsRegStatusChangeHandlerRef_t imsRegStatusChangeHandlerRef;
taf_radio_OpModeChangeHandlerRef_t opModeChangeHandlerRef;
taf_radio_NetStatusChangeHandlerRef_t netStatusChangeHandlerRef;
taf_radio_ImsStatusChangeHandlerRef_t imsStatusChangeHandlerRef;
taf_radio_CellInfoChangeHandlerRef_t cellInfoChangeHandlerRef;
taf_radio_NrIconTypeHandlerRef_t nrIconTypeHandlerRef;

//--------------------------------------------------------------------------------------------------
/**
 * Print help menu to stdout and exit.
 */
//--------------------------------------------------------------------------------------------------
void PrintHelpMenu
(
    void
)
{
    puts(
        "NAME:\n"
        "app runProc tafRadioIntTest tafRadioIntTest - Radio Service Integration Test.\n"
        "\n"
        "SYNOPSIS:\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- help\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- power <phone> <on|off|status>\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "reg <phone> <mode|status> [<mcc>] [<mnc>]\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "rat <phone> <prefer|status> [<rat_bitmask>]\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "domain <phone> <prefer|status> [<cs|ps|all>]\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "operator <phone> <add|remove|list> [<mcc>] [<mnc>] [<rat_bitmask>]\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "signal <phone> <monitor|metrics|delta> [<time|rat>] [<signal_delta>] [<hysdB>]\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- serving <phone>\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- neighbor <phone>\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- scan <phone> <mode> [<rat_bitmask>]\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "band <phone> <rat|status> [<band_bitmask>]\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "ims <phone> <mode> [<service>|<user_agent>]\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- handler\n"
        "\n"
        "DESCRIPTION:\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- help\n"
        "       Display this help and exit.\n"
        "\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- power <phone> <on|off|status>\n"
        "       phone : '1' or '2'.\n"
        "       Power 'on' or 'off' radio, or show radio power 'status'.\n"
        "\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "reg <phone> <mode|status> [<mcc>] [<mnc>]\n"
        "       phone : '1' or '2'.\n"
        "       Network registation with mode 'auto', 'manual-sync' or 'manual-async', or show "
        "network registration 'status'.\n"
        "       mcc : mobile country code, required with manual registration.\n"
        "       mnc : mobile network code, required with manual registration.\n"
        "\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "rat <phone> <prefer|status> [<rat_bitmask>]\n"
        "       phone : '1' or '2'.\n"
        "       Set rat preferences with 'prefer', or show current rat in use and peferences "
        "'status'.\n"
        "       rat_bitmask : rat bitmask, required with 'prefer' option.\n"
        "           GSM     : 0x1.\n"
        "           UMTS    : 0x2.\n"
        "           CDMA    : 0x4.\n"
        "           TDSCDMA : 0x8.\n"
        "           LTE     : 0x10.\n"
        "           NR5G    : 0x20.\n"
        "           ALL     : 0x40.\n"
        "\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "domain <phone> <prefer|status> [<cs|ps|all>]\n"
        "       phone : '1' or '2'.\n"
        "       Set service domain preferences with 'prefer', or show current service domain and "
        "peferences 'status'.\n"
        "       'cs' for Circuit-Switched, 'ps' for Packet-Switched, and 'all' for both, required "
        "with 'prefer' option.\n"
        "\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "operator <phone> <add|remove|list> [<mcc>] [<mnc>] [<rat_bitmask>]\n"
        "       phone       : '1' or '2'.\n"
        "       'add' or 'remove' preferred operator, or 'list' to show operator preferences.\n"
        "       mcc         : mobile country code, required with 'remove' or 'add' option.\n"
        "       mnc         : mobile network code, required with 'remove' or 'add' option.\n"
        "       rat_bitmask : rat bit mask, required with 'add' option.\n"
        "           GSM     : 0x1.\n"
        "           UMTS    : 0x2.\n"
        "           CDMA    : 0x4.\n"
        "           TDSCDMA : 0x8.\n"
        "           LTE     : 0x10.\n"
        "           NR5G    : 0x20.\n"
        "           ALL     : 0x40.\n"
        "\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "signal <phone> <monitor|metrics|delta> [<time|rat>] [<signal_delta>]\n"
        "       phone        : '1' or '2'.\n"
        "       'monitor' signal strength changes, or show signal 'metircs'.\n"
        "       time         : time in seconds, required with 'monitor' option.\n"
        "       rat          : radio access technoloy, required with 'delta' option.\n"
        "       signal_delta : signal deltas.\n"
        "           rssi delta in 0.1 dBm, required with 'delta' for RATs except LTE and NR5G.\n"
        "           rsrp_delta : rsrp delta in 0.1 dBm, required with 'delta' for LTE or NR5G.\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- serving <phone>\n"
        "       Show serving system status.\n"
        "       phone : '1' or '2'.\n"
        "\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- neighbor <phone>\n"
        "       Show neighboring cells information.\n"
        "       phone : '1' or '2'.\n"
        "\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- scan <phone> <mode> [<rat_bitmask>]\n"
        "       Perform network scan.\n"
        "       phone       : '1' or '2'.\n"
        "       mode        : 'plmn-sync', 'plmn-async', 'pci-sync' or 'pci-async'.\n"
        "       rat_bitmask : rat bit mask, required with 'pci-sync' 'pci-async' mode.\n"
        "           GSM     : 0x1.\n"
        "           UMTS    : 0x2.\n"
        "           CDMA    : 0x4.\n"
        "           TDSCDMA : 0x8.\n"
        "           LTE     : 0x10.\n"
        "\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "band <phone> <rat|status> [<band_bitmask>]\n"
        "       Set band preferences.\n"
        "       phone        : '1' or '2'.\n"
        "       Set '2G+3G' or 'LTE' band preferences, or show band capabilities and preferences "
        "with'status'.\n"
        "       band_bitmask : band bitmask, required with '2G+3G' or 'LTE' option\n"
        "           2G+3G : refer to BandBitMask in api.\n"
        "           LTE   : 4 LTE band bit masks in 64 bit.\n"
        "\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "ims <phone> <mode> [<service>|<user_agent>]\n"
        "       phone      : '1' or '2'.\n"
        "       mode       : 'status' to show IMS status, 'enable' or 'disable' IMS service, 'user' to "
        "set user agent.\n"
        "       service    : 'registation', 'voip', 'vonr', 'rtt' or 'sms'.\n"
        "       user_agent : user agent string.\n"
        "\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- handler <time>\n"
        "       Handler for network changes, can test with 'cm radio' configurations.\n"
        "       time : monitor time in seconds.\n"
        "\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- cellularCapability <phone>\n"
        "       To show hardware capabilities related to SIM and hardware RAT.\n"
        "\n"
    );

    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function checks the number of input parameters, if it is less than argNum, then it prints
 * the help menu.
 */
//--------------------------------------------------------------------------------------------------
void CheckArgs
(
    uint8_t argNum ///< [IN] The number of arguments.
)
{
    if (le_arg_NumArgs() < argNum)
    {
        PrintHelpMenu();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print network registration state.
 */
//--------------------------------------------------------------------------------------------------
void PrintNetRegState
(
    uint8_t phoneId,              ///< [IN] Phone ID.
    char* message,                ///< [IN] Addtional message to be printed.
    taf_radio_NetRegState_t state ///< [IN] Network registration enum.
)
{
    switch (state)
    {
        case TAF_RADIO_NET_REG_STATE_NONE:
            LE_INFO("Phone %d %s : Not registered.", phoneId, message);
            break;
        case TAF_RADIO_NET_REG_STATE_SEARCHING:
            LE_INFO("Phone %d %s : Searching network.", phoneId, message);
            break;
        case TAF_RADIO_NET_REG_STATE_HOME:
            LE_INFO("Phone %d %s : Home network.", phoneId, message);
            break;
        case TAF_RADIO_NET_REG_STATE_ROAMING:
            LE_INFO("Phone %d %s : Roaming network.", phoneId, message);
            break;
        case TAF_RADIO_NET_REG_STATE_DENIED:
            LE_INFO("Phone %d %s : Registration denied.", phoneId, message);
            break;
        case TAF_RADIO_NET_REG_STATE_NONE_AND_EMERGENCY_AVAILABLE:
            LE_INFO("Phone %d %s : Not registered and emergency available.", phoneId, message);
            break;
        case TAF_RADIO_NET_REG_STATE_SEARCHING_AND_EMERGENCY_AVAILABLE:
            LE_INFO("Phone %d %s : Searching and emergency available.", phoneId, message);
            break;
        case TAF_RADIO_NET_REG_STATE_DENIED_AND_EMERGENCY_AVAILABLE:
            LE_INFO("Phone %d %s : Denied and emergency available.", phoneId, message);
            break;
        case TAF_RADIO_NET_REG_STATE_UNKNOWN_AND_EMERGENCY_AVAILABLE:
            LE_INFO("Phone %d %s : Unknown registation state and emergency available.",
                phoneId, message);
            break;
        default:
            LE_INFO("Phone %d %s : Unknown.", phoneId, message);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print Radio Access Technology.
 */
//--------------------------------------------------------------------------------------------------
void PrintRAT
(
    taf_radio_Rat_t rat ///< [IN] Radio Access Technology enum.
)
{
    switch (rat)
    {
        case TAF_RADIO_RAT_GSM:
            LE_INFO("RAT : GSM");
            break;
        case TAF_RADIO_RAT_UMTS:
            LE_INFO("RAT : UMTS");
            break;
        case TAF_RADIO_RAT_CDMA:
            LE_INFO("RAT : CDMA");
            break;
        case TAF_RADIO_RAT_TDSCDMA:
            LE_INFO("RAT : TDSCDMA");
            break;
        case TAF_RADIO_RAT_LTE:
            LE_INFO("RAT : LTE");
            break;
        case TAF_RADIO_RAT_NR5G:
            LE_INFO("RAT : 5G SA");
            break;
        default:
            LE_INFO("RAT : Unknown");
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print NR icon type.
 */
//--------------------------------------------------------------------------------------------------
void PrintNrIcon
(
    taf_radio_NrIconType_t icon ///< [IN] NR icon type enum.
)
{
    switch (icon)
    {
        case TAF_RADIO_NR_ICON_5G:
            LE_INFO("NR icon type: 5G.");
            break;
        default:
            LE_INFO("NR icon type: Uknown.");
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print RAT service status.
 */
//--------------------------------------------------------------------------------------------------
void PrintRatSvcStatus
(
    taf_radio_RatSvcStatus_t status ///< [IN] RAT service status enum.
)
{
    switch (status)
    {
        case TAF_RADIO_RAT_SVC_STATUS_NO_SERVICE:
            LE_INFO("RAT service state : No Service.");
            break;
        case TAF_RADIO_RAT_SVC_STATUS_LIMITED:
            LE_INFO("RAT service state : Limited Service.");
            break;
        case TAF_RADIO_RAT_SVC_STATUS_SERVICE:
            LE_INFO("RAT service state : Full Service.");
            break;
        case TAF_RADIO_RAT_SVC_STATUS_LIMITED_REGIONAL:
            LE_INFO("RAT service state : Limited Regional Service.");
            break;
        case TAF_RADIO_RAT_SVC_STATUS_POWER_SAVE:
            LE_INFO("RAT service state : Power Save.");
            break;
        default:
            LE_INFO("RAT service state : Unknown.");
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print service domain.
 */
//--------------------------------------------------------------------------------------------------
void PrintSrvDomain
(
    taf_radio_ServiceDomainState_t domain ///< [IN] Service domomain enum.
)
{
    switch (domain)
    {
        case TAF_RADIO_SERVICE_DOMAIN_STATE_NO_SVC:
            LE_INFO("Domain : No Service");
            break;
        case TAF_RADIO_SERVICE_DOMAIN_STATE_CAMPED:
            LE_INFO("Domain : Camped");
            break;
        case TAF_RADIO_SERVICE_DOMAIN_STATE_CS_ONLY:
            LE_INFO("Domain : CS Only");
            break;
        case TAF_RADIO_SERVICE_DOMAIN_STATE_PS_ONLY:
            LE_INFO("Domain : PS Only");
            break;
        case TAF_RADIO_SERVICE_DOMAIN_STATE_CS_AND_PS:
            LE_INFO("Domain : CS and PS");
            break;
        default:
            LE_INFO("Domain : Unknown");
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print CS capability.
 */
//--------------------------------------------------------------------------------------------------
void PrintCsCap
(
    taf_radio_CsCap_t capability ///< [IN] CS capability.
)
{
    switch (capability)
    {
        case TAF_RADIO_CS_CAP_FULL_SERVICE:
            LE_INFO("CS Capability : Full service in CS domain is available.");
            break;
        case TAF_RADIO_CS_CAP_CSFB_NOT_PREFERRED:
            LE_INFO("CS Capability : CSFB is not preferred.");
            break;
        case TAF_RADIO_CS_CAP_SMS_ONLY:
            LE_INFO("CS Capability : CS registation is for SMS only.");
            break;
        case TAF_RADIO_CS_CAP_LIMITED:
            LE_INFO("CS Capability : CS registation failed for max attach or TAU attempts.");
            break;
        case TAF_RADIO_CS_CAP_BARRED:
            LE_INFO("CS Capability : CS domain is not available.");
            break;
        default:
            LE_INFO("CS Capability : Unknown");
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print Radio Access Technology preferences.
 */
//--------------------------------------------------------------------------------------------------
void PrintRatBitMask
(
    taf_radio_RatBitMask_t ratBitMask ///< [IN] Radio Access Technology bitmask.
)
{
    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_ALL)
    {
        LE_INFO("RAT : ALL.");
        return;
    }

    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_GSM)
    {
        LE_INFO("GSM");
    }

    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_UMTS)
    {
        LE_INFO("UMTS");
    }

    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_CDMA)
    {
        LE_INFO("CDMA");
    }

    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_TDSCDMA)
    {
        LE_INFO("TD-SCDMA");
    }

    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_LTE)
    {
        LE_INFO("LTE");
    }

    if (ratBitMask & TAF_RADIO_RAT_BIT_MASK_NR5G)
    {
        LE_INFO("NR5G");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function prints Mobile Country Code and Mobile Network Code of each preferred operator.
 */
//--------------------------------------------------------------------------------------------------
void PrintPrefOpList
(
    uint8_t phoneId ///< [IN] Phone ID.
)
{
    le_result_t result;
    taf_radio_PreferredOperatorListRef_t listRef = taf_radio_GetPreferredOperatorsList(phoneId);

    if (listRef)
    {
        taf_radio_PreferredOperatorRef_t opRef = taf_radio_GetFirstPreferredOperator(listRef);
        LE_TEST_OK(opRef != NULL, "taf_radio_GetFirstPreferredOperator - OK");

        uint32_t i = 0;
        char mccStr[TAF_RADIO_MCC_BYTES] = {0};
        char mncStr[TAF_RADIO_MNC_BYTES] = {0};
        taf_radio_RatBitMask_t ratMask;

        LE_INFO("Phone %d preferred operators list :", phoneId);

        while (opRef)
        {
            result = taf_radio_GetPreferredOperatorDetails(opRef, mccStr, TAF_RADIO_MCC_BYTES, mncStr,
                TAF_RADIO_MNC_BYTES, &ratMask);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetPreferredOperatorDetails - OK");

            LE_INFO("Operator %d :", i);
            LE_INFO("MCC : %s.", mccStr);
            LE_INFO("MNC : %s.", mncStr);
            PrintRatBitMask(ratMask);
            i++;

            opRef = taf_radio_GetNextPreferredOperator(listRef);
        }

        taf_radio_DeletePreferredOperatorsList(listRef);
        LE_TEST_OK(true, "taf_radio_DeletePreferredOperatorsList - OK");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function traverses network scan information linked list, and prints network name, Mobile
 * Country Code, Mobile Network Code and shows network status, to find if it is currently serving or
 * available, home or roaming network, and whether it is forbidden or preferred network.
 */
//--------------------------------------------------------------------------------------------------
void PrintScanInfoList
(
    taf_radio_ScanInformationListRef_t listRef ///< [IN] Network scan information list reference.
)
{

    if (listRef == NULL)
    {
        return;
    }

    taf_radio_ScanInformationRef_t infoRef = taf_radio_GetFirstCellularNetworkScan(listRef);
    LE_TEST_OK(infoRef != NULL, "taf_radio_GetFirstCellularNetworkScan - OK");

    uint32_t i = 1;
    char name[TAF_RADIO_NETWORK_NAME_MAX_LEN] = {0};
    char mccStr[TAF_RADIO_MCC_BYTES] = {0};
    char mncStr[TAF_RADIO_MNC_BYTES] = {0};
    taf_radio_Rat_t rat;
    le_result_t result;

    bool inUse = false;
    bool available = false;
    bool fobbiden = false;
    bool home = false;

    while (infoRef)
    {
        result = taf_radio_GetCellularNetworkName(infoRef, name, TAF_RADIO_NETWORK_NAME_MAX_LEN);
        LE_TEST_OK(result == LE_OK, "taf_radio_GetCellularNetworkName - OK");

        rat = taf_radio_GetCellularNetworkRat(infoRef);
        LE_TEST_OK(true, "taf_radio_GetCellularNetworkRat - OK");

        result = taf_radio_GetCellularNetworkMccMnc(infoRef, mccStr, TAF_RADIO_MCC_BYTES, mncStr,
            TAF_RADIO_MNC_BYTES);
        LE_TEST_OK(result == LE_OK, "taf_radio_GetCellularNetworkMccMnc - OK");

        inUse = taf_radio_IsCellularNetworkInUse(infoRef);
        LE_TEST_OK(true, "taf_radio_IsCellularNetworkInUse - OK");

        available = taf_radio_IsCellularNetworkAvailable(infoRef);
        LE_TEST_OK(true, "taf_radio_IsCellularNetworkAvailable - OK");

        fobbiden = taf_radio_IsCellularNetworkForbidden(infoRef);
        LE_TEST_OK(true, "taf_radio_IsCellularNetworkForbidden - OK");

        home = taf_radio_IsCellularNetworkHome(infoRef);
        LE_TEST_OK(true, "taf_radio_IsCellularNetworkHome - OK");

        if (inUse)
        {
            LE_INFO( "[In use]");
        }
        if (available)
        {
            LE_INFO("[Available]");
        }
        if (fobbiden)
        {
            LE_INFO("[Forbbien]");
        }
        if (home)
        {
            LE_INFO("[Home]");
        }

        LE_INFO("Network %d", i);
        LE_INFO("Name : %s.", name);
        PrintRAT(rat);
        LE_INFO("MCC : %s.", mccStr);
        LE_INFO("MNC : %s.", mncStr);

        i++;

        infoRef = taf_radio_GetNextCellularNetworkScan(listRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function traverses Physical Cell ID scan information linked list, and prints Cell ID, Global
 * Cell ID and Mobile Country Code and Mobile Network Code of each Public Land Mobile Network.
 */
//--------------------------------------------------------------------------------------------------
void PrintPciScanInfoList
(
    taf_radio_PciScanInformationListRef_t listRef ///< [IN] PCI scan information list reference.
)
{

    if (listRef == NULL)
    {
        return;
    }

    taf_radio_PciScanInformationRef_t infoRef = taf_radio_GetFirstPciScanInfo(listRef);
    LE_TEST_OK(infoRef != NULL, "taf_radio_GetFirstPciScanInfo - OK");

    uint32_t i = 1;
    uint32_t j;
    uint16_t cell_id = 0;
    uint32_t global_cell_id = 0;
    char mccStr[TAF_RADIO_MCC_BYTES] = {0};
    char mncStr[TAF_RADIO_MNC_BYTES] = {0};
    le_result_t result;
    taf_radio_PlmnInformationRef_t plmnRef;

    while (infoRef)
    {
        cell_id = taf_radio_GetPciScanCellId(infoRef);
        LE_TEST_OK(true, "taf_radio_GetPciScanCellId - OK");

        global_cell_id = taf_radio_GetPciScanGlobalCellId(infoRef);
        LE_TEST_OK(true, "taf_radio_GetPciScanGlobalCellId - OK");

        LE_INFO("PCI network %d", i);
        LE_INFO("Cell ID : %d", cell_id);
        LE_INFO("Globol Cell ID : %d", global_cell_id);

        j = 1;
        plmnRef = taf_radio_GetFirstPlmnInfo(infoRef);
        LE_TEST_OK(plmnRef != NULL, "taf_radio_GetFirstPlmnInfo - OK");
        while (plmnRef)
        {
            result = taf_radio_GetPciScanMccMnc(plmnRef, mccStr, TAF_RADIO_MCC_BYTES, mncStr,
                TAF_RADIO_MNC_BYTES);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetPciScanMccMnc - OK");

            LE_INFO("PLMN %d", j);
            LE_INFO("MCC : %s.", mccStr);
            LE_INFO("MNC : %s.", mncStr);

            j++;
            plmnRef = taf_radio_GetNextPlmnInfo(infoRef);
        }

        i++;
        infoRef = taf_radio_GetNextPciScanInfo(listRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function traverses neighboring cell information linked list, and prints Radio Access
 * Technology, Cell ID, Location Area Code, Signal Strength, Base Station Identity Code, Ec/Io and
 * Physical Cell ID.
 */
//--------------------------------------------------------------------------------------------------
void PrintNgbrCellsInfo
(
    uint8_t phoneId ///< [IN] Phone ID.
)
{
    taf_radio_NeighborCellsRef_t ngbrCellsRef = taf_radio_GetNeighborCellsInfo(phoneId);
    LE_TEST_OK(ngbrCellsRef != NULL, "taf_radio_GetNeighborCellsInfo - OK");

    if (ngbrCellsRef)
    {
        taf_radio_CellInfoRef_t cellInfoRef = taf_radio_GetFirstNeighborCellInfo(ngbrCellsRef);

        uint32_t i = 0;
        uint64_t cid;
        uint32_t lac;
        int32_t rxlevel;
        uint8_t bsic;
        uint16_t pcid;
        uint32_t nrpcid;
        taf_radio_Rat_t rat;
        le_result_t result;

        LE_INFO("Phone %d neighboring cells :", phoneId);

        while (cellInfoRef)
        {
            rat = taf_radio_GetNeighborCellRat(cellInfoRef);
            LE_TEST_OK(true, "taf_radio_GetNeighborCellRat - OK");

            LE_INFO("Neighbor cell %d :", i);
            PrintRAT(rat);

            switch (rat)
            {
                case TAF_RADIO_RAT_GSM:
                    cid = taf_radio_GetNeighborCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellId - OK");
                    lac = taf_radio_GetNeighborCellLocAreaCode(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellId - OK");
                    rxlevel = taf_radio_GetNeighborCellRxLevel(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellRxLevel - OK");
                    result = taf_radio_GetNeighborCellGsmBsic(cellInfoRef, &bsic);
                    LE_TEST_OK(result == LE_OK, "taf_radio_GetNeighborCellGsmBsic - OK");
                    LE_INFO("Cell ID                    : %" PRIuS, (size_t)cid);
                    LE_INFO("Local Area Code            : %d", lac);
                    LE_INFO("Signal Strength            : %d", rxlevel);
                    LE_INFO("Base Station Identity Code : %d", bsic);
                    break;
                case TAF_RADIO_RAT_UMTS:
                    cid = taf_radio_GetNeighborCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellId - OK");
                    rxlevel = taf_radio_GetNeighborCellRxLevel(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellRxLevel - OK");
                    LE_INFO("Cell ID         : %" PRIuS, (size_t)cid);
                    LE_INFO("Signal Strength : %d", rxlevel);
                    break;
                case TAF_RADIO_RAT_CDMA:
                    rxlevel = taf_radio_GetNeighborCellRxLevel(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellRxLevel - OK");
                    LE_INFO("Signal Strength : %d", rxlevel);
                    break;
                case TAF_RADIO_RAT_TDSCDMA:
                    cid = taf_radio_GetNeighborCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellId - OK");
                    rxlevel = taf_radio_GetNeighborCellRxLevel(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellRxLevel - OK");
                    LE_INFO("Cell ID         : %" PRIuS, (size_t)cid);
                    LE_INFO("Signal Strength : %d", rxlevel);
                    break;
                case TAF_RADIO_RAT_NR5G:
                    cid = taf_radio_GetNeighborCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellId - OK");
                    nrpcid = taf_radio_GetPhysicalNeighborNrCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetPhysicalNeighborNrCellId - OK");
                    rxlevel = taf_radio_GetNeighborCellRxLevel(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellRxLevel - OK");
                    LE_INFO("Cell ID          : %" PRIuS, (size_t)cid);
                    LE_INFO("Physical Cell ID : %d", nrpcid);
                    LE_INFO("Signal Strength  : %d", rxlevel);
                    break;
                case TAF_RADIO_RAT_LTE:
                    cid = taf_radio_GetNeighborCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellId - OK");
                    pcid = taf_radio_GetPhysicalNeighborLteCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetPhysicalNeighborLteCellId - OK");
                    rxlevel = taf_radio_GetNeighborCellRxLevel(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellRxLevel - OK");
                    LE_INFO("Cell ID          : %" PRIuS, (size_t)cid);
                    LE_INFO("Physical Cell ID : %d", pcid);
                    LE_INFO("Signal Strength  : %d", rxlevel);
                    break;
                default:
                    break;
            }

            i++;

            cellInfoRef = taf_radio_GetNextNeighborCellInfo(ngbrCellsRef);
            LE_TEST_OK(true, "taf_radio_GetNextNeighborCellInfo - OK");
        }

        result = taf_radio_DeleteNeighborCellsInfo(ngbrCellsRef);
        LE_TEST_OK(result == LE_OK, "taf_radio_DeleteNeighborCellsInfo - OK");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print band preferences and capabilities.
 */
//--------------------------------------------------------------------------------------------------
void PrintBandStatus
(
    uint8_t phoneId ///< [IN] Phone ID.
)
{
    le_result_t result;
    taf_radio_BandBitMask_t bandMask = 0x0;
    uint64_t lteBandMask = 0;
    uint64_t lteBand[TAF_RADIO_LTE_BAND_GROUP_NUM] = {0};
    uint8_t i;
    uint8_t j;
    size_t lteBandSize = 0;

    result = taf_radio_GetBandPreferences(&bandMask, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetBandPreferences - OK");
    LE_INFO("Phone %d 2G/3G band preferences 0x%" PRIxS, phoneId, (size_t)bandMask);

    result = taf_radio_GetBandCapabilities(&bandMask, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetBandCapabilities - OK");
    LE_INFO("Phone %d 2G/3G band capabilities 0x%" PRIxS, phoneId, (size_t)bandMask);

    result = taf_radio_GetLteBandPreferences(lteBand, &lteBandSize, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetLteBandPreferences - OK");
    for (i = 0; i < lteBandSize; i++)
    {
        lteBandMask = lteBand[i];

        for (j = 0; j < 64; j++)
        {
            if (lteBandMask & 0x1)
            {
                LE_INFO("Phone %d LTE band preferences (band %d)", phoneId, i * 64 + j + 1);
            }
            lteBandMask = lteBandMask >> 1;
        }
    }

    result = taf_radio_GetLteBandCapabilities(lteBand, &lteBandSize, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetLteBandCapabilities - OK");
    for (i = 0; i < lteBandSize; i++)
    {
        lteBandMask = lteBand[i];

        for (j = 0; j < 64; j++)
        {
            if (lteBandMask & 0x1)
            {
                LE_INFO("Phone %d LTE band capabilities (band %d)", phoneId, i * 64 + j + 1);
            }
            lteBandMask = lteBandMask >> 1;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print IMS registration state.
 */
//--------------------------------------------------------------------------------------------------
void PrintImsRegState
(
    uint8_t phoneId,               ///< [IN] Phone ID.
    taf_radio_ImsRegStatus_t state ///< [IN] IMS registration state.
)
{
    switch (state)
    {
        case TAF_RADIO_IMS_REG_STATUS_REGISTERED:
            LE_INFO("Phone %d IMS : Registered.", phoneId);
            break;
        case TAF_RADIO_IMS_REG_STATUS_NOT_REGISTERED:
            LE_INFO("Phone %d IMS : Not registered.", phoneId);
            break;
        default:
            LE_INFO("Phone %d IMS : Unknown.", phoneId);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print IMS service information.
 */
//--------------------------------------------------------------------------------------------------
void PrintImsSvcInfo
(
    uint8_t phoneId,                ///< [IN] Phone ID.
    taf_radio_ImsSvcType_t service, ///< [IN] IMS service.
    taf_radio_ImsSvcStatus_t status ///< [IN] IMS service status.
)
{
    switch (status)
    {
        case TAF_RADIO_IMS_SVC_STATUS_UNAVAILABLE:
            if (service == TAF_RADIO_IMS_SVC_TYPE_SMS)
            {
                LE_INFO("Phone %d IMS-SMS : Unavailble.", phoneId);
            }
            else if (service == TAF_RADIO_IMS_SVC_TYPE_VOIP)
            {
                LE_INFO("Phone %d IMS-Voice over IMS : Unavailble.", phoneId);
            }
            break;
        case TAF_RADIO_IMS_SVC_STATUS_LIMITED:
            if (service == TAF_RADIO_IMS_SVC_TYPE_SMS)
            {
                LE_INFO("Phone %d IMS-SMS : Limited service.", phoneId);
            }
            else if (service == TAF_RADIO_IMS_SVC_TYPE_VOIP)
            {
                LE_INFO("Phone %d IMS-Voice over IMS : Limited service.", phoneId);
            }
            break;
        case TAF_RADIO_IMS_SVC_STATUS_FULL_SERVICE:
            if (service == TAF_RADIO_IMS_SVC_TYPE_SMS)
            {
                LE_INFO("Phone %d IMS-SMS : Full service.", phoneId);
            }
            else if (service == TAF_RADIO_IMS_SVC_TYPE_VOIP)
            {
                LE_INFO("Phone %d IMS-Voice over IMS : Full service.", phoneId);
            }
            break;
        default:
            if (service == TAF_RADIO_IMS_SVC_TYPE_SMS)
            {
                LE_INFO("Phone %d IMS-SMS : Unknown.", phoneId);
            }
            else if (service == TAF_RADIO_IMS_SVC_TYPE_VOIP)
            {
                LE_INFO("Phone %d IMS-Voice over IMS : Unknown.", phoneId);
            }
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print IMS PDP error.
 */
//--------------------------------------------------------------------------------------------------
void PrintImsPdpError
(
    uint8_t phoneId,           ///< [IN] Phone ID.
    taf_radio_PdpError_t error ///< [IN] IMS PDP error.
)
{
    switch (error)
    {
        case TAF_RADIO_PDP_ERROR_GENERIC:
            LE_INFO("Phone %d PDP error : Generic failure reason.", phoneId);
            break;
        case TAF_RADIO_PDP_ERROR_OPTION_UNSUBSCRIBED:
            LE_INFO("Phone %d PDP error : Option is unsubscribed.", phoneId);
            break;
        case TAF_RADIO_PDP_ERROR_UNKNOWN_PDP:
            LE_INFO("Phone %d PDP error : PDP was unknown.", phoneId);
            break;
        case TAF_RADIO_PDP_ERROR_REASON_NOT_SPECIFIED:
            LE_INFO("Phone %d PDP error : Reason not specified.", phoneId);
            break;
        case TAF_RADIO_PDP_ERROR_CONNECTION_BRINGUP_FAILURE:
            LE_INFO("Phone %d PDP error : Connection bring-up failure.", phoneId);
            break;
        case TAF_RADIO_PDP_ERROR_CONNECTION_IKE_AUTH_FAILURE:
            LE_INFO("Phone %d PDP error : IKE authentication failure.", phoneId);
            break;
        case TAF_RADIO_PDP_ERROR_USER_AUTH_FAILURE:
            LE_INFO("Phone %d PDP error : User authentication failure.", phoneId);
            break;
        default:
            LE_INFO("Phone %d PDP error : Unknown error.", phoneId);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print operating mode.
 */
//--------------------------------------------------------------------------------------------------
void PrintOperatingMode
(
    taf_radio_OpMode_t mode ///< [IN] Operating mode.
)
{
    switch (mode)
    {
        case TAF_RADIO_OP_MODE_ONLINE:
            LE_INFO("Operating Mode : Online.");
            break;
        case TAF_RADIO_OP_MODE_AIRPLANE:
            LE_INFO("Operating Mode : Airplane.");
            break;
        case TAF_RADIO_OP_MODE_FACTORY_TEST:
            LE_INFO("Operating Mode : Factory Test.");
            break;
        case TAF_RADIO_OP_MODE_OFFLINE:
            LE_INFO("Operating Mode : Offline.");
            break;
        case TAF_RADIO_OP_MODE_RESETTING:
            LE_INFO("Operating Mode : Resetting.");
            break;
        case TAF_RADIO_OP_MODE_SHUTTING_DOWN:
            LE_INFO("Operating Mode : Shutdown.");
            break;
        case TAF_RADIO_OP_MODE_PERSISTENT_LOW_POWER:
            LE_INFO("Operating Mode : Persistent Low Power.");
            break;
        default:
            LE_INFO("Operating Mode : Unknown.");
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print RF bandwidth.
 */
//--------------------------------------------------------------------------------------------------
void PrintRFBandwidth
(
    uint8_t phoneId,                  ///< [IN] Phone ID.
    taf_radio_RFBandWidth_t bandwidth ///< [IN] RF bandwidth.
)
{
    switch (bandwidth)
    {
        case TAF_RADIO_RF_BANDWIDTH_GSM_BW_0_2:
            LE_INFO("Phone %d RF Bandwidth : GSM 0.2 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_WCDMA_BW_5:
            LE_INFO("Phone %d RF Bandwidth : WCDMA 5 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_WCDMA_BW_10:
            LE_INFO("Phone %d RF Bandwidth : WCDMA 10 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_TDSCDMA_BW_1_6:
            LE_INFO("Phone %d RF Bandwidth : TDSCDMA 1.6 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_LTE_BW_1_4:
            LE_INFO("Phone %d RF Bandwidth : LTE 1.4 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_LTE_BW_3:
            LE_INFO("Phone %d RF Bandwidth : LTE 3 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_LTE_BW_5:
            LE_INFO("Phone %d RF Bandwidth : LTE 5 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_LTE_BW_10:
            LE_INFO("Phone %d RF Bandwidth : LTE 10 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_LTE_BW_15:
            LE_INFO("Phone %d RF Bandwidth : LTE 15 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_LTE_BW_20:
            LE_INFO("Phone %d RF Bandwidth : LTE 20 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_5:
            LE_INFO("Phone %d RF Bandwidth : NR5G 5 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_10:
            LE_INFO("Phone %d RF Bandwidth : NR5G 10 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_15:
            LE_INFO("Phone %d RF Bandwidth : NR5G 15 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_20:
            LE_INFO("Phone %d RF Bandwidth : NR5G 20 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_25:
            LE_INFO("Phone %d RF Bandwidth : NR5G 25 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_30:
            LE_INFO("Phone %d RF Bandwidth : NR5G 30 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_40:
            LE_INFO("Phone %d RF Bandwidth : NR5G 40 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_50:
            LE_INFO("Phone %d RF Bandwidth : NR5G 50 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_60:
            LE_INFO("Phone %d RF Bandwidth : NR5G 60 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_70:
            LE_INFO("Phone %d RF Bandwidth : NR5G 70 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_80:
            LE_INFO("Phone %d RF Bandwidth : NR5G 80 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_90:
            LE_INFO("Phone %d RF Bandwidth : NR5G 90 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_100:
            LE_INFO("Phone %d RF Bandwidth : NR5G 100 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_200:
            LE_INFO("Phone %d RF Bandwidth : NR5G 200 MHz.", phoneId);
            break;
        case TAF_RADIO_RF_BANDWIDTH_NR5G_BW_400:
            LE_INFO("Phone %d RF Bandwidth : NR5G 400 MHz.", phoneId);
            break;
        default:
            LE_INFO("Phone %d RF Bandwidth : Invalid.", phoneId);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Configurations on GSM signal indication.
 */
//--------------------------------------------------------------------------------------------------
void GsmSignalConfiguration
(
    long phoneId,  ///< [IN] Phone ID.
    long rssiDelta, ///< [IN] RSSI delta.
    long hysteresisDb
)
{
    le_result_t result = taf_radio_SetSignalStrengthIndHysteresisTimer(5000,phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndHysteresisTimer - OK");

    result = taf_radio_SetSignalStrengthIndHysteresis(TAF_RADIO_SIG_TYPE_GSM_RSSI,hysteresisDb,
                                                      phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndHysteresis - OK");

    result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_GSM_RSSI,
        -1110, -510, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");
    }


    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_GSM_RSSI, rssiDelta, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Configurations on UMTS signal indication.
 */
//--------------------------------------------------------------------------------------------------
void UmtsSignalConfiguration
(
    long phoneId,   ///< [IN] Phone ID.
    long rssiDelta, ///< [IN] RSSI delta.
    long hysteresisDb
)
{
    le_result_t result = taf_radio_SetSignalStrengthIndHysteresisTimer(5000,phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndHysteresisTimer - OK");

    result = taf_radio_SetSignalStrengthIndHysteresis(TAF_RADIO_SIG_TYPE_UMTS_RSSI,hysteresisDb,
                                                     phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndHysteresis - OK");

    result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_UMTS_RSSI,
        -1130, -510, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");
    }

    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_UMTS_RSSI, rssiDelta, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Configurations on LTE signal indication.
 */
//--------------------------------------------------------------------------------------------------
void LteSignalConfiguration
(
    long phoneId,    ///< [IN] Phone ID.
    long rsrpDelta,  ///< [IN] RSRP delta.
    long hysteresisDb
)
{
    le_result_t result = taf_radio_SetSignalStrengthIndHysteresisTimer(5000,phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndHysteresisTimer - OK");

    result = taf_radio_SetSignalStrengthIndHysteresis(TAF_RADIO_SIG_TYPE_LTE_RSRP,hysteresisDb,
                                                      phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndHysteresis - OK");

    result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_LTE_RSRP,
        -1400, -440, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");
    }

    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_LTE_RSRP, rsrpDelta, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Configurations on NR5G signal indication.
 */
//--------------------------------------------------------------------------------------------------
void Nr5gSignalConfiguration
(
    long phoneId,   ///< [IN] Phone ID.
    long rsrpDelta, ///< [IN] RSRP delta.
    long hysteresisDb
)
{
    le_result_t result = taf_radio_SetSignalStrengthIndHysteresisTimer(5000,phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndHysteresisTimer - OK");

    result = taf_radio_SetSignalStrengthIndHysteresis(TAF_RADIO_SIG_TYPE_NR5G_RSRP,hysteresisDb,
                                                      phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndHysteresis - OK");

    result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_NR5G_RSRP,
        -1400, -440, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");
    }

    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_NR5G_RSRP, rsrpDelta, phoneId);
    if (result != LE_UNSUPPORTED)
    {
        LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for network registration state.
 */
//--------------------------------------------------------------------------------------------------
void NetRegStateHandler
(
    taf_radio_NetRegStateInd_t* netRegStateIndPtr, ///< [IN] Network registation state indication.
    void* contextPtr                               ///< [IN] Handler context.
)
{
    PrintNetRegState(netRegStateIndPtr->phoneId, "network regristration state changed to",
        netRegStateIndPtr->state);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for Packet switch state.
 */
//--------------------------------------------------------------------------------------------------
void PackSwStateHandler
(
    taf_radio_NetRegStateInd_t* packSwStateIndPtr, ///< [IN] Packet switched state indication.
    void* contextPtr                               ///< [IN] Handler context.
)
{
    PrintNetRegState(packSwStateIndPtr->phoneId, "packet switch state changed to",
        packSwStateIndPtr->state);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for network registration rejection.
 */
//--------------------------------------------------------------------------------------------------
void NetRegRejectHandler
(
    taf_radio_NetRegRejInd_t* netRegRejIndPtr, ///< [IN] Indication on network rejection.
    void* contextPtr                           ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d network registration rejection.", netRegRejIndPtr->phoneId);
    PrintRAT(netRegRejIndPtr->rat);
    PrintSrvDomain(netRegRejIndPtr->domain);
    LE_INFO("MCC : %s , MNC : %s", netRegRejIndPtr->mcc, netRegRejIndPtr->mnc);
    LE_INFO("Rejection cause : %d", netRegRejIndPtr->cause);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for Radio Access Technology change.
 */
//--------------------------------------------------------------------------------------------------
void RatChangeHandler
(
    taf_radio_RatChangeInd_t* ratChangeIndPtr, ///< [IN] Indication on RAT change.
    void* contextPtr                           ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d Radio Access Technology change.", ratChangeIndPtr->phoneId);
    PrintRAT(ratChangeIndPtr->rat);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for NR icon type change.
 */
//--------------------------------------------------------------------------------------------------
void NrIconTypeHandler
(
    taf_radio_NrIconType_t type, ///< [IN] Nr icon type.
    uint8_t phoneId,             ///< [IN] Phone ID.
    void* contextPtr             ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d NR icon type change.", phoneId);
    PrintNrIcon(type);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for GSM signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void GsmSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d GSM rssi : %d dBm", phoneId, ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for CellInfo change status.
 */
//--------------------------------------------------------------------------------------------------
void CellInfoChangeHandler
(
    taf_radio_CellInfoStatus_t cellStatus,
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d CellInfo status : %d", phoneId, cellStatus);

    switch (cellStatus)
    {
        case TAF_RADIO_CELL_SERVING_CHANGED:
            LE_INFO("Serving cell changed.");
            break;
        case TAF_RADIO_CELL_NEIGHBOR_CHANGED:
            LE_INFO("Neighbor cell changed.");
            break;
        case TAF_RADIO_CELL_SERVING_AND_NEIGHBOR_CHANGED:
            LE_INFO("Serving and neighbor changed.");
            break;
        default:
            LE_INFO("CellInfo : Unknown");
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for UMTS signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void UmtsSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d UMTS rssi : %d dBm", phoneId, ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for CDMA signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void CdmaSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d CDMA rssi : %d dBm", phoneId, ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for TDSCDMA signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void TdscdmaSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d TDSCDMA rssi : %d dBm", phoneId, ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for LTE signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void LteSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d LTE rssi : %d dBm", phoneId, ss);
    LE_INFO("Phone %d LTE rsrp : %d dB", phoneId, rsrp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for NR5G signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void Nr5gSsChangeHandler
(
    int32_t ss,      ///< [IN] Signal strength in dBm.
    int32_t rsrp,    ///< [IN] Reference signal receive quality in dB.
    uint8_t phoneId, ///< [IN] Phone ID.
    void* contextPtr ///< [IN] Handler context.
)
{
    LE_INFO("Phone %d NR5G rssi : %d dBm", phoneId, ss);
    LE_INFO("Phone %d NR5G rsrp : %d dB", phoneId, rsrp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for network registration state.
 */
//--------------------------------------------------------------------------------------------------
void ImsRegStateHandler
(
    taf_radio_ImsRegStatus_t status, ///< [IN] IMS registation state.
    uint8_t phoneId,                 ///< [IN] Phone ID.
    void* contextPtr                 ///< [IN] Handler context.
)
{
    PrintImsRegState(phoneId, status);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for oeprating mode chanegs.
 */
//--------------------------------------------------------------------------------------------------
void OpModeChangeHandler
(
    taf_radio_OpMode_t mode, ///< [IN] Operating mode.
    void* contextPtr         ///< [IN] Handler context.
)
{
    PrintOperatingMode(mode);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for IMS status.
 */
//--------------------------------------------------------------------------------------------------
void ImsStatusHandler
(
    taf_radio_ImsRef_t imsRef,         ///< [IN] IMS reference.
    taf_radio_ImsIndBitMask_t bitmask, ///< [IN] Indication bitmask.
    uint8_t phoneId,                   ///< [IN] Phone ID.
    void* contextPtr                   ///< [IN] Handler context.
)
{
    le_result_t result = LE_OK;

    if (bitmask & TAF_RADIO_IMS_IND_BIT_MASK_SVC_INFO)
    {
        taf_radio_ImsSvcStatus_t svcStatus = TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;
        result = taf_radio_GetImsSvcStatus(imsRef, TAF_RADIO_IMS_SVC_TYPE_VOIP, &svcStatus);
        if (result != LE_UNSUPPORTED)
        {
            LE_TEST_OK(result == LE_OK, "taf_radio_GetImsSvcStatus - LE_OK");
            PrintImsSvcInfo(phoneId, TAF_RADIO_IMS_SVC_TYPE_VOIP, svcStatus);
        }

        svcStatus = TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;
        result = taf_radio_GetImsSvcStatus(imsRef, TAF_RADIO_IMS_SVC_TYPE_SMS, &svcStatus);
        if (result != LE_UNSUPPORTED)
        {
            LE_TEST_OK(result == LE_OK, "taf_radio_GetImsSvcStatus - LE_OK");
            PrintImsSvcInfo(phoneId, TAF_RADIO_IMS_SVC_TYPE_SMS, svcStatus);
        }
    }

    if (bitmask & TAF_RADIO_IMS_IND_BIT_MASK_PDP_ERROR)
    {
        taf_radio_PdpError_t pdpError = TAF_RADIO_PDP_ERROR_UNKNOWN;
        result = taf_radio_GetImsPdpError(imsRef, &pdpError);
        LE_TEST_OK(result == LE_OK, "taf_radio_GetImsPdpError - LE_OK");
        PrintImsPdpError(phoneId, pdpError);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for network status change.
 */
//--------------------------------------------------------------------------------------------------
void NetStatusChangeHandler
(
    taf_radio_NetStatusRef_t netStatusRef,   ///< [IN] Network status reference.
    taf_radio_NetStatusIndBitMask_t bitmask, ///< [IN] Network status indication bitmask.
    uint8_t phoneId,                         ///< [IN] Phone ID.
    void* contextPtr                         ///< [IN] Handler context.
)
{
    if (bitmask & TAF_RADIO_NET_STATUS_IND_BIT_MASK_RAT_SVC_STATUS)
    {
        taf_radio_RatSvcStatus_t status = TAF_RADIO_RAT_SVC_STATUS_UNKNOWN;
        le_result_t result = taf_radio_GetRatSvcStatus(netStatusRef, &status);
        LE_TEST_OK(result == LE_OK, "taf_radio_GetRatSvcStatus - OK");
        LE_INFO("Phone %d RAT service status changed.", phoneId);
        PrintRatSvcStatus(status);
    }

    if (bitmask & TAF_RADIO_NET_STATUS_IND_BIT_MASK_SVC_DOMAIN)
    {
        taf_radio_ServiceDomainState_t domain = TAF_RADIO_SERVICE_DOMAIN_STATE_UNKNOWN;
        le_result_t result = taf_radio_GetServiceDomain(&domain, phoneId);
        LE_TEST_OK(result == LE_OK, "taf_radio_GetServiceDomain - OK");
        LE_INFO("Phone %d service domain changed.", phoneId);
        PrintSrvDomain(domain);
    }

    if (bitmask & TAF_RADIO_NET_STATUS_IND_BIT_MASK_LTE_CS_CAP)
    {
        taf_radio_CsCap_t cap = TAF_RADIO_CS_CAP_UNKNOWN;
        le_result_t result = taf_radio_GetLteCsCap(netStatusRef, &cap);
        LE_TEST_OK(result == LE_OK, "taf_radio_GetLteCsCap - OK");
        LE_INFO("Phone %d LTE CS capability changed.", phoneId);
        PrintCsCap(cap);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function prints Radio Access Technology in use. For GSM network, it prints Cell ID, Location
 * Area Code and Base Station Identity Code. For UMTS networkm, it prints Primary Scrambling Code.
 * For LTE newtork, it prints Tracking Area Code, E-UTRA Absolute Radio Frequency Channel Number,
 * Timing Adcance and Physical Serving Cell ID.
 */
//--------------------------------------------------------------------------------------------------
void PrintServingStatus
(
    uint8_t phoneId ///< [IN] Phone ID.
)
{
    le_result_t result;
    char mccStr[TAF_RADIO_MCC_BYTES] = {0};
    char mncStr[TAF_RADIO_MNC_BYTES] = {0};
    char longOperatorStr[TAF_RADIO_NETWORK_NAME_MAX_LEN] = {0};
    char shortOperatorStr[TAF_RADIO_NETWORK_NAME_MAX_LEN] = {0};

    taf_radio_NREndcAvailability_t endcStatus;
    taf_radio_NRDcnrRestriction_t dcnrStatus;

    taf_radio_Rat_t rat;
    result = taf_radio_GetRadioAccessTechInUse(&rat, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetRadioAccessTechInUse - OK");

    uint32_t cellId;
    uint32_t lac;
    uint8_t bsic = 0;

    uint16_t psc;

    uint16_t tac;
    uint8_t rac;
    uint32_t earFcn;
    uint32_t ta;
    uint16_t pscid;

    uint64_t nrCid;
    int32_t arFcn;
    int32_t nrTac;
    uint32_t pcid;

    uint32_t band;
    taf_radio_BandBitMask_t bandMask;
    taf_radio_RFBandWidth_t bandWidth;

    switch (rat)
    {
        case TAF_RADIO_RAT_GSM:
            cellId = taf_radio_GetServingCellId(phoneId);
            LE_TEST_OK(true, "taf_radio_GetServingCellId - OK");
            LE_INFO("Phone %d GSM Cell ID %d", phoneId, cellId);

            lac = taf_radio_GetServingCellLocAreaCode(phoneId);
            LE_TEST_OK(true, "taf_radio_GetServingCellLocAreaCode - OK");
            LE_INFO("Phone %d GSM Location Area Code %d", phoneId, lac);

            result = taf_radio_GetServingCellGsmBsic(&bsic, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetServingCellGsmBsic - OK");
            LE_INFO("Phone %d GSM Base Station ID %d", phoneId, bsic);

            result = taf_radio_GetServingCellArfcn(&arFcn, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetServingCellArfcn - LE_OK");
            LE_INFO("Phone %d GSM Absolute Radio Frequency Channel Number %d", phoneId, arFcn);

            result = taf_radio_GetServingCellRoutingAreaCode(&rac, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetServingCellRoutingAreaCode - LE_OK");
            LE_INFO("Phone %d GSM Routing Area Code %d", phoneId, rac);

            result = taf_radio_GetServingCellBandInfo(&bandMask, &bandWidth, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetServingCellBandInfo - LE_OK");
            LE_INFO("Phone %d GSM band 0x%" PRIxS, phoneId, (size_t)bandMask);
            PrintRFBandwidth(phoneId, bandWidth);
            break;
        case TAF_RADIO_RAT_UMTS:
            psc = taf_radio_GetServingCellScramblingCode(phoneId);
            LE_TEST_OK(true, "taf_radio_GetServingCellScramblingCode - OK");
            LE_INFO("Phone %d UMTS Primary Scrambling Code %d", phoneId, psc);

            result = taf_radio_GetServingCellUarfcn(&arFcn, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetServingCellUarfcn - LE_OK");
            LE_INFO("Phone %d UMTS Absolute Radio Frequency Channel Number %d", phoneId, arFcn);

            result = taf_radio_GetServingCellRoutingAreaCode(&rac, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetServingCellRoutingAreaCode - LE_OK");
            LE_INFO("Phone %d UMTS Routing Area Code %d", phoneId, rac);

            result = taf_radio_GetServingCellBandInfo(&bandMask, &bandWidth, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetServingCellBandInfo - LE_OK");
            LE_INFO("Phone %d UMTS band 0x%" PRIxS, phoneId, (size_t)bandMask);
            PrintRFBandwidth(phoneId, bandWidth);
            break;
        case TAF_RADIO_RAT_LTE:
            tac = taf_radio_GetServingCellLteTracAreaCode(phoneId);
            LE_TEST_OK(true, "taf_radio_GetServingCellLteTracAreaCode - OK");
            LE_INFO("Phone %d LTE Tracking Area Code %d", phoneId, tac);

            earFcn = taf_radio_GetServingCellEarfcn(phoneId);
            LE_TEST_OK(true, "taf_radio_GetServingCellEarfcn - OK");
            LE_INFO("Phone %d LTE E-UTRA Absolute Radio Frequency Channel Number %d",
                phoneId, earFcn);

            ta = taf_radio_GetServingCellTimingAdvance(phoneId);
            LE_TEST_OK(true, "taf_radio_GetServingCellTimingAdvance - OK");
            LE_INFO("Phone %d LTE Timing Adcance %d", phoneId, ta);

            pscid = taf_radio_GetPhysicalServingLteCellId(phoneId);
            LE_TEST_OK(true, "taf_radio_GetPhysicalServingLteCellId - OK");
            LE_INFO("Phone %d LTE Physical Serving Cell ID %d", phoneId, pscid);

            result = taf_radio_GetServingCellLteBandInfo(&band, &bandWidth, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetServingCellLteBandInfo - LE_OK");
            LE_INFO("Phone %d LTE band %d", phoneId, band);
            PrintRFBandwidth(phoneId, bandWidth);
            break;
        case TAF_RADIO_RAT_NR5G:
            nrCid = taf_radio_GetServingNrCellId(phoneId);
            LE_TEST_OK(true, "taf_radio_GetServingNrCellId - uint64_t");
            LE_INFO("Phone %d NR Cell ID %" PRIuS, phoneId, (size_t)nrCid);

            arFcn = taf_radio_GetServingCellNrArfcn(phoneId);
            LE_TEST_OK(true, "taf_radio_GetServingCellNrArfcn - int32_t");
            LE_INFO("Phone %d NR Absolute Radio Frequency Channel Number %d", phoneId, arFcn);

            nrTac = taf_radio_GetServingCellNrTracAreaCode(phoneId);
            LE_TEST_OK(true, "taf_radio_GetServingCellNrTracAreaCode - int32_t");
            LE_INFO("Phone %d NR Tracking Area Code %d", phoneId, nrTac);

            pcid = taf_radio_GetPhysicalServingNrCellId(phoneId);
            LE_TEST_OK(true, "taf_radio_GetPhysicalServingNrCellId - OK");
            LE_INFO("Phone %d NR5G Physical Serving Cell ID %d", phoneId, pcid);

            result = taf_radio_GetServingCellNrBandInfo(&band, &bandWidth, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetServingCellNrBandInfo - LE_OK");
            LE_INFO("Phone %d NR5G band %d", phoneId, band);
            PrintRFBandwidth(phoneId, bandWidth);
            break;
        case TAF_RADIO_RAT_TDSCDMA:
            result = taf_radio_GetServingCellRoutingAreaCode(&rac, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetServingCellRoutingAreaCode - LE_OK");
            LE_INFO("Phone %d TDSCDMA Routing Area Code %d", phoneId, rac);

            result = taf_radio_GetServingCellBandInfo(&bandMask, &bandWidth, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetServingCellBandInfo - LE_OK");
            LE_INFO("Phone %d TDSCDMA band 0x%" PRIxS, phoneId, (size_t)bandMask);
            PrintRFBandwidth(phoneId, bandWidth);
            break;
        default:
            LE_INFO("Unavailble RAT %d for serving system.", rat);
            break;
    }

    result = taf_radio_GetCurrentNetworkName(shortOperatorStr, TAF_RADIO_NETWORK_NAME_MAX_LEN,
             phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetCurrentNetworkName short- OK");
    LE_INFO("Phone %d current network short name %s", phoneId, shortOperatorStr);

    result = taf_radio_GetCurrentNetworkLongName(longOperatorStr, TAF_RADIO_NETWORK_NAME_MAX_LEN,
             phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetCurrentNetworkName long - OK");
    LE_INFO("Phone %d current network long name %s", phoneId, longOperatorStr);

    result = taf_radio_GetCurrentNetworkMccMnc(mccStr, TAF_RADIO_MCC_BYTES, mncStr,
        TAF_RADIO_MNC_BYTES, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetCurrentNetworkMccMnc - OK");
    LE_INFO("Phone %d current network MCC %s MNC %s", phoneId, mccStr, mncStr);

    result = taf_radio_GetNrDualConnectivityStatus(&endcStatus,&dcnrStatus,
            phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetNrDualConnectivityStatus - LE_OK");
    LE_INFO("Phone %d current endc / dcnr  status %d / %d", phoneId, endcStatus, dcnrStatus);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function prints Cellular Capabilities
 * Max SIM supported and max sim which can be simultaneously supported to make calls
 * SIM Rat capability
 * Hardware Rat capability
 */
//--------------------------------------------------------------------------------------------------
void PrintCellularCapabilityStatus
(
    uint8_t phoneId ///< [IN] Phone ID.
)
{

    le_result_t result;

    uint8_t totalSimCount = 0;
    uint8_t maxActiveSims = 0;

    result = taf_radio_GetHardwareSimConfig(&totalSimCount,&maxActiveSims);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetHardwareSimConfig - LE_OK");

    LE_INFO("totalSimCount : %d.", totalSimCount);
    LE_INFO("maxActiveSims : %d.", maxActiveSims);

    taf_radio_RatBitMask_t deviceRatCapMask = 0x0;
    taf_radio_RatBitMask_t simRatCapMask = 0x0;

    result = taf_radio_GetHardwareSimRatCapabilities(&deviceRatCapMask,&simRatCapMask,
             phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetHardwareSIMRatCapabilities - LE_OK");


    LE_INFO("Phone %d deviceRatCapabilities",phoneId);
    PrintRatBitMask(deviceRatCapMask);
    LE_INFO("Phone %d simRatCapabilities",phoneId);
    PrintRatBitMask(simRatCapMask);
}

//--------------------------------------------------------------------------------------------------
/**
 * This thread is created for adding handlers for network registation state, packet swicthed state,
 * network registation rejection, and Radio Access Technology change.
 */
//--------------------------------------------------------------------------------------------------
void* HandlerTestThread
(
    void* contextPtr ///< [IN] Thread context.
)
{
    // Connect to service.
    taf_radio_ConnectService();

    // Add handler.
    netRegStateHandlerRef = taf_radio_AddNetRegStateEventHandler(
        (taf_radio_NetRegStateHandlerFunc_t)NetRegStateHandler, NULL);
    LE_TEST_OK(netRegStateHandlerRef != NULL, "taf_radio_AddNetRegStateEventHandler - OK");

    packSwStateHandlerRef = taf_radio_AddPacketSwitchedChangeHandler(
        (taf_radio_PacketSwitchedChangeHandlerFunc_t)PackSwStateHandler, NULL);
    LE_TEST_OK(packSwStateHandlerRef != NULL, "taf_radio_AddPacketSwitchedChangeHandler - OK");

    netRegRejHandlerRef = taf_radio_AddNetRegRejectHandler(
        (taf_radio_NetRegRejectHandlerFunc_t)NetRegRejectHandler, NULL);
    LE_TEST_OK(netRegRejHandlerRef != NULL, "taf_radio_AddNetRegRejectHandler - OK");

    ratChangeHandlerRef = taf_radio_AddRatChangeHandler(
        (taf_radio_RatChangeHandlerFunc_t)RatChangeHandler, NULL);
    LE_TEST_OK(ratChangeHandlerRef != NULL, "taf_radio_AddRatChangeHandler - OK");

    imsRegStatusChangeHandlerRef = taf_radio_AddImsRegStatusChangeHandler(
        (taf_radio_ImsRegStatusChangeHandlerFunc_t)ImsRegStateHandler, NULL);
    LE_TEST_OK(imsRegStatusChangeHandlerRef != NULL, "taf_radio_AddImsRegStatusChangeHandler - OK");

    opModeChangeHandlerRef = taf_radio_AddOpModeChangeHandler(
        (taf_radio_OpModeChangeHandlerFunc_t)OpModeChangeHandler, NULL);
    LE_TEST_OK(opModeChangeHandlerRef != NULL, "taf_radio_AddOpModeChangeHandler - OK");

    netStatusChangeHandlerRef = taf_radio_AddNetStatusChangeHandler(
        (taf_radio_NetStatusHandlerFunc_t)NetStatusChangeHandler, NULL);
    LE_TEST_OK(netStatusChangeHandlerRef != NULL, "taf_radio_AddNetStatusChangeHandler - OK");

    imsStatusChangeHandlerRef = taf_radio_AddImsStatusChangeHandler(
        (taf_radio_ImsStatusChangeHandlerFunc_t)ImsStatusHandler, NULL);
    LE_TEST_OK(imsStatusChangeHandlerRef != NULL, "taf_radio_AddImsStatusChangeHandler - OK");

    nrIconTypeHandlerRef = taf_radio_AddNrIconTypeHandler(
        (taf_radio_NrIconTypeHandlerFunc_t)NrIconTypeHandler, NULL);
    LE_TEST_OK(nrIconTypeHandlerRef != NULL, "taf_radio_AddNrIconTypeHandler - !NULL");

    le_sem_Post((le_sem_Ref_t)contextPtr);
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create thread for test handler.
 */
//--------------------------------------------------------------------------------------------------
void CreateHandlerTestThread
(
    void
)
{
    le_sem_Ref_t semaphore = le_sem_Create("semaphore", 0);
    le_thread_Ref_t threadRef = le_thread_Create("HandlerTestThread", HandlerTestThread,
        (void*)semaphore);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handlers for network registation state, packet swicthed state, network registation
 * rejection, Radio Access Technology change, and IMS registration status.
 */
//--------------------------------------------------------------------------------------------------
void RemoveTestHandler
(
    void
)
{
    // Remove handler
    taf_radio_RemoveNetRegStateEventHandler(netRegStateHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveNetRegStateEventHandler - OK");

    taf_radio_RemovePacketSwitchedChangeHandler(packSwStateHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemovePacketSwitchedChangeHandler - OK");

    taf_radio_RemoveNetRegRejectHandler(netRegRejHandlerRef);
    LE_TEST_OK(true, "taf_pa_radio_RemoveNetRegRejectHandler - OK");

    taf_radio_RemoveRatChangeHandler(ratChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveRatChangeHandler - OK");

    taf_radio_RemoveImsRegStatusChangeHandler(imsRegStatusChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveImsRegStatusChangeHandler - OK");

    taf_radio_RemoveOpModeChangeHandler(opModeChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveOpModeChangeHandler - OK");

    taf_radio_RemoveNetStatusChangeHandler(netStatusChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveNetStatusChangeHandler - OK");

    taf_radio_RemoveImsStatusChangeHandler(imsStatusChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveImsStatusChangeHandler - OK");

    taf_radio_RemoveNrIconTypeHandler(nrIconTypeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveNrIconTypeHandler - void");
}

//--------------------------------------------------------------------------------------------------
/**
 * Signal test thread is created for adding handelrs for GSM/UMTS/CDMA/TD-SCDMA/LTE/NR5G signal
 * strength chanegs.
 */
//--------------------------------------------------------------------------------------------------
void* SignalTestThread
(
    void* contextPtr ///< [IN] Thread context.
)
{
    // Connect to service.
    taf_radio_ConnectService();

    // Add handler.
    gsmSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_GSM,
       (taf_radio_SignalStrengthChangeHandlerFunc_t)GsmSsChangeHandler, NULL);
    LE_TEST_OK(gsmSsChangeHandlerRef != NULL, "taf_radio_AddSignalStrengthChangeHandler - OK");

    umtsSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_UMTS,
       (taf_radio_SignalStrengthChangeHandlerFunc_t)UmtsSsChangeHandler, NULL);
    LE_TEST_OK(umtsSsChangeHandlerRef != NULL, "taf_radio_AddSignalStrengthChangeHandler - OK");

    cdmaSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_CDMA,
       (taf_radio_SignalStrengthChangeHandlerFunc_t)UmtsSsChangeHandler, NULL);
    LE_TEST_OK(cdmaSsChangeHandlerRef != NULL, "taf_radio_AddSignalStrengthChangeHandler - OK");

    lteSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_LTE,
       (taf_radio_SignalStrengthChangeHandlerFunc_t)LteSsChangeHandler, NULL);
    LE_TEST_OK(lteSsChangeHandlerRef != NULL, "taf_radio_AddSignalStrengthChangeHandler - OK");

    nr5gSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_NR5G,
       (taf_radio_SignalStrengthChangeHandlerFunc_t)Nr5gSsChangeHandler, NULL);
    LE_TEST_OK(nr5gSsChangeHandlerRef != NULL, "taf_radio_AddSignalStrengthChangeHandler - OK");

    cellInfoChangeHandlerRef = taf_radio_AddCellInfoChangeHandler(
       (taf_radio_CellInfoChangeHandlerFunc_t)CellInfoChangeHandler, NULL);
    LE_TEST_OK(cellInfoChangeHandlerRef != NULL, "taf_radio_AddCellInfoChangeHandler - OK");

    le_sem_Post((le_sem_Ref_t)contextPtr);
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create signal test thread.
 */
//--------------------------------------------------------------------------------------------------
void CreateSignalTestThread
(
    long phoneId ///< [IN] Phone ID.
)
{
    le_sem_Ref_t semaphore = le_sem_Create("semaphore", 0);
    le_thread_Ref_t threadRef = le_thread_Create("SignalTestThread", SignalTestThread,
        (void*)semaphore);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);
    le_sem_Delete(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handelrs for GSM/UMTS/CDMA/TD-SCDMA/LTE/NR5G signal strength changes.
 */
//--------------------------------------------------------------------------------------------------
void RemoveSignalTestHandler
(
    void
)
{
    // Remove handler
    taf_radio_RemoveSignalStrengthChangeHandler(gsmSsChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveSignalStrengthChangeHandler - OK");

    taf_radio_RemoveSignalStrengthChangeHandler(umtsSsChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveSignalStrengthChangeHandler - OK");

    taf_radio_RemoveSignalStrengthChangeHandler(cdmaSsChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveSignalStrengthChangeHandler - OK");

    taf_radio_RemoveSignalStrengthChangeHandler(lteSsChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveSignalStrengthChangeHandler - OK");

    taf_radio_RemoveSignalStrengthChangeHandler(nr5gSsChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveSignalStrengthChangeHandler - OK");

    taf_radio_RemoveCellInfoChangeHandler(cellInfoChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveCellInfoChangeHandler - OK");

}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for manual network registation.
 */
//--------------------------------------------------------------------------------------------------
static void ManualRegHandler
(
    le_result_t result, ///< [IN] Result of manual network registation.
    void* contextPtr    ///< [IN] Handler context.
)
{
    LE_INFO("result: %d", result);
}

//--------------------------------------------------------------------------------------------------
/**
 * Manual registation test thread.
 */
//--------------------------------------------------------------------------------------------------
void* ManualRegTestThread
(
    void* contextPtr ///< [IN] Thread context.
)
{
    // Connect to service.
    taf_radio_ConnectService();

    taf_radio_int_test_AsyncTest_t* testContextPtr =
       (taf_radio_int_test_AsyncTest_t*)contextPtr;
    taf_radio_SetManualRegisterModeAsync(testContextPtr->mcc, testContextPtr->mnc,
        ManualRegHandler, NULL, testContextPtr->phoneId);

    LE_TEST_OK(true, "taf_radio_SetManualRegisterModeAsync - OK");

    le_sem_Post(testContextPtr->semaphore);
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create manual registation test thread.
 */
//--------------------------------------------------------------------------------------------------
void CreateManualRegTestThread
(
    taf_radio_int_test_AsyncTest_t* contextPtr ///< [IN] Thread context.
)
{
    contextPtr->semaphore = le_sem_Create("semaphore", 0);
    le_thread_Ref_t threadRef = le_thread_Create("ManualRegTestThread", ManualRegTestThread,
        (void*)contextPtr);
    le_thread_Start(threadRef);
    le_sem_Wait(contextPtr->semaphore);
    le_sem_Delete(contextPtr->semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for network scan.
 */
//--------------------------------------------------------------------------------------------------
static void NetworkScanHandler
(
    taf_radio_ScanInformationListRef_t listRef, ///< [IN] Network scan information list reference.
    void* contextPtr                            ///< [IN] Handler context.
)
{
    PrintScanInfoList(listRef);

    le_result_t result = taf_radio_DeleteCellularNetworkScan(listRef);
    LE_TEST_OK(result == LE_OK, "taf_radio_DeleteCellularNetworkScan - OK");

    le_sem_Post((le_sem_Ref_t)contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Network scan test thread.
 */
//--------------------------------------------------------------------------------------------------
void* NetworkScanTestThread
(
    void* contextPtr ///< [IN] Thread context.
)
{
    // Connect to service.
    taf_radio_ConnectService();

    taf_radio_int_test_AsyncTest_t* testContextPtr = (taf_radio_int_test_AsyncTest_t*)contextPtr;
    taf_radio_PerformCellularNetworkScanAsync(NetworkScanHandler,
        (void*)testContextPtr->handlerSem, testContextPtr->phoneId);

    LE_TEST_OK(true, "taf_radio_PerformCellularNetworkScanAsync - OK");

    le_sem_Post(testContextPtr->semaphore);
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create network scan thread.
 */
//--------------------------------------------------------------------------------------------------
void CreateNetworkScanTestThread
(
    taf_radio_int_test_AsyncTest_t* contextPtr ///< [IN] Thread context.
)
{
    contextPtr->semaphore = le_sem_Create("semaphore", 0);
    le_thread_Ref_t threadRef = le_thread_Create("NetworkScanTestThread", NetworkScanTestThread,
        (void*)contextPtr);
    le_thread_Start(threadRef);
    le_sem_Wait(contextPtr->semaphore);
    le_sem_Delete(contextPtr->semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for Pysical Cell Identity network scan.
 */
//--------------------------------------------------------------------------------------------------
static void PciNetworkScanHandler
(
    taf_radio_PciScanInformationListRef_t listRef, ///< [IN] PCI scan information list reference.
    uint8_t phoneId,                               ///< [IN] Phone ID.
    void* contextPtr                               ///< [IN] Handler context.
)
{
    PrintPciScanInfoList(listRef);

    le_result_t result = taf_radio_DeletePciNetworkScan(listRef);
    LE_TEST_OK(result == LE_OK, "taf_radio_DeletePciNetworkScan - OK");

    le_sem_Post((le_sem_Ref_t)contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Pysical Cell Identity network scan test thread.
 */
//--------------------------------------------------------------------------------------------------
void* PciNetworkScanTestThread
(
    void* contextPtr ///< [IN] Thread context.
)
{
    // Connect to service.
    taf_radio_ConnectService();

    taf_radio_int_test_AsyncTest_t* testContextPtr = (taf_radio_int_test_AsyncTest_t*)contextPtr;
    taf_radio_PerformPciNetworkScanAsync(testContextPtr->ratMask, PciNetworkScanHandler,
        (void*)testContextPtr->handlerSem, testContextPtr->phoneId);

    LE_TEST_OK(true, "taf_radio_PerformPciNetworkScanAsync - OK");

    le_sem_Post(testContextPtr->semaphore);
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create Pysical Cell Identity network scan thread.
 */
//--------------------------------------------------------------------------------------------------
void CreatePciNetworkScanTestThread
(
    taf_radio_int_test_AsyncTest_t* contextPtr ///< [IN] Thread context.
)
{
    contextPtr->semaphore = le_sem_Create("semaphore", 0);
    le_thread_Ref_t threadRef = le_thread_Create("PciNetworkScanTestThread",
        PciNetworkScanTestThread, (void*)contextPtr);
    le_thread_Start(threadRef);
    le_sem_Wait(contextPtr->semaphore);
    le_sem_Delete(contextPtr->semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    CheckArgs(1);

    le_result_t result;
    const char* cmd = le_arg_GetArg(0);
    if (cmd == NULL)
    {
        PrintHelpMenu();
    }

    if (strncmp(cmd, "power", strlen("power")) == 0)
    {
        CheckArgs(3);
        LE_TEST_INFO("======== Power Test ========");

        const char* phone = le_arg_GetArg(1);
        const char* power = le_arg_GetArg(2);
        if (phone == NULL || power == NULL)
        {
            PrintHelpMenu();
        }

        long phoneId = strtol(phone, NULL, 10);

        if (strncmp(power, "on", strlen("on")) == 0)
        {
            result = taf_radio_SetRadioPower(LE_ON, DEFAULT_PHONE_ID);
            LE_TEST_OK(result == LE_OK, "taf_radio_SetRadioPower - OK");
        }
        else if (strncmp(power, "off", strlen("off")) == 0)
        {
            result = taf_radio_SetRadioPower(LE_OFF, DEFAULT_PHONE_ID);
            LE_TEST_OK(result == LE_OK, "taf_radio_SetRadioPower - OK");
        }
        else if (strncmp(power, "status", strlen("status")) == 0)
        {
            le_onoff_t powerStatus;
            result = taf_radio_GetRadioPower(&powerStatus, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetRadioPower - OK");
            if (powerStatus == LE_ON)
            {
                LE_INFO("Phone %ld power state : On.", phoneId);
            }
            else
            {
                LE_INFO("Phone %ld power state : Off.", phoneId);
            }

            taf_radio_OpMode_t mode;
            result = taf_radio_GetOperatingMode(&mode, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetOperatingMode - OK");
            PrintOperatingMode(mode);
        }
        else
        {
            PrintHelpMenu();
        }
    }
    else if (strncmp(cmd, "reg", strlen("reg")) == 0)
    {
        CheckArgs(3);
        LE_TEST_INFO("======== Network Registration Test ========");

        const char* phone = le_arg_GetArg(1);
        const char* op = le_arg_GetArg(2);
        if (phone == NULL || op == NULL)
        {
            PrintHelpMenu();
        }
        long phoneId = strtol(le_arg_GetArg(1), NULL, 10);

        if (strncmp(op, "auto", strlen("auto")) == 0)
        {
            result = taf_radio_SetAutomaticRegisterMode(phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_SetAutomaticRegisterMode - OK");
        }
        else if (strncmp(op, "manual-sync", strlen("manual-sync")) == 0)
        {
            CheckArgs(5);
            const char* mcc = le_arg_GetArg(3);
            const char* mnc = le_arg_GetArg(4);
            result = taf_radio_SetManualRegisterMode(mcc, mnc, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_SetManualRegisterMode - OK");
        }
        else if (strncmp(op, "manual-async", strlen("manual-async")) == 0)
        {
            CheckArgs(5);
            const char* mcc = le_arg_GetArg(3);
            const char* mnc = le_arg_GetArg(4);
            taf_radio_int_test_AsyncTest_t context;
            context.mcc = mcc;
            context.mnc = mnc;
            context.phoneId = phoneId;
            CreateManualRegTestThread(&context);

            // Wait for handler's response.
            le_thread_Sleep(5);
        }
        else if (strncmp(op, "status", strlen("status")) == 0)
        {
            taf_radio_NetRegState_t netRegState;
            taf_radio_NetRegState_t packSwState;
            result = taf_radio_GetNetRegState(&netRegState, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetNetRegState - OK");

            PrintNetRegState(phoneId, "network regristration state", netRegState);

            result = taf_radio_GetPacketSwitchedState(&packSwState, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetPacketSwitchedState - OK");

            PrintNetRegState(phoneId, "packet switched state", packSwState);

            bool isManual;
            char mccStr[TAF_RADIO_MCC_BYTES] = {0};
            char mncStr[TAF_RADIO_MNC_BYTES] = {0};
            result = taf_radio_GetRegisterMode(&isManual, mccStr, TAF_RADIO_MCC_BYTES, mncStr,
                TAF_RADIO_MNC_BYTES, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetRegisterMode - OK");
            if (isManual)
            {
                LE_INFO("Phone %ld registation mode : Manual (MCC:%s MNC:%s).",
                    phoneId, mccStr, mncStr);
            }
            else
            {
                LE_INFO("Phone %ld registation mode : Auto.", phoneId);
            }
        }
        else
        {
            PrintHelpMenu();
        }
    }
    else if (strncmp(cmd, "rat", strlen("rat")) == 0)
    {
        CheckArgs(3);
        LE_TEST_INFO("======== Radio Access Technology Test ========");

        const char* phone = le_arg_GetArg(1);
        const char* op = le_arg_GetArg(2);
        if (phone == NULL || op == NULL)
        {
            PrintHelpMenu();
        }
        long phoneId = strtol(phone, NULL, 10);

        taf_radio_NetStatusRef_t netRef = taf_radio_GetNetStatus(phoneId);
        LE_TEST_OK(netRef != NULL, "taf_radio_GetNetStatus - OK");

        if (strncmp(op, "prefer", strlen("prefer")) == 0)
        {
            CheckArgs(4);
            taf_radio_RatBitMask_t rat =
                (taf_radio_RatBitMask_t)strtoul(le_arg_GetArg(3), NULL, 16);
            result = taf_radio_SetRatPreferences(rat, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_SetRatPreferences - OK");
        }
        else if (strncmp(op, "status", strlen("status")) == 0)
        {
            taf_radio_Rat_t rat;
            taf_radio_RatBitMask_t ratMask = 0x0;
            result = taf_radio_GetRatPreferences(&ratMask, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetRatPreferences - OK");
            PrintRatBitMask(ratMask);

            result = taf_radio_GetRadioAccessTechInUse(&rat, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetRadioAccessTechInUse - OK");
            PrintRAT(rat);

            if (rat == TAF_RADIO_RAT_LTE)
            {
                taf_radio_CsCap_t cap = TAF_RADIO_CS_CAP_UNKNOWN;
                result = taf_radio_GetLteCsCap(netRef, &cap);
                LE_TEST_OK(result == LE_OK, "taf_radio_GetLteCsCap - OK");
                PrintCsCap(cap);

                taf_radio_NrIconType_t icon = TAF_RADIO_NR_ICON_TYPE_NONE;
                result = taf_radio_GetNrIconType(&icon, phoneId);
                LE_TEST_OK(result == LE_OK, "taf_radio_GetNrIconType - OK");
                PrintNrIcon(icon);
            }
            else if(rat == TAF_RADIO_RAT_NR5G)
            {
                taf_radio_NrIconType_t icon = TAF_RADIO_NR_ICON_TYPE_NONE;
                result = taf_radio_GetNrIconType(&icon, phoneId);
                LE_TEST_OK(result == LE_OK, "taf_radio_GetNrIconType - OK");
                PrintNrIcon(icon);
            }

            taf_radio_RatSvcStatus_t svcStatus = TAF_RADIO_RAT_SVC_STATUS_UNKNOWN;
            result = taf_radio_GetRatSvcStatus(netRef, &svcStatus);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetRatSvcStatus - OK");
            PrintRatSvcStatus(svcStatus);
        }
        else
        {
            PrintHelpMenu();
        }
    }
    else if (strncmp(cmd, "domain", strlen("domain")) == 0)
    {
        CheckArgs(3);
        LE_TEST_INFO("======== Radio Service Domain Test ========");

        const char* phone = le_arg_GetArg(1);
        const char* op = le_arg_GetArg(2);
        if (phone == NULL || op == NULL)
        {
            PrintHelpMenu();
        }
        long phoneId = strtol(phone, NULL, 10);

        if (strncmp(op, "prefer", strlen("prefer")) == 0)
        {
            CheckArgs(4);
            const char* dm = le_arg_GetArg(3);
            if (strncmp(dm, "cs", strlen("cs")) == 0)
            {
                result = taf_radio_SetServiceDomainPreferences(
                    TAF_RADIO_SERVICE_DOMAIN_STATE_CS_ONLY, phoneId);
                LE_TEST_OK(result == LE_OK, "taf_radio_SetServiceDomainPreferences - OK");
            }
            else if (strncmp(dm, "ps", strlen("ps")) == 0)
            {
                result = taf_radio_SetServiceDomainPreferences(
                    TAF_RADIO_SERVICE_DOMAIN_STATE_PS_ONLY, phoneId);
                LE_TEST_OK(result == LE_OK, "taf_radio_SetServiceDomainPreferences - OK");
            }
            else if (strncmp(dm, "all", strlen("all")) == 0)
            {
                result = taf_radio_SetServiceDomainPreferences(
                    TAF_RADIO_SERVICE_DOMAIN_STATE_CS_AND_PS, phoneId);
                LE_TEST_OK(result == LE_OK, "taf_radio_SetServiceDomainPreferences - OK");
            }
            else
            {
                PrintHelpMenu();
            }
        }
        else if (strncmp(op, "status", strlen("status")) == 0)
        {
            taf_radio_ServiceDomainState_t domain = TAF_RADIO_SERVICE_DOMAIN_STATE_UNKNOWN;
            result = taf_radio_GetServiceDomainPreferences(&domain, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetServiceDomainPreferences - OK");
            PrintSrvDomain(domain);

            result = taf_radio_GetServiceDomain(&domain, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetServiceDomain - OK");
            PrintSrvDomain(domain);
        }
        else
        {
            PrintHelpMenu();
        }
    }
    else if (strncmp(cmd, "operator", strlen("operator")) == 0)
    {
        CheckArgs(3);
        LE_TEST_INFO("======== Prefered Operator Test ========");

        const char* phone = le_arg_GetArg(1);
        const char* op = le_arg_GetArg(2);
        if (phone == NULL || op == NULL)
        {
            PrintHelpMenu();
        }
        long phoneId = strtol(phone, NULL, 10);

        if (strncmp(op, "add", strlen("add")) == 0)
        {
            CheckArgs(6);
            const char* mcc = le_arg_GetArg(3);
            const char* mnc = le_arg_GetArg(4);
            taf_radio_RatBitMask_t rat =
                (taf_radio_RatBitMask_t)strtoul(le_arg_GetArg(5), NULL, 16);
            result = taf_radio_AddPreferredOperator(mcc, mnc, rat, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_AddPreferredOperator - OK");
        }
        else if (strncmp(op, "remove", strlen("remove")) == 0)
        {
            CheckArgs(5);
            const char* mcc = le_arg_GetArg(3);
            const char* mnc = le_arg_GetArg(4);
            result = taf_radio_RemovePreferredOperator(mcc, mnc, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_RemovePreferredOperator - OK");
        }
        else if (strncmp(op, "list", strlen("list")) == 0)
        {
            PrintPrefOpList(phoneId);
        }
        else
        {
            PrintHelpMenu();
        }
    }
    else if (strncmp(cmd, "signal", strlen("signal")) == 0)
    {
        CheckArgs(3);
        LE_TEST_INFO("======== Signal Test ========");

        const char* phone = le_arg_GetArg(1);
        const char* op = le_arg_GetArg(2);
        if (phone == NULL || op == NULL)
        {
            PrintHelpMenu();
        }
        long phoneId = strtol(phone, NULL, 10);

        if (strncmp(op, "monitor", strlen("monitor")) == 0)
        {
            CheckArgs(4);
            long time = strtol(le_arg_GetArg(3), NULL, 10);

            CreateSignalTestThread(phoneId);

            // Wait for handler's response.
            le_thread_Sleep(time);
            RemoveSignalTestHandler();
        }
        else if (strncmp(op, "delta", strlen("delta")) == 0)
        {
            CheckArgs(6);
            const char* rat = le_arg_GetArg(3);
            if (rat == NULL)
            {
                PrintHelpMenu();
            }
            long hysdB = strtol(le_arg_GetArg(5), NULL, 10);
            if (strncmp(rat, "gsm", strlen("gsm")) == 0)
            {
                long rssiDelta = strtol(le_arg_GetArg(4), NULL, 10);
                GsmSignalConfiguration(phoneId, rssiDelta, hysdB);
            }
            else if (strncmp(rat, "umts", strlen("umts")) == 0)
            {
                long rssiDelta = strtol(le_arg_GetArg(4), NULL, 10);
                UmtsSignalConfiguration(phoneId, rssiDelta, hysdB);
            }
            else if (strncmp(rat, "lte", strlen("lte")) == 0)
            {
                long rsrpDelta = strtol(le_arg_GetArg(4), NULL, 10);
                LteSignalConfiguration(phoneId, rsrpDelta, hysdB);
            }
            else if (strncmp(rat, "nr5g", strlen("nrg5")) == 0)
            {
                long rsrpDelta = strtol(le_arg_GetArg(4), NULL, 10);
                Nr5gSignalConfiguration(phoneId, rsrpDelta, hysdB);
            }
            else
            {
                PrintHelpMenu();
            }
        }
        else if (strncmp(op, "metrics", strlen("metrics")) == 0)
        {
            uint32_t quality = 0;
            result = taf_radio_GetSignalQual(&quality, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetSignalQual - OK");
            LE_INFO("Phone %ld signal quality level : %d.", phoneId, quality);

            taf_radio_MetricsRef_t metrics = taf_radio_MeasureSignalMetrics(phoneId);
            LE_TEST_OK(metrics != NULL, "taf_radio_MeasureSignalMetrics - OK");

            taf_radio_RatBitMask_t ratMask = taf_radio_GetRatOfSignalMetrics(metrics);
            LE_TEST_OK(ratMask != 0, "taf_radio_GetRatOfSignalMetrics - OK");

            if (ratMask & TAF_RADIO_RAT_BIT_MASK_GSM)
            {
                int32_t rssi;
                uint32_t ber;
                result = taf_radio_GetGsmSignalMetrics(metrics, &rssi, &ber);
                LE_TEST_OK(result == LE_OK, "taf_radio_GetGsmSignalMetrics - OK");
                LE_INFO("GSM signal strength %d dBm.", rssi);
                LE_INFO("GSM bit error rate %d.", ber);
            }

            if (ratMask & (TAF_RADIO_RAT_BIT_MASK_UMTS | TAF_RADIO_RAT_BIT_MASK_TDSCDMA))
            {
                int32_t ss;
                uint32_t bler;
                int32_t rscp;
                result = taf_radio_GetUmtsSignalMetrics(metrics, &ss, &bler, &rscp);
                LE_TEST_OK(result == LE_OK, "taf_radio_GetUmtsSignalMetrics - OK");
                LE_INFO("UMTS signal strength %d dBm.", ss);
                LE_INFO("UMTS block error rate %d.", bler);
                LE_INFO("UMTS received signal channel power %d dBm.", rscp);
            }

            if (ratMask & TAF_RADIO_RAT_BIT_MASK_LTE)
            {
                int32_t ss;
                int32_t rsrq;
                int32_t rsrp;
                int32_t snr;
                result = taf_radio_GetLteSignalMetrics(metrics, &ss, &rsrq, &rsrp, &snr);
                LE_TEST_OK(result == LE_OK, "taf_radio_GetLteSignalMetrics - OK");
                LE_INFO("LTE signal strength %d dBm.", ss);
                LE_INFO("LTE reference signal receive quality %d dB.", rsrq);
                LE_INFO("LTE reference signal receive power %d dBm.", rsrp);
                LE_INFO("LTE signal to noise ratio %f dB.", (float)snr / 10);
            }

            if (ratMask & TAF_RADIO_RAT_BIT_MASK_NR5G)
            {
                int32_t rsrq;
                int32_t rsrp;
                int32_t snr;
                result = taf_radio_GetNr5gSignalMetrics(metrics, &rsrq, &rsrp, &snr);
                LE_TEST_OK(result == LE_OK, "taf_radio_GetNr5gSignalMetrics - OK");
                LE_INFO("NR5G reference signal receive quality %d dB.", rsrq);
                LE_INFO("NR5G reference signal receive power %d dBm.", rsrp);
                LE_INFO("NR5G signal to noise ratio %f dB.", (float)snr / 10);
            }

            result = taf_radio_DeleteSignalMetrics(metrics);
            LE_TEST_OK(result == LE_OK, "taf_radio_DeleteSignalMetrics - OK");
        }
        else
        {
            PrintHelpMenu();
        }
    }
    else if (strncmp(cmd, "neighbor", strlen("neighbor")) == 0)
    {
        CheckArgs(2);
        LE_TEST_INFO("======== Neighboring Cells Information Test ========");

        const char* phone = le_arg_GetArg(1);
        if (phone == NULL)
        {
            PrintHelpMenu();
        }
        long phoneId = strtol(phone, NULL, 10);

        PrintNgbrCellsInfo(phoneId);
    }
    else if (strncmp(cmd, "serving", strlen("serving")) == 0)
    {
        CheckArgs(2);
        LE_TEST_INFO("======== Serving Status Test ========");

        const char* phone = le_arg_GetArg(1);
        if (phone == NULL)
        {
            PrintHelpMenu();
        }
        long phoneId = strtol(phone, NULL, 10);

        PrintServingStatus(phoneId);
    }
    else if (strncmp(cmd, "scan", strlen("scan")) == 0)
    {
        CheckArgs(3);
        LE_TEST_INFO("======== Network Scan Test ========");

        const char* phone = le_arg_GetArg(1);
        const char* mode = le_arg_GetArg(2);
        if (phone == NULL || mode == NULL)
        {
            PrintHelpMenu();
        }
        long phoneId = strtol(phone, NULL, 10);

        if (strncmp(mode, "plmn-sync", strlen("plmn-sync")) == 0)
        {
            taf_radio_ScanInformationListRef_t listRef =
                taf_radio_PerformCellularNetworkScan(phoneId);
            LE_TEST_OK(listRef != NULL, "taf_radio_PerformCellularNetworkScan - OK");

            PrintScanInfoList(listRef);

            result = taf_radio_DeleteCellularNetworkScan(listRef);
            LE_TEST_OK(result == LE_OK, "taf_radio_DeleteCellularNetworkScan - OK");
        }
        else if (strncmp(mode, "plmn-async", strlen("plmn-async")) == 0)
        {
            taf_radio_int_test_AsyncTest_t context;
            context.phoneId = phoneId;
            context.handlerSem = le_sem_Create("handlerSem", 0);
            CreateNetworkScanTestThread(&context);

            // Wait for handler's response.
            le_sem_Wait(context.handlerSem);
            le_sem_Delete(context.handlerSem);
        }
        else if (strncmp(mode, "pci-sync", strlen("pci-sync")) == 0)
        {
            CheckArgs(4);
            taf_radio_RatBitMask_t rat = (taf_radio_RatBitMask_t)strtoul(le_arg_GetArg(3), NULL, 16);

            taf_radio_PciScanInformationListRef_t listRef =
                taf_radio_PerformPciNetworkScan(rat, phoneId);
            LE_TEST_OK(listRef != NULL, "taf_radio_PerformPciNetworkScan - OK");

            PrintPciScanInfoList(listRef);

            result = taf_radio_DeletePciNetworkScan(listRef);
            LE_TEST_OK(result == LE_OK, "taf_radio_DeletePciNetworkScan - OK");
        }
        else if (strncmp(mode, "pci-async", strlen("pci-async")) == 0)
        {
            CheckArgs(4);
            taf_radio_RatBitMask_t rat = (taf_radio_RatBitMask_t)strtoul(le_arg_GetArg(3), NULL, 16);

            taf_radio_int_test_AsyncTest_t context;
            context.ratMask = rat;
            context.phoneId = phoneId;
            context.handlerSem = le_sem_Create("handlerSem", 0);
            CreatePciNetworkScanTestThread(&context);

            // Wait for handler's response.
            le_sem_Wait(context.handlerSem);
            le_sem_Delete(context.handlerSem);
        }
        else
        {
            PrintHelpMenu();
        }
    }
    else if (strncmp(cmd, "band", strlen("band")) == 0)
    {
        CheckArgs(3);
        LE_TEST_INFO("======== Band Test ========");

        const char* phone = le_arg_GetArg(1);
        const char* op = le_arg_GetArg(2);
        if (phone == NULL || op == NULL)
        {
            PrintHelpMenu();
        }
        long phoneId = strtol(phone, NULL, 10);

        if (strncmp(op, "2G+3G", strlen("2G+3G")) == 0)
        {
            CheckArgs(4);
            taf_radio_BandBitMask_t band =
                (taf_radio_BandBitMask_t)strtoull(le_arg_GetArg(3), NULL, 16);

            result = taf_radio_SetBandPreferences(band, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_SetBandPreferences - OK");
        }
        else if (strncmp(op, "LTE", strlen("LTE")) == 0)
        {
            CheckArgs(7);
            uint64_t band[TAF_RADIO_LTE_BAND_GROUP_NUM];
            uint8_t i;
            for (i = 0; i < TAF_RADIO_LTE_BAND_GROUP_NUM; i++)
            {
                band[i] = strtoull(le_arg_GetArg(3 + i), NULL, 16);
            }
            result = taf_radio_SetLteBandPreferences(band, TAF_RADIO_LTE_BAND_GROUP_NUM, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_SetLteBandPreferences - OK");
        }
        else if (strncmp(op, "status", strlen("status")) == 0)
        {
            PrintBandStatus(phoneId);
        }
        else
        {
            PrintHelpMenu();
        }
    }
    else if (strncmp(cmd, "handler", strlen("handler")) == 0)
    {
        CheckArgs(2);
        LE_TEST_INFO("======== Handler Test ========");
        const char* timeStr = le_arg_GetArg(1);
        if (timeStr == NULL)
        {
            PrintHelpMenu();
        }
        long time = strtol(timeStr, NULL, 10);

        CreateHandlerTestThread();
        // Wait for handler's response.
        le_thread_Sleep(time);
        RemoveTestHandler();
    }
    else if (strncmp(cmd, "ims", strlen("ims")) == 0)
    {
        CheckArgs(3);
        LE_TEST_INFO("======== IMS Test ========");

        const char* phone = le_arg_GetArg(1);
        const char* op = le_arg_GetArg(2);
        if (phone == NULL || op == NULL)
        {
            PrintHelpMenu();
        }
        long phoneId = strtol(phone, NULL, 10);
        taf_radio_ImsRef_t imsRef = taf_radio_GetIms(phoneId);
        LE_TEST_OK(imsRef != NULL, "taf_radio_GetIms - LE_OK");

        if (strncmp(op, "status", strlen("status")) == 0)
        {
            taf_radio_ImsRegStatus_t regStatus = TAF_RADIO_IMS_REG_STATUS_UNKNOWN;
            result = taf_radio_GetImsRegStatus(&regStatus, phoneId);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetImsRegStatus - OK");

            PrintImsRegState(phoneId, regStatus);

            taf_radio_ImsSvcStatus_t svcStatus = TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;
            result = taf_radio_GetImsSvcStatus(imsRef, TAF_RADIO_IMS_SVC_TYPE_VOIP, &svcStatus);
            if (result != LE_UNSUPPORTED)
            {
                LE_TEST_OK(result == LE_OK, "taf_radio_GetImsSvcStatus - LE_OK");
                PrintImsSvcInfo(phoneId, TAF_RADIO_IMS_SVC_TYPE_VOIP, svcStatus);
            }

            svcStatus = TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;
            result = taf_radio_GetImsSvcStatus(imsRef, TAF_RADIO_IMS_SVC_TYPE_SMS, &svcStatus);
            if (result != LE_UNSUPPORTED)
            {
                LE_TEST_OK(result == LE_OK, "taf_radio_GetImsSvcStatus - LE_OK");
                PrintImsSvcInfo(phoneId, TAF_RADIO_IMS_SVC_TYPE_SMS, svcStatus);
            }

            bool enable = false;
            result = taf_radio_GetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_IMS_REG, &enable);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetImsSvcCfg - LE_OK");
            if (enable)
            {
                LE_INFO("IMS normal registration is enabled.");
            }
            else
            {
                LE_INFO("IMS normal registration is disabled.");
            }

            result = taf_radio_GetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_VOIP, &enable);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetImsSvcCfg - LE_OK");
            if (enable)
            {
                LE_INFO("IMS VoIP is enabled.");
            }
            else
            {
                LE_INFO("IMS VoIP is disabled.");
            }

            result = taf_radio_GetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_SMS, &enable);
            if (result != LE_UNSUPPORTED)
            {
                LE_TEST_OK(result == LE_OK, "taf_radio_GetImsSvcCfg - LE_OK");
                if (enable)
                {
                    LE_INFO("IMS SMS is enabled.");
                }
                else
                {
                    LE_INFO("IMS SMS is disabled.");
                }
            }

            result = taf_radio_GetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_RTT, &enable);
            if (result != LE_UNSUPPORTED)
            {
                LE_TEST_OK(result == LE_OK, "taf_radio_GetImsSvcCfg - LE_OK");
                if (enable)
                {
                    LE_INFO("IMS RTT is enabled.");
                }
                else
                {
                    LE_INFO("IMS RTT is disabled.");
                }
            }

            char userAgent[TAF_RADIO_IMS_USER_AGENT_BYTES] = {0};
            result = taf_radio_GetImsUserAgent(imsRef, userAgent, TAF_RADIO_IMS_USER_AGENT_BYTES);
            if (result != LE_UNSUPPORTED)
            {
                LE_TEST_OK(result == LE_OK, "taf_radio_GetImsUserAgent - LE_OK");
                LE_INFO("IMS user agent: %s", userAgent);
            }

            result = taf_radio_GetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_VONR, &enable);
            LE_TEST_OK(result == LE_OK, "taf_radio_GetImsSvcCfg - LE_OK");
            if (enable)
            {
                LE_INFO("IMS VoNR is enabled.");
            }
            else
            {
                LE_INFO("IMS VoNR is disabled.");
            }
        }
        else if (strncmp(op, "enable", strlen("enable")) == 0)
        {
            CheckArgs(4);
            const char* service = le_arg_GetArg(3);
            if (service == NULL)
            {
                PrintHelpMenu();
            }
            else if (strncmp(service, "registration", strlen("registration")) == 0)
            {
                result = taf_radio_SetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_IMS_REG, true);
                LE_TEST_OK(result == LE_OK, "taf_radio_SetImsSvcCfg - LE_OK");
            }
            else if (strncmp(service, "voip", strlen("voip")) == 0)
            {
                result = taf_radio_SetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_VOIP, true);
                LE_TEST_OK(result == LE_OK, "taf_radio_SetImsSvcCfg - LE_OK");
            }
            else if (strncmp(service, "sms", strlen("sms")) == 0)
            {
                result = taf_radio_SetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_SMS, true);
                if (result != LE_UNSUPPORTED)
                {
                    LE_TEST_OK(result == LE_OK, "taf_radio_SetImsSvcCfg - LE_OK");
                }
            }
            else if (strncmp(service, "rtt", strlen("rtt")) == 0)
            {
                result = taf_radio_SetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_RTT, true);
                if (result != LE_UNSUPPORTED)
                {
                    LE_TEST_OK(result == LE_OK, "taf_radio_SetImsSvcCfg - LE_OK");
                }
            }
            else if (strncmp(service, "vonr", strlen("vonr")) == 0)
            {
                result = taf_radio_SetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_VONR, true);
                LE_TEST_OK(result == LE_OK, "taf_radio_SetImsSvcCfg - LE_OK");
            }
            else
            {
                PrintHelpMenu();
            }
        }
        else if (strncmp(op, "disable", strlen("disable")) == 0)
        {
            CheckArgs(4);
            const char* service = le_arg_GetArg(3);
            if (service == NULL)
            {
                PrintHelpMenu();
            }
            else if (strncmp(service, "registration", strlen("registration")) == 0)
            {
                result = taf_radio_SetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_IMS_REG, false);
                LE_TEST_OK(result == LE_OK, "taf_radio_SetImsSvcCfg - LE_OK");
            }
            else if (strncmp(service, "voip", strlen("voip")) == 0)
            {
                result = taf_radio_SetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_VOIP, false);
                LE_TEST_OK(result == LE_OK, "taf_radio_SetImsSvcCfg - LE_OK");
            }
            else if (strncmp(service, "sms", strlen("sms")) == 0)
            {
                result = taf_radio_SetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_SMS, false);
                if (result != LE_UNSUPPORTED)
                {
                    LE_TEST_OK(result == LE_OK, "taf_radio_SetImsSvcCfg - LE_OK");
                }
            }
            else if (strncmp(service, "rtt", strlen("rtt")) == 0)
            {
                result = taf_radio_SetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_RTT, false);
                if (result != LE_UNSUPPORTED)
                {
                    LE_TEST_OK(result == LE_OK, "taf_radio_SetImsSvcCfg - LE_OK");
                }
            }
            else if (strncmp(service, "vonr", strlen("vonr")) == 0)
            {
                result = taf_radio_SetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_VONR, false);
                LE_TEST_OK(result == LE_OK, "taf_radio_SetImsSvcCfg - LE_OK");
            }
            else
            {
                PrintHelpMenu();
            }
        }
        else if (strncmp(op, "user", strlen("user")) == 0)
        {
            CheckArgs(4);
            const char* userAgent = le_arg_GetArg(3);
            if (userAgent == NULL)
            {
                PrintHelpMenu();
            }
            else
            {
                result = taf_radio_SetImsUserAgent(imsRef, userAgent);
                if (result != LE_UNSUPPORTED)
                {
                    LE_TEST_OK(result == LE_OK, "taf_radio_SetImsUserAgent - LE_OK");
                }
            }
        }
        else
        {
            PrintHelpMenu();
        }
    }
    else if (strncmp(cmd, "cellularCapability", strlen("cellularCapability")) == 0)
    {
        CheckArgs(2);
        LE_TEST_INFO("======== Cellular Capability ========");

        const char* phone = le_arg_GetArg(1);
        if (phone == NULL)
        {
            PrintHelpMenu();
        }
        long phoneId = strtol(phone, NULL, 10);

        PrintCellularCapabilityStatus(phoneId);
    }
    else
    {
        PrintHelpMenu();
    }

    LE_TEST_EXIT;
}
