/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate SIM Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void simRetTest_RunApis
(
   void
)
{
    le_result_t result;
    bool sim_id;
    LE_TEST_INFO("simRetTest_RunApis");

   //1.taf_sim_CreateSession- LE_BAD_PARAMETER scenario
    result = taf_sim_CreateSession(TAF_SIM_SESSION_TYPE_SEC_GW_PROV,NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_sim_CreateSession***-LE_BAD_PARAMETER ");

    //2.taf_sim_CreateSession- false scenario
    sim_id = taf_sim_IsPresent(3);
    LE_TEST_OK(sim_id == false , "***taf_sim_IsPresent***-false ");

    //3.taf_sim_IsReady- false scenario
    sim_id = taf_sim_IsReady(3);
    LE_TEST_OK(sim_id == false , "***taf_sim_IsReady***-false ");

    //4.taf_sim_GetICCID- LE_BAD_PARAMETER scenario
    result = taf_sim_GetICCID(TAF_SIM_UNSPECIFIED,NULL,TAF_SIM_ICCID_BYTES);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_sim_GetICCID***-LE_BAD_PARAMETER ");

    //5.taf_sim_GetICCID- LE_OVERFLOW scenario
    char iccid[TAF_SIM_ICCID_BYTES];
    result = taf_sim_GetICCID(TAF_SIM_UNSPECIFIED,iccid,TAF_SIM_ICCID_BYTES -1);
    LE_TEST_OK(result == LE_OVERFLOW , "***taf_sim_GetICCID***-LE_OVERFLOW ");

    //6.taf_sim_GetIMSI- LE_BAD_PARAMETER scenario
    result = taf_sim_GetIMSI(TAF_SIM_UNSPECIFIED,NULL,TAF_SIM_ICCID_BYTES);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_sim_GetIMSI***-LE_BAD_PARAMETER ");

    //7.taf_sim_GetIMSI- LE_OVERFLOW scenario
    char imsi[TAF_SIM_IMSI_BYTES];
    result = taf_sim_GetIMSI(TAF_SIM_UNSPECIFIED,imsi,TAF_SIM_IMSI_BYTES -1);
    LE_TEST_OK(result == LE_OVERFLOW , "***taf_sim_GetIMSI***-LE_OVERFLOW ");

    //8.taf_sim_GetIMSI- LE_BAD_PARAMETER scenario
    result = taf_sim_GetHomeNetworkOperator(TAF_SIM_UNSPECIFIED,NULL,50);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_sim_GetHomeNetworkOperator***-LE_BAD_PARAMETER ");

    //9.taf_sim_GetSubscriberPhoneNumber- LE_BAD_PARAMETER scenario
    result = taf_sim_GetSubscriberPhoneNumber(TAF_SIM_UNSPECIFIED,NULL,TAF_SIM_PHONE_NUM_MAX_BYTES);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_sim_GetSubscriberPhoneNumber***-LE_BAD_PARAMETER ");

    //10.taf_sim_GetSubscriberPhoneNumber- LE_OVERFLOW scenario
    char phoneNumber[TAF_SIM_PHONE_NUM_MAX_BYTES];
    result = taf_sim_GetSubscriberPhoneNumber(TAF_SIM_UNSPECIFIED,phoneNumber,TAF_SIM_PHONE_NUM_MAX_BYTES -1);
    LE_TEST_OK(result == LE_OVERFLOW , "***taf_sim_GetSubscriberPhoneNumber***-LE_OVERFLOW");

    //11.taf_sim_GetHomeNetworkMccMnc- LE_BAD_PARAMETER scenario
    result = taf_sim_GetHomeNetworkMccMnc(TAF_SIM_UNSPECIFIED,NULL,4,NULL,4);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_sim_GetHomeNetworkMccMnc***-LE_BAD_PARAMETER");

    //12.taf_sim_GetRemainingPUKTries - LE_BAD_PARAMETER scenario
    result = taf_sim_GetRemainingPUKTries(TAF_SIM_UNSPECIFIED,NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER , "***taf_sim_GetRemainingPUKTries***-LE_BAD_PARAMETER");

    //13.taf_sim_GetAppTypes - LE_BAD_PARAMETER scenario
    result = taf_sim_GetAppTypes(TAF_SIM_UNSPECIFIED,NULL,0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_sim_GetAppTypes-LE_BAD_PARAMETER");

    //14.taf_sim_OpenLogicalChannel - LE_BAD_PARAMETER scenario
    result = taf_sim_OpenLogicalChannel(TAF_SIM_UNSPECIFIED,TAF_SIM_APPTYPE_USIM,NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_sim_OpenLogicalChannel-LE_BAD_PARAMETER");

    //15.taf_sim_OpenLogicalChannelByAid - LE_BAD_PARAMETER scenario
    const char* aid ="test";
    result = taf_sim_OpenLogicalChannelByAid(TAF_SIM_UNSPECIFIED,aid,NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_sim_OpenLogicalChannelByAid-LE_BAD_PARAMETER");

    //17.taf_sim_SendApduOnChannel - LE_BAD_PARAMETER scenario
    uint8_t selectMFAPDU[] = {0x00, 0xA4, 0x00, 0x0C, 0x02, 0x3F, 0x00};
    result = taf_sim_SendApduOnChannel(TAF_SIM_UNSPECIFIED,2,selectMFAPDU,0,NULL,0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_sim_SendApduOnChannel-LE_BAD_PARAMETER");

    //18.taf_sim_SendApdu - LE_BAD_PARAMETER scenario
    result = taf_sim_SendApdu(TAF_SIM_UNSPECIFIED,selectMFAPDU,sizeof(selectMFAPDU),NULL,0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_sim_SendApdu-LE_BAD_PARAMETER");

    //19.taf_sim_SendCommand - LE_BAD_PARAMETER scenario
    char fileIdentifier[5]={'2', 'f', 'e', '2', '\0'};
    uint8_t data[1];
    char filePath[5] = {'3', 'F', '0', '0', '\0'};
    result = taf_sim_SendCommand(TAF_SIM_UNSPECIFIED,TAF_SIM_GET_RESPONSE,fileIdentifier,0,0,15,
                                 data,sizeof(data)/sizeof(data[0]),filePath,0,0,NULL,0);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_sim_SendCommand-LE_BAD_PARAMETER");

    //20.taf_sim_LocalSwapToEmergencyCallSubscription - LE_UNSUPPORTED scenario
    result = taf_sim_LocalSwapToEmergencyCallSubscription(TAF_SIM_UNSPECIFIED,TAF_SIM_MORPHO);
    LE_TEST_OK(result == LE_UNSUPPORTED, "taf_sim_LocalSwapToEmergencyCallSubscription-LE_UNSUPPORTED");

    //21.taf_sim_LocalSwapToCommercialSubscription - LE_UNSUPPORTED scenario
    result = taf_sim_LocalSwapToCommercialSubscription(TAF_SIM_UNSPECIFIED,TAF_SIM_MORPHO);
    LE_TEST_OK(result == LE_UNSUPPORTED, "taf_sim_LocalSwapToCommercialSubscription-LE_UNSUPPORTED");

    //22.taf_sim_SetPower - LE_BAD_PARAMETER scenario
    result = taf_sim_SetPower(TAF_SIM_UNSPECIFIED,2);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_sim_SetPower-LE_BAD_PARAMETER");

    //23.taf_sim_IsEmergencyCallSubscriptionSelected( - LE_BAD_PARAMETER scenario
    result = taf_sim_IsEmergencyCallSubscriptionSelected(TAF_SIM_UNSPECIFIED,NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_sim_IsEmergencyCallSubscriptionSelected(-LE_BAD_PARAMETER");

    //24.taf_sim_GetFirstFPLMNOperator -LE_OVERFLOW scenario
    taf_sim_FPLMNListRef_t FPLMNList = taf_sim_ReadFPLMNList(TAF_SIM_UNSPECIFIED);
    result = taf_sim_GetFirstFPLMNOperator(FPLMNList,NULL,0,NULL,0);
    LE_TEST_OK(result == LE_OVERFLOW, "taf_sim_GetFirstFPLMNOperator-LE_OVERFLOW");

    //24.taf_sim_GetNextFPLMNOperator -LE_OVERFLOW scenario
    result = taf_sim_GetNextFPLMNOperator(FPLMNList,NULL,0,NULL,0);
    LE_TEST_OK(result == LE_OVERFLOW, "taf_sim_GetNextFPLMNOperator-LE_OVERFLOW");

    //25.taf_sim_GetSlotCount -LE_FAULT scenario
    result = taf_sim_GetSlotCount(NULL);
    LE_TEST_OK(result == LE_FAULT, "taf_sim_GetSlotCount-LE_FAULT");

}
