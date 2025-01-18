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

LE_MEM_DEFINE_STATIC_POOL(PositionHandler, GNSS_POSITION_HANDLER_HIGH, sizeof(taf_locGnss_PositionHandler_t));
LE_MEM_DEFINE_STATIC_POOL(PositionSample, GNSS_POSITION_SAMPLE_MAX, sizeof(taf_locGnss_PositionSample_t));
LE_MEM_DEFINE_STATIC_POOL(PositionSampleRequest, GNSS_POSITION_SAMPLE_MAX, sizeof(taf_locGnss_PositionSampleRequest_t));
LE_MEM_DEFINE_STATIC_POOL(Client, LE_CONFIG_POSITIONING_ACTIVATION_MAX, sizeof(taf_locGnss_Client_t));
LE_REF_DEFINE_STATIC_MAP(PositionSampleMap, GNSS_POSITION_SAMPLE_MAX);
void bodyToSensorUtility(telux::loc::DREngineConfiguration& drConfig,
        const taf_locGnss_DrParams_t* drParamsPtr,taf_locGnss_Client_t* clientRequestPtr,
        le_result_t* sensor_Result);
void speedScaleUtility(telux::loc::DREngineConfiguration& drConfig,
        const taf_locGnss_DrParams_t* drParamsPtr,taf_locGnss_Client_t* clientRequestPtr,
        le_result_t* speedScale_Result);
void gyroScaleUtility(telux::loc::DREngineConfiguration& drConfig,
        const taf_locGnss_DrParams_t* drParamsPtr,taf_locGnss_Client_t* clientRequestPtr,
        le_result_t* gyroScale_Result);
void roundOffLocationData(double *locData, uint8_t dplace);
taf_locGnss &taf_locGnss::GetInstance()
{
    static taf_locGnss instance;
    return instance;
}

telux::common::Status taf_locGnss::DgnssManagerInit() {
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
            LE_DEBUG( "Failed to get Gnss manager object");
            return Status::FAILED;
        }

        startTime = std::chrono::system_clock::now();
        ServiceStatus dgnssMgrStatus = mDgnssManager->getServiceStatus();
        if(dgnssMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
            LE_DEBUG( "Dgnss subsystem is not ready, Please wait");
        }
        dgnssMgrStatus = prom.get_future().get();
        if(dgnssMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            LE_DEBUG( "Elapsed Time for Dgnss subsystems to ready : %lf", elapsedTime.count());
        } else {
            LE_DEBUG( "ERROR - Unable to initialize Dgnss subsystem");
            return telux::common::Status::NOTREADY;
        }
#endif
#ifdef TARGET_SA415M
        auto &locationFactory = LocationFactory::getInstance();
        mDgnssManager = locationFactory.getDgnssManager(DgnssDataFormat::DATA_FORMAT_RTCM_3);
        bool subSystemsStatus = mDgnssManager->isSubsystemReady();
        startTime = std::chrono::system_clock::now();
        if(!subSystemsStatus) {
            LE_DEBUG( "Dgnss subsystem is not ready, Please wait");
            std::future<bool> f = mDgnssManager->onSubsystemReady();
            subSystemsStatus = f.get();
        }

        if(subSystemsStatus) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            LE_DEBUG( "Elapsed Time for Dgnss subsystems to ready : %lf", elapsedTime.count());
        } else {
            LE_DEBUG( "ERROR - Unable to initialize Dgnss subsystem");
            return telux::common::Status::NOTREADY;
        }
#endif
   } else {
       LE_DEBUG("Dgnss manager is already initialized");
   }
   return telux::common::Status::SUCCESS;
}

telux::common::Status taf_locGnss::LocationManagerInit(taf_locGnss_Client_t* clientRequestPtr) {
    if(clientRequestPtr->locationManager == nullptr) {
        std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
        std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
        auto &locationFactory = LocationFactory::getInstance();
        clientRequestPtr->locationManager = locationFactory.getLocationManager([&](ServiceStatus status) {
                if (status == ServiceStatus::SERVICE_AVAILABLE) {
                    prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
                } else {
                    prom.set_value(ServiceStatus::SERVICE_UNAVAILABLE);
                }
                });
        startTime = std::chrono::system_clock::now();
        ServiceStatus locMgrStatus = clientRequestPtr->locationManager->getServiceStatus();
        if(locMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
            LE_DEBUG("Location subsystem is not ready, Please wait");
        }
        locMgrStatus = prom.get_future().get();
        if(locMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            LE_DEBUG("Elapsed Time for Subsystems to ready : %lf",elapsedTime.count());
        } else {
            LE_DEBUG("ERROR - Unable to initialize Location subsystem");
            return telux::common::Status::FAILED;
        }
#endif
#ifdef TARGET_SA415M
        auto &locationFactory = LocationFactory::getInstance();
        clientRequestPtr->locationManager = locationFactory.getLocationManager();
        bool subSystemsStatus = clientRequestPtr->locationManager->isSubsystemReady();
        startTime = std::chrono::system_clock::now();
        if(!subSystemsStatus) {
            LE_DEBUG( "Location subsystem is not ready, Please wait");
            std::future<bool> f = clientRequestPtr->locationManager->onSubsystemReady();
            subSystemsStatus = f.get();
        }

        if(subSystemsStatus) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            LE_DEBUG( "Elapsed Time for Subsystems to ready : %lf", elapsedTime.count());
        } else {
            LE_DEBUG( "ERROR - Unable to initialize Location subsystem");
            return telux::common::Status::NOTREADY;
        }
#endif

        clientRequestPtr->PositionHandlerRefMap = le_ref_CreateMap("PositionHandlerRefMap", 4);
        LE_DEBUG("LocationManagerInit for sessionRef: %p",  clientRequestPtr->sessionRef);
        clientRequestPtr->posListener = std::make_shared<tafLocationListener>();
        clientRequestPtr->posListener->clientSessionRef = &clientRequestPtr->sessionRef;

        clientRequestPtr->locationManager->registerListenerEx(clientRequestPtr->posListener);
        auto status = clientRequestPtr->locationManager->registerForSystemInfoUpdates(clientRequestPtr->posListener);
        if(telux::common::Status::SUCCESS == status)
        {
            LE_DEBUG("register a client specific listener for Location system information");
        }
        else
        {
            LE_DEBUG("cannot register client specific listener for Location system information");
        }
    } else {
        LE_DEBUG("Location manager already initialized");
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status taf_locGnss::LocationConfiguratorInit() {
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
            LE_DEBUG("Location configuration subsystem is not ready, Please wait");
        }
        locCfgStatus = prom.get_future().get();
        if(locCfgStatus == ServiceStatus::SERVICE_AVAILABLE) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            LE_DEBUG("Elapsed Time for configuration subsystems to ready : %lf",elapsedTime.count());
        } else {
            LE_DEBUG("ERROR - Unable to initialize Location configuration subsystem"
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
            LE_DEBUG("Location configuration subsystem is not ready, Please wait");
            std::future<bool> f = mLocationConfigurator->onSubsystemReady();
            subSystemsStatus = f.get();
        }

        if(subSystemsStatus) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            LE_DEBUG("Elapsed Time for configuration subsystems to ready : %lf",elapsedTime.count());
        } else {
            LE_DEBUG("ERROR - Unable to initialize Location configuration subsystem");
            return telux::common::Status::NOTREADY;
        }
#endif
    } else {
        LE_DEBUG("Location configurator is already initialized");
    }
    return telux::common::Status::SUCCESS;
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
    LastDataPtr->epochTime = CurrentDataPtr->epochTime;
    LastDataPtr->satsInViewCount = CurrentDataPtr->satsInViewCount;
    LastDataPtr->satsTrackingCount = CurrentDataPtr->satsTrackingCount;
    LastDataPtr->satsUsedCount = CurrentDataPtr->satsUsedCount;
    uint8_t i;
    for(i=0; i<TAF_LOCGNSS_SV_INFO_MAX_LEN; i++)
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

    for(i=0; i<TAF_LOCGNSS_SV_INFO_MAX_LEN; i++)
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
    for(i=0; i<TAF_LOCGNSS_MEASUREMENT_INFO_MAX; i++)
    {
        LastDataPtr->measInfo[i].gnssSignalType = CurrentDataPtr->measInfo[i].gnssSignalType;
        LastDataPtr->measInfo[i].gnssConstellation = CurrentDataPtr->measInfo[i].gnssConstellation;
        LastDataPtr->measInfo[i].gnssSvId = CurrentDataPtr->measInfo[i].gnssSvId;
    }
    LastDataPtr->measInfoCount = CurrentDataPtr->measInfoCount;
    LastDataPtr->reportStatus = CurrentDataPtr->reportStatus;
    LastDataPtr->altMeanSeaLevel = CurrentDataPtr->altMeanSeaLevel;
    for (i = 0; i < TAF_LOCGNSS_MEASUREMENT_INFO_MAX; i++) {
        LastDataPtr->SVIds[i] = CurrentDataPtr->SVIds[i];
    }
    LastDataPtr->SVIdsCount = CurrentDataPtr->SVIdsCount;
    for(i=0; i<TAF_LOCGNSS_NUMBER_OF_SIGNAL_TYPES_MAX; i++)
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

    le_ref_IterRef_t iterRef = le_ref_GetIterator(clientRequestPtr->PositionHandlerRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        posHandlerPtr = (taf_locGnss_PositionHandler_t*)le_ref_GetValue(iterRef);
        if(posHandlerPtr == NULL) {
            return;
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
            return;
        }
        posSampleReqPtr->positionSampleRef =
           (taf_locGnss_SampleRef_t)le_ref_CreateRef(gnss.PositionSampleMap, posSampleReqPtr);

        LE_DEBUG("Report sampleRef %p to the corresponding handler (handlerPtr %p)",
            posSampleReqPtr->positionSampleRef, posHandlerPtr->handlerFuncPtr);

        posHandlerPtr->handlerFuncPtr(posSampleReqPtr->positionSampleRef,
                                      posHandlerPtr->handlerContextPtr);
    }

    le_mem_Release(currentPosPtr);
}


void tafLocationListener::onDetailedEngineLocationUpdate(
      const std::vector<std::shared_ptr<telux::loc::ILocationInfoEx> > &locationEngineInfo) {
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = gnss.DiscoverSessionRef(*clientSessionRef);

    if (NULL == clientRequestPtr) {
        return;
    }

    for (auto locationInfo : locationEngineInfo) {
        if ((clientRequestPtr->mFirstFix) &&
            (locationInfo->getReportStatus() == telux::loc::ReportStatus::SUCCESS) &&
            (locationInfo->getLatitude() != NAN) &&
            (locationInfo->getLongitude() != NAN))
        {
            std::chrono::time_point<std::chrono::system_clock> mEndTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = mEndTime - clientRequestPtr->mStartTime;
            clientRequestPtr->mTtffPtr = elapsedTime.count() * 1e+3;

            if ((int)clientRequestPtr->mTtffPtr >= clientRequestPtr->mAcqRate)
            {
                clientRequestPtr->mFirstFix = false;
            } else{
                clientRequestPtr->mTtffPtr=  0;
            }
        }
    }
    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);
    if(gnss.NumOfPositionHandlers )
    {
        LE_DEBUG("**** Detailed Engine Location Report ****");
        for (auto locationInfo : locationEngineInfo) {
            if ( clientRequestPtr->mSvEnabled && clientRequestPtr->mGnssSigEnabled )
            {
                taf_locGnss_PositionSample_t* LocationData =
                        (taf_locGnss_PositionSample_t*)le_mem_ForceAlloc(gnss.PositionSamplePoolRef);
                uint8_t i;
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
                if(locationInfo->getAltitudeType() == telux::loc::AltitudeType::CALCULATED)
                {
                    clientRequestPtr->mAltType = TAF_LOCGNSS_ALT_TYPE_CALCULATED;
                    LE_DEBUG("onDetailedEngineLocationUpdate: AltitudeType->CALCULATED");
                }
                else if(locationInfo->getAltitudeType() == telux::loc::AltitudeType::ASSUMED)
                {
                    clientRequestPtr->mAltType = TAF_LOCGNSS_ALT_TYPE_ASSUMED;
                    LE_DEBUG("onDetailedEngineLocationUpdate: AltitudeType->ASSUMED");
                }
                else
                {
                    clientRequestPtr->mAltType = TAF_LOCGNSS_ALT_TYPE_UNKNOWN;
                    LE_DEBUG("onDetailedEngineLocationUpdate: AltitudeType->UNKNOWN");
                }
                for (auto i = 0; i < TAF_LOCGNSS_MEASUREMENT_INFO_MAX; i++) {
                    memset((void*) &LocationData->measInfo[i], 0, sizeof(taf_locGnss_GnssMeasurementInfo_t));
                }
                LocationData->measInfoCount = 0;
                std::vector<telux::loc::GnssMeasurementInfo> measInfo = locationInfo->getmeasUsageInfo();
                for (auto measInfoElement : measInfo) {
                    if (LocationData->measInfoCount < TAF_LOCGNSS_MEASUREMENT_INFO_MAX) {
                        LocationData->measInfo[LocationData->measInfoCount].gnssSignalType = measInfoElement.gnssSignalType;
                        LocationData->measInfo[LocationData->measInfoCount].gnssConstellation = (taf_locGnss_GnssSystem_t) measInfoElement.gnssConstellation;
                        LocationData->measInfo[LocationData->measInfoCount].gnssSvId = measInfoElement.gnssSvId;
                        LocationData->measInfoCount++;
                    }
                }

                std::vector<uint16_t> SVIds;
                locationInfo->getSVIds(SVIds);
                if(SVIds.size() > 0) {
                    for (auto i = 0; i < TAF_LOCGNSS_MEASUREMENT_INFO_MAX; i++) {
                        LocationData->SVIds[i] = 0;
                    }
                    LocationData->SVIdsCount = 0;
                    for (auto i = 0; i < (int) SVIds.size(); i++) {
                        if (LocationData->SVIdsCount < TAF_LOCGNSS_MEASUREMENT_INFO_MAX) {
                            LocationData->SVIds[LocationData->SVIdsCount] = SVIds.at(i);
                            LocationData->SVIdsCount++;
                        }
                    }
                }

                LocationData->reportStatus = (taf_locGnss_ReportStatus_t) locationInfo->getReportStatus();
                LocationData->altMeanSeaLevel = locationInfo->getAltitudeMeanSeaLevel();
                locData = 0.0;
                locData = locationInfo->getLatitude();
                roundOffLocationData(&locData,6);//round off to 6 decimal places
                LocationData->latitude = (int32_t)locData;
                LE_DEBUG("locationInfo->getLatitude():%.10f ",locationInfo->getLatitude());
                LE_DEBUG("LocationData->latitude:%d ",LocationData->latitude);
                locData = 0.0;
                locData = locationInfo->getLongitude();
                roundOffLocationData(&locData,6);//round off to 6 decimal places
                LocationData->longitude = (int32_t)locData;
                LE_DEBUG("locationInfo->getLongitude():%.10f ",locationInfo->getLongitude());
                LE_DEBUG("LocationData->longitude:%d ",LocationData->longitude);
                locData = 0.0;
                locData = locationInfo->getHorizontalUncertainty();
                roundOffLocationData(&locData,2);//round off to 2 decimal places
                LocationData->hAccuracy = (int32_t)locData;
                LE_DEBUG("locationInfo->getHorizontalUncertainty():%lf ",locationInfo->getHorizontalUncertainty());
                LE_DEBUG("LocationData->hAccuracy:%d ",LocationData->hAccuracy);
                locData = 0.0;
                locData = locationInfo->getAltitude();
                roundOffLocationData(&locData,3);//round off to 3 decimal places
                LocationData->altitude = (int32_t)locData;
                LE_DEBUG("locationInfo->getAltitude():%.10f ",locationInfo->getAltitude());
                LE_DEBUG("LocationData->altitude:%d ",LocationData->altitude);
                locData = 0.0;
                locData = locationInfo->getVerticalUncertainty();
                roundOffLocationData(&locData,1);//round off to 1 decimal place
                LocationData->vAccuracy = (int32_t)locData;
                LE_DEBUG("locationInfo->getVerticalUncertainty():%.10f ",locationInfo->getVerticalUncertainty());
                LE_DEBUG("LocationData->vAccuracy:%d ",LocationData->vAccuracy);
                LocationData->altitudeOnWgs84 = 0;
                LocationData->hSpeed = locationInfo->getSpeed()*100;
                LocationData->hSpeedAccuracy = locationInfo->getSpeedUncertainty()*1e+3;
                if(locationInfo->getVelocityEastNorthUp(clientRequestPtr->mVerticalSpeed) ==
                        telux::common::Status::SUCCESS)
                {
                    if(clientRequestPtr->mVerticalSpeed.size() == VERTICAL_SPEED_SIZE)
                    {
                        LocationData->vSpeed = (int32_t)
                            (clientRequestPtr->mVerticalSpeed[VERTICAL_SPEED_ACCURACY_INDEX]*100);
                        LE_DEBUG("onDetailedEngineLocationUpdate LocationData->vSpeed:%d ",
                            LocationData->vSpeed);
                        LE_DEBUG("onDetailedEngineLocationUpdate mVerticalSpeed:%lf ",
                            clientRequestPtr->mVerticalSpeed[clientRequestPtr->mVerticalSpeed.size()-1]);
                    }
                    else
                    {
                        LocationData->vSpeed = 0;
                        LE_DEBUG("mVerticalSpeed.size()!=3");
                    }
                }
                else
                {
                    LocationData->vSpeed = 0;
                    LE_DEBUG("onDetailedEngineLocationUpdate vSpeed fail");
                }
                if(locationInfo->getVelocityUncertaintyEastNorthUp(clientRequestPtr->mVerticalSpeedAccuracy) ==
                        telux::common::Status::SUCCESS)
                {
                    if(clientRequestPtr->mVerticalSpeedAccuracy.size() == VERTICAL_SPEED_SIZE)
                    {
                        LocationData->vSpeedAccuracy = (int32_t)
                            (clientRequestPtr->mVerticalSpeedAccuracy[VERTICAL_SPEED_ACCURACY_INDEX]*1000);
                        LE_DEBUG("onDetailedEngineLocationUpdate LocationData->vSpeedAccuracy:%d ",
                             LocationData->vSpeedAccuracy);
                        LE_DEBUG("onDetailedEngineLocationUpdate mVerticalSpeedAccuracy:%lf ",
                             clientRequestPtr->mVerticalSpeedAccuracy[clientRequestPtr->mVerticalSpeedAccuracy.size()-1]);
                    }
                    else
                    {
                        LocationData->vSpeedAccuracy = 0;
                        LE_DEBUG("mVerticalSpeedAccuracy.size()!=3");
                    }
                }
                else
                {
                    LocationData->vSpeedAccuracy = 0;
                    LE_DEBUG("onDetailedEngineLocationUpdate vSpeedAccuracy fail");
                }
                LocationData->magneticDeviation = locationInfo->getMagneticDeviation()*10;
                LocationData->epochTime = locationInfo->getTimeStamp();
                LE_DEBUG("onDetailedEngineLocationUpdate epochTime is : %" PRIu64"",locationInfo->getTimeStamp());
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
                float value = locationInfo->getTimeUncMs();
                LE_DEBUG("timeAccuracy value:%lf",value);
                if ((value * 1e+6) >= UINT32_MAX)
                {
                    LocationData->timeAccuracy = UINT32_MAX;
                    LE_DEBUG("timeAccuracy->UINT32_MAX:%d",LocationData->timeAccuracy);
                }
                else
                {
                    // do the round off to nanosecond
                    LocationData->timeAccuracy = (uint32_t)((uint64_t)((value*1e+7)+5))/10;
                    LE_DEBUG("LocationData->timeAccuracy:%d",LocationData->timeAccuracy);
                }
                LocationData->positionLatency = 0;
                LocationData->hdop = locationInfo->getHorizontalDop() *1e+3;
                LocationData->vdop = locationInfo->getVerticalDop() * 1e+3;
                LocationData->pdop = locationInfo->getPositionDop() * 1e+3;
                LocationData->gdop = locationInfo->getGeometricDop() * 1e+3;
                LocationData->tdop = locationInfo->getTimeDop() * 1e+3;
                if(locationInfo->getTimeStamp() != telux::loc::UNKNOWN_TIMESTAMP) {
                    time_t realtime;
                    realtime = (time_t)((locationInfo->getTimeStamp() / 1000));
                    tm *ltm = gmtime(&realtime);
                    LE_DEBUG("onDetailedEngineLocationUpdate !UNKNOWN_TIMESTAMP");
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
                    LE_DEBUG("onDetailedEngineLocationUpdate UNKNOWN_TIMESTAMP");
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
                if(locationInfo->getLeapSeconds(clientRequestPtr->mLeapSeconds) ==
                        telux::common::Status::SUCCESS)
                {
                    LocationData->leapSeconds = clientRequestPtr->mLeapSeconds;
                } else {
                    LocationData->leapSeconds = 0;
                }
                LocationData->leapSecondsUnc = locationInfo->getLeapSecondsUncertainty();
                LocationData->satsInViewCount = clientRequestPtr->mSatParams.satsInViewCount;
                LocationData->satsTrackingCount = clientRequestPtr->mTotalSVTracked;
                LocationData->satsUsedCount = locationInfo->getNumSvUsed();

                for(i=0; i<TAF_LOCGNSS_SV_INFO_MAX_LEN; i++)
                {
                    LocationData->satInfo[i].satId = clientRequestPtr->mSatInfo[i].satId;
                    LocationData->satInfo[i].satConst = clientRequestPtr->mSatInfo[i].satConst;
                    LocationData->satInfo[i].satUsed = clientRequestPtr->mSatInfo[i].satUsed;
                    LocationData->satInfo[i].satTracked = clientRequestPtr->mSatInfo[i].satTracked;
                    LocationData->satInfo[i].satSnr = clientRequestPtr->mSatInfo[i].satSnr;
                    LocationData->satInfo[i].satAzim = clientRequestPtr->mSatInfo[i].satAzim;
                    LocationData->satInfo[i].satElev = clientRequestPtr->mSatInfo[i].satElev;
                    LocationData->satInfo[i].signalType = clientRequestPtr->mSatInfo[i].signalType;
                    LocationData->satInfo[i].glonassFcn = clientRequestPtr->mSatInfo[i].glonassFcn;
                    LocationData->satInfo[i].baseBandCnr = clientRequestPtr->mSatInfo[i].baseBandCnr;
                }

                for(i=0; i<TAF_LOCGNSS_SV_INFO_MAX_LEN; i++)
                {
                    LocationData->satMeas[i].satId = 0;
                    LocationData->satMeas[i].satLatency = 0;
                }
                LocationData->robustConformity = locationInfo->getConformityIndex();
                LE_DEBUG("onDetailedEngineLocationUpdate LocationData->robustConformity %lf",
                                                (float)LocationData->robustConformity);
                LocationData->confidencePercent = locationInfo->getCalibrationConfidencePercent();
                telux::loc::DrCalibrationStatus calibrationStatus =
                                                locationInfo->getCalibrationStatus();
                LE_DEBUG("onDetailedEngineLocationUpdate calibrationStatus %d", calibrationStatus);
                if((calibrationStatus & telux::loc::DR_ROLL_CALIBRATION_NEEDED))
                {
                    LE_DEBUG("onDetailedEngineLocationUpdate Roll calibration is needed");
                    LocationData->calibrationStatus |= ((1<<TAF_LOCGNSS_DR_ROLL_CALIBRATION_NEEDED));
                }
                if((calibrationStatus & telux::loc::DR_PITCH_CALIBRATION_NEEDED))
                {
                    LE_DEBUG("onDetailedEngineLocationUpdate Pitch calibration is needed");
                    LocationData->calibrationStatus |= ((1<<TAF_LOCGNSS_DR_PITCH_CALIBRATION_NEEDED));
                }
                if((calibrationStatus & telux::loc::DR_YAW_CALIBRATION_NEEDED))
                {
                    LE_DEBUG("onDetailedEngineLocationUpdate Yaw calibration is needed");
                    LocationData->calibrationStatus |= ((1<<TAF_LOCGNSS_DR_YAW_CALIBRATION_NEEDED));
                }
                if((calibrationStatus & telux::loc::DR_ODO_CALIBRATION_NEEDED))
                {
                    LE_DEBUG("onDetailedEngineLocationUpdate Odo calibration is needed");
                    LocationData->calibrationStatus |= ((1<<TAF_LOCGNSS_DR_ODO_CALIBRATION_NEEDED));
                }
                if((calibrationStatus & telux::loc::DR_GYRO_CALIBRATION_NEEDED))
                {
                    LE_DEBUG("onDetailedEngineLocationUpdate Gyro calibration is needed");
                    LocationData->calibrationStatus |= ((1<<TAF_LOCGNSS_DR_GYRO_CALIBRATION_NEEDED));
                }
                telux::loc::DrSolutionStatus solutionStatus =
                                                locationInfo->getSolutionStatus();//lsc
                LE_DEBUG("DR solution status %d", solutionStatus);
                if((solutionStatus & telux::loc::VEHICLE_SENSOR_SPEED_INPUT_DETECTED))
                {
                    LE_DEBUG("Vehicle sensor speed input was detected");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_VEHICLE_SENSOR_SPEED_INPUT_DETECTED;
                }
                if((solutionStatus & telux::loc::VEHICLE_SENSOR_SPEED_INPUT_USED))
                {
                    LE_DEBUG("Vehicle sensor speed input was used");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_VEHICLE_SENSOR_SPEED_INPUT_USED;
                }
                if((solutionStatus & telux::loc::WARNING_UNCALIBRATED))
                {
                    LE_DEBUG("Dead recokining engine solution disengaged");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_UNCALIBRATED;
                }
                if((solutionStatus & telux::loc::WARNING_GNSS_QUALITY_INSUFFICIENT))
                {
                    LE_DEBUG("DRE solution disengaged due to bad GNSS quality");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_GNSS_QUALITY_INSUFFICIENT;
                }
                if((solutionStatus & telux::loc::WARNING_FERRY_DETECTED))
                {
                    LE_DEBUG("DRE solution disengaged as ferry condition detected");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_FERRY_DETECTED;
                }
                if((solutionStatus & telux::loc::ERROR_6DOF_SENSOR_UNAVAILABLE))
                {
                    LE_DEBUG("DRE solution disengaged as 6DOF sensor inputs not available");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_ERROR_6DOF_SENSOR_UNAVAILABLE;
                }
                if((solutionStatus & telux::loc::ERROR_VEHICLE_SPEED_UNAVAILABLE))
                {
                    LE_DEBUG("DRE solution disengaged as vehicle speed inputs not available");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_ERROR_VEHICLE_SPEED_UNAVAILABLE;
                }
                if((solutionStatus & telux::loc::ERROR_GNSS_EPH_UNAVAILABLE))
                {
                    LE_DEBUG("DRE solution disengaged as Ephemeris info not available");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_ERROR_GNSS_EPH_UNAVAILABLE;
                }
                if((solutionStatus & telux::loc::ERROR_GNSS_MEAS_UNAVAILABLE))
                {
                    LE_DEBUG("DRE solution disengaged as GNSS measurement info not available");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_ERROR_GNSS_MEAS_UNAVAILABLE;
                }
                if((solutionStatus & telux::loc::WARNING_INIT_POSITION_INVALID))
                {
                    LE_DEBUG("DRE solution disengaged due to non-availability of stored position from previous session");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_INIT_POSITION_INVALID;
                }
                if((solutionStatus & telux::loc::WARNING_INIT_POSITION_UNRELIABLE))
                {
                    LE_DEBUG("DRE solution dis-engaged due to vehicle motion detected at session start");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_INIT_POSITION_UNRELIABLE;
                }
                if((solutionStatus & telux::loc::WARNING_POSITON_UNRELIABLE))
                {
                    LE_DEBUG("DRE solution dis-engaged due to unreliable position");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_POSITON_UNRELIABLE;
                }
                if((solutionStatus & telux::loc::ERROR_GENERIC))
                {
                    LE_DEBUG("DRE solution dis-engaged due to a generic error");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_ERROR_GENERIC;
                }
                if((solutionStatus & telux::loc::WARNING_SENSOR_TEMP_OUT_OF_RANGE))
                {
                    LE_DEBUG("DRE solution dis-engaged due to Sensor Temperature being out of range");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_SENSOR_TEMP_OUT_OF_RANGE;
                }
                if((solutionStatus & telux::loc::WARNING_FACTORY_DATA_INCONSISTENT))
                {
                    LE_DEBUG("DRE solution dis-engaged due to insufficient user dynamics");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_USER_DYNAMICS_INSUFFICIENT;
                }
                if((solutionStatus & telux::loc::WARNING_USER_DYNAMICS_INSUFFICIENT))
                {
                    LE_DEBUG("DRE solution dis-engaged due to inconsistent factory data ");
                    LocationData->drSolutionStatus |= TAF_LOCGNSS_WARNING_FACTORY_DATA_INCONSISTENT;
                }
                LE_DEBUG("onDetailedEngineLocationUpdate Location position dynamic");
                telux::loc::GnssKinematicsData GnssKinData = locationInfo->getBodyFrameData();
                telux::loc::KinematicDataValidity GnssKinDataValidity =
                                                  GnssKinData.bodyFrameDataMask;
                LE_DEBUG("onDetailedEngineLocationUpdate GnssKinDataValidity:%0x",
                                                  GnssKinDataValidity);
                if((GnssKinDataValidity & telux::loc::HAS_LONG_ACCEL))
                {
                    LE_DEBUG("Navigation data has Forward Acceleration");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_LOCGNSS_HAS_LONG_ACCEL);
                }
                if((GnssKinDataValidity & telux::loc::HAS_LAT_ACCEL))
                {
                    LE_DEBUG("Navigation data has Sideward Acceleration");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_LOCGNSS_HAS_LAT_ACCEL);
                }
                if((GnssKinDataValidity & telux::loc::HAS_VERT_ACCEL))
                {
                    LE_DEBUG("Navigation data has Vertical Acceleration");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_LOCGNSS_HAS_VERT_ACCEL);
                }
                if((GnssKinDataValidity & telux::loc::HAS_YAW_RATE))
                {
                    LE_DEBUG("Navigation data has Heading Rate");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_LOCGNSS_HAS_YAW_RATE);
                }
                if((GnssKinDataValidity & telux::loc::HAS_PITCH))
                {
                    LE_DEBUG("Navigation data has Pitch");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |= (1<<TAF_LOCGNSS_HAS_PITCH);
                }
                if((GnssKinDataValidity & telux::loc::HAS_LONG_ACCEL_UNC))
                {
                    LE_DEBUG("Navigation data has Forward Acceleration Unc");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_LOCGNSS_HAS_LONG_ACCEL_UNC);
                }
                if((GnssKinDataValidity & telux::loc::HAS_LAT_ACCEL_UNC))
                {
                    LE_DEBUG("Navigation data has Sideward Acceleration Unc");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_LOCGNSS_HAS_LAT_ACCEL_UNC);
                }
                if((GnssKinDataValidity & telux::loc::HAS_VERT_ACCEL_UNC))
                {
                    LE_DEBUG("Navigation data has Vertical Acceleration Unc");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                    (1<<TAF_LOCGNSS_HAS_VERT_ACCEL_UNC);
                }
                if((GnssKinDataValidity & telux::loc::HAS_YAW_RATE_UNC))
                {
                    LE_DEBUG("Navigation data has Heading Rate Unc");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                    (1<<TAF_LOCGNSS_HAS_YAW_RATE_UNC);
                }
                if((GnssKinDataValidity & telux::loc::HAS_PITCH_UNC))
                {
                    LE_DEBUG("Navigation data has Body Pitch Unc");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                    (1<<TAF_LOCGNSS_HAS_PITCH_UNC);
                }
                if((GnssKinDataValidity & telux::loc::HAS_PITCH_RATE_BIT))
                {
                    LE_DEBUG("Navigation data has Pitch rate");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_LOCGNSS_HAS_PITCH_RATE_BIT);
                }
                if((GnssKinDataValidity & telux::loc::HAS_PITCH_RATE_UNC_BIT))
                {
                    LE_DEBUG("Navigation data has Pitch rate Unc");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_LOCGNSS_HAS_PITCH_RATE_UNC_BIT);
                }
                if((GnssKinDataValidity & telux::loc::HAS_ROLL_BIT))
                {
                    LE_DEBUG("Navigation data has roll");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |= (1<<TAF_LOCGNSS_HAS_ROLL_BIT);
                }
                if((GnssKinDataValidity & telux::loc::HAS_ROLL_UNC_BIT))
                {
                    LE_DEBUG("Navigation data has roll Unc ");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_LOCGNSS_HAS_ROLL_UNC_BIT);
                }
                if((GnssKinDataValidity & telux::loc::HAS_ROLL_RATE_BIT))
                {
                    LE_DEBUG("Navigation data has roll rate ");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_LOCGNSS_HAS_ROLL_RATE_BIT);
                }
                if((GnssKinDataValidity & telux::loc::HAS_ROLL_RATE_UNC_BIT))
                {
                    LE_DEBUG("Navigation data has roll rate Unc ");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |=
                                                     (1<<TAF_LOCGNSS_HAS_ROLL_RATE_UNC_BIT);
                }
                if((GnssKinDataValidity & telux::loc::HAS_YAW_BIT))
                {
                    LE_DEBUG("Navigation data has Yaw bit ");
                    LocationData->GnssKinematicsData.bodyFrameDataMask |= (1<<TAF_LOCGNSS_HAS_YAW_BIT);
                }
                if((GnssKinDataValidity & telux::loc::HAS_YAW_UNC_BIT))
                {
                    LE_DEBUG("Navigation data has Yaw bit Unc ");
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
                    LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTION_IONO);
                    LE_DEBUG("SBAS ionospheric correction is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_FAST])
                {
                    LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTION_FAST);
                    LE_DEBUG("SBAS fast correction is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_LONG])
                {
                    LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTION_LONG);
                    LE_DEBUG("SBAS long correction is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_INTEGRITY])
                {
                    LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_INTEGRITY);
                    LE_DEBUG("SBAS integrity information is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_DGNSS])
                {
                    LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTION_DGNSS);
                    LE_DEBUG("SBAS DGNSS correction information is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_RTK])
                {
                    LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTION_RTK);
                    LE_DEBUG("SBAS RTK correction information is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_PPP])
                {
                    LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTION_PPP);
                    LE_DEBUG("SBAS PPP correction information is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)telux::loc::SBAS_CORRECTION_RTK_FIXED])
                {
                    LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTION_RTK_FIXED);
                    LE_DEBUG("SBAS RTK fixed correction information is used");
                }
                if(correction[(telux::loc::SbasCorrectionType)
                               telux::loc::SBAS_CORRECTION_ONLY_SBAS_CORRECTED_SV_USED_])
                {
                    LocationData->sbasMask |= (1<<TAF_LOCGNSS_SBAS_CORRECTED_SV_USED);
                    LE_DEBUG("SBAS corrected SV is used");
                }
                telux::loc::LocationInfoValidity validityMask = locationInfo->getLocationInfoValidity();
                LE_DEBUG("LocationInfoExValidity->validityMask: %u ",validityMask);
                if((validityMask & telux::loc::HAS_LAT_LONG_BIT))
                {
                    LocationData->validityMask |= TAF_LOCGNSS_HAS_LAT_LONG_BIT;
                    LE_DEBUG("valid latitude longitude");
                }
                if((validityMask & telux::loc::HAS_ALTITUDE_BIT))
                {
                    LocationData->validityMask |= TAF_LOCGNSS_HAS_ALTITUDE_BIT;
                    LE_DEBUG("valid altitude");
                }
                if((validityMask & telux::loc::HAS_SPEED_BIT))
                {
                    LocationData->validityMask |= TAF_LOCGNSS_HAS_SPEED_BIT;
                    LE_DEBUG("valid speed");
                }
                if((validityMask & telux::loc::HAS_HEADING_BIT))
                {
                    LocationData->validityMask |= TAF_LOCGNSS_HAS_HEADING_BIT;
                    LE_DEBUG("valid heading");
                }
                if((validityMask & telux::loc::HAS_HORIZONTAL_ACCURACY_BIT))
                {
                    LocationData->validityMask |= TAF_LOCGNSS_HAS_HORIZONTAL_ACCURACY_BIT;
                    LE_DEBUG("valid horizontal accuracy");
                }
                if((validityMask & telux::loc::HAS_VERTICAL_ACCURACY_BIT))
                {
                    LocationData->validityMask |= TAF_LOCGNSS_HAS_VERTICAL_ACCURACY_BIT;
                    LE_DEBUG("valid vertical accuracy");
                }
                if((validityMask & telux::loc::HAS_SPEED_ACCURACY_BIT))
                {
                    LocationData->validityMask |= TAF_LOCGNSS_HAS_SPEED_ACCURACY_BIT;
                    LE_DEBUG("valid speed accuracy");
                }
                if((validityMask & telux::loc::HAS_HEADING_ACCURACY_BIT))
                {
                    LocationData->validityMask |= TAF_LOCGNSS_HAS_HEADING_ACCURACY_BIT;
                    LE_DEBUG("valid heading accuracy");
                }
                if((validityMask & telux::loc::HAS_TIMESTAMP_BIT))
                {
                    LocationData->validityMask |= TAF_LOCGNSS_HAS_TIMESTAMP_BIT;
                    LE_DEBUG("valid timestamp");
                }
                if((validityMask & telux::loc::HAS_ELAPSED_REAL_TIME_BIT))
                {
                    LocationData->validityMask |= TAF_LOCGNSS_HAS_ELAPSED_REAL_TIME_BIT;
                    LE_DEBUG("valid elapsed real time");
                }
                if((validityMask & telux::loc::HAS_ELAPSED_REAL_TIME_UNC_BIT))
                {
                    LocationData->validityMask |= TAF_LOCGNSS_HAS_ELAPSED_REAL_TIME_UNC_BIT;
                    LE_DEBUG("valid elapsed real time Uncertainity");
                }
                if((validityMask & telux::loc::HAS_GPTP_TIME_BIT))
                {
                    LocationData->validityMask |= TAF_LOCGNSS_HAS_GPTP_TIME_BIT;
                    LE_DEBUG("valid Gptp time");
                }
                if((validityMask & telux::loc::HAS_GPTP_TIME_UNC_BIT))
                {
                    LocationData->validityMask |= TAF_LOCGNSS_HAS_GPTP_TIME_UNC_BIT;
                    LE_DEBUG("valid Gptp time Uncertainity");
                }
                telux::loc::LocationInfoExValidity validityExMask =
                                                   locationInfo->getLocationInfoExValidity();
                LE_DEBUG("LocationInfoExValidity->validityExMask: %" PRIu64 "", validityExMask);
                if((validityExMask & telux::loc::HAS_ALTITUDE_MEAN_SEA_LEVEL))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_ALTITUDE_MEAN_SEA_LEVEL);
                    LE_DEBUG("valid altitude mean sea level");
                }
                if((validityExMask & telux::loc::HAS_DOP))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_DOP);
                    LE_DEBUG("valid pdop, hdop, vdop");
                }
                if((validityExMask & telux::loc::HAS_MAGNETIC_DEVIATION))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_MAGNETIC_DEVIATION);
                    LE_DEBUG("valid magnetic deviation");
                }
                if((validityExMask & telux::loc::HAS_HOR_RELIABILITY))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_HOR_RELIABILITY);
                    LE_DEBUG("valid horizontal reliability");
                }
                if((validityExMask & telux::loc::HAS_VER_RELIABILITY))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_VER_RELIABILITY);
                    LE_DEBUG("valid vertical reliability");
                }
                if((validityExMask & telux::loc::HAS_HOR_ACCURACY_ELIP_SEMI_MAJOR))
                {
                    LocationData->validityExMask |=
                                                 (1ULL << TAF_LOCGNSS_HAS_HOR_ACCURACY_ELIP_SEMI_MAJOR);
                    LE_DEBUG("valid elipsode semi major");
                }
                if((validityExMask & telux::loc::HAS_HOR_ACCURACY_ELIP_SEMI_MINOR))
                {
                    LocationData->validityExMask |=
                                                 (1ULL << TAF_LOCGNSS_HAS_HOR_ACCURACY_ELIP_SEMI_MINOR);
                    LE_DEBUG("valid elipsode semi minor");
                }
                if((validityExMask & telux::loc::HAS_HOR_ACCURACY_ELIP_AZIMUTH))
                {
                    LocationData->validityExMask |=
                                                 (1ULL << TAF_LOCGNSS_HAS_HOR_ACCURACY_ELIP_AZIMUTH);
                    LE_DEBUG("valid accuracy elipsode azimuth");
                }
                if((validityExMask & telux::loc::HAS_GNSS_SV_USED_DATA))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_GNSS_SV_USED_DATA);
                    LE_DEBUG("valid gnss sv used in pos data");
                }
                if((validityExMask & telux::loc::HAS_NAV_SOLUTION_MASK))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_NAV_SOLUTION_MASK);
                    LE_DEBUG("valid navSolutionMask");
                }
                if((validityExMask & telux::loc::HAS_POS_TECH_MASK))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_POS_TECH_MASK);
                    LE_DEBUG("valid LocPosTechMask");
                }
                if((validityExMask & telux::loc::HAS_SV_SOURCE_INFO))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_SV_SOURCE_INFO);
                    LE_DEBUG("valid LocSvInfoSource");
                }
                if((validityExMask & telux::loc::HAS_POS_DYNAMICS_DATA))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_POS_DYNAMICS_DATA);
                    LE_DEBUG("valid position dynamics data");
                }
                if((validityExMask & telux::loc::HAS_EXT_DOP))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_EXT_DOP);
                    LE_DEBUG("valid gdop, tdop");
                }
                if((validityExMask & telux::loc::HAS_NORTH_STD_DEV))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_NORTH_STD_DEV);
                    LE_DEBUG("valid North standard deviation");
                }
                if((validityExMask & telux::loc::HAS_EAST_STD_DEV))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_EAST_STD_DEV);
                    LE_DEBUG("valid East standard deviation");
                }
                if((validityExMask & telux::loc::HAS_NORTH_VEL))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_NORTH_VEL);
                    LE_DEBUG("valid North Velocity");
                }
                if((validityExMask & telux::loc::HAS_EAST_VEL))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_EAST_VEL);
                    LE_DEBUG("valid East Velocity");
                }
                if((validityExMask & telux::loc::HAS_UP_VEL))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_UP_VEL);
                    LE_DEBUG("valid Up Velocity");
                }
                if((validityExMask & telux::loc::HAS_NORTH_VEL_UNC))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_NORTH_VEL_UNC);
                    LE_DEBUG("valid North Velocity Uncertainty");
                }
                if((validityExMask & telux::loc::HAS_EAST_VEL_UNC))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_EAST_VEL_UNC);
                    LE_DEBUG("valid East Velocity Uncertainty");
                }
                if((validityExMask & telux::loc::HAS_UP_VEL_UNC))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_UP_VEL_UNC);
                    LE_DEBUG("valid Up Velocity Uncertainty");
                }
                if((validityExMask & telux::loc::HAS_LEAP_SECONDS))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_LEAP_SECONDS);
                    LE_DEBUG("valid leap_seconds");
                }
                if((validityExMask & telux::loc::HAS_TIME_UNC))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_TIME_UNC);
                    LE_DEBUG("valid timeUncMs");
                }
                if((validityExMask & telux::loc::HAS_NUM_SV_USED_IN_POSITION))
                {
                    LocationData->validityExMask |=
                                               (1ULL << TAF_LOCGNSS_HAS_NUM_SV_USED_IN_POSITION);
                    LE_DEBUG("valid number of sv used");
                }
                if((validityExMask & telux::loc::HAS_CALIBRATION_CONFIDENCE_PERCENT))
                {
                    LocationData->validityExMask |=
                                               (1ULL << TAF_LOCGNSS_HAS_CALIBRATION_CONFIDENCE_PERCENT);
                    LE_DEBUG("valid sensor calibrationConfidencePercent");
                }
                if((validityExMask & telux::loc::HAS_CALIBRATION_STATUS))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_CALIBRATION_STATUS);
                    LE_DEBUG("valid sensor calibrationConfidence");
                }
                if((validityExMask & telux::loc::HAS_OUTPUT_ENG_TYPE))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_OUTPUT_ENG_TYPE);
                    LE_DEBUG("valid output engine type");
                }
                if((validityExMask & telux::loc::HAS_OUTPUT_ENG_MASK))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_OUTPUT_ENG_MASK);
                    LE_DEBUG("valid output engine mask");
                }
                if((validityExMask & telux::loc::HAS_CONFORMITY_INDEX_FIX))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_CONFORMITY_INDEX_FIX);
                    LE_DEBUG("valid conformity index");
                }
                if((validityExMask & telux::loc::HAS_LLA_VRP_BASED))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_LLA_VRP_BASED);
                    LE_DEBUG("valid lla vrp based");
                }
                if((validityExMask & telux::loc::HAS_ENU_VELOCITY_VRP_BASED))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_ENU_VELOCITY_VRP_BASED);
                    LE_DEBUG("valid enu velocity vrp based");
                }
                if((validityExMask & telux::loc::HAS_ALTITUDE_TYPE))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_ALTITUDE_TYPE);
                    LE_DEBUG("valid altitude type");
                }
                if((validityExMask & telux::loc::HAS_REPORT_STATUS))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_REPORT_STATUS);
                    LE_DEBUG("valid report status");
                }
                if((validityExMask & telux::loc::HAS_INTEGRITY_RISK_USED))
                {
                    LocationData->validityExMask |= (1ULL << TAF_LOCGNSS_HAS_INTEGRITY_RISK_USED);
                    LE_DEBUG("valid integrity risk");
                }
                if((validityExMask & telux::loc::HAS_PROTECT_LEVEL_ALONG_TRACK))
                {
                    LocationData->validityExMask |=
                                                (1ULL << TAF_LOCGNSS_HAS_PROTECT_LEVEL_ALONG_TRACK);
                    LE_DEBUG("valid protect along track");
                }
                if((validityExMask & telux::loc::HAS_PROTECT_LEVEL_CROSS_TRACK))
                {
                    LocationData->validityExMask |=
                                                (1ULL << TAF_LOCGNSS_HAS_PROTECT_LEVEL_CROSS_TRACK);
                    LE_DEBUG("valid protect cross track");
                }
                if((validityExMask & telux::loc::HAS_PROTECT_LEVEL_VERTICAL))
                {
                    LocationData->validityExMask |=
                                                (1ULL << TAF_LOCGNSS_HAS_PROTECT_LEVEL_VERTICAL);
                    LE_DEBUG("valid protect vertical");
                }
                if((validityExMask & telux::loc::HAS_LEAP_SECONDS_UNC))
                {
                    LocationData->validityExMask |=
                                                (1ULL << TAF_LOCGNSS_HAS_LEAP_SECONDS_UNC);
                    LE_DEBUG("valid Leap seconds uncertainty");
                }
                telux::loc::PositioningEngine posEngineBits = locationInfo->getLocOutputEngMask();
                if(posEngineBits & telux::loc::STANDARD_POSITIONING_ENGINE)
                {
                    LocationData->engMask |= TAF_LOCGNSS_STANDARD_POSITIONING_ENGINE;
                    LE_DEBUG("eng Mask is STANDARD_POSITIONING_ENGINE");
                }
                if(posEngineBits & telux::loc::DEAD_RECKONING_ENGINE)
                {
                    LocationData->engMask |= TAF_LOCGNSS_DEAD_RECKONING_ENGINE;
                    LE_DEBUG("eng Mask is DEAD_RECKONING_ENGINE");
                }
                if(posEngineBits & telux::loc::PRECISE_POSITIONING_ENGINE)
                {
                    LocationData->engMask |= TAF_LOCGNSS_PRECISE_POSITIONING_ENGINE;
                    LE_DEBUG("eng Mask is PRECISE_POSITIONING_ENGINE");
                }
                if(posEngineBits & telux::loc::VP_POSITIONING_ENGINE)
                {
                    LocationData->engMask |= TAF_LOCGNSS_VP_POSITIONING_ENGINE;
                    LE_DEBUG("eng Mask is VP_POSITIONING_ENGINE");
                }
                telux::loc::LocationAggregationType locEngineType = locationInfo->getLocOutputEngType();
                if(locEngineType == telux::loc::LOC_OUTPUT_ENGINE_FUSED)
                {
                    LocationData->locationEngType = TAF_LOCGNSS_LOC_OUTPUT_ENGINE_FUSED;
                    LE_DEBUG("location eng type is FUSED");
                }
                if(locEngineType == telux::loc::LOC_OUTPUT_ENGINE_SPE)
                {
                    LocationData->locationEngType = TAF_LOCGNSS_LOC_OUTPUT_ENGINE_SPE;
                    LE_DEBUG("location eng type is SPE");
                }
                if(locEngineType == telux::loc::LOC_OUTPUT_ENGINE_PPE)
                {
                    LocationData->locationEngType = TAF_LOCGNSS_LOC_OUTPUT_ENGINE_PPE;
                    LE_DEBUG("location eng type is PPE");
                }
                if(locEngineType == telux::loc::LOC_OUTPUT_ENGINE_VPE)
                {
                    LocationData->locationEngType = TAF_LOCGNSS_LOC_OUTPUT_ENGINE_VPE;
                    LE_DEBUG("location eng type is VPE");
                }
                telux::loc::LocationReliability locReliability =
                                                   locationInfo->getHorizontalReliability();
                if(locReliability == telux::loc::LocationReliability::NOT_SET)
                {
                    LocationData->horiReliablity = TAF_LOCGNSS_RELIABILITY_NOT_SET;
                    LE_DEBUG("horizontal reliablity is NOT SET");
                }
                else if(locReliability == telux::loc::LocationReliability::VERY_LOW)
                {
                    LocationData->horiReliablity = TAF_LOCGNSS_RELIABILITY_VERY_LOW;
                    LE_DEBUG("horizontal reliablity is VERY LOW");
                }
                else if(locReliability == telux::loc::LocationReliability::LOW)
                {
                    LocationData->horiReliablity = TAF_LOCGNSS_RELIABILITY_LOW;
                    LE_DEBUG("horizontal reliablity is LOW");
                }
                else if(locReliability == telux::loc::LocationReliability::MEDIUM)
                {
                    LocationData->horiReliablity = TAF_LOCGNSS_RELIABILITY_MEDIUM;
                    LE_DEBUG("horizontal reliablity is MEDIUM");
                }
                else if(locReliability == telux::loc::LocationReliability::HIGH)
                {
                    LocationData->horiReliablity = TAF_LOCGNSS_RELIABILITY_HIGH;
                    LE_DEBUG("horizontal reliablity is HIGH");
                }
                else
                {
                    LocationData->horiReliablity = TAF_LOCGNSS_RELIABILITY_UNKNOWN;
                    LE_DEBUG("horizontal reliablity is UNKNOWN");
                }
                telux::loc::LocationReliability vertLocReliability =
                                                   locationInfo->getVerticalReliability();
                if(vertLocReliability == telux::loc::LocationReliability::NOT_SET)
                {
                    LocationData->vertReliablity = TAF_LOCGNSS_RELIABILITY_NOT_SET;
                    LE_DEBUG("vertical reliablity is NOT SET");
                }
                else if(vertLocReliability == telux::loc::LocationReliability::VERY_LOW)
                {
                    LocationData->vertReliablity = TAF_LOCGNSS_RELIABILITY_VERY_LOW;
                    LE_DEBUG("vertical reliablity is VERY LOW");
                }
                else if(vertLocReliability == telux::loc::LocationReliability::LOW)
                {
                    LocationData->vertReliablity = TAF_LOCGNSS_RELIABILITY_LOW;
                    LE_DEBUG("vertical reliablity is LOW");
                }
                else if(vertLocReliability == telux::loc::LocationReliability::MEDIUM)
                {
                    LocationData->vertReliablity = TAF_LOCGNSS_RELIABILITY_MEDIUM;
                    LE_DEBUG("vertical reliablity is MEDIUM");
                }
                else if(vertLocReliability == telux::loc::LocationReliability::HIGH)
                {
                    LocationData->vertReliablity = TAF_LOCGNSS_RELIABILITY_HIGH;
                    LE_DEBUG("vertical reliablity is HIGH");
                }
                else
                {
                    LocationData->vertReliablity = TAF_LOCGNSS_RELIABILITY_UNKNOWN;
                    LE_DEBUG("vertical reliablity is UNKNOWN");
                }
                LocationData->azimuth = locationInfo->getHorizontalUncertaintyAzimuth();
                LocationData->eastDev = locationInfo->getEastStandardDeviation();
                LocationData->northDev = locationInfo->getNorthStandardDeviation();
                LocationData->realTime = locationInfo->getElapsedRealTime();
                LocationData->realTimeUnc = locationInfo->getElapsedRealTimeUncertainty();
                telux::loc::LocationTechnology techMask = locationInfo->getTechMask();
                if((techMask & telux::loc::LOC_GNSS))
                {
                    LocationData->techMask |= TAF_LOCGNSS_LOC_GNSS;
                    LE_DEBUG("location calculated using GNSS");
                }
                if((techMask & telux::loc::LOC_CELL))
                {
                    LocationData->techMask |= TAF_LOCGNSS_LOC_CELL;
                    LE_DEBUG("location calculated using CELL");
                }
                if((techMask & telux::loc::LOC_WIFI))
                {
                    LocationData->techMask |= TAF_LOCGNSS_LOC_WIFI;
                    LE_DEBUG("location calculated using WIFI");
                }
                if((techMask & telux::loc::LOC_SENSORS))
                {
                    LocationData->techMask |= TAF_LOCGNSS_LOC_SENSORS;
                    LE_DEBUG("location calculated using SENSORS");
                }
                if((techMask & telux::loc::LOC_REFERENCE_LOCATION))
                {
                    LocationData->techMask |= TAF_LOCGNSS_LOC_REFERENCE_LOCATION;
                    LE_DEBUG("location calculated using Reference location");
                }
                if((techMask & telux::loc::LOC_INJECTED_COARSE_POSITION))
                {
                    LocationData->techMask |= TAF_LOCGNSS_LOC_INJECTED_COARSE_POSITION;
                    LE_DEBUG("location calculated using Coarse position injected");
                }
                if((techMask & telux::loc::LOC_AFLT))
                {
                    LocationData->techMask |= TAF_LOCGNSS_LOC_AFLT;
                    LE_DEBUG("location calculated using AFLT");
                }
                if((techMask & telux::loc::LOC_HYBRID))
                {
                    LocationData->techMask |= TAF_LOCGNSS_LOC_HYBRID;
                    LE_DEBUG("location calculated using GNSS and network-provided measurements");
                }
                if((techMask & telux::loc::LOC_PPE))
                {
                    LocationData->techMask |= TAF_LOCGNSS_LOC_PPE;
                    LE_DEBUG("location calculated using Precise position engine");
                }
                if((techMask & telux::loc::LOC_VEH))
                {
                    LocationData->techMask |= TAF_LOCGNSS_LOC_VEH;
                    LE_DEBUG("location calculated using Vehicular data");
                }
                if((techMask & telux::loc::LOC_VIS))
                {
                    LocationData->techMask |= TAF_LOCGNSS_LOC_VIS;
                    LE_DEBUG("location calculated using Visual data");
                }
                if((techMask & telux::loc::LOC_PROPAGATED))
                {
                    LocationData->techMask |= TAF_LOCGNSS_LOC_PROPAGATED;
                    LE_DEBUG("location calculated using Propagation logic");
                }

                for(i=0; i<TAF_LOCGNSS_NUMBER_OF_SIGNAL_TYPES_MAX; i++)
                {
                    LocationData->gnssData[i].gnssDataMask = clientRequestPtr->mGnssData[i].gnssDataMask;
                    LocationData->gnssData[i].jammerInd = clientRequestPtr->mGnssData[i].jammerInd;
                    LocationData->gnssData[i].agc = clientRequestPtr->mGnssData[i].agc;
                }
                LocationData->gPtpTime = locationInfo->getElapsedGptpTime();
                LocationData->gPtpTimeUnc = locationInfo->getElapsedGptpTimeUnc();
                LocationData->next = LE_DLS_LINK_INIT;

                le_event_ReportWithRefCounting(gnss.positionEventId, LocationData);
                clientRequestPtr->mSvEnabled = false;
                clientRequestPtr->mGnssSigEnabled = false;
            }
        }
    }
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);
}

void tafLocationListener::onGnssSVInfo(const std::shared_ptr<telux::loc::IGnssSVInfo> &gnssSVInfo) {
    auto &gnss = taf_locGnss::GetInstance();

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = gnss.DiscoverSessionRef(*clientSessionRef);

    if (NULL == clientRequestPtr) {
        LE_DEBUG("onGnssSVInfo did not find sessionRef: %p", *clientSessionRef);
        return;
    }

    //To get the constellation
    std::unique_lock<std::mutex> lock(clientRequestPtr->mMutex);
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
    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);
    if(gnss.NumOfPositionHandlers ) {
        LE_DEBUG("**** Satellite Vehicle Information ****");
        int i = 0;
        clientRequestPtr->mSatParams.satsInViewCount = gnssSVInfo->getSVInfoList().size();
        clientRequestPtr->mTotalSVTracked = 0;
        memset(&clientRequestPtr->mSatInfo, 0, sizeof(clientRequestPtr->mSatInfo));
        for(auto svInfo : gnssSVInfo->getSVInfoList()) {

            if(i >= TAF_LOCGNSS_SV_INFO_MAX_LEN)
            {
                LE_WARN("SvInfo overflows");
                continue;
            }

            switch(svInfo->getConstellation()) {
                case telux::loc::GnssConstellationType::GPS:
                    clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_GPS;
                    break;
                case telux::loc::GnssConstellationType::GLONASS:
                    clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_GLONASS;
                    break;
                case telux::loc::GnssConstellationType::BDS:
                    clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_BEIDOU;
                    break;
                case telux::loc::GnssConstellationType::GALILEO:
                    clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_GALILEO;
                    break;
                case telux::loc::GnssConstellationType::SBAS:
                    clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_SBAS;
                    break;
                case telux::loc::GnssConstellationType::QZSS:
                    clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_QZSS;
                    break;
                case telux::loc::GnssConstellationType::NAVIC:
                    clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_NAVIC;
                    break;
                default:
                    clientRequestPtr->mSatInfo[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_UNDEFINED;
                    LE_ERROR("Constellation type: UNKNOWN");
                    break;
            }

            clientRequestPtr->mSatInfo[i].satId = svInfo->getId();
            if(svInfo->getHasFix() == SVInfoAvailability::YES)
            {
                clientRequestPtr->mSatInfo[i].satUsed = true;
                LE_DEBUG("onGnssSVInfo: svInfo->getHasFix()->SVInfoAvailability::YES");
            }
            else
            {
                clientRequestPtr->mSatInfo[i].satUsed = false;
            }
            if(svInfo->getSnr()!=0)
            {
                clientRequestPtr->mTotalSVTracked++;
                clientRequestPtr->mSatInfo[i].satTracked = true;
                LE_DEBUG("onGnssSVInfo: svInfo->getSnr() is NON ZERO");
            }
            else
            {
            clientRequestPtr->mSatInfo[i].satTracked = false;
            }
            clientRequestPtr->mSatInfo[i].satSnr = svInfo->getSnr();
            clientRequestPtr->mSatInfo[i].satAzim = svInfo->getAzimuth();
            clientRequestPtr->mSatInfo[i].satElev = svInfo->getElevation();
            clientRequestPtr->mSatInfo[i].signalType = (uint32_t) svInfo->getSignalType();
            clientRequestPtr->mSatInfo[i].glonassFcn = svInfo->getGlonassFcn();
            clientRequestPtr->mSatInfo[i].baseBandCnr = svInfo->getBasebandCnr();
            i++;
        }
        clientRequestPtr->mSvEnabled = true;
        clientRequestPtr->mSatParams.satsTrackingCount = clientRequestPtr->mTotalSVTracked;
    }
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);
}

void tafLocationListener::onGnssSignalInfo(
        const std::shared_ptr<telux::loc::IGnssSignalInfo> &gnssDatainfo) {
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = gnss.DiscoverSessionRef(*clientSessionRef);

    if (NULL == clientRequestPtr) {
        LE_DEBUG("onGnssSignalInfo did not find sessionRef: %p", *clientSessionRef);
        return;
    }
    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);
    if(gnss.NumOfPositionHandlers ) {
        LE_DEBUG("**** Gnss Signal Information ****" );
        for(int sig = 0; sig < TAF_LOCGNSS_NUMBER_OF_SIGNAL_TYPES_MAX; sig++)
        {
            LE_DEBUG("onGnssSignalInfo Signal Type : %d",sig);
            LE_DEBUG("onGnssSignalInfo gnssDataMask: %d",gnssDatainfo->getGnssData().gnssDataMask[sig]);
            if(telux::loc::GnssDataValidityType::HAS_JAMMER == ((gnssDatainfo->getGnssData().gnssDataMask[sig])
            & (telux::loc::GnssDataValidityType::HAS_JAMMER)))
            {
                LE_DEBUG("onGnssSignalInfo jammerInd: %lf",gnssDatainfo->getGnssData().jammerInd[sig]);
                clientRequestPtr->mGnssData[sig].gnssDataMask |= TAF_LOCGNSS_HAS_JAMMER;
                clientRequestPtr->mGnssData[sig].jammerInd = gnssDatainfo->getGnssData().jammerInd[sig];
            }
            else
            {
                LE_DEBUG("onGnssSignalInfo JAMMER Ind Not Present ");
            }
            if(telux::loc::GnssDataValidityType::HAS_AGC == ((gnssDatainfo->getGnssData().gnssDataMask[sig])
             & (telux::loc::GnssDataValidityType::HAS_AGC)))
            {
                LE_DEBUG("onGnssSignalInfo agc: %lf",gnssDatainfo->getGnssData().agc[sig]);
                clientRequestPtr->mGnssData[sig].gnssDataMask |= TAF_LOCGNSS_HAS_AGC;
                clientRequestPtr->mGnssData[sig].agc = gnssDatainfo->getGnssData().agc[sig];
            }
            else
            {
                LE_DEBUG("onGnssSignalInfo AGC Not Present");
            }
        }
        clientRequestPtr->mGnssSigEnabled = true;
    }
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);
}

void tafLocationListener::onCapabilitiesInfo(
        const telux::loc::LocCapability capabilityInfo) {
    auto &gnss = taf_locGnss::GetInstance();
    LE_DEBUG("onCapabilitiesInfo: The capabilityInfo is %d for *sessionRef: %p", (int) capabilityInfo, *clientSessionRef);
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = gnss.DiscoverSessionRef(*clientSessionRef);

    if (NULL == clientRequestPtr) {
        LE_DEBUG("onCapabilitiesInfo did not find sessionRef: %p", *clientSessionRef);
        return;
    }

    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);
    if(gnss.NumOfCapabilityHandlers) {
        LE_DEBUG( "**** Gnss Capabilities Information ****" );
        CapabilityChangeEvent_t capabilityEvent;
        capabilityEvent.locCapability = (taf_locGnss_LocCapabilityType_t) capabilityInfo;
        le_event_Report(gnss.locCapabilityEventId, &capabilityEvent, sizeof(CapabilityChangeEvent_t));
    }
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);
}

void tafLocationListener::onGnssNmeaInfo(uint64_t timestamp, const std::string &nmea) {
    auto &gnss = taf_locGnss::GetInstance();
    LE_DEBUG("onGnssNmeaInfo for *sessionRef: %p", *clientSessionRef);
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = gnss.DiscoverSessionRef(*clientSessionRef);

    if (NULL == clientRequestPtr) {
        LE_DEBUG("onGnssNmeaInfo did not find sessionRef: %p", *clientSessionRef);
        return;
    }
    //Format : $GPGGA,075446.90,00-0.000000,S,00000.000000,E,1,00,1.0,936.4,M,-936.4,M,,*7D^M
    gnss.mNmeaBitMask = nmea;
    LE_DEBUG( "**** Gnss Nmea Information  gnss.mNmeaBitMask: %s****",gnss.mNmeaBitMask.c_str());
    std::unique_lock<std::mutex> lock(clientRequestPtr->mMutex);


    gnss.mNmeaVar.notify_one();
    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);
    LE_DEBUG("onGnssNmeaInfo: NumOfNmeaHandlers = %d", gnss.NumOfNmeaHandlers);
    if(gnss.NumOfNmeaHandlers) {
        LE_DEBUG( "**** Gnss Nmea Information ****" );
        //clientRequestPtr->mSatMeas.satId = nmea;
        clientRequestPtr->mSatMeas.satLatency = timestamp;

        NmeaInfoEvent_t nmeaEvent;
        nmeaEvent.timestamp = timestamp;
        const int length = gnss.mNmeaBitMask.length();
        nmeaEvent.nmeaMask[length] ='\0';
        for (int i = 0; i < length; i++)
        {
            nmeaEvent.nmeaMask[i] = gnss.mNmeaBitMask.c_str()[i];
        }
        LE_DEBUG( "**** NMEA handler string copied is: %s****",nmeaEvent.nmeaMask);
        le_event_Report(gnss.nmeaEventId, &nmeaEvent, sizeof(nmeaEvent));
    }
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);
}

void tafLocationListener::onGnssMeasurementsInfo(const telux::loc::
        GnssMeasurements &measurementInfo) {
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = gnss.DiscoverSessionRef(*clientSessionRef);

    if (NULL == clientRequestPtr) {
        LE_DEBUG("onGnssMeasurementsInfo did not find sessionRef: %p", *clientSessionRef);
        return;
    }
    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);
    if(gnss.NumOfPositionHandlers ) {
        LE_DEBUG("**** Gnss Measurements Information ****");
    }
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);
}

void tafLocationListener::onLocationSystemInfo(const telux::loc::LocationSystemInfo
        &locationSystemInfo) {
    auto &gnss = taf_locGnss::GetInstance();

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = gnss.DiscoverSessionRef(*clientSessionRef);

    if (NULL == clientRequestPtr) {
        LE_DEBUG("onLocationSystemInfo did not find sessionRef: %p", *clientSessionRef);
        return;
    }

    le_mutex_Lock(clientRequestPtr->mGnssMutexRef);

    LE_DEBUG( "**** Location System Information ****" );
    LE_DEBUG( "**** Location System Information locationSystemInfoValidity:%d",locationSystemInfo.valid);
    LE_DEBUG( "**** Location System Information LeapSecondInfoValidity:%d",locationSystemInfo.info.valid);
    telux::loc::LocationSystemInfoValidity locationSystemInfoMask = locationSystemInfo.valid;
    telux::loc::LeapSecondInfoValidity leapSecondSysInfoMask = locationSystemInfo.info.valid;
    if(locationSystemInfoMask & telux::loc::LOCATION_SYS_INFO_LEAP_SECOND)
    {
        LE_DEBUG(" onLocationSystemInfo: contains current leap second or leap second change info");
        clientRequestPtr->mCurrentLeapSeconds = locationSystemInfo.info.current;
        clientRequestPtr->mCurrentLeapSeconds = clientRequestPtr->mCurrentLeapSeconds * 1000;//millisseoncds
        LE_DEBUG("onLocationSystemInfo gnss.mCurrentLeapSeconds: %d",clientRequestPtr->mCurrentLeapSeconds);
    }
    if(leapSecondSysInfoMask & telux::loc::LEAP_SECOND_SYS_INFO_CURRENT_LEAP_SECONDS_BIT)
    {
        LE_DEBUG("onLocationSystemInfo: current leap second info is available.");
    }
    if(leapSecondSysInfoMask & telux::loc::LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT)
    {
        LE_DEBUG(" The last known leap change event is available.");
    }
    telux::loc::TimeInfo timeInfo = locationSystemInfo.info.info.timeInfo;
    LE_DEBUG("onLocationSystemInfo System time week : %u",(unsigned)timeInfo.systemWeek);
    LE_DEBUG("onLocationSystemInfo System time week ms: : %u",(unsigned)timeInfo.systemMsec);
    LE_DEBUG("onLocationSystemInfo LeapSecondCurrent : %d", unsigned(locationSystemInfo.info.current));
    clientRequestPtr->mGpsTime = (uint64_t)(timeInfo.systemWeek  * 7);//7 days
    clientRequestPtr->mGpsTime = (uint64_t) clientRequestPtr->mGpsTime * 24;//24 hours
    clientRequestPtr->mGpsTime = (uint64_t) clientRequestPtr->mGpsTime* 3600;//seconds
    clientRequestPtr->mGpsTime = (uint64_t) clientRequestPtr->mGpsTime* 1000; //milli seconds
    clientRequestPtr->mGpsTime = (uint64_t) clientRequestPtr->mGpsTime + (unsigned)timeInfo.systemMsec;
    LE_DEBUG("onLocationSystemInfo overall clientRequestPtr->mGpsTime : %" PRIi64 "",clientRequestPtr->mGpsTime);
    LE_DEBUG("onLocationSystemInfo System clk time : %lf",timeInfo.systemClkTimeBias);
    LE_DEBUG("onLocationSystemInfo System clk time uncertainty valid: %lf",timeInfo.systemClkTimeUncMs);
    LE_DEBUG("onLocationSystemInfo System reference valid: %u",(unsigned)timeInfo.refFCount);
    LE_DEBUG("onLocationSystemInfo System num clock reset valid: %u",(unsigned)timeInfo.numClockResets);
    LE_DEBUG("onLocationSystemInfo leapSecondsBeforeChange : %u",
            unsigned(locationSystemInfo.info.info.leapSecondsBeforeChange));
    LE_DEBUG("onLocationSystemInfo leapSecondsAfterChange :%u",
           unsigned(locationSystemInfo.info.info.leapSecondsAfterChange));
    clientRequestPtr->mChangeEventTime = locationSystemInfo.info.info.leapSecondsBeforeChange*1000;//msec
    clientRequestPtr->mNextLeapSeconds = locationSystemInfo.info.info.leapSecondsAfterChange*1000;//msec
    le_mutex_Unlock(clientRequestPtr->mGnssMutexRef);
}

void tafLocationListener::onXtraStatusUpdate(const telux::loc::XtraStatus xtraStatus)
{
    LE_DEBUG("********** Xtra Status Info **********" );
    LE_DEBUG("Xtra Feature Enabled: %d", xtraStatus.featureEnabled);
    LE_DEBUG("Xtra Feature Validity: %d",xtraStatus.xtraValidForHours);
    switch(xtraStatus.xtraDataStatus)
    {
        case telux::loc::XtraDataStatus::STATUS_UNKNOWN:
            LE_DEBUG("Unknown");
            break;
        case telux::loc::XtraDataStatus::STATUS_NOT_AVAIL:
            LE_DEBUG("Not available");
            break;
        case telux::loc::XtraDataStatus::STATUS_NOT_VALID:
            LE_DEBUG("Invalid");
            break;
        case telux::loc::XtraDataStatus::STATUS_VALID:
            LE_DEBUG("Valid");
            break;
    }
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
    auto &gnss = taf_locGnss::GetInstance();
    telux::common::Status status = telux::common::Status::FAILED;
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
    memset(&clientRequestPtr->mSatInfo, 0, sizeof(clientRequestPtr->mSatInfo));
    memset(&clientRequestPtr->mGnssData,0, sizeof(clientRequestPtr->mGnssData));
    clientRequestPtr->mGnssMutexRef = le_mutex_CreateRecursive("GnssMutexCl");
    clientRequestPtr->locationManager = nullptr;
    status = gnss.LocationManagerInit(clientRequestPtr);
    if (status != telux::common::Status::SUCCESS) {
        LE_FATAL("LocationManager for sessionRef %p is not available", clientRequestPtr);
    }

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

        InitializeClient(clientRequestPtr);

        void* reqRefPtr = le_ref_CreateRef(gnss.ClientRequestRefMap, clientRequestPtr);
        if (sessionRef!=nullptr) {
            //External client increase Client ref count
            gnss.mClientRefCount++;
        }

        LE_DEBUG("SessionRef %p was not found, Create Client session, total count %d", sessionRef, gnss.mClientRefCount);
        LE_DEBUG("reqRefPtr %p, clientRequestPtr %p", reqRefPtr, clientRequestPtr);

        clientRequestPtr->clientRefPtr = reqRefPtr;
    }

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

le_result_t taf_locGnss::SetConstellation
(
    taf_locGnss_ConstellationBitMask_t constellationMask
)
{

    le_result_t result = LE_FAULT;
    typedef std::vector<telux::loc::SvBlackListInfo> SvBlackList;
    SvBlackList svBlackList;
    telux::loc::SvBlackListInfo blackListInfo;
    bool deviceReset = false;
    blackListInfo.svId = 0; // Here 0 means blacklist all SVIds of a given constellation type
    blackListInfo.constellation = telux::loc::GnssConstellationType::UNKNOWN;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    LE_DEBUG("SetConstellation constellationMask is 0x%02X",constellationMask);
    if( constellationMask & TAF_LOCGNSS_CONSTELLATION_DEFAULT)
    {
        LE_DEBUG("constellation type is Default");
        deviceReset = true;
        constellationMask -= TAF_LOCGNSS_CONSTELLATION_DEFAULT;
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_GPS)
        {
            deviceReset = false;
            blackListInfo.constellation = telux::loc::GnssConstellationType::GPS;
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is GPS");
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_GLONASS)
        {
            deviceReset = false;
            blackListInfo.constellation = telux::loc::GnssConstellationType::GLONASS;
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is GLONASS");
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_BEIDOU)
        {
            deviceReset = false;
            blackListInfo.constellation = telux::loc::GnssConstellationType::BDS;
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is BEIDOU");
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_GALILEO)
        {
            deviceReset = false;
            blackListInfo.constellation = telux::loc::GnssConstellationType::GALILEO;
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is GALILEO");
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_SBAS)
        {
            deviceReset = false;
            blackListInfo.constellation = telux::loc::GnssConstellationType::SBAS;
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is SBAS");
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_QZSS)
        {
            deviceReset = false;
            blackListInfo.constellation = telux::loc::GnssConstellationType::QZSS;
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is QZSS");
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_NAVIC)
        {
            deviceReset = false;
            blackListInfo.constellation = telux::loc::GnssConstellationType::NAVIC;
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is NAVIC");
        }
        if (blackListInfo.constellation == telux::loc::GnssConstellationType::UNKNOWN)
        {
            LE_DEBUG("constellation type is UNKNOWN");
        }
    }
    else
    {
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_GPS)
        {
            blackListInfo.constellation = telux::loc::GnssConstellationType::GPS;
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is GPS");
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_GLONASS)
        {
            blackListInfo.constellation = telux::loc::GnssConstellationType::GLONASS;
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is GLONASS");
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_BEIDOU)
        {
            blackListInfo.constellation = telux::loc::GnssConstellationType::BDS;
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is BEIDOU");
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_GALILEO)
        {
            blackListInfo.constellation = telux::loc::GnssConstellationType::GALILEO;
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is GALILEO");
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_SBAS)
        {
            blackListInfo.constellation = telux::loc::GnssConstellationType::SBAS;
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is SBAS");
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_QZSS)
        {
            blackListInfo.constellation = telux::loc::GnssConstellationType::QZSS;
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is QZSS");
        }
        if( constellationMask & TAF_LOCGNSS_CONSTELLATION_NAVIC)
        {
            blackListInfo.constellation = telux::loc::GnssConstellationType::NAVIC;
            svBlackList.push_back(blackListInfo);
            LE_DEBUG("constellation type is NAVIC");
        }
        if (blackListInfo.constellation == telux::loc::GnssConstellationType::UNKNOWN)
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
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_ACTIVE:
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
                LE_DEBUG("SetConstellation is failed");
            }
            else
            {
                if(p.get_future().get() == LE_OK)
                {
                    result = LE_OK;
                    mConstellationMask = constellationMask;
                    LE_DEBUG("SetConstellation is success ");
                }
                else
                {
                    result = LE_FAULT;
                    LE_DEBUG("SetConstellation is failed");
                }
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

le_result_t taf_locGnss::Start
(
    void
)
{
    le_result_t result = LE_FAULT;

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    LE_DEBUG("Start: gnssClientPtr %p, gnssPtr->sessionRef %p, num of active client %d",
            clientRequestPtr, clientRequestPtr->sessionRef, mClientRefCount);

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

                std::promise<le_result_t> p;
                auto cb = [&p](telux::common::ErrorCode error) {
                    if(error == telux::common::ErrorCode::SUCCESS) {
                        p.set_value(LE_OK);
                    }
                    else {
                        p.set_value(LE_FAULT);
                    }
                };
                auto status = clientRequestPtr->locationManager->startDetailedEngineReports(
                        (uint32_t)optInterval, engineType, cb, reportMask);
                if(telux::common::Status::SUCCESS != status)
                {
                    result = LE_FAULT;
                    LE_DEBUG("Start() is failed");
                    LE_DEBUG("Start() commandResponse failed status: %d ", int(status));
                }
                else
                {
                    if(p.get_future().get() == LE_OK)
                    {
                        ConfigureAcqStartInfo(clientRequestPtr);
                        result = LE_OK;
                        LE_DEBUG("Start() is success");
                    }
                    else
                    {
                        result = LE_FAULT;
                        LE_DEBUG("Start() is failed");
                        LE_DEBUG("Start() commandResponse failed status: %d ", int(status));
                    }
                }
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
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_DUPLICATE;
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
    taf_locGnss_PositionHandler_t*  positionHandlerPtr =
        (taf_locGnss_PositionHandler_t*)le_mem_ForceAlloc(PositionHandlerPoolRef);
    memset(positionHandlerPtr, 0, sizeof(taf_locGnss_PositionHandler_t));
    positionHandlerPtr->next = LE_DLS_LINK_INIT;
    positionHandlerPtr->handlerFuncPtr = handlerPtr;
    positionHandlerPtr->handlerContextPtr = contextPtr;
    positionHandlerPtr->sessionRef = taf_locGnss_GetClientSessionRef();

    LE_DEBUG("AddPositionHandler() sessionRef: %p", positionHandlerPtr->sessionRef);

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, NULL, "clientRequestPtr is NULL");

    positionHandlerPtr->handlerRef =
        (taf_locGnss_PositionHandlerRef_t)le_ref_CreateRef(clientRequestPtr->PositionHandlerRefMap, positionHandlerPtr);

    NumOfPositionHandlers++;

    LE_DEBUG("Created positionHandlerRef(%p) for positionHandlerPtr(%p) (totalCnt=0x%x).",
        positionHandlerPtr->handlerRef, positionHandlerPtr, NumOfPositionHandlers);

    return positionHandlerPtr->handlerRef;
}

taf_locGnss_CapabilityChangeHandlerRef_t taf_locGnss::AddCapabilityHandler
(
    taf_locGnss_CapabilityChangeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    le_event_HandlerRef_t handlerRef;

    TAF_KILL_CLIENT_IF_RET_VAL(handlerPtr == NULL, NULL, "Handler pointer is NULL");

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, NULL, "AddCapabilityHandler: clientRequestPtr is NULL");

    handlerRef = le_event_AddLayeredHandler("CapabilityHandler", locCapabilityEventId,
            FirstLayerCapabilityHandler, (void*)handlerPtr);

    NumOfCapabilityHandlers++;

    return (taf_locGnss_CapabilityChangeHandlerRef_t) handlerRef;
}

void taf_locGnss::RemoveCapabilityHandler (taf_locGnss_CapabilityChangeHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    if (NumOfCapabilityHandlers > 0) {
        NumOfCapabilityHandlers--;
    }
}

void taf_locGnss::FirstLayerCapabilityHandler(void* reportPtr,
        void* secondLayerHandlerFunc)
{
    CapabilityChangeEvent_t* capEventPtr = (CapabilityChangeEvent_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(capEventPtr == NULL,"CapabilityChangeEventPtr is NULL");

    taf_locGnss_CapabilityChangeHandlerFunc_t clientHandlerFunc =
        (taf_locGnss_CapabilityChangeHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(capEventPtr->locCapability, le_event_GetContextPtr());
}

taf_locGnss_NmeaHandlerRef_t taf_locGnss::AddNmeaHandler
(
    taf_locGnss_NmeaHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    le_event_HandlerRef_t handlerRef;

    TAF_KILL_CLIENT_IF_RET_VAL(handlerPtr == NULL, NULL, "Handler pointer is NULL");

    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();
    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, NULL, "AddCapabilityHandler: clientRequestPtr is NULL");

    handlerRef = le_event_AddLayeredHandler("NmeaHandler", nmeaEventId,
            FirstLayerNmeaHandler, (void*)handlerPtr);

    NumOfNmeaHandlers++;

    return (taf_locGnss_NmeaHandlerRef_t) handlerRef;
}

void taf_locGnss::RemoveNmeaHandler (taf_locGnss_NmeaHandlerRef_t handlerRef)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    if (NumOfNmeaHandlers > 0) {
        NumOfNmeaHandlers--;
    }
}

void taf_locGnss::FirstLayerNmeaHandler(void* reportPtr,
        void* secondLayerHandlerFunc)
{
    NmeaInfoEvent_t* nmeaEventPtr = (NmeaInfoEvent_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL(nmeaEventPtr == NULL,"NmeaEventPtr is NULL");

    taf_locGnss_NmeaHandlerFunc_t clientHandlerFunc =
        (taf_locGnss_NmeaHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(nmeaEventPtr->timestamp, nmeaEventPtr->nmeaMask, le_event_GetContextPtr());
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
                    LE_DEBUG("constellation type is TAF_LOCGNSS_CONSTELLATION_GPS");
                }
                if(mConstellationMask & TAF_LOCGNSS_CONSTELLATION_GLONASS)
                {
                    *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_GLONASS;
                    result = LE_OK;
                    LE_DEBUG("constellation type is TAF_LOCGNSS_CONSTELLATION_GLONASS");
                }
                if(mConstellationMask & TAF_LOCGNSS_CONSTELLATION_BEIDOU)
                {
                    *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_BEIDOU;
                    result = LE_OK;
                    LE_DEBUG("constellation type is TAF_LOCGNSS_CONSTELLATION_BEIDOU");
                }
                if(mConstellationMask & TAF_LOCGNSS_CONSTELLATION_GALILEO)
                {
                    *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_GALILEO;
                    result = LE_OK;
                    LE_DEBUG("constellation type is TAF_LOCGNSS_CONSTELLATION_GALILEO");
                }
                if(mConstellationMask & TAF_LOCGNSS_CONSTELLATION_SBAS)
                {
                    *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_SBAS;
                    result = LE_OK;
                    LE_DEBUG("constellation type is TAF_LOCGNSS_CONSTELLATION_SBAS");
                }
                if(mConstellationMask & TAF_LOCGNSS_CONSTELLATION_QZSS)
                {
                    *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_QZSS;
                    result = LE_OK;
                    LE_DEBUG("constellation type is TAF_LOCGNSS_CONSTELLATION_QZSS");
                }
                if(mConstellationMask & TAF_LOCGNSS_CONSTELLATION_NAVIC)
                {
                    *constellationMaskPtr |= TAF_LOCGNSS_CONSTELLATION_NAVIC;
                    result = LE_OK;
                    LE_DEBUG("constellation type is TAF_LOCGNSS_CONSTELLATION_NAVIC");
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
    LE_DEBUG("GetPositionState reportStatus:%d",
                  posSampleReqPtr->positionSampleNodePtr->reportStatus);
    LE_DEBUG("GetPositionState techmask :%d",posSampleReqPtr->positionSampleNodePtr->techMask);
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
            LE_DEBUG("GetPositionState HAS_POS_TECH_MASK is set");
            if(posSampleReqPtr->positionSampleNodePtr->techMask & TAF_LOCGNSS_LOC_GNSS) //0x01
            {
                LE_DEBUG("GetPositionState tech mask includes GNSS ");
                if(posSampleReqPtr->positionSampleNodePtr->validityMask
                                                       & TAF_LOCGNSS_HAS_ALTITUDE_BIT)
                {
                    posSampleReqPtr->positionSampleNodePtr->fixState = TAF_LOCGNSS_STATE_FIX_3D;
                    LE_DEBUG("GetPositionState FixState is 3D");
                }
                else
                {
                    if(posSampleReqPtr->positionSampleNodePtr->validityMask & HAS_LAT_LONG_BIT)
                    {
                        posSampleReqPtr->positionSampleNodePtr->fixState = TAF_LOCGNSS_STATE_FIX_2D;
                        LE_DEBUG("GetPositionState FixState is 2D");
                    }
                    else
                    {
                        posSampleReqPtr->positionSampleNodePtr->fixState =
                                                                TAF_LOCGNSS_STATE_FIX_NO_POS;
                        LE_DEBUG("GetPositionState HAS_LAT_LONG_BIT is not set, returning No Fix");
                    }
                }
            }
            else
            {
                if((posSampleReqPtr->positionSampleNodePtr->techMask == TAF_LOCGNSS_LOC_SENSORS)
                ||(posSampleReqPtr->positionSampleNodePtr->techMask == TAF_LOCGNSS_STATE_FIX_ESTIMATED))
                {
                    LE_DEBUG("GetPositionState tech mask is SENSORS/ESTIMATED,returning estimated");
                    posSampleReqPtr->positionSampleNodePtr->fixState =
                                                                  TAF_LOCGNSS_STATE_FIX_ESTIMATED;
                }
                else
                {
                    posSampleReqPtr->positionSampleNodePtr->fixState = TAF_LOCGNSS_STATE_FIX_NO_POS;
                    LE_DEBUG("GetPositionState technmask is not estimated, so returning No Fix");
                }
            }
        }
        else
        {
            posSampleReqPtr->positionSampleNodePtr->fixState = TAF_LOCGNSS_STATE_FIX_NO_POS;
            LE_DEBUG("GetPositionState HAS_POS_TECH_MASK is not set, so returning No Fix");
        }
    }
    else //if report status is failure or intermediate
    {
        posSampleReqPtr->positionSampleNodePtr->fixState = TAF_LOCGNSS_STATE_FIX_NO_POS;
        LE_DEBUG("GetPositionState report status is failure, so returning No Fix");
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

le_result_t taf_locGnss::DeleteDRSensorCalData
(
    void
)
{
    le_result_t result = LE_OK;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss state [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
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
                   AidingData |1UL << 1 which is 2*/

                uint32_t AidingData = TAF_LOCGNSS_AIDING_DATA_DR_SENSOR_CALIBRATION;
                telux::common::Status status =
                                            mLocationConfigurator->deleteAidingData(AidingData,cb1);
                if (status != telux::common::Status::SUCCESS)
                {
                    if (status == telux::common::Status::NOTIMPLEMENTED)
                    {
                        LE_ERROR("DeleteDRSensorCalData failed or Not Implemented");
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
                LE_DEBUG("GetTimeAccuracy: LE_OUT_OF_RANGE");
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

le_result_t taf_locGnss::ForceColdRestart
(
    void
)
{
    le_result_t result = LE_OK;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_ACTIVE:
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

                LE_DEBUG("ForceColdRestart mStarted: %d", clientRequestPtr->mStarted);

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
                    if (clientRequestPtr->mStarted)
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
                        auto status = clientRequestPtr->locationManager->stopReports(cb2);
                        if(status != telux::common::Status::SUCCESS)
                        {
                            result = LE_FAULT;
                        }
                        else
                        {
                            std::future<le_result_t> futResult = p2.get_future();
                            if(futResult.get() == LE_OK)
                            {
                                clientRequestPtr->mStarted = false;
                                clientRequestPtr->GnssState = TAF_LOCGNSS_STATE_READY;
                                LE_DEBUG("ForceColdRestart->Stop() is success");
                            }
                            else
                            {
                                LE_DEBUG("ForceColdRestart->Stop() is failed");
                                result = LE_FAULT;
                            }
                        }
                    }
                }
                if(result == LE_OK)
                {
                    //start Detailed Engine report
                    if (!clientRequestPtr->mStarted) {
                        int optInterval = clientRequestPtr->mAcqRate;
                        LE_DEBUG("ForceColdRestart->  optInterval: %d",optInterval);
                        if( optInterval == 0  || optInterval < 100) {
                            LE_DEBUG("ForceColdRestart mAcqRate is zero, so set default to 100ms");
                            optInterval = 100;
                            clientRequestPtr->mAcqRate = optInterval;
                        }

                        LocReqEngine engineType = DEFAULT_UNKNOWN;
                        GnssReportTypeMask reportMask = DEFAULT_UNKNOWN;
                        reportMask = 0x7f;//all reports are enabled
                        LE_DEBUG("ForceColdRestart->reportMask : %u",reportMask);
                        engineType |= (1UL << clientRequestPtr->mEngineType);

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
                        auto status = clientRequestPtr->locationManager->startDetailedEngineReports(
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
                                ConfigureAcqStartInfo(clientRequestPtr);
                                LE_DEBUG("ForceColdRestart->Start() is success");
                            }
                            else
                            {
                                LE_DEBUG("ForceColdRestart->Start() is failed");
                                result = LE_FAULT;
                            }
                        }
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

le_result_t taf_locGnss::ForceWarmRestart
(
    void
)
{
    le_result_t result = LE_OK;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss state [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_ACTIVE:
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
                   AidingData |1UL << 0 which is 1*/

                uint32_t AidingData = TAF_LOCGNSS_AIDING_DATA_EPHEMERIS;
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
                    if (clientRequestPtr->mStarted)
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

                        status = clientRequestPtr->locationManager->stopReports(cb2);
                        if(status != telux::common::Status::SUCCESS)
                        {
                            result = LE_FAULT;
                        }
                        else
                        {
                            std::future<le_result_t> futResult = p2.get_future();
                            if(futResult.get() == LE_OK)
                            {
                                clientRequestPtr->mStarted = false;
                                clientRequestPtr->GnssState = TAF_LOCGNSS_STATE_READY;
                                LE_DEBUG("ForceWarmRestart->Stop() is success");
                            }
                            else
                            {
                                LE_DEBUG("ForceWarmRestart->Stop() is failed");
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
                    if (!clientRequestPtr->mStarted) {
                        int optInterval = clientRequestPtr->mAcqRate;
                        std::promise<le_result_t> p;
                        auto cb = [&p](telux::common::ErrorCode error) {
                            if(error == telux::common::ErrorCode::SUCCESS) {
                                p.set_value(LE_OK);
                            }
                            else {
                                p.set_value(LE_FAULT);
                            }
                        };
                        LE_DEBUG("ForceWarmRestart->  optInterval: %d",optInterval);
                        if( optInterval == 0  || optInterval < 100) {
                            LE_DEBUG("ForceWarmRestart mAcqRate is zero, so set default to 100ms");
                            optInterval = 100;
                            clientRequestPtr->mAcqRate = optInterval;
                        }

                        LocReqEngine engineType = DEFAULT_UNKNOWN;
                        GnssReportTypeMask reportMask = DEFAULT_UNKNOWN;
                        reportMask = 0x7f;//all reports are enabled
                        LE_DEBUG("ForceWarmRestart->reportMask : %u",reportMask);
                        engineType |= (1UL << clientRequestPtr->mEngineType);
                        LE_DEBUG("ForceWarmRestart ->startDetailedEngineReports()");
                        status = clientRequestPtr->locationManager->startDetailedEngineReports((uint32_t)optInterval,
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
                                ConfigureAcqStartInfo(clientRequestPtr);
                                LE_DEBUG("ForceWarmRestart->Start() is success");
                            }
                            else
                            {
                                LE_DEBUG("ForceWarmRestart->Start() is failed");
                                result = LE_FAULT;
                            }
                        }
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

le_result_t taf_locGnss::ForceHotRestart
(
    void
)
{
    le_result_t result = LE_OK;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Wrong Gnss State [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_ACTIVE:
            {
                // stop Detailed Reports
                if (clientRequestPtr->mStarted) {
                    std::promise<le_result_t> p;
                    auto cb = [&p](telux::common::ErrorCode error) {
                        if(error == telux::common::ErrorCode::SUCCESS) {
                            p.set_value(LE_OK);
                        }
                        else {
                            p.set_value(LE_FAULT);
                        }
                    };
                    auto status = clientRequestPtr->locationManager->stopReports(cb);
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
                            clientRequestPtr->mStarted = false;
                            clientRequestPtr->GnssState = TAF_LOCGNSS_STATE_READY;
                            LE_DEBUG("ForceHotRestart->Stop() is success");
                        }
                        else
                        {
                            LE_DEBUG("ForceHotRestart->Stop() is failed");
                            result = LE_FAULT;
                            return result;
                        }
                    }
                }
                sleep(5); // 5sec sleep required to stop and start Gnss engine

                //start Detailed report
                if (!clientRequestPtr->mStarted) {
                    int optInterval = clientRequestPtr->mAcqRate;
                    std::promise<le_result_t> p;
                    auto cb = [&p](telux::common::ErrorCode error) {
                        if(error == telux::common::ErrorCode::SUCCESS) {
                            p.set_value(LE_OK);
                        }
                        else {
                            p.set_value(LE_FAULT);
                        }
                    };
                    LE_DEBUG("ForceHotRestart->  optInterval: %d",optInterval);
                    if( optInterval == 0  || optInterval < 100) {
                        LE_DEBUG("ForceHotRestart()->mAcqRate is zero, so set default to 100ms");
                        optInterval = 100;
                        clientRequestPtr->mAcqRate = optInterval;
                    }
                    LocReqEngine engineType = DEFAULT_UNKNOWN;
                    GnssReportTypeMask reportMask = DEFAULT_UNKNOWN;
                    reportMask = 0x7f;//all reports are enabled
                    LE_DEBUG("ForceHotRestart->reportMask : %u",reportMask);
                    engineType |= (1UL << clientRequestPtr->mEngineType);
                    auto status = clientRequestPtr->locationManager->startDetailedEngineReports((uint32_t)optInterval,
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
                            ConfigureAcqStartInfo(clientRequestPtr);
                            LE_DEBUG("ForceHotRestart->Start() is success");
                        }
                        else
                        {
                            result = LE_FAULT;
                            LE_DEBUG("ForceHotRestart->Stop() is failed");
                        }
                    }
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

le_result_t taf_locGnss::SetMinElevation
(
    uint8_t  minElevation
)
{
    le_result_t result = LE_FAULT;
    TAF_ERROR_IF_RET_VAL( minElevation > TAF_LOCGNSS_MIN_ELEVATION_MAX_DEGREE, LE_OUT_OF_RANGE, "minimum elevation is above maximal range");
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
                    LE_DEBUG("SetMinElevation is success");
                    result = LE_OK;
                    mMinSvEle = minElevation;
                }
                else
                {
                    LE_DEBUG("SetMinElevation is failed");
                    result = LE_FAULT;
                }
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
le_result_t taf_locGnss::StartMode
(
    taf_locGnss_StartMode_t mode    ///< [IN] Start mode
)
{
    le_result_t result = LE_OK;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    if (mode >= TAF_LOCGNSS_UNKNOWN_START)
    {
        LE_ERROR("Invalid start mode %d", mode);
        return LE_BAD_PARAMETER;
    }
    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        {
            if(mode == TAF_LOCGNSS_HOT_START) //Hot Start
            {
                LE_DEBUG("Hot Start! No operation\n");
            }
            else if(mode == TAF_LOCGNSS_WARM_START) //Warm Start
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
                AidingData |1UL << 0 which is 1*/

                uint32_t AidingData = TAF_LOCGNSS_AIDING_DATA_EPHEMERIS;
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
            else if((mode == TAF_LOCGNSS_COLD_START) || (mode == TAF_LOCGNSS_FACTORY_START))
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

                LE_ERROR("Invalid Start Mode!\n");
                result = LE_FAULT;
            }

            if(result == LE_OK)
            {
                if (!clientRequestPtr->mStarted)
                {
                    int optInterval = clientRequestPtr->mAcqRate;
                    std::promise<le_result_t> p;
                    auto cb = [&p](telux::common::ErrorCode error) {
                        if(error == telux::common::ErrorCode::SUCCESS) {
                            p.set_value(LE_OK);
                        }
                        else {
                            p.set_value(LE_FAULT);
                        }
                    };
                    LE_DEBUG("StartMode()->  mAcqRate: %d",clientRequestPtr->mAcqRate);
                    if( optInterval == 0  || optInterval < 100)
                    {
                        LE_DEBUG("StartMode()->mAcqRate is zero, so set default to 100ms");
                        optInterval = 100;
                        clientRequestPtr->mAcqRate = optInterval;
                    }
                    LocReqEngine engineType = DEFAULT_UNKNOWN;
                    GnssReportTypeMask reportMask = DEFAULT_UNKNOWN;
                    reportMask = 0x7f;//all reports are enabled
                    LE_DEBUG("StartMode->reportMask : %u",reportMask);
                    engineType |= (1UL << clientRequestPtr->mEngineType);
                    auto status = clientRequestPtr->locationManager->startDetailedEngineReports((uint32_t)optInterval,
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
                            ConfigureAcqStartInfo(clientRequestPtr);
                            LE_DEBUG("StartMode->Start() is success");
                        }
                        else
                        {
                            LE_DEBUG("StartMode->Start() is failed");
                            result = LE_FAULT;
                        }
                    }
                }
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
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_DUPLICATE;
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
            std::promise<le_result_t> p;
            auto cb = [&p](uint8_t minSVElevation, telux::common::ErrorCode error) {
                LE_DEBUG("***Request minimum SV Elevation Info ****");
                auto &gnss = taf_locGnss::GetInstance();
                if(error == telux::common::ErrorCode::SUCCESS)
                {
                    gnss.mRequestMinEle = minSVElevation;
                    LE_DEBUG("onMinSVElevationInfo gnss.mRequestMinEle: %d",gnss.mRequestMinEle);
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
                    LE_DEBUG("requestMinSVElevation is Success");
                    result = LE_OK;
                    *minElevationPtr = mRequestMinEle;
                }
                else
                {
                    LE_DEBUG("requestMinSVElevation is Failed");
                    result = LE_FAULT;
                }
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

le_result_t taf_locGnss::Stop
(
    void
)
{
    le_result_t result = LE_FAULT;
    auto &gnss = taf_locGnss::GetInstance();
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_ACTIVE:
            {
                LE_DEBUG("Stop() is started: %d", (int) clientRequestPtr->mStarted);

                if (clientRequestPtr->mStarted)
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
                    auto status = clientRequestPtr->locationManager->stopReports(cb);
                    if(status != telux::common::Status::SUCCESS)
                    {
                        result = LE_FAULT;
                    }
                    else
                    {
                        std::future<le_result_t> futResult = p.get_future();
                        if(futResult.get() == LE_OK)
                        {
                            clientRequestPtr->mStarted = false;
                            clientRequestPtr->GnssState = TAF_LOCGNSS_STATE_READY;
                            result = LE_OK;
                            gnss.mNmeaMask = 0;//reset nmeaMask on triggering stop.
                            LE_DEBUG("Stop() is success");
                        }
                        else
                        {
                            result = LE_FAULT;
                            LE_DEBUG("Stop() is failed");
                        }
                    }
                }
            }
        break;
        case TAF_LOCGNSS_STATE_READY:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_DUPLICATE;
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
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d", clientRequestPtr->GnssState);
        }
        break;
    }
    return result;
}

le_result_t taf_locGnss::SetNmeaSentences
(
    taf_locGnss_NmeaBitMask_t nmeaMask ///< [IN] Bit mask for enabled NMEA sentences.
)
{
    le_result_t result = LE_FAULT;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

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
        switch (clientRequestPtr->GnssState)
        {
            case TAF_LOCGNSS_STATE_READY:
            case TAF_LOCGNSS_STATE_ACTIVE:
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
                    LE_ERROR("Unsuported items");
                    return LE_FAULT;
                }
                telux::loc::NmeaSentenceConfig nmeaType = 0;
                if((nmeaMask & TAF_LOCGNSS_NMEA_MASK_GGA) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPGGA))
                {
                    nmeaType |= telux::loc::NmeaSentenceType::GGA;
                    LE_DEBUG("SetNmeaSentences ->TAF_LOCGNSS_NMEA_MASK_GGA");
                }
                if((nmeaMask & TAF_LOCGNSS_NMEA_MASK_RMC) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPRMC))
                {
                    nmeaType |= telux::loc::NmeaSentenceType::RMC;
                    LE_DEBUG("SetNmeaSentences ->TAF_LOCGNSS_NMEA_MASK_RMC");
                }
                if((nmeaMask & TAF_LOCGNSS_NMEA_MASK_GSA) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GNGSA))
                {
                    nmeaType |= telux::loc::NmeaSentenceType::GSA;
                    LE_DEBUG("SetNmeaSentences ->TAF_LOCGNSS_NMEA_MASK_GSA");
                }
                if((nmeaMask & TAF_LOCGNSS_NMEA_MASK_VTG) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPVTG))
                {
                    nmeaType |= telux::loc::NmeaSentenceType::VTG;
                    LE_DEBUG("SetNmeaSentences ->TAF_LOCGNSS_NMEA_MASK_VTG");
                }
                if(nmeaMask & TAF_LOCGNSS_NMEA_MASK_GNS)
                {
                    nmeaType |= telux::loc::NmeaSentenceType::GNS;
                    LE_DEBUG("SetNmeaSentences ->TAF_LOCGNSS_NMEA_MASK_GNS");
                }
                if((nmeaMask & TAF_LOCGNSS_NMEA_MASK_DTM) || (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPDTM))
                {
                    nmeaType |= telux::loc::NmeaSentenceType::DTM;
                    LE_DEBUG("SetNmeaSentences ->TAF_LOCGNSS_NMEA_MASK_DTM");
                }
                if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GPGSV)
                {
                    nmeaType |= telux::loc::NmeaSentenceType::GPGSV;
                    LE_DEBUG("SetNmeaSentences ->GPGSV");
                }
                if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GLGSV)
                {
                    nmeaType |= telux::loc::NmeaSentenceType::GLGSV;
                    LE_DEBUG("SetNmeaSentences ->GLGSV");
                }
                if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GAGSV)
                {
                    nmeaType |= telux::loc::NmeaSentenceType::GAGSV;
                    LE_DEBUG("SetNmeaSentences ->GAGSV");
                }
                if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GQGSV)
                {
                    nmeaType |= telux::loc::NmeaSentenceType::GQGSV;
                    LE_DEBUG("SetNmeaSentences ->GQGSV");
                }
                if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GBGSV)
                {
                    nmeaType |= telux::loc::NmeaSentenceType::GBGSV;
                    LE_DEBUG("SetNmeaSentences ->GBGSV");
                }
                if (nmeaMask & TAF_LOCGNSS_NMEA_MASK_GIGSV)
                {
                    nmeaType |= telux::loc::NmeaSentenceType::GIGSV;
                    LE_DEBUG("SetNmeaSentences ->GIGSV");
                }
                LE_DEBUG("SetNmeaSentences nmeaMask mask is : %" PRIu64 "", nmeaMask);

                auto status = mLocationConfigurator->configureNmeaTypes(nmeaType, cb);
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
                        LE_DEBUG("SetNmeaSentences() is success");
                        SetNmeaConfig(nmeaMask);
                    }
                    else
                    {
                        result = LE_FAULT;
                        LE_DEBUG("SetNmeaSentences() is failed");
                    }
                }
                if (LE_OK != result)
                {
                    LE_ERROR("Unable to set the enabled NMEA sentences, error = %d (%s)",
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
    }

    return result;
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
            LE_INFO("the NMEA retrieved from the config is : %" PRIu64"",mNmeaMask);
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

    LE_INFO("Set NMEA LSB config node%s as : %" PRIu64"",config_node_nmea_lsb,(nmeaMask &0xffffffff));
    LE_INFO("Set NMEA MSB config node%s as : %" PRIu64"",config_node_nmea_msb,nmea_msb);

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

        LE_INFO("Get NMEA LSB config node%s as : %" PRIu64"","nmea_lsb",nmea);
    }
    if (le_cfg_NodeExists(iteratorRef, "nmea_msb"))
    {
        uint64_t configNmea = le_cfg_GetInt(iteratorRef,
                                       "nmea_msb", 0);
        nmea = nmea |(taf_locGnss_NmeaBitMask_t)configNmea <<32;//MSB 32 bits and LSB 32 bits

        LE_INFO("Get NMEA MSB config node%s as : %" PRIu64"","nmea_msb",(taf_locGnss_NmeaBitMask_t)configNmea);
        le_cfg_CancelTxn(iteratorRef);
        return nmea;
    }
    LE_WARN("config node %s doesn't exist", "nmea LSB and MSB");

    le_cfg_CancelTxn(iteratorRef);
    return 0;
}

le_result_t taf_locGnss::SetDRConfig(const taf_locGnss_DrParams_t* drParamsPtr)
{
    le_result_t result = LE_NOT_PERMITTED;
    le_result_t sensor_Result = LE_OK;
    le_result_t speedScale_Result = LE_OK;
    le_result_t gyroScale_Result = LE_OK;
    telux::loc::DREngineConfiguration drConfig;
    drConfig.validMask = static_cast<telux::loc::DRConfigValidity>(0);
    TAF_KILL_CLIENT_IF_RET_VAL( NULL == drParamsPtr, LE_FAULT, "drParamsPtr is NULL");
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

 // Check the GNSS device state
    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        {
            //Filling the DR parameters
            bodyToSensorUtility(drConfig,drParamsPtr,clientRequestPtr,&sensor_Result);
            if(sensor_Result == LE_OUT_OF_RANGE)
            {
                return sensor_Result;
            }
            speedScaleUtility(drConfig,drParamsPtr,clientRequestPtr,&speedScale_Result);
            if(speedScale_Result == LE_OUT_OF_RANGE)
            {
                return speedScale_Result;
            }
            gyroScaleUtility(drConfig,drParamsPtr,clientRequestPtr,&gyroScale_Result);
            if(gyroScale_Result == LE_OUT_OF_RANGE)
            {
                return gyroScale_Result;
            }
            std::promise<le_result_t> p;
            auto cb = [&p](telux::common::ErrorCode error) {
                if(error == telux::common::ErrorCode::SUCCESS) {
                    p.set_value(LE_OK);
                }
                else {
                    p.set_value(LE_FAULT);
                }
            };

            // Set the DR Configuration
            telux::common::Status status = mLocationConfigurator->configureDR(drConfig, cb);
            if (status == telux::common::Status::FAILED) {
                LE_DEBUG("SetDRConfig is failed");
                result = LE_FAULT;
            } else if (telux::common::Status::SUCCESS == status) {
                std::future<le_result_t> futResult = p.get_future();
                result = futResult.get();
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

void bodyToSensorUtility(telux::loc::DREngineConfiguration& drConfig,
        const taf_locGnss_DrParams_t* drParamsPtr,taf_locGnss_Client_t* clientRequestPtr,
        le_result_t* sensor_Result)
{
    if(clientRequestPtr->drParamsMask & TAF_LOCGNSS_BODY_TO_SENSOR_MOUNT_PARAMS_VALID)
    {
        LE_DEBUG("SetDRConfig validMask is BODY_TO_SENSOR_MOUNT_PARAMS_VALID");
        drConfig.validMask |= telux::loc::DRConfigValidityType::BODY_TO_SENSOR_MOUNT_PARAMS_VALID;
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

void speedScaleUtility(telux::loc::DREngineConfiguration& drConfig,
        const taf_locGnss_DrParams_t* drParamsPtr,taf_locGnss_Client_t* clientRequestPtr,
        le_result_t* speedScale_Result)
{
    if(clientRequestPtr->drParamsMask & TAF_LOCGNSS_VEHICLE_SPEED_SCALE_FACTOR_VALID)
    {
        LE_DEBUG("SetDRConfig validMask is VEHICLE_SPEED_SCALE_FACTOR_VALID");
        drConfig.validMask |= telux::loc::DRConfigValidityType::VEHICLE_SPEED_SCALE_FACTOR_VALID;
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
        LE_DEBUG("SetDRConfig validMask is VEHICLE_SPEED_SCALE_FACTOR_UNC_VALID");
        drConfig.validMask |= telux::loc::DRConfigValidityType::VEHICLE_SPEED_SCALE_FACTOR_UNC_VALID;
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

void gyroScaleUtility(telux::loc::DREngineConfiguration& drConfig,
        const taf_locGnss_DrParams_t* drParamsPtr,taf_locGnss_Client_t* clientRequestPtr,
        le_result_t* gyroScale_Result)
{
    if(clientRequestPtr->drParamsMask & TAF_LOCGNSS_GYRO_SCALE_FACTOR_VALID)
    {
        LE_DEBUG("SetDRConfig validMask is GYRO_SCALE_FACTOR_VALID");
        drConfig.validMask |= telux::loc::DRConfigValidityType::GYRO_SCALE_FACTOR_VALID;
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
        LE_DEBUG("SetDRConfig validMask is GYRO_SCALE_FACTOR_UNC_VALID");
        drConfig.validMask |= telux::loc::DRConfigValidityType::GYRO_SCALE_FACTOR_UNC_VALID;
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
    LE_DEBUG("roundOffLocationData locData: %lf",*locData);
    double decimal = pow(10,(dplace + 1));
    LE_DEBUG("roundOffLocationData decimal: %lf",decimal);
    if(*locData <0)
    {
        *locData = ((*locData*decimal)-5)/10;
    }
    else
    {
        *locData = ((*locData*decimal)+5)/10;
    }
}
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
le_result_t taf_locGnss::ConfigureEngineState
(
    taf_locGnss_EngineType_t engtype,///< [IN] value for Engine type.
    taf_locGnss_EngineState_t engState///< [IN] value for Engine state.
)
{
    le_result_t result = LE_OK;
    telux::loc::EngineType engineType;
    telux::loc::LocationEngineRunState engineState;
    LE_DEBUG("ConfigureEngineState engtype:%d", engtype);
    LE_DEBUG("ConfigureEngineState engState:%d", engState);
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");
    switch(engtype)
    {
        case TAF_LOCGNSS_ENGINE_TYPE_DRE:
            engineType = telux::loc::EngineType::DRE;
            LE_DEBUG("ConfigureEngineState TAF_LOCGNSS_ENGINE_TYPE_DRE");
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
        case TAF_LOCGNSS_ENGINE_STATE_SUSPENDED:
            engineState = telux::loc::LocationEngineRunState::SUSPENDED;
            LE_DEBUG("ConfigureEngineState TAF_LOCGNSS_ENGINE_STATE_SUSPENDED");
            break;
        case TAF_LOCGNSS_ENGINE_STATE_RUNNING:
            engineState = telux::loc::LocationEngineRunState::RUNNING;
            LE_DEBUG("ConfigureEngineState TAF_LOCGNSS_ENGINE_STATE_RUNNING");
            break;
        default:
        {
            LE_ERROR("Unknown Engine state %d", engState);
            result = LE_FAULT;
            return result;
        }
    }
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
                        LE_DEBUG("ConfigureEngineState succeed.");
                    }
                    else
                    {
                        LE_DEBUG("ConfigureEngineState failed");
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
#endif
le_result_t taf_locGnss::ConfigureRobustLocation
(
    uint8_t enable,///< [IN] value for enable/disable.
    uint8_t enabled911///< [IN] value for 911 enable/disable
)
{
    le_result_t result = LE_OK;
    LE_DEBUG("ConfigureRobustLocation enable:%d", enable);
    LE_DEBUG("ConfigureRobustLocation enabled911:%d", enabled911);
    bool enableRobustloc = false;
    bool enableE911loc = false;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");
    switch(enable)
    {
       case 0:
            enableRobustloc = false;
            LE_DEBUG("ConfigureRobustLocation Disable");
            break;
       case 1:
            enableRobustloc = true;
            LE_DEBUG("ConfigureRobustLocation Enable");
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
            LE_DEBUG("ConfigureRobustLocation disable enableE911");
            break;
       case 1:
            enableE911loc = true;
            LE_DEBUG("ConfigureRobustLocation Enable enableE911");
            break;
        default:
        {
            LE_ERROR("Wrong Input to enabled911: %d", enabled911);
            result = LE_FAULT;
            return result;
        }
    }
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
                    LE_DEBUG("ConfigureEngineState failed");
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
            LE_ERROR("Invalid GNSS state %d", clientRequestPtr->GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
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
    LE_DEBUG("RobustLocationInformation");
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
                std::promise<le_result_t> p;
                auto cb = [&p](const telux::loc::RobustLocationConfiguration
                    rLConfig, telux::common::ErrorCode error) {
                    auto &gnss = taf_locGnss::GetInstance();
                    LE_DEBUG("****onRobustLocationInfo **");
                    if(error == telux::common::ErrorCode::SUCCESS)
                    {
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
                        LE_DEBUG("***onRobustLocationInfo gnss.enable: %d", gnss.mEnable);
                        LE_DEBUG("***onRobustLocationInfo gnss.enabled911: %d", gnss.mEnabled911);
                        LE_DEBUG("***onRobustLocationInfo gnss.majorVersion: %d", gnss.mMajorVersion);
                        LE_DEBUG("***onRobustLocationInfo gnss.minorVersion: %d", gnss.mMinorVersion);
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
                        LE_DEBUG("RobustLocationInformation is Success");
                        *enable = mEnable;
                        *enabled911 = mEnabled911;
                        *majorVersion = mMajorVersion;
                        *minorVersion = mMinorVersion;
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
le_result_t taf_locGnss::DefaultSecondaryBandConstellations
(
)
{
    LE_DEBUG("DefaultSecondaryBandConstellations");

    le_result_t result = LE_FAULT;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
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
                LE_DEBUG("Not implemented");
                result = LE_FAULT;
            } else if (telux::common::Status::SUCCESS == status) {
                std::future<le_result_t> futResult = p.get_future();
                result = futResult.get();
                if(result == LE_OK)
                {
                    LE_DEBUG("Success");
                }
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

le_result_t taf_locGnss::RequestSecondaryBandConstellations
(
   uint32_t * constellationSb
)
{
    LE_DEBUG("RequestSecondaryBandConstellation");
    le_result_t result = LE_FAULT;

    if (NULL == constellationSb)
    {
        LE_KILL_CLIENT("constellationSb is NULL !");
        return result;
    }
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch ( clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]",  clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
        {
            // Set GNSS Request Secondary Band constellation
            std::promise<le_result_t> p;
            auto cb = [&p](telux::loc::ConstellationSet set, telux::common::ErrorCode error) {
                auto &gnss = taf_locGnss::GetInstance();
                LE_DEBUG("***Request Secondary Band Info ****");
                if(error == telux::common::ErrorCode::SUCCESS)
                {
                    for (auto item : set)
                    {
                        if (item == telux::loc::GnssConstellationType::GPS)
                        {
                            LE_DEBUG("onSecondaryBandInfo: GPS");
                            gnss.mRequestSB |= (1<<(TAF_LOCGNSS_SB_CONSTELLATION_GPS-1));//1st bit
                        }
                        else if (item == telux::loc::GnssConstellationType::GALILEO)
                        {
                            LE_DEBUG("onSecondaryBandInfo: GALILEO");
                            gnss.mRequestSB |= (1<<(TAF_LOCGNSS_SB_CONSTELLATION_GALILEO-1));//2nd bit
                        }
                        else if (item == telux::loc::GnssConstellationType::SBAS)
                        {
                            LE_DEBUG("onSecondaryBandInfo: SBAS");
                            gnss.mRequestSB |= (1<<(TAF_LOCGNSS_SB_CONSTELLATION_SBAS-1));//3rd bit
                        }
                        else if (item == telux::loc::GnssConstellationType::COMPASS)
                        {
                            LE_DEBUG("onSecondaryBandInfo: COMPASS");
                            gnss.mRequestSB |= (1<<(TAF_LOCGNSS_SB_CONSTELLATION_COMPASS-1)); //4th bit
                        }
                        else if (item == telux::loc::GnssConstellationType::GLONASS)
                        {
                            LE_DEBUG("onSecondaryBandInfo: GLONASS");
                            gnss.mRequestSB |= (1<<(TAF_LOCGNSS_SB_CONSTELLATION_GLONASS-1)); //5th bit
                        }
                        else if (item == telux::loc::GnssConstellationType::BDS)
                        {
                            LE_DEBUG("onSecondaryBandInfo: BDS");
                            gnss.mRequestSB |= (1<<(TAF_LOCGNSS_SB_CONSTELLATION_BDS-1));//6th bit
                        }
                        else if (item == telux::loc::GnssConstellationType::QZSS)
                        {
                            LE_DEBUG("onSecondaryBandInfo: QZSS");
                            gnss.mRequestSB |=(1<<(TAF_LOCGNSS_SB_CONSTELLATION_QZSS-1));//7th bit
                        }
                        else if (item == telux::loc::GnssConstellationType::NAVIC)
                        {
                            LE_DEBUG("onSecondaryBandInfo: NAVIC");
                            gnss.mRequestSB |= (1<<(TAF_LOCGNSS_SB_CONSTELLATION_NAVIC-1));//8th bit
                        }
                        else
                        {
                            LE_DEBUG("onSecondaryBandInfo: Not supported");
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
                LE_DEBUG("Request secondary band constellations is success");
                result = LE_OK;
                *constellationSb = mRequestSB;
            }
            else
            {
                LE_DEBUG("Request secondary band constellations is failed");
                result = LE_FAULT;
            }
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d",  clientRequestPtr->GnssState);
        }
        break;
    }
    return result;
}

le_result_t taf_locGnss::ConfigureSecondaryBandConstellations
(
   uint32_t constellationSb
)
{
    LE_DEBUG("ConfigureSecondaryBandConstellations");
    le_result_t result = LE_FAULT;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");
    telux::loc::ConstellationSet constellationSet{};
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_GPS-1))) //GPS->1
    {
        constellationSet.insert(telux::loc::GnssConstellationType::GPS);
        LE_DEBUG("ConfigureSecondary Band constellation GPS");
    }
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_GALILEO-1))) //GALILEO->2
    {
        constellationSet.insert(telux::loc::GnssConstellationType::GALILEO);
        LE_DEBUG("ConfigureSecondary Band constellation GALILEO");
    }
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_SBAS-1)))//SBAS->4
    {
        constellationSet.insert(telux::loc::GnssConstellationType::SBAS);
        LE_DEBUG("ConfigureSecondary Band constellation SBAS");
    }
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_COMPASS-1))) //COMPASS->8
    {
        constellationSet.insert(telux::loc::GnssConstellationType::COMPASS);
        LE_DEBUG("ConfigureSecondary Band constellation COMPASS");
    }
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_GLONASS-1)))//GLONASS->16
    {
        constellationSet.insert(telux::loc::GnssConstellationType::GLONASS);
        LE_DEBUG("ConfigureSecondary Band constellation GLONASS");
    }
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_BDS-1))) //BDS->32
    {
        constellationSet.insert(telux::loc::GnssConstellationType::BDS);
        LE_DEBUG("ConfigureSecondary Band constellation BDS");
    }
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_QZSS-1))) //QZSS->64
    {
        constellationSet.insert(telux::loc::GnssConstellationType::QZSS);
        LE_DEBUG("ConfigureSecondary Band constellation QZSS");
    }
    if( constellationSb & (1<<(TAF_LOCGNSS_SB_CONSTELLATION_NAVIC-1))) //NAVIC->128
    {
        constellationSet.insert(telux::loc::GnssConstellationType::NAVIC);
        LE_DEBUG("ConfigureSecondary Band constellation NAVIC");
    }
    switch ( clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        case TAF_LOCGNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]",  clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_LOCGNSS_STATE_READY:
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
                LE_DEBUG("Not implemented");
                result = LE_FAULT;
            } else if (telux::common::Status::SUCCESS == status) {
                std::future<le_result_t> futResult = p.get_future();
                result = futResult.get();
                if(result == LE_OK)
                {
                    LE_DEBUG("Success");
                }
            }
        }
        break;
        default:
        {
            result = LE_FAULT;
            LE_ERROR("Unknown GNSS state %d",  clientRequestPtr->GnssState);
        }
        break;
    }
    return result;
}
#endif


le_result_t taf_locGnss::SetLeverArmConfig(const taf_locGnss_LeverArmParams_t* LeverArmParamsPtr)
{
    le_result_t result = LE_NOT_PERMITTED;
    LeverArmConfigInfo configInfo;
    telux::loc::LeverArmType leverArmType;
    telux::loc::LeverArmParams leverArmParams;
    TAF_KILL_CLIENT_IF_RET_VAL( NULL == LeverArmParamsPtr, LE_FAULT, "LeverArmParamsPtr is NULL");
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch ( clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        {
            if((LeverArmParamsPtr->levArmType < TAF_LOCGNSS_LEVER_ARM_TYPE_GNSS_TO_VRP)
                || (LeverArmParamsPtr->levArmType >TAF_LOCGNSS_LEVER_ARM_TYPE_VPE_IMU_TO_GNSS))
            {
                LE_DEBUG("invalid Lever Arm type, returning");
                return LE_BAD_PARAMETER;
            }
            //Filling the Lever Arm types
            if(LeverArmParamsPtr->levArmType == TAF_LOCGNSS_LEVER_ARM_TYPE_GNSS_TO_VRP)
            {
                leverArmType = telux::loc::LEVER_ARM_TYPE_GNSS_TO_VRP;
            }
            else if(LeverArmParamsPtr->levArmType == TAF_LOCGNSS_LEVER_ARM_TYPE_DR_IMU_TO_GNSS)
            {
                leverArmType = telux::loc::LEVER_ARM_TYPE_DR_IMU_TO_GNSS;
            }
            else if(LeverArmParamsPtr->levArmType == TAF_LOCGNSS_LEVER_ARM_TYPE_VPE_IMU_TO_GNSS)
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
                    LE_DEBUG("Set Lever Arm parameters is OK");
                    result = LE_OK;
                }
                else
                {
                    LE_DEBUG("Set Lever Arm parameters is FAILED");
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

le_result_t taf_locGnss::SetEngineType(taf_locGnss_EngineReportsType_t EngineType)
{
    le_result_t result = LE_FAULT;
    LE_DEBUG("SetEngineType EngineType %d",EngineType);
    if((EngineType <TAF_LOCGNSS_ENGINE_REPORT_TYPE_FUSED)
          || (EngineType>TAF_LOCGNSS_ENGINE_REPORT_TYPE_VPE))
    {
        LE_DEBUG("SetEngineType: Unknown Engine type");
        return LE_BAD_PARAMETER;
    }
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    switch (clientRequestPtr->GnssState)
    {
        case TAF_LOCGNSS_STATE_READY:
        {
            // Set Engine Type
            clientRequestPtr->mEngineType = EngineType;
            result = LE_OK;
            LE_DEBUG("SetEngineType->EngineType:%d",clientRequestPtr->mEngineType);
        }
        break;
        case TAF_LOCGNSS_STATE_ACTIVE:
        case TAF_LOCGNSS_STATE_UNINITIALIZED:
        case TAF_LOCGNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", clientRequestPtr->GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
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
            LE_DEBUG("GetConformityIndex is not valid");
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
            LE_DEBUG("GetCalibrationData is not valid");
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
            LE_DEBUG("GetCalibrationData is not valid");
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
            LE_DEBUG("GetDRSolutionStatus is not valid");
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
            LE_DEBUG("GetBodyFrameData is not valid");
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
            LE_DEBUG("GetVRPBasedLLA latitude is not valid");
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
            LE_DEBUG("GetVRPBasedLLA longtitude is not valid");
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
            LE_DEBUG("GetVRPBasedLLA altitude is not valid");
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
            LE_DEBUG("GetVRPBasedVelocity east velocity is not valid");
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
            LE_DEBUG("GetVRPBasedVelocity north velocity is not valid");
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
            LE_DEBUG("GetVRPBasedVelocity up velocity is not valid");
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
            LE_DEBUG("GetSvUsedInPosition is invalid");
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
            LE_DEBUG("GetSbasCorrection is invalid");
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
            LE_DEBUG("GetPositionTechnology: loc tech is invalid");
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
            LE_DEBUG("GetLocationInfoValidity is invalid");
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
            LE_DEBUG("GetLocationInfoExValidity is invalid");
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
            LE_DEBUG("GetLocationOutputEngParams eng mask is invalid");
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
            LE_DEBUG("GetLocationOutputEngParams location engine type is invalid");
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
            LE_DEBUG("GetReliabilityInformation horizontal reliability is invalid");
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
            LE_DEBUG("GetReliabilityInformation vertical reliability is invalid");
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
            LE_DEBUG("GetStdDeviationAzimuthInfo: azimuth is invalid");
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
            LE_DEBUG("GetStdDeviationAzimuthInfo: eastDev is invalid");
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
            LE_DEBUG("GetStdDeviationAzimuthInfo: northDev is invalid");
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
            LE_DEBUG("GetRealTimeInformation: real time is invalid");
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
            LE_DEBUG("GetRealTimeInformation: real time unc is invalid");
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

    for (auto i = 0; i < (int) *sVIdsLen; i++) {
        sVIdsPtr[i] = posSampleReqPtr->positionSampleNodePtr->SVIds[i];
    }

    *sVIdsLen = posSampleReqPtr->positionSampleNodePtr->SVIdsCount;

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
    le_result_t result = LE_OK;
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
                svInfoPtr[i].satConst = TAF_LOCGNSS_SV_CONSTELLATION_UNDEFINED;
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

le_result_t taf_locGnss::SetMinGpsWeek
(
    uint16_t minGpsWeek
)
{
    le_result_t result = LE_FAULT;
    std::promise<telux::common::ErrorCode> p;
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
                LE_DEBUG("configureMinGpsWeek is failed");
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
            LE_DEBUG("GetMinGpsWeek: error code %d", (int) error);
            if (error == ErrorCode::SUCCESS) {
                LE_DEBUG("GetMinGpsWeek is Success.");
                result = LE_OK;
                *minGpsWeekPtr = p.get_future().get();
            }
            else
            {
                LE_DEBUG("GetMinGpsWeek is Failed!");
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

    *locCapabilityPtr = clientRequestPtr->locationManager->getCapabilities();

    LE_DEBUG("GetCapabilities: Location Capabilites is %" PRIu64 "", *locCapabilityPtr);

    return LE_OK;
}

le_result_t taf_locGnss::SetNmeaConfiguration
(
    taf_locGnss_NmeaBitMask_t nmeaMask,         ///< [IN] Bit mask for enabled NMEA sentences.
    taf_locGnss_GeodeticDatumType_t datumType,  ///< [IN] Specify the datum type to be configured.
    taf_locGnss_LocEngineType_t engineType      ///< [IN] Specify the Engine type.
)
{
    le_result_t result = LE_NOT_PERMITTED;
    std::promise<telux::common::ErrorCode> p;
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_VAL( NULL == clientRequestPtr, LE_FAULT, "clientRequestPtr is NULL");

    telux::loc::NmeaConfig nmeaConfig;
    nmeaConfig.sentenceConfig = nmeaMask;
    if(datumType>=TAF_LOCGNSS_GEODETIC_TYPE_WGS_84 && datumType <= TAF_LOCGNSS_GEODETIC_TYPE_PZ_90)
    {
        nmeaConfig.datumType = (telux::loc::GeodeticDatumType) datumType;
    }
    else
    {
        return LE_FAULT;
    }
#if defined(TARGET_SA525M)
    if(engineType>= TAF_LOCGNSS_LOC_ENGINE_FUSED && engineType <= TAF_LOCGNSS_LOC_ENGINE_VPE)
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
        switch (clientRequestPtr->GnssState)
        {
            case TAF_LOCGNSS_STATE_READY:
            case TAF_LOCGNSS_STATE_ACTIVE:
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
                    LE_DEBUG("SetNmeaConfiguration() is failed!");
                }
                if (LE_OK != result)
                {
                    LE_ERROR("Unable to set the enabled NMEA, error = %d (%s)",
                              result, LE_RESULT_TXT(result));
                }
            }
            break;
            case TAF_LOCGNSS_STATE_UNINITIALIZED:
            case TAF_LOCGNSS_STATE_DISABLED:
            {
                LE_ERROR("SetNmeaConfiguration: Bad state for that request [%d]", clientRequestPtr->GnssState);
                result = LE_NOT_PERMITTED;
            }
            break;
            default:
            {
                LE_ERROR("SetNmeaConfiguration: Unknown GNSS state %d", clientRequestPtr->GnssState);
                result = LE_FAULT;
            }
            break;
        }
    }

    return result;
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
            std::promise<le_result_t> p;
            auto cb = [&p](telux::loc::XtraStatus xtraStatus, telux::common::ErrorCode error)
            {
                auto &gnss = taf_locGnss::GetInstance();
                if(error == telux::common::ErrorCode::SUCCESS)
                {
                    LE_DEBUG("***Request xtra status Info ****");
                    LE_DEBUG("Xtra Feature Enabled: %d",xtraStatus.featureEnabled);
                    LE_DEBUG("Xtra status valid for hours :%d",xtraStatus.xtraValidForHours);
                    gnss.mfeatureEnabled = xtraStatus.featureEnabled;
                    gnss.mXtraValidForHours = xtraStatus.xtraValidForHours;
                    switch(xtraStatus.xtraDataStatus)
                    {
                        case telux::loc::XtraDataStatus::STATUS_UNKNOWN:
                        LE_DEBUG("GetXtraStatus Unknown");
                        gnss.mXtraDataStatus = TAF_LOCGNSS_XTRA_DATA_STATUS_UNKNOWN;
                        break;
                        case telux::loc::XtraDataStatus::STATUS_NOT_AVAIL:
                        LE_DEBUG("GetXtraStatus Not available");
                        gnss.mXtraDataStatus = TAF_LOCGNSS_XTRA_DATA_STATUS_NOT_AVAIL;
                        break;
                        case telux::loc::XtraDataStatus::STATUS_NOT_VALID:
                        LE_DEBUG("GetXtraStatus Invalid");
                        gnss.mXtraDataStatus = TAF_LOCGNSS_XTRA_DATA_STATUS_NOT_VALID;
                        break;
                        case telux::loc::XtraDataStatus::STATUS_VALID:
                        LE_DEBUG("GetXtraStatus Valid \n");
                        gnss.mXtraDataStatus = TAF_LOCGNSS_XTRA_DATA_STATUS_VALID;
                        break;
                    }
                    p.set_value(LE_OK);
                }
                else
                {
                    LE_DEBUG("*Request xtra status failed errorCode: %d ", int(error));
                    p.set_value(LE_FAULT);
                }
            };
            auto status = mLocationConfigurator->requestXtraStatus(cb);
            if (status != telux::common::Status::SUCCESS) {
                return LE_FAULT;
            }
            std::future<le_result_t> futResult = p.get_future();
            if(futResult.get() == LE_OK)
            {
                LE_DEBUG("Request xtra status is success");
                result = LE_OK;
                xtraParams->featureEnabled = mfeatureEnabled;
                xtraParams->xtraValidForHours = mXtraValidForHours;
                xtraParams->xtraDataStatus = mXtraDataStatus;
            }
            else
            {
                LE_ERROR("Request xtra status is failed");
                result = LE_FAULT;
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
void taf_locGnss::RemovePositionHandler
(
    taf_locGnss_PositionHandlerRef_t handlerRef
)
{
    taf_locGnss_Client_t* clientRequestPtr = NULL;
    clientRequestPtr = AcquireSessionRef();

    TAF_ERROR_IF_RET_NIL( NULL == clientRequestPtr, "clientRequestPtr is NULL");

    taf_locGnss_PositionHandler_t* positionHandlerPtr =
        (taf_locGnss_PositionHandler_t*)le_ref_Lookup(clientRequestPtr->PositionHandlerRefMap, handlerRef);

    if (positionHandlerPtr != NULL)
    {
        if(positionHandlerPtr->handlerRef != handlerRef) {
            return;
        }
        NumOfPositionHandlers--;

        LE_DEBUG("Removed positionHandlerRef(%p) for positionHandlerPtr(%p) (totalCnt=0x%x).",
                 positionHandlerPtr->handlerRef, positionHandlerPtr, NumOfPositionHandlers);
        le_mem_Release(positionHandlerPtr);
        le_ref_DeleteRef(clientRequestPtr->PositionHandlerRefMap, handlerRef);
    }
    else
    {
        LE_ERROR("Invaild handlerRef(%p).", handlerRef);
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
        LE_DEBUG("Remove Client Position Ctrl (%p)",RefPtr);
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

void taf_locGnss::OpenEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    LE_DEBUG("OnClientConnection sessionRef: %p", sessionRef);

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
    LE_DEBUG("SessionRef (%p) has been closed", sessionRef);

    TAF_ERROR_IF_RET_NIL( sessionRef == NULL, "sessionRef is NULL");

    if (gnss.GetState() == TAF_LOCGNSS_STATE_ACTIVE) {
        gnss.Stop();
    }


    le_ref_IterRef_t iterRef = le_ref_GetIterator(gnss.PositionSampleMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        taf_locGnss_PositionSampleRequest_t *positionSampleRequestPtr =
                                (taf_locGnss_PositionSampleRequest_t*)le_ref_GetValue(iterRef);
        if(positionSampleRequestPtr == NULL) {
            return;
        }

        if (positionSampleRequestPtr->sessionRef == sessionRef)
        {
            taf_locGnss_SampleRef_t safeRef = (taf_locGnss_SampleRef_t)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release taf_locGnss_ReleaseSampleRef 0x%p, Session 0x%p", safeRef, sessionRef);

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
        taf_locGnss_Client_t* gnssPtr = (taf_locGnss_Client_t*) le_ref_GetValue(iterRef);
        LE_ASSERT(gnssPtr != NULL);

        if (sessionRef == gnssPtr->sessionRef)
        {
            if (gnss.mClientRefCount > 0)
            {
                //External Client release, decrement client ref count
                gnss.mClientRefCount--;
                LE_DEBUG("CloseEventHandler client count: %d",gnss.mClientRefCount);
            }
            gnss.CleanUp(gnssPtr);
            void* safeRefPtr = (void*)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release taf_locGnss_ReleaseClientRef 0x%p, Session 0x%p",
                     safeRefPtr, gnssPtr->sessionRef);

            gnss.ReleaseClientRef(safeRefPtr);
        }

        result = le_ref_NextNode(iterRef);
    }
}

taf_locGnss::~taf_locGnss() {
    auto &gnss = taf_locGnss::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(gnss.ClientRequestRefMap);
    le_result_t result = le_ref_NextNode(iterRef);

    while (LE_OK == result)
    {
        taf_locGnss_Client_t* gnssPtr = (taf_locGnss_Client_t*) le_ref_GetValue(iterRef);
        if(gnssPtr == NULL) {
            return;
        }

        LE_DEBUG("gnssPtr %p, gnssPtr->sessionRef %p",
                 gnssPtr, gnssPtr->sessionRef);
        if(gnssPtr->locationManager && gnssPtr->posListener) {
            gnssPtr->locationManager->deRegisterListenerEx(gnssPtr->posListener);
            auto status = gnssPtr->locationManager->deRegisterForSystemInfoUpdates(gnssPtr->posListener);
            if(status == telux::common::Status::SUCCESS)
            {
                LE_DEBUG("Deregistered a listener for location system information");
            }
            else
            {
                LE_ERROR("Failed to deregister a listener for location system information");
            }
        }
        if(gnssPtr->posListener) {
            gnssPtr->posListener = nullptr;
        }

        if(gnssPtr->locationManager) {
            gnssPtr->locationManager = nullptr;
        }
        result = le_ref_NextNode(iterRef);
   }
}

void taf_locGnss::CleanUp(taf_locGnss_Client_t* clientPtr)
{
    if(clientPtr == NULL)
    {
        LE_ERROR("ClientPtr is Null");
        return;
    }

    if(clientPtr->locationManager)
    {
        telux::common::Status status = telux::common::Status::FAILED;
        status = clientPtr->locationManager->deRegisterListenerEx(clientPtr->posListener);
        if(status == telux::common::Status::SUCCESS)
        {
            LE_DEBUG("client deRegisterListenerEx");
        }
        else
        {
            LE_DEBUG("Failed to deRegisterListenerEx");
        }
        status = clientPtr->locationManager->deRegisterForSystemInfoUpdates(clientPtr->posListener);
        if(status == telux::common::Status::SUCCESS)
        {
            LE_DEBUG("Deregistered a listener for location system information");
        }
        else
        {
            LE_DEBUG("Failed to deregister a listener for location system information");
        }
    }

    if(clientPtr->posListener)
    {
        clientPtr->posListener = nullptr;
        LE_DEBUG("clientPtr->posListener is nullptr");
    }
}

void taf_locGnss::Init()
{

    telux::common::Status status = telux::common::Status::FAILED;
    SessionCtxList = LE_DLS_LIST_INIT;
    NumOfPositionHandlers = 0;
    NumOfCapabilityHandlers = 0;
    NumOfNmeaHandlers = 0;
    mClientRefCount = 0;

    status = LocationConfiguratorInit();
    if (status != telux::common::Status::SUCCESS) {
        LE_FATAL("LocationConfigurator not available");
    }

    // UnComment this code for injection of RTCM3 format data
    /*status = DgnssManagerInit();
    if (status != telux::common::Status::SUCCESS) {
        LE_CRIT("DGnss not available");
    }*/

    PositionHandlerPoolRef = le_mem_InitStaticPool(PositionHandler, GNSS_POSITION_HANDLER_HIGH,
            sizeof(taf_locGnss_PositionHandler_t));

    PositionSamplePoolRef = le_mem_InitStaticPool(PositionSample, GNSS_POSITION_SAMPLE_MAX,
            sizeof(taf_locGnss_PositionSample_t));

    PositionSampleRequestPoolRef = le_mem_InitStaticPool(PositionSampleRequest,
            GNSS_POSITION_SAMPLE_MAX, sizeof(taf_locGnss_PositionSampleRequest_t));


    PositionSampleMap = le_ref_InitStaticMap(PositionSampleMap, GNSS_POSITION_SAMPLE_MAX);

    ClientRequestRefMap = le_ref_CreateMap("ClientRequestRefMap", TAF_CONFIG_POSITIONING_ACTIVATION_MAX);

    ClientPoolRef = le_mem_InitStaticPool(Client, TAF_CONFIG_POSITIONING_ACTIVATION_MAX, sizeof(taf_locGnss_Client_t));
    positionEventId = le_event_CreateIdWithRefCounting("positionEventId");

    HandlerRef = le_event_AddHandler("LocUpdateEventId", positionEventId, taf_locGnss::GnssPositionHandler);

    locCapabilityEventId = le_event_CreateId("LocCapabilityEventId", sizeof(CapabilityChangeEvent_t));
    nmeaEventId = le_event_CreateId("NmeaEventId", sizeof(NmeaInfoEvent_t));

    le_msg_ServiceRef_t msgService = taf_locGnss_GetServiceRef();
    le_msg_AddServiceOpenHandler(msgService, OpenEventHandler, NULL);
    le_msg_AddServiceCloseHandler(msgService, CloseEventHandler, NULL);
    if (GetNmeaConfig()== 0)
    {
       TAF_LOCGNSS_NMEA_DEFAULT = 0x1f8000fc0;//If value is 0 it will set the default NMEA sentences (all sentences enabled)
       LE_DEBUG("Nmea node doesn't exist, so set default NMEA value");
       SetNmeaConfig(TAF_LOCGNSS_NMEA_DEFAULT);
    }
    return;
}
le_result_t taf_locGnss::SetDRConfigValidity(taf_locGnss_DRConfigValidityType_t validMask)
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