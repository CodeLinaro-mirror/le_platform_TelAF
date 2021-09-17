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

telux::common::Status taf_Gnss::LocationManagerInit()
{
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

telux::common::Status taf_Gnss::LocationConfiguratorInit()
{
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

void taf_Gnss::GnssPositionHandler
(
 void* reportPtr
)
{
    auto &gnss = taf_Gnss::GetInstance();
    taf_gnss_PositionHandler_t*  posHandlerPtr;
    le_dls_Link_t*              linkPtr;
    uint8_t i;
    taf_gnss_PositionSampleRequest_t*    posSampleReqPtr=NULL;
    taf_gnss_SampleRef_t* positionPtr = (taf_gnss_SampleRef_t*)reportPtr;

    TAF_ERROR_IF_RET_NIL( positionPtr == NULL, "positionPtr is Null");

    LE_DEBUG("Handler Function called with position %p", positionPtr);

    if(!gnss.NumOfPositionHandlers)
    {
        LE_DEBUG("No positioning handlers, exit Handler Function");
        le_mem_Release(positionPtr);
        return;
    }

    linkPtr = le_dls_Peek(&gnss.PositionHandlerList);
    if (NULL != linkPtr)
    {
        posSampleReqPtr =
            (taf_gnss_PositionSampleRequest_t*)le_mem_ForceAlloc(gnss.PositionSampleRequestPoolRef);

        posSampleReqPtr->positionSampleNodePtr =
            (taf_gnss_PositionSample_t*)le_mem_ForceAlloc(gnss.PositionSamplePoolRef);

        // No need to add reference for singele Handler
        for(i=0 ; i<gnss.NumOfPositionHandlers-1 ; i++)
        {
            le_mem_AddRef((void *)posSampleReqPtr);
            le_mem_AddRef((void *)posSampleReqPtr->positionSampleNodePtr);
        }

        memcpy(posSampleReqPtr->positionSampleNodePtr, &gnss.LastPositionSample,
                sizeof(taf_gnss_PositionSample_t));

        posSampleReqPtr->positionSampleNodePtr->next = LE_DLS_LINK_INIT;
        le_dls_Queue(&gnss.PositionSampleList, &(posSampleReqPtr->
                    positionSampleNodePtr->next));

        do
        {
            posHandlerPtr =
                (taf_gnss_PositionHandler_t*)CONTAINER_OF(linkPtr, taf_gnss_PositionHandler_t, next);

            LE_DEBUG("Report sample %p to the corresponding handler (handler %p)",
                    posSampleReqPtr->positionSampleNodePtr,
                    posHandlerPtr->handlerFuncPtr);

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
}

void tafLocationListener::onBasicLocationUpdate(const std::shared_ptr<telux::loc::ILocationInfoBase> &locationInfo) {
    LE_DEBUG("*** Basic Location Report ***");
}

void tafLocationListener::onDetailedLocationUpdate(const std::shared_ptr<telux::loc::ILocationInfoEx> &locationInfo) {
    LE_DEBUG("**** Detailed Location Report ****");
    auto &gnss = taf_Gnss::GetInstance();

    if (gnss.mTtffEnabled) {
        gnss.mEndTime = std::chrono::system_clock::now();
        std::unique_lock<std::mutex> lock(gnss.mMutex);
        gnss.mTtffEnabled = false;
        gnss.mCondVar.notify_one();
    }

    if(gnss.NumOfPositionHandlers)
    {
        uint8_t i;
        gnss.LastPositionSample.fixState = TAF_GNSS_STATE_FIX_2D;
        gnss.LastPositionSample.latitudeValid = true;
        gnss.LastPositionSample.longitudeValid = true;
        gnss.LastPositionSample.hAccuracyValid = true;
        gnss.LastPositionSample.altitudeValid = true;
        gnss.LastPositionSample.altitudeOnWgs84Valid = false;
        gnss.LastPositionSample.horUncEllipseSemiMajorValid = false;
        gnss.LastPositionSample.horUncEllipseSemiMinorValid = false;
        gnss.LastPositionSample.horConfidenceValid = false;
        gnss.LastPositionSample.vAccuracyValid = true;
        gnss.LastPositionSample.hSpeedValid = true;
        gnss.LastPositionSample.hSpeedAccuracyValid = true;
        gnss.LastPositionSample.vSpeedValid = true;
        gnss.LastPositionSample.vSpeedAccuracyValid = true;
        gnss.LastPositionSample.directionValid = false;
        gnss.LastPositionSample.directionAccuracyValid = false;
        gnss.LastPositionSample.dateValid = true;
        gnss.LastPositionSample.timeValid = true;
        gnss.LastPositionSample.gpsTimeValid = true;
        gnss.LastPositionSample.timeAccuracyValid = true;
        gnss.LastPositionSample.leapSecondsValid = true;
        gnss.LastPositionSample.positionLatencyValid = false;
        gnss.LastPositionSample.hdopValid = true;
        gnss.LastPositionSample.vdopValid = true;
        gnss.LastPositionSample.pdopValid = true;
        gnss.LastPositionSample.gdopValid = true;
        gnss.LastPositionSample.tdopValid = true;
        gnss.LastPositionSample.magneticDeviationValid = true;
        gnss.LastPositionSample.satsInViewCountValid = false;
        gnss.LastPositionSample.satsTrackingCountValid = false;
        gnss.LastPositionSample.satsUsedCountValid = false;
        gnss.LastPositionSample.satInfoValid = false;
        gnss.LastPositionSample.satMeasValid = false;
        //convert float to int32_t and pass to IPC
        gnss.LastPositionSample.latitude = locationInfo->getLatitude() * 1e+6;
        gnss.LastPositionSample.longitude = locationInfo->getLongitude() * 1e+6;
        gnss.LastPositionSample.hAccuracy = locationInfo->getHorizontalUncertainty()* 1e+2;
        gnss.LastPositionSample.altitude = locationInfo->getAltitude() * 1e+3;
        gnss.LastPositionSample.vAccuracy = locationInfo->getVerticalUncertainty() * 10;
        gnss.LastPositionSample.altitudeOnWgs84 = 0;
        gnss.LastPositionSample.hSpeedAccuracy = 0;
        gnss.LastPositionSample.vSpeed = locationInfo->getSpeed();
        gnss.LastPositionSample.vSpeedAccuracy = locationInfo->getSpeedUncertainty();
        gnss.LastPositionSample.magneticDeviation = locationInfo->getMagneticDeviation();
        gnss.LastPositionSample.epochTime = 0;
        gnss.LastPositionSample.horUncEllipseSemiMajor = 0;
        gnss.LastPositionSample.horUncEllipseSemiMinor = 0;
        gnss.LastPositionSample.hSpeed = 0;
        gnss.LastPositionSample.direction = 0;
        gnss.LastPositionSample.directionAccuracy = 0;
        telux::loc::SystemTime sysTime = locationInfo->getGnssSystemTime();
        telux::loc::GnssSystem system = sysTime.gnssSystemTimeSrc;
        telux::loc::SystemTimeInfo sysTimeInfo = sysTime.time;
        if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GPS) {
            telux::loc::TimeInfo timeInfo = sysTimeInfo.gps;
            gnss.LastPositionSample.gpsWeek = timeInfo.systemWeek;
            gnss.LastPositionSample.gpsTimeOfWeek = timeInfo.systemMsec;
        }else {
            gnss.LastPositionSample.gpsWeek = 0;
            gnss.LastPositionSample.gpsTimeOfWeek = 0;
        }
        gnss.LastPositionSample.timeAccuracy = locationInfo->getTimeUncMs();
        gnss.LastPositionSample.positionLatency = 0;
        gnss.LastPositionSample.hdop = locationInfo->getHorizontalDop();
        gnss.LastPositionSample.vdop = locationInfo->getVerticalDop();
        gnss.LastPositionSample.pdop = locationInfo->getPositionDop();
        gnss.LastPositionSample.gdop = locationInfo->getGeometricDop();
        gnss.LastPositionSample.tdop = locationInfo->getTimeDop();
        if(locationInfo->getTimeStamp() != telux::loc::UNKNOWN_TIMESTAMP) {
            time_t realtime;
            realtime = (time_t)((locationInfo->getTimeStamp() / 1000));
            tm *ltm = localtime(&realtime);
            gnss.LastPositionSample.year = 1900+ltm->tm_year;
            gnss.LastPositionSample.month = 1+ltm->tm_mon;
            gnss.LastPositionSample.day = ltm->tm_mday;
            gnss.LastPositionSample.hours = 5+ltm->tm_hour;
            gnss.LastPositionSample.minutes = 30+ltm->tm_min;
            gnss.LastPositionSample.seconds = ltm->tm_sec;
            gnss.LastPositionSample.milliseconds = 0;
        } else {
            LE_DEBUG("Time stamp Not Valid");
            gnss.LastPositionSample.year = 0;
            gnss.LastPositionSample.month = 0;
            gnss.LastPositionSample.day = 0;
            gnss.LastPositionSample.hours = 0;
            gnss.LastPositionSample.minutes = 0;
            gnss.LastPositionSample.seconds = 0;
            gnss.LastPositionSample.milliseconds = 0;
        }
        gnss.LastPositionSample.horConfidence = 0;
        if(locationInfo->getLeapSeconds(gnss.mLeapSeconds) == telux::common::Status::SUCCESS) {
            gnss.LastPositionSample.leapSeconds = gnss.mLeapSeconds;
        } else {
            gnss.LastPositionSample.leapSeconds = 0;
        }
        gnss.LastPositionSample.satsInViewCount = 0;
        gnss.LastPositionSample.satsTrackingCount = 0;
        gnss.LastPositionSample.satsUsedCount = 0;

        for(i=0; i<TAF_GNSS_SV_INFO_MAX_LEN; i++)
        {
            gnss.LastPositionSample.satInfo[i].satId = 0;
            gnss.LastPositionSample.satInfo[i].satConst = TAF_GNSS_SV_CONSTELLATION_UNDEFINED;
            gnss.LastPositionSample.satInfo[i].satUsed = 0;
            gnss.LastPositionSample.satInfo[i].satTracked = 0;
            gnss.LastPositionSample.satInfo[i].satSnr = 0;
            gnss.LastPositionSample.satInfo[i].satAzim = 0;
            gnss.LastPositionSample.satInfo[i].satElev = 0;
        }

        for(i=0; i<TAF_GNSS_SV_INFO_MAX_LEN; i++)
        {
            gnss.LastPositionSample.satMeas[i].satId = 0;
            gnss.LastPositionSample.satMeas[i].satLatency = 0;
        }
        gnss.LastPositionSample.next = LE_DLS_LINK_INIT;

        le_event_Report(gnss.positionEventId, &gnss.LastPositionSample, sizeof(taf_gnss_PositionSample_t));
    }

}

void tafLocationListener::onDetailedEngineLocationUpdate(
      const std::vector<std::shared_ptr<telux::loc::ILocationInfoEx> > &locationEngineInfo) {
    LE_DEBUG("**** Detailed Engine Location Report ****");
}

void tafLocationListener::onGnssSVInfo(const std::shared_ptr<telux::loc::IGnssSVInfo> &gnssSVInfo) {
    LE_DEBUG("**** Satellite Vehicle Information ****");
    auto &gnss = taf_Gnss::GetInstance();
    if(gnss.mConstellationEnabled) {
        gnss.mConstellationEnabled = false;
        std::unique_lock<std::mutex> lock(gnss.mMutex);
        for(auto svInfo : gnssSVInfo->getSVInfoList()) {
            switch(svInfo->getConstellation()) {
                case telux::loc::GnssConstellationType::GPS:
                    gnss.mConstellationMask = TAF_GNSS_CONSTELLATION_GPS;
                    break;
                case telux::loc::GnssConstellationType::GLONASS:
                    gnss.mConstellationMask = TAF_GNSS_CONSTELLATION_GLONASS;
                    break;
                case telux::loc::GnssConstellationType::BDS:
                    gnss.mConstellationMask = TAF_GNSS_CONSTELLATION_BEIDOU;
                    break;
                case telux::loc::GnssConstellationType::GALILEO:
                    gnss.mConstellationMask = TAF_GNSS_CONSTELLATION_GALILEO;
                    break;
                case telux::loc::GnssConstellationType::SBAS:
                    gnss.mConstellationMask = TAF_GNSS_CONSTELLATION_SBAS;
                    break;
                case telux::loc::GnssConstellationType::QZSS:
                    gnss.mConstellationMask = TAF_GNSS_CONSTELLATION_QZSS;
                    break;
                default:
                    gnss.mConstellationMask = -1;
                    LE_ERROR("Constellation type: UNKNOWN");
                    break;
            }
        }
        gnss.mCondVar.notify_one();
    }
}

void tafLocationListener::onGnssSignalInfo(
   const std::shared_ptr<telux::loc::IGnssSignalInfo> &gnssDatainfo) {
   LE_DEBUG("**** Gnss Signal Information ****" );
}

void tafLocationListener::onGnssNmeaInfo(uint64_t timestamp, const std::string &nmea) {
   LE_DEBUG( "**** Gnss Nmea Information ****" );
}

void tafLocationListener::onGnssMeasurementsInfo(const telux::loc::
     GnssMeasurements &measurementInfo) {
   LE_DEBUG("**** Gnss Measurements Information ****");
}

void tafLocationListener::onLocationSystemInfo(const telux::loc::LocationSystemInfo
     &locationSystemInfo) {
   LE_DEBUG( "**** Location System Information ****" );
}

void LocationCommandCallback::commandResponse(telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      LE_DEBUG("commandResponse sent successfully");
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

taf_gnss_State_t taf_Gnss::GetState
(
 void
)
{
    mStarted = true;
#if 1 // Start Detailed Reports
            int optInterval = 10000;
            mLocCmdResponseCb = std::make_shared<LocationCommandCallback>();
            mLocationManager->startDetailedReports((uint32_t)optInterval,
                    std::bind(&LocationCommandCallback::commandResponse,
                        mLocCmdResponseCb, std::placeholders::_1));
            mStartTime = std::chrono::system_clock::now();
#endif
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
            mLocCmdResponseCb = std::make_shared<LocationCommandCallback> ();
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
            //result = pa_gnss_SetConstellationArea(satConstellation, constellationArea);
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
            *satsInViewCountPtr = posSampleReqPtr->positionSampleNodePtr->
                                                                satsInViewCount;
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
            //result = pa_gnss_GetAcquisitionRate(ratePtr);
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
                    satConstPtr[i] = posSampleReqPtr->positionSampleNodePtr->
                        satInfo[i].satConst;
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
                    satUsedPtr[i] = posSampleReqPtr->positionSampleNodePtr->
                        satInfo[i].satUsed;
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
                    satSnrPtr[i] = posSampleReqPtr->positionSampleNodePtr->
                        satInfo[i].satSnr;
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
                    satAzimPtr[i] = posSampleReqPtr->positionSampleNodePtr->
                        satInfo[i].satAzim;
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
                    satElevPtr[i] = posSampleReqPtr->positionSampleNodePtr->
                        satInfo[i].satElev;
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
                int optInterval = 1000;
                mLocCmdResponseCb = std::make_shared<LocationCommandCallback>();
                mLocationManager->startDetailedReports((uint32_t)optInterval,
                        std::bind(&LocationCommandCallback::commandResponse,
                            mLocCmdResponseCb, std::placeholders::_1));
                mStartTime = std::chrono::system_clock::now();
            }
#endif
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
    mConstellationEnabled = true;

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
                std::unique_lock<std::mutex> lock(mMutex);
                mCondVar.wait(lock);
                *constellationMaskPtr = mConstellationMask;

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
    LE_ERROR("SessionRef (%p) has been closed", sessionRef);

    TAF_ERROR_IF_RET_NIL( sessionRef == NULL, "sessionRef is NULL");
    // stop Detailed Reports
    gnss.mLocCmdResponseCb = std::make_shared<LocationCommandCallback>();
    gnss.mLocationManager->stopReports(std::bind(&LocationCommandCallback::commandResponse,
                gnss.mLocCmdResponseCb, std::placeholders::_1));
    gnss.mStarted = false;

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

    status = LocationManagerInit();
    if (status != telux::common::Status::SUCCESS) {
        LE_CRIT("LocationManager not available");
    }

    status = LocationConfiguratorInit();
    if (status != telux::common::Status::SUCCESS) {
        LE_CRIT("LocationConfigurator not available");
    }

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

    positionEventId = le_event_CreateId("positionEventId", sizeof(taf_gnss_PositionSample_t));

    le_msg_ServiceRef_t msgService = taf_gnss_GetServiceRef();
    le_msg_AddServiceCloseHandler(msgService, CloseEventHandler, NULL);

    // UnComment this code for injection of RTCM3 format data
    /*telux::common::Status status = telux::common::Status::FAILED;
      status = DgnssManagerInit();
      if (status != telux::common::Status::SUCCESS) {
      LE_CRIT("Gnss not available");
      }*/

    return;
}
