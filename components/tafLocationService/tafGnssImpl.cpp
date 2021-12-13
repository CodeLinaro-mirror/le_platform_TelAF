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

 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.

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

using namespace telux::loc;
using namespace telux::common;
using namespace telux::tafsvc;

LE_MEM_DEFINE_STATIC_POOL(PositionHandler, GNSS_POSITION_HANDLER_HIGH, sizeof(taf_gnss_PositionHandler_t));
LE_MEM_DEFINE_STATIC_POOL(PositionSample, GNSS_POSITION_SAMPLE_MAX, sizeof(taf_gnss_PositionSample_t));
LE_MEM_DEFINE_STATIC_POOL(PositionSampleRequest, GNSS_POSITION_SAMPLE_MAX, sizeof(taf_gnss_PositionSampleRequest_t));
LE_MEM_DEFINE_STATIC_POOL(Client, LE_CONFIG_POSITIONING_ACTIVATION_MAX, sizeof(taf_gnss_Client_t));
LE_REF_DEFINE_STATIC_MAP(PositionSampleMap, GNSS_POSITION_SAMPLE_MAX);

taf_Gnss &taf_Gnss::GetInstance()
{
    static taf_Gnss instance;
    return instance;
}

telux::common::Status taf_Gnss::DgnssManagerInit() {
    if(mDgnssManager == nullptr) {
        std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
        auto &locationFactory = LocationFactory::getInstance();
        mDgnssManager = locationFactory.getDgnssManager(DgnssDataFormat::DATA_FORMAT_RTCM_3,
            [&](ServiceStatus status) {
                if (status == ServiceStatus::SERVICE_AVAILABLE) {
                    prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
                } else {
                    prom.set_value(ServiceStatus::SERVICE_FAILED);
                }
            });
        if (!mDgnssManager) {
            LE_INFO( "Failed to get Gnss manager object");
            return Status::FAILED;
        }

        std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
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
   } else {
       LE_INFO("Dgnss manager is already initialized");
   }
   return telux::common::Status::SUCCESS;
}

telux::common::Status taf_Gnss::LocationManagerInit() {
    if(mLocationManager == nullptr) {
        std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
        auto &locationFactory = LocationFactory::getInstance();
        mLocationManager = locationFactory.getLocationManager([&](ServiceStatus status) {
                if (status == ServiceStatus::SERVICE_AVAILABLE) {
                prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
                } else {
                prom.set_value(ServiceStatus::SERVICE_FAILED);
                }
                });
        std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
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
        auto &locationFactory = LocationFactory::getInstance();
        mLocationConfigurator = locationFactory.getLocationConfigurator([&](ServiceStatus status) {
                if (status == ServiceStatus::SERVICE_AVAILABLE) {
                prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
                } else {
                prom.set_value(ServiceStatus::SERVICE_FAILED);
                }
                });
        std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
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
    }

    for(i=0; i<TAF_GNSS_SV_INFO_MAX_LEN; i++)
    {
        LastDataPtr->satMeas[i].satId = CurrentDataPtr->satMeas[i].satId;
        LastDataPtr->satMeas[i].satLatency = CurrentDataPtr->satMeas[i].satLatency;
    }
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
    le_dls_Link_t* linkPtr;
    uint8_t i;
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

    linkPtr = le_dls_Peek(&gnss.PositionHandlerList);
    if (NULL != linkPtr)
    {
        posSampleReqPtr = (taf_gnss_PositionSampleRequest_t*)le_mem_ForceAlloc(gnss.PositionSampleRequestPoolRef);

        posSampleReqPtr->positionSampleNodePtr =
            (taf_gnss_PositionSample_t*)le_mem_ForceAlloc(gnss.PositionSamplePoolRef);

        // No need to add reference for single Handler
        for(i=0 ; i<gnss.NumOfPositionHandlers-1 ; i++)
        {
            le_mem_AddRef((void *)posSampleReqPtr);
            le_mem_AddRef((void *)posSampleReqPtr->positionSampleNodePtr);
        }

        memcpy(posSampleReqPtr->positionSampleNodePtr, &gnss.LastPositionSample,
                sizeof(taf_gnss_PositionSample_t));

        posSampleReqPtr->positionSampleNodePtr->next = LE_DLS_LINK_INIT;
        le_dls_Queue(&gnss.PositionSampleList, &(posSampleReqPtr->positionSampleNodePtr->next));

        do
        {
            posHandlerPtr =
                (taf_gnss_PositionHandler_t*)CONTAINER_OF(linkPtr, taf_gnss_PositionHandler_t, next);

            LE_DEBUG("Report sample %p to the corresponding handler (handler %p)",
                    posSampleReqPtr->positionSampleNodePtr, posHandlerPtr->handlerFuncPtr);

            taf_gnss_SampleRef_t safePositionSampleRef = (taf_gnss_SampleRef_t)le_ref_CreateRef(gnss.PositionSampleMap,
                    posSampleReqPtr);

            posSampleReqPtr->sessionRef = posHandlerPtr->sessionRef;

            posSampleReqPtr->positionSampleRef = safePositionSampleRef;

            if(safePositionSampleRef != NULL)
            {
                posHandlerPtr->handlerFuncPtr(safePositionSampleRef,
                        posHandlerPtr->handlerContextPtr);
            }

            linkPtr = le_dls_PeekNext(&gnss.PositionHandlerList, linkPtr);
        } while (NULL != linkPtr);
    }

    le_mem_Release(currentPosPtr);
}

void tafLocationListener::onBasicLocationUpdate(const std::shared_ptr<telux::loc::ILocationInfoBase> &locationInfo) {
    LE_DEBUG("*** Basic Location Report ***");
}

void tafLocationListener::onDetailedLocationUpdate(const std::shared_ptr<telux::loc::ILocationInfoEx> &locationInfo) {
    auto &gnss = taf_Gnss::GetInstance();
    if (gnss.mTtffEnabled) {
        gnss.mEndTime = std::chrono::system_clock::now();
        std::unique_lock<std::mutex> lock(gnss.mMutex);
        gnss.mTtffEnabled = false;
        gnss.mCondVar.notify_one();
    }

    le_mutex_Lock(gnss.mGnssMutexRef);
    if(gnss.NumOfPositionHandlers ) {
        LE_DEBUG("**** Detailed Location Report ****");
        gnss.mLocEnabled = true;

        telux::loc::LocationTechnology techMask = locationInfo->getTechMask();
        if((techMask & telux::loc::LOC_GNSS)) {
            LE_INFO( "location calculated using GNSS" );
        }
        if((techMask & telux::loc::LOC_CELL)) {
            LE_INFO( "location calculated using CELL" );
        }
        if((techMask & telux::loc::LOC_WIFI)) {
            LE_INFO( "location calculated using WIFI" );
        }
        if((techMask & telux::loc::LOC_SENSORS)) {
            LE_INFO( "location calculated using SENSORS" );
        }
        if((techMask & telux::loc::LOC_REFERENCE_LOCATION)) {
            LE_INFO( "location calculated using Reference location" );
        }
        if((techMask & telux::loc::LOC_INJECTED_COARSE_POSITION)) {
            LE_INFO( "location calculated using Coarse position injected into the location engine" );
        }
        if((techMask & telux::loc::LOC_AFLT)) {
            LE_INFO( "location calculated using AFLT" );
        }
        if((techMask & telux::loc::LOC_HYBRID)) {
            LE_INFO( "location calculated using GNSS and network-provided measurements" );
        }
        if((techMask & telux::loc::LOC_PPE)) {
            LE_INFO( "location calculated using Precise position engine" );
        }
        if((techMask & telux::loc::LOC_VEH)) {
            LE_INFO( "location calculated using Vehicular data" );
        }
        if((techMask & telux::loc::LOC_VIS)) {
            LE_INFO( "location calculated using Visual data" );
        }

        if ( gnss.mSvEnabled && gnss.mGnssSigEnabled && gnss.mGnssNmeaEnabled ) {
            taf_gnss_PositionSample_t* LocationData = (taf_gnss_PositionSample_t*)le_mem_ForceAlloc(gnss.PositionSamplePoolRef);
            uint8_t i;
            LocationData->fixState = TAF_GNSS_STATE_FIX_3D;
            LocationData->latitudeValid = true;
            LocationData->longitudeValid = true;
            LocationData->hAccuracyValid = true;
            LocationData->altitudeValid = true;
            LocationData->altitudeOnWgs84Valid = false;
            LocationData->horUncEllipseSemiMajorValid = true;
            LocationData->horUncEllipseSemiMinorValid = true;
            LocationData->horConfidenceValid = false;
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

            LocationData->latitude = locationInfo->getLatitude() * 1e+6;
            LocationData->longitude = locationInfo->getLongitude() * 1e+6;
            LocationData->hAccuracy = locationInfo->getHorizontalUncertainty()* 1e+2;
            LocationData->altitude = locationInfo->getAltitude() * 1e+3;
            LocationData->vAccuracy = locationInfo->getVerticalUncertainty() * 10;
            LocationData->altitudeOnWgs84 = 0;
            LocationData->hSpeedAccuracy = 0;
            LocationData->vSpeed = locationInfo->getSpeed();
            LocationData->vSpeedAccuracy = locationInfo->getSpeedUncertainty();
            LocationData->magneticDeviation = locationInfo->getMagneticDeviation();
            LocationData->epochTime = locationInfo->getTimeStamp();
            LocationData->horUncEllipseSemiMajor = locationInfo->getHorizontalUncertaintySemiMajor();
            LocationData->horUncEllipseSemiMinor = locationInfo->getHorizontalUncertaintySemiMinor();
            LocationData->hSpeed = 0;
            LocationData->direction = locationInfo->getHeading();
            LocationData->directionAccuracy = locationInfo->getHeadingUncertainty();
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
                LocationData->year = 1900+ltm->tm_year;
                LocationData->month = 1+ltm->tm_mon;
                LocationData->day = ltm->tm_mday;
                LocationData->hours = 5+ltm->tm_hour;
                LocationData->minutes = 30+ltm->tm_min;
                LocationData->seconds = ltm->tm_sec;
                LocationData->milliseconds = 0;
            } else {
                LE_DEBUG("Time stamp Not Valid");
                LocationData->year = 0;
                LocationData->month = 0;
                LocationData->day = 0;
                LocationData->hours = 0;
                LocationData->minutes = 0;
                LocationData->seconds = 0;
                LocationData->milliseconds = 0;
            }
            LocationData->horConfidence = 0;
            if(locationInfo->getLeapSeconds(gnss.mLeapSeconds) == telux::common::Status::SUCCESS) {
                LocationData->leapSeconds = gnss.mLeapSeconds;
            } else {
                LocationData->leapSeconds = 0;
            }
            LocationData->satsInViewCount = 0;
            LocationData->satsTrackingCount = 0;
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
            }

            for(i=0; i<TAF_GNSS_SV_INFO_MAX_LEN; i++)
            {
                LocationData->satMeas[i].satId = 0;
                LocationData->satMeas[i].satLatency = 0;
            }
            LocationData->next = LE_DLS_LINK_INIT;

            le_event_ReportWithRefCounting(gnss.positionEventId, LocationData);
            gnss.mLocEnabled = false;
            gnss.mSvEnabled = false;
            gnss.mGnssSigEnabled = false;
            gnss.mGnssNmeaEnabled = false;
        }
    }
    le_mutex_Unlock(gnss.mGnssMutexRef);
}

void tafLocationListener::onDetailedEngineLocationUpdate(
      const std::vector<std::shared_ptr<telux::loc::ILocationInfoEx> > &locationEngineInfo) {
    auto &gnss = taf_Gnss::GetInstance();
    le_mutex_Lock(gnss.mGnssMutexRef);
    LE_DEBUG("**** Detailed Engine Location Report ****");
    le_mutex_Unlock(gnss.mGnssMutexRef);
}

void tafLocationListener::onGnssSVInfo(const std::shared_ptr<telux::loc::IGnssSVInfo> &gnssSVInfo) {
    auto &gnss = taf_Gnss::GetInstance();

    if (gnss.mMinElelvEnabled) {
        for(auto svInfo : gnssSVInfo->getSVInfoList()) {
            gnss.mMinElev = svInfo->getElevation();
            break;
        }
        std::unique_lock<std::mutex> lock(gnss.mMutex);
        gnss.mMinElelvEnabled = false;
        gnss.mCondVar.notify_one();
    }

    le_mutex_Lock(gnss.mGnssMutexRef);
    if(gnss.NumOfPositionHandlers ) {
        LE_DEBUG("**** Satellite Vehicle Information ****");
        int i = 0;
        gnss.mConstellationEnabled = false;
        for(auto svInfo : gnssSVInfo->getSVInfoList()) {
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
            gnss.mSatInfo[i].satUsed = 0;
            gnss.mSatInfo[i].satTracked = 0;
            gnss.mSatInfo[i].satSnr = svInfo->getSnr();
            gnss.mSatInfo[i].satAzim = svInfo->getAzimuth();
            gnss.mSatInfo[i].satElev = svInfo->getElevation();
            i++;
        }
        gnss.mSvEnabled = true;
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

void tafLocationListener::onGnssNmeaInfo(uint64_t timestamp, const std::string &nmea) {
    auto &gnss = taf_Gnss::GetInstance();
    le_mutex_Lock(gnss.mGnssMutexRef);
    if(gnss.NumOfPositionHandlers ) {
        LE_DEBUG( "**** Gnss Nmea Information ****" );
        //gnss.mSatMeas.satId = nmea;
        gnss.mSatMeas.satLatency = timestamp;
        gnss.mGnssNmeaEnabled = true;
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

void LocationCommandCallback::commandResponse(telux::common::ErrorCode error) {
    auto &gnss = taf_Gnss::GetInstance();
   if(error == telux::common::ErrorCode::SUCCESS) {
      LE_DEBUG("commandResponse %s sent successfully", gnss.mCommandName.c_str());
   } else {
      LE_DEBUG(" commandResponse failed errorCode: %d ", int(error));
   }
}

le_result_t taf_Gnss::PositionDataCoversion
(
 int32_t value,
 taf_gnss_DataType_t dataType,
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
    if (!mStarted) {
        mStarted = true;
        int optInterval = mAcqRate;
        if( optInterval == 0  || optInterval < 1000) {
            LE_DEBUG("mAcqRate is zero, so set default to 1000ms");
            optInterval = 1000;
            mAcqRate = optInterval;
        }
        mLocCmdResponseCb = std::make_shared<LocationCommandCallback>("startDetailedReports");
        mLocationManager->startDetailedReports((uint32_t)optInterval,
                std::bind(&LocationCommandCallback::commandResponse,
                    mLocCmdResponseCb, std::placeholders::_1));
        mStartTime = std::chrono::system_clock::now();
    }
#if 0 // Start Detailed Reports with specific config
    //0 - Location 1 - SV 2 - NMEA 3 - DATA 4 - Measurement 5 - NHzMeasurement
    int opt = 1000;
    GnssReportTypeMask reportMask = 0;
    reportMask |= 1UL << 0;
    mLocCmdResponseCb = std::make_shared<LocationCommandCallback>();
    mLocationManager->startDetailedReports( (uint32_t)opt, std::bind(&LocationCommandCallback::commandResponse,
                mLocCmdResponseCb, std::placeholders::_1), reportMask);
#endif
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

    switch (constellationMask)
    {
        case TAF_GNSS_CONSTELLATION_GPS:
        break;
        case TAF_GNSS_CONSTELLATION_GLONASS:
        blackListInfo.constellation = telux::loc::GnssConstellationType::GLONASS;
        break;
        case TAF_GNSS_CONSTELLATION_BEIDOU:
        blackListInfo.constellation = telux::loc::GnssConstellationType::BDS;
        break;
        case TAF_GNSS_CONSTELLATION_GALILEO:
        blackListInfo.constellation = telux::loc::GnssConstellationType::GALILEO;
        break;
        case TAF_GNSS_CONSTELLATION_SBAS:
        blackListInfo.constellation = telux::loc::GnssConstellationType::SBAS;
        break;
        case TAF_GNSS_CONSTELLATION_QZSS:
        blackListInfo.constellation = telux::loc::GnssConstellationType::QZSS;
        break;
        default:
        {
            LE_ERROR("Unknown GNSS CONSTELLATION %d", constellationMask);
            result = LE_FAULT;
        }
        break;
    }

    blackListInfo.svId = 0;
    svBlackList.push_back(blackListInfo);

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
            // Set GNSS constellation
            mLocCmdResponseCb = std::make_shared<LocationCommandCallback> ("configureConstellations");
            telux::common::Status status = mLocationConfigurator->configureConstellations(svBlackList,
                    std::bind(&LocationCommandCallback::commandResponse, mLocCmdResponseCb,
                        std::placeholders::_1), deviceReset);
            if (status == telux::common::Status::NOTIMPLEMENTED) {
                LE_INFO("Not implemented");
                result = LE_FAULT;
            } else if (telux::common::Status::SUCCESS == status) {
                result = LE_OK;
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
            GnssState = TAF_GNSS_STATE_ACTIVE;
            result = LE_OK;
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

le_result_t taf_Gnss::SetConstellationArea
(
    taf_gnss_Constellation_t satConstellation,
    taf_gnss_ConstellationArea_t constellationArea
)
{
    le_result_t result = LE_FAULT;

    switch (GnssState)
    {
        case TAF_GNSS_STATE_ACTIVE:
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
        {
            // Set GNSS constellation area
            result = LE_OK; // by default all constellation is set in TelSDK
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
        case TAF_GNSS_STATE_ACTIVE:
        case TAF_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case TAF_GNSS_STATE_READY:
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

    mTtffEnabled = true;
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
#if 1 // Start Detailed Reports, if not already started
            if (!mStarted) {
                mStarted = true;
                int optInterval = 1000;
                mAcqRate = optInterval;
                mLocCmdResponseCb = std::make_shared<LocationCommandCallback>("startDetailedReports");
                mLocationManager->startDetailedReports((uint32_t)optInterval,
                        std::bind(&LocationCommandCallback::commandResponse,
                            mLocCmdResponseCb, std::placeholders::_1));
            }
#endif
            mStartTime = std::chrono::system_clock::now();
            std::unique_lock<std::mutex> lock(mMutex);
            mCondVar.wait(lock);
            std::chrono::duration<double> elapsedTime = mEndTime - mStartTime;
            *ttffPtr = elapsedTime.count() * 1e+6;
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
    taf_gnss_PositionHandler_t*  positionHandlerPtr = (taf_gnss_PositionHandler_t*)le_mem_ForceAlloc(PositionHandlerPoolRef);
    positionHandlerPtr->next = LE_DLS_LINK_INIT;
    positionHandlerPtr->handlerFuncPtr = handlerPtr;
    positionHandlerPtr->handlerContextPtr = contextPtr;
    positionHandlerPtr->sessionRef = taf_gnss_GetClientSessionRef();

    HandlerRef = le_event_AddHandler("LocUpdateEventId", positionEventId, GnssPositionHandler);
    le_event_SetContextPtr(HandlerRef, contextPtr);
    le_dls_Queue(&PositionHandlerList, &(positionHandlerPtr->next));
    NumOfPositionHandlers++;
    LE_DEBUG("Position handler %p added", HandlerRef);

    return (taf_gnss_PositionHandlerRef_t)positionHandlerPtr;
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
        case TAF_GNSS_STATE_ACTIVE:
            {
                LE_ERROR("Bad state for that request [%d]", GnssState);
                result = LE_NOT_PERMITTED;
            }
            break;
        case TAF_GNSS_STATE_READY:
            {
                // Get GNSS constellation
                //std::unique_lock<std::mutex> lock(mMutex);
                //mCondVar.wait(lock);
                *constellationMaskPtr = -1;//mConstellationMask;

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

taf_gnss_SampleRef_t taf_Gnss::GetLastSampleRef
(
    void
)
{
    auto &gnss = taf_Gnss::GetInstance();
    taf_gnss_PositionSampleRequest_t* posSampleReqPtr = (taf_gnss_PositionSampleRequest_t*)le_mem_ForceAlloc(PositionSampleRequestPoolRef);
    posSampleReqPtr->positionSampleNodePtr = (taf_gnss_PositionSample_t*)le_mem_ForceAlloc(PositionSamplePoolRef);

    memcpy(posSampleReqPtr->positionSampleNodePtr, &LastPositionSample, sizeof(taf_gnss_PositionSample_t));

    posSampleReqPtr->positionSampleNodePtr->next = LE_DLS_LINK_INIT;
    le_dls_Queue(&PositionSampleList, &(posSampleReqPtr->positionSampleNodePtr->next));

    LE_DEBUG("Get sample %p", posSampleReqPtr->positionSampleNodePtr);

    posSampleReqPtr->sessionRef = taf_gnss_GetClientSessionRef();

    taf_gnss_SampleRef_t reqRef = (taf_gnss_SampleRef_t)le_ref_CreateRef(gnss.PositionSampleMap, posSampleReqPtr);
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
        if ((false == posSampleReqPtr->positionSampleNodePtr->hSpeedAccuracyValid) ||
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
        if ((false == posSampleReqPtr->positionSampleNodePtr->vSpeedAccuracyValid) ||
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
    if ((!gpsTime) || (!currentLeapSeconds) || (!changeEventTime) || (!nextLeapSeconds))
    {
        LE_ERROR("Null pointer provided: gpsTime: %p, currentLeapSeconds: %p, "
                 "changeEventTimePtr: %p, nextLeapSecondsPtr: %p",
                  gpsTime, currentLeapSeconds, changeEventTime, nextLeapSeconds);

        return LE_FAULT;
    }

    return LE_OK;
    //GetLeapSeconds(gpsTime, currentLeapSeconds, changeEventTime, nextLeapSeconds);
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
            mAcqRate = rate;
            result = LE_OK;
        }
        break;
        case TAF_GNSS_STATE_UNINITIALIZED:
        case TAF_GNSS_STATE_ACTIVE:
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
                LE_DEBUG("ForceColdRestart called");
                mLocCmdResponseCb = std::make_shared<LocationCommandCallback>( "Delete All Aiding Data Cold Start");
                telux::common::Status status = mLocationConfigurator->deleteAllAidingData(
                        std::bind(&LocationCommandCallback::commandResponse, mLocCmdResponseCb, std::placeholders::_1));
                if (status == telux::common::Status::NOTIMPLEMENTED) {
                    LE_ERROR("ForceColdRestart failed or Not Implemented");
                    GnssState = TAF_GNSS_STATE_READY;
                    result = LE_FAULT;
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
                /* Specifies AidingDataType mask */
                /* 0 - EPHEMERIS 1 - DR_SENSOR_CALIBRATION
                   AidingData |1UL << (0,1) which is 3*/

                uint32_t AidingData = 3;
                mLocCmdResponseCb = std::make_shared<LocationCommandCallback>( "Delete Aiding Data warm Start");
                telux::common::Status status = mLocationConfigurator->deleteAidingData(AidingData,
                        std::bind(&LocationCommandCallback::commandResponse, mLocCmdResponseCb, std::placeholders::_1));
                if (status == telux::common::Status::NOTIMPLEMENTED) {
                    LE_ERROR("ForceColdRestart failed or Not Implemented");
                    GnssState = TAF_GNSS_STATE_READY;
                    result = LE_FAULT;
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
                    mLocCmdResponseCb = std::make_shared<LocationCommandCallback>("stopReports");
                    mLocationManager->stopReports(std::bind(&LocationCommandCallback::commandResponse,
                                mLocCmdResponseCb, std::placeholders::_1));
                    mStarted = false;
                }
                sleep(5); // 5sec sleep required to stop and start Gnss engine

                //start Detailed report
                if (!mStarted) {
                    mStarted = true;
                    int optInterval = mAcqRate;
                    if( optInterval == 0  || optInterval < 1000) {
                        LE_DEBUG("mAcqRate is zero, so set default to 1000ms");
                        optInterval = 1000;
                        mAcqRate = optInterval;
                    }
                    mLocCmdResponseCb = std::make_shared<LocationCommandCallback>("startDetailedReports");
                    mLocationManager->startDetailedReports((uint32_t)optInterval,
                            std::bind(&LocationCommandCallback::commandResponse,
                                mLocCmdResponseCb, std::placeholders::_1));
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
    return LE_OK;//GetSupportedConstellations(constellationMaskPtr);
}

le_result_t taf_Gnss::SetMinElevation
(
    uint8_t  minElevation
)
{
    TAF_ERROR_IF_RET_VAL( minElevation > TAF_GNSS_MIN_ELEVATION_MAX_DEGREE, LE_OUT_OF_RANGE, "minimum elevation is above maximal range");

    return LE_UNSUPPORTED;//No support to set MinElevation in TelSDK
}

le_result_t taf_Gnss::GetMinElevation
(
   uint8_t*  minElevationPtr
)
{
    TAF_KILL_CLIENT_IF_RET_VAL( NULL == minElevationPtr, LE_FAULT, "minElevationPtr is NULL");
    mMinElelvEnabled = true;
    std::unique_lock<std::mutex> lock(mMutex);
    mCondVar.wait(lock);
    *minElevationPtr =  mMinElev;

    return LE_OK;
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
                GnssState = TAF_GNSS_STATE_READY;
                /*mLocCmdResponseCb = std::make_shared<LocationCommandCallback>();
                  mLocationManager->stopReports(std::bind(&LocationCommandCallback::commandResponse,
                  mLocCmdResponseCb, std::placeholders::_1));*/
                result = LE_OK;
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

void taf_Gnss::RemovePositionHandler
(
    taf_gnss_PositionHandlerRef_t handlerRef
)
{
    auto &gnss = taf_Gnss::GetInstance();
    taf_gnss_PositionHandler_t* posHandlerPtr;
    le_dls_Link_t* linkPtr;

    linkPtr = le_dls_Peek(&gnss.PositionHandlerList);
    if (linkPtr != NULL)
    {
        do
        {
            posHandlerPtr =
                (taf_gnss_PositionHandler_t*)CONTAINER_OF(linkPtr, taf_gnss_PositionHandler_t, next);

            if ((taf_gnss_PositionHandlerRef_t)posHandlerPtr == handlerRef)
            {
                le_mem_Release(posHandlerPtr);
                NumOfPositionHandlers--;
                linkPtr=NULL;
            }
            else
            {
                linkPtr = le_dls_PeekNext(&gnss.PositionHandlerList, linkPtr);
            }
        } while (linkPtr != NULL);
    }
}

void taf_Gnss::PosDestructor
(
    void* obj
)
{
    auto &gnss = taf_Gnss::GetInstance();
    taf_gnss_PositionSample_t *positionSampleNodePtr;
    le_dls_Link_t   *linkPtr;

    LE_FATAL_IF((NULL == obj), "Position Sample Object does not exist!");

    linkPtr = le_dls_Peek(&gnss.PositionSampleList);
    if (linkPtr != NULL)
    {
        do
        {
            positionSampleNodePtr = (taf_gnss_PositionSample_t*)
                                    CONTAINER_OF(linkPtr, taf_gnss_PositionSample_t, next);
            if (positionSampleNodePtr == (taf_gnss_PositionSample_t*)obj)
            {
                le_dls_Remove(&gnss.PositionSampleList, linkPtr);
                linkPtr=NULL;
            }
            else
            {
                linkPtr = le_dls_PeekNext(&gnss.PositionSampleList, linkPtr);
            }
        } while (linkPtr != NULL);
    }
}

void taf_Gnss::PosHandleDestructor
(
    void* obj
)
{
    auto &gnss = taf_Gnss::GetInstance();
    taf_gnss_PositionHandler_t *posHandlerPtr;
    le_dls_Link_t *linkPtr;

    linkPtr = le_dls_Peek(&gnss.PositionHandlerList);
    if (linkPtr != NULL)
    {
        do
        {
            posHandlerPtr =
                (taf_gnss_PositionHandler_t*)CONTAINER_OF(linkPtr, taf_gnss_PositionHandler_t, next);

            if (posHandlerPtr == (taf_gnss_PositionHandler_t*)obj)
            {
                le_dls_Remove(&gnss.PositionHandlerList, linkPtr);
                linkPtr=NULL;
            }
            else
            {
                linkPtr = le_dls_PeekNext(&gnss.PositionHandlerList, linkPtr);
            }
        } while (linkPtr != NULL);
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
    // stop Detailed Reports
    if (gnss.mStarted) {
        gnss.mLocCmdResponseCb = std::make_shared<LocationCommandCallback>("stopReports");
        gnss.mLocationManager->stopReports(std::bind(&LocationCommandCallback::commandResponse,
                    gnss.mLocCmdResponseCb, std::placeholders::_1));
        gnss.mStarted = false;
    }

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

            taf_gnss_ReleaseSampleRef(safeRef);
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
    PositionHandlerList = LE_DLS_LIST_DECL_INIT;
    PositionSampleList = LE_DLS_LIST_DECL_INIT;
    SessionCtxList = LE_DLS_LIST_INIT;
    SessionCtxPool = NULL;
    HandlerPool = NULL;
    SessionRefPool = NULL;
    GnssState = TAF_GNSS_STATE_UNINITIALIZED;
    NumOfPositionHandlers = 0;
    memset(&LastPositionSample, 0, sizeof(LastPositionSample));
    LastPositionSample.fixState = TAF_GNSS_STATE_FIX_NO_POS;
    memset(&mSatInfo, 0, sizeof(mSatInfo));

    status = LocationManagerInit();
    if (status != telux::common::Status::SUCCESS) {
        LE_CRIT("LocationManager not available");
    }

    status = LocationConfiguratorInit();
    if (status != telux::common::Status::SUCCESS) {
        LE_CRIT("LocationConfigurator not available");
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
    le_mem_SetDestructor(PositionHandlerPoolRef, PosHandleDestructor);

    PositionSamplePoolRef = le_mem_InitStaticPool(PositionSample, GNSS_POSITION_SAMPLE_MAX,
            sizeof(taf_gnss_PositionSample_t));
    le_mem_SetDestructor(PositionSamplePoolRef, PosDestructor);

    PositionSampleRequestPoolRef = le_mem_InitStaticPool(PositionSampleRequest,
            GNSS_POSITION_SAMPLE_MAX, sizeof(taf_gnss_PositionSampleRequest_t));

    PositionSampleMap = le_ref_InitStaticMap(PositionSampleMap, GNSS_POSITION_SAMPLE_MAX);

    ClientRequestRefMap = le_ref_CreateMap("ClientRequestRefMap", TAF_CONFIG_POSITIONING_ACTIVATION_MAX);

    ClientPoolRef = le_mem_InitStaticPool(Client, TAF_CONFIG_POSITIONING_ACTIVATION_MAX, sizeof(taf_gnss_Client_t));

    positionEventId = le_event_CreateIdWithRefCounting("positionEventId");

    le_msg_ServiceRef_t msgService = taf_gnss_GetServiceRef();
    le_msg_AddServiceCloseHandler(msgService, CloseEventHandler, NULL);
    mGnssMutexRef =  le_mutex_CreateRecursive("GnssMutex");

    return;
}
