/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include "tafTime.hpp"

using namespace telux::platform;
using namespace telux::common;
using namespace telux::tafsvc;

/*======================================================================

 FUNCTION        taf_time_SetSystemTime

 DESCRIPTION     Set system READ time.

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
    const taf_time_TimeSpec_t * timeVal,
    bool ackTimeSvc
)
{
    taf_time_TimeSpec_t time;

    time.sec = timeVal->sec;
    time.nanosec = timeVal->nanosec;

    auto &tafTime = taf_Time::GetInstance();

    return tafTime.SetSystemTime(time,
                                TAF_TIME_SRC_NAME_EX_APP,
                                ackTimeSvc);
}

/*======================================================================

 FUNCTION        taf_time_GetSystemTime

 DESCRIPTION     Get system READ time.

 DEPENDENCIES    Initialization of Time Service

 PARAMETERS      [IN] taf_time_TimeSpec_t * timeVal: Time in
                      seconds and nanoseconds.

 RETURN VALUE    le_result_t
                 LE_FAULT:             Fail.
                 LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_time_GetSystemTime
(
    taf_time_TimeSpec_t* timeVal
)
{
    auto &time = taf_Time::GetInstance();
    return time.GetSystemTime(timeVal);
}

/*======================================================================

 FUNCTION        taf_time_GetGnssTime

 DESCRIPTION     Get GNSS time that is maintained by time service.

 DEPENDENCIES    Initialization of Time Service

 PARAMETERS      [IN] taf_time_TimeSpec_t * timeVal: Time in
                      seconds and nanoseconds.

 RETURN VALUE    le_result_t
                 LE_FAULT:             Fail.
                 LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_time_GetGnssTime
(
    taf_time_TimeSpec_t* timeVal
)
{
    auto &tafTime = taf_Time::GetInstance();
    return tafTime.GetGnssTime(timeVal);
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
        tafTime.timeSourceChangeId, taf_Time::LayerTimeSourceChangeHandler,
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

/**
 * Initialization for Time Service.
*/
void taf_time_service_int(void)
{
    LE_INFO("Time Service Init...");
    auto &time = taf_Time::GetInstance();
    time.Init();
    LE_INFO("Time Service ready");
    return;
}

/**
 * The initialization of TelAF Time Service component.
*/
COMPONENT_INIT
{
    taf_time_service_int();
    LE_INFO("TelAf time service initialization done\n");
}
