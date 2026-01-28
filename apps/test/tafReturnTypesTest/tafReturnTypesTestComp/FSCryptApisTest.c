/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate FS Crypt Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void fscryptRetTest_RunApis
(
   void
)
{
    le_result_t res;
    LE_TEST_INFO("fscryptRetTest_RunApis");

    //1.taf_fsc_GetStorageRef NULL scenario
    static char DIR_TEST[TAF_FSC_MAX_STORAGE_NAME_SIZE/2] = "/persist/fs-crypt";
    taf_fsc_StorageRef_t fsc;
    fsc = taf_fsc_GetStorageRef(DIR_TEST,NULL);
    LE_TEST_OK(fsc == NULL,"taf_fsc_GetStorageRef-NULL");

    //2.taf_fsc_LockStorage LE_BAD_PARAMETER scenario
    res = taf_fsc_LockStorage(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_fsc_LockStorage-LE_BAD_PARAMETER");

    //3.taf_fsc_UnlockStorage LE_BAD_PARAMETER scenario
    res = taf_fsc_UnlockStorage(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_fsc_UnlockStorage-LE_BAD_PARAMETER");

    //4.taf_fsc_DeleteStorage LE_BAD_PARAMETER scenario
    res = taf_fsc_DeleteStorage(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_fsc_DeleteStorage-LE_BAD_PARAMETER");

}
