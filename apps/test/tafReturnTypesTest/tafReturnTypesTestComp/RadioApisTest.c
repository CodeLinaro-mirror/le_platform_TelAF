/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Radio Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void radioRetTest_RunApis
(
   void
)
{
    le_result_t res;
    LE_TEST_INFO("radioRetTest_RunApis");

   //1.taf_radio_SetRadioPower -LE_BAD_PARAMETER scenario
   res = taf_radio_SetRadioPower(2,1);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_SetRadioPower-LE_BAD_PARAMETER");

   //2.taf_radio_GetRadioPower -LE_BAD_PARAMETER scenario
   res = taf_radio_GetRadioPower(NULL,1);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetRadioPower-LE_BAD_PARAMETER");

   //3.taf_radio_SetAutomaticRegisterMode -LE_BAD_PARAMETER scenario
   res = taf_radio_SetAutomaticRegisterMode(0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_SetAutomaticRegisterMode-LE_BAD_PARAMETER");

   //4.taf_radio_SetManualRegisterMode -LE_BAD_PARAMETER scenario
   char mccStr[TAF_RADIO_MCC_BYTES] = {0};
   char mncStr[TAF_RADIO_MNC_BYTES] = {0};
   res = taf_radio_SetManualRegisterMode(mccStr,mncStr,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_SetManualRegisterMode-LE_BAD_PARAMETER");

   //5.taf_radio_GetRegisterMode -LE_BAD_PARAMETER scenario
   res = taf_radio_GetRegisterMode(NULL,NULL,0,NULL,0,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetRegisterMode-LE_BAD_PARAMETER");

   //6.taf_radio_GetBandCapabilities -LE_BAD_PARAMETER scenario
   res = taf_radio_GetBandCapabilities(NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetBandCapabilities-LE_BAD_PARAMETER");

   //7.taf_radio_GetLteBandCapabilities -LE_BAD_PARAMETER scenario
   res = taf_radio_GetLteBandCapabilities(NULL,NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetLteBandCapabilities-LE_BAD_PARAMETER");

   //8.taf_radio_SetBandPreferences -LE_BAD_PARAMETER scenario
   taf_radio_BandBitMask_t bandMask = TAF_RADIO_BAND_BIT_MASK_CLASS_1_ALL_BLOCKS;
   res = taf_radio_SetBandPreferences(bandMask,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_SetBandPreferences-LE_BAD_PARAMETER");

   //9.taf_radio_GetBandPreferences -LE_BAD_PARAMETER scenario
   res = taf_radio_GetBandPreferences(&bandMask,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetBandPreferences-LE_BAD_PARAMETER");

   //10.taf_radio_SetLteBandPreferences -LE_BAD_PARAMETER scenario
   uint64_t lteBand[TAF_RADIO_LTE_BAND_GROUP_NUM] = {0};
   res = taf_radio_SetLteBandPreferences(lteBand,0,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_SetLteBandPreferences-LE_BAD_PARAMETER");

   //11.taf_radio_GetLteBandPreferences -LE_BAD_PARAMETER scenario
   res = taf_radio_GetLteBandPreferences(NULL,0,3);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetLteBandPreferences-LE_BAD_PARAMETER");

   //12.taf_radio_AddPreferredOperator -LE_BAD_PARAMETER scenario
   taf_radio_RatBitMask_t ratMask = TAF_RADIO_RAT_BIT_MASK_GSM;
   res = taf_radio_AddPreferredOperator(mccStr,mncStr,ratMask,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_AddPreferredOperator-LE_BAD_PARAMETER");

   //13.taf_radio_RemovePreferredOperator -LE_BAD_PARAMETER scenario
   res = taf_radio_RemovePreferredOperator(mccStr,mncStr,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_RemovePreferredOperator-LE_BAD_PARAMETER");

   //14.taf_radio_DeletePreferredOperatorsList -LE_BAD_PARAMETER scenario
   res = taf_radio_DeletePreferredOperatorsList(NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_DeletePreferredOperatorsList-LE_BAD_PARAMETER");

   //15.taf_radio_GetPreferredOperatorsList -NULL scenario
   taf_radio_PreferredOperatorListRef_t list;
   list = taf_radio_GetPreferredOperatorsList(0);
   LE_TEST_OK(list == NULL,"taf_radio_GetPreferredOperatorsList-NULL");

   //16.taf_radio_GetFirstPreferredOperator -NULL scenario
   taf_radio_PreferredOperatorRef_t pref;
   pref = taf_radio_GetFirstPreferredOperator(NULL);
   LE_TEST_OK(pref == NULL,"taf_radio_GetFirstPreferredOperator-NULL");

   //17.taf_radio_GetNextPreferredOperator -NULL scenario
   pref = taf_radio_GetNextPreferredOperator(NULL);
   LE_TEST_OK(pref == NULL,"taf_radio_GetNextPreferredOperator-NULL");

   //18.taf_radio_GetPreferredOperatorDetails -LE_BAD_PARAMETER scenario
   res = taf_radio_GetPreferredOperatorDetails(NULL,NULL,0,NULL,0,NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetPreferredOperatorDetails-LE_BAD_PARAMETER");

   //19.taf_radio_GetRadioAccessTechInUse -LE_BAD_PARAMETER scenario
   res = taf_radio_GetRadioAccessTechInUse(NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetRadioAccessTechInUse-LE_BAD_PARAMETER");

   //20.taf_radio_SetRatPreferences -LE_BAD_PARAMETER scenario
   res = taf_radio_SetRatPreferences(ratMask,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_SetRatPreferences-LE_BAD_PARAMETER");

   //21.taf_radio_GetRatPreferences -LE_BAD_PARAMETER scenario
   res = taf_radio_GetRatPreferences(NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetRatPreferences-LE_BAD_PARAMETER");

   //22.taf_radio_GetNetRegState -LE_BAD_PARAMETER scenario
   res = taf_radio_GetNetRegState(NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetNetRegState-LE_BAD_PARAMETER");

   //23.taf_radio_GetPacketSwitchedState -LE_BAD_PARAMETER scenario
   res = taf_radio_GetPacketSwitchedState(NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetPacketSwitchedState-LE_BAD_PARAMETER");

   //24.taf_radio_GetServiceDomain -LE_BAD_PARAMETER scenario
   res = taf_radio_GetServiceDomain(NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetServiceDomain-LE_BAD_PARAMETER");

   //25.taf_radio_SetServiceDomainPreferences -LE_BAD_PARAMETER scenario
   taf_radio_ServiceDomainState_t domain = TAF_RADIO_SERVICE_DOMAIN_STATE_UNKNOWN;
   res = taf_radio_SetServiceDomainPreferences(domain,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_SetServiceDomainPreferences-LE_BAD_PARAMETER");

   //26.taf_radio_GetServiceDomainPreferences -LE_BAD_PARAMETER scenario
   res = taf_radio_GetServiceDomainPreferences(&domain,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetServiceDomainPreferences-LE_BAD_PARAMETER");

   //27.taf_radio_SetSignalStrengthIndThresholds -LE_BAD_PARAMETER scenario
   res = taf_radio_SetSignalStrengthIndThresholds(TAF_RADIO_SIG_TYPE_GSM_RSSI,-1110, -510,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_SetSignalStrengthIndThresholds-LE_BAD_PARAMETER");

   //28.taf_radio_SetSignalStrengthIndDelta -LE_BAD_PARAMETER scenario
   res = taf_radio_SetSignalStrengthIndDelta(TAF_RADIO_SIG_TYPE_GSM_RSSI,10,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_SetSignalStrengthIndDelta-LE_BAD_PARAMETER");

   //29.taf_radio_GetSignalQual -LE_BAD_PARAMETER scenario
   res = taf_radio_GetSignalQual(NULL,1);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetSignalQual-LE_BAD_PARAMETER");

   //30.taf_radio_MeasureSignalMetrics -NULL scenario
   taf_radio_MetricsRef_t metrics;
   metrics = taf_radio_MeasureSignalMetrics(0);
   LE_TEST_OK(metrics == NULL,"taf_radio_MeasureSignalMetrics-NULL");

   //31.taf_radio_DeleteSignalMetrics -LE_BAD_PARAMETER scenario
   res = taf_radio_DeleteSignalMetrics(NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_DeleteSignalMetrics-LE_BAD_PARAMETER");

   //32.taf_radio_GetRatOfSignalMetrics -NULL scenario
   ratMask = taf_radio_GetRatOfSignalMetrics(NULL);
   LE_TEST_OK(ratMask == 0x00,"taf_radio_GetRatOfSignalMetrics-NULL");

   //33.taf_radio_GetGsmSignalMetrics -LE_BAD_PARAMETER scenario
   res = taf_radio_GetGsmSignalMetrics(NULL,NULL,NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetGsmSignalMetrics-LE_BAD_PARAMETER");

   //34.taf_radio_GetUmtsSignalMetrics -LE_BAD_PARAMETER scenario
   res = taf_radio_GetUmtsSignalMetrics(NULL,NULL,NULL,NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetUmtsSignalMetrics-LE_BAD_PARAMETER");

   //35.taf_radio_GetLteSignalMetrics -LE_BAD_PARAMETER scenario
   res = taf_radio_GetLteSignalMetrics(NULL,NULL,NULL,NULL,NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetLteSignalMetrics-LE_BAD_PARAMETER");

   //36.taf_radio_GetCdmaSignalMetrics -LE_BAD_PARAMETER scenario
   res = taf_radio_GetCdmaSignalMetrics(NULL,NULL,NULL,NULL,NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetCdmaSignalMetrics-LE_BAD_PARAMETER");

   //37.taf_radio_GetCdmaSignalMetrics -LE_OK scenario
   int32_t ss,rsrq,rsrp,snr;
   taf_radio_MetricsRef_t met = taf_radio_MeasureSignalMetrics(1);
   res = taf_radio_GetCdmaSignalMetrics(met,&ss,&rsrq,&rsrp,&snr);
   LE_TEST_OK(res == LE_OK,"taf_radio_GetCdmaSignalMetrics-LE_OK");

   //38.taf_radio_GetNr5gSignalMetrics -LE_BAD_PARAMETER scenario
   res = taf_radio_GetNr5gSignalMetrics(NULL,NULL,NULL,NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetNr5gSignalMetrics-LE_BAD_PARAMETER");

   //39.taf_radio_GetServingCellId -UINT32_MAX scenario
   uint32_t Nr5g;
   Nr5g = taf_radio_GetServingCellId(0);
   LE_TEST_OK(Nr5g == UINT32_MAX,"taf_radio_GetServingCellId-UINT32_MAX");

   //40.taf_radio_GetServingNrCellId -UINT64_MAX scenario
   uint64_t NrCell;
   NrCell = taf_radio_GetServingNrCellId(0);
   LE_TEST_OK(NrCell == UINT64_MAX,"taf_radio_GetServingNrCellId-UINT64_MAX");

   //41.taf_radio_GetServingCellLocAreaCode -UINT32_MAX scenario
   Nr5g = taf_radio_GetServingCellLocAreaCode(0);
   LE_TEST_OK(Nr5g == UINT32_MAX,"taf_radio_GetServingCellLocAreaCode-UINT32_MAX");

   //42.taf_radio_GetServingCellLteTracAreaCode -UINT16_MAX scenario
   uint16_t CellLte;
   CellLte = taf_radio_GetServingCellLteTracAreaCode(0);
   LE_TEST_OK(CellLte == UINT16_MAX,"taf_radio_GetServingCellLteTracAreaCode-UINT16_MAX");

   //43.taf_radio_GetServingCellNrTracAreaCode -INT32_MAX scenario
   int32_t CellNr;
   CellNr = taf_radio_GetServingCellNrTracAreaCode(0);
   LE_TEST_OK(CellNr == INT32_MAX,"taf_radio_GetServingCellNrTracAreaCode-INT32_MAX");

   //43.taf_radio_GetServingCellEarfcn -UINT32_MAX scenario
   Nr5g = taf_radio_GetServingCellEarfcn(0);
   LE_TEST_OK(Nr5g == UINT32_MAX,"taf_radio_GetServingCellEarfcn-UINT32_MAX");

   //44.taf_radio_GetServingCellNrArfcn -INT32_MAX  scenario
   CellNr = taf_radio_GetServingCellNrArfcn(0);
   LE_TEST_OK(CellNr == INT32_MAX ,"taf_radio_GetServingCellNrArfcn-INT32_MAX ");

   //45.taf_radio_GetServingCellTimingAdvance -UINT32_MAX  scenario
   Nr5g = taf_radio_GetServingCellTimingAdvance(0);
   LE_TEST_OK(Nr5g == UINT32_MAX ,"taf_radio_GetServingCellTimingAdvance-UINT32_MAX ");

   //46.taf_radio_GetPhysicalServingLteCellId -UINT16_MAX  scenario
   CellLte = taf_radio_GetPhysicalServingLteCellId(0);
   LE_TEST_OK(CellLte == UINT16_MAX ,"taf_radio_GetPhysicalServingLteCellId-UINT16_MAX ");

   //47.taf_radio_GetPhysicalServingNrCellId -UINT32_MAX  scenario
   Nr5g = taf_radio_GetPhysicalServingNrCellId(0);
   LE_TEST_OK(Nr5g == UINT32_MAX ,"taf_radio_GetPhysicalServingNrCellId-UINT32_MAX ");

   //48.taf_radio_GetServingCellGsmBsic -LE_BAD_PARAMETER scenario
   res = taf_radio_GetServingCellGsmBsic(NULL,1);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetServingCellGsmBsic-LE_BAD_PARAMETER");

   //49.taf_radio_GetServingCellScramblingCode -UINT16_MAX scenario
   CellLte = taf_radio_GetServingCellScramblingCode(0);
   LE_TEST_OK(CellLte == UINT16_MAX,"taf_radio_GetServingCellScramblingCode-UINT16_MAX");

   //50.taf_radio_GetCurrentNetworkName -LE_BAD_PARAMETER scenario
   char shortOperatorStr[TAF_RADIO_NETWORK_NAME_MAX_LEN] = {0};
   res = taf_radio_GetCurrentNetworkName(shortOperatorStr,TAF_RADIO_NETWORK_NAME_MAX_LEN,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetCurrentNetworkName-LE_BAD_PARAMETER");

   //51.taf_radio_GetCurrentNetworkMccMnc -LE_BAD_PARAMETER scenario
   res = taf_radio_GetCurrentNetworkMccMnc(NULL,0,NULL,0,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetCurrentNetworkMccMnc-LE_BAD_PARAMETER");

   //52.taf_radio_PerformCellularNetworkScan -NULL scenario
   taf_radio_ScanInformationListRef_t Scan;
   Scan = taf_radio_PerformCellularNetworkScan(0);
   LE_TEST_OK(Scan == NULL,"taf_radio_PerformCellularNetworkScan-NULL");

   //52.taf_radio_GetFirstCellularNetworkScan -NULL scenario
   taf_radio_ScanInformationListRef_t listRef = NULL;
   taf_radio_ScanInformationRef_t scan_info;
   scan_info = taf_radio_GetFirstCellularNetworkScan(listRef);
   LE_TEST_OK(scan_info == NULL,"taf_radio_GetFirstCellularNetworkScan-NULL");

   //53.taf_radio_GetNextCellularNetworkScan -NULL scenario
   scan_info = taf_radio_GetNextCellularNetworkScan(listRef);
   LE_TEST_OK(scan_info == NULL,"taf_radio_GetNextCellularNetworkScan-NULL");

   //54.taf_radio_DeleteCellularNetworkScan -LE_BAD_PARAMETER scenario
   res = taf_radio_DeleteCellularNetworkScan(listRef);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_DeleteCellularNetworkScan-LE_BAD_PARAMETER");

   //55.taf_radio_GetCellularNetworkMccMnc -LE_BAD_PARAMETER scenario
   res = taf_radio_GetCellularNetworkMccMnc(scan_info,NULL,0,NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetCellularNetworkMccMnc-LE_BAD_PARAMETER");

   //56.taf_radio_GetCellularNetworkName -LE_BAD_PARAMETER scenario
   res = taf_radio_GetCellularNetworkName(scan_info,NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetCellularNetworkName-LE_BAD_PARAMETER");

   //57.taf_radio_GetCellularNetworkRat -TAF_RADIO_RAT_UNKNOWN scenario
   taf_radio_Rat_t rat;
   rat = taf_radio_GetCellularNetworkRat(scan_info);
   LE_TEST_OK(rat == TAF_RADIO_RAT_UNKNOWN,"taf_radio_GetCellularNetworkRat-TAF_RADIO_RAT_UNKNOWN");

   //58.taf_radio_IsCellularNetworkInUse -false scenario
   bool ret;
   ret = taf_radio_IsCellularNetworkInUse(scan_info);
   LE_TEST_OK(ret == false,"taf_radio_IsCellularNetworkInUse-false");

   //59.taf_radio_IsCellularNetworkAvailable -false scenario
   ret = taf_radio_IsCellularNetworkAvailable(scan_info);
   LE_TEST_OK(ret == false,"taf_radio_IsCellularNetworkAvailable-false");

   //60.taf_radio_IsCellularNetworkHome -false scenario
   ret = taf_radio_IsCellularNetworkHome(scan_info);
   LE_TEST_OK(ret == false,"taf_radio_IsCellularNetworkHome-false");

   //61.taf_radio_IsCellularNetworkForbidden -false scenario
   ret = taf_radio_IsCellularNetworkForbidden(scan_info);
   LE_TEST_OK(ret == false,"taf_radio_IsCellularNetworkForbidden-false");

   //62.taf_radio_PerformPciNetworkScan -NULL scenario
   taf_radio_PciScanInformationListRef_t Pci;
   Pci = taf_radio_PerformPciNetworkScan(ratMask,0);
   LE_TEST_OK(Pci == NULL,"taf_radio_PerformPciNetworkScan-NULL");

   //63.taf_radio_GetFirstPciScanInfo -NULL scenario
   taf_radio_PciScanInformationRef_t pci_info;
   taf_radio_PciScanInformationListRef_t pci_scan = NULL;
   pci_info = taf_radio_GetFirstPciScanInfo(pci_scan);
   LE_TEST_OK(pci_info == NULL,"taf_radio_GetFirstPciScanInfo-NULL");

   //64.taf_radio_GetNextPciScanInfo -NULL scenario
   pci_info = taf_radio_GetNextPciScanInfo(pci_scan);
   LE_TEST_OK(pci_info == NULL,"taf_radio_GetNextPciScanInfo-NULL");

   //65.taf_radio_DeletePciNetworkScan -LE_BAD_PARAMETER scenario
   res = taf_radio_DeletePciNetworkScan(pci_scan);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_DeletePciNetworkScan-LE_BAD_PARAMETER");

   //66.taf_radio_GetFirstPlmnInfo -NULL scenario
   taf_radio_PlmnInformationRef_t plmn;
   plmn = taf_radio_GetFirstPlmnInfo(pci_info);
   LE_TEST_OK(plmn == NULL,"taf_radio_GetFirstPlmnInfo-NULL");

   //67.taf_radio_GetNextPlmnInfo -NULL scenario
   plmn = taf_radio_GetNextPlmnInfo(pci_info);
   LE_TEST_OK(plmn == NULL,"taf_radio_GetNextPlmnInfo-NULL");

   //68.taf_radio_GetPciScanCellId -UINT16_MAX scenario
   uint16_t cellid;
   cellid = taf_radio_GetPciScanCellId(pci_info);
   LE_TEST_OK(cellid == UINT16_MAX,"taf_radio_GetPciScanCellId-UINT16_MAX");

   //68.taf_radio_GetPciScanGlobalCellId -UINT32_MAX scenario
   uint32_t global;
   global = taf_radio_GetPciScanGlobalCellId(pci_info);
   LE_TEST_OK(global == UINT32_MAX,"taf_radio_GetPciScanGlobalCellId-UINT32_MAX");

   //69.taf_radio_GetPciScanMccMnc -LE_BAD_PARAMETER scenario
   res = taf_radio_GetPciScanMccMnc(plmn,NULL,0,NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetPciScanMccMnc-LE_BAD_PARAMETER");

   //70.taf_radio_GetNeighborCellsInfo -NULL scenario
   taf_radio_NeighborCellsRef_t  nei = NULL;
   nei = taf_radio_GetNeighborCellsInfo(0);
   LE_TEST_OK(nei == NULL,"taf_radio_GetNeighborCellsInfo-NULL");

   //71.taf_radio_DeleteNeighborCellsInfo -LE_NOT_FOUND scenario
   res = taf_radio_DeleteNeighborCellsInfo(nei);
   LE_TEST_OK(res == LE_NOT_FOUND,"taf_radio_DeleteNeighborCellsInfo-LE_NOT_FOUND");

   //72.taf_radio_GetFirstNeighborCellInfo -NULL scenario
   taf_radio_CellInfoRef_t cellinfo;
   cellinfo = taf_radio_GetFirstNeighborCellInfo(nei);
   LE_TEST_OK(cellinfo == NULL,"taf_radio_GetFirstNeighborCellInfo-NULL");

   //73.taf_radio_GetNextNeighborCellInfo -NULL scenario
   cellinfo = taf_radio_GetNextNeighborCellInfo(nei);
   LE_TEST_OK(cellinfo == NULL,"taf_radio_GetNextNeighborCellInfo-NULL");

   //74.taf_radio_GetNeighborCellId -UINT64_MAX scenario
   taf_radio_CellInfoRef_t info = NULL;
   uint64_t val;
   val = taf_radio_GetNeighborCellId(info);
   LE_TEST_OK(val == UINT64_MAX,"taf_radio_GetNeighborCellId-UINT64_MAX");

   //75.taf_radio_GetNeighborCellLocAreaCode -UINT32_MAX  scenario
   Nr5g = taf_radio_GetNeighborCellLocAreaCode(info);
   LE_TEST_OK(Nr5g == UINT32_MAX,"taf_radio_GetNeighborCellLocAreaCode-UINT32_MAX");

   //76.taf_radio_GetNeighborCellRxLevel -INT32_MAX scenario
   CellNr = taf_radio_GetNeighborCellRxLevel(info);
   LE_TEST_OK(CellNr == INT32_MAX ,"taf_radio_GetNeighborCellRxLevel-INT32_MAX");

   //77.taf_radio_GetNeighborCellRxLevel -INT32_MAX scenario
   CellNr = taf_radio_GetNeighborCellRxLevel(info);
   LE_TEST_OK(CellNr == INT32_MAX ,"taf_radio_GetNeighborCellRxLevel-INT32_MAX");

   //78.taf_radio_GetNeighborCellRat -TAF_RADIO_RAT_UNKNOWN scenario
   rat = taf_radio_GetNeighborCellRat(info);
   LE_TEST_OK(rat == TAF_RADIO_RAT_UNKNOWN ,"taf_radio_GetNeighborCellRat-TAF_RADIO_RAT_UNKNOWN");

   //79.taf_radio_GetPhysicalNeighborLteCellId -UINT16_MAX scenario
   cellid = taf_radio_GetPhysicalNeighborLteCellId(info);
   LE_TEST_OK(cellid == UINT16_MAX ,"taf_radio_GetPhysicalNeighborLteCellId-UINT16_MAX");

   //80.taf_radio_GetPhysicalNeighborNrCellId -UINT32_MAX scenario
   Nr5g = taf_radio_GetPhysicalNeighborNrCellId(info);
   LE_TEST_OK(Nr5g == UINT32_MAX ,"taf_radio_GetPhysicalNeighborNrCellId-UINT32_MAX");

   //81.taf_radio_GetNeighborCellGsmBsic -LE_FAULT scenario
   res = taf_radio_GetNeighborCellGsmBsic(info,NULL);
   LE_TEST_OK(res == LE_FAULT ,"taf_radio_GetNeighborCellGsmBsic-LE_FAULT");

   //82.taf_radio_GetNetStatus -NULL scenario
   taf_radio_NetStatusRef_t net;
   net = taf_radio_GetNetStatus(0);
   LE_TEST_OK(net == NULL ,"taf_radio_GetNetStatus-NULL");

   //83.taf_radio_GetRatSvcStatus -LE_BAD_PARAMETER scenario
   res = taf_radio_GetRatSvcStatus(NULL,NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetRatSvcStatus-LE_BAD_PARAMETER");

   //84.taf_radio_GetLteCsCap -LE_BAD_PARAMETER scenario
   taf_radio_NetStatusRef_t csCap = NULL;
   res = taf_radio_GetLteCsCap(csCap,NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetLteCsCap-LE_BAD_PARAMETER");

   //85.taf_radio_GetLteCsCap -LE_BAD_PARAMETER scenario
   res = taf_radio_GetLteCsCap(csCap,NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_radio_GetLteCsCap-LE_BAD_PARAMETER");

   //86.taf_radio_GetIms -NULL scenario
   taf_radio_ImsRef_t ims;
   ims = taf_radio_GetIms(0);
   LE_TEST_OK(ims == NULL,"taf_radio_GetIms-NULL");

   //87.taf_radio_GetImsRegStatus -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetImsRegStatus(NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetImsRegStatus-LE_BAD_PARAMETER ");

   //88.taf_radio_GetImsSvcStatus -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetImsSvcStatus(NULL,TAF_RADIO_IMS_SVC_TYPE_SMS,NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetImsSvcStatus-LE_BAD_PARAMETER ");

   //89.taf_radio_GetImsPdpError -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetImsPdpError(NULL,NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetImsPdpError-LE_BAD_PARAMETER");

   //90.taf_radio_SetImsSvcCfg -LE_BAD_PARAMETER  scenario
   res = taf_radio_SetImsSvcCfg(NULL,TAF_RADIO_IMS_SVC_TYPE_SMS,true);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_SetImsSvcCfg-LE_BAD_PARAMETER");

   //91.taf_radio_SetImsUserAgent -LE_BAD_PARAMETER  scenario
   char userAgent[TAF_RADIO_IMS_USER_AGENT_BYTES] = "tafRadioUnitTest";
   res = taf_radio_SetImsUserAgent(NULL,userAgent);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_SetImsUserAgent-LE_BAD_PARAMETER");

   //92.taf_radio_GetImsUserAgent -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetImsUserAgent(NULL,userAgent,TAF_RADIO_IMS_USER_AGENT_BYTES);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetImsUserAgent-LE_BAD_PARAMETER");

   //93.taf_radio_GetNrDualConnectivityStatus -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetNrDualConnectivityStatus(NULL,NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetNrDualConnectivityStatus-LE_BAD_PARAMETER");

   //94.taf_radio_GetCurrentNetworkLongName -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetCurrentNetworkLongName(NULL,0,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetCurrentNetworkLongName-LE_BAD_PARAMETER");

   //95.taf_radio_GetHardwareSimConfig -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetHardwareSimConfig(NULL,NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetHardwareSimConfig-LE_BAD_PARAMETER");

   //96.taf_radio_GetHardwareSimRatCapabilities -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetHardwareSimRatCapabilities(NULL,NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetHardwareSimRatCapabilities-LE_BAD_PARAMETER");

   //97.taf_radio_SetSignalStrengthIndHysteresis -LE_BAD_PARAMETER  scenario
   res = taf_radio_SetSignalStrengthIndHysteresis(TAF_RADIO_SIG_TYPE_GSM_RSSI,100,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_SetSignalStrengthIndHysteresis-LE_BAD_PARAMETER");

   //98.taf_radio_SetSignalStrengthIndHysteresisTimer -LE_BAD_PARAMETER  scenario
   res = taf_radio_SetSignalStrengthIndHysteresisTimer(5000,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_SetSignalStrengthIndHysteresisTimer-LE_BAD_PARAMETER");

   //99.taf_radio_GetServingCellArfcn -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetServingCellArfcn(NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetServingCellArfcn-LE_BAD_PARAMETER");

   //100.taf_radio_GetServingCellUarfcn -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetServingCellUarfcn(NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetServingCellUarfcn-LE_BAD_PARAMETER");

   //101.taf_radio_SetOperatingMode -LE_BAD_PARAMETER  scenario
   res = taf_radio_SetOperatingMode(7,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_SetOperatingMode-LE_BAD_PARAMETER");

   //102.taf_radio_GetOperatingMode -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetOperatingMode(NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetOperatingMode-LE_BAD_PARAMETER");

   //103.taf_radio_GetServingCellRoutingAreaCode -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetServingCellRoutingAreaCode(NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetServingCellRoutingAreaCode-LE_BAD_PARAMETER");

   //104.taf_radio_GetServingCellBandInfo -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetServingCellBandInfo(NULL,NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetServingCellBandInfo-LE_BAD_PARAMETER");

   //105.taf_radio_GetServingCellLteBandInfo -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetServingCellLteBandInfo(NULL,NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetServingCellLteBandInfo-LE_BAD_PARAMETER");

   //106.taf_radio_GetServingCellNrBandInfo -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetServingCellNrBandInfo(NULL,NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetServingCellNrBandInfo-LE_BAD_PARAMETER");

   //107.taf_radio_GetNrIconType -LE_BAD_PARAMETER  scenario
   res = taf_radio_GetNrIconType(NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER ,"taf_radio_GetNrIconType-LE_BAD_PARAMETER");

}
