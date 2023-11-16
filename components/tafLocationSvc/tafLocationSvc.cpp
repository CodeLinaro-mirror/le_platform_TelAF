/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * Changes from Qualcomm Innovation Center are provided under the following license:

 * Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:

 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.

 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.

 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.

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

#include "legato.h"
#include "interfaces.h"
#include <telux/loc/LocationManager.hpp>
#include "tafPos.hpp"
#include "tafGnss.hpp"
#include "tafSvcIF.hpp"

using namespace telux::tafsvc;

/**
 * The initialization of TelAF Location component.
*/
COMPONENT_INIT
{
    auto &pos = taf_Pos::GetInstance();
    pos.Init();
    auto &gnss = taf_Gnss::GetInstance();
    gnss.Init();

    LE_INFO("Location Service init completed...");
}

/**
* FUNCTION     : SetAcquisitionRate
* DESCRIPTION  : Set the acquisition rate
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES:  LE_OK On Success, LE_OUT_OF_RANGE Invalid acquisition rate
*/
le_result_t taf_pos_SetAcquisitionRate
(
 uint32_t  acquisitionRate
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.SetAcquisitionRate(acquisitionRate);
}

/**
* FUNCTION     : GetAcquisitionRate
* DESCRIPTION  : Get the acquisition rate
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Acquisition rate in milliseconds.
*/
uint32_t taf_pos_GetAcquisitionRate
(
 void
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.GetAcquisitionRate();
}

/**
* FUNCTION     : GetFixState
* DESCRIPTION  : Get the position fix state
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK On Success, LE_FAULT Function failed to get the fix state
*/
le_result_t taf_pos_GetFixState
(
 taf_gnss_FixState_t* statePtr
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.GetFixState(statePtr);
}

/**
* FUNCTION     : AddMovementHandler
* DESCRIPTION  : This function must be called to register a handler for movement notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: handler reference, which is only needed for later removal of the handler
*                Doesn't return on failure, so there's no need to check the return value for errors
*/
taf_pos_MovementHandlerRef_t taf_pos_AddMovementHandler(uint32_t hMagnitude,uint32_t vMagnitude,
                                        taf_pos_MovementHandlerFunc_t handlerPtr, void* contextPtr)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.AddMovementHandler(hMagnitude, vMagnitude, handlerPtr, contextPtr);
}

/**
* FUNCTION     : RemoveMovementHandler
* DESCRIPTION  : This function must be called to remove a handler for movement notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Doesn't return on failure, so there's no need to check the return value for errors
*/
void taf_pos_RemoveMovementHandler(taf_pos_MovementHandlerRef_t handlerRef)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.RemoveMovementHandler(handlerRef);
}

/**
* FUNCTION     : Release
* DESCRIPTION  : This function must be called to release the position sample
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: If the caller is passing an invalid Position reference into this function,
*                it is a fatal error, the function will not return.
*/
void taf_pos_sample_Release
(
 taf_pos_SampleRef_t positionSampleRef
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.Release(positionSampleRef);
}

/**
* FUNCTION     : GetTime
* DESCRIPTION  : Get the time of the last updated location
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_GetTime
(
 uint16_t* hrsPtr,
 uint16_t* minPtr,
 uint16_t* secPtr,
 uint16_t* msecPtr
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.GetTime(hrsPtr, minPtr, secPtr, msecPtr);
}

/**
* FUNCTION     : GetDate
* DESCRIPTION  : Get the date of the last updated location
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_GetDate
(
 uint16_t* yearPtr,
 uint16_t* monthPtr,
 uint16_t* dayPtr
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.GetDate(yearPtr, monthPtr, dayPtr);
}

/**
* FUNCTION     : GetMotion
* DESCRIPTION  : Get the date of the last updated location
* DEPENDECY    : Get the motion's data (Horizontal Speed, Horizontal Speed's
* accuracy, Vertical Speed, Vertical Speed's accuracy).
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_GetMotion
(
 uint32_t* hSpeed,
 uint32_t* hSpeedAccuracy,
 int32_t*  vSpeed,
 int32_t*  vSpeedAccuracy
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.GetMotion(hSpeed,hSpeedAccuracy,vSpeed,vSpeedAccuracy);
}

/**
* FUNCTION     : Get2DLocation
* DESCRIPTION  : Get the 2D location's data (Latitude, Longitude, Horizontal
*                accuracy)
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_Get2DLocation
(
 int32_t* latitudePtr,
 int32_t* longitudePtr,
 int32_t* hAccuracyPtr
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.Get2DLocation(latitudePtr,longitudePtr,hAccuracyPtr);
}

/**
* FUNCTION     : Get3DLocation
* DESCRIPTION  : Get the 3D location's data (Latitude, Longitude, Horizontal
*                accuracy, , Vertical accuracy)
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_Get3DLocation
(
 int32_t* latitudePtr,
 int32_t* longitudePtr,
 int32_t* hAccuracyPtr,
 int32_t* altitudePtr,
 int32_t* vAccuracyPtr
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.Get3DLocation(latitudePtr,longitudePtr,hAccuracyPtr,altitudePtr,vAccuracyPtr);
}

/**
* FUNCTION     : sample_Get2DLocation
* DESCRIPTION  : Get the sample's 2D location's data (Latitude, Longitude, Horizontal
*                accuracy)
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_sample_Get2DLocation
(
 taf_pos_SampleRef_t positionSampleRef,
 int32_t* latitudePtr,
 int32_t* longitudePtr,
 int32_t* hAccuracyPtr
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.sample_Get2DLocation(positionSampleRef,latitudePtr,longitudePtr,hAccuracyPtr);
}

/**
* FUNCTION     : sample_GetAltitude
* DESCRIPTION  : Get the position sample's altitude
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_sample_GetAltitude
(
taf_pos_SampleRef_t positionSampleRef,
int32_t* altitudePtr,
int32_t* altitudeAccuracyPtr
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.sample_GetAltitude(positionSampleRef,altitudePtr,altitudeAccuracyPtr);
}

/**
* FUNCTION     : sample_GetTime
* DESCRIPTION  : This function must be called to get the position sample's time
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_sample_GetTime
(
    taf_pos_SampleRef_t  positionSampleRef,
    uint16_t* hoursPtr,
    uint16_t* minutesPtr,
    uint16_t* secondsPtr,
    uint16_t* millisecondsPtr
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.sample_GetTime(positionSampleRef,hoursPtr,minutesPtr,secondsPtr,millisecondsPtr);
}

/**
* FUNCTION     : sample_GetDate
* DESCRIPTION  : This function is called to get the position sample's date
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_sample_GetDate
(
   taf_pos_SampleRef_t positionSampleRef,
   uint16_t* yearPtr,
   uint16_t* monthPtr,
   uint16_t* dayPtr
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.sample_GetDate(positionSampleRef,yearPtr,monthPtr,dayPtr);
}
/**
* FUNCTION     : sample_GetHorizontalSpeed
* DESCRIPTION  : Get the position sample's horizontal speed
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_sample_GetHorizontalSpeed
(
    taf_pos_SampleRef_t positionSampleRef,
    uint32_t* hSpeedPtr,
    uint32_t* hSpeedAccuracyPtr
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.sample_GetHorizontalSpeed(positionSampleRef,hSpeedPtr,hSpeedAccuracyPtr);
}

/**
* FUNCTION     : sample_GetDirection
* DESCRIPTION  : Direction of movement is the direction that the vehicle or person is actually moving
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_sample_GetDirection
(
    taf_pos_SampleRef_t  positionSampleRef,
    uint32_t* directionPtr,
    uint32_t* directionAccuracyPtr
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.sample_GetDirection(positionSampleRef,directionPtr,directionAccuracyPtr);
}

/**
* FUNCTION     : sample_GetVerticalSpeed
* DESCRIPTION  : Get the position sample's vertical speed
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_sample_GetVerticalSpeed
(
    taf_pos_SampleRef_t  positionSampleRef,
    int32_t* vSpeedPtr,
    int32_t* vSpeedAccuracyPtr
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.sample_GetVerticalSpeed( positionSampleRef,vSpeedPtr,vSpeedAccuracyPtr);
}

/**
* FUNCTION     : SetDistanceResolution
* DESCRIPTION  : Set the resolution for the positioning distance values
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_SetDistanceResolution
(
 taf_pos_Resolution_t resolution
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.SetDistanceResolution(resolution);
}

/**
* FUNCTION     : sample_GetFixState
* DESCRIPTION  : Get the position sample's fix state
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_sample_GetFixState
(
    taf_pos_SampleRef_t  positionSampleRef,
    taf_gnss_FixState_t*  statePtr
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.sample_GetFixState(positionSampleRef,statePtr);
}

/**
* FUNCTION     : GetDirection
* DESCRIPTION  : Get the direction indication. Direction of movement is the direction that the vehicle or person
*                is actually moving.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_pos_GetDirection
(
 uint32_t* directionPtr,
 uint32_t* directionAccuracyPtr
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.GetDirection(directionPtr,directionAccuracyPtr);
}

/**
* FUNCTION     : posCtrl_Request
* DESCRIPTION  : Request activation of the positioning service
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Reference
*/
taf_posCtrl_ActivationRef_t taf_posCtrl_Request
(
    void
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.posCtrl_Request();
}

/**
* FUNCTION     : Release
* DESCRIPTION  : Release the Positioning services
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: None
*/
void taf_posCtrl_Release
(
 taf_posCtrl_ActivationRef_t ref
)
{
    auto &pos = taf_Pos::GetInstance();
    return pos.posCtrl_Release(ref);
}

/**
* FUNCTION     : GetDate
* DESCRIPTION  : Get the position sample's date
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_gnss_GetDate
(
 taf_gnss_SampleRef_t positionSampleRef,
 uint16_t* yearPtr,
 uint16_t* monthPtr,
 uint16_t* dayPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetDate(positionSampleRef, yearPtr, monthPtr, dayPtr);
}

/**
* FUNCTION     : GetTime
* DESCRIPTION  : Get the position sample's time
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_gnss_GetTime
(
 taf_gnss_SampleRef_t positionSampleRef,
 uint16_t* hoursPtr,
 uint16_t* minutesPtr,
 uint16_t* secondsPtr,
 uint16_t* millisecondsPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetTime(positionSampleRef, hoursPtr, minutesPtr, secondsPtr, millisecondsPtr);
}

/**
* FUNCTION     : GetGpsLeapSeconds
* DESCRIPTION  : Get the position sample's UTC leap seconds in advance
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success,LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_gnss_GetGpsLeapSeconds
(
 taf_gnss_SampleRef_t positionSampleRef,
 uint8_t* leapSecondsPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetGpsLeapSeconds(positionSampleRef, leapSecondsPtr);
}

/**
* FUNCTION     : GetDirection
* DESCRIPTION  : Get the position sample's direction. Direction of movement is the direction that the vehicle or
 *               person is actually moving.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_gnss_GetDirection
(
 taf_gnss_SampleRef_t positionSampleRef,
 uint32_t* directionPtr,
 uint32_t* directionAccuracyPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetDirection(positionSampleRef, directionPtr, directionAccuracyPtr);
}

/**
* FUNCTION     : GetVerticalSpeed
* DESCRIPTION  : Get the position sample's vertical speed
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_gnss_GetVerticalSpeed
(
 taf_gnss_SampleRef_t positionSampleRef,
int32_t* vspeedPtr,
int32_t* vspeedAccuracyPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetVerticalSpeed(positionSampleRef, vspeedPtr, vspeedAccuracyPtr);
}

/**
* FUNCTION     : GetHorizontalSpeed
* DESCRIPTION  : Get the position sample's horizontal speed
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on fail
*/
le_result_t taf_gnss_GetHorizontalSpeed
(
 taf_gnss_SampleRef_t positionSampleRef,
uint32_t* hspeedPtr,
uint32_t* hspeedAccuracyPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetHorizontalSpeed(positionSampleRef, hspeedPtr, hspeedAccuracyPtr);
}

/**
* FUNCTION     : GetAltitude
* DESCRIPTION  : Get the position sample's altitude
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on fail
*/
le_result_t taf_gnss_GetAltitude
(
 taf_gnss_SampleRef_t positionSampleRef,
int32_t* altitudePtr,
int32_t* vAccuracyPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetAltitude(positionSampleRef, altitudePtr, vAccuracyPtr);
}

/**
* FUNCTION     : GetLocation
* DESCRIPTION  : Get the location's data (Latitude, Longitude, Horizontal accuracy)
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
le_result_t taf_gnss_GetLocation
(
 taf_gnss_SampleRef_t positionSampleRef,
int32_t* latitudePtr,
int32_t* longitudePtr,
int32_t* hAccuracyPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetLocation(positionSampleRef, latitudePtr, longitudePtr, hAccuracyPtr);
}

/**
* FUNCTION     : RemovePositionHandler
* DESCRIPTION  : This function must be called to remove a handler for position notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Doesn't return on failure, so there's no need to check the return value for errors
*/
void taf_gnss_RemovePositionHandler
(
 taf_gnss_PositionHandlerRef_t handlerRef
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.RemovePositionHandler(handlerRef);
}

/**
* FUNCTION     : AddPositionHandler
* DESCRIPTION  : This function must be called to register an handler for position notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: A handler reference, which is only needed for later removal of the handler
*/
taf_gnss_PositionHandlerRef_t taf_gnss_AddPositionHandler
(
 taf_gnss_PositionHandlerFunc_t handlerPtr,
 void* contextPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return (taf_gnss_PositionHandlerRef_t)gnss.AddPositionHandler(handlerPtr, contextPtr);
}

/**
* FUNCTION     : RemoveCapabilityHandler
* DESCRIPTION  : This function must be called to remove a handler for Capability notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Doesn't return on failure, so there's no need to check the return value for errors
*/
void taf_gnss_RemoveCapabilityChangeHandler
(
 taf_gnss_CapabilityChangeHandlerRef_t handlerRef
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.RemoveCapabilityHandler(handlerRef);
}

/**
* FUNCTION     : AddCapabilityHandler
* DESCRIPTION  : This function must be called to register an handler for Capability notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: A handler reference, which is only needed for later removal of the handler
*/
taf_gnss_CapabilityChangeHandlerRef_t taf_gnss_AddCapabilityChangeHandler
(
 taf_gnss_CapabilityChangeHandlerFunc_t handlerPtr,
 void* contextPtr
)
{
    le_event_HandlerRef_t handlerRef;
    auto &gnss = taf_Gnss::GetInstance();
    handlerRef = (le_event_HandlerRef_t)gnss.AddCapabilityHandler(handlerPtr, contextPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_gnss_CapabilityChangeHandlerRef_t)(handlerRef);
}

/**
* FUNCTION     : RemoveNmeaHandler
* DESCRIPTION  : This function must be called to remove a handler for NMEA notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Doesn't return on failure, so there's no need to check the return value for errors
*/
void taf_gnss_RemoveNmeaHandler
(
 taf_gnss_NmeaHandlerRef_t handlerRef
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.RemoveNmeaHandler(handlerRef);
}

/**
* FUNCTION     : AddNmeaHandler
* DESCRIPTION  : This function must be called to register an handler for NMEA notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: A handler reference, which is only needed for later removal of the handler
*/
taf_gnss_NmeaHandlerRef_t taf_gnss_AddNmeaHandler
(
 taf_gnss_NmeaHandlerFunc_t handlerPtr,
 void* contextPtr
)
{
    le_event_HandlerRef_t handlerRef;
    auto &gnss = taf_Gnss::GetInstance();
    handlerRef = (le_event_HandlerRef_t)gnss.AddNmeaHandler(handlerPtr, contextPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_gnss_NmeaHandlerRef_t)(handlerRef);
}

/**
* FUNCTION     : Enable
* DESCRIPTION  : This function enables the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
le_result_t taf_gnss_Enable
(
 void
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.Enable();
}

/**
* FUNCTION     : SetConstellation
* DESCRIPTION  : Set the GNSS constellation bit mask
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
le_result_t taf_gnss_SetConstellation
(
taf_gnss_ConstellationBitMask_t constellationMask
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.SetConstellation( constellationMask);
}

/**
* FUNCTION     : Start
* DESCRIPTION  : This function starts the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
le_result_t taf_gnss_Start
(
 void
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.Start();
}

/**
* FUNCTION     : GetConstellation
* DESCRIPTION  : Get the GNSS constellation bit mask
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT failed
*/
le_result_t taf_gnss_GetConstellation
(
taf_gnss_ConstellationBitMask_t *constellationMaskPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetConstellation(constellationMaskPtr);
}

/**
* FUNCTION     : Disable
* DESCRIPTION  : This function disables the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_DUPLICATE LE_NOT_PERMITTED on failed with reason
*/
le_result_t taf_gnss_Disable
(
void
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.Disable();
}

/**
* FUNCTION     : Stop
* DESCRIPTION  : This function stops the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_DUPLICATE LE_NOT_PERMITTED on failed with reason
*/
le_result_t taf_gnss_Stop
(
 void
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.Stop();
}

/**
* FUNCTION     : GetState
* DESCRIPTION  : This function returns the state of the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: returns current state
*/
taf_gnss_State_t taf_gnss_GetState
(
 void
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetState();
}

/**
* FUNCTION     : GetSatellitesStatus
* DESCRIPTION  : Get the Satellites Vehicle status
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failure
*/
le_result_t taf_gnss_GetSatellitesStatus
(
taf_gnss_SampleRef_t positionSampleRef,
uint8_t* satsInViewCountPtr,
uint8_t* satsTrackingCountPtr,
uint8_t* satsUsedCountPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetSatellitesStatus(positionSampleRef, satsInViewCountPtr, satsTrackingCountPtr, satsUsedCountPtr);
}

/**
* FUNCTION     : GetAcquisitionRate
* DESCRIPTION  : This function gets the GNSS device acquisition rate
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failure, LE_NOT_PERMITTED If the GNSS device is not in "ready" state
*/
le_result_t taf_gnss_GetAcquisitionRate
(
 uint32_t* ratePtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetAcquisitionRate(ratePtr);
}

/**
* FUNCTION     : GetTtff
* DESCRIPTION  : Get the TTFF in milliseconds
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_BUSY LE_NOT_PERMITTED LE_FAULT on failed with reason
*/
le_result_t taf_gnss_GetTtff
(
 uint32_t* ttffPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetTtff(ttffPtr);
}

/**
* FUNCTION     : GetSatellitesInfo
* DESCRIPTION  : Get the Satellites Vehicle information
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on fail, LE_OUT_OF_RANGE retrieved parameters is invalid
*/
le_result_t taf_gnss_GetSatellitesInfo
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint16_t* satIdPtr,
    size_t* satIdNumPtr,
    taf_gnss_Constellation_t* satConstPtr,
    size_t* satConstNumPtr,
    bool* satUsedPtr,
    size_t* satUsedNumPtr,
    uint8_t* satSnrPtr,
    size_t* satSnrNumPtr,
    uint16_t* satAzimPtr,
    size_t* satAzimNumPtr,
    uint8_t* satElevPtr,
    size_t* satElevNumPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetSatellitesInfo(positionSampleRef, satIdPtr, satIdNumPtr, satConstPtr, satConstNumPtr,
            satUsedPtr, satUsedNumPtr, satSnrPtr, satSnrNumPtr, satAzimPtr, satAzimNumPtr, satElevPtr, satElevNumPtr);
}

/**
* FUNCTION     : GetLastSampleRef
* DESCRIPTION  : This function gets the last updated position sample object reference.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: A reference to last Position's sample
*/
taf_gnss_SampleRef_t taf_gnss_GetLastSampleRef
(
    void
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetLastSampleRef();
}

/**
* FUNCTION     : GetPositionState
* DESCRIPTION  : This function gets the position sample's fix state
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success,LE_FAULT on failure
*/
le_result_t taf_gnss_GetPositionState
(
    taf_gnss_SampleRef_t positionSampleRef,

    taf_gnss_FixState_t* statePtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetPositionState(positionSampleRef, statePtr);
}

/**
* FUNCTION     : ReleaseSampleRef
* DESCRIPTION  : This function must be called to release the position sample
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: If the caller is passing an invalid Position reference into this function,
*                it is a fatal error, the function will not return.
*/
void taf_gnss_ReleaseSampleRef
(
 taf_gnss_SampleRef_t    positionSampleRef
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.ReleaseSampleRef(positionSampleRef);
}

/**
* FUNCTION     : GetTimeAccuracy
* DESCRIPTION  : Get the position sample's time accurary
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
le_result_t taf_gnss_GetTimeAccuracy
(
 taf_gnss_SampleRef_t    positionSampleRef,
 uint32_t* timeAccuracyPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetTimeAccuracy(positionSampleRef, timeAccuracyPtr);
}

/**
* FUNCTION     : GetEpochTime
* DESCRIPTION  : Get the position sample's epoch time
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
le_result_t taf_gnss_GetEpochTime
(
 taf_gnss_SampleRef_t    positionSampleRef,
 uint64_t* millisecondsPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetEpochTime(positionSampleRef, millisecondsPtr);
}

/**
* FUNCTION     : SetDopResolution
* DESCRIPTION  : Set the resolution for the DOP parameters
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_BAD_PARAMETER on failed
*/
le_result_t taf_gnss_SetDopResolution
(
 taf_gnss_Resolution_t resolution
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.SetDopResolution(resolution);
}

/**
* FUNCTION     : GetDilutionOfPrecision
* DESCRIPTION  : Get the DOP parameter (Dilution Of Precision) for the fixed position
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
le_result_t taf_gnss_GetDilutionOfPrecision
(
 taf_gnss_SampleRef_t    positionSampleRef,
 taf_gnss_DopType_t dopType,
 uint16_t* dopPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetDilutionOfPrecision(positionSampleRef, dopType, dopPtr);
}

/**
* FUNCTION     : GetLeapSeconds
* DESCRIPTION  : This function gets leap seconds information
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_TIMEOUT LE_UNSUPPORTED LE_FAULT on failed
*/
le_result_t taf_gnss_GetLeapSeconds
(
 uint64_t* gpsTimePtr,
 int32_t* currentLeapSecondsPtr,
 uint64_t* changeEventTimePtr,
 int32_t* nextLeapSecondsPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetLeapSeconds(gpsTimePtr,currentLeapSecondsPtr,changeEventTimePtr,nextLeapSecondsPtr);
}

/**
* FUNCTION     : GetGpsTime
* DESCRIPTION  : Get the position sample's GPS time
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
le_result_t taf_gnss_GetGpsTime
(
 taf_gnss_SampleRef_t    positionSampleRef,
 uint32_t* gpsWeekPtr,
 uint32_t* gpsTimeOfWeekPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetGpsTime(positionSampleRef, gpsWeekPtr, gpsTimeOfWeekPtr);
}

/**
* FUNCTION     : SetAcquisitionRate
* DESCRIPTION  : This function sets the GNSS device acquisition rate
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE LE_NOT_PERMITTED LE_UNSUPPORTED on failed
*/
le_result_t taf_gnss_SetAcquisitionRate
(
 uint32_t  rate
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.SetAcquisitionRate(rate);
}

/**
* FUNCTION     : ForceColdRestart
* DESCRIPTION  : This function performs cold restart,Delete all Aiding data
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed
*/
le_result_t taf_gnss_ForceColdRestart
(
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.ForceColdRestart();
}

/**
* FUNCTION     : ForceWarmRestart
* DESCRIPTION  : This function performs warm restart, delete aiding data
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed
*/
le_result_t taf_gnss_ForceWarmRestart
(
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.ForceWarmRestart();
}

/**
* FUNCTION     : ForceFactoryRestart
* DESCRIPTION  :
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed
*/
le_result_t taf_gnss_ForceFactoryRestart
(
)
{
    LE_DEBUG("Feature not Supported");
    return LE_UNSUPPORTED;
}

/**
* FUNCTION     : ForceWarmRestart
* DESCRIPTION  :
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed
*/
le_result_t taf_gnss_ForceHotRestart
(
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.ForceHotRestart();
}

/**
* FUNCTION     : GetSupportedConstellations
* DESCRIPTION  : Returns all supported satellite constellations
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_UNSUPPORTED on failed
*/
le_result_t taf_gnss_GetSupportedConstellations
(
 taf_gnss_ConstellationBitMask_t* constellationMaskPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetSupportedConstellations(constellationMaskPtr);
}

/**
* FUNCTION     : SetMinElevation
* DESCRIPTION  : sets the GNSS minimum elevation
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT LE_UNSUPPORTED on failed
*/
le_result_t taf_gnss_SetMinElevation
(
 uint8_t  minElevation
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.SetMinElevation(minElevation);
}

/**
* FUNCTION     : StartMode
* DESCRIPTION  : starts the GNSS device in the specified start mode
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_NOT_PERMITTED LE_FAULT LE_UNSUPPORTED on failed
*/
le_result_t taf_gnss_StartMode
(
 taf_gnss_StartMode_t  mode
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.StartMode(mode);
}

/**
* FUNCTION     : GetMinElevation
* DESCRIPTION  : gets the GNSS minimum elevation
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED on failed
*/
le_result_t taf_gnss_GetMinElevation
(
 uint8_t*  minElevationPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetMinElevation(minElevationPtr);
}

/**
* FUNCTION     : SetNmeaSentences
* DESCRIPTION  : sets the enabled NMEA sentences using a bit mask.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED on failed
*/

le_result_t taf_gnss_SetNmeaSentences
(
    taf_gnss_NmeaBitMask_t nmeaMask ///< [IN] Bit mask for enabled NMEA sentences.
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.SetNmeaSentences(nmeaMask);
}

/**
* FUNCTION     : GetNmeaSentences
* DESCRIPTION  : Gets the bit mask for the enabled NMEA sentences.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED on failed
*/
le_result_t taf_gnss_GetNmeaSentences
(
    taf_gnss_NmeaBitMask_t* nmeaMaskPtr ///< [OUT] Bit mask for enabled NMEA sentences.
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetNmeaSentences(nmeaMaskPtr);
}

/**
* FUNCTION     : SetDRConfig
* DESCRIPTION  : Set the Dead Reckoing configuration Parameters
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
le_result_t taf_gnss_SetDRConfig
(
    const taf_gnss_DrParams_t* drParamsPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.SetDRConfig(drParamsPtr);
}
/**
* FUNCTION     : GetSupportedNmeaSentences
* DESCRIPTION  : Gets the bit mask for the supported NMEA sentences.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED on failed
*/
le_result_t taf_gnss_GetSupportedNmeaSentences
(
    taf_gnss_NmeaBitMask_t* nmeaMaskPtr ///< [OUT] Bit mask for supported NMEA sentences.
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetSupportedNmeaSentences(nmeaMaskPtr);
}
/**
* FUNCTION     : ConfigureEngineState
* DESCRIPTION  : Set the Engine state for SPE/PPE/DRE/VPE Engines
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
le_result_t taf_gnss_ConfigureEngineState
(
    taf_gnss_EngineType_t engtype,
    taf_gnss_EngineState_t engState
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.ConfigureEngineState(engtype,engState);
}
#endif
/**
* FUNCTION     : ConfigureRobustLocation
* DESCRIPTION  : Enable or Disable Robust Location for 911 enable or disable
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
le_result_t taf_gnss_ConfigureRobustLocation
(
    uint8_t enable,
    uint8_t enabled911
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.ConfigureRobustLocation(enable,enabled911);
}

/**
* FUNCTION     : RobustLocationInformation
* DESCRIPTION  : Get the Robust Location information for Enable/Disable, 911 enable or disable, major & minor version numbers
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
le_result_t taf_gnss_RobustLocationInformation
(
   uint8_t* enable,
   uint8_t* enabled911,
   uint8_t* majorVersion,
   uint8_t* minorVersion
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.RobustLocationInformation(enable,enabled911,majorVersion,minorVersion);
}

/**
* FUNCTION     : EmptySecondaryBandConstellation
* DESCRIPTION  : Set the Secondary Band Empty Constellations
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
le_result_t taf_gnss_DefaultSecondaryBandConstellations
(
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.DefaultSecondaryBandConstellations();
}
#endif
/**
* FUNCTION     : RequestSecondaryBandConstellation
* DESCRIPTION  : Get the Secondary Band GNSS constellation type disabled
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
le_result_t taf_gnss_RequestSecondaryBandConstellations
(
   uint32_t* constellationSb
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.RequestSecondaryBandConstellations(constellationSb);
}
#endif
/**
* FUNCTION     : ConfigureSecondaryBandConstellation
* DESCRIPTION  : Configure Secondary Band GNSS constellation type to be disabled
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
le_result_t taf_gnss_ConfigureSecondaryBandConstellations
(
    uint32_t constellationSb
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.ConfigureSecondaryBandConstellations(constellationSb);
}
#endif
/**
* FUNCTION     : GetMagneticDeviation
* DESCRIPTION  : Get the position sample's magnetic deviation
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
le_result_t taf_gnss_GetMagneticDeviation
(
    taf_gnss_SampleRef_t positionSampleRef,
    int32_t* magneticDeviationPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetMagneticDeviation(positionSampleRef,magneticDeviationPtr);
}

/**
* FUNCTION     : GetEllipticalUncertainty
* DESCRIPTION  : Get the semi-major and semi-minor horizontal elliptical uncertainty.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
le_result_t taf_gnss_GetEllipticalUncertainty
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint32_t* horUncEllipseSemiMajorPtr,
    uint32_t* horUncEllipseSemiMinorPtr,
    uint8_t*  horConfidencePtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetEllipticalUncertainty(positionSampleRef,horUncEllipseSemiMajorPtr,
            horUncEllipseSemiMinorPtr,horConfidencePtr);
}

/**
* FUNCTION     : SetLeverArmConfig
* DESCRIPTION  : Set the Lever Arm configuration Parameters
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_BAD_PARAMETER LE_NOT_PERMITTED on failed with reason
*/
le_result_t taf_gnss_SetLeverArmConfig
(
    const taf_gnss_LeverArmParams_t* LeverArmParamsPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.SetLeverArmConfig(LeverArmParamsPtr);
}

/**
* FUNCTION     : SetEngineType
* DESCRIPTION  : This function sets the GNSS device with engine type
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_BAD_PARAMETER LE_NOT_PERMITTED on failed with reason
*/
le_result_t taf_gnss_SetEngineType
(
    taf_gnss_EngineReportsType_t EngineType
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.SetEngineType(EngineType);
}

/**
* FUNCTION     : GetConformityIndex
* DESCRIPTION  : This function gets the conformity index
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_gnss_GetConformityIndex
(
 taf_gnss_SampleRef_t positionSampleRef,
double* indexPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetConformityIndex(positionSampleRef,indexPtr);
}

/**
* FUNCTION     : GetCalibrationData
* DESCRIPTION  : This function gets the sensor calibration status and confidence percent.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_gnss_GetCalibrationData
(
 taf_gnss_SampleRef_t positionSampleRef,
uint32_t* calibPtr,
uint8_t* percentPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetCalibrationData(positionSampleRef,calibPtr,percentPtr);
}

/**
* FUNCTION     : GetBodyFrameData
* DESCRIPTION  : This function gets the body frame data
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_gnss_GetBodyFrameData
(
    taf_gnss_SampleRef_t positionSampleRef,
taf_gnss_KinematicsData_t* bodyDataPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetBodyFrameData(positionSampleRef,bodyDataPtr);
}

/**
* FUNCTION     : GetVRPBasedLLA
* DESCRIPTION  : This function gets Vechile reference point based latitude,longitude & altitude information
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_gnss_GetVRPBasedLLA
(
    taf_gnss_SampleRef_t positionSampleRef,
    double* vrpLatitudePtr,
    double* vrpLongitudePtr,
    double* vrpAttitudePtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetVRPBasedLLA(positionSampleRef,vrpLatitudePtr,vrpLongitudePtr,vrpAttitudePtr);
}

/**
* FUNCTION     : GetVRPBasedVelocity
* DESCRIPTION  : This function gets Vehicle reference point based east,north & up velocity information
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_gnss_GetVRPBasedVelocity
(
    taf_gnss_SampleRef_t positionSampleRef,
    double* eastVelPtr,
    double* northVelPtr,
    double* upVelPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetVRPBasedVelocity(positionSampleRef,eastVelPtr,northVelPtr,upVelPtr);
}

/**
* FUNCTION     : GetSvUsedInPosition
* DESCRIPTION  : This function gets the set of satellite vehicles that are used to calculate position.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_gnss_GetSvUsedInPosition
(
    taf_gnss_SampleRef_t positionSampleRef,
    taf_gnss_SvUsedInPosition_t* svDataPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetSvUsedInPosition(positionSampleRef,svDataPtr);
}

/**
* FUNCTION     : GetSbasCorrection
* DESCRIPTION  : This function gets navigation solution mask used to indicate SBAS corrections.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_gnss_GetSbasCorrection
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint32_t* sbasMaskPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetSbasCorrection(positionSampleRef,sbasMaskPtr);
}

/**
* FUNCTION     : GetPositionTechnology
* DESCRIPTION  : This function gets technology mask to indicate which technology is used.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_gnss_GetPositionTechnology
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint32_t* techMaskPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetPositionTechnology(positionSampleRef,techMaskPtr);
}

/**
* FUNCTION     : GetLocationInfoValidity
* DESCRIPTION  : This function gets the validity of the Location basic Info.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_gnss_GetLocationInfoValidity
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint32_t* validityMaskPtr,
    uint64_t* validityExMaskPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetLocationInfoValidity(positionSampleRef,validityMaskPtr,validityExMaskPtr);
}

/**
* FUNCTION     : GetLocationOutputEngParams
* DESCRIPTION  : This function gets the combination of position engines and type of
*                location engine used in calculating the position report.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_gnss_GetLocationOutputEngParams
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint16_t* engMaskPtr,
    uint16_t* locationEngTypePtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetLocationOutputEngParams(positionSampleRef,engMaskPtr,locationEngTypePtr);
}

/**
* FUNCTION     : GetReliabilityInformation
* DESCRIPTION  : This function gets the reliability of the horizontal & vertical positions.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_gnss_GetReliabilityInformation
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint16_t* horiReliabilityPtr,
    uint16_t* vertReliabilityPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetReliabilityInformation(positionSampleRef, horiReliabilityPtr,
        vertReliabilityPtr);
}

/**
* FUNCTION     : GetStdDeviationAzimuthInfo
* DESCRIPTION  : This function gets the elliptical horizontal uncertainty azimuth of orientation,
*                east and north standard deviations.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_gnss_GetStdDeviationAzimuthInfo
(
    taf_gnss_SampleRef_t positionSampleRef,
    double* azimuthPtr,
    double* eastDevPtr,
    double* northDevPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetStdDeviationAzimuthInfo(positionSampleRef,azimuthPtr,eastDevPtr,northDevPtr);
}

/**
* FUNCTION     : GetRealTimeInformation
* DESCRIPTION  : This function gets elapsed real time and its uncertainity values in nano-second.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_gnss_GetRealTimeInformation
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint64_t* realTimePtr,
    uint64_t* realTimeUncPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetRealTimeInformation(positionSampleRef,realTimePtr,realTimeUncPtr);
}

/**
* FUNCTION     : GetMeasurementUsageInfo
* DESCRIPTION  : This function retrieves gnss measurement usage info.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_OVERFLOW, LE_BAD_PARAMETER, LE_NO_MEMORY
*                LE_OUT_OF_RANGE on failed with reason.
*/
le_result_t taf_gnss_GetMeasurementUsageInfo
(
    taf_gnss_SampleRef_t positionSampleRef,
    taf_gnss_GnssMeasurementInfo_t* LE_NONNULL measInfoPtr,
    size_t* measInfoLen
)
{
    TAF_ERROR_IF_RET_VAL(positionSampleRef == NULL, LE_BAD_PARAMETER, "Invalid gnss sample reference");
    TAF_ERROR_IF_RET_VAL(measInfoPtr == NULL, LE_NO_MEMORY, "measInfoPtr is NULL");
    TAF_ERROR_IF_RET_VAL(*measInfoLen == 0, LE_OUT_OF_RANGE, "measInfoLen is ZERO");
    TAF_ERROR_IF_RET_VAL(*measInfoLen > TAF_GNSS_MEASUREMENT_INFO_MAX, LE_OVERFLOW, "Too many elements.");

    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetMeasurementUsageInfo(positionSampleRef, measInfoPtr, measInfoLen);
}

/**
* FUNCTION     : GetReportStatus
* DESCRIPTION  : This function retrieves the status of report in terms of how optimally
               : the report was calculated by the engine.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_BAD_PARAMETER, LE_NO_MEMORY on failed with reason
*/
le_result_t taf_gnss_GetReportStatus
(
    taf_gnss_SampleRef_t positionSampleRef,
    int32_t* reportStatusPtr
)
{
    TAF_ERROR_IF_RET_VAL(positionSampleRef == NULL, LE_BAD_PARAMETER, "Invalid gnss sample reference");
    TAF_ERROR_IF_RET_VAL(reportStatusPtr == NULL, LE_NO_MEMORY, "reportStatusPtr is NULL");

    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetReportStatus(positionSampleRef, reportStatusPtr);
}

/**
* FUNCTION     : GetAltitudeMeanSeaLevel
* DESCRIPTION  : This function retrieves the altitude with respect to mean sea level in Meters.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_BAD_PARAMETER, LE_NO_MEMORY on failed with reason
*/
le_result_t taf_gnss_GetAltitudeMeanSeaLevel
(
    taf_gnss_SampleRef_t positionSampleRef,
    double* altMeanSeaLevelPtr
)
{
    TAF_ERROR_IF_RET_VAL(positionSampleRef == NULL, LE_BAD_PARAMETER, "Invalid gnss sample reference");
    TAF_ERROR_IF_RET_VAL(altMeanSeaLevelPtr == NULL, LE_NO_MEMORY, "altMeanSeaLevelPtr is NULL");

    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetAltitudeMeanSeaLevel(positionSampleRef, altMeanSeaLevelPtr);
}

/**
* FUNCTION     : GetSVIds
* DESCRIPTION  : This function retrieves GNSS Satellite Vehicles used in position data.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_OVERFLOW, LE_BAD_PARAMETER, LE_NO_MEMORY
*                LE_OUT_OF_RANGE on failed with reason.
*/
le_result_t taf_gnss_GetSVIds
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint16_t* LE_NONNULL sVIdsPtr,
    size_t* sVIdsLen
)
{
    TAF_ERROR_IF_RET_VAL(positionSampleRef == NULL, LE_BAD_PARAMETER, "Invalid gnss sample reference");
    TAF_ERROR_IF_RET_VAL(sVIdsPtr == NULL, LE_NO_MEMORY, "measInfoPtr is NULL");
    TAF_ERROR_IF_RET_VAL(*sVIdsLen == 0, LE_OUT_OF_RANGE, "measInfoLen is ZERO");
    TAF_ERROR_IF_RET_VAL(*sVIdsLen > TAF_GNSS_MEASUREMENT_INFO_MAX, LE_OVERFLOW, "Too many elements.");

    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetSVIds(positionSampleRef, sVIdsPtr, sVIdsLen);
}

/**
* FUNCTION     : GetSatellitesInfoEx
* DESCRIPTION  : This function retrieves satellites vehicle information of a given constellation.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_OVERFLOW, LE_BAD_PARAMETER, LE_NO_MEMORY
*                LE_OUT_OF_RANGE on failed with reason.
*/
le_result_t taf_gnss_GetSatellitesInfoEx
(
    taf_gnss_SampleRef_t positionSampleRef,
    taf_gnss_Constellation_t constellation,
    taf_gnss_SvInfo_t* LE_NONNULL svInfoPtr,
    size_t* svInfoLen
)
{
    TAF_ERROR_IF_RET_VAL(positionSampleRef == NULL, LE_BAD_PARAMETER, "Invalid gnss sample reference");
    TAF_ERROR_IF_RET_VAL(svInfoPtr == NULL, LE_NO_MEMORY, "svInfoPtr is NULL");
    TAF_ERROR_IF_RET_VAL(*svInfoLen == 0, LE_OUT_OF_RANGE, "svInfoLen is ZERO");

    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetSatellitesInfoEx(positionSampleRef, constellation, svInfoPtr, svInfoLen);
}

/**
* FUNCTION     : SetMinGpsWeek
* DESCRIPTION  : This function sets the minimum GPS week used by the modem GNSS standard position
*                engine (SPE) and shall not be called while GNSS SPE is in the middle of a session.
*                Client needs to assure that there is no active GNSS SPE session prior to issuing
*                this command. Behavior is not defined if client issues a second request of
*                SetMinGpsWeek without waiting for the previous SetMinGpsWeek to finish. Additionally
*                minimum GPS week number shall NEVER be in the future of the current GPS Week.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_NOT_PERMITTED on failed
*/
le_result_t taf_gnss_SetMinGpsWeek
(
    uint16_t minGpsWeek
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.SetMinGpsWeek(minGpsWeek);
}

/**
* FUNCTION     : GetMinGpsWeek
* DESCRIPTION  : This function gets the minimum GPS week.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_NO_MEMORY, LE_NOT_PERMITTED on failed.
*/
le_result_t taf_gnss_GetMinGpsWeek
(
 uint16_t*  minGpsWeekPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetMinGpsWeek(minGpsWeekPtr);
}

/**
* FUNCTION     : GetCapabilities
* DESCRIPTION  : This function gets the GNSS capability information.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed.
*/
le_result_t taf_gnss_GetCapabilities
(
 uint64_t*  locCapabilityPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.GetCapabilities(locCapabilityPtr);
}

/**
* FUNCTION     : SetNmeaConfiguration
* DESCRIPTION  : Sets the NMEA sentences. Without prior invocation to this API, all NMEA
                 sentences supported in the system will get generated and delivered to
                 all the clients that register to receive NMEA sentences. The NMEA sentence
                 type configuration is common across all clients and updating it will affect
                 all clients. Please note that for the NMEA datum type request to be successful,
                 the nmea provider configuration in the GPS configuration file should be set to
                 application processor.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_BAD_PARAMETER, LE_NOT_PERMITTED on failed
*/

le_result_t taf_gnss_SetNmeaConfiguration
(
    taf_gnss_NmeaBitMask_t nmeaMask,         ///< [IN] Bit mask for enabled NMEA sentences.
    taf_gnss_GeodeticDatumType_t datumType,  ///< [IN] Specify the datum type to be configured.
    taf_gnss_LocEngineType_t engineType                      ///< [IN] Specify the Engine type.
)
{
    auto &gnss = taf_Gnss::GetInstance();
    return gnss.SetNmeaConfiguration(nmeaMask, datumType, engineType);
}
