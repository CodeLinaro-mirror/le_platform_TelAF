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
#ifdef LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING
#include "posCfgEntries.h"
#endif
#include <iostream>
#include <string>
#include <memory>
#include <unistd.h>
#include "tafPos.hpp"
#include "tafGnss.hpp"

using namespace tafsvc;
using namespace std;

LE_MEM_DEFINE_STATIC_POOL(PosSample, TAF_LOCPOS_MAX_OBJ, sizeof(taf_locPos_Sample_t));
LE_MEM_DEFINE_STATIC_POOL(PosSampleRequest, TAF_LOCPOS_MAX_OBJ, sizeof(PosSampleRequest_t));
LE_MEM_DEFINE_STATIC_POOL(PosHandler, HIGH_POS_HANDLER_COUNT, sizeof(taf_locPos_SampleHandler_t));
LE_REF_DEFINE_STATIC_MAP(PosSampleMap, POSITIONING_SAMPLE_MAX);
LE_REF_DEFINE_STATIC_MAP(PositioningClient, TAF_CONFIG_POSITIONING_ACTIVATION_MAX);
LE_MEM_DEFINE_STATIC_POOL(PosCtrlHandler, TAF_CONFIG_POSITIONING_ACTIVATION_MAX, sizeof(ClientRequest_t));
LE_MEM_DEFINE_STATIC_POOL(PosCtrlCmdPool, MAX_CMD_LOCATION_POOL_SIZE, sizeof(PosCtrlCmdInfo_t));

taf_locPos &taf_locPos::GetInstance()
{
    static taf_locPos instance;
    return instance;
}

uint32_t taf_locPos::ComputeCommonSmallestRate
(
 uint32_t rate
)
{
    taf_locPos_SampleHandler_t *posHandlerNodePtr;

    le_ref_IterRef_t iterRef = le_ref_GetIterator(MovementHandlerRefMap);

    while(le_ref_NextNode(iterRef) == LE_OK)
    {
        posHandlerNodePtr = (taf_locPos_SampleHandler_t*)le_ref_GetValue(iterRef);
        LE_ASSERT(posHandlerNodePtr != NULL);
        if (posHandlerNodePtr->acquisitionRate < rate)
        {
            rate = posHandlerNodePtr->acquisitionRate;
            return rate;
        }
    }

    return rate;
}

uint32_t taf_locPos::CalculateAcquisitionRate
(
 uint32_t avgSpeed,
 uint32_t hMagnitude,
 uint32_t vMagnitude
 )
{
    uint32_t  rate;
    uint32_t  metersec = avgSpeed * SEC_TO_MSEC/HOURS_TO_SEC;

    LE_DEBUG("metersec %" PRIu32 " (m/sec), h_Magnitude %" PRIu32 ", v_Magnitude %" PRIu32 "",
            metersec, hMagnitude, vMagnitude);

    if ((hMagnitude < metersec) || (vMagnitude < metersec))
    {
        return 1;
    }

    rate = 0;
    while (! (((hMagnitude >= (metersec + metersec*rate)) &&
                    (vMagnitude >= (metersec + metersec*rate)))&&
                ((hMagnitude < (metersec + metersec*(rate+1))) ||
                 (vMagnitude < (metersec + metersec*(rate+1)))))) {
        rate++;
    }
    // To get smallest AcquisitionRate mininum two is required
    // Default AcquisitionRate 1000msec
    return rate + 2;
}

int32_t taf_locPos::TransformDistance
(
    int32_t val,
    taf_locPos_DistanceValueType_t type
)
{
    auto &pos = taf_locPos::GetInstance();
    int32_t resVal = 0;

    if ( ALTITUDE == type) {
        if ( TAF_LOCPOS_RES_DECIMETER == pos.DistanceResolution ) {
            resVal = val / 1e+2;
        } else if ( TAF_LOCPOS_RES_CENTIMETER == pos.DistanceResolution ) {
            resVal = val / 10;
        } else if ( TAF_LOCPOS_RES_MILLIMETER == pos.DistanceResolution ) {
            resVal = val;
        } else {
            resVal = val / 1e+3;
        }
    } else if ( H_ACCURACY == type ) {
        if ( TAF_LOCPOS_RES_DECIMETER == pos.DistanceResolution ) {
            resVal = val / 10;
        } else if ( TAF_LOCPOS_RES_CENTIMETER == pos.DistanceResolution ) {
            resVal = val;
        } else if ( TAF_LOCPOS_RES_MILLIMETER == pos.DistanceResolution ) {
            resVal = val * 10;
        } else {
            resVal = val / 1e+2;
        }
    } else if ( V_ACCURACY == type) {
        if ( TAF_LOCPOS_RES_DECIMETER == pos.DistanceResolution ) {
            resVal = val;
        } else if ( TAF_LOCPOS_RES_CENTIMETER == pos.DistanceResolution ) {
            resVal = val * 10;
        } else if ( TAF_LOCPOS_RES_MILLIMETER == pos.DistanceResolution ) {
            resVal = val * 1e+2;
        } else {
            resVal = val / 10;
        }
    } else {
        LE_ERROR("Wrong type");
    }
    return resVal;
}

uint32_t taf_locPos::CalculateDistance
(
    uint32_t lat1,
    uint32_t long1,
    uint32_t lat2,
    uint32_t long2
)
{
    #define PI 3.14159265
    // Uses the haversine formula to calculate the great-circle distance betwen two points
    double radius = 6371; // Radius of the earth in km
    double latitude1 = ((double)lat1) / 1e+6*PI / 180;
    double latitude2 = ((double)lat2) / 1e+6*PI / 180;
    double dLat = ((double)lat2-(double)lat1)/ 1e+6*PI/180;
    double dLon = ((double)long2-(double)long1)/ 1e+6*PI/180;
    double a, c; // c is Angular distance in radians and a is the square of half the chord length

    // note that 'a' must be between 0 and 1.
    // also note that 'a' approaches 1 when the points are at opposite ends of earth, and bigger error could be introduced
    a = sin(dLat/2) * sin(dLat/2) + sin(dLon/2) * sin(dLon/2) * cos(latitude1) * cos(latitude2);
    c = 2 * atan2(sqrt(a), sqrt(1-a));

    LE_DEBUG("Calculated distance is %e meters (double)", (double)(radius * c * 1000));
    return (uint32_t)(radius * c * 1000);
}

bool taf_locPos::IsBeyondMagnitude
(
 uint32_t posMagnitude,
 uint32_t posMove,
 uint32_t posAccuracy
)
{
    if (posMove > posMagnitude)
    {
        if (((posMove - posAccuracy) < posMagnitude) || (posAccuracy > posMove))
        {
            return false;
        } else {
            return true;
        }
    } else {
        return false;
    }
}

le_result_t taf_locPos::GetDirection
(
 uint32_t* dirPtr,
 uint32_t* dirAccuracyPtr
)
{
    TAF_KILL_CLIENT_IF_RET_VAL((dirPtr == NULL) && (dirAccuracyPtr == NULL), LE_FAULT, "Invalid input parameters");

    le_result_t resGnss = LE_OK;
    le_result_t resPos = LE_OK;
    uint32_t dir;
    uint32_t dirAccuracy;

    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_SampleRef_t positionSampleRef = gnss.GetLastSampleRef();

    resGnss =gnss.GetDirection(positionSampleRef, &dir, &dirAccuracy);

    if ((resGnss == LE_OK)||(resGnss == LE_OUT_OF_RANGE))
    {
        if (dirAccuracyPtr)
        {
            if (dirAccuracy != UINT32_MAX)
            {
                *dirAccuracyPtr = dirAccuracy / 10;
            }
            else
            {
                *dirAccuracyPtr = dirAccuracy;
                resPos = LE_OUT_OF_RANGE;
            }
        }
        if (dirPtr)
        {
            if (dir != UINT32_MAX)
            {
                *dirPtr = dir/10;
            }
            else
            {
                *dirPtr = dir;
                resPos = LE_OUT_OF_RANGE;
            }
        }
    }
    else
    {
        resPos = LE_FAULT;
    }

    gnss.ReleaseSampleRef(positionSampleRef);

    return resPos;
}
le_result_t taf_locPos::GetMotion
(
    uint32_t*   horiSpeedPtr,
    uint32_t*   horiSpeedAccuracyPtr,
    int32_t*    vertSpeedPtr,
    int32_t*    vertSpeedAccuracyPtr
)
{
    if ((horiSpeedPtr == NULL)&&
        (horiSpeedAccuracyPtr == NULL)&&
        (vertSpeedPtr == NULL)&&
        (vertSpeedAccuracyPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid input parameters!");
        return LE_FAULT;
    }

    uint32_t horiSpeed;
    uint32_t horiSpeedAccuracy;
    int32_t vertSpeed;
    int32_t vertSpeedAccuracy;
    le_result_t Result = LE_OK;
    le_result_t posResult = LE_OK;

    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_SampleRef_t positionSampleRef = gnss.GetLastSampleRef();

    // Get horizontal speed
    Result = gnss.GetHorizontalSpeed(positionSampleRef, &horiSpeed, &horiSpeedAccuracy);
    if ((Result == LE_OK)||(Result == LE_OUT_OF_RANGE))
    {
        if (horiSpeedPtr)
        {
            if (horiSpeed != UINT32_MAX)
            {
                *horiSpeedPtr = horiSpeed/100;
            }
            else
            {
                *horiSpeedPtr = horiSpeed;
                posResult = LE_OUT_OF_RANGE;
            }
        }
        if (horiSpeedAccuracyPtr)
        {
            if (horiSpeedAccuracy != UINT32_MAX)
            {
               *horiSpeedAccuracyPtr = horiSpeedAccuracy/10;
            }
            else
            {
               *horiSpeedAccuracyPtr = horiSpeedAccuracy;
                posResult = LE_OUT_OF_RANGE;
            }
        }
    }
    else
    {
         posResult = LE_FAULT;
    }

    // Get vertical speed
    Result = gnss.GetVerticalSpeed(positionSampleRef, &vertSpeed, &vertSpeedAccuracy);

    if (((Result == LE_OK)||(Result == LE_OUT_OF_RANGE))
        &&(posResult != LE_FAULT))
    {
        if (vertSpeedPtr)
        {
            if (vertSpeed != INT32_MAX)
            {
                *vertSpeedPtr = vertSpeed/100;
            }
            else
            {
                *vertSpeedPtr = vertSpeed;
                 posResult = LE_OUT_OF_RANGE;
            }
        }
        if (vertSpeedAccuracyPtr)
        {
            if (vertSpeedAccuracy != INT32_MAX)
            {
                *vertSpeedAccuracyPtr = vertSpeedAccuracy/10;
            }
            else
            {
                *vertSpeedAccuracyPtr = vertSpeedAccuracy;
                posResult = LE_OUT_OF_RANGE;
            }
        }
    }
    else
    {
        posResult = LE_FAULT;
    }

    gnss.ReleaseSampleRef(positionSampleRef);

    return posResult;
}
le_result_t taf_locPos::CalculateMove
(
  taf_locPos_SampleHandler_t *posHandlerNodePtr,
  const PositionParam_t  *posParamPtr,
  bool  *hflagPtr,
  bool  *vflagPtr
)
{
    TAF_ERROR_IF_RET_VAL( posHandlerNodePtr == NULL, LE_FAULT, "posHandlerNodePtr is Null");

    TAF_ERROR_IF_RET_VAL( (posHandlerNodePtr->horizontalMagnitude != 0) && (!(posParamPtr->locationValid)), LE_FAULT, "Longitude or Latitude are not relevant");

    TAF_ERROR_IF_RET_VAL( (posHandlerNodePtr->verticalMagnitude != 0) && (!(posParamPtr->altitudeValid)), LE_FAULT, "Altitude is  not relevant");


    LE_DEBUG("Last Position lat.%" PRIi32 ", long.%" PRIi32, posHandlerNodePtr->lastLat, posHandlerNodePtr->lastLong);

    if (posHandlerNodePtr->lastLat == 0)
    {
        posHandlerNodePtr->lastLat = posParamPtr->latitude;
    }

    if (posHandlerNodePtr->lastLong == 0)
    {
        posHandlerNodePtr->lastLong = posParamPtr->longitude;
    }

    if (posHandlerNodePtr->lastAlt == 0)
    {
        posHandlerNodePtr->lastAlt = posParamPtr->altitude;
    }

    uint32_t horizontalMove = CalculateDistance(posHandlerNodePtr->lastLat, posHandlerNodePtr->lastLong,
                                              posParamPtr->latitude, posParamPtr->longitude);

    uint32_t verticalMove = abs(posParamPtr->altitude - posHandlerNodePtr->lastAlt);

    LE_DEBUG("horizontalMove.%" PRIu32 ", verticalMove.%" PRIu32, horizontalMove, verticalMove);

    if (INT32_MAX == posParamPtr->vAccuracy)
    {
        *vflagPtr = false;
    }
    else
    {
        *vflagPtr = IsBeyondMagnitude(posHandlerNodePtr->verticalMagnitude,
                                   verticalMove,
                                   posParamPtr->vAccuracy/10);
    }

    if (INT32_MAX == posParamPtr->hAccuracy)
    {
        *hflagPtr = false;
    }
    else
    {
        *hflagPtr = IsBeyondMagnitude(posHandlerNodePtr->horizontalMagnitude,
                                   horizontalMove, posParamPtr->hAccuracy/100);
    }
    LE_DEBUG("Vertical IsBeyondMagnitude.%d", *vflagPtr);
    LE_DEBUG("Horizontal IsBeyondMagnitude.%d", *hflagPtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * [3/3 RESPOND] - Runs on LocationSvc main thread.
 * Called back by taf_locGnss::StartInternal completion.
 */
//--------------------------------------------------------------------------------------------------
static void PosCtrlRequestRespond(le_result_t gnssResult, void* contextPtr)
{
    PosCtrlCmdInfo_t* cPtr = static_cast<PosCtrlCmdInfo_t*>(contextPtr);
    auto &pos = taf_locPos::GetInstance();

    if (gnssResult != LE_OK)
    {
        LE_ERROR("PosCtrlRequestRespond: GNSS start failed [rc=%d] - releasing refs", gnssResult);

        // Clean up the refs allocated in the entry point
        le_ref_DeleteRef(pos.ActivationRequestRefMap, cPtr->activationRef);
        le_mem_Release(cPtr->clientRequestPtr);

        taf_locPosCtrl_RequestRespond(cPtr->cmdRef, NULL);
        le_mem_Release(cPtr);
        return;
    }

    // GNSS started OK — increment activations and finalize client request
    pos.CurrentActivationsCount++;
    cPtr->clientRequestPtr->sessionRef       = cPtr->sessionRef;
    cPtr->clientRequestPtr->posCtrlActivationRef = cPtr->activationRef;

    LE_DEBUG("PosCtrlRequestRespond: ref(%p) sessionRef(%p)",
             cPtr->activationRef, cPtr->sessionRef);

    taf_locPosCtrl_RequestRespond(cPtr->cmdRef, cPtr->activationRef);
    le_mem_Release(cPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * [1/3 ENTRY] TAF server entry point for posCtrl Request.
 * Allocates activation ref and queues GNSS start via internal async API.
 */
//--------------------------------------------------------------------------------------------------
void taf_locPos::locPosCtrl_Request
(
    taf_locPosCtrl_ServerCmdRef_t cmdRef
)
{
    auto &pos = taf_locPos::GetInstance();
    auto &gnss = taf_locGnss::GetInstance();

    // Allocate client request node and activation ref upfront
    ClientRequest_t* clientRequestPtr =
        (ClientRequest_t*)le_mem_ForceAlloc(PosCtrlHandlerPoolRef);

    taf_locPosCtrl_ActivationRef_t reqRef =
        (taf_locPosCtrl_ActivationRef_t)le_ref_CreateRef(ActivationRequestRefMap,
                                                          clientRequestPtr);

    // Pack into cmd info to carry across async boundary
    PosCtrlCmdInfo_t* cPtr = (PosCtrlCmdInfo_t*)le_mem_ForceAlloc(PosCtrlCmdPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef           = cmdRef;
    cPtr->activationRef    = reqRef;
    cPtr->sessionRef       = taf_locPosCtrl_GetClientSessionRef();
    cPtr->clientRequestPtr = clientRequestPtr;

    if (pos.CurrentActivationsCount == 0)
    {
        // First activation — need to start GNSS first, respond only after start completes
#ifdef LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING
        le_cfg_IteratorRef_t posConfig = le_cfg_CreateWriteTxn(CFG_POSITIONING_PATH);
        le_cfg_SetInt(posConfig, CFG_NODE_RATE, AcqRate);
        le_cfg_CommitTxn(posConfig);
#endif
        if (gnss.SetAcquisitionRate(AcqRate) != LE_OK)
        {
            LE_WARN("GnssStartInternalWorker:Failed to setAcquisition rate(%" PRIu32 ")", AcqRate);
        }

        // Async GNSS start — PosCtrlRequestRespond called on completion
        gnss.StartInternal(cPtr->sessionRef, PosCtrlRequestRespond, cPtr);
    }
    else
    {
        // GNSS already running — respond immediately with new activation ref
        pos.CurrentActivationsCount++;
        clientRequestPtr->sessionRef           = cPtr->sessionRef;
        clientRequestPtr->posCtrlActivationRef = reqRef;

        LE_DEBUG("taf_locPosCtrl_Request (already active): ref(%p) sessionRef(%p)",
                 reqRef, cPtr->sessionRef);

        taf_locPosCtrl_RequestRespond(cmdRef, reqRef);
        le_mem_Release(cPtr);
    }
}


void taf_locPos::PositionHandler
(
 taf_locGnss_SampleRef_t positionRef,
 void* contextPtr
)
{
    auto &pos = taf_locPos::GetInstance();
    auto &gnss = taf_locGnss::GetInstance();
    le_result_t result;
    bool        locationValid = false;
    bool        altitudeValid = false;
    int32_t     longitude;
    int32_t     latitude;
    int32_t     altitude;
    int32_t     hAccuracy;
    int32_t     vAccuracy;
    int32_t  vSpeed;
    uint32_t hSpeed;
    uint32_t direction;
    uint32_t hSpeedAccuracy;
    int32_t  vSpeedAccuracy;
    uint32_t directionAccuracy;
    uint16_t milliseconds;
    uint16_t seconds;
    uint16_t minutes;
    uint16_t hours;
    uint16_t day;
    uint16_t month;
    uint16_t year;
    uint8_t leapSeconds;
    PositionParam_t posObj;
    taf_locGnss_FixState_t gnssState;

    taf_locPos_SampleHandler_t* posHandlerNodePtr;
    PosSampleRequest_t*     posRequestPtr = NULL;

    TAF_ERROR_IF_RET_NIL( positionRef == NULL, "positionRef is Null");

    if (!pos.NumOfHandlers)
    {
        LE_DEBUG("Release Handler,No positioning Sample handler");
        gnss.ReleaseSampleRef(positionRef);
        return;
    }

    LE_DEBUG("Handler Function called %p", positionRef);

    result = gnss.GetLocation(positionRef, &latitude, &longitude, &hAccuracy);
    if ((LE_OK == result) ||
        ((LE_OUT_OF_RANGE == result) && (INT32_MAX != latitude) && (INT32_MAX != longitude)))
    {
        LE_DEBUG("Position lat.% " PRIi32 ", long.% " PRIi32 ", hAccuracy.% " PRIi32,
                 latitude, longitude, TransformDistance(hAccuracy, H_ACCURACY));
        locationValid = true;
    }
    else
    {
        LE_DEBUG("Position unknown [% " PRIi32 ",%" PRIi32 ",%" PRIi32 "]", latitude, longitude, hAccuracy);
        locationValid = false;
    }

    result = gnss.GetAltitude(positionRef, &altitude, &vAccuracy);

    if ((LE_OK == result) ||
        ((LE_OUT_OF_RANGE != result) && (INT32_MAX != altitude)))
    {
        LE_DEBUG("Altitude.%" PRIi32 ", verticalAccuracy.%" PRIi32, TransformDistance(altitude, ALTITUDE), TransformDistance(vAccuracy, V_ACCURACY));
        altitudeValid = true;
    }
    else
    {
        LE_DEBUG("Altitude unknown [%" PRIi32 ",%" PRIi32 "]", altitude, vAccuracy);
        altitudeValid = false;
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(pos.MovementHandlerRefMap);

    posObj.locationValid = locationValid;
    posObj.altitudeValid = altitudeValid;
    posObj.latitude = latitude;
    posObj.longitude = longitude;
    posObj.hAccuracy = hAccuracy;
    posObj.altitude = altitude;
    posObj.vAccuracy = vAccuracy;

    while(le_ref_NextNode(iterRef) == LE_OK)
    {
        bool horizontalFlag, verticalFlag;
        posHandlerNodePtr = (taf_locPos_SampleHandler_t*)le_ref_GetValue(iterRef);
        LE_ASSERT(posHandlerNodePtr != NULL);

        if (LE_FAULT == CalculateMove(posHandlerNodePtr, &posObj, &horizontalFlag, &verticalFlag))
        {
            gnss.ReleaseSampleRef(positionRef);
            return;
        }

        if (((0 != posHandlerNodePtr->verticalMagnitude) && (verticalFlag)) ||
             ((0 == posHandlerNodePtr->verticalMagnitude)
             && (0 == posHandlerNodePtr->horizontalMagnitude)) ||
             ((0 != posHandlerNodePtr->horizontalMagnitude) && (horizontalFlag)))
        {
            posRequestPtr = (PosSampleRequest_t*)le_mem_ForceAlloc(pos.PosRequestPoolRef);
            memset(posRequestPtr,0,sizeof(PosSampleRequest_t));
            posRequestPtr->posSampleNodePtr = (taf_locPos_Sample_t*)le_mem_ForceAlloc(pos.PosPoolRef);
            memset(posRequestPtr->posSampleNodePtr,0,sizeof(taf_locPos_Sample_t));
            posRequestPtr->posSampleNodePtr->latitudeValid = CHECK_VALIDITY(latitude,INT32_MAX);
            posRequestPtr->posSampleNodePtr->latitude = latitude;

            posRequestPtr->posSampleNodePtr->longitudeValid = CHECK_VALIDITY(longitude,INT32_MAX);
            posRequestPtr->posSampleNodePtr->longitude = longitude;

            posRequestPtr->posSampleNodePtr->altitudeValid = CHECK_VALIDITY(altitude,INT32_MAX);
            posRequestPtr->posSampleNodePtr->altitude = altitude;

            posRequestPtr->posSampleNodePtr->hAccuracyValid = CHECK_VALIDITY(hAccuracy,INT32_MAX);
            posRequestPtr->posSampleNodePtr->hAccuracy = hAccuracy;

            posRequestPtr->posSampleNodePtr->vAccuracyValid = CHECK_VALIDITY(vAccuracy,INT32_MAX);
            posRequestPtr->posSampleNodePtr->vAccuracy = vAccuracy;

            gnss.GetHorizontalSpeed(positionRef, &hSpeed, &hSpeedAccuracy);
            posRequestPtr->posSampleNodePtr->hSpeedValid = CHECK_VALIDITY(hSpeed,UINT32_MAX);
            posRequestPtr->posSampleNodePtr->hSpeed = hSpeed;
            posRequestPtr->posSampleNodePtr->hSpeedAccuracyValid = CHECK_VALIDITY(hSpeedAccuracy,UINT32_MAX);
            posRequestPtr->posSampleNodePtr->hSpeedAccuracy = hSpeedAccuracy;

            gnss.GetVerticalSpeed(positionRef, &vSpeed, &vSpeedAccuracy);
            posRequestPtr->posSampleNodePtr->vSpeedValid = CHECK_VALIDITY(vSpeed,INT32_MAX);
            posRequestPtr->posSampleNodePtr->vSpeed = vSpeed;
            posRequestPtr->posSampleNodePtr->vSpeedAccuracyValid = CHECK_VALIDITY(vSpeedAccuracy,INT32_MAX);
            posRequestPtr->posSampleNodePtr->vSpeedAccuracy = vSpeedAccuracy;

            posRequestPtr->posSampleNodePtr->headingAccuracyValid = false;
            posRequestPtr->posSampleNodePtr->headingAccuracy = UINT32_MAX;
            posRequestPtr->posSampleNodePtr->headingValid = false;
            posRequestPtr->posSampleNodePtr->heading = UINT32_MAX;

            gnss.GetDirection(positionRef, &direction, &directionAccuracy);

            posRequestPtr->posSampleNodePtr->directionAccuracyValid = CHECK_VALIDITY(directionAccuracy,UINT32_MAX);
            posRequestPtr->posSampleNodePtr->directionAccuracy = directionAccuracy;
            posRequestPtr->posSampleNodePtr->directionValid = CHECK_VALIDITY(direction,UINT32_MAX);
            posRequestPtr->posSampleNodePtr->direction = direction;

            if (LE_OK == gnss.GetDate(positionRef, &year, &month, &day))
            {
                posRequestPtr->posSampleNodePtr->dateValid = true;
            }
            else
            {
                posRequestPtr->posSampleNodePtr->dateValid = false;
            }
            posRequestPtr->posSampleNodePtr->day = day;
            posRequestPtr->posSampleNodePtr->month = month;
            posRequestPtr->posSampleNodePtr->year = year;

            if (LE_OK == gnss.GetTime(positionRef, &hours, &minutes, &seconds, &milliseconds))
            {
                posRequestPtr->posSampleNodePtr->timeValid = true;
            }
            else
            {
                posRequestPtr->posSampleNodePtr->timeValid = false;
            }
            posRequestPtr->posSampleNodePtr->milliseconds = milliseconds;
            posRequestPtr->posSampleNodePtr->seconds = seconds;
            posRequestPtr->posSampleNodePtr->minutes = minutes;
            posRequestPtr->posSampleNodePtr->hours = hours;

            if (LE_OK == gnss.GetGpsLeapSeconds(positionRef, &leapSeconds))
            {
               posRequestPtr->posSampleNodePtr->leapSecondsValid = true;
            }
            else
            {
                posRequestPtr->posSampleNodePtr->leapSecondsValid = false;
            }
            posRequestPtr->posSampleNodePtr->leapSeconds = leapSeconds;

            if (LE_OK != gnss.GetPositionState(positionRef, &gnssState))
            {
                posRequestPtr->posSampleNodePtr->fixState = TAF_LOCGNSS_STATE_FIX_NO_POS;
                LE_ERROR("Failed to get a position fix");
            }
            else
            {
                posRequestPtr->posSampleNodePtr->fixState = (taf_locGnss_FixState_t)gnssState;
            }

            posHandlerNodePtr->lastLat = latitude;
            posHandlerNodePtr->lastLong = longitude;
            posHandlerNodePtr->lastAlt = altitude;

            LE_DEBUG("Report sampleRef %p to the corresponding handler (handlerPtr %p)",
                     posRequestPtr->posSampleNodePtr, posHandlerNodePtr->handlerFuncPtr);

            taf_locPos_SampleRef_t reqRef = (taf_locPos_SampleRef_t)le_ref_CreateRef(pos.PosSampleMap, posRequestPtr);

            posRequestPtr->sessionRef = posHandlerNodePtr->sessionRef;

            posRequestPtr->positionSampleRef = reqRef;

            posHandlerNodePtr->handlerFuncPtr(reqRef, posHandlerNodePtr->handlerContextPtr);
        }

    }

    gnss.ReleaseSampleRef(positionRef);
}

taf_locPos_MovementHandlerRef_t taf_locPos::AddMovementHandler
(
 uint32_t horizontalMagnitude,
 uint32_t verticalMagnitude,
 taf_locPos_MovementHandlerFunc_t handlerPtr,
 void* contextPtr
 )
{
    taf_locPos_SampleHandler_t*  posHandlerNodePtr = NULL;
    TAF_KILL_CLIENT_IF_RET_VAL((handlerPtr == NULL), NULL, "handlerPtr pointer is NULL");

    posHandlerNodePtr = (taf_locPos_SampleHandler_t*)le_mem_ForceAlloc(PosHandlerPoolRef);
    memset(posHandlerNodePtr, 0, sizeof(taf_locPos_SampleHandler_t));
    posHandlerNodePtr->next = LE_DLS_LINK_INIT;
    posHandlerNodePtr->handlerFuncPtr = handlerPtr;
    posHandlerNodePtr->handlerContextPtr = contextPtr;
    posHandlerNodePtr->acquisitionRate = CalculateAcquisitionRate(SUPPOSED_AVERAGE_SPEED,
                                                  horizontalMagnitude, verticalMagnitude) * SEC_TO_MSEC;
    posHandlerNodePtr->sessionRef = taf_locPos_GetClientSessionRef();
    posHandlerNodePtr->handlerRef =
        (taf_locPos_MovementHandlerRef_t)le_ref_CreateRef(MovementHandlerRefMap, posHandlerNodePtr);
    AcqRate = ComputeCommonSmallestRate(posHandlerNodePtr->acquisitionRate);

    LE_DEBUG("Calculated acquisition rate %" PRIu32 " msec for an average speed of %d km/h",
             posHandlerNodePtr->acquisitionRate, SUPPOSED_AVERAGE_SPEED);
    LE_DEBUG("Smallest computed acquisition rate %" PRIu32 " msec", AcqRate);

#ifdef LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING
    {
        le_cfg_IteratorRef_t posConfig = le_cfg_CreateWriteTxn(CFG_POSITIONING_PATH);
        le_cfg_SetInt(posConfig, CFG_NODE_RATE, AcqRate);
        le_cfg_CommitTxn(posConfig);
    }
#endif

    posHandlerNodePtr->verticalMagnitude = verticalMagnitude;
    posHandlerNodePtr->horizontalMagnitude = horizontalMagnitude;

    posHandlerNodePtr->lastLat = 0;
    posHandlerNodePtr->lastAlt = 0;
    posHandlerNodePtr->lastLong = 0;

    if (0 == NumOfHandlers)
    {
        if (NULL == (GnssHandlerRef=taf_locGnss_AddPositionHandler(PositionHandler, NULL)))
        {
            LE_ERROR("Failed to add GNSS's handler!");
            le_mem_Release(posHandlerNodePtr);
            return NULL;
        }
    }

    NumOfHandlers++;
    return posHandlerNodePtr->handlerRef;
}

le_result_t taf_locPos::SetAcquisitionRate
(
    taf_locPos_ServerCmdRef_t cmdRef,
    uint32_t  acqRate
)
{
        LE_INFO("SetAcquisitionRate Called");
#ifdef LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING
    if (!acqRate)
    {
        LE_WARN("Invalid acquisition rate (%" PRIu32 ")", acqRate);
        return LE_OUT_OF_RANGE;
    }

    le_cfg_IteratorRef_t posConfig = le_cfg_CreateWriteTxn(CFG_POSITIONING_PATH);
    le_cfg_SetInt(posConfig,CFG_NODE_RATE,acqRate);
    le_cfg_CommitTxn(posConfig);
    return LE_OK;
#else
    return LE_UNSUPPORTED;
#endif
}

le_result_t taf_locPos::Get2DLocation
(
    int32_t* latPtr,
    int32_t* longPtr,
    int32_t* hAccuracyPtr
)
{
    TAF_KILL_CLIENT_IF_RET_VAL((latPtr == NULL) && (longPtr == NULL)
                               && (hAccuracyPtr == NULL), LE_FAULT, "Invalid input parameters");

    le_result_t resGnss = LE_OK;
    le_result_t resPos = LE_OK;
    int32_t     latitude;
    int32_t     longitude;
    int32_t     hAccuracy;

    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_SampleRef_t positionSampleRef = gnss.GetLastSampleRef();

    resGnss = gnss.GetLocation(positionSampleRef, &latitude, &longitude, &hAccuracy);
    if ((resGnss == LE_OK)||(resGnss == LE_OUT_OF_RANGE))
    {
        if (latPtr)
        {
            if (latitude == INT32_MAX)
            {
                resPos = LE_OUT_OF_RANGE;
            }
            *latPtr = latitude;
        }
        if (longPtr)
        {
            if (longitude == INT32_MAX)
            {
                resPos = LE_OUT_OF_RANGE;
            }
            *longPtr = longitude;
        }
        if (hAccuracyPtr)
        {
            if (hAccuracy == INT32_MAX)
            {
                resPos = LE_OUT_OF_RANGE;
                *hAccuracyPtr = hAccuracy;
            }
            else
            {
                *hAccuracyPtr = TransformDistance(hAccuracy, H_ACCURACY);
            }
        }
    }
    else
    {
        resPos = LE_FAULT;
    }

    gnss.ReleaseSampleRef(positionSampleRef);

    return resPos;
}

uint32_t taf_locPos::GetAcquisitionRate
(
 void
)
{
    uint32_t  acqRate = DEFAULT_ACQUISITION_RATE;

#ifdef LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING
    le_cfg_IteratorRef_t posConfig = le_cfg_CreateReadTxn(CFG_POSITIONING_PATH);
      acqRate = le_cfg_GetInt(posConfig, CFG_NODE_RATE, DEFAULT_ACQUISITION_RATE);
      le_cfg_CancelTxn(posConfig);

    LE_DEBUG("acquisition rate (%" PRIu32 ") for positioning",acqRate);
#endif

    return acqRate;
}

le_result_t taf_locPos::GetFixState
(
    taf_locPos_ServerCmdRef_t cmdRef,
    taf_locGnss_FixState_t* statePtr
)
{
    taf_locGnss_FixState_t gnssState;
    
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_SampleRef_t positionSampleRef = gnss.GetLastSampleRef();

    TAF_KILL_CLIENT_IF_RET_VAL((statePtr == NULL) || (positionSampleRef == NULL), LE_FAULT, "state pointer is NULL / Invalid reference");

    if (LE_OK != gnss.GetPositionState(positionSampleRef, &gnssState))
    {
        *statePtr = TAF_LOCGNSS_STATE_FIX_NO_POS;
        LE_ERROR("Failed to get the position fix state");
    }
    else
    {
        *statePtr = (taf_locGnss_FixState_t)gnssState;
    }

    gnss.ReleaseSampleRef(positionSampleRef);
    return LE_OK;
}

le_result_t taf_locPos::GetTime
(
    uint16_t* hrsPtr,
    uint16_t* minPtr,
    uint16_t* secPtr,
    uint16_t* msecondsPtr
)
{
    TAF_KILL_CLIENT_IF_RET_VAL((hrsPtr == NULL) || (minPtr == NULL) || (secPtr == NULL) || (msecondsPtr == NULL), LE_FAULT, "Invalid input parameters");
    le_result_t resPos = LE_OK;

    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_SampleRef_t positionSampleRef = gnss.GetLastSampleRef();

    resPos = gnss.GetTime(positionSampleRef, hrsPtr, minPtr, secPtr, msecondsPtr);

    gnss.ReleaseSampleRef(positionSampleRef);

    return resPos;
}

le_result_t taf_locPos::GetDate
(
    uint16_t* yearPtr,
    uint16_t* monthPtr,
    uint16_t* dayPtr
)
{
    TAF_KILL_CLIENT_IF_RET_VAL((yearPtr == NULL) || (monthPtr == NULL) || (dayPtr == NULL),
            LE_FAULT, "Invalid input parameters");
    le_result_t posResult = LE_OK;

    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_SampleRef_t positionSampleRef = gnss.GetLastSampleRef();

    posResult = gnss.GetDate(positionSampleRef, yearPtr, monthPtr, dayPtr);

    gnss.ReleaseSampleRef(positionSampleRef);

    return posResult;
}

le_result_t taf_locPos::Get3DLocation
(
    int32_t* latPtr, int32_t* longPtr,
    int32_t* hAccuracyPtr, int32_t* altitudePtr,
    int32_t* vAccuracyPtr
)
{
    // At least one valid pointer
    TAF_KILL_CLIENT_IF_RET_VAL((latPtr == NULL) && (longPtr == NULL) && (hAccuracyPtr == NULL)
               && (altitudePtr == NULL) && (vAccuracyPtr == NULL), LE_FAULT, "Invalid input parameters");

    le_result_t resGnss = LE_OK;
    le_result_t resPos = LE_OK;
    int32_t     latitude;
    int32_t     longitude;
    int32_t     altitude;
    int32_t     hAccuracy;
    int32_t     vAccuracy;

    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_SampleRef_t positionSampleRef = gnss.GetLastSampleRef();

    resGnss = gnss.GetLocation(positionSampleRef, &latitude, &longitude, &hAccuracy);
    if ((resGnss == LE_OK)||(resGnss == LE_OUT_OF_RANGE))
    {
        if (longPtr)
        {
            if (longitude == INT32_MAX)
            {
                resPos = LE_OUT_OF_RANGE;
            }
            *longPtr = longitude;
        }
        if (latPtr)
        {
            if (latitude == INT32_MAX)
            {
                resPos = LE_OUT_OF_RANGE;
            }
            *latPtr = latitude;
        }
        if (hAccuracyPtr)
        {
            if (hAccuracy == INT32_MAX)
            {
                *hAccuracyPtr = hAccuracy;
                resPos = LE_OUT_OF_RANGE;
            }
            else
            {
                *hAccuracyPtr = TransformDistance(hAccuracy, H_ACCURACY);
            }
        }
    }
    else
    {
        resPos = LE_FAULT;
    }

    resGnss = gnss.GetAltitude(positionSampleRef, &altitude, &vAccuracy);

    if (((resGnss == LE_OK)||(resGnss == LE_OUT_OF_RANGE))
        &&(resPos != LE_FAULT))
    {
        if (vAccuracyPtr)
        {
            if (vAccuracy == INT32_MAX)
            {
                *vAccuracyPtr = vAccuracy;
                resPos = LE_OUT_OF_RANGE;
            }
            else
            {
                *vAccuracyPtr = TransformDistance(vAccuracy, V_ACCURACY);
            }
        }
        if (altitudePtr)
        {
            if (altitude == INT32_MAX)
            {
                *altitudePtr = altitude;
                resPos = LE_OUT_OF_RANGE;
            }
            else
            {
                *altitudePtr = TransformDistance(altitude, ALTITUDE);
            }
        }
    }
    else
    {
        resPos = LE_FAULT;
    }

    gnss.ReleaseSampleRef(positionSampleRef);

    return resPos;
}


le_result_t taf_locPos::sample_Get2DLocation
(
    taf_locPos_SampleRef_t positionSampleRef,
    int32_t* latPtr, int32_t* longPtr,
    int32_t* hAccuracyPtr
)
{
    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = (PosSampleRequest_t*)le_ref_Lookup(PosSampleMap, positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL((posSampleRequestPtr == NULL), LE_BAD_PARAMETER, "Invalid reference (%p) provided!",positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL(( posSampleRequestPtr->posSampleNodePtr == NULL), LE_FAULT, "Invalid reference (%p) provided!",positionSampleRef);

    if (longPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->longitudeValid)
        {
            *longPtr = posSampleRequestPtr->posSampleNodePtr->longitude;
        }
        else
        {
            *longPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (latPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->latitudeValid)
        {
            *latPtr = posSampleRequestPtr->posSampleNodePtr->latitude;
        }
        else
        {
            *latPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (hAccuracyPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->hAccuracyValid)
        {
            *hAccuracyPtr =
               TransformDistance(posSampleRequestPtr->posSampleNodePtr->hAccuracy, H_ACCURACY);
        }
        else
        {
            *hAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

le_result_t taf_locPos::sample_GetAltitude
(
    taf_locPos_SampleRef_t positionSampleRef,
    int32_t* altitudePtr, int32_t* altitudeAccuracyPtr
)
{
    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = (PosSampleRequest_t*)le_ref_Lookup(PosSampleMap, positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL((posSampleRequestPtr == NULL), LE_BAD_PARAMETER, "Invalid reference (%p) provided!",positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL(( posSampleRequestPtr->posSampleNodePtr == NULL), LE_FAULT, "Invalid reference (%p) provided!",positionSampleRef);

    if (altitudePtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->altitudeValid)
        {
            *altitudePtr = TransformDistance(posSampleRequestPtr->posSampleNodePtr->altitude, ALTITUDE);
        }
        else
        {
            *altitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (altitudeAccuracyPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->vAccuracyValid)
        {
            *altitudeAccuracyPtr = TransformDistance(posSampleRequestPtr->posSampleNodePtr->vAccuracy, V_ACCURACY);
        }
        else
        {
            *altitudeAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}

le_result_t taf_locPos::sample_GetTime
(
    taf_locPos_SampleRef_t  positionSampleRef,
    uint16_t* hoursPtr, uint16_t* minutesPtr,
    uint16_t* secondsPtr, uint16_t* millisecondsPtr
)
{
    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = (PosSampleRequest_t*)le_ref_Lookup(PosSampleMap, positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL((posSampleRequestPtr == NULL), LE_BAD_PARAMETER, "Invalid reference (%p) provided!",positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL(( posSampleRequestPtr->posSampleNodePtr == NULL), LE_FAULT, "Invalid reference (%p) provided!",positionSampleRef);

    if (posSampleRequestPtr->posSampleNodePtr->timeValid)
    {
        result = LE_OK;
        if (millisecondsPtr)
        {
            *millisecondsPtr = posSampleRequestPtr->posSampleNodePtr->milliseconds;
        }
        if (secondsPtr)
        {
            *secondsPtr = posSampleRequestPtr->posSampleNodePtr->seconds;
        }
        if (minutesPtr)
        {
            *minutesPtr = posSampleRequestPtr->posSampleNodePtr->minutes;
        }
        if (hoursPtr)
        {
            *hoursPtr = posSampleRequestPtr->posSampleNodePtr->hours;
        }
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        if (millisecondsPtr)
        {
            *millisecondsPtr = 0;
        }
        if (secondsPtr)
        {
            *secondsPtr = 0;
        }
        if (minutesPtr)
        {
            *minutesPtr = 0;
        }
        if (hoursPtr)
        {
            *hoursPtr = 0;
        }
    }

    return result;
}

le_result_t taf_locPos::sample_GetDate
(
    taf_locPos_SampleRef_t positionSampleRef,
    uint16_t* yearPtr,
    uint16_t* monthPtr,
    uint16_t* dayPtr
)
{

    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = (PosSampleRequest_t*)le_ref_Lookup(PosSampleMap,
            positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL((posSampleRequestPtr == NULL), LE_BAD_PARAMETER,
            "Invalid reference (%p) provided!",positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL(( posSampleRequestPtr->posSampleNodePtr == NULL), LE_FAULT,
            "Invalid reference (%p) provided!",positionSampleRef);

    if (posSampleRequestPtr->posSampleNodePtr->dateValid)
    {
        result = LE_OK;
        if (yearPtr)
        {
            *yearPtr = posSampleRequestPtr->posSampleNodePtr->year;
        }
        if (monthPtr)
        {
            *monthPtr = posSampleRequestPtr->posSampleNodePtr->month;
        }
        if (dayPtr)
        {
            *dayPtr = posSampleRequestPtr->posSampleNodePtr->day;
        }
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        if (dayPtr)
        {
            *dayPtr = 0;
        }
        if (monthPtr)
        {
            *monthPtr = 0;
        }
        if (yearPtr)
        {
            *yearPtr = 0;
        }
    }

    return result;
}
le_result_t taf_locPos::sample_GetHorizontalSpeed
(
    taf_locPos_SampleRef_t positionSampleRef,
    uint32_t* horizontalSpeedPtr,
    uint32_t* horizontalSpeedAccuracyPtr
)
{
    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = (PosSampleRequest_t*)le_ref_Lookup(PosSampleMap, positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL((posSampleRequestPtr == NULL), LE_BAD_PARAMETER, "Invalid reference (%p) provided!",positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL(( posSampleRequestPtr->posSampleNodePtr == NULL), LE_FAULT, "Invalid reference (%p) provided!",positionSampleRef);
    if (horizontalSpeedAccuracyPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->hSpeedAccuracyValid)
        {
            *horizontalSpeedAccuracyPtr = posSampleRequestPtr->posSampleNodePtr->hSpeedAccuracy/10;
        }
        else
        {
            *horizontalSpeedAccuracyPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (horizontalSpeedPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->hSpeedValid)
        {
            *horizontalSpeedPtr = posSampleRequestPtr->posSampleNodePtr->hSpeed/100;
        }
        else
        {
            *horizontalSpeedPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}

le_result_t taf_locPos::sample_GetDirection
(
    taf_locPos_SampleRef_t  positionSampleRef,
    uint32_t* dirPtr, uint32_t* dirAccuracyPtr
)
{
    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = (PosSampleRequest_t*)le_ref_Lookup(PosSampleMap, positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL((posSampleRequestPtr == NULL), LE_BAD_PARAMETER, "Invalid reference (%p) provided!",positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL(( posSampleRequestPtr->posSampleNodePtr == NULL), LE_FAULT, "Invalid reference (%p) provided!",positionSampleRef);

    if (dirAccuracyPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->directionAccuracyValid)
        {
            *dirAccuracyPtr = posSampleRequestPtr->posSampleNodePtr->directionAccuracy/10;
        }
        else
        {
            *dirAccuracyPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (dirPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->directionValid)
        {
            *dirPtr = posSampleRequestPtr->posSampleNodePtr->direction/10;
        }
        else
        {
            *dirPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}

le_result_t taf_locPos::sample_GetVerticalSpeed
(
    taf_locPos_SampleRef_t  positionSampleRef,
    int32_t* verticalSpeedPtr, int32_t* verticalSpeedAccuracyPtr
)
{
    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = (PosSampleRequest_t*)le_ref_Lookup(PosSampleMap, positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL((posSampleRequestPtr == NULL), LE_BAD_PARAMETER, "Invalid reference (%p) provided!",positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL(( posSampleRequestPtr->posSampleNodePtr == NULL), LE_FAULT, "Invalid reference (%p) provided!",positionSampleRef);

    if (verticalSpeedAccuracyPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->vSpeedAccuracyValid)
        {
            *verticalSpeedAccuracyPtr = posSampleRequestPtr->posSampleNodePtr->vSpeedAccuracy/10;
        }
        else
        {
            *verticalSpeedAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (verticalSpeedPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->vSpeedValid)
        {
            *verticalSpeedPtr = posSampleRequestPtr->posSampleNodePtr->vSpeed/100;
        }
        else
        {
            *verticalSpeedPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}

le_result_t taf_locPos::SetDistanceResolution
(
    taf_locPos_Resolution_t resolution
)
{
    if (resolution >= TAF_LOCPOS_RES_UNKNOWN)
    {
        LE_ERROR("Invalid resolution (%d)", resolution);
        return LE_BAD_PARAMETER;
    }

    LE_DEBUG("resolution %d saved", resolution);

    DistanceResolution = resolution;
    return LE_OK;
}

le_result_t taf_locPos::sample_GetFixState
(
    taf_locPos_SampleRef_t  positionSampleRef,
    taf_locGnss_FixState_t*  statePtr
)
{
    PosSampleRequest_t* posSampleRequestPtr = (PosSampleRequest_t*)le_ref_Lookup(PosSampleMap, positionSampleRef);

    TAF_ERROR_IF_RET_VAL((NULL == statePtr), LE_BAD_PARAMETER, "Invalid parameters");

    TAF_KILL_CLIENT_IF_RET_VAL((posSampleRequestPtr == NULL), LE_BAD_PARAMETER, "Invalid reference (%p) provided!",positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL(( posSampleRequestPtr->posSampleNodePtr == NULL), LE_FAULT, "Invalid reference (%p) provided!",positionSampleRef);

    if (statePtr)
    {
        *statePtr = posSampleRequestPtr->posSampleNodePtr->fixState;
    }

    return LE_OK;
}

void taf_locPos::RemoveMovementHandler
(
 taf_locPos_MovementHandlerRef_t handlerRef
)
{
    auto &gnss = taf_locGnss::GetInstance();

    taf_locPos_SampleHandler_t* posHandlerNodePtr =
        (taf_locPos_SampleHandler_t*)le_ref_Lookup(MovementHandlerRefMap, handlerRef);

    if (posHandlerNodePtr != NULL)
    {
        LE_ASSERT(posHandlerNodePtr->handlerRef == handlerRef);
        NumOfHandlers--;
        LE_DEBUG("Removed positionHandlerRef(%p) for posHandlerNodePtr(%p) (totalCnt=0x%x).",
            posHandlerNodePtr->handlerRef, posHandlerNodePtr, NumOfHandlers);
        le_mem_Release(posHandlerNodePtr);
        le_ref_DeleteRef(MovementHandlerRefMap, handlerRef);
    }
    else
    {
        LE_ERROR("Invaild handlerRef(%p).", handlerRef);
    }

    if (NumOfHandlers == 0)
    {
        gnss.RemovePositionHandler(GnssHandlerRef);
        GnssHandlerRef = NULL;
    }
}

static void PosCtrlReleaseInternalRespond(void* cmdPtr, void*)
{
    PosCtrlCmdInfo_t* cPtr = (PosCtrlCmdInfo_t*)cmdPtr;
    auto &pos = taf_locPos::GetInstance();

    LE_DEBUG("PosCtrlReleaseInternalRespond: activationRef=%p retCode=%d",
             cPtr->activationRef, cPtr->retCode);

    if (cPtr->retCode != LE_OK)
    {
        LE_ERROR("PosCtrlReleaseInternalRespond: GNSS Stop failed [rc=%d] — "
                 "proceeding with cleanup anyway", cPtr->retCode);
    }

    le_ref_DeleteRef(pos.ActivationRequestRefMap, cPtr->activationRef);
    LE_DEBUG("PosCtrlReleaseInternalRespond: Remove Position Ctrl (%p)", cPtr->activationRef);
    le_mem_Release(cPtr->posPtr);

    le_mem_Release(cPtr);
}

static void OnGnssStopCompleteInternal(le_result_t result, void* contextPtr)
{
    auto &gnss = taf_locGnss::GetInstance();
    PosCtrlCmdInfo_t* cPtr = (PosCtrlCmdInfo_t*)contextPtr;
    cPtr->retCode = result;

    // ✅ Queue cleanup back to main LocationSvc thread
    le_event_QueueFunctionToThread(
        gnss.LocationSvcThRef, PosCtrlReleaseInternalRespond, cPtr, NULL);
}

void taf_locPos::locPosCtrl_ReleaseInternal
(
    taf_locPosCtrl_ActivationRef_t ref,
    le_msg_SessionRef_t            sessionRef
)
{
    auto &gnss = taf_locGnss::GetInstance();

    void* posPtr = le_ref_Lookup(ActivationRequestRefMap, ref);
    if (posPtr == NULL)
    {
        LE_ERROR("locPosCtrl_ReleaseInternal: Invalid ref (%p)", ref);
        return;
    }

    PosCtrlCmdInfo_t* cPtr =
        (PosCtrlCmdInfo_t*)le_mem_ForceAlloc(PosCtrlCmdPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));

    cPtr->cmdRef        = NULL;
    cPtr->activationRef = ref;
    cPtr->posPtr        = posPtr;
    cPtr->sessionRef    = sessionRef;

    if (CurrentActivationsCount > 0)
    {
        CurrentActivationsCount--;

        if (CurrentActivationsCount == 0)
        {
            LE_INFO("locPosCtrl_ReleaseInternal: Last client, stopping GNSS async");

            gnss.StopInternal(sessionRef, OnGnssStopCompleteInternal, cPtr);
            return;
        }
    }

    le_ref_DeleteRef(ActivationRequestRefMap, ref);
    LE_DEBUG("locPosCtrl_ReleaseInternal: Remove Position Ctrl (%p)", ref);
    le_mem_Release(posPtr);
    le_mem_Release(cPtr);
}

void taf_locPos::PosCtrlCloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    auto &pos = taf_locPos::GetInstance();
    LE_DEBUG("PosCtrlCloseSessionEventHandler: SessionRef (%p) has been closed", sessionRef);

    if (!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(pos.ActivationRequestRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        ClientRequest_t* posCtrlHandlerPtr =
            (ClientRequest_t*)le_ref_GetValue(iterRef);
        LE_ASSERT(posCtrlHandlerPtr != NULL);

        if (posCtrlHandlerPtr->sessionRef == sessionRef)
        {
            taf_locPosCtrl_ActivationRef_t safeRef =
                (taf_locPosCtrl_ActivationRef_t)le_ref_GetSafeRef(iterRef);

            LE_DEBUG("PosCtrlCloseSessionEventHandler: releasing safeRef=0x%p session=0x%p",
                     safeRef, sessionRef);

            pos.locPosCtrl_ReleaseInternal(safeRef, sessionRef);
        }
    }
}

static void PosCtrlReleaseRespond(void* cmdPtr, void*)
{
    PosCtrlCmdInfo_t* cPtr = (PosCtrlCmdInfo_t*)cmdPtr;
    auto &pos = taf_locPos::GetInstance();

    if (cPtr->retCode != LE_OK)
    {
        LE_ERROR("PosCtrlReleaseRespond: GNSS Stop failed [rc=%d]", cPtr->retCode);
    }

    le_ref_DeleteRef(pos.ActivationRequestRefMap, cPtr->activationRef);
    LE_DEBUG("PosCtrlReleaseRespond: Remove Position Ctrl (%p)", cPtr->activationRef);
    le_mem_Release(cPtr->posPtr);

    if (cPtr->cmdRef != NULL)
    {
        taf_locPosCtrl_ReleaseRespond(cPtr->cmdRef);
    }

    le_mem_Release(cPtr);
}

static void OnGnssStopComplete(le_result_t result, void* contextPtr)
{
    auto &gnss = taf_locGnss::GetInstance();
    PosCtrlCmdInfo_t* cPtr = (PosCtrlCmdInfo_t*)contextPtr;
    cPtr->retCode = result;

    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, PosCtrlReleaseRespond, cPtr, NULL);
}

void taf_locPos::locPosCtrl_Release
(
    taf_locPosCtrl_ServerCmdRef_t  cmdRef,
    taf_locPosCtrl_ActivationRef_t ref
)
{
    auto &gnss = taf_locGnss::GetInstance();

    void* posPtr = le_ref_Lookup(ActivationRequestRefMap, ref);
    TAF_KILL_CLIENT_IF_RET_NIL((posPtr == NULL),
        "Invalid positioning service activation reference %p", ref);

    PosCtrlCmdInfo_t* cPtr =
        (PosCtrlCmdInfo_t*)le_mem_ForceAlloc(PosCtrlCmdPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef        = cmdRef;
    cPtr->activationRef = ref;
    cPtr->posPtr        = posPtr;
    cPtr->sessionRef    = taf_locPosCtrl_GetClientSessionRef();

    if (CurrentActivationsCount > 0)
    {
        CurrentActivationsCount--;

        if (CurrentActivationsCount == 0)
        {
            LE_INFO("locPosCtrl_Release: Last client, stopping GNSS async");
            gnss.StopInternal(cPtr->sessionRef, OnGnssStopComplete, cPtr);
            return;
        }
    }

    le_ref_DeleteRef(ActivationRequestRefMap, ref);
    LE_DEBUG("locPosCtrl_Release: Remove Position Ctrl (%p)", ref);
    le_mem_Release(posPtr);
    taf_locPosCtrl_ReleaseRespond(cmdRef);
    le_mem_Release(cPtr);
}

void taf_locPos::Release
(
    taf_locPos_ServerCmdRef_t cmdRef,
    taf_locPos_SampleRef_t positionSampleRef
)
{
    TAF_KILL_CLIENT_IF_RET_NIL((positionSampleRef == NULL),  "Invalid reference");

    PosSampleRequest_t* posRequestPtr = (PosSampleRequest_t*)le_ref_Lookup(PosSampleMap, positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_NIL((posRequestPtr == NULL) || (posRequestPtr->posSampleNodePtr == NULL),  "Invalid reference");

    le_ref_DeleteRef(PosSampleMap, positionSampleRef);
    le_mem_Release(posRequestPtr->posSampleNodePtr);
    le_mem_Release(posRequestPtr);
}

void taf_locPos::ReleaseInternal
(
    taf_locPos_SampleRef_t positionSampleRef
)
{
    TAF_ERROR_IF_RET_NIL((positionSampleRef == NULL), "Null positionSampleRef");

    PosSampleRequest_t* posRequestPtr =
        (PosSampleRequest_t*)le_ref_Lookup(PosSampleMap, positionSampleRef);

    if (posRequestPtr == NULL || posRequestPtr->posSampleNodePtr == NULL)
    {
        LE_ERROR("ReleaseInternal: Invalid reference (%p)", positionSampleRef);
        return;
    }

    le_ref_DeleteRef(PosSampleMap, positionSampleRef);
    le_mem_Release(posRequestPtr->posSampleNodePtr);
    le_mem_Release(posRequestPtr);

    LE_DEBUG("ReleaseInternal: released positionSampleRef (%p)", positionSampleRef);
}

void taf_locPos::PosCloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    auto &pos = taf_locPos::GetInstance();
    LE_DEBUG("PosCloseSessionEventHandler: SessionRef (%p) has been closed", sessionRef);

    if (!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(pos.PosSampleMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        PosSampleRequest_t* posRequestPtr =
            (PosSampleRequest_t*)le_ref_GetValue(iterRef);
        LE_ASSERT(posRequestPtr != NULL);

        if (posRequestPtr->sessionRef == sessionRef)
        {
            taf_locPos_SampleRef_t safeRef =
                (taf_locPos_SampleRef_t)le_ref_GetSafeRef(iterRef);

            LE_DEBUG("PosCloseSessionEventHandler: releasing safeRef=0x%p session=0x%p",
                     safeRef, sessionRef);

            pos.ReleaseInternal(safeRef);
        }
    }
}

void taf_locPos::Init()
{
   LE_INFO("taf_locPos Init!!");

   PosPoolRef = le_mem_InitStaticPool(PosSample, TAF_LOCPOS_MAX_OBJ, sizeof(taf_locPos_Sample_t));
   PosRequestPoolRef = le_mem_InitStaticPool(PosSampleRequest, TAF_LOCPOS_MAX_OBJ, sizeof(PosSampleRequest_t));
   PosCtrlCmdPoolRef = le_mem_InitStaticPool(PosCtrlCmdPool, MAX_CMD_LOCATION_POOL_SIZE, sizeof(PosCtrlCmdInfo_t));

   posMsgService = taf_locPos_GetServiceRef();
   le_msg_AddServiceCloseHandler(posMsgService, PosCloseSessionEventHandler, NULL);

   posCtrlMsgService = taf_locPosCtrl_GetServiceRef();
   le_msg_AddServiceCloseHandler(posCtrlMsgService, PosCtrlCloseSessionEventHandler, NULL);
   PosHandlerPoolRef = le_mem_InitStaticPool(PosHandler, HIGH_POS_HANDLER_COUNT, sizeof(taf_locPos_SampleHandler_t));
   PosSampleMap = le_ref_InitStaticMap(PosSampleMap, POSITIONING_SAMPLE_MAX);
   ActivationRequestRefMap = le_ref_InitStaticMap(PositioningClient, TAF_CONFIG_POSITIONING_ACTIVATION_MAX);
   PosCtrlHandlerPoolRef = le_mem_InitStaticPool(PosCtrlHandler, TAF_CONFIG_POSITIONING_ACTIVATION_MAX,
           sizeof(ClientRequest_t));
   MovementHandlerRefMap = le_ref_CreateMap("MovementHandlerRefMap", 16);
   DistanceResolution = TAF_LOCPOS_RES_METER;
   AcqRate = DEFAULT_ACQUISITION_RATE;
   CurrentActivationsCount = 0;
   NumOfHandlers = 0;
   GnssHandlerRef = NULL;
   return;
}
