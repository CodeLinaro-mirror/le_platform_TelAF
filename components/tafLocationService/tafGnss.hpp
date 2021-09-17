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
#include <algorithm>
#include <cctype>
#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <telux/loc/LocationConfigurator.hpp>
#include <telux/loc/LocationDefines.hpp>
#include <telux/loc/DgnssManager.hpp>
#include "telux/common/CommonDefines.hpp"
#include <telux/loc/LocationManager.hpp>
#include <telux/loc/LocationListener.hpp>
#include "tafSvcIF.hpp"

using namespace telux::loc;
using GnssReportTypeMask = uint32_t;
#define TAF_CONFIG_POSITIONING_ACTIVATION_MAX 13
#define GNSS_POSITION_SAMPLE_MAX         1
#define GNSS_POSITION_HANDLER_HIGH       1

namespace telux {
namespace tafsvc {

    typedef struct {
        uint16_t satId;
        int32_t  satLatency;
    }
    taf_gnss_SvMeas_t;

    typedef struct {
        taf_gnss_Constellation_t satConst;
        bool      satUsed;
        bool      satTracked;
        uint8_t   satSnr;
        uint8_t   satElev;
        uint16_t  satId;
        uint16_t  satAzim;
    }
    taf_gnss_SvInfo_t;

    typedef struct taf_gnss_PositionSample
    {
        taf_gnss_FixState_t fixState;
        bool      latitudeValid;
        bool      longitudeValid;
        bool      hAccuracyValid;
        bool      altitudeValid;
        bool      altitudeOnWgs84Valid;
        bool      horUncEllipseSemiMajorValid;
        bool      horUncEllipseSemiMinorValid;
        bool      horConfidenceValid;
        bool      vAccuracyValid;
        bool      hSpeedValid;
        bool      hSpeedAccuracyValid;
        bool      vSpeedValid;
        bool      vSpeedAccuracyValid;
        bool      directionValid;
        bool      directionAccuracyValid;
        bool      dateValid;
        bool      timeValid;
        bool      gpsTimeValid;
        bool      timeAccuracyValid;
        bool      leapSecondsValid;
        bool      positionLatencyValid;
        bool      hdopValid;
        bool      vdopValid;
        bool      pdopValid;
        bool      gdopValid;
        bool      tdopValid;
        bool      magneticDeviationValid;
        bool      satsInViewCountValid;
        bool      satsTrackingCountValid;
        bool      satsUsedCountValid;
        bool      satInfoValid;
        bool      satMeasValid;
        int32_t   latitude;
        int32_t   longitude;
        int32_t   hAccuracy;
        int32_t   altitude;
        int32_t   altitudeOnWgs84;
        int32_t   vAccuracy;
        int32_t   hSpeedAccuracy;
        int32_t   vSpeed;
        int32_t   vSpeedAccuracy;
        int32_t   magneticDeviation;
        uint64_t  epochTime;
        uint32_t  horUncEllipseSemiMajor;
        uint32_t  horUncEllipseSemiMinor;
        uint32_t  hSpeed;
        uint32_t  direction;
        uint32_t  directionAccuracy;
        uint32_t  gpsWeek;
        uint32_t  gpsTimeOfWeek;
        uint32_t  timeAccuracy;
        uint32_t  positionLatency;
        uint32_t  hdop;
        uint32_t  vdop;
        uint32_t  pdop;
        uint32_t  gdop;
        uint32_t  tdop;
        uint16_t  day;
        uint16_t  month;
        uint16_t  year;
        uint16_t  milliseconds;
        uint16_t  seconds;
        uint16_t  minutes;
        uint16_t  hours;
        uint8_t   horConfidence;
        uint8_t   leapSeconds;
        uint8_t   satsInViewCount;
        uint8_t   satsTrackingCount;
        uint8_t   satsUsedCount;
        taf_gnss_SvInfo_t  satInfo[TAF_GNSS_SV_INFO_MAX_LEN];
        taf_gnss_SvMeas_t  satMeas[TAF_GNSS_SV_INFO_MAX_LEN];
        le_dls_Link_t   next;
    }
    taf_gnss_PositionSample_t;

    typedef struct taf_gnss_PositionHandler
    {
        taf_gnss_PositionHandlerRef_t handlerRef;
        taf_gnss_PositionHandlerFunc_t handlerFuncPtr;
        void*                         handlerContextPtr;
        le_msg_SessionRef_t           sessionRef;
        le_dls_Link_t                 next;
    }
    taf_gnss_PositionHandler_t;

    typedef struct
    {
        taf_gnss_SampleRef_t             positionSampleRef;
        taf_gnss_PositionSample_t*       positionSampleNodePtr;
        le_msg_SessionRef_t             sessionRef;
        le_dls_Link_t                   next;
    }
    taf_gnss_PositionSampleRequest_t;

    typedef struct
    {
        void*                       clientRefPtr;
        le_msg_SessionRef_t         sessionRef;
        taf_gnss_Resolution_t        dopResolution;
        taf_gnss_Resolution_t        vAccuracyResolution;
        taf_gnss_Resolution_t        vSpeedAccuracyResolution;
        taf_gnss_Resolution_t        hSpeedAccuracyResolution;
        le_dls_Link_t               next;
    }
    taf_gnss_Client_t;

    class LocationCommandCallback : public telux::common::ICommandResponseCallback {
        public:
            void commandResponse(telux::common::ErrorCode error);

            void onGnssEnergyConsumedInfo(telux::loc::GnssEnergyConsumedInfo gnssEnergyConsumed,
                    telux::common::ErrorCode error);

            void onGetYearOfHwInfo(uint16_t yearOfHw, telux::common::ErrorCode error);

            void onMinGpsWeekInfo(uint16_t minGpsWeek, telux::common::ErrorCode error);

            void onMinSVElevationInfo(uint8_t minSVElevation, telux::common::ErrorCode error);

            void onRobustLocationInfo(const telux::loc::RobustLocationConfiguration rLConfig,
                    telux::common::ErrorCode error);

            void onSecondaryBandInfo(const telux::loc::ConstellationSet set,
                    telux::common::ErrorCode error);

            void onTerrestrialPositionInfo(const std::shared_ptr<
                    telux::loc::ILocationInfoBase> locationInfo);
    };

    class tafLocationListener : public telux::loc::ILocationListener,
    public telux::loc::ILocationSystemInfoListener {
        public:
            void onBasicLocationUpdate(
                    const std::shared_ptr<telux::loc::ILocationInfoBase> &locationInfo) override;

            void onDetailedLocationUpdate(
                    const std::shared_ptr<telux::loc::ILocationInfoEx> &locationInfo) override;

            void onGnssSVInfo(const std::shared_ptr<telux::loc::IGnssSVInfo> &gnssSVInfo) override;

            void onGnssSignalInfo(const std::shared_ptr<telux::loc::IGnssSignalInfo> &gnssDatainfo) override;

            void onGnssNmeaInfo(uint64_t timestamp, const std::string &nmea) override;

            void onDetailedEngineLocationUpdate(const std::vector<std::shared_ptr<telux::loc::ILocationInfoEx>>
                    &locationEngineInfo) override;

            void onGnssMeasurementsInfo(const telux::loc::GnssMeasurements &measurementInfo) override;

            void onLocationSystemInfo(const telux::loc::LocationSystemInfo &locationSystemInfo) override;

            ~tafLocationListener() {};
    };

    class GnssListener : public IDgnssStatusListener {
        public:
            void onDgnssStatusUpdate(DgnssStatus status) override;
    };

    class taf_Gnss: public ITafSvc
    {
        public:
            taf_Gnss() {};
            ~taf_Gnss();
            void Init();
            static taf_Gnss &GetInstance();
            static void PosHandleDestructor( void* obj);
            static void PosDestructor( void* obj);
            static le_result_t CheckValidatePosition(
                    taf_gnss_PositionSampleRequest_t* positionSampleRequestNodePtr);
            static le_result_t PositionDataCoversion(int32_t value, taf_gnss_DataType_t dataType,int32_t* valuePtr);
            static taf_gnss_Client_t* DiscoverSessionRef( le_msg_SessionRef_t sessionRef);
            static void GnssPositionHandler(void* reportPtr);

            taf_gnss_PositionHandlerRef_t AddPositionHandler(
                    taf_gnss_PositionHandlerFunc_t handlerPtr, void* contextPtr);
            void RemovePositionHandler(taf_gnss_PositionHandlerRef_t handlerRef);
            taf_gnss_SampleRef_t GetLastSampleRef(void);
            void ReleaseClientRef( void* RefPtr);
            static void CloseEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);
            le_result_t GetPositionState( taf_gnss_SampleRef_t positionSampleRef,
                    taf_gnss_FixState_t* statePtr);
            void ReleaseSampleRef(taf_gnss_SampleRef_t positionSampleRef);
            le_result_t GetDate( taf_gnss_SampleRef_t positionSampleRef, uint16_t* yearPtr,
                    uint16_t* monthPtr, uint16_t* dayPtr);
            le_result_t GetTime( taf_gnss_SampleRef_t positionSampleRef, uint16_t* hoursPtr,
                    uint16_t* minutesPtr, uint16_t* secondsPtr, uint16_t* millisecondsPtr );
            le_result_t GetLocation( taf_gnss_SampleRef_t positionSampleRef, int32_t* latitudePtr,
                    int32_t* longitudePtr, int32_t* hAccuracyPtr);
            le_result_t GetDirection( taf_gnss_SampleRef_t positionSampleRef, uint32_t* directionPtr, uint32_t* directionAccuracyPtr);

            le_result_t GetAltitude( taf_gnss_SampleRef_t positionSampleRef, int32_t* altitudePtr,
                    int32_t* vAccuracyPtr );
            le_result_t GetHorizontalSpeed( taf_gnss_SampleRef_t positionSampleRef, uint32_t* hspeedPtr,
                    uint32_t* hspeedAccuracyPtr );
            le_result_t GetVerticalSpeed( taf_gnss_SampleRef_t positionSampleRef, int32_t* vspeedPtr,
                    int32_t* vspeedAccuracyPtr );
            le_result_t GetGpsLeapSeconds( taf_gnss_SampleRef_t positionSampleRef, uint8_t* leapSecondsPtr);

            le_result_t Enable(void);
            le_result_t SetConstellation(taf_gnss_ConstellationBitMask_t constellationMask);
            le_result_t Start(void);
            le_result_t SetConstellationArea( taf_gnss_Constellation_t satConstellation,
                    taf_gnss_ConstellationArea_t constellationArea);

            le_result_t GetConstellation( taf_gnss_ConstellationBitMask_t *constellationMaskPtr);
            le_result_t Disable(void);
            le_result_t Stop(void);
            taf_gnss_State_t GetState( void);

            le_result_t GetSatellitesStatus( taf_gnss_SampleRef_t positionSampleRef, uint8_t* satsInViewCountPtr,
                    uint8_t* satsTrackingCountPtr, uint8_t* satsUsedCountPtr);
            le_result_t GetAcquisitionRate( uint32_t* ratePtr);
            le_result_t GetTtff( uint32_t* ttffPtr);
            le_result_t GetSatellitesInfo(taf_gnss_SampleRef_t positionSampleRef, uint16_t* satIdPtr,
                    size_t* satIdNumElementsPtr, taf_gnss_Constellation_t* satConstPtr,
                    size_t* satConstNumElementsPtr, bool* satUsedPtr, size_t* satUsedNumElementsPtr,
                    uint8_t* satSnrPtr,size_t* satSnrNumElementsPtr,  uint16_t* satAzimPtr,
                    size_t* satAzimNumElementsPtr, uint8_t* satElevPtr, size_t* satElevNumElementsPtr);
            le_mem_PoolRef_t   PositionHandlerPoolRef;
            le_mem_PoolRef_t   PositionSampleRequestPoolRef;
            le_event_Id_t positionEventId;
            le_mem_PoolRef_t   PositionSamplePoolRef;
            taf_gnss_PositionSample_t   LastPositionSample;
            std::chrono::time_point<std::chrono::system_clock> mStartTime;
            std::chrono::time_point<std::chrono::system_clock> mEndTime;
            std::condition_variable mCondVar;
            std::mutex mMutex;
            uint32_t mTtffPtr;
            int32_t NumOfPositionHandlers;
            uint8_t mLeapSeconds = 0;
            bool mStarted = false;
            bool mTtffEnabled = false;
            bool mConstellationEnabled = false;
            taf_gnss_ConstellationBitMask_t mConstellationMask;

        private:
            std::shared_ptr<ILocationManager> mLocationManager = nullptr;
            std::shared_ptr<ILocationConfigurator> mLocationConfigurator = nullptr;
            std::shared_ptr<LocationCommandCallback> mLocCmdResponseCb = nullptr;
            std::shared_ptr<tafLocationListener> mPosListener = nullptr;
            std::shared_ptr<ILocationInfoBase> mBaselocationInfo = nullptr;
            telux::common::Status LocationConfiguratorInit();
            telux::common::Status LocationManagerInit();
            telux::common::Status DgnssManagerInit();
            std::shared_ptr<IDgnssManager> mDgnssManager = nullptr;
            le_mem_PoolRef_t   ClientPoolRef;
            le_ref_MapRef_t PositionSampleMap;
            le_ref_MapRef_t ClientRequestRefMap;
            le_dls_List_t PositionHandlerList;
            le_dls_List_t PositionSampleList;
            le_dls_List_t    SessionCtxList;
            taf_gnss_State_t GnssState;
            taf_gnss_SampleRef_t positionSampleRef;
            le_event_HandlerRef_t HandlerRef;
            le_mem_PoolRef_t HandlerPool;
            le_mem_PoolRef_t SessionCtxPool;
            le_mem_PoolRef_t SessionRefPool;
    };
    }
}
