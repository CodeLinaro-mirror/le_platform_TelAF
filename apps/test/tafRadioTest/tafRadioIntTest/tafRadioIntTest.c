/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "tafRadioIntTest.h"

taf_radio_NetRegStateEventHandlerRef_t netRegStateHandlerRef;
taf_radio_PacketSwitchedChangeHandlerRef_t packSwStateHandlerRef;
taf_radio_NetRegRejectHandlerRef_t netRegRejHandlerRef;
taf_radio_RatChangeHandlerRef_t ratChangeHandlerRef;
taf_radio_SignalStrengthChangeHandlerRef_t gsmSsChangeHandlerRef;
taf_radio_SignalStrengthChangeHandlerRef_t umtsSsChangeHandlerRef;
taf_radio_SignalStrengthChangeHandlerRef_t cdmaSsChangeHandlerRef;
taf_radio_SignalStrengthChangeHandlerRef_t tdscdmaSsChangeHandlerRef;
taf_radio_SignalStrengthChangeHandlerRef_t lteSsChangeHandlerRef;
taf_radio_SignalStrengthChangeHandlerRef_t nr5gSsChangeHandlerRef;

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
        "operator <phone> <add|remove|list> [<mcc>] [<mnc>] [<rat_bitmask>]\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "signal <phone> <monitor|metrics> [<time>] [<rssi_delta>] [<rsrp_delta>]\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- serving <phone>\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- neighbor <phone>\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- scan <phone> <mode> [<rat_bitmask>]\n"
        "    app runProc tafRadioIntTest tafRadioIntTest -- "
        "band <phone> <rat|status> [<band_bitmask>]\n"
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
        "           rssi delta in 0.1 dBm, required with 'delta' for RATs except NR5G.\n"
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
        "       phone         : '1' or '2'.\n"
        "       mode  : 'plmn-sync', 'plmn-async', 'pci-sync' or 'pci-async'.\n"
        "       rat_bitmask   : rat bit mask, required with 'pci-sync' 'pci-async' mode.\n"
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
        "    app runProc tafRadioIntTest tafRadioIntTest -- handler <time>\n"
        "       Handler for network changes, can test with 'cm radio' configurations.\n"
        "       time : monitor time in seconds.\n"
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
            LE_INFO("RAT : NR5G");
            break;
        default:
            LE_INFO("RAT : Unknown");
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
        case TAF_RADIO_SERVICE_DOMAIN_STATE_CS_ONLY:
            LE_INFO("Domain : CS Only");
            break;
        case TAF_RADIO_SERVICE_DOMAIN_STATE_PS_ONLY:
            LE_INFO("Domain : PS_Only");
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
    LE_TEST_OK(listRef != NULL, "taf_radio_GetPreferredOperatorsList - OK");
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
            LE_TEST_OK(opRef != NULL, "taf_radio_GetNextPreferredOperator - OK");
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
        LE_TEST_OK(infoRef != NULL, "taf_radio_GetNextCellularNetworkScan - OK");
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
            LE_TEST_OK(plmnRef != NULL, "taf_radio_GetNextPlmnInfo - OK");
        }

        i++;
        infoRef = taf_radio_GetNextPciScanInfo(listRef);
        LE_TEST_OK(infoRef != NULL, "taf_radio_GetNextPciScanInfo - OK");
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
        LE_TEST_OK(cellInfoRef != NULL, "taf_radio_GetFirstNeighborCellInfo - OK");

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
                    LE_INFO("Cell ID                    : %llu", cid);
                    LE_INFO("Local Area Code            : %d", lac);
                    LE_INFO("Signal Strength            : %d", rxlevel);
                    LE_INFO("Base Station Identity Code : %d", bsic);
                    break;
                case TAF_RADIO_RAT_UMTS:
                    cid = taf_radio_GetNeighborCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellId - OK");
                    rxlevel = taf_radio_GetNeighborCellRxLevel(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellRxLevel - OK");
                    LE_INFO("Cell ID         : %llu", cid);
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
                    LE_INFO("Cell ID         : %llu", cid);
                    LE_INFO("Signal Strength : %d", rxlevel);
                    break;
                case TAF_RADIO_RAT_NR5G:
                    cid = taf_radio_GetNeighborCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellId - OK");
                    nrpcid = taf_radio_GetPhysicalNeighborNrCellId(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetPhysicalNeighborNrCellId - OK");
                    rxlevel = taf_radio_GetNeighborCellRxLevel(cellInfoRef);
                    LE_TEST_OK(true, "taf_radio_GetNeighborCellRxLevel - OK");
                    LE_INFO("Cell ID          : %llu", cid);
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
                    LE_INFO("Cell ID          : %llu", cid);
                    LE_INFO("Physical Cell ID : %d", pcid);
                    LE_INFO("Signal Strength  : %d", rxlevel);
                    break;
                default:
                    break;
            }

            i++;

            cellInfoRef = taf_radio_GetNextNeighborCellInfo(ngbrCellsRef);
            LE_TEST_OK(cellInfoRef != NULL, "taf_radio_GetNextNeighborCellInfo - OK");
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
    LE_INFO("Phone %d 2G/3G band preferences 0x%llx.", phoneId, bandMask);

    result = taf_radio_GetBandCapabilities(&bandMask, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetBandCapabilities - OK");
    LE_INFO("Phone %d 2G/3G band capabilities 0x%llx.", phoneId, bandMask);

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
 * Configurations on GSM signal indication.
 */
//--------------------------------------------------------------------------------------------------
void GsmSignalConfiguration
(
    long phoneId,  ///< [IN] Phone ID.
    long rssiDelta ///< [IN] RSSI delta.
)
{
    le_result_t result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_GSM_RSSI,
        -1110, -480, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");

    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_GSM_RSSI, rssiDelta, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Configurations on UMTS signal indication.
 */
//--------------------------------------------------------------------------------------------------
void UmtsSignalConfiguration
(
    long phoneId,  ///< [IN] Phone ID.
    long rssiDelta ///< [IN] RSSI delta.
)
{
    le_result_t result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_UMTS_RSSI,
        -1210, 0, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");

    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_UMTS_RSSI, rssiDelta, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Configurations on CDMA signal indication.
 */
//--------------------------------------------------------------------------------------------------
void CdmaSignalConfiguration
(
    long phoneId,  ///< [IN] Phone ID.
    long rssiDelta ///< [IN] RSSI delta.
)
{
    le_result_t result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_CDMA_RSSI,
        -1050, -210, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");

    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_CDMA_RSSI, rssiDelta, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Configurations on TD-SCDMA signal indication.
 */
//--------------------------------------------------------------------------------------------------
void TdscdmaSignalConfiguration
(
    long phoneId,  ///< [IN] Phone ID.
    long rssiDelta ///< [IN] RSSI delta.
)
{
    le_result_t result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_TDSCDMA_RSSI,
        -1200, -250, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");

    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_TDSCDMA_RSSI, rssiDelta,
        phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Configurations on LTE signal indication.
 */
//--------------------------------------------------------------------------------------------------
void LteSignalConfiguration
(
    long phoneId,   ///< [IN] Phone ID.
    long rssiDelta, ///< [IN] RSSI delta.
    long rsrpDelta  ///< [IN] RSRP delta.
)
{
    le_result_t result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_LTE_RSSI,
        -1200, 0, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");
    result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_LTE_RSRP,
        -1400, -440, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");

    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_LTE_RSSI, rssiDelta, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_LTE_RSRP, rsrpDelta, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Configurations on NR5G signal indication.
 */
//--------------------------------------------------------------------------------------------------
void Nr5gSignalConfiguration
(
    long phoneId,  ///< [IN] Phone ID.
    long rsrpDelta ///< [IN] RSRP delta.
)
{
    le_result_t result = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_NR5G_RSRP,
        -1400, -440, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndThresholds - OK");

    result = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_NR5G_RSRP, rsrpDelta, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_SetSignalStrengthIndDelta - OK");
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
    char name[TAF_RADIO_NETWORK_NAME_MAX_LEN] = {0};

    taf_radio_Rat_t rat;
    result = taf_radio_GetRadioAccessTechInUse(&rat, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetRadioAccessTechInUse - OK");

    uint32_t cellId;
    uint32_t lac;
    uint8_t bsic = 0;

    uint16_t psc;

    uint16_t tac;
    uint32_t earFcn;
    uint32_t ta;
    uint16_t pscid;

    uint64_t nrCid;
    int32_t arFcn;
    int32_t nrTac;
    uint32_t pcid;

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
            break;
        case TAF_RADIO_RAT_UMTS:
            psc = taf_radio_GetServingCellScramblingCode(phoneId);
            LE_TEST_OK(true, "taf_radio_GetServingCellScramblingCode - OK");
            LE_INFO("Phone %d UMTS Primary Scrambling Code %d", phoneId, psc);
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
            break;
        case TAF_RADIO_RAT_NR5G:
            nrCid = taf_radio_GetServingNrCellId(phoneId);
            LE_TEST_OK(true, "taf_radio_GetServingNrCellId - uint64_t");
            LE_INFO("Phone %d NR Cell ID %llu", phoneId, nrCid);

            arFcn = taf_radio_GetServingCellNrArfcn(phoneId);
            LE_TEST_OK(true, "taf_radio_GetServingCellNrArfcn - int32_t");
            LE_INFO("Phone %d NR Absolute Radio Frequency Channel Number %d", phoneId, arFcn);

            nrTac = taf_radio_GetServingCellNrTracAreaCode(phoneId);
            LE_TEST_OK(true, "taf_radio_GetServingCellNrTracAreaCode - int32_t");
            LE_INFO("Phone %d NR Tracking Area Code %d", phoneId, nrTac);

            pcid = taf_radio_GetPhysicalServingNrCellId(phoneId);
            LE_TEST_OK(true, "taf_radio_GetPhysicalServingNrCellId - OK");
            LE_INFO("Phone %d NR5G Physical Serving Cell ID %d", phoneId, pcid);
            break;
        default:
            LE_INFO("Unavailble RAT %d for serving system.", rat);
            break;
    }

    result = taf_radio_GetCurrentNetworkName(name, TAF_RADIO_NETWORK_NAME_MAX_LEN, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetCurrentNetworkName - OK");
    LE_INFO("Phone %d current network name %s", phoneId, name);

    result = taf_radio_GetCurrentNetworkMccMnc(mccStr, TAF_RADIO_MCC_BYTES, mncStr,
        TAF_RADIO_MNC_BYTES, phoneId);
    LE_TEST_OK(result == LE_OK, "taf_radio_GetCurrentNetworkMccMnc - OK");
    LE_INFO("Phone %d current network MCC %s MNC %s", phoneId, mccStr, mncStr);
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
 * rejection, and Radio Access Technology change.
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

    tdscdmaSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_TDSCDMA,
       (taf_radio_SignalStrengthChangeHandlerFunc_t)TdscdmaSsChangeHandler, NULL);
    LE_TEST_OK(tdscdmaSsChangeHandlerRef != NULL, "taf_radio_AddSignalStrengthChangeHandler - OK");

    lteSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_LTE,
       (taf_radio_SignalStrengthChangeHandlerFunc_t)LteSsChangeHandler, NULL);
    LE_TEST_OK(lteSsChangeHandlerRef != NULL, "taf_radio_AddSignalStrengthChangeHandler - OK");

    nr5gSsChangeHandlerRef = taf_radio_AddSignalStrengthChangeHandler(TAF_RADIO_RAT_NR5G,
       (taf_radio_SignalStrengthChangeHandlerFunc_t)Nr5gSsChangeHandler, NULL);
    LE_TEST_OK(nr5gSsChangeHandlerRef != NULL, "taf_radio_AddSignalStrengthChangeHandler - OK");

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

    taf_radio_RemoveSignalStrengthChangeHandler(tdscdmaSsChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveSignalStrengthChangeHandler - OK");

    taf_radio_RemoveSignalStrengthChangeHandler(lteSsChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveSignalStrengthChangeHandler - OK");

    taf_radio_RemoveSignalStrengthChangeHandler(nr5gSsChangeHandlerRef);
    LE_TEST_OK(true, "taf_radio_RemoveSignalStrengthChangeHandler - OK");
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

    if (strncmp(cmd, "power", strlen("power")) == 0)
    {
        CheckArgs(3);
        LE_TEST_INFO("======== Power Test ========");

        long phoneId = strtol(le_arg_GetArg(1), NULL, 10);
        const char* power = le_arg_GetArg(2);

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

        long phoneId = strtol(le_arg_GetArg(1), NULL, 10);
        const char* op = le_arg_GetArg(2);

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

        long phoneId = strtol(le_arg_GetArg(1), NULL, 10);
        const char* op = le_arg_GetArg(2);

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

        long phoneId = strtol(le_arg_GetArg(1), NULL, 10);
        const char* op = le_arg_GetArg(2);

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

        long phoneId = strtol(le_arg_GetArg(1), NULL, 10);
        const char* op = le_arg_GetArg(2);

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
            CheckArgs(4);
            const char* rat = le_arg_GetArg(3);

            if (strncmp(rat, "gsm", strlen("gsm")) == 0)
            {
                CheckArgs(5);
                long rssiDelta = strtol(le_arg_GetArg(4), NULL, 10);
                GsmSignalConfiguration(phoneId, rssiDelta);
            }
            else if (strncmp(rat, "umts", strlen("umts")) == 0)
            {
                CheckArgs(5);
                long rssiDelta = strtol(le_arg_GetArg(4), NULL, 10);
                UmtsSignalConfiguration(phoneId, rssiDelta);
            }
            else if (strncmp(rat, "cdma", strlen("cdma")) == 0)
            {
                CheckArgs(5);
                long rssiDelta = strtol(le_arg_GetArg(4), NULL, 10);
                CdmaSignalConfiguration(phoneId, rssiDelta);
            }
            else if (strncmp(rat, "tdscdma", strlen("tdscdma")) == 0)
            {
                CheckArgs(5);
                long rssiDelta = strtol(le_arg_GetArg(4), NULL, 10);
                TdscdmaSignalConfiguration(phoneId, rssiDelta);
            }
            else if (strncmp(rat, "lte", strlen("lte")) == 0)
            {
                CheckArgs(6);
                long rssiDelta = strtol(le_arg_GetArg(4), NULL, 10);
                long rsrpDelta = strtol(le_arg_GetArg(5), NULL, 10);
                LteSignalConfiguration(phoneId, rssiDelta, rsrpDelta);
            }
            else if (strncmp(rat, "nr5g", strlen("nrg5")) == 0)
            {
                CheckArgs(5);
                long rsrpDelta = strtol(le_arg_GetArg(4), NULL, 10);
                Nr5gSignalConfiguration(phoneId, rsrpDelta);
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

        long phoneId = strtol(le_arg_GetArg(1), NULL, 10);

        PrintNgbrCellsInfo(phoneId);
    }
    else if (strncmp(cmd, "serving", strlen("serving")) == 0)
    {
        CheckArgs(2);
        LE_TEST_INFO("======== Serving Status Test ========");

        long phoneId = strtol(le_arg_GetArg(1), NULL, 10);

        PrintServingStatus(phoneId);
    }
    else if (strncmp(cmd, "scan", strlen("scan")) == 0)
    {
        CheckArgs(3);
        LE_TEST_INFO("======== Network Scan Test ========");

        long phoneId = strtol(le_arg_GetArg(1), NULL, 10);
        const char* mode = le_arg_GetArg(2);

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

        long phoneId = strtol(le_arg_GetArg(1), NULL, 10);
        const char* op = le_arg_GetArg(2);

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

        long time = strtol(le_arg_GetArg(1), NULL, 10);

        CreateHandlerTestThread();
        // Wait for handler's response.
        le_thread_Sleep(time);
        RemoveTestHandler();
    }
    else
    {
        PrintHelpMenu();
    }

    LE_TEST_EXIT;
}
