/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include <iostream>
#include <string>
#include <memory>
#include <ctime>
#include <setjmp.h>
#include "tafTime.hpp"

using namespace tafsvc;

#define TIMER_SAFECALL 5
DECLARE_SAFE_CALL();

//--------------------------------------------------------------------------------------------------
/**
 * Register a handler to be notified when the system time source changes.
 *
 * @return
 *  - taf_time_AddTimeSourceChangeHandlerRef_t Handler reference.
 */
//--------------------------------------------------------------------------------------------------

taf_time_TimeSourceChangeHandlerRef_t taf_time_AddTimeSourceChangeHandler
(
    taf_time_TimeSourceChangeHandlerFunc_t handlerPtr,
        ///< [IN] Handler function for system time source status change registration.
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
    TAF_ERROR_IF_RET_VAL(timeValPtr == NULL, LE_BAD_PARAMETER, "timeValPtr is NULL");
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
    TAF_ERROR_IF_RET_VAL(timeValPtr == NULL, LE_BAD_PARAMETER, "timeValPtr is NULL");
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
    TAF_ERROR_IF_RET_VAL(timeValPtr == NULL, LE_BAD_PARAMETER, "timeValPtr is NULL");
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
    TAF_ERROR_IF_RET_VAL(failedLoops == NULL, LE_BAD_PARAMETER, "failedLoops is NULL");
    TAF_ERROR_IF_RET_VAL(loopIntervalSec == NULL, LE_BAD_PARAMETER, "loopIntervalSec is NULL");
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
    TAF_ERROR_IF_RET_VAL(timeZone == NULL, LE_BAD_PARAMETER, "timeZone is NULL");
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
    TAF_ERROR_IF_RET_VAL(dayltSavAdj == NULL, LE_BAD_PARAMETER, "dayltSavAdj is NULL");
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
    auto &time = taf_Time::GetInstance();
// load driver
    LE_DEBUG("Loading the driver");
    time.timeInf = (time_Inf_t*)taf_devMgr_LoadDrv(TAF_TIME_MODULE_NAME, nullptr);
    if (time.timeInf == nullptr)
    {
        LE_WARN("Can not load the driver %s", TAF_TIME_MODULE_NAME);
        time.isDrvPresent = false;
    }
    else // successfully loaded
    {
        LE_DEBUG("Driver loaded successfully....");
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

 FUNCTION        taf_time_SetTrustTime

 DESCRIPTION     Sets time and validity for system.

 DEPENDENCIES    Initialization of Time Service.

 PARAMETERS      [IN] taf_time_SourceRef_t sourceRef: Source ref
                 [IN] const taf_time_TimeSpec_t * timeVal: Time in
                      seconds and nanoseconds.
                 [IN] Validity to set given source


 RETURN VALUE
                - LE_OK if successful.
                - LE_FAULT if any error occurs.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_time_SetTrustTime
(
    taf_time_SourceRef_t sourceRef,
    const taf_time_TimeSpec_t * timeValPtr,
    bool validity
)
{
    auto& tafTime = taf_Time::GetInstance();
    taf_time_TimeSpec_t time;
    time.sec = timeValPtr->sec;
    time.nanosec = timeValPtr->nanosec;

    return tafTime.SetTrustTime(sourceRef, time, validity);
}

/**
 * The initialization of TelAF Time Service component.
*/
COMPONENT_INIT
{
    LE_INFO("Time Service is starting ...");
    taf_time_service_int();
    LE_INFO("Time service initialization done\n");
}
