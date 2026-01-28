/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"
#include "UpdateApisTest.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Update Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void updateRetTest_RunApis
(
   void
)
{
    le_result_t res;
    LE_TEST_INFO("updateRetTest_RunApis");

    //1.taf_update_GetDownloadSession LE_BAD_PARAMETER scenario
    res = taf_update_GetDownloadSession(SESSION_CONF_FILE, NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_update_GetDownloadSession-LE_BAD_PARAMETER");

    //2.taf_update_StartDownload LE_UNSUPPORTED_LE_FAULT scenario
    res = taf_update_StartDownload(NULL);
    LE_TEST_OK(((res == LE_UNSUPPORTED) | (res == LE_FAULT)),"taf_update_StartDownload-LE_UNSUPPORTED_LE_FAULT");

    //3.taf_update_PauseDownload LE_UNSUPPORTED_LE_FAULT scenario
    res = taf_update_PauseDownload(NULL);
    LE_TEST_OK(((res == LE_UNSUPPORTED) | (res == LE_FAULT)),"taf_update_PauseDownload-LE_UNSUPPORTED_LE_FAULT");

    //4.taf_update_ResumeDownload LE_UNSUPPORTED_LE_FAULT scenario
    res = taf_update_ResumeDownload(NULL);
    LE_TEST_OK(((res == LE_UNSUPPORTED) | (res == LE_FAULT)),"taf_update_ResumeDownload-LE_UNSUPPORTED_LE_FAULT");

    //5.taf_update_CancelDownload LE_UNSUPPORTED_LE_FAULT scenario
    res = taf_update_CancelDownload(NULL);
    LE_TEST_OK(((res == LE_UNSUPPORTED) | (res == LE_FAULT)),"taf_update_CancelDownload-LE_UNSUPPORTED_LE_FAULT");

    //6.taf_update_GetInstallationSession LE_BAD_PARAMETER scenario
    res = taf_update_GetInstallationSession(5,SESSION_CONF_FILE,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_update_GetInstallationSession-LE_BAD_PARAMETER");

    //7.taf_update_GetInstallationSession LE_UNSUPPORTED scenario
    taf_update_SessionRef_t sessRef = NULL;
    res = taf_update_GetInstallationSession(5,SESSION_CONF_FILE,&sessRef);
    LE_TEST_OK(res == LE_UNSUPPORTED,"taf_update_GetInstallationSession-LE_UNSUPPORTED");

    //8.taf_update_InstallPreCheck LE_FAULT scenario
    res =  taf_update_InstallPreCheck(NULL, IMAGE_VERSION_FILE);
    LE_TEST_OK(res == LE_FAULT,"taf_update_InstallPreCheck-LE_FAULT");

    //9.taf_update_StartInstall LE_FAULT scenario
    res =  taf_update_StartInstall(NULL, IMAGE_VERSION_FILE);
    LE_TEST_OK(res == LE_FAULT,"taf_update_StartInstall-LE_FAULT");

    //10.taf_update_PauseInstall LE_FAULT scenario
    res =  taf_update_PauseInstall(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_update_PauseInstall-LE_FAULT");

    //11.taf_update_ResumeInstall LE_FAULT scenario
    res =  taf_update_ResumeInstall(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_update_ResumeInstall-LE_FAULT");

    //12.taf_update_CancelInstall LE_FAULT scenario
    res =  taf_update_CancelInstall(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_update_CancelInstall-LE_FAULT");

    //13.taf_update_InstallPostCheck LE_FAULT scenario
    res =  taf_update_InstallPostCheck(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_update_InstallPostCheck-LE_FAULT");

    //14.taf_update_GetActiveBank LE_FAULT scenario
    taf_update_Bank_t bank = TAF_UPDATE_BANK_UNKNOWN;
    res =  taf_update_GetActiveBank(NULL,&bank);
    LE_TEST_OK(res == LE_FAULT,"taf_update_GetActiveBank-LE_FAULT");

    //15.taf_update_EraseBank LE_FAULT scenario
    res =  taf_update_EraseBank(NULL,bank);
    LE_TEST_OK(res == LE_FAULT,"taf_update_EraseBank-LE_FAULT");

    //16.taf_update_VerifyActivation LE_FAULT scenario
    res =  taf_update_VerifyActivation(NULL,IMAGE_VERSION_FILE);
    LE_TEST_OK(res == LE_FAULT,"taf_update_VerifyActivation-LE_FAULT");

    //17.taf_update_PauseActivation LE_FAULT scenario
    res =  taf_update_PauseActivation(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_update_PauseActivation-LE_FAULT");

    //18.taf_update_ResumeActivation LE_FAULT scenario
    res =  taf_update_ResumeActivation(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_update_ResumeActivation-LE_FAULT");

    //19.taf_update_Rollback LE_FAULT scenario
    res =  taf_update_Rollback(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_update_Rollback-LE_FAULT");

    //20.taf_update_Sync LE_FAULT scenario
    res =  taf_update_Sync(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_update_Sync-LE_FAULT");

    //21.taf_update_StartSync LE_FAULT scenario
    res =  taf_update_StartSync(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_update_StartSync-LE_FAULT");

    //22.taf_update_PauseSync LE_FAULT scenario
    res =  taf_update_PauseSync(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_update_PauseSync-LE_FAULT");

    //23.taf_update_ResumeSync LE_FAULT scenario
    res =  taf_update_ResumeSync(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_update_ResumeSync-LE_FAULT");

    //24.taf_update_CancelSync LE_FAULT scenario
    res =  taf_update_CancelSync(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_update_CancelSync-LE_FAULT");

    //25.taf_fwupdate_GetFirmwareVersion LE_BAD_PARAMETER, scenario
    res = taf_fwupdate_GetFirmwareVersion(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_fwupdate_GetFirmwareVersion-LE_BAD_PARAMETER,");
}
