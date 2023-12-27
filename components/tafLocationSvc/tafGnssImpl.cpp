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

 * Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.

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
#include "watchdogChain.h"
#include <telux/loc/LocationFactory.hpp>
#include "tafGnss.hpp"
#include "float.h"

using namespace telux::loc;
using namespace telux::common;
using namespace telux::tafsvc;

LE_MEM_DEFINE_STATIC_POOL(PositionHandler, GNSS_POSITION_HANDLER_HIGH, sizeof(taf_gnss_PositionHandler_t));
LE_MEM_DEFINE_STATIC_POOL(PositionSample, GNSS_POSITION_SAMPLE_MAX, sizeof(taf_gnss_PositionSample_t));
LE_MEM_DEFINE_STATIC_POOL(PositionSampleRequest, GNSS_POSITION_SAMPLE_MAX, sizeof(taf_gnss_PositionSampleRequest_t));
LE_MEM_DEFINE_STATIC_POOL(Client, LE_CONFIG_POSITIONING_ACTIVATION_MAX, sizeof(taf_gnss_Client_t));
LE_REF_DEFINE_STATIC_MAP(PositionSampleMap, GNSS_POSITION_SAMPLE_MAX);
void bodyToSensorUtility(telux::loc::DREngineConfiguration& drConfig,
        const taf_gnss_DrParams_t* drParamsPtr);
void speedScaleUtility(telux::loc::DREngineConfiguration& drConfig,
        const taf_gnss_DrParams_t* drParamsPtr);
void gyroScaleUtility(telux::loc::DREngineConfiguration& drConfig,
        const taf_gnss_DrParams_t* drParamsPtr);

taf_Gnss &taf_Gnss::GetInstance()
{
    static taf_Gnss instance;
    return instance;
}

telux::common::Status taf_Gnss::DgnssManagerInit() {
    if(mDgnssManager == nullptr) {
        std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
        std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
        auto &locationFactory = LocationFactory::getInstance();
        mDgnssManager = locationFactory.getDgnssManager(DgnssDataFormat::DATA_FORMAT_RTCM_3,
            [&](ServiceStatus status) {
                if (status == ServiceStatus::SERVICE_AVAILABLE) {
                    prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
                } else {
                    prom.set_value(ServiceStatus::SERVICE_UNAVAILABLE);
                }
            });
        if (!mDgnssManager) {
            LE_INFO( "Failed to get Gnss manager object");
            return Status::FAILED;
        }

        startTime = std::chrono::system_clock::now();
        ServiceStatus dgnssMgrStatus = mDgnssManager->getServiceStatus();
        if(dgnssMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
            LE_INFO( "Dgnss subsystem is not ready, Please wait");
        }
        dgnssMgrStatus = prom.get_future().get();
        if(dgnssMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            LE_INFO( "Elapsed Time for Dgnss subsystems to ready : %lf", elapsedTime.count());
        } else {
            LE_INFO( "ERROR - Unable to initialize Dgnss subsystem");
            return telux::common::Status::NOTREADY;
        }
#endif
#ifdef TARGET_SA415M
        auto &locationFactory = LocationFactory::getInstance();
        mDgnssManager = locationFactory.getDgnssManager(DgnssDataFormat::DATA_FORMAT_RTCM_3);
        bool subSystemsStatus = mDgnssManager->isSubsystemReady();
        startTime = std::chrono::system_clock::now();
        if(!subSystemsStatus) {
            LE_INFO( "Dgnss subsystem is not ready, Please wait");
            std::future<bool> f = mDgnssManager->onSubsystemReady();
            subSystemsStatus = f.get();
        }

        if(subSystemsStatus) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            LE_INFO( "Elapsed Time for Dgnss subsystems to ready : %lf", elapsedTime.count());
        } else {
            LE_INFO( "ERROR - Unable to initialize Dgnss subsystem");
            return telux::common::Status::NOTREADY;
        }
#endif
   } else {
       LE_INFO("Dgnss manager is already initialized");
   }
   return telux::common::Status::SUCCESS;
}

telux::common::Status taf_Gnss::LocationManagerInit() {
    if(mLocationManager == nullptr) {
        std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
        std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
        auto &locationFactory = LocationFactory::getInstance();
        mLocationManager = locationFactory.getLocationManager([&](ServiceStatus status) {
                if (status == ServiceStatus::SERVICE_AVAILABLE) {
                prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
                } else {
                prom.set_value(ServiceStatus::SERVICE_UNAVAILABLE);
                }
                });
        startTime = std::chrono::system_clock::now();
        ServiceStatus locMgrStatus = mLocationManager->getServiceStatus();
        if(locMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
            LE_INFO("Location subsystem is not ready, Please wait");
        }
        locMgrStatus = prom.get_future().get();
        if(locMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            LE_INFO("Elapsed Time for Subsystems to ready : %lf",elapsedTime.count());
        } else {
            LE_INFO("ERROR - Unable to initialize Location subsystem");
            return telux::common::Status::FAILED;
        }
#endif
#ifdef TARGET_SA415M
        auto &locationFactory = LocationFactory::getInstance();
        mLocationManager = locationFactory.getLocationManager();
        bool subSystemsStatus = mLocationManager->isSubsystemReady();
        startTime = std::chrono::system_clock::now();
        if(!subSystemsStatus) {
            LE_INFO( "Location subsystem is not ready, Please wait");
            std::future<bool> f = mLocationManager->onSubsystemReady();
            subSystemsStatus = f.get();
        }

        if(subSystemsStatus) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            LE_INFO( "Elapsed Time for Subsystems to ready : %lf", elapsedTime.count());
        } else {
            LE_INFO( "ERROR - Unable to initialize Location subsystem");
            return telux::common::Status::NOTREADY;
        }
#endif
        mPosListener = std::make_shared<tafLocationListener>();
        mLocationManager->registerListenerEx(mPosListener);
    } else {
        LE_INFO("Location manager already initialized");
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status taf_Gnss::LocationConfiguratorInit() {
    if(mLocationConfigurator == nullptr) {
        std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
        std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
        auto &locationFactory = LocationFactory::getInstance();
        mLocationConfigurator = locationFactory.getLocationConfigurator([&](ServiceStatus status) {
                if (status == ServiceStatus::SERVICE_AVAILABLE) {
                prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
                } else {
                prom.set_value(ServiceStatus::SERVICE_UNAVAILABLE);
                }
                });
        startTime = std::chrono::system_clock::now();
        ServiceStatus locCfgStatus = mLocationConfigurator->getServiceStatus();
        if(locCfgStatus != ServiceStatus::SERVICE_AVAILABLE) {
            LE_INFO("Location configuration subsystem is not ready, Please wait");
        }
        locCfgStatus = prom.get_future().get();
        if(locCfgStatus == ServiceStatus::SERVICE_AVAILABLE) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            LE_INFO("Elapsed Time for configuration subsystems to ready : %lf",elapsedTime.count());
        } else {
            LE_INFO("ERROR - Unable to initialize Location configuration subsystem"
                   );
            return telux::common::Status::FAILED;
        }
#endif
#ifdef TARGET_SA415M
        auto &locationFactory = LocationFactory::getInstance();
        mLocationConfigurator = locationFactory.getLocationConfigurator();
        bool subSystemsStatus = mLocationConfigurator->isSubsystemReady();
        startTime = std::chrono::system_clock::now();
        if(!subSystemsStatus) {
            LE_INFO("Location configuration subsystem is not ready, Please wait");
            std::future<bool> f = mLocationConfigurator->onSubsystemReady();
            subSystemsStatus = f.get();
        }

        if(subSystemsStatus) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            LE_INFO("Elapsed Time for configuration subsystems to ready : %lf",elapsedTime.count());
        } else {
            LE_INFO("ERROR - Unable to initialize Location configuration subsystem");
            return telux::common::Status::NOTREADY;
        }
#endif
    } else {
        LE_INFO("Location configurator is already initialized");
    }
    return telux::common::Status::SUCCESS;
}

void taf_Gnss::CopyPositionData
(
    taf_gnss_PositionSample_t* LastDataPtr,
    taf_gnss_PositionSample_t* CurrentDataPtr
)
{
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
    LastDataPtr->epochTime = CurrentDataPtr->epochTime;
    LastDataPtr->satsInViewCount = CurrentDataPtr->satsInViewCount;
    LastDataPtr->satsTrackingCount = CurrentDataPtr->satsTrackingCount;
    LastDataPtr->satsUsedCount = CurrentDataPtr->satsUsedCount;
    uint8_t i;
    for(i=0; i<TAF_GNSS_SV_INFO_MAX_LEN; i++)
    {
        LastDataPtr->satInfo[i].satId = CurrentDataPtr->satInfo[i].satId;
        LastDataPtr->satInfo[i].satConst = CurrentDataPtr->satInfo[i].satConst;
        LastDataPtr->satInfo[i].satUsed = CurrentDataPtr->satInfo[i].satUsed;
        LastDataPtr->satInfo[i].satTracked = CurrentDataPtr->satInfo[i].satTracked;
        LastDataPtr->satInfo[i].satSnr = CurrentDataPtr->satInfo[i].satSnr;
        LastDataPtr->satInfo[i].satAzim = CurrentDataPtr->satInfo[i].satAzim;
        LastDataPtr->satInfo[i].satElev = CurrentDataPtr->satInfo[i].satElev;
        LastDataPtr->satInfo[i].signalType = CurrentDataPtr->satInfo[i].signalType;
        LastDataPtr->satInfo[i].glonassFcn = CurrentDataPtr->satInfo[i].glonassFcn;
        LastDataPtr->satInfo[i].baseBandCnr = CurrentDataPtr->satInfo[i].baseBandCnr;
    }

    for(i=0; i<TAF_GNSS_SV_INFO_MAX_LEN; i++)
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
    for(i=0; i<TAF_GNSS_MEASUREMENT_INFO_MAX; i++)
    {
        LastDataPtr->measInfo[i].gnssSignalType = CurrentDataPtr->measInfo[i].gnssSignalType;
        LastDataPtr->measInfo[i].gnssConstellation = CurrentDataPtr->measInfo[i].gnssConstellation;
        LastDataPtr->measInfo[i].gnssSvId = CurrentDataPtr->measInfo[i].gnssSvId;
    }
    LastDataPtr->measInfoCount = CurrentDataPtr->measInfoCount;
    LastDataPtr->reportStatus = CurrentDataPtr->reportStatus;
    LastDataPtr->altMeanSeaLevel = CurrentDataPtr->altMeanSeaLevel;
    for (i = 0; i < TAF_GNSS_MEASUREMENT_INFO_MAX; i++) {
        LastDataPtr->SVIds[i] = CurrentDataPtr->SVIds[i];
    }
    LastDataPtr->SVIdsCount = CurrentDataPtr->SVIdsCount;
    LastDataPtr->next = LE_DLS_LINK_INIT;

    return;
}

void taf_Gnss::GnssPositionHandler
(
 void* reportPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    taf_gnss_PositionHandler_t*  posHandlerPtr;
    taf_gnss_PositionSampleRequest_t*    posSampleReqPtr=NULL;
    taf_gnss_PositionSample_t* currentPosPtr = (taf_gnss_PositionSample_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL( currentPosPtr == NULL, "currentPosPtr is Null");

    LE_DEBUG("Handler Function called with position %p", currentPosPtr);
    CopyPositionData(&gnss.LastPositionSample, currentPosPtr);

    if(!gnss.NumOfPositionHandlers)
    {
        LE_INFO("No positioning handlers, exit Handler Function");
        le_mem_Release(currentPosPtr);
        return;
    }

    le_ref_IterRef_t iterRef = le_ref_GetIterator(gnss.PositionHandlerRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        posHandlerPtr = (taf_gnss_PositionHandler_t*)le_ref_GetValue(iterRef);
        LE_ASSERT(posHandlerPtr != NULL);
        posSampleReqPtr = (taf_gnss_PositionSampleRequest_t*)le_mem_ForceAlloc(gnss.PositionSampleRequestPoolRef);
        memset(posSampleReqPtr, 0, sizeof(taf_gnss_PositionSampleRequest_t));

        posSampleReqPtr->positionSampleNodePtr =
            (taf_gnss_PositionSample_t*)le_mem_ForceAlloc(gnss.PositionSamplePoolRef);
        memset(posSampleReqPtr->positionSampleNodePtr, 0, sizeof(taf_gnss_PositionSample_t));

        memcpy(posSampleReqPtr->positionSampleNodePtr, &gnss.LastPositionSample,
                sizeof(taf_gnss_PositionSample_t));

        posSampleReqPtr->sessionRef = posHandlerPtr->sessionRef;

        posSampleReqPtr->positionSampleRef =
           (taf_gnss_SampleRef_t)le_ref_CreateRef(gnss.PositionSampleMap, posSampleReqPtr);

        LE_DEBUG("Report sampleRef %p to the corresponding handler (handlerPtr %p)",
            posSampleReqPtr->positionSampleRef, posHandlerPtr->handlerFuncPtr);

        posHandlerPtr->handlerFuncPtr(posSampleReqPtr->positionSampleRef,
                                      posHandlerPtr->handlerContextPtr);

    }

    le_mem_Release(currentPosPtr);
}


void tafLocationListener::onDetailedEngineLocationUpdate(
      const std::vector<std::shared_ptr<telux::loc::ILocationInfoEx> > &locationEngineInfo) {
    auto &gnss = taf_Gnss::GetInstance();
    if (gnss.mTtffEnabled)
    {
        gnss.mEndTime = std::chrono::system_clock::now();
        gnss.mTtffEnabled = false;
    }
    le_mutex_Lock(gnss.mGnssMutexRef);
    if(gnss.NumOfPositionHandlers )
    {
        LE_DEBUG("**** Detailed Engine Location Report ****");
        for (auto locationInfo : locationEngineInfo) {
            LE_INFO( "onDetailedEngineLocationUpdate conformity: %lf",
                    locationInfo->getConformityIndex());
            if ( gnss.mSvEnabled && gnss.mGnssSigEnabled )
            {
                taf_gnss_PositionSample_t* LocationData =
                        (taf_gnss_PositionSample_t*)le_mem_ForceAlloc(gnss.PositionSamplePoolRef);
                uint8_t i;
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
                if(locationInfo->getAltitudeType() == telux::loc::AltitudeType::CALCULATED)
                {
                    gnss.mAltType = TAF_GNSS_ALT_TYPE_CALCULATED;
                    LE_INFO("onDetailedEngineLocationUpdate: AltitudeType->CALCULATED");
                }
                else if(locationInfo->getAltitudeType() == telux::loc::AltitudeType::ASSUMED)
                {
                    gnss.mAltType = TAF_GNSS_ALT_TYPE_ASSUMED;
                    LE_INFO("onDetailedEngineLocationUpdate: AltitudeType->ASSUMED");
                }
                else
                {
                    gnss.mAltType = TAF_GNSS_ALT_TYPE_UNKNOWN;
                    LE_INFO("onDetailedEngineLocationUpdate: AltitudeType->UNKNOWN");
                }
                for (auto i = 0; i < TAF_GNSS_MEASUREMENT_INFO_MAX; i++) {
                    memset((void*) &LocationData->measInfo[i], 0, sizeof(taf_gnss_GnssMeasurementInfo_t));
                }
                LocationData->measInfoCount = 0;
                std::vector<telux::loc::GnssMeasurementInfo> measInfo = locationInfo->getmeasUsageInfo();
                for (auto measInfoElement : measInfo) {
                    if (LocationData->measInfoCount < TAF_GNSS_MEASUREMENT_INFO_MAX) {
                        LocationData->measInfo[LocationData->measInfoCount].gnssSignalType = measInfoElement.gnssSignalType;
                        LocationData->measInfo[LocationData->measInfoCount].gnssConstellation = (taf_gnss_GnssSystem_t) measInfoElement.gnssConstellation;
                        LocationData->measInfo[LocationData->measInfoCount].gnssSvId = measInfoElement.gnssSvId;
                        LocationData->measInfoCount++;
                    }
                }

                std::vector<uint16_t> SVIds;
                locationInfo->getSVIds(SVIds);
                if(SVIds.size() > 0) {
                    for (auto i = 0; i < TAF_GNSS_MEASUREMENT_INFO_MAX; i++) {
                        LocationData->SVIds[i] = 0;
                    }
                    LocationData->SVIdsCount = 0;
                    for (auto i = 0; i < (int) SVIds.size(); i++) {
                        if (LocationData->SVIdsCount < TAF_GNSS_MEASUREMENT_INFO_MAX) {
                            LocationData->SVIds[LocationData->SVIdsCount] = SVIds.at(i);
                            LocationData->SVIdsCount++;
                        }
                    }
                }

                LocationData->reportStatus = (taf_gnss_ReportStatus_t) locationInfo->getReportStatus();
                LocationData->altMeanSeaLevel = locationInfo->getAltitudeMeanSeaLevel();
                LocationData->latitude = locationInfo->getLatitude() * 1e+6;
                LocationData->longitude = locationInfo->getLongitude() * 1e+6;
                LocationData->hAccuracy = locationInfo->getHorizontalUncertainty()* 1e+2;
                LocationData->altitude = locationInfo->getAltitude() * 1e+3;
                LocationData->vAccuracy = locationInfo->getVerticalUncertainty() * 10;
                LocationData->altitudeOnWgs84 = 0;
                LocationData->hSpeed = locationInfo->getSpeed()*100;
                LocationData->hSpeedAccuracy = locationInfo->getSpeedUncertainty()*1e+3;
                if(locationInfo->getVelocityEastNorthUp(gnss.mVerticalSpeed) ==
                        telux::common::Status::SUCCESS)
                {
                    for(auto i = 0; (unsigned)i < gnss.mVerticalSpeed.size() - 1; ++i)
                    {
                        LocationData->vSpeed = (int32_t) (gnss.mVerticalSpeed[i]*100);
                        LE_INFO("onDetailedEngineLocationUpdate gnss.mVerticalSpeed[%d]:"
                                "%lf ",i,gnss.mVerticalSpeed[i]);
                        LE_INFO("onDetailedEngineLocationUpdate LocationData->vSpeed[%d]:"
                                "%d ",i,LocationData->vSpeed);
                    }
                }
                else
                {
                    LocationData->vSpeed = 0;
                    LE_INFO("onDetailedEngineLocationUpdate vSpeed fail");
                }
                if(locationInfo->getVelocityUncertaintyEastNorthUp(gnss.mVerticalSpeedAccuracy) ==
                        telux::common::Status::SUCCESS)
                {
                    for(auto i = 0; (unsigned)i < gnss.mVerticalSpeedAccuracy.size() - 1; ++i)
                    {
                        LocationData->vSpeedAccuracy =(int32_t)(gnss.mVerticalSpeedAccuracy[i]*1e+3);
                        LE_INFO("onDetailedEngineLocationUpdate gnss.mVerticalSpeedAccuracy[%d]:"
                                "%lf ",i, gnss.mVerticalSpeedAccuracy[i]);
                        LE_INFO("onDetailedEngineLocationUpdate LocationData->vSpeedAccuracy[%d]:"
                                "%d ",i, LocationData->vSpeedAccuracy);
                    }
                }
                else
                {
                    LocationData->vSpeedAccuracy = 0;
                    LE_INFO("onDetailedEngineLocationUpdate vSpeedAccuracy fail");
                }
                LocationData->magneticDeviation = locationInfo->getMagneticDeviation()*10;
                LocationData->epochTime = locationInfo->getTimeStamp();
                LocationData->horUncEllipseSemiMajor =
                        locationInfo->getHorizontalUncertaintySemiMajor();
                LocationData->horUncEllipseSemiMinor =
                        locationInfo->getHorizontalUncertaintySemiMinor();
                LocationData->direction = locationInfo->getHeading()*10;
                LocationData->directionAccuracy = locationInfo->getHeadingUncertainty()*10;
                telux::loc::SystemTime sysTime = locationInfo->getGnssSystemTime();
                telux::loc::GnssSystem system = sysTime.gnssSystemTimeSrc;
                telux::loc::SystemTimeInfo sysTimeInfo = sysTime.time;
                if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GPS) {
                    telux::loc::TimeInfo timeInfo = sysTimeInfo.gps;
                    LocationData->gpsWeek = timeInfo.systemWeek;
                    LocationData->gpsTimeOfWeek = timeInfo.systemMsec;
                }else {
                    LocationData->gpsWeek = 0;
                    LocationData->gpsTimeOfWeek = 0;
                }
                LocationData->timeAccuracy = locationInfo->getTimeUncMs();
                LocationData->positionLatency = 0;
                LocationData->hdop = locationInfo->getHorizontalDop() *1e+3;
                LocationData->vdop = locationInfo->getVerticalDop() * 1e+3;
                LocationData->pdop = locationInfo->getPositionDop() * 1e+3;
                LocationData->gdop = locationInfo->getGeometricDop() * 1e+3;
                LocationData->tdop = locationInfo->getTimeDop() * 1e+3;
                if(locationInfo->getTimeStamp() != telux::loc::UNKNOWN_TIMESTAMP) {
                    time_t realtime;
                    realtime = (time_t)((locationInfo->getTimeStamp() / 1000));
                    tm *ltm = localtime(&realtime);
                    LE_INFO("onDetailedEngineLocationUpdate !UNKNOWN_TIMESTAMP");
                    if(ltm != NULL)
                    {
                        LocationData->year = 1900+ltm->tm_year;
                        LocationData->month = 1+ltm->tm_mon;
                        LocationData->day = ltm->tm_mday;
                        //To match UTC time
                        LocationData->hours = ltm->tm_hour;
                        LocationData->minutes = ltm->tm_min;
                        LocationData->seconds = ltm->tm_sec;
                        LocationData->milliseconds = (locationInfo->getTimeStamp())%1000;
                    }
                    else
                    {
                        LE_ERROR("onDetailedEngineLocationUpdate local time ltm is NULL");
                    }
                } else {
                    LE_DEBUG("Time stamp Not Valid");
                    LE_INFO("onDetailedEngineLocationUpdate UNKNOWN_TIMESTAMP");
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
                if(locationInfo->getLeapSeconds(gnss.mLeapSeconds) ==
                        telux::common::Status::SUCCESS)
                {
                    LocationData->leapSeconds = gnss.mLeapSeconds;
                } else {
                    LocationData->leapSeconds = 0;
                }
                LocationData->satsInViewCount = gnss.mSatParams.satsInViewCount;
                LocationData->satsTrackingCount = gnss.mTotalSVTracked;
                LocationData->satsUsedCount = locationInfo->getNumSvUsed();

                for(i=0; i<TAF_GNSS_SV_INFO_MAX_LEN; i++)
                {
                    LocationData->satInfo[i].satId = gnss.mSatInfo[i].satId;
                    LocationData->satInfo[i].satConst = gnss.mSatInfo[i].satConst;
                    LocationData->satInfo[i].satUsed = gnss.mSatInfo[i].satUsed;
                    LocationData->satInfo[i].satTracked = gnss.mSatInfo[i].satTracked;
                    LocationData->satInfo[i].satSnr = gnss.mSatInfo[i].satSnr;
                    LocationData->satInfo[i].satAzim = gnss.mSatInfo[i].satAzim;
                    LocationData->satInfo[i].satElev = gnss.mSatInfo[i].satElev;
                    LocationData->satInfo[i].signalType = gnss.mSatInfo[i].signalType;
                    LocationData->satInfo[i].glonassFcn = gnss.mSatInfo[i].glonassFcn;
                    LocationData->satInfo[i].baseBandCnr = gnss.mSatInfo[i].baseBandCnr;
                }

                for(i=0; i<TAF_GNSS_SV_INFO_MAX_LEN; i++)
                {
                    LocationData->satMeas[i].satId = 0;
                    LocationData->satMeas[i].satLatency = 0;
                }
                LocationData->robustConformity = locationInfo->getConformityIndex();
                LocationData->confidencePercent = locationInfo->getCalibrationConfidencePercent();
                telux::loc::DrCalibrationStatus calibrationStatus =
                                                locationInfo->getCalibrationStatus();
                LE_INFO("onDetailedEngineLocationUpdate calibrationStatus %d", calibrationStatus);
                if((calibrationStatus & telux::loc::DR_ROLL_CALIBRATION_NEEDED))
                {
                    LE_INFO("onDetailedEngineLocationUpdate Roll calibration is needed");
                    LocationData->calibrationStatus |= ((1<<TAF_GNSS_DR_ROLL_CALIBRATION_NEEDED));
                }
                if((calibrationStatus & telux::loc::DR_PITCH_CALIBRATION_NEEDED))
                {
                    LE_INFO("onDetailedEngineLocationUpdate Pitch calibration is needed");
                    LocationData->calibrationStatus |= ((1<<TAF_GNSS_DR_PITCH_CALIBRATION_NEEDED));
                }
                if((calibrationStatus & telux::loc::DR_YAW_CALIBRATION_NEEDED))
                {
                    LE_INFO("onDetailedEngineLocationUpdate Yaw calibration is needed");
                    LocationData->calibrationStatus |= ((1<<TAF_GNSS_DR_YAW_CALIBRATION_NEEDED));
                }
                if((calibrationStatus & telux::loc::DR_ODO_CALIBRATION_NEEDED))
                {
                    LE_INFO("onDetailedEngineLocationUpdate Odo calibration is needed");
                    LocationData->calibrationStatus |= ((1<<TAF_GNSS_DR_ODO_CALIBRATION_NEEDED));
                }
                if((calibrationStatus & telux::loc::DR_GYRO_CALIBRATION_NEEDED))
                {
                    LE_INFO("onDetailedEngineLocationUpdate Gyro calibration is needed");
                    LocationData->calibrationStatus |= ((1<<TAF_GNSS_DR_GYRO_CALIBRATION_NEEDED));
                }
                LE_INFO("onDetailedEngineLocationUpdate Location position dynamic");
                telux::loc::GnssKinematicsData GnssKinData = locationInfo->getBodyFrameData();
                telux::loc::KinematicDataValidity GnssKinDataValidity =
                                                  GnssKinData.bodyFrameDataMask;
                LE_INFO("onDetailedEngineLocationUpdate GnssKinDataValidity:%0x",
                                                  GnssKinDataValidity);
                if((GnssKinDataValidity & telux::loc::HAS_LONG_ACCEL))
                {
                    LE_INFO("Navigation data has Forward Acceleration");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_GNSS_HAS_LONG_ACCEL);
                }
                if((GnssKinDataValidity & telux::loc::HAS_LAT_ACCEL))
                {
                    LE_INFO("Navigation data has Sideward Acceleration");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_GNSS_HAS_LAT_ACCEL);
                }
                if((GnssKinDataValidity & telux::loc::HAS_VERT_ACCEL))
                {
                    LE_INFO("Navigation data has Vertical Acceleration");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_GNSS_HAS_VERT_ACCEL);
                }
                if((GnssKinDataValidity & telux::loc::HAS_YAW_RATE))
                {
                    LE_INFO("Navigation data has Heading Rate");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_GNSS_HAS_YAW_RATE);
                }
                if((GnssKinDataValidity & telux::loc::HAS_PITCH))
                {
                    LE_INFO("Navigation data has Pitch");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |= (1<<TAF_GNSS_HAS_PITCH);
                }
                if((GnssKinDataValidity & telux::loc::HAS_LONG_ACCEL_UNC))
                {
                    LE_INFO("Navigation data has Forward Acceleration Unc");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_GNSS_HAS_LONG_ACCEL_UNC);
                }
                if((GnssKinDataValidity & telux::loc::HAS_LAT_ACCEL_UNC))
                {
                    LE_INFO("Navigation data has Sideward Acceleration Unc");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_GNSS_HAS_LAT_ACCEL_UNC);
                }
                if((GnssKinDataValidity & telux::loc::HAS_VERT_ACCEL_UNC))
                {
                    LE_INFO("Navigation data has Vertical Acceleration Unc");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                    (1<<TAF_GNSS_HAS_VERT_ACCEL_UNC);
                }
                if((GnssKinDataValidity & telux::loc::HAS_YAW_RATE_UNC))
                {
                    LE_INFO("Navigation data has Heading Rate Unc");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                    (1<<TAF_GNSS_HAS_YAW_RATE_UNC);
                }
                if((GnssKinDataValidity & telux::loc::HAS_PITCH_UNC))
                {
                    LE_INFO("Navigation data has Body Pitch Unc");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                    (1<<TAF_GNSS_HAS_PITCH_UNC);
                }
                if((GnssKinDataValidity & telux::loc::HAS_PITCH_RATE_BIT))
                {
                    LE_INFO("Navigation data has Pitch rate");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_GNSS_HAS_PITCH_RATE_BIT);
                }
                if((GnssKinDataValidity & telux::loc::HAS_PITCH_RATE_UNC_BIT))
                {
                    LE_INFO("Navigation data has Pitch rate Unc");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_GNSS_HAS_PITCH_RATE_UNC_BIT);
                }
                if((GnssKinDataValidity & telux::loc::HAS_ROLL_BIT))
                {
                    LE_INFO("Navigation data has roll");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |= (1<<TAF_GNSS_HAS_ROLL_BIT);
                }
                if((GnssKinDataValidity & telux::loc::HAS_ROLL_UNC_BIT))
                {
                    LE_INFO("Navigation data has roll Unc ");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_GNSS_HAS_ROLL_UNC_BIT);
                }
                if((GnssKinDataValidity & telux::loc::HAS_ROLL_RATE_BIT))
                {
                    LE_INFO("Navigation data has roll rate ");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_GNSS_HAS_ROLL_RATE_BIT);
                }
                if((GnssKinDataValidity & telux::loc::HAS_ROLL_RATE_UNC_BIT))
                {
                    LE_INFO("Navigation data has roll rate Unc ");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_GNSS_HAS_ROLL_RATE_UNC_BIT);
                }
                if((GnssKinDataValidity & telux::loc::HAS_YAW_BIT))
                {
                    LE_INFO("Navigation data has Yaw bit ");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |= (1<<TAF_GNSS_HAS_YAW_BIT);
                }
                if((GnssKinDataValidity & telux::loc::HAS_YAW_UNC_BIT))
                {
                    LE_INFO("Navigation data has Yaw bit Unc ");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_GNSS_HAS_YAW_UNC_BIT);
                }
                else
                {
                    LE_INFO("Navigation data is not found");
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

                LocationData->vrpLatitude = locationInfo->getVRPBasedLLA().latitude;
                LocationData->vrpLongitude = locationInfo->getVRPBasedLLA().longitude;
                LocationData->vrpAltitude = locationInfo->getVRPBasedLLA().altitude;
                LocationData->eastVel = locationInfo->getVRPBasedENUVelocity()[0];
                LocationData->northVel = locationInfo->getVRPBasedENUVelocity()[1];
                LocationData->upVel = locationInfo->getVRPBasedENUVelocity()[2];
                LocationData->svData.gps = locationInfo->getSvUsedInPosition().gps;
                LocationData->svData.glo = locationInfo->getSvUsedInPosition().glo;
                LocationData->svData.gal = locationInfo->getSvUsedInPosition().gal;
                LocationData->svData.bds = locationInfo->getSvUsedInPosition().bds;
                LocationData->svData.qzss = locationInfo->getSvUsedInPosition().qzss;
                LocationData->svData.navic = locationInfo->getSvUsedInPosition().navic;
                telux::loc::SbasCorrection correction = locationInfo->getSbasCorrection();
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_IONO])
                {
                    LocationData->sbasMask |= (1<<TAF_GNSS_SBAS_CORRECTION_IONO);
                    LE_INFO("SBAS ionospheric correction is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_FAST])
                {
                    LocationData->sbasMask |= (1<<TAF_GNSS_SBAS_CORRECTION_FAST);
                    LE_INFO("SBAS fast correction is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_LONG])
                {
                    LocationData->sbasMask |= (1<<TAF_GNSS_SBAS_CORRECTION_LONG);
                    LE_INFO("SBAS long correction is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_INTEGRITY])
                {
                    LocationData->sbasMask |= (1<<TAF_GNSS_SBAS_INTEGRITY);
                    LE_INFO("SBAS integrity information is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_DGNSS])
                {
                    LocationData->sbasMask |= (1<<TAF_GNSS_SBAS_CORRECTION_DGNSS);
                    LE_INFO("SBAS DGNSS correction information is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_RTK])
                {
                    LocationData->sbasMask |= (1<<TAF_GNSS_SBAS_CORRECTION_RTK);
                    LE_INFO("SBAS RTK correction information is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_PPP])
                {
                    LocationData->sbasMask |= (1<<TAF_GNSS_SBAS_CORRECTION_PPP);
                    LE_INFO("SBAS PPP correction information is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_RTK_FIXED])
                {
                    LocationData->sbasMask |= (1<<TAF_GNSS_SBAS_CORRECTION_RTK_FIXED);
                    LE_INFO("SBAS RTK fixed correction information is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)
                               telux::loc::SBAS_CORRECTION_ONLY_SBAS_CORRECTED_SV_USED_])
                {
                    LocationData->sbasMask |= (1<<TAF_GNSS_SBAS_CORRECTED_SV_USED);
                    LE_INFO("SBAS corrected SV is used");
                }
                telux::loc::LocationInfoValidity validityMask = locationInfo->getLocationInfoValidity();
                LE_INFO("LocationInfoExValidity->validityMask: %u ",validityMask);
                if((validityMask & telux::loc::HAS_LAT_LONG_BIT))
                {
                    LocationData->validityMask |= TAF_GNSS_HAS_LAT_LONG_BIT;
                    LE_INFO("valid latitude longitude");
                }
                if((validityMask & telux::loc::HAS_ALTITUDE_BIT))
                {
                    LocationData->validityMask |= TAF_GNSS_HAS_ALTITUDE_BIT;
                    LE_INFO("valid altitude");
                }
                if((validityMask & telux::loc::HAS_SPEED_BIT))
                {
                    LocationData->validityMask |= TAF_GNSS_HAS_SPEED_BIT;
                    LE_INFO("valid speed");
                }
                if((validityMask & telux::loc::HAS_HEADING_BIT))
                {
                    LocationData->validityMask |= TAF_GNSS_HAS_HEADING_BIT;
                    LE_INFO("valid heading");
                }
                if((validityMask & telux::loc::HAS_HORIZONTAL_ACCURACY_BIT))
                {
                    LocationData->validityMask |= TAF_GNSS_HAS_HORIZONTAL_ACCURACY_BIT;
                    LE_INFO("valid horizontal accuracy");
                }
                if((validityMask & telux::loc::HAS_VERTICAL_ACCURACY_BIT))
                {
                    LocationData->validityMask |= TAF_GNSS_HAS_VERTICAL_ACCURACY_BIT;
                    LE_INFO("valid vertical accuracy");
                }
                if((validityMask & telux::loc::HAS_SPEED_ACCURACY_BIT))
                {
                    LocationData->validityMask |= TAF_GNSS_HAS_SPEED_ACCURACY_BIT;
                    LE_INFO("valid speed accuracy");
                }
                if((validityMask & telux::loc::HAS_HEADING_ACCURACY_BIT))
                {
                    LocationData->validityMask |= TAF_GNSS_HAS_HEADING_ACCURACY_BIT;
                    LE_INFO("valid heading accuracy");
                }
                if((validityMask & telux::loc::HAS_TIMESTAMP_BIT))
                {
                    LocationData->validityMask |= TAF_GNSS_HAS_TIMESTAMP_BIT;
                    LE_INFO("valid timestamp");
                }
                if((validityMask & telux::loc::HAS_ELAPSED_REAL_TIME_BIT))
                {
                    LocationData->validityMask |= TAF_GNSS_HAS_ELAPSED_REAL_TIME_BIT;
                    LE_INFO("valid elapsed real time");
                }
                if((validityMask & telux::loc::HAS_ELAPSED_REAL_TIME_UNC_BIT))
                {
                    LocationData->validityMask |= TAF_GNSS_HAS_ELAPSED_REAL_TIME_UNC_BIT;
                    LE_INFO("valid elapsed real time Uncertainity");
                }
                telux::loc::LocationInfoExValidity validityExMask =
                                                   locationInfo->getLocationInfoExValidity();
                LE_INFO("LocationInfoExValidity->validityExMask: %" PRIu64 "", validityExMask);
                if((validityExMask & telux::loc::HAS_ALTITUDE_MEAN_SEA_LEVEL))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_ALTITUDE_MEAN_SEA_LEVEL);
                    LE_INFO("valid altitude mean sea level");
                }
                if((validityExMask & telux::loc::HAS_DOP))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_DOP);
                    LE_INFO("valid pdop, hdop, vdop");
                }
                if((validityExMask & telux::loc::HAS_MAGNETIC_DEVIATION))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_MAGNETIC_DEVIATION);
                    LE_INFO("valid magnetic deviation");
                }
                if((validityExMask & telux::loc::HAS_HOR_RELIABILITY))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_HOR_RELIABILITY);
                    LE_INFO("valid horizontal reliability");
                }
                if((validityExMask & telux::loc::HAS_VER_RELIABILITY))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_VER_RELIABILITY);
                    LE_INFO("valid vertical reliability");
                }
                if((validityExMask & telux::loc::HAS_HOR_ACCURACY_ELIP_SEMI_MAJOR))
                {
                    LocationData->validityExMask |=
                                                 (1ULL << TAF_GNSS_HAS_HOR_ACCURACY_ELIP_SEMI_MAJOR);
                    LE_INFO("valid elipsode semi major");
                }
                if((validityExMask & telux::loc::HAS_HOR_ACCURACY_ELIP_SEMI_MINOR))
                {
                    LocationData->validityExMask |=
                                                 (1ULL << TAF_GNSS_HAS_HOR_ACCURACY_ELIP_SEMI_MINOR);
                    LE_INFO("valid elipsode semi minor");
                }
                if((validityExMask & telux::loc::HAS_HOR_ACCURACY_ELIP_AZIMUTH))
                {
                    LocationData->validityExMask |=
                                                 (1ULL << TAF_GNSS_HAS_HOR_ACCURACY_ELIP_AZIMUTH);
                    LE_INFO("valid accuracy elipsode azimuth");
                }
                if((validityExMask & telux::loc::HAS_GNSS_SV_USED_DATA))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_GNSS_SV_USED_DATA);
                    LE_INFO("valid gnss sv used in pos data");
                }
                if((validityExMask & telux::loc::HAS_NAV_SOLUTION_MASK))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_NAV_SOLUTION_MASK);
                    LE_INFO("valid navSolutionMask");
                }
                if((validityExMask & telux::loc::HAS_POS_TECH_MASK))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_POS_TECH_MASK);
                    LE_INFO("valid LocPosTechMask");
                }
                if((validityExMask & telux::loc::HAS_SV_SOURCE_INFO))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_SV_SOURCE_INFO);
                    LE_INFO("valid LocSvInfoSource");
                }
                if((validityExMask & telux::loc::HAS_POS_DYNAMICS_DATA))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_POS_DYNAMICS_DATA);
                    LE_INFO("valid position dynamics data");
                }
                if((validityExMask & telux::loc::HAS_EXT_DOP))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_EXT_DOP);
                    LE_INFO("valid gdop, tdop");
                }
                if((validityExMask & telux::loc::HAS_NORTH_STD_DEV))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_NORTH_STD_DEV);
                    LE_INFO("valid North standard deviation");
                }
                if((validityExMask & telux::loc::HAS_EAST_STD_DEV))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_EAST_STD_DEV);
                    LE_INFO("valid East standard deviation");
                }
                if((validityExMask & telux::loc::HAS_NORTH_VEL))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_NORTH_VEL);
                    LE_INFO("valid North Velocity");
                }
                if((validityExMask & telux::loc::HAS_EAST_VEL))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_EAST_VEL);
                    LE_INFO("valid East Velocity");
                }
                if((validityExMask & telux::loc::HAS_UP_VEL))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_UP_VEL);
                    LE_INFO("valid Up Velocity");
                }
                if((validityExMask & telux::loc::HAS_NORTH_VEL_UNC))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_NORTH_VEL_UNC);
                    LE_INFO("valid North Velocity Uncertainty");
                }
                if((validityExMask & telux::loc::HAS_EAST_VEL_UNC))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_EAST_VEL_UNC);
                    LE_INFO("valid East Velocity Uncertainty");
                }
                if((validityExMask & telux::loc::HAS_UP_VEL_UNC))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_UP_VEL_UNC);
                    LE_INFO("valid Up Velocity Uncertainty");
                }
                if((validityExMask & telux::loc::HAS_LEAP_SECONDS))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_LEAP_SECONDS);
                    LE_INFO("valid leap_seconds");
                }
                if((validityExMask & telux::loc::HAS_TIME_UNC))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_TIME_UNC);
                    LE_INFO("valid timeUncMs");
                }
                if((validityExMask & telux::loc::HAS_NUM_SV_USED_IN_POSITION))
                {
                    LocationData->validityExMask |=
                                               (1ULL << TAF_GNSS_HAS_NUM_SV_USED_IN_POSITION);
                    LE_INFO("valid number of sv used");
                }
                if((validityExMask & telux::loc::HAS_CALIBRATION_CONFIDENCE_PERCENT))
                {
                    LocationData->validityExMask |=
                                               (1ULL << TAF_GNSS_HAS_CALIBRATION_CONFIDENCE_PERCENT);
                    LE_INFO("valid sensor calibrationConfidencePercent");
                }
                if((validityExMask & telux::loc::HAS_CALIBRATION_STATUS))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_CALIBRATION_STATUS);
                    LE_INFO("valid sensor calibrationConfidence");
                }
                if((validityExMask & telux::loc::HAS_OUTPUT_ENG_TYPE))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_OUTPUT_ENG_TYPE);
                    LE_INFO("valid output engine type");
                }
                if((validityExMask & telux::loc::HAS_OUTPUT_ENG_MASK))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_OUTPUT_ENG_MASK);
                    LE_INFO("valid output engine mask");
                }
                if((validityExMask & telux::loc::HAS_CONFORMITY_INDEX_FIX))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_CONFORMITY_INDEX_FIX);
                    LE_INFO("valid conformity index");
                }
                if((validityExMask & telux::loc::HAS_LLA_VRP_BASED))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_LLA_VRP_BASED);
                    LE_INFO("valid lla vrp based");
                }
                if((validityExMask & telux::loc::HAS_ENU_VELOCITY_VRP_BASED))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_ENU_VELOCITY_VRP_BASED);
                    LE_INFO("valid enu velocity vrp based");
                }
                if((validityExMask & telux::loc::HAS_ALTITUDE_TYPE))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_ALTITUDE_TYPE);
                    LE_INFO("valid altitude type");
                }
                if((validityExMask & telux::loc::HAS_REPORT_STATUS))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_REPORT_STATUS);
                    LE_INFO("valid report status");
                }
                if((validityExMask & telux::loc::HAS_INTEGRITY_RISK_USED))
                {
                    LocationData->validityExMask |= (1ULL << TAF_GNSS_HAS_INTEGRITY_RISK_USED);
                    LE_INFO("valid integrity risk");
                }
                if((validityExMask & telux::loc::HAS_PROTECT_LEVEL_ALONG_TRACK))
                {
                    LocationData->validityExMask |=
                                                (1ULL << TAF_GNSS_HAS_PROTECT_LEVEL_ALONG_TRACK);
                    LE_INFO("valid protect along track");
                }
                if((validityExMask & telux::loc::HAS_PROTECT_LEVEL_CROSS_TRACK))
                {
                    LocationData->validityExMask |=
                                                (1ULL << TAF_GNSS_HAS_PROTECT_LEVEL_CROSS_TRACK);
                    LE_INFO("valid protect cross track");
                }
                if((validityExMask & telux::loc::HAS_PROTECT_LEVEL_VERTICAL))
                {
                    LocationData->validityExMask |=
                                                (1ULL << TAF_GNSS_HAS_PROTECT_LEVEL_VERTICAL);
                    LE_INFO("valid protect vertical");
                }
                telux::loc::PositioningEngine posEngineBits = locationInfo->getLocOutputEngMask();
                if(posEngineBits & telux::loc::STANDARD_POSITIONING_ENGINE)
                {
                    LocationData->engMask |= TAF_GNSS_STANDARD_POSITIONING_ENGINE;
                    LE_INFO("eng Mask is STANDARD_POSITIONING_ENGINE");
                }
                if(posEngineBits & telux::loc::DEAD_RECKONING_ENGINE)
                {
                    LocationData->engMask |= TAF_GNSS_DEAD_RECKONING_ENGINE;
                    LE_INFO("eng Mask is DEAD_RECKONING_ENGINE");
                }
                if(posEngineBits & telux::loc::PRECISE_POSITIONING_ENGINE)
                {
                    LocationData->engMask |= TAF_GNSS_PRECISE_POSITIONING_ENGINE;
                    LE_INFO("eng Mask is PRECISE_POSITIONING_ENGINE");
                }
                if(posEngineBits & telux::loc::VP_POSITIONING_ENGINE)
                {
                    LocationData->engMask |= TAF_GNSS_VP_POSITIONING_ENGINE;
                    LE_INFO("eng Mask is VP_POSITIONING_ENGINE");
                }
                telux::loc::LocationAggregationType locEngineType = locationInfo->getLocOutputEngType();
                if(locEngineType == telux::loc::LOC_OUTPUT_ENGINE_FUSED)
                {
                    LocationData->locationEngType = TAF_GNSS_LOC_OUTPUT_ENGINE_FUSED;
                    LE_INFO("location eng type is FUSED");
                }
                if(locEngineType == telux::loc::LOC_OUTPUT_ENGINE_SPE)
                {
                    LocationData->locationEngType = TAF_GNSS_LOC_OUTPUT_ENGINE_SPE;
                    LE_INFO("location eng type is SPE");
                }
                if(locEngineType == telux::loc::LOC_OUTPUT_ENGINE_PPE)
                {
                    LocationData->locationEngType = TAF_GNSS_LOC_OUTPUT_ENGINE_PPE;
                    LE_INFO("location eng type is PPE");
                }
                if(locEngineType == telux::loc::LOC_OUTPUT_ENGINE_VPE)
                {
                    LocationData->locationEngType = TAF_GNSS_LOC_OUTPUT_ENGINE_VPE;
                    LE_INFO("location eng type is VPE");
                }
                telux::loc::LocationReliability locReliability =
                                                   locationInfo->getHorizontalReliability();
                if(locReliability == telux::loc::LocationReliability::NOT_SET)
                {
                    LocationData->horiReliablity = TAF_GNSS_RELIABILITY_NOT_SET;
                    LE_INFO("horizontal reliablity is NOT SET");
                }
                else if(locReliability == telux::loc::LocationReliability::VERY_LOW)
                {
                    LocationData->horiReliablity = TAF_GNSS_RELIABILITY_VERY_LOW;
                    LE_INFO("horizontal reliablity is VERY LOW");
                }
                else if(locReliability == telux::loc::LocationReliability::LOW)
                {
                    LocationData->horiReliablity = TAF_GNSS_RELIABILITY_LOW;
                    LE_INFO("horizontal reliablity is LOW");
                }
                else if(locReliability == telux::loc::LocationReliability::MEDIUM)
                {
                    LocationData->horiReliablity = TAF_GNSS_RELIABILITY_MEDIUM;
                    LE_INFO("horizontal reliablity is MEDIUM");
                }
                else if(locReliability == telux::loc::LocationReliability::HIGH)
                {
                    LocationData->horiReliablity = TAF_GNSS_RELIABILITY_HIGH;
                    LE_INFO("horizontal reliablity is HIGH");
                }
                else
                {
                    LocationData->horiReliablity = TAF_GNSS_RELIABILITY_UNKNOWN;
                    LE_INFO("horizontal reliablity is UNKNOWN");
                }
                telux::loc::LocationReliability vertLocReliability =
                                                   locationInfo->getVerticalReliability();
                if(vertLocReliability == telux::loc::LocationReliability::NOT_SET)
                {
                    LocationData->vertReliablity = TAF_GNSS_RELIABILITY_NOT_SET;
                    LE_INFO("vertical reliablity is NOT SET");
                }
                else if(vertLocReliability == telux::loc::LocationReliability::VERY_LOW)
                {
                    LocationData->vertReliablity = TAF_GNSS_RELIABILITY_VERY_LOW;
                    LE_INFO("vertical reliablity is VERY LOW");
                }
                else if(vertLocReliability == telux::loc::LocationReliability::LOW)
                {
                    LocationData->vertReliablity = TAF_GNSS_RELIABILITY_LOW;
                    LE_INFO("vertical reliablity is LOW");
                }
                else if(vertLocReliability == telux::loc::LocationReliability::MEDIUM)
                {
                    LocationData->vertReliablity = TAF_GNSS_RELIABILITY_MEDIUM;
                    LE_INFO("vertical reliablity is MEDIUM");
                }
                else if(vertLocReliability == telux::loc::LocationReliability::HIGH)
                {
                    LocationData->vertReliablity = TAF_GNSS_RELIABILITY_HIGH;
                    LE_INFO("vertical reliablity is HIGH");
                }
                else
                {
                    LocationData->vertReliablity = TAF_GNSS_RELIABILITY_UNKNOWN;
                    LE_INFO("vertical reliablity is UNKNOWN");
                }
                LocationData->azimuth = locationInfo->getHorizontalUncertaintyAzimuth();
                LocationData->eastDev = locationInfo->getEastStandardDeviation();
                LocationData->northDev = locationInfo->getNorthStandardDeviation();
                LocationData->realTime = locationInfo->getElapsedRealTime();
                LocationData->realTimeUnc = locationInfo->getElapsedRealTimeUncertainty();
                telux::loc::LocationTechnology techMask = locationInfo->getTechMask();
                if((techMask & telux::loc::LOC_GNSS))
                {
                    LocationData->techMask |= TAF_GNSS_LOC_GNSS;
                    LE_INFO("location calculated using GNSS");
                }
                if((techMask & telux::loc::LOC_CELL))
                {
                    LocationData->techMask |= TAF_GNSS_LOC_CELL;
                    LE_INFO("location calculated using CELL");
                }
                if((techMask & telux::loc::LOC_WIFI))
                {
                    LocationData->techMask |= TAF_GNSS_LOC_WIFI;
                    LE_INFO("location calculated using WIFI");
                }
                if((techMask & telux::loc::LOC_SENSORS))
                {
                    LocationData->techMask |= TAF_GNSS_LOC_SENSORS;
                    LE_INFO("location calculated using SENSORS");
                }
                if((techMask & telux::loc::LOC_REFERENCE_LOCATION))
                {
                    LocationData->techMask |= TAF_GNSS_LOC_REFERENCE_LOCATION;
                    LE_INFO("location calculated using Reference location");
                }
                if((techMask & telux::loc::LOC_INJECTED_COARSE_POSITION))
                {
                    LocationData->techMask |= TAF_GNSS_LOC_INJECTED_COARSE_POSITION;
                    LE_INFO("location calculated using Coarse position injected");
                }
                if((techMask & telux::loc::LOC_AFLT))
                {
                    LocationData->techMask |= TAF_GNSS_LOC_AFLT;
                    LE_INFO("location calculated using AFLT");
                }
                if((techMask & telux::loc::LOC_HYBRID))
                {
                    LocationData->techMask |= TAF_GNSS_LOC_HYBRID;
                    LE_INFO("location calculated using GNSS and network-provided measurements");
                }
                if((techMask & telux::loc::LOC_PPE))
                {
                    LocationData->techMask |= TAF_GNSS_LOC_PPE;
                    LE_INFO("location calculated using Precise position engine");
                }
                if((techMask & telux::loc::LOC_VEH))
                {
                    LocationData->techMask |= TAF_GNSS_LOC_VEH;
                    LE_INFO("location calculated using Vehicular data");
                }
                if((techMask & telux::loc::LOC_VIS))
                {
                    LocationData->techMask |= TAF_GNSS_LOC_VIS;
                    LE_INFO("location calculated using Visual data");
                }
                if((techMask & telux::loc::LOC_PROPAGATED))
                {
                    LocationData->techMask |= TAF_GNSS_LOC_PROPAGATED;
                    LE_INFO("location calculated using Propagation logic");
                }
                LocationData->next = LE_DLS_LINK_INIT;

                le_event_ReportWithRefCounting(gnss.positionEventId, LocationData);
                gnss.mSvEnabled = false;
                gnss.mGnssSigEnabled = false;
            }
        }
    }
    le_mutex_Unlock(gnss.mGnssMutexRef);
}

void tafLocationListener::onGnssSVInfo(const std::shared_ptr<telux::loc::IGnssSVInfo> &gnssSVInfo) {
    auto &gnss = taf_Gnss::GetInstance();

    //To get the constellation
    std::unique_lock<std::mutex> lock(gnss.mMutex);
    for(auto svInfo : gnssSVInfo->getSVInfoList()) {
        switch(svInfo->getConstellation()) {
            case telux::loc::GnssConstellationType::GPS:
                LE_DEBUG("onGnssSVInfo CONSTELLATION_GPS");
                break;
            case telux::loc::GnssConstellationType::GLONASS:
                LE_DEBUG("onGnssSVInfo CONSTELLATION_GLONASS");
                break;
            case telux::loc::GnssConstellationType::BDS:
                LE_DEBUG("onGnssSVInfo CONSTELLATION_BEIDOU");
                break;
            case telux::loc::GnssConstellationType::GALILEO:
                LE_DEBUG("onGnssSVInfo CONSTELLATION_GALILEO");
                break;
            case telux::loc::GnssConstellationType::SBAS:
                LE_DEBUG("onGnssSVInfo CONSTELLATION_SBAS");
                break;
            case telux::loc::GnssConstellationType::QZSS:
                LE_DEBUG("onGnssSVInfo CONSTELLATION_QZSS");
                break;
            case telux::loc::GnssConstellationType::NAVIC:
                LE_DEBUG("onGnssSVInfo CONSTELLATION_NAVIC");
                break;
            default:
                LE_ERROR("Constellation type: UNKNOWN");
                break;
        }
    }
    gnss.mCondVar.notify_one();
    le_mutex_Lock(gnss.mGnssMutexRef);
    if(gnss.NumOfPositionHandlers ) {
        LE_DEBUG("**** Satellite Vehicle Information ****");
        int i = 0;
        gnss.mConstellationEnabled = false;
        gnss.mSatParams.satsInViewCount = gnssSVInfo->getSVInfoList().size();
        gnss.mTotalSVTracked = 0;
        for(auto svInfo : gnssSVInfo->getSVInfoList()) {

            if(i >= TAF_GNSS_SV_INFO_MAX_LEN)
            {
                LE_WARN("SvInfo overflows");
                continue;
            }

            switch(svInfo->getConstellation()) {
                case telux::loc::GnssConstellationType::GPS:
                    gnss.mSatInfo[i].satConst = TAF_GNSS_SV_CONSTELLATION_GPS;
                    break;
                case telux::loc::GnssConstellationType::GLONASS:
                    gnss.mSatInfo[i].satConst = TAF_GNSS_SV_CONSTELLATION_GLONASS;
                    break;
                case telux::loc::GnssConstellationType::BDS:
                    gnss.mSatInfo[i].satConst = TAF_GNSS_SV_CONSTELLATION_BEIDOU;
                    break;
                case telux::loc::GnssConstellationType::GALILEO:
                    gnss.mSatInfo[i].satConst = TAF_GNSS_SV_CONSTELLATION_GALILEO;
                    break;
                case telux::loc::GnssConstellationType::SBAS:
                    gnss.mSatInfo[i].satConst = TAF_GNSS_SV_CONSTELLATION_SBAS;
                    break;
                case telux::loc::GnssConstellationType::QZSS:
                    gnss.mSatInfo[i].satConst = TAF_GNSS_SV_CONSTELLATION_QZSS;
                    break;
                default:
                    gnss.mSatInfo[i].satConst = TAF_GNSS_SV_CONSTELLATION_UNDEFINED;
                    LE_ERROR("Constellation type: UNKNOWN");
                    break;
            }

            gnss.mSatInfo[i].satId = svInfo->getId();
            if(svInfo->getHasFix() == SVInfoAvailability::YES)
            {
                gnss.mSatInfo[i].satUsed = true;
                LE_DEBUG("onGnssSVInfo: svInfo->getHasFix()->SVInfoAvailability::YES");
            }
            else
            {
                gnss.mSatInfo[i].satUsed = false;
            }
            if(svInfo->getSnr()!=0)
            {
                gnss.mTotalSVTracked++;
                gnss.mSatInfo[i].satTracked = true;
                LE_DEBUG("onGnssSVInfo: svInfo->getSnr() is NON ZERO");
            }
            else
            {
            gnss.mSatInfo[i].satTracked = false;
            }
            gnss.mSatInfo[i].satSnr = svInfo->getSnr();
            gnss.mSatInfo[i].satAzim = svInfo->getAzimuth();
            gnss.mSatInfo[i].satElev = svInfo->getElevation();
            gnss.mSatInfo[i].signalType = (uint32_t) svInfo->getSignalType();
            gnss.mSatInfo[i].glonassFcn = svInfo->getGlonassFcn();
            gnss.mSatInfo[i].baseBandCnr = svInfo->getBasebandCnr();
            i++;
        }
        gnss.mSvEnabled = true;
        gnss.mSatParams.satsTrackingCount = gnss.mTotalSVTracked;
    }
    le_mutex_Unlock(gnss.mGnssMutexRef);
}

void tafLocationListener::onGnssSignalInfo(
        const std::shared_ptr<telux::loc::IGnssSignalInfo> &gnssDatainfo) {
    auto &gnss = taf_Gnss::GetInstance();
    le_mutex_Lock(gnss.mGnssMutexRef);
    if(gnss.NumOfPositionHandlers ) {
        LE_DEBUG("**** Gnss Signal Information ****" );
        gnss.mGnssSigEnabled = true;
    }
    le_mutex_Unlock(gnss.mGnssMutexRef);
}

void tafLocationListener::onCapabilitiesInfo(
        const telux::loc::LocCapability capabilityInfo) {
    LE_INFO("onCapabilitiesInfo: The capabilityInfo is %d", (int) capabilityInfo);
    auto &gnss = taf_Gnss::GetInstance();
    le_mutex_Lock(gnss.mGnssMutexRef);
    if(gnss.NumOfCapabilityHandlers) {
        LE_DEBUG( "**** Gnss Capabilities Information ****" );
        CapabilityChangeEvent_t capabilityEvent;
        capabilityEvent.locCapability = (taf_gnss_LocCapabilityType_t) capabilityInfo;
        le_event_Report(gnss.locCapabilityEventId, &capabilityEvent, sizeof(CapabilityChangeEvent_t));
    }
    le_mutex_Unlock(gnss.mGnssMutexRef);
}

void tafLocationListener::onGnssNmeaInfo(uint64_t timestamp, const std::string &nmea) {
    auto &gnss = taf_Gnss::GetInstance();
    //Format : $GPGGA,075446.90,00-0.000000,S,00000.000000,E,1,00,1.0,936.4,M,-936.4,M,,*7D^M
    gnss.mNmeaBitMask = nmea;
    LE_DEBUG( "**** Gnss Nmea Information  gnss.mNmeaBitMask: %s****",gnss.mNmeaBitMask.c_str());
    std::unique_lock<std::mutex> lock(gnss.mMutex);

    if ((gnss.mNmeaBitMask.compare(3,3,"GGA",0,3)) ==0) //1
    {
        gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GGA;
        if ((gnss.mNmeaBitMask.compare(1,2,"GP",0,2)) ==0)
        {
            gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GPGGA;
        }
        LE_DEBUG("onGnssNmeaInfo: GGA");
    }
    if ((gnss.mNmeaBitMask.compare(3,3,"RMC",0,3)) ==0) //2
    {
        gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_RMC;
        if ((gnss.mNmeaBitMask.compare(1,2,"GP",0,2)) ==0)
        {
            gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GPRMC;
        }
        LE_DEBUG("onGnssNmeaInfo: RMC");
    }
    if ((gnss.mNmeaBitMask.compare(3,3,"GSA",0,3)) ==0) //4
    {
        gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GSA;
        if ((gnss.mNmeaBitMask.compare(1,2,"GN",0,2)) ==0)
        {
            gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GNGSA;
        }
        LE_DEBUG("onGnssNmeaInfo: GSA");
    }
    if ((gnss.mNmeaBitMask.compare(3,3,"VTG",0,3)) ==0) //8
    {
        gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_VTG;
        if ((gnss.mNmeaBitMask.compare(1,2,"GP",0,2)) ==0)
        {
            gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GPVTG;
        }
        LE_DEBUG("onGnssNmeaInfo: VTG");
    }
    if ((gnss.mNmeaBitMask.compare(3,3,"GNS",0,3)) ==0) //16
    {
        gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GNS;
        if ((gnss.mNmeaBitMask.compare(1,2,"GP",0,2)) ==0)
        {
            gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GPGNS;
        }
        LE_DEBUG("onGnssNmeaInfo: GNS");
    }
    if ((gnss.mNmeaBitMask.compare(3,3,"DTM",0,3)) ==0) //32
    {
        gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_DTM;
        if ((gnss.mNmeaBitMask.compare(1,2,"GP",0,2)) ==0)
        {
            gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GPDTM;
        }
        LE_DEBUG("onGnssNmeaInfo: DTM");
    }
    if((gnss.mNmeaBitMask.compare(1,5,"GPGSV",0,5)) ==0) //64
    {
        gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GPGSV;
        LE_DEBUG("onGnssNmeaInfo: GPGSV");
    }
    if((gnss.mNmeaBitMask.compare(1,5,"GLGSV",0,5)) ==0) //128
    {
        gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GLGSV;
        LE_DEBUG("onGnssNmeaInfo: GLGSV");
    }
    if ((gnss.mNmeaBitMask.compare(1,5,"GAGSV",0,5)) ==0) //256
    {
        gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GAGSV;
        LE_DEBUG("onGnssNmeaInfo: GAGSV");
    }
    if((gnss.mNmeaBitMask.compare(1,5,"GQGSV",0,5)) ==0) //512
    {
        gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GQGSV;
        LE_DEBUG("onGnssNmeaInfo: GQGSV");
    }
    if((gnss.mNmeaBitMask.compare(1,5,"GBGSV",0,5)) ==0) //1024
    {
        gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GBGSV;
        LE_DEBUG("onGnssNmeaInfo: GBGSV");
    }
    if((gnss.mNmeaBitMask.compare(1,5,"GIGSV",0,5)) ==0) //2048
    {
        gnss.mNmeaMask |= TAF_GNSS_NMEA_MASK_GIGSV;
        LE_DEBUG("onGnssNmeaInfo: GIGSV");
    }

    gnss.mNmeaVar.notify_one();
    le_mutex_Lock(gnss.mGnssMutexRef);
    LE_DEBUG("onGnssNmeaInfo: NumOfNmeaHandlers = %d", gnss.NumOfNmeaHandlers);
    if(gnss.NumOfNmeaHandlers) {
        LE_DEBUG( "**** Gnss Nmea Information ****" );
        //gnss.mSatMeas.satId = nmea;
        gnss.mSatMeas.satLatency = timestamp;

        NmeaInfoEvent_t nmeaEvent;
        nmeaEvent.timestamp = timestamp;
        const int length = gnss.mNmeaBitMask.length();
        nmeaEvent.nmeaMask[length] ='\0';
        for (int i = 0; i < length; i++)
        {
            nmeaEvent.nmeaMask[i] = gnss.mNmeaBitMask.c_str()[i];
        }
        LE_INFO( "**** NMEA handler string copied is: %s****",nmeaEvent.nmeaMask);
        le_event_Report(gnss.nmeaEventId, &nmeaEvent, sizeof(nmeaEvent));
    }
    le_mutex_Unlock(gnss.mGnssMutexRef);
}

void tafLocationListener::onGnssMeasurementsInfo(const telux::loc::
        GnssMeasurements &measurementInfo) {
    auto &gnss = taf_Gnss::GetInstance();
    le_mutex_Lock(gnss.mGnssMutexRef);
    if(gnss.NumOfPositionHandlers ) {
        LE_DEBUG("**** Gnss Measurements Information ****");
    }
    le_mutex_Unlock(gnss.mGnssMutexRef);
}

void tafLocationListener::onLocationSystemInfo(const telux::loc::LocationSystemInfo
        &locationSystemInfo) {
    auto &gnss = taf_Gnss::GetInstance();
    le_mutex_Lock(gnss.mGnssMutexRef);
    if(gnss.NumOfPositionHandlers ) {
        LE_DEBUG( "**** Location System Information ****" );
    }
    le_mutex_Unlock(gnss.mGnssMutexRef);
}

LocationCommandCallback::LocationCommandCallback(std::string cmdName) {
    auto &gnss = taf_Gnss::GetInstance();
    gnss.mCommandName = cmdName;
}

le_result_t taf_Gnss::PositionDataCoversion
(
 int32_t value,
 int8_t dataType,
 int32_t* dataPtr
)
{
    le_msg_SessionRef_t sessionRef = taf_gnss_GetClientSessionRef();
    taf_gnss_Resolution_t resolution = TAF_GNSS_RES_UNKNOWN;

    TAF_ERROR_IF_RET_VAL( dataPtr == NULL, LE_FAULT, "dataPtr is Null");

    taf_gnss_Client_t* clientReqPtr = DiscoverSessionRef(sessionRef);
    le_result_t result = LE_FAULT;

    if (NULL != clientReqPtr)
    {
        switch(dataType)
        {
            case TAF_GNSS_DATA_VACCURACY:
                resolution = clientReqPtr->vAccuracyResolution;
                break;
            case TAF_GNSS_DATA_VSPEEDACCURACY:
                resolution = clientReqPtr->vSpeedAccuracyResolution;
                break;
            case TAF_GNSS_DATA_HSPEEDACCURACY:
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
            case TAF_GNSS_DATA_VSPEEDACCURACY:
                resolution = TAF_GNSS_RES_ONE_DECIMAL;
                break;
            case TAF_GNSS_DATA_VACCURACY:
                resolution = TAF_GNSS_RES_THREE_DECIMAL;
                break;
            case TAF_GNSS_DATA_HSPEEDACCURACY:
                resolution = TAF_GNSS_RES_ONE_DECIMAL;
                break;
            default:
                LE_ERROR("Unsupported data type.");
                return result;
        }
    }

    switch(resolution)
    {
        case TAF_GNSS_RES_THREE_DECIMAL:
             *dataPtr = value;
             break;
        case TAF_GNSS_RES_TWO_DECIMAL:
             *dataPtr = value / 10;
             break;
        case TAF_GNSS_RES_ONE_DECIMAL:
             *dataPtr = value / 100;
             break;
        case TAF_GNSS_RES_ZERO_DECIMAL:
             *dataPtr = value / 1000;
             break;
        default:
             LE_ERROR("Unsupported resolution.");
             return result;
    }
    LE_DEBUG("resolution %d, value %" PRIi32 ", new value %" PRIi32, (int)resolution, value, *dataPtr);
    return LE_OK;
}

taf_gnss_Client_t* taf_Gnss::DiscoverSessionRef
(
    le_msg_SessionRef_t sessionRef
)
{
    auto &gnss = taf_Gnss::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(gnss.ClientRequestRefMap);
    le_result_t result = le_ref_NextNode(iterRef);

    while (LE_OK == result)
    {
        taf_gnss_Client_t* gnssPtr = (taf_gnss_Client_t*) le_ref_GetValue(iterRef);
        LE_ASSERT(gnssPtr != NULL);

        LE_DEBUG("gnssPtr %p, gnssPtr->sessionRef %p, sessionRef %p",
                 gnssPtr, gnssPtr->sessionRef, sessionRef);

        if (sessionRef == gnssPtr->sessionRef)
        {
             LE_DEBUG("SessionRef %p found in Client session", sessionRef);
             return gnssPtr;
        }
        result = le_ref_NextNode(iterRef);
    }
    return NULL;
}

le_result_t taf_Gnss::CheckValidatePosition
(
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
)
{
    TAF_KILL_CLIENT_IF_RET_VAL( (posSampleReqPtr == NULL) || (NULL == posSampleReqPtr->positionSampleNodePtr), LE_FAULT, "posSampleReqPtr is Null");
    return LE_OK;
}

void taf_Gnss::InitializeClient
(
    taf_gnss_Client_t* clientRequestPtr
)
{
    clientRequestPtr->dopResolution = TAF_GNSS_RES_THREE_DECIMAL;
    clientRequestPtr->vAccuracyResolution = TAF_GNSS_RES_THREE_DECIMAL;
    clientRequestPtr->vSpeedAccuracyResolution = TAF_GNSS_RES_ONE_DECIMAL;
    clientRequestPtr->hSpeedAccuracyResolution = TAF_GNSS_RES_ONE_DECIMAL;
}

taf_gnss_Client_t* taf_Gnss::AcquireSessionRef
(
    void
)
{
    auto &gnss = taf_Gnss::GetInstance();
    taf_gnss_Client_t* clientRequestPtr = NULL;
    le_msg_SessionRef_t sessionRef = taf_gnss_GetClientSessionRef();

    clientRequestPtr = DiscoverSessionRef(sessionRef);

    if (NULL == clientRequestPtr)
    {
        clientRequestPtr = (taf_gnss_Client_t*)le_mem_ForceAlloc(gnss.ClientPoolRef);

        InitializeClient(clientRequestPtr);

        void* reqRefPtr = le_ref_CreateRef(gnss.ClientRequestRefMap, clientRequestPtr);

        LE_DEBUG("SessionRef %p was not found, Create Client session", sessionRef);
        LE_DEBUG("reqRefPtr %p, clientRequestPtr %p", reqRefPtr, clientRequestPtr);

        clientRequestPtr->sessionRef = sessionRef;
        clientRequestPtr->clientRefPtr = reqRefPtr;
    }

    return clientRequestPtr;
}

uint32_t taf_Gnss::TranslateDop
(
    uint32_t dopValue
)
{
    uint16_t retVal = 0;

    taf_gnss_Client_t* clientRequestPtr = NULL;
    le_msg_SessionRef_t sessionRef = taf_gnss_GetClientSessionRef();
    taf_gnss_Resolution_t resolution = TAF_GNSS_RES_UNKNOWN;

    clientRequestPtr = DiscoverSessionRef(sessionRef);

    if (NULL != clientRequestPtr)
    {
        resolution = clientRequestPtr->dopResolution;
    }

    if ( TAF_GNSS_RES_ZERO_DECIMAL == resolution )
    {
        retVal = dopValue / 1e+3;
    } else if ( TAF_GNSS_RES_ONE_DECIMAL == resolution ) {
        retVal = dopValue /100;
    }  else if ( TAF_GNSS_RES_TWO_DECIMAL == resolution ) {
        retVal = dopValue /10;
    }  else {
        retVal = dopValue;
    }

    LE_DEBUG("resolution %d, dopValue %" PRIu32 ", new dopValue %" PRIu16, (int)resolution, dopValue, retVal);
    return retVal;
}

taf_gnss_State_t taf_Gnss::GetState
(
 void
)
{
    return GnssState;
}

le_result_t taf_Gnss::Enable
(
 void
)
{
    le_result_t result = LE_FAULT;

    switch (GnssState)
    {
        case TAF_GNSS_STATE_DISABLED:
        {
                GnssState = TAF_GNSS_STATE_READY;
                result = LE_OK;
                mStarted = false;
        }
        break;
        case TAF_GNSS_STATE_READY:
        case TAF_GNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_DUPLICATE;
        }
        break;
        case TAF_GNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", GnssState);
        }
        break;
    }

    return result;
}

le_result_t taf_Gnss::SetConstellation
(
    taf_gnss_ConstellationBitMask_t constellationMask
)
{

    le_result_t result = LE_FAULT;
    typedef std::vector<telux::loc::SvBlackListInfo> SvBlackList;
    SvBlackList svBlackList;
    telux::loc::SvBlackListInfo blackListInfo;
    bool deviceReset = false;
    blackListInfo.svId = 0; // Here 0 means blacklist all SVIds of a given constellation type
    blackListInfo.constellation = telux::loc::GnssConstellationType::UNKNOWN;

    LE_INFO("SetConstellation constellationMask is 0x%02X",constellationMask);
    if( constellationMask & TAF_GNSS_CONSTELLATION_GPS)
    {
        LE_INFO("constellation type GPS is not supported");
    }
    if( constellationMask & TAF_GNSS_CONSTELLATION_GLONASS)
    {
        blackListInfo.constellation = telux::loc::GnssConstellationType::GLONASS;
        svBlackList.push_back(blackListInfo);
        LE_INFO("constellation type is GLONASS");
    }
    if( constellationMask & TAF_GNSS_CONSTELLATION_BEIDOU)
    {
        blackListInfo.constellation = telux::loc::GnssConstellationType::BDS;
        svBlackList.push_back(blackListInfo);
        LE_INFO("constellation type is BEIDOU");
    }
    if( constellationMask & TAF_GNSS_CONSTELLATION_GALILEO)
    {
        blackListInfo.constellation = telux::loc::GnssConstellationType::GALILEO;
        svBlackList.push_back(blackListInfo);
        LE_INFO("constellation type is GALILEO");
    }
    if( constellationMask & TAF_GNSS_CONSTELLATION_SBAS)
    {
        blackListInfo.constellation = telux::loc::GnssConstellationType::SBAS;
        svBlackList.push_back(blackListInfo);
        LE_INFO("constellation type is SBAS");
    }
    if( constellationMask & TAF_GNSS_CONSTELLATION_QZSS)
    {
        blackListInfo.constellation = telux::loc::GnssConstellationType::QZSS;
        svBlackList.push_back(blackListInfo);
        LE_INFO("constellation type is QZSS");
    }
    if( constellationMask & TAF_GNSS_CONSTELLATION_NAVIC)
    {
        blackListInfo.constellation = telux::loc::GnssConstellationType::NAVIC;
        svBlackList.push_back(blackListInfo);
        LE_INFO("constellation type is NAVIC");
    }
    if (blackListInfo.constellation == telux::loc::GnssConstellationType::UNKNOWN)
    {
        svBlackList.push_back(blackListInfo);
        LE_INFO("constellation type is UNKNOWN");
    }

    switch (GnssState)
    {
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
        case TAF_GNSS_STATE_ACTIVE:
        {
            // Set GNSS constellation
            std::promise<le_result_t> p;
            auto cb = [&p](telux::common::ErrorCode error) {
                if(error == telux::common::ErrorCode::SUCCESS) {
                    p.set_value(LE_OK);
                }
                else {
                    p.set_value(LE_FAULT);
                }
            };

            telux::common::Status status =
                mLocationConfigurator->configureConstellations(svBlackList, cb, deviceReset);
            if(telux::common::Status::SUCCESS != status)
            {
                result = LE_FAULT;
            }
            else
            {
                if(p.get_future().get() == LE_OK)
                {
                    result = LE_OK;
                    mConstellationMask = constellationMask;
                    LE_INFO("SetConstellation is success");
                }
                else
                {
                    result = LE_FAULT;
                }
            }
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", GnssState);
        }
        break;
    }

    return result;
}

le_result_t taf_Gnss::Start
(
    void
)
{
    le_result_t result = LE_FAULT;

    switch (GnssState)
    {
        case TAF_GNSS_STATE_READY:
        {
            // Start GNSS
            if (!mStarted)
            {
                int optInterval = mAcqRate;
                LE_INFO("Start->  mAcqRate: %d",mAcqRate);
                if( optInterval == 0  || optInterval < 100)
                {
                    LE_DEBUG("Start->mAcqRate is zero, so set default to 100ms");
                    optInterval = 100;
                    mAcqRate = optInterval;
                }
                LocReqEngine engineType = DEFAULT_UNKNOWN;
                GnssReportTypeMask reportMask = DEFAULT_UNKNOWN;
                reportMask = 0x7f;//all reports are enabled
                LE_INFO("Start->reportMask : %u",reportMask);
                LE_INFO("Start->mEngineType : %d",mEngineType);
                engineType |= (1UL << mEngineType);//FUSED mode is supported by default

                std::promise<le_result_t> p;
                auto cb = [&p](telux::common::ErrorCode error) {
                    if(error == telux::common::ErrorCode::SUCCESS) {
                        p.set_value(LE_OK);
                    }
                    else {
                        p.set_value(LE_FAULT);
                    }
                };
                auto status = mLocationManager->startDetailedEngineReports(
                        (uint32_t)optInterval, engineType, cb, reportMask);
                if(telux::common::Status::SUCCESS != status)
                {
                    result = LE_FAULT;
                }
                else
                {
                    if(p.get_future().get() == LE_OK)
                    {
                        mStarted = true;
                        GnssState = TAF_GNSS_STATE_ACTIVE;
                        result = LE_OK;
                        LE_INFO("Start() is success");
                        mStartTime = std::chrono::system_clock::now();
                        mTtffEnabled = true;
                    }
                    else
                    {
                        result = LE_FAULT;
                        LE_INFO("Start() is failed");
                    }
                }
            }
        }
        break;
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_DUPLICATE;
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", GnssState);
        }
        break;
    }
    return result;
}

le_result_t taf_Gnss::GetSatellitesStatus
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint8_t* satsInViewCountPtr,
    uint8_t* satsTrackingCountPtr,
    uint8_t* satsUsedCountPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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

le_result_t taf_Gnss::GetAcquisitionRate
(
    uint32_t* ratePtr
)
{
    le_result_t result = LE_FAULT;

    TAF_KILL_CLIENT_IF_RET_VAL((ratePtr == NULL), LE_FAULT, "Pointer is NULL");

    switch (GnssState)
    {
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
        case TAF_GNSS_STATE_ACTIVE:
        {
            // Set the GNSS device acquisition rate
            *ratePtr = mAcqRate;
            result = LE_OK;
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", GnssState);
        }
        break;
    }

    return result;
}

le_result_t taf_Gnss::GetSatellitesInfo
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
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);
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
                    satConstPtr[i] = TAF_GNSS_SV_CONSTELLATION_UNDEFINED;
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

le_result_t taf_Gnss::GetTtff
(
    uint32_t* ttffPtr
)
{

    TAF_KILL_CLIENT_IF_RET_VAL((ttffPtr == NULL), LE_FAULT, "ttffPtr is NULL");

    le_result_t result = LE_FAULT;

    switch (GnssState)
    {
        case TAF_GNSS_STATE_DISABLED:
        case TAF_GNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
        case TAF_GNSS_STATE_ACTIVE:
        {
// Start Detailed Engine Reports, if not already started

            if(!mTtffPtr && !mTtffEnabled) //calculate ttff on device boot up & cold/warm/hot restart procedure
            {
                std::chrono::duration<double> elapsedTime = mEndTime - mStartTime;
                *ttffPtr = elapsedTime.count() * 1e+3;
                mTtffPtr = *ttffPtr ;
                mTtffEnable = true;
            }
            else //fetch the first TTFF value & return back
            {
                *ttffPtr = mTtffPtr;
            }
            result = LE_OK;
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", GnssState);
        }
        break;
    }
    return result;
}

taf_gnss_PositionHandlerRef_t taf_Gnss::AddPositionHandler
(
 taf_gnss_PositionHandlerFunc_t handlerPtr,
 void* contextPtr
)
{
    taf_gnss_PositionHandler_t*  positionHandlerPtr =
        (taf_gnss_PositionHandler_t*)le_mem_ForceAlloc(PositionHandlerPoolRef);
    memset(positionHandlerPtr, 0, sizeof(taf_gnss_PositionHandler_t));
    positionHandlerPtr->next = LE_DLS_LINK_INIT;
    positionHandlerPtr->handlerFuncPtr = handlerPtr;
    positionHandlerPtr->handlerContextPtr = contextPtr;
    positionHandlerPtr->sessionRef = taf_gnss_GetClientSessionRef();
    positionHandlerPtr->handlerRef =
        (taf_gnss_PositionHandlerRef_t)le_ref_CreateRef(PositionHandlerRefMap, positionHandlerPtr);

    NumOfPositionHandlers++;

    LE_DEBUG("Created positionHandlerRef(%p) for positionHandlerPtr(%p) (totalCnt=0x%x).",
        positionHandlerPtr->handlerRef, positionHandlerPtr, NumOfPositionHandlers);

    return positionHandlerPtr->handlerRef;
}

taf_gnss_CapabilityChangeHandlerRef_t taf_Gnss::AddCapabilityHandler
(
    taf_gnss_CapabilityChangeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    le_event_HandlerRef_t handlerRef;

    TAF_KILL_CLIENT_IF_RET_VAL(handlerPtr == NULL, NULL, "Handler pointer is NULL");

    handlerRef = le_event_AddLayeredHandler("CapabilityHandler", locCapabilityEventId,
            FirstLayerCapabilityHandler, (void*)handlerPtr);

    NumOfCapabilityHandlers++;

    return (taf_gnss_CapabilityChangeHandlerRef_t) handlerRef;
}

void taf_Gnss::RemoveCapabilityHandler (taf_gnss_CapabilityChangeHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    if (NumOfCapabilityHandlers > 0) {
        NumOfCapabilityHandlers--;
    }
}

void taf_Gnss::FirstLayerCapabilityHandler(void* reportPtr,
        void* secondLayerHandlerFunc)
{
    CapabilityChangeEvent_t* capEventPtr = (CapabilityChangeEvent_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(capEventPtr == NULL,"CapabilityChangeEventPtr is NULL");

    taf_gnss_CapabilityChangeHandlerFunc_t clientHandlerFunc =
        (taf_gnss_CapabilityChangeHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(capEventPtr->locCapability, le_event_GetContextPtr());
}

taf_gnss_NmeaHandlerRef_t taf_Gnss::AddNmeaHandler
(
    taf_gnss_NmeaHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    le_event_HandlerRef_t handlerRef;

    TAF_KILL_CLIENT_IF_RET_VAL(handlerPtr == NULL, NULL, "Handler pointer is NULL");

    handlerRef = le_event_AddLayeredHandler("NmeaHandler", nmeaEventId,
            FirstLayerNmeaHandler, (void*)handlerPtr);

    NumOfNmeaHandlers++;

    return (taf_gnss_NmeaHandlerRef_t) handlerRef;
}

void taf_Gnss::RemoveNmeaHandler (taf_gnss_NmeaHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    if (NumOfNmeaHandlers > 0) {
        NumOfNmeaHandlers--;
    }
}

void taf_Gnss::FirstLayerNmeaHandler(void* reportPtr,
        void* secondLayerHandlerFunc)
{
    NmeaInfoEvent_t* nmeaEventPtr = (NmeaInfoEvent_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(nmeaEventPtr == NULL,"NmeaEventPtr is NULL");

    taf_gnss_NmeaHandlerFunc_t clientHandlerFunc =
        (taf_gnss_NmeaHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(nmeaEventPtr->timestamp, nmeaEventPtr->nmeaMask, le_event_GetContextPtr());
}

le_result_t taf_Gnss::GetConstellation
(
    taf_gnss_ConstellationBitMask_t *constellationMaskPtr
)
{
    le_result_t result = LE_FAULT;

    TAF_KILL_CLIENT_IF_RET_VAL((constellationMaskPtr == NULL), LE_FAULT, "constellationMaskPtr is NULL");
    //mConstellationEnabled = true;

    switch (GnssState)
    {
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_DISABLED:
            {
                LE_ERROR("Bad state for that request [%d]", GnssState);
                result = LE_NOT_PERMITTED;
            }
            break;
        case TAF_GNSS_STATE_READY:
        case TAF_GNSS_STATE_ACTIVE:
            {
                // Get GNSS constellation
                if(mConstellationMask & TAF_GNSS_CONSTELLATION_GPS)
                {
                    *constellationMaskPtr |= TAF_GNSS_CONSTELLATION_GPS;
                    result = LE_OK;
                    LE_DEBUG("constellation type is TAF_GNSS_CONSTELLATION_GPS");
                }
                if(mConstellationMask & TAF_GNSS_CONSTELLATION_GLONASS)
                {
                    *constellationMaskPtr |= TAF_GNSS_CONSTELLATION_GLONASS;
                    result = LE_OK;
                    LE_DEBUG("constellation type is TAF_GNSS_CONSTELLATION_GLONASS");
                }
                if(mConstellationMask & TAF_GNSS_CONSTELLATION_BEIDOU)
                {
                    *constellationMaskPtr |= TAF_GNSS_CONSTELLATION_BEIDOU;
                    result = LE_OK;
                    LE_DEBUG("constellation type is TAF_GNSS_CONSTELLATION_BEIDOU");
                }
                if(mConstellationMask & TAF_GNSS_CONSTELLATION_GALILEO)
                {
                    *constellationMaskPtr |= TAF_GNSS_CONSTELLATION_GALILEO;
                    result = LE_OK;
                    LE_DEBUG("constellation type is TAF_GNSS_CONSTELLATION_GALILEO");
                }
                if(mConstellationMask & TAF_GNSS_CONSTELLATION_SBAS)
                {
                    *constellationMaskPtr |= TAF_GNSS_CONSTELLATION_SBAS;
                    result = LE_OK;
                    LE_DEBUG("constellation type is TAF_GNSS_CONSTELLATION_SBAS");
                }
                if(mConstellationMask & TAF_GNSS_CONSTELLATION_QZSS)
                {
                    *constellationMaskPtr |= TAF_GNSS_CONSTELLATION_QZSS;
                    result = LE_OK;
                    LE_DEBUG("constellation type is TAF_GNSS_CONSTELLATION_QZSS");
                }
                if(mConstellationMask & TAF_GNSS_CONSTELLATION_NAVIC)
                {
                    *constellationMaskPtr |= TAF_GNSS_CONSTELLATION_NAVIC;
                    result = LE_OK;
                    LE_DEBUG("constellation type is TAF_GNSS_CONSTELLATION_NAVIC");
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
                LE_ERROR("Unknown GNSS state %d", GnssState);
            }
            break;
    }

    return result;
}

taf_gnss_SampleRef_t taf_Gnss::GetLastSampleRef
(
    void
)
{
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr =
        (taf_gnss_PositionSampleRequest_t*)le_mem_ForceAlloc(PositionSampleRequestPoolRef);

    memset(posSampleReqPtr, 0, sizeof(taf_gnss_PositionSampleRequest_t));

    posSampleReqPtr->positionSampleNodePtr =
       (taf_gnss_PositionSample_t*)le_mem_ForceAlloc(PositionSamplePoolRef);
    memset(posSampleReqPtr->positionSampleNodePtr, 0, sizeof(taf_gnss_PositionSample_t));

    memcpy(posSampleReqPtr->positionSampleNodePtr, &LastPositionSample, sizeof(taf_gnss_PositionSample_t));


    LE_DEBUG("Get sample %p", posSampleReqPtr->positionSampleNodePtr);

    posSampleReqPtr->sessionRef = taf_gnss_GetClientSessionRef();

    taf_gnss_SampleRef_t reqRef = (taf_gnss_SampleRef_t)le_ref_CreateRef(PositionSampleMap, posSampleReqPtr);
    posSampleReqPtr->positionSampleRef = reqRef;

    return reqRef;
}

le_result_t taf_Gnss::GetPositionState
(
 taf_gnss_SampleRef_t positionSampleRef,
 taf_gnss_FixState_t* statePtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr =
                                                (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(gnss.PositionSampleMap,positionSampleRef);

    TAF_KILL_CLIENT_IF_RET_VAL((statePtr == NULL), LE_FAULT, "statePtr is NULL");

    result = gnss.CheckValidatePosition(posSampleReqPtr);
    if (LE_OK != result)
    {
        return result;
    }


    if (posSampleReqPtr->positionSampleNodePtr->satsUsedCount <3)
    {
        posSampleReqPtr->positionSampleNodePtr->fixState = TAF_GNSS_STATE_FIX_ESTIMATED;
        LE_INFO("FixState is Estimated");
    }
    else if ((posSampleReqPtr->positionSampleNodePtr->satsUsedCount ==3) ||
             ((posSampleReqPtr->positionSampleNodePtr->satsUsedCount >3) &&
              (posSampleReqPtr->positionSampleNodePtr->altitudeValid == false))||
             ((posSampleReqPtr->positionSampleNodePtr->satsUsedCount >3) &&
              (posSampleReqPtr->positionSampleNodePtr->altitudeValid == true)
                && (mAltType == TAF_GNSS_ALT_TYPE_ASSUMED)))
    {
        posSampleReqPtr->positionSampleNodePtr->fixState = TAF_GNSS_STATE_FIX_2D;
        LE_INFO("FixState is 2D");
    }
    else
    {
        posSampleReqPtr->positionSampleNodePtr->fixState = TAF_GNSS_STATE_FIX_3D;
        LE_INFO("FixState is 3D");
    }
    *statePtr = posSampleReqPtr->positionSampleNodePtr->fixState;

    return result;
}

le_result_t taf_Gnss::GetDirection
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint32_t* directionPtr,

    uint32_t* directionAccuracyPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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

le_result_t taf_Gnss::GetDate
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint16_t* yearPtr,
    uint16_t* monthPtr,
    uint16_t* dayPtr
)
{

    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr =
                                                (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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

le_result_t taf_Gnss::GetTime
(
 taf_gnss_SampleRef_t positionSampleRef,
 uint16_t* hrsPtr,
 uint16_t* minPtr,
 uint16_t* secPtr,
 uint16_t* msecPtr
 )
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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

le_result_t taf_Gnss::GetLocation
(
 taf_gnss_SampleRef_t positionSampleRef,
 int32_t* latitudePtr,
 int32_t* longitudePtr,
 int32_t* hAccuracyPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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

le_result_t taf_Gnss::GetAltitude
(
 taf_gnss_SampleRef_t positionSampleRef,
 int32_t* altitudePtr,
 int32_t* vAccuracyPtr
 )
{
    auto &gnss = taf_Gnss::GetInstance();
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t * posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
                                     TAF_GNSS_DATA_VACCURACY,
                                     vAccuracyPtr))
           )
        {
            *vAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}

le_result_t taf_Gnss::GetHorizontalSpeed
(
 taf_gnss_SampleRef_t positionSampleRef,
 uint32_t* hspeedPtr,
 uint32_t* hspeedAccuracyPtr
 )
{
    auto &gnss = taf_Gnss::GetInstance();
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
                                TAF_GNSS_DATA_HSPEEDACCURACY,
                                (int32_t*)hspeedAccuracyPtr))
           )
        {
            *hspeedAccuracyPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

le_result_t taf_Gnss::GetVerticalSpeed
(
 taf_gnss_SampleRef_t positionSampleRef,
 int32_t* vspeedPtr,
 int32_t* vspeedAccuracyPtr
 )
{
    auto &gnss = taf_Gnss::GetInstance();
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
                                TAF_GNSS_DATA_VSPEEDACCURACY,
                                vspeedAccuracyPtr))
            )
        {
            *vspeedAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

le_result_t taf_Gnss::GetGpsLeapSeconds
(
 taf_gnss_SampleRef_t positionSampleRef,
 uint8_t* leapSecondsPtr
)
{
    le_result_t result;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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

le_result_t taf_Gnss::GetMagneticDeviation
(
 taf_gnss_SampleRef_t positionSampleRef,
 int32_t* magneticDeviationPtr
)
{
    le_result_t result;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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

le_result_t taf_Gnss::GetEllipticalUncertainty
(
 taf_gnss_SampleRef_t positionSampleRef,
 uint32_t* horUncEllipseSemiMajorPtr,
 uint32_t* horUncEllipseSemiMinorPtr,
 uint8_t*  horConfidencePtr
)
{
    le_result_t result;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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

le_result_t taf_Gnss::GetTimeAccuracy
(
 taf_gnss_SampleRef_t posRef,
 uint32_t* timeAccuracyPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posReqPtr
                                            = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,posRef);

    TAF_KILL_CLIENT_IF_RET_VAL((timeAccuracyPtr == NULL), LE_FAULT, "Invalid reference");

    result = CheckValidatePosition(posReqPtr);
    if (LE_OK != result)
    {
        return result;
    }

    if (posReqPtr->positionSampleNodePtr->timeAccuracyValid)
    {
        result = LE_OK;
        if (timeAccuracyPtr)
        {
            *timeAccuracyPtr = posReqPtr->positionSampleNodePtr->timeAccuracy;
        }
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        if (timeAccuracyPtr)
        {
            *timeAccuracyPtr = UINT16_MAX;
        }
    }

    return result;
}

le_result_t taf_Gnss::GetEpochTime
(
 taf_gnss_SampleRef_t posRef,
 uint64_t* millisecondsPtr
)
{
    le_result_t result;
    taf_gnss_PositionSampleRequest_t* posReqPtr
                                            = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,posRef);

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

le_result_t taf_Gnss::SetDopResolution
(
 taf_gnss_Resolution_t resolution
)
{
    taf_gnss_Client_t* clientRequestPtr = NULL;

    TAF_ERROR_IF_RET_VAL( resolution >= TAF_GNSS_RES_UNKNOWN, LE_BAD_PARAMETER, "Invalid resolution");

    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    clientRequestPtr->dopResolution = resolution;
    LE_DEBUG("clientRequest %p, resolution %d saved", clientRequestPtr, (int)resolution);
    return LE_OK;
}


le_result_t taf_Gnss::GetDilutionOfPrecision
(
 taf_gnss_SampleRef_t posRef,
 taf_gnss_DopType_t dopType,
 uint16_t* dopPtr
)
{
    uint32_t dop;
    bool dopValid = false;
    taf_gnss_PositionSampleRequest_t* posReqPtr = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,posRef);
    le_result_t result = CheckValidatePosition(posReqPtr);
    if (LE_OK != result)
    {
        return result;
    }

    if (dopPtr)
    {
        *dopPtr = UINT16_MAX;

        if( TAF_GNSS_PDOP == dopType) {
            if (posReqPtr->positionSampleNodePtr->pdopValid)
            {
                dop = TranslateDop(posReqPtr->positionSampleNodePtr->pdop);
                dopValid = true;
            }
        } else if( TAF_GNSS_HDOP == dopType) {
            if (posReqPtr->positionSampleNodePtr->hdopValid)
            {
                dop = TranslateDop(posReqPtr->positionSampleNodePtr->hdop);
                dopValid = true;
            }
        } else if( TAF_GNSS_VDOP == dopType) {
            if (posReqPtr->positionSampleNodePtr->vdopValid)
            {
                dop = TranslateDop(posReqPtr->positionSampleNodePtr->vdop);
                dopValid = true;
            }
        } else if ( TAF_GNSS_GDOP == dopType) {
            if (posReqPtr->positionSampleNodePtr->gdopValid)
            {
                dop = TranslateDop(posReqPtr->positionSampleNodePtr->gdop);
                dopValid = true;
            }
        } else if( TAF_GNSS_TDOP == dopType) {
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

le_result_t taf_Gnss::GetLeapSeconds
(
 uint64_t* gpsTime,
 int32_t* currentLeapSeconds,
 uint64_t* changeEventTime,
 int32_t* nextLeapSeconds
)
{
    LE_DEBUG("GetLeapSeconds is not implemented");
    return LE_UNSUPPORTED;
}

le_result_t taf_Gnss::GetGpsTime
(
    taf_gnss_SampleRef_t posRef,
    uint32_t* gpsWeek,
    uint32_t* gpsTimeOfWeek
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posReqPtr = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,posRef);

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

le_result_t taf_Gnss::SetAcquisitionRate
(
    uint32_t  rate
)
{
    le_result_t result;

    TAF_ERROR_IF_RET_VAL( 0 == rate, LE_OUT_OF_RANGE, "Acquisition rate is zero");

    // Check the GNSS device state
    switch (GnssState)
    {
        case TAF_GNSS_STATE_READY:
        {
            // Set the GNSS device acquisition rate
            if(rate < 100)
            {
                rate = 100;
                LE_DEBUG("SetAcquisitionRate -> mAcqRate is less than 100ms, so set default to 100ms");
            }
            mAcqRate = rate;
            result = LE_OK;
        }
        break;
        case TAF_GNSS_STATE_ACTIVE:
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_DISABLED:
        {
            result = LE_NOT_PERMITTED;
            LE_ERROR("Bad state for that request [%d]", GnssState);
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", GnssState);
        }
        break;
    }

    return result;
}

le_result_t taf_Gnss::ForceColdRestart
(
    void
)
{
    le_result_t result = LE_OK;

    switch (GnssState)
    {
        case TAF_GNSS_STATE_READY:
        case TAF_GNSS_STATE_DISABLED:
        case TAF_GNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_ACTIVE:
            {
               //Delete All Aiding Data
                std::promise<le_result_t> p1;
                auto cb1 = [&p1](telux::common::ErrorCode error) {
                    if(error == telux::common::ErrorCode::SUCCESS) {
                        p1.set_value(LE_OK);
                    }
                    else {
                        p1.set_value(LE_FAULT);
                    }
                };

                LE_INFO("ForceColdRestart mStarted: %d", mStarted);

                telux::common::Status status = mLocationConfigurator->deleteAllAidingData(cb1);
                if (status != telux::common::Status::SUCCESS)
                {
                    if (status == telux::common::Status::NOTIMPLEMENTED)
                    {
                        LE_ERROR("ForceColdRestart failed or Not Implemented");
                    }
                    result = LE_FAULT;
                }
                else
                {
                    std::future<le_result_t> futResult = p1.get_future();
                    if(futResult.get() != LE_OK)
                    {
                        result = LE_FAULT;
                    }
                }
                if(result == LE_OK)
                {
                    // stop Detailed Reports
                    if (mStarted)
                    {
                        std::promise<le_result_t> p2;
                        auto cb2 = [&p2](telux::common::ErrorCode error) {
                            if(error == telux::common::ErrorCode::SUCCESS) {
                                p2.set_value(LE_OK);
                            }
                            else {
                                p2.set_value(LE_FAULT);
                            }
                        };
                        auto status = mLocationManager->stopReports(cb2);
                        if(status != telux::common::Status::SUCCESS)
                        {
                            result = LE_FAULT;
                        }
                        else
                        {
                            std::future<le_result_t> futResult = p2.get_future();
                            if(futResult.get() == LE_OK)
                            {
                                mStarted = false;
                                GnssState = TAF_GNSS_STATE_READY;
                                LE_INFO("ForceColdRestart->Stop() is success");
                            }
                            else
                            {
                                LE_INFO("ForceColdRestart->Stop() is failed");
                                result = LE_FAULT;
                            }
                        }
                    }
                }
                if(result == LE_OK)
                {
                    //start Detailed Engine report
                    if (!mStarted) {
                        int optInterval = mAcqRate;
                        LE_INFO("ForceColdRestart->  optInterval: %d",optInterval);
                        if( optInterval == 0  || optInterval < 100) {
                            LE_DEBUG("ForceColdRestart mAcqRate is zero, so set default to 100ms");
                            optInterval = 100;
                            mAcqRate = optInterval;
                        }

                        LocReqEngine engineType = DEFAULT_UNKNOWN;
                        GnssReportTypeMask reportMask = DEFAULT_UNKNOWN;
                        reportMask = 0x7f;//all reports are enabled
                        LE_INFO("ForceColdRestart->reportMask : %u",reportMask);
                        engineType |= (1UL << mEngineType);

                        std::promise<le_result_t> p;
                        auto cb = [&p](telux::common::ErrorCode error) {
                            if(error == telux::common::ErrorCode::SUCCESS) {
                                p.set_value(LE_OK);
                            }
                            else {
                                p.set_value(LE_FAULT);
                            }
                        };

                        LE_DEBUG("ForceColdRestart ->startDetailedEngineReports()");
                        auto status = mLocationManager->startDetailedEngineReports(
                            (uint32_t)optInterval, engineType, cb, reportMask);
                        if(status != telux::common::Status::SUCCESS)
                        {
                            result = LE_FAULT;
                        }
                        else
                        {
                            std::future<le_result_t> futResult = p.get_future();
                            if(futResult.get() == LE_OK)
                            {
                                mStarted = true;
                                GnssState = TAF_GNSS_STATE_ACTIVE;
                                LE_INFO("ForceColdRestart->Start() is success");
                                mTtffPtr = 0; //reset TTFF value
                                mStartTime = std::chrono::system_clock::now();
                                mTtffEnabled = true;
                            }
                            else
                            {
                                LE_INFO("ForceColdRestart->Start() is failed");
                                result = LE_FAULT;
                            }
                        }
                    }
                }
            }
        break;
        default:
        {
            LE_ERROR("Invalid GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

le_result_t taf_Gnss::ForceWarmRestart
(
    void
)
{
    le_result_t result = LE_OK;

    switch (GnssState)
    {
        case TAF_GNSS_STATE_READY:
        case TAF_GNSS_STATE_DISABLED:
        case TAF_GNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss state [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_ACTIVE:
        {
                std::promise<le_result_t> p1;
                auto cb1 = [&p1](telux::common::ErrorCode error) {
                    if(error == telux::common::ErrorCode::SUCCESS) {
                        p1.set_value(LE_OK);
                    }
                    else {
                        p1.set_value(LE_FAULT);
                    }
                };

                /* Specifies AidingDataType mask */
                /* 0 - EPHEMERIS 1 - DR_SENSOR_CALIBRATION
                   AidingData |1UL << (0,1) which is 3*/

                uint32_t AidingData = 3;
                telux::common::Status status = mLocationConfigurator->deleteAidingData(AidingData, cb1);
                if (status != telux::common::Status::SUCCESS)
                {
                    if (status == telux::common::Status::NOTIMPLEMENTED)
                    {
                        LE_ERROR("ForceWarmRestart failed or Not Implemented");
                    }
                    result = LE_FAULT;
                }
                else
                {
                    std::future<le_result_t> futResult = p1.get_future();
                    if(futResult.get() != LE_OK)
                    {
                        result = LE_FAULT;
                    }
                }

                if(result == LE_OK)
                {
                    // stop Detailed Reports
                    if (mStarted)
                    {
                        std::promise<le_result_t> p2;
                        auto cb2 = [&p2](telux::common::ErrorCode error) {
                            if(error == telux::common::ErrorCode::SUCCESS) {
                                p2.set_value(LE_OK);
                            }
                            else {
                                p2.set_value(LE_FAULT);
                            }
                        };

                        status = mLocationManager->stopReports(cb2);
                        if(status != telux::common::Status::SUCCESS)
                        {
                            result = LE_FAULT;
                        }
                        else
                        {
                            std::future<le_result_t> futResult = p2.get_future();
                            if(futResult.get() == LE_OK)
                            {
                                mStarted = false;
                                GnssState = TAF_GNSS_STATE_READY;
                                LE_INFO("ForceWarmRestart->Stop() is success");
                            }
                            else
                            {
                                LE_INFO("ForceWarmRestart->Stop() is failed");
                                result = LE_FAULT;
                            }
                        }
                    }
                    else
                    {
                        result = LE_FAULT;
                    }
                }
                if(result == LE_OK)
                {
                    //start Detailed Engine report
                    if (!mStarted) {
                        int optInterval = mAcqRate;
                        std::promise<le_result_t> p;
                        auto cb = [&p](telux::common::ErrorCode error) {
                            if(error == telux::common::ErrorCode::SUCCESS) {
                                p.set_value(LE_OK);
                            }
                            else {
                                p.set_value(LE_FAULT);
                            }
                        };
                        LE_INFO("ForceWarmRestart->  optInterval: %d",optInterval);
                        if( optInterval == 0  || optInterval < 100) {
                            LE_DEBUG("ForceWarmRestart mAcqRate is zero, so set default to 100ms");
                            optInterval = 100;
                            mAcqRate = optInterval;
                        }

                        LocReqEngine engineType = DEFAULT_UNKNOWN;
                        GnssReportTypeMask reportMask = DEFAULT_UNKNOWN;
                        reportMask = 0x7f;//all reports are enabled
                        LE_INFO("ForceWarmRestart->reportMask : %u",reportMask);
                        engineType |= (1UL << mEngineType);
                        LE_DEBUG("ForceWarmRestart ->startDetailedEngineReports()");
                        status = mLocationManager->startDetailedEngineReports((uint32_t)optInterval,
                                engineType, cb, reportMask);
                        if(status != telux::common::Status::SUCCESS)
                        {
                            result = LE_FAULT;
                        }
                        else
                        {
                            std::future<le_result_t> futResult = p.get_future();
                            if(futResult.get() == LE_OK)
                            {
                                mStarted = true;
                                GnssState = TAF_GNSS_STATE_ACTIVE;
                                LE_INFO("ForceWarmRestart->Start() is success");
                                mTtffPtr = 0; //reset TTFF value
                                mStartTime = std::chrono::system_clock::now();
                                mTtffEnabled = true;
                            }
                            else
                            {
                                LE_INFO("ForceWarmRestart->Start() is failed");
                                result = LE_FAULT;
                            }
                        }
                    }
                }
        }
        break;
        default:
        {
            LE_ERROR("Invalid GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

le_result_t taf_Gnss::ForceHotRestart
(
    void
)
{
    le_result_t result = LE_OK;

    switch (GnssState)
    {
        case TAF_GNSS_STATE_READY:
        case TAF_GNSS_STATE_DISABLED:
        case TAF_GNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_ACTIVE:
            {
                // stop Detailed Reports
                if (mStarted) {
                    std::promise<le_result_t> p;
                    auto cb = [&p](telux::common::ErrorCode error) {
                        if(error == telux::common::ErrorCode::SUCCESS) {
                            p.set_value(LE_OK);
                        }
                        else {
                            p.set_value(LE_FAULT);
                        }
                    };
                    auto status = mLocationManager->stopReports(cb);
                    if(status != telux::common::Status::SUCCESS)
                    {
                        result = LE_FAULT;
                        return result;
                    }
                    else
                    {
                        std::future<le_result_t> futResult = p.get_future();
                        if(futResult.get() == LE_OK)
                        {
                            mStarted = false;
                            GnssState = TAF_GNSS_STATE_READY;
                            LE_INFO("ForceHotRestart->Stop() is success");
                        }
                        else
                        {
                            LE_INFO("ForceHotRestart->Stop() is failed");
                            result = LE_FAULT;
                            return result;
                        }
                    }
                }
                sleep(5); // 5sec sleep required to stop and start Gnss engine

                //start Detailed report
                if (!mStarted) {
                    int optInterval = mAcqRate;
                    std::promise<le_result_t> p;
                    auto cb = [&p](telux::common::ErrorCode error) {
                        if(error == telux::common::ErrorCode::SUCCESS) {
                            p.set_value(LE_OK);
                        }
                        else {
                            p.set_value(LE_FAULT);
                        }
                    };
                    LE_INFO("ForceHotRestart->  optInterval: %d",optInterval);
                    if( optInterval == 0  || optInterval < 100) {
                        LE_DEBUG("ForceHotRestart()->mAcqRate is zero, so set default to 100ms");
                        optInterval = 100;
                        mAcqRate = optInterval;
                    }
                    LocReqEngine engineType = DEFAULT_UNKNOWN;
                    GnssReportTypeMask reportMask = DEFAULT_UNKNOWN;
                    reportMask = 0x7f;//all reports are enabled
                    LE_INFO("ForceHotRestart->reportMask : %u",reportMask);
                    engineType |= (1UL << mEngineType);
                    auto status = mLocationManager->startDetailedEngineReports((uint32_t)optInterval,
                        engineType, cb, reportMask);
                    if(status != telux::common::Status::SUCCESS)
                    {
                        result = LE_FAULT;
                    }
                    else
                    {
                        LE_DEBUG("ForceHotRestart()->startDetailedEngineReports");
                        std::future<le_result_t> futResult = p.get_future();
                        if(futResult.get() == LE_OK)
                        {
                            mStarted = true;
                            GnssState = TAF_GNSS_STATE_ACTIVE;
                            mTtffPtr = 0; //reset TTFF value
                            mStartTime = std::chrono::system_clock::now();
                            mTtffEnabled = true;
                            LE_INFO("ForceHotRestart->Start() is success");
                        }
                        else
                        {
                            result = LE_FAULT;
                            LE_INFO("ForceHotRestart->Stop() is failed");
                        }
                    }
                }
            }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Invalid GNSS state %d", GnssState);
        }
        break;
    }

    return result;
}

le_result_t taf_Gnss::GetSupportedConstellations
(
 taf_gnss_ConstellationBitMask_t* constellationMaskPtr
)
{
     TAF_ERROR_IF_RET_VAL( constellationMaskPtr == NULL, LE_FAULT, "constellationMaskPtr is NULL !");
     le_result_t result = LE_NOT_PERMITTED;

    // Check the GNSS device state
    switch (GnssState)
    {
        case TAF_GNSS_STATE_READY:
        {
            //filling the supported bitmask values
            *constellationMaskPtr |= TAF_GNSS_CONSTELLATION_GLONASS;
            *constellationMaskPtr |= TAF_GNSS_CONSTELLATION_BEIDOU;
            *constellationMaskPtr |= TAF_GNSS_CONSTELLATION_GALILEO;
            *constellationMaskPtr |= TAF_GNSS_CONSTELLATION_SBAS;
            *constellationMaskPtr |= TAF_GNSS_CONSTELLATION_QZSS;
            result = LE_OK;
        }
        break;
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_ACTIVE:
        case TAF_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

le_result_t taf_Gnss::SetMinElevation
(
    uint8_t  minElevation
)
{
    le_result_t result = LE_FAULT;
    TAF_ERROR_IF_RET_VAL( minElevation > TAF_GNSS_MIN_ELEVATION_MAX_DEGREE, LE_OUT_OF_RANGE, "minimum elevation is above maximal range");

    switch (GnssState)
    {
        case TAF_GNSS_STATE_ACTIVE:
        case TAF_GNSS_STATE_DISABLED:
        case TAF_GNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
        {
            std::promise<le_result_t> p;
            auto cb = [&p](telux::common::ErrorCode error) {
                if(error == telux::common::ErrorCode::SUCCESS) {
                    p.set_value(LE_OK);
                }
                else {
                    p.set_value(LE_FAULT);
                }
            };
            auto status = mLocationConfigurator->configureMinSVElevation(minElevation, cb);
            if(status != telux::common::Status::SUCCESS)
            {
                result = LE_FAULT;
            }
            else
            {
                std::future<le_result_t> futResult = p.get_future();
                if(futResult.get() == LE_OK)
                {
                    LE_INFO("SetMinElevation is success");
                    result = LE_OK;
                    mMinSvEle = minElevation;
                }
                else
                {
                    LE_INFO("SetMinElevation is failed");
                    result = LE_FAULT;
                }
            }
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Invalid GNSS state %d", GnssState);
        }
        break;
    }

   return result;
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
le_result_t taf_Gnss::StartMode
(
    taf_gnss_StartMode_t mode    ///< [IN] Start mode
)
{
    le_result_t result = LE_OK;

    if (mode >= TAF_GNSS_UNKNOWN_START)
    {
        LE_ERROR("Invalid start mode %d", mode);
        return LE_BAD_PARAMETER;
    }
    switch (GnssState)
    {
        case TAF_GNSS_STATE_READY:
        {
            if(mode == TAF_GNSS_HOT_START) //Hot Start
            {
                LE_INFO("Hot Start! No operation\n");
            }
            else if(mode == TAF_GNSS_WARM_START) //Warm Start
            {
                LE_DEBUG("Warm Start Mode");
                std::promise<le_result_t> p1;
                auto cb1 = [&p1](telux::common::ErrorCode error) {
                    if(error == telux::common::ErrorCode::SUCCESS) {
                        p1.set_value(LE_OK);
                    }
                    else {
                        p1.set_value(LE_FAULT);
                    }
                };

                /* Specifies AidingDataType mask */
                /* 0 - EPHEMERIS 1 - DR_SENSOR_CALIBRATION
                AidingData |1UL << (0,1) which is 3*/

                uint32_t AidingData = 3;
                telux::common::Status status = mLocationConfigurator->deleteAidingData(
                        AidingData, cb1);
                if(status != telux::common::Status::SUCCESS)
                {
                    if (status == telux::common::Status::NOTIMPLEMENTED)
                    {
                        LE_ERROR("StartMode()-> Warm start failed or Not Implemented");
                    }
                    result = LE_FAULT;
                }
                else
                {
                    std::future<le_result_t> futResult = p1.get_future();
                    result = futResult.get();
                }
            }
            //Cold or Factory Start
            else if((mode == TAF_GNSS_COLD_START) || (mode == TAF_GNSS_FACTORY_START))
            {
                std::promise<le_result_t> p2;
                auto cb2 = [&p2](telux::common::ErrorCode error) {
                    if(error == telux::common::ErrorCode::SUCCESS) {
                        p2.set_value(LE_OK);
                    }
                    else {
                        p2.set_value(LE_FAULT);
                    }
                };

                LE_DEBUG("Cold/Factory called");
                telux::common::Status status = mLocationConfigurator->deleteAllAidingData(cb2);
                if(status != telux::common::Status::SUCCESS)
                {
                    if (status == telux::common::Status::NOTIMPLEMENTED)
                    {
                        LE_ERROR("StartMode()-> Cold or factory start failed or Not Implemented");
                    }
                    result = LE_FAULT;
                }
                else
                {
                    std::future<le_result_t> futResult = p2.get_future();
                    result = futResult.get();
                }
            }
            else
            {

                LE_INFO("Invalid Start Mode!\n");
                result = LE_FAULT;
            }

            if(result == LE_OK)
            {
                if (!mStarted)
                {
                    int optInterval = mAcqRate;
                    std::promise<le_result_t> p;
                    auto cb = [&p](telux::common::ErrorCode error) {
                        if(error == telux::common::ErrorCode::SUCCESS) {
                            p.set_value(LE_OK);
                        }
                        else {
                            p.set_value(LE_FAULT);
                        }
                    };
                    LE_INFO("StartMode()->  mAcqRate: %d",mAcqRate);
                    if( optInterval == 0  || optInterval < 100)
                    {
                        LE_DEBUG("StartMode()->mAcqRate is zero, so set default to 100ms");
                        optInterval = 100;
                        mAcqRate = optInterval;
                    }
                    LocReqEngine engineType = DEFAULT_UNKNOWN;
                    GnssReportTypeMask reportMask = DEFAULT_UNKNOWN;
                    reportMask = 0x7f;//all reports are enabled
                    LE_INFO("StartMode->reportMask : %u",reportMask);
                    engineType |= (1UL << mEngineType);
                    auto status = mLocationManager->startDetailedEngineReports((uint32_t)optInterval,
                        engineType, cb, reportMask);
                    if(status != telux::common::Status::SUCCESS)
                    {
                        result = LE_FAULT;
                    }
                    else
                    {
                        mStartTime = std::chrono::system_clock::now();
                        std::future<le_result_t> futResult = p.get_future();
                        if(futResult.get() == LE_OK)
                        {
                            mStarted = true;
                            GnssState = TAF_GNSS_STATE_ACTIVE;
                            LE_INFO("StartMode->Start() is success");
                            mTtffEnabled = true;
                            mTtffPtr = 0; //reset ttff value
                            mStartTime = std::chrono::system_clock::now();
                        }
                        else
                        {
                            LE_INFO("StartMode->Start() is failed");
                            result = LE_FAULT;
                        }
                    }
                }
            }
        }
        break;
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_DUPLICATE;
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", GnssState);
        }
        break;
    }
    return result;
}

le_result_t taf_Gnss::GetMinElevation
(
   uint8_t*  minElevationPtr
)
{
    le_result_t result = LE_FAULT;
    TAF_KILL_CLIENT_IF_RET_VAL( NULL == minElevationPtr, LE_FAULT, "minElevationPtr is NULL");
    switch (GnssState)
    {
        case TAF_GNSS_STATE_ACTIVE:
        case TAF_GNSS_STATE_DISABLED:
        case TAF_GNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
        {
            std::promise<le_result_t> p;
            auto cb = [&p](uint8_t minSVElevation, telux::common::ErrorCode error) {
                LE_INFO("***Request minimum SV Elevation Info ****");
                auto &gnss = taf_Gnss::GetInstance();
                if(error == telux::common::ErrorCode::SUCCESS)
                {
                    LE_DEBUG("onMinSVElevationInfo %s sent successfully", gnss.mCommandName.c_str());
                    gnss.mRequestMinEle = minSVElevation;
                    LE_INFO("onMinSVElevationInfo gnss.mRequestMinEle: %d",gnss.mRequestMinEle);
                    p.set_value(LE_OK);
                }
                else
                {
                    LE_DEBUG(" onMinSVElevationInfo failed errorCode: %d ", int(error));
                    p.set_value(LE_FAULT);
                }
            };

            auto status = mLocationConfigurator->requestMinSVElevation(cb);
            if(status != telux::common::Status::SUCCESS)
            {
                result = LE_FAULT;
            }
            else
            {
                std::future<le_result_t> futResult = p.get_future();
                if(futResult.get() == LE_OK)
                {
                    LE_INFO("requestMinSVElevation is Success");
                    result = LE_OK;
                    *minElevationPtr = mRequestMinEle;
                }
                else
                {
                    LE_INFO("requestMinSVElevation is Failed");
                    result = LE_FAULT;
                }
            }
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Invalid GNSS state %d", GnssState);
        }
        break;
    }

    return result;
}

le_result_t taf_Gnss::Disable
(
    void
)
{
    le_result_t result = LE_FAULT;

    switch (GnssState)
    {
        case TAF_GNSS_STATE_READY:
        {
                GnssState = TAF_GNSS_STATE_DISABLED;
                result = LE_OK;
        }
        break;
        case TAF_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_DUPLICATE;
        }
        break;
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", GnssState);
        }
        break;
    }

    return result;
}

le_result_t taf_Gnss::Stop
(
    void
)
{
    le_result_t result = LE_FAULT;

    switch (GnssState)
    {
        case TAF_GNSS_STATE_ACTIVE:
            {
                if (mStarted)
                {
                    std::promise<le_result_t> p;
                    auto cb = [&p](telux::common::ErrorCode error) {
                        if(error == telux::common::ErrorCode::SUCCESS) {
                            p.set_value(LE_OK);
                        }
                        else {
                            p.set_value(LE_FAULT);
                        }
                    };
                    auto status = mLocationManager->stopReports(cb);
                    if(status != telux::common::Status::SUCCESS)
                    {
                        result = LE_FAULT;
                    }
                    else
                    {
                        std::future<le_result_t> futResult = p.get_future();
                        if(futResult.get() == LE_OK)
                        {
                            mStarted = false;
                            GnssState = TAF_GNSS_STATE_READY;
                            result = LE_OK;
                            mNmeaMask = 0;//reset nmeaMask on triggering stop.
                            LE_INFO("Stop() is success");
                        }
                        else
                        {
                            result = LE_FAULT;
                            LE_INFO("Stop() is failed");
                        }
                    }
                }
            }
        break;
        case TAF_GNSS_STATE_READY:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_DUPLICATE;
        }
        break;
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", GnssState);
        }
        break;
    }
    return result;
}

le_result_t taf_Gnss::SetNmeaSentences
(
    taf_gnss_NmeaBitMask_t nmeaMask ///< [IN] Bit mask for enabled NMEA sentences.
)
{
    le_result_t result = LE_FAULT;

    LE_DEBUG("SetNmeaSentences nmeaMask: %" PRIu64"", nmeaMask);

    // Check if the bit mask is correct
    if (nmeaMask == 0)
    {
        LE_ERROR("Unable to set the enabled NMEA sentences, wrong bit mask %" PRIu64"", nmeaMask);
        result = LE_BAD_PARAMETER;
    }
    else
    {
        // Check the GNSS device state
        switch (GnssState)
        {
            case TAF_GNSS_STATE_READY:
            {
                // Set the enabled NMEA sentences
                std::promise<le_result_t> p;
                auto cb = [&p](telux::common::ErrorCode error) {
                    if(error == telux::common::ErrorCode::SUCCESS) {
                        p.set_value(LE_OK);
                    }
                    else {
                        p.set_value(LE_FAULT);
                    }
                };
                auto status = mLocationConfigurator->configureNmeaTypes(nmeaMask, cb);
                if(status != telux::common::Status::SUCCESS)
                {
                    result = LE_FAULT;
                }
                else
                {
                    std::future<le_result_t> futResult = p.get_future();
                    if(futResult.get() == LE_OK)
                    {
                        result = LE_OK;
                        LE_INFO("SetNmeaSentences() is success");
                    }
                    else
                    {
                        result = LE_FAULT;
                        LE_INFO("SetNmeaSentences() is failed");
                    }
                }
                if (LE_OK != result)
                {
                    LE_ERROR("Unable to set the enabled NMEA sentences, error = %d (%s)",
                              result, LE_RESULT_TXT(result));
                }
            }
            break;
            case TAF_GNSS_STATE_UNINITIALIZED:
            case TAF_GNSS_STATE_ACTIVE:
            case TAF_GNSS_STATE_DISABLED:
            {
                LE_ERROR("Bad state for that request [%d]", GnssState);
                result = LE_NOT_PERMITTED;
            }
            break;
            default:
            {
                LE_ERROR("Unknown GNSS state %d", GnssState);
                result = LE_FAULT;
            }
            break;
        }
    }

    return result;
}

le_result_t taf_Gnss::GetNmeaSentences
(
    taf_gnss_NmeaBitMask_t* nmeaMaskPtr
)
{

    if (NULL == nmeaMaskPtr)
    {
        LE_KILL_CLIENT("nmeaMaskPtr is NULL !");
        return LE_FAULT;
    }

     le_result_t result = LE_NOT_PERMITTED;

    // Check the GNSS device state
    switch (GnssState)
    {
        case TAF_GNSS_STATE_ACTIVE:
        {
            // Get the enabled NMEA sentences
            std::unique_lock<std::mutex> lock(mMutex);
            auto nmeaStatus = mNmeaVar.wait_for(lock,std::chrono::seconds(DEFAULT_TIMEOUT_IN_SECONDS));
            if(nmeaStatus == std::cv_status::timeout)
            {
                LE_DEBUG("NmeaSentence type not found within %d seconds",DEFAULT_TIMEOUT_IN_SECONDS);
                result = LE_TIMEOUT;
                return result;
            }
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
        case TAF_GNSS_STATE_READY:
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

le_result_t taf_Gnss::GetSupportedNmeaSentences
(
    taf_gnss_NmeaBitMask_t* nmeaMaskPtr
)
{

    TAF_ERROR_IF_RET_VAL( nmeaMaskPtr == NULL, LE_FAULT, "nmeaMaskPtr is NULL !");
    le_result_t result = LE_NOT_PERMITTED;

    // Check the GNSS device state
    switch (GnssState)
    {
        case TAF_GNSS_STATE_READY:
        {
            //filling the supported bitmask values
            *nmeaMaskPtr |= TAF_GNSS_NMEA_MASK_GPGGA;
            *nmeaMaskPtr |= TAF_GNSS_NMEA_MASK_GPRMC;
            *nmeaMaskPtr |= TAF_GNSS_NMEA_MASK_GNGSA;
            *nmeaMaskPtr |= TAF_GNSS_NMEA_MASK_GPVTG;
            *nmeaMaskPtr |= TAF_GNSS_NMEA_MASK_GPGNS;
            *nmeaMaskPtr |= TAF_GNSS_NMEA_MASK_GPDTM;
            *nmeaMaskPtr |= TAF_GNSS_NMEA_MASK_GPGSV;
            *nmeaMaskPtr |= TAF_GNSS_NMEA_MASK_GLGSV;
            *nmeaMaskPtr |= TAF_GNSS_NMEA_MASK_GAGSV;
            *nmeaMaskPtr |= TAF_GNSS_NMEA_MASK_GQGSV;
            *nmeaMaskPtr |= TAF_GNSS_NMEA_MASK_GBGSV;
            *nmeaMaskPtr |= TAF_GNSS_NMEA_MASK_GIGSV;
            result = LE_OK;
        }
        break;
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_ACTIVE:
        case TAF_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

le_result_t taf_Gnss::SetDRConfig(const taf_gnss_DrParams_t* drParamsPtr)
{
    le_result_t result = LE_NOT_PERMITTED;
    telux::loc::DREngineConfiguration drConfig;
    drConfig.validMask = static_cast<telux::loc::DRConfigValidity>(0);
    TAF_KILL_CLIENT_IF_RET_VAL( NULL == drParamsPtr, LE_FAULT, "drParamsPtr is NULL");

 // Check the GNSS device state
    switch (GnssState)
    {
        case TAF_GNSS_STATE_READY:
        {
                //Filling the DR parameters
            bodyToSensorUtility(drConfig,drParamsPtr);
            speedScaleUtility(drConfig,drParamsPtr);
            gyroScaleUtility(drConfig,drParamsPtr);

            std::promise<le_result_t> p;
            auto cb = [&p](telux::common::ErrorCode error) {
                if(error == telux::common::ErrorCode::SUCCESS) {
                    p.set_value(LE_OK);
                }
                else {
                    p.set_value(LE_FAULT);
                }
            };

            // Set the DR Configuration Validity
            telux::common::Status status = mLocationConfigurator->configureDR(drConfig, cb);
            if (status == telux::common::Status::FAILED) {
                LE_INFO("SetDRConfigValidity is failed");
                result = LE_FAULT;
            } else if (telux::common::Status::SUCCESS == status) {
                std::future<le_result_t> futResult = p.get_future();
                result = futResult.get();
                if(result == LE_OK)
                {
                    LE_INFO("SetDRConfigValidity is Success");
                }
            }
            if (LE_OK != result)
            {
                LE_ERROR("Unable to set the DR Configuration Validity , error = %d (%s)",
                          result, LE_RESULT_TXT(result));
            }
        }
        break;
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_ACTIVE:
        case TAF_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }
    return result;
}

void bodyToSensorUtility(telux::loc::DREngineConfiguration& drConfig,
        const taf_gnss_DrParams_t* drParamsPtr)
{
    drConfig.validMask |= telux::loc::DRConfigValidityType::BODY_TO_SENSOR_MOUNT_PARAMS_VALID;
    drConfig.mountParam.rollOffset =(float) drParamsPtr->rollOffset;
    drConfig.mountParam.yawOffset = (float) drParamsPtr->yawOffset;
    drConfig.mountParam.pitchOffset = (float) drParamsPtr->pitchOffset;
    drConfig.mountParam.offsetUnc = (float) drParamsPtr->offsetUnc;
}

void speedScaleUtility(telux::loc::DREngineConfiguration& drConfig,
        const taf_gnss_DrParams_t* drParamsPtr)
{
    drConfig.validMask |= telux::loc::DRConfigValidityType::VEHICLE_SPEED_SCALE_FACTOR_VALID;
    drConfig.speedFactor = (float) drParamsPtr->speedFactor;
    drConfig.validMask |= telux::loc::DRConfigValidityType::VEHICLE_SPEED_SCALE_FACTOR_UNC_VALID;
    drConfig.speedFactorUnc = (float) drParamsPtr->speedFactorUnc;
}

void gyroScaleUtility(telux::loc::DREngineConfiguration& drConfig,
        const taf_gnss_DrParams_t* drParamsPtr)
{
    drConfig.validMask |= telux::loc::DRConfigValidityType::GYRO_SCALE_FACTOR_VALID;
    drConfig.gyroFactor = (float) drParamsPtr->gyroFactor;
    drConfig.validMask |= telux::loc::DRConfigValidityType::GYRO_SCALE_FACTOR_UNC_VALID;
    drConfig.gyroFactorUnc = (float) drParamsPtr->gyroFactorUnc;
}
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
le_result_t taf_Gnss::ConfigureEngineState
(
    taf_gnss_EngineType_t engtype,///< [IN] value for Engine type.
    taf_gnss_EngineState_t engState///< [IN] value for Engine state.
)
{
    le_result_t result = LE_OK;
    telux::loc::EngineType engineType;
    telux::loc::LocationEngineRunState engineState;
    LE_INFO("ConfigureEngineState engtype:%d", engtype);
    LE_INFO("ConfigureEngineState engState:%d", engState);
    switch(engtype)
    {
        case TAF_GNSS_ENGINE_TYPE_DRE:
            engineType = telux::loc::EngineType::DRE;
            LE_INFO("ConfigureEngineState TAF_GNSS_ENGINE_TYPE_DRE");
            break;
        default:
        {
            LE_ERROR("Unknown Engine type %d", engtype);
            result = LE_FAULT;
            return result;
        }
    }
    switch(engState)
    {
        case TAF_GNSS_ENGINE_STATE_SUSPENDED:
            engineState = telux::loc::LocationEngineRunState::SUSPENDED;
            LE_INFO("ConfigureEngineState TAF_GNSS_ENGINE_STATE_SUSPENDED");
            break;
        case TAF_GNSS_ENGINE_STATE_RUNNING:
            engineState = telux::loc::LocationEngineRunState::RUNNING;
            LE_INFO("ConfigureEngineState TAF_GNSS_ENGINE_STATE_RUNNING");
            break;
        default:
        {
            LE_ERROR("Unknown Engine state %d", engState);
            result = LE_FAULT;
            return result;
        }
    }
    switch (GnssState)
    {
        case TAF_GNSS_STATE_DISABLED:
        case TAF_GNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
        case TAF_GNSS_STATE_ACTIVE:
            {
                std::promise<le_result_t> p;
                auto cb = [&p](telux::common::ErrorCode error) {
                    if(error == telux::common::ErrorCode::SUCCESS) {
                        p.set_value(LE_OK);
                    }
                    else {
                        p.set_value(LE_FAULT);
                    }
                };
                telux::common::Status status = mLocationConfigurator->configureEngineState(
                        engineType, engineState, cb);
                if(status != telux::common::Status::SUCCESS)
                {
                    result = LE_FAULT;
                }
                else
                {
                    std::future<le_result_t> futResult = p.get_future();
                    if(futResult.get() == LE_OK)
                    {
                        LE_INFO("ConfigureEngineState succeed.");
                    }
                    else
                    {
                        LE_INFO("ConfigureEngineState failed");
                        result = LE_FAULT;
                    }
                }
            }
        break;
        default:
        {
            LE_ERROR("Invalid GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}
#endif
le_result_t taf_Gnss::ConfigureRobustLocation
(
    uint8_t enable,///< [IN] value for enable/disable.
    uint8_t enabled911///< [IN] value for 911 enable/disable
)
{
    le_result_t result = LE_OK;
    LE_INFO("ConfigureRobustLocation enable:%d", enable);
    LE_INFO("ConfigureRobustLocation enabled911:%d", enabled911);
    bool enableRobustloc = false;
    bool enableE911loc = false;
    switch(enable)
    {
       case 0:
            enableRobustloc = false;
            LE_INFO("ConfigureRobustLocation Disable");
            break;
       case 1:
            enableRobustloc = true;
            LE_INFO("ConfigureRobustLocation Enable");
            break;
        default:
        {
            LE_ERROR("Wrong Input to enable: %d", enable);
            result = LE_FAULT;
            return result;
        }
    }
    switch(enabled911)
    {
       case 0:
            enableE911loc = false;
            LE_INFO("ConfigureRobustLocation disable enableE911");
            break;
       case 1:
            enableE911loc = true;
            LE_INFO("ConfigureRobustLocation Enable enableE911");
            break;
        default:
        {
            LE_ERROR("Wrong Input to enabled911: %d", enabled911);
            result = LE_FAULT;
            return result;
        }
    }
    switch (GnssState)
    {
        case TAF_GNSS_STATE_DISABLED:
        case TAF_GNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
        case TAF_GNSS_STATE_ACTIVE:
            {
                std::promise<le_result_t> p;
                auto cb = [&p](telux::common::ErrorCode error) {
                    if(error == telux::common::ErrorCode::SUCCESS) {
                        p.set_value(LE_OK);
                    }
                    else {
                        p.set_value(LE_FAULT);
                    }
                };
                telux::common::Status status = mLocationConfigurator->configureRobustLocation(
                    enableRobustloc, enableE911loc, cb);
                if(status != telux::common::Status::SUCCESS)
                {
                    LE_INFO("ConfigureEngineState failed");
                    result = LE_FAULT;
                }
                else
                {
                    std::future<le_result_t> futResult = p.get_future();
                    result = futResult.get();
                }
            }
        break;
        default:
        {
            LE_ERROR("Invalid GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

le_result_t taf_Gnss::RobustLocationInformation
(
    uint8_t* enable,///< [OUT] value for enable/disable.
    uint8_t* enabled911,///< [OUT] value for 911 enable/disable
    uint8_t* majorVersion,///< [OUT] value for majorVersion number
    uint8_t* minorVersion ///< [OUT] value for majorVersion number
)
{
    le_result_t result = LE_OK;
    LE_INFO("RobustLocationInformation");
    if((enable == NULL) || (enabled911 == NULL)
        || (majorVersion == NULL) || (minorVersion == NULL))
    {
        return LE_FAULT;
    }
    switch (GnssState)
    {
        case TAF_GNSS_STATE_DISABLED:
        case TAF_GNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
        case TAF_GNSS_STATE_ACTIVE:
            {
                std::promise<le_result_t> p;
                auto cb = [&p](const telux::loc::RobustLocationConfiguration
                    rLConfig, telux::common::ErrorCode error) {
                    auto &gnss = taf_Gnss::GetInstance();
                    LE_INFO("****onRobustLocationInfo **");
                    if(error == telux::common::ErrorCode::SUCCESS)
                    {
                        LE_DEBUG("onRobustLocationInfo %s sent successfully", gnss.mCommandName.c_str());
                        if(rLConfig.validMask & telux::loc::VALID_ENABLED)
                        {
                            gnss.mEnable = rLConfig.enabled;
                        }
                        if(rLConfig.validMask & telux::loc::VALID_ENABLED_FOR_E911)
                        {
                            gnss.mEnabled911 = rLConfig.enabledForE911;
                        }
                        if(rLConfig.validMask & telux::loc::VALID_VERSION)
                        {
                            gnss.mMajorVersion = unsigned (rLConfig.version.major);
                            gnss.mMinorVersion = rLConfig.version.minor;
                        }
                        LE_INFO("***onRobustLocationInfo gnss.enable: %d", gnss.mEnable);
                        LE_INFO("***onRobustLocationInfo gnss.enabled911: %d", gnss.mEnabled911);
                        LE_INFO("***onRobustLocationInfo gnss.majorVersion: %d", gnss.mMajorVersion);
                        LE_INFO("***onRobustLocationInfo gnss.minorVersion: %d", gnss.mMinorVersion);
                        p.set_value(LE_OK);
                    }
                    else
                    {
                        LE_DEBUG(" onRobustLocationInfo failed errorCode: %d ", int(error));
                        p.set_value(LE_FAULT);
                    }
                };

                auto status = mLocationConfigurator->requestRobustLocation(cb);
                if(status != telux::common::Status::SUCCESS)
                {
                    result = LE_FAULT;
                }
                else
                {
                    std::future<le_result_t> futResult = p.get_future();
                    if(futResult.get() == LE_OK)
                    {
                        LE_INFO("RobustLocationInformation is Success");
                        *enable = mEnable;
                        *enabled911 = mEnabled911;
                        *majorVersion = mMajorVersion;
                        *minorVersion = mMinorVersion;
                    }
                    else
                    {
                        LE_INFO("RobustLocationInformation is Failed");
                        result = LE_FAULT;
                    }
                }
            }
        break;
        default:
        {
            LE_ERROR("Invalid GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
le_result_t taf_Gnss::DefaultSecondaryBandConstellations
(
)
{
    LE_INFO("DefaultSecondaryBandConstellations");

    le_result_t result = LE_FAULT;

    switch (GnssState)
    {
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_DISABLED:
        case TAF_GNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
        {
            std::promise<le_result_t> p;
            auto cb = [&p](telux::common::ErrorCode error) {
                if(error == telux::common::ErrorCode::SUCCESS) {
                    p.set_value(LE_OK);
                }
                else {
                    p.set_value(LE_FAULT);
                }
            };

            // Set GNSS Request Secondary Band constellation
            mRequestSB = 0;//reset the value before configuring
            telux::loc::ConstellationSet constellationSet{};
            telux::common::Status status = mLocationConfigurator->configureSecondaryBand(
                constellationSet, cb);
            if (status == telux::common::Status::NOTIMPLEMENTED) {
                LE_INFO("Not implemented");
                result = LE_FAULT;
            } else if (telux::common::Status::SUCCESS == status) {
                std::future<le_result_t> futResult = p.get_future();
                result = futResult.get();
                if(result == LE_OK)
                {
                    LE_INFO("Success");
                }
            }
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", GnssState);
        }
        break;
    }
    return result;
}

le_result_t taf_Gnss::RequestSecondaryBandConstellations
(
   uint32_t * constellationSb
)
{
    LE_INFO("RequestSecondaryBandConstellation");
    le_result_t result = LE_FAULT;

    if (NULL == constellationSb)
    {
        LE_KILL_CLIENT("constellationSb is NULL !");
        return result;
    }

    switch (GnssState)
    {
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_DISABLED:
        case TAF_GNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
        {
            // Set GNSS Request Secondary Band constellation
            std::promise<le_result_t> p;
            auto cb = [&p](telux::loc::ConstellationSet set, telux::common::ErrorCode error) {
                auto &gnss = taf_Gnss::GetInstance();
                LE_INFO("***Request Secondary Band Info ****");
                if(error == telux::common::ErrorCode::SUCCESS)
                {
                    for (auto item : set)
                    {
                        if (item == telux::loc::GnssConstellationType::GPS)
                        {
                            LE_INFO("onSecondaryBandInfo: GPS");
                            gnss.mRequestSB |= (1<<(TAF_GNSS_SB_CONSTELLATION_GPS-1));//1st bit
                        }
                        else if (item == telux::loc::GnssConstellationType::GALILEO)
                        {
                            LE_INFO("onSecondaryBandInfo: GALILEO");
                            gnss.mRequestSB |= (1<<(TAF_GNSS_SB_CONSTELLATION_GALILEO-1));//2nd bit
                        }
                        else if (item == telux::loc::GnssConstellationType::SBAS)
                        {
                            LE_INFO("onSecondaryBandInfo: SBAS");
                            gnss.mRequestSB |= (1<<(TAF_GNSS_SB_CONSTELLATION_SBAS-1));//3rd bit
                        }
                        else if (item == telux::loc::GnssConstellationType::COMPASS)
                        {
                            LE_INFO("onSecondaryBandInfo: COMPASS");
                            gnss.mRequestSB |= (1<<(TAF_GNSS_SB_CONSTELLATION_COMPASS-1)); //4th bit
                        }
                        else if (item == telux::loc::GnssConstellationType::GLONASS)
                        {
                            LE_INFO("onSecondaryBandInfo: GLONASS");
                            gnss.mRequestSB |= (1<<(TAF_GNSS_SB_CONSTELLATION_GLONASS-1)); //5th bit
                        }
                        else if (item == telux::loc::GnssConstellationType::BDS)
                        {
                            LE_INFO("onSecondaryBandInfo: BDS");
                            gnss.mRequestSB |= (1<<(TAF_GNSS_SB_CONSTELLATION_BDS-1));//6th bit
                        }
                        else if (item == telux::loc::GnssConstellationType::QZSS)
                        {
                            LE_INFO("onSecondaryBandInfo: QZSS");
                            gnss.mRequestSB |=(1<<(TAF_GNSS_SB_CONSTELLATION_QZSS-1));//7th bit
                        }
                        else if (item == telux::loc::GnssConstellationType::NAVIC)
                        {
                            LE_INFO("onSecondaryBandInfo: NAVIC");
                            gnss.mRequestSB |= (1<<(TAF_GNSS_SB_CONSTELLATION_NAVIC-1));//8th bit
                        }
                        else
                        {
                            LE_INFO("onSecondaryBandInfo: Not supported");
                        }
                    }
                    p.set_value(LE_OK);
                }
                else
                {
                    LE_DEBUG("onSecondaryBandInfo failed errorCode: %d ", int(error));
                    p.set_value(LE_FAULT);
                }
            };
            auto status = mLocationConfigurator->requestSecondaryBandConfig(cb);
            if(status != telux::common::Status::SUCCESS) {
                return LE_FAULT;
            }
            std::future<le_result_t> futResult = p.get_future();
            if(futResult.get() == LE_OK)
            {
                LE_INFO("Request secondary band constellations is success");
                result = LE_OK;
                *constellationSb = mRequestSB;
            }
            else
            {
                LE_INFO("Request secondary band constellations is failed");
                result = LE_FAULT;
            }
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", GnssState);
        }
        break;
    }
    return result;
}

le_result_t taf_Gnss::ConfigureSecondaryBandConstellations
(
   uint32_t constellationSb
)
{
    LE_INFO("ConfigureSecondaryBandConstellations");
    le_result_t result = LE_FAULT;
    telux::loc::ConstellationSet constellationSet{};
    if( constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_GPS-1))) //GPS->1
    {
        constellationSet.insert(telux::loc::GnssConstellationType::GPS);
        LE_INFO("ConfigureSecondary Band constellation GPS");
    }
    if( constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_GALILEO-1))) //GALILEO->2
    {
        constellationSet.insert(telux::loc::GnssConstellationType::GALILEO);
        LE_INFO("ConfigureSecondary Band constellation GALILEO");
    }
    if( constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_SBAS-1)))//SBAS->4
    {
        constellationSet.insert(telux::loc::GnssConstellationType::SBAS);
        LE_INFO("ConfigureSecondary Band constellation SBAS");
    }
    if( constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_COMPASS-1))) //COMPASS->8
    {
        constellationSet.insert(telux::loc::GnssConstellationType::COMPASS);
        LE_INFO("ConfigureSecondary Band constellation COMPASS");
    }
    if( constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_GLONASS-1)))//GLONASS->16
    {
        constellationSet.insert(telux::loc::GnssConstellationType::GLONASS);
        LE_INFO("ConfigureSecondary Band constellation GLONASS");
    }
    if( constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_BDS-1))) //BDS->32
    {
        constellationSet.insert(telux::loc::GnssConstellationType::BDS);
        LE_INFO("ConfigureSecondary Band constellation BDS");
    }
    if( constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_QZSS-1))) //QZSS->64
    {
        constellationSet.insert(telux::loc::GnssConstellationType::QZSS);
        LE_INFO("ConfigureSecondary Band constellation QZSS");
    }
    if( constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_NAVIC-1))) //NAVIC->128
    {
        constellationSet.insert(telux::loc::GnssConstellationType::NAVIC);
        LE_INFO("ConfigureSecondary Band constellation NAVIC");
    }
    switch (GnssState)
    {
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_DISABLED:
        case TAF_GNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
        {
            std::promise<le_result_t> p;
            auto cb = [&p](telux::common::ErrorCode error) {
                if(error == telux::common::ErrorCode::SUCCESS) {
                    p.set_value(LE_OK);
                }
                else {
                    p.set_value(LE_FAULT);
                }
            };

            // Configure Secondary Band constellation
            mRequestSB = 0;//reset the value before configuring
            telux::common::Status status = mLocationConfigurator->configureSecondaryBand(
                constellationSet, cb);
            if (status == telux::common::Status::NOTIMPLEMENTED) {
                LE_INFO("Not implemented");
                result = LE_FAULT;
            } else if (telux::common::Status::SUCCESS == status) {
                std::future<le_result_t> futResult = p.get_future();
                result = futResult.get();
                if(result == LE_OK)
                {
                    LE_INFO("Success");
                }
            }
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", GnssState);
        }
        break;
    }
    return result;
}
#endif


le_result_t taf_Gnss::SetLeverArmConfig(const taf_gnss_LeverArmParams_t* LeverArmParamsPtr)
{
    le_result_t result = LE_NOT_PERMITTED;
    LeverArmConfigInfo configInfo;
    telux::loc::LeverArmType leverArmType;
    telux::loc::LeverArmParams leverArmParams;
    TAF_KILL_CLIENT_IF_RET_VAL( NULL == LeverArmParamsPtr, LE_FAULT, "LeverArmParamsPtr is NULL");

    switch (GnssState)
    {
        case TAF_GNSS_STATE_READY:
        {
            if((LeverArmParamsPtr->levArmType < TAF_GNSS_LEVER_ARM_TYPE_GNSS_TO_VRP)
                || (LeverArmParamsPtr->levArmType >TAF_GNSS_LEVER_ARM_TYPE_VPE_IMU_TO_GNSS))
            {
                LE_INFO("invalid Lever Arm type, returning");
                return LE_BAD_PARAMETER;
            }
            //Filling the Lever Arm types
            if(LeverArmParamsPtr->levArmType == TAF_GNSS_LEVER_ARM_TYPE_GNSS_TO_VRP)
            {
                leverArmType = telux::loc::LEVER_ARM_TYPE_GNSS_TO_VRP;
            }
            else if(LeverArmParamsPtr->levArmType == TAF_GNSS_LEVER_ARM_TYPE_DR_IMU_TO_GNSS)
            {
                leverArmType = telux::loc::LEVER_ARM_TYPE_DR_IMU_TO_GNSS;
            }
            else if(LeverArmParamsPtr->levArmType == TAF_GNSS_LEVER_ARM_TYPE_VPE_IMU_TO_GNSS)
            {
                leverArmType = telux::loc::LEVER_ARM_TYPE_VPE_IMU_TO_GNSS;
            }

            //Filling the Lever Arm Paramters
            leverArmParams.forwardOffset = (float)LeverArmParamsPtr->forwardOffsetMeters;
            leverArmParams.sidewaysOffset =(float)LeverArmParamsPtr->sidewaysOffsetMeters;
            leverArmParams.upOffset = (float)LeverArmParamsPtr->upOffsetMeters;
            configInfo.insert({leverArmType, leverArmParams});

            //Set the Lever Arm Configuration
            std::promise<le_result_t> p;
            auto cb = [&p](telux::common::ErrorCode error) {
                if(error == telux::common::ErrorCode::SUCCESS) {
                    p.set_value(LE_OK);
                }
                else {
                    p.set_value(LE_FAULT);
                }
            };
            auto status = mLocationConfigurator->configureLeverArm(configInfo, cb);
            if (status != telux::common::Status::SUCCESS)
            {
                result = LE_FAULT;
            }
            else
            {
                std::future<le_result_t> futResult = p.get_future();
                if(futResult.get() == LE_OK)
                {
                    LE_INFO("Set Lever Arm parameters is OK");
                    result = LE_OK;
                }
                else
                {
                    LE_INFO("Set Lever Arm parameters is FAILED");
                    result = LE_FAULT;
                }
                if (LE_OK != result)
                {
                    LE_ERROR("Unable to set the Lever Arm Configuration error = %d (%s)",
                            result, LE_RESULT_TXT(result));
                }
            }
        }
        break;
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_ACTIVE:
        case TAF_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }
    return result;
}

le_result_t taf_Gnss::SetEngineType(taf_gnss_EngineReportsType_t EngineType)
{
    le_result_t result = LE_FAULT;
    LE_INFO("SetEngineType EngineType %d",EngineType);
    if((EngineType <TAF_GNSS_ENGINE_REPORT_TYPE_FUSED)
          || (EngineType>TAF_GNSS_ENGINE_REPORT_TYPE_VPE))
    {
        LE_INFO("SetEngineType: Unknown Engine type");
        return LE_BAD_PARAMETER;
    }

    switch (GnssState)
    {
        case TAF_GNSS_STATE_READY:
        {
            // Set Engine Type
            mEngineType = EngineType;
            result = LE_OK;
            LE_INFO("SetEngineType->EngineType:%d",mEngineType);
        }
        break;
        case TAF_GNSS_STATE_ACTIVE:
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", GnssState);
        }
        break;
    }
    return result;
}

le_result_t taf_Gnss::GetConformityIndex
(
    taf_gnss_SampleRef_t positionSampleRef,
    double* indexPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
            LE_INFO("GetConformityIndex is not valid");
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_Gnss::GetCalibrationData
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint32_t* calibPtr,
    uint8_t* percentPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
            LE_INFO("GetCalibrationData is not valid");
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
            LE_INFO("GetCalibrationData is not valid");
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_Gnss::GetBodyFrameData
(
    taf_gnss_SampleRef_t positionSampleRef,
    taf_gnss_KinematicsData_t* bodyDataPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
            LE_INFO("GetBodyFrameData is not valid");
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_Gnss::GetVRPBasedLLA
(
    taf_gnss_SampleRef_t positionSampleRef,
    double* vrpLatitudePtr,
    double* vrpLongitudePtr,
    double* vrpAltitudePtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
            LE_INFO("GetVRPBasedLLA latitude is not valid");
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
            LE_INFO("GetVRPBasedLLA longtitude is not valid");
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
            LE_INFO("GetVRPBasedLLA altitude is not valid");
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

le_result_t taf_Gnss::GetVRPBasedVelocity
(
    taf_gnss_SampleRef_t positionSampleRef,
    double* eastVelPtr,
    double* northVelPtr,
    double* upVelPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
            LE_INFO("GetVRPBasedVelocity east velocity is not valid");
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
            LE_INFO("GetVRPBasedVelocity north velocity is not valid");
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
            LE_INFO("GetVRPBasedVelocity up velocity is not valid");
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

le_result_t taf_Gnss::GetSvUsedInPosition
(
    taf_gnss_SampleRef_t positionSampleRef,
    taf_gnss_SvUsedInPosition_t* svDataPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
            LE_INFO("GetSvUsedInPosition is invalid");
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

le_result_t taf_Gnss::GetSbasCorrection
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint32_t* sbasMaskPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
            LE_INFO("GetSbasCorrection is invalid");
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_Gnss::GetPositionTechnology
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint32_t* techMaskPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
            LE_INFO("GetPositionTechnology: loc tech is invalid");
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_Gnss::GetLocationInfoValidity
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint32_t* validityMaskPtr,
    uint64_t* validityExMaskPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
            LE_INFO("GetLocationInfoValidity is invalid");
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
            LE_INFO("GetLocationInfoExValidity is invalid");
            result = LE_OUT_OF_RANGE;
        }
    }
    else
    {
        result = LE_FAULT;
    }
    return result;
}

le_result_t taf_Gnss::GetLocationOutputEngParams
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint16_t* engMaskPtr,
    uint16_t* locationEngTypePtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
            LE_INFO("GetLocationOutputEngParams eng mask is invalid");
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
            LE_INFO("GetLocationOutputEngParams location engine type is invalid");
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

le_result_t taf_Gnss::GetReliabilityInformation
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint16_t* horiReliabilityPtr,
    uint16_t* vertReliabilityPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr =
        (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap, positionSampleRef);

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
            LE_INFO("GetReliabilityInformation horizontal reliability is invalid");
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
            LE_INFO("GetReliabilityInformation vertical reliability is invalid");
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

le_result_t taf_Gnss::GetStdDeviationAzimuthInfo
(
    taf_gnss_SampleRef_t positionSampleRef,
    double* azimuthPtr,
    double* eastDevPtr,
    double* northDevPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
            LE_INFO("GetStdDeviationAzimuthInfo: azimuth is invalid");
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
            LE_INFO("GetStdDeviationAzimuthInfo: eastDev is invalid");
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
        if (posSampleReqPtr->positionSampleNodePtr->eastDevValid)
        {
            *northDevPtr = posSampleReqPtr->positionSampleNodePtr->northDev;
        }
        else
        {
            LE_INFO("GetStdDeviationAzimuthInfo: northDev is invalid");
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

le_result_t taf_Gnss::GetRealTimeInformation
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint64_t* realTimePtr,
    uint64_t* realTimeUncPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
                                            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
            LE_INFO("GetRealTimeInformation: real time is invalid");
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
            LE_INFO("GetRealTimeInformation: real time unc is invalid");
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

le_result_t taf_Gnss::GetMeasurementUsageInfo
(
    taf_gnss_SampleRef_t positionSampleRef,
    taf_gnss_GnssMeasurementInfo_t* measInfoPtr,
    size_t* measInfoLen
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

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

le_result_t taf_Gnss::GetReportStatus
(
    taf_gnss_SampleRef_t positionSampleRef,
	int32_t* reportStatusPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    *reportStatusPtr = posSampleReqPtr->positionSampleNodePtr->reportStatus;

    return LE_OK;
}

le_result_t taf_Gnss::GetAltitudeMeanSeaLevel
(
    taf_gnss_SampleRef_t positionSampleRef,
	double* altMeanSeaLevelPtr
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    *altMeanSeaLevelPtr = posSampleReqPtr->positionSampleNodePtr->altMeanSeaLevel;

    return LE_OK;
}

le_result_t taf_Gnss::GetSVIds
(
    taf_gnss_SampleRef_t positionSampleRef,
    uint16_t* sVIdsPtr,
    size_t* sVIdsLen
)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr
            = (taf_gnss_PositionSampleRequest_t *)le_ref_Lookup(PositionSampleMap,positionSampleRef);

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    for (auto i = 0; i < (int) *sVIdsLen; i++) {
        sVIdsPtr[i] = posSampleReqPtr->positionSampleNodePtr->SVIds[i];
    }

    *sVIdsLen = posSampleReqPtr->positionSampleNodePtr->SVIdsCount;

    return LE_OK;
}

le_result_t taf_Gnss::GetSatellitesInfoEx
(
    taf_gnss_SampleRef_t positionSampleRef,
    taf_gnss_Constellation_t constellation,
    taf_gnss_SvInfo_t* svInfoPtr,
    size_t* svInfoLen

)
{
    le_result_t result = LE_OK;
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(PositionSampleMap,positionSampleRef);
    int i;

    result = CheckValidatePosition(posSampleReqPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (svInfoPtr && svInfoLen)
    {
        size_t svInfoNumElements = NUM_ARRAY_MEMBERS(posSampleReqPtr->positionSampleNodePtr->satInfo);
        LE_INFO("GetSatellitesInfoEx: svInfoNumElements %d, constellation: %d, input len: %d", (int)svInfoNumElements, (int) constellation, (int) *svInfoLen);

        if (posSampleReqPtr->positionSampleNodePtr->satInfoValid)
        {
            int count = 0;
            for(i=0; i < (int) svInfoNumElements; i++)
            {
                if(posSampleReqPtr->positionSampleNodePtr->satInfo[i].satConst == constellation) {
                  if (count < (int)*svInfoLen) {
                    svInfoPtr[count].satId = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satId;
                    svInfoPtr[count].satConst = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satConst;
                    svInfoPtr[count].satTracked = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satTracked;
                    svInfoPtr[count].satSnr = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satSnr;
                    svInfoPtr[count].satAzim = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satAzim;
                    svInfoPtr[count].satElev = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satElev;
                    svInfoPtr[count].signalType = posSampleReqPtr->positionSampleNodePtr->satInfo[i].signalType;
                    svInfoPtr[count].glonassFcn = posSampleReqPtr->positionSampleNodePtr->satInfo[i].glonassFcn;
                    svInfoPtr[count].baseBandCnr = posSampleReqPtr->positionSampleNodePtr->satInfo[i].baseBandCnr;
                    count++;
                  }
                }
            }
            if ((int)*svInfoLen <= count) {
                result = LE_OVERFLOW;
            }
            *svInfoLen = count;
        }
        else
        {
            for(i=0; i<(int)*svInfoLen; i++)
            {
                svInfoPtr[i].satId = UINT16_MAX;
                svInfoPtr[i].satConst = TAF_GNSS_SV_CONSTELLATION_UNDEFINED;
                svInfoPtr[i].satTracked = false;
                svInfoPtr[i].satSnr = UINT8_MAX;
                svInfoPtr[i].satAzim = UINT16_MAX;
                svInfoPtr[i].satElev = UINT8_MAX;
                svInfoPtr[i].signalType = UINT32_MAX;
                svInfoPtr[i].glonassFcn = UINT16_MAX;
                svInfoPtr[i].baseBandCnr = 0.0;
            }
            result = LE_OUT_OF_RANGE;
        }

        if (posSampleReqPtr->positionSampleNodePtr->satsUsedCountValid)
        {
            for(i=0; i < (int)*svInfoLen; i++)
            {
                svInfoPtr[i].satUsed = posSampleReqPtr->positionSampleNodePtr->satInfo[i].satUsed;
            }
        }
        else
        {
            for(i=0; i < (int)*svInfoLen; i++)
            {
                svInfoPtr[i].satUsed = false;
            }
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

le_result_t taf_Gnss::SetMinGpsWeek
(
    uint16_t minGpsWeek
)
{
    le_result_t result = LE_FAULT;
    std::promise<telux::common::ErrorCode> p;

    switch (GnssState)
    {
        case TAF_GNSS_STATE_ACTIVE:
        case TAF_GNSS_STATE_DISABLED:
        case TAF_GNSS_STATE_UNINITIALIZED:
        {
             LE_ERROR("Wrong Gnss State [%d]", GnssState);
             result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
        {
            telux::common::ResponseCallback cb = [&p](telux::common::ErrorCode error) { p.set_value(error); };
            telux::common::Status status = mLocationConfigurator->configureMinGpsWeek(minGpsWeek, cb);

            if (status == Status::SUCCESS) {
                telux::common::ErrorCode error = p.get_future().get();
                if (error == ErrorCode::SUCCESS) {
                    return LE_OK;
                }
            }
            else
            {
                LE_INFO("configureMinGpsWeek is failed");
                result = LE_FAULT;
            }
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Invalid GNSS state %d", GnssState);
        }
        break;
    }

   return result;
}

le_result_t taf_Gnss::GetMinGpsWeek
(
   uint16_t*  minGpsWeekPtr
)
{
    le_result_t result = LE_FAULT;
    TAF_ERROR_IF_RET_VAL(NULL == minGpsWeekPtr, LE_FAULT, "minGpsWeekPtr is NULL");
    switch (GnssState)
    {
        case TAF_GNSS_STATE_DISABLED:
        case TAF_GNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("GetMinGpsWeek: Wrong Gnss State [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
        case TAF_GNSS_STATE_ACTIVE:
        {
            std::promise<uint16_t> p;
            std::promise<telux::common::ErrorCode> q;
            telux::loc::ILocationConfigurator::GetMinGpsWeekCallback cb =
                [&p, &q](uint16_t minGpsWeek, telux::common::ErrorCode error) {
                    p.set_value(minGpsWeek);
                    q.set_value(error);
                };

            telux::common::Status status = mLocationConfigurator->requestMinGpsWeek(cb);
            if (status != telux::common::Status::SUCCESS) {
                return result;
            }
            telux::common::ErrorCode error = q.get_future().get();
            LE_INFO("GetMinGpsWeek: error code %d", (int) error);
            if (error == ErrorCode::SUCCESS) {
                LE_INFO("GetMinGpsWeek is Success.");
                result = LE_OK;
                *minGpsWeekPtr = p.get_future().get();
            }
            else
            {
                LE_INFO("GetMinGpsWeek is Failed!");
                result = LE_FAULT;
            }
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("GetMinGpsWeek: Invalid GNSS state %d", GnssState);
        }
        break;
    }

    return result;
}

le_result_t taf_Gnss::GetCapabilities
(
   uint64_t*  locCapabilityPtr
)
{
    TAF_ERROR_IF_RET_VAL(NULL == locCapabilityPtr, LE_FAULT, "locCapabilityPtr is NULL");

    *locCapabilityPtr = mLocationManager->getCapabilities();

    LE_INFO("GetCapabilities: Location Capabilites is %" PRIu64 "", *locCapabilityPtr);

    return LE_OK;
}

le_result_t taf_Gnss::SetNmeaConfiguration
(
    taf_gnss_NmeaBitMask_t nmeaMask,         ///< [IN] Bit mask for enabled NMEA sentences.
    taf_gnss_GeodeticDatumType_t datumType,  ///< [IN] Specify the datum type to be configured.
    taf_gnss_LocEngineType_t engineType      ///< [IN] Specify the Engine type.
)
{
    le_result_t result = LE_NOT_PERMITTED;
    std::promise<telux::common::ErrorCode> p;

    telux::loc::NmeaConfig nmeaConfig;
    nmeaConfig.sentenceConfig = nmeaMask;
    if(datumType>=TAF_GNSS_GEODETIC_TYPE_WGS_84 && datumType <= TAF_GNSS_GEODETIC_TYPE_PZ_90)
    {
        nmeaConfig.datumType = (telux::loc::GeodeticDatumType) datumType;
    }
    else
    {
        return LE_FAULT;
    }
#if defined(TARGET_SA525M)
    if(engineType>= TAF_GNSS_LOC_ENGINE_FUSED && engineType <= TAF_GNSS_LOC_ENGINE_VPE)
    {
        nmeaConfig.engineType = engineType;
    }
    else
    {
        return LE_FAULT;
    }
#else
    (void)engineType;
#endif
    LE_DEBUG("SetNmeaConfiguration nmeaMask: %" PRIu64" and datumType: %d", nmeaMask, datumType);

    // Check if the bit mask is correct
    if (nmeaMask == 0)
    {
        LE_ERROR("Unable to set the enabled NMEA, wrong bit mask %" PRIu64 "", nmeaMask);
        result = LE_BAD_PARAMETER;
    }
    else
    {
        // Check the GNSS device state
        switch (GnssState)
        {
            case TAF_GNSS_STATE_READY:
            case TAF_GNSS_STATE_ACTIVE:
            {
                // Configure the NMEA sentences
                telux::common::ResponseCallback cb = [&p](telux::common::ErrorCode error) { p.set_value(error); };
                telux::common::Status status = mLocationConfigurator->configureNmea(nmeaConfig, cb);
                if (status == Status::SUCCESS) {
                    telux::common::ErrorCode error = p.get_future().get();
                    if (error == ErrorCode::SUCCESS) {
                        return LE_OK;
                    }
                }
                else
                {
                    result = LE_FAULT;
                    LE_INFO("SetNmeaConfiguration() is failed!");
                }
                if (LE_OK != result)
                {
                    LE_ERROR("Unable to set the enabled NMEA, error = %d (%s)",
                              result, LE_RESULT_TXT(result));
                }
            }
            break;
            case TAF_GNSS_STATE_UNINITIALIZED:
            case TAF_GNSS_STATE_DISABLED:
            {
                LE_ERROR("SetNmeaConfiguration: Bad state for that request [%d]", GnssState);
                result = LE_NOT_PERMITTED;
            }
            break;
            default:
            {
                LE_ERROR("SetNmeaConfiguration: Unknown GNSS state %d", GnssState);
                result = LE_FAULT;
            }
            break;
        }
    }

    return result;
}

void taf_Gnss::RemovePositionHandler
(
    taf_gnss_PositionHandlerRef_t handlerRef
)
{
    taf_gnss_PositionHandler_t* positionHandlerPtr =
        (taf_gnss_PositionHandler_t*)le_ref_Lookup(PositionHandlerRefMap, handlerRef);

    if (positionHandlerPtr != NULL)
    {
        LE_ASSERT(positionHandlerPtr->handlerRef == handlerRef);
        NumOfPositionHandlers--;

        LE_DEBUG("Removed positionHandlerRef(%p) for positionHandlerPtr(%p) (totalCnt=0x%x).",
                 positionHandlerPtr->handlerRef, positionHandlerPtr, NumOfPositionHandlers);
        le_mem_Release(positionHandlerPtr);
        le_ref_DeleteRef(PositionHandlerRefMap, handlerRef);
    }
    else
    {
        LE_ERROR("Invaild handlerRef(%p).", handlerRef);
    }
}

void taf_Gnss::ReleaseClientRef
(
    void* RefPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    void* clientPosPtr = le_ref_Lookup(gnss.ClientRequestRefMap, RefPtr);
    if (NULL == clientPosPtr)
    {
        LE_KILL_CLIENT("Invalid positioning service activation reference %p", RefPtr);
    }
    else
    {
        le_ref_DeleteRef(gnss.ClientRequestRefMap, RefPtr);
        LE_DEBUG("Remove Client Position Ctrl (%p)",RefPtr);
        le_mem_Release(clientPosPtr);
    }
}

void taf_Gnss::ReleaseSampleRef
(
 taf_gnss_SampleRef_t positionSampleRef
)
{
    auto &gnss = taf_Gnss::GetInstance();
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr = (taf_gnss_PositionSampleRequest_t*)le_ref_Lookup(gnss.PositionSampleMap,
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

void taf_Gnss::CloseEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    LE_INFO("SessionRef (%p) has been closed", sessionRef);

    TAF_ERROR_IF_RET_NIL( sessionRef == NULL, "sessionRef is NULL");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(gnss.PositionSampleMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        taf_gnss_PositionSampleRequest_t *positionSampleRequestPtr =
                                (taf_gnss_PositionSampleRequest_t*)le_ref_GetValue(iterRef);
        LE_ASSERT(positionSampleRequestPtr != NULL);

        if (positionSampleRequestPtr->sessionRef == sessionRef)
        {
            taf_gnss_SampleRef_t safeRef = (taf_gnss_SampleRef_t)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release taf_gnss_ReleaseSampleRef 0x%p, Session 0x%p", safeRef, sessionRef);

            le_ref_DeleteRef(gnss.PositionSampleMap, safeRef);
            le_mem_Release(positionSampleRequestPtr->positionSampleNodePtr);
            le_mem_Release(positionSampleRequestPtr);
        }

        result = le_ref_NextNode(iterRef);
    }

    iterRef = le_ref_GetIterator(gnss.ClientRequestRefMap);
    result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        taf_gnss_Client_t* gnssPtr = (taf_gnss_Client_t*) le_ref_GetValue(iterRef);
        LE_ASSERT(gnssPtr != NULL);

        if (sessionRef == gnssPtr->sessionRef)
        {
            void* safeRefPtr = (void*)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release taf_gnss_ReleaseClientRef 0x%p, Session 0x%p",
                     safeRefPtr, gnssPtr->sessionRef);

            gnss.ReleaseClientRef(safeRefPtr);
        }

        result = le_ref_NextNode(iterRef);
    }
}

taf_Gnss::~taf_Gnss() {
   if(mLocationManager && mPosListener) {
      mLocationManager->deRegisterListenerEx(mPosListener);
   }
   if(mPosListener) {
      mPosListener = nullptr;
   }

   if(mLocationManager) {
      mLocationManager = nullptr;
   }
}

void taf_Gnss::Init()
{

    telux::common::Status status = telux::common::Status::FAILED;
    SessionCtxList = LE_DLS_LIST_INIT;
    mTtffEnable = false;
    mTtffPtr = 0;
    GnssState = TAF_GNSS_STATE_UNINITIALIZED;
    NumOfPositionHandlers = 0;
    NumOfCapabilityHandlers = 0;
    NumOfNmeaHandlers = 0;
    memset(&LastPositionSample, 0, sizeof(LastPositionSample));
    LastPositionSample.fixState = TAF_GNSS_STATE_FIX_NO_POS;
    memset(&mSatParams, 0, sizeof(mSatParams));
    memset(&mSatInfo, 0, sizeof(mSatInfo));

    status = LocationManagerInit();
    if (status != telux::common::Status::SUCCESS) {
        LE_FATAL("LocationManager not available");
    }

    status = LocationConfiguratorInit();
    if (status != telux::common::Status::SUCCESS) {
        LE_FATAL("LocationConfigurator not available");
    }

    // UnComment this code for injection of RTCM3 format data
    /*status = DgnssManagerInit();
    if (status != telux::common::Status::SUCCESS) {
        LE_CRIT("DGnss not available");
    }*/

    switch (GnssState)
    {
        case TAF_GNSS_STATE_UNINITIALIZED:
            {
                GnssState = TAF_GNSS_STATE_READY;
            }
            break;
        case TAF_GNSS_STATE_READY:
        case TAF_GNSS_STATE_ACTIVE:
        case TAF_GNSS_STATE_DISABLED:
            {
                LE_ERROR("Bad state for that request [%d]", GnssState);
            }
            break;
        default:
            {
                LE_ERROR("Unknown GNSS state [%d]", GnssState);
            }
            break;
    }

    PositionHandlerPoolRef = le_mem_InitStaticPool(PositionHandler, GNSS_POSITION_HANDLER_HIGH,
            sizeof(taf_gnss_PositionHandler_t));

    PositionSamplePoolRef = le_mem_InitStaticPool(PositionSample, GNSS_POSITION_SAMPLE_MAX,
            sizeof(taf_gnss_PositionSample_t));

    PositionSampleRequestPoolRef = le_mem_InitStaticPool(PositionSampleRequest,
            GNSS_POSITION_SAMPLE_MAX, sizeof(taf_gnss_PositionSampleRequest_t));

    PositionHandlerRefMap = le_ref_CreateMap("PositionHandlerRefMap", 16);

    PositionSampleMap = le_ref_InitStaticMap(PositionSampleMap, GNSS_POSITION_SAMPLE_MAX);

    ClientRequestRefMap = le_ref_CreateMap("ClientRequestRefMap", TAF_CONFIG_POSITIONING_ACTIVATION_MAX);

    ClientPoolRef = le_mem_InitStaticPool(Client, TAF_CONFIG_POSITIONING_ACTIVATION_MAX, sizeof(taf_gnss_Client_t));

    positionEventId = le_event_CreateIdWithRefCounting("positionEventId");

    HandlerRef = le_event_AddHandler("LocUpdateEventId", positionEventId, taf_Gnss::GnssPositionHandler);

    locCapabilityEventId = le_event_CreateId("LocCapabilityEventId", sizeof(CapabilityChangeEvent_t));
    nmeaEventId = le_event_CreateId("NmeaEventId", sizeof(NmeaInfoEvent_t));

    le_msg_ServiceRef_t msgService = taf_gnss_GetServiceRef();
    le_msg_AddServiceCloseHandler(msgService, CloseEventHandler, NULL);
    mGnssMutexRef =  le_mutex_CreateRecursive("GnssMutex");

    return;
}
