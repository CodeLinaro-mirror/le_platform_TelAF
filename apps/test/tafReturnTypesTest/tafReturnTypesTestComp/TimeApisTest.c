/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Time Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void timeRetTest_RunApis
(
  void
)
{
    le_result_t res;
    LE_TEST_INFO("timeRetTest_RunApis");

    //1.taf_time_GetTimeRef NULL scenario
    taf_time_TimeRef_t  time = taf_time_GetTimeRef(7);
    LE_TEST_OK(time == NULL,"***taf_time_GetTimeRef***-NULL");

    //2.taf_time_GetTime LE_BAD_PARAMETER scenario
    res = taf_time_GetTime(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_time_GetTime***-LE_BAD_PARAMETER");

    //3.taf_time_GetRefSystemTime LE_BAD_PARAMETER scenario
    res = taf_time_GetRefSystemTime(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_time_GetRefSystemTime***-LE_BAD_PARAMETER");

    //4.taf_time_GetRefGptpTime LE_BAD_PARAMETER scenario
    res = taf_time_GetRefGptpTime(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_time_GetRefGptpTime***-LE_BAD_PARAMETER");

    //5.taf_time_ReleaseTimeRef LE_BAD_PARAMETER scenario
    res = taf_time_ReleaseTimeRef(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_time_ReleaseTimeRef***-LE_BAD_PARAMETER");

    //6.taf_time_GetFailedLoops LE_BAD_PARAMETER scenario
    res = taf_time_GetFailedLoops(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_time_GetFailedLoops***-LE_BAD_PARAMETER");

    //7.taf_time_GetFailedLoops LE_FAULT scenario
    int32_t failedLoops =0;
    int64_t loopIntervalSec = 0;
    res = taf_time_GetFailedLoops(NULL,&failedLoops,&loopIntervalSec);
    LE_TEST_OK(res == LE_FAULT,"***taf_time_GetFailedLoops***-LE_FAULT");

    //8.taf_time_IsAvailable false scenario
    bool avail;
    avail = taf_time_IsAvailable(NULL);
    LE_TEST_OK(avail == false,"***taf_time_IsAvailable***-false");

    //9.taf_time_GetSystemTimeSourceID LE_FAULT scenario
    res = taf_time_GetSystemTimeSourceID(NULL);
    LE_TEST_OK(res == LE_FAULT,"***taf_time_GetSystemTimeSourceID***-LE_FAULT");

    //10.taf_time_GetTimeZone LE_BAD_PARAMETER scenario
    res = taf_time_GetTimeZone(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_time_GetTimeZone***-LE_BAD_PARAMETER");

    //11.taf_time_GetTimeZone LE_FAULT scenario
    int8_t timeZone = 0;
    res = taf_time_GetTimeZone(NULL,&timeZone);
    LE_TEST_OK(res == LE_FAULT,"***taf_time_GetTimeZone***-LE_FAULT");

    //12.taf_time_GetTimeDayAdj LE_BAD_PARAMETER scenario
	res = taf_time_GetTimeDayAdj(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"***taf_time_GetTimeDayAdj***-LE_BAD_PARAMETER");

    //13.taf_time_GetTimeDayAdj LE_FAULT scenario
    uint8_t Dayad = 0;
    res = taf_time_GetTimeDayAdj(NULL,&Dayad);
    LE_TEST_OK(res == LE_FAULT,"***taf_time_GetTimeDayAdj***-LE_FAULT");

    //14.taf_time_IsSourceValid false scenario
    avail = taf_time_IsSourceValid(NULL);
    LE_TEST_OK(avail == false,"***taf_time_IsSourceValid***-false");

    //15.taf_time_SetValidity LE_FAULT scenario
    res = taf_time_SetValidity(NULL,false);
    LE_TEST_OK(res == LE_FAULT,"***taf_time_SetValidity***-LE_FAULT");


}
