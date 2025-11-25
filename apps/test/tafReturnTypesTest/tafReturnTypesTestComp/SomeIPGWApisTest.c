/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate SomeIP gateway Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void someipgwRetTest_RunApis
(
  void
)
{
    le_result_t res;
    LE_TEST_INFO("someipgwRetTest_RunApis");

    //1.taf_someipClnt_GetClientIdEx LE_BAD_PARAMETER scenario
    res = taf_someipClnt_GetClientIdEx("SOMEIP",NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipClnt_GetClientIdEx***-LE_BAD_PARAMETER");

    //2.taf_someipClnt_RequestService NULL scenario
    taf_someipClnt_ServiceRef_t serviceRef;
    serviceRef = taf_someipClnt_RequestService(0xFFFF, 0x5678);
    LE_TEST_OK(serviceRef == NULL,"***taf_someipClnt_GetClientIdEx***-NULL");

    //3.taf_someipClnt_RequestServiceEx NULL scenario
    serviceRef = taf_someipClnt_RequestServiceEx(0xFFFF, 0x5678,"someip");
    LE_TEST_OK(serviceRef == NULL,"***taf_someipClnt_RequestServiceEx***-NULL");

    //4.taf_someipClnt_RequestServiceWithVersion NULL scenario
    serviceRef = taf_someipClnt_RequestServiceWithVersion(0xFFFF, 0x5678,1,1,"someip");
    LE_TEST_OK(serviceRef == NULL,"***taf_someipClnt_RequestServiceWithVersion***-NULL");

    //5.taf_someipClnt_ReleaseService LE_BAD_PARAMETER scenario
    res = taf_someipClnt_ReleaseService(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipClnt_ReleaseService***-LE_BAD_PARAMETER");

    //6.taf_someipClnt_GetState LE_BAD_PARAMETER scenario
    res = taf_someipClnt_GetState(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipClnt_GetState***-LE_BAD_PARAMETER");

    //7.taf_someipClnt_GetVersion LE_BAD_PARAMETER scenario
    res = taf_someipClnt_GetVersion(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipClnt_GetVersion***-LE_BAD_PARAMETER");

    //8.taf_someipClnt_CreateMsg NULL scenario
    taf_someipClnt_TxMsgRef_t tx;
    tx = taf_someipClnt_CreateMsg(NULL,0x7FFF);
    LE_TEST_OK(tx == NULL,"***taf_someipClnt_CreateMsg***-NULL");

    //9.taf_someipClnt_SetNonRet LE_BAD_PARAMETER scenario
    res = taf_someipClnt_SetNonRet(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipClnt_SetNonRet***-LE_BAD_PARAMETER");

    //10.taf_someipClnt_SetReliable LE_BAD_PARAMETER scenario
    res = taf_someipClnt_SetReliable(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipClnt_SetReliable***-LE_BAD_PARAMETER");

    //11.taf_someipClnt_SetTimeout LE_BAD_PARAMETER scenario
    res = taf_someipClnt_SetTimeout(NULL,5000);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipClnt_SetTimeout***-LE_BAD_PARAMETER");

    //12.taf_someipClnt_SetPayload LE_BAD_PARAMETER scenario
    static uint8_t itsData[20] = {0};
    static uint32_t itsSize = 20;
    res = taf_someipClnt_SetPayload(NULL,itsData,itsSize);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipClnt_SetPayload***-LE_BAD_PARAMETER");

    //13.taf_someipClnt_DeleteMsg LE_BAD_PARAMETER scenario
    res = taf_someipClnt_DeleteMsg(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipClnt_DeleteMsg***-LE_BAD_PARAMETER");

    //14.taf_someipClnt_EnableEventGroup LE_BAD_PARAMETER scenario
    res = taf_someipClnt_EnableEventGroup(NULL,0xFFFF,0x5678,TAF_SOMEIPDEF_ET_EVENT);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipClnt_EnableEventGroup***-LE_BAD_PARAMETER");

    //15.taf_someipClnt_DisableEventGroup LE_BAD_PARAMETER scenario
    res = taf_someipClnt_DisableEventGroup(NULL,0xFFFF);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipClnt_DisableEventGroup***-LE_BAD_PARAMETER");

    //16.taf_someipClnt_SubscribeEventGroup LE_BAD_PARAMETER scenario
    res = taf_someipClnt_SubscribeEventGroup(NULL,0xFFFF);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipClnt_SubscribeEventGroup***-LE_BAD_PARAMETER");

    //17.taf_someipClnt_UnsubscribeEventGroup LE_BAD_PARAMETER scenario
    res = taf_someipClnt_UnsubscribeEventGroup(NULL,0xFFFF);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipClnt_UnsubscribeEventGroup***-LE_BAD_PARAMETER");

    //18.taf_someipSvr_GetService NULL scenario
    taf_someipSvr_ServiceRef_t srv;
    srv = taf_someipSvr_GetService(0xFFFF,0x0000);
    LE_TEST_OK(srv == NULL,"***taf_someipSvr_GetService***-NULL");

    //19.taf_someipSvr_GetServiceEx NULL scenario
    srv = taf_someipSvr_GetServiceEx(0xFFFF,0x0000,"service");
    LE_TEST_OK(srv == NULL,"***taf_someipSvr_GetServiceEx***-NULL");

    //20.taf_someipSvr_SetServiceVersion LE_BAD_PARAMETER scenario
    res = taf_someipSvr_SetServiceVersion(NULL,1,1);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_SetServiceVersion***-LE_BAD_PARAMETER");

    //21.taf_someipSvr_SetServicePort LE_BAD_PARAMETER scenario
    res = taf_someipSvr_SetServicePort(NULL,1,1,true);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_SetServicePort***-LE_BAD_PARAMETER");

    //22.taf_someipSvr_OfferService LE_BAD_PARAMETER scenario
    res = taf_someipSvr_OfferService(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_OfferService***-LE_BAD_PARAMETER");

    //23.taf_someipSvr_StopOfferService LE_BAD_PARAMETER scenario
    res = taf_someipSvr_StopOfferService(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_StopOfferService***-LE_BAD_PARAMETER");

    //24.taf_someipSvr_EnableEvent LE_BAD_PARAMETER scenario
    res = taf_someipSvr_EnableEvent(NULL,0xFFFF,0x4568);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_EnableEvent***-LE_BAD_PARAMETER");

    //25.taf_someipSvr_SetEventType LE_BAD_PARAMETER scenario
    res = taf_someipSvr_SetEventType(NULL,0xFFFF,TAF_SOMEIPDEF_ET_FIELD);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_SetEventType***-LE_BAD_PARAMETER");

    //26.taf_someipSvr_SetEventCycleTime LE_BAD_PARAMETER scenario
    res = taf_someipSvr_SetEventCycleTime(NULL,0xFFFF,30000);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_SetEventCycleTime***-LE_BAD_PARAMETER");

    //27.taf_someipSvr_DisableEvent LE_BAD_PARAMETER scenario
    res = taf_someipSvr_DisableEvent(NULL,0xFFFF);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_DisableEvent***-LE_BAD_PARAMETER");

    //28.taf_someipSvr_OfferEvent LE_BAD_PARAMETER scenario
    res = taf_someipSvr_OfferEvent(NULL,0xFFFF);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_OfferEvent***-LE_BAD_PARAMETER");

    //29.taf_someipSvr_StopOfferEvent LE_BAD_PARAMETER scenario
    res = taf_someipSvr_StopOfferEvent(NULL,0xFFFF);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_StopOfferEvent***-LE_BAD_PARAMETER");

    //30.taf_someipSvr_Notify LE_BAD_PARAMETER scenario
    uint8_t termCmd[2] = { 0xff, 0xff };
    res = taf_someipSvr_Notify(NULL,0xFFFF,termCmd,2);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_Notify***-LE_BAD_PARAMETER");

    //31.taf_someipSvr_GetServiceId LE_BAD_PARAMETER scenario
    res = taf_someipSvr_GetServiceId(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_GetServiceId***-LE_BAD_PARAMETER");

    //32.taf_someipSvr_GetMethodId LE_BAD_PARAMETER scenario
    res = taf_someipSvr_GetMethodId(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_GetMethodId***-LE_BAD_PARAMETER");

    //33.taf_someipSvr_GetClientId LE_BAD_PARAMETER scenario
    res = taf_someipSvr_GetClientId(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_GetClientId***-LE_BAD_PARAMETER");

    //34.taf_someipSvr_GetMsgType LE_BAD_PARAMETER scenario
    res = taf_someipSvr_GetMsgType(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_GetMsgType***-LE_BAD_PARAMETER");

    //35.taf_someipSvr_GetPayloadSize LE_BAD_PARAMETER scenario
    res = taf_someipSvr_GetPayloadSize(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_GetPayloadSize***-LE_BAD_PARAMETER");

    //36.taf_someipSvr_GetPayloadData LE_BAD_PARAMETER scenario
    res = taf_someipSvr_GetPayloadData(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_GetPayloadData***-LE_BAD_PARAMETER");

    //37.taf_someipSvr_SendResponse LE_BAD_PARAMETER scenario
    static uint8_t PayloadData[TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE];
    res = taf_someipSvr_SendResponse(NULL,false,0,PayloadData,TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_SendResponse***-LE_BAD_PARAMETER");

    //38.taf_someipSvr_ReleaseRxMsg LE_BAD_PARAMETER scenario
    res = taf_someipSvr_ReleaseRxMsg(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_someipSvr_ReleaseRxMsg***-LE_BAD_PARAMETER");

}
