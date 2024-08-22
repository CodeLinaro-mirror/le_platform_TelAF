/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <iostream>
#include <string>
#include <memory>
#include <ctime>
#include <setjmp.h>
#include "tafTime.hpp"

using namespace telux::platform;
using namespace telux::common;
using namespace telux::tafsvc;

#define TIMER_SAFECALL 5
DECLARE_SAFE_CALL();

/*======================================================================

 FUNCTION        taf_time_SetSystemTime

 DESCRIPTION     Set system REAL time.

 DEPENDENCIES    Initialization of Time Service

 PARAMETERS      [IN] const taf_time_TimeSpec_t * timeVal: Time in
                      seconds and nanoseconds.
                 [IN] bool notifySvc: Flag to indicate if the time is
                      set externally

 RETURN VALUE    le_result_t
                 LE_BAD_PARAMETER:     Invalid parameters.
                 LE_FAULT:             Fail.
                 LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_time_SetSystemTime
(
    const taf_time_TimeSpec_t * timeValPtr,
    bool ackTimeSvc
)
{
    taf_time_TimeSpec_t time;

    time.sec = timeValPtr->sec;
    time.nanosec = timeValPtr->nanosec;

    auto &tafTime = taf_Time::GetInstance();

    return tafTime.SetSystemTime(time,
                                TAF_TIME_SRC_NAME_EX_APP,
                                ackTimeSvc);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for time source status change registration.
 *
 * @return
 *  - taf_time_AddTimeSourceChangeHandlerRef_t Handler reference.
 */
//--------------------------------------------------------------------------------------------------

taf_time_TimeSourceChangeHandlerRef_t taf_time_AddTimeSourceChangeHandler
(
    taf_time_TimeSourceChangeHandlerFunc_t handlerPtr,
        ///< [IN] Handler function for time source status change registration.
    void* contextPtr
        ///< [IN] Handler context.
)
{
    auto &tafTime = taf_Time::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("TimeSourceChangeHandler",
        tafTime.timeSourceChangeId, tafTime.LayerTimeSourceChangeHandler,
        (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_time_TimeSourceChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for time source status change registration.
 */
//--------------------------------------------------------------------------------------------------
void taf_time_RemoveTimeSourceChangeHandler
(
    taf_time_TimeSourceChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

/*======================================================================

 FUNCTION        taf_time_GetTimeRef

 DESCRIPTION     Gets the reference object of a time source.

 DEPENDENCIES    Initialization of Time Service.

 PARAMETERS      [IN] taf_time_TimeSources_t sourceId: Time source ID

 RETURN VALUE
                - Reference to the time source instance.
                - NULL if not available.

 SIDE EFFECTS

======================================================================*/
taf_time_TimeRef_t taf_time_GetTimeRef
(
    taf_time_TimeSources_t sourceId  ///< Time source ID.
)
{
    auto &tafTime = taf_Time::GetInstance();
    return tafTime.GetTimeRef(sourceId);
}

/*======================================================================

 FUNCTION        taf_time_GetTime

 DESCRIPTION     Gets the time for this time reference and creates
                 related time information.

 DEPENDENCIES    Need to be called after the reference object was
                 Created.

 PARAMETERS      [IN] taf_time_TimeRef_t * timeSrcRef: reference
                      for related time source object.
                 [OUT] taf_time_TimeSpec_t * timeVal: Time in
                       seconds and nanoseconds.

 RETURN VALUE    le_result_t
                 LE_FAULT:        Something went wrong.
                 LE_UNAVAILABLE:  This reference time is currently not
                                  unavailable.
                 LE_UNSUPPORTED:  Not supported for current time source.
                 LE_OK:           Succeeded.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_time_GetTime
(
    taf_time_TimeRef_t timeSrcRef,
    taf_time_TimeSpec_t* timeValPtr
)
{
    auto &tafTime = taf_Time::GetInstance();
    return tafTime.GetTime(timeSrcRef, timeValPtr);
}

/*======================================================================

 FUNCTION        taf_time_GetRefSystemTime

 DESCRIPTION     Gets system time that created when related source
                 time was created.

 DEPENDENCIES    Need to be called after the reference object was
                 Created.

 PARAMETERS      [IN] taf_time_TimeRef_t * timeSrcRef: reference
                      for related time source object.
                 [OUT] taf_time_TimeSpec_t * timeVal: Time in
                       seconds and nanoseconds.

 RETURN VALUE    le_result_t
                 LE_FAULT:        Something went wrong.
                 LE_UNAVAILABLE:  This reference time is currently not
                                  unavailable.
                 LE_UNSUPPORTED:  Not supported for current time source.
                 LE_OK:           Succeeded.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_time_GetRefSystemTime
(
    taf_time_TimeRef_t timeSrcRef,
    taf_time_TimeSpec_t* timeValPtr
)
{
    auto &tafTime = taf_Time::GetInstance();
    return tafTime.GetRefSystemTime(timeSrcRef, timeValPtr);
}

/*======================================================================

 FUNCTION        taf_time_GetRefSystemTime

 DESCRIPTION     Gets gptp time that created when related source
                 time was created.

 DEPENDENCIES    Need to be called after this API:
                 taf_time_GetTime().

 PARAMETERS      [IN] taf_time_TimeRef_t * timeSrcRef: reference
                      for related time source object.
                 [OUT] taf_time_TimeSpec_t * timeVal: Time in
                       seconds and nanoseconds.

 RETURN VALUE    le_result_t
                 LE_FAULT:        Something went wrong.
                 LE_UNAVAILABLE:  This reference time is currently not
                                  unavailable.
                 LE_UNSUPPORTED:  Not supported for current time source.
                 LE_OK:           Succeeded.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_time_GetRefGptpTime
(
    taf_time_TimeRef_t timeSrcRef,
    taf_time_TimeSpec_t* timeValPtr
)
{
    auto &tafTime = taf_Time::GetInstance();
    return tafTime.GetRefGptpTime(timeSrcRef, timeValPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Release a time source reference.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_time_ReleaseTimeRef
(
    taf_time_TimeRef_t timeSrcRef
)
{
    auto &tafTime = taf_Time::GetInstance();
    return tafTime.ReleaseTimeRef(timeSrcRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for the time source and its reference object.
 *
 * @return
 *  - taf_time_TimeValueChangeHandlerRef_t Handler reference.
 */
//--------------------------------------------------------------------------------------------------
taf_time_TimeValueChangeHandlerRef_t taf_time_AddTimeValueChangeHandler
(
    taf_time_TimeSources_t sourceId,
        ///< [IN] Time source ID.
    taf_time_TimeValueChangeHandlerFunc_t handlerPtr,
        ///< [IN] Handler function pointer.
    void* contextPtr
        ///< [IN] Handler context.
)
{
    auto &time = taf_Time::GetInstance();
    return time.AddTimeValueChangeHandler(sourceId, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for time source reference object.
 */
//--------------------------------------------------------------------------------------------------
void taf_time_RemoveTimeValueChangeHandler
(
    taf_time_TimeValueChangeHandlerRef_t handlerRef
        ///< [IN] Handler Reference.
)
{
    auto &time = taf_Time::GetInstance();
    return time.RemoveTimeValueChangeHandler(handlerRef);
}


/*-------------------------------------------------------------------------

 FUNCTION        taf_time_GetRtcTimeReqAsync

 DESCRIPTION     Get time RTC device or VHAL interface in async mode.

 DEPENDENCIES    Initialization of Time Service

 PARAMETERS      [IN] taf_time_TimeSpec_t * timeVal: Time in
                      seconds and nanoseconds.
                 [IN] taf_time_AsyncSetTimeReqHandlerFunc_t: callback
                      handler function

 RETURN VALUE    le_result_t
                 LE_FAULT:             Fail.
                 LE_OK:                Success.
                 LE_UNSUPPORTED:   Not supported.

 SIDE EFFECTS

------------------------------------------------------------------------------*/
le_result_t taf_time_GetRtcTimeReqAsync(taf_time_AsyncGetTimeReqHandlerFunc_t handlerPtr,
        void* contextPtr)
{
    auto& time = taf_Time::GetInstance();
    return time.GetRtcTimeReqAsync(handlerPtr, contextPtr);
}

/*-------------------------------------------------------------------------

 FUNCTION        taf_time_SetRtcTimeReqAsync

 DESCRIPTION     Update the time to RTC device or VHAL interface in async mode.

 DEPENDENCIES    Initialization of Time Service

 PARAMETERS      [IN] taf_time_TimeSpec_t * timeVal: Time in
                      seconds and nanoseconds.
                 [IN] taf_time_AsyncSetTimeReqHandlerFunc_t: callback
                      handler function

 RETURN VALUE    le_result_t
                 LE_FAULT:             Fail.
                 LE_OK:                Success.
                 LE_UNSUPPORTED:   Not supported.

 SIDE EFFECTS
------------------------------------------------------------------------------*/
le_result_t taf_time_SetRtcTimeReqAsync(const taf_time_TimeSpec_t* timeValPtr,
        taf_time_AsyncSetTimeReqHandlerFunc_t handlerPtr, void* contextPtr)
{
    auto& time = taf_Time::GetInstance();
    return time.SetRtcTimeReqAsync(timeValPtr, handlerPtr, contextPtr);
}

/*-------------------------------------------------------------------------

 FUNCTION        taf_time_SetTimeToRtc

 DESCRIPTION     Update the time to RTC device or VHAL interface.

 DEPENDENCIES    Initialization of Time Service

 PARAMETERS      [IN] taf_time_TimeSpec_t * timeVal: Time in
                      seconds and nanoseconds.

 RETURN VALUE    le_result_t
                 LE_FAULT:             Fail.
                 LE_OK:                Success.

 SIDE EFFECTS

------------------------------------------------------------------------------*/
le_result_t taf_time_SetTimeToRtc
(
    const taf_time_TimeSpec_t* timeVal
)
{
    taf_time_TimeSpec_t time;
    time.sec = timeVal->sec;
    time.nanosec = timeVal->nanosec;

    auto& tafTime = taf_Time::GetInstance();
    return tafTime.SetTimeToRtc(time);
}

/*======================================================================

 FUNCTION        taf_time_GetSourceRef

 DESCRIPTION     Gets the reference object of a source.

 DEPENDENCIES    Initialization of Time Service.

 PARAMETERS      [IN] taf_time_TimeSources_t sourceId: Source ID

 RETURN VALUE
                - Reference to the time source instance.
                - NULL if not available.

 SIDE EFFECTS

======================================================================*/
taf_time_SourceRef_t taf_time_GetSourceRef
(
    taf_time_TimeSources_t sourceId  ///< Source ID.
)
{
    auto& tafTime = taf_Time::GetInstance();
    return tafTime.GetSourceRef(sourceId);
}

/*======================================================================

 FUNCTION        taf_time_GetFailedLoops

 DESCRIPTION     Gets the number of failed loops for a source.

 DEPENDENCIES    Initialization of Time Service.

 PARAMETERS      [IN] taf_time_SourceRef_t sourceRef: Source ref
                 [OUT] Number of failed loops for given source
                 [OUT] Loop interval time in seconds

 RETURN VALUE
                - LE_OK if successful.
                - LE_FAULT if any error occurs.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_time_GetFailedLoops
(
    taf_time_SourceRef_t sourceRef,
    int32_t* failedLoops,
    int64_t* loopIntervalSec
)
{
    auto& tafTime = taf_Time::GetInstance();
    return tafTime.GetFailedLoops(sourceRef, failedLoops, loopIntervalSec);
}


/*======================================================================

 FUNCTION        taf_time_IsAvailable

 DESCRIPTION     Gets the availability of a source.

 DEPENDENCIES    Initialization of Time Service.

 PARAMETERS      [IN] taf_time_SourceRef_t sourceRef: Source ref
                 [OUT] Availability of given source


 RETURN VALUE
                - LE_OK if successful.
                - LE_FAULT if any error occurs.

 SIDE EFFECTS

======================================================================*/
bool taf_time_IsAvailable
(
    taf_time_SourceRef_t sourceRef
)
{
    auto& tafTime = taf_Time::GetInstance();
    return tafTime.IsAvailable(sourceRef);
}


/*======================================================================

 FUNCTION        taf_time_GetSystemTimeSourceID

 DESCRIPTION     Gets the name of the time source that has set the
                 system time

 DEPENDENCIES    Initialization of Time Service.

 PARAMETERS      [OUT] taf_time_TimeSources_t sourceId: Source ID

 RETURN VALUE
                - LE_OK if successful.
                - LE_FAULT if any error occurs.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_time_GetSystemTimeSourceID
(
   taf_time_TimeSources_t *sourceId
)
{
    auto& tafTime = taf_Time::GetInstance();
    return tafTime.GetSystemTimeSourceID(sourceId);
}

/*======================================================================

 FUNCTION        taf_time_GetTimeZone

 DESCRIPTION     Gets the Offset from Universal time i.e. the difference
                 between local time and Universal time, in increments of
                 15 minutes (signed value). The time zone range is [-48, 48],
                 so the range of minutes is [-720, 720].

 DEPENDENCIES    Initialization of Time Service.

 PARAMETERS      [IN] taf_time_SourceRef_t sourceRef
                 [OUT] int8_t* timeZone

 RETURN VALUE
                - LE_OK if successful.
                - LE_FAULT if any error occurs.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_time_GetTimeZone
(
   taf_time_SourceRef_t sourceRef,
   int8_t* timeZone
)
{
    auto& tafTime = taf_Time::GetInstance();
    return tafTime.GetTimeZone(sourceRef, timeZone);
}

/*======================================================================

 FUNCTION        taf_time_GetTimeDayAdj

 DESCRIPTION     Gets the daylight saving adjustment in hours. Possible
                 values for the output are 0, 1, and 2.

 DEPENDENCIES    Initialization of Time Service.

 PARAMETERS      [IN] taf_time_SourceRef_t sourceRef
                 [OUT] int8_t* dayltSavAdj

 RETURN VALUE
                - LE_OK if successful.
                - LE_FAULT if any error occurs.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_time_GetTimeDayAdj
(
   taf_time_SourceRef_t sourceRef,
   uint8_t* dayltSavAdj
)
{
    auto& tafTime = taf_Time::GetInstance();
    return tafTime.GetTimeDayAdj(sourceRef, dayltSavAdj);
}

//-------------------------------------------------------------------------------------------------
/**
 * Initialization for Time Service.
*/
//-------------------------------------------------------------------------------------------------
void taf_time_service_int(void)
{
    LE_INFO("Time Service Init...");
    auto &time = taf_Time::GetInstance();
// load driver
    LE_INFO("Loading the driver");
    time.timeInf = (time_Inf_t*)taf_devMgr_LoadDrv(TAF_TIME_MODULE_NAME, nullptr);
    if (time.timeInf == nullptr)
    {
        LE_ERROR("Can not load the driver %s", TAF_TIME_MODULE_NAME);
        time.isDrvPresent = false;
    }
    else // successfully loaded
    {
        LE_INFO("Driver loaded successfully....");
        time.isDrvPresent = true;

        // init first
        int ret = 0;
        ENTER_SAFE_CALL(TIMER_SAFECALL, ret, (*(time.timeInf->InitHAL)));
        EXIT_SAFE_CALL();

        if (ret == -1)
        {
            LE_ERROR("Called InitHAL failed");
        }
    }
    time.Init();
    LE_INFO("Time Service ready");
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for the time source and its reference object.
 *
 * @return
 *  - taf_time_TimeSourceStatusHandlerRef_t Handler reference.
 */
 //--------------------------------------------------------------------------------------------------
taf_time_TimeSourceStatusHandlerRef_t taf_time_AddTimeSourceStatusHandler
(
    taf_time_SourceRef_t srcRef,
    taf_time_StatusEventType_t eventType,
    taf_time_TimeSourceStatusHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto& time = taf_Time::GetInstance();
    return time.AddTimeSourceStatusHandler(srcRef, eventType, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for time source reference object.
 */
 //--------------------------------------------------------------------------------------------------
void taf_time_RemoveTimeSourceStatusHandler
(
    taf_time_TimeSourceStatusHandlerRef_t handlerRef
)
{
    auto& time = taf_Time::GetInstance();
    return time.RemoveTimeSourceStatusHandler(handlerRef);
}

/*======================================================================

 FUNCTION        taf_time_IsSourceValid

 DESCRIPTION     Gets the validity of a source.

 DEPENDENCIES    Initialization of Time Service.

 PARAMETERS      [IN] taf_time_SourceRef_t sourceRef: Source ref
                 [OUT] Validity of given source


 RETURN VALUE
                - true if valid.
                - false if not invalid.

 SIDE EFFECTS

======================================================================*/
bool taf_time_IsSourceValid
(
    taf_time_SourceRef_t sourceRef
)
{
    auto& tafTime = taf_Time::GetInstance();
    return tafTime.IsSourceValid(sourceRef);
}

/*======================================================================

 FUNCTION        taf_time_SetValidity

 DESCRIPTION     Sets the validity of a source.

 DEPENDENCIES    Initialization of Time Service.

 PARAMETERS      [IN] taf_time_SourceRef_t sourceRef: Source ref
                 [IN] Validity to set given source


 RETURN VALUE
                - LE_OK if successful.
                - LE_FAULT if any error occurs.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_time_SetValidity
(
    taf_time_SourceRef_t sourceRef,
    bool validity
)
{
    auto& tafTime = taf_Time::GetInstance();
    return tafTime.SetValidity(sourceRef, validity);
}

/**
 * The initialization of TelAF Time Service component.
*/
COMPONENT_INIT
{
    taf_time_service_int();
    LE_INFO("TelAf time service initialization done\n");
}
