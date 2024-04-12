/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include "main.h"

static le_mem_PoolRef_t DrFramePool = NULL;
static le_mem_PoolRef_t LevArmFramePool = NULL;

static le_sem_Ref_t PositionHandlerSem;
static taf_gnss_PositionHandlerRef_t PositionHandlerRef = NULL;
static taf_pos_MovementHandlerRef_t  SamplePositionHandlerRef = NULL;
static taf_gnss_NmeaHandlerRef_t NmeaHandlerRef = NULL;
static taf_gnss_CapabilityChangeHandlerRef_t CapabilityChangeHandlerRef = NULL;


void PrintGnssSignalType(uint32_t signalTypeMask) {
   LE_TEST_INFO("Signals: ");
   if (signalTypeMask & TAF_GNSS_GPS_L1CA) {
     LE_TEST_INFO("GPS L1CA, ");
   }
   if (signalTypeMask & TAF_GNSS_GPS_L1C) {
     LE_TEST_INFO("GPS L1C, ");
   }
   if (signalTypeMask & TAF_GNSS_GPS_L2) {
     LE_TEST_INFO("GPS L2, ");
   }
   if (signalTypeMask & TAF_GNSS_GPS_L5) {
     LE_TEST_INFO("GPS L5, ");
   }
   if (signalTypeMask & TAF_GNSS_GLONASS_G1) {
     LE_TEST_INFO("Glonass G1, ");
   }
   if (signalTypeMask & TAF_GNSS_GLONASS_G2) {
     LE_TEST_INFO("Glonass G2, ");
   }
   if (signalTypeMask & TAF_GNSS_GALILEO_E1) {
     LE_TEST_INFO("Galileo E1, ");
   }
   if (signalTypeMask & TAF_GNSS_GALILEO_E5A) {
     LE_TEST_INFO("Galileo E5A, ");
   }
   if (signalTypeMask & TAF_GNSS_GALILIEO_E5B) {
     LE_TEST_INFO("Galileo E5B, ");
   }
   if (signalTypeMask & TAF_GNSS_BEIDOU_B1) {
     LE_TEST_INFO("Beidou B1, ");
   }
   if (signalTypeMask & TAF_GNSS_BEIDOU_B2) {
     LE_TEST_INFO("Beidou B2, ");
   }
   if (signalTypeMask & TAF_GNSS_QZSS_L1CA) {
     LE_TEST_INFO("QZSS L1CA, ");
   }
   if (signalTypeMask & TAF_GNSS_QZSS_L1S) {
     LE_TEST_INFO("QZSS L1S, ");
   }
   if (signalTypeMask & TAF_GNSS_QZSS_L2) {
     LE_TEST_INFO("QZSS L2, ");
   }
   if (signalTypeMask & TAF_GNSS_QZSS_L5) {
     LE_TEST_INFO("QZSS L5, ");
   }
   if (signalTypeMask & TAF_GNSS_SBAS_L1) {
     LE_TEST_INFO("SBAS L1, ");
   }
   if (signalTypeMask & TAF_GNSS_BEIDOU_B1I) {
     LE_TEST_INFO("Beidou B1I, ");
   }
   if (signalTypeMask & TAF_GNSS_BEIDOU_B1C) {
     LE_TEST_INFO("Beidou B1C, ");
   }
   if (signalTypeMask & TAF_GNSS_BEIDOU_B2I) {
     LE_TEST_INFO("Beidou B2I, ");
   }
   if (signalTypeMask & TAF_GNSS_BEIDOU_B2AI) {
     LE_TEST_INFO("Beidou B2AI, ");
   }
   if (signalTypeMask & TAF_GNSS_NAVIC_L5) {
     LE_TEST_INFO("Navic L5, ");
   }
   if (signalTypeMask & TAF_GNSS_BEIDOU_B2AQ) {
     LE_TEST_INFO("Beidou B2AQ, ");
   }
   if (signalTypeMask == TAF_GNSS_UNKNOWN_SIGNAL_MASK) {
     LE_TEST_INFO("No signal, ");
   }
}

static void PositionHandlerFunction
(
    taf_gnss_SampleRef_t positionSampleRef,
    void* contextPtr
)
{
    le_result_t result;
    taf_gnss_FixState_t state;
    int32_t  latitude;
    int32_t  longitude;
    int32_t  hAccuracy;
    uint32_t direction = 0;
    uint32_t directionAccuracy = 0;
    int32_t  altitude;
    int32_t  vAccuracy;
    uint32_t hSpeed;
    uint32_t hSpeedAccuracy;
    int32_t  vSpeed = 0;
    int32_t  vSpeedAccuracy = 0;
    uint8_t  leapSeconds = 0;
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hours;
    uint16_t minutes;
    uint16_t seconds;
    uint16_t milliseconds;
    int i;
    uint16_t satelliteIdPtr[TAF_GNSS_SV_INFO_MAX_LEN];
    size_t satelliteIdNumElements = NUM_ARRAY_MEMBERS(satelliteIdPtr);
    taf_gnss_Constellation_t satelliteConstPtr[TAF_GNSS_SV_INFO_MAX_LEN];
    size_t satelliteConstNumElements = NUM_ARRAY_MEMBERS(satelliteConstPtr);
    bool satelliteUsedPtr[TAF_GNSS_SV_INFO_MAX_LEN];
    size_t satelliteUsedNumElements = NUM_ARRAY_MEMBERS(satelliteUsedPtr);
    uint8_t satelliteSnrPtr[TAF_GNSS_SV_INFO_MAX_LEN];
    size_t satelliteSnrNumElements = NUM_ARRAY_MEMBERS(satelliteSnrPtr);
    uint16_t satelliteAzimPtr[TAF_GNSS_SV_INFO_MAX_LEN];
    size_t satelliteAzimNumElements = NUM_ARRAY_MEMBERS(satelliteAzimPtr);
    uint8_t satelliteElevPtr[TAF_GNSS_SV_INFO_MAX_LEN];
    size_t satelliteElevNumElements = NUM_ARRAY_MEMBERS(satelliteElevPtr);
    uint32_t TimeAccuracy = 0;
    uint64_t EpochTime = 0;
    uint32_t gpsWeek;
    uint32_t gpsTimeOfWeek;
    taf_gnss_Resolution_t DopRes;
    taf_gnss_DopType_t dopType = TAF_GNSS_PDOP;
    uint16_t dop[TAF_GNSS_RES_UNKNOWN];
    uint8_t satsInViewCount;
    uint8_t satsTrackingCount;
    uint8_t satsUsedCount;
    int32_t  magneticDeviation;
    uint32_t horUncEllipseSemiMajor;
    uint32_t horUncEllipseSemiMinor;
    uint8_t  horConfidence;
    double indexPtr;
    uint8_t percentPtr;
    uint32_t calibPtr;
    double vrpLatitude;
    double vrpLongitude;
    double vrpAltitude;
    double eastVel;
    double northVel;
    double upVel;
    uint32_t sbasMask = 0;
    uint32_t validityMask = 0;
    uint64_t validityExMask = 0;
    uint16_t engMask = 0;
    uint16_t locationEngType = 0;
    uint16_t horiReliablity = 0;
    uint16_t vertReliablity = 0;
    double azimuth;
    double eastDev;
    double northDev;
    uint64_t realTime;
    uint64_t realTimeUnc;
    uint32_t techMask;
    static const char *tabDop[] =
    {
        "Position dilution of precision (PDOP)",
        "Horizontal dilution of precision (HDOP)",
        "Vertical dilution of precision (VDOP)",
        "Geometric dilution of precision (GDOP)",
        "Time dilution of precision (TDOP)"
    };
    taf_gnss_KinematicsData_t *bodyFrameData;
    le_mem_PoolRef_t bodyFramePool = NULL;
    bodyFramePool = le_mem_CreatePool("bodyFramePool", sizeof(taf_gnss_KinematicsData_t));
    bodyFrameData = (taf_gnss_KinematicsData_t*) le_mem_ForceAlloc(bodyFramePool);

    taf_gnss_SvUsedInPosition_t *svData;
    le_mem_PoolRef_t svFramePool = NULL;
    svFramePool = le_mem_CreatePool("svFramePool", sizeof(taf_gnss_SvUsedInPosition_t));
    svData = (taf_gnss_SvUsedInPosition_t*) le_mem_ForceAlloc(svFramePool);

    //138.GetPositionState
    LE_TEST_INFO("taf_gnss_GetPositionState() API is triggerred to get position state");
    result = taf_gnss_GetPositionState(positionSampleRef, &state);
    LE_TEST_OK(result == LE_OK,"taf_gnss_GetPositionState-LE_OK");
    if(state == TAF_GNSS_STATE_FIX_NO_POS)
    {
        LE_TEST_INFO("No fix Positionmhence release the sample reference & return");
        taf_gnss_ReleaseSampleRef(positionSampleRef);
        return;
    }
    if(result == LE_OK)
    {
        LE_TEST_INFO("Position state: %s", (TAF_GNSS_STATE_FIX_2D == state)?"2D Fix"
                                     :(TAF_GNSS_STATE_FIX_3D == state)?"3D Fix"
                                     : "Unknown");
    }
    else
    {
        LE_TEST_INFO("Failed to get position state");
    }

    //139.Get Location
    LE_TEST_INFO("taf_gnss_GetLocation() API is triggerred to get 2d-Location information");
    result = taf_gnss_GetLocation(positionSampleRef, &latitude, &longitude, &hAccuracy);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)),"taf_gnss_GetLocation -LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Latitude(positive->north) : %.6f\n",(float)latitude/1e6);
        LE_TEST_INFO("Longitude(positive->east) : %.6f\n",(float)longitude/1e6);
        LE_TEST_INFO("hAccuracy                 : %.2fm\n",(float)hAccuracy/1e2);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("Location invalid [%d, %d, %d]\n",
               latitude,
               longitude,
               hAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed! to get Location information\n");
    }

    //140.Get Direction
    LE_TEST_INFO("taf_gnss_GetDirection() API is triggerred to get Direction information");
    result = taf_gnss_GetDirection(positionSampleRef, &direction,&directionAccuracy);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)),"taf_gnss_GetDirection-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Direction(0 degree is True North) : %.1f degrees\n",(float)direction/10.0);
        LE_TEST_INFO("Accuracy                  : %.1f degrees\n",(float)directionAccuracy/10.0);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("Direction invalid [%u, %u]\n",direction,directionAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed ! to get Direction information");
    }

    //141.Get Altitude
    LE_TEST_INFO("taf_gnss_GetAltitude() API is triggerred to get Altitude information");
    result = taf_gnss_GetAltitude(positionSampleRef, &altitude, &vAccuracy);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)), "taf_gnss_GetAltitude - LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Altitude  : %.3fm\n",(float)altitude/1e3);
        LE_TEST_INFO("vAccuracy : %.1fm\n",(float)vAccuracy/10.0);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("Altitude invalid [%d, %d]\n",altitude,vAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed! to get Altitude\n");
    }

    //142.Get Horizontal Speed
    LE_TEST_INFO("taf_gnss_GetHorizontalSpeed() API is triggerred to get Horizontal Speed");
    result = taf_gnss_GetHorizontalSpeed( positionSampleRef, &hSpeed, &hSpeedAccuracy);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)),
        "taf_gnss_GetHorizontalSpeed - LE_OK");
    if (result == LE_OK)
    {
        LE_TEST_INFO("hSpeed %.2fm/s\n",hSpeed/100.0);
        LE_TEST_INFO("Accuracy %.1fm/s\n",hSpeedAccuracy/10.0);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("hSpeed invalid [%u, %u]\n",hSpeed,hSpeedAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed! to get Horizontal Speed\n");
    }

    //143.Get Vertical Speed
    LE_TEST_INFO("taf_gnss_GetVerticalSpeed() API is triggerred to get Vertical Speed");
    result = taf_gnss_GetVerticalSpeed( positionSampleRef, &vSpeed, &vSpeedAccuracy);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)),
        "taf_gnss_GetVerticalSpeed - LE_OK");
    if (result == LE_OK)
    {
        LE_TEST_INFO( "vSpeed %.2fm/s\n",vSpeed/100.0);
        LE_TEST_INFO("Accuracy %.1fm/s\n",vSpeedAccuracy/10.0);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("vSpeed invalid [%d, %d]\n",vSpeed,vSpeedAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed! to get Vertifical Speed\n");
    }

    //144. Get GPS Leap Seconds
    LE_TEST_INFO("taf_gnss_GetGpsLeapSeconds() API is triggerred to get Gps Leap Seconds");
    result = taf_gnss_GetGpsLeapSeconds(positionSampleRef, &leapSeconds);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)),
        "taf_gnss_GetGpsLeapSeconds -LE_OK");
    if ((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("GPS Leap Seconds: %d secs \n", leapSeconds);
    }
    else
    {
        LE_TEST_INFO("Failed! to get GPS Leap Seconds\n");
    }

    //145. Get Date
    LE_TEST_INFO("taf_gnss_GetDate() API is triggerred to get Date Information");
    result = taf_gnss_GetDate(positionSampleRef, &year, &month, &day);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)), "taf_gnss_GetDate -LE_OK");
    if (result == LE_OK)
    {
        LE_TEST_INFO("Date(YYYY-MM-DD) %04d-%02d-%02d\n",year,month,day);
    }

    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("Date invalid %04d-%02d-%02d\n",year,month,day);
    }
    else
    {
        LE_TEST_INFO("Failed! to get Date information\n");
    }

    //146. Get Time
    LE_TEST_INFO("taf_gnss_GetTime() API is triggerred to get Time Information");
    result = taf_gnss_GetTime(positionSampleRef, &hours, &minutes, &seconds, &milliseconds);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)), "taf_gnss_GetTime -LE_OK");
    if (result == LE_OK)
    {
        LE_TEST_INFO("Time(HH:MM:SS:MS) %02d:%02d:%02d:%03d\n",hours,minutes,seconds,milliseconds);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("Time invalid %02d:%02d:%02d.%03d\n",hours,minutes,seconds,milliseconds);
    }
    else
    {
        LE_TEST_INFO("Failed! to get Time information");
    }

    //147. Satellite Information
    LE_TEST_INFO("taf_gnss_GetSatellitesInfo() API is triggerred to get Satellite Information");
    result =  taf_gnss_GetSatellitesInfo(positionSampleRef,
                                        satelliteIdPtr,
                                        &satelliteIdNumElements,
                                        satelliteConstPtr,
                                        &satelliteConstNumElements,
                                        satelliteUsedPtr,
                                        &satelliteUsedNumElements,
                                        satelliteSnrPtr,
                                        &satelliteSnrNumElements,
                                        satelliteAzimPtr,
                                        &satelliteAzimNumElements,
                                        satelliteElevPtr,
                                        &satelliteElevNumElements);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)),
        "taf_gnss_GetSatellitesInfo -LE_OK");
    if((result == LE_OK)||(result == LE_OUT_OF_RANGE))
    {
        for(i=0; i<satelliteIdNumElements; i++)
        {
            if((satelliteIdPtr[i] != 0)&&(satelliteIdPtr[i] != UINT8_MAX))
            {
                LE_TEST_INFO("[%02d] SVid: %03d C%01d - U%d - SNR%02d - Azim%03d - Elev%02d\n"
                        , i
                        , satelliteIdPtr[i]
                        , satelliteConstPtr[i]
                        , satelliteUsedPtr[i]
                        , satelliteSnrPtr[i]
                        , satelliteAzimPtr[i]
                        , satelliteElevPtr[i]);
            }
        }
    }
    else
    {
        LE_TEST_INFO("Failed! to get Satellite Information\n");
    }

    int index = 0;
    for (int constellation = 1; constellation < TAF_GNSS_SV_CONSTELLATION_MAX; constellation++) {

      taf_gnss_SvInfo_t svInfo[TAF_GNSS_SV_INFO_MAX_SATS_IN_CONSTELLATIONS];
      size_t svInfoLen = TAF_GNSS_SV_INFO_MAX_SATS_IN_CONSTELLATIONS;

      result = taf_gnss_GetSatellitesInfoEx(positionSampleRef, constellation, svInfo, &svInfoLen);

      if((result == LE_OK)||(result == LE_OUT_OF_RANGE)||(result == LE_OVERFLOW))
      {
        LE_INFO("gnss unit test: result %d, constellation: %d, numOfSvInfo: %d", (int)result, (int) constellation, (int) svInfoLen);

        LE_TEST_OK(result == LE_OK || result == LE_OUT_OF_RANGE || result == LE_OVERFLOW, "taf_gnss_GetSatellitesInfoEx-Success");

        for(i=0; i<(int) svInfoLen; i++)
        {
            if((svInfo[i].satId != 0)&&(svInfo[i].satId != UINT8_MAX))
            {
                LE_TEST_INFO("[%02d] SVid %03d - C%01d - U%d - T%d - SNR%02d - Azim%03d - Elev%02d\n"
                        , index++
                        , svInfo[i].satId
                        , svInfo[i].satConst
                        , svInfo[i].satUsed
                        , svInfo[i].satTracked
                        , svInfo[i].satSnr
                        , svInfo[i].satAzim
                        , svInfo[i].satElev);

                PrintGnssSignalType(svInfo[i].signalType);
                LE_TEST_INFO("\n");
                LE_TEST_INFO("Glonass FCN: %d\n", svInfo[i].glonassFcn);
                LE_TEST_INFO("Baseband Carrier To Noise Ratio: %lfdB-Hz\n", svInfo[i].baseBandCnr);
            }
        }
      }
      else
      {
        LE_TEST_INFO("taf_gnss_GetSatellitesInfoEx is failed for constellation: %d\n", (int)constellation);
      }
    }

    //148.Get Time Accuracy
    LE_TEST_INFO("taf_gnss_GetTimeAccuracy() API is triggerred to get Time Accuracy");
    result = taf_gnss_GetTimeAccuracy(positionSampleRef, &TimeAccuracy);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)),
        "taf_gnss_GetTimeAccuracy -LE_OK");
    if (result == LE_OK)
    {
        LE_TEST_INFO("GPS time accuracy %dns\n", TimeAccuracy);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("GPS time accuracy invalid [%d]\n", TimeAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed! to get time accuracy\n");
    }

    //149.Get Epoch Time
    LE_TEST_INFO("taf_gnss_GetEpochTime() API is triggerred to get Epoch Time");
    positionSampleRef = taf_gnss_GetLastSampleRef();
    LE_TEST_INFO("taf_gnss_GetLastSampleRef() API is triggerred to get last sample reference");
    result = taf_gnss_GetEpochTime(positionSampleRef, &EpochTime);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)), "taf_gnss_GetEpochTime -LE_OK");
    if (LE_OK == result)
    {
        LE_TEST_INFO("Epoch Time %llu ms\n", (unsigned long long int) EpochTime);
    }
    else if (LE_OUT_OF_RANGE == result)
    {
        LE_TEST_INFO("Time invalid %llu ms\n", (unsigned long long int) EpochTime);
    }
    else
    {
        LE_TEST_INFO("Failed! to get epoch time\n");
    }

    //150.Set DOP Resolution -LE_BAD_PARAMETER
    LE_TEST_INFO("SetDopResolution() API is triggerred to set DOP resolution");
    result = taf_gnss_SetDopResolution(TAF_GNSS_RES_UNKNOWN);
    LE_TEST_OK(result==LE_BAD_PARAMETER,"taf_gnss_SetDopResolution-LE_BAD_PARAMETER");

    //151. Set DOP & Get Dilution of Precision (150-189)
    LE_TEST_INFO("GetDopResolution() API is triggerred to get DOP resolution");
    do
    {
        // Get DOP parameter in all resolutions
        for (DopRes=TAF_GNSS_RES_ZERO_DECIMAL; DopRes<TAF_GNSS_RES_UNKNOWN; DopRes++)
        {
            result = taf_gnss_SetDopResolution(DopRes);

           //Set DOP
            LE_TEST_OK(result==LE_OK,"taf_gnss_SetDopResolution-LE_OK");

            if (LE_OK != result)
            {
                LE_TEST_INFO("Failed! to set DOP resolution\n");
                break;
            }

            result = taf_gnss_GetDilutionOfPrecision(positionSampleRef,
                                                    dopType,
                                                    &dop[DopRes]);
            //Get DOP
            LE_TEST_OK(result==LE_OK,"taf_gnss_GetDilutionOfPrecision-LE_OK");

            if (LE_OUT_OF_RANGE == result)
            {
                LE_TEST_INFO("%s invalid %d\n", tabDop[dopType], dop[0]);
                break;
            }
            else if (LE_OK != result)
            {
                LE_TEST_INFO("Failed! to get DOP\n");
                break;
            }
        }
        if (LE_OK == result)
        {
            LE_TEST_INFO("%s [%.1f %.1f %.2f %.3f]\n", tabDop[dopType],
                   (float)dop[TAF_GNSS_RES_ZERO_DECIMAL],
                   (float)dop[TAF_GNSS_RES_ONE_DECIMAL]/10,
                   (float)dop[TAF_GNSS_RES_TWO_DECIMAL]/100,
                   (float)dop[TAF_GNSS_RES_THREE_DECIMAL]/1000);
        }
        dopType++;
    }
    while (dopType != TAF_GNSS_DOP_LAST);

    //152.Get GPS time
    result = taf_gnss_GetGpsTime(positionSampleRef, &gpsWeek, &gpsTimeOfWeek);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)), "taf_gnss_GetGpsTime-LE_OK");
    if (result == LE_OK)
    {
        LE_TEST_INFO("GPS time, Week %02d:TimeOfWeek %d ms\n",gpsWeek,gpsTimeOfWeek);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("GPS time invalid [%d, %d]\n",gpsWeek,gpsTimeOfWeek);
    }
    else
    {
        LE_TEST_INFO("Failed! to get GPS time\n");
    }

    //153.Get Satellite Status
    LE_TEST_INFO("taf_gnss_GetSatellitesStatus is triggered to get satellite status\n");
    result =  taf_gnss_GetSatellitesStatus(positionSampleRef,
                                          &satsInViewCount,
                                          &satsTrackingCount,
                                          &satsUsedCount);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)),
        "taf_gnss_GetSatellitesStatus-LE_OK");

    if ((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("satsInView %d - satsTracking %d - satsUsed %d\n",
               (satsInViewCount == UINT8_MAX) ? 0: satsInViewCount,
               (satsTrackingCount == UINT8_MAX) ? 0: satsTrackingCount,
               (satsUsedCount == UINT8_MAX) ? 0: satsUsedCount);
    }
    else
    {
        LE_TEST_INFO("Failed! to get satellite status \n");
    }

    //154. Get the Magnetic deviation
    LE_TEST_INFO("taf_gnss_GetMagneticDeviation() is triggered to get the magnetic deviation\n");
    result = taf_gnss_GetMagneticDeviation( positionSampleRef, &magneticDeviation);
    LE_TEST_OK((result == LE_OK), "taf_gnss_GetMagneticDeviation-LE_OK");
    if (LE_OK == result)
    {
        LE_TEST_INFO("magnetic deviation: %.1f degrees", (float)(magneticDeviation/10.0));
    }
    else
    {
        LE_TEST_INFO("magnetic deviation unknown [%d]",magneticDeviation);
    }

    //155. Get elliptical uncertainity
    LE_TEST_INFO("taf_gnss_GetEllipticalUncertainty() is triggered to get"
        "elliptical uncertiainity information \n");
    result = taf_gnss_GetEllipticalUncertainty(positionSampleRef,&horUncEllipseSemiMajor,
        &horUncEllipseSemiMinor,&horConfidence);
    LE_TEST_OK((result == LE_OK),"taf_gnss_GetEllipticalUncertainty -LE_OK");
    if (result == LE_OK)
    {
        LE_TEST_INFO("HorizontalUncertainty SemiMajor: %.2fm/s\n",(float)horUncEllipseSemiMajor);
        LE_TEST_INFO("HorizontalUncertainty SemiMinor: %.2fm/s\n",(float)horUncEllipseSemiMinor);
        LE_TEST_INFO("Horizontal Confidence level: %d%%\n",horConfidence);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("HorizontalUncertainty invalid [%u, %u %u]\n",
                horUncEllipseSemiMajor,
                horUncEllipseSemiMinor,
                horConfidence);
    }
    else
    {
        LE_TEST_INFO("Failed! to get elliptical uncertainity information\n");
    }

    //GetPositionState
    LE_TEST_INFO("taf_gnss_GetPositionState() API is triggerred to get position state");
    result = taf_gnss_GetPositionState(positionSampleRef, &state);
    LE_TEST_OK(result == LE_OK,"taf_gnss_GetPositionState-LE_OK");
    if(state == TAF_GNSS_STATE_FIX_NO_POS)
    {
        LE_TEST_INFO("No fix Position hence release the sample reference & return");
        taf_gnss_ReleaseSampleRef(positionSampleRef);
        return;
    }
    if(result == LE_OK)
    {
        LE_TEST_INFO("Position state: %s", (TAF_GNSS_STATE_FIX_2D == state)?"2D Fix"
                                     :(TAF_GNSS_STATE_FIX_3D == state)?"3D Fix"
                                     : "Unknown");
    }
    else
    {
        LE_TEST_INFO("Failed to get position state");
    }

    //GetConformityIndex
    LE_TEST_INFO("taf_gnss_GetConformityIndex() API to get conformity index");
    result = taf_gnss_GetConformityIndex(positionSampleRef, &indexPtr);
    LE_TEST_OK(result == LE_OK,"taf_gnss_GetConformityIndex-LE_OK");
    if (result == LE_OK)
    {
        LE_TEST_INFO("ComformingIndex: %.2f\n" "(0.0->Least conforming 1.0->Most conforming)\n"
                  ,(float)indexPtr);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("GetConformityIndex is invalid\n");
    }
    else
    {
        LE_TEST_INFO("Failed! See log for details!\n");
    }

    //GetCalibrationData
    LE_TEST_INFO("taf_gnss_GetCalibrationData() API is to get confidence percent");
    result = taf_gnss_GetCalibrationData(positionSampleRef,&calibPtr,&percentPtr);
    LE_TEST_OK(result == LE_OK,"taf_gnss_GetCalibrationData-LE_OK");
    if (result == LE_OK)
    {
        if(calibPtr & (1<<TAF_GNSS_DR_ROLL_CALIBRATION_NEEDED))
        {
            LE_TEST_INFO("Roll calibration is needed");
        }
        if(calibPtr & (1<<TAF_GNSS_DR_PITCH_CALIBRATION_NEEDED))
        {
            LE_TEST_INFO("Pitch calibration is needed");
        }
        if(calibPtr & (1<<TAF_GNSS_DR_YAW_CALIBRATION_NEEDED))
        {
            LE_TEST_INFO("Yaw calibration is needed");
        }
        if(calibPtr & (1<<TAF_GNSS_DR_ODO_CALIBRATION_NEEDED))
        {
            LE_TEST_INFO("Odo calibration is needed");
        }
        if(calibPtr & (1<<TAF_GNSS_DR_GYRO_CALIBRATION_NEEDED))
        {
            LE_TEST_INFO("Gyro calibration is needed");
        }
        if(calibPtr == 0)
        {
            LE_TEST_INFO("calibration status not found");
        }
        LE_TEST_INFO("Sensro calibration confidence percent: %u%%\n"
                  ,percentPtr);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("Confidence data is invalid\n");
    }
    else
    {
        LE_TEST_INFO("Failed! See log for details!\n");
    }

    //GetBodyFrameData
    LE_TEST_INFO("taf_gnss_GetBodyFrameData() is called to get body frame data");
    result = taf_gnss_GetBodyFrameData( positionSampleRef,
                                              bodyFrameData);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetBodyFrameData-LE_OK");
    if (result == LE_OK)
    {
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_LONG_ACCEL))
       {
           LE_TEST_INFO("Kinematics data has Forward Accelaration");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_LAT_ACCEL))
       {
           LE_TEST_INFO("Kinematics data has has Sideward Acceleration");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_VERT_ACCEL))
       {
           LE_TEST_INFO("Kinematics data has Vertical Acceleration");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_YAW_RATE))
       {
           LE_TEST_INFO("Kinematics has data has Heading Rate");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_PITCH))
       {
           LE_TEST_INFO("Kinematics has body pitch");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_LONG_ACCEL_UNC))
       {
           LE_TEST_INFO("Kinematics data has has Forward Acceleration Uncertainty");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_LAT_ACCEL_UNC))
       {
           LE_TEST_INFO("Kinematics data has has Sideward Acceleration Uncertainty");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_VERT_ACCEL_UNC))
       {
           LE_TEST_INFO("Kinematics data has Vertical Acceleration Uncertainty");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_YAW_RATE_UNC))
       {
           LE_TEST_INFO("Kinematics data Heading rate uncertainity");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_PITCH_UNC))
       {
           LE_TEST_INFO("Kinematics has body pitch Uncertainity");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_PITCH_RATE_BIT))
       {
           LE_TEST_INFO("Kinematics data has pitch rate");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_PITCH_RATE_UNC_BIT))
       {
           LE_TEST_INFO("Kinematics has pitch rate Uncertainity");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_ROLL_BIT))
       {
           LE_TEST_INFO("Kinematics data has roll");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_ROLL_UNC_BIT))
       {
           LE_TEST_INFO("Kinematics data has roll Uncertainity");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_ROLL_RATE_BIT))
       {
           LE_TEST_INFO("Kinematics data has roll rate");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_ROLL_RATE_UNC_BIT))
       {
           LE_TEST_INFO("Kinematics data has roll rate Uncertainity");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_YAW_BIT))
       {
           LE_TEST_INFO("Kinematics data has yaw");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_GNSS_HAS_YAW_UNC_BIT))
       {
           LE_TEST_INFO("Kinematics data has yaw uncertainity");
       }
       LE_TEST_INFO("\nForward Acceleration in body frame(m/s2):%lf",
                                     (float)bodyFrameData->longAccel);
       LE_TEST_INFO("Sideward Acceleration in body frame (m/s2):%lf",
                                     (float) bodyFrameData->latAccel);
       LE_TEST_INFO("Vertical Acceleration in body frame (m/s2):%lf",
                                     (float) bodyFrameData->vertAccel);
       LE_TEST_INFO("Heading Rate (Radians/second):%lf",(float) bodyFrameData->yawRate);
       LE_TEST_INFO("Body pitch (Radians)::%lf",(float) bodyFrameData->pitch);
       LE_TEST_INFO("Uncertainty of Forward Acceleration in body frame:%lf",
                                     (float) bodyFrameData->longAccelUnc);
       LE_TEST_INFO("Uncertainty of Side-ward Acceleration in body frame:%lf",
                                     (float) bodyFrameData->latAccelUnc);
       LE_TEST_INFO("Uncertainty of Vertical Acceleration in body frame:%lf",
                                     (float)bodyFrameData->vertAccelUnc);
       LE_TEST_INFO("Uncertainty of Heading Rate:%lf",(float) bodyFrameData->yawRateUnc);
       LE_TEST_INFO("Uncertainty of Body pitch:%lf",(float) bodyFrameData->pitchUnc);
       LE_TEST_INFO("Body pitch rate:%lf",(float) bodyFrameData->pitchRate);
       LE_TEST_INFO("Uncertainty of pitch rate:%lf",(float) bodyFrameData->pitchRateUnc);
       LE_TEST_INFO("Roll of body frame, clockwise is positive:%lf",(float)bodyFrameData->roll);
       LE_TEST_INFO("Uncertainty of roll, 68 per confidence level:%lf",
                                          (float)bodyFrameData->rollUnc);
       LE_TEST_INFO("Roll rate of body frame, clockwise is positive:%lf",
                                          (float)bodyFrameData->rollRate);
       LE_TEST_INFO("Uncertainty of roll rate, 68 per confidence level:%lf",
                                          (float)bodyFrameData->rollRateUnc);
       LE_TEST_INFO("Yaw of body frame, clockwise is positive:%lf",(float)bodyFrameData->yaw);
       LE_TEST_INFO("Uncertainty of yaw, 68 per confidence level:%lf",(float)bodyFrameData->yawUnc);

    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("Kinematics data is not found");
    }
    else
    {
        LE_TEST_INFO("Failed! See log for details\n");
    }

    //release bodyFrameData referene
    le_mem_Release(bodyFrameData);

    //Get VRP based latitude, longitude & altitude information
    LE_TEST_INFO("taf_gnss_GetVRPBasedLLA() is called to get VRP based information");
    result = taf_gnss_GetVRPBasedLLA(positionSampleRef,
                                              &vrpLatitude,&vrpLongitude,&vrpAltitude);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetVRPBasedLLA-LE_OK");
    if (result == LE_OK)
    {
        LE_TEST_INFO("VRP based Latitude(positive->north) : %lf degrees\n"
               "VRP based Longitude(positive->east) : %lf degrees\n"
               "VRP based altitude                 : %lfm\n",
                vrpLatitude,
                vrpLongitude,
                (float)vrpAltitude);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("VRP bsed Location is invalid [%lf, %lf, %lf]\n",
               vrpLatitude,
               vrpLongitude,
               vrpAltitude);
    }
    else
    {
        LE_TEST_INFO("Failed! See log for details\n");
    }

    //Get VRP based east, north & up velocity information
    LE_TEST_INFO("taf_gnss_GetVRPBasedVelocity() is called to get VRP based information");
    result = taf_gnss_GetVRPBasedVelocity(positionSampleRef,
                                              &eastVel,&northVel,&upVel);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetVRPBasedVelocity-LE_OK");
    if (result == LE_OK)
    {
        LE_TEST_INFO("VRP based east velocity  : %lf\n"
               "VRP based north velocity : %lf\n"
               "VRP based up velocity    : %lf\n",
                (float)eastVel,
                (float)northVel,
                (float)upVel);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("GetVRPBasedVelocity invalid [%lf, %lf, %lf]\n",
               eastVel,
               northVel,
               upVel);
    }
    else
    {
        LE_TEST_INFO("Failed! See log for details\n");
    }

    //Get set of satellite vehicles that are used to calculate position
    LE_TEST_INFO("taf_gnss_GetSvUsedInPosition() is called to get SVs");
    result = taf_gnss_GetSvUsedInPosition(positionSampleRef,svData);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetSvUsedInPosition-LE_OK");
    if (result == LE_OK)
    {
        LE_TEST_INFO("SVs from GPS constellation  : %"PRIu64"\n",svData->gps);
        LE_TEST_INFO("SVs from GLONASS constellation  : %"PRIu64"\n",svData->glo);
        LE_TEST_INFO("SVs from GALILEO constellation   : %"PRIu64"\n",svData->gal);
        LE_TEST_INFO("SVs from BEIDOU constellation  : %"PRIu64"\n",svData->bds);
        LE_TEST_INFO("SVs from QZSS constellation  : %"PRIu64"\n",svData->qzss);
        LE_TEST_INFO("SVs from NAVIC constellation  : %"PRIu64"\n",svData->navic);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("GetSvData is invalid\n");
    }
    else
    {
        LE_TEST_INFO("Failed! See log for details\n");
    }
    //release Svdata reference
    le_mem_Release(svData);

    //Get navigation solution mask used to indicate SBAS corrections.
    LE_TEST_INFO("taf_gnss_GetSbasCorrection() is called");
    result = taf_gnss_GetSbasCorrection(positionSampleRef,&sbasMask);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetSbasCorrection-LE_OK");
    if (result == LE_OK)
    {
        if(sbasMask & TAF_GNSS_SBAS_CORRECTION_IONO)
        {
            LE_TEST_INFO("SBAS ionospheric correction is used\n");
        }
        if(sbasMask & TAF_GNSS_SBAS_CORRECTION_FAST)
        {
            LE_TEST_INFO("SBAS fast correction is used\n");
        }
        if(sbasMask & TAF_GNSS_SBAS_CORRECTION_LONG)
        {
            LE_TEST_INFO("SBAS long correction is used\n");
        }
        if(sbasMask & TAF_GNSS_SBAS_INTEGRITY)
        {
            LE_TEST_INFO("SBAS integrity information is used\n");
        }
        if(sbasMask & TAF_GNSS_SBAS_CORRECTION_DGNSS)
        {
            LE_TEST_INFO("SBAS DGNSS correction information is used\n");
        }
        if(sbasMask & TAF_GNSS_SBAS_CORRECTION_RTK)
        {
            LE_TEST_INFO("SBAS RTK correction information is used\n");
        }
        if(sbasMask & TAF_GNSS_SBAS_CORRECTION_PPP)
        {
            LE_TEST_INFO("SBAS PPP correction information is used\n");
        }
        if(sbasMask & TAF_GNSS_SBAS_CORRECTION_RTK_FIXED)
        {
            LE_TEST_INFO("SBAS RTK fixed correction information is used\n");
        }
        if(sbasMask & TAF_GNSS_SBAS_CORRECTED_SV_USED)
        {
            LE_TEST_INFO("SBAS correction SV is used\n");
        }
        if(sbasMask == 0)
        {
            LE_TEST_INFO("no SBAS corrections data\n");
        }
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("GetSbasType is invalid\n");
    }
    else
    {
        LE_TEST_INFO("Failed! See log for details\n");
    }

    //Get position technology mask used to indicate which technology is used.
    LE_TEST_INFO("taf_gnss_GetPositionTechnology() is called");
    result = taf_gnss_GetPositionTechnology(positionSampleRef,&techMask);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetPositionTechnology-LE_OK");
    if (result == LE_OK)
    {
        if(techMask &TAF_GNSS_LOC_GNSS)
        {
            LE_TEST_INFO("location calculated using GNSS\n");
        }
        if(techMask &TAF_GNSS_LOC_CELL)
        {
            LE_TEST_INFO("location calculated using CELL\n");
        }
        if(techMask &TAF_GNSS_LOC_WIFI)
        {
            LE_TEST_INFO("location calculated using WIFI\n");
        }
        if(techMask &TAF_GNSS_LOC_SENSORS)
        {
            LE_TEST_INFO("location calculated using SENSORS\n");
        }
        if(techMask &TAF_GNSS_LOC_REFERENCE_LOCATION)
        {
            LE_TEST_INFO("location calculated using reference location\n");
        }
        if(techMask &TAF_GNSS_LOC_INJECTED_COARSE_POSITION)
        {
            LE_TEST_INFO("location calculated using Coarse position injected\n");
        }
        if(techMask &TAF_GNSS_LOC_AFLT)
        {
            LE_TEST_INFO("location calculated using AFLT\n");
        }
        if(techMask &TAF_GNSS_LOC_HYBRID)
        {
            LE_TEST_INFO("location calculated using GNSS and network-provided measurements\n");
        }
        if(techMask &TAF_GNSS_LOC_PPE)
        {
            LE_TEST_INFO("location calculated using Precise position engine\n");
        }
        if(techMask &TAF_GNSS_LOC_VEH)
        {
            LE_TEST_INFO("location calculated using Vehicular data\n");
        }
        if(techMask &TAF_GNSS_LOC_VIS)
        {
            LE_TEST_INFO("location calculated using Visual data\n");
        }
        if(techMask &TAF_GNSS_LOC_PROPAGATED)
        {
            LE_TEST_INFO("location calculated using propagation logic\n");
        }
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("GetTechnologyInformation is invalid\n");
    }
    else
    {
        LE_TEST_INFO("Failed! See log for details\n");
    }

    //Get the validity of the Location basic Info.
    LE_TEST_INFO("taf_gnss_GetLocationInfoValidity() is called");
    result = taf_gnss_GetLocationInfoValidity(positionSampleRef,&validityMask,&validityExMask);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetLocationInfoValidity-LE_OK");
    if (result == LE_OK)
    {
        LE_TEST_INFO("** Location Info Validity Information ***\n");
        if(validityMask & TAF_GNSS_HAS_LAT_LONG_BIT)
        {
            LE_TEST_INFO("valid latitude longitude\n");
        }
        if(validityMask & TAF_GNSS_HAS_ALTITUDE_BIT)
        {
            LE_TEST_INFO("valid altitude\n");
        }
        if(validityMask & TAF_GNSS_HAS_SPEED_BIT)
        {
            LE_TEST_INFO("valid speed\n");
        }
        if(validityMask & TAF_GNSS_HAS_HEADING_BIT)
        {
            LE_TEST_INFO("valid heading\n");
        }
        if(validityMask & TAF_GNSS_HAS_HORIZONTAL_ACCURACY_BIT)
        {
            LE_TEST_INFO("valid horizontal accuracy\n");
        }
        if(validityMask & TAF_GNSS_HAS_VERTICAL_ACCURACY_BIT)
        {
            LE_TEST_INFO("valid vertical accuracy\n");
        }
        if(validityMask & TAF_GNSS_HAS_SPEED_ACCURACY_BIT)
        {
            LE_TEST_INFO("valid speed accuracy \n");
        }
        if(validityMask & TAF_GNSS_HAS_HEADING_ACCURACY_BIT)
        {
            LE_TEST_INFO("valid heading accuracy\n");
        }
        if(validityMask & TAF_GNSS_HAS_TIMESTAMP_BIT)
        {
            LE_TEST_INFO("valid timestamp\n");
        }
        if(validityMask & TAF_GNSS_HAS_ELAPSED_REAL_TIME_BIT)
        {
            LE_TEST_INFO("valid elapsed real time\n");
        }
        if(validityMask & TAF_GNSS_HAS_ELAPSED_REAL_TIME_UNC_BIT)
        {
            LE_TEST_INFO("valid elapsed real time Uncertainity\n");
        }
        if(validityMask == 0)
        {
            LE_TEST_INFO("no Valid Mask\n");
        }

        LE_TEST_INFO("\n** Location ex Info Validity Information ***\n");

        if(validityExMask & (1ULL << TAF_GNSS_HAS_ALTITUDE_MEAN_SEA_LEVEL))
        {
            LE_TEST_INFO("valid altitude mean sea level\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_DOP))
        {
            LE_TEST_INFO("valid pdop, hdop, vdop\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_MAGNETIC_DEVIATION))
        {
            LE_TEST_INFO("valid magnetic deviation\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_HOR_RELIABILITY))
        {
            LE_TEST_INFO("valid horizontal reliability\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_VER_RELIABILITY))
        {
            LE_TEST_INFO("valid vertical reliability\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_HOR_ACCURACY_ELIP_SEMI_MAJOR))
        {
            LE_TEST_INFO("valid elipsode semi major\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_HOR_ACCURACY_ELIP_SEMI_MINOR))
        {
            LE_TEST_INFO("valid elipsode semi minor\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_HOR_ACCURACY_ELIP_AZIMUTH))
        {
            LE_TEST_INFO("valid accuracy elipsode azimuth\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_GNSS_SV_USED_DATA))
        {
            LE_TEST_INFO("valid gnss sv used in pos data\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_NAV_SOLUTION_MASK))
        {
            LE_TEST_INFO("valid navSolutionMask\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_POS_TECH_MASK))
        {
            LE_TEST_INFO("valid LocPosTechMask\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_SV_SOURCE_INFO))
        {
            LE_TEST_INFO("valid LocSvInfoSource\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_POS_DYNAMICS_DATA))
        {
            LE_TEST_INFO("valid position dynamics data\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_EXT_DOP))
        {
            LE_TEST_INFO("valid gdop, tdop\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_NORTH_STD_DEV))
        {
            LE_TEST_INFO("valid North standard deviation\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_EAST_STD_DEV))
        {
            LE_TEST_INFO("valid East standard deviation\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_NORTH_VEL))
        {
            LE_TEST_INFO("valid North Velocity\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_EAST_VEL))
        {
            LE_TEST_INFO("valid East Velocity""\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_UP_VEL))
        {
            LE_TEST_INFO("valid Up Velocity\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_NORTH_VEL_UNC))
        {
            LE_TEST_INFO("valid North Velocity Uncertainty\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_EAST_VEL_UNC))
        {
            LE_TEST_INFO("valid East Velocity Uncertainty\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_UP_VEL_UNC))
        {
            LE_TEST_INFO("valid Up Velocity Uncertainty\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_LEAP_SECONDS))
        {
            LE_TEST_INFO("valid leap_seconds\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_TIME_UNC))
        {
            LE_TEST_INFO("valid timeUncMs\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_NUM_SV_USED_IN_POSITION))
        {
            LE_TEST_INFO("valid number of sv used\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_CALIBRATION_CONFIDENCE_PERCENT))
        {
            LE_TEST_INFO("valid sensor calibrationConfidencePercent\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_CALIBRATION_STATUS))
        {
            LE_TEST_INFO("valid sensor calibrationConfidence\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_OUTPUT_ENG_TYPE))
        {
            LE_TEST_INFO("valid output engine type\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_OUTPUT_ENG_MASK))
        {
            LE_TEST_INFO("valid output engine mask\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_CONFORMITY_INDEX_FIX))
        {
            LE_TEST_INFO("valid conformity index\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_LLA_VRP_BASED))
        {
            LE_TEST_INFO("valid lla vrp based\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_ENU_VELOCITY_VRP_BASED))
        {
            LE_TEST_INFO("valid enu velocity vrp based\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_ALTITUDE_TYPE))
        {
            LE_TEST_INFO("valid altitude type\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_REPORT_STATUS))
        {
            LE_TEST_INFO("valid report status\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_INTEGRITY_RISK_USED))
        {
            LE_TEST_INFO("valid integrity risk\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_PROTECT_LEVEL_ALONG_TRACK))
        {
            LE_TEST_INFO("valid protect along track\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_PROTECT_LEVEL_CROSS_TRACK))
        {
            LE_TEST_INFO("valid protect cross track\n");
        }
        if(validityExMask & (1ULL << TAF_GNSS_HAS_PROTECT_LEVEL_VERTICAL))
        {
            LE_TEST_INFO("valid protect vertical\n");
        }
        if(validityExMask == 0)
        {
            LE_TEST_INFO("no ValidEx Mask\n");
        }
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("GetValidityInfo is invalid\n");
    }
    else
    {
        LE_TEST_INFO("Failed! See log for details\n");
    }

    //Gets the the combination of position engines and location engine type
    //used in calculating the position report.
    LE_TEST_INFO("taf_gnss_GetLocationOutputEngParams() is called");
    result = taf_gnss_GetLocationOutputEngParams(positionSampleRef,&engMask,&locationEngType);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetLocationOutputEngParams-LE_OK");
    if (result == LE_OK)
    {
        if(engMask & TAF_GNSS_STANDARD_POSITIONING_ENGINE)
        {
            LE_TEST_INFO("SPE used in the reports\n");
        }
        if(engMask & TAF_GNSS_DEAD_RECKONING_ENGINE)
        {
            LE_TEST_INFO("DRE used in the reports\n");
        }
        if(engMask & TAF_GNSS_PRECISE_POSITIONING_ENGINE)
        {
            LE_TEST_INFO("PPE used in the reports\n");
        }
        if(engMask & TAF_GNSS_VP_POSITIONING_ENGINE)
        {
            LE_TEST_INFO("VPE used in the reports\n");
        }
        if(engMask == 0)
        {
            LE_TEST_INFO("no output engine Mask Mask\n");
        }
        if(locationEngType == TAF_GNSS_LOC_OUTPUT_ENGINE_FUSED)
        {
            LE_TEST_INFO("This is FUSED engine reports\n");
        }
        if(locationEngType == TAF_GNSS_LOC_OUTPUT_ENGINE_SPE)
        {
            LE_TEST_INFO("This is SPE engine reports\n");
        }
        if(locationEngType == TAF_GNSS_LOC_OUTPUT_ENGINE_PPE)
        {
            LE_TEST_INFO("This is PPE engine reports\n");
        }
        if(locationEngType == TAF_GNSS_LOC_OUTPUT_ENGINE_VPE)
        {
            LE_TEST_INFO("This is VPE engine reports\n");
        }
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("GetEngineOutputParams is invalid\n");
    }
    else
    {
        LE_TEST_INFO("Failed! See log for details\n");
    }

    //Gets the reliability of the horizontal & vertical positions.
    LE_TEST_INFO("taf_gnss_GetReliabilityInformation() is called");
    result = taf_gnss_GetReliabilityInformation(positionSampleRef,&horiReliablity,&vertReliablity);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetReliabilityInformation-LE_OK");
    if (result == LE_OK)
    {
        if(horiReliablity & TAF_GNSS_RELIABILITY_NOT_SET)
        {
            LE_TEST_INFO("Horizontal reliability: NOT_SET\n");
        }
        else if(horiReliablity & TAF_GNSS_RELIABILITY_VERY_LOW)
        {
            LE_TEST_INFO("Horizontal reliability: VERY_LOW\n");
        }
        else if(horiReliablity & TAF_GNSS_RELIABILITY_LOW)
        {
            LE_TEST_INFO("Horizontal reliability: LOW\n");
        }
        else if(horiReliablity & TAF_GNSS_RELIABILITY_MEDIUM)
        {
            LE_TEST_INFO("Horizontal reliability: MEDIUM\n");
        }
        else if(horiReliablity & TAF_GNSS_RELIABILITY_HIGH)
        {
            LE_TEST_INFO("Horizontal reliability: HIGH\n");
        }
        else
        {
            LE_TEST_INFO("Horizontal reliability: UNKNOWN\n");
        }
        if(vertReliablity & TAF_GNSS_RELIABILITY_NOT_SET)
        {
            LE_TEST_INFO("Vertical reliability: NOT_SET\n");
        }
        else if(vertReliablity & TAF_GNSS_RELIABILITY_VERY_LOW)
        {
            LE_TEST_INFO("Vertical reliability: VERY_LOW\n");
        }
        else if(vertReliablity & TAF_GNSS_RELIABILITY_LOW)
        {
            LE_TEST_INFO("Vertical reliability: LOW\n");
        }
        else if(vertReliablity & TAF_GNSS_RELIABILITY_MEDIUM)
        {
            LE_TEST_INFO("Vertical reliability: MEDIUM\n");
        }
        else if(vertReliablity & TAF_GNSS_RELIABILITY_HIGH)
        {
            LE_TEST_INFO("Vertical reliability: HIGH\n");
        }
        else
        {
            LE_TEST_INFO("Vertical reliability: UNKNOWN\n");
        }
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("GetReliabilityInfo is invalid\n");
    }
    else
    {
        LE_TEST_INFO("Failed! See log for details\n");
    }

    //Gets the elliptical horizontal uncertainty azimuth of orientation,east
    //and north standard deviations..
    LE_TEST_INFO("taf_gnss_GetStdDeviationAzimuthInfo() is called");
    result = taf_gnss_GetStdDeviationAzimuthInfo(positionSampleRef,&azimuth,&eastDev,&northDev);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetStdDeviationAzimuthInfo-LE_OK");
    if (result == LE_OK)
    {
        LE_TEST_INFO("Azimuth: %lf degrees\n",(float)azimuth);
        LE_TEST_INFO("East standard deviation: %lfm\n",(float)eastDev);
        LE_TEST_INFO("North standard deviation: %lfm\n",(float)northDev);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("taf_gnss_GetStdDeviationAzimuthInfo is invalid\n");
    }
    else
    {
        LE_TEST_INFO("Failed! See log for details\n");
    }

    //Gets the elapsed real time and its uncertainity values.
    LE_TEST_INFO("taf_gnss_GetRealTimeInformation() is called");
    result = taf_gnss_GetRealTimeInformation(positionSampleRef,&realTime,&realTimeUnc);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetRealTimeInformation-LE_OK");
    if (result == LE_OK)
    {
        LE_TEST_INFO("Elapsed real time: %"PRIu64" ns\n",realTime);
        LE_TEST_INFO("Elapsed real time uncertainity: %"PRIu64" ns\n",realTimeUnc);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("GetRealTimeInfo is invalid\n");
    }
    else
    {
        LE_TEST_INFO("Failed! See log for details\n");
    }

    //Gets gnss meaurement usage info.
    taf_gnss_GnssMeasurementInfo_t measInfo[TAF_GNSS_MEASUREMENT_INFO_MAX];
    size_t gnssMeasLen = TAF_GNSS_MEASUREMENT_INFO_MAX;
    result = taf_gnss_GetMeasurementUsageInfo(positionSampleRef, measInfo, &gnssMeasLen);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetMeasurementUsageInfo-LE_OK");

    if (result != LE_OK) {
        LE_TEST_INFO("Error to get measurement info. Error: %d\n", (int) result);
    } else {
        for(uint16_t i = 0; (i < gnssMeasLen); i++) {
            uint32_t signalTypeMask = measInfo[i].gnssSignalType;
            PrintGnssSignalType(signalTypeMask);
        }
    }

    //Gets status of report in terms of how optimally the report was calculated by engine.
    int32_t reportStatus = -1;
    result = taf_gnss_GetReportStatus(positionSampleRef, &reportStatus);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetReportStatus-LE_OK");

    if (result == LE_OK)
    {
        taf_gnss_ReportStatus_t status = (taf_gnss_ReportStatus_t) reportStatus;
        LE_TEST_INFO("Report Status is: ");
        if (status == TAF_GNSS_REPORT_STATUS_UNKNOWN) {
            LE_TEST_INFO("UNKNOWN\n");
        }
        if (status == TAF_GNSS_REPORT_STATUS_SUCCESS) {
            LE_TEST_INFO("SUCCESS\n");
        }
        if (status == TAF_GNSS_REPORT_STATUS_INTERMEDIATE) {
            LE_TEST_INFO("INTERMEDIATE\n");
        }
        if (status == TAF_GNSS_REPORT_STATUS_FAILURE) {
            LE_TEST_INFO("FAILURE\n");
        }
    }

    //Gets the altitude with respect to mean sea level in meters.
    double altMSeaLevel;
    result = taf_gnss_GetAltitudeMeanSeaLevel(positionSampleRef, &altMSeaLevel);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetAltitudeMeanSeaLevel-LE_OK");

    if (result == LE_OK)
    {
        LE_TEST_INFO("Altitude with respect to mean sea level: %lfm\n",(float)altMSeaLevel);
    }

    //Gets GNSS Satellite Vehicles used in position data.
    uint16_t svIds[TAF_GNSS_MEASUREMENT_INFO_MAX];
    size_t svIdsLen = TAF_GNSS_MEASUREMENT_INFO_MAX;
    result = taf_gnss_GetSVIds(positionSampleRef, svIds, &svIdsLen);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetSVIds-LE_OK");

    if (svIdsLen > 0) LE_TEST_INFO("Ids of used SVs:");
    for(uint16_t i = 0; i < svIdsLen; i++) {
        LE_TEST_INFO(" %d", svIds[i]);
    }

    //Get Jammer and Automatic Gain Control information
    size_t maxSigTypes = TAF_GNSS_NUMBER_OF_SIGNAL_TYPES_MAX;
    taf_gnss_GnssData_t gnssDataPtr[TAF_GNSS_NUMBER_OF_SIGNAL_TYPES_MAX];
    LE_TEST_INFO("taf_gnss_GetGnssData is triggered\n");
    result = taf_gnss_GetGnssData(positionSampleRef, gnssDataPtr, &maxSigTypes);

    LE_TEST_OK(result == LE_OK, "taf_gnss_GetGnssData-LE_OK");

    for(uint8_t i = 0; i < maxSigTypes; i++)
    {
        LE_TEST_INFO("GetGnssData type :%d", i);
        LE_TEST_INFO("gnssDataMask:%d", gnssDataPtr[i].gnssDataMask);
        if(gnssDataPtr[i].gnssDataMask & TAF_GNSS_HAS_JAMMER)
        {
            LE_TEST_INFO("jammerInd is present");
            LE_TEST_INFO("jammerInd: %lf",gnssDataPtr[i].jammerInd);
        }
        else
        {
            LE_TEST_INFO("jammerInd is not present");
        }
        if(gnssDataPtr[i].gnssDataMask & TAF_GNSS_HAS_AGC)
        {
            LE_TEST_INFO("Automatic Gain Control is present");
            LE_TEST_INFO("AGC: %lf",gnssDataPtr[i].agc);
        }
        else
        {
            LE_TEST_INFO("Automatic Gain Control is not present");
        }
        LE_TEST_INFO("\n");
    }


    LE_TEST_INFO("taf_gnss_ReleaseSampleRef is triggered");
    taf_gnss_ReleaseSampleRef(positionSampleRef);
    le_sem_Post(PositionHandlerSem);

}

static void SamplePositionHandler
(
    taf_pos_SampleRef_t positionSampleRef,
    void* contextPtr
)
{
    le_result_t result;
    taf_gnss_FixState_t fixState;
    int32_t lati,longi,accuracy;
    uint16_t year = 0;
    uint16_t month = 0;
    uint16_t day = 0;
    uint16_t hours;
    uint16_t minutes;
    uint16_t seconds;
    uint16_t milliseconds;
    int32_t  alti,altaccuracy;
    uint32_t hval,hAccuracy;
    int32_t val,vAccuracy;
    uint32_t uval, uAccuracy;

    //175.taf_pos_sample_GetFixState()
    LE_TEST_INFO("taf_pos_sample_GetFixState() API is triggered to position fix state");
    result = taf_pos_sample_GetFixState(positionSampleRef, &fixState);
    LE_TEST_OK((result == LE_OK),"taf_pos_sample_GetFixState-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Sample Position fix state:%s",(TAF_GNSS_STATE_FIX_NO_POS == fixState)?"No Fix"
                                     :(TAF_GNSS_STATE_FIX_2D == fixState)?"2D Fix"
                                     :(TAF_GNSS_STATE_FIX_3D == fixState)?"3D Fix"
                                     :(TAF_GNSS_STATE_FIX_ESTIMATED == fixState)?"Estimated Fix"
                                     : "Unknown");
    }
    else
    {
        LE_TEST_INFO("Failed to get sample position fix state\n");
    }

    //176.taf_pos_sample_Get2DLocation()
    LE_TEST_INFO("taf_pos_sample_Get2DLocation() API is triggered to get 2D location information ");
    result = taf_pos_sample_Get2DLocation(positionSampleRef, &lati, &longi, &accuracy);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),
        "taf_pos_sample_Get2DLocation-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_INFO("taf_pos_sample_Get2DLocation: lat:%.6f, long:%.6f, accuracy:%d",(float)longi/1e6,(float)lati/1e6,accuracy);
    }
    else
    {
        LE_TEST_INFO("Failed to get position sample 2D location\n");
    }

    //177.taf_pos_sample_GetDate()
    LE_TEST_INFO("taf_pos_sample_GetDate() API is triggered to get pos sample Date information\n");
    result = taf_pos_sample_GetDate(positionSampleRef, &year, &month, &day);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),"taf_pos_sample_GetDate-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("taf_pos_sample_GetDate: year.%d, month.%d, day.%d", year, month, day);
    }
    else
    {
        LE_TEST_INFO("Failed to get position sample Date information\n");
    }

    //178.taf_pos_sample_GetTime()
    LE_TEST_INFO("taf_pos_sample_GetTime() API is triggered to get pos sample time information\n");
    result = taf_pos_sample_GetTime(positionSampleRef, &hours, &minutes, &seconds, &milliseconds);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),"taf_pos_sample_GetTime-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("taf_pos_sample_GetTime: hours.%d, minutes.%d, seconds.%d, milliseconds.%d",
            hours, minutes, seconds,milliseconds);
    }
    else
    {
        LE_TEST_INFO("Failed to get position sample time information\n");
    }

    //179.taf_pos_sample_GetAltitude()
    LE_TEST_INFO("taf_pos_sample_GetAltitude() API is triggered to get pos sample a information\n");
    result = taf_pos_sample_GetAltitude(positionSampleRef, &alti, &altaccuracy);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),
        "taf_pos_sample_GetAltitude-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("GetAltitude: alt: %d, accuracy: %d",alti,altaccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed to get position sample altitude information\n");
    }

    //180.taf_pos_sample_GetHorizontalSpeed()
    LE_TEST_INFO("taf_pos_sample_GetHorizontalSpeed() API is triggered to get pos"
        "sample Get horizontal speed\n");
    result = taf_pos_sample_GetHorizontalSpeed(positionSampleRef, &hval, &hAccuracy);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),
        "taf_pos_sample_GetHorizontalSpeed-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("taf_pos_sample_GetHorizontalSpeed: hSpeed: %u, accuracy:%u", hval,hAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed to get position sample horizontal speed information");
    }

    //181.taf_pos_sample_GetVerticalSpeed()
    LE_TEST_INFO("taf_pos_sample_GetVerticalSpeed() API is triggered to get pos"
        "sample Get vertical speed\n");
    result = taf_pos_sample_GetVerticalSpeed(positionSampleRef, &val, &vAccuracy);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),
        "taf_pos_sample_GetVerticalSpeed-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("GetVerticalSpeed: vSpeed: %d, vSpeedAccuracy: %d",val,vAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed to get position sample horizontal speed information");
    }

    //182.taf_pos_sample_GetDirection()
    LE_TEST_INFO("taf_pos_sample_GetDirection() API is triggered to get direction information\n");
    result = taf_pos_sample_GetDirection(positionSampleRef, &uval, &uAccuracy);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),
        "taf_pos_sample_GetDirection-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("taf_pos_sample_GetDirection: direction.%u, accuracy.%u",uval, uAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed to get position sample direction information");
    }

    //taf_pos_sample_Release
    LE_TEST_INFO("taf_gnss_ReleaseSampleRef is triggered");
    taf_pos_sample_Release(positionSampleRef);

}

static void* PositionThread
(
    void* context
)
{
    LE_TEST_INFO("======== Position Handler thread  ========");
    taf_gnss_ConnectService();

    le_result_t result = taf_gnss_Start();

    LE_INFO("Result of gnss start: %d", (int)result);

    PositionHandlerRef = taf_gnss_AddPositionHandler(PositionHandlerFunction, NULL);

    //137.Position Handler
    LE_TEST_OK((PositionHandlerRef != NULL),
            "Confirm position handler was added successfully");

    LE_TEST_INFO("======== Position Handler thread before le_event_RunLoop ========");
    le_event_RunLoop();
    LE_TEST_INFO("======== Position Handler thread After le_event_RunLoop ========");
    return NULL;
}

static void* SamplePositionThread
(
    void* context
)
{
    LE_TEST_INFO("======== Sample Position Handler thread  ========");
    taf_pos_ConnectService();

    //174.Sample Position Handler
    SamplePositionHandlerRef = taf_pos_AddMovementHandler(0, 0, SamplePositionHandler, NULL);
    LE_TEST_OK((SamplePositionHandlerRef != NULL),
        "Confirm sample position handler was added successfully");

    LE_TEST_INFO("======== Sample Position Handler thread before le_event_RunLoop ========");
    le_event_RunLoop();
    LE_TEST_INFO("======== Sample Position Handler thread After le_event_RunLoop ========");
    return NULL;
}

static void TestTafSamplePositionHandler
(
    void
)
{
    le_thread_Ref_t positionThreadRef;
    taf_posCtrl_ActivationRef_t activationRef;
    LE_INFO("TestTafSamplePositionHandler");

    //173.taf_posCtrl_Request
    LE_TEST_INFO("taf_posCtrl_Request() API is called to get positioning services");
    activationRef = taf_posCtrl_Request();
    LE_TEST_OK((activationRef!=NULL),"taf_posCtrl_Request-LE_OK");

    // Add Position Handler Test
    positionThreadRef = le_thread_Create("PositionThreadTest",SamplePositionThread,NULL);
    LE_INFO("TestTafSamplePositionHandler positionThreadRef :%p",positionThreadRef);
    le_thread_Start(positionThreadRef);
    LE_INFO("TestTafSamplePositionHandler PositionHandlerRef :%p",PositionHandlerRef);
    LE_TEST_INFO("Wait for 3 seconds to trigger SamplePositionHandlerfunction");
    le_thread_Sleep(3);
    taf_pos_RemoveMovementHandler(SamplePositionHandlerRef);

    LE_INFO("TestTafGnssPositionHandler->cancel the thread");
    le_thread_Cancel(positionThreadRef);

    //183.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting GNSS fixes");
    LE_TEST_OK(taf_gnss_Stop() == LE_DUPLICATE, "taf_gnss_Stop-LE_DUPLICATE");//lsc
    LE_INFO("Release the positioning service");
    taf_posCtrl_Release(activationRef);

}

static void TestTafGnssPositionHandler
(
    void
)
{

    le_thread_Ref_t positionThreadRef;
    LE_INFO("TestTafGnssPositionHandler");

    //136. taf_gnss_Start() This will trigger startDetailedEngineReports() TelSDK API
    LE_TEST_OK(((taf_gnss_Start()) == LE_OK), "taf_gnss_Start-LE_OK");
    LE_TEST_INFO("Wait for 5 seconds");
    le_thread_Sleep(5);

    // Add Position Handler Test
    positionThreadRef = le_thread_Create("PositionThreadTest",PositionThread,NULL);
    LE_INFO("TestTafGnssPositionHandler positionThreadRef :%p",positionThreadRef);
    le_thread_Start(positionThreadRef);
    LE_INFO("TestTafGnssPositionHandler PositionHandlerRef :%p",PositionHandlerRef);
    LE_TEST_INFO("Wait for 3 seconds to trigger PositionHandlerfunction");
    le_thread_Sleep(3);
    taf_gnss_RemovePositionHandler(PositionHandlerRef);

    LE_INFO("TestTafGnssPositionHandler->cancel the thread");
    le_thread_Cancel(positionThreadRef);

    //156.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting GNSS fixes");
    LE_TEST_OK(taf_gnss_Stop() == LE_OK, "taf_gnss_Stop-LE_OK");
}


void DisplayNmea(const char nmeaMask[TAF_GNSS_NMEA_STRING_MAX]) {
    LE_INFO( "**** DisplayNmea NMEA handler string copied: %s****",nmeaMask);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for NMEA notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void NmeaHandlerFunction
(
    uint64_t timestamp,
    const char nmeaInfo[TAF_GNSS_NMEA_STRING_MAX],
    void* contextPtr
)
{
    LE_INFO("\n************* NMEA Information ***************\n");
    LE_INFO("Timestamp                    : %"PRIu64" \n", timestamp);
    DisplayNmea(nmeaInfo);
    LE_INFO("**********************************************\n");
}

static void* NmeaThread
(
    void* context
)
{
    LE_TEST_INFO("======== Nmea Handler thread  ========");
    taf_gnss_ConnectService();

    le_result_t result = taf_gnss_Start();

    LE_INFO("Result of gnss start: %d", (int)result);

    NmeaHandlerRef = taf_gnss_AddNmeaHandler(NmeaHandlerFunction, NULL);

    //137.Nmea Handler
    LE_TEST_OK((NmeaHandlerRef != NULL),
            "Confirm Nmea handler was added successfully");

    LE_TEST_INFO("======== Nmea Handler thread before le_event_RunLoop ========");
    le_event_RunLoop();
    LE_TEST_INFO("======== Nmea Handler thread After le_event_RunLoop ========");
    return NULL;
}

static void TestTafGnssNmeaHandler
(
    void
)
{
    le_thread_Ref_t nmeaThreadRef;
    LE_INFO("TestTafGnssNmeaHandler");

    LE_TEST_OK(((taf_gnss_Start()) == LE_OK), "taf_gnss_Start-LE_OK");
    LE_TEST_INFO("Wait for 5 seconds");
    le_thread_Sleep(5);

    // Add Nmea Handler Test
    nmeaThreadRef = le_thread_Create("NmeaThreadTest",NmeaThread,NULL);
    LE_INFO("TestTafGnssNmeaHandler nmeaThreadRef :%p",nmeaThreadRef);
    le_thread_Start(nmeaThreadRef);
    LE_INFO("TestTafGnssNmeaHandler NmeaHandlerRef :%p",NmeaHandlerRef);
    LE_TEST_INFO("Wait for 3 seconds to trigger NmeaHandlerfunction");
    le_thread_Sleep(3);
    taf_gnss_RemoveNmeaHandler(NmeaHandlerRef);

    LE_INFO("TestTafGnssNmeaHandler->cancel the thread");
    le_thread_Cancel(nmeaThreadRef);

    //156.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting GNSS fixes");
    LE_TEST_OK(taf_gnss_Stop() == LE_OK, "taf_gnss_Stop-LE_OK");
}

void DisplayCapabilities(taf_gnss_LocCapabilityType_t capabilityMask) {
  LE_INFO("\n************* Capabilities Information *************\n");
  LE_INFO("The location capabilities bit mask: 0x%08X\n", capabilityMask);
  if (capabilityMask & TAF_GNSS_TIME_BASED_TRACKING) {
    LE_INFO("Time based tracking\n");
  }
  if (capabilityMask & TAF_GNSS_DISTANCE_BASED_TRACKING) {
    LE_INFO("Distance based tracking\n");
  }
  if (capabilityMask & TAF_GNSS_GNSS_MEASUREMENTS) {
    LE_INFO("GNSS Measurement\n");
  }
  if (capabilityMask & TAF_GNSS_CONSTELLATION_ENABLEMENT) {
    LE_INFO("Constellation enablement\n");
  }
  if (capabilityMask & TAF_GNSS_CARRIER_PHASE) {
    LE_INFO("Carrier phase\n");
  }
  if (capabilityMask & TAF_GNSS_QWES_GNSS_SINGLE_FREQUENCY) {
    LE_INFO("QWES GNSS single frequency\n");
  }
  if (capabilityMask & TAF_GNSS_QWES_GNSS_MULTI_FREQUENCY) {
    LE_INFO("QWES GNSS multi frequency\n");
  }
  if (capabilityMask & TAF_GNSS_QWES_VPE) {
    LE_INFO("QWES VPE\n");
  }
  if (capabilityMask & TAF_GNSS_QWES_CV2X_LOCATION_BASIC) {
    LE_INFO("QWES CV2X location basic\n");
  }
  if (capabilityMask & TAF_GNSS_QWES_CV2X_LOCATION_PREMIUM) {
    LE_INFO("QWES CV2X location premium\n");
  }
  if (capabilityMask & TAF_GNSS_QWES_PPE) {
    LE_INFO("QWES PPE\n");
  }
  if (capabilityMask & TAF_GNSS_QWES_QDR2) {
    LE_INFO("QWES QDR2\n");
  }
  if (capabilityMask & TAF_GNSS_QWES_QDR3) {
    LE_INFO("QWES QDR3\n");
  }
  if (capabilityMask & TAF_GNSS_TIME_BASED_BATCHING) {
    LE_INFO("TIME_BASED_BATCHING\n");
  }
  if (capabilityMask & TAF_GNSS_DISTANCE_BASED_BATCHING) {
    LE_INFO("DISTANCE_BASED_BATCHING\n");
  }
  if (capabilityMask & TAF_GNSS_GEOFENCE) {
    LE_INFO("GEOFENCE\n");
  }
  if (capabilityMask & TAF_GNSS_OUTDOOR_TRIP_BATCHING) {
    LE_INFO("OUTDOOR_TRIP_BATCHING\n");
  }
  if (capabilityMask & TAF_GNSS_SV_POLYNOMIAL) {
    LE_INFO("SV_POLYNOMIAL\n");
  }
  if (capabilityMask & TAF_GNSS_NLOS_ML20) {
    LE_INFO("NLOS_ML20\n");
  }
  LE_INFO("****************************************************\n");
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for location capability notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CapabilityHandlerFunction
(
    taf_gnss_LocCapabilityType_t locCapability,
    void* contextPtr
)
{
    DisplayCapabilities(locCapability);
}

static void* CapabilityChangeThread
(
    void* context
)
{
    LE_TEST_INFO("======== CapabilityChange Handler thread  ========");
    taf_gnss_ConnectService();

    le_result_t result = taf_gnss_Start();

    LE_INFO("Result of gnss start: %d", (int)result);

    CapabilityChangeHandlerRef = taf_gnss_AddCapabilityChangeHandler(CapabilityHandlerFunction, NULL);

    //137.CapabilityChange Handler
    LE_TEST_OK((CapabilityChangeHandlerRef != NULL),
            "Confirm CapabilityChange handler was added successfully");

    LE_TEST_INFO("======== CapabilityChange Handler thread before le_event_RunLoop ========");
    le_event_RunLoop();
    LE_TEST_INFO("======== CapabilityChange Handler thread After le_event_RunLoop ========");
    return NULL;
}

static void TestTafGnssCapabilityChangeHandler
(
    void
)
{
    le_thread_Ref_t capabilityChangeThreadRef;
    LE_INFO("TestTafGnssCapabilityChangeHandler");

    LE_TEST_OK(((taf_gnss_Start()) == LE_OK), "taf_gnss_Start-LE_OK");
    LE_TEST_INFO("Wait for 5 seconds");
    le_thread_Sleep(5);

    // Add CapabilityChange Handler Test
    capabilityChangeThreadRef = le_thread_Create("CapabilityChangeThreadTest",CapabilityChangeThread,NULL);
    LE_INFO("TestTafGnssCapabilityChangeHandler capabilityChangeThreadRef :%p",capabilityChangeThreadRef);
    le_thread_Start(capabilityChangeThreadRef);
    LE_INFO("TestTafGnssCapabilityChangeHandler CapabilityChangeHandlerRef :%p",CapabilityChangeHandlerRef);
    LE_TEST_INFO("Wait for 3 seconds to trigger CapabilityChangeHandlerfunction");
    le_thread_Sleep(3);
    taf_gnss_RemoveCapabilityChangeHandler(CapabilityChangeHandlerRef);

    LE_INFO("TestTafGnssCapabilityChangeHandler->cancel the thread");
    le_thread_Cancel(capabilityChangeThreadRef);

    //156.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting GNSS fixes");
    LE_TEST_OK(taf_gnss_Stop() == LE_OK, "taf_gnss_Stop-LE_OK");
}

static void TestTafGnssStart
(
    void
)
{
    le_result_t result = LE_FAULT;
    uint32_t ttff = 0;

   //1.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to report detailed Engine Reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

   //2.Get GNSS State
    LE_TEST_INFO("taf_gnss_GetState() API is called to get the current GNSS state");
    LE_TEST_OK(((taf_gnss_GetState()) == TAF_GNSS_STATE_ACTIVE), "Get GNSS state as ACTIVE");

   //3.Start -Duplicate
    LE_TEST_INFO("taf_gnss_Start() API is called again to check whether it returns"
        "duplicate state or not");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_DUPLICATE, "taf_gnss_Start-LE_DUPLICATE");

   //4.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting GNSS fixes");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

   //5.Stop - Duplicate
    LE_TEST_INFO("taf_gnss_Stop() API is called to check whether it returns"
        "duplicate state or not");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_DUPLICATE, "taf_gnss_Stop-LE_DUPLICATE");

   //6.Disable GNSS
    LE_TEST_INFO("taf_gnss_Disable() API is called to disable GNSS engine");
    result = taf_gnss_Disable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Disable-LE_OK");

   //7. Disable -Duplicate
    LE_TEST_INFO("taf_gnss_Disable() API is called to check whether it returns"
        "duplicate state or not");
    result = taf_gnss_Disable();
    LE_TEST_OK(result == LE_DUPLICATE, "taf_gnss_Disable-LE_DUPLICATE");

   //8.Stop - Not Permitted
    LE_TEST_INFO("taf_gnss_Stop() API is called to check whether it returns"
        "Not Permitted state or not");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_Stop-LE_NOT_PERMITTED");

   //9.Start-Not Permitted
    LE_TEST_INFO("taf_gnss_Start() API is called again to check whether it returns"
        "Not permitted state or not");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_Start-LE_NOT_PERMITTED");

   //10.Enable GNSS
    LE_TEST_INFO("taf_gnss_Enable() API is called to Enable GNSS engine");
    result = taf_gnss_Enable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Enable-LE_OK");

   //11.Enable GNSS-Duplicate
    LE_TEST_INFO("taf_gnss_Enable() API is called to check whether it returns"
        "duplicate state or not");
    result = taf_gnss_Enable();
    LE_TEST_OK(result == LE_DUPLICATE, "taf_gnss_Enable-LE_DUPLICATE");

   //12.Start-
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

   //13. Disable -Not Permitted
    LE_TEST_INFO("taf_gnss_Disable() API is called to check whether it returns"
        "Not permitted state or not");
    result = taf_gnss_Disable();
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_Disable-LE_NOT_PERMITTED");

   //14.GetTtff
    LE_TEST_INFO("taf_gnss_GetTtff() API is called to get time to first fix");
    result = taf_gnss_GetTtff(&ttff);
    LE_TEST_OK(result == LE_OK, "taf_gnss_Tfff-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("TTFF start = %d msec", ttff);
    }
    else
    {
        LE_TEST_INFO("TTFF start not available");
    }

   //15.Stop
    LE_TEST_INFO("taf_gnss_Start() API is called");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //16.GetTtff
    LE_TEST_INFO("taf_gnss_GetTtff() API is called to get time to first fix");
    result = taf_gnss_GetTtff(&ttff);
    LE_TEST_OK(result == LE_OK, "taf_gnss_Tfff-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("TTFF start = %d msec", ttff);
    }
    else
    {
        LE_TEST_INFO("TTFF start not available");
    }

    //17.Disable GNSS
    LE_TEST_INFO("taf_gnss_Disable() API is called to disable GNSS engine");
    result = taf_gnss_Disable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Disable-LE_OK");

   //18.GetTtff -Not Permitted
    LE_TEST_INFO("taf_gnss_GetTtff() API is called to check whether it returns"
        "Not Permitted state or not");
    result = taf_gnss_GetTtff(&ttff);
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_Tfff-LE_NOT_PERMITTED");

    //19.Enable GNSS
    LE_TEST_INFO("taf_gnss_Enable() API is called to disable GNSS engine");
    result = taf_gnss_Enable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Enable-LE_OK");

}

static void TestTafGnssStartType
(
    void
)
{
    le_result_t result = LE_FAULT;
    taf_gnss_EngineReportsType_t EngineType;

   //SetEngineType - FUSED
    LE_TEST_INFO("taf_gnss_SetEngineType() API is called to report FUSED Engine Reporting");
    EngineType = TAF_GNSS_ENGINE_REPORT_TYPE_FUSED;
    result = taf_gnss_SetEngineType(EngineType);
    LE_TEST_OK(result == LE_OK, "taf_gnss_SetEngineType-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to start reporting GNSS fixes");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

   //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting GNSS fixes");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //SetEngineType - SPE
    LE_TEST_INFO("taf_gnss_SetEngineType() API is called to report SPE Engine Reporting");
    EngineType = TAF_GNSS_ENGINE_REPORT_TYPE_SPE;
    result = taf_gnss_SetEngineType(EngineType);
    LE_TEST_OK(result == LE_OK, "taf_gnss_SetEngineType-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to start reporting GNSS fixes");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting GNSS fixes");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //SetEngineType - PPE
    LE_TEST_INFO("taf_gnss_SetEngineType() API is called to report PPE Engine Reporting");
    EngineType = TAF_GNSS_ENGINE_REPORT_TYPE_PPE;
    result = taf_gnss_SetEngineType(EngineType);
    LE_TEST_OK(result == LE_OK, "taf_gnss_SetEngineType-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to start reporting GNSS fixes");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting GNSS fixes");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //SetEngineType - VPE
    LE_TEST_INFO("taf_gnss_SetEngineType() API is called to report VPE Engine Reporting");
    EngineType = TAF_GNSS_ENGINE_REPORT_TYPE_VPE;
    result = taf_gnss_SetEngineType(EngineType);
    LE_TEST_OK(result == LE_OK, "taf_gnss_SetEngineType-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to start reporting GNSS fixes");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting GNSS fixes");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //SetEngineType - LE_BAD_PARAMETER
    LE_TEST_INFO("taf_gnss_SetEngineType()triggered to check whether it returns BAD parameter or not");
    EngineType = TAF_GNSS_ENGINE_REPORT_TYPE_FUSED-1;
    result = taf_gnss_SetEngineType(EngineType);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_gnss_SetEngineType-LE_BAD_PARAMETER");

    //SetEngineType - LE_BAD_PARAMETER
    LE_TEST_INFO("taf_gnss_SetEngineType()triggered to check whether it returns BAD parameter or not");
    EngineType = TAF_GNSS_ENGINE_REPORT_TYPE_VPE+1;
    result = taf_gnss_SetEngineType(EngineType);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_gnss_SetEngineType-LE_BAD_PARAMETER");

   //Disable
    LE_TEST_INFO("taf_gnss_Disable() API is called to disable the GNSS device");
    result = taf_gnss_Disable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Disable-LE_OK");

    //SetEngineType - LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_SetEngineType() is triggered to check whether it returns not permitted or not");
    EngineType = TAF_GNSS_ENGINE_REPORT_TYPE_VPE;
    result = taf_gnss_SetEngineType(EngineType);
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_SetEngineType-LE_NOT_PERMITTED");

    //Enable GNSS
    LE_TEST_INFO("taf_gnss_Enable() API is called to enable GNSS engine");
    result = taf_gnss_Enable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Enable-LE_OK");

    //SetEngineType - FUSED
    LE_TEST_INFO("taf_gnss_SetEngineType() API is called to report FUSED Engine Reporting");
    EngineType = TAF_GNSS_ENGINE_REPORT_TYPE_FUSED;
    result = taf_gnss_SetEngineType(EngineType);
    LE_TEST_OK(result == LE_OK, "taf_gnss_SetEngineType-LE_OK");

}


static void TestTafGnssConstellations
(
    void
)
{
    le_result_t result = LE_FAULT;
    taf_gnss_ConstellationBitMask_t constellationMask;

   //20.GetSupportedConstellations
    LE_TEST_INFO("taf_gnss_GetSupportedConstellations() API is called to get the list of"
        "supported constellations");
    result = taf_gnss_GetSupportedConstellations(&constellationMask);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetSupportedConstellations-LE_OK");
    if(result == LE_OK)
    {
        if (constellationMask & TAF_GNSS_CONSTELLATION_GLONASS)
        {
            LE_TEST_INFO("GLONASS is Supported\n");
        }
        if (constellationMask & TAF_GNSS_CONSTELLATION_BEIDOU)
        {
            LE_TEST_INFO("BEDIDOU is Supported\n");
        }
        if (constellationMask & TAF_GNSS_CONSTELLATION_GALILEO)
        {
            LE_TEST_INFO("GALILEO is Supported\n");
        }
        if (constellationMask & TAF_GNSS_CONSTELLATION_SBAS)
        {
            LE_TEST_INFO("SBAS is Supported\n");
        }
        if (constellationMask & TAF_GNSS_CONSTELLATION_QZSS)
        {
            LE_TEST_INFO("QZSS is Supported\n");
        }
    }
    else
    {
        LE_TEST_INFO("Failed! to get supported constellations");
    }

    //SetConstellation-UNDEFINED
    constellationMask = 0;
    LE_TEST_INFO("taf_gnss_SetConstellation() API to set 0- constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_FAULT,"taf_gnss_SetConstellation-LE_FAULT");

   //21.SetConstellation-GPS
    constellationMask = TAF_GNSS_CONSTELLATION_GPS;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set GPS constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_FAULT,"taf_gnss_SetConstellation-Not Implemented");

   //22.SetConstellation-GLONASS
    constellationMask = TAF_GNSS_CONSTELLATION_GLONASS;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set GLONASS constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");

    //22.1 SetConstellation-SBAS and BEIDOU
    constellationMask = TAF_GNSS_CONSTELLATION_SBAS | TAF_GNSS_CONSTELLATION_BEIDOU;
    LE_TEST_INFO("SetConstellation() API is called to set SBAS and BEIDOU constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");

    //22.2 SetConstellation-GLONASS and QZSS
    constellationMask = TAF_GNSS_CONSTELLATION_GLONASS | TAF_GNSS_CONSTELLATION_QZSS;
    LE_TEST_INFO("SetConstellation() API is called to set GLONASS and QZSS constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");

   //23.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

   //24.GetSupportedConstellations -LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_GetSupportedConstellations() API is called to check whether it returns"
        "Not permitted or not");
    result = taf_gnss_GetSupportedConstellations(&constellationMask);
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_GetSupportedConstellations-LE_NOT_PERMITTED");

   //25.SetConstellation-BEIDOU
    constellationMask = TAF_GNSS_CONSTELLATION_BEIDOU;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set BEIDOU constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");

   //26.SetConstellation-GALILEO
    constellationMask = TAF_GNSS_CONSTELLATION_GALILEO;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set GALILEO constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");

   //27.SetConstellation-SBAS
    constellationMask = TAF_GNSS_CONSTELLATION_SBAS;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set SBAS constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");

    //28.SetConstellation-QZSS
    constellationMask = TAF_GNSS_CONSTELLATION_QZSS;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set QZSS constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");

    //SetConstellation-NAVIC
    #ifdef TARGET_SA525M
    constellationMask = TAF_GNSS_CONSTELLATION_NAVIC;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set NAVIC constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");
    #endif

    //SetConstellation-ALL constellations
    #ifdef TARGET_SA525M
    constellationMask = 0x7E;
    LE_INFO("taf_gnss_SetConstellation triggered for Sa525m");
    #else
    constellationMask = 0x3E;
    LE_INFO("taf_gnss_SetConstellation triggered for Sa515m");
    #endif
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set All constellation types");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");

    //29.GetConstellation
    LE_TEST_INFO("taf_gnss_GetConstellation() API is called to get constellation types enabled");
    result = taf_gnss_GetConstellation(&constellationMask);
    LE_TEST_INFO("taf_gnss_GetConstellation() : %d",constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_GetConstellation-LE_OK");
    if(result == LE_OK)
    {
        if(constellationMask & TAF_GNSS_CONSTELLATION_GPS)
        {
            LE_TEST_INFO("GPS activated");
        }
        else
        {
            LE_TEST_INFO("GPS Not activated");
        }
        if(constellationMask & TAF_GNSS_CONSTELLATION_GLONASS)
        {
            LE_TEST_INFO("GLONASS activated");
        }
        else
        {
            LE_TEST_INFO("GLONASS Not activated");
        }
        if(constellationMask & TAF_GNSS_CONSTELLATION_BEIDOU)
        {
            LE_TEST_INFO("BEIDOU activated");
        }
        else
        {
            LE_TEST_INFO("BEIDOU Not activated");
        }
        if(constellationMask & TAF_GNSS_CONSTELLATION_GALILEO)
        {
            LE_TEST_INFO("GALILEO activated");
        }
        else
        {
            LE_TEST_INFO("GALILEO Not activated");
        }
        if(constellationMask & TAF_GNSS_CONSTELLATION_SBAS)
        {
            LE_TEST_INFO("SBAS activated");
        }
        else
        {
            LE_TEST_INFO("SBAS Not activated");
        }
        if(constellationMask & TAF_GNSS_CONSTELLATION_QZSS)
        {
            LE_TEST_INFO("QZSS activated");
        }
        else
        {
            LE_TEST_INFO("QZSS Not activated");
        }
        if(constellationMask & TAF_GNSS_CONSTELLATION_NAVIC)
        {
            LE_TEST_INFO("NAVIC activated");
        }
        else
        {
            LE_TEST_INFO("NAVIC Not activated");
        }
    }
    else
    {
        LE_TEST_INFO("Failed! to GetConstellation");
    }

   //30.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

   //31.SetConstellation-GPS -LE_OK
    constellationMask = TAF_GNSS_CONSTELLATION_GALILEO;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set GPS constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");

    //Disable GNSS
    LE_TEST_INFO("taf_gnss_Disable() API is called to disable GNSS engine");
    result = taf_gnss_Disable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Disable-LE_OK");

    //32.GetConstellation- LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_GetConstellation() API is called to get"
        "GPS constellation types enabled");
    result = taf_gnss_GetConstellation(&constellationMask);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"taf_gnss_GetConstellation-LE_NOT_PERMITTED");

    //SetConstellation-QZSS
    constellationMask = TAF_GNSS_CONSTELLATION_QZSS;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set QZSS constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"taf_gnss_SetConstellation-LE_NOT_PERMITTED");

    //Enable GNSS
    LE_TEST_INFO("taf_gnss_Enable() API is called to Enable GNSS engine");
    result = taf_gnss_Enable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Enable-LE_OK");


}

static void TestTafGnssNmeaSentences
(
    void
)
{
    le_result_t result = LE_FAULT;
    taf_gnss_NmeaBitMask_t nmeaMaskPtr;

    //33.GetSupportedNmeaSentences
    LE_TEST_INFO("GetSupportedNmeaSentences() API is called to get the list of"
        "supported NMEA sentences");
    result = taf_gnss_GetSupportedNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetSupportedNmeaSentences-LE_OK");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GPGGA)
        {
            LE_TEST_INFO("GPGGA Supported\n");
        }
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GPRMC)
        {
            LE_TEST_INFO("GPRMC Supported\n");
        }
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GNGSA)
        {
            LE_TEST_INFO("GNGSA Supported\n");
        }
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GPVTG)
        {
            LE_TEST_INFO("GPVTG Supported\n");
        }
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GPGNS)
        {
            LE_TEST_INFO("GPGNS Supported\n");
        }
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GPDTM)
        {
            LE_TEST_INFO("GPDTM Supported\n");
        }
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GPGSV)
        {
            LE_TEST_INFO("GPGSV Supported\n");
        }
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GLGSV)
        {
            LE_TEST_INFO("GLGSV Supported\n");
        }
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GAGSV)
        {
            LE_TEST_INFO("GAGSV Supported\n");
        }
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GQGSV)
        {
            LE_TEST_INFO("GQGSV Supported\n");
        }
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GBGSV)
        {
            LE_TEST_INFO("GBGSV Supported\n");
        }
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GIGSV)
        {
            LE_TEST_INFO("GIGSV Supported\n");
        }
    }
    else
    {
        LE_TEST_INFO("Failed! to get supported NMEA sentences\n");
    }

   //34. SetNmeaSentences - GPGGA
    LE_TEST_INFO("SetNmeaSentences() API is called to set GPGGA NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPGGA;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

   //35.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");
    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

    //36.GetSupportedNmeaSentences- LE_NOT_PERMITTED
    LE_TEST_INFO("GetSupportedNmeaSentences() API is called to get the list of"
        "supported NMEA sentences");
    result = taf_gnss_GetSupportedNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_GetSupportedNmeaSentences-LE_NOT_PERMITTED");

    //37. SetNmeaSentence - LE_NOT_PERMITTED
    LE_TEST_INFO("SetNmeaSentences() API is called to check whether it returns"
        "Not permitted or not");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPGGA;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_NOT_PERMITTED, "taf_gnss_SetNmeaSentences-LE_NOT_PERMITTED");

    //38.GetNmeaSentences- LE_OK
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GPGGA)
        {
            LE_TEST_INFO("GPGGA enabled\n");
        }
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //39.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //40. SetNmeaSentences - GPGGA
    LE_TEST_INFO("SetNmeaSentences() API is called to set GPGGA NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPGGA;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //41.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GPGGA)
        {
            LE_TEST_INFO("GPGGA enabled\n");
        }
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //42. SetNmeaSentence - GPRMC
    LE_TEST_INFO("SetNmeaSentences() API is called to set GPRMC NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPRMC;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to start reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //43.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GPRMC)
        {
            LE_TEST_INFO("GPRMC enabled\n");
        }
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //44. SetNmeaSentence - GNGSA
    LE_TEST_INFO("SetNmeaSentences() API is called to set GNGSA NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GNGSA;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //45.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GNGSA)
        {
            LE_TEST_INFO("GNGSA enabled\n");
        }
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //46.SetNmeaSentence - GPVTG
    LE_TEST_INFO("SetNmeaSentences() API is called to set GPVTG NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPVTG;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //47.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GPVTG)
        {
            LE_TEST_INFO("GPVTG enabled\n");
        }
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //48. SetNmeaSentence - GPGNS
    LE_TEST_INFO("SetNmeaSentences() API is called to set GPGNS NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPGNS;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //49.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GPGNS)
        {
            LE_TEST_INFO("GPGNS enabled\n");
        }
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //50. SetNmeaSentence - GPDTM
    LE_TEST_INFO("SetNmeaSentences() API is called to set GPDTM NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPDTM;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //51.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GPDTM)
        {
            LE_TEST_INFO("GPDTM enabled\n");
        }
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //52. SetNmeaSentence - GPGSV
    LE_TEST_INFO("SetNmeaSentences() API is called to set GPGSV NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPGSV;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //53.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GPGSV)
        {
            LE_TEST_INFO("GPGSV enabled\n");
        }
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //54. SetNmeaSentence - GLGSV
    LE_TEST_INFO("SetNmeaSentences() API is called to set GLGSV NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GLGSV;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //55.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GLGSV)
        {
            LE_TEST_INFO("GLGSV enabled\n");
        }
    }
    else if(result == LE_TIMEOUT)
    {
        LE_TEST_INFO("GLGSV NmeaSentence type is not being received\n");
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //56. SetNmeaSentence - GAGSV
    LE_TEST_INFO("SetNmeaSentences() API is called to set GAGSV NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GAGSV;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //57.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    if(result == LE_OK)
    {
        LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GAGSV)
        {
            LE_TEST_INFO("GAGSV enabled\n");
        }
    }
    else if(result == LE_TIMEOUT)
    {
        LE_TEST_OK(result==LE_TIMEOUT, "taf_gnss_GetNmeaSentences-LE_TIMEOUT");
        LE_TEST_INFO("GAGSV NmeaSentence type is not being received\n");
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //58. SetNmeaSentence - GQGSV
    LE_TEST_INFO("SetNmeaSentences() API is called to set GQGSV NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GQGSV;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //59.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GQGSV)
        {
            LE_TEST_INFO("GQGSV enabled\n");
        }
    }
    else if(result == LE_TIMEOUT)
    {
        LE_TEST_INFO("GQGSV NmeaSentence type is not being received\n");
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //60. SetNmeaSentence - GBGSV
    LE_TEST_INFO("SetNmeaSentences() API is called to set GBGSV NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GBGSV;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //61.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GBGSV)
        {
            LE_TEST_INFO("GBGSV enabled\n");
        }
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //62. SetNmeaSentence - GIGSV
    LE_TEST_INFO("SetNmeaSentences() API is called to set GIGSV NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GIGSV;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //63.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_TIMEOUT, "taf_gnss_GetNmeaSentences-LE_TIMEOUT");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GIGSV)
        {
            LE_TEST_INFO("GIGSV enabled\n");
        }
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //64. SetNmeaSentence - Combination of GIGSV,GBGSV&
    LE_TEST_INFO("SetNmeaSentences() API is called to set GIGSV NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GIGSV|TAF_GNSS_NMEA_MASK_GBGSV|TAF_GNSS_NMEA_MASK_GAGSV;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //65.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GIGSV)
        {
            LE_TEST_INFO("GIGSV enabled\n");
        }
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GBGSV)
        {
            LE_TEST_INFO("GBGSV enabled\n");
        }
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GAGSV)
        {
            LE_TEST_INFO("GAGSV enabled\n");
        }
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //66. SetNmeaSentence - 0xFFFFFFFF
    LE_TEST_INFO("SetNmeaSentences() API is called to set 0xFFFFFFFF NMEA sentence type");
    nmeaMaskPtr = 0xFFFFFFFF;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //67.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {

       LE_TEST_INFO("nmeaMaskPtr: %"PRIu64"\n",nmeaMaskPtr);

    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //SetNmeaSentence ->0
    LE_TEST_INFO("SetNmeaSentences() API is called to set 0- NMEA sentence type");
    nmeaMaskPtr = 0;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_BAD_PARAMETER, "taf_gnss_SetNmeaSentences-LE_BAD_PARAMETER");

    //SetNmeaSentence ->0x1000
    LE_TEST_INFO("SetNmeaSentences() API is called to set 0x1000- NMEA sentence type");
    nmeaMaskPtr = 0x1000;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //GetNmeaSentences- LE_TIMEOUT
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_TIMEOUT, "taf_gnss_GetNmeaSentences-LE_TIMEOUT");

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //SetNmeaSentence ->0x8000000
    LE_TEST_INFO("SetNmeaSentences() API is called to set 0x8000000 NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GGA;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {

       LE_TEST_INFO("nmeaMaskPtr: %"PRIu64"\n",nmeaMaskPtr);

    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //SetNmeaSentence ->0x10000000
    LE_TEST_INFO("SetNmeaSentences() API is called to set 0x10000000 NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_RMC;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {

       LE_TEST_INFO("nmeaMaskPtr: %"PRIu64"\n",nmeaMaskPtr);

    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }


    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //SetNmeaSentence ->0x20000000
    LE_TEST_INFO("SetNmeaSentences() API is called to set 0x20000000 NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GSA;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {

       LE_TEST_INFO("nmeaMaskPtr: %"PRIu64"\n",nmeaMaskPtr);

    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //SetNmeaSentence ->0x40000000
    LE_TEST_INFO("SetNmeaSentences() API is called to set 0x40000000 NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_VTG;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {

       LE_TEST_INFO("nmeaMaskPtr: %"PRIu64"\n",nmeaMaskPtr);

    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //SetNmeaSentence ->0x80000000
    LE_TEST_INFO("SetNmeaSentences() API is called to set 0x80000000 NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GNS;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {

       LE_TEST_INFO("nmeaMaskPtr: %"PRIu64"\n",nmeaMaskPtr);

    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //SetNmeaSentence ->0x100000000
    LE_TEST_INFO("SetNmeaSentences() API is called to set 0x100000000 NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_DTM;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");

    //Start
    LE_TEST_INFO("taf_gnss_Start() API is called to stop reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {

       LE_TEST_INFO("nmeaMaskPtr: %"PRIu64"\n",nmeaMaskPtr);

    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

   //68.Disable GNSS
    LE_TEST_INFO("taf_gnss_Disable() API is called to disable GNSS engine");
    result = taf_gnss_Disable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Disable-LE_OK");

    //69.GetNmeaSentences- LE_NOT_PERMITTED
    LE_TEST_INFO("GetNmeaSentences() API is called to check whether it returns"
        "Not permitted or not");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_NOT_PERMITTED, "taf_gnss_GetNmeaSentences-LE_NOT_PERMITTED");

    //70.Enable GNSS
    LE_TEST_INFO("taf_gnss_Enable() API is called to disable GNSS engine");
    result = taf_gnss_Enable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Enable-LE_OK");

}

static void TestTafGnssAcquisitionRate
(
    void
)
{
    le_result_t result = LE_FAULT;
    uint32_t acqRate;

    //71.SetAcquisitionRate
    LE_TEST_INFO("taf_gnss_SetAcquisitionRate() API is called to set Acq Rate 1");
    acqRate = 1;
    result = taf_gnss_SetAcquisitionRate(acqRate);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetAcquisitionRate-LE_OK");

    //72.GetAcquisitionRate
    LE_TEST_INFO("taf_gnss_GetAcquisitionRate() API is called to get Acq Rate");
    result = taf_gnss_GetAcquisitionRate(&acqRate);
    LE_TEST_OK(result == LE_OK,"taf_gnss_GetAcquisitionRate-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Acquisition Rate is %d\n",acqRate);
    }
    else
    {
        LE_TEST_INFO("Failed to get Acquisition Rate\n");
    }

    //73.SetAcquisitionRate
    LE_TEST_INFO("taf_gnss_SetAcquisitionRate() API is called to set Acq Rate 1001");
    acqRate = 101;
    result = taf_gnss_SetAcquisitionRate(acqRate);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetAcquisitionRate-LE_OK");

    //74.GetAcquisitionRate
    LE_TEST_INFO("taf_gnss_GetAcquisitionRate() API is called to get Acq Rate");
    result = taf_gnss_GetAcquisitionRate(&acqRate);
    LE_TEST_OK(result == LE_OK,"taf_gnss_GetAcquisitionRate-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Acquisition Rate is %d\n",acqRate);
    }
    else
    {
        LE_TEST_INFO("Failed to get Acquisition Rate\n");
    }

    //75.SetAcquisitionRate
    LE_TEST_INFO("taf_gnss_SetAcquisitionRate() API is called to set Acq Rate 0");
    acqRate = 0;
    result = taf_gnss_SetAcquisitionRate(acqRate);
    LE_TEST_OK(result == LE_OUT_OF_RANGE,"taf_gnss_SetAcquisitionRate-LE_OUT_OF_RANGE");

    //76.GetAcquisitionRate
    LE_TEST_INFO("taf_gnss_GetAcquisitionRate() API is called to get Acq Rate");
    result = taf_gnss_GetAcquisitionRate(&acqRate);
    LE_TEST_OK(result == LE_OK,"taf_gnss_GetAcquisitionRate-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Acquisition Rate is %d\n",acqRate);
    }
    else
    {
        LE_TEST_INFO("Failed to get Acquisition Rate\n");
    }

    //77.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    //78.SetAcquisitionRate-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_SetAcquisitionRate() API is called to check whether it returns"
        "Not Permitted or not");
    acqRate = 1;
    result = taf_gnss_SetAcquisitionRate(acqRate);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"taf_gnss_SetAcquisitionRate-LE_NOT_PERMITTED");

    //79.GetAcquisitionRate-LE_OK
    LE_TEST_INFO("taf_gnss_GetAcquisitionRate() API is called to get Acq Rate");
    result = taf_gnss_GetAcquisitionRate(&acqRate);
    LE_TEST_OK(result == LE_OK,"taf_gnss_GetAcquisitionRate-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Acquisition Rate is %d\n",acqRate);
    }
    else
    {
        LE_TEST_INFO("Failed to get Acquisition Rate\n");
    }

    //80.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //81.Disable GNSS
    LE_TEST_INFO("taf_gnss_Disable() API is called to disable GNSS engine");
    result = taf_gnss_Disable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Disable-LE_OK");

    //82.GetAcquisitionRate-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_GetAcquisitionRate() API is called to check whether it returns"
        "Not Permitted or not");
    result = taf_gnss_GetAcquisitionRate(&acqRate);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"taf_gnss_GetAcquisitionRate-LE_NOT_PERMITTED");

    //83.Enable GNSS
    LE_TEST_INFO("taf_gnss_Enable() API is called to disable GNSS engine");
    result = taf_gnss_Enable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Enable-LE_OK");

}

static void TestTafGnssSecBandConstellations
(
    void
)
{
    le_result_t result = LE_FAULT;
    uint32_t constellationSb=0;

    //84.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to start reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    //85.DefaultSecondaryBandConstellations-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_DefaultSecondaryBandConstellations() API is triggered to check whether"
        "it returns Not Permitted or not");
    result = taf_gnss_DefaultSecondaryBandConstellations();
    LE_TEST_OK(result == LE_NOT_PERMITTED,"DefaultSecondaryBandConstellations-LE_NOT_PERMITTED");

    //86.ConfigureSecondaryBandConstellations-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_ConfigureSecondaryBandConstellations() API is triggered to check whether"
        "it returns Not Permitted or not");
    constellationSb |= (1<<(TAF_GNSS_SB_CONSTELLATION_GPS-1));
    result = taf_gnss_ConfigureSecondaryBandConstellations(constellationSb);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"ConfigureSecondaryBandConstellations-LE_NOT_PERMITTED");

    //87.RequestSecondaryBandConstellations-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_RequestSecondaryBandConstellations() API is triggered to check whether"
        "it returns Not Permitted or not");
    result = taf_gnss_RequestSecondaryBandConstellations(&constellationSb);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"RequestSecondaryBandConstellations-LE_NOT_PERMITTED");

    //88.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //89.DefaultSecondaryBandConstellations
    LE_TEST_INFO("taf_gnss_DefaultSecondaryBandConstellations() API is triggered to set default"
        "Secondary Band Constellations");
    result = taf_gnss_DefaultSecondaryBandConstellations();
    LE_TEST_OK(result == LE_OK,"DefaultSecondaryBandConstellations-LE_OK");

    //90.RequestSecondaryBandConstellations
    LE_TEST_INFO("taf_gnss_RequestSecondaryBandConstellations() API is triggered to get secondary"
        "band constellations set");
    constellationSb = 0;
    result = taf_gnss_RequestSecondaryBandConstellations(&constellationSb);
    LE_TEST_INFO("taf_gnss_RequestSecondaryBandConstellations constellationSb: %d",constellationSb);
    LE_TEST_OK(result == LE_OK,"RequestSecondaryBandConstellations-LE_OK");
    if(result == LE_OK)
    {
        if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_GPS-1)))//1st bit
        {
            LE_TEST_INFO("GPS constellation is disabled \n");
        }
        if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_GALILEO-1)))//2nd bit
        {
            LE_TEST_INFO("GALILEO constellation is disabled \n");
        }
        if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_SBAS-1)))//3rd bit
        {
            LE_TEST_INFO("SBAS constellation is disabled \n");
        }
        if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_COMPASS-1)))//4th bit
        {
            LE_TEST_INFO("COMPASS constellation is disabled \n");
        }
        if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_GLONASS-1))) //5th bit
        {
            LE_TEST_INFO("GLONASS constellation is disabled \n");
        }
        if(constellationSb &(1<<(TAF_GNSS_SB_CONSTELLATION_BDS-1))) //6th bit
        {
            LE_TEST_INFO("BDS constellation is disabled \n");
        }
        if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_QZSS-1)))//7th bit
        {
            LE_TEST_INFO("QZAS constellation is disabled \n");
        }
        if(constellationSb &(1<<(TAF_GNSS_SB_CONSTELLATION_NAVIC-1)))//8th bit
        {
            LE_TEST_INFO("NAVIC constellation is disabled \n");
        }
    }
    else
    {
        LE_TEST_INFO("failed ! to get secondary band constellations");
    }

    //91.ConfigureSecondaryBandConstellations
    LE_TEST_INFO("taf_gnss_ConfigureSecondaryBandConstellations() API is triggered to"
        "configure/disable sec band constellations");
    constellationSb = 0;
    constellationSb |= (1<<(TAF_GNSS_SB_CONSTELLATION_GPS-1));
    constellationSb |= (1<<(TAF_GNSS_SB_CONSTELLATION_GALILEO-1));
    constellationSb |= (1<<(TAF_GNSS_SB_CONSTELLATION_SBAS-1));
    constellationSb |= (1<<(TAF_GNSS_SB_CONSTELLATION_COMPASS-1));
    constellationSb |= (1<<(TAF_GNSS_SB_CONSTELLATION_GLONASS-1));
    constellationSb |= (1<<(TAF_GNSS_SB_CONSTELLATION_BDS-1));
    constellationSb |= (1<<(TAF_GNSS_SB_CONSTELLATION_QZSS-1));
    constellationSb |= (1<<(TAF_GNSS_SB_CONSTELLATION_NAVIC-1));
    result = taf_gnss_ConfigureSecondaryBandConstellations(constellationSb);
    LE_TEST_OK(result == LE_OK,"ConfigureSecondaryBandConstellations-LE_OK");

   //92.RequestSecondaryBandConstellations
    LE_TEST_INFO("taf_gnss_RequestSecondaryBandConstellations() API is triggered to"
        "get secondary band constellations set");
    result = taf_gnss_RequestSecondaryBandConstellations(&constellationSb);
    LE_TEST_INFO("taf_gnss_RequestSecondaryBandConstellations constellationSb: %d",constellationSb);
    LE_TEST_OK(result == LE_OK,"RequestSecondaryBandConstellations-LE_OK");
    if(result == LE_OK)
    {
        if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_GPS-1)))//1st bit
        {
            LE_TEST_INFO("GPS constellation is disabled \n");
        }
        if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_GALILEO-1)))//2nd bit
        {
            LE_TEST_INFO("GALILEO constellation is disabled \n");
        }
        if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_SBAS-1)))//3rd bit
        {
            LE_TEST_INFO("SBAS constellation is disabled \n");
        }
        if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_COMPASS-1)))//4th bit
        {
            LE_TEST_INFO("COMPASS constellation is disabled \n");
        }
        if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_GLONASS-1))) //5th bit
        {
            LE_TEST_INFO("GLONASS constellation is disabled \n");
        }
        if(constellationSb &(1<<(TAF_GNSS_SB_CONSTELLATION_BDS-1))) //6th bit
        {
            LE_TEST_INFO("BDS constellation is disabled \n");
        }
        if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_QZSS-1)))//7th bit
        {
            LE_TEST_INFO("QZAS constellation is disabled \n");
        }
        if(constellationSb &(1<<(TAF_GNSS_SB_CONSTELLATION_NAVIC-1)))//8th bit
        {
            LE_TEST_INFO("NAVIC constellation is disabled \n");
        }
    }
    else
    {
        LE_TEST_INFO("failed ! to get secondary band constellations");
    }
}

static void TestTafGnssEngines
(
    void
)
{
    le_result_t result = LE_FAULT;
    int engineType;
    int engineState;
    taf_gnss_DrParams_t *drParamsPtr;
    DrFramePool = le_mem_CreatePool("DrframePool", sizeof(taf_gnss_DrParams_t));
    drParamsPtr = (taf_gnss_DrParams_t*) le_mem_ForceAlloc(DrFramePool);

    //93.SetDRConfig -Success
    LE_TEST_INFO("taf_gnss_SetDRConfig() API is triggered to configure Dead Reckoning Engine"
        "parameters");
    drParamsPtr->rollOffset = 1.2;
    drParamsPtr->yawOffset = 2.3;
    drParamsPtr->pitchOffset = 3.4;
    drParamsPtr->offsetUnc = 180.0;
    drParamsPtr->speedFactor = 1.0;
    drParamsPtr->speedFactorUnc = 0.0;
    drParamsPtr->gyroFactor = 1.0;
    drParamsPtr->gyroFactorUnc = 0.0;
    result = taf_gnss_SetDRConfig(drParamsPtr);
    LE_TEST_OK(result == LE_OK, "taf_gnss_SetDRConfig-LE_OK");

    //94.SetDRConfig -Failure
    drParamsPtr->offsetUnc = 180.1;
    result = taf_gnss_SetDRConfig(drParamsPtr);
    LE_TEST_OK(result == LE_FAULT, "taf_gnss_SetDRConfig-LE_FAULT");

    //95.Start
    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    //96.SetDRConfig -LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_SetDRConfig() API is triggered to check whether it returns"
        "Not permitted or not");
    result = taf_gnss_SetDRConfig(drParamsPtr);
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_SetDRConfig-LE_NOT_PERMITTED");

    //Free the DR reference
    le_mem_Release(drParamsPtr);

    //97.taf_gnss_ConfigureEngineState- SPE/SUSPEND
    engineType = 1;
    engineState = 1;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure SUSPEND state"
        "for SPE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-LE_FAULT");

    //98.taf_gnss_ConfigureEngineState- SPE/RESUME
    engineType = 1;
    engineState = 2;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure RESUME state"
        "for SPE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-LE_FAULT");

    //99.taf_gnss_ConfigureEngineState- PPE/SUSPEND
    engineType = 2;
    engineState = 1;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure SUSPEND state"
        "for PPE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-LE_FAULT");

    //100.taf_gnss_ConfigureEngineState- PPE/RESUME
    engineType = 2;
    engineState = 2;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure RESUME state"
        "for PPE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-LE_FAULT");

    //101.taf_gnss_ConfigureEngineState- DRE/SUSPEND
    engineType = 3;
    engineState = 1;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure SUSPEND state for"
        "DRE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-Not Supported");

    //102.taf_gnss_ConfigureEngineState- DRE/RESUME
    engineType = 3;
    engineState = 2;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure RESUME state for"
        "DRE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-Not Supported");

    //103.taf_gnss_ConfigureEngineState- VPE/SUSPEND
    engineType = 4;
    engineState = 1;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure SUSPEND state for"
        "VPE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-LE_FAULT");

    //104.taf_gnss_ConfigureEngineState- VPE/RESUME
    engineType = 4;
    engineState = 2;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure RESUME state for"
        "VPE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-LE_FAULT");

    //105.taf_gnss_ConfigureEngineState- Failure -Invalid Engine type
    engineType = 5;
    engineState = 1;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to check for failure scenario");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-LE_FAULT");

    //106.taf_gnss_ConfigureEngineState- Failure-Invalid Engine type
    engineType = 4;
    engineState = 3;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to check for failure scenario");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-LE_FAULT");

    //taf_gnss_ConfigureEngineState- Failure-Invalid Engine State
    engineType = 3;
    engineState = 0;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to check for failure scenario");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-LE_FAULT");

    //107.Stop
    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //108.taf_gnss_ConfigureEngineState- DRE/SUSPEND
    engineType = 3;
    engineState = 1;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure SUSPEND state for"
        "DRE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-Not Supported");

    //109.taf_gnss_ConfigureEngineState- DRE/RESUME
    engineType = 3;
    engineState = 2;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure RESUME state for"
        "DRE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-Not Supported");

    //110.Disable GNSS
    LE_TEST_INFO("taf_gnss_Disable() API is called to disable GNSS engine");
    result = taf_gnss_Disable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Disable-LE_OK");

    //111.taf_gnss_ConfigureEngineState- LE_NOT_PERMITTED
    engineType = 3;
    engineState = 1;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to check whether it returns"
        "LE_NOT_PERMITTED or not");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_NOT_PERMITTED,"taf_gnss_ConfigureEngineState-LE_NOT_PERMITTED");

    //112.Enable GNSS
    LE_TEST_INFO("taf_gnss_Enable() API is called to disable GNSS engine");
    result = taf_gnss_Enable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Enable-LE_OK");

}

static void TestTafLeverArmParams
(
    void
)
{

    le_result_t result = LE_FAULT;
    taf_gnss_LeverArmParams_t *leverArmParamsPtr;
    LevArmFramePool = le_mem_CreatePool("LevArmFramePool", sizeof(taf_gnss_LeverArmParams_t));
    leverArmParamsPtr = (taf_gnss_LeverArmParams_t*) le_mem_ForceAlloc(LevArmFramePool);

    leverArmParamsPtr->forwardOffsetMeters = 5.5;
    leverArmParamsPtr->sidewaysOffsetMeters = 1.2;
    leverArmParamsPtr->upOffsetMeters = 1.0;
    leverArmParamsPtr->levArmType = TAF_GNSS_LEVER_ARM_TYPE_GNSS_TO_VRP;
    LE_TEST_INFO("taf_gnss_SetLeverArmConfig() API is called to set Lever Arm paramaters");
    result = taf_gnss_SetLeverArmConfig(leverArmParamsPtr);
    LE_TEST_OK(result == LE_OK, "taf_gnss_SetLeverArmConfig-LE_OK");

    LE_TEST_INFO("taf_gnss_SetLeverArmConfig() 3 seconds of delay");
    le_thread_Sleep(3);
    leverArmParamsPtr->forwardOffsetMeters = 5.5;
    leverArmParamsPtr->sidewaysOffsetMeters = 1.2;
    leverArmParamsPtr->upOffsetMeters = 1.0;
    leverArmParamsPtr->levArmType = TAF_GNSS_LEVER_ARM_TYPE_DR_IMU_TO_GNSS;
    LE_TEST_INFO("taf_gnss_SetLeverArmConfig() API is called to set Lever Arm paramaters");
    result = taf_gnss_SetLeverArmConfig(leverArmParamsPtr);
    LE_TEST_OK(result == LE_FAULT, "taf_gnss_SetLeverArmConfig-LE_FAULT");

    LE_TEST_INFO("taf_gnss_SetLeverArmConfig() 3 seconds of delay");
    le_thread_Sleep(3);
    leverArmParamsPtr->forwardOffsetMeters = 5.5;
    leverArmParamsPtr->sidewaysOffsetMeters = 1.2;
    leverArmParamsPtr->upOffsetMeters = 1.0;
    leverArmParamsPtr->levArmType = TAF_GNSS_LEVER_ARM_TYPE_VPE_IMU_TO_GNSS;
    LE_TEST_INFO("taf_gnss_SetLeverArmConfig() API is called to set Lever Arm paramaters");
    result = taf_gnss_SetLeverArmConfig(leverArmParamsPtr);
    LE_TEST_OK(result == LE_FAULT, "taf_gnss_SetLeverArmConfig-LE_FAULT");

    //Failure case
    leverArmParamsPtr->levArmType = TAF_GNSS_LEVER_ARM_TYPE_VPE_IMU_TO_GNSS+1;
    LE_TEST_INFO("taf_gnss_SetLeverArmConfig() API is called to check whether it returns UNSUPPORTED or not");
    result = taf_gnss_SetLeverArmConfig(leverArmParamsPtr);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_gnss_SetLeverArmConfig-LE_BAD_PARAMETER");

    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    leverArmParamsPtr->levArmType = TAF_GNSS_LEVER_ARM_TYPE_VPE_IMU_TO_GNSS+1;
    LE_TEST_INFO("taf_gnss_SetLeverArmConfig() API is called to check whether it returns not permiited or not ");
    result = taf_gnss_SetLeverArmConfig(leverArmParamsPtr);
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_SetLeverArmConfig-LE_NOT_PERMITTED");

    //Free the Lever arm parameters reference
    le_mem_Release(leverArmParamsPtr);

    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

}

static void TestTafGnssRobustLocation
(
    void
)
{
    le_result_t result = LE_FAULT;
    uint8_t enable;
    uint8_t enabled911;
    uint8_t majorVersion;
    uint8_t minorVersion;


    //113.Configure Robust Locaiton - Enable/1 Enabled911/1
    LE_TEST_INFO("taf_gnss_ConfigureRobustLocation() API is called to configure"
        "Enable->1 Enabled911->1");
    enable = 1;
    enabled911 = 1;
    result = taf_gnss_ConfigureRobustLocation(enable,enabled911);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureRobustLocation-LE_OK");
    le_thread_Sleep(3);
    LE_INFO("TestTafGnssRobustLocation Wait for 3 seconds");

    //114.Robust Location information
    LE_TEST_INFO("taf_gnss_RobustLocationInformation() API is called to get robust"
        "location information");
    result = taf_gnss_RobustLocationInformation(&enable,&enabled911, &majorVersion,&minorVersion);
    LE_TEST_OK(result==LE_OK,"taf_gnss_RobustLocationInformation-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Robust Location Information Enable: %d", enable);
        LE_TEST_INFO("Robust Location Information enabled911: %d", enabled911);
        LE_TEST_INFO("Robust Location Information majorVersion number: %d", majorVersion);
        LE_TEST_INFO("Robust Location Information minorVersion number: %d", minorVersion);
    }
    else
    {
        LE_TEST_INFO("Failed! to get Robust Location Information");
    }

    //115.Configure Robust Location - Enable/1 Enabled911/0
    LE_TEST_INFO("taf_gnss_ConfigureRobustLocation() API is called to configure"
        "Enable->1 Enabled911->0");
    enable = 1;
    enabled911 = 0;
    result = taf_gnss_ConfigureRobustLocation(enable,enabled911);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureRobustLocation-LE_OK");
    le_thread_Sleep(3);
    LE_INFO("TestTafGnssRobustLocation Wait for 3 seconds");

    //116.Robust Location information
    LE_TEST_INFO("taf_gnss_RobustLocationInformation() API is called to get robust"
        "location information");
    result = taf_gnss_RobustLocationInformation(&enable,&enabled911, &majorVersion,&minorVersion);
    LE_TEST_OK(result==LE_OK,"taf_gnss_RobustLocationInformation-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Robust Location Information Enable: %d", enable);
        LE_TEST_INFO("Robust Location Information enabled911: %d", enabled911);
        LE_TEST_INFO("Robust Location Information majorVersion number: %d", majorVersion);
        LE_TEST_INFO("Robust Location Information minorVersion number: %d", minorVersion);
    }
    else
    {
        LE_TEST_INFO("Failed! to get Robust Location Information");
    }

    //117.Configure Robust Locaiton - Enable/0 Enabled911/1
    LE_TEST_INFO("taf_gnss_ConfigureRobustLocation() API is called to configure"
        "Enable->0 Enabled911->1");
    enable = 0;
    enabled911 = 1;
    result = taf_gnss_ConfigureRobustLocation(enable,enabled911);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureRobustLocation-LE_OK");
    le_thread_Sleep(3);
    LE_INFO("TestTafGnssRobustLocation Wait for 3 seconds");

    //118.Robust Location information
    LE_TEST_INFO("taf_gnss_RobustLocationInformation() API is called to get robust"
        "location information");
    result = taf_gnss_RobustLocationInformation(&enable,&enabled911, &majorVersion,&minorVersion);
    LE_TEST_OK(result==LE_OK,"taf_gnss_RobustLocationInformation-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Robust Location Information Enable: %d", enable);
        LE_TEST_INFO("Robust Location Information enabled911: %d", enabled911);
        LE_TEST_INFO("Robust Location Information majorVersion number: %d", majorVersion);
        LE_TEST_INFO("Robust Location Information minorVersion number: %d", minorVersion);
    }
    else
    {
        LE_TEST_INFO("Failed! to get Robust Location Information");
    }

   //119.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");
    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

    //120.Configure Robust Locaiton - Enable/0 Enabled911/1
    LE_TEST_INFO("taf_gnss_ConfigureRobustLocation() API is called to configure"
        "Enable->0 Enabled911->0");
    enable = 0;
    enabled911 = 0;
    result = taf_gnss_ConfigureRobustLocation(enable,enabled911);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureRobustLocation-LE_OK");

    //121.Robust Location information
    LE_TEST_INFO("taf_gnss_RobustLocationInformation() API is called to get"
        "robust location information");
    result = taf_gnss_RobustLocationInformation(&enable,&enabled911, &majorVersion,&minorVersion);
    LE_TEST_OK(result==LE_OK,"taf_gnss_RobustLocationInformation-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Robust Location Information Enable: %d", enable);
        LE_TEST_INFO("Robust Location Information enabled911: %d", enabled911);
        LE_TEST_INFO("Robust Location Information majorVersion number: %d", majorVersion);
        LE_TEST_INFO("Robust Location Information minorVersion number: %d", minorVersion);
    }
    else
    {
        LE_TEST_INFO("Failed! to get Robust Location Information");
    }

    //122.Configure Robust Locaiton - Enable/1 Enabled911/1
    LE_TEST_INFO("taf_gnss_ConfigureRobustLocation() API is called to configure"
        "Enable->1 Enabled911->1");
    enable = 1;
    enabled911 = 1;
    result = taf_gnss_ConfigureRobustLocation(enable,enabled911);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureRobustLocation-LE_OK");

    //123.Robust Location information
    LE_TEST_INFO("taf_gnss_RobustLocationInformation() API is called to get robust"
        "location information");
    result = taf_gnss_RobustLocationInformation(&enable,&enabled911, &majorVersion,&minorVersion);
    LE_TEST_OK(result==LE_OK,"taf_gnss_RobustLocationInformation-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Robust Location Information Enable: %d", enable);
        LE_TEST_INFO("Robust Location Information enabled911: %d", enabled911);
        LE_TEST_INFO("Robust Location Information majorVersion number: %d", majorVersion);
        LE_TEST_INFO("Robust Location Information minorVersion number: %d", minorVersion);
    }
    else
    {
        LE_TEST_INFO("Failed! to get Robust Location Information");
    }

    //Configure Robust Location - Enable/2 Enabled911/1
    LE_TEST_INFO("taf_gnss_ConfigureRobustLocation() API is called to configure"
        "Enable->2 Enabled911->1");
    enable = 2;
    enabled911 = 1;
    result = taf_gnss_ConfigureRobustLocation(enable,enabled911);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureRobustLocation-LE_FAULT");

    //Configure Robust Location - Enable/1 Enabled911/2
    LE_TEST_INFO("taf_gnss_ConfigureRobustLocation() API is called to configure"
        "Enable->1 Enabled911->2");
    enable = 1;
    enabled911 = 2;
    result = taf_gnss_ConfigureRobustLocation(enable,enabled911);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureRobustLocation-LE_FAULT");


   //124.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

   //125.Disable GNSS
    LE_TEST_INFO("taf_gnss_Disable() API is called to disable GNSS engine");
    result = taf_gnss_Disable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Disable-LE_OK");

    //126.Configure Robust Location - LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_ConfigureRobustLocation() API is called to check whether it returns"
        "Not permitted or not");
    enable = 1;
    enabled911 = 1;
    result = taf_gnss_ConfigureRobustLocation(enable,enabled911);
    LE_TEST_OK(result==LE_NOT_PERMITTED,"taf_gnss_ConfigureRobustLocation-LE_NOT_PERMITTED");

    //127.Robust Location information-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_RobustLocationInformation() API is called to check whether it returns"
        "Not permitted or not");
    result = taf_gnss_RobustLocationInformation(&enable,&enabled911, &majorVersion,&minorVersion);
    LE_TEST_OK(result==LE_NOT_PERMITTED,"taf_gnss_RobustLocationInformation-LE_NOT_PERMITTED");

    //128.Enable GNSS
    LE_TEST_INFO("taf_gnss_Enable() API is called to disable GNSS engine");
    result = taf_gnss_Enable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Enable-LE_OK");

}

static void TestTafGnssMinElevation
(
    void
)
{
    le_result_t result = LE_FAULT;
    uint8_t  minElevation;

    //129.taf_gnss_SetMinElevation -LE_OUT_OF_RANGE
    LE_TEST_INFO("taf_gnss_SetMinElevation() API is triggered to check whether it returns"
        " Out of range or not");
    minElevation = TAF_GNSS_MIN_ELEVATION_MAX_DEGREE+1;
    result = taf_gnss_SetMinElevation(minElevation);
    LE_TEST_OK(result == LE_OUT_OF_RANGE, "taf_gnss_SetMinElevation-LE_OUT_OF_RANGE");

    //130.taf_gnss_SetMinElevation
    LE_TEST_INFO("taf_gnss_SetMinElevation() API is triggered to set min SV elevation");
    minElevation = 50;
    result = taf_gnss_SetMinElevation(minElevation);
    LE_TEST_OK(result == LE_OK, "taf_gnss_SetMinElevation-LE_OK");
    le_thread_Sleep(3);
    LE_TEST_INFO("TestTafGnssMinElevation wait for 3 seconds");

    //131.taf_gnss_GetMinElevation
    LE_TEST_INFO("taf_gnss_GetMinElevation() API is triggered to get min SV elevation");
    result = taf_gnss_GetMinElevation(&minElevation);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetMinElevation-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("GetMinElevation : %d",minElevation);
    }
    else
    {
        LE_TEST_INFO("Failed to GetMinElevation ");
    }

    //132.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");
    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

    //133.taf_gnss_SetMinElevation-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_SetMinElevation() API is triggered to check whether it returns"
        " Not permitted or not");
    minElevation = 1;
    result = taf_gnss_SetMinElevation(minElevation);
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_SetMinElevation-LE_NOT_PERMITTED");

    //134.taf_gnss_GetMinElevation-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_GetMinElevation() API is triggered to check whether it returns"
        " Not permitted or not");
    result = taf_gnss_GetMinElevation(&minElevation);
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_GetMinElevation-LE_NOT_PERMITTED");

    //135.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");
    LE_TEST_INFO("wait for 5 seconds");
    le_thread_Sleep(5);
}

static void TestTafSetMinGpsWeek
(
    void
)
{
    uint16_t minGpsWeek = 1;

    le_result_t result = taf_gnss_SetMinGpsWeek(minGpsWeek);

    switch (result)
    {
        case LE_OK:
            LE_TEST_INFO("Success!\n");
            break;
        case LE_FAULT:
            LE_TEST_INFO("Failed to set the minimum GPS week\n");
            break;
        case LE_NOT_PERMITTED:
            LE_TEST_INFO("GNSS device is not in \"Ready\" state\n");
            break;
        default:
            LE_TEST_INFO("Invalid status\n");
            break;
    }
}

static void TestTafGetMinGpsWeek
(
    void
)
{
    uint16_t  minGpsWeek;
    le_result_t result = taf_gnss_GetMinGpsWeek(&minGpsWeek);

    switch (result)
    {
        case LE_OK:
            LE_TEST_INFO("Minimum GPS week: %d\n", minGpsWeek);
            break;
        case LE_FAULT:
            LE_TEST_INFO("Failed to get the minimum GPS week. See logs for details\n");
            break;
        case LE_NOT_PERMITTED:
            LE_TEST_INFO("GNSS device is not in \"Ready\" state\n");
            break;
        default:
            LE_TEST_INFO("Invalid status\n");
            break;
    }
}

static void TestTafGetCapabilities
(
    void
)
{
    uint64_t  locCapability;
    le_result_t result = taf_gnss_GetCapabilities(&locCapability);

    switch (result)
    {
        case LE_OK:
            LE_TEST_INFO("The location capabilities: %"PRIu64"\n", locCapability);
            break;
        case LE_FAULT:
            LE_TEST_INFO("Failed to get the location capabilities. See logs for details\n");
            break;
        default:
            LE_TEST_INFO("Invalid status\n");
            break;
    }
}

static void TestTafSetNmeaConfiguration
(
    void
)
{
    taf_gnss_NmeaBitMask_t nmeaMask = TAF_GNSS_NMEA_MASK_GPGGA | TAF_GNSS_NMEA_MASK_GPGNS | TAF_GNSS_NMEA_MASK_GPGSV
            | TAF_GNSS_NMEA_MASK_GLGSV | TAF_GNSS_NMEA_MASK_GAGSV;
    taf_gnss_GeodeticDatumType_t datumType = TAF_GNSS_GEODETIC_TYPE_WGS_84;
    uint16_t engineType = TAF_GNSS_LOC_ENGINE_FUSED;

    le_result_t result = taf_gnss_SetNmeaConfiguration(nmeaMask, datumType, engineType);

    switch (result)
    {
        case LE_OK:
            LE_TEST_INFO("Successfully enabled the NMEA!\n");
            break;
        case LE_FAULT:
            LE_TEST_INFO("Failed to set NMEA. See logs for details\n");
            break;
        case LE_BAD_PARAMETER:
            LE_TEST_INFO("Failed to set NMEA, incompatible bit mask\n");
            break;
       case LE_NOT_PERMITTED:
            LE_TEST_INFO("SetNmea: GNSS is not in ready state!\n");
            break;
        default:
            LE_TEST_INFO("Failed to set NMEA, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }
}

static void TestTafPosHandler
(
    void
)
{
    le_result_t result;
    int32_t  latitude;
    int32_t  longitude;
    int32_t  hAccuracy;
    int32_t  altitude;
    int32_t  vAccuracy;
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hours;
    uint16_t minutes;
    uint16_t seconds;
    uint16_t milliseconds;
    uint32_t hSpeed;
    uint32_t hSpeedAccuracy;
    int32_t  vSpeed;
    int32_t  vSpeedAccuracy;
    uint32_t direction;
    uint32_t directionAccuracy = 0;
    taf_gnss_FixState_t fixState;
    uint32_t acquisitionRate = 0;
    taf_posCtrl_ActivationRef_t activationRef;

    //157.taf_posCtrl_Request
    LE_TEST_INFO("taf_posCtrl_Request() API is called to get positioning services");
    activationRef = taf_posCtrl_Request();
    LE_TEST_OK((activationRef!=NULL),"taf_posCtrl_Request-LE_OK");

    //taf_gnss_Start() This will trigger startDetailedEngineReports() TelSDK API
    //LE_TEST_OK(((taf_gnss_Start()) == LE_OK), "taf_gnss_Start-LE_OK");
    LE_TEST_INFO("Wait for 5 seconds");
    le_thread_Sleep(5);

    //158.taf_pos_Get2DLocation
    LE_TEST_INFO("taf_pos_Get2DLocation() API is called to get 2D location information");
    result = taf_pos_Get2DLocation(&latitude, &longitude, &hAccuracy);
    LE_TEST_OK((LE_OK == result) || (LE_OUT_OF_RANGE == result),"taf_pos_Get2DLocation -LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Latitude(positive->north) : %.6f\n",(float)latitude/1e6);
        LE_TEST_INFO("Longitude(positive->east) : %.6f\n",(float)longitude/1e6);
        LE_TEST_INFO("hAccuracy                 : %.2fm\n",(float)hAccuracy/1e2);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("Location invalid [%d, %d, %d]\n",
               latitude,
               longitude,
               hAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed! to get 2D Location information\n");
    }

    //159. Get Date
    LE_TEST_INFO("taf_pos_GetDate is triggered to get date information");
    result = taf_pos_GetDate(&year, &month, &day);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),"taf_pos_GetDate-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("taf_pos_GetDate-> year.%d, month.%d, day.%d",year, month, day);
    }
    else
    {
        LE_TEST_INFO("Failed to get Pos Date\n");
    }

    //160.Get Time
    LE_TEST_INFO("taf_pos_GetDate is triggered to get time information");
    result = taf_pos_GetTime(&hours, &minutes, &seconds, &milliseconds);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),"taf_pos_GetTime-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_INFO("taf_pos_GetTime: hours.%d, minutes.%d, seconds.%d, milliseconds.%d",
            hours, minutes, seconds,milliseconds);
    }
    else
    {
        LE_TEST_INFO("Failed to get Pos Time\n");
    }

    //161.taf_pos_GetFixState
    LE_TEST_INFO("taf_pos_GetFixState is triggered to get time Fix state");
    result = taf_pos_GetFixState(&fixState);
    LE_TEST_OK(result == LE_OK,"taf_pos_GetFixState-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Position fix state: %s", (TAF_GNSS_STATE_FIX_NO_POS == fixState)?"No Fix"
                                     :(TAF_GNSS_STATE_FIX_2D == fixState)?"2D Fix"
                                     :(TAF_GNSS_STATE_FIX_3D == fixState)?"3D Fix"
                                     :(TAF_GNSS_STATE_FIX_ESTIMATED == fixState)?"Estimated Fix"
                                     : "Unknown");
    }
    else
    {
        LE_TEST_INFO("Failed to get pos Fix state\n");
    }

    //162.taf_pos_GetMotion
    LE_TEST_INFO("taf_pos_GetMotion() API is triggered to get heading information\n");
    result = taf_pos_GetMotion(&hSpeed, &hSpeedAccuracy, &vSpeed, &vSpeedAccuracy);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),"taf_pos_GetMotion-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_INFO("taf_pos_GetMotion hSpeed:%u, hSpeedAccuracy:%u, vSpeed:%d, vSpeedAccuracy:%d",
            hSpeed, hSpeedAccuracy, vSpeed, vSpeedAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed | to get Motion information\n");
    }

    //163.taf_pos_GetDirection
    LE_TEST_INFO("taf_pos_GetDirection() API is triggered to get direction information\n");
    result = taf_pos_GetDirection(&direction, &directionAccuracy);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),"taf_pos_GetDirection-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("taf_pos_GetDirection: direction.%u, directionAccuracy.%u",
            direction, directionAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed | to get Direction information\n");
    }

    //164.taf_pos_SetAcquisitionRate
    LE_TEST_INFO("taf_pos_SetAcquisitionRate() is triggered to check whether it returns"
        "LE_UNSUPPORTED or not\n");
    acquisitionRate = 3000;
    result = taf_pos_SetAcquisitionRate(acquisitionRate);
    LE_TEST_INFO("taf_pos_SetAcquisitionRate()-> result: %d\n",result);
    LE_TEST_OK((result == LE_OK),"taf_pos_SetAcquisitionRate-LE_OK");

    //165.taf_pos_GetAcquisitionRate
    LE_TEST_INFO("taf_pos_SetAcquisitionRate() is triggered to get Aquisition rate\n");
    acquisitionRate = taf_pos_GetAcquisitionRate();
    LE_TEST_INFO("taf_pos_SetAcquisitionRate()-> Aquisition rate: %d\n",acquisitionRate);
    LE_TEST_OK(((3000 == acquisitionRate) || (DEFAULT_ACQUISITION_RATE == acquisitionRate)),
        "taf_pos_GetAcquisitionRate-LE_OK");

    //166.taf_pos_Get3DLocation
    LE_TEST_INFO("taf_pos_Get3DLocation() is triggered to get 3D location information\n");
    result = taf_pos_Get3DLocation(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)),"taf_pos_Get3DLocation-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("taf_pos_Get3DLocation latitude:%.6f, longitude:%.6f, hAccuracy:%d, altitude:%.3f"
            ", vAccuracy:%d",latitude/1e6, longitude/1e6, hAccuracy, altitude/1e3, vAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed to get Pos 3D location information\n");
    }

    //167.taf_pos_SetDistanceResolution()-LE_BAD_PARAMETER
    LE_TEST_INFO("taf_pos_SetDistanceResolution() is triggered to set distance resolution\n");
    result = taf_pos_SetDistanceResolution(TAF_POS_RES_UNKNOWN);
    LE_TEST_OK((result == LE_BAD_PARAMETER),"taf_pos_SetDistanceResolution-LE_BAD_PARAMETER");

    //168.taf_pos_SetDistanceResolution
    LE_TEST_INFO("taf_pos_SetDistanceResolution() is triggered to set TAF_POS_RES_METER\n");
    result = taf_pos_SetDistanceResolution(TAF_POS_RES_METER);
    LE_TEST_OK((result == LE_OK),"taf_pos_SetDistanceResolution-LE_OK");

    //169.taf_pos_SetDistanceResolution
    LE_TEST_INFO("taf_pos_SetDistanceResolution() is triggered to set TAF_POS_RES_DECIMETER\n");
    result = taf_pos_SetDistanceResolution(TAF_POS_RES_DECIMETER);
    LE_TEST_OK((result == LE_OK),"taf_pos_SetDistanceResolution-LE_OK");

    //170.taf_pos_SetDistanceResolution
    LE_TEST_INFO("taf_pos_SetDistanceResolution() is triggered to set TAF_POS_RES_CENTIMETER\n");
    result = taf_pos_SetDistanceResolution(TAF_POS_RES_CENTIMETER);
    LE_TEST_OK((result == LE_OK),"taf_pos_SetDistanceResolution-LE_OK");

    //171.taf_pos_SetDistanceResolution
    LE_TEST_INFO("taf_pos_SetDistanceResolution() is triggered to set TAF_POS_RES_MILLIMETER\n");
    result = taf_pos_SetDistanceResolution(TAF_POS_RES_MILLIMETER);
    LE_TEST_OK((result == LE_OK),"taf_pos_SetDistanceResolution-LE_OK");

    //172.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting GNSS fixes");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_DUPLICATE, "taf_gnss_Stop-LE_DUPLICATE");//lsc

    LE_INFO("Release the positioning service");
    taf_posCtrl_Release(activationRef);

}

static void TestTafGnssStartMode
(
    void
)
{
    le_result_t result = LE_FAULT;
    uint32_t ttff = 0;

    //184.Disable
    LE_TEST_INFO("taf_gnss_Disable() is triggered to disable Engine state\n");
    result = taf_gnss_Disable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Disable-LE_OK");

    //185.StartMode -Hot Start/LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in  hot mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_HOT_START);
    LE_TEST_OK((result == LE_NOT_PERMITTED),
               "taf_gnss_StartMode-LE_NOT_PERMITTED");

    //186.StartMode -Warm Start/LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in  Warm mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_WARM_START);
    LE_TEST_OK((result == LE_NOT_PERMITTED),
               "taf_gnss_StartMode-LE_NOT_PERMITTED");

    //187.StartMode -Warm Start/LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in cold mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_COLD_START);
    LE_TEST_OK((result == LE_NOT_PERMITTED),
               "taf_gnss_StartMode-LE_NOT_PERMITTED");

    //188.StartMode -Factory Start/LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in factory mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_FACTORY_START);
    LE_TEST_OK((result == LE_NOT_PERMITTED),
               "taf_gnss_StartMode-LE_NOT_PERMITTED");

    //189.StartMode -UNKNOWN/LE_BAD_PARAMETER
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in factory mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_UNKNOWN_START);
    LE_TEST_OK((result == LE_BAD_PARAMETER),"taf_gnss_StartMode-LE_BAD_PARAMETER");

    //190.Enable
    LE_TEST_INFO("taf_gnss_Disable() is triggered to Enable Engine state\n");
    result = taf_gnss_Enable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Enable-LE_OK");

    //191.StartMode -Hot/LE_OK
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in hot mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_HOT_START);
    LE_TEST_OK((result == LE_OK),"taf_gnss_StartMode-LE_OK");
    LE_TEST_INFO("Wait for 10 seconds");
    le_thread_Sleep(10);

    //GetTtff - to receive the latest ttff value after setting startMode
    LE_TEST_INFO("taf_gnss_GetTtff() API is called to get time to first fix");
    result = taf_gnss_GetTtff(&ttff);
    if(result == LE_OK)
    {
        LE_TEST_INFO("TTFF start = %d msec", ttff);
    }
    else
    {
        LE_TEST_INFO("TTFF start not available");
    }

    //192.StartMode -Hot/LE_DUPLICATE
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in hot mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_HOT_START);
    LE_TEST_OK((result == LE_DUPLICATE),"taf_gnss_StartMode-LE_DUPLICATE");

    //193.StartMode -Warm/LE_DUPLICATE
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in Warm mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_WARM_START);
    LE_TEST_OK((result == LE_DUPLICATE),"taf_gnss_StartMode-LE_DUPLICATE");

    //194.Stop
    LE_TEST_INFO("taf_gnss_Stop() is triggered to Enable Engine state\n");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //195.StartMode -Warm/LE_OK
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in Warm mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_WARM_START);
    LE_TEST_OK((result == LE_OK),"taf_gnss_StartMode-LE_OK");
    LE_TEST_INFO("Wait for 30 seconds");
    le_thread_Sleep(30);

    //196.StartMode -Cold/LE_DUPLICATE
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in Cold mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_COLD_START);
    LE_TEST_OK((result == LE_DUPLICATE),"taf_gnss_StartMode-LE_DUPLICATE");

    //197.Stop
    LE_TEST_INFO("taf_gnss_Stop() is triggered to Enable Engine state\n");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //198.StartMode -Cold/LE_OK
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in Cold mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_COLD_START);
    LE_TEST_OK((result == LE_OK),"taf_gnss_StartMode-LE_OK");
    LE_TEST_INFO("Wait for 30 seconds");
    le_thread_Sleep(30);

    //199.StartMode -Factory/LE_DUPLICATEm
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in Factory mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_FACTORY_START);
    LE_TEST_OK((result == LE_DUPLICATE),"taf_gnss_StartMode-LE_DUPLICATE");

    //200.Stop
    LE_TEST_INFO("taf_gnss_Stop() is triggered to Enable Engine state\n");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //201.StartMode -Factory/LE_OK
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in Factory mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_FACTORY_START);
    LE_TEST_OK((result == LE_OK),"taf_gnss_StartMode-LE_OK");
    LE_TEST_INFO("Wait for 60 seconds");
    le_thread_Sleep(60);

    //202.Stop
    LE_TEST_INFO("taf_gnss_Stop() is triggered to Enable Engine state\n");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

}

static void TestTafGnssXtraInformation
(
    void
)
{
    taf_gnss_XtraStatusParams_t *XtraParamsPtr;
    le_result_t result = LE_FAULT;
    le_mem_PoolRef_t XtraFramePool = NULL;
    XtraFramePool = le_mem_CreatePool("XtraFramePool", sizeof(taf_gnss_XtraStatusParams_t));
    XtraParamsPtr = (taf_gnss_XtraStatusParams_t*) le_mem_ForceAlloc(XtraFramePool);

    if(XtraParamsPtr != NULL)
    {
        LE_TEST_INFO("taf_gnss_GetXtraStatus API is called to get xtra information");
        result = taf_gnss_GetXtraStatus(XtraParamsPtr);
    }
    else
    {
        LE_TEST_INFO("XtraParamPtr is NULL pointer");
    }
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetXtraStatus-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("**** Request Xtra Status Info ****\n");
        LE_TEST_INFO("GetXtraStatus featureEnabled:%d",XtraParamsPtr->featureEnabled);
        if(XtraParamsPtr->xtraDataStatus == TAF_GNSS_XTRA_DATA_STATUS_UNKNOWN)
        {
            LE_TEST_INFO("GetXtraStatus xtraDataStatus:Unknown");
        }
        else if(XtraParamsPtr->xtraDataStatus == TAF_GNSS_XTRA_DATA_STATUS_NOT_AVAIL)
        {
            LE_TEST_INFO("GetXtraStatus xtraDataStatus:Not available");
        }
        else if(XtraParamsPtr->xtraDataStatus == TAF_GNSS_XTRA_DATA_STATUS_NOT_VALID)
        {
            LE_TEST_INFO("GetXtraStatus xtraDataStatus:Not valid");
        }
        else if(XtraParamsPtr->xtraDataStatus == TAF_GNSS_XTRA_DATA_STATUS_VALID)
        {
            LE_TEST_INFO("GetXtraStatus xtraDataStatus:Valid");
        }
    }
    else
    {
        LE_TEST_INFO("taf_gnss_GetXtraStatus failed to get xtra status");
    }
    LE_TEST_INFO("GetXtraStatus xtraValidForHours:%d\n",XtraParamsPtr->xtraValidForHours);

    //release the memory
    le_mem_Release(XtraParamsPtr);
}

static void TestTafGnssRestart
(
    void
)
{
    le_result_t result = LE_FAULT;
    uint64_t gpsTime;
    int32_t currentLeapSeconds;
    uint64_t changeEventTime;
    int32_t nextLeapSeconds;
    uint32_t ttff = 0;

   //203.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");
    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

   //204.Force Warm Restart
    LE_TEST_INFO("taf_gnss_ForceWarmRestart() API is called to perform"
        " Warm restart of GNSS engine");
    result = taf_gnss_ForceWarmRestart();
    LE_TEST_OK(result == LE_OK, "taf_gnss_ForceWarmRestart-LE_OK");
    LE_TEST_INFO("Wait for 30 seconds to get fixes");
    le_thread_Sleep(30);

    //GetTtff - to receive the latest ttff value after performing warm restart
    LE_TEST_INFO("taf_gnss_GetTtff() to get ttff value after warm restart");
    result = taf_gnss_GetTtff(&ttff);
    if(result == LE_OK)
    {
        LE_TEST_INFO("TTFF start = %d msec", ttff);
    }
    else
    {
        LE_TEST_INFO("TTFF start not available");
    }

   //205.Force Cold Restart
    LE_TEST_INFO("taf_gnss_ForceColdRestart() API is called to perform"
        " Cold restart of GNSS engine");
    result = taf_gnss_ForceColdRestart();
    LE_TEST_OK(result == LE_OK, "taf_gnss_ForceColdRestart-LE_OK");
    LE_TEST_INFO("Wait for 60 seconds to get fixes");
    le_thread_Sleep(60);

    //GetTtff - to receive the latest ttff value after performing cold restart
    LE_TEST_INFO("taf_gnss_GetTtff() to get ttff value after cold restart");
    result = taf_gnss_GetTtff(&ttff);
    if(result == LE_OK)
    {
        LE_TEST_INFO("TTFF start = %d msec", ttff);
    }
    else
    {
        LE_TEST_INFO("TTFF start not available");
    }

   //206.Force Hot Restart
    LE_TEST_INFO("taf_gnss_ForceHotRestart() API is called to perform"
        " Warm restart of GNSS engine");
    result = taf_gnss_ForceHotRestart();
    LE_TEST_OK(result == LE_OK, "taf_gnss_ForceHotRestart-LE_OK");
    LE_TEST_INFO("Wait for 10 seconds to get fixes");
    le_thread_Sleep(10);

    //GetTtff - to receive the latest ttff value after performing hot restart
    LE_TEST_INFO("taf_gnss_GetTtff() to get ttff value after hot restart");
    result = taf_gnss_GetTtff(&ttff);
    if(result == LE_OK)
    {
        LE_TEST_INFO("TTFF start = %d msec", ttff);
    }
    else
    {
        LE_TEST_INFO("TTFF start not available");
    }

    //Get Leap Seconds-LE_OK
    LE_TEST_INFO("taf_gnss_GetLeapSeconds() API is triggerred to get Leap Seconds");
    result =taf_gnss_GetLeapSeconds(&gpsTime,&currentLeapSeconds,&changeEventTime,&nextLeapSeconds);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetLeapSeconds-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("taf_gnss_GetLeapSeconds gpsTime = %" PRIu64 " msec", gpsTime);
        LE_TEST_INFO("taf_gnss_GetLeapSeconds currentLeapSeconds = %d msec", currentLeapSeconds);
        LE_TEST_INFO("taf_gnss_GetLeapSeconds changeEventTime = %" PRIu64 " msec", changeEventTime);
        LE_TEST_INFO("taf_gnss_GetLeapSeconds nextLeapSeconds = %d msec", nextLeapSeconds);
    }
    else
    {
        LE_TEST_INFO("taf_gnss_GetLeapSeconds is failed");
    }

   //207.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

   //208.Force Warm Restart-Not Permitted
    LE_TEST_INFO("taf_gnss_ForceWarmRestart() API is called to check whether it returns"
        " Not permitted state or not");
    result = taf_gnss_ForceWarmRestart();
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_ForceWarmRestart-LE_NOT_PERMITTED");

   //209.Force Cold Restart-Not Permitted
    LE_TEST_INFO("taf_gnss_ForceColdRestart() API is called to check whether it returns"
        " Not permitted state or not");
    result = taf_gnss_ForceColdRestart();
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_ForceColdRestart-LE_NOT_PERMITTED");

   //210.Force Hot Restart- Not permitted
    LE_TEST_INFO("taf_gnss_ForceHotRestart() API is called to check whether it returns"
        " Not permitted state or not");
    result = taf_gnss_ForceHotRestart();
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_ForceHotRestart-LE_NOT_PERMITTED");

   //211.Force Factory Restart- Not Supported
    LE_TEST_INFO("taf_gnss_ForceFactoryRestart() API is called to check whether it returns"
        " Not supported state or not");
    result = taf_gnss_ForceFactoryRestart();
    LE_TEST_OK(result == LE_UNSUPPORTED, "taf_gnss_ForceFactoryRestart-LE_UNSUPPORTED");

    //212. Get Leap Seconds-LE_OK
    LE_TEST_INFO("taf_gnss_GetLeapSeconds() API is triggerred to get Leap Seconds");
    result =taf_gnss_GetLeapSeconds(&gpsTime,&currentLeapSeconds,&changeEventTime,&nextLeapSeconds);
    LE_TEST_OK(result == LE_OK, "taf_gnss_GetLeapSeconds-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("taf_gnss_GetLeapSeconds gpsTime = %" PRIu64 " msec", gpsTime);
        LE_TEST_INFO("taf_gnss_GetLeapSeconds currentLeapSeconds = %d msec", currentLeapSeconds);
        LE_TEST_INFO("taf_gnss_GetLeapSeconds changeEventTime = %" PRIu64 " msec", changeEventTime);
        LE_TEST_INFO("taf_gnss_GetLeapSeconds nextLeapSeconds = %d msec", nextLeapSeconds);
    }
    else
    {
        LE_TEST_INFO("taf_gnss_GetLeapSeconds is failed");
    }


}

COMPONENT_INIT
{
   PositionHandlerSem = le_sem_Create("PosHandlerSem", 0);

   LE_TEST_INFO("======== TestTafSetMinGpsWeek APIs Test  ========");
   TestTafSetMinGpsWeek();

   LE_TEST_INFO("Wait for 10 seconds to apply minGpsWeek by engin");
   le_thread_Sleep(10);

   LE_TEST_INFO("======== TestTafGetMinGpsWeek APIs Test  ========");
   TestTafGetMinGpsWeek();

   LE_TEST_INFO("======== TestTafSetNmeaConfiguration APIs Test  ========");
   TestTafSetNmeaConfiguration();

   LE_TEST_INFO("Wait for 5 seconds to apply Nmea");
   le_thread_Sleep(5);

   LE_TEST_INFO("======== TestTafGetCapabilities APIs Test  ========");
   TestTafGetCapabilities();

   LE_TEST_INFO("======== TestTafGnssStart APIs Test  ========");
   TestTafGnssStart();

   LE_TEST_INFO("======== TestTafGnssStartType APIs Test  ========");
   TestTafGnssStartType();

   LE_TEST_INFO("========= TestTafGnssConstellations APIs Test ===");
   TestTafGnssConstellations();

   LE_TEST_INFO("=========TestTafGnssNmeaSentences APIs Test=====");
   TestTafGnssNmeaSentences();

   LE_TEST_INFO("=========TestTafGnssAcquisitionRate APIs Test=====");
   TestTafGnssAcquisitionRate();

   LE_TEST_INFO("====TestTafGnssSecBandConstellations APIs Test====");
   TestTafGnssSecBandConstellations();

   LE_TEST_INFO("====TestTafGnssEngines APIs Test====");
   TestTafGnssEngines();

   LE_TEST_INFO("====TestTafLeverArmParams Test====");
   TestTafLeverArmParams();

   LE_TEST_INFO("====TestTafGnssRobustLocation APIs Test====");
   TestTafGnssRobustLocation();

   LE_TEST_INFO("====TestTafGnssMinElevation APIs Test====");
   TestTafGnssMinElevation();

   LE_TEST_INFO("======== GNSS Location information APIs Test  ========");
   TestTafGnssPositionHandler();

   LE_TEST_INFO("======== GNSS NMEA handler Test  ========");
   TestTafGnssNmeaHandler();

   LE_TEST_INFO("======== GNSS Capability handler Test  ========");
   TestTafGnssCapabilityChangeHandler();

   LE_TEST_INFO("==== GNSS Position information APIs Test====");
   TestTafPosHandler();

   LE_TEST_INFO("==== GNSS Sample Position information APIs Test====");
   TestTafSamplePositionHandler();

   LE_TEST_INFO("====TestTafGnssStartMode APIs Test====");
   TestTafGnssStartMode();

   LE_TEST_INFO("======== TestTafGnssXtraInformation ======");
   TestTafGnssXtraInformation();

   LE_TEST_INFO("======== TestTafGnssRestart ======");
   TestTafGnssRestart();

   LE_TEST_INFO("======== LE_TEST_EXIT  ========");
   LE_TEST_EXIT;
}