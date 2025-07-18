/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Power Manager Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void pmRetTest_RunApis
(
    void
)
{
   le_result_t res;
   LE_TEST_INFO("pmRetTest_RunApis");

   //1.taf_pm_StayAwake -LE_BAD_PARAMETER scenario
   res = taf_pm_StayAwake(NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_pm_StayAwake-LE_BAD_PARAMETER");

   //2.taf_pm_Relax -LE_BAD_PARAMETER scenario
   res = taf_pm_Relax(NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_pm_Relax-LE_BAD_PARAMETER");

   //3.taf_pm_GetFirstMachineName -LE_BAD_PARAMETER scenario
   char name[32] = {0};
   res = taf_pm_GetFirstMachineName(NULL,name,32);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_pm_GetFirstMachineName-LE_BAD_PARAMETER");

   //4.taf_pm_GetNextMachineName -LE_BAD_PARAMETER scenario
   res = taf_pm_GetNextMachineName(NULL,name,32);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_pm_GetNextMachineName-LE_BAD_PARAMETER");

   //5.taf_pm_DeleteMachineList -LE_BAD_PARAMETER scenario
   res = taf_pm_DeleteMachineList(NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_pm_DeleteMachineList-LE_BAD_PARAMETER");

   //6.taf_pm_GetNackClientInfo -LE_BAD_PARAMETER scenario
   res = taf_pm_GetNackClientInfo(NULL,NULL,0);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_pm_GetNackClientInfo-LE_BAD_PARAMETER");

   //7.taf_pm_GetUnrespClientInfo -LE_BAD_PARAMETER scenario
   res = taf_pm_GetUnrespClientInfo(NULL,NULL,NULL);
   LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_pm_GetUnrespClientInfo-LE_BAD_PARAMETER");

   //8.taf_pm_SetModemWakeupSel LE_OK = scenario
   res = taf_pm_SetModemWakeupSel(TAF_PM_NODE_MODEM_WS_BIT_MASK_SMS);
   LE_TEST_OK(res == LE_OK,"taf_pm_SetModemWakeupSel-LE_NOT_IMPLEMENTED =");
   LE_TEST_INFO("chuda :%d",(int) res);

   //9.taf_pm_GetModemWakeupSel LE_OK = scenario
   taf_pm_NodeModemWsBitMask_t bitset;
   res = taf_pm_GetModemWakeupSel(&bitset);
   LE_TEST_OK(res == LE_OK,"taf_pm_GetModemWakeupSel-LE_NOT_IMPLEMENTED");

   //10.taf_pm_GetModemAwakeReason LE_OK = scenario
   taf_pm_NodeModemWsBitMask_t wsBitmaskPtr;
   res = taf_pm_GetModemAwakeReason(&wsBitmaskPtr);
   LE_TEST_OK(res == LE_OK,"taf_pm_GetModemAwakeReason-LE_NOT_IMPLEMENTED");
}
