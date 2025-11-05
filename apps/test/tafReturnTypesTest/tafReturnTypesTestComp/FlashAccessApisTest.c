/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Flash Access Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void flashaccessRetTest_RunApis
(
  void
)
{
    le_result_t res;
    LE_TEST_INFO("flashaccessRetTest_RunApis");

    //1.taf_flash_MtdOpen LE_BAD_PARAMETER scenario
    res = taf_flash_MtdOpen("abl_xyz", TAF_FLASH_READ_WRITE, NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_MtdOpen-LE_BAD_PARAMETER,");

    //2.taf_flash_MtdClose LE_BAD_PARAMETER scenario
    res = taf_flash_MtdClose(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_MtdClose-LE_BAD_PARAMETER");

    //3.taf_flash_MtdInformation LE_BAD_PARAMETER scenario
    res = taf_flash_MtdInformation(NULL,NULL,NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_MtdInformation-LE_BAD_PARAMETER");

    //4.taf_flash_MtdEraseBlock LE_BAD_PARAMETER scenario
    res = taf_flash_MtdEraseBlock(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_MtdEraseBlock-LE_BAD_PARAMETER");

    //5.taf_flash_MtdReadPage LE_BAD_PARAMETER scenario
    res = taf_flash_MtdReadPage(NULL,0,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_MtdReadPage-LE_BAD_PARAMETER");

    //6.taf_flash_MtdWritePage LE_BAD_PARAMETER scenario
    uint8_t page[TAF_FLASH_MTD_PAGE_MAX_WRITE_SIZE] = { 0 };
    res = taf_flash_MtdWritePage(NULL,0,page,TAF_FLASH_MTD_PAGE_MAX_WRITE_SIZE);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_MtdWritePage-LE_BAD_PARAMETER");

    //7.taf_flash_MtdRead LE_BAD_PARAMETER scenario
    res = taf_flash_MtdRead(NULL,0,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_MtdRead-LE_BAD_PARAMETER");

    //8.taf_flash_MtdWrite LE_BAD_PARAMETER scenario
    res = taf_flash_MtdWrite(NULL,0,page,TAF_FLASH_MTD_PAGE_MAX_WRITE_SIZE);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_MtdWrite-LE_BAD_PARAMETER");

    //9.taf_flash_MtdErase LE_BAD_PARAMETER scenario
    res = taf_flash_MtdErase(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_MtdErase-LE_BAD_PARAMETER");

    //10.taf_flash_MtdIsBlockGood false scenario
    bool flashMtd;
    flashMtd = taf_flash_MtdIsBlockGood(NULL,0);
    LE_TEST_OK(flashMtd == false,"taf_flash_MtdIsBlockGood-false");

    //11.taf_flash_UbiOpen LE_BAD_PARAMETER scenario
    res = taf_flash_UbiOpen("abl_xyz",TAF_FLASH_READ_ONLY,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_UbiOpen-LE_BAD_PARAMETER");

    //12.taf_flash_UbiClose LE_BAD_PARAMETER scenario
    res = taf_flash_UbiClose(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_UbiClose-LE_BAD_PARAMETER");

    //13.taf_flash_UbiInformation LE_BAD_PARAMETER scenario
    res = taf_flash_UbiInformation(NULL,NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_UbiInformation-LE_BAD_PARAMETER");

    //14.taf_flash_UbiRead LE_BAD_PARAMETER scenario
    res = taf_flash_UbiRead(NULL,0,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_UbiRead-LE_BAD_PARAMETER");

    //15.taf_flash_UbiInitWrite LE_BAD_PARAMETER scenario
    res = taf_flash_UbiInitWrite(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_UbiInitWrite-LE_BAD_PARAMETER");

    //16.taf_flash_UbiWrite LE_BAD_PARAMETER scenario
    uint8_t block[TAF_FLASH_UBI_MAX_WRITE_SIZE] = { 0 };
    size_t blockSize = TAF_FLASH_UBI_MAX_READ_SIZE;
    res = taf_flash_UbiWrite(NULL,block,blockSize);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_UbiWrite-LE_BAD_PARAMETER");

    //17.taf_flash_UbiErase LE_BAD_PARAMETER scenario
    res = taf_flash_UbiErase(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_flash_UbiErase-LE_BAD_PARAMETER");

}
