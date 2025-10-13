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
 */

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2021-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include <telux/loc/LocationManager.hpp>
#include "tafPos.hpp"
#include "tafGnss.hpp"
#include "tafSvcIF.hpp"

using namespace tafsvc;

/**
 * The initialization of TelAF Location component.
*/
COMPONENT_INIT
{
    auto &pos = taf_locPos::GetInstance();
    pos.Init();
    auto &gnss = taf_locGnss::GetInstance();
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
le_result_t taf_locPos_SetAcquisitionRate
(
 uint32_t  acquisitionRate
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.SetAcquisitionRate(acquisitionRate);
}

/**
* FUNCTION     : GetAcquisitionRate
* DESCRIPTION  : Get the acquisition rate
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Acquisition rate in milliseconds.
*/
uint32_t taf_locPos_GetAcquisitionRate
(
 void
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.GetAcquisitionRate();
}

/**
* FUNCTION     : GetFixState
* DESCRIPTION  : Get the position fix state
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK On Success, LE_FAULT Function failed to get the fix state
*/
le_result_t taf_locPos_GetFixState
(
 taf_locGnss_FixState_t* statePtr
)
{
    auto &pos = taf_locPos::GetInstance();
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
taf_locPos_MovementHandlerRef_t taf_locPos_AddMovementHandler(uint32_t hMagnitude,uint32_t vMagnitude,
                                        taf_locPos_MovementHandlerFunc_t handlerPtr, void* contextPtr)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.AddMovementHandler(hMagnitude, vMagnitude, handlerPtr, contextPtr);
}

/**
* FUNCTION     : RemoveMovementHandler
* DESCRIPTION  : This function must be called to remove a handler for movement notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Doesn't return on failure, so there's no need to check the return value for errors
*/
void taf_locPos_RemoveMovementHandler(taf_locPos_MovementHandlerRef_t handlerRef)
{
    auto &pos = taf_locPos::GetInstance();
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
void taf_locPos_sample_Release
(
 taf_locPos_SampleRef_t positionSampleRef
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.Release(positionSampleRef);
}

/**
* FUNCTION     : GetTime
* DESCRIPTION  : Get the time of the last updated location
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_locPos_GetTime
(
 uint16_t* hrsPtr,
 uint16_t* minPtr,
 uint16_t* secPtr,
 uint16_t* msecPtr
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.GetTime(hrsPtr, minPtr, secPtr, msecPtr);
}

/**
* FUNCTION     : GetDate
* DESCRIPTION  : Get the date of the last updated location
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_locPos_GetDate
(
 uint16_t* yearPtr,
 uint16_t* monthPtr,
 uint16_t* dayPtr
)
{
    auto &pos = taf_locPos::GetInstance();
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
le_result_t taf_locPos_GetMotion
(
 uint32_t* hSpeed,
 uint32_t* hSpeedAccuracy,
 int32_t*  vSpeed,
 int32_t*  vSpeedAccuracy
)
{
    auto &pos = taf_locPos::GetInstance();
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
le_result_t taf_locPos_Get2DLocation
(
 int32_t* latitudePtr,
 int32_t* longitudePtr,
 int32_t* hAccuracyPtr
)
{
    auto &pos = taf_locPos::GetInstance();
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
le_result_t taf_locPos_Get3DLocation
(
 int32_t* latitudePtr,
 int32_t* longitudePtr,
 int32_t* hAccuracyPtr,
 int32_t* altitudePtr,
 int32_t* vAccuracyPtr
)
{
    auto &pos = taf_locPos::GetInstance();
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
le_result_t taf_locPos_sample_Get2DLocation
(
 taf_locPos_SampleRef_t positionSampleRef,
 int32_t* latitudePtr,
 int32_t* longitudePtr,
 int32_t* hAccuracyPtr
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.sample_Get2DLocation(positionSampleRef,latitudePtr,longitudePtr,hAccuracyPtr);
}

/**
* FUNCTION     : sample_GetAltitude
* DESCRIPTION  : Get the position sample's altitude
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_locPos_sample_GetAltitude
(
taf_locPos_SampleRef_t positionSampleRef,
int32_t* altitudePtr,
int32_t* altitudeAccuracyPtr
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.sample_GetAltitude(positionSampleRef,altitudePtr,altitudeAccuracyPtr);
}

/**
* FUNCTION     : sample_GetTime
* DESCRIPTION  : This function must be called to get the position sample's time
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_locPos_sample_GetTime
(
    taf_locPos_SampleRef_t  positionSampleRef,
    uint16_t* hoursPtr,
    uint16_t* minutesPtr,
    uint16_t* secondsPtr,
    uint16_t* millisecondsPtr
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.sample_GetTime(positionSampleRef,hoursPtr,minutesPtr,secondsPtr,millisecondsPtr);
}

/**
* FUNCTION     : sample_GetDate
* DESCRIPTION  : This function is called to get the position sample's date
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_locPos_sample_GetDate
(
   taf_locPos_SampleRef_t positionSampleRef,
   uint16_t* yearPtr,
   uint16_t* monthPtr,
   uint16_t* dayPtr
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.sample_GetDate(positionSampleRef,yearPtr,monthPtr,dayPtr);
}
/**
* FUNCTION     : sample_GetHorizontalSpeed
* DESCRIPTION  : Get the position sample's horizontal speed
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_locPos_sample_GetHorizontalSpeed
(
    taf_locPos_SampleRef_t positionSampleRef,
    uint32_t* hSpeedPtr,
    uint32_t* hSpeedAccuracyPtr
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.sample_GetHorizontalSpeed(positionSampleRef,hSpeedPtr,hSpeedAccuracyPtr);
}

/**
* FUNCTION     : sample_GetDirection
* DESCRIPTION  : Direction of movement is the direction that the vehicle or person is actually moving
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_locPos_sample_GetDirection
(
    taf_locPos_SampleRef_t  positionSampleRef,
    uint32_t* directionPtr,
    uint32_t* directionAccuracyPtr
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.sample_GetDirection(positionSampleRef,directionPtr,directionAccuracyPtr);
}

/**
* FUNCTION     : sample_GetVerticalSpeed
* DESCRIPTION  : Get the position sample's vertical speed
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_locPos_sample_GetVerticalSpeed
(
    taf_locPos_SampleRef_t  positionSampleRef,
    int32_t* vSpeedPtr,
    int32_t* vSpeedAccuracyPtr
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.sample_GetVerticalSpeed( positionSampleRef,vSpeedPtr,vSpeedAccuracyPtr);
}

/**
* FUNCTION     : SetDistanceResolution
* DESCRIPTION  : Set the resolution for the positioning distance values
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_locPos_SetDistanceResolution
(
 taf_locPos_Resolution_t resolution
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.SetDistanceResolution(resolution);
}

/**
* FUNCTION     : sample_GetFixState
* DESCRIPTION  : Get the position sample's fix state
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_locPos_sample_GetFixState
(
    taf_locPos_SampleRef_t  positionSampleRef,
    taf_locGnss_FixState_t*  statePtr
)
{
    auto &pos = taf_locPos::GetInstance();
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
le_result_t taf_locPos_GetDirection
(
 uint32_t* directionPtr,
 uint32_t* directionAccuracyPtr
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.GetDirection(directionPtr,directionAccuracyPtr);
}

/**
* FUNCTION     : locPosCtrl_Request
* DESCRIPTION  : Request activation of the positioning service
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Reference
*/
taf_locPosCtrl_ActivationRef_t taf_locPosCtrl_Request
(
    void
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.locPosCtrl_Request();
}

/**
* FUNCTION     : Release
* DESCRIPTION  : Release the Positioning services
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: None
*/
void taf_locPosCtrl_Release
(
 taf_locPosCtrl_ActivationRef_t ref
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.locPosCtrl_Release(ref);
}

/**
* FUNCTION     : GetDate
* DESCRIPTION  : Get the position sample's date
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_locGnss_GetDate
(
 taf_locGnss_SampleRef_t positionSampleRef,
 uint16_t* yearPtr,
 uint16_t* monthPtr,
 uint16_t* dayPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetDate(positionSampleRef, yearPtr, monthPtr, dayPtr);
}

/**
* FUNCTION     : GetTime
* DESCRIPTION  : Get the position sample's time
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_locGnss_GetTime
(
 taf_locGnss_SampleRef_t positionSampleRef,
 uint16_t* hoursPtr,
 uint16_t* minutesPtr,
 uint16_t* secondsPtr,
 uint16_t* millisecondsPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetTime(positionSampleRef, hoursPtr, minutesPtr, secondsPtr, millisecondsPtr);
}

/**
* FUNCTION     : GetGpsLeapSeconds
* DESCRIPTION  : Get the position sample's UTC leap seconds in advance
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success,LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_locGnss_GetGpsLeapSeconds
(
 taf_locGnss_SampleRef_t positionSampleRef,
 uint8_t* leapSecondsPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
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
le_result_t taf_locGnss_GetDirection
(
 taf_locGnss_SampleRef_t positionSampleRef,
 uint32_t* directionPtr,
 uint32_t* directionAccuracyPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetDirection(positionSampleRef, directionPtr, directionAccuracyPtr);
}

/**
* FUNCTION     : GetVerticalSpeed
* DESCRIPTION  : Get the position sample's vertical speed
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
le_result_t taf_locGnss_GetVerticalSpeed
(
 taf_locGnss_SampleRef_t positionSampleRef,
int32_t* vspeedPtr,
int32_t* vspeedAccuracyPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetVerticalSpeed(positionSampleRef, vspeedPtr, vspeedAccuracyPtr);
}

/**
* FUNCTION     : GetHorizontalSpeed
* DESCRIPTION  : Get the position sample's horizontal speed
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on fail
*/
le_result_t taf_locGnss_GetHorizontalSpeed
(
 taf_locGnss_SampleRef_t positionSampleRef,
uint32_t* hspeedPtr,
uint32_t* hspeedAccuracyPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetHorizontalSpeed(positionSampleRef, hspeedPtr, hspeedAccuracyPtr);
}

/**
* FUNCTION     : GetAltitude
* DESCRIPTION  : Get the position sample's altitude
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on fail
*/
le_result_t taf_locGnss_GetAltitude
(
 taf_locGnss_SampleRef_t positionSampleRef,
int32_t* altitudePtr,
int32_t* vAccuracyPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetAltitude(positionSampleRef, altitudePtr, vAccuracyPtr);
}

/**
* FUNCTION     : GetLocation
* DESCRIPTION  : Get the location's data (Latitude, Longitude, Horizontal accuracy)
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
le_result_t taf_locGnss_GetLocation
(
 taf_locGnss_SampleRef_t positionSampleRef,
int32_t* latitudePtr,
int32_t* longitudePtr,
int32_t* hAccuracyPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetLocation(positionSampleRef, latitudePtr, longitudePtr, hAccuracyPtr);
}

/**
* FUNCTION     : RemovePositionHandler
* DESCRIPTION  : This function must be called to remove a handler for position notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Doesn't return on failure, so there's no need to check the return value for errors
*/
void taf_locGnss_RemovePositionHandler
(
 taf_locGnss_PositionHandlerRef_t handlerRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.RemovePositionHandler(handlerRef);
}

/**
* FUNCTION     : RemoveMeasurementHandler
* DESCRIPTION  : This function must be called to remove a handler for measurement notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Doesn't return on failure, so there's no need to check the return value for errors
*/
void taf_locGnss_RemoveMeasurementHandler
(
 taf_locGnss_MeasurementHandlerRef_t handlerRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.RemoveMeasurementHandler(handlerRef);
}

/**
* FUNCTION     : AddPositionHandler
* DESCRIPTION  : This function must be called to register an handler for position notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: A handler reference, which is only needed for later removal of the handler
*/
taf_locGnss_PositionHandlerRef_t taf_locGnss_AddPositionHandler
(
 taf_locGnss_PositionHandlerFunc_t handlerPtr,
 void* contextPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return (taf_locGnss_PositionHandlerRef_t)gnss.AddPositionHandler(handlerPtr, contextPtr);
}

/**
* FUNCTION     : AddMeasurementHandler
* DESCRIPTION  : This function must be called to register an handler for measurement notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: A handler reference, which is only needed for later removal of the handler
*/
taf_locGnss_MeasurementHandlerRef_t taf_locGnss_AddMeasurementHandler
(
 taf_locGnss_MeasurementHandlerFunc_t handlerPtr,
 void* contextPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return (taf_locGnss_MeasurementHandlerRef_t)gnss.AddMeasurementHandler(handlerPtr, contextPtr);
}

/**
* FUNCTION     : RemoveCapabilityHandler
* DESCRIPTION  : This function must be called to remove a handler for Capability notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Doesn't return on failure, so there's no need to check the return value for errors
*/
void taf_locGnss_RemoveCapabilityChangeHandler
(
 taf_locGnss_CapabilityChangeHandlerRef_t handlerRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.RemoveCapabilityHandler(handlerRef);
}

/**
* FUNCTION     : AddCapabilityHandler
* DESCRIPTION  : This function must be called to register an handler for Capability notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: A handler reference, which is only needed for later removal of the handler
*/
taf_locGnss_CapabilityChangeHandlerRef_t taf_locGnss_AddCapabilityChangeHandler
(
 taf_locGnss_CapabilityChangeHandlerFunc_t handlerPtr,
 void* contextPtr
)
{
    le_event_HandlerRef_t handlerRef;
    auto &gnss = taf_locGnss::GetInstance();
    handlerRef = (le_event_HandlerRef_t)gnss.AddCapabilityHandler(handlerPtr, contextPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_locGnss_CapabilityChangeHandlerRef_t)(handlerRef);
}

/**
* FUNCTION     : RemoveNmeaHandler
* DESCRIPTION  : This function must be called to remove a handler for NMEA notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Doesn't return on failure, so there's no need to check the return value for errors
*/
void taf_locGnss_RemoveNmeaHandler
(
 taf_locGnss_NmeaHandlerRef_t handlerRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.RemoveNmeaHandler(handlerRef);
}

/**
* FUNCTION     : AddNmeaHandler
* DESCRIPTION  : This function must be called to register an handler for NMEA notifications
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: A handler reference, which is only needed for later removal of the handler
*/
taf_locGnss_NmeaHandlerRef_t taf_locGnss_AddNmeaHandler
(
 taf_locGnss_NmeaHandlerFunc_t handlerPtr,
 void* contextPtr
)
{
    le_event_HandlerRef_t handlerRef;
    auto &gnss = taf_locGnss::GetInstance();
    handlerRef = (le_event_HandlerRef_t)gnss.AddNmeaHandler(handlerPtr, contextPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_locGnss_NmeaHandlerRef_t)(handlerRef);
}

/**
* FUNCTION     : Enable
* DESCRIPTION  : This function enables the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
le_result_t taf_locGnss_Enable
(
 void
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.Enable();
}

/**
* FUNCTION     : SetConstellation
* DESCRIPTION  : Set the GNSS constellation bit mask
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed with reason
*/
le_result_t taf_locGnss_SetConstellation
(
taf_locGnss_ConstellationBitMask_t constellationMask
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.SetConstellation( constellationMask);
}

/**
* FUNCTION     : Start
* DESCRIPTION  : This function starts the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
le_result_t taf_locGnss_Start
(
 void
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.Start();
}

/**
* FUNCTION     : GetConstellation
* DESCRIPTION  : Get the GNSS constellation bit mask
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed with reason
*/
le_result_t taf_locGnss_GetConstellation
(
taf_locGnss_ConstellationBitMask_t *constellationMaskPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetConstellation(constellationMaskPtr);
}

/**
* FUNCTION     : Disable
* DESCRIPTION  : This function disables the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_DUPLICATE LE_NOT_PERMITTED on failed with reason
*/
le_result_t taf_locGnss_Disable
(
void
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.Disable();
}

/**
* FUNCTION     : Stop
* DESCRIPTION  : This function stops the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_DUPLICATE LE_NOT_PERMITTED on failed with reason
*/
le_result_t taf_locGnss_Stop
(
 void
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.Stop();
}

/**
* FUNCTION     : GetState
* DESCRIPTION  : This function returns the state of the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: returns current state
*/
taf_locGnss_State_t taf_locGnss_GetState
(
 void
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetState();
}

/**
* FUNCTION     : GetSatellitesStatus
* DESCRIPTION  : Get the Satellites Vehicle status
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failure
*/
le_result_t taf_locGnss_GetSatellitesStatus
(
taf_locGnss_SampleRef_t positionSampleRef,
uint8_t* satsInViewCountPtr,
uint8_t* satsTrackingCountPtr,
uint8_t* satsUsedCountPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetSatellitesStatus(positionSampleRef, satsInViewCountPtr, satsTrackingCountPtr, satsUsedCountPtr);
}

/**
* FUNCTION     : GetAcquisitionRate
* DESCRIPTION  : This function gets the GNSS device acquisition rate
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failure, LE_NOT_PERMITTED If the GNSS device is not in "ready" state
*/
le_result_t taf_locGnss_GetAcquisitionRate
(
 uint32_t* ratePtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetAcquisitionRate(ratePtr);
}

/**
* FUNCTION     : GetTtff
* DESCRIPTION  : Get the TTFF in milliseconds
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_BUSY LE_NOT_PERMITTED LE_FAULT on failed with reason
*/
le_result_t taf_locGnss_GetTtff
(
 uint32_t* ttffPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetTtff(ttffPtr);
}

/**
* FUNCTION     : GetSatellitesInfo
* DESCRIPTION  : Get the Satellites Vehicle information
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on fail, LE_OUT_OF_RANGE retrieved parameters is invalid
*/
le_result_t taf_locGnss_GetSatellitesInfo
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint16_t* satIdPtr,
    size_t* satIdNumPtr,
    taf_locGnss_Constellation_t* satConstPtr,
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
    auto &gnss = taf_locGnss::GetInstance();
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
taf_locGnss_SampleRef_t taf_locGnss_GetLastSampleRef
(
    void
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetLastSampleRef();
}

/**
* FUNCTION     : GetPositionState
* DESCRIPTION  : This function gets the position sample's fix state
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success,LE_FAULT on failure
*/
le_result_t taf_locGnss_GetPositionState
(
    taf_locGnss_SampleRef_t positionSampleRef,

    taf_locGnss_FixState_t* statePtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
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
void taf_locGnss_ReleaseSampleRef
(
 taf_locGnss_SampleRef_t    positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.ReleaseSampleRef(positionSampleRef);
}

/**
* FUNCTION     : GetTimeAccuracy
* DESCRIPTION  : Get the position sample's time accurary
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
le_result_t taf_locGnss_GetTimeAccuracy
(
 taf_locGnss_SampleRef_t    positionSampleRef,
 uint32_t* timeAccuracyPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetTimeAccuracy(positionSampleRef, timeAccuracyPtr);
}

/**
* FUNCTION     : GetEpochTime
* DESCRIPTION  : Get the position sample's epoch time
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
le_result_t taf_locGnss_GetEpochTime
(
 taf_locGnss_SampleRef_t    positionSampleRef,
 uint64_t* millisecondsPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetEpochTime(positionSampleRef, millisecondsPtr);
}

/**
* FUNCTION     : SetDopResolution
* DESCRIPTION  : Set the resolution for the DOP parameters
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_BAD_PARAMETER on failed
*/
le_result_t taf_locGnss_SetDopResolution
(
 taf_locGnss_Resolution_t resolution
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.SetDopResolution(resolution);
}

/**
* FUNCTION     : GetDilutionOfPrecision
* DESCRIPTION  : Get the DOP parameter (Dilution Of Precision) for the fixed position
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
le_result_t taf_locGnss_GetDilutionOfPrecision
(
 taf_locGnss_SampleRef_t    positionSampleRef,
 taf_locGnss_DopType_t dopType,
 uint16_t* dopPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetDilutionOfPrecision(positionSampleRef, dopType, dopPtr);
}

/**
* FUNCTION     : GetLeapSeconds
* DESCRIPTION  : This function gets leap seconds information
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success,LE_FAULT on failed
*/
le_result_t taf_locGnss_GetLeapSeconds
(
 uint64_t* gpsTimePtr,
 int32_t* currentLeapSecondsPtr,
 uint64_t* changeEventTimePtr,
 int32_t* nextLeapSecondsPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetLeapSeconds(gpsTimePtr,currentLeapSecondsPtr,changeEventTimePtr,nextLeapSecondsPtr);
}

/**
* FUNCTION     : GetGpsTime
* DESCRIPTION  : Get the position sample's GPS time
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
le_result_t taf_locGnss_GetGpsTime
(
 taf_locGnss_SampleRef_t    positionSampleRef,
 uint32_t* gpsWeekPtr,
 uint32_t* gpsTimeOfWeekPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetGpsTime(positionSampleRef, gpsWeekPtr, gpsTimeOfWeekPtr);
}

/**
* FUNCTION     : SetAcquisitionRate
* DESCRIPTION  : This function sets the GNSS device acquisition rate
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE LE_NOT_PERMITTED LE_UNSUPPORTED on failed
*/
le_result_t taf_locGnss_SetAcquisitionRate
(
 uint32_t  rate
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.SetAcquisitionRate(rate);
}

/**
* FUNCTION     : ForceColdRestart
* DESCRIPTION  : This function performs cold restart,Delete all Aiding data
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed
*/
le_result_t taf_locGnss_ForceColdRestart
(
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.ForceColdRestart();
}

/**
* FUNCTION     : ForceWarmRestart
* DESCRIPTION  : This function performs warm restart, delete aiding data
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed
*/
le_result_t taf_locGnss_ForceWarmRestart
(
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.ForceWarmRestart();
}

/**
* FUNCTION     : ForceFactoryRestart
* DESCRIPTION  :
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed
*/
le_result_t taf_locGnss_ForceFactoryRestart
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
le_result_t taf_locGnss_ForceHotRestart
(
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.ForceHotRestart();
}

/**
* FUNCTION     : GetSupportedConstellations
* DESCRIPTION  : Returns all supported satellite constellations
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed
*/
le_result_t taf_locGnss_GetSupportedConstellations
(
 taf_locGnss_ConstellationBitMask_t* constellationMaskPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetSupportedConstellations(constellationMaskPtr);
}

/**
* FUNCTION     : SetMinElevation
* DESCRIPTION  : sets the GNSS minimum elevation
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT LE_UNSUPPORTED on failed
*/
le_result_t taf_locGnss_SetMinElevation
(
 uint8_t  minElevation
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.SetMinElevation(minElevation);
}

/**
* FUNCTION     : StartMode
* DESCRIPTION  : starts the GNSS device in the specified start mode
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_NOT_PERMITTED LE_FAULT LE_UNSUPPORTED on failed
*/
le_result_t taf_locGnss_StartMode
(
 taf_locGnss_StartMode_t  mode
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.StartMode(mode);
}

/**
* FUNCTION     : GetMinElevation
* DESCRIPTION  : gets the GNSS minimum elevation
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED on failed
*/
le_result_t taf_locGnss_GetMinElevation
(
 uint8_t*  minElevationPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetMinElevation(minElevationPtr);
}

/**
* FUNCTION     : SetNmeaSentences
* DESCRIPTION  : sets the enabled NMEA sentences using a bit mask.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed
*/

le_result_t taf_locGnss_SetNmeaSentences
(
    taf_locGnss_NmeaBitMask_t nmeaMask ///< [IN] Bit mask for enabled NMEA sentences.
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.SetNmeaSentences(nmeaMask);
}

/**
* FUNCTION     : GetNmeaSentences
* DESCRIPTION  : Gets the bit mask for the enabled NMEA sentences.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed
*/
le_result_t taf_locGnss_GetNmeaSentences
(
    taf_locGnss_NmeaBitMask_t* nmeaMaskPtr ///< [OUT] Bit mask for enabled NMEA sentences.
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetNmeaSentences(nmeaMaskPtr);
}

/**
* FUNCTION     : SetDRConfig
* DESCRIPTION  : Set the Dead Reckoing configuration Parameters
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_locGnss_SetDRConfig
(
    const taf_locGnss_DrParams_t* drParamsPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.SetDRConfig(drParamsPtr);
}
/**
* FUNCTION     : GetSupportedNmeaSentences
* DESCRIPTION  : Gets the bit mask for the supported NMEA sentences.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED on failed
*/
le_result_t taf_locGnss_GetSupportedNmeaSentences
(
    taf_locGnss_NmeaBitMask_t* nmeaMaskPtr ///< [OUT] Bit mask for supported NMEA sentences.
)
{
    auto &gnss = taf_locGnss::GetInstance();
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
le_result_t taf_locGnss_ConfigureEngineState
(
    taf_locGnss_EngineType_t engtype,
    taf_locGnss_EngineState_t engState
)
{
    auto &gnss = taf_locGnss::GetInstance();
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
le_result_t taf_locGnss_ConfigureRobustLocation
(
    uint8_t enable,
    uint8_t enabled911
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.ConfigureRobustLocation(enable,enabled911);
}

/**
* FUNCTION     : RobustLocationInformation
* DESCRIPTION  : Get the Robust Location information for Enable/Disable, 911 enable or disable, major & minor version numbers
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
le_result_t taf_locGnss_RobustLocationInformation
(
   uint8_t* enable,
   uint8_t* enabled911,
   uint8_t* majorVersion,
   uint8_t* minorVersion
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.RobustLocationInformation(enable,enabled911,majorVersion,minorVersion);
}

/**
* FUNCTION     : DefaultSecondaryBandConstellations
* DESCRIPTION  : Set the Secondary Band Empty Constellations
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed with reason
*/
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
le_result_t taf_locGnss_DefaultSecondaryBandConstellations
(
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.DefaultSecondaryBandConstellations();
}
#endif
/**
* FUNCTION     : RequestSecondaryBandConstellation
* DESCRIPTION  : Get the Secondary Band GNSS constellation type disabled
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed with reason
*/
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
le_result_t taf_locGnss_RequestSecondaryBandConstellations
(
   uint32_t* constellationSb
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.RequestSecondaryBandConstellations(constellationSb);
}
#endif
/**
* FUNCTION     : ConfigureSecondaryBandConstellation
* DESCRIPTION  : Configure Secondary Band GNSS constellation type to be disabled
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed with reason
*/
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
le_result_t taf_locGnss_ConfigureSecondaryBandConstellations
(
    uint32_t constellationSb
)
{
    auto &gnss = taf_locGnss::GetInstance();
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
le_result_t taf_locGnss_GetMagneticDeviation
(
    taf_locGnss_SampleRef_t positionSampleRef,
    int32_t* magneticDeviationPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetMagneticDeviation(positionSampleRef,magneticDeviationPtr);
}

/**
* FUNCTION     : GetEllipticalUncertainty
* DESCRIPTION  : Get the semi-major and semi-minor horizontal elliptical uncertainty.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
le_result_t taf_locGnss_GetEllipticalUncertainty
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint32_t* horUncEllipseSemiMajorPtr,
    uint32_t* horUncEllipseSemiMinorPtr,
    uint8_t*  horConfidencePtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
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
le_result_t taf_locGnss_SetLeverArmConfig
(
    const taf_locGnss_LeverArmParams_t* LeverArmParamsPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.SetLeverArmConfig(LeverArmParamsPtr);
}

/**
* FUNCTION     : SetEngineType
* DESCRIPTION  : This function sets the GNSS device with engine type
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_BAD_PARAMETER LE_NOT_PERMITTED on failed with reason
*/
le_result_t taf_locGnss_SetEngineType
(
    taf_locGnss_EngineReportsType_t EngineType
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.SetEngineType(EngineType);
}

/**
* FUNCTION     : GetConformityIndex
* DESCRIPTION  : This function gets the conformity index
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_locGnss_GetConformityIndex
(
 taf_locGnss_SampleRef_t positionSampleRef,
double* indexPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetConformityIndex(positionSampleRef,indexPtr);
}

/**
* FUNCTION     : GetCalibrationData
* DESCRIPTION  : This function gets the sensor calibration status and confidence percent.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_locGnss_GetCalibrationData
(
 taf_locGnss_SampleRef_t positionSampleRef,
uint32_t* calibPtr,
uint8_t* percentPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetCalibrationData(positionSampleRef,calibPtr,percentPtr);
}

/**
* FUNCTION     : GetBodyFrameData
* DESCRIPTION  : This function gets the body frame data
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_locGnss_GetBodyFrameData
(
    taf_locGnss_SampleRef_t positionSampleRef,
taf_locGnss_KinematicsData_t* bodyDataPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetBodyFrameData(positionSampleRef,bodyDataPtr);
}

/**
* FUNCTION     : GetVRPBasedLLA
* DESCRIPTION  : This function gets Vechile reference point based latitude,longitude & altitude information
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_locGnss_GetVRPBasedLLA
(
    taf_locGnss_SampleRef_t positionSampleRef,
    double* vrpLatitudePtr,
    double* vrpLongitudePtr,
    double* vrpAttitudePtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetVRPBasedLLA(positionSampleRef,vrpLatitudePtr,vrpLongitudePtr,vrpAttitudePtr);
}

/**
* FUNCTION     : GetVRPBasedVelocity
* DESCRIPTION  : This function gets Vehicle reference point based east,north & up velocity information
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_locGnss_GetVRPBasedVelocity
(
    taf_locGnss_SampleRef_t positionSampleRef,
    double* eastVelPtr,
    double* northVelPtr,
    double* upVelPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetVRPBasedVelocity(positionSampleRef,eastVelPtr,northVelPtr,upVelPtr);
}

/**
* FUNCTION     : GetSvUsedInPosition
* DESCRIPTION  : This function gets the set of satellite vehicles that are used to calculate position.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_locGnss_GetSvUsedInPosition
(
    taf_locGnss_SampleRef_t positionSampleRef,
    taf_locGnss_SvUsedInPosition_t* svDataPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetSvUsedInPosition(positionSampleRef,svDataPtr);
}

/**
* FUNCTION     : GetSbasCorrection
* DESCRIPTION  : This function gets navigation solution mask used to indicate SBAS corrections.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_locGnss_GetSbasCorrection
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint32_t* sbasMaskPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetSbasCorrection(positionSampleRef,sbasMaskPtr);
}

/**
* FUNCTION     : GetPositionTechnology
* DESCRIPTION  : This function gets technology mask to indicate which technology is used.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_locGnss_GetPositionTechnology
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint32_t* techMaskPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetPositionTechnology(positionSampleRef,techMaskPtr);
}

/**
* FUNCTION     : GetLocationInfoValidity
* DESCRIPTION  : This function gets the validity of the Location basic Info.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_locGnss_GetLocationInfoValidity
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint32_t* validityMaskPtr,
    uint64_t* validityExMaskPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
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
le_result_t taf_locGnss_GetLocationOutputEngParams
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint16_t* engMaskPtr,
    uint16_t* locationEngTypePtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetLocationOutputEngParams(positionSampleRef,engMaskPtr,locationEngTypePtr);
}

/**
* FUNCTION     : GetReliabilityInformation
* DESCRIPTION  : This function gets the reliability of the horizontal & vertical positions.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_locGnss_GetReliabilityInformation
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint16_t* horiReliabilityPtr,
    uint16_t* vertReliabilityPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
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
le_result_t taf_locGnss_GetStdDeviationAzimuthInfo
(
    taf_locGnss_SampleRef_t positionSampleRef,
    double* azimuthPtr,
    double* eastDevPtr,
    double* northDevPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetStdDeviationAzimuthInfo(positionSampleRef,azimuthPtr,eastDevPtr,northDevPtr);
}

/**
* FUNCTION     : GetRealTimeInformation
* DESCRIPTION  : This function gets elapsed real time and its uncertainity values in nano-second.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_locGnss_GetRealTimeInformation
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint64_t* realTimePtr,
    uint64_t* realTimeUncPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
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
le_result_t taf_locGnss_GetMeasurementUsageInfo
(
    taf_locGnss_SampleRef_t positionSampleRef,
    taf_locGnss_GnssMeasurementInfo_t* LE_NONNULL measInfoPtr,
    size_t* measInfoLen
)
{
    TAF_ERROR_IF_RET_VAL(positionSampleRef == NULL, LE_BAD_PARAMETER, "Invalid gnss sample reference");
    TAF_ERROR_IF_RET_VAL(measInfoPtr == NULL, LE_NO_MEMORY, "measInfoPtr is NULL");
    TAF_ERROR_IF_RET_VAL(*measInfoLen == 0, LE_OUT_OF_RANGE, "measInfoLen is ZERO");
    TAF_ERROR_IF_RET_VAL(*measInfoLen > TAF_LOCGNSS_MEASUREMENT_INFO_MAX, LE_OVERFLOW, "Too many elements.");

    auto &gnss = taf_locGnss::GetInstance();
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
le_result_t taf_locGnss_GetReportStatus
(
    taf_locGnss_SampleRef_t positionSampleRef,
    int32_t* reportStatusPtr
)
{
    TAF_ERROR_IF_RET_VAL(positionSampleRef == NULL, LE_BAD_PARAMETER, "Invalid gnss sample reference");
    TAF_ERROR_IF_RET_VAL(reportStatusPtr == NULL, LE_NO_MEMORY, "reportStatusPtr is NULL");

    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetReportStatus(positionSampleRef, reportStatusPtr);
}

/**
* FUNCTION     : GetAltitudeMeanSeaLevel
* DESCRIPTION  : This function retrieves the altitude with respect to mean sea level in Meters.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_BAD_PARAMETER, LE_NO_MEMORY on failed with reason
*/
le_result_t taf_locGnss_GetAltitudeMeanSeaLevel
(
    taf_locGnss_SampleRef_t positionSampleRef,
    double* altMeanSeaLevelPtr
)
{
    TAF_ERROR_IF_RET_VAL(positionSampleRef == NULL, LE_BAD_PARAMETER, "Invalid gnss sample reference");
    TAF_ERROR_IF_RET_VAL(altMeanSeaLevelPtr == NULL, LE_NO_MEMORY, "altMeanSeaLevelPtr is NULL");

    auto &gnss = taf_locGnss::GetInstance();
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
le_result_t taf_locGnss_GetSVIds
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint16_t* LE_NONNULL sVIdsPtr,
    size_t* sVIdsLen
)
{
    TAF_ERROR_IF_RET_VAL(positionSampleRef == NULL, LE_BAD_PARAMETER, "Invalid gnss sample reference");
    TAF_ERROR_IF_RET_VAL(sVIdsPtr == NULL, LE_NO_MEMORY, "measInfoPtr is NULL");
    TAF_ERROR_IF_RET_VAL(*sVIdsLen == 0, LE_OUT_OF_RANGE, "measInfoLen is ZERO");
    TAF_ERROR_IF_RET_VAL(*sVIdsLen > TAF_LOCGNSS_MEASUREMENT_INFO_MAX, LE_OVERFLOW, "Too many elements.");

    auto &gnss = taf_locGnss::GetInstance();
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
le_result_t taf_locGnss_GetSatellitesInfoEx
(
    taf_locGnss_SampleRef_t positionSampleRef,
    taf_locGnss_Constellation_t constellation,
    taf_locGnss_SvInfo_t* LE_NONNULL svInfoPtr,
    size_t* svInfoLen
)
{
    TAF_ERROR_IF_RET_VAL(positionSampleRef == NULL, LE_BAD_PARAMETER, "Invalid gnss sample reference");
    TAF_ERROR_IF_RET_VAL(svInfoPtr == NULL, LE_NO_MEMORY, "svInfoPtr is NULL");
    TAF_ERROR_IF_RET_VAL(*svInfoLen == 0, LE_OUT_OF_RANGE, "svInfoLen is ZERO");

    auto &gnss = taf_locGnss::GetInstance();
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
le_result_t taf_locGnss_SetMinGpsWeek
(
    uint16_t minGpsWeek
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.SetMinGpsWeek(minGpsWeek);
}

/**
* FUNCTION     : GetMinGpsWeek
* DESCRIPTION  : This function gets the minimum GPS week.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_NO_MEMORY, LE_NOT_PERMITTED on failed.
*/
le_result_t taf_locGnss_GetMinGpsWeek
(
 uint16_t*  minGpsWeekPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetMinGpsWeek(minGpsWeekPtr);
}

/**
* FUNCTION     : GetCapabilities
* DESCRIPTION  : This function gets the GNSS capability information.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed.
*/
le_result_t taf_locGnss_GetCapabilities
(
 uint64_t*  locCapabilityPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
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

le_result_t taf_locGnss_SetNmeaConfiguration
(
    taf_locGnss_NmeaBitMask_t nmeaMask,         ///< [IN] Bit mask for enabled NMEA sentences.
    taf_locGnss_GeodeticDatumType_t datumType,  ///< [IN] Specify the datum type to be configured.
    taf_locGnss_LocEngineType_t engineType      ///< [IN] Specify the Engine type.
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.SetNmeaConfiguration(nmeaMask, datumType, engineType);
}

/**
* FUNCTION     : GetXtraStatus
* DESCRIPTION  : Gets the Xtra status.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_NOT_PERMITTED on failed
*/
le_result_t taf_locGnss_GetXtraStatus
(
    taf_locGnss_XtraStatusParams_t* xtraParams //Specify Xtra assistant data's current status,
                                            // validity and whether it is enabled.
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetXtraStatus(xtraParams);
}

/**
* FUNCTION     : GetGnssData
* DESCRIPTION  : Get GNSS data for data mask, jammer indication and agc.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_NO_MEMORY on failed
*/
le_result_t taf_locGnss_GetGnssData
(
    taf_locGnss_SampleRef_t positionSampleRef,
    taf_locGnss_GnssData_t* gnssDataPtr,
    size_t* maxSignalTypes
)
{
    auto &gnss = taf_locGnss::GetInstance();
    TAF_ERROR_IF_RET_VAL(gnssDataPtr == NULL, LE_NO_MEMORY, "gnssDataPtr is NULL");
    return gnss.GetGnssData(positionSampleRef,gnssDataPtr,maxSignalTypes);
}
/**
* FUNCTION     : SetDRConfigValidity
* DESCRIPTION  : Sets the dead reckoning parameters validity mask
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed with reason
*/
le_result_t taf_locGnss_SetDRConfigValidity
(
    taf_locGnss_DRConfigValidityType_t validMask
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.SetDRConfigValidity(validMask);
}
/**
* FUNCTION     : GetGptpTime
* DESCRIPTION  : Gets Gptp time and its uncertainity.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed
*/
le_result_t taf_locGnss_GetGptpTime
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint64_t* gPtpTime,
    uint64_t* gPtpTimeUnc
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetGptpTime(positionSampleRef,gPtpTime,gPtpTimeUnc);
}

/**
* FUNCTION     : DeleteDRSensorCalData
* DESCRIPTION  : This function deletes dead reckoning sensor calibration data.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_NOT_PERMITTED on failed
*/
le_result_t taf_locGnss_DeleteDRSensorCalData
(
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.DeleteDRSensorCalData();
}

/**
* FUNCTION     : GetDRSolutionStatus
* DESCRIPTION  : This function gets the dead reckoning sensor solution status.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
le_result_t taf_locGnss_GetDRSolutionStatus
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint32_t* solutionStatusPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetDRSolutionStatus(positionSampleRef,solutionStatusPtr);
}

/**
* FUNCTION     : GetLeapSecondsUncertainty
* DESCRIPTION  : Gets leap seconds uncertainty.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE and LE_FAULT on failed
*/
le_result_t taf_locGnss_GetLeapSecondsUncertainty
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint8_t* leapSecondsUncPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetLeapSecondsUncertainty(positionSampleRef,leapSecondsUncPtr);
}

/**
* FUNCTION     : GetIsNHz
* DESCRIPTION  : Gets the frequency for GNSS measurements generated at NHz or not.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success and LE_FAULT on failed
*/
le_result_t taf_locGnss_GetIsNHz
(
    taf_locGnss_MeasSampleRef_t measSampleRef,
        ///< [IN] Measurement sample reference.
    bool* isNHZPtr
        ///< [OUT] Frequency generated at NHz or not .
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetIsNHz(measSampleRef,isNHZPtr);
}

/**
* FUNCTION     : GetClockValidityMask
* DESCRIPTION  : Gets the values of GnssMeasurements Clock validity mask.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success and LE_FAULT on failed
*/
//--------------------------------------------------------------------------------------------------
/**
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_locGnss_GetClockValidityMask
(
    taf_locGnss_MeasSampleRef_t measSampleRef,
        ///< [IN] Measurement sample reference.
    uint32_t* clockValidityMaskPtr
        ///< [OUT] ClockValidity Mask.
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetClockValidityMask(measSampleRef,clockValidityMaskPtr);
}

/**
* FUNCTION     : GetClockData
* DESCRIPTION  : Gets the GNSS measurements clock data.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success and LE_FAULT on failed
*/
//--------------------------------------------------------------------------------------------------
/**
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_locGnss_GetClockData
(
    taf_locGnss_MeasSampleRef_t measSampleRef,
        ///< [IN] Measurement sample reference.
    taf_locGnss_ClockData_t * clockDataPtr
        ///< [OUT] GnssMeasurementClockData
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetClockData(measSampleRef,clockDataPtr);
}

/**
* FUNCTION     : GetMeasurementsData
* DESCRIPTION  : Gets the Specify the signal measurement information such as satellite vehicle pseudo range,
*                satellite vehicle time, carrier phase measurement etc. from GNSS positioning engine.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success and LE_FAULT on failed
*/
//--------------------------------------------------------------------------------------------------
/**
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_locGnss_GetMeasurementsData
(
    taf_locGnss_MeasSampleRef_t measSampleRef,
        ///< [IN] Measurement sample reference.
    taf_locGnss_MeasurementsData_t* measDataPtr,
        ///< [OUT] GnssMeasurementData
    size_t* measDataSizePtr
        ///< [INOUT]
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetMeasurementsData(measSampleRef,measDataPtr, measDataSizePtr);
}

/**
* FUNCTION     : ReleaseMeasSampleRef
* DESCRIPTION  : This function must be called to release the measurement sample
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: If the caller is passing an invalid measurement reference into this function,
*                it is a fatal error, the function will not return.
*/
void taf_locGnss_ReleaseMeasSampleRef
(
    taf_locGnss_MeasSampleRef_t    measSampleRef
    ///< [IN] Measurement sample reference.
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.ReleaseMeasSampleRef(measSampleRef);
}

le_result_t taf_locGnss_GetMeasDataValidityMask
(
    taf_locGnss_MeasSampleRef_t measSampleRef,
        ///< [IN] Measurement sample reference.
    uint32_t* measDataValidityMaskPtr,
        ///< [OUT] GnssMeasurementData
    size_t* measDataValidityMaskSizePtr
        ///< [INOUT]
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.GetMeasDataValidityMask(measSampleRef, measDataValidityMaskPtr,
            measDataValidityMaskSizePtr);
}