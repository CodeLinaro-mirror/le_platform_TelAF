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
#include "watchdogChain.h"
#include "tafGnss.hpp"
#include <any>
#include <new>
#include "float.h"

using namespace tafpa::location;
using namespace tafsvc;

LE_MEM_DEFINE_STATIC_POOL(PositionHandler, GNSS_POSITION_HANDLER_HIGH, sizeof(taf_locGnss_PositionHandler_t));
LE_MEM_DEFINE_STATIC_POOL(MeasurementHandler, GNSS_POSITION_HANDLER_HIGH, sizeof(taf_locGnss_MeasurementHandler_t));
LE_MEM_DEFINE_STATIC_POOL(PositionExHandler, GNSS_POSITION_HANDLER_HIGH, sizeof(taf_locGnss_PositionExHandler_t));
LE_MEM_DEFINE_STATIC_POOL(NmeaHandler, GNSS_POSITION_HANDLER_HIGH, sizeof(taf_locGnss_NmeaHandler_t));
LE_MEM_DEFINE_STATIC_POOL(CapHandler, GNSS_POSITION_HANDLER_HIGH, sizeof(taf_locGnss_CapHandler_t));
LE_MEM_DEFINE_STATIC_POOL(PositionSample, GNSS_POSITION_SAMPLE_MAX, sizeof(taf_locGnss_PositionSample_t));
LE_MEM_DEFINE_STATIC_POOL(MeasurementSample, GNSS_POSITION_SAMPLE_MAX, sizeof(taf_locGnss_GnssMeasurements_t));
LE_MEM_DEFINE_STATIC_POOL(NmeaSample, GNSS_POSITION_SAMPLE_MAX, sizeof(NmeaInfoEvent_t));
LE_MEM_DEFINE_STATIC_POOL(CapSample, GNSS_POSITION_SAMPLE_MAX, sizeof(CapabilityChangeEvent_t));
LE_MEM_DEFINE_STATIC_POOL(PositionSampleRequest, GNSS_POSITION_SAMPLE_MAX, sizeof(taf_locGnss_PositionSampleRequest_t));
LE_MEM_DEFINE_STATIC_POOL(MeasurementSampleRequest, GNSS_POSITION_SAMPLE_MAX, sizeof(taf_locGnss_MeasurementSampleRequest_t));
LE_MEM_DEFINE_STATIC_POOL(PositionSampleEx, GNSS_POSITION_SAMPLE_MAX, sizeof(taf_locGnss_PositionSampleEx_t));
LE_MEM_DEFINE_STATIC_POOL(PositionExSample, GNSS_POSITION_SAMPLE_MAX, sizeof(taf_locGnss_PositionExSample_t));
LE_MEM_DEFINE_STATIC_POOL(DgnssSource, GNSS_POSITION_SAMPLE_MAX, sizeof(taf_locGnss_DgnssSource_t));
LE_MEM_DEFINE_STATIC_POOL(Client, LE_CONFIG_POSITIONING_ACTIVATION_MAX, sizeof(taf_locGnss_Client_t));
LE_MEM_DEFINE_STATIC_POOL(DgnssHandler, GNSS_POSITION_HANDLER_HIGH, sizeof(taf_locGnss_DgnssStatusChangeHandler_t));
LE_MEM_DEFINE_STATIC_POOL(DgnssStatusRequest, GNSS_POSITION_SAMPLE_MAX, sizeof(DgnssStatusEvent_t));
LE_MEM_DEFINE_STATIC_POOL(CmdLocationPool, MAX_CMD_LOCATION_POOL_SIZE, sizeof(LocationCmdInfo_t));
LE_MEM_DEFINE_STATIC_POOL(GnssInternalCmdPool, MAX_CMD_LOCATION_POOL_SIZE, sizeof(GnssInternalCmd_t));
LE_REF_DEFINE_STATIC_MAP(PositionSampleMap, GNSS_POSITION_SAMPLE_MAX);
LE_REF_DEFINE_STATIC_MAP(PositionExSampleMap, GNSS_POSITION_SAMPLE_MAX);
LE_REF_DEFINE_STATIC_MAP(MeasurementSampleMap, GNSS_POSITION_SAMPLE_MAX);
void bodyToSensorUtility(taf_pa_location_DREngineConfiguration_t& drConfig,
        const taf_locGnss_DrParams_t* drParamsPtr,taf_locGnss_Client_t* clientRequestPtr,
        le_result_t* sensor_Result);
void speedScaleUtility(taf_pa_location_DREngineConfiguration_t& drConfig,
        const taf_locGnss_DrParams_t* drParamsPtr,taf_locGnss_Client_t* clientRequestPtr,
        le_result_t* speedScale_Result);
void gyroScaleUtility(taf_pa_location_DREngineConfiguration_t& drConfig,
        const taf_locGnss_DrParams_t* drParamsPtr,taf_locGnss_Client_t* clientRequestPtr,
        le_result_t* gyroScale_Result);
void roundOffLocationData(double *locData, uint8_t dplace);
taf_locGnss &taf_locGnss::GetInstance()
{
    static taf_locGnss instance;
    return instance;
}

void Handler::onGnssSVInfo(taf_pa_location_LocationId clientId, const std::vector<std::shared_ptr<taf_pa_location_GnssSVInfo_t>>& GnssSVInfo, std::any context)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_Client_t* clientRequestPtr = NULL;

    le_msg_SessionRef_t sessionRef = std::any_cast<le_msg_SessionRef_t>(context);

    clientRequestPtr = gnss.DiscoverSessionRef(sessionRef);

    if (NULL == clientRequestPtr) {
        LE_CRIT("onGnssSVInfo did not find sessionRef: %p", sessionRef);
        return;
    }

    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);
    int i = 0;
    clientRequestPtr->mSatParams.satsInViewCount = GnssSVInfo.size();
    clientRequestPtr->mTotalSVTracked = 0;
    memset(&clientRequestPtr->mSatInfo, 0, sizeof(clientRequestPtr->mSatInfo));
    for(auto svInfo : GnssSVInfo) {
        if(i >= TAF_LOCGNSS_SV_INFO_MAX_LEN)
        {
            LE_WARN("SvInfo overflows");
            continue;
        }

        switch(svInfo->constType) {
            case TAF_PA_LOCATION_GPS:
                clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_GPS;
                break;
            case TAF_PA_LOCATION_GLONASS:
                clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_GLONASS;
                break;
            case TAF_PA_LOCATION_BDS:
                clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_BEIDOU;
                break;
            case TAF_PA_LOCATION_GALILEO:
                clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_GALILEO;
                break;
            case TAF_PA_LOCATION_SBAS:
                clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_SBAS;
                break;
            case TAF_PA_LOCATION_QZSS:
                clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_QZSS;
                break;
            case TAF_PA_LOCATION_NAVIC:
                clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_NAVIC;
                break;
            default:
                clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_UNDEFINED;
                LE_ERROR("Constellation type: UNKNOWN");
                break;
        }

        clientRequestPtr->mSatInfo[i].satId = svInfo->satId;
        if(svInfo->hasFix == TAF_PA_LOCATION_YES)
        {
            clientRequestPtr->mSatInfo[i].satUsed = true;
        }
        else
        {
            clientRequestPtr->mSatInfo[i].satUsed = false;
        }
        if(svInfo->Snr !=0)
        {
            clientRequestPtr->mTotalSVTracked++;
            clientRequestPtr->mSatInfo[i].satTracked = true;
        }
        else
        {
            clientRequestPtr->mSatInfo[i].satTracked = false;
        }
        clientRequestPtr->mSatInfo[i].satSnr = svInfo->Snr;
        clientRequestPtr->mSatInfo[i].satAzim = svInfo->azimuth;
        clientRequestPtr->mSatInfo[i].satElev = svInfo->elevation;
        clientRequestPtr->mSatInfo[i].signalType = svInfo->signalType;
        clientRequestPtr->mSatInfo[i].glonassFcn = svInfo->GlonassFcn;
        clientRequestPtr->mSatInfo[i].baseBandCnr = svInfo->BasebandCnr;
        i++;
    }
    clientRequestPtr->mSatParams.satsTrackingCount = clientRequestPtr->mTotalSVTracked;
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);

}

void Handler::onGnssSignalInfo(taf_pa_location_LocationId clientId, const std::shared_ptr<taf_pa_location_GnssData_t>& gnssDatainfo, std::any context)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_Client_t* clientRequestPtr = NULL;

    le_msg_SessionRef_t sessionRef = std::any_cast<le_msg_SessionRef_t>(context);

    clientRequestPtr = gnss.DiscoverSessionRef(sessionRef);

    if (NULL == clientRequestPtr) {
        LE_CRIT("onGnssSignalInfo did not find sessionRef: %p", sessionRef);
        return;
    }

    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);
    if(gnss.NumOfPositionHandlers ) {
        for(int sig = 0; sig < TAF_LOCGNSS_NUMBER_OF_SIGNAL_TYPES_MAX; sig++)
        {
            LE_DEBUG("Signal Type : %d; gnssDataMask: %d",sig, gnssDatainfo->gnssDataMask[sig]);
            if(TAF_PA_LOCATION_HAS_JAMMER == ((gnssDatainfo->gnssDataMask[sig])
            & (TAF_PA_LOCATION_HAS_JAMMER)))
            {
                LE_DEBUG("onGnssSignalInfo jammerInd: %lf",gnssDatainfo->jammerInd[sig]);
                clientRequestPtr->mGnssData[sig].gnssDataMask |= TAF_LOCGNSS_HAS_JAMMER;
                clientRequestPtr->mGnssData[sig].jammerInd = gnssDatainfo->jammerInd[sig];
            }
            else
            {
                LE_DEBUG("onGnssSignalInfo JAMMER Ind Not Present ");
            }
            if(TAF_PA_LOCATION_HAS_AGC == ((gnssDatainfo->gnssDataMask[sig])
             & (TAF_PA_LOCATION_HAS_AGC)))
            {
                LE_DEBUG("onGnssSignalInfo agc: %lf",gnssDatainfo->agc[sig]);
                clientRequestPtr->mGnssData[sig].gnssDataMask |= TAF_LOCGNSS_HAS_AGC;
                clientRequestPtr->mGnssData[sig].agc = gnssDatainfo->agc[sig];
            }
            else
            {
                LE_DEBUG("onGnssSignalInfo AGC Not Present");
            }
        }
    }
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);
}

void Handler::onGnssNmeaInfo(taf_pa_location_LocationId clientId, const std::shared_ptr<taf_pa_location_NmeaInfoEvent_t>& nmeaEventInfo, std::any context)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_Client_t* clientRequestPtr = NULL;

    le_msg_SessionRef_t sessionRef = std::any_cast<le_msg_SessionRef_t>(context);

    clientRequestPtr = gnss.DiscoverSessionRef(sessionRef);

    if (NULL == clientRequestPtr) {
        LE_CRIT("onGnssMeasurementsInfo did not find sessionRef: %p", sessionRef);
        return;
    }
    //Format : $GPGGA,075446.90,00-0.000000,S,00000.000000,E,1,00,1.0,936.4,M,-936.4,M,,*7D^M
    LE_DEBUG("onGnssNmeaInfo for *sessionRef: %p, NumOfNmeaHandlers=%d",sessionRef,
            gnss.NumOfNmeaHandlers);
    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);
    if(gnss.NumOfNmeaHandlers) {
        //clientRequestPtr->mSatMeas.satId = nmea;
        clientRequestPtr->mSatMeas.satLatency = nmeaEventInfo->timestamp;

        NmeaInfoEvent_t* NmeaData = (NmeaInfoEvent_t*)le_mem_ForceAlloc(gnss.NmeaSamplePoolRef);

        NmeaData->timestamp = nmeaEventInfo->timestamp;
        const int length = nmeaEventInfo->nmeaMask.length();
        NmeaData->nmeaMask[length] ='\0';
        for (int i = 0; i < length; i++)
        {
            NmeaData->nmeaMask[i] = nmeaEventInfo->nmeaMask.c_str()[i];
        }
        LE_DEBUG( "**** NMEA handler string copied is: %s****",NmeaData->nmeaMask);
        NmeaData->clientSessionRefPtr = &clientRequestPtr->sessionRef;
        le_event_ReportWithRefCounting(gnss.nmeaEventId, NmeaData);
    }
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);

}

inline bool IsSbasStationId(uint16_t id)
{
    return ((id >= SBAS_STATION_ID_RANGE1_MIN && id <= SBAS_STATION_ID_RANGE1_MAX) ||
            (id >= SBAS_STATION_ID_RANGE2_MIN && id <= SBAS_STATION_ID_RANGE2_MAX));
}


void Handler::onDetailedEngineLocationUpdate(taf_pa_location_LocationId clientId, const std::vector<std::shared_ptr<taf_pa_location_LocEngineInfo_t>>& locationEngineInfo, std::any context)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_Client_t* clientRequestPtr = NULL;

    le_msg_SessionRef_t sessionRef = std::any_cast<le_msg_SessionRef_t>(context);

    clientRequestPtr = gnss.DiscoverSessionRef(sessionRef);

    if (NULL == clientRequestPtr) {
        LE_CRIT("onDetailedEngineLocationUpdate did not find sessionRef: %p", sessionRef);
        return;
    }

    for (auto locationInfo : locationEngineInfo) {
        std::chrono::time_point<std::chrono::system_clock> mEndTime = std::chrono::system_clock::now();
        if (clientRequestPtr->mTtffReportCount < TTFF_REPORT_COUNT)
        {
            LE_DEBUG("TTFF reportStatus = %d, received report time = %" PRIu64 ", utc timeStamp = %" PRIu64 "", (int)(locationInfo->reportStatus), mEndTime.time_since_epoch().count(), locationInfo->timeStamp);
            (clientRequestPtr->mTtffReportCount)++;
        }
        if ((clientRequestPtr->mFirstFix) &&
            (locationInfo->reportStatus == TAF_PA_LOCATION_SUCCESS) &&
            (locationInfo->latitude != NAN) &&
            (locationInfo->longitude != NAN))
        {
            if (locationInfo->posTechnology == TAF_PA_LOCATION_GNSS_PROPAGATED)
            {
                LE_DEBUG("The position technology of this position report is PROPAGATED");
                clientRequestPtr->mTtffPtr=  0;
            } else{
                clientRequestPtr->mFirstFix = false;
                std::chrono::duration<double> elapsedTime = mEndTime - clientRequestPtr->mStartTime;
                clientRequestPtr->mTtffPtr = elapsedTime.count() * 1e+3;
                LE_DEBUG("TTFF mEndTime = %ld, TTFF value = %d", mEndTime.time_since_epoch().count(), clientRequestPtr->mTtffPtr);
            }
        }
    }
    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);
    if(gnss.NumOfPositionHandlers )
    {
        for (auto locationInfo : locationEngineInfo) {
            taf_locGnss_PositionSample_t* LocationData =
                    (taf_locGnss_PositionSample_t*)le_mem_ForceAlloc(gnss.PositionSamplePoolRef);

            memset(LocationData, 0, sizeof(taf_locGnss_PositionSample_t));

            double locData;
            LocationData->clientSessionRefPtr = &clientRequestPtr->sessionRef;
            LocationData->latitudeValid = true;
            LocationData->longitudeValid = true;
            LocationData->hAccuracyValid = true;
            LocationData->altitudeValid = true;
            LocationData->altitudeOnWgs84Valid = false;
            LocationData->horUncEllipseSemiMajorValid = true;
            LocationData->horUncEllipseSemiMinorValid = true;
            LocationData->horConfidenceValid = true;
            LocationData->vAccuracyValid = true;
            LocationData->hSpeedValid = true;
            LocationData->hSpeedAccuracyValid = true;
            LocationData->vSpeedValid = true;
            LocationData->vSpeedAccuracyValid = true;
            LocationData->directionValid = true;
            LocationData->directionAccuracyValid = true;
            LocationData->dateValid = true;
            LocationData->timeValid = true;
            LocationData->gpsTimeValid = true;
            LocationData->timeAccuracyValid = true;
            LocationData->leapSecondsValid = true;
            LocationData->positionLatencyValid = false;
            LocationData->hdopValid = true;
            LocationData->vdopValid = true;
            LocationData->pdopValid = true;
            LocationData->gdopValid = true;
            LocationData->tdopValid = true;
            LocationData->magneticDeviationValid = true;
            LocationData->satsInViewCountValid = true;
            LocationData->satsTrackingCountValid = true;
            LocationData->satsUsedCountValid = true;
            LocationData->satInfoValid = true;
            LocationData->satMeasValid = false;
            LocationData->conformityValid = true;
            LocationData->confidencePercentValid = true;
            LocationData->calibrationStatusValid = true;
            LocationData->GnssKinematicsDataValid = true;
            LocationData->vrpLatitudeValid = true;
            LocationData->vrpLongitudeValid = true;
            LocationData->vrpAltitudeValid = true;
            LocationData->eastVelValid = true;

            LocationData->northVelValid = true;
            LocationData->upVelValid = true;
            LocationData->svDataValid = true;
            LocationData->sbasMaskValid = true;
            LocationData->validityMaskValid = true;
            LocationData->validityExMaskValid = true;
            LocationData->engMaskValid = true;
            LocationData->locationEngTypeValid = true;
            LocationData->horiReliablityValid = true;
            LocationData->vertReliablityValid = true;
            LocationData->azimuthValid = true;
            LocationData->eastDevValid = true;
            LocationData->northDevValid = true;
            LocationData->realTimeValid = true;
            LocationData->realTimeUncValid = true;
            LocationData->techMaskValid = true;
            LocationData->gnssDataValid = true;
            LocationData->gPtpTimeValid = true;
            LocationData->gPtpTimeUncValid = true;
            LocationData->drSolutionStatusValid = true;
            LocationData->leapSecondsUncValid = true;
            LocationData->navSolutionMaskValid = true;
            LocationData->protectionLevelAlongTrackValid = true;
            LocationData->protectionLevelCrossTrackValid = true;
            LocationData->protectionLevelVerticalValid = true;
            LocationData->baselineLengthValid = true;
            LocationData->ageCorrectionsValid = true;
            LocationData->integrityRiskUsedValid = true;
            if(locationInfo->mAltType == TAF_PA_LOCATION_CALCULATED)
            {
                clientRequestPtr->mAltType = TAF_LOCGNSS_ALT_TYPE_CALCULATED;
            }
            else if(locationInfo->mAltType == TAF_PA_LOCATION_ASSUMED)
            {
                clientRequestPtr->mAltType = TAF_LOCGNSS_ALT_TYPE_ASSUMED;
            }
            else
            {
                clientRequestPtr->mAltType = TAF_LOCGNSS_ALT_TYPE_UNKNOWN;
            }
            for (auto i = 0; i < TAF_LOCGNSS_MEASUREMENT_INFO_MAX; i++) {
                memset((void*) &LocationData->measInfo[i], 0, sizeof(taf_locGnss_GnssMeasurementInfo_t));
            }
            LocationData->measInfoCount = 0;
            std::vector<taf_pa_location_GnssMeasurementInfo_t> measInfo = locationInfo->measInfoData;
            for (auto measInfoElement : measInfo) {
                if (LocationData->measInfoCount < TAF_LOCGNSS_MEASUREMENT_INFO_MAX) {
                    LocationData->measInfo[LocationData->measInfoCount].gnssSignalType = measInfoElement.gnssSignalType;
                    LocationData->measInfo[LocationData->measInfoCount].gnssConstellation = (taf_locGnss_GnssSystem_t) measInfoElement.gnssConstellation;
                    LocationData->measInfo[LocationData->measInfoCount].gnssSvId = measInfoElement.gnssSvId;
                    LocationData->measInfoCount++;
                }
            }

            std::vector<uint16_t> SVIds = locationInfo->SVIdData;

            LocationData->SVIdsCount = 0;
            if(SVIds.size() > 0) {
                for (auto i = 0; i < TAF_LOCGNSS_MEASUREMENT_INFO_MAX; i++) {
                    LocationData->SVIds[i] = 0;
                }
                for (auto i = 0; i < (int) SVIds.size(); i++) {
                    if (LocationData->SVIdsCount < TAF_LOCGNSS_MEASUREMENT_INFO_MAX) {
                        LocationData->SVIds[LocationData->SVIdsCount] = SVIds.at(i);
                        LocationData->SVIdsCount++;
                    }
                }
            }

            LE_DEBUG("LocationData->SVIdsCount: %d",(int)LocationData->SVIdsCount);

            LE_DEBUG("Lat: %.10f, Lon: %.10f, HUnc: %lf",
                    locationInfo->latitude,
                    locationInfo->longitude,
                    locationInfo->hUncertainity);

            LE_DEBUG("Alt: %.10f, VUnc: %.10f, TimeAcc: %lf",
                    locationInfo->altitude,
                    locationInfo->vUncertainity,
                    (float)locationInfo->timeAccuracy);

            LE_DEBUG("Conformity: %lf, CalibStatus: %d, DRStatus: %d",
                    locationInfo->robustConformity,
                    locationInfo->calibrationStatus,
                    locationInfo->drSolutionStatus);

            LE_DEBUG("KinMask: %0x, ValidMask: %u, ValidEx: %" PRIu64,
                    locationInfo->GnssKinematicsData.bodyFrameDataMask,
                    locationInfo->validityMask,
                    locationInfo->validityExMask);

            LE_DEBUG("EngineMask: %" PRIu32 ", TechMask: %" PRIu32,
                    locationInfo->engMask,
                    locationInfo->techMask);

            LocationData->reportStatus = (taf_locGnss_ReportStatus_t) locationInfo->reportStatus;
            LocationData->altMeanSeaLevel = locationInfo->altMeanSeaLevel;
            locData = 0.0;
            locData = locationInfo->latitude;
            roundOffLocationData(&locData,6);//round off to 6 decimal places
            LocationData->latitude = (int32_t)locData;
            locData = 0.0;
            locData = locationInfo->longitude;
            roundOffLocationData(&locData,6);//round off to 6 decimal places
            LocationData->longitude = (int32_t)locData;
            locData = 0.0;
            locData = locationInfo->hUncertainity;
            roundOffLocationData(&locData,2);//round off to 2 decimal places
            LocationData->hAccuracy = (int32_t)locData;
            locData = 0.0;
            locData = locationInfo->altitude;
            roundOffLocationData(&locData,3);//round off to 3 decimal places
            LocationData->altitude = (int32_t)locData;
            locData = 0.0;
            locData = locationInfo->vUncertainity;
            roundOffLocationData(&locData,1);//round off to 1 decimal place
            LocationData->vAccuracy = (int32_t)locData;
            LocationData->altitudeOnWgs84 = 0;
            LocationData->hSpeed = locationInfo->hSpeed*100;
            LocationData->hSpeedAccuracy = locationInfo->hSpeedUncertainity*1e+3;
            if(locationInfo->verticalSpeed.size() == VERTICAL_SPEED_SIZE)
            {
                LocationData->vSpeed = (int32_t)
                    (locationInfo->verticalSpeed[VERTICAL_SPEED_ACCURACY_INDEX]*100);
                LE_DEBUG("LocationData->vSpeed:%d;  verticalSpeed:%lf",
                    LocationData->vSpeed, locationInfo->verticalSpeed[locationInfo->verticalSpeed.size()-1]);
            }else{
                LocationData->vSpeed = 0;
            }

            if(locationInfo->verticalSpeedAccuracy.size() == VERTICAL_SPEED_SIZE)
            {
                LocationData->vSpeedAccuracy = (int32_t)
                    (locationInfo->verticalSpeedAccuracy[VERTICAL_SPEED_ACCURACY_INDEX]*1000);
                LE_DEBUG("LocationData->vSpeedAccuracy:%d; verticalSpeedAccuracy:%lf",
                    LocationData->vSpeedAccuracy, locationInfo->verticalSpeedAccuracy[locationInfo->verticalSpeed.size()-1]);
            }
            else
            {
                LocationData->vSpeedAccuracy = 0;
            }
            LocationData->magneticDeviation = locationInfo->magneticDeviation*10;
            LocationData->epochTime = locationInfo->epochTime;
            LocationData->horUncEllipseSemiMajor =
                    locationInfo->horUncEllipseSemiMajor;
            LocationData->horUncEllipseSemiMinor =
                    locationInfo->horUncEllipseSemiMinor;
            LocationData->direction = locationInfo->direction*10;
            LocationData->directionAccuracy = locationInfo->directionAccuracy*10;
            LocationData->gpsWeek = locationInfo->gpsWeek;
            LocationData->gpsTimeOfWeek = locationInfo->gpsTimeOfWeek;

            float value = locationInfo->timeAccuracy;
            if ((value * 1e+6) >= UINT32_MAX)
            {
                LocationData->timeAccuracy = UINT32_MAX;
            }
            else
            {
                // do the round off to nanosecond
                LocationData->timeAccuracy = (uint32_t)((uint64_t)((value*1e+7)+5))/10;
            }
            LocationData->positionLatency = 0;
            LocationData->hdop = locationInfo->hdop *1e+3;
            LocationData->vdop = locationInfo->vdop * 1e+3;
            LocationData->pdop = locationInfo->pdop * 1e+3;
            LocationData->gdop = locationInfo->gdop * 1e+3;
            LocationData->tdop = locationInfo->tdop * 1e+3;
            if(locationInfo->timeStamp != 0) {
                time_t realtime;
                realtime = (time_t)((locationInfo->timeStamp / 1000));
                tm *ltm = gmtime(&realtime);
                if(ltm != NULL)
                {
                    LocationData->year = 1900+ltm->tm_year;
                    LocationData->month = 1+ltm->tm_mon;
                    LocationData->day = ltm->tm_mday;
                    //To match UTC time
                    LocationData->hours = ltm->tm_hour;
                    LocationData->minutes = ltm->tm_min;
                    LocationData->seconds = ltm->tm_sec;
                    LocationData->milliseconds = (locationInfo->timeStamp)%1000;
                }
                else
                {
                    LE_ERROR("onDetailedEngineLocationUpdate local time ltm is NULL");
                }
            } else {
                //UNKNOWN_TIMESTAMP which is zero(as UTC timeStamp has elapsed since
                //January 1, 1970, it cannot be 0)
                LocationData->year = 1970;
                LocationData->month = 1;
                LocationData->day = 1;
                LocationData->hours = 0;
                LocationData->minutes = 0;
                LocationData->seconds = 0;
                LocationData->milliseconds = 0;
            }
            LocationData->horConfidence = 39;
            LocationData->leapSeconds = locationInfo->leapSeconds;
            LocationData->leapSecondsUnc = locationInfo->leapSecondsUnc;
            LocationData->satsInViewCount = clientRequestPtr->mSatParams.satsInViewCount;
            LocationData->satsTrackingCount = clientRequestPtr->mTotalSVTracked;
            LocationData->satsUsedCount = locationInfo->satsUsedCount;

            for(auto i=0; i<TAF_LOCGNSS_SV_INFO_MAX_LEN; i++)
            {
                LocationData->satInfo[i].satId = clientRequestPtr->mSatInfo[i].satId;
                LocationData->satInfo[i].satConst = clientRequestPtr->mSatInfo[i].satConst;
                LocationData->satInfo[i].satUsed = clientRequestPtr->mSatInfo[i].satUsed;
                LocationData->satInfo[i].satUsedDgnss = false;
                LocationData->satInfo[i].satTracked = clientRequestPtr->mSatInfo[i].satTracked;
                LocationData->satInfo[i].satSnr = clientRequestPtr->mSatInfo[i].satSnr;
                LocationData->satInfo[i].satAzim = clientRequestPtr->mSatInfo[i].satAzim;
                LocationData->satInfo[i].satElev = clientRequestPtr->mSatInfo[i].satElev;
                LocationData->satInfo[i].signalType = clientRequestPtr->mSatInfo[i].signalType;
                LocationData->satInfo[i].glonassFcn = clientRequestPtr->mSatInfo[i].glonassFcn;
                LocationData->satInfo[i].baseBandCnr = clientRequestPtr->mSatInfo[i].baseBandCnr;
            }

            for(auto i=0; i<TAF_LOCGNSS_SV_INFO_MAX_LEN; i++)
            {
                LocationData->satMeas[i].satId = 0;
                LocationData->satMeas[i].satLatency = 0;
            }
            LocationData->robustConformity = locationInfo->robustConformity;
            LocationData->confidencePercent = locationInfo->confidencePercent;
            taf_pa_location_DrCalibrationStatusType_t calibrationStatus =
                                            locationInfo->calibrationStatus;
            if((calibrationStatus & TAF_PA_LOCATION_DR_ROLL_CALIBRATION_NEEDED))
            {
                LocationData->calibrationStatus |= ((1<<TAF_LOCGNSS_DR_ROLL_CALIBRATION_NEEDED));
            }
            if((calibrationStatus & TAF_PA_LOCATION_DR_PITCH_CALIBRATION_NEEDED))
            {
                LocationData->calibrationStatus |= ((1<<TAF_LOCGNSS_DR_PITCH_CALIBRATION_NEEDED));
            }
            if((calibrationStatus & TAF_PA_LOCATION_DR_YAW_CALIBRATION_NEEDED))
            {
                LocationData->calibrationStatus |= ((1<<TAF_LOCGNSS_DR_YAW_CALIBRATION_NEEDED));
            }
            if((calibrationStatus & TAF_PA_LOCATION_DR_ODO_CALIBRATION_NEEDED))
            {
                LocationData->calibrationStatus |= ((1<<TAF_LOCGNSS_DR_ODO_CALIBRATION_NEEDED));
            }
            if((calibrationStatus & TAF_PA_LOCATION_DR_GYRO_CALIBRATION_NEEDED))
            {
                LocationData->calibrationStatus |= ((1<<TAF_LOCGNSS_DR_GYRO_CALIBRATION_NEEDED));
            }
            taf_pa_location_DrSolutionStatusType_t solutionStatus =
                                            locationInfo->drSolutionStatus;
            if((solutionStatus & TAF_PA_LOCATION_VEHICLE_SENSOR_SPEED_INPUT_DETECTED))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_VEHICLE_SENSOR_SPEED_INPUT_DETECTED;
            }
            if((solutionStatus & TAF_PA_LOCATION_VEHICLE_SENSOR_SPEED_INPUT_USED))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_VEHICLE_SENSOR_SPEED_INPUT_USED;
            }
            if((solutionStatus & TAF_PA_LOCATION_WARNING_UNCALIBRATED))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_UNCALIBRATED;
            }
            if((solutionStatus & TAF_PA_LOCATION_WARNING_GNSS_QUALITY_INSUFFICIENT))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_GNSS_QUALITY_INSUFFICIENT;
            }
            if((solutionStatus & TAF_PA_LOCATION_WARNING_FERRY_DETECTED))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_FERRY_DETECTED;
            }
            if((solutionStatus & TAF_PA_LOCATION_ERROR_6DOF_SENSOR_UNAVAILABLE))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_ERROR_6DOF_SENSOR_UNAVAILABLE;
            }
            if((solutionStatus & TAF_PA_LOCATION_ERROR_VEHICLE_SPEED_UNAVAILABLE))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_ERROR_VEHICLE_SPEED_UNAVAILABLE;
            }
            if((solutionStatus & TAF_PA_LOCATION_ERROR_GNSS_EPH_UNAVAILABLE))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_ERROR_GNSS_EPH_UNAVAILABLE;
            }
            if((solutionStatus & TAF_PA_LOCATION_ERROR_GNSS_MEAS_UNAVAILABLE))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_ERROR_GNSS_MEAS_UNAVAILABLE;
            }
            if((solutionStatus & TAF_PA_LOCATION_WARNING_INIT_POSITION_INVALID))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_INIT_POSITION_INVALID;
            }
            if((solutionStatus & TAF_PA_LOCATION_WARNING_INIT_POSITION_UNRELIABLE))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_INIT_POSITION_UNRELIABLE;
            }
            if((solutionStatus & TAF_PA_LOCATION_WARNING_POSITON_UNRELIABLE))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_POSITON_UNRELIABLE;
            }
            if((solutionStatus & TAF_PA_LOCATION_ERROR_GENERIC))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_ERROR_GENERIC;
            }
            if((solutionStatus & TAF_PA_LOCATION_WARNING_SENSOR_TEMP_OUT_OF_RANGE))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_SENSOR_TEMP_OUT_OF_RANGE;
            }
            if((solutionStatus & TAF_PA_LOCATION_WARNING_FACTORY_DATA_INCONSISTENT))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_USER_DYNAMICS_INSUFFICIENT;
            }
            if((solutionStatus & TAF_PA_LOCATION_WARNING_USER_DYNAMICS_INSUFFICIENT))
            {
                LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_FACTORY_DATA_INCONSISTENT;
            }
            taf_pa_location_GnssKinematicsData_t GnssKinData = locationInfo->GnssKinematicsData;
            taf_pa_location_KinematicDataValidityType_t GnssKinDataValidity =
                                              GnssKinData.bodyFrameDataMask;
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_LONG_ACCEL))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_LONG_ACCEL);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_LAT_ACCEL))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_LAT_ACCEL);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_VERT_ACCEL))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_VERT_ACCEL);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_YAW_RATE))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_YAW_RATE);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_PITCH))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |= (1<<TAF_LOCGNSS_HAS_PITCH);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_LONG_ACCEL_UNC))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_LONG_ACCEL_UNC);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_LAT_ACCEL_UNC))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_LAT_ACCEL_UNC);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_VERT_ACCEL_UNC))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_VERT_ACCEL_UNC);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_YAW_RATE_UNC))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_YAW_RATE_UNC);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_PITCH_UNC))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_PITCH_UNC);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_PITCH_RATE_BIT))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_PITCH_RATE_BIT);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_PITCH_RATE_UNC_BIT))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_PITCH_RATE_UNC_BIT);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_ROLL_BIT))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |= (1<<TAF_LOCGNSS_HAS_ROLL_BIT);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_ROLL_UNC_BIT))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_ROLL_UNC_BIT);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_ROLL_RATE_BIT))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_ROLL_RATE_BIT);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_ROLL_RATE_UNC_BIT))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_ROLL_RATE_UNC_BIT);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_YAW_BIT))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |= (1<<TAF_LOCGNSS_HAS_YAW_BIT);
            }
            if((GnssKinDataValidity & TAF_PA_LOCATION_HAS_YAW_UNC_BIT))
            {
                LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                 (1<<TAF_LOCGNSS_HAS_YAW_UNC_BIT);
            }
            else
            {
                LE_DEBUG("Navigation data is not found");
            }
            LocationData->GnssKinematicsData.longAccel = GnssKinData.longAccel;
            LocationData->GnssKinematicsData.latAccel = GnssKinData.latAccel;
            LocationData->GnssKinematicsData.vertAccel = GnssKinData.vertAccel;
            LocationData->GnssKinematicsData.yawRate = GnssKinData.yawRate;
            LocationData->GnssKinematicsData.pitch = GnssKinData.pitch;
            LocationData->GnssKinematicsData.longAccelUnc = GnssKinData.longAccelUnc;
            LocationData->GnssKinematicsData.latAccelUnc = GnssKinData.latAccelUnc;
            LocationData->GnssKinematicsData.vertAccelUnc = GnssKinData.vertAccelUnc;
            LocationData->GnssKinematicsData.yawRateUnc = GnssKinData.yawRateUnc;
            LocationData->GnssKinematicsData.pitchUnc = GnssKinData.pitchUnc;
            LocationData->GnssKinematicsData.pitchRate = GnssKinData.pitchRate;
            LocationData->GnssKinematicsData.pitchRateUnc = GnssKinData.pitchRateUnc;
            LocationData->GnssKinematicsData.roll = GnssKinData.roll;
            LocationData->GnssKinematicsData.rollUnc = GnssKinData.rollUnc;
            LocationData->GnssKinematicsData.rollRate = GnssKinData.rollRate;
            LocationData->GnssKinematicsData.rollRateUnc = GnssKinData.rollRateUnc;
            LocationData->GnssKinematicsData.yaw = GnssKinData.yaw;
            LocationData->GnssKinematicsData.yawUnc = GnssKinData.yawUnc;

            LocationData->vrpLatitude = locationInfo->vrpLatitude;
            LocationData->vrpLongitude = locationInfo->vrpLongitude;
            LocationData->vrpAltitude = locationInfo->vrpAltitude;
            LocationData->eastVel = locationInfo->eastVel;
            LocationData->northVel = locationInfo->northVel;
            LocationData->upVel = locationInfo->upVel;
            taf_pa_location_SvUsedInPosition_t svData = locationInfo->svData;
            LocationData->svData.gps = svData.gps;
            LocationData->svData.glo = svData.glo;
            LocationData->svData.gal = svData.gal;
            LocationData->svData.bds = svData.bds;
            LocationData->svData.qzss = svData.qzss;
            LocationData->svData.navic = svData.navic;
            std::string correction = locationInfo->sbasMask;
            std::bitset<TAF_PA_LOCATION_SBAS_COUNT> sbasData(correction);
            LE_DEBUG("sbasData:%s",sbasData.to_string().c_str());
            if(sbasData[(taf_pa_location_SbasCorrectionType_t)TAF_PA_LOCATION_SBAS_CORRECTION_IONO])
            {
                LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTION_IONO);
            }
            if(sbasData[(taf_pa_location_SbasCorrectionType_t)TAF_PA_LOCATION_SBAS_CORRECTION_FAST])
            {
                LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTION_FAST);
            }
            if(sbasData[(taf_pa_location_SbasCorrectionType_t)TAF_PA_LOCATION_SBAS_CORRECTION_LONG])
            {
                LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTION_LONG);
            }
            if(sbasData[(taf_pa_location_SbasCorrectionType_t)TAF_PA_LOCATION_SBAS_INTEGRITY])
            {
                LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_INTEGRITY);
            }
            if(sbasData[(taf_pa_location_SbasCorrectionType_t)TAF_PA_LOCATION_SBAS_CORRECTION_DGNSS])
            {
                LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTION_DGNSS);
            }
            if(sbasData[(taf_pa_location_SbasCorrectionType_t)TAF_PA_LOCATION_SBAS_CORRECTION_RTK])
            {
                LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTION_RTK);
            }
            if(sbasData[(taf_pa_location_SbasCorrectionType_t)TAF_PA_LOCATION_SBAS_CORRECTION_PPP])
            {
                LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTION_PPP);
            }
            if(sbasData[(taf_pa_location_SbasCorrectionType_t)TAF_PA_LOCATION_SBAS_CORRECTION_RTK_FIXED])
            {
                LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTION_RTK_FIXED);
            }
            if(sbasData[(taf_pa_location_SbasCorrectionType_t)
                            TAF_PA_LOCATION_SBAS_CORRECTED_SV_USED])
            {
                LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTED_SV_USED);
            }
            taf_pa_location_LocationValidityType_t validityMask = locationInfo->validityMask;
            if((validityMask & TAF_PA_LOCATION_HAS_LAT_LONG_BIT))
            {
                LocationData->validityMask |= TAF_LOCGNSS_HAS_LAT_LONG_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_ALTITUDE_BIT))
            {
                LocationData->validityMask |= TAF_LOCGNSS_HAS_ALTITUDE_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_SPEED_BIT))
            {
                LocationData->validityMask |= TAF_LOCGNSS_HAS_SPEED_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_HEADING_BIT))
            {
                LocationData->validityMask |= TAF_LOCGNSS_HAS_HEADING_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_HORIZONTAL_ACCURACY_BIT))
            {
                LocationData->validityMask |= TAF_LOCGNSS_HAS_HORIZONTAL_ACCURACY_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_VERTICAL_ACCURACY_BIT))
            {
                LocationData->validityMask |= TAF_LOCGNSS_HAS_VERTICAL_ACCURACY_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_SPEED_ACCURACY_BIT))
            {
                LocationData->validityMask |= TAF_LOCGNSS_HAS_SPEED_ACCURACY_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_HEADING_ACCURACY_BIT))
            {
                LocationData->validityMask |= TAF_LOCGNSS_HAS_HEADING_ACCURACY_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_TIMESTAMP_BIT))
            {
                LocationData->validityMask |= TAF_LOCGNSS_HAS_TIMESTAMP_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_ELAPSED_REAL_TIME_BIT))
            {
                LocationData->validityMask |= TAF_LOCGNSS_HAS_ELAPSED_REAL_TIME_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_ELAPSED_REAL_TIME_UNC_BIT))
            {
                LocationData->validityMask |= TAF_LOCGNSS_HAS_ELAPSED_REAL_TIME_UNC_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_GPTP_TIME_BIT))
            {
                LocationData->validityMask |= TAF_LOCGNSS_HAS_GPTP_TIME_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_GPTP_TIME_UNC_BIT))
            {
                LocationData->validityMask |= TAF_LOCGNSS_HAS_GPTP_TIME_UNC_BIT;
            }
            taf_pa_location_LocationInfoExValidityType_t validityExMask =
                                               locationInfo->validityExMask;
            if((validityExMask & TAF_PA_LOCATION_HAS_ALTITUDE_MEAN_SEA_LEVEL))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_ALTITUDE_MEAN_SEA_LEVEL);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_DOP))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_DOP);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_MAGNETIC_DEVIATION))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_MAGNETIC_DEVIATION);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_HOR_RELIABILITY))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_HOR_RELIABILITY);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_VER_RELIABILITY))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_VER_RELIABILITY);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_HOR_ACCURACY_ELIP_SEMI_MAJOR))
            {
                LocationData->validityExMask |=
                                             (1ULL << TAF_LOCGNSS_HAS_HOR_ACCURACY_ELIP_SEMI_MAJOR);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_HOR_ACCURACY_ELIP_SEMI_MINOR))
            {
                LocationData->validityExMask |=
                                             (1ULL << TAF_LOCGNSS_HAS_HOR_ACCURACY_ELIP_SEMI_MINOR);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_HOR_ACCURACY_ELIP_AZIMUTH))
            {
                LocationData->validityExMask |=
                                             (1ULL << TAF_LOCGNSS_HAS_HOR_ACCURACY_ELIP_AZIMUTH);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_GNSS_SV_USED_DATA))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_GNSS_SV_USED_DATA);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_NAV_SOLUTION_MASK))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_NAV_SOLUTION_MASK);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_POS_TECH_MASK))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_POS_TECH_MASK);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_SV_SOURCE_INFO))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_SV_SOURCE_INFO);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_POS_DYNAMICS_DATA))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_POS_DYNAMICS_DATA);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_EXT_DOP))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_EXT_DOP);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_NORTH_STD_DEV))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_NORTH_STD_DEV);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_EAST_STD_DEV))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_EAST_STD_DEV);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_NORTH_VEL))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_NORTH_VEL);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_EAST_VEL))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_EAST_VEL);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_UP_VEL))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_UP_VEL);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_NORTH_VEL_UNC))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_NORTH_VEL_UNC);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_EAST_VEL_UNC))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_EAST_VEL_UNC);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_UP_VEL_UNC))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_UP_VEL_UNC);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_LEAP_SECONDS))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_LEAP_SECONDS);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_TIME_UNC))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_TIME_UNC);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_NUM_SV_USED_IN_POSITION))
            {
                LocationData->validityExMask |=
                                           (1ULL << TAF_LOCGNSS_HAS_NUM_SV_USED_IN_POSITION);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_CALIBRATION_CONFIDENCE_PERCENT))
            {
                LocationData->validityExMask |=
                                           (1ULL << TAF_LOCGNSS_HAS_CALIBRATION_CONFIDENCE_PERCENT);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_CALIBRATION_STATUS))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_CALIBRATION_STATUS);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_OUTPUT_ENG_TYPE))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_OUTPUT_ENG_TYPE);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_OUTPUT_ENG_MASK))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_OUTPUT_ENG_MASK);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_CONFORMITY_INDEX_FIX))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_CONFORMITY_INDEX_FIX);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_LLA_VRP_BASED))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_LLA_VRP_BASED);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_ENU_VELOCITY_VRP_BASED))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_ENU_VELOCITY_VRP_BASED);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_ALTITUDE_TYPE))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_ALTITUDE_TYPE);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_REPORT_STATUS))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_REPORT_STATUS);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_INTEGRITY_RISK_USED))
            {
                LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_INTEGRITY_RISK_USED);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_PROTECT_LEVEL_ALONG_TRACK))
            {
                LocationData->validityExMask |=
                                            (1ULL << TAF_LOCGNSS_HAS_PROTECT_LEVEL_ALONG_TRACK);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_PROTECT_LEVEL_CROSS_TRACK))
            {
                LocationData->validityExMask |=
                                            (1ULL << TAF_LOCGNSS_HAS_PROTECT_LEVEL_CROSS_TRACK);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_PROTECT_LEVEL_VERTICAL))
            {
                LocationData->validityExMask |=
                                            (1ULL << TAF_LOCGNSS_HAS_PROTECT_LEVEL_VERTICAL);
            }
            if((validityExMask & TAF_PA_LOCATION_HAS_LEAP_SECONDS_UNC))
            {
                LocationData->validityExMask |=
                                            (1ULL << TAF_LOCGNSS_HAS_LEAP_SECONDS_UNC);
            }
            taf_pa_location_PositioningEngineType_t posEngineBits = locationInfo->engMask;
            if(posEngineBits & TAF_PA_LOCATION_STANDARD_POSITIONING_ENGINE)
            {
                LocationData->engMask |= TAF_LOCGNSS_STANDARD_POSITIONING_ENGINE;
            }
            if(posEngineBits & TAF_PA_LOCATION_DEAD_RECKONING_ENGINE)
            {
                LocationData->engMask |= TAF_LOCGNSS_DEAD_RECKONING_ENGINE;
            }
            if(posEngineBits & TAF_PA_LOCATION_PRECISE_POSITIONING_ENGINE)
            {
                LocationData->engMask |= TAF_LOCGNSS_PRECISE_POSITIONING_ENGINE;
            }
            if(posEngineBits & TAF_PA_LOCATION_VP_POSITIONING_ENGINE)
            {
                LocationData->engMask |= TAF_LOCGNSS_VP_POSITIONING_ENGINE;
            }
            taf_pa_location_LocationAggregationType_t locEngineType = locationInfo->locationEngType;
            if(locEngineType == TAF_PA_LOCATION_LOC_OUTPUT_ENGINE_FUSED)
            {
                LocationData->locationEngType = TAF_LOCGNSS_LOC_OUTPUT_ENGINE_FUSED;
            }
            if(locEngineType == TAF_PA_LOCATION_LOC_OUTPUT_ENGINE_SPE)
            {
                LocationData->locationEngType = TAF_LOCGNSS_LOC_OUTPUT_ENGINE_SPE;
            }
            if(locEngineType == TAF_PA_LOCATION_LOC_OUTPUT_ENGINE_PPE)
            {
                LocationData->locationEngType = TAF_LOCGNSS_LOC_OUTPUT_ENGINE_PPE;
            }
            if(locEngineType == TAF_PA_LOCATION_LOC_OUTPUT_ENGINE_VPE)
            {
                LocationData->locationEngType = TAF_LOCGNSS_LOC_OUTPUT_ENGINE_VPE;
            }
            taf_pa_location_LocationReliability_t locReliability =
                                               locationInfo->horiReliablity;
            if(locReliability == TAF_PA_LOCATION_NOT_SET)
            {
                LocationData->horiReliablity = TAF_LOCGNSS_RELIABILITY_NOT_SET;
            }
            else if(locReliability == TAF_PA_LOCATION_VERY_LOW)
            {
                LocationData->horiReliablity = TAF_LOCGNSS_RELIABILITY_VERY_LOW;
            }
            else if(locReliability == TAF_PA_LOCATION_LOW)
            {
                LocationData->horiReliablity = TAF_LOCGNSS_RELIABILITY_LOW;
            }
            else if(locReliability == TAF_PA_LOCATION_MEDIUM)
            {
                LocationData->horiReliablity = TAF_LOCGNSS_RELIABILITY_MEDIUM;
            }
            else if(locReliability == TAF_PA_LOCATION_HIGH)
            {
                LocationData->horiReliablity = TAF_LOCGNSS_RELIABILITY_HIGH;
            }
            else
            {
                LocationData->horiReliablity = TAF_LOCGNSS_RELIABILITY_UNKNOWN;
            }
            taf_pa_location_LocationReliability_t vertLocReliability =
                                               locationInfo->vertReliablity;
            if(vertLocReliability == TAF_PA_LOCATION_NOT_SET)
            {
                LocationData->vertReliablity = TAF_LOCGNSS_RELIABILITY_NOT_SET;
            }
            else if(vertLocReliability == TAF_PA_LOCATION_VERY_LOW)
            {
                LocationData->vertReliablity = TAF_LOCGNSS_RELIABILITY_VERY_LOW;
            }
            else if(vertLocReliability == TAF_PA_LOCATION_LOW)
            {
                LocationData->vertReliablity = TAF_LOCGNSS_RELIABILITY_LOW;
            }
            else if(vertLocReliability == TAF_PA_LOCATION_MEDIUM)
            {
                LocationData->vertReliablity = TAF_LOCGNSS_RELIABILITY_MEDIUM;
            }
            else if(vertLocReliability == TAF_PA_LOCATION_HIGH)
            {
                LocationData->vertReliablity = TAF_LOCGNSS_RELIABILITY_HIGH;
            }
            else
            {
                LocationData->vertReliablity = TAF_LOCGNSS_RELIABILITY_UNKNOWN;
            }
            LocationData->azimuth = locationInfo->azimuth;
            LocationData->eastDev = locationInfo->eastDev;
            LocationData->northDev = locationInfo->northDev;
            LocationData->realTime = locationInfo->realTime;
            LocationData->realTimeUnc = locationInfo->realTimeUnc;
            taf_pa_location_LocationTechnologyType_t techMask = locationInfo->techMask;
            if((techMask & TAF_PA_LOCATION_LOC_GNSS))
            {
                LocationData->techMask |= TAF_LOCGNSS_LOC_GNSS;
            }
            if((techMask & TAF_PA_LOCATION_LOC_CELL))
            {
                LocationData->techMask |= TAF_LOCGNSS_LOC_CELL;
            }
            if((techMask & TAF_PA_LOCATION_LOC_WIFI))
            {
                LocationData->techMask |= TAF_LOCGNSS_LOC_WIFI;
            }
            if((techMask & TAF_PA_LOCATION_LOC_SENSORS))
            {
                LocationData->techMask |= TAF_LOCGNSS_LOC_SENSORS;
            }
            if((techMask & TAF_PA_LOCATION_LOC_REFERENCE_LOCATION))
            {
                LocationData->techMask |= TAF_LOCGNSS_LOC_REFERENCE_LOCATION;
            }
            if((techMask & TAF_PA_LOCATION_LOC_INJECTED_COARSE_POSITION))
            {
                LocationData->techMask |= TAF_LOCGNSS_LOC_INJECTED_COARSE_POSITION;
            }
            if((techMask & TAF_PA_LOCATION_LOC_AFLT))
            {
                LocationData->techMask |= TAF_LOCGNSS_LOC_AFLT;
            }
            if((techMask & TAF_PA_LOCATION_LOC_HYBRID))
            {
                LocationData->techMask |= TAF_LOCGNSS_LOC_HYBRID;
            }
            if((techMask & TAF_PA_LOCATION_LOC_PPE))
            {
                LocationData->techMask |= TAF_LOCGNSS_LOC_PPE;
            }
            if((techMask & TAF_PA_LOCATION_LOC_VEH))
            {
                LocationData->techMask |= TAF_LOCGNSS_LOC_VEH;
            }
            if((techMask & TAF_PA_LOCATION_LOC_VIS))
            {
                LocationData->techMask |= TAF_LOCGNSS_LOC_VIS;
            }
            if((techMask & TAF_PA_LOCATION_LOC_PROPAGATED))
            {
                LocationData->techMask |= TAF_LOCGNSS_LOC_PROPAGATED;
            }

            for(auto i=0; i<TAF_LOCGNSS_NUMBER_OF_SIGNAL_TYPES_MAX; i++)
            {
                LocationData->gnssData[i].gnssDataMask = clientRequestPtr->mGnssData[i].gnssDataMask;
                LocationData->gnssData[i].jammerInd = clientRequestPtr->mGnssData[i].jammerInd;
                LocationData->gnssData[i].agc = clientRequestPtr->mGnssData[i].agc;
            }

            std::vector<uint16_t> dgnssStations = locationInfo->dgnssStationIds;

            LocationData->dgnssStationIdsCount = 0;
            LE_DEBUG("dgnssStations.size(): %ld", dgnssStations.size());
            if (!dgnssStations.empty()) {
                for (auto dgnssStation : dgnssStations){
                    if (IsSbasStationId(dgnssStation))
                    {
                        for (int n = 0; n < TAF_LOCGNSS_SV_INFO_MAX_LEN; n++)
                        {
                            if (LocationData->satInfo[n].satId != 0 &&
                                LocationData->satInfo[n].satId != UINT16_MAX &&
                                LocationData->satInfo[n].satId == dgnssStation)
                            {
                                LE_DEBUG("DGNSS satellite (id=%d) is providing corrections", dgnssStation);
                                LocationData->satInfo[n].satUsedDgnss = true;
                                break;
                            }
                        }
                        continue;
                    }
                    if (dgnssStation < 1000)
                    {
                        LE_WARN("Invalid DGNSS station ID %d (<1000), ignore", dgnssStation);
                        continue;
                    }
                    if(LocationData->dgnssStationIdsCount >= TAF_LOCGNSS_MAX_MONITOR_STATION_IDS)
                    {
                        LE_ERROR("Maximum DGNSS station IDs reached, ignoring remaining stations");
                        break;
                    }
                    LocationData->dgnssStationIds[LocationData->dgnssStationIdsCount] = dgnssStation-1000;
                    LocationData->dgnssStationIdsCount++;
                }
                LE_DEBUG("DGNSS Station Count=%d", LocationData->dgnssStationIdsCount);
            }

            LocationData->navSolutionMask = locationInfo->naviSolution;
            LocationData->gPtpTime = locationInfo->gPtpTime;
            LocationData->gPtpTimeUnc = locationInfo->gPtpTimeUnc;
            LocationData->protectionLevelAlongTrack = locationInfo->protectionlevelAlongTrack;
            LocationData->protectionLevelCrossTrack = locationInfo->protectionlevelCrossTrack;
            LocationData->protectionLevelVertical = locationInfo->protectionlevelVertical;
            LocationData->ageCorrections = locationInfo->ageOfCorrections;
            LocationData->baselineLength = locationInfo->baselineLength;
            LocationData->integrityRiskUsed  = locationInfo->integrityRiskUsed;
            LocationData->next = LE_DLS_LINK_INIT;

            le_event_ReportWithRefCounting(gnss.positionEventId, LocationData);
        }
    }
    if(gnss.NumOfPositionExHandlers)
    {
        for (auto locationInfo : locationEngineInfo) {
            taf_locGnss_PositionExSample_t* BasicLocationData = (taf_locGnss_PositionExSample_t*)le_mem_ForceAlloc(gnss.PositionExSamplePoolRef);

            memset(BasicLocationData, 0, sizeof(taf_locGnss_PositionExSample_t));

            BasicLocationData->clientSessionRefPtr = &clientRequestPtr->sessionRef;

            double locData;
            locData = 0.0;
            locData = locationInfo->latitude;
            roundOffLocationData(&locData,6);//round off to 6 decimal places
            BasicLocationData->latitude = (int32_t)locData;
            locData = 0.0;
            locData = locationInfo->longitude;
            roundOffLocationData(&locData,6);//round off to 6 decimal places
            BasicLocationData->longitude = (int32_t)locData;
            locData = 0.0;
            locData = locationInfo->hUncertainity;
            roundOffLocationData(&locData,2);//round off to 2 decimal places
            BasicLocationData->hAccuracy = (int32_t)locData;
            locData = 0.0;
            locData = locationInfo->altitude;
            roundOffLocationData(&locData,3);//round off to 3 decimal places
            BasicLocationData->altitude = (int32_t)locData;
            locData = 0.0;
            locData = locationInfo->vUncertainity;
            roundOffLocationData(&locData,1);//round off to 1 decimal place
            BasicLocationData->vAccuracy = (int32_t)locData;
            BasicLocationData->hSpeed = locationInfo->hSpeed*100;
            BasicLocationData->direction = locationInfo->direction*10;
            BasicLocationData->directionAccuracy = locationInfo->directionAccuracy*10;
            BasicLocationData->epochTime = locationInfo->epochTime;
            BasicLocationData->realTime = locationInfo->realTime;
            BasicLocationData->realTimeUnc = locationInfo->realTimeUnc;
            BasicLocationData->hSpeedAccuracy = locationInfo->hSpeedUncertainity*1e+3;

            BasicLocationData->satsInViewCount = clientRequestPtr->mSatParams.satsInViewCount;
            BasicLocationData->satsTrackingCount = clientRequestPtr->mTotalSVTracked;
            BasicLocationData->satsUsedCount = locationInfo->satsUsedCount;

            BasicLocationData->hdop = locationInfo->hdop *1e+3;
            BasicLocationData->vdop = locationInfo->vdop * 1e+3;
            BasicLocationData->pdop = locationInfo->pdop * 1e+3;

            BasicLocationData->altMeanSeaLevel = locationInfo->altMeanSeaLevel;
            BasicLocationData->magneticDeviation = locationInfo->magneticDeviation*10;

            taf_pa_location_LocationTechnologyType_t techMask = locationInfo->techMask;
            if((techMask & TAF_PA_LOCATION_LOC_GNSS))
            {
                BasicLocationData->techMask |= TAF_LOCGNSS_LOC_GNSS;
            }
            if((techMask & TAF_PA_LOCATION_LOC_CELL))
            {
                BasicLocationData->techMask |= TAF_LOCGNSS_LOC_CELL;
            }
            if((techMask & TAF_PA_LOCATION_LOC_WIFI))
            {
                BasicLocationData->techMask |= TAF_LOCGNSS_LOC_WIFI;
            }
            if((techMask & TAF_PA_LOCATION_LOC_SENSORS))
            {
                BasicLocationData->techMask |= TAF_LOCGNSS_LOC_SENSORS;
            }
            if((techMask & TAF_PA_LOCATION_LOC_REFERENCE_LOCATION))
            {
                BasicLocationData->techMask |= TAF_LOCGNSS_LOC_REFERENCE_LOCATION;
            }
            if((techMask & TAF_PA_LOCATION_LOC_INJECTED_COARSE_POSITION))
            {
                BasicLocationData->techMask |= TAF_LOCGNSS_LOC_INJECTED_COARSE_POSITION;
            }
            if((techMask & TAF_PA_LOCATION_LOC_AFLT))
            {
                BasicLocationData->techMask |= TAF_LOCGNSS_LOC_AFLT;
            }
            if((techMask & TAF_PA_LOCATION_LOC_HYBRID))
            {
                BasicLocationData->techMask |= TAF_LOCGNSS_LOC_HYBRID;
            }
            if((techMask & TAF_PA_LOCATION_LOC_PPE))
            {
                BasicLocationData->techMask |= TAF_LOCGNSS_LOC_PPE;
            }
            if((techMask & TAF_PA_LOCATION_LOC_VEH))
            {
                BasicLocationData->techMask |= TAF_LOCGNSS_LOC_VEH;
            }
            if((techMask & TAF_PA_LOCATION_LOC_VIS))
            {
                BasicLocationData->techMask |= TAF_LOCGNSS_LOC_VIS;
            }
            if((techMask & TAF_PA_LOCATION_LOC_PROPAGATED))
            {
                BasicLocationData->techMask |= TAF_LOCGNSS_LOC_PROPAGATED;
            }
            taf_pa_location_LocationValidityType_t validityMask = locationInfo->validityMask;
            if((validityMask & TAF_PA_LOCATION_HAS_LAT_LONG_BIT))
            {
                BasicLocationData->validityMask |= TAF_LOCGNSS_HAS_LAT_LONG_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_ALTITUDE_BIT))
            {
                BasicLocationData->validityMask |= TAF_LOCGNSS_HAS_ALTITUDE_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_SPEED_BIT))
            {
                BasicLocationData->validityMask |= TAF_LOCGNSS_HAS_SPEED_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_HEADING_BIT))
            {
                BasicLocationData->validityMask |= TAF_LOCGNSS_HAS_HEADING_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_HORIZONTAL_ACCURACY_BIT))
            {
                BasicLocationData->validityMask |= TAF_LOCGNSS_HAS_HORIZONTAL_ACCURACY_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_VERTICAL_ACCURACY_BIT))
            {
                BasicLocationData->validityMask |= TAF_LOCGNSS_HAS_VERTICAL_ACCURACY_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_SPEED_ACCURACY_BIT))
            {
                BasicLocationData->validityMask |= TAF_LOCGNSS_HAS_SPEED_ACCURACY_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_HEADING_ACCURACY_BIT))
            {
                BasicLocationData->validityMask |= TAF_LOCGNSS_HAS_HEADING_ACCURACY_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_TIMESTAMP_BIT))
            {
                BasicLocationData->validityMask |= TAF_LOCGNSS_HAS_TIMESTAMP_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_ELAPSED_REAL_TIME_BIT))
            {
                BasicLocationData->validityMask |= TAF_LOCGNSS_HAS_ELAPSED_REAL_TIME_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_ELAPSED_REAL_TIME_UNC_BIT))
            {
                BasicLocationData->validityMask |= TAF_LOCGNSS_HAS_ELAPSED_REAL_TIME_UNC_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_GPTP_TIME_BIT))
            {
                BasicLocationData->validityMask |= TAF_LOCGNSS_HAS_GPTP_TIME_BIT;
            }
            if((validityMask & TAF_PA_LOCATION_HAS_GPTP_TIME_UNC_BIT))
            {
                BasicLocationData->validityMask |= TAF_LOCGNSS_HAS_GPTP_TIME_UNC_BIT;
            }

            le_event_ReportWithRefCounting(gnss.PositionExEventId, BasicLocationData);
        }
    }
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);

}

void Handler::onGnssMeasurementsInfo(taf_pa_location_LocationId clientId, const std::shared_ptr<taf_pa_location_GnssMeasurements_t>& measurementInfo, std::any context)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_Client_t* clientRequestPtr = NULL;

    le_msg_SessionRef_t sessionRef = std::any_cast<le_msg_SessionRef_t>(context);

    clientRequestPtr = gnss.DiscoverSessionRef(sessionRef);

    if (NULL == clientRequestPtr) {
        LE_ERROR("onGnssMeasurementsInfo did not find sessionRef: %p", sessionRef);
        return;
    }
    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);
    if(gnss.NumOfMeasurementHandlers ) {
        taf_locGnss_GnssMeasurements_t* MeasurementData =
                    (taf_locGnss_GnssMeasurements_t*)le_mem_ForceAlloc(gnss.MeasurementSamplePoolRef);

        taf_pa_location_GnssMeasurementsClockValidityType_t GnssMeasurementsClockValidityMask = measurementInfo->clock.valid;
        if((GnssMeasurementsClockValidityMask & TAF_PA_LOCATION_LEAP_SECOND_BIT))
        {
            MeasurementData->clock.valid |= TAF_LOCGNSS_LEAP_SECOND_BIT;
        }
        if((GnssMeasurementsClockValidityMask & TAF_PA_LOCATION_TIME_BIT))
        {
            MeasurementData->clock.valid |= TAF_LOCGNSS_TIME_BIT;
        }
        if((GnssMeasurementsClockValidityMask & TAF_PA_LOCATION_TIME_UNCERTAINTY_BIT))
        {
            MeasurementData->clock.valid |= TAF_LOCGNSS_TIME_UNCERTAINTY_BIT;
        }
        if((GnssMeasurementsClockValidityMask & TAF_PA_LOCATION_FULL_BIAS_BIT))
        {
            MeasurementData->clock.valid |= TAF_LOCGNSS_FULL_BIAS_BIT;
        }
        if((GnssMeasurementsClockValidityMask & TAF_PA_LOCATION_BIAS_BIT))
        {
            MeasurementData->clock.valid |=  TAF_LOCGNSS_BIAS_BIT;
        }
        if((GnssMeasurementsClockValidityMask & TAF_PA_LOCATION_BIAS_UNCERTAINTY_BIT))
        {
            MeasurementData->clock.valid |= TAF_LOCGNSS_BIAS_UNCERTAINTY_BIT;
        }
        if((GnssMeasurementsClockValidityMask & TAF_PA_LOCATION_DRIFT_BIT))
        {
            MeasurementData->clock.valid |= TAF_LOCGNSS_DRIFT_BIT;
        }
        if((GnssMeasurementsClockValidityMask & TAF_PA_LOCATION_DRIFT_UNCERTAINTY_BIT))
        {
            MeasurementData->clock.valid |= TAF_LOCGNSS_DRIFT_UNCERTAINTY_BIT;
        }
        if((GnssMeasurementsClockValidityMask & TAF_PA_LOCATION_HW_CLOCK_DISCONTINUITY_COUNT_BIT))
        {
            MeasurementData->clock.valid |= TAF_LOCGNSS_HW_CLOCK_DISCONTINUITY_COUNT_BIT;
        }
        if((GnssMeasurementsClockValidityMask & TAF_PA_LOCATION_ELAPSED_REAL_TIME_BIT))
        {
            MeasurementData->clock.valid |= TAF_LOCGNSS_ELAPSED_REAL_TIME_BIT;
        }
        if((GnssMeasurementsClockValidityMask & TAF_PA_LOCATION_ELAPSED_REAL_TIME_UNC_BIT))
        {
            MeasurementData->clock.valid |= TAF_LOCGNSS_ELAPSED_REAL_TIME_UNC_BIT;
        }
        if((GnssMeasurementsClockValidityMask & TAF_PA_LOCATION_ELAPSED_GPTP_TIME_BIT))
        {
            MeasurementData->clock.valid |= TAF_LOCGNSS_ELAPSED_GPTP_TIME_BIT;
        }
        if((GnssMeasurementsClockValidityMask & TAF_PA_LOCATION_ELAPSED_GPTP_TIME_UNC_BIT))
        {
            MeasurementData->clock.valid |= TAF_LOCGNSS_ELAPSED_GPTP_TIME_UNC_BIT;
        }

        MeasurementData->clock.leapSecond = measurementInfo->clock.leapSecond;
        MeasurementData->clock.timeNs = measurementInfo->clock.timeNs;
        MeasurementData->clock.timeUncertaintyNs = measurementInfo->clock.timeUncertaintyNs;
        MeasurementData->clock.fullBiasNs = measurementInfo->clock.fullBiasNs;
        MeasurementData->clock.biasNs = measurementInfo->clock.biasNs;
        MeasurementData->clock.biasUncertaintyNs = measurementInfo->clock.biasUncertaintyNs;
        MeasurementData->clock.driftNsps = measurementInfo->clock.driftNsps;
        MeasurementData->clock.driftUncertaintyNsps = measurementInfo->clock.driftUncertaintyNsps;
        MeasurementData->clock.hwClockDiscontinuityCount = measurementInfo->clock.hwClockDiscontinuityCount;
        MeasurementData->clock.elapsedRealTime = measurementInfo->clock.elapsedRealTime;
        MeasurementData->clock.elapsedRealTimeUnc = measurementInfo->clock.elapsedRealTimeUnc;
        MeasurementData->clock.elapsedgPTPTime = measurementInfo->clock.elapsedgPTPTime;
        MeasurementData->clock.elapsedgPTPTimeUnc = measurementInfo->clock.elapsedgPTPTimeUnc;

        LE_DEBUG("measurementInfo->measurements.size() : %d", (int)measurementInfo->measurements.size());

        for (auto i = 0; i < TAF_LOCGNSS_MEASUREMENT_INFO_MAX; i++) {
            memset((void*) &MeasurementData->measData[i], 0, sizeof(taf_locGnss_GnssMeasurementsData_t));
        }

        MeasurementData->measCount = 0;
        std::vector<taf_pa_location_GnssMeasurementsData_t> measInfoElements= measurementInfo->measurements;
        for (auto measInfoElement : measInfoElements) {
            if (MeasurementData->measCount < TAF_LOCGNSS_MEASUREMENT_INFO_MAX) {
                MeasurementData->measData[MeasurementData->measCount].svId = measInfoElement.svId;
                MeasurementData->measData[MeasurementData->measCount].timeOffsetNs = measInfoElement.timeOffsetNs;
                MeasurementData->measData[MeasurementData->measCount].stateMask = (taf_locGnss_GnssMeasurementsStateValidityType_t)measInfoElement.stateMask;

                MeasurementData->measData[MeasurementData->measCount].receivedSvTimeNs = measInfoElement.receivedSvTimeNs;
                MeasurementData->measData[MeasurementData->measCount].receivedSvTimeSubNs = measInfoElement.receivedSvTimeSubNs;
                MeasurementData->measData[MeasurementData->measCount].receivedSvTimeUncertaintyNs = measInfoElement.receivedSvTimeUncertaintyNs;
                MeasurementData->measData[MeasurementData->measCount].carrierToNoiseDbHz = measInfoElement.carrierToNoiseDbHz;
                MeasurementData->measData[MeasurementData->measCount].pseudorangeRateMps = measInfoElement.pseudorangeRateMps;
                MeasurementData->measData[MeasurementData->measCount].pseudorangeRateUncertaintyMps = measInfoElement.pseudorangeRateUncertaintyMps;
                MeasurementData->measData[MeasurementData->measCount].adrStateMask = (taf_locGnss_GnssMeasurementsAdrStateValidityType_t)measInfoElement.adrStateMask;
                MeasurementData->measData[MeasurementData->measCount].adrMeters = measInfoElement.adrMeters;
                MeasurementData->measData[MeasurementData->measCount].adrUncertaintyMeters = measInfoElement.adrUncertaintyMeters;
                MeasurementData->measData[MeasurementData->measCount].carrierFrequencyHz = measInfoElement.carrierFrequencyHz;
                MeasurementData->measData[MeasurementData->measCount].carrierCycles = measInfoElement.carrierCycles;
                MeasurementData->measData[MeasurementData->measCount].carrierPhase = measInfoElement.carrierPhase;
                MeasurementData->measData[MeasurementData->measCount].carrierPhaseUncertainty = measInfoElement.carrierPhaseUncertainty;
                MeasurementData->measData[MeasurementData->measCount].multipathIndicator = (taf_locGnss_GnssMeasurementsMultipathIndicator_t)measInfoElement.multipathIndicator;
                MeasurementData->measData[MeasurementData->measCount].signalToNoiseRatioDb = measInfoElement.signalToNoiseRatioDb;
                MeasurementData->measData[MeasurementData->measCount].agcLevelDb = measInfoElement.agcLevelDb;
                MeasurementData->measData[MeasurementData->measCount].gnssSignalType = (taf_locGnss_GnssSignalType_t)measInfoElement.gnssSignalType;
                MeasurementData->measData[MeasurementData->measCount].basebandCarrierToNoise = measInfoElement.basebandCarrierToNoise;
                MeasurementData->measData[MeasurementData->measCount].fullInterSignalBias = measInfoElement.fullInterSignalBias;
                MeasurementData->measData[MeasurementData->measCount].fullInterSignalBiasUncertainty = measInfoElement.fullInterSignalBiasUncertainty;


                taf_pa_location_GnssMeasurementsDataValidityType_t GnssMeasurementsDataValidityType = measInfoElement.valid;
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_SV_ID_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_SV_ID_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_SV_TYPE_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_SV_TYPE_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_STATE_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_STATE_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_RECEIVED_SV_TIME_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_RECEIVED_SV_TIME_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_RECEIVED_SV_TIME_UNCERTAINTY_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_RECEIVED_SV_TIME_UNCERTAINTY_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_CARRIER_TO_NOISE_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_CARRIER_TO_NOISE_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_PSEUDORANGE_RATE_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_PSEUDORANGE_RATE_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_PSEUDORANGE_RATE_UNCERTAINTY_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_PSEUDORANGE_RATE_UNCERTAINTY_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_ADR_STATE_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_ADR_STATE_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_ADR_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_ADR_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_ADR_UNCERTAINTY_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_ADR_UNCERTAINTY_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_CARRIER_FREQUENCY_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_CARRIER_FREQUENCY_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_CARRIER_CYCLES_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_CARRIER_CYCLES_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_CARRIER_PHASE_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_CARRIER_PHASE_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_CARRIER_PHASE_UNCERTAINTY_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_CARRIER_PHASE_UNCERTAINTY_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_MULTIPATH_INDICATOR_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_MULTIPATH_INDICATOR_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_SIGNAL_TO_NOISE_RATIO_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_SIGNAL_TO_NOISE_RATIO_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_AUTOMATIC_GAIN_CONTROL_BIT))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_AUTOMATIC_GAIN_CONTROL_BIT;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_GNSS_SIGNAL_TYPE))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_GNSS_SIGNAL_TYPE;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_BASEBAND_CARRIER_TO_NOISE))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_BASEBAND_CARRIER_TO_NOISE;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_FULL_ISB))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_FULL_ISB;
                }
                if((GnssMeasurementsDataValidityType & TAF_PA_LOCATION_FULL_ISB_UNCERTAINTY))
                {
                    MeasurementData->measData[MeasurementData->measCount].valid |= TAF_LOCGNSS_FULL_ISB_UNCERTAINTY;
                }

                switch(measInfoElement.svType) {
                    case TAF_PA_LOCATION_GPS:
                        MeasurementData->measData[MeasurementData->measCount].svType = TAF_LOCGNSS_SB_CONSTELLATION_GPS;
                        break;
                    case TAF_PA_LOCATION_GLONASS:
                        MeasurementData->measData[MeasurementData->measCount].svType = TAF_LOCGNSS_SB_CONSTELLATION_GLONASS;
                        break;
                    case TAF_PA_LOCATION_BDS:
                        MeasurementData->measData[MeasurementData->measCount].svType = TAF_LOCGNSS_SB_CONSTELLATION_BDS;
                        break;
                    case TAF_PA_LOCATION_GALILEO:
                        MeasurementData->measData[MeasurementData->measCount].svType = TAF_LOCGNSS_SB_CONSTELLATION_GALILEO;
                        break;
                    case TAF_PA_LOCATION_SBAS:
                        MeasurementData->measData[MeasurementData->measCount].svType = TAF_LOCGNSS_SB_CONSTELLATION_SBAS;
                        break;
                    case TAF_PA_LOCATION_QZSS:
                        MeasurementData->measData[MeasurementData->measCount].svType = TAF_LOCGNSS_SB_CONSTELLATION_QZSS;
                        break;
                    case TAF_PA_LOCATION_NAVIC:
                        MeasurementData->measData[MeasurementData->measCount].svType = TAF_LOCGNSS_SB_CONSTELLATION_NAVIC;
                        break;
                    default:
                        MeasurementData->measData[MeasurementData->measCount].svType = TAF_LOCGNSS_SB_CONSTELLATION_UNKNOWN;
                        break;
                }

                MeasurementData->measCount++;
            }
        }
        MeasurementData->isNHz = measurementInfo->isNHz;
        MeasurementData->agcStatusL1 = (taf_locGnss_AgcStatus_t)measurementInfo->agcStatusL1;
        MeasurementData->agcStatusL2 = (taf_locGnss_AgcStatus_t)measurementInfo->agcStatusL2;
        MeasurementData->agcStatusL5 = (taf_locGnss_AgcStatus_t)measurementInfo->agcStatusL5;
        MeasurementData->clientSessionRefPtr = &clientRequestPtr->sessionRef;

        le_event_ReportWithRefCounting(gnss.measurementEventId, MeasurementData);
    }
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);
}

void Handler::onLocationSystemInfo(taf_pa_location_LocationId clientId, const std::shared_ptr<taf_pa_location_LocationSystemInfo_t>& LocSysInfo, std::any context)
{

    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_Client_t* clientRequestPtr = NULL;

    le_msg_SessionRef_t sessionRef = std::any_cast<le_msg_SessionRef_t>(context);

    clientRequestPtr = gnss.DiscoverSessionRef(sessionRef);

    if (NULL == clientRequestPtr) {
        LE_CRIT("onLocationSystemInfo did not find sessionRef: %p", sessionRef);
        return;
    }

    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);

    LE_DEBUG("locationSystemInfoValidity:%d; LeapSecondInfoValidity:%d",LocSysInfo->validLocationSystemInfoMask,LocSysInfo->validLeapSecondSysInfoMask);

    uint32_t locationSystemInfoMask = LocSysInfo->validLocationSystemInfoMask;
    uint32_t leapSecondSysInfoMask = LocSysInfo->validLeapSecondSysInfoMask;
    if(locationSystemInfoMask & TAF_PA_LOCATION_LOCATION_SYS_INFO_LEAP_SECOND)
    {
        clientRequestPtr->mCurrentLeapSeconds = LocSysInfo->current;
        clientRequestPtr->mCurrentLeapSeconds = clientRequestPtr->mCurrentLeapSeconds * 1000;//millisseoncds
    }
    if(leapSecondSysInfoMask & TAF_PA_LOCATION_LEAP_SECOND_SYS_INFO_CURRENT_LEAP_SECONDS_BIT)
    {
        LE_DEBUG("onLocationSystemInfo: current leap second info is available.");
    }
    if(leapSecondSysInfoMask & TAF_PA_LOCATION_LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT)
    {
        LE_DEBUG(" The last known leap change event is available.");
    }

    LE_DEBUG("System time week: %u, week ms: %u, LeapSecondCurrent: %d",
        (unsigned)LocSysInfo->timeinfo.systemWeek,
        (unsigned)LocSysInfo->timeinfo.systemMsec,
        (unsigned)LocSysInfo->current);

    LE_DEBUG("System clk time: %lf, clk time uncertainty: %lf",
        LocSysInfo->timeinfo.systemClkTimeBias,
        LocSysInfo->timeinfo.systemClkTimeUncMs);

    LE_DEBUG("System reference valid: %u, num clock resets: %u",
        (unsigned)LocSysInfo->timeinfo.refFCount,
        (unsigned)LocSysInfo->timeinfo.numClockResets);

    LE_DEBUG("LeapSecondsBeforeChange: %u, LeapSecondsAfterChange: %u",
        (unsigned)LocSysInfo->leapSecondsBeforeChange,
        (unsigned)LocSysInfo->leapSecondsAfterChange);
    clientRequestPtr->mGpsTime = (uint64_t)(LocSysInfo->timeinfo.systemWeek  * 7);//7 days
    clientRequestPtr->mGpsTime = (uint64_t) clientRequestPtr->mGpsTime * 24;//24 hours
    clientRequestPtr->mGpsTime = (uint64_t) clientRequestPtr->mGpsTime* 3600;//seconds
    clientRequestPtr->mGpsTime = (uint64_t) clientRequestPtr->mGpsTime* 1000; //milli seconds
    clientRequestPtr->mGpsTime = (uint64_t) clientRequestPtr->mGpsTime + (unsigned)LocSysInfo->timeinfo.systemMsec;
    clientRequestPtr->mChangeEventTime = LocSysInfo->leapSecondsBeforeChange*1000;//msec
    clientRequestPtr->mNextLeapSeconds = LocSysInfo->leapSecondsAfterChange*1000;//msec
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);

}

void Handler::onCapabilitiesInfo(taf_pa_location_LocationId clientId, const std::shared_ptr<taf_pa_location_CapabilityChangeEvent_t>& capEventInfo, std::any context)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_Client_t* clientRequestPtr = NULL;

    le_msg_SessionRef_t sessionRef = std::any_cast<le_msg_SessionRef_t>(context);

    clientRequestPtr = gnss.DiscoverSessionRef(sessionRef);

    if (NULL == clientRequestPtr) {
        LE_CRIT("onCapabilitiesInfo did not find sessionRef: %p", sessionRef);
        return;
    }

    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);
    if(gnss.NumOfCapabilityHandlers) {
        CapabilityChangeEvent_t capabilityEvent;
        capabilityEvent.locCapability = (taf_locGnss_LocCapabilityType_t) capEventInfo->locCapability;
        le_event_Report(gnss.locCapabilityEventId, &capabilityEvent, sizeof(CapabilityChangeEvent_t));
    }
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);

}

void Handler::onXtraStatusUpdate(taf_pa_location_LocationId clientId, const std::shared_ptr<taf_pa_location_XtraStatus_t>& xtraStatus, std::any context)
{
    LE_DEBUG("Xtra Feature Enabled: %d; Xtra Feature Validity: %d; xtraDataStatus:%d",
        xtraStatus->featureEnabled, xtraStatus->xtraValidForHours, (int)xtraStatus->xtraDataStatus);
    switch(xtraStatus->xtraDataStatus)
    {
        case TAF_PA_LOCATION_STATUS_UNKNOWN:
            break;
        case TAF_PA_LOCATION_STATUS_NOT_AVAIL:
            break;
        case TAF_PA_LOCATION_STATUS_NOT_VALID:
            break;
        case TAF_PA_LOCATION_STATUS_VALID:
            break;
    }

}


void Handler::onDgnssStatusUpdate(const std::shared_ptr<tafpa::location::taf_pa_location_DgnssStatus_t>& dgnssStatus, std::any context)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_INFO("onDgnssStatusUpdate: DGNSS status = %d", dgnssStatus->status);

    taf_locGnss_Client_t* clientRequestPtr = NULL;

    le_msg_SessionRef_t sessionRef = std::any_cast<le_msg_SessionRef_t>(context);
    LE_INFO("sessionRef: %p", sessionRef);

    clientRequestPtr = gnss.DiscoverSessionRef(sessionRef);

    if (NULL == clientRequestPtr) {
        LE_ERROR("onDgnssStatusUpdate did not find sessionRef: %p", sessionRef);
        return;
    }
    LE_INFO("onDgnssStatusUpdate: NumOfDgnssStatusHandlers = %d", gnss.NumOfDgnssStatusHandlers);
    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);
    if(gnss.NumOfDgnssStatusHandlers) {

        DgnssStatusEvent_t* DgnssStatusData = (DgnssStatusEvent_t*)le_mem_ForceAlloc(gnss.DgnssStatusPoolRef);

        DgnssStatusData->clientSessionRefPtr = &clientRequestPtr->sessionRef;
        if(dgnssStatus->status == TAF_PA_LOCATION_DATA_SOURCE_NOT_SUPPORTED)
        {
            DgnssStatusData->dgnssStatus = TAF_LOCGNSS_DGNSS_STATUS_DATA_SOURCE_NOT_SUPPORTED;
        }
        else if(dgnssStatus->status == TAF_PA_LOCATION_DATA_FORMAT_NOT_SUPPORTED)
        {
            DgnssStatusData->dgnssStatus = TAF_LOCGNSS_DGNSS_STATUS_DATA_FORMAT_NOT_SUPPORTED;
        }
        else if(dgnssStatus->status == TAF_PA_LOCATION_OTHER_SOURCE_IN_USE)
        {
            DgnssStatusData->dgnssStatus = TAF_LOCGNSS_DGNSS_STATUS_OTHER_SOURCE_IN_USE;
        }
        else if(dgnssStatus->status == TAF_PA_LOCATION_MESSAGE_PARSE_ERROR)
        {
            DgnssStatusData->dgnssStatus = TAF_LOCGNSS_DGNSS_STATUS_MESSAGE_PARSE_ERROR;
        }
        else if(dgnssStatus->status == TAF_PA_LOCATION_DATA_SOURCE_USABLE)
        {
            DgnssStatusData->dgnssStatus = TAF_LOCGNSS_DGNSS_STATUS_DATA_SOURCE_USABLE;
        }
        else if(dgnssStatus->status == TAF_PA_LOCATION_DATA_SOURCE_NOT_USABLE)
        {
            DgnssStatusData->dgnssStatus = TAF_LOCGNSS_DGNSS_STATUS_DATA_SOURCE_NOT_USABLE;
        }
        else if(dgnssStatus->status == TAF_PA_LOCATION_CDFW_STOP_SOURCE_INJECT)
        {
            DgnssStatusData->dgnssStatus = TAF_LOCGNSS_DGNSS_STATUS_CDFW_STOP_SOURCE_INJECT;
        }

        le_event_ReportWithRefCounting(gnss.dgnssStatusEventId, DgnssStatusData);
    }
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);
}

void taf_locGnss::CopyPositionData
(
    taf_locGnss_PositionSample_t* LastDataPtr,
    taf_locGnss_PositionSample_t* CurrentDataPtr
)
{
    LastDataPtr->clientSessionRefPtr = CurrentDataPtr->clientSessionRefPtr;
    LastDataPtr->fixState = CurrentDataPtr->fixState;
    LastDataPtr->latitudeValid = CurrentDataPtr->latitudeValid;
    LastDataPtr->latitude = CurrentDataPtr->latitude;
    LastDataPtr->longitudeValid = CurrentDataPtr->longitudeValid;
    LastDataPtr->longitude = CurrentDataPtr->longitude;
    LastDataPtr->hAccuracyValid = CurrentDataPtr->hAccuracyValid;
    LastDataPtr->hAccuracy = CurrentDataPtr->hAccuracy;
    LastDataPtr->altitudeValid = CurrentDataPtr->altitudeValid;
    LastDataPtr->altitude = CurrentDataPtr->altitude;
    LastDataPtr->vAccuracyValid = CurrentDataPtr->vAccuracyValid;
    LastDataPtr->vAccuracy = CurrentDataPtr->vAccuracy;
    LastDataPtr->altitudeOnWgs84Valid = CurrentDataPtr->altitudeOnWgs84Valid;
    LastDataPtr->altitudeOnWgs84 = CurrentDataPtr->altitudeOnWgs84;
    LastDataPtr->hSpeedValid = CurrentDataPtr->hSpeedValid;
    LastDataPtr->hSpeed = CurrentDataPtr->hSpeed;
    LastDataPtr->vSpeedValid = CurrentDataPtr->vSpeedValid;
    LastDataPtr->vSpeed = CurrentDataPtr->vSpeed;
    LastDataPtr->horUncEllipseSemiMajorValid = CurrentDataPtr->horUncEllipseSemiMajorValid;
    LastDataPtr->horUncEllipseSemiMajor = CurrentDataPtr->horUncEllipseSemiMajor;
    LastDataPtr->horUncEllipseSemiMinorValid = CurrentDataPtr->horUncEllipseSemiMinorValid;
    LastDataPtr->horUncEllipseSemiMinor = CurrentDataPtr->horUncEllipseSemiMinor;
    LastDataPtr->directionAccuracyValid = CurrentDataPtr->directionAccuracyValid;
    LastDataPtr->directionAccuracy = CurrentDataPtr->directionAccuracy;
    LastDataPtr->positionLatencyValid = CurrentDataPtr->positionLatencyValid;
    LastDataPtr->positionLatency = CurrentDataPtr->positionLatency;
    LastDataPtr->hSpeedAccuracyValid = CurrentDataPtr->hSpeedAccuracyValid;
    LastDataPtr->hSpeedAccuracy = CurrentDataPtr->hSpeedAccuracy;
    LastDataPtr->vSpeedAccuracyValid = CurrentDataPtr->vSpeedAccuracyValid;
    LastDataPtr->vSpeedAccuracy = CurrentDataPtr->vSpeedAccuracy;
    LastDataPtr->horConfidenceValid =CurrentDataPtr->horConfidenceValid;
    LastDataPtr->horConfidence = CurrentDataPtr->horConfidence;
    LastDataPtr->directionValid = CurrentDataPtr->directionValid ;
    LastDataPtr->direction = CurrentDataPtr->direction;
    LastDataPtr->leapSecondsValid = CurrentDataPtr->leapSecondsValid;
    LastDataPtr->timeAccuracyValid = CurrentDataPtr->timeAccuracyValid;
    LastDataPtr->timeAccuracy = CurrentDataPtr->timeAccuracy;
    LastDataPtr->gpsTimeValid = CurrentDataPtr->gpsTimeValid;
    LastDataPtr->gpsWeek = CurrentDataPtr->gpsWeek;
    LastDataPtr->gpsTimeOfWeek = CurrentDataPtr->gpsTimeOfWeek;
    LastDataPtr->timeValid = CurrentDataPtr->timeValid;
    LastDataPtr->dateValid = CurrentDataPtr->dateValid;
    LastDataPtr->year = CurrentDataPtr->year;
    LastDataPtr->month =CurrentDataPtr->month;
    LastDataPtr->day = CurrentDataPtr->day;
    LastDataPtr->hours = CurrentDataPtr->hours;
    LastDataPtr->minutes = CurrentDataPtr->minutes;
    LastDataPtr->seconds = CurrentDataPtr->seconds;
    LastDataPtr->milliseconds = CurrentDataPtr->milliseconds;
    LastDataPtr->leapSeconds = CurrentDataPtr->leapSeconds;
    LastDataPtr->hdopValid = CurrentDataPtr->hdopValid;
    LastDataPtr->hdop = CurrentDataPtr->hdop;
    LastDataPtr->vdopValid = CurrentDataPtr->vdopValid;
    LastDataPtr->vdop = CurrentDataPtr->vdop;
    LastDataPtr->pdopValid = CurrentDataPtr->pdopValid;
    LastDataPtr->pdop = CurrentDataPtr->pdop;
    LastDataPtr->gdopValid = CurrentDataPtr->gdopValid;
    LastDataPtr->gdop = CurrentDataPtr->gdop;
    LastDataPtr->tdopValid = CurrentDataPtr->tdopValid;
    LastDataPtr->tdop = CurrentDataPtr->tdop;
    LastDataPtr->satsTrackingCountValid = CurrentDataPtr->satsTrackingCountValid;
    LastDataPtr->satsUsedCountValid = CurrentDataPtr->satsUsedCountValid;
    LastDataPtr->magneticDeviationValid = CurrentDataPtr->magneticDeviationValid;
    LastDataPtr->satsInViewCountValid = CurrentDataPtr->satsInViewCountValid;
    LastDataPtr->satInfoValid = CurrentDataPtr->satInfoValid;
    LastDataPtr->satMeasValid = CurrentDataPtr->satMeasValid;
    LastDataPtr->magneticDeviation = CurrentDataPtr->magneticDeviation;

    uint64_t previousTime = LastDataPtr->epochTime;

    uint64_t currentTime = CurrentDataPtr->epochTime;
    LE_DEBUG("sessionRef: %p, Previous epochTime: %" PRIu64 ", Current epochTime: %" PRIu64 ","
        "EpochTime Difference: %" PRIu64 , CurrentDataPtr->clientSessionRefPtr,
            previousTime, currentTime, (currentTime - previousTime));

    LastDataPtr->epochTime = CurrentDataPtr->epochTime;
    LastDataPtr->satsInViewCount = CurrentDataPtr->satsInViewCount;
    LastDataPtr->satsTrackingCount = CurrentDataPtr->satsTrackingCount;
    LastDataPtr->satsUsedCount = CurrentDataPtr->satsUsedCount;
    for(auto i=0; i<TAF_LOCGNSS_SV_INFO_MAX_LEN; i++)
    {
        LastDataPtr->satInfo[i].satId = CurrentDataPtr->satInfo[i].satId;
        LastDataPtr->satInfo[i].satConst = CurrentDataPtr->satInfo[i].satConst;
        LastDataPtr->satInfo[i].satUsed = CurrentDataPtr->satInfo[i].satUsed;
        LastDataPtr->satInfo[i].satUsedDgnss = CurrentDataPtr->satInfo[i].satUsedDgnss;
        LastDataPtr->satInfo[i].satTracked = CurrentDataPtr->satInfo[i].satTracked;
        LastDataPtr->satInfo[i].satSnr = CurrentDataPtr->satInfo[i].satSnr;
        LastDataPtr->satInfo[i].satAzim = CurrentDataPtr->satInfo[i].satAzim;
        LastDataPtr->satInfo[i].satElev = CurrentDataPtr->satInfo[i].satElev;
        LastDataPtr->satInfo[i].signalType = CurrentDataPtr->satInfo[i].signalType;
        LastDataPtr->satInfo[i].glonassFcn = CurrentDataPtr->satInfo[i].glonassFcn;
        LastDataPtr->satInfo[i].baseBandCnr = CurrentDataPtr->satInfo[i].baseBandCnr;
    }

    for(auto i=0; i<TAF_LOCGNSS_SV_INFO_MAX_LEN; i++)
    {
        LastDataPtr->satMeas[i].satId = CurrentDataPtr->satMeas[i].satId;
        LastDataPtr->satMeas[i].satLatency = CurrentDataPtr->satMeas[i].satLatency;
    }

    LastDataPtr->robustConformity = CurrentDataPtr->robustConformity;
    LastDataPtr->conformityValid = CurrentDataPtr->conformityValid;
    LastDataPtr->confidencePercent = CurrentDataPtr->confidencePercent;
    LastDataPtr->confidencePercentValid = CurrentDataPtr->confidencePercentValid;
    LastDataPtr->calibrationStatus = CurrentDataPtr->calibrationStatus;
    LastDataPtr->calibrationStatusValid = CurrentDataPtr->calibrationStatusValid;

    LastDataPtr->GnssKinematicsDataValid = CurrentDataPtr->GnssKinematicsDataValid;
    LastDataPtr->GnssKinematicsData.bodyFrameDataMask =
                                    CurrentDataPtr->GnssKinematicsData.bodyFrameDataMask;
    LastDataPtr->GnssKinematicsData.longAccel = CurrentDataPtr->GnssKinematicsData.longAccel;
    LastDataPtr->GnssKinematicsData.latAccel = CurrentDataPtr->GnssKinematicsData.latAccel;
    LastDataPtr->GnssKinematicsData.vertAccel = CurrentDataPtr->GnssKinematicsData.vertAccel;
    LastDataPtr->GnssKinematicsData.yawRate = CurrentDataPtr->GnssKinematicsData.yawRate;
    LastDataPtr->GnssKinematicsData.pitch = CurrentDataPtr->GnssKinematicsData.pitch;
    LastDataPtr->GnssKinematicsData.longAccelUnc = CurrentDataPtr->GnssKinematicsData.longAccelUnc;
    LastDataPtr->GnssKinematicsData.latAccelUnc = CurrentDataPtr->GnssKinematicsData.latAccelUnc;
    LastDataPtr->GnssKinematicsData.vertAccelUnc = CurrentDataPtr->GnssKinematicsData.vertAccelUnc;
    LastDataPtr->GnssKinematicsData.yawRateUnc = CurrentDataPtr->GnssKinematicsData.yawRateUnc;
    LastDataPtr->GnssKinematicsData.pitchUnc = CurrentDataPtr->GnssKinematicsData.pitchUnc;
    LastDataPtr->GnssKinematicsData.pitchRate = CurrentDataPtr->GnssKinematicsData.pitchRate;
    LastDataPtr->GnssKinematicsData.pitchRateUnc = CurrentDataPtr->GnssKinematicsData.pitchRateUnc;
    LastDataPtr->GnssKinematicsData.roll = CurrentDataPtr->GnssKinematicsData.roll;
    LastDataPtr->GnssKinematicsData.rollUnc = CurrentDataPtr->GnssKinematicsData.rollUnc;
    LastDataPtr->GnssKinematicsData.rollRate = CurrentDataPtr->GnssKinematicsData.rollRate;
    LastDataPtr->GnssKinematicsData.rollRateUnc = CurrentDataPtr->GnssKinematicsData.rollRateUnc;
    LastDataPtr->GnssKinematicsData.yaw = CurrentDataPtr->GnssKinematicsData.yaw;
    LastDataPtr->GnssKinematicsData.yawUnc = CurrentDataPtr->GnssKinematicsData.yawUnc;

    LastDataPtr->vrpLatitudeValid = CurrentDataPtr->vrpLatitudeValid;
    LastDataPtr->vrpLatitude = CurrentDataPtr->vrpLatitude;
    LastDataPtr->vrpLongitudeValid = CurrentDataPtr->vrpLongitudeValid;
    LastDataPtr->vrpLongitude = CurrentDataPtr->vrpLongitude;
    LastDataPtr->vrpAltitudeValid = CurrentDataPtr->vrpAltitudeValid;
    LastDataPtr->vrpAltitude = CurrentDataPtr->vrpAltitude;
    LastDataPtr->eastVelValid = CurrentDataPtr->eastVelValid;
    LastDataPtr->eastVel = CurrentDataPtr->eastVel;
    LastDataPtr->northVelValid = CurrentDataPtr->northVelValid;
    LastDataPtr->northVel = CurrentDataPtr->northVel;
    LastDataPtr->upVelValid = CurrentDataPtr->upVelValid;
    LastDataPtr->upVel = CurrentDataPtr->upVel;
    LastDataPtr->svDataValid = CurrentDataPtr->svDataValid;
    LastDataPtr->svData.gps = CurrentDataPtr->svData.gps;
    LastDataPtr->svData.glo = CurrentDataPtr->svData.glo;
    LastDataPtr->svData.gal = CurrentDataPtr->svData.gal;
    LastDataPtr->svData.bds = CurrentDataPtr->svData.bds;
    LastDataPtr->svData.qzss = CurrentDataPtr->svData.qzss;
    LastDataPtr->svData.navic = CurrentDataPtr->svData.navic;
    LastDataPtr->sbasMask = CurrentDataPtr->sbasMask;
    LastDataPtr->sbasMaskValid = CurrentDataPtr->sbasMaskValid;
    LastDataPtr->validityMask = CurrentDataPtr->validityMask;
    LastDataPtr->validityMaskValid = CurrentDataPtr->validityMaskValid;
    LastDataPtr->validityExMask = CurrentDataPtr->validityExMask;
    LastDataPtr->validityExMaskValid = CurrentDataPtr->validityExMaskValid;
    LastDataPtr->engMask = CurrentDataPtr->engMask;
    LastDataPtr->engMaskValid = CurrentDataPtr->engMaskValid;
    LastDataPtr->locationEngType = CurrentDataPtr->locationEngType;
    LastDataPtr->locationEngTypeValid = CurrentDataPtr->locationEngTypeValid;
    LastDataPtr->horiReliablity = CurrentDataPtr->horiReliablity;
    LastDataPtr->horiReliablityValid = CurrentDataPtr->horiReliablityValid;
    LastDataPtr->vertReliablity = CurrentDataPtr->vertReliablity;
    LastDataPtr->vertReliablityValid = CurrentDataPtr->vertReliablityValid;
    LastDataPtr->azimuth = CurrentDataPtr->azimuth;
    LastDataPtr->azimuthValid = CurrentDataPtr->azimuthValid;
    LastDataPtr->eastDev = CurrentDataPtr->eastDev;
    LastDataPtr->eastDevValid = CurrentDataPtr->eastDevValid;
    LastDataPtr->northDev = CurrentDataPtr->northDev;
    LastDataPtr->northDevValid = CurrentDataPtr->northDevValid;
    LastDataPtr->realTime = CurrentDataPtr->realTime;
    LastDataPtr->realTimeValid = CurrentDataPtr->realTimeValid;
    LastDataPtr->realTimeUnc = CurrentDataPtr->realTimeUnc;
    LastDataPtr->realTimeUncValid = CurrentDataPtr->realTimeUncValid;
    LastDataPtr->techMask = CurrentDataPtr->techMask;
    LastDataPtr->techMaskValid = CurrentDataPtr->techMaskValid;
    for(auto i=0; i<TAF_LOCGNSS_MEASUREMENT_INFO_MAX; i++)
    {
        LastDataPtr->measInfo[i].gnssSignalType = CurrentDataPtr->measInfo[i].gnssSignalType;
        LastDataPtr->measInfo[i].gnssConstellation = CurrentDataPtr->measInfo[i].gnssConstellation;
        LastDataPtr->measInfo[i].gnssSvId = CurrentDataPtr->measInfo[i].gnssSvId;
    }
    LastDataPtr->measInfoCount = CurrentDataPtr->measInfoCount;
    LastDataPtr->reportStatus = CurrentDataPtr->reportStatus;
    LastDataPtr->altMeanSeaLevel = CurrentDataPtr->altMeanSeaLevel;
    for (auto i = 0; i < TAF_LOCGNSS_MEASUREMENT_INFO_MAX; i++) {
        LastDataPtr->SVIds[i] = CurrentDataPtr->SVIds[i];
    }
    LastDataPtr->SVIdsCount = CurrentDataPtr->SVIdsCount;
    for(auto i=0; i<TAF_LOCGNSS_NUMBER_OF_SIGNAL_TYPES_MAX; i++)
    {
        LastDataPtr->gnssData[i].gnssDataMask = CurrentDataPtr->gnssData[i].gnssDataMask;
        LastDataPtr->gnssData[i].jammerInd = CurrentDataPtr->gnssData[i].jammerInd;
        LastDataPtr->gnssData[i].agc = CurrentDataPtr->gnssData[i].agc;
    }
    LastDataPtr->gnssDataValid = CurrentDataPtr->gnssDataValid;
    LastDataPtr->gPtpTimeValid = CurrentDataPtr->gPtpTimeValid;
    LastDataPtr->gPtpTime = CurrentDataPtr->gPtpTime;
    LastDataPtr->gPtpTimeUncValid = CurrentDataPtr->gPtpTimeUncValid;
    LastDataPtr->gPtpTimeUnc = CurrentDataPtr->gPtpTimeUnc;
    LastDataPtr->drSolutionStatus = CurrentDataPtr->drSolutionStatus;
    LastDataPtr->drSolutionStatusValid = CurrentDataPtr->drSolutionStatusValid;
    LastDataPtr->leapSecondsUncValid = CurrentDataPtr->leapSecondsUncValid;
    LastDataPtr->leapSecondsUnc = CurrentDataPtr->leapSecondsUnc;
    LastDataPtr->navSolutionMaskValid = CurrentDataPtr->navSolutionMaskValid;
    LastDataPtr->navSolutionMask = CurrentDataPtr->navSolutionMask;
    LastDataPtr->dgnssStationIdsCount = CurrentDataPtr->dgnssStationIdsCount;

    for (auto i = 0; i < TAF_LOCGNSS_MAX_MONITOR_STATION_IDS; i++)
    {
        LastDataPtr->dgnssStationIds[i] = CurrentDataPtr->dgnssStationIds[i];
    }
    LastDataPtr->protectionLevelAlongTrackValid = CurrentDataPtr->protectionLevelAlongTrackValid;
    LastDataPtr->protectionLevelCrossTrackValid = CurrentDataPtr->protectionLevelCrossTrackValid;
    LastDataPtr->protectionLevelVerticalValid = CurrentDataPtr->protectionLevelVerticalValid;
    LastDataPtr->baselineLengthValid = CurrentDataPtr->baselineLengthValid;
    LastDataPtr->ageCorrectionsValid = CurrentDataPtr->ageCorrectionsValid;
    LastDataPtr->integrityRiskUsedValid = CurrentDataPtr->integrityRiskUsedValid;
    LastDataPtr->protectionLevelAlongTrack = CurrentDataPtr->protectionLevelAlongTrack;
    LastDataPtr->protectionLevelCrossTrack = CurrentDataPtr->protectionLevelCrossTrack;
    LastDataPtr->protectionLevelVertical = CurrentDataPtr->protectionLevelVertical;
    LastDataPtr->baselineLength = CurrentDataPtr->baselineLength;
    LastDataPtr->ageCorrections = CurrentDataPtr->ageCorrections;
    LastDataPtr->integrityRiskUsed = CurrentDataPtr->integrityRiskUsed;
    LastDataPtr->next = LE_DLS_LINK_INIT;

    return;
}

void taf_locGnss::GnssPositionHandler
(
 void* reportPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_PositionHandler_t*  posHandlerPtr;
    taf_locGnss_PositionSampleRequest_t*    posSampleReqPtr=NULL;
    taf_locGnss_PositionSample_t* currentPosPtr = (taf_locGnss_PositionSample_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL( currentPosPtr == NULL, "currentPosPtr is Null");

    LE_DEBUG("Handler Function called with position %p", currentPosPtr);

    taf_locGnss_Client_t* clientRequestPtr = NULL;

    clientRequestPtr = gnss.DiscoverSessionRef(*currentPosPtr->clientSessionRefPtr);

    TAF_ERROR_IF_RET_NIL(NULL == clientRequestPtr, "GnssPositionHandler did not find sessionRef");

    CopyPositionData(&clientRequestPtr->LastPositionSample, currentPosPtr);

    if(!gnss.NumOfPositionHandlers)
    {
        LE_DEBUG("No positioning handlers, exit Handler Function");
        le_mem_Release(currentPosPtr);
        return;
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(gnss.PositionHandlerRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        posHandlerPtr = (taf_locGnss_PositionHandler_t*)le_ref_GetValue(iterRef);
        if(posHandlerPtr == NULL) {
            continue;
        }

        posSampleReqPtr = (taf_locGnss_PositionSampleRequest_t*)le_mem_ForceAlloc(gnss.PositionSampleRequestPoolRef);
        memset(posSampleReqPtr, 0, sizeof(taf_locGnss_PositionSampleRequest_t));

        posSampleReqPtr->positionSampleNodePtr =
            (taf_locGnss_PositionSample_t*)le_mem_ForceAlloc(gnss.PositionSamplePoolRef);
        memset(posSampleReqPtr->positionSampleNodePtr, 0, sizeof(taf_locGnss_PositionSample_t));

        memcpy(posSampleReqPtr->positionSampleNodePtr, &clientRequestPtr->LastPositionSample,
                sizeof(taf_locGnss_PositionSample_t));

        posSampleReqPtr->sessionRef = posHandlerPtr->sessionRef;

        if (posSampleReqPtr->sessionRef != *currentPosPtr->clientSessionRefPtr) {
            LE_DEBUG("GnssPositionHandler session ref does not match! ReqPtr.sessionRef: %p, Sample.sessionRef: %p", posSampleReqPtr->sessionRef, *currentPosPtr->clientSessionRefPtr);
            continue;
        }
        posSampleReqPtr->positionSampleRef =
           (taf_locGnss_SampleRef_t)le_ref_CreateRef(gnss.PositionSampleMap, posSampleReqPtr);

        LE_INFO("Report sampleRef %p to the corresponding handler (handlerPtr %p)",
            posSampleReqPtr->positionSampleRef, posHandlerPtr->handlerFuncPtr);

        posHandlerPtr->handlerFuncPtr(posSampleReqPtr->positionSampleRef,
                                      posHandlerPtr->handlerContextPtr);
    }

    le_mem_Release(currentPosPtr);
}

void taf_locGnss::GnssPositionExHandler
(
 void* reportPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_PositionExHandler_t*  extendPosHandlerPtr;
    taf_locGnss_PositionSampleEx_t*    extendPosSampleReqPtr=NULL;
    taf_locGnss_PositionExSample_t* currentPosPtr = (taf_locGnss_PositionExSample_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL( currentPosPtr == NULL, "currentPosPtr is Null");

    LE_INFO("GnssPositionExHandler Function called with position %p", currentPosPtr);

    taf_locGnss_Client_t* clientRequestPtr = NULL;

    clientRequestPtr = gnss.DiscoverSessionRef(*currentPosPtr->clientSessionRefPtr);

    TAF_ERROR_IF_RET_NIL(NULL == clientRequestPtr, "GnssPositionExHandler did not find sessionRef");

    if(!gnss.NumOfPositionExHandlers)
    {
        LE_INFO("No positioning handlers, exit Handler Function");
        le_mem_Release(currentPosPtr);
        return;
    }

    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);
    le_ref_IterRef_t iterRef = le_ref_GetIterator(gnss.PositionExHandlerRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        extendPosHandlerPtr = (taf_locGnss_PositionExHandler_t*)le_ref_GetValue(iterRef);
        if(extendPosHandlerPtr == NULL) {
            le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);
            return;
        }

        extendPosSampleReqPtr = (taf_locGnss_PositionSampleEx_t*)le_mem_ForceAlloc(gnss.PositionSampleExPoolRef);

        memset(extendPosSampleReqPtr, 0, sizeof(taf_locGnss_PositionSampleEx_t));

        taf_locGnss_SampleExRef_t postitionSampleExRef =
           (taf_locGnss_SampleExRef_t)le_ref_CreateRef(gnss.PositionExSampleMap, extendPosSampleReqPtr);

        extendPosSampleReqPtr->longitude = currentPosPtr->longitude;
        extendPosSampleReqPtr->latitude = currentPosPtr->latitude;
        extendPosSampleReqPtr->hAccuracy = currentPosPtr->hAccuracy;
        extendPosSampleReqPtr->altitude = currentPosPtr->altitude;
        extendPosSampleReqPtr->direction = currentPosPtr->direction;
        extendPosSampleReqPtr->directionAccuracy = currentPosPtr->directionAccuracy;
        extendPosSampleReqPtr->validityMask = currentPosPtr->validityMask;
        extendPosSampleReqPtr->techMask = currentPosPtr->techMask;
        extendPosSampleReqPtr->hSpeed = currentPosPtr->hSpeed;
        extendPosSampleReqPtr->vAccuracy = currentPosPtr->vAccuracy;
        extendPosSampleReqPtr->epochTime = currentPosPtr->epochTime;
        extendPosSampleReqPtr->hSpeedAccuracy = currentPosPtr->hSpeedAccuracy;
        extendPosSampleReqPtr->realTime = currentPosPtr->realTime;
        extendPosSampleReqPtr->realTimeUnc = currentPosPtr->realTimeUnc;

        extendPosSampleReqPtr->satsInViewCount = currentPosPtr->satsInViewCount;
        extendPosSampleReqPtr->satsTrackingCount = currentPosPtr->satsTrackingCount;
        extendPosSampleReqPtr->satsUsedCount = currentPosPtr->satsUsedCount;

        extendPosSampleReqPtr->hdop = currentPosPtr->hdop;
        extendPosSampleReqPtr->vdop = currentPosPtr->vdop;
        extendPosSampleReqPtr->pdop = currentPosPtr->pdop;

        extendPosSampleReqPtr->altMeanSeaLevel = currentPosPtr->altMeanSeaLevel;
        extendPosSampleReqPtr->magneticDeviation = currentPosPtr->magneticDeviation;

        if (extendPosHandlerPtr->sessionRef != *currentPosPtr->clientSessionRefPtr) {
            LE_ERROR("GnssPositionExHandler session ref does not match! ReqPtr.sessionRef: %p, Sample.sessionRef: %p", extendPosHandlerPtr->sessionRef, *currentPosPtr->clientSessionRefPtr);
            continue;
        }

        LE_INFO("epochTime: %" PRIu64 "", extendPosSampleReqPtr->epochTime);

        struct timeval tv;

        struct tm* tm_info;

        gettimeofday(&tv, NULL);

        tm_info = localtime(&tv.tv_sec);

        LE_INFO("Time: %02d:%02d:%02d.%03ld\n",

            tm_info->tm_hour,

            tm_info->tm_min,

            tm_info->tm_sec,

            tv.tv_usec / 1000);  // convert microseconds to milliseconds

        LE_DEBUG("Report SampleRef %p to the corresponding extend handler (handlerPtr %p)",
                postitionSampleExRef, extendPosHandlerPtr->handlerFuncPtr);

        extendPosHandlerPtr->handlerFuncPtr(postitionSampleExRef, extendPosSampleReqPtr,
                                      extendPosHandlerPtr->handlerContextPtr);
    }
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);

    le_mem_Release(currentPosPtr);
}

void taf_locGnss::GnssMeasurementHandler
(
 void* reportPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_MeasurementHandler_t*  measHandlerPtr;
    taf_locGnss_MeasurementSampleRequest_t*    measSampleReqPtr=NULL;
    taf_locGnss_GnssMeasurements_t* currentPosPtr = (taf_locGnss_GnssMeasurements_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL( currentPosPtr == NULL, "currentPosPtr is Null");

    LE_INFO("Handler Function called with position %p", currentPosPtr);

    taf_locGnss_Client_t* clientRequestPtr = NULL;

    LE_INFO("*currentPosPtr->clientSessionRefPtr: %p", *currentPosPtr->clientSessionRefPtr);

    clientRequestPtr = gnss.DiscoverSessionRef(*currentPosPtr->clientSessionRefPtr);

    TAF_ERROR_IF_RET_NIL(NULL == clientRequestPtr, "GnssPositionHandler did not find sessionRef");

    if(!gnss.NumOfMeasurementHandlers)
    {
        LE_ERROR("No positioning handlers, exit Handler Function");
        le_mem_Release(currentPosPtr);
        return;
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(gnss.MeasurementHandlerRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        measHandlerPtr = (taf_locGnss_MeasurementHandler_t*)le_ref_GetValue(iterRef);
        if(measHandlerPtr == NULL) {
            LE_ERROR("Pointer is NULL");
            return; 
        }
        measSampleReqPtr = (taf_locGnss_MeasurementSampleRequest_t*)le_mem_ForceAlloc(gnss.MeasurementSampleRequestPoolRef);
        memset(measSampleReqPtr, 0, sizeof(taf_locGnss_MeasurementSampleRequest_t));

        measSampleReqPtr->measSampleNodePtr = (taf_locGnss_GnssMeasurements_t*)le_mem_ForceAlloc(gnss.MeasurementSamplePoolRef);

        memset(measSampleReqPtr->measSampleNodePtr, 0, sizeof(taf_locGnss_GnssMeasurements_t));

        measSampleReqPtr->measSampleNodePtr->clock.valid = currentPosPtr->clock.valid;
        measSampleReqPtr->measSampleNodePtr->clock.leapSecond = currentPosPtr->clock.leapSecond;
        measSampleReqPtr->measSampleNodePtr->clock.timeNs = currentPosPtr->clock.timeNs;
        measSampleReqPtr->measSampleNodePtr->clock.timeUncertaintyNs = currentPosPtr->clock.timeUncertaintyNs;
        measSampleReqPtr->measSampleNodePtr->clock.fullBiasNs = currentPosPtr->clock.fullBiasNs;
        measSampleReqPtr->measSampleNodePtr->clock.biasNs = currentPosPtr->clock.biasNs;
        measSampleReqPtr->measSampleNodePtr->clock.biasUncertaintyNs = currentPosPtr->clock.biasUncertaintyNs;
        measSampleReqPtr->measSampleNodePtr->clock.driftNsps = currentPosPtr->clock.driftNsps;
        measSampleReqPtr->measSampleNodePtr->clock.driftUncertaintyNsps = currentPosPtr->clock.driftUncertaintyNsps;
        measSampleReqPtr->measSampleNodePtr->clock.hwClockDiscontinuityCount = currentPosPtr->clock.hwClockDiscontinuityCount;
        measSampleReqPtr->measSampleNodePtr->clock.elapsedRealTime = currentPosPtr->clock.elapsedRealTime;
        measSampleReqPtr->measSampleNodePtr->clock.elapsedRealTimeUnc = currentPosPtr->clock.elapsedRealTimeUnc;
        measSampleReqPtr->measSampleNodePtr->clock.elapsedgPTPTime = currentPosPtr->clock.elapsedgPTPTime;
        measSampleReqPtr->measSampleNodePtr->clock.elapsedgPTPTimeUnc = currentPosPtr->clock.elapsedgPTPTimeUnc;

        LE_INFO("measCount: %d",(int)currentPosPtr->measCount);
        measSampleReqPtr->measSampleNodePtr->measCount = currentPosPtr->measCount;


        for(auto i=0; i< (int)currentPosPtr->measCount; i++)
        {
            measSampleReqPtr->measSampleNodePtr->measData[i].svId = currentPosPtr->measData[i].svId;
            measSampleReqPtr->measSampleNodePtr->measData[i].timeOffsetNs = currentPosPtr->measData[i].timeOffsetNs;
            measSampleReqPtr->measSampleNodePtr->measData[i].stateMask = currentPosPtr->measData[i].stateMask;
            measSampleReqPtr->measSampleNodePtr->measData[i].valid = currentPosPtr->measData[i].valid;
            measSampleReqPtr->measSampleNodePtr->measData[i].svType = currentPosPtr->measData[i].svType;
            measSampleReqPtr->measSampleNodePtr->measData[i].receivedSvTimeNs = currentPosPtr->measData[i].receivedSvTimeNs;
            measSampleReqPtr->measSampleNodePtr->measData[i].receivedSvTimeSubNs = currentPosPtr->measData[i].receivedSvTimeSubNs;
            measSampleReqPtr->measSampleNodePtr->measData[i].receivedSvTimeUncertaintyNs = currentPosPtr->measData[i].receivedSvTimeUncertaintyNs;
            measSampleReqPtr->measSampleNodePtr->measData[i].carrierToNoiseDbHz = currentPosPtr->measData[i].carrierToNoiseDbHz;
            measSampleReqPtr->measSampleNodePtr->measData[i].pseudorangeRateMps = currentPosPtr->measData[i].pseudorangeRateMps;
            measSampleReqPtr->measSampleNodePtr->measData[i].pseudorangeRateUncertaintyMps = currentPosPtr->measData[i].pseudorangeRateUncertaintyMps;
            measSampleReqPtr->measSampleNodePtr->measData[i].adrStateMask = currentPosPtr->measData[i].adrStateMask;
            measSampleReqPtr->measSampleNodePtr->measData[i].adrMeters = currentPosPtr->measData[i].adrMeters;
            measSampleReqPtr->measSampleNodePtr->measData[i].adrUncertaintyMeters = currentPosPtr->measData[i].adrUncertaintyMeters;
            measSampleReqPtr->measSampleNodePtr->measData[i].carrierFrequencyHz = currentPosPtr->measData[i].carrierFrequencyHz;
            measSampleReqPtr->measSampleNodePtr->measData[i].carrierCycles = currentPosPtr->measData[i].carrierCycles;
            measSampleReqPtr->measSampleNodePtr->measData[i].carrierPhase = currentPosPtr->measData[i].carrierPhase;
            measSampleReqPtr->measSampleNodePtr->measData[i].carrierPhaseUncertainty = currentPosPtr->measData[i].carrierPhaseUncertainty;
            measSampleReqPtr->measSampleNodePtr->measData[i].multipathIndicator = currentPosPtr->measData[i].multipathIndicator;
            measSampleReqPtr->measSampleNodePtr->measData[i].signalToNoiseRatioDb = currentPosPtr->measData[i].signalToNoiseRatioDb;
            measSampleReqPtr->measSampleNodePtr->measData[i].agcLevelDb = currentPosPtr->measData[i].agcLevelDb;
            measSampleReqPtr->measSampleNodePtr->measData[i].gnssSignalType = currentPosPtr->measData[i].gnssSignalType;
            measSampleReqPtr->measSampleNodePtr->measData[i].basebandCarrierToNoise = currentPosPtr->measData[i].basebandCarrierToNoise;
            measSampleReqPtr->measSampleNodePtr->measData[i].fullInterSignalBias = currentPosPtr->measData[i].fullInterSignalBias;
            measSampleReqPtr->measSampleNodePtr->measData[i].fullInterSignalBiasUncertainty = currentPosPtr->measData[i].fullInterSignalBiasUncertainty;
        }

        measSampleReqPtr->measSampleNodePtr->isNHz = currentPosPtr->isNHz;
        measSampleReqPtr->measSampleNodePtr->agcStatusL1 = currentPosPtr->agcStatusL1;
        measSampleReqPtr->measSampleNodePtr->agcStatusL2 = currentPosPtr->agcStatusL2;
        measSampleReqPtr->measSampleNodePtr->agcStatusL5 = currentPosPtr->agcStatusL5;

        measSampleReqPtr->sessionRef = measHandlerPtr->sessionRef;

        if (measSampleReqPtr->sessionRef != *currentPosPtr->clientSessionRefPtr) {
            LE_DEBUG("GnssPositionHandler session ref does not match! ReqPtr.sessionRef: %p, Sample.sessionRef: %p", measSampleReqPtr->sessionRef, *currentPosPtr->clientSessionRefPtr);
            continue;
        }
        measSampleReqPtr->measSampleRef =
           (taf_locGnss_MeasSampleRef_t)le_ref_CreateRef(gnss.MeasurementSampleMap, measSampleReqPtr);

        LE_INFO("Report sampleRef %p to the corresponding handler (handlerPtr %p)",
            measSampleReqPtr->measSampleRef, measHandlerPtr->handlerFuncPtr);

        measHandlerPtr->handlerFuncPtr(measSampleReqPtr->measSampleRef,
                                      measHandlerPtr->handlerContextPtr);
    }

    le_mem_Release(currentPosPtr);
}

void taf_locGnss::GnssNmeaHandler
(
 void* reportPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_NmeaHandler_t*  nmeaHandlerPtr;
    NmeaInfoEvent_t* nmeaEventPtr = (NmeaInfoEvent_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL( nmeaEventPtr == NULL, "nmeaEventPtr is Null");

    LE_INFO("Handler Function called with position %p", nmeaEventPtr);

    if(!gnss.NumOfNmeaHandlers)
    {
        LE_DEBUG("No Nmea handlers, exit Handler Function");
        le_mem_Release(nmeaEventPtr);
        return;
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(gnss.NmeaHandlerRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        nmeaHandlerPtr = (taf_locGnss_NmeaHandler_t*)le_ref_GetValue(iterRef);
        if(nmeaHandlerPtr == NULL) {
            continue;
        }

        if (nmeaHandlerPtr->sessionRef == *nmeaEventPtr->clientSessionRefPtr) {
            nmeaHandlerPtr->handlerFuncPtr(nmeaEventPtr->timestamp, nmeaEventPtr->nmeaMask,
                    nmeaHandlerPtr->handlerContextPtr);
        }
        else
        {
            LE_DEBUG("GnssNmeaHandler session ref does not match! ReqPtr.sessionRef: %p, Sample.sessionRef: %p", nmeaHandlerPtr->sessionRef, *nmeaEventPtr->clientSessionRefPtr);
        }
    }

    le_mem_Release(nmeaEventPtr);
}

void taf_locGnss::GnssCapabilityHandler
(
 void* reportPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_CapHandler_t*  capHandlerPtr;
    CapabilityChangeEvent_t* capChangePtr = (CapabilityChangeEvent_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL( capChangePtr == NULL, "capChangePtr is Null");

    LE_INFO("Handler Function called with position %p", capChangePtr);

    if(!gnss.NumOfCapabilityHandlers)
    {
        LE_DEBUG("No capability change handlers, exit Handler Function");
        le_mem_Release(capChangePtr);
        return;
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(gnss.CapHandlerRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        capHandlerPtr = (taf_locGnss_CapHandler_t*)le_ref_GetValue(iterRef);
        if(capHandlerPtr == NULL) {
            continue;
        }

        if (capHandlerPtr->sessionRef == *capChangePtr->clientSessionRefPtr) {
            capHandlerPtr->handlerFuncPtr(capChangePtr->locCapability,
                                      capHandlerPtr->handlerContextPtr);
        }
        else
        {
            LE_DEBUG("GnssCapabilityHandler session ref does not match! ReqPtr.sessionRef: %p, Sample.sessionRef: %p", capHandlerPtr->sessionRef, *capChangePtr->clientSessionRefPtr);
        }
    }

    le_mem_Release(capChangePtr);
}

le_result_t taf_locGnss::PositionDataCoversion
(
 int32_t value,
 int8_t dataType,
 int32_t* dataPtr
)
{
    le_msg_SessionRef_t sessionRef = taf_locGnss_GetClientSessionRef();
    taf_locGnss_Resolution_t resolution = TAF_LOCGNSS_RES_UNKNOWN;

    TAF_ERROR_IF_RET_VAL( dataPtr == NULL, LE_FAULT, "dataPtr is Null");

    taf_locGnss_Client_t* clientReqPtr = DiscoverSessionRef(sessionRef);
    le_result_t result = LE_FAULT;

    if (NULL != clientReqPtr)
    {
        switch(dataType)
        {
            case TAF_LOCGNSS_DATA_VACCURACY:
                resolution = clientReqPtr->vAccuracyResolution;
                break;
            case TAF_LOCGNSS_DATA_VSPEEDACCURACY:
                resolution = clientReqPtr->vSpeedAccuracyResolution;
                break;
            case TAF_LOCGNSS_DATA_HSPEEDACCURACY:
                resolution = clientReqPtr->hSpeedAccuracyResolution;
                break;
            default:
                LE_ERROR("Unsupported data type.");
                return result;
        }
    }
    else
    {
        switch(dataType)
        {
            case TAF_LOCGNSS_DATA_VSPEEDACCURACY:
                resolution = TAF_LOCGNSS_RES_ONE_DECIMAL;
                break;
            case TAF_LOCGNSS_DATA_VACCURACY:
                resolution = TAF_LOCGNSS_RES_THREE_DECIMAL;
                break;
            case TAF_LOCGNSS_DATA_HSPEEDACCURACY:
                resolution = TAF_LOCGNSS_RES_ONE_DECIMAL;
                break;
            default:
                LE_ERROR("Unsupported data type.");
                return result;
        }
    }

    switch(resolution)
    {
        case TAF_LOCGNSS_RES_THREE_DECIMAL:
             *dataPtr = value;
             break;
        case TAF_LOCGNSS_RES_TWO_DECIMAL:
             *dataPtr = value / 10;
             break;
        case TAF_LOCGNSS_RES_ONE_DECIMAL:
             *dataPtr = value / 100;
             break;
        case TAF_LOCGNSS_RES_ZERO_DECIMAL:
             *dataPtr = value / 1000;
             break;
        default:
             LE_ERROR("Unsupported resolution.");
             return result;
    }
    LE_DEBUG("resolution %d, value %" PRIi32 ", new value %" PRIi32, (int)resolution, value, *dataPtr);
    return LE_OK;
}

taf_locGnss_Client_t* taf_locGnss::DiscoverSessionRef
(
    le_msg_SessionRef_t sessionRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    std::unique_lock<std::mutex> lock(gnss.mtx);
    le_ref_IterRef_t iterRef = le_ref_GetIterator(gnss.ClientRequestRefMap);
    le_result_t result = le_ref_NextNode(iterRef);

    while (LE_OK == result)
    {
        taf_locGnss_Client_t* gnssPtr = (taf_locGnss_Client_t*) le_ref_GetValue(iterRef);
        if(gnssPtr == NULL) {
            return NULL;
        }

        if (sessionRef == gnssPtr->sessionRef)
        {
             return gnssPtr;
        }
        result = le_ref_NextNode(iterRef);
    }
    return NULL;
}

le_result_t taf_locGnss::CheckValidatePosition
(
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
)
{
    TAF_KILL_CLIENT_IF_RET_VAL( (posSampleReqPtr == NULL) || (NULL == posSampleReqPtr->positionSampleNodePtr), LE_FAULT, "posSampleReqPtr is Null");
    return LE_OK;
}

void taf_locGnss::InitializeClient
(
    taf_locGnss_Client_t* clientRequestPtr
)
{
    memset(&clientRequestPtr->LastPositionSample, 0, sizeof(clientRequestPtr->LastPositionSample));
    clientRequestPtr->LastPositionSample.fixState = TAF_LOCGNSS_STATE_FIX_NO_POS;
    memset(&clientRequestPtr->mSatParams, 0, sizeof(clientRequestPtr->mSatParams));
    clientRequestPtr->dopResolution = TAF_LOCGNSS_RES_THREE_DECIMAL;
    clientRequestPtr->vAccuracyResolution = TAF_LOCGNSS_RES_THREE_DECIMAL;
    clientRequestPtr->vSpeedAccuracyResolution = TAF_LOCGNSS_RES_ONE_DECIMAL;
    clientRequestPtr->hSpeedAccuracyResolution = TAF_LOCGNSS_RES_ONE_DECIMAL;
    clientRequestPtr->GnssState = TAF_LOCGNSS_STATE_READY;
    clientRequestPtr->mStarted = false;
    clientRequestPtr->mFirstFix = false;
    clientRequestPtr->mEngineType = 0; //By default set to FUSED mode
    clientRequestPtr->mTtffPtr = 0;
    clientRequestPtr->mAcqRate = 0;
    clientRequestPtr->mGpsTime = 0;
    clientRequestPtr->mCurrentLeapSeconds = 0;
    clientRequestPtr->mChangeEventTime = 0;
    clientRequestPtr->mLeapSeconds = 0;
    clientRequestPtr->mNextLeapSeconds = 0;
    clientRequestPtr->drParamsMask = 0x1f;//by default enable all masks
    clientRequestPtr->activeSourceRef = NULL;
    clientRequestPtr->activeDgnssFormat = TAF_LOCGNSS_DGNSS_FORMAT_UNKNOWN;
    memset(&clientRequestPtr->mSatInfo, 0, sizeof(clientRequestPtr->mSatInfo));
    memset(&clientRequestPtr->mGnssData,0, sizeof(clientRequestPtr->mGnssData));
    clientRequestPtr->mGnssMutexRef = le_mutex_CreateRecursive("GnssMutexCl");
    LE_DEBUG("LocationManagerInit for sessionRef: %p",  clientRequestPtr->sessionRef);

    clientRequestPtr->eventListener.onGnssSVInfo = &Handler::onGnssSVInfo;
    clientRequestPtr->eventListener.onGnssSignalInfo = &Handler::onGnssSignalInfo;
    clientRequestPtr->eventListener.onGnssNmeaInfo = &Handler::onGnssNmeaInfo;
    clientRequestPtr->eventListener.onDetailedEngineLocationUpdate = &Handler::onDetailedEngineLocationUpdate;
    clientRequestPtr->eventListener.onGnssMeasurementsInfo = &Handler::onGnssMeasurementsInfo;
    clientRequestPtr->eventListener.onLocationSystemInfo = &Handler::onLocationSystemInfo;
    clientRequestPtr->eventListener.onCapabilitiesInfo = &Handler::onCapabilitiesInfo;
    clientRequestPtr->eventListener.onXtraStatusUpdate = &Handler::onXtraStatusUpdate;

    if(taf_pa_location_RegisterListener(clientRequestPtr->locationClient, &clientRequestPtr->eventListener,std::any(clientRequestPtr->sessionRef)) !=  PA_OK){
        LE_ERROR("Listener register failed for %p", clientRequestPtr->sessionRef);
        return;
    }

    return;

}

taf_locGnss_Client_t* taf_locGnss::AcquireSessionRef
(
    void
)
{
    auto &gnss = taf_locGnss::GetInstance();

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    le_msg_SessionRef_t sessionRef = taf_locGnss_GetClientSessionRef();

    clientRequestPtr = DiscoverSessionRef(sessionRef);

    if (NULL == clientRequestPtr)
    {
        //when client count already reached 12
        if(gnss.mClientRefCount >= (TAF_CONFIG_POSITIONING_ACTIVATION_MAX-1))
        {
            LE_DEBUG("Reached maximum clients count: %d",gnss.mClientRefCount);
            return NULL;
        }
        clientRequestPtr = (taf_locGnss_Client_t*)le_mem_ForceAlloc(gnss.ClientPoolRef);

        clientRequestPtr->sessionRef = sessionRef;

        if(clientRequestPtr->locationClient == 0)
        {
            taf_pa_location_LocationId newClientId = 0;
            pa_result_t createRes = taf_pa_location_CreateClient(&newClientId);
            if (createRes != PA_OK || newClientId == 0)
            {
                LE_ERROR("taf_pa_location_CreateClient failed: res=%d, id=%d",
                         (int)createRes, (int)newClientId);
                le_mem_Release(clientRequestPtr);
                return NULL;
            }
            clientRequestPtr->locationClient = newClientId;
        }else{
            LE_INFO("Reference already created!!");
        }

        InitializeClient(clientRequestPtr);

        void* reqRefPtr = le_ref_CreateRef(gnss.ClientRequestRefMap, clientRequestPtr);
        if (sessionRef!=nullptr) {
            //External client increase Client ref count
            gnss.mClientRefCount++;
        }

        LE_INFO("SessionRef %p was not found, Create Client session, total count %d", sessionRef, gnss.mClientRefCount);
        LE_INFO("reqRefPtr %p, clientRequestPtr %p", reqRefPtr, clientRequestPtr);
        LE_INFO("clientRequestPtr->locationClient: %d", (int)clientRequestPtr->locationClient);

        clientRequestPtr->clientRefPtr = reqRefPtr;
    }

    return clientRequestPtr;
}

taf_locGnss_Client_t* taf_locGnss::AcquireSessionRefInternal
(
    le_msg_SessionRef_t sessionRef   ///< [IN] Explicitly provided session ref
)
{
    auto &gnss = taf_locGnss::GetInstance();

    // Reuse existing client if already registered for this sessionRef
    taf_locGnss_Client_t* clientRequestPtr = DiscoverSessionRef(sessionRef);
    if (clientRequestPtr != NULL)
    {
        LE_DEBUG("AcquireSessionRefInternal: session %p already exists (client: %ld)",
                 sessionRef, clientRequestPtr->locationClient);
        return clientRequestPtr;
    }

    // Check max client limit
    if (gnss.mClientRefCount >= (TAF_CONFIG_POSITIONING_ACTIVATION_MAX - 1))
    {
        LE_DEBUG("AcquireSessionRefInternal: max clients reached: %d",
                 gnss.mClientRefCount);
        return NULL;
    }

    // Allocate and create PA client
    clientRequestPtr = (taf_locGnss_Client_t*)le_mem_ForceAlloc(gnss.ClientPoolRef);

    clientRequestPtr->sessionRef = sessionRef;

    if (clientRequestPtr->locationClient == 0)
    {
        taf_pa_location_LocationId newClientId = 0;
        pa_result_t createRes = taf_pa_location_CreateClient(&newClientId);
        if (createRes != PA_OK || newClientId == 0)
        {
            LE_ERROR("AcquireSessionRefInternal: taf_pa_location_CreateClient failed: "
                     "res=%d, id=%d", (int)createRes, (int)newClientId);
            le_mem_Release(clientRequestPtr);
            return NULL;
        }
        clientRequestPtr->locationClient = newClientId;
    }

    InitializeClient(clientRequestPtr);

    void* reqRefPtr = le_ref_CreateRef(gnss.ClientRequestRefMap, clientRequestPtr);

    LE_INFO("AcquireSessionRefInternal: SessionRef %p created, total count %d",
            sessionRef, gnss.mClientRefCount);
    LE_INFO("reqRefPtr %p, clientRequestPtr %p", reqRefPtr, clientRequestPtr);
    LE_INFO("clientRequestPtr->locationClient: %d",
            (int)clientRequestPtr->locationClient);

    clientRequestPtr->clientRefPtr = reqRefPtr;
    return clientRequestPtr;
}

uint32_t taf_locGnss::TranslateDop
(
    uint32_t dopValue
)
{
    uint16_t retVal = 0;

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    le_msg_SessionRef_t sessionRef = taf_locGnss_GetClientSessionRef();
    taf_locGnss_Resolution_t resolution = TAF_LOCGNSS_RES_UNKNOWN;

    clientRequestPtr = DiscoverSessionRef(sessionRef);

    if (NULL != clientRequestPtr)
    {
        resolution = clientRequestPtr->dopResolution;
    }

    if ( TAF_LOCGNSS_RES_ZERO_DECIMAL == resolution )
    {
        retVal = dopValue / 1e+3;
    } else if ( TAF_LOCGNSS_RES_ONE_DECIMAL == resolution ) {
        retVal = dopValue /100;
    }  else if ( TAF_LOCGNSS_RES_TWO_DECIMAL == resolution ) {
        retVal = dopValue /10;
    }  else {
        retVal = dopValue;
    }

    LE_DEBUG("resolution %d, dopValue %" PRIu32 ", new dopValue %" PRIu16, (int)resolution, dopValue, retVal);
    return retVal;
}

void taf_locGnss::ConfigureAcqStartInfo(taf_locGnss_Client_t* clientRequestPtr) {
    clientRequestPtr->mStarted = true;
    clientRequestPtr->GnssState = TAF_LOCGNSS_STATE_ACTIVE;
    clientRequestPtr->mTtffPtr = 0;
    clientRequestPtr->mStartTime = std::chrono::system_clock::now();
    clientRequestPtr->mTtffReportCount = 0;
    LE_DEBUG("TTFF mStartTime = %ld", (clientRequestPtr->mStartTime).time_since_epoch().count());
    clientRequestPtr->mFirstFix = true;
}

taf_locGnss_State_t taf_locGnss::GetState
(
 void
)
{
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, TAF_LOCGNSS_STATE_READY, "clientRequestPtr is NULL");

    LE_DEBUG("GNSS GetState [%d]", clientRequestPtr->GnssState);

    return clientRequestPtr->GnssState;
}

le_result_t taf_locGnss::Enable
(
 void
)
{
    le_result_t result = LE_FAULT;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_DISABLED:
        {
                clientRequestPtr->GnssState = TAF_LOCGNSS_STATE_READY;
                result = LE_OK;
                clientRequestPtr->mStarted = false;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_DUPLICATE;
        }
        break;
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }

    return result;
}

// ---------------------------------------------------------------------------
// SetConstellation
// ---------------------------------------------------------------------------
static void SetConstellationSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_SetConstellationRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::SetConstellationWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_ConstellationBitMask_t constellationMask = cPtr->params.setConstellation.constellationMask;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_ERROR("SetConstellationWorker: clientRequestPtr is NULL");
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetConstellationSvcRespond, cPtr, NULL);
        return;
    }

    cPtr->retCode = LE_FAULT;
    typedef std::vector<taf_pa_location_SvBlackListInfo_t> SvBlackList;
    SvBlackList svBlackList;
    taf_pa_location_SvBlackListInfo_t blackListInfo;
    bool deviceReset = false;
    blackListInfo.svId = 0; // Here 0 means blacklist all SVIds of a given constellation type
    blackListInfo.constellation = (taf_pa_location_GnssConstellationType_t)TAF_PA_LOCATION_UNKNOWN;

    LE_DEBUG("SetConstellation constellationMask is 0x%02X",constellationMask);
    if( constellationMask & TAF_LOCGNSS_CONSTELLATION_DEFAULT)
    {
        deviceReset = true;
        constellationMask -= TAF_LOCGNSS_CONSTELLATION_DEFAULT;
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_GPS)
        {
            deviceReset = false;
            blackListInfo.constellation = TAF_PA_LOCATION_GPS;
            svBlackList.push_back(blackListInfo);
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_GLONASS)
        {
            deviceReset = false;
            blackListInfo.constellation = TAF_PA_LOCATION_GLONASS;
            svBlackList.push_back(blackListInfo);
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_BEIDOU)
        {
            deviceReset = false;
            blackListInfo.constellation = TAF_PA_LOCATION_BDS;
            svBlackList.push_back(blackListInfo);
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_GALILEO)
        {
            deviceReset = false;
            blackListInfo.constellation = TAF_PA_LOCATION_GALILEO;
            svBlackList.push_back(blackListInfo);
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_SBAS)
        {
            deviceReset = false;
            blackListInfo.constellation = TAF_PA_LOCATION_SBAS;
            svBlackList.push_back(blackListInfo);
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_QZSS)
        {
            deviceReset = false;
            blackListInfo.constellation = TAF_PA_LOCATION_QZSS;
            svBlackList.push_back(blackListInfo);
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_NAVIC)
        {
            deviceReset = false;
            blackListInfo.constellation = TAF_PA_LOCATION_NAVIC;
            svBlackList.push_back(blackListInfo);
        }
        if (blackListInfo.constellation == (taf_pa_location_GnssConstellationType_t)TAF_PA_LOCATION_UNKNOWN)
        {
            LE_DEBUG("constellation type is UNKNOWN");
        }
    }
    else
    {
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_GPS)
        {
            blackListInfo.constellation = TAF_PA_LOCATION_GPS;
            svBlackList.push_back(blackListInfo);
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_GLONASS)
        {
            blackListInfo.constellation = TAF_PA_LOCATION_GLONASS;
            svBlackList.push_back(blackListInfo);
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_BEIDOU)
        {
            blackListInfo.constellation = TAF_PA_LOCATION_BDS;
            svBlackList.push_back(blackListInfo);
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_GALILEO)
        {
            blackListInfo.constellation = TAF_PA_LOCATION_GALILEO;
            svBlackList.push_back(blackListInfo);
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_SBAS)
        {
            blackListInfo.constellation = TAF_PA_LOCATION_SBAS;
            svBlackList.push_back(blackListInfo);
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_QZSS)
        {
            blackListInfo.constellation = TAF_PA_LOCATION_QZSS;
            svBlackList.push_back(blackListInfo);
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_NAVIC)
        {
            blackListInfo.constellation = TAF_PA_LOCATION_NAVIC;
            svBlackList.push_back(blackListInfo);
        }
        if (blackListInfo.constellation == (taf_pa_location_GnssConstellationType_t)TAF_PA_LOCATION_UNKNOWN)
        {
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is UNKNOWN");
        }
    }

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            // Set GNSS constellation
            typedef struct{
                    pa_result_t result;
            }taf_SelfTestResult_t;
            auto cb = [](pa_result_t result, std::any context) {
                taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                resPtr->result = result;
            };
            taf_SelfTestResult_t resCallback = {};
            pa_result_t res = taf_pa_location_configureConstellations(svBlackList, cb, deviceReset, (std::any)(std::any)&resCallback);
            if(res != PA_OK)
            {
                cPtr->retCode = LE_FAULT;
            }
            else
            {
                if(resCallback.result == PA_OK)
                {
                    cPtr->retCode = LE_OK;
                    gnss.mConstellationMask = constellationMask;
                }
                else
                {
                    cPtr->retCode = LE_FAULT;
                }
            }
        }
        break;
        default:
        {
            cPtr->retCode = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetConstellationSvcRespond, cPtr, NULL);
}

void taf_locGnss::SetConstellation(taf_locGnss_ServerCmdRef_t cmdRef,
                                    taf_locGnss_ConstellationBitMask_t constellationMask)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    cPtr->params.setConstellation.constellationMask = constellationMask;
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, SetConstellationWorker, cPtr, NULL);
}

le_result_t taf_locGnss::GetSatellitesStatus
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint8_t* satsInViewCountPtr,
    uint8_t* satsTrackingCountPtr,
    uint8_t* satsUsedCountPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (satsInViewCountPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->satsInViewCountValid)
        {
            *satsInViewCountPtr = posSampleReqPtr->positionSampleNodePtr->satsInViewCount;
        }
        else
        {
            *satsInViewCountPtr = UINT8_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    if (satsTrackingCountPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->satsTrackingCountValid)
        {
            *satsTrackingCountPtr = posSampleReqPtr->positionSampleNodePtr->
                                                                  satsTrackingCount;
        }
        else
        {
            *satsTrackingCountPtr = UINT8_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    if (satsUsedCountPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->satsUsedCountValid)
        {
            *satsUsedCountPtr = posSampleReqPtr->positionSampleNodePtr->
                                                              satsUsedCount;
        }
        else
        {
            *satsUsedCountPtr = UINT8_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

le_result_t taf_locGnss::GetAcquisitionRate
(
    uint32_t* ratePtr
)
{
    le_result_t result = LE_FAULT;

    TAF_KILL_CLIENT_IF_RET_VAL((ratePtr == NULL), LE_FAULT, "Pointer is NULL");

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            // Set the GNSS device acquisition rate
            *ratePtr = clientRequestPtr->mAcqRate;
            result = LE_OK;
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }

    return result;
}

le_result_t taf_locGnss::GetSatellitesInfo
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
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);
    int i;

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (satIdPtr)
    {
        if (NULL != satIdNumPtr) {
            if (posSampleReqPtr->positionSampleNodePtr->satInfoValid)
            {
                for(i=0; i < (int)*satIdNumPtr; i++)
                {
                    satIdPtr[i] = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satId;
                }
            }
            else
            {
                for(i=0; i<(int)*satIdNumPtr; i++)
                {
                    satIdPtr[i] = UINT16_MAX;
                }
                result = LE_OUT_OF_RANGE;
            }
        }
    }

    if (satConstPtr)
    {
        if (NULL != satConstNumPtr) {
            if (posSampleReqPtr->positionSampleNodePtr->satInfoValid)
            {
                for(i=0; i < (int)*satConstNumPtr; i++)
                {
                    satConstPtr[i] = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satConst;
                }
            }
            else
            {
                for(i=0; i < (int)*satConstNumPtr; i++)
                {
                    satConstPtr[i] = TAF_LOCGNSS_SV_CONSTELLATION_UNDEFINED;
                }
                result = LE_OUT_OF_RANGE;
            }
        }
    }

    if (satUsedPtr)
    {
        if (NULL != satUsedNumPtr) {
            if (posSampleReqPtr->positionSampleNodePtr->satsUsedCountValid)
            {
                for(i=0; i < (int)*satUsedNumPtr; i++)
                {
                    satUsedPtr[i] = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satUsed;
                }
            }
            else
            {
                for(i=0; i < (int)*satUsedNumPtr; i++)
                {
                    satUsedPtr[i] = false;
                }
                result = LE_OUT_OF_RANGE;
            }
        }
    }

    if (satSnrPtr)
    {
        if (NULL != satSnrNumPtr) {
            if (posSampleReqPtr->positionSampleNodePtr->satInfoValid)
            {
                for(i=0; i < (int)*satSnrNumPtr; i++)
                {
                    satSnrPtr[i] = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satSnr;
                }
            }
            else
            {
                for(i=0; i < (int)*satSnrNumPtr; i++)
                {
                    satSnrPtr[i] = UINT8_MAX;
                }
                result = LE_OUT_OF_RANGE;
            }
        }
    }

    if (satAzimPtr)
    {
        if (NULL != satAzimNumPtr) {
            if (posSampleReqPtr->positionSampleNodePtr->satInfoValid)
            {
                for(i=0; i < (int)*satAzimNumPtr; i++)
                {
                    satAzimPtr[i] = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satAzim;
                }
            }
            else
            {
                for(i=0; i < (int)*satAzimNumPtr; i++)
                {
                    satAzimPtr[i] = UINT16_MAX;
                }
                result = LE_OUT_OF_RANGE;
            }
        }
    }

    if (satElevPtr)
    {
        if (NULL != satElevNumPtr) {
            if (posSampleReqPtr->positionSampleNodePtr->satInfoValid)
            {
                for(i=0; i < (int)*satElevNumPtr; i++)
                {
                    satElevPtr[i] = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satElev;
                }
            }
            else
            {
                for(i=0; i < (int)*satElevNumPtr; i++)
                {
                    satElevPtr[i] = UINT8_MAX;
                }
                result = LE_OUT_OF_RANGE;
            }
        }
    }

    return result;
}

le_result_t taf_locGnss::GetTtff
(
    uint32_t* ttffPtr
)
{

    TAF_KILL_CLIENT_IF_RET_VAL((ttffPtr == NULL), LE_FAULT, "ttffPtr is NULL");

    le_result_t result = LE_FAULT;

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            if(!clientRequestPtr->mTtffPtr) //calculate ttff on device boot up & cold/warm/hot restart procedure
            {
                *ttffPtr = 0;
                return LE_BUSY;
            }
            else //fetch the first TTFF value & return back
            {
                *ttffPtr = clientRequestPtr->mTtffPtr;
                return LE_OK;
             }
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }
    return result;
}

taf_locGnss_PositionHandlerRef_t taf_locGnss::AddPositionHandler
(
 taf_locGnss_PositionHandlerFunc_t handlerPtr,
 void* contextPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, NULL, "clientRequestPtr is NULL");

    taf_locGnss_PositionHandler_t*  positionHandlerPtr =
        (taf_locGnss_PositionHandler_t*)le_mem_ForceAlloc(PositionHandlerPoolRef);
    memset(positionHandlerPtr, 0, sizeof(taf_locGnss_PositionHandler_t));
    positionHandlerPtr->next = LE_DLS_LINK_INIT;
    positionHandlerPtr->handlerFuncPtr = handlerPtr;
    positionHandlerPtr->handlerContextPtr = contextPtr;
    positionHandlerPtr->sessionRef = taf_locGnss_GetClientSessionRef();

    LE_DEBUG("AddPositionHandler() sessionRef: %p", positionHandlerPtr->sessionRef);

    positionHandlerPtr->handlerRef =
        (taf_locGnss_PositionHandlerRef_t)le_ref_CreateRef(gnss.PositionHandlerRefMap, positionHandlerPtr);

    NumOfPositionHandlers++;

    LE_DEBUG("Created positionHandlerRef(%p) for positionHandlerPtr(%p) (totalCnt=0x%x).",
        positionHandlerPtr->handlerRef, positionHandlerPtr, NumOfPositionHandlers);

    return positionHandlerPtr->handlerRef;
}

taf_locGnss_PositionExHandlerRef_t taf_locGnss::AddPositionExHandler
(
 taf_locGnss_PositionExHandlerFunc_t handlerPtr,
 void* contextPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, NULL, "clientRequestPtr is NULL");

    taf_locGnss_PositionExHandler_t*  PositionHandlerExPtr =
        (taf_locGnss_PositionExHandler_t*)le_mem_ForceAlloc(PositionExHandlerPoolRef);

    memset(PositionHandlerExPtr, 0, sizeof(taf_locGnss_PositionExHandler_t));

    PositionHandlerExPtr->next = LE_DLS_LINK_INIT;
    PositionHandlerExPtr->handlerFuncPtr = handlerPtr;
    PositionHandlerExPtr->handlerContextPtr = contextPtr;
    PositionHandlerExPtr->sessionRef = taf_locGnss_GetClientSessionRef();

    LE_INFO("AddPositionExHandler() sessionRef: %p", PositionHandlerExPtr->sessionRef);

    PositionHandlerExPtr->handlerRef =
        (taf_locGnss_PositionExHandlerRef_t)le_ref_CreateRef(gnss.PositionExHandlerRefMap, PositionHandlerExPtr);

    NumOfPositionExHandlers++;

    LE_INFO("Created PositionHandlerExRef(%p) for PositionHandlerExPtr(%p) (totalCnt=0x%x).",
        PositionHandlerExPtr->handlerRef, PositionHandlerExPtr, NumOfPositionExHandlers);

    return PositionHandlerExPtr->handlerRef;
}

taf_locGnss_MeasurementHandlerRef_t taf_locGnss::AddMeasurementHandler
(
 taf_locGnss_MeasurementHandlerFunc_t handlerPtr,
 void* contextPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, NULL, "clientRequestPtr is NULL");

    taf_locGnss_MeasurementHandler_t*  measHandlerPtr =
        (taf_locGnss_MeasurementHandler_t*)le_mem_ForceAlloc(MeasurementHandlerPoolRef);
    memset(measHandlerPtr, 0, sizeof(taf_locGnss_MeasurementHandler_t));
    measHandlerPtr->next = LE_DLS_LINK_INIT;
    measHandlerPtr->handlerFuncPtr = handlerPtr;
    measHandlerPtr->handlerContextPtr = contextPtr;
    measHandlerPtr->sessionRef = taf_locGnss_GetClientSessionRef();

    LE_INFO("AddMeasurementHandler() sessionRef: %p", measHandlerPtr->sessionRef);

    measHandlerPtr->handlerRef =
        (taf_locGnss_MeasurementHandlerRef_t)le_ref_CreateRef(gnss.MeasurementHandlerRefMap, measHandlerPtr);

    NumOfMeasurementHandlers++;

    LE_INFO("Created measHandlerPtrRef(%p) for measHandlerPtr(%p) (totalCnt=0x%x).",
        measHandlerPtr->handlerRef, measHandlerPtr, NumOfMeasurementHandlers);

    return measHandlerPtr->handlerRef;;
}

taf_locGnss_CapabilityChangeHandlerRef_t taf_locGnss::AddCapabilityHandler
(
    taf_locGnss_CapabilityChangeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, NULL, "clientRequestPtr is NULL");

    taf_locGnss_CapHandler_t*  capHandlerPtr =
        (taf_locGnss_CapHandler_t*)le_mem_ForceAlloc(CapabilityHandlerPoolRef);
    memset(capHandlerPtr, 0, sizeof(taf_locGnss_CapHandler_t));

    capHandlerPtr->next = LE_DLS_LINK_INIT;
    capHandlerPtr->handlerFuncPtr = handlerPtr;
    capHandlerPtr->handlerContextPtr = contextPtr;
    capHandlerPtr->sessionRef = taf_locGnss_GetClientSessionRef();

    LE_INFO("AddCapabilityHandler() sessionRef: %p", capHandlerPtr->sessionRef);

    capHandlerPtr->handlerRef =
        (taf_locGnss_CapabilityChangeHandlerRef_t)le_ref_CreateRef(gnss.CapHandlerRefMap, capHandlerPtr);

    NumOfCapabilityHandlers++;

    LE_INFO("Created capHandlerPtrRef(%p) for capHandlerPtr(%p) (totalCnt=0x%x).",
        capHandlerPtr->handlerRef, capHandlerPtr, NumOfCapabilityHandlers);

    return capHandlerPtr->handlerRef;
}

void taf_locGnss::RemoveCapabilityHandler (taf_locGnss_CapabilityChangeHandlerRef_t handlerRef)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_CapHandler_t* capHandlerPtr =
        (taf_locGnss_CapHandler_t*)le_ref_Lookup(gnss.CapHandlerRefMap, handlerRef);

    if (capHandlerPtr != NULL)
    {
        if(capHandlerPtr->handlerRef != handlerRef) {
            return;
        }
        NumOfCapabilityHandlers--;

        LE_INFO("Removed capHandlerPtrRef(%p) for capHandlerPtr(%p) (totalCnt=0x%x).",
                 capHandlerPtr->handlerRef, capHandlerPtr, NumOfCapabilityHandlers);
        le_ref_DeleteRef(gnss.CapHandlerRefMap, handlerRef);
        le_mem_Release(capHandlerPtr);
    }
    else
    {
        LE_ERROR("Invalid handlerRef(%p).", handlerRef);
    }
}

taf_locGnss_NmeaHandlerRef_t taf_locGnss::AddNmeaHandler
(
    taf_locGnss_NmeaHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, NULL, "clientRequestPtr is NULL");

    taf_locGnss_NmeaHandler_t*  nmeaHandlerPtr =
        (taf_locGnss_NmeaHandler_t*)le_mem_ForceAlloc(NmeaHandlerPoolRef);
    memset(nmeaHandlerPtr, 0, sizeof(taf_locGnss_NmeaHandler_t));

    nmeaHandlerPtr->next = LE_DLS_LINK_INIT;
    nmeaHandlerPtr->handlerFuncPtr = handlerPtr;
    nmeaHandlerPtr->handlerContextPtr = contextPtr;
    nmeaHandlerPtr->sessionRef = taf_locGnss_GetClientSessionRef();

    LE_INFO("AddNmeaHandler() sessionRef: %p", nmeaHandlerPtr->sessionRef);

    nmeaHandlerPtr->handlerRef =
        (taf_locGnss_NmeaHandlerRef_t)le_ref_CreateRef(gnss.NmeaHandlerRefMap, nmeaHandlerPtr);

    NumOfNmeaHandlers++;

    LE_INFO("Created nmeaHandlerPtrRef(%p) for nmeaHandlerPtr(%p) (totalCnt=0x%x).",
        nmeaHandlerPtr->handlerRef, nmeaHandlerPtr, NumOfNmeaHandlers);

    return nmeaHandlerPtr->handlerRef;
}

void taf_locGnss::RemoveNmeaHandler (taf_locGnss_NmeaHandlerRef_t handlerRef)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_NmeaHandler_t* nmeaHandlerPtr =
        (taf_locGnss_NmeaHandler_t*)le_ref_Lookup(gnss.NmeaHandlerRefMap, handlerRef);

    if (nmeaHandlerPtr != NULL)
    {
        if(nmeaHandlerPtr->handlerRef != handlerRef) {
            return;
        }
        NumOfNmeaHandlers--;

        LE_INFO("Removed nmeaHandlerPtrRef(%p) for nmeaHandlerPtr(%p) (totalCnt=0x%x).",
                 nmeaHandlerPtr->handlerRef, nmeaHandlerPtr, NumOfNmeaHandlers);
        le_ref_DeleteRef(gnss.NmeaHandlerRefMap, handlerRef);
        le_mem_Release(nmeaHandlerPtr);
    }
    else
    {
        LE_ERROR("Invalid handlerRef(%p).", handlerRef);
    }
}

le_result_t taf_locGnss::GetConstellation
(
    taf_locGnss_ConstellationBitMask_t *constellationMaskPtr
)
{
    le_result_t result = LE_FAULT;

    TAF_KILL_CLIENT_IF_RET_VAL((constellationMaskPtr == NULL), LE_FAULT, "constellationMaskPtr is NULL");
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
            {
                LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
                result = LE_NOT_PERMITTED;
            }
            break;
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
            {
                // Get GNSS constellation
                LE_DEBUG("GetConstellation constellationMask is 0x%02X",mConstellationMask);
                if(mConstellationMask & TAF_LOCGNSS_CONSTELLATION_GPS)
                {
                    *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_GPS;
                    result = LE_OK;
                }
                if(mConstellationMask & TAF_LOCGNSS_CONSTELLATION_GLONASS)
                {
                    *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_GLONASS;
                    result = LE_OK;
                }
                if(mConstellationMask & TAF_LOCGNSS_CONSTELLATION_BEIDOU)
                {
                    *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_BEIDOU;
                    result = LE_OK;
                }
                if(mConstellationMask & TAF_LOCGNSS_CONSTELLATION_GALILEO)
                {
                    *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_GALILEO;
                    result = LE_OK;
                }
                if(mConstellationMask & TAF_LOCGNSS_CONSTELLATION_SBAS)
                {
                    *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_SBAS;
                    result = LE_OK;
                }
                if(mConstellationMask & TAF_LOCGNSS_CONSTELLATION_QZSS)
                {
                    *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_QZSS;
                    result = LE_OK;
                }
                if(mConstellationMask & TAF_LOCGNSS_CONSTELLATION_NAVIC)
                {
                    *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_NAVIC;
                    result = LE_OK;
                }
                if (LE_OK != result)
                {
                    *constellationMaskPtr = 0;
                    LE_ERROR("constellation type is invalid");
                    result = LE_FAULT;
                    LE_ERROR("Unable to get the constellation, error = %d (%s)",
                          result, LE_RESULT_TXT(result));
                }
            }
            break;
        default:
            {
                result = LE_FAULT;
                LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
            }
            break;
    }

    return result;
}

taf_locGnss_SampleRef_t taf_locGnss::GetLastSampleRef
(
    void
)
{
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    le_msg_SessionRef_t sessionRef = taf_locGnss_GetClientSessionRef();

    clientRequestPtr = DiscoverSessionRef(sessionRef);

    TAF_ERROR_IF_RET_VAL(NULL == clientRequestPtr, NULL, "clientRequestPtr is NULL");

    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr =
        (taf_locGnss_PositionSampleRequest_t*)le_mem_ForceAlloc(PositionSampleRequestPoolRef);

    memset(posSampleReqPtr, 0, sizeof(taf_locGnss_PositionSampleRequest_t));

    posSampleReqPtr->positionSampleNodePtr =
       (taf_locGnss_PositionSample_t*)le_mem_ForceAlloc(PositionSamplePoolRef);
    memset(posSampleReqPtr->positionSampleNodePtr, 0, sizeof(taf_locGnss_PositionSample_t));

    memcpy(posSampleReqPtr->positionSampleNodePtr, &clientRequestPtr->LastPositionSample, sizeof(taf_locGnss_PositionSample_t));


    LE_DEBUG("Get sample %p", posSampleReqPtr->positionSampleNodePtr);

    posSampleReqPtr->sessionRef = sessionRef;

    taf_locGnss_SampleRef_t reqRef = (taf_locGnss_SampleRef_t)le_ref_CreateRef(PositionSampleMap, posSampleReqPtr);
    posSampleReqPtr->positionSampleRef = reqRef;

    return reqRef;
}

le_result_t taf_locGnss::GetPositionState
(
 taf_locGnss_SampleRef_t positionSampleRef,
 taf_locGnss_FixState_t* statePtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr =
                                                (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(gnss.PositionSampleMap,positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL((statePtr == NULL), LE_FAULT, "statePtr is NULL");

    result = gnss.CheckValidatePosition(posSampleReqPtr);
    if (LE_OK != result)
    {
        return result;
    }
    LE_DEBUG("GetPositionState reportStatus:%d, techmask :%d",
        posSampleReqPtr->positionSampleNodePtr->reportStatus,
            posSampleReqPtr->positionSampleNodePtr->techMask);
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = gnss.DiscoverSessionRef(posSampleReqPtr->sessionRef);

    if (NULL == clientRequestPtr) {
        LE_ERROR("GetPositionState did not find sessionRef: %p", posSampleReqPtr->sessionRef);
        return LE_FAULT;
    }

    if(posSampleReqPtr->positionSampleNodePtr->reportStatus == TAF_LOCGNSS_REPORT_STATUS_SUCCESS)//if report status is success
    {
        if(posSampleReqPtr->positionSampleNodePtr->validityExMask
                                                       & (1ULL << TAF_LOCGNSS_HAS_POS_TECH_MASK))
        {
            if(posSampleReqPtr->positionSampleNodePtr->techMask & TAF_LOCGNSS_LOC_GNSS) //0x01
            {
                if(posSampleReqPtr->positionSampleNodePtr->validityMask
                                                       & TAF_LOCGNSS_HAS_ALTITUDE_BIT)
                {
                    posSampleReqPtr->positionSampleNodePtr->fixState = TAF_LOCGNSS_STATE_FIX_3D;
                }
                else
                {
                    if(posSampleReqPtr->positionSampleNodePtr->validityMask & TAF_LOCGNSS_HAS_LAT_LONG_BIT)
                    {
                        posSampleReqPtr->positionSampleNodePtr->fixState = TAF_LOCGNSS_STATE_FIX_2D;
                    }
                    else
                    {
                        posSampleReqPtr->positionSampleNodePtr->fixState =
                                                                TAF_LOCGNSS_STATE_FIX_NO_POS;
                    }
                }
            }
            else
            {
                if((posSampleReqPtr->positionSampleNodePtr->techMask == TAF_LOCGNSS_LOC_SENSORS)
                ||(posSampleReqPtr->positionSampleNodePtr->techMask == TAF_LOCGNSS_STATE_FIX_ESTIMATED))
                {
                    posSampleReqPtr->positionSampleNodePtr->fixState =
                                                                  TAF_LOCGNSS_STATE_FIX_ESTIMATED;
                }
                else
                {
                    posSampleReqPtr->positionSampleNodePtr->fixState = TAF_LOCGNSS_STATE_FIX_NO_POS;
                }
            }
        }
        else
        {
            posSampleReqPtr->positionSampleNodePtr->fixState = TAF_LOCGNSS_STATE_FIX_NO_POS;
        }
    }
    else //if report status is failure or intermediate
    {
        posSampleReqPtr->positionSampleNodePtr->fixState = TAF_LOCGNSS_STATE_FIX_NO_POS;
    }
    *statePtr = posSampleReqPtr->positionSampleNodePtr->fixState;

    return result;
}

le_result_t taf_locGnss::GetDirection
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint32_t* directionPtr,
    uint32_t* directionAccuracyPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (directionPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->directionValid)
        {
            *directionPtr = posSampleReqPtr->positionSampleNodePtr->direction;
        }
        else
        {
            *directionPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (directionAccuracyPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->directionAccuracyValid)
        {
            *directionAccuracyPtr = posSampleReqPtr->positionSampleNodePtr->
                                    directionAccuracy;
        }
        else
        {
            *directionAccuracyPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

le_result_t taf_locGnss::GetDate
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint16_t* yearPtr,
    uint16_t* monthPtr,
    uint16_t* dayPtr
)
{

    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr =
                                                (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL((yearPtr== NULL) || (NULL == monthPtr)
            || (NULL == dayPtr), LE_FAULT, "Invalid pointers provided");

    result = CheckValidatePosition(posSampleReqPtr);
    if (LE_OK != result)
    {
        return result;
    }

    if (posSampleReqPtr->positionSampleNodePtr->dateValid)
    {
        result = LE_OK;
        if (yearPtr)
        {
            *yearPtr = posSampleReqPtr->positionSampleNodePtr->year;
        }
        if (monthPtr)
        {
            *monthPtr = posSampleReqPtr->positionSampleNodePtr->month;
        }
        if (dayPtr)
        {
            *dayPtr = posSampleReqPtr->positionSampleNodePtr->day;
        }
    } else {
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

le_result_t taf_locGnss::GetTime
(
 taf_locGnss_SampleRef_t positionSampleRef,
 uint16_t* hrsPtr,
 uint16_t* minPtr,
 uint16_t* secPtr,
 uint16_t* msecPtr
 )
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL((NULL == hrsPtr) || (NULL == minPtr)
                               || (NULL == secPtr) || (NULL == msecPtr), LE_FAULT, "Invalid pointers provided");

    result = CheckValidatePosition(posSampleReqPtr);
    if (LE_OK != result)
    {
        return result;
    }

    if (posSampleReqPtr->positionSampleNodePtr->timeValid)
    {
        result = LE_OK;
        if (hrsPtr)
        {
            *hrsPtr = posSampleReqPtr->positionSampleNodePtr->hours;
        }
        if (minPtr)
        {
            *minPtr = posSampleReqPtr->positionSampleNodePtr->minutes;
        }
        if (secPtr)
        {
            *secPtr = posSampleReqPtr->positionSampleNodePtr->seconds;
        }
        if (msecPtr)
        {
            *msecPtr = posSampleReqPtr->positionSampleNodePtr->milliseconds;
        }
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        if (hrsPtr)
        {
            *hrsPtr = 0;
        }
        if (minPtr)
        {
            *minPtr = 0;
        }
        if (secPtr)
        {
            *secPtr = 0;
        }
        if (msecPtr)
        {
            *msecPtr = 0;
        }
    }

    return result;
}

le_result_t taf_locGnss::GetLocation
(
 taf_locGnss_SampleRef_t positionSampleRef,
 int32_t* latitudePtr,
 int32_t* longitudePtr,
 int32_t* hAccuracyPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (latitudePtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->latitudeValid)
        {
            *latitudePtr = posSampleReqPtr->positionSampleNodePtr->latitude;
        }
        else
        {
            *latitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (longitudePtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->longitudeValid)
        {
            *longitudePtr = posSampleReqPtr->positionSampleNodePtr->longitude;
        }
        else
        {
            *longitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (hAccuracyPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->hAccuracyValid)
        {
            *hAccuracyPtr = posSampleReqPtr->positionSampleNodePtr->hAccuracy;
        }
        else
        {
            *hAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

le_result_t taf_locGnss::GetAltitude
(
 taf_locGnss_SampleRef_t positionSampleRef,
 int32_t* altitudePtr,
 int32_t* vAccuracyPtr
 )
{
    auto &gnss = taf_locGnss::GetInstance();
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t * posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (altitudePtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->altitudeValid)
        {
            *altitudePtr = posSampleReqPtr->positionSampleNodePtr->altitude;
        }
        else
         {
            *altitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (vAccuracyPtr)
    {
        if ((false == posSampleReqPtr->positionSampleNodePtr->vAccuracyValid) ||
            (LE_OK != gnss.PositionDataCoversion(
                                     posSampleReqPtr->positionSampleNodePtr->vAccuracy,
                                     TAF_LOCGNSS_DATA_VACCURACY,
                                     vAccuracyPtr))
           )
        {
            *vAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}

le_result_t taf_locGnss::GetHorizontalSpeed
(
 taf_locGnss_SampleRef_t positionSampleRef,
 uint32_t* hspeedPtr,
 uint32_t* hspeedAccuracyPtr
 )
{
    auto &gnss = taf_locGnss::GetInstance();
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (hspeedPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->hSpeedValid)
        {
            *hspeedPtr = posSampleReqPtr->positionSampleNodePtr->hSpeed;
        }
        else
        {
            *hspeedPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (hspeedAccuracyPtr)
    {
        if((false == posSampleReqPtr->positionSampleNodePtr->hSpeedAccuracyValid) ||
           (LE_OK != gnss.PositionDataCoversion(
                                posSampleReqPtr->positionSampleNodePtr->hSpeedAccuracy,
                                TAF_LOCGNSS_DATA_HSPEEDACCURACY,
                                (int32_t*)hspeedAccuracyPtr))
           )
        {
            *hspeedAccuracyPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

le_result_t taf_locGnss::GetVerticalSpeed
(
 taf_locGnss_SampleRef_t positionSampleRef,
 int32_t* vspeedPtr,
 int32_t* vspeedAccuracyPtr
 )
{
    auto &gnss = taf_locGnss::GetInstance();
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (vspeedPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->vSpeedValid)
        {
            *vspeedPtr = posSampleReqPtr->positionSampleNodePtr->vSpeed;
        }
        else
        {
            *vspeedPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (vspeedAccuracyPtr)
    {
        if((false == posSampleReqPtr->positionSampleNodePtr->vSpeedAccuracyValid) ||
            (LE_OK != gnss.PositionDataCoversion(
                                posSampleReqPtr->positionSampleNodePtr->vSpeedAccuracy,
                                TAF_LOCGNSS_DATA_VSPEEDACCURACY,
                                vspeedAccuracyPtr))
            )
        {
            *vspeedAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

le_result_t taf_locGnss::GetGpsLeapSeconds
(
 taf_locGnss_SampleRef_t positionSampleRef,
 uint8_t* leapSecondsPtr
)
{
    le_result_t result;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL((leapSecondsPtr == NULL), LE_FAULT, "Invalid reference");

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (posSampleReqPtr->positionSampleNodePtr->leapSecondsValid)
    {
        result = LE_OK;
        *leapSecondsPtr = posSampleReqPtr->positionSampleNodePtr->leapSeconds;
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        *leapSecondsPtr = UINT8_MAX;
    }

    return result;
}

// ---------------------------------------------------------------------------
// DeleteDRSensorCalData
// ---------------------------------------------------------------------------
static void DeleteDRSensorCalDataSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_DeleteDRSensorCalDataRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::DeleteDRSensorCalDataWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, DeleteDRSensorCalDataSvcRespond, cPtr, NULL);
        return;
    }

    cPtr->retCode = LE_OK;

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss state [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        {
                typedef struct{
                    pa_result_t result;
                }taf_SelfTestResult_t;
                auto cb1 = [](pa_result_t result, std::any context) {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                };

                /* Specifies AidingDataType mask */
                /* 0 - EPHEMERIS 1 - DR_SENSOR_CALIBRATION
                   AidingData |1UL << 1 which is 2*/


                taf_SelfTestResult_t resCallback = {};
                uint32_t AidingData = TAF_LOCGNSS_AIDING_DATA_DR_SENSOR_CALIBRATION;
                pa_result_t res = taf_pa_location_deleteAidingData((taf_pa_location_AidingDataType_t)AidingData, cb1,(std::any)&resCallback);
                if (res != PA_OK)
                {
                    if (res == PA_NOT_IMPLEMENTED)
                    {
                        LE_ERROR("DeleteDRSensorCalData failed or Not Implemented");
                    }
                    cPtr->retCode = LE_FAULT;
                }
                else
                {
                    if(resCallback.result != PA_OK)
                    {
                        cPtr->retCode = LE_FAULT;
                    }
                }
        }
        break;
        default:
        {
            LE_ERROR("Invalid GNSS state %d", clientRequestPtr->GnssState);
            cPtr->retCode = LE_FAULT;
        }
        break;
    }
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, DeleteDRSensorCalDataSvcRespond, cPtr, NULL);
}

void taf_locGnss::DeleteDRSensorCalData(taf_locGnss_ServerCmdRef_t cmdRef)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, DeleteDRSensorCalDataWorker, cPtr, NULL);
}

le_result_t taf_locGnss::GetLeapSecondsUncertainty
(
 taf_locGnss_SampleRef_t positionSampleRef,
 uint8_t* leapSecondsUncPtr
)
{
    le_result_t result;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL((leapSecondsUncPtr == NULL), LE_FAULT, "Invalid reference");

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (posSampleReqPtr->positionSampleNodePtr->leapSecondsUncValid)
    {
        result = LE_OK;
        *leapSecondsUncPtr = posSampleReqPtr->positionSampleNodePtr->leapSecondsUnc;
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        *leapSecondsUncPtr = UINT8_MAX;
    }

    return result;
}

le_result_t taf_locGnss::GetMagneticDeviation
(
 taf_locGnss_SampleRef_t positionSampleRef,
 int32_t* magneticDeviationPtr
)
{
    le_result_t result;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL((magneticDeviationPtr == NULL), LE_FAULT, "Invalid reference");

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (posSampleReqPtr->positionSampleNodePtr->magneticDeviationValid)
    {
        result = LE_OK;
        *magneticDeviationPtr = posSampleReqPtr->positionSampleNodePtr->magneticDeviation;
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        *magneticDeviationPtr = INT32_MAX;
    }

    return result;
}

le_result_t taf_locGnss::GetEllipticalUncertainty
(
 taf_locGnss_SampleRef_t positionSampleRef,
 uint32_t* horUncEllipseSemiMajorPtr,
 uint32_t* horUncEllipseSemiMinorPtr,
 uint8_t*  horConfidencePtr
)
{
    le_result_t result;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }
    if(horUncEllipseSemiMajorPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->horUncEllipseSemiMajorValid)
        {
            result = LE_OK;
            *horUncEllipseSemiMajorPtr = posSampleReqPtr->positionSampleNodePtr->horUncEllipseSemiMajor;
        }
        else
        {
            result = LE_OUT_OF_RANGE;
            *horUncEllipseSemiMajorPtr = UINT32_MAX;
        }
    }
    if(horUncEllipseSemiMinorPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->horUncEllipseSemiMinorValid)
        {
            result = LE_OK;
            *horUncEllipseSemiMinorPtr = posSampleReqPtr->positionSampleNodePtr->horUncEllipseSemiMinor;
        }
        else
        {
            result = LE_OUT_OF_RANGE;
            *horUncEllipseSemiMinorPtr = UINT32_MAX;
        }
    }
    if(horConfidencePtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->horConfidenceValid)
        {
            result = LE_OK;
            *horConfidencePtr = posSampleReqPtr->positionSampleNodePtr->horConfidence;
        }
        else
        {
            result = LE_OUT_OF_RANGE;
            *horConfidencePtr = UINT8_MAX;
        }
    }

    return result;
}

le_result_t taf_locGnss::GetTimeAccuracy
(
 taf_locGnss_SampleRef_t posRef,
 uint32_t* timeAccuracyPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,posRef);

    TAF_KILL_CLIENT_IF_RET_VAL((timeAccuracyPtr == NULL), LE_FAULT, "Invalid reference");

    result = CheckValidatePosition(posReqPtr);
    if (LE_OK != result)
    {
        return result;
    }

    if (posReqPtr->positionSampleNodePtr->timeAccuracyValid)
    {
        if (timeAccuracyPtr)
        {
            *timeAccuracyPtr = posReqPtr->positionSampleNodePtr->timeAccuracy;
            if(*timeAccuracyPtr == UINT32_MAX)
            {
                result = LE_OUT_OF_RANGE;
            }
        }
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        if (timeAccuracyPtr)
        {
            *timeAccuracyPtr = UINT32_MAX;
        }
    }

    return result;
}

le_result_t taf_locGnss::GetEpochTime
(
 taf_locGnss_SampleRef_t posRef,
 uint64_t* millisecondsPtr
)
{
    le_result_t result;
    taf_locGnss_PositionSampleRequest_t* posReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,posRef);

    TAF_KILL_CLIENT_IF_RET_VAL((millisecondsPtr == NULL), LE_FAULT, "Invalid reference");

    result = CheckValidatePosition(posReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (posReqPtr->positionSampleNodePtr->timeValid)
    {
        result = LE_OK;
        *millisecondsPtr = posReqPtr->positionSampleNodePtr->epochTime;
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        *millisecondsPtr = 0;
    }

    return result;
}

le_result_t taf_locGnss::SetDopResolution
(
 taf_locGnss_Resolution_t resolution
)
{
    taf_locGnss_Client_t* clientRequestPtr = NULL;

    TAF_ERROR_IF_RET_VAL( resolution >= TAF_LOCGNSS_RES_UNKNOWN, LE_BAD_PARAMETER, "Invalid resolution");

    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    clientRequestPtr->dopResolution = resolution;
    LE_DEBUG("clientRequest %p, resolution %d saved", clientRequestPtr, (int)resolution);
    return LE_OK;
}


le_result_t taf_locGnss::GetDilutionOfPrecision
(
 taf_locGnss_SampleRef_t posRef,
 taf_locGnss_DopType_t dopType,
 uint16_t* dopPtr
)
{
    uint32_t dop = 0;
    bool dopValid = false;
    taf_locGnss_PositionSampleRequest_t* posReqPtr = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,posRef);
    le_result_t result = CheckValidatePosition(posReqPtr);
    if (LE_OK != result)
    {
        return result;
    }

    if (dopPtr)
    {
        *dopPtr = UINT16_MAX;

        if( TAF_LOCGNSS_PDOP == dopType) {
            if (posReqPtr->positionSampleNodePtr->pdopValid)
            {
                dop = TranslateDop(posReqPtr->positionSampleNodePtr->pdop);
                dopValid = true;
            }
        } else if( TAF_LOCGNSS_HDOP == dopType) {
            if (posReqPtr->positionSampleNodePtr->hdopValid)
            {
                dop = TranslateDop(posReqPtr->positionSampleNodePtr->hdop);
                dopValid = true;
            }
        } else if( TAF_LOCGNSS_VDOP == dopType) {
            if (posReqPtr->positionSampleNodePtr->vdopValid)
            {
                dop = TranslateDop(posReqPtr->positionSampleNodePtr->vdop);
                dopValid = true;
            }
        } else if ( TAF_LOCGNSS_GDOP == dopType) {
            if (posReqPtr->positionSampleNodePtr->gdopValid)
            {
                dop = TranslateDop(posReqPtr->positionSampleNodePtr->gdop);
                dopValid = true;
            }
        } else if( TAF_LOCGNSS_TDOP == dopType) {
            if (posReqPtr->positionSampleNodePtr->tdopValid)
            {
                dop = TranslateDop(posReqPtr->positionSampleNodePtr->tdop);
                dopValid = true;
            }
        } else {
            LE_ERROR("Unknown dilution of precision type %d", dopType);
        }

        if ((true == dopValid) && (!(dop >> 16)))
        {
            *dopPtr = (uint16_t)dop;
            return LE_OK;
        }
    }

    return LE_OUT_OF_RANGE;
}

le_result_t taf_locGnss::GetLeapSeconds
(
 uint64_t* gpsTime,
 int32_t* currentLeapSeconds,
 uint64_t* changeEventTime,
 int32_t* nextLeapSeconds
)
{
    TAF_ERROR_IF_RET_VAL( NULL == gpsTime, LE_FAULT, "gpsTime is NULL");
    TAF_ERROR_IF_RET_VAL( NULL == currentLeapSeconds, LE_FAULT, "currentLeapSeconds is NULL");
    TAF_ERROR_IF_RET_VAL( NULL == changeEventTime, LE_FAULT, "changeEventTime is NULL");
    TAF_ERROR_IF_RET_VAL( NULL == nextLeapSeconds, LE_FAULT, "nextLeapSeconds is NULL");

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    *gpsTime = clientRequestPtr->mGpsTime;
    *currentLeapSeconds = clientRequestPtr->mCurrentLeapSeconds;
    *changeEventTime = clientRequestPtr->mChangeEventTime;
    *nextLeapSeconds = clientRequestPtr->mNextLeapSeconds;
    return LE_OK;
}

le_result_t taf_locGnss::GetGpsTime
(
    taf_locGnss_SampleRef_t posRef,
    uint32_t* gpsWeek,
    uint32_t* gpsTimeOfWeek
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posReqPtr = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,posRef);

    TAF_KILL_CLIENT_IF_RET_VAL(((NULL == gpsWeek) || (NULL == gpsTimeOfWeek)), LE_FAULT, "Invalid reference");

    result = CheckValidatePosition(posReqPtr);
    if (LE_OK != result) {
        return result;
    }

    if (posReqPtr->positionSampleNodePtr->gpsTimeValid) {
        if (gpsTimeOfWeek) {
            *gpsTimeOfWeek = posReqPtr->positionSampleNodePtr->gpsTimeOfWeek;
        }
        if (gpsWeek) {
            *gpsWeek = posReqPtr->positionSampleNodePtr->gpsWeek;
        }
        result = LE_OK;
    } else {
        if (gpsTimeOfWeek) {
            *gpsTimeOfWeek = 0;
        }
        if (gpsWeek) {
            *gpsWeek = 0;
        }
        result = LE_OUT_OF_RANGE;
    }

    return result;
}

le_result_t taf_locGnss::SetAcquisitionRate
(
    uint32_t  rate
)
{
    le_result_t result;

    TAF_ERROR_IF_RET_VAL( 0 == rate, LE_OUT_OF_RANGE, "Acquisition rate is zero");

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    // Check the GNSS device state
    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        {
            // Set the GNSS device acquisition rate
            if(rate < 100)
            {
                LE_ERROR("SetAcquisitionRate -> mAcqRate is less than 100ms, return LE_OUT_OF_RANGE");
                return LE_OUT_OF_RANGE;
            }
            clientRequestPtr->mAcqRate = rate;
            result = LE_OK;
        }
        break;
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            result = LE_NOT_PERMITTED;
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }

    return result;
}

// ---------------------------------------------------------------------------
// ForceColdRestart
// ---------------------------------------------------------------------------
static void ForceColdRestartSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_ForceColdRestartRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::ForceColdRestartWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_ERROR("ForceColdRestartWorker: clientRequestPtr is NULL");
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ForceColdRestartSvcRespond, cPtr, NULL);
        return;
    }

    typedef struct{ pa_result_t result; }taf_SelfTestResult_t;
    le_result_t result = LE_OK;

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
            LE_ERROR("ForceColdRestart: Wrong state [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
            break;
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            if (clientRequestPtr->mStarted)
            {
                auto cb = [](pa_result_t r, std::any ctx) {
                    ((taf_SelfTestResult_t*)std::any_cast<taf_SelfTestResult_t*>(ctx))->result = r; };
                taf_SelfTestResult_t rc = {};
                pa_result_t res = taf_pa_location_stopReports(clientRequestPtr->locationClient, cb, (std::any)&rc);
                if (res != PA_OK) { result = LE_FAULT; }
                else if (rc.result == PA_OK) { clientRequestPtr->mStarted = false; clientRequestPtr->GnssState = TAF_LOCGNSS_STATE_READY; }
                else { result = LE_FAULT; }
            }
            if (result == LE_OK)
            {
                auto cb1 = [](pa_result_t r, std::any ctx) {
                    ((taf_SelfTestResult_t*)std::any_cast<taf_SelfTestResult_t*>(ctx))->result = r; };
                taf_SelfTestResult_t rc1 = {};
                pa_result_t res1 = taf_pa_location_deleteAllAidingData(cb1, (std::any)&rc1);
                if (res1 != PA_OK) { result = LE_FAULT; }
                else if (rc1.result != PA_OK) { result = LE_FAULT; }
            }
            if (result == LE_OK && !clientRequestPtr->mStarted)
            {
                int optInterval = clientRequestPtr->mAcqRate;
                if (optInterval == 0 || optInterval < 100) { optInterval = 100; clientRequestPtr->mAcqRate = optInterval; }
                LocReqEngine engineType = DEFAULT_UNKNOWN;
                GnssReportTypeMask reportMask = 0x7f;
                engineType |= (1UL << clientRequestPtr->mEngineType);
                auto cb2 = [](pa_result_t r, std::any ctx) {
                    ((taf_SelfTestResult_t*)std::any_cast<taf_SelfTestResult_t*>(ctx))->result = r; };
                taf_SelfTestResult_t rc2 = {};
                sleep(1);
                pa_result_t res2 = taf_pa_location_startDetailedEngineReports(clientRequestPtr->locationClient,
                    (uint32_t)optInterval, engineType, cb2, reportMask, (std::any)&rc2);
                if (res2 != PA_OK) { result = LE_FAULT; }
                else if (rc2.result == PA_OK) { ConfigureAcqStartInfo(clientRequestPtr); }
                else { result = LE_FAULT; }
            }
            break;
        }
        default:
            LE_ERROR("ForceColdRestart: Invalid state %d", clientRequestPtr->GnssState);
            result = LE_FAULT;
            break;
    }
    cPtr->retCode = result;
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ForceColdRestartSvcRespond, cPtr, NULL);
}

void taf_locGnss::ForceColdRestart(taf_locGnss_ServerCmdRef_t cmdRef)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, ForceColdRestartWorker, cPtr, NULL);
}

// ---------------------------------------------------------------------------
// ForceWarmRestart
// ---------------------------------------------------------------------------
static void ForceWarmRestartSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_ForceWarmRestartRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::ForceWarmRestartWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ForceWarmRestartSvcRespond, cPtr, NULL);
        return;
    }

    typedef struct{ pa_result_t result; }taf_SelfTestResult_t;
    le_result_t result = LE_OK;

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
            LE_ERROR("ForceWarmRestart: Wrong state [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
            break;
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            if (clientRequestPtr->mStarted)
            {
                auto cb = [](pa_result_t r, std::any ctx) {
                    ((taf_SelfTestResult_t*)std::any_cast<taf_SelfTestResult_t*>(ctx))->result = r; };
                taf_SelfTestResult_t rc = {};
                pa_result_t res = taf_pa_location_stopReports(clientRequestPtr->locationClient, cb, (std::any)&rc);
                if (res != PA_OK || rc.result != PA_OK) { result = LE_FAULT; }
                else { clientRequestPtr->mStarted = false; clientRequestPtr->GnssState = TAF_LOCGNSS_STATE_READY; }
            }
            else { result = LE_FAULT; }

            if (result == LE_OK)
            {
                auto cb1 = [](pa_result_t r, std::any ctx) {
                    ((taf_SelfTestResult_t*)std::any_cast<taf_SelfTestResult_t*>(ctx))->result = r; };
                taf_SelfTestResult_t rc1 = {};
                uint32_t AidingData = TAF_LOCGNSS_AIDING_DATA_EPHEMERIS;
                pa_result_t res1 = taf_pa_location_deleteAidingData(
                    (taf_pa_location_AidingDataType_t)AidingData, cb1, (std::any)&rc1);
                if (res1 != PA_OK) { result = LE_FAULT; }
                else if (rc1.result != PA_OK) { result = (le_result_t)rc1.result; }
            }

            if (result == LE_OK && !clientRequestPtr->mStarted)
            {
                int optInterval = clientRequestPtr->mAcqRate;
                if (optInterval == 0 || optInterval < 100) { optInterval = 100; clientRequestPtr->mAcqRate = optInterval; }
                LocReqEngine engineType = DEFAULT_UNKNOWN;
                GnssReportTypeMask reportMask = 0x7f;
                engineType |= (1UL << clientRequestPtr->mEngineType);
                sleep(1);
                auto cb2 = [](pa_result_t r, std::any ctx) {
                    ((taf_SelfTestResult_t*)std::any_cast<taf_SelfTestResult_t*>(ctx))->result = r; };
                taf_SelfTestResult_t rc2 = {};
                pa_result_t res2 = taf_pa_location_startDetailedEngineReports(clientRequestPtr->locationClient,
                    (uint32_t)optInterval, engineType, cb2, reportMask, (std::any)&rc2);
                if (res2 != PA_OK) { result = LE_FAULT; }
                else if (rc2.result == PA_OK) { ConfigureAcqStartInfo(clientRequestPtr); }
                else { result = LE_FAULT; }
            }
            break;
        }
        default:
            LE_ERROR("ForceWarmRestart: Invalid state %d", clientRequestPtr->GnssState);
            result = LE_FAULT;
            break;
    }
    cPtr->retCode = result;
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ForceWarmRestartSvcRespond, cPtr, NULL);
}

void taf_locGnss::ForceWarmRestart(taf_locGnss_ServerCmdRef_t cmdRef)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, ForceWarmRestartWorker, cPtr, NULL);
}

// ---------------------------------------------------------------------------
// ForceHotRestart
// ---------------------------------------------------------------------------
static void ForceHotRestartSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_ForceHotRestartRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::ForceHotRestartWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ForceHotRestartSvcRespond, cPtr, NULL);
        return;
    }

    typedef struct{ pa_result_t result; }taf_SelfTestResult_t;
    le_result_t result = LE_OK;

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
            LE_ERROR("ForceHotRestart: Wrong state [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
            break;
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            if (clientRequestPtr->mStarted)
            {
                auto cb = [](pa_result_t r, std::any ctx) {
                    ((taf_SelfTestResult_t*)std::any_cast<taf_SelfTestResult_t*>(ctx))->result = r; };
                taf_SelfTestResult_t rc = {};
                pa_result_t res = taf_pa_location_stopReports(clientRequestPtr->locationClient, cb, (std::any)&rc);
                if (res != PA_OK) { result = LE_FAULT; }
                else if (rc.result == PA_OK) { clientRequestPtr->mStarted = false; clientRequestPtr->GnssState = TAF_LOCGNSS_STATE_READY; }
                else { result = LE_FAULT; }
            }
            if (result == LE_OK && !clientRequestPtr->mStarted)
            {
                int optInterval = clientRequestPtr->mAcqRate;
                if (optInterval == 0 || optInterval < 100) { optInterval = 100; clientRequestPtr->mAcqRate = optInterval; }
                LocReqEngine engineType = DEFAULT_UNKNOWN;
                GnssReportTypeMask reportMask = 0x7f;
                engineType |= (1UL << clientRequestPtr->mEngineType);
                sleep(1);
                auto cb2 = [](pa_result_t r, std::any ctx) {
                    ((taf_SelfTestResult_t*)std::any_cast<taf_SelfTestResult_t*>(ctx))->result = r; };
                taf_SelfTestResult_t rc2 = {};
                pa_result_t res2 = taf_pa_location_startDetailedEngineReports(clientRequestPtr->locationClient,
                    (uint32_t)optInterval, engineType, cb2, reportMask, (std::any)&rc2);
                if (res2 != PA_OK) { result = LE_FAULT; }
                else if (rc2.result == PA_OK) { ConfigureAcqStartInfo(clientRequestPtr); }
                else { result = LE_FAULT; }
            }
            break;
        }
        default:
            LE_ERROR("ForceHotRestart: Invalid state %d", clientRequestPtr->GnssState);
            result = LE_FAULT;
            break;
    }
    cPtr->retCode = result;
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ForceHotRestartSvcRespond, cPtr, NULL);
}

void taf_locGnss::ForceHotRestart(taf_locGnss_ServerCmdRef_t cmdRef)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, ForceHotRestartWorker, cPtr, NULL);
}

le_result_t taf_locGnss::GetSupportedConstellations
(
 taf_locGnss_ConstellationBitMask_t* constellationMaskPtr
)
{
     TAF_ERROR_IF_RET_VAL( constellationMaskPtr == NULL, LE_FAULT, "constellationMaskPtr is NULL !");
     le_result_t result = LE_NOT_PERMITTED;
     taf_locGnss_Client_t* clientRequestPtr = NULL;
     clientRequestPtr = AcquireSessionRef();

     TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    // Check the GNSS device state
    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        {
            //filling the supported bitmask values
            *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_GLONASS;
            *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_BEIDOU;
            *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_GALILEO;
            *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_SBAS;
            *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_QZSS;
            *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_NAVIC;
            result = LE_OK;
        }
        break;
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

static void StartSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_StartRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::StartWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();

    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    cPtr->retCode = LE_FAULT;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_ERROR("StartWorker: clientRequestPtr is NULL");
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, StartSvcRespond, cPtr, NULL);
        return;
    }

    LE_DEBUG("Start: gnssClientPtr %p, gnssPtr->sessionRef %p, num of active client %d",
            clientRequestPtr, clientRequestPtr->sessionRef, gnss.mClientRefCount);

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        {
            // Start GNSS
            if (!clientRequestPtr->mStarted)
            {
                int optInterval = clientRequestPtr->mAcqRate;
                LE_DEBUG("Start->  mAcqRate: %d",clientRequestPtr->mAcqRate);
                if( optInterval == 0  || optInterval < 100)
                {
                    LE_DEBUG("Start->mAcqRate is zero, so set default to 100ms");
                    optInterval = 100;
                    clientRequestPtr->mAcqRate = optInterval;
                }
                LocReqEngine engineType = DEFAULT_UNKNOWN;
                GnssReportTypeMask reportMask = DEFAULT_UNKNOWN;
                reportMask = 0x7f;//all reports are enabled
                LE_DEBUG("Start->reportMask : %u",reportMask);
                LE_DEBUG("Start->mEngineType : %d",clientRequestPtr->mEngineType);
                engineType |= (1UL << clientRequestPtr->mEngineType);//FUSED mode is supported by default

                typedef struct{
                    pa_result_t result;
                }taf_SelfTestResult_t;
                auto cb = [](pa_result_t result, std::any context) {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                };
                taf_SelfTestResult_t resCallback = {};
                pa_result_t res = taf_pa_location_startDetailedEngineReports(clientRequestPtr->locationClient,
                    (uint32_t)optInterval, engineType, cb, reportMask, (std::any)(std::any)&resCallback);
                if(res != PA_OK)
                {
                    cPtr->retCode = LE_FAULT;
                    LE_DEBUG("Start() commandResponse failed status: %d ", int(cPtr->retCode));
                }
                else
                {
                    if(resCallback.result == PA_OK)
                    {
                        ConfigureAcqStartInfo(clientRequestPtr);
                        cPtr->retCode = LE_OK;
                    }
                    else
                    {
                        cPtr->retCode = LE_FAULT;
                        LE_DEBUG("Start() commandResponse failed status: %d ", int(cPtr->retCode));
                    }
                }
            }
        }
        break;
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_DUPLICATE;
        }
        break;
        default:
        {
            cPtr->retCode = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }

    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, StartSvcRespond, cPtr, NULL);
}

void taf_locGnss::Start
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    
    le_msg_SessionRef_t sessionRef = taf_locGnss_GetClientSessionRef();
    taf_locGnss_Client_t* clientRequestPtr = AcquireSessionRef();
    if (clientRequestPtr == NULL)
    {
        LE_ERROR("Start: Failed to acquire session for sessionRef %p", sessionRef);
        taf_locGnss_StartRespond(cmdRef, LE_FAULT);
        return;
    }

    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();

    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, StartWorker, cPtr, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Respond trampoline - queued back to LocationSvc thread after StartWorker completes.
 */
//--------------------------------------------------------------------------------------------------
static void GnssStartInternalRespond(void* cmdPtr, void*)
{
    GnssInternalCmd_t* cPtr = (GnssInternalCmd_t*)cmdPtr;

    if (cPtr->completeCb)
    {
        cPtr->completeCb(cPtr->retCode, cPtr->contextPtr);
    }

    le_mem_Release(cPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Worker for internal GNSS start (reuses same PA logic as StartWorker).
 */
//--------------------------------------------------------------------------------------------------
static void GnssStartInternalWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    GnssInternalCmd_t* cPtr = (GnssInternalCmd_t*)cmdPtr;

    LE_ASSERT(cPtr != NULL);
    cPtr->retCode = LE_FAULT;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_ERROR("GnssStartInternalWorker: clientRequestPtr is NULL");
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef,
                                       GnssStartInternalRespond, cPtr, NULL);
        return;
    }

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        {
            if (!clientRequestPtr->mStarted)
            {
                int optInterval = clientRequestPtr->mAcqRate;
                if (optInterval == 0 || optInterval < 100)
                {
                    optInterval = 100;
                    clientRequestPtr->mAcqRate = optInterval;
                }

                LocReqEngine engineType = DEFAULT_UNKNOWN;
                GnssReportTypeMask reportMask = 0x7f;
                engineType |= (1UL << clientRequestPtr->mEngineType);

                typedef struct { pa_result_t result; } taf_SelfTestResult_t;
                auto cb = [](pa_result_t result, std::any context) {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                };
                taf_SelfTestResult_t resCallback = {};

                pa_result_t res = taf_pa_location_startDetailedEngineReports(
                    clientRequestPtr->locationClient,
                    (uint32_t)optInterval, engineType, cb, reportMask,
                    (std::any)&resCallback);

                cPtr->retCode = (res == PA_OK && resCallback.result == PA_OK) ? LE_OK : LE_FAULT;

                if (cPtr->retCode == LE_OK)
                {
                    gnss.ConfigureAcqStartInfo(clientRequestPtr);
                }
            }
            else
            {
                // Already started — treat as OK
                cPtr->retCode = LE_OK;
            }
        }
        break;

        case TAF_LOCGNSS_STATE_ACTIVE:
            cPtr->retCode = LE_DUPLICATE;
            break;

        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
            cPtr->retCode = LE_NOT_PERMITTED;
            break;

        default:
            cPtr->retCode = LE_FAULT;
            break;
    }

    le_event_QueueFunctionToThread(gnss.LocationSvcThRef,
                                   GnssStartInternalRespond, cPtr, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Start GNSS internally from locPos context (no TAF cmdRef needed).
 * Completion result is delivered via callback on the LocationSvc main thread.
 */
//--------------------------------------------------------------------------------------------------
void taf_locGnss::StartInternal
(
    le_msg_SessionRef_t           sessionRef,  ///< [IN] Client session ref.
    taf_locGnss_CompleteCb_t      completeCb,  ///< [IN] Completion callback.
    void*                         contextPtr   ///< [IN] Context passed to callback.
)
{
    auto &gnss = taf_locGnss::GetInstance();

    GnssInternalCmd_t* cPtr =
        (GnssInternalCmd_t*)le_mem_ForceAlloc(GnssInternalCmdPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));

    cPtr->sessionRef  = sessionRef;
    cPtr->completeCb  = completeCb;
    cPtr->contextPtr  = contextPtr;

    taf_locGnss_Client_t* clientRequestPtr = gnss.AcquireSessionRefInternal(sessionRef);

    if (clientRequestPtr == NULL)
    {
        LE_ERROR("StartInternal: Failed to acquire session for sessionRef %p",
                 sessionRef);
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef,
                                       GnssStartInternalRespond, cPtr, NULL);
        return;
    }

    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef,
                                   GnssStartInternalWorker, cPtr, NULL);
}

// ---------------------------------------------------------------------------
// SetMinElevation
// ---------------------------------------------------------------------------
static void SetMinElevationSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_SetMinElevationRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::SetMinElevationWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    uint8_t minElevation = cPtr->params.setMinElevation.minElevation;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetMinElevationSvcRespond, cPtr, NULL);
        return;
    }

    le_result_t result = LE_FAULT;
    if (minElevation > TAF_LOCGNSS_MIN_ELEVATION_MAX_DEGREE)
    {
        result = LE_OUT_OF_RANGE;
    }
    else
    {
        switch (clientRequestPtr->GnssState)
        {
            case TAF_LOCGNSS_STATE_ACTIVE:
            case TAF_LOCGNSS_STATE_DISABLED:
            case TAF_LOCGNSS_STATE_UNINITIALIZED:
                LE_ERROR("SetMinElevation: Wrong state [%d]", clientRequestPtr->GnssState);
                result = LE_NOT_PERMITTED;
                break;
            case TAF_LOCGNSS_STATE_READY:
            {
                typedef struct{ pa_result_t result; }taf_SelfTestResult_t;
                auto cb = [](pa_result_t r, std::any ctx) {
                    ((taf_SelfTestResult_t*)std::any_cast<taf_SelfTestResult_t*>(ctx))->result = r; };
                taf_SelfTestResult_t rc = {};
                pa_result_t res = taf_pa_location_configureMinSVElevation(minElevation, cb, (std::any)&rc);
                if (res != PA_OK) { result = LE_FAULT; }
                else if (rc.result == PA_OK) { result = LE_OK; gnss.mMinSvEle = minElevation; }
                else { result = LE_FAULT; }
                break;
            }
            default:
                LE_ERROR("SetMinElevation: Invalid state %d", clientRequestPtr->GnssState);
                result = LE_FAULT;
                break;
        }
    }
    cPtr->retCode = result;
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetMinElevationSvcRespond, cPtr, NULL);
}

void taf_locGnss::SetMinElevation(taf_locGnss_ServerCmdRef_t cmdRef, uint8_t minElevation)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    cPtr->params.setMinElevation.minElevation = minElevation;
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, SetMinElevationWorker, cPtr, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function starts the GNSS device in the specified start mode.
 *
 * @return
 *  - LE_OK              The function succeeded.
 *  - LE_BAD_PARAMETER   Invalid start mode
 *  - LE_FAULT           The function failed.
 *  - LE_DUPLICATE       If the GNSS device is already started.
 *  - LE_NOT_PERMITTED   If the GNSS device is not initialized or disabled.
 *
 * @warning This function may be subject to limitations depending on the platform. Please refer to
 *          the @ref platformConstraintsGnss page.
 */
//--------------------------------------------------------------------------------------------------
static void StartModeSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_StartModeRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::StartModeWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_StartMode_t mode = cPtr->params.startMode.mode;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, StartModeSvcRespond, cPtr, NULL);
        return;
    }

    le_result_t result = LE_OK;
    if (mode >= TAF_LOCGNSS_UNKNOWN_START) { result = LE_BAD_PARAMETER; }
    else
    {
        typedef struct{ pa_result_t result; }taf_SelfTestResult_t;
        switch (clientRequestPtr->GnssState)
        {
            case TAF_LOCGNSS_STATE_READY:
            {
                if (mode == TAF_LOCGNSS_HOT_START)
                {
                    LE_DEBUG("Hot Start – no aiding data deletion");
                }
                else if (mode == TAF_LOCGNSS_WARM_START)
                {
                    auto cb1 = [](pa_result_t r, std::any ctx) {
                        ((taf_SelfTestResult_t*)std::any_cast<taf_SelfTestResult_t*>(ctx))->result = r; };
                    taf_SelfTestResult_t rc1 = {};
                    uint32_t AidingData = TAF_LOCGNSS_AIDING_DATA_EPHEMERIS;
                    pa_result_t res1 = taf_pa_location_deleteAidingData(
                        (taf_pa_location_AidingDataType_t)AidingData, cb1, (std::any)&rc1);
                    if (res1 != PA_OK) { result = LE_FAULT; }
                    else { result = (le_result_t)rc1.result; }
                }
                else if ((mode == TAF_LOCGNSS_COLD_START) || (mode == TAF_LOCGNSS_FACTORY_START))
                {
                    auto cb2 = [](pa_result_t r, std::any ctx) {
                        ((taf_SelfTestResult_t*)std::any_cast<taf_SelfTestResult_t*>(ctx))->result = r; };
                    taf_SelfTestResult_t rc2 = {};
                    pa_result_t res2 = taf_pa_location_deleteAllAidingData(cb2, (std::any)&rc2);
                    if (res2 != PA_OK) { result = LE_FAULT; }
                    else { result = (le_result_t)rc2.result; }
                }
                else { result = LE_FAULT; }

                if (result == LE_OK && !clientRequestPtr->mStarted)
                {
                    int optInterval = clientRequestPtr->mAcqRate;
                    if (optInterval == 0 || optInterval < 100) { optInterval = 100; clientRequestPtr->mAcqRate = optInterval; }
                    LocReqEngine engineType = DEFAULT_UNKNOWN;
                    GnssReportTypeMask reportMask = 0x7f;
                    engineType |= (1UL << clientRequestPtr->mEngineType);
                    auto cb3 = [](pa_result_t r, std::any ctx) {
                        ((taf_SelfTestResult_t*)std::any_cast<taf_SelfTestResult_t*>(ctx))->result = r; };
                    taf_SelfTestResult_t rc3 = {};
                    pa_result_t res3 = taf_pa_location_startDetailedEngineReports(clientRequestPtr->locationClient,
                        (uint32_t)optInterval, engineType, cb3, reportMask, (std::any)&rc3);
                    if (res3 != PA_OK) { result = LE_FAULT; }
                    else if (rc3.result == PA_OK) { ConfigureAcqStartInfo(clientRequestPtr); }
                    else { result = LE_FAULT; }
                }
                break;
            }
            case TAF_LOCGNSS_STATE_UNINITIALIZED:
            case TAF_LOCGNSS_STATE_DISABLED:
                LE_ERROR("StartMode: Bad state [%d]", clientRequestPtr->GnssState);
                result = LE_NOT_PERMITTED;
                break;
            case TAF_LOCGNSS_STATE_ACTIVE:
                LE_ERROR("StartMode: Bad state [%d]", clientRequestPtr->GnssState);
                result = LE_DUPLICATE;
                break;
            default:
                LE_ERROR("StartMode: Unknown state %d", clientRequestPtr->GnssState);
                result = LE_FAULT;
                break;
        }
    }
    cPtr->retCode = result;
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, StartModeSvcRespond, cPtr, NULL);
}

void taf_locGnss::StartMode(taf_locGnss_ServerCmdRef_t cmdRef, taf_locGnss_StartMode_t mode)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    cPtr->params.startMode.mode = mode;
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, StartModeWorker, cPtr, NULL);
}

le_result_t taf_locGnss::GetMinElevation
(
   uint8_t*  minElevationPtr
)
{
    le_result_t result = LE_FAULT;
    TAF_KILL_CLIENT_IF_RET_VAL( NULL == minElevationPtr, LE_FAULT, "minElevationPtr is NULL");
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");
    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        {
            typedef struct{
                pa_result_t result;
                uint8_t minSVElevation;
            }taf_SelfTestResult_t;
            auto cb = [](pa_result_t result,
                uint8_t* minSVElevation_,std::any context) {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                    if(result == PA_OK) {
                        resPtr->minSVElevation = *minSVElevation_;
                    }
            };

            taf_SelfTestResult_t resCallback = {};
            pa_result_t res = taf_pa_location_requestMinSVElevation(cb,(std::any)&resCallback);

            if(res == PA_OK){
                if(resCallback.result == PA_OK)
                {
                    LE_DEBUG("requestMinSVElevation is Success");
                    result = LE_OK;
                    *minElevationPtr = resCallback.minSVElevation;
                }
                else
                {
                    LE_DEBUG("requestMinSVElevation is Failed");
                    result = LE_FAULT;
                }
            }
            else
            {
                result = LE_FAULT;
            }
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Invalid GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }

    return result;
}

le_result_t taf_locGnss::Disable
(
    void
)
{
    le_result_t result = LE_FAULT;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        {
                clientRequestPtr->GnssState = TAF_LOCGNSS_STATE_DISABLED;
                result = LE_OK;
        }
        break;
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_DUPLICATE;
        }
        break;
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Respond trampoline - queued back to LocationSvc thread after StopWorker completes.
 */
//--------------------------------------------------------------------------------------------------
static void GnssStopInternalRespond(void* cmdPtr, void*)
{
    GnssInternalCmd_t* cPtr = (GnssInternalCmd_t*)cmdPtr;

    if (cPtr->completeCb)
    {
        cPtr->completeCb(cPtr->retCode, cPtr->contextPtr);
    }

    le_mem_Release(cPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Worker for internal GNSS stop — reuses same PA logic as StopWorker.
 */
//--------------------------------------------------------------------------------------------------
static void GnssStopInternalWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    GnssInternalCmd_t* cPtr = (GnssInternalCmd_t*)cmdPtr;

    LE_ASSERT(cPtr != NULL);
    cPtr->retCode = LE_FAULT;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_ERROR("GnssStopInternalWorker: clientRequestPtr is NULL");
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, GnssStopInternalRespond, cPtr, NULL);
        return;
    }

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            if (clientRequestPtr->mStarted)
            {
                typedef struct { pa_result_t result; } taf_SelfTestResult_t;
                auto cb = [](pa_result_t result, std::any context)
                {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                };
                taf_SelfTestResult_t resCallback = {};
                pa_result_t res = taf_pa_location_stopReports(
                    clientRequestPtr->locationClient, cb, (std::any)&resCallback);

                if (res != PA_OK)
                {
                    cPtr->retCode = LE_FAULT;
                }
                else
                {
                    if (resCallback.result == PA_OK)
                    {
                        clientRequestPtr->mStarted = false;
                        clientRequestPtr->GnssState = TAF_LOCGNSS_STATE_READY;
                        gnss.mNmeaMask = 0;
                        cPtr->retCode = LE_OK;
                        LE_DEBUG("GnssStopInternalWorker: Stop success");
                    }
                    else
                    {
                        cPtr->retCode = LE_FAULT;
                        LE_DEBUG("GnssStopInternalWorker: Stop failed");
                    }
                }
            }
        }
        break;

        case TAF_LOCGNSS_STATE_READY:
            LE_ERROR("GnssStopInternalWorker: Bad state [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_DUPLICATE;
            break;

        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
            LE_ERROR("GnssStopInternalWorker: Bad state [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
            break;

        default:
            cPtr->retCode = LE_FAULT;
            LE_ERROR("GnssStopInternalWorker: Unknown GNSS state %d", clientRequestPtr->GnssState);
            break;
    }

    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, GnssStopInternalRespond, cPtr, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal GNSS Stop entry point — called by locPos without a gnss cmdRef.
 * Queues work to LocationWorkerThRef and invokes completeCb on LocationSvcThRef when done.
 */
//--------------------------------------------------------------------------------------------------
void taf_locGnss::StopInternal
(
    le_msg_SessionRef_t            sessionRef,
    taf_locGnss_CompleteCb_t       completeCb,
    void*                          contextPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    GnssInternalCmd_t* cPtr = (GnssInternalCmd_t*)le_mem_ForceAlloc(GnssInternalCmdPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->sessionRef = sessionRef;
    cPtr->completeCb = completeCb;
    cPtr->contextPtr = contextPtr;

    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, GnssStopInternalWorker, cPtr, NULL);
}

static void StopSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_StopRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::StopWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();

    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    cPtr->retCode = LE_FAULT;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, StopSvcRespond, cPtr, NULL);
        return;
    }

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_ACTIVE:
            {
                LE_DEBUG("Stop() is started: %d", (int) clientRequestPtr->mStarted);

                if (clientRequestPtr->mStarted)
                {
                    typedef struct{
                        pa_result_t result;
                    }taf_SelfTestResult_t;
                    auto cb = [](pa_result_t result, std::any context) {
                        taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                        resPtr->result = result;
                    };
                    taf_SelfTestResult_t resCallback = {};
                    pa_result_t res = taf_pa_location_stopReports(clientRequestPtr->locationClient, cb,(std::any)&resCallback);
                    if(res != PA_OK)
                    {
                        cPtr->retCode = LE_FAULT;
                    }
                    else
                    {
                        if(resCallback.result == PA_OK)
                        {
                            clientRequestPtr->mStarted = false;
                            clientRequestPtr->GnssState = TAF_LOCGNSS_STATE_READY;
                            cPtr->retCode = LE_OK;
                            gnss.mNmeaMask = 0;//reset nmeaMask on triggering stop.
                            LE_DEBUG("Stop() is success");
                        }
                        else
                        {
                            cPtr->retCode = LE_FAULT;
                            LE_DEBUG("Stop() is failed");
                        }
                    }
                }
            }
        break;
        case TAF_LOCGNSS_STATE_READY:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_DUPLICATE;
        }
        break;
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            cPtr->retCode = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }

    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, StopSvcRespond, cPtr, NULL);
}

void taf_locGnss::Stop
(
    taf_locGnss_ServerCmdRef_t cmdRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();

    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, StopWorker, cPtr, NULL);
}

le_result_t taf_locGnss::SetNmeaSentencesInternal
(
    taf_locGnss_NmeaBitMask_t nmeaMask
)
{
    auto &gnss = taf_locGnss::GetInstance();
    if (nmeaMask == 0)
    {
        LE_ERROR("SetNmeaSentencesInternal: invalid nmeaMask=0");
        return LE_BAD_PARAMETER;
    }

    if ((nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPGSA) ||
        (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GAGGA) ||
        (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GAGSA) ||
        (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GARMC) ||
        (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GAVTG) ||
        (nmeaMask & TAF_LOCGNSS_NMEA_MASK_PSTIS) ||
        (nmeaMask & TAF_LOCGNSS_NMEA_MASK_REMOVED) ||
        (nmeaMask & TAF_LOCGNSS_NMEA_MASK_PTYPE) ||
        (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPGRS) ||
        (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPGLL) ||
        (nmeaMask & TAF_LOCGNSS_NMEA_MASK_DEBUG) ||
        (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GAGNS) ||
        (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GNGNS) ||
        (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPGST) ||
        (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPZDA))
    {
        LE_ERROR("SetNmeaSentencesInternal: unsupported NMEA mask bits set");
        return LE_FAULT;
    }

    uint32_t nmeaType = 0;

    if ((nmeaMask & TAF_LOCGNSS_NMEA_MASK_GGA) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPGGA))
        nmeaType |= TAF_PA_LOCATION_GGA;
    if ((nmeaMask & TAF_LOCGNSS_NMEA_MASK_RMC) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPRMC))
        nmeaType |= TAF_PA_LOCATION_RMC;
    if ((nmeaMask & TAF_LOCGNSS_NMEA_MASK_GSA) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GNGSA))
        nmeaType |= TAF_PA_LOCATION_GSA;
    if ((nmeaMask & TAF_LOCGNSS_NMEA_MASK_VTG) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPVTG))
        nmeaType |= TAF_PA_LOCATION_VTG;
    if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GNS)
        nmeaType |= TAF_PA_LOCATION_GNS;
    if ((nmeaMask & TAF_LOCGNSS_NMEA_MASK_DTM) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPDTM))
        nmeaType |= TAF_PA_LOCATION_DTM;
    if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPGSV)
        nmeaType |= TAF_PA_LOCATION_GPGSV;
    if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GLGSV)
        nmeaType |= TAF_PA_LOCATION_GLGSV;
    if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GAGSV)
        nmeaType |= TAF_PA_LOCATION_GAGSV;
    if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GQGSV)
        nmeaType |= TAF_PA_LOCATION_GQGSV;
    if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GBGSV)
        nmeaType |= TAF_PA_LOCATION_GBGSV;
    if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GIGSV)
        nmeaType |= TAF_PA_LOCATION_GIGSV;

    typedef struct { pa_result_t result; } taf_SelfTestResult_t;
    auto cb = [](pa_result_t result, std::any context)
    {
        taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
        resPtr->result = result;
    };
    taf_SelfTestResult_t resCallback = {};

    pa_result_t res = taf_pa_location_configureNmeaTypes(
        (taf_pa_location_NmeaSentenceType_t)nmeaType, cb, (std::any)&resCallback);

    if (res != PA_OK || resCallback.result != PA_OK)
    {
        LE_ERROR("SetNmeaSentencesInternal: PA call failed res=%d cbResult=%d",
                 (int)res, (int)resCallback.result);
        return LE_FAULT;
    }
    else
    {
        gnss.SetNmeaConfig(nmeaMask);
    }

    LE_INFO("SetNmeaSentencesInternal: NMEA configured successfully nmeaMask=%" PRIu64, nmeaMask);
    return LE_OK;
}

// ---------------------------------------------------------------------------
// SetNmeaSentences
// ---------------------------------------------------------------------------
static void SetNmeaSentencesSvcRespond(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    if (cPtr->retCode == LE_OK)
    {
        gnss.SetNmeaConfig(cPtr->params.setNmeaSentences.nmeaMask);
    }
    taf_locGnss_SetNmeaSentencesRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::SetNmeaSentencesWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_NmeaBitMask_t nmeaMask = cPtr->params.setNmeaSentences.nmeaMask;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetNmeaSentencesSvcRespond, cPtr, NULL);
        return;
    }

    if (nmeaMask == 0)
    {
        LE_ERROR("Unable to set the enabled NMEA sentences, wrong bit mask %" PRIu64"", nmeaMask);
        cPtr->retCode = LE_BAD_PARAMETER;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetNmeaSentencesSvcRespond, cPtr, NULL);
        return;
    }
    else
    {
        // Check the GNSS device state
        switch (clientRequestPtr->GnssState)
        {
            case TAF_LOCGNSS_STATE_READY:
            case TAF_LOCGNSS_STATE_ACTIVE:
            {
                // Set the enabled NMEA sentences
                if ((nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPGSA) ||
                    (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GAGGA) ||
                    (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GAGSA) ||
                    (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GARMC) ||
                    (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GAVTG) ||
                    (nmeaMask & TAF_LOCGNSS_NMEA_MASK_PSTIS) ||
                    (nmeaMask & TAF_LOCGNSS_NMEA_MASK_REMOVED)||
                    (nmeaMask & TAF_LOCGNSS_NMEA_MASK_PTYPE) ||
                    (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPGRS) ||
                    (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPGLL) ||
                    (nmeaMask & TAF_LOCGNSS_NMEA_MASK_DEBUG) ||
                    (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GAGNS) ||
                    (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GNGNS) ||
                    (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPGST) ||
                    (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPZDA))
                {
                    cPtr->retCode = LE_FAULT;
                    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetNmeaSentencesSvcRespond, cPtr, NULL);
                    return;
                }
                uint32_t nmeaType = 0;
                if((nmeaMask & TAF_LOCGNSS_NMEA_MASK_GGA) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPGGA))
                {
                    nmeaType |= TAF_PA_LOCATION_GGA;
                }
                if((nmeaMask & TAF_LOCGNSS_NMEA_MASK_RMC) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPRMC))
                {
                    nmeaType |= TAF_PA_LOCATION_RMC;
                }
                if((nmeaMask & TAF_LOCGNSS_NMEA_MASK_GSA) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GNGSA))
                {
                    nmeaType |= TAF_PA_LOCATION_GSA;
                }
                if((nmeaMask & TAF_LOCGNSS_NMEA_MASK_VTG) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPVTG))
                {
                    nmeaType |= TAF_PA_LOCATION_VTG;
                }
                if(nmeaMask & TAF_LOCGNSS_NMEA_MASK_GNS)
                {
                    nmeaType |= TAF_PA_LOCATION_GNS;
                }
                if((nmeaMask & TAF_LOCGNSS_NMEA_MASK_DTM) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPDTM))
                {
                    nmeaType |= TAF_PA_LOCATION_DTM;
                }
                if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPGSV)
                {
                    nmeaType |= TAF_PA_LOCATION_GPGSV;
                }
                if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GLGSV)
                {
                    nmeaType |= TAF_PA_LOCATION_GLGSV;
                }
                if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GAGSV)
                {
                    nmeaType |= TAF_PA_LOCATION_GAGSV;
                }
                if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GQGSV)
                {
                    nmeaType |= TAF_PA_LOCATION_GQGSV;
                }
                if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GBGSV)
                {
                    nmeaType |= TAF_PA_LOCATION_GBGSV;
                }
                if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GIGSV)
                {
                    nmeaType |= TAF_PA_LOCATION_GIGSV;
                }

                typedef struct{
                    pa_result_t result;
                }taf_SelfTestResult_t;
                auto cb1 = [](pa_result_t result, std::any context) {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                };
                taf_SelfTestResult_t resCallback = {};
                pa_result_t res = taf_pa_location_configureNmeaTypes((taf_pa_location_NmeaSentenceType_t)nmeaType,cb1,(std::any)&resCallback);
                if(res == PA_OK){
                    if(resCallback.result == PA_OK)
                    {
                        cPtr->retCode = LE_OK;
                    }
                    else
                    {
                        cPtr->retCode = LE_FAULT;
                    }
                }
                else
                {
                    LE_DEBUG("SetNmeaSentences is failed");
                    cPtr->retCode = LE_FAULT;
                }
                if (LE_OK != cPtr->retCode)
                {
                    LE_ERROR("Unable to set the enabled NMEA sentences, error = %d (%s)",
                              cPtr->retCode, LE_RESULT_TXT(cPtr->retCode));
                }
            }
            break;
            case TAF_LOCGNSS_STATE_UNINITIALIZED:
            case TAF_LOCGNSS_STATE_DISABLED:
            {
                LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
                cPtr->retCode = LE_NOT_PERMITTED;
            }
            break;
            default:
            {
                LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
                cPtr->retCode = LE_FAULT;
            }
            break;
        }
    }

    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetNmeaSentencesSvcRespond, cPtr, NULL);
}

void taf_locGnss::SetNmeaSentences(taf_locGnss_ServerCmdRef_t cmdRef,
                                    taf_locGnss_NmeaBitMask_t nmeaMask)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    cPtr->params.setNmeaSentences.nmeaMask = nmeaMask;
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, SetNmeaSentencesWorker, cPtr, NULL);
}

le_result_t taf_locGnss::GetNmeaSentences
(
    taf_locGnss_NmeaBitMask_t* nmeaMaskPtr
)
{

    if (NULL == nmeaMaskPtr)
    {
        LE_KILL_CLIENT("nmeaMaskPtr is NULL !");
        return LE_FAULT;
    }

    le_result_t result = LE_NOT_PERMITTED;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    // Check the GNSS device state
    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            // Get the enabled NMEA sentences
            mNmeaMask = GetNmeaConfig();
            LE_DEBUG("the NMEA retrieved from the config is : %" PRIu64"",mNmeaMask);
            if(mNmeaMask != 0)
            {
                *nmeaMaskPtr = mNmeaMask;
                result = LE_OK;
            }
            else
            {
                result = LE_FAULT;
            }
            if (LE_OK != result)
            {
                *nmeaMaskPtr = 0;
                LE_ERROR("NmeaSentence type is invalid");
                result = LE_FAULT;
                LE_ERROR("Unable to get the enabled NMEA sentences, error = %d (%s)",
                          result, LE_RESULT_TXT(result));
            }
        }
        break;
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

le_result_t taf_locGnss::GetSupportedNmeaSentences
(
    taf_locGnss_NmeaBitMask_t* nmeaMaskPtr
)
{

    TAF_ERROR_IF_RET_VAL( nmeaMaskPtr == NULL, LE_FAULT, "nmeaMaskPtr is NULL !");
    le_result_t result = LE_NOT_PERMITTED;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    // Check the GNSS device state
    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            //filling the supported bitmask values
            *nmeaMaskPtr |= TAF_LOCGNSS_NMEA_MASK_GPGSV;
            *nmeaMaskPtr |= TAF_LOCGNSS_NMEA_MASK_GLGSV;
            *nmeaMaskPtr |= TAF_LOCGNSS_NMEA_MASK_GAGSV;
            *nmeaMaskPtr |= TAF_LOCGNSS_NMEA_MASK_GQGSV;
            *nmeaMaskPtr |= TAF_LOCGNSS_NMEA_MASK_GBGSV;
            *nmeaMaskPtr |= TAF_LOCGNSS_NMEA_MASK_GIGSV;
            *nmeaMaskPtr |= TAF_LOCGNSS_NMEA_MASK_GGA;
            *nmeaMaskPtr |= TAF_LOCGNSS_NMEA_MASK_RMC;
            *nmeaMaskPtr |= TAF_LOCGNSS_NMEA_MASK_GSA;
            *nmeaMaskPtr |= TAF_LOCGNSS_NMEA_MASK_VTG;
            *nmeaMaskPtr |= TAF_LOCGNSS_NMEA_MASK_GNS;
            *nmeaMaskPtr |= TAF_LOCGNSS_NMEA_MASK_DTM;
            result = LE_OK;
        }
        break;
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

le_result_t taf_locGnss::SetNmeaConfig(const taf_locGnss_NmeaBitMask_t nmeaMask)
{
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn("tafLocationSvc:/");

    char config_node_nmea_lsb[LENGTH_CFG_NODE] = {};
    char config_node_nmea_msb[LENGTH_CFG_NODE] = {};
    snprintf(config_node_nmea_lsb, sizeof(config_node_nmea_lsb), "%s", "nmeaSentences_lsb");
    le_cfg_SetInt(iteratorRef, "nmea_lsb", nmeaMask);//store LSB 32 bits

    taf_locGnss_NmeaBitMask_t nmea_msb = (taf_locGnss_NmeaBitMask_t) nmeaMask >>32;//store 32 bits(MSB) out of 64 bits
    snprintf(config_node_nmea_msb, sizeof(config_node_nmea_msb), "%s", "nmeaSentences_msb");
    le_cfg_SetInt(iteratorRef, "nmea_msb", nmea_msb);
    le_cfg_CommitTxn(iteratorRef);

    LE_DEBUG("Set NMEA LSB config node%s as : %" PRIu64"",config_node_nmea_lsb,(nmeaMask &0xffffffff));
    LE_DEBUG("Set NMEA MSB config node%s as : %" PRIu64"",config_node_nmea_msb,nmea_msb);

    return LE_OK;
}

taf_locGnss_NmeaBitMask_t taf_locGnss::GetNmeaConfig()
{
    taf_locGnss_NmeaBitMask_t nmea = 0;
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn("tafLocationSvc:/");

    if (le_cfg_NodeExists(iteratorRef, "nmea_lsb"))
    {
        uint32_t configNmea = le_cfg_GetInt(iteratorRef,
                                       "nmea_lsb", 0);
        nmea = (taf_locGnss_NmeaBitMask_t)configNmea;//fetch LSB 32 bits

        LE_DEBUG("Get NMEA LSB config node%s as : %" PRIu64"","nmea_lsb",nmea);
    }
    if (le_cfg_NodeExists(iteratorRef, "nmea_msb"))
    {
        uint64_t configNmea = le_cfg_GetInt(iteratorRef,
                                       "nmea_msb", 0);
        nmea = nmea |(taf_locGnss_NmeaBitMask_t)configNmea <<32;//MSB 32 bits and LSB 32 bits

        LE_DEBUG("Get NMEA MSB config node%s as : %" PRIu64"","nmea_msb",(taf_locGnss_NmeaBitMask_t)configNmea);
        le_cfg_CancelTxn(iteratorRef);
        return nmea;
    }
    LE_WARN("config node %s doesn't exist", "nmea LSB and MSB");

    le_cfg_CancelTxn(iteratorRef);
    return 0;
}

// ---------------------------------------------------------------------------
// SetDRConfig
// ---------------------------------------------------------------------------
static void SetDRConfigSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_SetDRConfigRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::SetDRConfigWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetDRConfigSvcRespond, cPtr, NULL);
        return;
    }

    le_result_t result = LE_NOT_PERMITTED;
    le_result_t sensor_Result = LE_OK;
    le_result_t speedScale_Result = LE_OK;
    le_result_t gyroScale_Result = LE_OK;
    taf_pa_location_DREngineConfiguration_t drConfig;
    drConfig.validMask = static_cast<uint16_t>(0);

    taf_locGnss_DrParams_t* dRParamsPtr = &cPtr->params.setDRConfig.drParams;

    // Check the GNSS device state
    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        {
            //Filling the DR parameters
            bodyToSensorUtility(drConfig,dRParamsPtr,clientRequestPtr,&sensor_Result);
            if(sensor_Result == LE_OUT_OF_RANGE)
            {
                cPtr->retCode = sensor_Result;
                le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetDRConfigSvcRespond, cPtr, NULL);
                return;
            }
            speedScaleUtility(drConfig,dRParamsPtr,clientRequestPtr,&speedScale_Result);
            if(speedScale_Result == LE_OUT_OF_RANGE)
            {
                cPtr->retCode = speedScale_Result;
                le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetDRConfigSvcRespond, cPtr, NULL);
                return;
            }
            gyroScaleUtility(drConfig,dRParamsPtr,clientRequestPtr,&gyroScale_Result);
            if(gyroScale_Result == LE_OUT_OF_RANGE)
            {
                cPtr->retCode = gyroScale_Result;
                le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetDRConfigSvcRespond, cPtr, NULL);
                return;
            }
            typedef struct{
                pa_result_t result;
            }taf_SelfTestResult_t;
            auto cb = [](pa_result_t result, std::any context) {
                taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                resPtr->result = result;
            };
            taf_SelfTestResult_t resCallback = {};
            pa_result_t res = taf_pa_location_configureDR(drConfig, cb,(std::any)&resCallback);
            if (res == PA_FAULT) {
                LE_DEBUG("SetDRConfig is failed");
                cPtr->retCode = LE_FAULT;
            } else if (res == PA_OK) {
                result = (le_result_t)resCallback.result;
                if(result == LE_OK)
                {
                    LE_DEBUG("SetDRConfig is Success");
                }
            }
            if (LE_OK != result)
            {
                LE_ERROR("Unable to set the DR Configuration , error = %d (%s)",
                          result, LE_RESULT_TXT(result));
            }
        }
        break;
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
            cPtr->retCode = LE_FAULT;
        }
        break;
    }

    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetDRConfigSvcRespond, cPtr, NULL);
}

void taf_locGnss::SetDRConfig(taf_locGnss_ServerCmdRef_t cmdRef,
                               const taf_locGnss_DrParams_t* drParamsPtr)
{
    auto &gnss = taf_locGnss::GetInstance();
    if (drParamsPtr == NULL)
    {
        LE_ERROR("SetDRConfig: drParamsPtr is NULL");
        taf_locGnss_SetDRConfigRespond(cmdRef, LE_FAULT);
        return;
    }
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    cPtr->params.setDRConfig.drParams = *drParamsPtr;
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, SetDRConfigWorker, cPtr, NULL);
}

void bodyToSensorUtility(taf_pa_location_DREngineConfiguration_t& drConfig,
        const taf_locGnss_DrParams_t* drParamsPtr,taf_locGnss_Client_t* clientRequestPtr,
        le_result_t* sensor_Result)
{
    if(clientRequestPtr->drParamsMask & TAF_LOCGNSS_BODY_TO_SENSOR_MOUNT_PARAMS_VALID)
    {
        drConfig.validMask |= TAF_PA_LOCATION_BODY_TO_SENSOR_MOUNT_PARAMS_VALID;
        if((float) (drParamsPtr->rollOffset >=-180.0) && (float) (drParamsPtr->rollOffset <=180.0))
        {
            drConfig.mountParam.rollOffset =(float) drParamsPtr->rollOffset;
        }
        else
        {
            *sensor_Result = LE_OUT_OF_RANGE;
             LE_ERROR("SetDRConfig -> rollOffSet out of range");
        }
        if((float) (drParamsPtr->yawOffset >=-180.0) && (float) (drParamsPtr->yawOffset <=180.0))
        {
            drConfig.mountParam.yawOffset = (float) drParamsPtr->yawOffset;
        }
        else
        {
            *sensor_Result = LE_OUT_OF_RANGE;
             LE_ERROR("SetDRConfig -> yawOffset out of range");
        }
        if((float) (drParamsPtr->pitchOffset >=-180.0) && (float) (drParamsPtr->pitchOffset <=180.0))
        {
            drConfig.mountParam.pitchOffset = (float) drParamsPtr->pitchOffset;
        }
        else
        {
            *sensor_Result = LE_OUT_OF_RANGE;
             LE_ERROR("SetDRConfig -> pitchOffset out of range");
        }
        if((float) (drParamsPtr->offsetUnc >=-180.0) && (float) (drParamsPtr->offsetUnc <=180.0))
        {
            drConfig.mountParam.offsetUnc = (float) drParamsPtr->offsetUnc;
        }
        else
        {
            *sensor_Result = LE_OUT_OF_RANGE;
             LE_ERROR("SetDRConfig -> offsetUnc out of range");
        }
    }
    else
    {
        LE_ERROR("SetDRConfig invalidMask body to sensor mount params");
    }
}

void speedScaleUtility(taf_pa_location_DREngineConfiguration_t& drConfig,
        const taf_locGnss_DrParams_t* drParamsPtr,taf_locGnss_Client_t* clientRequestPtr,
        le_result_t* speedScale_Result)
{
    if(clientRequestPtr->drParamsMask & TAF_LOCGNSS_VEHICLE_SPEED_SCALE_FACTOR_VALID)
    {
        drConfig.validMask |= TAF_PA_LOCATION_VEHICLE_SPEED_SCALE_FACTOR_VALID;
        if((float)(drParamsPtr->speedFactor>=0.9) && (float)(drParamsPtr->speedFactor<=1.1))
        {
            drConfig.speedFactor = (float) drParamsPtr->speedFactor;
        }
        else
        {
            *speedScale_Result = LE_OUT_OF_RANGE;
            LE_DEBUG("SetDRConfig -> speed scale factor out of range");
        }
    }
    else
    {
        LE_DEBUG("SetDRConfig invalidMask vechile speed scale factor");
    }
    if(clientRequestPtr->drParamsMask & TAF_LOCGNSS_VEHICLE_SPEED_SCALE_FACTOR_UNC_VALID)
    {
        drConfig.validMask |= TAF_PA_LOCATION_VEHICLE_SPEED_SCALE_FACTOR_UNC_VALID;
        if((float)(drParamsPtr->speedFactorUnc>=0.0) && (float)(drParamsPtr->speedFactorUnc<=0.1))
        {
            drConfig.speedFactorUnc = (float) drParamsPtr->speedFactorUnc;
        }
        else
        {
            *speedScale_Result = LE_OUT_OF_RANGE;
            LE_DEBUG("SetDRConfig -> speed scale unc factor out of range");
        }
    }
    else
    {
        LE_DEBUG("SetDRConfig invalidMask vechile speed scale unc factor");
    }
}

void gyroScaleUtility(taf_pa_location_DREngineConfiguration_t& drConfig,
        const taf_locGnss_DrParams_t* drParamsPtr,taf_locGnss_Client_t* clientRequestPtr,
        le_result_t* gyroScale_Result)
{
    if(clientRequestPtr->drParamsMask & TAF_LOCGNSS_GYRO_SCALE_FACTOR_VALID)
    {
        drConfig.validMask |= TAF_PA_LOCATION_GYRO_SCALE_FACTOR_VALID;
        if((float)(drParamsPtr->gyroFactor>=0.9) && (float)(drParamsPtr->gyroFactor<=1.1))
        {
            drConfig.gyroFactor = (float) drParamsPtr->gyroFactor;
        }
        else
        {
            *gyroScale_Result = LE_OUT_OF_RANGE;
            LE_DEBUG("SetDRConfig -> gyro factor out of range");
        }
    }
    else
    {
         LE_DEBUG("SetDRConfig InvalidMask gyro scale factor");
    }
    if(clientRequestPtr->drParamsMask & TAF_LOCGNSS_GYRO_SCALE_FACTOR_UNC_VALID)
    {
        drConfig.validMask |= TAF_PA_LOCATION_GYRO_SCALE_FACTOR_UNC_VALID;
        if((float)(drParamsPtr->gyroFactorUnc>=0.0) && (float)(drParamsPtr->gyroFactorUnc<=0.1))
        {
            drConfig.gyroFactorUnc = (float) drParamsPtr->gyroFactorUnc;
        }
        else
        {
            *gyroScale_Result = LE_OUT_OF_RANGE;
            LE_DEBUG("SetDRConfig -> gyro factor unc out of range");
        }
    }
    else
    {
         LE_DEBUG("SetDRConfig InvalidMask gyro scale unc factor");
    }
}

void roundOffLocationData(double *locData, uint8_t dplace)
{
    double decimal = pow(10,(dplace + 1));
    if(*locData <0)
    {
        *locData = ((*locData*decimal)-5)/10;
    }
    else
    {
        *locData = ((*locData*decimal)+5)/10;
    }
}

// ---------------------------------------------------------------------------
// ConfigureEngineState  (SA515M / SA525M only)
// ---------------------------------------------------------------------------
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
static void ConfigureEngineStateSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_ConfigureEngineStateRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::ConfigureEngineStateWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_EngineType_t  engType  = cPtr->params.configureEngineState.engType;
    taf_locGnss_EngineState_t engineState = cPtr->params.configureEngineState.engState;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ConfigureEngineStateSvcRespond, cPtr, NULL);
        return;
    }

    cPtr->retCode = LE_OK;
    taf_pa_location_EngineType_t paEngineType;
    taf_pa_location_LocationEngineRunState_t paEngineState;
    LE_DEBUG("ConfigureEngineState engtype:%d, engState:%d", engType, engineState);
    switch(engType)
    {
        case TAF_LOCGNSS_ENGINE_TYPE_DRE:
            paEngineType = TAF_PA_LOCATION_DRE;
            break;
        default:
        {
            LE_ERROR("Unknown Engine type %d", engType);
            cPtr->retCode = LE_FAULT;
            le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ConfigureEngineStateSvcRespond, cPtr, NULL);
            return;
        }
    }
    switch(engineState)
    {
        case TAF_LOCGNSS_ENGINE_STATE_SUSPENDED:
            paEngineState = TAF_PA_LOCATION_SUSPENDED;
            break;
        case TAF_LOCGNSS_ENGINE_STATE_RUNNING:
            paEngineState = TAF_PA_LOCATION_RUNNING;
            break;
        default:
        {
            LE_ERROR("Unknown Engine state %d", engineState);
            cPtr->retCode = LE_FAULT;
            le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ConfigureEngineStateSvcRespond, cPtr, NULL);
            return;
        }
    }
    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
            {
                typedef struct{
                    pa_result_t result;
                }taf_SelfTestResult_t;
                auto cb = [](pa_result_t result, std::any context) {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                };
                taf_SelfTestResult_t resCallback = {};
                pa_result_t res = taf_pa_location_configureEngineState(
                    paEngineType, paEngineState, cb,(std::any)&resCallback);
                if(res != PA_OK){
                    cPtr->retCode = LE_FAULT;
                }
                else
                {
                    if(resCallback.result == PA_OK)
                    {
                        LE_DEBUG("ConfigureEngineState succeed.");
                    }
                    else
                    {
                        cPtr->retCode = LE_FAULT;
                    }
                }
            }
        break;
        default:
        {
            LE_ERROR("Invalid GNSS state %d", clientRequestPtr->GnssState);
            cPtr->retCode = LE_FAULT;
        }
        break;
    }
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ConfigureEngineStateSvcRespond, cPtr, NULL);
}

void taf_locGnss::ConfigureEngineState(taf_locGnss_ServerCmdRef_t cmdRef,
                                        taf_locGnss_EngineType_t engtype,
                                        taf_locGnss_EngineState_t engState)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    cPtr->params.configureEngineState.engType  = engtype;
    cPtr->params.configureEngineState.engState = engState;
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, ConfigureEngineStateWorker, cPtr, NULL);
}
#endif

// ---------------------------------------------------------------------------
// ConfigureRobustLocation
// ---------------------------------------------------------------------------
static void ConfigureRobustLocationSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_ConfigureRobustLocationRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::ConfigureRobustLocationWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    uint8_t enable    = cPtr->params.configureRobustLocation.enable;
    uint8_t enabled911 = cPtr->params.configureRobustLocation.enabled911;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ConfigureRobustLocationSvcRespond, cPtr, NULL);
        return;
    }

    cPtr->retCode = LE_OK;
    LE_DEBUG("ConfigureRobustLocation enable:%d, enabled911:%d", enable, enabled911);
    bool enableRobustloc = false;
    bool enableE911loc = false;

    switch(enable)
    {
       case 0:
            enableRobustloc = false;
            break;
       case 1:
            enableRobustloc = true;
            break;
        default:
        {
            LE_ERROR("Wrong Input to enable: %d", enable);
            cPtr->retCode = LE_FAULT;
            le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ConfigureRobustLocationSvcRespond, cPtr, NULL);
            return;
        }
    }
    switch(enabled911)
    {
       case 0:
            enableE911loc = false;
            break;
       case 1:
            enableE911loc = true;
            break;
        default:
        {
            LE_ERROR("Wrong Input to enabled911: %d", enabled911);
            cPtr->retCode = LE_FAULT;
            le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ConfigureRobustLocationSvcRespond, cPtr, NULL);
            return;
        }
    }
    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
            {
                typedef struct{
                    pa_result_t result;
                }taf_SelfTestResult_t;
                auto cb = [](pa_result_t result, std::any context) {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                };
                taf_SelfTestResult_t resCallback = {};
                pa_result_t res = taf_pa_location_configureRobustLocation(
                    enableRobustloc, enableE911loc, cb,(std::any)&resCallback);
                if(res != PA_OK){
                    cPtr->retCode = LE_FAULT;
                }
                else
                {
                    cPtr->retCode = (le_result_t)resCallback.result;
                }
            }
        break;
        default:
        {
            LE_ERROR("Invalid GNSS state %d", clientRequestPtr->GnssState);
            cPtr->retCode = LE_FAULT;
        }
        break;
    }
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ConfigureRobustLocationSvcRespond, cPtr, NULL);
}

void taf_locGnss::ConfigureRobustLocation(taf_locGnss_ServerCmdRef_t cmdRef,
                                           uint8_t enable, uint8_t enabled911)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    cPtr->params.configureRobustLocation.enable     = enable;
    cPtr->params.configureRobustLocation.enabled911 = enabled911;
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, ConfigureRobustLocationWorker, cPtr, NULL);
}

le_result_t taf_locGnss::RobustLocationInformation
(
    uint8_t* enable,///< [OUT] value for enable/disable.
    uint8_t* enabled911,///< [OUT] value for 911 enable/disable
    uint8_t* majorVersion,///< [OUT] value for majorVersion number
    uint8_t* minorVersion ///< [OUT] value for majorVersion number
)
{
    le_result_t result = LE_OK;
    if((enable == NULL) || (enabled911 == NULL)
        || (majorVersion == NULL) || (minorVersion == NULL))
    {
        return LE_FAULT;
    }
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");
    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
            {
                typedef struct{
                    pa_result_t result;
                    taf_pa_location_RobustLocationConfiguration_t robustLocationData;
                }taf_SelfTestResult_t;
                auto cb = [](pa_result_t result,
                    taf_pa_location_RobustLocationConfiguration_t robLocConfig_,std::any context) {
                        taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                        resPtr->result = result;
                        if(result == PA_OK) {
                            if(robLocConfig_.validMask & TAF_PA_LOCATION_VALID_ENABLED)
                            {
                                resPtr->robustLocationData.enabled = robLocConfig_.enabled;
                            }
                            if(robLocConfig_.validMask & TAF_PA_LOCATION_VALID_ENABLED_FOR_E911)
                            {
                                resPtr->robustLocationData.enabledForE911 = robLocConfig_.enabledForE911;
                            }
                            if(robLocConfig_.validMask & TAF_PA_LOCATION_VALID_VERSION)
                            {
                                resPtr->robustLocationData.version.major = uint8_t (robLocConfig_.version.major);
                                resPtr->robustLocationData.version.minor = uint16_t (robLocConfig_.version.minor);
                            }
                            LE_DEBUG("***onRobustLocationInfo gnss.enable: %d", resPtr->robustLocationData.enabled);
                            LE_DEBUG("***onRobustLocationInfo gnss.enabled911: %d", resPtr->robustLocationData.enabledForE911);
                            LE_DEBUG("***onRobustLocationInfo gnss.majorVersion: %d", resPtr->robustLocationData.version.major);
                            LE_DEBUG("***onRobustLocationInfo gnss.minorVersion: %d", resPtr->robustLocationData.version.minor);
                        }
                };

                taf_SelfTestResult_t resCallback = {};
                pa_result_t res = taf_pa_location_requestRobustLocation(cb, (std::any)&resCallback);
                if(res != PA_OK)
                {
                    result = LE_FAULT;
                }
                else
                {
                    if(resCallback.result == PA_OK)
                    {
                        *enable = resCallback.robustLocationData.enabled;
                        *enabled911 = resCallback.robustLocationData.enabledForE911;
                        *majorVersion = resCallback.robustLocationData.version.major;
                        *minorVersion = resCallback.robustLocationData.version.minor;
                    }
                    else
                    {
                        LE_DEBUG("RobustLocationInformation is Failed");
                        result = LE_FAULT;
                    }
                }
            }
        break;
        default:
        {
            LE_ERROR("Invalid GNSS state %d", clientRequestPtr->GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
static void DefaultSecondaryBandConstellationsSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_DefaultSecondaryBandConstellationsRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::DefaultSecondaryBandConstellationsWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, DefaultSecondaryBandConstellationsSvcRespond, cPtr, NULL);
        return;
    }

    cPtr->retCode = LE_FAULT;

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        {
            typedef struct{
                pa_result_t result;
            }taf_SelfTestResult_t;
            auto cb = [](pa_result_t result, std::any context) {
                taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                resPtr->result = result;
            };
            taf_SelfTestResult_t resCallback = {};

            // Configure Secondary Band constellation
            gnss.mRequestSB = 0;//reset the value before configuring
            std::unordered_set<taf_pa_location_GnssConstellationType_t> constSet{};
            pa_result_t res = taf_pa_location_configureSecondaryBand(constSet,cb,(std::any)&resCallback);
            if (res == PA_NOT_IMPLEMENTED) {
                cPtr->retCode = LE_FAULT;
            } else if (res == PA_OK) {
                cPtr->retCode = (le_result_t)resCallback.result;
                if(cPtr->retCode == LE_OK)
                {
                    LE_DEBUG("Success");
                }
            }
        }
        break;
        default:
        {
            cPtr->retCode = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, DefaultSecondaryBandConstellationsSvcRespond, cPtr, NULL);
}

void taf_locGnss::DefaultSecondaryBandConstellations(taf_locGnss_ServerCmdRef_t cmdRef)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, DefaultSecondaryBandConstellationsWorker, cPtr, NULL);
}

static void RequestSecondaryBandConstellationsRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_RequestSecondaryBandConstellationsRespond(cPtr->cmdRef, cPtr->retCode, cPtr->params.configureSecondaryBand.constellationSb);
    le_mem_Release(cPtr);
}

void taf_locGnss::RequestSecondaryBandConstellationsWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->params.configureSecondaryBand.constellationSb = 0;
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, RequestSecondaryBandConstellationsRespond, cPtr, NULL);
        return;
    }

    cPtr->retCode = LE_FAULT;

    switch ( clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]",  clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        {
            // Set GNSS Request Secondary Band constellation
            typedef struct{
                pa_result_t result;
                int secConstellationValue;
            }taf_SelfTestResult_t;
            auto cb = [](pa_result_t result,
                std::set<taf_pa_location_GnssConstellationType_t> constellationSet_,std::any context) {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                    if(result == PA_OK) {
                        for (auto item : constellationSet_)
                        {
                            if (item == TAF_PA_LOCATION_GPS)
                            {
                                resPtr->secConstellationValue |= (1<<(TAF_LOCGNSS_SB_CONSTELLATION_GPS-1));//1st bit
                            }
                            else if (item == TAF_PA_LOCATION_GALILEO)
                            {
                                resPtr->secConstellationValue |= (1<<(TAF_LOCGNSS_SB_CONSTELLATION_GALILEO-1));//2nd bit
                            }
                            else if (item == TAF_PA_LOCATION_SBAS)
                            {
                                resPtr->secConstellationValue |= (1<<(TAF_LOCGNSS_SB_CONSTELLATION_SBAS-1));//3rd bit
                            }
                            else if (item == TAF_PA_LOCATION_COMPASS)
                            {
                                resPtr->secConstellationValue |= (1<<(TAF_LOCGNSS_SB_CONSTELLATION_COMPASS-1)); //4th bit
                            }
                            else if (item == TAF_PA_LOCATION_GLONASS)
                            {
                                resPtr->secConstellationValue |= (1<<(TAF_LOCGNSS_SB_CONSTELLATION_GLONASS-1)); //5th bit
                            }
                            else if (item == TAF_PA_LOCATION_BDS)
                            {
                                resPtr->secConstellationValue |= (1<<(TAF_LOCGNSS_SB_CONSTELLATION_BDS-1));//6th bit
                            }
                            else if (item == TAF_PA_LOCATION_QZSS)
                            {
                                resPtr->secConstellationValue |=(1<<(TAF_LOCGNSS_SB_CONSTELLATION_QZSS-1));//7th bit
                            }
                            else if (item == TAF_PA_LOCATION_NAVIC)
                            {
                                resPtr->secConstellationValue |= (1<<(TAF_LOCGNSS_SB_CONSTELLATION_NAVIC-1));//8th bit
                            }
                            else
                            {
                                LE_DEBUG("onSecondaryBandInfo: Not supported");
                            }
                        }
                    }
            };

            taf_SelfTestResult_t resCallback = {};
            pa_result_t res = taf_pa_location_requestSecondaryBandConfig(cb,(std::any)&resCallback);
            if(res != PA_OK)
            {
                cPtr->retCode = LE_FAULT;
            }
            else
            {
                if(resCallback.result == PA_OK)
                {
                    cPtr->retCode = LE_OK;
                    cPtr->params.configureSecondaryBand.constellationSb = resCallback.secConstellationValue;
                    gnss.mRequestSB = resCallback.secConstellationValue;
                }
                else
                {
                    cPtr->retCode = LE_FAULT;
                }
            }
        }
        break;
        default:
        {
            cPtr->retCode = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d",  clientRequestPtr->GnssState);
        }
        break;
    }
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, RequestSecondaryBandConstellationsRespond, cPtr, NULL);
}

void taf_locGnss::RequestSecondaryBandConstellations(taf_locGnss_ServerCmdRef_t cmdRef)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, RequestSecondaryBandConstellationsWorker, cPtr, NULL);
}

// ---------------------------------------------------------------------------
// ConfigureSecondaryBandConstellations
// ---------------------------------------------------------------------------
static void ConfigureSecondaryBandConstellationsSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_ConfigureSecondaryBandConstellationsRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::ConfigureSecondaryBandConstellationsWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    uint32_t constellationSb = cPtr->params.configureSecondaryBand.constellationSb;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ConfigureSecondaryBandConstellationsSvcRespond, cPtr, NULL);
        return;
    }

    cPtr->retCode = LE_FAULT;

    std::unordered_set<taf_pa_location_GnssConstellationType_t> constSet{};
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_GPS-1))) //GPS->1
    {
        constSet.insert(TAF_PA_LOCATION_GPS);
    }
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_GALILEO-1))) //GALILEO->2
    {
        constSet.insert(TAF_PA_LOCATION_GALILEO);
    }
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_SBAS-1)))//SBAS->4
    {
        constSet.insert(TAF_PA_LOCATION_SBAS);
    }
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_COMPASS-1))) //COMPASS->8
    {
        constSet.insert(TAF_PA_LOCATION_COMPASS);
    }
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_GLONASS-1)))//GLONASS->16
    {
        constSet.insert(TAF_PA_LOCATION_GLONASS);
    }
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_BDS-1))) //BDS->32
    {
        constSet.insert(TAF_PA_LOCATION_BDS);
    }
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_QZSS-1))) //QZSS->64
    {
        constSet.insert(TAF_PA_LOCATION_QZSS);
    }
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_NAVIC-1))) //NAVIC->128
    {
        constSet.insert(TAF_PA_LOCATION_NAVIC);
    }
    switch ( clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]",  clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        {
            typedef struct{
                pa_result_t result;
            }taf_SelfTestResult_t;
            auto cb = [](pa_result_t result, std::any context) {
                taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                resPtr->result = result;
            };
            taf_SelfTestResult_t resCallback = {};

            // Configure Secondary Band constellation
            gnss.mRequestSB = 0;//reset the value before configuring
            pa_result_t res = taf_pa_location_configureSecondaryBand(constSet,cb,(std::any)&resCallback);
            if (res == PA_NOT_IMPLEMENTED) {
                cPtr->retCode = LE_FAULT;
            } else if (res == PA_OK) {
                cPtr->retCode = (le_result_t)resCallback.result;
                if(cPtr->retCode == LE_OK)
                {
                    LE_DEBUG("Success");
                }
            }
        }
        break;
        default:
        {
            cPtr->retCode = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d",  clientRequestPtr->GnssState);
        }
        break;
    }
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ConfigureSecondaryBandConstellationsSvcRespond, cPtr, NULL);
}

void taf_locGnss::ConfigureSecondaryBandConstellations(taf_locGnss_ServerCmdRef_t cmdRef,
                                                         uint32_t constellationSb)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    cPtr->params.configureSecondaryBand.constellationSb = constellationSb;
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, ConfigureSecondaryBandConstellationsWorker, cPtr, NULL);
}
#endif

// ---------------------------------------------------------------------------
// SetLeverArmConfig
// ---------------------------------------------------------------------------
static void SetLeverArmConfigSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_SetLeverArmConfigRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::SetLeverArmConfigWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetLeverArmConfigSvcRespond, cPtr, NULL);
        return;
    }

    taf_locGnss_LeverArmParams_t* LeverArmParamsPtr = &cPtr->params.setLeverArmConfig.leverArmParams;

    cPtr->retCode = LE_NOT_PERMITTED;
    taf_pa_location_LeverArmParams_t* LeverArmConfigInfoPtr = new taf_pa_location_LeverArmParams_t();
    if (LeverArmConfigInfoPtr == NULL)
    {
        cPtr->retCode = LE_NO_MEMORY;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetLeverArmConfigSvcRespond, cPtr, NULL);
        return;
    }

    switch ( clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        {
            if((LeverArmParamsPtr->levArmType < TAF_LOCGNSS_LEVER_ARM_TYPE_GNSS_TO_VRP)
                || (LeverArmParamsPtr->levArmType >TAF_LOCGNSS_LEVER_ARM_TYPE_VPE_IMU_TO_GNSS))
            {
                LE_ERROR("invalid Lever Arm type, returning");
                cPtr->retCode = LE_BAD_PARAMETER;
                le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetLeverArmConfigSvcRespond, cPtr, NULL);
                return;
            }
            //Filling the Lever Arm types
            if(LeverArmParamsPtr->levArmType == TAF_LOCGNSS_LEVER_ARM_TYPE_GNSS_TO_VRP)
            {
                LeverArmConfigInfoPtr->levArmType = TAF_PA_LOCATION_LEVER_ARM_TYPE_GNSS_TO_VRP;
            }
            else if(LeverArmParamsPtr->levArmType == TAF_LOCGNSS_LEVER_ARM_TYPE_DR_IMU_TO_GNSS)
            {
                LeverArmConfigInfoPtr->levArmType = TAF_PA_LOCATION_LEVER_ARM_TYPE_DR_IMU_TO_GNSS;
            }
            else if(LeverArmParamsPtr->levArmType == TAF_LOCGNSS_LEVER_ARM_TYPE_VPE_IMU_TO_GNSS)
            {
                LeverArmConfigInfoPtr->levArmType = TAF_PA_LOCATION_LEVER_ARM_TYPE_VPE_IMU_TO_GNSS;
            }

            //Filling the Lever Arm Paramters
            LeverArmConfigInfoPtr->forwardOffset = (float)LeverArmParamsPtr->forwardOffsetMeters;
            LeverArmConfigInfoPtr->sidewaysOffset =(float)LeverArmParamsPtr->sidewaysOffsetMeters;
            LeverArmConfigInfoPtr->upOffset = (float)LeverArmParamsPtr->upOffsetMeters;

            //Set the Lever Arm Configuration
            typedef struct{
                pa_result_t result;
            }taf_SelfTestResult_t;
            auto cb = [](pa_result_t result, std::any context) {
                taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                resPtr->result = result;
            };
            taf_SelfTestResult_t resCallback = {};
            pa_result_t res = taf_pa_location_configureLeverArm(LeverArmConfigInfoPtr,cb,(std::any)&resCallback);
            if (res != PA_OK)
            {
                cPtr->retCode = LE_FAULT;
            }
            else
            {
                if(resCallback.result == PA_OK)
                {
                    cPtr->retCode = LE_OK;
                }
                else
                {
                    cPtr->retCode = LE_FAULT;
                }
                if (LE_OK != cPtr->retCode)
                {
                    LE_ERROR("Unable to set the Lever Arm Configuration error = %d (%s)",
                            cPtr->retCode, LE_RESULT_TXT(cPtr->retCode));
                }
            }
        }
        break;
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
            cPtr->retCode = LE_FAULT;
        }
        break;
    }

    delete LeverArmConfigInfoPtr;
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetLeverArmConfigSvcRespond, cPtr, NULL);
}

void taf_locGnss::SetLeverArmConfig(taf_locGnss_ServerCmdRef_t cmdRef,
                                     const taf_locGnss_LeverArmParams_t* LeverArmParamsPtr)
{
    auto &gnss = taf_locGnss::GetInstance();
    if (LeverArmParamsPtr == NULL)
    {
        LE_ERROR("SetLeverArmConfig: LeverArmParamsPtr is NULL");
        taf_locGnss_SetLeverArmConfigRespond(cmdRef, LE_FAULT);
        return;
    }
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    cPtr->params.setLeverArmConfig.leverArmParams = *LeverArmParamsPtr;  // deep copy
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, SetLeverArmConfigWorker, cPtr, NULL);
}


// ---------------------------------------------------------------------------
// SetEngineType
// ---------------------------------------------------------------------------
static void SetEngineTypeSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_SetEngineTypeRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::SetEngineTypeWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_EngineReportsType_t engineType = cPtr->params.setEngineType.engineType;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetEngineTypeSvcRespond, cPtr, NULL);
        return;
    }

    cPtr->retCode = LE_FAULT;
    if((engineType <TAF_LOCGNSS_ENGINE_REPORT_TYPE_FUSED)
          || (engineType>TAF_LOCGNSS_ENGINE_REPORT_TYPE_VPE))
    {
        cPtr->retCode = LE_BAD_PARAMETER;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetEngineTypeSvcRespond, cPtr, NULL);
        return;
    }

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        {
            // Set Engine Type
            clientRequestPtr->mEngineType = engineType;
            cPtr->retCode = LE_OK;
        }
        break;
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
        }
        break;
        break;
        default:
        {
            cPtr->retCode = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetEngineTypeSvcRespond, cPtr, NULL);
}

void taf_locGnss::SetEngineType(taf_locGnss_ServerCmdRef_t cmdRef,
                                 taf_locGnss_EngineReportsType_t EngineType)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    cPtr->params.setEngineType.engineType = EngineType;
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, SetEngineTypeWorker, cPtr, NULL);
}


le_result_t taf_locGnss::GetConformityIndex
(
    taf_locGnss_SampleRef_t positionSampleRef,
    double* indexPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (indexPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->conformityValid)
        {
            *indexPtr = posSampleReqPtr->positionSampleNodePtr->robustConformity;
        }
        else
        {
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_locGnss::GetCalibrationData
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint32_t* calibPtr,
    uint8_t* percentPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }
    if (calibPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->calibrationStatusValid)
        {
            *calibPtr = posSampleReqPtr->positionSampleNodePtr->calibrationStatus;
        }
        else
        {
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    if (percentPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->confidencePercentValid)
        {
            *percentPtr = posSampleReqPtr->positionSampleNodePtr->confidencePercent;
        }
        else
        {
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_locGnss::GetDRSolutionStatus
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint32_t* solutionStatusPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }
    if (solutionStatusPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->drSolutionStatusValid)
        {
            *solutionStatusPtr = posSampleReqPtr->positionSampleNodePtr->drSolutionStatus;
        }
        else
        {
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}


le_result_t taf_locGnss::GetBodyFrameData
(
    taf_locGnss_SampleRef_t positionSampleRef,
    taf_locGnss_KinematicsData_t* bodyDataPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (bodyDataPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->GnssKinematicsDataValid)
        {
            bodyDataPtr->bodyFrameDataMask =
                       posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.bodyFrameDataMask;
            bodyDataPtr->longAccel =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.longAccel;
            bodyDataPtr->latAccel =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.latAccel;
            bodyDataPtr->vertAccel =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.vertAccel;
            bodyDataPtr->yawRate =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.yawRate;
            bodyDataPtr->pitch =
                         posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.pitch;
            bodyDataPtr->longAccelUnc =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.longAccelUnc;
            bodyDataPtr->latAccelUnc =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.latAccelUnc;
            bodyDataPtr->vertAccelUnc =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.vertAccelUnc;
            bodyDataPtr->yawRateUnc =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.yawRateUnc;
            bodyDataPtr->pitchUnc =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.pitchUnc;
            bodyDataPtr->pitchRate =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.pitchRate;
            bodyDataPtr->pitchRateUnc =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.pitchRateUnc;
            bodyDataPtr->roll =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.roll;
            bodyDataPtr->rollUnc =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.rollUnc;
            bodyDataPtr->rollRate =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.rollRate;
            bodyDataPtr->rollRateUnc =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.rollRateUnc;
            bodyDataPtr->yaw =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.yaw;
            bodyDataPtr->yawUnc =
                        posSampleReqPtr->positionSampleNodePtr->GnssKinematicsData.yawUnc;
        }
        else
        {
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_locGnss::GetVRPBasedLLA
(
    taf_locGnss_SampleRef_t positionSampleRef,
    double* vrpLatitudePtr,
    double* vrpLongitudePtr,
    double* vrpAltitudePtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (vrpLatitudePtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->vrpLatitudeValid)
        {
            *vrpLatitudePtr = posSampleReqPtr->positionSampleNodePtr->vrpLatitude;
        }
        else
        {
            *vrpLatitudePtr = DBL_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    if (vrpLongitudePtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->vrpLongitudeValid)
        {
            *vrpLongitudePtr = posSampleReqPtr->positionSampleNodePtr->vrpLongitude;
        }
        else
        {
            *vrpLongitudePtr = DBL_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    if (vrpAltitudePtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->vrpAltitudeValid)
        {
            *vrpAltitudePtr = posSampleReqPtr->positionSampleNodePtr->vrpAltitude;
        }
        else
        {
            *vrpAltitudePtr = DBL_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_locGnss::GetVRPBasedVelocity
(
    taf_locGnss_SampleRef_t positionSampleRef,
    double* eastVelPtr,
    double* northVelPtr,
    double* upVelPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (eastVelPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->eastVelValid)
        {
            *eastVelPtr = posSampleReqPtr->positionSampleNodePtr->eastVel;
        }
        else
        {
            *eastVelPtr = DBL_MAX;
            return LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    if (northVelPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->northVelValid)
        {
            *northVelPtr = posSampleReqPtr->positionSampleNodePtr->northVel;
        }
        else
        {
            *northVelPtr = DBL_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    if (upVelPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->upVelValid)
        {
            *upVelPtr = posSampleReqPtr->positionSampleNodePtr->upVel;
        }
        else
        {
            *upVelPtr = DBL_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_locGnss::GetSvUsedInPosition
(
    taf_locGnss_SampleRef_t positionSampleRef,
    taf_locGnss_SvUsedInPosition_t* svDataPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (svDataPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->svDataValid)
        {
            svDataPtr->gps = posSampleReqPtr->positionSampleNodePtr->svData.gps;
            svDataPtr->glo = posSampleReqPtr->positionSampleNodePtr->svData.glo;
            svDataPtr->gal = posSampleReqPtr->positionSampleNodePtr->svData.gal;
            svDataPtr->bds = posSampleReqPtr->positionSampleNodePtr->svData.bds;
            svDataPtr->qzss = posSampleReqPtr->positionSampleNodePtr->svData.qzss;
            svDataPtr->navic = posSampleReqPtr->positionSampleNodePtr->svData.navic;
        }
        else
        {
            svDataPtr->gps = UINT64_MAX;
            svDataPtr->glo = UINT64_MAX;
            svDataPtr->gal = UINT64_MAX;
            svDataPtr->bds = UINT64_MAX;
            svDataPtr->qzss = UINT64_MAX;
            svDataPtr->navic = UINT64_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_locGnss::GetSbasCorrection
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint32_t* sbasMaskPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (sbasMaskPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->sbasMaskValid)
        {
            *sbasMaskPtr = posSampleReqPtr->positionSampleNodePtr->sbasMask;
        }
        else
        {
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_locGnss::GetNavigationSolution
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint32_t* navSolutionPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (navSolutionPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->navSolutionMaskValid)
        {
            uint32_t paMask = posSampleReqPtr->positionSampleNodePtr->navSolutionMask;
            uint32_t serviceMask = 0;

            if (paMask & TAF_PA_LOCATION_NAV_SBAS_SOLUTION_IONO)
                serviceMask |= TAF_LOCGNSS_NAV_SBAS_SOLUTION_IONO;

            if (paMask & TAF_PA_LOCATION_NAV_SBAS_SOLUTION_FAST)
                serviceMask |= TAF_LOCGNSS_NAV_SBAS_SOLUTION_FAST;

            if (paMask & TAF_PA_LOCATION_NAV_SBAS_SOLUTION_LONG)
                serviceMask |= TAF_LOCGNSS_NAV_SBAS_SOLUTION_LONG;

            if (paMask & TAF_PA_LOCATION_NAV_SBAS_INTEGRITY)
                serviceMask |= TAF_LOCGNSS_NAV_SBAS_INTEGRITY;

            if (paMask & TAF_PA_LOCATION_NAV_DGNSS_SOLUTION)
                serviceMask |= TAF_LOCGNSS_NAV_DGNSS_SOLUTION;

            if (paMask & TAF_PA_LOCATION_NAV_RTK_SOLUTION)
                serviceMask |= TAF_LOCGNSS_NAV_RTK_SOLUTION;

            if (paMask & TAF_PA_LOCATION_NAV_PPP_SOLUTION)
                serviceMask |= TAF_LOCGNSS_NAV_PPP_SOLUTION;

            if (paMask & TAF_PA_LOCATION_NAV_RTK_FIXED_SOLUTION)
                serviceMask |= TAF_LOCGNSS_NAV_RTK_FIXED_SOLUTION;

            if (paMask & TAF_PA_LOCATION_NAV_ONLY_SBAS_CORRECTED_SV_USED)
                serviceMask |= TAF_LOCGNSS_NAV_ONLY_SBAS_CORRECTED_SV_USED;

            *navSolutionPtr = (taf_locGnss_NavigationSolutionType_t)serviceMask;
        }
        else
        {
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_locGnss::GetPositionTechnology
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint32_t* techMaskPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (techMaskPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->techMaskValid)
        {
            *techMaskPtr = posSampleReqPtr->positionSampleNodePtr->techMask;
        }
        else
        {
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_locGnss::GetLocationInfoValidity
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint32_t* validityMaskPtr,
    uint64_t* validityExMaskPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (validityMaskPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->validityMaskValid)
        {
            *validityMaskPtr = posSampleReqPtr->positionSampleNodePtr->validityMask;
        }
        else
        {
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    if (validityExMaskPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->validityExMaskValid)
        {
            *validityExMaskPtr = posSampleReqPtr->positionSampleNodePtr->validityExMask;
        }
        else
        {
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_locGnss::GetLocationOutputEngParams
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint16_t* engMaskPtr,
    uint16_t* locationEngTypePtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (engMaskPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->engMaskValid)
        {
            *engMaskPtr = posSampleReqPtr->positionSampleNodePtr->engMask;
        }
        else
        {
            *engMaskPtr = UINT16_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    if (locationEngTypePtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->locationEngTypeValid)
        {
            *locationEngTypePtr = posSampleReqPtr->positionSampleNodePtr->locationEngType;
        }
        else
        {
            *locationEngTypePtr = UINT16_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_locGnss::GetReliabilityInformation
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint16_t* horiReliabilityPtr,
    uint16_t* vertReliabilityPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr =
        (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap, positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (horiReliabilityPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->horiReliablityValid)
        {
            *horiReliabilityPtr = posSampleReqPtr->positionSampleNodePtr->horiReliablity;
        }
        else
        {
            *horiReliabilityPtr = UINT16_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    if (vertReliabilityPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->vertReliablityValid)
        {
            *vertReliabilityPtr = posSampleReqPtr->positionSampleNodePtr->vertReliablity;
        }
        else
        {
            *vertReliabilityPtr = UINT16_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_locGnss::GetStdDeviationAzimuthInfo
(
    taf_locGnss_SampleRef_t positionSampleRef,
    double* azimuthPtr,
    double* eastDevPtr,
    double* northDevPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (azimuthPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->azimuthValid)
        {
            *azimuthPtr = posSampleReqPtr->positionSampleNodePtr->azimuth;
        }
        else
        {
            *azimuthPtr = DBL_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    if (eastDevPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->eastDevValid)
        {
            *eastDevPtr = posSampleReqPtr->positionSampleNodePtr->eastDev;
        }
        else
        {
            *eastDevPtr = DBL_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    if (northDevPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->northDevValid)
        {
            *northDevPtr = posSampleReqPtr->positionSampleNodePtr->northDev;
        }
        else
        {
            *northDevPtr = DBL_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_locGnss::GetRealTimeInformation
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint64_t* realTimePtr,
    uint64_t* realTimeUncPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (realTimePtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->realTimeValid)
        {
            *realTimePtr = posSampleReqPtr->positionSampleNodePtr->realTime;
        }
        else
        {
            *realTimePtr = UINT64_MAX;
            return LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    if (realTimeUncPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->realTimeUncValid)
        {
            *realTimeUncPtr = posSampleReqPtr->positionSampleNodePtr->realTimeUnc;
        }
        else
        {
            *realTimeUncPtr = UINT64_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_locGnss::GetMeasurementUsageInfo
(
    taf_locGnss_SampleRef_t positionSampleRef,
    taf_locGnss_GnssMeasurementInfo_t* measInfoPtr,
    size_t* measInfoLen
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    for (auto i = 0; i < (int) *measInfoLen; i++) {
        measInfoPtr[i].gnssSignalType = posSampleReqPtr->positionSampleNodePtr->measInfo[i].gnssSignalType;
        measInfoPtr[i].gnssConstellation = posSampleReqPtr->positionSampleNodePtr->measInfo[i].gnssConstellation;
        measInfoPtr[i].gnssSvId = posSampleReqPtr->positionSampleNodePtr->measInfo[i].gnssSvId;
    }

    *measInfoLen = posSampleReqPtr->positionSampleNodePtr->measInfoCount;

    return LE_OK;
}

le_result_t taf_locGnss::GetReportStatus
(
    taf_locGnss_SampleRef_t positionSampleRef,
	int32_t* reportStatusPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    *reportStatusPtr = posSampleReqPtr->positionSampleNodePtr->reportStatus;

    return LE_OK;
}

le_result_t taf_locGnss::GetAltitudeMeanSeaLevel
(
    taf_locGnss_SampleRef_t positionSampleRef,
	double* altMeanSeaLevelPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    *altMeanSeaLevelPtr = posSampleReqPtr->positionSampleNodePtr->altMeanSeaLevel;

    return LE_OK;
}

le_result_t taf_locGnss::GetSVIds
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint16_t* sVIdsPtr,
    size_t* sVIdsLen
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
            = (taf_locGnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    *sVIdsLen = posSampleReqPtr->positionSampleNodePtr->SVIdsCount;
    for (auto i = 0; i < (int) *sVIdsLen; i++) {
        sVIdsPtr[i] = posSampleReqPtr->positionSampleNodePtr->SVIds[i];
    }

    return LE_OK;
}

le_result_t taf_locGnss::GetSatellitesInfoEx
(
    taf_locGnss_SampleRef_t positionSampleRef,
    taf_locGnss_Constellation_t constellation,
    taf_locGnss_SvInfo_t* svInfoPtr,
    size_t* svInfoLen

)
{
    le_result_t result = LE_FAULT;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);
    int i;

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (svInfoPtr && svInfoLen)
    {
        size_t svInfoNumElements = NUM_ARRAY_MEMBERS(posSampleReqPtr->positionSampleNodePtr->satInfo);
        LE_DEBUG("GetSatellitesInfoEx: svInfoNumElements %d, constellation: %d, input len: %d", (int)svInfoNumElements, (int) constellation, (int) *svInfoLen);

        int count = 0;
        bool truncated = false;
        if (posSampleReqPtr->positionSampleNodePtr->satInfoValid)
        {
            for(i=0; i < (int) svInfoNumElements; i++)
            {
                if(posSampleReqPtr->positionSampleNodePtr->satInfo[i].satConst == constellation) {
                    if (count < (int)*svInfoLen) {
                        svInfoPtr[count].satId = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satId;
                        svInfoPtr[count].satConst = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satConst;
                        svInfoPtr[count].satUsed = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satUsed;
                        svInfoPtr[count].satTracked = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satTracked;
                        svInfoPtr[count].satSnr = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satSnr;
                        svInfoPtr[count].satAzim = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satAzim;
                        svInfoPtr[count].satElev = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satElev;
                        svInfoPtr[count].signalType = posSampleReqPtr->positionSampleNodePtr->satInfo[i].signalType;
                        svInfoPtr[count].glonassFcn = posSampleReqPtr->positionSampleNodePtr->satInfo[i].glonassFcn;
                        svInfoPtr[count].baseBandCnr = posSampleReqPtr->positionSampleNodePtr->satInfo[i].baseBandCnr;
                        svInfoPtr[count].satUsedDgnss = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satUsedDgnss;
                        count++;
                    }else{
                        truncated = true;
                    }
                }
            }
            if (truncated){
                count = 0;
                *svInfoLen = 0;
                result = LE_OVERFLOW;
            }else{
                result = LE_OK;
            }
            *svInfoLen = count;
            return result;
        }
        else
        {
            for(i=0; i<(int)*svInfoLen; i++)
            {
                svInfoPtr[i].satId = UINT16_MAX;
                svInfoPtr[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_UNDEFINED;
                svInfoPtr[i].satUsed = false;
                svInfoPtr[i].satTracked = false;
                svInfoPtr[i].satSnr = UINT8_MAX;
                svInfoPtr[i].satAzim = UINT16_MAX;
                svInfoPtr[i].satElev = UINT8_MAX;
                svInfoPtr[i].signalType = UINT32_MAX;
                svInfoPtr[i].glonassFcn = UINT16_MAX;
                svInfoPtr[i].baseBandCnr = 0.0;
                svInfoPtr[i].satUsedDgnss = false;
            }
            *svInfoLen = 0;
            return LE_OUT_OF_RANGE;
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// SetMinGpsWeek
// ---------------------------------------------------------------------------
static void SetMinGpsWeekSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_SetMinGpsWeekRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::SetMinGpsWeekWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    uint16_t minGpsWeek = cPtr->params.setMinGpsWeek.minGpsWeek;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetMinGpsWeekSvcRespond, cPtr, NULL);
        return;
    }

    cPtr->retCode = LE_FAULT;

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
             LE_ERROR("Wrong Gnss State [%d]", clientRequestPtr->GnssState);
             cPtr->retCode = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        {
            typedef struct{
                pa_result_t result;
            }taf_SelfTestResult_t;
            auto cb1 = [](pa_result_t result, std::any context) {
                taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                resPtr->result = result;
            };
            taf_SelfTestResult_t resCallback = {};
            pa_result_t res = taf_pa_location_configureMinGpsWeek(minGpsWeek,cb1,(std::any)&resCallback);
            if(res == PA_OK){
                if(resCallback.result == PA_OK)
                {
                    cPtr->retCode = LE_OK;
                }
            }
            else
            {
                cPtr->retCode = LE_FAULT;
            }
        }
        break;
        default:
        {
            cPtr->retCode = LE_FAULT;
            LE_ERROR("Invalid GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetMinGpsWeekSvcRespond, cPtr, NULL);
}

void taf_locGnss::SetMinGpsWeek(taf_locGnss_ServerCmdRef_t cmdRef, uint16_t minGpsWeek)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    cPtr->params.setMinGpsWeek.minGpsWeek = minGpsWeek;
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, SetMinGpsWeekWorker, cPtr, NULL);
}



le_result_t taf_locGnss::GetMinGpsWeek
(
   uint16_t*  minGpsWeekPtr
)
{
    le_result_t result = LE_FAULT;
    TAF_ERROR_IF_RET_VAL(NULL == minGpsWeekPtr, LE_FAULT, "minGpsWeekPtr is NULL");
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");
    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("GetMinGpsWeek: Wrong Gnss State [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            typedef struct{
                pa_result_t result;
                uint16_t minGpsWeek;
            }taf_SelfTestResult_t;
            auto cb = [](pa_result_t result,
                uint16_t* minGpsWeek_,std::any context) {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                    if(result == PA_OK) {
                        resPtr->minGpsWeek = *minGpsWeek_;
                    }
            };

            taf_SelfTestResult_t resCallback = {};
            pa_result_t res = taf_pa_location_requestMinGpsWeek(cb,(std::any)&resCallback);

            if(res == PA_OK){
                if(resCallback.result == PA_OK)
                {
                    result = LE_OK;
                    *minGpsWeekPtr = resCallback.minGpsWeek;
                }
            }
            else
            {
                result = LE_FAULT;
            }
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("GetMinGpsWeek: Invalid GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }

    return result;
}

le_result_t taf_locGnss::GetCapabilities
(
   uint64_t*  locCapabilityPtr
)
{
    TAF_ERROR_IF_RET_VAL(NULL == locCapabilityPtr, LE_FAULT, "locCapabilityPtr is NULL");
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    *locCapabilityPtr = 0;
    uint32_t caps = 0;
    pa_result_t capsRes = taf_pa_location_getCapabilities(
        clientRequestPtr->locationClient, &caps,
        std::any(clientRequestPtr->sessionRef));
    if (capsRes != PA_OK)
    {
        LE_ERROR("taf_pa_location_getCapabilities failed: %d", (int)capsRes);
        return LE_FAULT;
    }
    else
    {
        *locCapabilityPtr = (uint64_t)caps;
    }

    LE_DEBUG("GetCapabilities: Location Capabilites is %" PRIu64 "", *locCapabilityPtr);

    return LE_OK;
}

// ---------------------------------------------------------------------------
// SetNmeaConfiguration
// ---------------------------------------------------------------------------
static void SetNmeaConfigurationSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_SetNmeaConfigurationRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::SetNmeaConfigurationWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_NmeaBitMask_t       nmeaMask  = cPtr->params.setNmeaConfiguration.nmeaMask;
    taf_locGnss_GeodeticDatumType_t datumType = cPtr->params.setNmeaConfiguration.datumType;
    taf_locGnss_LocEngineType_t     engineType = cPtr->params.setNmeaConfiguration.engineType;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetNmeaConfigurationSvcRespond, cPtr, NULL);
        return;
    }

    cPtr->retCode = LE_NOT_PERMITTED;

    taf_pa_location_NmeaConfig_t nmeaConfig;
    nmeaConfig.sentenceConfig = nmeaMask;
    if(datumType>=TAF_LOCGNSS_GEODETIC_TYPE_WGS_84 && datumType <= TAF_LOCGNSS_GEODETIC_TYPE_PZ_90)
    {
        nmeaConfig.datumType = (taf_pa_location_GeodeticDatumType_t) datumType;
    }
    else
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetNmeaConfigurationSvcRespond, cPtr, NULL);
        return;
    }
#if defined(TARGET_SA525M)
    if(engineType>= TAF_LOCGNSS_LOC_ENGINE_FUSED && engineType <= TAF_LOCGNSS_LOC_ENGINE_VPE)
    {
        nmeaConfig.engineType = engineType;
    }
    else
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetNmeaConfigurationSvcRespond, cPtr, NULL);
        return;
    }
#else
    (void)engineType;
#endif
    LE_DEBUG("SetNmeaConfiguration nmeaMask: %" PRIu64" and datumType: %d", nmeaMask, datumType);

    // Check if the bit mask is correct
    if (nmeaMask == 0)
    {
        LE_ERROR("Unable to set the enabled NMEA, wrong bit mask %" PRIu64 "", nmeaMask);
        cPtr->retCode = LE_BAD_PARAMETER;
    }
    else
    {
        // Check the GNSS device state
        switch (clientRequestPtr->GnssState)
        {
            case TAF_LOCGNSS_STATE_READY:
            case TAF_LOCGNSS_STATE_ACTIVE:
            {
                typedef struct{
                    pa_result_t result;
                }taf_SelfTestResult_t;
                auto cb = [](pa_result_t result, std::any context) {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                };
                taf_SelfTestResult_t resCallback = {};
                pa_result_t res = taf_pa_location_configureNmea(nmeaConfig,cb,(std::any)&resCallback);
                if(res == PA_OK){
                    if(resCallback.result == PA_OK)
                    {
                        cPtr->retCode = LE_OK;
                    }
                    else
                    {
                    cPtr->retCode = LE_FAULT;
                    }
                }
                else
                {
                    cPtr->retCode = LE_FAULT;
                    LE_DEBUG("SetNmeaConfiguration() is failed!");
                }
                if (LE_OK != cPtr->retCode)
                {
                    LE_ERROR("Unable to set the enabled NMEA, error = %d (%s)",
                              cPtr->retCode, LE_RESULT_TXT(cPtr->retCode));
                }
            }
            break;
            case TAF_LOCGNSS_STATE_UNINITIALIZED:
            case TAF_LOCGNSS_STATE_DISABLED:
            {
                LE_ERROR("SetNmeaConfiguration: Bad state for that request [%d]", clientRequestPtr->GnssState);
                cPtr->retCode = LE_NOT_PERMITTED;
            }
            break;
            default:
            {
                LE_ERROR("SetNmeaConfiguration: Unknown GNSS state %d", clientRequestPtr->GnssState);
                cPtr->retCode = LE_FAULT;
            }
            break;
        }
    }

    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, SetNmeaConfigurationSvcRespond, cPtr, NULL);
}

void taf_locGnss::SetNmeaConfiguration(taf_locGnss_ServerCmdRef_t cmdRef,
                                         taf_locGnss_NmeaBitMask_t nmeaMask,
                                         taf_locGnss_GeodeticDatumType_t datumType,
                                         taf_locGnss_LocEngineType_t engineType)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    cPtr->params.setNmeaConfiguration.nmeaMask   = nmeaMask;
    cPtr->params.setNmeaConfiguration.datumType  = datumType;
    cPtr->params.setNmeaConfiguration.engineType = engineType;
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, SetNmeaConfigurationWorker, cPtr, NULL);
}

le_result_t taf_locGnss::GetXtraStatus
(
    taf_locGnss_XtraStatusParams_t* xtraParams //Specify Xtra assistant data's current status,
                                          // validity and whether it is enabled.
)
{
    le_result_t result = LE_NOT_PERMITTED;

    TAF_ERROR_IF_RET_VAL(NULL == xtraParams, LE_FAULT, "xtraParams is NULL");
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            typedef struct{
                pa_result_t result;
                taf_pa_location_XtraStatus_t xtraStatus;
            }taf_SelfTestResult_t;
            auto cb = [](pa_result_t result,
                taf_pa_location_XtraStatus_t xtraStatus_,std::any context) {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                    if(result == PA_OK) {
                        resPtr->xtraStatus.featureEnabled = xtraStatus_.featureEnabled;
                        resPtr->xtraStatus.xtraValidForHours = xtraStatus_.xtraValidForHours;
                        resPtr->xtraStatus.xtraDataStatus = xtraStatus_.xtraDataStatus;
                    }
            };
            taf_SelfTestResult_t resCallback = {};
            pa_result_t res = taf_pa_location_requestXtraStatus(cb,(std::any)&resCallback);
            if(res != PA_OK){
                return LE_FAULT;
            }
            else
            {
                if(resCallback.result == PA_OK)
                {
                    result = LE_OK;
                    xtraParams->featureEnabled = resCallback.xtraStatus.featureEnabled;
                    xtraParams->xtraValidForHours = resCallback.xtraStatus.xtraValidForHours;
                    xtraParams->xtraDataStatus = resCallback.xtraStatus.xtraDataStatus;
                }
                else
                {
                    LE_ERROR("Request xtra status is failed");
                    result = LE_FAULT;
                }
            }
        }
        break;
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            LE_ERROR("GetXtraStatus: Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
                LE_ERROR("GetXtraStatus: Unknown GNSS state %d", clientRequestPtr->GnssState);
                result = LE_FAULT;
        }
        break;
    }
    return result;
}

le_result_t taf_locGnss::GetGnssData
(
    taf_locGnss_SampleRef_t positionSampleRef,
    taf_locGnss_GnssData_t* gnssDataPtr,
    size_t* maxSignalTypes
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);
    int i;

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (posSampleReqPtr->positionSampleNodePtr->gnssDataValid)
    {
        for(i=0; i < (int)*maxSignalTypes; i++)
        {
            gnssDataPtr[i].gnssDataMask = posSampleReqPtr->positionSampleNodePtr->gnssData[i].gnssDataMask;
            gnssDataPtr[i].jammerInd = posSampleReqPtr->positionSampleNodePtr->gnssData[i].jammerInd;
            gnssDataPtr[i].agc = posSampleReqPtr->positionSampleNodePtr->gnssData[i].agc;
        }
    }
    else
    {
        LE_DEBUG("GetGnssData Invalid");
    }
    return result;
}

le_result_t taf_locGnss::GetGptpTime
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint64_t* gPtpTime,
    uint64_t* gPtpTimeUnc
)
{
    le_result_t result = LE_OK;
    TAF_KILL_CLIENT_IF_RET_VAL(((NULL == gPtpTime) || (NULL == gPtpTimeUnc)), LE_FAULT, "Invalid reference");
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (LE_OK != result)
    {
        return result;
    }

    if (posSampleReqPtr->positionSampleNodePtr->gPtpTimeValid)
    {
        *gPtpTime = posSampleReqPtr->positionSampleNodePtr->gPtpTime;
    }
    else
    {
        LE_ERROR("taf_locGnss_GetGptpTime time is invalid");
    }
    if (posSampleReqPtr->positionSampleNodePtr->gPtpTimeUncValid)
    {
        *gPtpTimeUnc = posSampleReqPtr->positionSampleNodePtr->gPtpTimeUnc;
    }
    else
    {
        LE_ERROR("taf_locGnss_GetGptpTime timeUnc is invalid");
    }
    return result;
}

le_result_t taf_locGnss::GetIsNHz(taf_locGnss_MeasSampleRef_t measSampleRef, bool* isNHZPtr)
{
    le_result_t result = LE_OK;

    taf_locGnss_MeasurementSampleRequest_t* measSampleReqPtr
                                            = (taf_locGnss_MeasurementSampleRequest_t*)le_ref_Lookup(MeasurementSampleMap,measSampleRef);

    if(measSampleReqPtr == NULL || measSampleReqPtr->measSampleNodePtr == NULL)
    {
        LE_ERROR("measSampleReqPtr is NULL");
        return LE_FAULT;
    }

    LE_INFO("measSampleReqPtr->measSampleNodePtr->isNHz: %d",(int)measSampleReqPtr->measSampleNodePtr->isNHz);

    *isNHZPtr = measSampleReqPtr->measSampleNodePtr->isNHz;

    return result;
}

le_result_t taf_locGnss::GetClockValidityMask(taf_locGnss_MeasSampleRef_t measSampleRef, uint32_t* clockValidityMaskPtr)
{
    le_result_t result = LE_OK;

    taf_locGnss_MeasurementSampleRequest_t* measSampleReqPtr
                                            = (taf_locGnss_MeasurementSampleRequest_t*)le_ref_Lookup(MeasurementSampleMap,measSampleRef);

    if(measSampleReqPtr == NULL || measSampleReqPtr->measSampleNodePtr == NULL)
    {
        LE_ERROR("measSampleReqPtr is NULL");
        return LE_FAULT;
    }

    LE_INFO("measSampleReqPtr->measSampleNodePtr->clock.valid: %d",(int)measSampleReqPtr->measSampleNodePtr->clock.valid);

    *clockValidityMaskPtr = measSampleReqPtr->measSampleNodePtr->clock.valid;

    return result;
}


le_result_t taf_locGnss::GetClockData(taf_locGnss_MeasSampleRef_t measSampleRef, taf_locGnss_ClockData_t* clockDataPtr)
{
    le_result_t result = LE_OK;

    taf_locGnss_MeasurementSampleRequest_t* measSampleReqPtr
                                            = (taf_locGnss_MeasurementSampleRequest_t*)le_ref_Lookup(MeasurementSampleMap,measSampleRef);

    if(measSampleReqPtr == NULL || measSampleReqPtr->measSampleNodePtr == NULL)
    {
        LE_ERROR("measSampleReqPtr is NULL");
        return LE_FAULT;
    }

    LE_INFO("measSampleReqPtr->measSampleNodePtr->clock.elapsedRealTime:  %" PRIu64 "", measSampleReqPtr->measSampleNodePtr->clock.elapsedRealTime);

    clockDataPtr->leapSecond = measSampleReqPtr->measSampleNodePtr->clock.leapSecond;
    clockDataPtr->timeNs = measSampleReqPtr->measSampleNodePtr->clock.timeNs;
    clockDataPtr->timeUncertaintyNs = measSampleReqPtr->measSampleNodePtr->clock.timeUncertaintyNs;
    clockDataPtr->fullBiasNs = measSampleReqPtr->measSampleNodePtr->clock.fullBiasNs;
    clockDataPtr->biasNs = measSampleReqPtr->measSampleNodePtr->clock.biasNs;
    clockDataPtr->biasUncertaintyNs = measSampleReqPtr->measSampleNodePtr->clock.biasUncertaintyNs;
    clockDataPtr->driftNsps = measSampleReqPtr->measSampleNodePtr->clock.driftNsps;
    clockDataPtr->driftUncertaintyNsps = measSampleReqPtr->measSampleNodePtr->clock.driftUncertaintyNsps;
    clockDataPtr->hwClockDiscontinuityCount = measSampleReqPtr->measSampleNodePtr->clock.hwClockDiscontinuityCount;
    clockDataPtr->elapsedRealTime = measSampleReqPtr->measSampleNodePtr->clock.elapsedRealTime;
    clockDataPtr->elapsedRealTimeUnc = measSampleReqPtr->measSampleNodePtr->clock.elapsedRealTimeUnc;
    clockDataPtr->elapsedgPTPTime = measSampleReqPtr->measSampleNodePtr->clock.elapsedgPTPTime;
    clockDataPtr->elapsedgPTPTimeUnc = measSampleReqPtr->measSampleNodePtr->clock.elapsedgPTPTimeUnc;

    return result;
}

le_result_t taf_locGnss::GetMeasurementsData(taf_locGnss_MeasSampleRef_t measSampleRef, taf_locGnss_MeasurementsData_t * measDataPtr, size_t* measDataSizePtr)
{
    le_result_t result = LE_OK;

    taf_locGnss_MeasurementSampleRequest_t* measSampleReqPtr
                                            = (taf_locGnss_MeasurementSampleRequest_t*)le_ref_Lookup(MeasurementSampleMap,measSampleRef);


    if(measSampleReqPtr == NULL || measSampleReqPtr->measSampleNodePtr == NULL)
    {
        LE_ERROR("measSampleReqPtr is NULL");
        return LE_FAULT;
    }

    *measDataSizePtr = measSampleReqPtr->measSampleNodePtr->measCount;

    LE_INFO("*measDataSizePtr: %d",(int)*measDataSizePtr);

    for (auto i = 0; i < (int) *measDataSizePtr; i++) {
        measDataPtr[i].svId = measSampleReqPtr->measSampleNodePtr->measData[i].svId;
        measDataPtr[i].svType = measSampleReqPtr->measSampleNodePtr->measData[i].svType;
        measDataPtr[i].timeOffsetNs = measSampleReqPtr->measSampleNodePtr->measData[i].timeOffsetNs;
        measDataPtr[i].stateMask = measSampleReqPtr->measSampleNodePtr->measData[i].stateMask;
        measDataPtr[i].receivedSvTimeNs = measSampleReqPtr->measSampleNodePtr->measData[i].receivedSvTimeNs;
        measDataPtr[i].receivedSvTimeSubNs = measSampleReqPtr->measSampleNodePtr->measData[i].receivedSvTimeSubNs;
        measDataPtr[i].receivedSvTimeUncertaintyNs = measSampleReqPtr->measSampleNodePtr->measData[i].receivedSvTimeUncertaintyNs;
        measDataPtr[i].carrierToNoiseDbHz = measSampleReqPtr->measSampleNodePtr->measData[i].carrierToNoiseDbHz;
        measDataPtr[i].pseudorangeRateMps = measSampleReqPtr->measSampleNodePtr->measData[i].pseudorangeRateMps;
        measDataPtr[i].pseudorangeRateUncertaintyMps = measSampleReqPtr->measSampleNodePtr->measData[i].pseudorangeRateUncertaintyMps;
        measDataPtr[i].adrStateMask = measSampleReqPtr->measSampleNodePtr->measData[i].adrStateMask;
        measDataPtr[i].adrMeters = measSampleReqPtr->measSampleNodePtr->measData[i].adrMeters;
        measDataPtr[i].adrUncertaintyMeters = measSampleReqPtr->measSampleNodePtr->measData[i].adrUncertaintyMeters;
        measDataPtr[i].carrierFrequencyHz = measSampleReqPtr->measSampleNodePtr->measData[i].carrierFrequencyHz;
        measDataPtr[i].carrierCycles = measSampleReqPtr->measSampleNodePtr->measData[i].carrierCycles;
        measDataPtr[i].carrierPhase = measSampleReqPtr->measSampleNodePtr->measData[i].carrierPhase;
        measDataPtr[i].carrierPhaseUncertainty = measSampleReqPtr->measSampleNodePtr->measData[i].carrierPhaseUncertainty;
        measDataPtr[i].multipathIndicator = measSampleReqPtr->measSampleNodePtr->measData[i].multipathIndicator;
        measDataPtr[i].signalToNoiseRatioDb = measSampleReqPtr->measSampleNodePtr->measData[i].signalToNoiseRatioDb;
        measDataPtr[i].agcLevelDb = measSampleReqPtr->measSampleNodePtr->measData[i].agcLevelDb;
        measDataPtr[i].gnssSignalType = measSampleReqPtr->measSampleNodePtr->measData[i].gnssSignalType;
        measDataPtr[i].basebandCarrierToNoise = measSampleReqPtr->measSampleNodePtr->measData[i].basebandCarrierToNoise;
        measDataPtr[i].fullInterSignalBias = measSampleReqPtr->measSampleNodePtr->measData[i].fullInterSignalBias;
        measDataPtr[i].fullInterSignalBiasUncertainty = measSampleReqPtr->measSampleNodePtr->measData[i].fullInterSignalBiasUncertainty;
    }

    return result;
}

static void InjectMerkleRespond
(
    void* cmdPtr,
    void* context
)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    LE_INFO("InjectMerkleRespond: result [%d]", cPtr->retCode);

    taf_locGnss_InjectMerkleTreeInformationByPathRespond(cPtr->cmdRef,
                                                          cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::InjectMerkleWorker
(
    void* cmdPtr,
    void* context
)
{
    auto& gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    taf_locGnss_Client_t* clientRequestPtr =
        gnss.DiscoverSessionRef(cPtr->sessionRef);

    if (clientRequestPtr == NULL)
    {
        LE_ERROR("InjectMerkleWorker: clientRequestPtr is NULL");
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef,
                                       InjectMerkleRespond, cPtr, NULL);
        return;
    }

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("InjectMerkleWorker: Wrong Gnss State [%d]",
                     clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
            le_event_QueueFunctionToThread(gnss.LocationSvcThRef,
                                           InjectMerkleRespond, cPtr, NULL);
            return;
        }

        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
            break;

        default:
        {
            LE_ERROR("InjectMerkleWorker: Invalid GNSS state [%d]",
                     clientRequestPtr->GnssState);
            cPtr->retCode = LE_FAULT;
            le_event_QueueFunctionToThread(gnss.LocationSvcThRef,
                                           InjectMerkleRespond, cPtr, NULL);
            return;
        }
    }

    std::ifstream configFileStream;
    std::string   merkleTreeStr;

    configFileStream.open(cPtr->params.injectMerkle.merkleTreeFilePath);
    if (!configFileStream.is_open())
    {
        LE_ERROR("InjectMerkleWorker: Failed to open file: %s",
                 cPtr->params.injectMerkle.merkleTreeFilePath);
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef,
                                       InjectMerkleRespond, cPtr, NULL);
        return;
    }

    std::string line;
    while (std::getline(configFileStream, line))
    {
        merkleTreeStr += line;
    }
    configFileStream.close();

    if (merkleTreeStr.empty())
    {
        LE_ERROR("InjectMerkleWorker: Merkle tree data is empty after reading file");
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef,
                                       InjectMerkleRespond, cPtr, NULL);
        return;
    }

    LE_DEBUG("InjectMerkleWorker: merkleTreeStr size: %zu bytes",
             merkleTreeStr.size());

    typedef struct { pa_result_t result; } MerkleResult_t;
    MerkleResult_t resCallback = {};

    auto cb = [](pa_result_t result, std::any context)
    {
        MerkleResult_t* resPtr = std::any_cast<MerkleResult_t*>(context);
        resPtr->result = result;
    };

    pa_result_t res = taf_pa_location_injectMerkleTreeInformation(
                          merkleTreeStr, cb, (std::any)&resCallback);

    if (res == PA_OK && resCallback.result == PA_OK)
    {
        LE_INFO("InjectMerkleWorker: InjectMerkleTreeInformation success!!");
        cPtr->retCode = LE_OK;
    }
    else
    {
        LE_ERROR("InjectMerkleWorker: InjectMerkleTreeInformation failed "
                 "[res=%d, cbRes=%d]", (int)res, (int)resCallback.result);
        cPtr->retCode = LE_FAULT;
    }

    le_event_QueueFunctionToThread(gnss.LocationSvcThRef,
                                   InjectMerkleRespond, cPtr, NULL);
}

void taf_locGnss::InjectMerkleData
(
    taf_locGnss_ServerCmdRef_t cmdRef,
    const char* LE_NONNULL merkleTreeFilePath
)
{
    auto& gnss = taf_locGnss::GetInstance();

    if (merkleTreeFilePath == NULL || merkleTreeFilePath[0] == '\0')
    {
        LE_ERROR("InjectMerkleTreeInformationByPath: merkleTreeFilePath is null/empty");
        taf_locGnss_InjectMerkleTreeInformationByPathRespond(cmdRef, LE_BAD_PARAMETER);
        return;
    }

    taf_locGnss_Client_t* clientRequestPtr = gnss.AcquireSessionRef();
    if (clientRequestPtr == NULL)
    {
        LE_ERROR("InjectMerkleTreeInformationByPath: Failed to acquire session");
        taf_locGnss_InjectMerkleTreeInformationByPathRespond(cmdRef, LE_FAULT);
        return;
    }

    LocationCmdInfo_t* cPtr =
        (LocationCmdInfo_t*)le_mem_ForceAlloc(gnss.CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));

    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();

    size_t pathLen = strnlen(merkleTreeFilePath,
                             sizeof(cPtr->params.injectMerkle.merkleTreeFilePath));
    if (pathLen >= sizeof(cPtr->params.injectMerkle.merkleTreeFilePath))
    {
        LE_ERROR("InjectMerkleTreeInformationByPath: path too long (%zu)", pathLen);
        taf_locGnss_InjectMerkleTreeInformationByPathRespond(cmdRef, LE_OVERFLOW);
        le_mem_Release(cPtr);
        return;
    }

    memcpy(cPtr->params.injectMerkle.merkleTreeFilePath,
            merkleTreeFilePath,
            sizeof(cPtr->params.injectMerkle.merkleTreeFilePath));

    LE_INFO("InjectMerkleTreeInformationByPath: queuing to worker, path=%s",
            cPtr->params.injectMerkle.merkleTreeFilePath);

    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef,
                                   InjectMerkleWorker, cPtr, NULL);
}

// ---------------------------------------------------------------------------
// ConfigureOsnma
// ---------------------------------------------------------------------------
static void ConfigureOsnmaSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    taf_locGnss_ConfigureOsnmaRespond(cPtr->cmdRef, cPtr->retCode);
    le_mem_Release(cPtr);
}

void taf_locGnss::ConfigureOsnmaWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_ASSERT(cmdPtr != NULL);
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;
    bool galOsnma = cPtr->params.configureOsnma.galOsnma;

    taf_locGnss_Client_t* clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ConfigureOsnmaSvcRespond, cPtr, NULL);
        return;
    }

    cPtr->retCode = LE_FAULT;

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        {
            typedef struct{
                pa_result_t result;
            }taf_SelfTestResult_t;
            auto cb = [](pa_result_t result, std::any context) {
                taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                resPtr->result = result;
            };
            taf_SelfTestResult_t resCallback = {};
            pa_result_t res = taf_pa_location_configureOsnma(galOsnma,cb,(std::any)&resCallback);
            if(res == PA_OK){
                if(resCallback.result == PA_OK)
                {
                    LE_INFO("ConfigureOsnma status PASS for: %d",galOsnma);
                    cPtr->retCode = LE_OK;
                }
            }
            else
            {
                LE_ERROR("ConfigureOsnma is failed");
                cPtr->retCode = LE_FAULT;
            }
        }
        break;
        default:
        {
            cPtr->retCode = LE_FAULT;
            LE_ERROR("Invalid GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }
    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ConfigureOsnmaSvcRespond, cPtr, NULL);
}

void taf_locGnss::ConfigureOsnma(taf_locGnss_ServerCmdRef_t cmdRef, bool galOsnma)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));
    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();
    cPtr->params.configureOsnma.galOsnma = galOsnma;
    le_event_QueueFunctionToThread(gnss.LocationWorkerThRef, ConfigureOsnmaWorker, cPtr, NULL);
}

void taf_locGnss::ReleaseMeasSampleRef
(
 taf_locGnss_MeasSampleRef_t  measSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_MeasurementSampleRequest_t* measSampleReqPtr = (taf_locGnss_MeasurementSampleRequest_t*)le_ref_Lookup(gnss.MeasurementSampleMap,
                                                                measSampleRef);

    if(measSampleReqPtr == NULL || measSampleReqPtr->measSampleNodePtr == NULL) return;

    le_ref_DeleteRef(gnss.MeasurementSampleMap,measSampleRef);
    le_mem_Release(measSampleReqPtr->measSampleNodePtr);
    le_mem_Release(measSampleReqPtr);
}

le_result_t taf_locGnss::GetMeasDataValidityMask
(
    taf_locGnss_MeasSampleRef_t measSampleRef,
    uint32_t* measDataValidityMaskPtr,
    size_t* measDataValidityMaskSizePtr
)
{
    le_result_t result = LE_OK;

    taf_locGnss_MeasurementSampleRequest_t* measSampleReqPtr
                                            = (taf_locGnss_MeasurementSampleRequest_t*)le_ref_Lookup(MeasurementSampleMap,measSampleRef);


    if(measSampleReqPtr == NULL || measSampleReqPtr->measSampleNodePtr == NULL)
    {
        LE_ERROR("measSampleReqPtr is NULL");
        return LE_FAULT;
    }

    *measDataValidityMaskSizePtr = measSampleReqPtr->measSampleNodePtr->measCount;

    LE_INFO("*measDataValidityMaskSizePtr: %d",(int)*measDataValidityMaskSizePtr);

    for (auto i = 0; i < (int) *measDataValidityMaskSizePtr; i++) {
        measDataValidityMaskPtr[i] = measSampleReqPtr->measSampleNodePtr->measData[i].valid;
    }

    return result;
}

le_result_t taf_locGnss::GetDgnssStationIds
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint16_t* stationIdsPtr,
    size_t* stationIdsSizePtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK) {
        return result;
    }

    size_t outputCount = 0;

    size_t inputSize = posSampleReqPtr->positionSampleNodePtr->dgnssStationIdsCount;

    for (size_t i = 0; i < inputSize; i++) {

        if (outputCount >= TAF_LOCGNSS_MAX_MONITOR_STATION_IDS) {
            LE_ERROR("Maximum DGNSS station IDs reached, ignoring remaining stations");
            break;
        }

        uint16_t stationId = posSampleReqPtr->positionSampleNodePtr->dgnssStationIds[i];

        if(stationId > 0 && stationId <= 1023){
            stationIdsPtr[outputCount] = stationId;
            outputCount++;
        }else{
            LE_DEBUG("Exceeded monitoring station range!!");
        }
    }

    *stationIdsSizePtr = outputCount;

    LE_INFO("Returned %zu DGNSS station IDs", *stationIdsSizePtr);

    return result;
}

static void InjectDgnssCorrectionSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    LE_DEBUG("InjectDgnssCorrectionSvcRespond: retCode=%d", cPtr->retCode);

    taf_locGnss_InjectDgnssCorrectionRespond(cPtr->cmdRef, cPtr->retCode);

    le_mem_Release(cPtr);
}

void taf_locGnss::InjectDgnssCorrectionWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    LE_ASSERT(cPtr != NULL);

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, InjectDgnssCorrectionSvcRespond, cPtr, NULL);
        return;
    }

    if ((cPtr->params.injectDgnssCorrection.correctionDataSize == 0) ||
    (cPtr->params.injectDgnssCorrection.correctionDataSize > TAF_LOCGNSS_DATA_LEN_MAX))
    {
        LE_ERROR("InjectDgnssCorrection: invalid correctionDataSize=%zu",
        cPtr->params.injectDgnssCorrection.correctionDataSize);
        cPtr->retCode = LE_BAD_PARAMETER;
        le_event_QueueFunctionToThread(
        gnss.LocationSvcThRef,
        InjectDgnssCorrectionSvcRespond,
        cPtr,
        NULL);
    return;
    }

    if(cPtr->params.injectDgnssCorrection.sourceRef != clientRequestPtr->activeSourceRef){
        LE_ERROR("InjectionData called from another client!!");
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, InjectDgnssCorrectionSvcRespond, cPtr, NULL);
        return;
    }

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
            le_event_QueueFunctionToThread(gnss.LocationSvcThRef, InjectDgnssCorrectionSvcRespond, cPtr, NULL);
            return;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            typedef struct{
                pa_result_t result;
            }taf_SelfTestResult_t;
            auto cb = [](pa_result_t result, std::any context) {
                taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                resPtr->result = result;
            };
            taf_SelfTestResult_t resCallback = {};

            LE_INFO("Buffer size: %zu bytes", cPtr->params.injectDgnssCorrection.correctionDataSize);

            pa_result_t res = taf_pa_location_injectCorrectionData(cPtr->params.injectDgnssCorrection.correctionData, static_cast<uint32_t>(cPtr->params.injectDgnssCorrection.correctionDataSize),cb,(std::any)&resCallback);
            if(res == PA_OK){
                if(resCallback.result == PA_OK)
                {
                    LE_INFO("InjectCorrectionData success!!");
                    cPtr->retCode = LE_OK;
                }
                else
                {
                    LE_ERROR("InjectCorrectionData callback failed with result: %d", resCallback.result);
                    cPtr->retCode = LE_FAULT;
                }
            }
            else
            {
                LE_ERROR("InjectCorrectionData is failed");
                cPtr->retCode = LE_FAULT;
            }
        }
        break;
        default:
        {
            cPtr->retCode = LE_FAULT;
            LE_ERROR("Invalid GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }

    LE_INFO("InjectDgnssCorrectionWorker: sourceRef=%p size=%zu retCode=%d",
            cPtr->params.injectDgnssCorrection.sourceRef,
            cPtr->params.injectDgnssCorrection.correctionDataSize,
            cPtr->retCode);

    le_event_QueueFunctionToThread(
        gnss.LocationSvcThRef, InjectDgnssCorrectionSvcRespond, cPtr, NULL);
}

void taf_locGnss::InjectDgnssCorrection
(
    taf_locGnss_ServerCmdRef_t   cmdRef,
    taf_locGnss_DgnssSourceRef_t sourceRef,
    const uint8_t*               correctionDataPtr,
    size_t                       correctionDataSize
)
{
    auto &gnss = taf_locGnss::GetInstance();

    if (correctionDataPtr == NULL)
    {
        LE_ERROR("taf_locGnss_InjectDgnssCorrection: correctionDataPtr is NULL");
        taf_locGnss_InjectDgnssCorrectionRespond(cmdRef, LE_BAD_PARAMETER);
        return;
    }

    if (correctionDataSize == 0 || correctionDataSize > TAF_LOCGNSS_DATA_LEN_MAX)
    {
        LE_ERROR("taf_locGnss_InjectDgnssCorrection: correctionDataSize=%zu exceeds max=%zu",
                 correctionDataSize, (size_t)TAF_LOCGNSS_DATA_LEN_MAX);
        taf_locGnss_InjectDgnssCorrectionRespond(cmdRef, LE_OVERFLOW);
        return;
    }

    LocationCmdInfo_t* cPtr =
        (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));

    cPtr->cmdRef     = cmdRef;
    cPtr->sessionRef = taf_locGnss_GetClientSessionRef();

    cPtr->params.injectDgnssCorrection.sourceRef          = sourceRef;
    cPtr->params.injectDgnssCorrection.correctionDataSize = correctionDataSize;

    memcpy(cPtr->params.injectDgnssCorrection.correctionData,
           correctionDataPtr,
           correctionDataSize);

    le_event_QueueFunctionToThread(
        gnss.LocationWorkerThRef, InjectDgnssCorrectionWorker, cPtr, NULL);
}

static void CreateDgnssSourceSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    LE_DEBUG("CreateDgnssSourceSvcRespond: sourceRef=%p retCode=%d",
             cPtr->params.createDgnssSource.sourceRef, cPtr->retCode);

    taf_locGnss_CreateDgnssSourceRespond(
        cPtr->cmdRef,
        cPtr->params.createDgnssSource.sourceRef);

    le_mem_Release(cPtr);
}

void taf_locGnss::CreateDgnssSourceWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    LE_ASSERT(cPtr != NULL);

    LE_INFO("CreateDgnssSource!!");
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    size_t count = 0;

    clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, CreateDgnssSourceSvcRespond, cPtr, NULL);
        return;
    }

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_FAULT;
            le_event_QueueFunctionToThread(gnss.LocationSvcThRef, CreateDgnssSourceSvcRespond, cPtr, NULL);
            return;
        }
        break;
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_READY:
        {
            typedef struct{
                    pa_result_t result;
            }taf_SelfTestResult_t;
            auto cb = [](pa_result_t result, std::any context) {
                taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                resPtr->result = result;
            };

            taf_SelfTestResult_t resCallback = {};
            if(clientRequestPtr->activeSourceRef == NULL && clientRequestPtr->activeDgnssFormat == TAF_LOCGNSS_DGNSS_FORMAT_UNKNOWN){
                le_ref_IterRef_t iter = le_ref_GetIterator(gnss.DgnssSourceRefMap);
                while (le_ref_NextNode(iter) == LE_OK)
                {
                    count++;
                }
                LE_INFO("DgnssManagercount: %zu",count);
                if(count == 0){
                    pa_result_t res = taf_pa_location_initializeDgnss((taf_pa_location_DgnssDataFormat_t)cPtr->params.createDgnssSource.dgnssDataFormat,cb,(std::any)&resCallback);
                    if(res != PA_OK)
                    {
                        LE_ERROR("DgnssManager failed in PA layer!!");
                    }
                    else
                    {
                        if(resCallback.result == PA_OK)
                        {
                            LE_INFO("DgnssManager init success");
                            taf_locGnss_DgnssSource_t* gnssSource = (taf_locGnss_DgnssSource_t*)
                                le_mem_ForceAlloc(gnss.DgnssSourcePoolRef);
                            gnssSource->sessionRef = clientRequestPtr->sessionRef;
                            gnssSource->format    = cPtr->params.createDgnssSource.dgnssDataFormat;
                            gnssSource->sourceRef = (taf_locGnss_DgnssSourceRef_t) le_ref_CreateRef(gnss.DgnssSourceRefMap, gnssSource);
                            clientRequestPtr->activeDgnssFormat = cPtr->params.createDgnssSource.dgnssDataFormat;
                            clientRequestPtr->activeSourceRef = gnssSource->sourceRef;
                            cPtr->params.createDgnssSource.sourceRef = gnssSource->sourceRef;
                        }
                        else
                        {
                            LE_ERROR("DgnssManager init failed");
                        }
                    }
                    gnss.dgnssListener.onDgnssStatusUpdate = &Handler::onDgnssStatusUpdate;
                    if(taf_pa_location_registerDgnssEventListener(&gnss.dgnssListener, std::any(clientRequestPtr->sessionRef)) !=  PA_OK){
                        LE_ERROR("Dgnss Listener register failed");
                    }else{
                        LE_INFO("Dgnss Listener registered!!");
                    }
                }else{
                    LE_INFO("Already one client acquired Dgnss Manager!!");
                    cPtr->retCode = LE_FAULT;
                    cPtr->params.createDgnssSource.sourceRef = NULL;
                    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, CreateDgnssSourceSvcRespond, cPtr, NULL);
                    return;
                }
            }else if(clientRequestPtr->activeSourceRef != NULL && clientRequestPtr->activeDgnssFormat == cPtr->params.createDgnssSource.dgnssDataFormat){
                LE_INFO("Reference already created for this format!!");
                cPtr->retCode = LE_OK;
                cPtr->params.createDgnssSource.sourceRef = clientRequestPtr->activeSourceRef;
                le_event_QueueFunctionToThread(gnss.LocationSvcThRef, CreateDgnssSourceSvcRespond, cPtr, NULL);
                return;
            }else if(clientRequestPtr->activeSourceRef != NULL && clientRequestPtr->activeDgnssFormat != cPtr->params.createDgnssSource.dgnssDataFormat){
                le_result_t releaseResult = gnss.InternalReleaseDgnssSourceOnWorker(clientRequestPtr, clientRequestPtr->activeSourceRef); 
                if(releaseResult == LE_OK){
                    LE_INFO("Release Success for previous source!!");
                }else{
                    LE_ERROR("Release source failed!!");
                }

                pa_result_t res = taf_pa_location_initializeDgnss((taf_pa_location_DgnssDataFormat_t)cPtr->params.createDgnssSource.dgnssDataFormat,cb,(std::any)&resCallback);
                if(res != PA_OK)
                {
                    LE_ERROR("DgnssManager failed in PA layer!!");
                }
                else
                {
                    if(resCallback.result == PA_OK)
                    {
                        LE_INFO("DgnssManager init success");
                    }
                    else
                    {
                        LE_ERROR("DgnssManager init failed");
                    }
                }

                pa_result_t res1 = taf_pa_location_createDgnssSource((taf_pa_location_DgnssDataFormat_t)cPtr->params.createDgnssSource.dgnssDataFormat,cb,(std::any)&resCallback);
                if(res1 != PA_OK)
                {
                    LE_ERROR("CreateDgnssSource failed in PA layer!!");
                }
                else
                {
                    if(resCallback.result == PA_OK)
                    {
                        LE_INFO("CreateSource is success");
                        taf_locGnss_DgnssSource_t* gnssSource = (taf_locGnss_DgnssSource_t*)
                            le_mem_ForceAlloc(gnss.DgnssSourcePoolRef);
                        gnssSource->sessionRef = clientRequestPtr->sessionRef;
                        gnssSource->format    = cPtr->params.createDgnssSource.dgnssDataFormat;
                        gnssSource->sourceRef = (taf_locGnss_DgnssSourceRef_t) le_ref_CreateRef(gnss.DgnssSourceRefMap, gnssSource);
                        clientRequestPtr->activeDgnssFormat = cPtr->params.createDgnssSource.dgnssDataFormat;
                        clientRequestPtr->activeSourceRef = gnssSource->sourceRef;
                        cPtr->params.createDgnssSource.sourceRef = gnssSource->sourceRef;
                    }
                    else
                    {
                        LE_ERROR("CreateSource is failed");
                    }
                }
                gnss.dgnssListener.onDgnssStatusUpdate = &Handler::onDgnssStatusUpdate;
                if(taf_pa_location_registerDgnssEventListener(&gnss.dgnssListener, std::any(clientRequestPtr->sessionRef)) !=  PA_OK){
                    LE_ERROR("Dgnss Listener register failed");
                }else{
                    LE_INFO("Dgnss Listener registered!!");
                }
            }
        }
        break;
        default:
        {
            LE_ERROR("Invalid GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }

    cPtr->retCode = (cPtr->params.createDgnssSource.sourceRef != NULL) ? LE_OK : LE_FAULT;

    LE_INFO("CreateDgnssSourceWorker: sourceRef=%p retCode=%d",
            cPtr->params.createDgnssSource.sourceRef, cPtr->retCode);

    le_event_QueueFunctionToThread(
        gnss.LocationSvcThRef, CreateDgnssSourceSvcRespond, cPtr, NULL);
}

void taf_locGnss::CreateDgnssSource
(
    taf_locGnss_ServerCmdRef_t  cmdRef,
    taf_locGnss_DgnssFormat_t   dgnssDataFormat
)
{
    auto &gnss = taf_locGnss::GetInstance();

    LocationCmdInfo_t* cPtr =
        (LocationCmdInfo_t*)le_mem_ForceAlloc(gnss.CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));

    cPtr->cmdRef                                  = cmdRef;
    cPtr->sessionRef                              = taf_locGnss_GetClientSessionRef();
    cPtr->params.createDgnssSource.dgnssDataFormat = dgnssDataFormat;
    cPtr->params.createDgnssSource.sourceRef       = NULL;

    le_event_QueueFunctionToThread(
        gnss.LocationWorkerThRef, CreateDgnssSourceWorker, cPtr, NULL);
}

le_result_t taf_locGnss::InternalReleaseDgnssSourceOnWorker
(
    taf_locGnss_Client_t*         clientRequestPtr,
    taf_locGnss_DgnssSourceRef_t  sourceRef
)
{
    auto &gnss = taf_locGnss::GetInstance();

    typedef struct { pa_result_t result; } taf_SelfTestResult_t;

    taf_SelfTestResult_t resCallback = {};
    auto cb = [](pa_result_t result, std::any context)
    {
        taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
        resPtr->result = result;
    };

    pa_result_t res = taf_pa_location_releaseDgnssSource(cb, (std::any)&resCallback);
    if (res != PA_OK)
    {
        LE_ERROR("InternalRelease: taf_pa_location_releaseDgnssSource returned error");
        return LE_FAULT;
    }
    if (resCallback.result != PA_OK)
    {
        LE_ERROR("InternalRelease: releaseDgnssSource PA callback failed");
        return LE_FAULT;
    }

    if (sourceRef != NULL)
    {
        taf_locGnss_DgnssSource_t* dgnssSource =
            (taf_locGnss_DgnssSource_t*)le_ref_Lookup(gnss.DgnssSourceRefMap, sourceRef);

        if (!dgnssSource)
        {
            LE_ERROR("InternalRelease: DgnssSource not found in ref-map");
            return LE_NOT_FOUND;
        }

        if (dgnssSource->sourceRef != sourceRef ||
            clientRequestPtr->sessionRef != dgnssSource->sessionRef)
        {
            LE_ERROR("InternalRelease: sourceRef/session mismatch — "
                     "expected session=%p got=%p",
                     clientRequestPtr->sessionRef, dgnssSource->sessionRef);
            return LE_FAULT;
        }

        le_ref_DeleteRef(gnss.DgnssSourceRefMap, sourceRef);
        le_mem_Release(dgnssSource);
    }

    clientRequestPtr->activeSourceRef   = NULL;
    clientRequestPtr->activeDgnssFormat = TAF_LOCGNSS_DGNSS_FORMAT_UNKNOWN;
    LE_INFO("InternalRelease: source ref cleaned up and client state reset");

    if (taf_pa_location_deregisterDgnssEventListener(
            std::any(clientRequestPtr->sessionRef)) != PA_OK)
    {
        LE_ERROR("InternalRelease: deregisterDgnssEventListener failed");
    }
    else
    {
        LE_INFO("InternalRelease: DGNSS listener deregistered");
    }

    taf_SelfTestResult_t resCallback1 = {};
    auto cb1 = [](pa_result_t result, std::any context)
    {
        taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
        resPtr->result = result;
    };

    pa_result_t deInitRes = taf_pa_location_deInitializeDgnss(cb1, (std::any)&resCallback1);
    if (deInitRes != PA_OK || resCallback1.result != PA_OK)
    {
        LE_ERROR("InternalRelease: deInitializeDgnss failed (res=%d cbResult=%d)",
                 deInitRes, resCallback1.result);
        return LE_FAULT;
    }

    LE_INFO("InternalRelease: DGNSS manager de-initialised successfully");
    return LE_OK;
}


static void ReleaseDgnssSourceSvcRespond(void* cmdPtr, void*)
{
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    LE_DEBUG("ReleaseDgnssSourceSvcRespond: retCode=%d", cPtr->retCode);

    taf_locGnss_ReleaseDgnssSourceRespond(cPtr->cmdRef, cPtr->retCode);

    le_mem_Release(cPtr);
}

void taf_locGnss::ReleaseDgnssSourceWorker(void* cmdPtr, void*)
{
    auto &gnss = taf_locGnss::GetInstance();
    LocationCmdInfo_t* cPtr = (LocationCmdInfo_t*)cmdPtr;

    LE_ASSERT(cPtr != NULL);

    taf_locGnss_Client_t* clientRequestPtr = NULL;

    clientRequestPtr = gnss.DiscoverSessionRef(cPtr->sessionRef);
    if (NULL == clientRequestPtr)
    {
        cPtr->retCode = LE_FAULT;
        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ReleaseDgnssSourceSvcRespond, cPtr, NULL);
        return;
    }

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", clientRequestPtr->GnssState);
            cPtr->retCode = LE_NOT_PERMITTED;
            le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ReleaseDgnssSourceSvcRespond, cPtr, NULL);
            return;
        }
        break;
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_READY:
        {
            if(clientRequestPtr->activeSourceRef != NULL && clientRequestPtr->activeDgnssFormat != TAF_LOCGNSS_DGNSS_FORMAT_UNKNOWN){
                typedef struct{
                    pa_result_t result;
                }taf_SelfTestResult_t;
                auto cb = [](pa_result_t result, std::any context) {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                };

                taf_SelfTestResult_t resCallback = {};
                pa_result_t res = taf_pa_location_releaseDgnssSource(cb,(std::any)&resCallback);
                if(res != PA_OK)
                {
                    cPtr->retCode = LE_FAULT;
                    le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ReleaseDgnssSourceSvcRespond, cPtr, NULL);
                    return;
                }
                else
                {
                    if(resCallback.result == PA_OK)
                    {
                        LE_INFO("ReleaseSource is success");
                        if(cPtr->params.releaseDgnssSource.sourceRef != NULL)
                        {
                            taf_locGnss_DgnssSource_t* dgnssSource = (taf_locGnss_DgnssSource_t*)le_ref_Lookup(gnss.DgnssSourceRefMap, cPtr->params.releaseDgnssSource.sourceRef);

                            if (!dgnssSource){
                                LE_ERROR("DgnssSource not found!!");
                                cPtr->retCode = LE_NOT_FOUND;
                                le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ReleaseDgnssSourceSvcRespond, cPtr, NULL);
                                return;
                            }

                            if (dgnssSource->sourceRef == cPtr->params.releaseDgnssSource.sourceRef && clientRequestPtr->sessionRef == dgnssSource->sessionRef){
                                LE_INFO("dgnssSource released");
                                clientRequestPtr->activeSourceRef = NULL;
                                clientRequestPtr->activeDgnssFormat = TAF_LOCGNSS_DGNSS_FORMAT_UNKNOWN;
                                le_ref_DeleteRef(gnss.DgnssSourceRefMap, cPtr->params.releaseDgnssSource.sourceRef);
                                le_mem_Release(dgnssSource);
                            }else{
                                LE_ERROR("Session mismatch!!");
                                cPtr->retCode = LE_FAULT;
                                le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ReleaseDgnssSourceSvcRespond, cPtr, NULL);
                                return;
                            }
                        }
                        cPtr->retCode = LE_OK;
                    }
                    else
                    {
                        LE_DEBUG("ReleaseSource is failed");
                        cPtr->retCode = LE_FAULT;
                        le_event_QueueFunctionToThread(gnss.LocationSvcThRef, ReleaseDgnssSourceSvcRespond, cPtr, NULL);
                        return;
                    }
                }

                if(taf_pa_location_deregisterDgnssEventListener(std::any(clientRequestPtr->sessionRef)) !=  PA_OK){
                    LE_ERROR("Dgnss Listener DeRegister failed");
                }else{
                    LE_INFO("Dgnss Listener DeRegistered!!");
                }

                auto cb1 = [](pa_result_t result, std::any context) {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                };
                taf_SelfTestResult_t resCallback1 = {};
                pa_result_t deInit_res = taf_pa_location_deInitializeDgnss(cb1,(std::any)&resCallback1);
                if(deInit_res != PA_OK)
                {
                    cPtr->retCode = LE_FAULT;
                }
                else
                {
                    if(resCallback1.result == PA_OK)
                    {
                        LE_INFO("Dgnss Manager removed created for this source!!");
                        cPtr->retCode = LE_OK;
                    }
                    else
                    {
                        LE_DEBUG("Dgnss Manager removal failed");
                        cPtr->retCode = LE_FAULT;
                    }
                }
            }else{
                LE_INFO("Already source deleted for this client!!");
                cPtr->retCode = LE_OK;
            }
        }
        break;
        default:
        {
            cPtr->retCode = LE_FAULT;
            LE_ERROR("Invalid GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }

    LE_INFO("ReleaseDgnssSourceWorker: sourceRef=%p retCode=%d",
            cPtr->params.releaseDgnssSource.sourceRef, cPtr->retCode);

    le_event_QueueFunctionToThread(
        gnss.LocationSvcThRef, ReleaseDgnssSourceSvcRespond, cPtr, NULL);
}

void taf_locGnss::ReleaseDgnssSource
(
    taf_locGnss_ServerCmdRef_t   cmdRef,
    taf_locGnss_DgnssSourceRef_t sourceRef
)
{
    auto &gnss = taf_locGnss::GetInstance();

    LocationCmdInfo_t* cPtr =
        (LocationCmdInfo_t*)le_mem_ForceAlloc(CmdLocationPoolRef);
    memset(cPtr, 0, sizeof(*cPtr));

    cPtr->cmdRef                               = cmdRef;
    cPtr->sessionRef                           = taf_locGnss_GetClientSessionRef();
    cPtr->params.releaseDgnssSource.sourceRef  = sourceRef;

    le_event_QueueFunctionToThread(
        gnss.LocationWorkerThRef, ReleaseDgnssSourceWorker, cPtr, NULL);
}

le_result_t taf_locGnss::SetEngineIntegrityRisk
(
    taf_locGnss_EngineType_t engineType,
    uint32_t integrityRisk
)
{
    le_result_t result = LE_FAULT;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
             LE_ERROR("Wrong Gnss State [%d]", clientRequestPtr->GnssState);
             result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_READY:
        {
            typedef struct{
                pa_result_t result;
            }taf_SelfTestResult_t;
            auto cb = [](pa_result_t result, std::any context) {
                taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                resPtr->result = result;
            };
            taf_SelfTestResult_t resCallback = {};
            pa_result_t res = taf_pa_location_configureEngineIntegrityRisk((taf_pa_location_EngineType_t)engineType,integrityRisk,cb,(std::any)&resCallback);
            if(res == PA_OK){
                if(resCallback.result == PA_OK)
                {
                    LE_INFO("SetEngineIntegrityRisk is success");
                    return LE_OK;
                }
            }
            else
            {
                LE_INFO("SetEngineIntegrityRisk is failed");
                result = LE_FAULT;
            }
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Invalid GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }
   return result;
}

le_result_t taf_locGnss::GetProtectionLevels
(
    taf_locGnss_SampleRef_t positionSampleRef,
    double* protectionLevelAlongTrackPtr,
    double* protectionLevelCrossTrackPtr,
    double* protectionLevelVerticalPtr
)
{
    le_result_t result = LE_OK;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr
        = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if ((protectionLevelAlongTrackPtr == NULL) &&
        (protectionLevelCrossTrackPtr == NULL) &&
        (protectionLevelVerticalPtr == NULL))
    {
        return LE_FAULT;
    }

    if (protectionLevelAlongTrackPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->protectionLevelAlongTrackValid)
        {
            *protectionLevelAlongTrackPtr =
                posSampleReqPtr->positionSampleNodePtr->protectionLevelAlongTrack;
        }
        else
        {
            *protectionLevelAlongTrackPtr = NAN;
            LE_DEBUG("GetProtectionLevels: along-track PL invalid");
        }
    }

    if (protectionLevelCrossTrackPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->protectionLevelCrossTrackValid)
        {
            *protectionLevelCrossTrackPtr =
                posSampleReqPtr->positionSampleNodePtr->protectionLevelCrossTrack;
        }
        else
        {
            *protectionLevelCrossTrackPtr = NAN;
            LE_DEBUG("GetProtectionLevels: cross-track PL invalid");
        }
    }

    if (protectionLevelVerticalPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->protectionLevelVerticalValid)
        {
            *protectionLevelVerticalPtr =
                posSampleReqPtr->positionSampleNodePtr->protectionLevelVertical;
        }
        else
        {
            *protectionLevelVerticalPtr = NAN;
            LE_DEBUG("GetProtectionLevels: vertical PL invalid");
        }
    }
    return result;
}

le_result_t taf_locGnss::GetBaselineLength
(
    taf_locGnss_SampleRef_t positionSampleRef,
    double* baselineLengthPtr
)
{
    le_result_t result = LE_FAULT;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (baselineLengthPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->baselineLengthValid)
        {
            *baselineLengthPtr = posSampleReqPtr->positionSampleNodePtr->baselineLength;
            return LE_OK;
        }
    }
    return result;
}

le_result_t taf_locGnss::GetAgeOfCorrections
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint64_t* ageCorrectionsPtr
)
{
    le_result_t result = LE_FAULT;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if(ageCorrectionsPtr){
        if (posSampleReqPtr->positionSampleNodePtr->ageCorrectionsValid)
        {
            *ageCorrectionsPtr = posSampleReqPtr->positionSampleNodePtr->ageCorrections;
            return LE_OK;
        }
    }
    return result;
}

le_result_t taf_locGnss::GetIntegrityRiskUsed
(
    taf_locGnss_SampleRef_t positionSampleRef,
    uint32_t* integrityRiskUsedPtr
)
{
    le_result_t result = LE_FAULT;
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (integrityRiskUsedPtr)
    {
        if (posSampleReqPtr->positionSampleNodePtr->integrityRiskUsedValid)
        {
            *integrityRiskUsedPtr = posSampleReqPtr->positionSampleNodePtr->integrityRiskUsed;
            return LE_OK;
        }
        else
        {
            LE_DEBUG("GetIntegrityRiskUsed Invalid");
        }
    }
    return result;
}

taf_locGnss_DgnssStatusChangeHandlerRef_t taf_locGnss::AddDgnssStatusChangeHandler
(
 taf_locGnss_DgnssStatusChangeHandlerFunc_t handlerPtr,
 void* contextPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_DgnssStatusChangeHandler_t*  dgnssStatusHandlerPtr =
        (taf_locGnss_DgnssStatusChangeHandler_t*)le_mem_ForceAlloc(gnss.DgnssStatusHandlerPoolRef);
    memset(dgnssStatusHandlerPtr, 0, sizeof(taf_locGnss_DgnssStatusChangeHandler_t));
    dgnssStatusHandlerPtr->next = LE_DLS_LINK_INIT;
    dgnssStatusHandlerPtr->handlerFuncPtr = handlerPtr;
    dgnssStatusHandlerPtr->handlerContextPtr = contextPtr;
    dgnssStatusHandlerPtr->sessionRef = taf_locGnss_GetClientSessionRef();

    LE_DEBUG("AddDgnssStatusChangeHandler() sessionRef: %p", dgnssStatusHandlerPtr->sessionRef);

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, NULL, "clientRequestPtr is NULL");

    dgnssStatusHandlerPtr->handlerRef =
        (taf_locGnss_DgnssStatusChangeHandlerRef_t)le_ref_CreateRef(gnss.DgnssStatusHandlerRefMap, dgnssStatusHandlerPtr);

    NumOfDgnssStatusHandlers++;

    LE_DEBUG("Created dgnssStatusHandlerRef(%p) for dgnssStatusHandlerPtr(%p) (totalCnt=0x%x).",
        dgnssStatusHandlerPtr->handlerRef, dgnssStatusHandlerPtr, NumOfDgnssStatusHandlers);

    return dgnssStatusHandlerPtr->handlerRef;
}

void taf_locGnss::RemoveDgnssStatusChangeHandler
(
    taf_locGnss_DgnssStatusChangeHandlerRef_t handlerRef
)
{
    LE_INFO("RemoveDgnssStatusChangeHandler!!");
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_DgnssStatusChangeHandler_t* dgnssStatusHandlerPtr =
        (taf_locGnss_DgnssStatusChangeHandler_t*)le_ref_Lookup(gnss.DgnssStatusHandlerRefMap, handlerRef);

    if (dgnssStatusHandlerPtr != NULL)
    {
        if(dgnssStatusHandlerPtr->handlerRef != handlerRef) {
            return;
        }
        NumOfDgnssStatusHandlers--;

        LE_DEBUG("Removed dgnssStatusHandlerRef(%p) for dgnssStatusHandlerPtr(%p) (totalCnt=0x%x).",
                 dgnssStatusHandlerPtr->handlerRef, dgnssStatusHandlerPtr, NumOfDgnssStatusHandlers);
        le_ref_DeleteRef(gnss.DgnssStatusHandlerRefMap, handlerRef);
        le_mem_Release(dgnssStatusHandlerPtr);
    }
    else
    {
        LE_ERROR("Invaild handlerRef(%p).", handlerRef);
    }
}

void taf_locGnss::DgnssStatusHandler
(
 void* reportPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_DgnssStatusChangeHandler_t*  dgnssStatusHandlerPtr;
    DgnssStatusEvent_t* currentPosPtr = (DgnssStatusEvent_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL( currentPosPtr == NULL, "currentPosPtr is Null");

    LE_DEBUG("Handler Function called with position %p", currentPosPtr);

    taf_locGnss_Client_t* clientRequestPtr = NULL;

    clientRequestPtr = gnss.DiscoverSessionRef(*currentPosPtr->clientSessionRefPtr);

    TAF_ERROR_IF_RET_NIL(NULL == clientRequestPtr, "clientRequestPtr is NULL!!");

    if(!gnss.NumOfDgnssStatusHandlers)
    {
        LE_DEBUG("No NumOfDgnssStatusHandlers handlers, exit Handler Function");
        le_mem_Release(currentPosPtr);
        return;
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(gnss.DgnssStatusHandlerRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        dgnssStatusHandlerPtr = (taf_locGnss_DgnssStatusChangeHandler_t*)le_ref_GetValue(iterRef);
        if(dgnssStatusHandlerPtr == NULL) {
            return;
        }

        if (dgnssStatusHandlerPtr->sessionRef == *currentPosPtr->clientSessionRefPtr) {
            LE_INFO("Report dgnssRef %p to the corresponding handler (handlerPtr %p)",
                clientRequestPtr->activeSourceRef, dgnssStatusHandlerPtr->handlerFuncPtr);

            dgnssStatusHandlerPtr->handlerFuncPtr(clientRequestPtr->activeSourceRef, currentPosPtr->dgnssStatus,
                                        dgnssStatusHandlerPtr->handlerContextPtr);
        }else{
            LE_DEBUG("DgnssStatusHandler session ref does not match! ReqPtr.sessionRef: %p, Sample.sessionRef: %p", dgnssStatusHandlerPtr->sessionRef, *currentPosPtr->clientSessionRefPtr);
        }
    }

    le_mem_Release(currentPosPtr);
}

void taf_locGnss::RemovePositionHandler
(
    taf_locGnss_PositionHandlerRef_t handlerRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_PositionHandler_t* positionHandlerPtr =
        (taf_locGnss_PositionHandler_t*)le_ref_Lookup(gnss.PositionHandlerRefMap, handlerRef);

    if (positionHandlerPtr != NULL)
    {
        if(positionHandlerPtr->handlerRef != handlerRef) {
            return;
        }
        NumOfPositionHandlers--;

        LE_DEBUG("Removed positionHandlerRef(%p) for positionHandlerPtr(%p) (totalCnt=0x%x).",
                 positionHandlerPtr->handlerRef, positionHandlerPtr, NumOfPositionHandlers);
        le_ref_DeleteRef(gnss.PositionHandlerRefMap, handlerRef);
        le_mem_Release(positionHandlerPtr);
    }
    else
    {
        LE_ERROR("Invaild handlerRef(%p).", handlerRef);
    }
}

void taf_locGnss::RemovePositionExHandler
(
    taf_locGnss_PositionExHandlerRef_t handlerRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_PositionExHandler_t* positionExHandlerPtr =
        (taf_locGnss_PositionExHandler_t*)le_ref_Lookup(gnss.PositionExHandlerRefMap, handlerRef);

    if (positionExHandlerPtr != NULL)
    {
        if(positionExHandlerPtr->handlerRef != handlerRef) {
            return;
        }
        NumOfPositionExHandlers--;

        LE_INFO("Removed positionExHandlerRef(%p) for positionExHandlerPtr(%p) (totalCnt=0x%x).",
                 positionExHandlerPtr->handlerRef, positionExHandlerPtr, NumOfPositionExHandlers);
        le_mem_Release(positionExHandlerPtr);
        le_ref_DeleteRef(gnss.PositionExHandlerRefMap, handlerRef);
    }
    else
    {
        LE_ERROR("Invaild handlerRef(%p).", handlerRef);
    }
}

void taf_locGnss::RemoveMeasurementHandler
(
    taf_locGnss_MeasurementHandlerRef_t handlerRef
)
{
    LE_INFO("RemoveMeasurementHandler");

    auto &gnss = taf_locGnss::GetInstance();

    taf_locGnss_MeasurementHandler_t* measHandlerPtr =
        (taf_locGnss_MeasurementHandler_t*)le_ref_Lookup(gnss.MeasurementHandlerRefMap, handlerRef);

    if (measHandlerPtr != NULL)
    {
        if(measHandlerPtr->handlerRef != handlerRef) {
            return;
        }
        NumOfMeasurementHandlers--;

        LE_INFO("Removed positionHandlerRef(%p) for measHandlerPtr(%p) (totalCnt=0x%x).",
                 measHandlerPtr->handlerRef, measHandlerPtr, NumOfMeasurementHandlers);
        le_mem_Release(measHandlerPtr);
        le_ref_DeleteRef(gnss.MeasurementHandlerRefMap, handlerRef);
    }
    else
    {
        LE_ERROR("HandlerRef(%p) not found in active session.", handlerRef);
    }
}

void taf_locGnss::ReleaseClientRef
(
    void* RefPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    void* clientPosPtr = le_ref_Lookup(gnss.ClientRequestRefMap, RefPtr);
    if (NULL == clientPosPtr)
    {
        LE_ERROR("Invalid positioning service activation reference %p", RefPtr);
    }
    else
    {
        le_ref_DeleteRef(gnss.ClientRequestRefMap, RefPtr);
        LE_INFO("Remove Client Position Ctrl (%p)",RefPtr);
        le_mem_Release(clientPosPtr);
    }
}

void taf_locGnss::ReleaseSampleRef
(
 taf_locGnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_PositionSampleRequest_t* posSampleReqPtr = (taf_locGnss_PositionSampleRequest_t*)le_ref_Lookup(gnss.PositionSampleMap,
                                                                positionSampleRef);

    le_result_t result = gnss.CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return;
    }

    le_ref_DeleteRef(gnss.PositionSampleMap,positionSampleRef);
    le_mem_Release(posSampleReqPtr->positionSampleNodePtr);
    le_mem_Release(posSampleReqPtr);
}

void taf_locGnss::ReleaseSampleExRef
(
 taf_locGnss_SampleExRef_t postitionSampleExRef
)
{
    LE_INFO("Sample Reference deleted!!");
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_PositionSampleEx_t* posSampleReqPtr =
            (taf_locGnss_PositionSampleEx_t*)le_ref_Lookup(gnss.PositionExSampleMap,
                    postitionSampleExRef);

    if (posSampleReqPtr == NULL)
    {
        return;
    }

    le_ref_DeleteRef(gnss.PositionExSampleMap,postitionSampleExRef);
    le_mem_Release(posSampleReqPtr);
}

void taf_locGnss::OpenEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    LE_INFO("OnClientConnection sessionRef: %p", sessionRef);

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_NIL(NULL == clientRequestPtr, "OnClientConnection clientRequestPtr is NULL");
    return;
}

void taf_locGnss::CloseEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    auto &gnss = taf_locGnss::GetInstance();
    LE_INFO("SessionRef (%p) has been closed", sessionRef);

    TAF_ERROR_IF_RET_NIL( sessionRef == NULL, "sessionRef is NULL");

    if (gnss.GetState() == TAF_LOCGNSS_STATE_ACTIVE) {
        gnss.Stop(NULL);
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(gnss.PositionSampleMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        taf_locGnss_PositionSampleRequest_t *positionSampleRequestPtr =
                                (taf_locGnss_PositionSampleRequest_t*)le_ref_GetValue(iterRef);
        taf_locGnss_SampleRef_t safeRef = (taf_locGnss_SampleRef_t)le_ref_GetSafeRef(iterRef);

        result = le_ref_NextNode(iterRef);

        if (positionSampleRequestPtr != NULL && positionSampleRequestPtr->sessionRef == sessionRef)
        {
            LE_DEBUG("Release taf_locGnss_ReleaseSampleRef 0x%p, Session 0x%p", safeRef, sessionRef);
            le_ref_DeleteRef(gnss.PositionSampleMap, safeRef);
            if (positionSampleRequestPtr->positionSampleNodePtr) le_mem_Release(positionSampleRequestPtr->positionSampleNodePtr);
            le_mem_Release(positionSampleRequestPtr);
        }
    }

    iterRef = le_ref_GetIterator(gnss.ClientRequestRefMap);
    result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        taf_locGnss_Client_t* gnssPtr = (taf_locGnss_Client_t*) le_ref_GetValue(iterRef);
        void* safeRefPtr = (void*)le_ref_GetSafeRef(iterRef);

        result = le_ref_NextNode(iterRef);

        if (gnssPtr != NULL && sessionRef == gnssPtr->sessionRef)
        {
            if (gnss.mClientRefCount > 0)
            {
                //External Client release, decrement client ref count
                gnss.mClientRefCount--;
                LE_INFO("CloseEventHandler client count: %d",gnss.mClientRefCount);
            }

            if(gnssPtr->activeSourceRef != NULL && gnssPtr->activeDgnssFormat != 0){
                if(taf_pa_location_deregisterDgnssEventListener(std::any(gnssPtr->sessionRef)) !=  PA_OK){
                    LE_ERROR("Dgnss Listener DeRegister failed");
                }else{
                    LE_INFO("Dgnss Listener DeRegistered!!");
                }

                typedef struct{
                    pa_result_t result;
                }taf_SelfTestResult_t;
                auto cb = [](pa_result_t result, std::any context) {
                    taf_SelfTestResult_t* resPtr = std::any_cast<taf_SelfTestResult_t*>(context);
                    resPtr->result = result;
                };
                taf_SelfTestResult_t resCallback = {};
                pa_result_t deInit_res = taf_pa_location_deInitializeDgnss(cb,(std::any)&resCallback);
                if(deInit_res != PA_OK)
                {
                    result = LE_FAULT;
                }
                else
                {
                    if(resCallback.result == PA_OK)
                    {
                        LE_INFO("Dgnss Manager removed created for this source!!");
                    }
                    else
                    {
                        LE_ERROR("Dgnss Manager removal failed");
                    }
                }
            }

            gnss.CleanUp(gnssPtr);

            if (gnss.PositionHandlerRefMap)
            {
                le_ref_IterRef_t handlerIter = le_ref_GetIterator(gnss.PositionHandlerRefMap);
                le_result_t handlerRes = le_ref_NextNode(handlerIter);

                while (LE_OK == handlerRes)
                {
                    taf_locGnss_PositionHandler_t* handlerPtr =
                        (taf_locGnss_PositionHandler_t*)le_ref_GetValue(handlerIter);

                    void* safeRef = (void*)le_ref_GetSafeRef(handlerIter);
                    handlerRes = le_ref_NextNode(handlerIter);

                    if (handlerPtr) {
                        LE_DEBUG("Force releasing memory for handlerRef %p, safeRef %p",
                            handlerPtr->handlerRef, safeRef);

                        if (gnss.NumOfPositionHandlers > 0) {
                            gnss.NumOfPositionHandlers--;
                        } else {
                            LE_WARN("NumOfPositionHandlers already at 0, potential double cleanup");
                        }

                        le_ref_DeleteRef(gnss.PositionHandlerRefMap, safeRef);
                        le_mem_Release(handlerPtr);
                    }
                }
            }
            LE_DEBUG("Release taf_locGnss_ReleaseClientRef 0x%p, Session 0x%p",
                     safeRefPtr, gnssPtr->sessionRef);

            gnss.ReleaseClientRef(gnssPtr->clientRefPtr);
        }
    }

    iterRef = le_ref_GetIterator(gnss.DgnssSourceRefMap);
    result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        taf_locGnss_DgnssSource_t *dgnssSourcePtr =
                                (taf_locGnss_DgnssSource_t*)le_ref_GetValue(iterRef);
        if(dgnssSourcePtr == NULL) {
            return;
        }

        if (dgnssSourcePtr->sessionRef == sessionRef)
        {
            taf_locGnss_DgnssSourceRef_t safeRef = (taf_locGnss_DgnssSourceRef_t)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release taf_locGnss_ReleasDgnssRef 0x%p, Session 0x%p", safeRef, sessionRef);

            le_ref_DeleteRef(gnss.DgnssSourceRefMap, safeRef);
            le_mem_Release(dgnssSourcePtr);
        }
        result = le_ref_NextNode(iterRef);
    }
}

taf_locGnss::~taf_locGnss()
{
    LE_INFO("~taf_locGnss!!");
}

void taf_locGnss::CleanUp(taf_locGnss_Client_t* clientPtr)
{
    LE_INFO("CleanUp!!");
    if(clientPtr == NULL)
    {
        LE_ERROR("ClientPtr is Null");
        return;
    }

    if(tafpa::location::taf_pa_location_DeleteClient(clientPtr->locationClient) != PA_OK)
    {
        LE_ERROR("Unable to delete reference: %d", (int)clientPtr->locationClient);
    }else{
        clientPtr->locationClient = 0;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Main function for worker thread.
 */
//--------------------------------------------------------------------------------------------------
static void* LocationWorkerThread(void* context)
{
    LE_UNUSED(context);

    LE_INFO("Worker thread is now running.");
    le_cfg_ConnectService();
    le_event_RunLoop();

    return NULL;
}

// ---------------------------------------------------------------------------
// FUNCTION     : LocationEventWorkerThread
// DESCRIPTION  : Entry point for the dedicated PA-event processing thread.
//                All three event handlers (position, capability, NMEA) are
//                registered on this thread's event loop, so PA callback
//                processing is fully decoupled from the main service thread.
// ---------------------------------------------------------------------------
static void* LocationEventWorkerThread(void* ctxPtr)
{
    LE_UNUSED(ctxPtr);
    LE_DEBUG("LocationEventWorkerThread started");
    le_event_RunLoop();

    return NULL;
}

static void RegisterLocationEventHandlers(void* param1, void* param2)
{
    LE_UNUSED(param1);
    LE_UNUSED(param2);

    auto& gnss = taf_locGnss::GetInstance();

    gnss.HandlerRef    = le_event_AddHandler("LocUpdateEventId",
                                              gnss.positionEventId,
                                              taf_locGnss::GnssPositionHandler);

    gnss.CapHandlerRef = le_event_AddHandler("LocCapabilityEventId",
                                              gnss.locCapabilityEventId,
                                              taf_locGnss::GnssCapabilityHandler);

    gnss.NmeaHandlerRef = le_event_AddHandler("NmeaEventID",
                                               gnss.nmeaEventId,
                                               taf_locGnss::GnssNmeaHandler);

    
    gnss.HandlerExRef = le_event_AddHandler("LocUpdateEventId1", gnss.PositionExEventId,
                                             taf_locGnss::GnssPositionExHandler);

    gnss.MeasurementHandlerRef = le_event_AddHandler("MeasurementEventID", gnss.measurementEventId,
                                               taf_locGnss::GnssMeasurementHandler);

    LE_DEBUG("All PA event handlers registered on LocationEventWorkerThRef");
}

void taf_locGnss::Init()
{
    SessionCtxList = LE_DLS_LIST_INIT;
    NumOfPositionHandlers = 0;
    NumOfPositionExHandlers = 0;
    NumOfCapabilityHandlers = 0;
    NumOfNmeaHandlers = 0;
    mClientRefCount = 0;
    NumOfMeasurementHandlers = 0;
    NumOfDgnssStatusHandlers = 0;

    LE_INFO("taf_locGnss-->Init!!");

    if (taf_pa_location_Init() != PA_OK)
    {
        LE_ERROR("Cannot initialize location platform adaptor");
    }else{
        LE_INFO("Init Successfull!!");
    }

    PositionHandlerPoolRef = le_mem_InitStaticPool(PositionHandler, GNSS_POSITION_HANDLER_HIGH,
            sizeof(taf_locGnss_PositionHandler_t));

    MeasurementHandlerPoolRef = le_mem_InitStaticPool(MeasurementHandler, GNSS_POSITION_HANDLER_HIGH,
            sizeof(taf_locGnss_MeasurementHandler_t));

    PositionExHandlerPoolRef = le_mem_InitStaticPool(PositionExHandler, GNSS_POSITION_HANDLER_HIGH,
            sizeof(taf_locGnss_PositionExHandler_t));

    DgnssStatusHandlerPoolRef = le_mem_InitStaticPool(DgnssHandler, GNSS_POSITION_HANDLER_HIGH,
            sizeof(taf_locGnss_DgnssStatusChangeHandler_t));

    NmeaHandlerPoolRef = le_mem_InitStaticPool(NmeaHandler, GNSS_POSITION_HANDLER_HIGH,
            sizeof(taf_locGnss_NmeaHandler_t));

    CapabilityHandlerPoolRef = le_mem_InitStaticPool(CapHandler, GNSS_POSITION_HANDLER_HIGH,
            sizeof(taf_locGnss_CapHandler_t));

    PositionSamplePoolRef = le_mem_InitStaticPool(PositionSample, GNSS_POSITION_SAMPLE_MAX,
            sizeof(taf_locGnss_PositionSample_t));

    MeasurementSamplePoolRef = le_mem_InitStaticPool(MeasurementSample, GNSS_POSITION_SAMPLE_MAX,
            sizeof(taf_locGnss_GnssMeasurements_t));

    NmeaSamplePoolRef = le_mem_InitStaticPool(NmeaSample, GNSS_POSITION_SAMPLE_MAX,
            sizeof(NmeaInfoEvent_t));

    CapSamplePoolRef = le_mem_InitStaticPool(CapSample, GNSS_POSITION_SAMPLE_MAX,
            sizeof(CapabilityChangeEvent_t));

    PositionSampleRequestPoolRef = le_mem_InitStaticPool(PositionSampleRequest,
            GNSS_POSITION_SAMPLE_MAX, sizeof(taf_locGnss_PositionSampleRequest_t));

    MeasurementSampleRequestPoolRef = le_mem_InitStaticPool(MeasurementSampleRequest,
            GNSS_POSITION_SAMPLE_MAX, sizeof(taf_locGnss_MeasurementSampleRequest_t));

    PositionSampleExPoolRef = le_mem_InitStaticPool(PositionSampleEx,
            GNSS_POSITION_SAMPLE_MAX, sizeof(taf_locGnss_PositionSampleEx_t));

    PositionExSamplePoolRef = le_mem_InitStaticPool(PositionExSample,
            GNSS_POSITION_SAMPLE_MAX, sizeof(taf_locGnss_PositionExSample_t));

    DgnssSourcePoolRef = le_mem_InitStaticPool(DgnssSource, GNSS_POSITION_SAMPLE_MAX, 
            sizeof(taf_locGnss_DgnssSource_t));

    DgnssStatusPoolRef = le_mem_InitStaticPool(DgnssStatusRequest, GNSS_POSITION_SAMPLE_MAX, 
            sizeof(DgnssStatusEvent_t));

    CmdLocationPoolRef = le_mem_InitStaticPool(CmdLocationPool, MAX_CMD_LOCATION_POOL_SIZE, sizeof(LocationCmdInfo_t));

    GnssInternalCmdPoolRef = le_mem_InitStaticPool(GnssInternalCmdPool,
    MAX_CMD_LOCATION_POOL_SIZE, sizeof(GnssInternalCmd_t));

    PositionSampleMap = le_ref_InitStaticMap(PositionSampleMap, GNSS_POSITION_SAMPLE_MAX);

    MeasurementSampleMap = le_ref_InitStaticMap(MeasurementSampleMap, GNSS_POSITION_SAMPLE_MAX);

    PositionExSampleMap = le_ref_InitStaticMap(PositionExSampleMap, GNSS_POSITION_SAMPLE_MAX);

    ClientRequestRefMap = le_ref_CreateMap("ClientRequestRefMap", TAF_CONFIG_POSITIONING_ACTIVATION_MAX);

    ClientPoolRef = le_mem_InitStaticPool(Client, TAF_CONFIG_POSITIONING_ACTIVATION_MAX, sizeof(taf_locGnss_Client_t));
    positionEventId = le_event_CreateIdWithRefCounting("positionEventId");

    measurementEventId = le_event_CreateIdWithRefCounting("measurementEventId");

    PositionExEventId = le_event_CreateIdWithRefCounting("PositionExEventId");

    locCapabilityEventId = le_event_CreateIdWithRefCounting("locCapabilityEventID");

    nmeaEventId = le_event_CreateIdWithRefCounting("NmeaEventId");

    NmeaHandlerRefMap = le_ref_CreateMap("NmeaHandlerRefMap", 4);

    CapHandlerRefMap = le_ref_CreateMap("CapHandlerRefMap", 4);

    PositionHandlerRefMap = le_ref_CreateMap("PositionHandlerRefMap", 4);
    MeasurementHandlerRefMap = le_ref_CreateMap("MeasurementHandlerRefMap", 4);
    PositionExHandlerRefMap = le_ref_CreateMap("PositionExHandlerRefMap", 4);

    DgnssSourceRefMap = le_ref_CreateMap("DgnssSourceMap", 10);
    DgnssStatusHandlerRefMap = le_ref_CreateMap("DgnssStatusHandlerRefMap", 4);

    dgnssStatusEventId = le_event_CreateIdWithRefCounting("dgnssStatusEventId");

    DgnssStatusHandlerRef = le_event_AddHandler("dgnssStatusEventId", dgnssStatusEventId, taf_locGnss::DgnssStatusHandler);

    le_msg_ServiceRef_t msgService = taf_locGnss_GetServiceRef();
    le_msg_AddServiceOpenHandler(msgService, OpenEventHandler, NULL);
    le_msg_AddServiceCloseHandler(msgService, CloseEventHandler, NULL);

    taf_locGnss_NmeaBitMask_t nmeaMask = GetNmeaConfig();
    LE_DEBUG("nmeaMask mask is : %" PRIu64 "", nmeaMask);

    if (nmeaMask == 0)
    {
        LE_DEBUG("Nmea node doesn't exist, so set default NMEA value");
        nmeaMask = TAF_LOCGNSS_NMEA_DEFAULT;
        SetNmeaConfig(TAF_LOCGNSS_NMEA_DEFAULT);
    }
    else
    {
       LE_DEBUG("Set nmeaMask from config tree");
    }

    le_result_t nmResult = SetNmeaSentencesInternal(nmeaMask);
    if (nmResult != LE_OK)
    {
        LE_WARN("Init: Failed to configure NMEA sentences [rc=%d], "
                "using default config", nmResult);
        SetNmeaConfig(TAF_LOCGNSS_NMEA_CONFIG_DEFAULT);
    }

    LocationSvcThRef = le_thread_GetCurrent();
    LocationWorkerThRef = le_thread_Create("LocationWorkerThread", LocationWorkerThread, NULL);
    if (LocationWorkerThRef == NULL)
    {
        LE_FATAL("Init: failed to create LocationWorkerThread");
        return;
    }

    le_thread_SetJoinable(LocationWorkerThRef);
    le_thread_Start(LocationWorkerThRef);

    LocationEventWorkerThRef = le_thread_Create("LocationEventWorkerThRef",
                                            LocationEventWorkerThread,
                                            NULL);
    le_thread_Start(LocationEventWorkerThRef);
    le_event_QueueFunctionToThread(LocationEventWorkerThRef,
                               RegisterLocationEventHandlers,
                               NULL, NULL);

    return;
}

le_result_t taf_locGnss::SetDRConfigValidity
(
    taf_locGnss_DRConfigValidityType_t validMask
)
{
    le_result_t result = LE_NOT_PERMITTED;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    // Check the GNSS device state
    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        {
            clientRequestPtr->drParamsMask = validMask;
            LE_DEBUG("SetDRConfigValidity clientRequestPtr->drParamsMask:%d",clientRequestPtr->drParamsMask);
            result = LE_OK;
        }
        break;
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
            result = LE_FAULT;
        }
        break;
    }
    return result;
}
