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
 *  Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
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

void taf_locPosCtrl_Request (
    taf_locPosCtrl_ServerCmdRef_t cmdRef
)
{
    auto &pos = taf_locPos::GetInstance();
    pos.locPosCtrl_Request(cmdRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Releases the Positioning Control service.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_locPosCtrl_Release (
    taf_locPosCtrl_ServerCmdRef_t cmdRef,
    taf_locPosCtrl_ActivationRef_t ref ///< [IN] Reference to a Positioning Control service activation request.
)
{
    auto &pos = taf_locPos::GetInstance();
    pos.locPosCtrl_Release(cmdRef, ref);
}

taf_locPos_MovementHandlerRef_t taf_locPos_AddMovementHandler
(
    uint32_t horizontalMagnitude,
        ///< [IN] Horizontal magnitude in meters.
    uint32_t verticalMagnitude,
        ///< [IN] Vertical magnitude in meters.
    taf_locPos_MovementHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.AddMovementHandler(horizontalMagnitude, verticalMagnitude, handlerPtr, contextPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_locPos_Movement'
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_RemoveMovementHandler
(
    taf_locPos_MovementHandlerRef_t handlerRef
        ///< [IN]
)
{
    auto &pos = taf_locPos::GetInstance();
    return pos.RemoveMovementHandler(handlerRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the 2D location data (latitude, longitude, horizontal accuracy).
 *
 * @return
 * - LE_FAULT         Failed to get the 2D location data.
 * - LE_OUT_OF_RANGE  One, or more, retrieved parameters is invalid.
 * - LE_OK            Succeeded.
 *
 * @b NOTE: latitudePtr, longitudePtr, hAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_Get2DLocation (
    taf_locPos_ServerCmdRef_t cmdRef
)
{
    auto &pos = taf_locPos::GetInstance();
    int32_t latitude = 0;
    int32_t longitude = 0;
    int32_t hAccuracy = 0;
    auto res = pos.Get2DLocation(&latitude, &longitude, &hAccuracy);
    taf_locPos_Get2DLocationRespond(cmdRef,res,latitude,longitude,hAccuracy);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the date of the last updated location.
 *
 * @return
 * - LE_FAULT         Failed to get the date.
 * - LE_OUT_OF_RANGE  The retrieved date is invalid.
 * - LE_OK            Succeeded.
 *
 * @b NOTE: Currently the API implementation is in progress.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_GetDate (
    taf_locPos_ServerCmdRef_t cmdRef
)
{
    auto &pos = taf_locPos::GetInstance();
    uint16_t yearPtr = 0;
    uint16_t monthPtr = 0;
    uint16_t dayPtr = 0;
    auto result = pos.GetDate(&yearPtr, &monthPtr, &dayPtr);
    taf_locPos_GetDateRespond(cmdRef,result,yearPtr,monthPtr,dayPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the position fix state.
 *
 * @return
 * - LE_FAULT         Failed to get the position fix state.
 * - LE_OK            Succeeded.
 *
 * @b NOTE: In case the function fails to get the position fix state, a fatal error occurs,
 *       the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_GetFixState (
    taf_locPos_ServerCmdRef_t cmdRef
)
{
    auto &pos = taf_locPos::GetInstance();
    taf_locGnss_FixState_t statePtr = TAF_LOCGNSS_STATE_FIX_NO_POS;
    le_result_t result = pos.GetFixState(cmdRef,&statePtr);
    taf_locPos_GetFixStateRespond(cmdRef,result,statePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the motion data (horizontal speed, horizontal speed
 * accuracy, vertical speed, and vertical speed accuracy).
 *
 * @return
 * - LE_FAULT         Failed to get the motion data.
 * - LE_OUT_OF_RANGE  One, or more, of the retrieved parameters is invalid (set to INT32_MAX,
 *                    UINT32_MAX).
 * - LE_OK            Succeeded.
 *
 * @b NOTE: hSpeedPtr, hSpeedAccuracyPtr, vSpeedPtr, and vSpeedAccuracyPtr can be set to NULL if not
 *       needed.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_GetMotion (
    taf_locPos_ServerCmdRef_t cmdRef
)
{
    auto &pos = taf_locPos::GetInstance();
    uint32_t hSpeed = 0;
    uint32_t hSpeedAccuracy = 0;
    int32_t  vSpeed = 0;
    int32_t  vSpeedAccuracy = 0;
    auto res = pos.GetMotion(&hSpeed,&hSpeedAccuracy,&vSpeed,&vSpeedAccuracy);
    taf_locPos_GetMotionRespond(cmdRef,res,hSpeed,hSpeedAccuracy,vSpeed,vSpeedAccuracy);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the position sample's direction. Direction of movement is the direction that the vehicle or
 * person is actually moving.
 *
 * @return
 * - LE_FAULT         Failed to get the direction indication.
 * - LE_OUT_OF_RANGE  One, or more, of the retrieved parameters is invalid.
 * - LE_OK            Succeeded.
 *
 * @b NOTE: Direction is given in degrees.
 *       Direction ranges from 0 to 359 degrees, where 0 is true North degree.
 *
 * @b NOTE: directionPtr and directionAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_GetDirection (
    taf_locPos_ServerCmdRef_t cmdRef
)
{
    auto &pos = taf_locPos::GetInstance();
    uint32_t directionPtr = 0;
    uint32_t directionAccuracyPtr = 0;
    auto res = pos.GetDirection(&directionPtr,&directionAccuracyPtr);
    taf_locPos_GetDirectionRespond(cmdRef, res, directionPtr,directionAccuracyPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the position sample's time.
 *
 * @return
 * - LE_FAULT         Failed to get the time.
 * - LE_OUT_OF_RANGE  The retrieved time is invalid.
 * - LE_OK            Succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_GetTime (
    taf_locPos_ServerCmdRef_t cmdRef
)
{
    auto &pos = taf_locPos::GetInstance();
    uint16_t hrsPtr = 0;
    uint16_t minPtr = 0;
    uint16_t secPtr = 0;
    uint16_t msecPtr = 0;
    auto result = pos.GetTime(&hrsPtr, &minPtr, &secPtr, &msecPtr);
    taf_locPos_GetTimeRespond(cmdRef,result,hrsPtr,minPtr,secPtr,msecPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the acquisition rate.
 *
 * @return
 * - LE_OUT_OF_RANGE    Acquisition rate is invalid.
 * - LE_OK              Succeeded.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_SetAcquisitionRate (
    taf_locPos_ServerCmdRef_t cmdRef,
    uint32_t acquisitionRate ///< [IN] Acquisition Rate in milliseconds.
)
{
    auto &pos = taf_locPos::GetInstance();
    auto res = pos.SetAcquisitionRate(cmdRef, acquisitionRate);
    taf_locPos_SetAcquisitionRateRespond(cmdRef, res);
}
//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the acquisition rate in milliseconds.
 *
 * @return
 * Acquisition rate in milliseconds.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_GetAcquisitionRate (
    taf_locPos_ServerCmdRef_t cmdRef
)
{
    auto &pos = taf_locPos::GetInstance();
    uint32_t acqRate = pos.GetAcquisitionRate();
    taf_locPos_GetAcquisitionRateRespond(cmdRef, acqRate);
}
//--------------------------------------------------------------------------------------------------
/**
 * Releases the position sample.
 *
 * @b NOTE: If the caller is passing an invalid position reference to this function,
 *       it is a fatal error and the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_sample_Release (
    taf_locPos_ServerCmdRef_t cmdRef,
    taf_locPos_SampleRef_t positionSampleRef ///< [IN] Position sample reference.
)
{
    auto &pos = taf_locPos::GetInstance();
    pos.Release(cmdRef, positionSampleRef);
    taf_locPos_sample_ReleaseRespond(cmdRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the 3D location data (latitude, longitude, altitude,
 * horizontal accuracy, vertical accuracy)
 *
 * @return
 * - LE_FAULT         Failed to get the 3D location data.
 * - LE_OUT_OF_RANGE  One, or more, of the retrieved parameters is invalid.
 * - LE_OK            Succeeded.
 *
 * @b NOTE: latitudePtr, longitudePtr,hAccuracyPtr, altitudePtr, and vAccuracyPtr can be set to NULL
 *       if not needed.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_Get3DLocation (
    taf_locPos_ServerCmdRef_t cmdRef
)
{
    auto &pos = taf_locPos::GetInstance();
    int32_t latitudePtr = 0;
    int32_t longitudePtr = 0;
    int32_t hAccuracyPtr = 0;
    int32_t altitudePtr = 0;
    int32_t vAccuracyPtr = 0;
    auto res = pos.Get3DLocation(&latitudePtr,&longitudePtr,&hAccuracyPtr,&altitudePtr,&vAccuracyPtr);
    taf_locPos_Get3DLocationRespond(cmdRef, res, latitudePtr,longitudePtr,hAccuracyPtr,altitudePtr,vAccuracyPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the position sample's 2D location (latitude, longitude,
 * horizontal accuracy)
 *
 * @return
 * - LE_FAULT         Failed to find the positionSample.
 * - LE_OUT_OF_RANGE  One, or more, of the retrieved parameters is invalid.
 * - LE_OK            Suceeded.
 *
 * @b NOTE: If the caller passes an invalid position reference to this function,
 *       it is a fatal error and the function will not return.
 *
 * @b NOTE: latitudePtr, longitudePtr, and horizontalAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_sample_Get2DLocation (
    taf_locPos_ServerCmdRef_t cmdRef,
    taf_locPos_SampleRef_t positionSampleRef ///< [IN] Position sample reference.
)
{
    auto &pos = taf_locPos::GetInstance();
    int32_t latitudePtr = 0;
    int32_t longitudePtr = 0;
    int32_t hAccuracyPtr = 0;
    auto res = pos.sample_Get2DLocation(positionSampleRef,&latitudePtr,&longitudePtr,&hAccuracyPtr);
    taf_locPos_sample_Get2DLocationRespond(cmdRef,res,latitudePtr,longitudePtr,hAccuracyPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the position sample's altitude.
 *
 * @return
 * - LE_FAULT         Failed to find the positionSample.
 * - LE_OUT_OF_RANGE  One, or more, of the retrieved parameters is invalid.
 * - LE_OK            Suceeded.
 *
 * @b NOTE: If the caller passes an invalid position reference to this function,
 *       it is a fatal error and the function will not return.
 *
 * @b NOTE: altitudePtr and altitudeAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_sample_GetAltitude (
    taf_locPos_ServerCmdRef_t cmdRef,
    taf_locPos_SampleRef_t positionSampleRef ///< [IN] Position sample reference.
)
{
    auto &pos = taf_locPos::GetInstance();
    int32_t altitudePtr = 0;
    int32_t altitudeAccuracyPtr = 0;
    auto res = pos.sample_GetAltitude(positionSampleRef,&altitudePtr,&altitudeAccuracyPtr);
    taf_locPos_sample_GetAltitudeRespond(cmdRef, res, altitudePtr,altitudeAccuracyPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the position sample's time.
 *
 * @return
 * - LE_FAULT         Failed to get the time.
 * - LE_OUT_OF_RANGE  The retrieved time is invalid.
 * - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_sample_GetTime (
    taf_locPos_ServerCmdRef_t cmdRef,
    taf_locPos_SampleRef_t positionSampleRef ///< [IN] Position sample reference.
)
{
    auto &pos = taf_locPos::GetInstance();
    uint16_t hoursPtr = 0;
    uint16_t minutesPtr = 0;
    uint16_t secondsPtr = 0;
    uint16_t millisecondsPtr = 0;
    auto result = pos.sample_GetTime(positionSampleRef,&hoursPtr,&minutesPtr,&secondsPtr,&millisecondsPtr);
    taf_locPos_sample_GetTimeRespond(cmdRef,result,hoursPtr, minutesPtr,secondsPtr,millisecondsPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the position sample's horizontal speed.
 *
 * @return
 * - LE_FAULT         Failed to find the positionSample.
 * - LE_OUT_OF_RANGE  One, or more, of the retrieved parameters is invalid.
 * - LE_OK            Suceeded.
 *
 * @b NOTE: If the caller passes an invalid position reference to this function,
 *       it is a fatal error and the function will not return.
 *
 * @b NOTE: hSpeedPtr and hSpeedAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_sample_GetHorizontalSpeed (
    taf_locPos_ServerCmdRef_t cmdRef,
    taf_locPos_SampleRef_t positionSampleRef ///< [IN] Position sample reference.
)
{
    auto &pos = taf_locPos::GetInstance();
    uint32_t hSpeedPtr = 0;
    uint32_t hSpeedAccuracyPtr = 0;
    auto res = pos.sample_GetHorizontalSpeed(positionSampleRef,&hSpeedPtr,&hSpeedAccuracyPtr);
    taf_locPos_sample_GetHorizontalSpeedRespond(cmdRef, res, hSpeedPtr, hSpeedAccuracyPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's direction
 *
 * @return
 * - LE_FAULT         Failed to find the positionSample.
 * - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid.
 * - LE_OK            Suceeded.
 *
 * @b NOTE: Direction is given in degrees.
 *       Direction ranges from 0 to 359 degrees, where 0 is true North.
 *
 * @b NOTE: If the caller passed an invalid position reference to this function,
 *       it is a fatal error and the function will not return.
 *
 * @b NOTE: directionPtr and directionAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_sample_GetDirection (
    taf_locPos_ServerCmdRef_t cmdRef,
    taf_locPos_SampleRef_t positionSampleRef ///< [IN] Position sample reference.
)
{
    auto &pos = taf_locPos::GetInstance();
    uint32_t directionPtr = 0;
    uint32_t directionAccuracyPtr = 0;
    auto res = pos.sample_GetDirection(positionSampleRef,&directionPtr,&directionAccuracyPtr);
    taf_locPos_sample_GetDirectionRespond(cmdRef, res, directionPtr, directionAccuracyPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the position sample's vertical speed.
 *
 * @return
 * - LE_FAULT         Failed to find the positionSample.
 * - LE_OUT_OF_RANGE  One, or more, of the retrieved parameters is not valid.
 * - LE_OK            Suceeded.
 *
 * @b NOTE: If the caller passes an invalid position reference to this function,
 *       it is a fatal error and the function will not return.
 *
 * @b NOTE: vSpeedPtr and vSpeedAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_sample_GetVerticalSpeed (
    taf_locPos_ServerCmdRef_t cmdRef,
    taf_locPos_SampleRef_t positionSampleRef ///< [IN] Position sample reference
)
{
    auto &pos = taf_locPos::GetInstance();
    int32_t vSpeedPtr = 0;
    int32_t vSpeedAccuracyPtr = 0;
    auto result = pos.sample_GetVerticalSpeed(positionSampleRef, &vSpeedPtr, &vSpeedAccuracyPtr);
    taf_locPos_sample_GetVerticalSpeedRespond(cmdRef,result,vSpeedPtr,vSpeedAccuracyPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the resolution for the positioning distance values.
 *
 * @return
 * - LE_OK               Suceeded.
 * - LE_BAD_PARAMETER    Invalid parameter provided.
 *
 * @b NOTE: The positioning distance values are: the altitude above sea level, the horizontal
 *       position accuracy and the vertical position accuracy. The API sets the same resolution to
 *       all distance values. The resolution change request takes effect immediately.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_SetDistanceResolution (
    taf_locPos_ServerCmdRef_t cmdRef,
    taf_locPos_Resolution_t resolution ///< [IN] Resolution.
)
{
    auto &pos = taf_locPos::GetInstance();
    le_result_t result = pos.SetDistanceResolution(resolution);
    taf_locPos_SetDistanceResolutionRespond(cmdRef, result);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the position sample's fix state.
 *
 * @return
 * - LE_FAULT         Failed to get the position sample's fix state.
 * - LE_OK            Suceeded.
 *
 * @b NOTE: If the caller passes an invalid position reference to this function,
 *       it is a fatal error and the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_sample_GetFixState (
    taf_locPos_ServerCmdRef_t cmdRef,
    taf_locPos_SampleRef_t positionSampleRef ///< [IN] Position sample reference.
)
{
    auto &pos = taf_locPos::GetInstance();
    taf_locGnss_FixState_t statePtr = TAF_LOCGNSS_STATE_FIX_NO_POS;
    auto result = pos.sample_GetFixState(positionSampleRef,&statePtr);
    taf_locPos_sample_GetFixStateRespond(cmdRef,result,statePtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the position sample date.
 *
 * @return
 * - LE_FAULT         Failed to get the date.
 * - LE_OUT_OF_RANGE  The retrieved date is invalid.
 * - LE_OK            Suceeded.
 *
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos_sample_GetDate (
    taf_locPos_ServerCmdRef_t cmdRef,
    taf_locPos_SampleRef_t positionSampleRef ///< [IN] Position sample reference.
)
{
    auto &pos = taf_locPos::GetInstance();
    uint16_t yearPtr = 0;
    uint16_t monthPtr = 0;
    uint16_t dayPtr = 0;
    auto result = pos.sample_GetDate(positionSampleRef,&yearPtr,&monthPtr,&dayPtr);
    taf_locPos_sample_GetDateRespond(cmdRef, result, yearPtr,monthPtr,dayPtr);
}

/**
* FUNCTION     : GetDate
* DESCRIPTION  : Get the position sample's date
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
void taf_locGnss_GetDate
(
 taf_locGnss_ServerCmdRef_t cmdRef,
 taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint16_t year = 0;
    uint16_t month = 0;
    uint16_t day = 0;
    le_result_t res = gnss.GetDate(positionSampleRef, &year, &month, &day);
    taf_locGnss_GetDateRespond(cmdRef, res, year, month, day);
}

/**
* FUNCTION     : GetTime
* DESCRIPTION  : Get the position sample's time
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
void taf_locGnss_GetTime
(
 taf_locGnss_ServerCmdRef_t cmdRef,
 taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint16_t hours = 0;
    uint16_t minutes = 0;
    uint16_t seconds = 0;
    uint16_t milliseconds = 0;
    le_result_t res = gnss.GetTime(positionSampleRef, &hours, &minutes, &seconds, &milliseconds);
    taf_locGnss_GetTimeRespond(cmdRef, res, hours, minutes, seconds, milliseconds);
}

/**
* FUNCTION     : GetGpsLeapSeconds
* DESCRIPTION  : Get the position sample's UTC leap seconds in advance
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success,LE_OUT_OF_RANGE LE_FAULT on fail
*/
void taf_locGnss_GetGpsLeapSeconds
(
 taf_locGnss_ServerCmdRef_t cmdRef,
 taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint8_t leapSeconds = 0;
    le_result_t res = gnss.GetGpsLeapSeconds(positionSampleRef, &leapSeconds);
    taf_locGnss_GetGpsLeapSecondsRespond(cmdRef, res, leapSeconds);
}

/**
* FUNCTION     : GetDirection
* DESCRIPTION  : Get the position sample's direction. Direction of movement is the direction that the vehicle or
 *               person is actually moving.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
void taf_locGnss_GetDirection
(
 taf_locGnss_ServerCmdRef_t cmdRef,
 taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t direction = 0;
    uint32_t directionAccuracy = 0;
    le_result_t res = gnss.GetDirection(positionSampleRef, &direction, &directionAccuracy);
    taf_locGnss_GetDirectionRespond(cmdRef, res, direction, directionAccuracy);
}

/**
* FUNCTION     : GetVerticalSpeed
* DESCRIPTION  : Get the position sample's vertical speed
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT on fail
*/
void taf_locGnss_GetVerticalSpeed
(
 taf_locGnss_ServerCmdRef_t cmdRef,
 taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    int32_t vspeed = 0;
    int32_t vspeedAccuracy = 0;
    le_result_t res =  gnss.GetVerticalSpeed(positionSampleRef, &vspeed, &vspeedAccuracy);
    taf_locGnss_GetVerticalSpeedRespond(cmdRef, res, vspeed, vspeedAccuracy);
}

/**
* FUNCTION     : GetHorizontalSpeed
* DESCRIPTION  : Get the position sample's horizontal speed
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on fail
*/
void taf_locGnss_GetHorizontalSpeed
(
 taf_locGnss_ServerCmdRef_t cmdRef,
 taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t hspeed = 0;
    uint32_t hspeedAccuracy = 0;
    le_result_t res = gnss.GetHorizontalSpeed(positionSampleRef, &hspeed, &hspeedAccuracy);
    taf_locGnss_GetHorizontalSpeedRespond(cmdRef, res, hspeed, hspeedAccuracy);
}

/**
* FUNCTION     : GetAltitude
* DESCRIPTION  : Get the position sample's altitude
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on fail
*/
void taf_locGnss_GetAltitude
(
 taf_locGnss_ServerCmdRef_t cmdRef,
 taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    int32_t altitude = 0;
    int32_t vAccuracy = 0;
    le_result_t res = gnss.GetAltitude(positionSampleRef, &altitude, &vAccuracy);
    taf_locGnss_GetAltitudeRespond(cmdRef, res, altitude, vAccuracy);
}

/**
* FUNCTION     : GetLocation
* DESCRIPTION  : Get the location's data (Latitude, Longitude, Horizontal accuracy)
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
void taf_locGnss_GetLocation
(
 taf_locGnss_ServerCmdRef_t cmdRef,
 taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    int32_t latitude = 0;
    int32_t longitude = 0;
    int32_t hAccuracy = 0;
    le_result_t res = gnss.GetLocation(positionSampleRef, &latitude, &longitude, &hAccuracy);
    taf_locGnss_GetLocationRespond(cmdRef, res, latitude, longitude, hAccuracy);
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

taf_locGnss_PositionExHandlerRef_t taf_locGnss_AddPositionExHandler
(
    taf_locGnss_PositionExHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return (taf_locGnss_PositionExHandlerRef_t)gnss.AddPositionExHandler(handlerPtr, contextPtr);
}


void taf_locGnss_RemovePositionExHandler
(
 taf_locGnss_PositionExHandlerRef_t handlerRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.RemovePositionExHandler(handlerRef);
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
    auto &gnss = taf_locGnss::GetInstance();
    return (taf_locGnss_CapabilityChangeHandlerRef_t)gnss.AddCapabilityHandler(handlerPtr, contextPtr);
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
    auto &gnss = taf_locGnss::GetInstance();
    return (taf_locGnss_NmeaHandlerRef_t)gnss.AddNmeaHandler(handlerPtr, contextPtr);
}

/**
* FUNCTION     : Enable
* DESCRIPTION  : This function enables the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
void taf_locGnss_Enable
(
 taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    le_result_t res = gnss.Enable();
    taf_locGnss_EnableRespond(cmdRef, res);
}

/**
* FUNCTION     : SetConstellation
* DESCRIPTION  : Set the GNSS constellation bit mask
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed with reason
*/
void taf_locGnss_SetConstellation
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_ConstellationBitMask_t constellationMask
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.SetConstellation(cmdRef, constellationMask);
}

/**
* FUNCTION     : Start
* DESCRIPTION  : This function starts the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
void taf_locGnss_Start
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.Start(cmdRef);
}

/**
* FUNCTION     : GetConstellation
* DESCRIPTION  : Get the GNSS constellation bit mask
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed with reason
*/
void taf_locGnss_GetConstellation
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_ConstellationBitMask_t constellationMask = 0;
    le_result_t res = gnss.GetConstellation(&constellationMask);
    taf_locGnss_GetConstellationRespond(cmdRef, res, constellationMask);
}

/**
* FUNCTION     : Disable
* DESCRIPTION  : This function disables the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_DUPLICATE LE_NOT_PERMITTED on failed with reason
*/
void taf_locGnss_Disable
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    le_result_t res = gnss.Disable();
    taf_locGnss_DisableRespond(cmdRef, res);
}

/**
* FUNCTION     : Stop
* DESCRIPTION  : This function stops the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_DUPLICATE LE_NOT_PERMITTED on failed with reason
*/
void taf_locGnss_Stop
(
 taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.Stop(cmdRef);
}

/**
* FUNCTION     : GetState
* DESCRIPTION  : This function returns the state of the GNSS device
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: returns current state
*/
void taf_locGnss_GetState
(
 taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_State_t state = gnss.GetState();
    taf_locGnss_GetStateRespond(cmdRef, state);
}

/**
* FUNCTION     : GetSatellitesStatus
* DESCRIPTION  : Get the Satellites Vehicle status
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failure
*/
void taf_locGnss_GetSatellitesStatus
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint8_t satsInViewCount = 0;
    uint8_t satsTrackingCount = 0;
    uint8_t satsUsedCount = 0;
    le_result_t res = gnss.GetSatellitesStatus(positionSampleRef, &satsInViewCount, &satsTrackingCount, &satsUsedCount);
    taf_locGnss_GetSatellitesStatusRespond(cmdRef, res, satsInViewCount, satsTrackingCount, satsUsedCount);
}

/**
* FUNCTION     : GetAcquisitionRate
* DESCRIPTION  : This function gets the GNSS device acquisition rate
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failure, LE_NOT_PERMITTED If the GNSS device is not in "ready" state
*/
void taf_locGnss_GetAcquisitionRate
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t rate = 0;
    le_result_t res = gnss.GetAcquisitionRate(&rate);
    taf_locGnss_GetAcquisitionRateRespond(cmdRef, res, rate);
}

/**
* FUNCTION     : GetTtff
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_BUSY LE_NOT_PERMITTED LE_FAULT on failed with reason
*/
void taf_locGnss_GetTtff
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t ttff = 0;
    le_result_t res = gnss.GetTtff(&ttff);
    taf_locGnss_GetTtffRespond(cmdRef, res, ttff);
}

/**
* FUNCTION     : GetSatellitesInfo
* DESCRIPTION  : Get the Satellites Vehicle information
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on fail, LE_OUT_OF_RANGE retrieved parameters is invalid
*/
void taf_locGnss_GetSatellitesInfo
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef,
    size_t satIdNum,
    size_t satConstNum,
    size_t satUsedNum,
    size_t satSnrNum,
    size_t satAzimNum,
    size_t satElevNum
)
{
    auto &gnss = taf_locGnss::GetInstance();

    uint16_t                    satId[satIdNum];
    taf_locGnss_Constellation_t satConst[satConstNum];
    bool                        satUsed[satUsedNum];
    uint8_t                     satSnr[satSnrNum];
    uint16_t                    satAzim[satAzimNum];
    uint8_t                     satElev[satElevNum];

    le_result_t res = gnss.GetSatellitesInfo(
        positionSampleRef,
        satId,    &satIdNum,
        satConst, &satConstNum,
        satUsed,  &satUsedNum,
        satSnr,   &satSnrNum,
        satAzim,  &satAzimNum,
        satElev,  &satElevNum);

    taf_locGnss_GetSatellitesInfoRespond(
        cmdRef, res,
        satId,    satIdNum,
        satConst, satConstNum,
        satUsed,  satUsedNum,
        satSnr,   satSnrNum,
        satAzim,  satAzimNum,
        satElev,  satElevNum);
}

/**
* FUNCTION     : GetLastSampleRef
* DESCRIPTION  : This function gets the last updated position sample object reference.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: A reference to last Position's sample
*/
void taf_locGnss_GetLastSampleRef
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_SampleRef_t ref = gnss.GetLastSampleRef();
    taf_locGnss_GetLastSampleRefRespond(cmdRef, ref);
}

/**
* FUNCTION     : GetPositionState
* DESCRIPTION  : This function gets the position sample's fix state
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success,LE_FAULT on failure
*/
void taf_locGnss_GetPositionState
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_FixState_t state = TAF_LOCGNSS_STATE_FIX_NO_POS;
    le_result_t res = gnss.GetPositionState(positionSampleRef, &state);
    taf_locGnss_GetPositionStateRespond(cmdRef, res, state);

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
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t    positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.ReleaseSampleRef(positionSampleRef);
    taf_locGnss_ReleaseSampleRefRespond(cmdRef);
}

void taf_locGnss_ReleaseSampleExRef
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleExRef_t    postitionSampleExRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.ReleaseSampleExRef(postitionSampleExRef);
    taf_locGnss_ReleaseSampleExRefRespond(cmdRef);
}

/**
* FUNCTION     : GetTimeAccuracy
* DESCRIPTION  : Get the position sample's time accurary
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
void taf_locGnss_GetTimeAccuracy
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t    positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t timeAccuracy = 0;
    le_result_t res = gnss.GetTimeAccuracy(positionSampleRef, &timeAccuracy);
    taf_locGnss_GetTimeAccuracyRespond(cmdRef, res, timeAccuracy);
}

/**
* FUNCTION     : GetEpochTime
* DESCRIPTION  : Get the position sample's epoch time
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
void taf_locGnss_GetEpochTime
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t    positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint64_t milliseconds = 0;
    le_result_t res = gnss.GetEpochTime(positionSampleRef, &milliseconds);
    taf_locGnss_GetEpochTimeRespond(cmdRef, res, milliseconds);
}

/**
* FUNCTION     : SetDopResolution
* DESCRIPTION  : Set the resolution for the DOP parameters
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_BAD_PARAMETER on failed
*/
void taf_locGnss_SetDopResolution
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_Resolution_t resolution
)
{
    auto &gnss = taf_locGnss::GetInstance();
    le_result_t res = gnss.SetDopResolution(resolution);
    taf_locGnss_SetDopResolutionRespond(cmdRef, res);
}

/**
* FUNCTION     : GetDilutionOfPrecision
* DESCRIPTION  : Get the DOP parameter (Dilution Of Precision) for the fixed position
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
void taf_locGnss_GetDilutionOfPrecision
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t    positionSampleRef,
    taf_locGnss_DopType_t dopType
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint16_t dop = 0;
    le_result_t res = gnss.GetDilutionOfPrecision(positionSampleRef, dopType, &dop);
    taf_locGnss_GetDilutionOfPrecisionRespond(cmdRef, res, dop);
}

/**
* FUNCTION     : GetLeapSeconds
* DESCRIPTION  : This function gets leap seconds information
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success,LE_FAULT on failed
*/
void taf_locGnss_GetLeapSeconds
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint64_t gpsTime = 0;
    int32_t currentLeapSeconds = 0;
    uint64_t changeEventTime = 0;
    int32_t nextLeapSeconds = 0;
    le_result_t res =  gnss.GetLeapSeconds(&gpsTime,&currentLeapSeconds,&changeEventTime,&nextLeapSeconds);
    taf_locGnss_GetLeapSecondsRespond(cmdRef, res, gpsTime,currentLeapSeconds,changeEventTime,nextLeapSeconds);
}

/**
* FUNCTION     : GetGpsTime
* DESCRIPTION  : Get the position sample's GPS time
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
void taf_locGnss_GetGpsTime
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t    positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t gpsWeek = 0;
    uint32_t gpsTimeOfWeek = 0;
    le_result_t res = gnss.GetGpsTime(positionSampleRef, &gpsWeek, &gpsTimeOfWeek);
    taf_locGnss_GetGpsTimeRespond(cmdRef, res, gpsWeek, gpsTimeOfWeek);
}

/**
* FUNCTION     : SetAcquisitionRate
* DESCRIPTION  : This function sets the GNSS device acquisition rate
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE LE_NOT_PERMITTED LE_UNSUPPORTED on failed
*/
void taf_locGnss_SetAcquisitionRate
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    uint32_t  rate
)
{
    auto &gnss = taf_locGnss::GetInstance();
    le_result_t res = gnss.SetAcquisitionRate(rate);
    taf_locGnss_SetAcquisitionRateRespond(cmdRef, res);
}

/**
* FUNCTION     : ForceColdRestart
* DESCRIPTION  : This function performs cold restart,Delete all Aiding data
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed
*/
void taf_locGnss_ForceColdRestart
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.ForceColdRestart(cmdRef);
}

/**
* FUNCTION     : ForceWarmRestart
* DESCRIPTION  : This function performs warm restart, delete aiding data
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed
*/
void taf_locGnss_ForceWarmRestart
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.ForceWarmRestart(cmdRef);
}

/**
* FUNCTION     : ForceFactoryRestart
* DESCRIPTION  :
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed
*/
void taf_locGnss_ForceFactoryRestart
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    LE_DEBUG("Feature not Supported");
    taf_locGnss_ForceFactoryRestartRespond(cmdRef, LE_UNSUPPORTED);
}

/**
* FUNCTION     : ForceHotRestart
* DESCRIPTION  :
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed
*/
void taf_locGnss_ForceHotRestart
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.ForceHotRestart(cmdRef);
}

/**
* FUNCTION     : GetSupportedConstellations
* DESCRIPTION  : Returns all supported satellite constellations
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed
*/
void taf_locGnss_GetSupportedConstellations
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_ConstellationBitMask_t constellationMask = 0;
    le_result_t res = gnss.GetSupportedConstellations(&constellationMask);
    taf_locGnss_GetSupportedConstellationsRespond(cmdRef, res, constellationMask);
}

/**
* FUNCTION     : SetMinElevation
* DESCRIPTION  : sets the GNSS minimum elevation
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE LE_FAULT LE_UNSUPPORTED on failed
*/
void taf_locGnss_SetMinElevation
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    uint8_t  minElevation
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.SetMinElevation(cmdRef, minElevation);
}

/**
* FUNCTION     : StartMode
* DESCRIPTION  : starts the GNSS device in the specified start mode
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_NOT_PERMITTED LE_FAULT LE_UNSUPPORTED on failed
*/
void taf_locGnss_StartMode
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_StartMode_t  mode
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.StartMode(cmdRef, mode);
}

/**
* FUNCTION     : GetMinElevation
* DESCRIPTION  : gets the GNSS minimum elevation
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED on failed
*/
void taf_locGnss_GetMinElevation
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint8_t minElevation = 0;
    le_result_t res = gnss.GetMinElevation(&minElevation);
    taf_locGnss_GetMinElevationRespond(cmdRef, res, minElevation);
}

/**
* FUNCTION     : SetNmeaSentences
* DESCRIPTION  : sets the enabled NMEA sentences using a bit mask.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed
*/

void taf_locGnss_SetNmeaSentences
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_NmeaBitMask_t nmeaMask ///< [IN] Bit mask for enabled NMEA sentences.
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.SetNmeaSentences(cmdRef, nmeaMask);
}

/**
* FUNCTION     : GetNmeaSentences
* DESCRIPTION  : Gets the bit mask for the enabled NMEA sentences.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed
*/
void taf_locGnss_GetNmeaSentences
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_NmeaBitMask_t nmeaMask = 0;
    le_result_t res = gnss.GetNmeaSentences(&nmeaMask);
    taf_locGnss_GetNmeaSentencesRespond(cmdRef, res, nmeaMask);
}

/**
* FUNCTION     : SetDRConfig
* DESCRIPTION  : Set the Dead Reckoing configuration Parameters
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_SetDRConfig
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    const taf_locGnss_DrParams_t* drParamsPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.SetDRConfig(cmdRef, drParamsPtr);
}
/**
* FUNCTION     : GetSupportedNmeaSentences
* DESCRIPTION  : Gets the bit mask for the supported NMEA sentences.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED on failed
*/
void taf_locGnss_GetSupportedNmeaSentences
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_NmeaBitMask_t nmeaMask = 0;
    le_result_t res = gnss.GetSupportedNmeaSentences(&nmeaMask);
    taf_locGnss_GetSupportedNmeaSentencesRespond(cmdRef, res, nmeaMask);
}
/**
* FUNCTION     : ConfigureEngineState
* DESCRIPTION  : Set the Engine state for SPE/PPE/DRE/VPE Engines
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
void taf_locGnss_ConfigureEngineState
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_EngineType_t engtype,
    taf_locGnss_EngineState_t engState
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.ConfigureEngineState(cmdRef, engtype,engState);
}
#endif
/**
* FUNCTION     : ConfigureRobustLocation
* DESCRIPTION  : Enable or Disable Robust Location for 911 enable or disable
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
void taf_locGnss_ConfigureRobustLocation
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    uint8_t enable,
    uint8_t enabled911
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.ConfigureRobustLocation(cmdRef, enable,enabled911);
}

/**
* FUNCTION     : RobustLocationInformation
* DESCRIPTION  : Get the Robust Location information for Enable/Disable, 911 enable or disable, major & minor version numbers
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
void taf_locGnss_RobustLocationInformation
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint8_t enable = 0;
    uint8_t enabled911 = 0;
    uint8_t majorVersion = 0;
    uint8_t minorVersion = 0;
    le_result_t res = gnss.RobustLocationInformation(&enable, &enabled911, &majorVersion, &minorVersion);
    taf_locGnss_RobustLocationInformationRespond(cmdRef, res, enable, enabled911, majorVersion, minorVersion);
}

/**
* FUNCTION     : DefaultSecondaryBandConstellations
* DESCRIPTION  : Set the Secondary Band Empty Constellations
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed with reason
*/
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
void taf_locGnss_DefaultSecondaryBandConstellations
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.DefaultSecondaryBandConstellations(cmdRef);
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
void taf_locGnss_RequestSecondaryBandConstellations
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.RequestSecondaryBandConstellations(cmdRef);
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
void taf_locGnss_ConfigureSecondaryBandConstellations
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    uint32_t constellationSb
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.ConfigureSecondaryBandConstellations(cmdRef, constellationSb);
}
#endif
/**
* FUNCTION     : GetMagneticDeviation
* DESCRIPTION  : Get the position sample's magnetic deviation
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
void taf_locGnss_GetMagneticDeviation
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    int32_t magneticDeviation = 0;
    le_result_t res = gnss.GetMagneticDeviation(positionSampleRef,&magneticDeviation);
    taf_locGnss_GetMagneticDeviationRespond(cmdRef, res, magneticDeviation);
}

/**
* FUNCTION     : GetEllipticalUncertainty
* DESCRIPTION  : Get the semi-major and semi-minor horizontal elliptical uncertainty.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_UNSUPPORTED LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
void taf_locGnss_GetEllipticalUncertainty
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t horUncEllipseSemiMajor = 0;
    uint32_t horUncEllipseSemiMinor = 0;
    uint8_t  horConfidence = 0;
    le_result_t res = gnss.GetEllipticalUncertainty(positionSampleRef,&horUncEllipseSemiMajor,
            &horUncEllipseSemiMinor,&horConfidence);
    taf_locGnss_GetEllipticalUncertaintyRespond(cmdRef, res, horUncEllipseSemiMajor,
            horUncEllipseSemiMinor,horConfidence);
}

/**
* FUNCTION     : SetLeverArmConfig
* DESCRIPTION  : Set the Lever Arm configuration Parameters
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_BAD_PARAMETER LE_NOT_PERMITTED on failed with reason
*/
void taf_locGnss_SetLeverArmConfig
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    const taf_locGnss_LeverArmParams_t* LeverArmParamsPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.SetLeverArmConfig(cmdRef, LeverArmParamsPtr);
}

/**
* FUNCTION     : SetEngineType
* DESCRIPTION  : This function sets the GNSS device with engine type
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_BAD_PARAMETER LE_NOT_PERMITTED on failed with reason
*/
void taf_locGnss_SetEngineType
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_EngineReportsType_t EngineType
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.SetEngineType(cmdRef, EngineType);
}

/**
* FUNCTION     : GetConformityIndex
* DESCRIPTION  : This function gets the conformity index
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetConformityIndex
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    double index = 0.0;
    le_result_t res = gnss.GetConformityIndex(positionSampleRef, &index);
    taf_locGnss_GetConformityIndexRespond(cmdRef, res, index);
}

/**
* FUNCTION     : GetCalibrationData
* DESCRIPTION  : This function gets the sensor calibration status and confidence percent.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetCalibrationData
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t calib = 0;
    uint8_t percent = 0;
    le_result_t res = gnss.GetCalibrationData(positionSampleRef, &calib, &percent);
    taf_locGnss_GetCalibrationDataRespond(cmdRef, res, calib, percent);
}

/**
* FUNCTION     : GetBodyFrameData
* DESCRIPTION  : This function gets the body frame data
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetBodyFrameData
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_KinematicsData_t bodyData = {};
    le_result_t res = gnss.GetBodyFrameData(positionSampleRef, &bodyData);
    taf_locGnss_GetBodyFrameDataRespond(cmdRef, res, &bodyData);
}

/**
* FUNCTION     : GetVRPBasedLLA
* DESCRIPTION  : This function gets Vechile reference point based latitude,longitude & altitude information
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetVRPBasedLLA
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    double vrpLatitude = 0.0;
    double vrpLongitude = 0.0;
    double vrpAltitude = 0.0;
    le_result_t res = gnss.GetVRPBasedLLA(positionSampleRef, &vrpLatitude, &vrpLongitude, &vrpAltitude);
    taf_locGnss_GetVRPBasedLLARespond(cmdRef, res, vrpLatitude, vrpLongitude, vrpAltitude);
}

/**
* FUNCTION     : GetVRPBasedVelocity
* DESCRIPTION  : This function gets Vehicle reference point based east,north & up velocity information
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetVRPBasedVelocity
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    double eastVel = 0.0;
    double northVel = 0.0;
    double upVel = 0.0;
    le_result_t res = gnss.GetVRPBasedVelocity(positionSampleRef, &eastVel, &northVel, &upVel);
    taf_locGnss_GetVRPBasedVelocityRespond(cmdRef, res, eastVel, northVel, upVel);
}

/**
* FUNCTION     : GetSvUsedInPosition
* DESCRIPTION  : This function gets the set of satellite vehicles that are used to calculate position.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetSvUsedInPosition
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_SvUsedInPosition_t svData = {};
    le_result_t res = gnss.GetSvUsedInPosition(positionSampleRef, &svData);
    taf_locGnss_GetSvUsedInPositionRespond(cmdRef, res, &svData);
}

/**
* FUNCTION     : GetSbasCorrection
* DESCRIPTION  : This function gets navigation solution mask used to indicate SBAS corrections.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetSbasCorrection
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t sbasMask = 0;
    le_result_t res = gnss.GetSbasCorrection(positionSampleRef, &sbasMask);
    taf_locGnss_GetSbasCorrectionRespond(cmdRef, res, sbasMask);
}

/**
* FUNCTION     : GetPositionTechnology
* DESCRIPTION  : This function gets technology mask to indicate which technology is used.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetPositionTechnology
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t techMask = 0;
    le_result_t res = gnss.GetPositionTechnology(positionSampleRef, &techMask);
    taf_locGnss_GetPositionTechnologyRespond(cmdRef, res, techMask);
}

/**
* FUNCTION     : GetLocationInfoValidity
* DESCRIPTION  : This function gets the validity of the Location basic Info.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetLocationInfoValidity
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t validityMask = 0;
    uint64_t validityExMask = 0;
    le_result_t res = gnss.GetLocationInfoValidity(positionSampleRef, &validityMask, &validityExMask);
    taf_locGnss_GetLocationInfoValidityRespond(cmdRef, res, validityMask, validityExMask);
}

/**
* FUNCTION     : GetLocationOutputEngParams
* DESCRIPTION  : This function gets the combination of position engines and type of
*                location engine used in calculating the position report.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetLocationOutputEngParams
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint16_t engMask = 0;
    uint16_t locationEngType = 0;
    le_result_t res = gnss.GetLocationOutputEngParams(positionSampleRef, &engMask, &locationEngType);
    taf_locGnss_GetLocationOutputEngParamsRespond(cmdRef, res, engMask, locationEngType);
}

/**
* FUNCTION     : GetReliabilityInformation
* DESCRIPTION  : This function gets the reliability of the horizontal & vertical positions.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetReliabilityInformation
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint16_t horiReliability = 0;
    uint16_t vertReliability = 0;
    le_result_t res = gnss.GetReliabilityInformation(positionSampleRef, &horiReliability, &vertReliability);
    taf_locGnss_GetReliabilityInformationRespond(cmdRef, res, horiReliability, vertReliability);
}

/**
* FUNCTION     : GetStdDeviationAzimuthInfo
* DESCRIPTION  : This function gets the elliptical horizontal uncertainty azimuth of orientation,
*                east and north standard deviations.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetStdDeviationAzimuthInfo
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    double azimuth = 0.0;
    double eastDev = 0.0;
    double northDev = 0.0;
    le_result_t res = gnss.GetStdDeviationAzimuthInfo(positionSampleRef, &azimuth, &eastDev, &northDev);
    taf_locGnss_GetStdDeviationAzimuthInfoRespond(cmdRef, res, azimuth, eastDev, northDev);
}

/**
* FUNCTION     : GetRealTimeInformation
* DESCRIPTION  : This function gets elapsed real time and its uncertainity values in nano-second.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetRealTimeInformation
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint64_t realTime = 0;
    uint64_t realTimeUnc = 0;
    le_result_t res = gnss.GetRealTimeInformation(positionSampleRef, &realTime, &realTimeUnc);
    taf_locGnss_GetRealTimeInformationRespond(cmdRef, res, realTime, realTimeUnc);
}

/**
* FUNCTION     : GetMeasurementUsageInfo
* DESCRIPTION  : This function retrieves gnss measurement usage info.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_OVERFLOW, LE_BAD_PARAMETER, LE_NO_MEMORY
*                LE_OUT_OF_RANGE on failed with reason.
*/
void taf_locGnss_GetMeasurementUsageInfo
(
    taf_locGnss_ServerCmdRef_t  cmdRef,
    taf_locGnss_SampleRef_t     positionSampleRef,  ///< [IN]    Position sample reference.
    size_t                      measInfoPtrSize     ///< [INOUT] Max entries / actual count.
)
{
    auto &gnss = taf_locGnss::GetInstance();

    size_t maxLen = measInfoPtrSize;
    std::vector<taf_locGnss_GnssMeasurementInfo_t> measInfo(maxLen);

    le_result_t res = gnss.GetMeasurementUsageInfo(positionSampleRef, measInfo.data(), &maxLen);
    if (res != LE_OK)
    {
        maxLen = 0;
    }

    taf_locGnss_GetMeasurementUsageInfoRespond(cmdRef, res, measInfo.data(), maxLen);
}

/**
* FUNCTION     : GetReportStatus
* DESCRIPTION  : This function retrieves the status of report in terms of how optimally
               : the report was calculated by the engine.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_BAD_PARAMETER, LE_NO_MEMORY on failed with reason
*/
void taf_locGnss_GetReportStatus
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    // TAF_ERROR_IF(positionSampleRef == NULL, "Invalid gnss sample reference");

    auto &gnss = taf_locGnss::GetInstance();
    int32_t reportStatus = 0;
    le_result_t res = gnss.GetReportStatus(positionSampleRef, &reportStatus);
    taf_locGnss_GetReportStatusRespond(cmdRef, res, reportStatus);
}

/**
* FUNCTION     : GetAltitudeMeanSeaLevel
* DESCRIPTION  : This function retrieves the altitude with respect to mean sea level in Meters.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_BAD_PARAMETER, LE_NO_MEMORY on failed with reason
*/
void taf_locGnss_GetAltitudeMeanSeaLevel
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{

    auto &gnss = taf_locGnss::GetInstance();
    double altMeanSeaLevel = 0.0;
    le_result_t res = gnss.GetAltitudeMeanSeaLevel(positionSampleRef, &altMeanSeaLevel);
    taf_locGnss_GetAltitudeMeanSeaLevelRespond(cmdRef, res, altMeanSeaLevel);
}

/**
* FUNCTION     : GetSVIds
* DESCRIPTION  : This function retrieves GNSS Satellite Vehicles used in position data.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_OVERFLOW, LE_BAD_PARAMETER, LE_NO_MEMORY
*                LE_OUT_OF_RANGE on failed with reason.
*/
void taf_locGnss_GetSVIds
(
    taf_locGnss_ServerCmdRef_t  _cmdRef,
    taf_locGnss_SampleRef_t     positionSampleRef,  ///< [IN]    Position sample reference.
    size_t                      sVIdsLen            ///< [INOUT] Max entries / actual count.
)
{
    auto &gnss = taf_locGnss::GetInstance();

    size_t maxLen = sVIdsLen;
    std::vector<uint16_t> svIds(maxLen);

    le_result_t res = gnss.GetSVIds(positionSampleRef, svIds.data(), &maxLen);
    if (res != LE_OK){
        maxLen = 0;
    }

    taf_locGnss_GetSVIdsRespond(_cmdRef, res, svIds.data(), maxLen);
}

/**
* FUNCTION     : GetSatellitesInfoEx
* DESCRIPTION  : This function retrieves satellites vehicle information of a given constellation.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_OVERFLOW, LE_BAD_PARAMETER, LE_NO_MEMORY
*                LE_OUT_OF_RANGE on failed with reason.
*/
void taf_locGnss_GetSatellitesInfoEx
(
    taf_locGnss_ServerCmdRef_t    _cmdRef,
    taf_locGnss_SampleRef_t       positionSampleRef,  ///< [IN]    Position sample reference.
    taf_locGnss_Constellation_t   constellation,      ///< [IN]    Constellation filter.
    size_t                        svInfoSize          ///< [INOUT] Max entries / actual count.
)
{
    auto &gnss = taf_locGnss::GetInstance();

    size_t maxLen = svInfoSize;
    std::vector<taf_locGnss_SvInfo_t> svInfo(maxLen);

    le_result_t res = gnss.GetSatellitesInfoEx(positionSampleRef, constellation, svInfo.data(), &maxLen);
    if (res != LE_OK)
    {
        maxLen = 0;
    }

    taf_locGnss_GetSatellitesInfoExRespond(_cmdRef, res, svInfo.data(), maxLen);
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
void taf_locGnss_SetMinGpsWeek
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    uint16_t minGpsWeek
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.SetMinGpsWeek(cmdRef, minGpsWeek);
}

/**
* FUNCTION     : GetMinGpsWeek
* DESCRIPTION  : This function gets the minimum GPS week.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT, LE_NO_MEMORY, LE_NOT_PERMITTED on failed.
*/
void taf_locGnss_GetMinGpsWeek
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint16_t minGpsWeek = 0;
    le_result_t res = gnss.GetMinGpsWeek(&minGpsWeek);
    taf_locGnss_GetMinGpsWeekRespond(cmdRef, res, minGpsWeek);
}

/**
* FUNCTION     : GetCapabilities
* DESCRIPTION  : This function gets the GNSS capability information.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed.
*/
void taf_locGnss_GetCapabilities
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint64_t locCapability = 0;
    le_result_t res = gnss.GetCapabilities(&locCapability);
    taf_locGnss_GetCapabilitiesRespond(cmdRef, res, locCapability);
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

void taf_locGnss_SetNmeaConfiguration
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_NmeaBitMask_t nmeaMask,         ///< [IN] Bit mask for enabled NMEA sentences.
    taf_locGnss_GeodeticDatumType_t datumType,  ///< [IN] Specify the datum type to be configured.
    taf_locGnss_LocEngineType_t engineType      ///< [IN] Specify the Engine type.
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.SetNmeaConfiguration(cmdRef, nmeaMask, datumType, engineType);
}

/**
* FUNCTION     : GetXtraStatus
* DESCRIPTION  : Gets the Xtra status.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_NOT_PERMITTED on failed
*/
void taf_locGnss_GetXtraStatus
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_XtraStatusParams_t xtraParams = {};
    le_result_t res = gnss.GetXtraStatus(&xtraParams);
    taf_locGnss_GetXtraStatusRespond(cmdRef, res, &xtraParams);
}

/**
* FUNCTION     : GetGnssData
* DESCRIPTION  : Get GNSS data for data mask, jammer indication and agc.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_NO_MEMORY on failed
*/
void taf_locGnss_GetGnssData
(
    taf_locGnss_ServerCmdRef_t  _cmdRef,
    taf_locGnss_SampleRef_t     positionSampleRef,  ///< [IN]    Position sample reference.
    size_t                      maxSignalTypes      ///< [INOUT] Max entries / actual count.
)
{
    auto &gnss = taf_locGnss::GetInstance();

    size_t maxTypes = maxSignalTypes;

    std::vector<taf_locGnss_GnssData_t> gnssData(maxTypes);

    le_result_t res = gnss.GetGnssData(
        positionSampleRef,
        gnssData.data(),
        &maxTypes);

    if (res != LE_OK)
    {
        maxTypes = 0;
    }

    taf_locGnss_GetGnssDataRespond(
        _cmdRef,
        res,
        gnssData.data(),
        maxTypes);
}

/**
* FUNCTION     : SetDRConfigValidity
* DESCRIPTION  : Sets the dead reckoning parameters validity mask
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed with reason
*/
void taf_locGnss_SetDRConfigValidity
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_DRConfigValidityType_t validMask
)
{
    auto &gnss = taf_locGnss::GetInstance();
    le_result_t res = gnss.SetDRConfigValidity(validMask);
    taf_locGnss_SetDRConfigValidityRespond(cmdRef, res);
}

/**
* FUNCTION     : GetGptpTime
* DESCRIPTION  : Gets Gptp time and its uncertainity.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT on failed
*/
void taf_locGnss_GetGptpTime
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint64_t gPtpTime = 0;
    uint64_t gPtpTimeUnc = 0;
    le_result_t res = gnss.GetGptpTime(positionSampleRef, &gPtpTime, &gPtpTimeUnc);
    taf_locGnss_GetGptpTimeRespond(cmdRef, res, gPtpTime, gPtpTimeUnc);
}

/**
* FUNCTION     : DeleteDRSensorCalData
* DESCRIPTION  : This function deletes dead reckoning sensor calibration data.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_NOT_PERMITTED on failed
*/
void taf_locGnss_DeleteDRSensorCalData
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.DeleteDRSensorCalData(cmdRef);
}

/**
* FUNCTION     : GetDRSolutionStatus
* DESCRIPTION  : This function gets the dead reckoning sensor solution status.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetDRSolutionStatus
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t solutionStatus = 0;
    le_result_t res = gnss.GetDRSolutionStatus(positionSampleRef, &solutionStatus);
    taf_locGnss_GetDRSolutionStatusRespond(cmdRef, res, solutionStatus);
}

/**
* FUNCTION     : GetLeapSecondsUncertainty
* DESCRIPTION  : Gets leap seconds uncertainty.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_OUT_OF_RANGE and LE_FAULT on failed
*/
void taf_locGnss_GetLeapSecondsUncertainty
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint8_t leapSecondsUnc = 0;
    le_result_t res = gnss.GetLeapSecondsUncertainty(positionSampleRef, &leapSecondsUnc);
    taf_locGnss_GetLeapSecondsUncertaintyRespond(cmdRef, res, leapSecondsUnc);
}

/**
* FUNCTION     : GetIsNHz
* DESCRIPTION  : Gets the frequency for GNSS measurements generated at NHz or not.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success and LE_FAULT on failed
*/
void taf_locGnss_GetIsNHz
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_MeasSampleRef_t measSampleRef
        ///< [IN] Measurement sample reference.
)
{
    auto &gnss = taf_locGnss::GetInstance();
    bool isNHz = false;
    le_result_t res = gnss.GetIsNHz(measSampleRef, &isNHz);
    taf_locGnss_GetIsNHzRespond(cmdRef, res, isNHz);
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
void taf_locGnss_GetClockValidityMask
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_MeasSampleRef_t measSampleRef
        ///< [IN] Measurement sample reference.
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t clockValidityMask = 0;
    le_result_t res = gnss.GetClockValidityMask(measSampleRef, &clockValidityMask);
    taf_locGnss_GetClockValidityMaskRespond(cmdRef, res, clockValidityMask);
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
void taf_locGnss_GetClockData
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_MeasSampleRef_t measSampleRef
        ///< [IN] Measurement sample reference.
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_ClockData_t clockData = {};
    le_result_t res = gnss.GetClockData(measSampleRef, &clockData);
    taf_locGnss_GetClockDataRespond(cmdRef, res, &clockData);
}

void taf_locGnss_GetMeasurementsData
(
    taf_locGnss_ServerCmdRef_t      _cmdRef,
    taf_locGnss_MeasSampleRef_t     measSampleRef,  ///< [IN]    Measurement sample reference.
    size_t                          measDataSize    ///< [INOUT] Max entries / actual count.
)
{
    auto &gnss = taf_locGnss::GetInstance();

    std::vector<taf_locGnss_MeasurementsData_t> measData(measDataSize);

    le_result_t res = gnss.GetMeasurementsData(
        measSampleRef,
        measData.data(),
        &measDataSize);

    taf_locGnss_GetMeasurementsDataRespond(
        _cmdRef,
        res,
        measData.data(),
        measDataSize);
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
    taf_locGnss_ServerCmdRef_t _cmdRef,
    taf_locGnss_MeasSampleRef_t    measSampleRef
    ///< [IN] Measurement sample reference.
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.ReleaseMeasSampleRef(measSampleRef);
    taf_locGnss_ReleaseMeasSampleRefRespond(_cmdRef);
}

void taf_locGnss_GetMeasDataValidityMask
(
    taf_locGnss_ServerCmdRef_t   _cmdRef,
    taf_locGnss_MeasSampleRef_t  measSampleRef,            ///< [IN]    Measurement sample reference.
    size_t                       measDataValidityMaskSize  ///< [INOUT] Max entries / actual count.
)
{
    auto &gnss = taf_locGnss::GetInstance();

    uint32_t measDataValidityMask[measDataValidityMaskSize];
    memset(measDataValidityMask, 0, sizeof(measDataValidityMask));

    le_result_t res = gnss.GetMeasDataValidityMask(
        measSampleRef,
        measDataValidityMask,
        &measDataValidityMaskSize);

    taf_locGnss_GetMeasDataValidityMaskRespond(
        _cmdRef,
        res,
        measDataValidityMask,
        measDataValidityMaskSize);
}


/**
* FUNCTION     : GetNavigationSolution
* DESCRIPTION  : This function gets navigation solution mask used to indicate solutions used in the fix.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetNavigationSolution
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
        ///< [IN] Position sample reference.
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t navSolution = 0;
    le_result_t res = gnss.GetNavigationSolution(positionSampleRef, &navSolution);
    taf_locGnss_GetNavigationSolutionRespond(cmdRef, res, navSolution);
}

/**
* FUNCTION     : GetDgnssStationIds
* DESCRIPTION  : This function gets list of DGNSS station IDs providing corrections.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_OUT_OF_RANGE on failed with reason
*/
void taf_locGnss_GetDgnssStationIds
(
    taf_locGnss_ServerCmdRef_t  cmdRef,
    taf_locGnss_SampleRef_t     positionSampleRef,  ///< [IN]    Position sample reference.
    size_t                      stationIdsSize      ///< [INOUT] Max entries / actual count.
)
{
    auto &gnss = taf_locGnss::GetInstance();

    std::vector<uint16_t> stationIds(stationIdsSize, 0);

    le_result_t res = gnss.GetDgnssStationIds(
        positionSampleRef,
        stationIds.data(),
        &stationIdsSize);

    taf_locGnss_GetDgnssStationIdsRespond(
        cmdRef, res,
        stationIds.data(),
        stationIdsSize);
}

/**
* FUNCTION     : CreateDgnssInjectionSource
* DESCRIPTION  : This function create a Dgnss injection source.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_NOT_PERMITTED on failed with reason
*/
void taf_locGnss_CreateDgnssSource
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_DgnssFormat_t dgnssDataFormat
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.CreateDgnssSource(cmdRef, dgnssDataFormat);
}

/**
* FUNCTION     : ReleaseDgnssInjectionSource
* DESCRIPTION  : This function release current Dgnss injection source.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_NOT_PERMITTED on failed with reason
*/
void taf_locGnss_ReleaseDgnssSource
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_DgnssSourceRef_t sourceRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.ReleaseDgnssSource(cmdRef, sourceRef);
}

/**
* FUNCTION     : InjectDgnssCorrection
* DESCRIPTION  : This function used to inject correction data.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT,LE_NOT_PERMITTED,LE_BAD_PARAMETER on failed with reason
*/
void taf_locGnss_InjectDgnssCorrection
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_DgnssSourceRef_t sourceRef,
    const uint8_t* correctionDataPtr,
    size_t correctionDataSize

)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.InjectDgnssCorrection(cmdRef, sourceRef, correctionDataPtr, correctionDataSize);
}

taf_locGnss_DgnssStatusChangeHandlerRef_t taf_locGnss_AddDgnssStatusChangeHandler
(
    taf_locGnss_DgnssStatusChangeHandlerFunc_t handlerPtr,
        ///< [IN] Injection status change handler
    void* contextPtr
        ///< [IN]
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return (taf_locGnss_DgnssStatusChangeHandlerRef_t)gnss.AddDgnssStatusChangeHandler(handlerPtr, contextPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_locGnss_DgnssInjectionStatusChange'
 */
//--------------------------------------------------------------------------------------------------
void taf_locGnss_RemoveDgnssStatusChangeHandler
(
    taf_locGnss_DgnssStatusChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
    auto &gnss = taf_locGnss::GetInstance();
    return gnss.RemoveDgnssStatusChangeHandler(handlerRef);
}

/**
* FUNCTION     : InjectMerkleTreeInformationByPath
* DESCRIPTION  : This function Injects the Merkle Tree information via an XML configuration file
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED LE_BAD_PARAMETER on failed with reason
*/
//--------------------------------------------------------------------------------------------------
/**
 */
//--------------------------------------------------------------------------------------------------
void taf_locGnss_InjectMerkleTreeInformationByPath
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    const char* LE_NONNULL merkleTreeFilePath
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.InjectMerkleData(cmdRef, merkleTreeFilePath);
}

/**
* FUNCTION     : ConfigureOsnma
* DESCRIPTION  : This function Enables or disables the OSNMA feature in the modem.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_NOT_PERMITTED on failed with reason
*/
//--------------------------------------------------------------------------------------------------
/**
 */
//--------------------------------------------------------------------------------------------------
void taf_locGnss_ConfigureOsnma
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    bool galOsnma
        ///< [IN]
)
{
    auto &gnss = taf_locGnss::GetInstance();
    gnss.ConfigureOsnma(cmdRef, galOsnma);
}

/**
* FUNCTION     : SetEngineIntegrityRisk
* DESCRIPTION  : Set the Integrity risk of desired engine
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_BAD_PARAMETER LE_NOT_PERMITTED on failed with reason
*/
void taf_locGnss_SetEngineIntegrityRisk
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_EngineType_t engtype,
        ///< [IN]
    uint32_t integrityRisk
        ///< [IN]
)
{
    auto &gnss = taf_locGnss::GetInstance();
    auto res = gnss.SetEngineIntegrityRisk(engtype, integrityRisk);
    taf_locGnss_SetEngineIntegrityRiskRespond(cmdRef, res);
}

/**
* FUNCTION     : GetProtectionLevels
* DESCRIPTION  : Get the protection level values at the specified integrity risk
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
void taf_locGnss_GetProtectionLevels
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
        ///< [IN]
)
{
    auto &gnss = taf_locGnss::GetInstance();
    double protectionLevelAlongTrackPtr = 0.0;
    double protectionLevelCrossTrackPtr = 0.0;
    double protectionLevelVerticalPtr = 0.0;
    auto res = gnss.GetProtectionLevels(positionSampleRef, &protectionLevelAlongTrackPtr,
            &protectionLevelCrossTrackPtr, &protectionLevelVerticalPtr);
    taf_locGnss_GetProtectionLevelsRespond(cmdRef, res, protectionLevelAlongTrackPtr, protectionLevelCrossTrackPtr, protectionLevelVerticalPtr);
}

/**
* FUNCTION     : GetBaselineLength
* DESCRIPTION  : Get the distance between the basestation and the receiver.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
void taf_locGnss_GetBaselineLength
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
        ///< [IN]
)
{
    auto &gnss = taf_locGnss::GetInstance();
    double baselineLengthPtr = 0.0;
    auto res = gnss.GetBaselineLength(positionSampleRef, &baselineLengthPtr);
    taf_locGnss_GetBaselineLengthRespond(cmdRef, res, baselineLengthPtr);
}

/**
* FUNCTION     : GetAgeOfCorrections
* DESCRIPTION  : Get the difference in time between the fix timestamp using the correction
*                and the time of the correction data.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
void taf_locGnss_GetAgeOfCorrections
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
        ///< [IN]
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint64_t ageCorrectionsPtr = 0;
    auto result = gnss.GetAgeOfCorrections(positionSampleRef, &ageCorrectionsPtr);
    taf_locGnss_GetAgeOfCorrectionsRespond(cmdRef, result, ageCorrectionsPtr);
}

/**
* FUNCTION     : GetIntegrityRiskUsed
* DESCRIPTION  : Gets the integrity risk used for protection level parameters
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_OUT_OF_RANGE on failed
*/
void taf_locGnss_GetIntegrityRiskUsed
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_SampleRef_t positionSampleRef
        ///< [IN]
)
{
    auto &gnss = taf_locGnss::GetInstance();
    uint32_t integrityRiskUsedPtr = 0;
    auto res = gnss.GetIntegrityRiskUsed(positionSampleRef, &integrityRiskUsedPtr);
    taf_locGnss_GetIntegrityRiskUsedRespond(cmdRef, res, integrityRiskUsedPtr);
}

/**
* FUNCTION     : SetDataResolution
* DESCRIPTION  : Sets the output resolution per field group per client session.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_BAD_PARAMETER  LE_NOT_PERMITTED on failed.
*/
void taf_locGnss_SetDataResolution
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    taf_locGnss_DataType_t     dataType,
    taf_locGnss_Resolution_t   resolution
)
{
    auto &gnss = taf_locGnss::GetInstance();
    le_result_t result = gnss.SetDataResolution(dataType, resolution);
    taf_locGnss_SetDataResolutionRespond(cmdRef, result);
}

/**
* FUNCTION     : GetDataResolution
* DESCRIPTION  : Gets the output resolution field set group per client session.
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT LE_BAD_PARAMETER LE_NOT_PERMITTED on failed.
*/
void taf_locGnss_GetDataResolution
(
    taf_locGnss_ServerCmdRef_t  cmdRef,
    taf_locGnss_DataType_t      dataType
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_Resolution_t resolution = TAF_LOCGNSS_RES_UNKNOWN;
    le_result_t res = gnss.GetDataResolution(dataType, &resolution);
    taf_locGnss_GetDataResolutionRespond(cmdRef, res, resolution);
}