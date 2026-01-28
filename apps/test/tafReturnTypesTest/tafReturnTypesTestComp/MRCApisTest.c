/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate MRC Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void mrcRetTest_RunApis
(
   void
)
{
    le_result_t res;
    LE_TEST_INFO("mrcRetTest_RunApis");

    //1.taf_mrc_SendOtaEndMsg LE_BAD_PARAMETER scenario
    res = taf_mrc_SendOtaEndMsg(3);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_mrc_SendOtaEndMsg-LE_BAD_PARAMETER");

    //2.taf_mrc_SendSyncStatusMsg LE_FAULT scenario
    res = taf_mrc_SendSyncStatusMsg(3);
    LE_TEST_OK(res == LE_FAULT,"taf_mrc_SendSyncStatusMsg-LE_FAULT");

}
