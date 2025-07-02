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
using LocReqEngine = uint16_t;
const int DEFAULT_UNKNOWN = 0;
#define TAF_CONFIG_POSITIONING_ACTIVATION_MAX 13
#define GNSS_POSITION_SAMPLE_MAX         1
#define GNSS_POSITION_HANDLER_HIGH       1
#define DEFAULT_TIMEOUT_IN_SECONDS 5
#define VERTICAL_SPEED_SIZE 3
#define VERTICAL_SPEED_ACCURACY_INDEX 2
#define LENGTH_CFG_NODE 50
#define TTFF_REPORT_COUNT 10
#define TAF_LOCGNSS_NMEA_DEFAULT 0x1f8000fc0
#define TAF_LOCGNSS_NMEA_CONFIG_DEFAULT 0

enum DataType
{
    TAF_LOCGNSS_DATA_VACCURACY,
    TAF_LOCGNSS_DATA_VSPEEDACCURACY,
    TAF_LOCGNSS_DATA_HSPEEDACCURACY,
    TAF_LOCGNSS_DATA_UNKNOWN
};

enum AidingDataType
{
    TAF_LOCGNSS_AIDING_DATA_EPHEMERIS = (1<<0),
    TAF_LOCGNSS_AIDING_DATA_DR_SENSOR_CALIBRATION = (1<<1)
};

namespace tafsvc {

    typedef struct {
        uint16_t satId;
        int32_t  satLatency;
    }
    taf_locGnss_SvMeas_t;

    typedef struct taf_locGnss_PositionSample
    {
        taf_locGnss_FixState_t fixState;
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
        taf_locGnss_GnssMeasurementInfo_t measInfo[TAF_LOCGNSS_MEASUREMENT_INFO_MAX];
        uint8_t   measInfoCount;
        uint16_t SVIds[TAF_LOCGNSS_MEASUREMENT_INFO_MAX];
        uint8_t   SVIdsCount;
        taf_locGnss_ReportStatus_t reportStatus;
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
        taf_locGnss_SvInfo_t  satInfo[TAF_LOCGNSS_SV_INFO_MAX_LEN];
        taf_locGnss_SvMeas_t  satMeas[TAF_LOCGNSS_SV_INFO_MAX_LEN];
        float    robustConformity;
        bool     conformityValid;
        uint8_t  confidencePercent;
        bool     confidencePercentValid;
        uint8_t  calibrationStatus;
        uint8_t  calibrationStatusValid;
        uint32_t  drSolutionStatus;
        uint32_t  drSolutionStatusValid;
        taf_locGnss_KinematicsData_t GnssKinematicsData;
        bool     GnssKinematicsDataValid;
        double   vrpLatitude;
        bool     vrpLatitudeValid;
        double   vrpLongitude;
        bool     vrpLongitudeValid;
        double   vrpAltitude;
        bool     vrpAltitudeValid;
        double   eastVel;
        bool     eastVelValid;
        double   northVel;
        bool     northVelValid;
        double   upVel;
        bool     upVelValid;
        taf_locGnss_SvUsedInPosition_t svData;
        bool     svDataValid;
        uint32_t sbasMask;
        bool     sbasMaskValid;
        uint32_t validityMask;
        bool     validityMaskValid;
        uint64_t validityExMask;
        bool     validityExMaskValid;
        uint16_t engMask;
        bool     engMaskValid;
        uint16_t locationEngType;
        bool     locationEngTypeValid;
        uint16_t horiReliablity;
        bool     horiReliablityValid;
        uint16_t vertReliablity;
        bool     vertReliablityValid;
        double   azimuth;
        bool     azimuthValid;
        double   eastDev;
        bool     eastDevValid;
        double   northDev;
        bool     northDevValid;
        uint64_t realTime;
        bool realTimeValid;
        uint64_t realTimeUnc;
        bool realTimeUncValid;
        uint32_t techMask;
        uint32_t techMaskValid;
        double   altMeanSeaLevel;
        taf_locGnss_GnssData_t gnssData[TAF_LOCGNSS_NUMBER_OF_SIGNAL_TYPES_MAX];
        bool  gnssDataValid;
        le_msg_SessionRef_t*         clientSessionRefPtr;
        bool gPtpTimeValid;
        uint64_t gPtpTime;
        bool gPtpTimeUncValid;
        uint64_t gPtpTimeUnc;
        bool   leapSecondsUncValid;
        uint8_t leapSecondsUnc;
        le_dls_Link_t   next;
    }
    taf_locGnss_PositionSample_t;

    typedef struct taf_locGnss_PositionHandler
    {
        taf_locGnss_PositionHandlerRef_t handlerRef;
        taf_locGnss_PositionHandlerFunc_t handlerFuncPtr;
        void*                         handlerContextPtr;
        le_msg_SessionRef_t           sessionRef;
        le_dls_Link_t                 next;
    }
    taf_locGnss_PositionHandler_t;

    typedef struct
    {
        taf_locGnss_LocCapabilityType_t locCapability;
    }
    CapabilityChangeEvent_t;

    typedef struct
    {
        uint64_t timestamp;
        char nmeaMask[TAF_LOCGNSS_NMEA_STRING_MAX];
    }
    NmeaInfoEvent_t;

    typedef struct
    {
        taf_locGnss_SampleRef_t             positionSampleRef;
        taf_locGnss_PositionSample_t*       positionSampleNodePtr;
        le_msg_SessionRef_t             sessionRef;
        le_dls_Link_t                   next;
    }
    taf_locGnss_PositionSampleRequest_t;

    class tafLocationListener : public telux::loc::ILocationListener,
    public telux::loc::ILocationConfigListener,public telux::loc::ILocationSystemInfoListener {
        public:

            void onGnssSVInfo(const std::shared_ptr<telux::loc::IGnssSVInfo> &gnssSVInfo) override;

            void onGnssSignalInfo(const std::shared_ptr<telux::loc::IGnssSignalInfo> &gnssDatainfo) override;

            void onGnssNmeaInfo(uint64_t timestamp, const std::string &nmea) override;

            void onDetailedEngineLocationUpdate(const std::vector<std::shared_ptr<telux::loc::ILocationInfoEx>>
                    &locationEngineInfo) override;

            void onGnssMeasurementsInfo(const telux::loc::GnssMeasurements &measurementInfo) override;

            void onLocationSystemInfo(const telux::loc::LocationSystemInfo &locationSystemInfo) override;

            void onCapabilitiesInfo(const telux::loc::LocCapability capabilityInfo) override;
            void onXtraStatusUpdate(const telux::loc::XtraStatus xtraStatus) override;

            le_msg_SessionRef_t*         clientSessionRef;

            ~tafLocationListener() {};
    };

    class GnssListener : public IDgnssStatusListener {
        public:
            void onDgnssStatusUpdate(DgnssStatus status) override;
    };

    typedef struct
    {
        void*                       clientRefPtr;
        le_msg_SessionRef_t         sessionRef;
        taf_locGnss_Resolution_t        dopResolution;
        taf_locGnss_Resolution_t        vAccuracyResolution;
        taf_locGnss_Resolution_t        vSpeedAccuracyResolution;
        taf_locGnss_Resolution_t        hSpeedAccuracyResolution;
        taf_locGnss_State_t GnssState;
        int mAcqRate;
        LocReqEngine mEngineType;
        bool mFirstFix;
        std::chrono::time_point<std::chrono::system_clock> mStartTime;
        std::shared_ptr<ILocationManager> locationManager;
        bool mStarted;
        std::shared_ptr<tafLocationListener> posListener;

        taf_locGnss_PositionSample_t   LastPositionSample;
        le_ref_MapRef_t PositionHandlerRefMap;
        taf_locGnss_PositionSample_t mSatParams;
        std::vector<float> mVerticalSpeed;
        std::vector<float> mVerticalSpeedAccuracy;
        uint8_t mTtffReportCount;
        uint32_t mTtffPtr;
        uint8_t mTotalSVTracked;
        uint64_t mGpsTime;
        int32_t mCurrentLeapSeconds;
        uint64_t mChangeEventTime;
        uint8_t mLeapSeconds;
        int32_t mNextLeapSeconds;
        le_dls_List_t    SvInfoList;
        taf_locGnss_SvInfo_t  mSatInfo[TAF_LOCGNSS_SV_INFO_MAX_LEN];
        taf_locGnss_GnssData_t mGnssData[TAF_LOCGNSS_NUMBER_OF_SIGNAL_TYPES_MAX];
        taf_locGnss_SvMeas_t  mSatMeas;
        taf_locGnss_AltType_t mAltType;
        std::mutex mMutex;
        le_mutex_Ref_t mGnssMutexRef;
        taf_locGnss_DRConfigValidityType_t drParamsMask;
        le_dls_Link_t               next;
    }
    taf_locGnss_Client_t;

    class taf_locGnss: public ITafSvc
    {
        public:
            taf_locGnss() {};
            ~taf_locGnss();
            void Init();
            static taf_locGnss &GetInstance();
            static le_result_t CheckValidatePosition(
                    taf_locGnss_PositionSampleRequest_t* positionSampleRequestNodePtr);
            static le_result_t PositionDataCoversion(int32_t value, int8_t dataType,int32_t* valuePtr);
            static uint32_t TranslateDop(uint32_t dopValue);
            static void InitializeClient(taf_locGnss_Client_t* clientRequestPtr);
            static taf_locGnss_Client_t* DiscoverSessionRef( le_msg_SessionRef_t sessionRef);
            static taf_locGnss_Client_t* AcquireSessionRef(void);
            static void GnssPositionHandler(void* reportPtr);
            static void CopyPositionData(taf_locGnss_PositionSample_t* posSampleDataPtr,
                    taf_locGnss_PositionSample_t* posDataPtr );
            static void ConfigureAcqStartInfo(taf_locGnss_Client_t* clientRequestPtr);

            taf_locGnss_PositionHandlerRef_t AddPositionHandler(
                    taf_locGnss_PositionHandlerFunc_t handlerPtr, void* contextPtr);
            void RemovePositionHandler(taf_locGnss_PositionHandlerRef_t handlerRef);

            taf_locGnss_CapabilityChangeHandlerRef_t AddCapabilityHandler(
                    taf_locGnss_CapabilityChangeHandlerFunc_t handlerPtr, void* contextPtr);
            static void FirstLayerCapabilityHandler(void* reportPtr, void* secondLayerHandlerFunc);
            void RemoveCapabilityHandler(taf_locGnss_CapabilityChangeHandlerRef_t handlerRef);
            taf_locGnss_NmeaHandlerRef_t AddNmeaHandler(taf_locGnss_NmeaHandlerFunc_t handlerPtr, void* contextPtr);
            void RemoveNmeaHandler(taf_locGnss_NmeaHandlerRef_t handlerRef);
            static void FirstLayerNmeaHandler(void* reportPtr, void* secondLayerHandlerFunc);

            taf_locGnss_SampleRef_t GetLastSampleRef(void);
            void ReleaseClientRef( void* RefPtr);
            static void CloseEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);
            static void OpenEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);
            le_result_t GetPositionState( taf_locGnss_SampleRef_t positionSampleRef,
                    taf_locGnss_FixState_t* statePtr);
            void ReleaseSampleRef(taf_locGnss_SampleRef_t positionSampleRef);
            le_result_t GetDate( taf_locGnss_SampleRef_t positionSampleRef, uint16_t* yearPtr,
                    uint16_t* monthPtr, uint16_t* dayPtr);
            le_result_t GetTime( taf_locGnss_SampleRef_t positionSampleRef, uint16_t* hoursPtr,
                    uint16_t* minutesPtr, uint16_t* secondsPtr, uint16_t* millisecondsPtr );
            le_result_t GetLocation( taf_locGnss_SampleRef_t positionSampleRef, int32_t* latitudePtr,
                    int32_t* longitudePtr, int32_t* hAccuracyPtr);
            le_result_t GetDirection( taf_locGnss_SampleRef_t positionSampleRef, uint32_t* directionPtr, uint32_t* directionAccuracyPtr);

            le_result_t GetAltitude( taf_locGnss_SampleRef_t positionSampleRef, int32_t* altitudePtr,
                    int32_t* vAccuracyPtr );
            le_result_t GetHorizontalSpeed( taf_locGnss_SampleRef_t positionSampleRef, uint32_t* hspeedPtr,
                    uint32_t* hspeedAccuracyPtr );
            le_result_t GetVerticalSpeed( taf_locGnss_SampleRef_t positionSampleRef, int32_t* vspeedPtr,
                    int32_t* vspeedAccuracyPtr );
            le_result_t GetGpsLeapSeconds( taf_locGnss_SampleRef_t positionSampleRef, uint8_t* leapSecondsPtr);

            le_result_t Enable(void);
            le_result_t SetConstellation(taf_locGnss_ConstellationBitMask_t constellationMask);
            le_result_t Start(void);
            le_result_t GetConstellation( taf_locGnss_ConstellationBitMask_t *constellationMaskPtr);
            le_result_t Disable(void);
            le_result_t Stop(void);
            taf_locGnss_State_t GetState( void);

            le_result_t GetSatellitesStatus( taf_locGnss_SampleRef_t positionSampleRef, uint8_t* satsInViewCountPtr,
                    uint8_t* satsTrackingCountPtr, uint8_t* satsUsedCountPtr);
            le_result_t GetAcquisitionRate( uint32_t* ratePtr);
            le_result_t GetTtff( uint32_t* ttffPtr);
            le_result_t GetSatellitesInfo(taf_locGnss_SampleRef_t positionSampleRef, uint16_t* satIdPtr,
                    size_t* satIdNumElementsPtr, taf_locGnss_Constellation_t* satConstPtr,
                    size_t* satConstNumElementsPtr, bool* satUsedPtr, size_t* satUsedNumElementsPtr,
                    uint8_t* satSnrPtr,size_t* satSnrNumElementsPtr,  uint16_t* satAzimPtr,
                    size_t* satAzimNumElementsPtr, uint8_t* satElevPtr, size_t* satElevNumElementsPtr);
            le_result_t GetTimeAccuracy( taf_locGnss_SampleRef_t positionSampleRef, uint32_t* timeAccuracyPtr);
            le_result_t GetEpochTime( taf_locGnss_SampleRef_t positionSampleRef, uint64_t* millisecondsPtr);
            le_result_t SetDopResolution(taf_locGnss_Resolution_t resolution);
            le_result_t GetDilutionOfPrecision( taf_locGnss_SampleRef_t positionSampleRef, taf_locGnss_DopType_t dopType, uint16_t* dopPtr);
            le_result_t GetGpsTime(taf_locGnss_SampleRef_t positionSampleRef, uint32_t* gpsWeek, uint32_t* gpsTimeOfWeek);
            le_result_t GetLeapSeconds( uint64_t* gpsTime, int32_t* currentLeapSeconds, uint64_t* changeEventTime,int32_t*  nextLeapSeconds);
            le_result_t SetAcquisitionRate( uint32_t ratePtr);
            le_result_t ForceColdRestart();
            le_result_t ForceWarmRestart();
            le_result_t ForceHotRestart();
            le_result_t DeleteDRSensorCalData();
            le_result_t GetSupportedConstellations(taf_locGnss_ConstellationBitMask_t* constellationMaskPtr);
            le_result_t SetMinElevation( uint8_t  minElevation);
            le_result_t StartMode(taf_locGnss_StartMode_t mode);
            le_result_t GetMinElevation( uint8_t*  minElevationPtr);
            le_result_t SetNmeaSentences(taf_locGnss_NmeaBitMask_t nmeaMask);
            le_result_t GetNmeaSentences(taf_locGnss_NmeaBitMask_t* nmeaMaskPtr);
            le_result_t GetSupportedNmeaSentences(taf_locGnss_NmeaBitMask_t* nmeaMaskPtr);
            le_result_t SetDRConfig(const taf_locGnss_DrParams_t* drParamsPtr);
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
            le_result_t ConfigureEngineState(taf_locGnss_EngineType_t engtype,
                    taf_locGnss_EngineState_t engState);
#endif
            le_result_t ConfigureRobustLocation(uint8_t enable,uint8_t enabled911);
            le_result_t RobustLocationInformation(uint8_t* enable, uint8_t* enabled911,
                    uint8_t* majorVersion,uint8_t* minorVersion);
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
            le_result_t DefaultSecondaryBandConstellations();
            le_result_t RequestSecondaryBandConstellations(uint32_t* constellationSb);
            le_result_t ConfigureSecondaryBandConstellations(uint32_t constellationSb);
#endif
            le_result_t GetMagneticDeviation(taf_locGnss_SampleRef_t positionSampleRef,
                    int32_t* magneticDeviationPtr);
            le_result_t GetEllipticalUncertainty(taf_locGnss_SampleRef_t positionSampleRef,
                    uint32_t* horUncEllipseSemiMajorPtr,uint32_t* horUncEllipseSemiMinorPtr,
                    uint8_t*  horConfidencePtr);
            le_result_t SetLeverArmConfig(const taf_locGnss_LeverArmParams_t* LeverArmParamsPtr);
            le_result_t SetEngineType(taf_locGnss_EngineReportsType_t EngineType);
            le_result_t GetConformityIndex(taf_locGnss_SampleRef_t positionSampleRef,double* indexPtr);
            le_result_t GetCalibrationData(taf_locGnss_SampleRef_t positionSampleRef,
                    uint32_t* calibPtr,uint8_t* percentPtr);
            le_result_t GetDRSolutionStatus(taf_locGnss_SampleRef_t positionSampleRef,
                    uint32_t* solutionStatusPtr);
            le_result_t GetBodyFrameData(taf_locGnss_SampleRef_t positionSampleRef,
                    taf_locGnss_KinematicsData_t* bodyDataPtr);
            le_result_t GetVRPBasedLLA(taf_locGnss_SampleRef_t positionSampleRef,
                    double* vrpLatitudePtr, double* vrpLongitudePtr,double* vrpAttitudePtr);
            le_result_t GetVRPBasedVelocity(taf_locGnss_SampleRef_t positionSampleRef,
                    double* eastVelPtr, double* northVelPtr, double* upVelPtr);
            le_result_t GetSvUsedInPosition(taf_locGnss_SampleRef_t positionSampleRef,
                    taf_locGnss_SvUsedInPosition_t* svDataPtr);
            le_result_t GetSbasCorrection(taf_locGnss_SampleRef_t positionSampleRef,
                    uint32_t* sbasMaskPtr);
            le_result_t GetPositionTechnology(taf_locGnss_SampleRef_t positionSampleRef,
                    uint32_t* techMaskPtr);
            le_result_t GetLocationInfoValidity(taf_locGnss_SampleRef_t positionSampleRef,
                    uint32_t* validityMaskPtr,uint64_t* validityExMaskPtr);
            le_result_t GetLocationOutputEngParams(taf_locGnss_SampleRef_t positionSampleRef,
                    uint16_t* engMaskPtr, uint16_t* locationEngTypePtr);
            le_result_t GetReliabilityInformation(taf_locGnss_SampleRef_t positionSampleRef,
                    uint16_t* horiReliblityPtr, uint16_t* vertReliblityPtr);
            le_result_t GetStdDeviationAzimuthInfo(taf_locGnss_SampleRef_t positionSampleRef,
                    double* azimuthPtr,double* eastDevPtr, double* northDevPtr);
            le_result_t GetRealTimeInformation(taf_locGnss_SampleRef_t positionSampleRef,
                    uint64_t* realTimePtr,uint64_t* realTimeUncPtr);
            le_result_t GetMeasurementUsageInfo(taf_locGnss_SampleRef_t positionSampleRef, taf_locGnss_GnssMeasurementInfo_t* measInfoPtr, size_t* measInfoLen);
            le_result_t GetReportStatus(taf_locGnss_SampleRef_t positionSampleRef, int32_t* reportStatusPtr);
            le_result_t GetAltitudeMeanSeaLevel(taf_locGnss_SampleRef_t positionSampleRef, double* altMeanSeaLevelPtr);
            le_result_t GetSVIds(taf_locGnss_SampleRef_t positionSampleRef, uint16_t* sVIdsPtr, size_t* sVIdsLen);
            le_result_t GetSatellitesInfoEx(taf_locGnss_SampleRef_t positionSampleRef, taf_locGnss_Constellation_t constellation, taf_locGnss_SvInfo_t* svInfoPtr, size_t* svInfoLen);
            le_result_t SetMinGpsWeek(uint16_t minGpsWeek);
            le_result_t GetMinGpsWeek(uint16_t* minGpsWeekPtr);
            le_result_t GetCapabilities(uint64_t* locCapabilityPtr);
            le_result_t SetNmeaConfiguration(taf_locGnss_NmeaBitMask_t nmeaMask, taf_locGnss_GeodeticDatumType_t datumType, taf_locGnss_LocEngineType_t engineType);
            le_result_t GetXtraStatus(taf_locGnss_XtraStatusParams_t* xtraParams);
            le_result_t GetGnssData(taf_locGnss_SampleRef_t positionSampleRef,taf_locGnss_GnssData_t* gnssDataPtr,size_t* maxSignalTypes);

            le_result_t SetDRConfigValidity(taf_locGnss_DRConfigValidityType_t validMask);
            le_result_t GetGptpTime(taf_locGnss_SampleRef_t positionSampleRef,uint64_t* gPtpTime,uint64_t* gPtpTimeUnc);
            le_result_t GetLeapSecondsUncertainty(taf_locGnss_SampleRef_t positionSampleRef,uint8_t* leapSecondsUncPtr);
            le_result_t SetNmeaConfig(const taf_locGnss_NmeaBitMask_t nmea);
            taf_locGnss_NmeaBitMask_t GetNmeaConfig();
            void CleanUp(taf_locGnss_Client_t*);
            le_mem_PoolRef_t   PositionHandlerPoolRef;
            le_mem_PoolRef_t   PositionSampleRequestPoolRef;
            le_mem_PoolRef_t   PositionSamplePoolRef;
            int32_t NumOfPositionHandlers;
            int32_t NumOfCapabilityHandlers;
            int32_t mClientRefCount;
            int32_t NumOfNmeaHandlers;
            uint8_t mEnable;
            uint8_t mEnabled911;
            uint8_t mMajorVersion;
            uint8_t mMinorVersion;
            int mRequestSB;
            int mRequestMinEle;
            uint8_t mfeatureEnabled;
            uint32_t mXtraValidForHours;
            uint32_t mXtraDataStatus;
            le_event_Id_t positionEventId;
            le_event_Id_t nmeaEventId;
            le_event_Id_t locCapabilityEventId;
            le_event_HandlerRef_t HandlerRef;

            taf_locGnss_ConstellationBitMask_t mConstellationMask;
            taf_locGnss_NmeaBitMask_t mNmeaMask = 0;
            uint8_t mMinSvEle;
            std::mutex mtx;

        private:
            std::shared_ptr<ILocationConfigurator> mLocationConfigurator = nullptr;
            std::shared_ptr<IDgnssManager> mDgnssManager = nullptr;
            telux::common::Status LocationConfiguratorInit();
            telux::common::Status LocationManagerInit(taf_locGnss_Client_t* clientRequestPtr);
            telux::common::Status DgnssManagerInit();
            le_mem_PoolRef_t   ClientPoolRef;
            le_ref_MapRef_t PositionSampleMap;
            le_ref_MapRef_t ClientRequestRefMap;
            le_dls_List_t    SessionCtxList;
    };
    }
