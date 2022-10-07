/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

static le_sem_Ref_t PositionHandlerSem;
static taf_gnss_PositionHandlerRef_t PositionHandlerRef = NULL;
static taf_pos_MovementHandlerRef_t  SamplePositionHandlerRef = NULL;
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
    static const char *tabDop[] =
    {
        "Position dilution of precision (PDOP)",
        "Horizontal dilution of precision (HDOP)",
        "Vertical dilution of precision (VDOP)",
        "Geometric dilution of precision (GDOP)",
        "Time dilution of precision (TDOP)"
    };

    //118.GetPositionState
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
        LE_TEST_INFO("Position state: %s", (TAF_GNSS_STATE_FIX_NO_POS == state)?"No Fix"
                                     :(TAF_GNSS_STATE_FIX_2D == state)?"2D Fix"
                                     :(TAF_GNSS_STATE_FIX_3D == state)?"3D Fix"
                                     : "Unknown");
    }
    else
    {
        LE_TEST_INFO("Failed to get position state");
    }

    //119.Get Location
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

    //120.Get Direction
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

    //121.Get Altitude
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

    //122.Get Horizontal Speed
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

    //123.Get Vertical Speed
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

    //124. Get GPS Leap Seconds
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

    //125. Get Date
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

    //126. Get Time
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

    //127. Satellite Information
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

    //128.Get Time Accuracy
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

    //129.Get Epoch Time
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

    //130.Set DOP Resolution -LE_BAD_PARAMETER
    LE_TEST_INFO("SetDopResolution() API is triggerred to set DOP resolution");
    result = taf_gnss_SetDopResolution(TAF_GNSS_RES_UNKNOWN);
    LE_TEST_OK(result==LE_BAD_PARAMETER,"taf_gnss_SetDopResolution-LE_BAD_PARAMETER");

    //131. Set DOP & Get Dilution of Precision (150-189)
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

    //132.Get GPS time
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

    //133.Get Satellite Status
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

    //134. Get the Magnetic deviation
    LE_TEST_INFO("taf_gnss_GetMagneticDeviation() is triggered to get the magnetic deviation\n");
    result = taf_gnss_GetMagneticDeviation( positionSampleRef, &magneticDeviation);
    LE_TEST_OK((result == LE_OK), "taf_gnss_GetMagneticDeviation-LE_OK");
    if (LE_OK == result)
    {
        LE_TEST_INFO("magnetic deviation %d", magneticDeviation/10);
    }
    else
    {
        LE_TEST_INFO("magnetic deviation unknown [%d]",magneticDeviation);
    }

    //135. Get elliptical uncertainity
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

    //Stop
    //LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting GNSS fixes");
    //LE_TEST_OK(taf_gnss_Stop() == LE_OK, "taf_gnss_Stop-LE_OK");

    LE_TEST_INFO("taf_gnss_ReleaseSampleRef is triggered");
    taf_gnss_ReleaseSampleRef(positionSampleRef);
    le_sem_Post(PositionHandlerSem);
    //exit(result);

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

    //155.taf_pos_sample_GetFixState()
    LE_TEST_INFO("taf_pos_sample_GetFixState() API is triggered to position fix state");
    result = taf_pos_sample_GetFixState(positionSampleRef, &fixState);
    LE_TEST_OK((result == LE_OK),"taf_pos_sample_GetFixState-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Sample Position fix state:%s",(TAF_GNSS_STATE_FIX_NO_POS == fixState)?"No Fix"
                                     :(TAF_GNSS_STATE_FIX_2D == fixState)?"2D Fix"
                                     :(TAF_GNSS_STATE_FIX_3D == fixState)?"3D Fix"
                                     : "Unknown");
    }
    else
    {
        LE_TEST_INFO("Failed to get sample position fix state\n");
    }

    //156.taf_pos_sample_Get2DLocation()
    LE_TEST_INFO("taf_pos_sample_Get2DLocation() API is triggered to get 2D location information ");
    result = taf_pos_sample_Get2DLocation(positionSampleRef, &lati, &longi, &accuracy);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),
        "taf_pos_sample_Get2DLocation-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_INFO("taf_pos_sample_Get2DLocation: lat.%d, long.%d, accuracy.%d", longi, lati,accuracy);
    }
    else
    {
        LE_TEST_INFO("Failed to get position sample 2D location\n");
    }

    //157.taf_pos_sample_GetDate()
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

    //158.taf_pos_sample_GetTime()
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

    //159.taf_pos_sample_GetAltitude()
    LE_TEST_INFO("taf_pos_sample_GetAltitude() API is triggered to get pos sample a information\n");
    result = taf_pos_sample_GetAltitude(positionSampleRef, &alti, &altaccuracy);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),
        "taf_pos_sample_GetAltitude-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("GetAltitude: alt.%d, accuracy.%d", alti, altaccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed to get position sample altitude information\n");
    }

    //160.taf_pos_sample_GetHorizontalSpeed()
    LE_TEST_INFO("taf_pos_sample_GetHorizontalSpeed() API is triggered to get pos"
        "sample Get horizontal speed\n");
    result = taf_pos_sample_GetHorizontalSpeed(positionSampleRef, &hval, &hAccuracy);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),
        "taf_pos_sample_GetHorizontalSpeed-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("taf_pos_sample_GetHorizontalSpeed: hSpeed.%u, accuracy.%u", hval, hAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed to get position sample horizontal speed information");
    }

    //161.taf_pos_sample_GetVerticalSpeed()
    LE_TEST_INFO("taf_pos_sample_GetVerticalSpeed() API is triggered to get pos"
        "sample Get vertical speed\n");
    result = taf_pos_sample_GetVerticalSpeed(positionSampleRef, &val, &vAccuracy);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),
        "taf_pos_sample_GetVerticalSpeed-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("GetVerticalSpeed: vSpeed.%d, accuracy.%d", val, accuracy);
    }
    else
    {
        LE_TEST_INFO("Failed to get position sample horizontal speed information");
    }

    //162.taf_pos_sample_GetDirection()
    LE_TEST_INFO("taf_pos_sample_GetDirection() API is triggered to get direction information\n");
    result = taf_pos_sample_GetDirection(positionSampleRef, &uval, &uAccuracy);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),
        "taf_pos_sample_GetDirection-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("taf_pos_sample_GetDirection: direction.%u, accuracy.%u", uval, uAccuracy);
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
    PositionHandlerRef = taf_gnss_AddPositionHandler(PositionHandlerFunction, NULL);

    //117.Position Handler
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

    //154.Sample Position Handler
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

    //153.taf_posCtrl_Request
    LE_TEST_INFO("taf_posCtrl_Request() API is called to get positioning services");
    activationRef = taf_posCtrl_Request();
    LE_TEST_OK((activationRef!=NULL),"taf_posCtrl_Request-LE_OK");

    // Add Position Handler Test
    positionThreadRef = le_thread_Create("PositionThreadTest",SamplePositionThread,NULL);
    LE_INFO("TestTafSamplePositionHandler positionThreadRef :%p",positionThreadRef);
    le_thread_Start(positionThreadRef);
    LE_INFO("TestTafSamplePositionHandler PositionHandlerRef :%p",PositionHandlerRef);
    LE_TEST_INFO("Wait for 1 second to trigger SamplePositionHandlerfunction");
    le_thread_Sleep(1);
    taf_pos_RemoveMovementHandler(SamplePositionHandlerRef);

    LE_INFO("TestTafGnssPositionHandler->cancel the thread");
    le_thread_Cancel(positionThreadRef);

    //163.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting GNSS fixes");
    LE_TEST_OK(taf_gnss_Stop() == LE_OK, "taf_gnss_Stop-LE_OK");
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

    //116. taf_gnss_Start() This will trigger startDetailedEngineReports() TelSDK API
    LE_TEST_OK(((taf_gnss_Start()) == LE_OK), "taf_gnss_Start-LE_OK");
    LE_TEST_INFO("Wait for 5 seconds");
    le_thread_Sleep(5);

    // Add Position Handler Test
    positionThreadRef = le_thread_Create("PositionThreadTest",PositionThread,NULL);
    LE_INFO("TestTafGnssPositionHandler positionThreadRef :%p",positionThreadRef);
    le_thread_Start(positionThreadRef);
    LE_INFO("TestTafGnssPositionHandler PositionHandlerRef :%p",PositionHandlerRef);
    LE_TEST_INFO("Wait for 1 second to trigger PositionHandlerfunction");
    le_thread_Sleep(1);
    taf_gnss_RemovePositionHandler(PositionHandlerRef);

    LE_INFO("TestTafGnssPositionHandler->cancel the thread");
    le_thread_Cancel(positionThreadRef);

    //136.Stop
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
    LE_TEST_INFO("taf_gnss_Start() API is called again to check whether it returns"
        "Not permitted state or not");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

   //16.GetTtff -Not Permitted
    LE_TEST_INFO("taf_gnss_GetTtff() API is called to check whether it returns"
        "Not Permitted state or not");
    result = taf_gnss_GetTtff(&ttff);
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_Tfff-LE_NOT_PERMITTED");

}

static void TestTafGnssConstellations
(
    void
)
{
    le_result_t result = LE_FAULT;
    taf_gnss_ConstellationBitMask_t constellationMask;

   //17.GetSupportedConstellations
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

   //18.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

   //19.GetSupportedConstellations -LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_GetSupportedConstellations() API is called to check whether it returns"
        "Not permitted or not");
    result = taf_gnss_GetSupportedConstellations(&constellationMask);
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_GetSupportedConstellations-LE_NOT_PERMITTED");

   //20.SetConstellation-GPS
    constellationMask = TAF_GNSS_CONSTELLATION_GPS;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set GPS constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_FAULT,"taf_gnss_SetConstellation-Not Implemented");

   //21.SetConstellation-GLONASS
    constellationMask = TAF_GNSS_CONSTELLATION_GLONASS;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set GLONASS constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");

   //22.SetConstellation-BEIDOU
    constellationMask = TAF_GNSS_CONSTELLATION_BEIDOU;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set BEIDOU constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");

   //23.SetConstellation-GALILEO
    constellationMask = TAF_GNSS_CONSTELLATION_GALILEO;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set GALILEO constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");

   //24.SetConstellation-SBAS
    constellationMask = TAF_GNSS_CONSTELLATION_SBAS;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set SBAS constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");

    //25.SetConstellation-QZSS
    constellationMask = TAF_GNSS_CONSTELLATION_QZSS;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set QZSS constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetConstellation-LE_OK");

    //26.GetConstellation
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
    }
    else
    {
        LE_TEST_INFO("Failed! to GetConstellation");
    }

   //27.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

   //28.SetConstellation-GPS -LE_NOT_PERMITTED
    constellationMask = TAF_GNSS_CONSTELLATION_GPS;
    LE_TEST_INFO("taf_gnss_SetConstellation() API is called to set GPS constellation type");
    result = taf_gnss_SetConstellation(constellationMask);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"taf_gnss_SetConstellation-LE_NOT_PERMITTED");

   //29.GetConstellation- LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_GetConstellation() API is called to get"
        "GPS constellation types enabled");
    result = taf_gnss_GetConstellation(&constellationMask);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"taf_gnss_GetConstellation-LE_NOT_PERMITTED");

}

static void TestTafGnssNmeaSentences
(
    void
)
{
    le_result_t result = LE_FAULT;
    taf_gnss_NmeaBitMask_t nmeaMaskPtr;

    //30.GetSupportedNmeaSentences
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

   //31.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

    //32.GetSupportedNmeaSentences- LE_NOT_PERMITTED
    LE_TEST_INFO("GetSupportedNmeaSentences() API is called to get the list of"
        "supported NMEA sentences");
    result = taf_gnss_GetSupportedNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_GetSupportedNmeaSentences-LE_NOT_PERMITTED");

    //33. SetNmeaSentences - GPGGA
    LE_TEST_INFO("SetNmeaSentences() API is called to set GPGGA NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPGGA;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //34.GetNmeaSentences
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

    //35. SetNmeaSentence - GPRMC
    LE_TEST_INFO("SetNmeaSentences() API is called to set GPRMC NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPRMC;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //36.GetNmeaSentences
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


    //37. SetNmeaSentence - GNGSA
    LE_TEST_INFO("SetNmeaSentences() API is called to set GNGSA NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GNGSA;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //38.GetNmeaSentences
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

    //39.SetNmeaSentence - GPVTG
    LE_TEST_INFO("SetNmeaSentences() API is called to set GPVTG NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPVTG;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //40.GetNmeaSentences
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

    //41. SetNmeaSentence - GPGNS
    LE_TEST_INFO("SetNmeaSentences() API is called to set GPGNS NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPGNS;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //42.GetNmeaSentences
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

    //43. SetNmeaSentence - GPDTM
    LE_TEST_INFO("SetNmeaSentences() API is called to set GPDTM NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPDTM;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //44.GetNmeaSentences
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

    //45. SetNmeaSentence - GPGSV
    LE_TEST_INFO("SetNmeaSentences() API is called to set GPGSV NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPGSV;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //46.GetNmeaSentences
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

    //47. SetNmeaSentence - GLGSV
    LE_TEST_INFO("SetNmeaSentences() API is called to set GLGSV NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GLGSV;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //48.GetNmeaSentences
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
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //49. SetNmeaSentence - GAGSV
    LE_TEST_INFO("SetNmeaSentences() API is called to set GAGSV NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GAGSV;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //50.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {
        if(nmeaMaskPtr & TAF_GNSS_NMEA_MASK_GAGSV)
        {
            LE_TEST_INFO("GAGSV enabled\n");
        }
    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //51. SetNmeaSentence - GQGSV
    LE_TEST_INFO("SetNmeaSentences() API is called to set GQGSV NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GQGSV;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //52.GetNmeaSentences
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
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //53. SetNmeaSentence - GBGSV
    LE_TEST_INFO("SetNmeaSentences() API is called to set GBGSV NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GBGSV;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //54.GetNmeaSentences
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

    //55. SetNmeaSentence - GIGSV
    LE_TEST_INFO("SetNmeaSentences() API is called to set GIGSV NMEA sentence type");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GIGSV;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");
    le_thread_Sleep(2);
    LE_TEST_INFO("wait for 2 seconds");

    //56.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
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

    //57. SetNmeaSentence - 0xFFFFFFFF
    LE_TEST_INFO("SetNmeaSentences() API is called to set 0xFFFFFFFF NMEA sentence type");
    nmeaMaskPtr = 0xFFFFFFFF;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_SetNmeaSentences-LE_OK");
    le_thread_Sleep(10);
    LE_TEST_INFO("wait for 10 seconds");

    //58.GetNmeaSentences
    LE_TEST_INFO("GetNmeaSentences() API is called to get NMEA sentence type");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_OK, "taf_gnss_GetNmeaSentences-LE_OK");
    if(result == LE_OK)
    {

       LE_TEST_INFO("nmeaMaskPtr: %0x\n",nmeaMaskPtr);

    }
    else
    {
        LE_TEST_INFO("Failed to Get an NMEA Sentence\n");
    }

    //59.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //60. SetNmeaSentence - LE_NOT_PERMITTED
    LE_TEST_INFO("SetNmeaSentences() API is called to check whether it returns"
        "Not permitted or not");
    nmeaMaskPtr = TAF_GNSS_NMEA_MASK_GPGGA;
    result = taf_gnss_SetNmeaSentences(nmeaMaskPtr);
    LE_TEST_OK(result==LE_NOT_PERMITTED, "taf_gnss_SetNmeaSentences-LE_NOT_PERMITTED");

    //61.GetNmeaSentences- LE_NOT_PERMITTED
    LE_TEST_INFO("GetNmeaSentences() API is called to check whether it returns"
        "Not permitted or not");
    result = taf_gnss_GetNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_NOT_PERMITTED, "taf_gnss_GetNmeaSentences-LE_NOT_PERMITTED");
}

static void TestTafGnssAcquisitionRate
(
    void
)
{
    le_result_t result = LE_FAULT;
    uint32_t acqRate;

    //62.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    //63.SetAcquisitionRate
    LE_TEST_INFO("taf_gnss_SetAcquisitionRate() API is called to set Acq Rate 1");
    acqRate = 1;
    result = taf_gnss_SetAcquisitionRate(acqRate);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetAcquisitionRate-LE_OK");

    //64.GetAcquisitionRate
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

    //65.SetAcquisitionRate
    LE_TEST_INFO("taf_gnss_SetAcquisitionRate() API is called to set Acq Rate 1001");
    acqRate = 1001;
    result = taf_gnss_SetAcquisitionRate(acqRate);
    LE_TEST_OK(result == LE_OK,"taf_gnss_SetAcquisitionRate-LE_OK");

    //66.GetAcquisitionRate
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

    //67.SetAcquisitionRate
    LE_TEST_INFO("taf_gnss_SetAcquisitionRate() API is called to set Acq Rate 0");
    acqRate = 0;
    result = taf_gnss_SetAcquisitionRate(acqRate);
    LE_TEST_OK(result == LE_OUT_OF_RANGE,"taf_gnss_SetAcquisitionRate-LE_OUT_OF_RANGE");

    //68.GetAcquisitionRate
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

    //69.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //70.SetAcquisitionRate-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_SetAcquisitionRate() API is called to check whether it returns"
        "Not Permitted or not");
    acqRate = 1;
    result = taf_gnss_SetAcquisitionRate(acqRate);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"taf_gnss_SetAcquisitionRate-LE_NOT_PERMITTED");

    //71.GetAcquisitionRate-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_GetAcquisitionRate() API is called to check whether it returns"
        "Not Permitted or not");
    result = taf_gnss_GetAcquisitionRate(&acqRate);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"taf_gnss_GetAcquisitionRate-LE_NOT_PERMITTED");
}

static void TestTafGnssSecBandConstellations
(
    void
)
{
    le_result_t result = LE_FAULT;
    int32_t constellationSb=0;

    //72.Start
    LE_TEST_INFO("taf_gnss_Stop() API is called to start reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");

    //73.DefaultSecondaryBandConstellations-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_DefaultSecondaryBandConstellations() API is triggered to check whether"
        "it returns Not Permitted or not");
    result = taf_gnss_DefaultSecondaryBandConstellations();
    LE_TEST_OK(result == LE_NOT_PERMITTED,"DefaultSecondaryBandConstellations-LE_NOT_PERMITTED");

    //74.ConfigureSecondaryBandConstellations-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_ConfigureSecondaryBandConstellations() API is triggered to check whether"
        "it returns Not Permitted or not");
    constellationSb |= (1<<(TAF_GNSS_SB_CONSTELLATION_GPS-1));
    result = taf_gnss_ConfigureSecondaryBandConstellations(constellationSb);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"ConfigureSecondaryBandConstellations-LE_NOT_PERMITTED");

    //75.RequestSecondaryBandConstellations-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_RequestSecondaryBandConstellations() API is triggered to check whether"
        "it returns Not Permitted or not");
    result = taf_gnss_RequestSecondaryBandConstellations(&constellationSb);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"RequestSecondaryBandConstellations-LE_NOT_PERMITTED");

    //76.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //77.DefaultSecondaryBandConstellations
    LE_TEST_INFO("taf_gnss_DefaultSecondaryBandConstellations() API is triggered to set default"
        "Secondary Band Constellations");
    result = taf_gnss_DefaultSecondaryBandConstellations();
    LE_TEST_OK(result == LE_OK,"DefaultSecondaryBandConstellations-LE_OK");

    //78.RequestSecondaryBandConstellations
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

    //79.ConfigureSecondaryBandConstellations
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

   //80.RequestSecondaryBandConstellations
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

    //81.SetDRConfig -Success
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

    //82.SetDRConfig -Failure
    drParamsPtr->offsetUnc = 180.1;
    result = taf_gnss_SetDRConfig(drParamsPtr);
    LE_TEST_OK(result == LE_FAULT, "taf_gnss_SetDRConfig-LE_FAULT");

    //83.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");
    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

    //84.SetDRConfig -LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_SetDRConfig() API is triggered to check whether it returns"
        "Not permitted or not");
    result = taf_gnss_SetDRConfig(drParamsPtr);
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_SetDRConfig-LE_NOT_PERMITTED");

    //Free the DR reference
    le_mem_Release(drParamsPtr);

    //85.taf_gnss_ConfigureEngineState- SPE/SUSPEND
    engineType = 1;
    engineState = 1;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure SUSPEND state"
        "for SPE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureEngineState-LE_OK");

    //86.taf_gnss_ConfigureEngineState- SPE/RESUME
    engineType = 1;
    engineState = 2;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure RESUME state"
        "for SPE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureEngineState-LE_OK");

    //87.taf_gnss_ConfigureEngineState- PPE/SUSPEND
    engineType = 2;
    engineState = 1;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure SUSPEND state"
        "for PPE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureEngineState-LE_OK");

    //88.taf_gnss_ConfigureEngineState- PPE/RESUME
    engineType = 2;
    engineState = 2;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure RESUME state"
        "for PPE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureEngineState-LE_OK");

    //89.taf_gnss_ConfigureEngineState- DRE/SUSPEND
    engineType = 3;
    engineState = 1;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure SUSPEND state for"
        "DRE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureEngineState-LE_OK");

    //90.taf_gnss_ConfigureEngineState- DRE/RESUME
    engineType = 3;
    engineState = 2;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure RESUME state for"
        "DRE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureEngineState-LE_OK");

    //91.taf_gnss_ConfigureEngineState- VPE/SUSPEND
    engineType = 4;
    engineState = 1;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure SUSPEND state for"
        "VPE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureEngineState-LE_OK");

    //92.taf_gnss_ConfigureEngineState- VPE/RESUME
    engineType = 4;
    engineState = 2;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to configure RESUME state for"
        "VPE engine type");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureEngineState-LE_OK");

    //93.taf_gnss_ConfigureEngineState- Failure -Invalid Engine type
    engineType = 5;
    engineState = 1;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to check for failure scenario");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-LE_FAULT");

    //94.taf_gnss_ConfigureEngineState- Failure-Invalid Engine State
    engineType = 4;
    engineState = 3;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to check for failure scenario");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_FAULT,"taf_gnss_ConfigureEngineState-LE_FAULT");

    //95.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //96.taf_gnss_ConfigureEngineState -LE_NOT_PERMITTED
    engineType = 1;
    engineState = 1;
    LE_TEST_INFO("taf_gnss_ConfigureEngineState API is triggered to check whether it"
        "returns Not permitted or not");
    result = taf_gnss_ConfigureEngineState(engineType,engineState);
    LE_TEST_OK(result==LE_NOT_PERMITTED,"taf_gnss_ConfigureEngineState-LE_NOT_PERMITTED");

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

   //97.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");
    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

    //98.Configure Robust Locaiton - Enable/1 Enabled911/1
    LE_TEST_INFO("taf_gnss_ConfigureRobustLocation() API is called to configure"
        "Enable->1 Enabled911->1");
    enable = 1;
    enabled911 = 1;
    result = taf_gnss_ConfigureRobustLocation(enable,enabled911);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureRobustLocation-LE_OK");

    //99.Robust Location information
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

    //100.Configure Robust Locaiton - Enable/1 Enabled911/0
    LE_TEST_INFO("taf_gnss_ConfigureRobustLocation() API is called to configure"
        "Enable->1 Enabled911->0");
    enable = 1;
    enabled911 = 0;
    result = taf_gnss_ConfigureRobustLocation(enable,enabled911);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureRobustLocation-LE_OK");

    //101.Robust Location information
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

    //102.Configure Robust Locaiton - Enable/0 Enabled911/1
    LE_TEST_INFO("taf_gnss_ConfigureRobustLocation() API is called to configure"
        "Enable->0 Enabled911->1");
    enable = 0;
    enabled911 = 1;
    result = taf_gnss_ConfigureRobustLocation(enable,enabled911);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureRobustLocation-LE_OK");

    //103.Robust Location information
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

    //104.Configure Robust Locaiton - Enable/0 Enabled911/1
    LE_TEST_INFO("taf_gnss_ConfigureRobustLocation() API is called to configure"
        "Enable->0 Enabled911->0");
    enable = 0;
    enabled911 = 0;
    result = taf_gnss_ConfigureRobustLocation(enable,enabled911);
    LE_TEST_OK(result==LE_OK,"taf_gnss_ConfigureRobustLocation-LE_OK");

    //105.Robust Location information
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

   //106.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //107.Configure Robust Locaiton - LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_ConfigureRobustLocation() API is called to check whether it returns"
        "Not permitted or not");
    enable = 1;
    enabled911 = 1;
    result = taf_gnss_ConfigureRobustLocation(enable,enabled911);
    LE_TEST_OK(result==LE_NOT_PERMITTED,"taf_gnss_ConfigureRobustLocation-LE_NOT_PERMITTED");

    //108.Robust Location information-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_RobustLocationInformation() API is called to check whether it returns"
        "Not permitted or not");
    result = taf_gnss_RobustLocationInformation(&enable,&enabled911, &majorVersion,&minorVersion);
    LE_TEST_OK(result==LE_NOT_PERMITTED,"taf_gnss_RobustLocationInformation-LE_NOT_PERMITTED");

}

static void TestTafGnssMinElevation
(
    void
)
{
    le_result_t result = LE_FAULT;
    uint8_t  minElevation;

    //109.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");
    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

    //110.taf_gnss_SetMinElevation -LE_OUT_OF_RANGE
    LE_TEST_INFO("taf_gnss_SetMinElevation() API is triggered to check whether it returns"
        " Out of range or not");
    minElevation = TAF_GNSS_MIN_ELEVATION_MAX_DEGREE+1;
    result = taf_gnss_SetMinElevation(minElevation);
    LE_TEST_OK(result == LE_OUT_OF_RANGE, "taf_gnss_SetMinElevation-LE_OUT_OF_RANGE");

    //111.taf_gnss_SetMinElevation
    LE_TEST_INFO("taf_gnss_SetMinElevation() API is triggered to set min SV elevation");
    minElevation = 1;
    result = taf_gnss_SetMinElevation(minElevation);
    LE_TEST_OK(result == LE_OK, "taf_gnss_SetMinElevation-LE_OK");

    //112.taf_gnss_GetMinElevation
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

    //113.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //114.taf_gnss_SetMinElevation-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_SetMinElevation() API is triggered to check whether it returns"
        " Not permitted or not");
    minElevation = 1;
    result = taf_gnss_SetMinElevation(minElevation);
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_SetMinElevation-LE_NOT_PERMITTED");

    //115.taf_gnss_GetMinElevation-LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_GetMinElevation() API is triggered to check whether it returns"
        " Not permitted or not");
    result = taf_gnss_GetMinElevation(&minElevation);
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_GetMinElevation-LE_NOT_PERMITTED");


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

    //137.taf_posCtrl_Request
    LE_TEST_INFO("taf_posCtrl_Request() API is called to get positioning services");
    activationRef = taf_posCtrl_Request();
    LE_TEST_OK((activationRef!=NULL),"taf_posCtrl_Request-LE_OK");

    //taf_gnss_Start() This will trigger startDetailedEngineReports() TelSDK API
    //LE_TEST_OK(((taf_gnss_Start()) == LE_OK), "taf_gnss_Start-LE_OK");
    LE_TEST_INFO("Wait for 5 seconds");
    le_thread_Sleep(5);

    //138.taf_pos_Get2DLocation
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

    //139. Get Date
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

    //140.Get Time
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

    //141.taf_pos_GetFixState
    LE_TEST_INFO("taf_pos_GetFixState is triggered to get time Fix state");
    result = taf_pos_GetFixState(&fixState);
    LE_TEST_OK(result == LE_OK,"taf_pos_GetFixState-LE_OK");
    if(result == LE_OK)
    {
        LE_TEST_INFO("Position fix state: %s", (TAF_GNSS_STATE_FIX_NO_POS == fixState)?"No Fix"
                                     :(TAF_GNSS_STATE_FIX_2D == fixState)?"2D Fix"
                                     :(TAF_GNSS_STATE_FIX_3D == fixState)?"3D Fix"
                                     : "Unknown");
    }
    else
    {
        LE_TEST_INFO("Failed to get pos Fix state\n");
    }

    //142.taf_pos_GetMotion
    LE_TEST_INFO("taf_pos_GetMotion() API is triggered to get heading information\n");
    result = taf_pos_GetMotion(&hSpeed, &hSpeedAccuracy, &vSpeed, &vSpeedAccuracy);
    LE_TEST_OK(((result == LE_OK) || (result == LE_OUT_OF_RANGE)),"taf_pos_GetMotion-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_INFO("taf_pos_GetMotion hSpeed.%u, hSpeedAccuracy.%u, vSpeed.%d, vSpeedAccuracy.%d",
            hSpeed, hSpeedAccuracy, vSpeed, vSpeedAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed | to get Motion information\n");
    }

    //143.taf_pos_GetDirection
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

    //144.taf_pos_SetAcquisitionRate
    LE_TEST_INFO("taf_pos_SetAcquisitionRate() is triggered to check whether it returns"
        "LE_UNSUPPORTED or not\n");
    acquisitionRate = 3000;
    result = taf_pos_SetAcquisitionRate(acquisitionRate);
    LE_TEST_INFO("taf_pos_SetAcquisitionRate()-> result: %d\n",result);
    LE_TEST_OK((result == LE_OK),"taf_pos_SetAcquisitionRate-LE_OK");

    //145.taf_pos_GetAcquisitionRate
    LE_TEST_INFO("taf_pos_SetAcquisitionRate() is triggered to get Aquisition rate\n");
    acquisitionRate = taf_pos_GetAcquisitionRate();
    LE_TEST_INFO("taf_pos_SetAcquisitionRate()-> Aquisition rate: %d\n",acquisitionRate);
    LE_TEST_OK(((3000 == acquisitionRate) || (DEFAULT_ACQUISITION_RATE == acquisitionRate)),
        "taf_pos_GetAcquisitionRate-LE_OK");

    //146.taf_pos_Get3DLocation
    LE_TEST_INFO("taf_pos_Get3DLocation() is triggered to get 3D location information\n");
    result = taf_pos_Get3DLocation(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy);
    LE_TEST_OK(((LE_OK == result) || (LE_OUT_OF_RANGE == result)),"taf_pos_Get3DLocation-LE_OK");
    if((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        LE_TEST_INFO("taf_pos_Get3DLocation latitude.%d, longitude.%d, hAccuracy.%d, altitude.%d"
            ", vAccuracy.%d",latitude, longitude, hAccuracy, altitude, vAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed to get Pos 3D location information\n");
    }

    //147.taf_pos_SetDistanceResolution()-LE_BAD_PARAMETER
    LE_TEST_INFO("taf_pos_SetDistanceResolution() is triggered to set distance resolution\n");
    result = taf_pos_SetDistanceResolution(TAF_POS_RES_UNKNOWN);
    LE_TEST_OK((result == LE_BAD_PARAMETER),"taf_pos_SetDistanceResolution-LE_BAD_PARAMETER");

    //148.taf_pos_SetDistanceResolution
    LE_TEST_INFO("taf_pos_SetDistanceResolution() is triggered to set TAF_POS_RES_METER\n");
    result = taf_pos_SetDistanceResolution(TAF_POS_RES_METER);
    LE_TEST_OK((result == LE_OK),"taf_pos_SetDistanceResolution-LE_OK");

    //149.taf_pos_SetDistanceResolution
    LE_TEST_INFO("taf_pos_SetDistanceResolution() is triggered to set TAF_POS_RES_DECIMETER\n");
    result = taf_pos_SetDistanceResolution(TAF_POS_RES_DECIMETER);
    LE_TEST_OK((result == LE_OK),"taf_pos_SetDistanceResolution-LE_OK");

    //150.taf_pos_SetDistanceResolution
    LE_TEST_INFO("taf_pos_SetDistanceResolution() is triggered to set TAF_POS_RES_CENTIMETER\n");
    result = taf_pos_SetDistanceResolution(TAF_POS_RES_CENTIMETER);
    LE_TEST_OK((result == LE_OK),"taf_pos_SetDistanceResolution-LE_OK");

    //151.taf_pos_SetDistanceResolution
    LE_TEST_INFO("taf_pos_SetDistanceResolution() is triggered to set TAF_POS_RES_MILLIMETER\n");
    result = taf_pos_SetDistanceResolution(TAF_POS_RES_MILLIMETER);
    LE_TEST_OK((result == LE_OK),"taf_pos_SetDistanceResolution-LE_OK");

    //152.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting GNSS fixes");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    LE_INFO("Release the positioning service");
    taf_posCtrl_Release(activationRef);

}

static void TestTafGnssStartMode
(
    void
)
{
    le_result_t result = LE_FAULT;

    //164.Disable
    LE_TEST_INFO("taf_gnss_Disable() is triggered to disable Engine state\n");
    result = taf_gnss_Disable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Disable-LE_OK");

    //165.StartMode -Hot Start/LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in  hot mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_HOT_START);
    LE_TEST_OK((result == LE_NOT_PERMITTED),
               "taf_gnss_StartMode-LE_NOT_PERMITTED");

    //166.StartMode -Warm Start/LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in  Warm mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_WARM_START);
    LE_TEST_OK((result == LE_NOT_PERMITTED),
               "taf_gnss_StartMode-LE_NOT_PERMITTED");

    //167.StartMode -Warm Start/LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in cold mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_COLD_START);
    LE_TEST_OK((result == LE_NOT_PERMITTED),
               "taf_gnss_StartMode-LE_NOT_PERMITTED");

    //168.StartMode -Factory Start/LE_NOT_PERMITTED
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in factory mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_FACTORY_START);
    LE_TEST_OK((result == LE_NOT_PERMITTED),
               "taf_gnss_StartMode-LE_NOT_PERMITTED");

    //169.StartMode -UNKNOWN/LE_BAD_PARAMETER
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in factory mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_UNKNOWN_START);
    LE_TEST_OK((result == LE_BAD_PARAMETER),"taf_gnss_StartMode-LE_BAD_PARAMETER");

    //170.Enable
    LE_TEST_INFO("taf_gnss_Disable() is triggered to Enable Engine state\n");
    result = taf_gnss_Enable();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Enable-LE_OK");

    //171.StartMode -Hot/LE_OK
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in hot mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_HOT_START);
    LE_TEST_OK((result == LE_OK),"taf_gnss_StartMode-LE_OK");
    LE_TEST_INFO("Wait for 10 seconds");
    le_thread_Sleep(10);

    //172.StartMode -Hot/LE_DUPLICATE
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in hot mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_HOT_START);
    LE_TEST_OK((result == LE_DUPLICATE),"taf_gnss_StartMode-LE_DUPLICATE");

    //173.StartMode -Warm/LE_DUPLICATE
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in Warm mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_WARM_START);
    LE_TEST_OK((result == LE_DUPLICATE),"taf_gnss_StartMode-LE_DUPLICATE");

    //174.Stop
    LE_TEST_INFO("taf_gnss_Stop() is triggered to Enable Engine state\n");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //175.StartMode -Warm/LE_OK
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in Warm mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_WARM_START);
    LE_TEST_OK((result == LE_OK),"taf_gnss_StartMode-LE_OK");
    LE_TEST_INFO("Wait for 30 seconds");
    le_thread_Sleep(30);

    //176.StartMode -Cold/LE_DUPLICATE
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in Cold mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_COLD_START);
    LE_TEST_OK((result == LE_DUPLICATE),"taf_gnss_StartMode-LE_DUPLICATE");

    //177.Stop
    LE_TEST_INFO("taf_gnss_Stop() is triggered to Enable Engine state\n");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //178.StartMode -Cold/LE_OK
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in Cold mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_COLD_START);
    LE_TEST_OK((result == LE_OK),"taf_gnss_StartMode-LE_OK");
    LE_TEST_INFO("Wait for 30 seconds");
    le_thread_Sleep(30);

    //179.StartMode -Factory/LE_DUPLICATEm
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in Factory mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_FACTORY_START);
    LE_TEST_OK((result == LE_DUPLICATE),"taf_gnss_StartMode-LE_DUPLICATE");

    //180.Stop
    LE_TEST_INFO("taf_gnss_Stop() is triggered to Enable Engine state\n");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

    //181.StartMode -Factory/LE_OK
    LE_TEST_INFO("taf_gnss_StartMode() is triggered to start the engine in Factory mode\n");
    result = taf_gnss_StartMode(TAF_GNSS_FACTORY_START);
    LE_TEST_OK((result == LE_OK),"taf_gnss_StartMode-LE_OK");
    LE_TEST_INFO("Wait for 60 seconds");
    le_thread_Sleep(60);

    //182.Stop
    LE_TEST_INFO("taf_gnss_Stop() is triggered to Enable Engine state\n");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

}

static void TestTafGnssRestart
(
    void
)
{
    le_result_t result = LE_FAULT;

   //183.Start
    LE_TEST_INFO("taf_gnss_Start() API is called to trigger detailed Engine reporting");
    result = taf_gnss_Start();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Start-LE_OK");
    LE_TEST_INFO("wait for 3 seconds");
    le_thread_Sleep(3);

   //184.Force Warm Restart
    LE_TEST_INFO("taf_gnss_ForceWarmRestart() API is called to perform"
        " Warm restart of GNSS engine");
    result = taf_gnss_ForceWarmRestart();
    LE_TEST_OK(result == LE_OK, "taf_gnss_ForceWarmRestart-LE_OK");
    LE_TEST_INFO("Wait for 30 seconds to get fixes");
    le_thread_Sleep(30);

   //185.Force Cold Restart
    LE_TEST_INFO("taf_gnss_ForceColdRestart() API is called to perform"
        " Cold restart of GNSS engine");
    result = taf_gnss_ForceColdRestart();
    LE_TEST_OK(result == LE_OK, "taf_gnss_ForceColdRestart-LE_OK");
    LE_TEST_INFO("Wait for 60 seconds to get fixes");
    le_thread_Sleep(60);

   //186.Force Hot Restart
    LE_TEST_INFO("taf_gnss_ForceHotRestart() API is called to perform"
        " Warm restart of GNSS engine");
    result = taf_gnss_ForceHotRestart();
    LE_TEST_OK(result == LE_OK, "taf_gnss_ForceHotRestart-LE_OK");
    LE_TEST_INFO("Wait for 10 seconds to get fixes");
    le_thread_Sleep(10);

   //187.Stop
    LE_TEST_INFO("taf_gnss_Stop() API is called to stop reporting");
    result = taf_gnss_Stop();
    LE_TEST_OK(result == LE_OK, "taf_gnss_Stop-LE_OK");

   //188.Force Warm Restart-Not Permitted
    LE_TEST_INFO("taf_gnss_ForceWarmRestart() API is called to check whether it returns"
        " Not permitted state or not");
    result = taf_gnss_ForceWarmRestart();
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_ForceWarmRestart-LE_NOT_PERMITTED");

   //189.Force Cold Restart-Not Permitted
    LE_TEST_INFO("taf_gnss_ForceColdRestart() API is called to check whether it returns"
        " Not permitted state or not");
    result = taf_gnss_ForceColdRestart();
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_ForceColdRestart-LE_NOT_PERMITTED");

   //190.Force Hot Restart- Not permitted
    LE_TEST_INFO("taf_gnss_ForceHotRestart() API is called to check whether it returns"
        " Not permitted state or not");
    result = taf_gnss_ForceHotRestart();
    LE_TEST_OK(result == LE_NOT_PERMITTED, "taf_gnss_ForceHotRestart-LE_NOT_PERMITTED");

}

COMPONENT_INIT
{
   PositionHandlerSem = le_sem_Create("PosHandlerSem", 0);

   LE_TEST_INFO("======== TestTafGnssStart APIs Test  ========");
   TestTafGnssStart();

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

   LE_TEST_INFO("====TestTafGnssRobustLocation APIs Test====");
   TestTafGnssRobustLocation();

   LE_TEST_INFO("====TestTafGnssMinElevation APIs Test====");
   TestTafGnssMinElevation();

   LE_TEST_INFO("======== GNSS Location information APIs Test  ========");
   TestTafGnssPositionHandler();

   LE_TEST_INFO("==== GNSS Position information APIs Test====");
   TestTafPosHandler();

   LE_TEST_INFO("==== GNSS Sample Position information APIs Test====");
   TestTafSamplePositionHandler();

   LE_TEST_INFO("====TestTafGnssStartMode APIs Test====");
   TestTafGnssStartMode();

   LE_TEST_INFO("======== TestTafGnssRestart ======");
   TestTafGnssRestart();

   LE_TEST_INFO("======== LE_TEST_EXIT  ========");
   LE_TEST_EXIT;
}