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
#include <mutex>
#include <chrono>
#include <bitset>
#include <fstream>
#include "tafSvcIF.hpp"
#include "tafLocationPa.hpp"

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
#define TIMEOUT_SECONDS 5
#define SBAS_STATION_ID_RANGE1_MIN 120
#define SBAS_STATION_ID_RANGE1_MAX 158
#define SBAS_STATION_ID_RANGE2_MIN 183
#define SBAS_STATION_ID_RANGE2_MAX 191
#define MAX_CMD_LOCATION_POOL_SIZE 64

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
        uint16_t dgnssStationIds[TAF_LOCGNSS_MAX_MONITOR_STATION_IDS];
        uint8_t  dgnssStationIdsCount;
        uint32_t navSolutionMask;
        bool     navSolutionMaskValid;
        double protectionLevelAlongTrack;
        double protectionLevelCrossTrack;
        double protectionLevelVertical;
        double baselineLength;
        uint64_t ageCorrections;
        uint32_t integrityRiskUsed;
        bool protectionLevelAlongTrackValid;
        bool protectionLevelCrossTrackValid;
        bool protectionLevelVerticalValid;
        bool baselineLengthValid;
        bool ageCorrectionsValid;
        bool integrityRiskUsedValid;
        le_dls_Link_t   next;
    }
    taf_locGnss_PositionSample_t;

    typedef struct taf_locGnss_GnssMeasurementsClock
    {
        taf_locGnss_GnssMeasurementsClockValidityType_t valid;
        int16_t leapSecond;
        int64_t timeNs;
        double timeUncertaintyNs;
        int64_t fullBiasNs;
        double biasNs;
        double biasUncertaintyNs;
        double driftNsps;
        double driftUncertaintyNsps;
        uint32_t hwClockDiscontinuityCount;
        uint64_t elapsedRealTime;
        uint64_t elapsedRealTimeUnc;
        uint64_t elapsedgPTPTime;
        uint64_t elapsedgPTPTimeUnc;
    }taf_locGnss_GnssMeasurementsClock_t;

    typedef struct taf_locGnss_GnssMeasurementsData
    {
        taf_locGnss_GnssMeasurementsDataValidityType_t valid;
        int16_t svId;
        taf_locGnss_SBConstellation_t svType;
        double timeOffsetNs;
        taf_locGnss_GnssMeasurementsStateValidityType_t stateMask;
        int64_t receivedSvTimeNs;
        double receivedSvTimeSubNs;
        int64_t receivedSvTimeUncertaintyNs;
        double carrierToNoiseDbHz;
        double pseudorangeRateMps;
        double pseudorangeRateUncertaintyMps;
        taf_locGnss_GnssMeasurementsAdrStateValidityType_t adrStateMask;
        double adrMeters;
        double adrUncertaintyMeters;
        double carrierFrequencyHz;
        int64_t carrierCycles;
        double carrierPhase;
        double carrierPhaseUncertainty;
        taf_locGnss_GnssMeasurementsMultipathIndicator_t multipathIndicator;
        double signalToNoiseRatioDb;
        double agcLevelDb;
        taf_locGnss_GnssSignalType_t gnssSignalType;
        double basebandCarrierToNoise;
        double fullInterSignalBias;
        double fullInterSignalBiasUncertainty;
    }taf_locGnss_GnssMeasurementsData_t;

    typedef struct
    {
        taf_locGnss_GnssMeasurementsClock_t clock;
        taf_locGnss_GnssMeasurementsData_t measData[TAF_LOCGNSS_MEASUREMENT_INFO_MAX];
        uint32_t   measCount;
        bool isNHz;
        taf_locGnss_AgcStatus_t     agcStatusL1;
        taf_locGnss_AgcStatus_t     agcStatusL2;
        taf_locGnss_AgcStatus_t     agcStatusL5;
        le_msg_SessionRef_t*          clientSessionRefPtr;
     }taf_locGnss_GnssMeasurements_t;

    typedef struct taf_locGnss_PositionHandler
    {
        taf_locGnss_PositionHandlerRef_t handlerRef;
        taf_locGnss_PositionHandlerFunc_t handlerFuncPtr;
        void*                         handlerContextPtr;
        le_msg_SessionRef_t           sessionRef;
        le_dls_Link_t                 next;
    }
    taf_locGnss_PositionHandler_t;

    typedef struct taf_locGnss_MeasurementHandler
    {
        taf_locGnss_MeasurementHandlerRef_t handlerRef;
        taf_locGnss_MeasurementHandlerFunc_t handlerFuncPtr;
        void*                         handlerContextPtr;
        le_msg_SessionRef_t           sessionRef;
        le_dls_Link_t                 next;
    }
    taf_locGnss_MeasurementHandler_t;

    typedef struct
    {
        taf_locGnss_LocCapabilityType_t locCapability;
        le_msg_SessionRef_t*          clientSessionRefPtr;
    }
    CapabilityChangeEvent_t;

    typedef struct
    {
        uint64_t timestamp;
        char nmeaMask[TAF_LOCGNSS_NMEA_STRING_MAX];
        le_msg_SessionRef_t*          clientSessionRefPtr;
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

    typedef struct
    {
        taf_locGnss_MeasSampleRef_t             measSampleRef;
        taf_locGnss_GnssMeasurements_t*       measSampleNodePtr;
        le_msg_SessionRef_t             sessionRef;
        le_dls_Link_t                   next;
    }
    taf_locGnss_MeasurementSampleRequest_t;

    typedef struct
    {
        taf_locGnss_DgnssSourceRef_t sourceRef;
        taf_locGnss_DgnssFormat_t format;
        le_msg_SessionRef_t             sessionRef;
    } taf_locGnss_DgnssSource_t;

    typedef struct
    {
        taf_locGnss_DgnssStatus_t     dgnssStatus;
        le_msg_SessionRef_t*          clientSessionRefPtr;
    }
    DgnssStatusEvent_t;

    typedef struct taf_locGnss_DgnssStatusChangeHandler
    {
        taf_locGnss_DgnssStatusChangeHandlerRef_t handlerRef;
        taf_locGnss_DgnssStatusChangeHandlerFunc_t handlerFuncPtr;
        void*                         handlerContextPtr;
        le_msg_SessionRef_t           sessionRef;
        le_dls_Link_t                 next;
    } taf_locGnss_DgnssStatusChangeHandler_t;

    typedef struct
    {
        int32_t   longitude;
        int32_t   latitude;
        int32_t   hAccuracy;
        int32_t   altitude;
        uint32_t  direction;
        uint32_t  directionAccuracy;
        uint32_t  validityMask;
        uint32_t  techMask;
        uint32_t  hSpeed;
        int32_t   vAccuracy;
        uint64_t  epochTime;
        int32_t   hSpeedAccuracy;
        uint64_t  realTime;
        uint64_t  realTimeUnc;
        uint8_t   satsInViewCount;
        uint8_t   satsTrackingCount;
        uint8_t   satsUsedCount;
        uint32_t  hdop;
        uint32_t  vdop;
        uint32_t  pdop;
        double  altMeanSeaLevel;
        int32_t   magneticDeviation;
        le_msg_SessionRef_t*         clientSessionRefPtr;
    }
    taf_locGnss_PositionExSample_t;

    typedef struct
    {
        taf_locGnss_PositionExHandlerRef_t handlerRef;
        taf_locGnss_PositionExHandlerFunc_t handlerFuncPtr;
        void*                         handlerContextPtr;
        le_msg_SessionRef_t           sessionRef;
        le_dls_Link_t                 next;
    }taf_locGnss_PositionExHandler_t;

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
        bool mStarted;
        tafpa::location::taf_pa_location_LocationId locationClient;
        tafpa::location::taf_pa_location_EventListener eventListener;

        taf_locGnss_PositionSample_t   LastPositionSample;
        taf_locGnss_PositionSample_t mSatParams;
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
        taf_locGnss_DgnssSourceRef_t activeSourceRef;
        taf_locGnss_DgnssFormat_t activeDgnssFormat;
        le_dls_Link_t               next;
    }
    taf_locGnss_Client_t;

    typedef struct taf_locGnss_NmeaHandler
    {
        taf_locGnss_NmeaHandlerRef_t handlerRef;
        taf_locGnss_NmeaHandlerFunc_t handlerFuncPtr;
        void*                         handlerContextPtr;
        le_msg_SessionRef_t           sessionRef;
        le_dls_Link_t                 next;
    }
    taf_locGnss_NmeaHandler_t;

    typedef struct taf_locGnss_CapHandler
    {
        taf_locGnss_CapabilityChangeHandlerRef_t handlerRef;
        taf_locGnss_CapabilityChangeHandlerFunc_t handlerFuncPtr;
        void*                         handlerContextPtr;
        le_msg_SessionRef_t           sessionRef;
        le_dls_Link_t                 next;
    }
    taf_locGnss_CapHandler_t;

    // ------------------------------------------------------------------------------------------
    // Single unified envelope passed between main thread and worker thread.
    // The 'params' union carry all function-specific IN parameters;'retCode' carries the result.
    // cmdRef and sessionRef are common to every async command.
    // ------------------------------------------------------------------------------------------
    typedef struct
    {
        taf_locGnss_ServerCmdRef_t cmdRef;       ///< TAF server command reference
        le_result_t                retCode;      ///< PA result written by worker, read by respond
        le_msg_SessionRef_t        sessionRef;   ///< client session(used by worker)

        // ------------------------------------------------------------------
        // IN-parameter union – only the active member is valid per command.
        // ------------------------------------------------------------------
        union
        {
            // SetConstellation
            struct {
                taf_locGnss_ConstellationBitMask_t constellationMask;
            } setConstellation;

            // SetMinElevation
            struct {
                uint8_t minElevation;
            } setMinElevation;

            // StartMode
            struct {
                taf_locGnss_StartMode_t mode;
            } startMode;

            // SetNmeaSentences
            struct {
                taf_locGnss_NmeaBitMask_t nmeaMask;
            } setNmeaSentences;

            // SetDRConfig – copy of the full DR params struct
            struct {
                taf_locGnss_DrParams_t drParams;
            } setDRConfig;

            // ConfigureEngineState
            struct {
                taf_locGnss_EngineType_t  engType;
                taf_locGnss_EngineState_t engState;
            } configureEngineState;

            // ConfigureRobustLocation
            struct {
                uint8_t enable;
                uint8_t enabled911;
            } configureRobustLocation;

            // ConfigureSecondaryBandConstellations / DefaultSecondaryBandConstellations
            struct {
                uint32_t constellationSb;
            } configureSecondaryBand;

            // SetLeverArmConfig – copy of the full lever-arm params struct
            struct {
                taf_locGnss_LeverArmParams_t leverArmParams;
            } setLeverArmConfig;

            // SetEngineType
            struct {
                taf_locGnss_EngineReportsType_t engineType;
            } setEngineType;

            // SetMinGpsWeek
            struct {
                uint16_t minGpsWeek;
            } setMinGpsWeek;

            // SetNmeaConfiguration
            struct {
                taf_locGnss_NmeaBitMask_t        nmeaMask;
                taf_locGnss_GeodeticDatumType_t  datumType;
                taf_locGnss_LocEngineType_t      engineType;
            } setNmeaConfiguration;

            // SetDRConfigValidity
            struct {
                taf_locGnss_DRConfigValidityType_t validMask;
            } setDRConfigValidity;

            // ConfigureOsnma
            struct {
                bool galOsnma;
            } configureOsnma;

            // CreateDgnssSource
            struct {
                taf_locGnss_DgnssFormat_t        dgnssDataFormat;
                taf_locGnss_DgnssSourceRef_t     sourceRef;        ///< written back by worker
            } createDgnssSource;

            // ReleaseDgnssSource
            struct {
                taf_locGnss_DgnssSourceRef_t     sourceRef;
            } releaseDgnssSource;

            // InjectDgnssCorrection
            struct {
                taf_locGnss_DgnssSourceRef_t     sourceRef;
                uint8_t                          correctionData[TAF_LOCGNSS_DATA_LEN_MAX];
                size_t                           correctionDataSize;
            } injectDgnssCorrection;

            struct{
                char merkleTreeFilePath[PATH_MAX];
            } injectMerkle;

        } params;

    } LocationCmdInfo_t;

    //-----------------------------------------------------------------------------------
    /**
     * Callback type for internal GNSS start completion notification.
     */
    //-----------------------------------------------------------------------------------
    typedef void (*taf_locGnss_CompleteCb_t)(le_result_t result, void* contextPtr);

    //-----------------------------------------------------------------------------------
    /**
     * Internal command info for GNSS start triggered by locPosCtrl.
     */
    //-----------------------------------------------------------------------------------
    typedef struct
    {
        taf_locGnss_CompleteCb_t      completeCb;   ///< Completion callback.
        void*                         contextPtr;   ///< Context to pass to callback.
        le_msg_SessionRef_t           sessionRef;   ///< Client session ref.
        le_result_t                   retCode;      ///< Result.
    } GnssInternalCmd_t;

    class taf_locGnss: public ITafSvc
    {
        public:
            taf_locGnss() {};
            ~taf_locGnss();
            void Init();
            static taf_locGnss &GetInstance();
            le_result_t Enable(void);
            le_result_t Disable(void);
            le_result_t SetAcquisitionRate( uint32_t ratePtr);
            le_result_t GetConstellation(taf_locGnss_ConstellationBitMask_t *constellationMaskPtr);
            static le_result_t CheckValidatePosition(
                    taf_locGnss_PositionSampleRequest_t* positionSampleRequestNodePtr);
            static le_result_t PositionDataCoversion(int32_t value, int8_t dataType,int32_t* valuePtr);
            static uint32_t TranslateDop(uint32_t dopValue);
            static void InitializeClient(taf_locGnss_Client_t* clientRequestPtr);
            static taf_locGnss_Client_t* DiscoverSessionRef( le_msg_SessionRef_t sessionRef);
            static taf_locGnss_Client_t* AcquireSessionRef(void);
            static void GnssPositionHandler(void* reportPtr);
            static void GnssMeasurementHandler(void* reportPtr);
            static void GnssPositionExHandler(void* reportPtr);
            static void CopyPositionData(taf_locGnss_PositionSample_t* posSampleDataPtr,
                    taf_locGnss_PositionSample_t* posDataPtr );
            static void ConfigureAcqStartInfo(taf_locGnss_Client_t* clientRequestPtr);

            taf_locGnss_PositionHandlerRef_t AddPositionHandler(
                    taf_locGnss_PositionHandlerFunc_t handlerPtr, void* contextPtr);
            void RemovePositionHandler(taf_locGnss_PositionHandlerRef_t handlerRef);

            taf_locGnss_PositionExHandlerRef_t AddPositionExHandler(
                    taf_locGnss_PositionExHandlerFunc_t handlerPtr, void* contextPtr);
            void RemovePositionExHandler(taf_locGnss_PositionExHandlerRef_t handlerRef);

            taf_locGnss_MeasurementHandlerRef_t AddMeasurementHandler(
                    taf_locGnss_MeasurementHandlerFunc_t handlerPtr, void* contextPtr);
            void RemoveMeasurementHandler(taf_locGnss_MeasurementHandlerRef_t handlerRef);

            taf_locGnss_CapabilityChangeHandlerRef_t AddCapabilityHandler(
                    taf_locGnss_CapabilityChangeHandlerFunc_t handlerPtr, void* contextPtr);
            static void GnssCapabilityHandler(void* reportPtr);
            void RemoveCapabilityHandler(taf_locGnss_CapabilityChangeHandlerRef_t handlerRef);
            taf_locGnss_NmeaHandlerRef_t AddNmeaHandler(taf_locGnss_NmeaHandlerFunc_t handlerPtr, void* contextPtr);
            void RemoveNmeaHandler(taf_locGnss_NmeaHandlerRef_t handlerRef);
            static void GnssNmeaHandler(void* reportPtr);

            taf_locGnss_SampleRef_t GetLastSampleRef(void);
            void ReleaseClientRef( void* RefPtr);
            static void CloseEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);
            static void OpenEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);
            le_result_t GetPositionState( taf_locGnss_SampleRef_t positionSampleRef,
                    taf_locGnss_FixState_t* statePtr);
            void ReleaseSampleRef(taf_locGnss_SampleRef_t positionSampleRef);
            void ReleaseSampleExRef(taf_locGnss_SampleExRef_t postitionSampleExRef);
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
            le_result_t SetNmeaSentencesInternal(taf_locGnss_NmeaBitMask_t nmeaMask);

            // -----------------------------------------------------------------------
            // Async entry-point signatures (public, called from tafLocationSvc.cpp).
            // All PA-calling functions now take (cmdRef [, IN-params...]) and return void.
            // -----------------------------------------------------------------------
            void Stop(taf_locGnss_ServerCmdRef_t cmdRef);
            void Start(taf_locGnss_ServerCmdRef_t cmdRef);
            void SetConstellation(taf_locGnss_ServerCmdRef_t cmdRef,
                                  taf_locGnss_ConstellationBitMask_t constellationMask);
            void ForceColdRestart(taf_locGnss_ServerCmdRef_t cmdRef);
            void ForceWarmRestart(taf_locGnss_ServerCmdRef_t cmdRef);
            void ForceHotRestart(taf_locGnss_ServerCmdRef_t cmdRef);
            void SetMinElevation(taf_locGnss_ServerCmdRef_t cmdRef, uint8_t minElevation);
            void StartMode(taf_locGnss_ServerCmdRef_t cmdRef, taf_locGnss_StartMode_t mode);
            void SetNmeaSentences(taf_locGnss_ServerCmdRef_t cmdRef,
                                  taf_locGnss_NmeaBitMask_t nmeaMask);
            void SetDRConfig(taf_locGnss_ServerCmdRef_t cmdRef,
                             const taf_locGnss_DrParams_t* drParamsPtr);
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
            void ConfigureEngineState(taf_locGnss_ServerCmdRef_t cmdRef,
                                      taf_locGnss_EngineType_t engtype,
                                      taf_locGnss_EngineState_t engState);
#endif
            void ConfigureRobustLocation(taf_locGnss_ServerCmdRef_t cmdRef,
                                         uint8_t enable, uint8_t enabled911);
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
            void DefaultSecondaryBandConstellations(taf_locGnss_ServerCmdRef_t cmdRef);
            void ConfigureSecondaryBandConstellations(taf_locGnss_ServerCmdRef_t cmdRef,
                                                      uint32_t constellationSb);
            void RequestSecondaryBandConstellations(taf_locGnss_ServerCmdRef_t cmdRef);
#endif
            void SetLeverArmConfig(taf_locGnss_ServerCmdRef_t cmdRef,
                                   const taf_locGnss_LeverArmParams_t* LeverArmParamsPtr);
            void SetEngineType(taf_locGnss_ServerCmdRef_t cmdRef,
                               taf_locGnss_EngineReportsType_t EngineType);
            void SetMinGpsWeek(taf_locGnss_ServerCmdRef_t cmdRef, uint16_t minGpsWeek);
            void SetNmeaConfiguration(taf_locGnss_ServerCmdRef_t cmdRef,
                                      taf_locGnss_NmeaBitMask_t nmeaMask,
                                      taf_locGnss_GeodeticDatumType_t datumType,
                                      taf_locGnss_LocEngineType_t engineType);
            void DeleteDRSensorCalData(taf_locGnss_ServerCmdRef_t cmdRef);
            void ConfigureOsnma(taf_locGnss_ServerCmdRef_t cmdRef, bool galOsnma);

            // -----------------------------------------------------------------------
            // Non-PA / synchronous getters (unchanged)
            // -----------------------------------------------------------------------
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
            le_result_t GetSupportedConstellations(taf_locGnss_ConstellationBitMask_t* constellationMaskPtr);
            le_result_t GetMinElevation( uint8_t*  minElevationPtr);
            le_result_t GetNmeaSentences(taf_locGnss_NmeaBitMask_t* nmeaMaskPtr);
            le_result_t GetSupportedNmeaSentences(taf_locGnss_NmeaBitMask_t* nmeaMaskPtr);
            le_result_t RobustLocationInformation(uint8_t* enable, uint8_t* enabled911,
                    uint8_t* majorVersion,uint8_t* minorVersion);
            le_result_t GetMagneticDeviation(taf_locGnss_SampleRef_t positionSampleRef,
                    int32_t* magneticDeviationPtr);
            le_result_t GetEllipticalUncertainty(taf_locGnss_SampleRef_t positionSampleRef,
                    uint32_t* horUncEllipseSemiMajorPtr,uint32_t* horUncEllipseSemiMinorPtr,
                    uint8_t*  horConfidencePtr);
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
            le_result_t GetMinGpsWeek(uint16_t* minGpsWeekPtr);
            le_result_t GetCapabilities(uint64_t* locCapabilityPtr);
            le_result_t GetXtraStatus(taf_locGnss_XtraStatusParams_t* xtraParams);
            le_result_t GetGnssData(taf_locGnss_SampleRef_t positionSampleRef,taf_locGnss_GnssData_t* gnssDataPtr,size_t* maxSignalTypes);
            le_result_t GetNavigationSolution(taf_locGnss_SampleRef_t positionSampleRef,uint32_t* navSolutionPtr);
            le_result_t GetDgnssStationIds(taf_locGnss_SampleRef_t positionSampleRef,uint16_t* stationIdsPtr,size_t* stationIdsSizePtr);
            void InjectDgnssCorrection(taf_locGnss_ServerCmdRef_t cmdRef, taf_locGnss_DgnssSourceRef_t sourceRef, const uint8_t* correctionDataPtr, size_t correctionDataSize);
            void CreateDgnssSource(taf_locGnss_ServerCmdRef_t cmdRef, taf_locGnss_DgnssFormat_t dgnssDataFormat);
            void ReleaseDgnssSource(taf_locGnss_ServerCmdRef_t cmdRef, taf_locGnss_DgnssSourceRef_t sourceRef);

            static void DgnssStatusHandler(void* reportPtr);
            taf_locGnss_DgnssStatusChangeHandlerRef_t AddDgnssStatusChangeHandler(
                    taf_locGnss_DgnssStatusChangeHandlerFunc_t handlerPtr, void* contextPtr);
            void RemoveDgnssStatusChangeHandler(taf_locGnss_DgnssStatusChangeHandlerRef_t handlerRef);

            le_result_t SetDRConfigValidity(taf_locGnss_DRConfigValidityType_t validMask);
            le_result_t GetGptpTime(taf_locGnss_SampleRef_t positionSampleRef,uint64_t* gPtpTime,uint64_t* gPtpTimeUnc);
            le_result_t GetLeapSecondsUncertainty(taf_locGnss_SampleRef_t positionSampleRef,uint8_t* leapSecondsUncPtr);
            le_result_t SetNmeaConfig(const taf_locGnss_NmeaBitMask_t nmea);

            taf_locGnss_NmeaBitMask_t GetNmeaConfig();
            void CleanUp(taf_locGnss_Client_t*);

            le_result_t GetIsNHz(taf_locGnss_MeasSampleRef_t measSampleRef, bool* isNHZPtr);
            le_result_t GetClockValidityMask(taf_locGnss_MeasSampleRef_t measSampleRef, uint32_t* clockValidityMaskPtr);
            le_result_t GetClockData(taf_locGnss_MeasSampleRef_t measSampleRef, taf_locGnss_ClockData_t * clockDataPtr);
            le_result_t GetMeasurementsData(taf_locGnss_MeasSampleRef_t measSampleRef, taf_locGnss_MeasurementsData_t * measDataPtr, size_t* measDataSizePtr);
            void ReleaseMeasSampleRef(taf_locGnss_MeasSampleRef_t    measSampleRef);
            le_result_t GetMeasDataValidityMask(taf_locGnss_MeasSampleRef_t measSampleRef,
                uint32_t* measDataValidityMaskPtr, size_t* measDataValidityMaskSizePtr);

            void InjectMerkleData(taf_locGnss_ServerCmdRef_t cmdRef, const char* merkleTreeFilePath);

            le_result_t SetEngineIntegrityRisk(taf_locGnss_EngineType_t engtype, uint32_t integrityRisk);
            le_result_t GetProtectionLevels(taf_locGnss_SampleRef_t positionSampleRef,double* protectionLevelAlongTrackPtr,
                    double* protectionLevelCrossTrackPtr,double* protectionLevelVerticalPtr);
            le_result_t GetBaselineLength(taf_locGnss_SampleRef_t positionSampleRef, double* baselineLengthPtr);
            le_result_t GetAgeOfCorrections(taf_locGnss_SampleRef_t positionSampleRef, uint64_t* ageCorrectionsPtr);
            le_result_t GetIntegrityRiskUsed(taf_locGnss_SampleRef_t positionSampleRef, uint32_t* integrityRiskUsedPtr);

            void StartInternal(le_msg_SessionRef_t sessionRef, taf_locGnss_CompleteCb_t completeCb, void* contextPtr);
            void StopInternal(le_msg_SessionRef_t sessionRef, taf_locGnss_CompleteCb_t completeCb, void* contextPtr);
            static taf_locGnss_Client_t* AcquireSessionRefInternal(le_msg_SessionRef_t sessionRef);
            static le_result_t InternalReleaseDgnssSourceOnWorker(taf_locGnss_Client_t* clientRequestPtr, taf_locGnss_DgnssSourceRef_t  sourceRef);

            le_mem_PoolRef_t   PositionHandlerPoolRef;
            le_mem_PoolRef_t   PositionExHandlerPoolRef;
            le_mem_PoolRef_t   PositionSampleRequestPoolRef;
            le_mem_PoolRef_t   PositionSamplePoolRef;
            le_mem_PoolRef_t   PositionSampleExPoolRef;
            le_mem_PoolRef_t   PositionExSamplePoolRef;
            le_mem_PoolRef_t   MeasurementHandlerPoolRef;
            le_mem_PoolRef_t   MeasurementSampleRequestPoolRef;
            le_mem_PoolRef_t   MeasurementSamplePoolRef;
            le_mem_PoolRef_t   MeasurementDataSamplePoolRef;
            le_ref_MapRef_t PositionHandlerRefMap;
            le_ref_MapRef_t MeasurementHandlerRefMap;
            le_ref_MapRef_t PositionExHandlerRefMap;
            le_mem_PoolRef_t   NmeaHandlerPoolRef;
            le_mem_PoolRef_t   NmeaSamplePoolRef;
            le_mem_PoolRef_t   CapabilityHandlerPoolRef;
            le_mem_PoolRef_t   CapSamplePoolRef;
            int32_t NumOfPositionHandlers;
            int32_t NumOfPositionExHandlers;
            int32_t NumOfCapabilityHandlers;
            int32_t mClientRefCount;
            int32_t NumOfNmeaHandlers;
            int32_t NumOfMeasurementHandlers;
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
            le_event_Id_t PositionExEventId;
            le_event_Id_t measurementEventId;
            le_event_Id_t nmeaEventId;
            le_event_Id_t locCapabilityEventId;
            le_event_HandlerRef_t HandlerRef;
            le_event_HandlerRef_t MeasurementHandlerRef;
            le_event_HandlerRef_t HandlerExRef;
            le_event_HandlerRef_t NmeaHandlerRef;
            le_event_HandlerRef_t CapHandlerRef;

            le_thread_Ref_t LocationSvcThRef    = NULL;    // service main thread
            le_thread_Ref_t LocationWorkerThRef = NULL;    // PA worker thread
            le_thread_Ref_t LocationEventWorkerThRef;   ///< Thread that owns all PA event handlers
            le_mem_PoolRef_t CmdLocationPoolRef = NULL;
            le_mem_PoolRef_t GnssInternalCmdPoolRef = NULL;

            taf_locGnss_ConstellationBitMask_t mConstellationMask;
            taf_locGnss_NmeaBitMask_t mNmeaMask = 0;
            uint8_t mMinSvEle;
            std::mutex mtx;
            le_ref_MapRef_t NmeaHandlerRefMap;
            le_ref_MapRef_t CapHandlerRefMap;

            tafpa::location::taf_pa_location_DgnssEventListener dgnssListener;
            le_ref_MapRef_t DgnssSourceRefMap;
            le_mem_PoolRef_t DgnssSourcePoolRef;
            le_mem_PoolRef_t   DgnssStatusHandlerPoolRef;
            le_mem_PoolRef_t DgnssStatusPoolRef;
            le_ref_MapRef_t DgnssStatusHandlerRefMap;
            int32_t NumOfDgnssStatusHandlers;
            le_event_HandlerRef_t DgnssStatusHandlerRef;
            le_event_Id_t dgnssStatusEventId;

            // -----------------------------------------------------------------------
            // Worker thread entry points – one per async PA-calling function.
            // -----------------------------------------------------------------------
            static void StopWorker(void* cmdPtr, void* context);
            static void StartWorker(void* cmdPtr, void* context);
            static void SetConstellationWorker(void* cmdPtr, void* context);
            static void ForceColdRestartWorker(void* cmdPtr, void* context);
            static void ForceWarmRestartWorker(void* cmdPtr, void* context);
            static void ForceHotRestartWorker(void* cmdPtr, void* context);
            static void SetMinElevationWorker(void* cmdPtr, void* context);
            static void StartModeWorker(void* cmdPtr, void* context);
            static void SetNmeaSentencesWorker(void* cmdPtr, void* context);
            static void SetDRConfigWorker(void* cmdPtr, void* context);
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
            static void ConfigureEngineStateWorker(void* cmdPtr, void* context);
#endif
            static void ConfigureRobustLocationWorker(void* cmdPtr, void* context);
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
            static void DefaultSecondaryBandConstellationsWorker(void* cmdPtr, void* context);
            static void ConfigureSecondaryBandConstellationsWorker(void* cmdPtr, void* context);
            static void RequestSecondaryBandConstellationsWorker(void* cmdPtr, void* context);
#endif
            static void SetLeverArmConfigWorker(void* cmdPtr, void* context);
            static void SetEngineTypeWorker(void* cmdPtr, void* context);
            static void SetMinGpsWeekWorker(void* cmdPtr, void* context);
            static void SetNmeaConfigurationWorker(void* cmdPtr, void* context);
            static void DeleteDRSensorCalDataWorker(void* cmdPtr, void* context);
            static void ConfigureOsnmaWorker(void* cmdPtr, void* context);
            static void CreateDgnssSourceWorker(void* cmdPtr, void* context);
            static void ReleaseDgnssSourceWorker(void* cmdPtr, void* context);
            static void InjectDgnssCorrectionWorker(void* cmdPtr, void* context);
            static void InjectMerkleWorker(void* cmdPtr, void* context);

        private:
            le_mem_PoolRef_t   ClientPoolRef;
            le_ref_MapRef_t PositionSampleMap;
            le_ref_MapRef_t MeasurementSampleMap;
            le_ref_MapRef_t PositionExSampleMap;
            le_ref_MapRef_t ClientRequestRefMap;
            le_dls_List_t    SessionCtxList;
    };
    class Handler : public ITafSvc{
        public:

            void Init() {
                return;
            }
            static void onGnssSVInfo(tafpa::location::taf_pa_location_LocationId clientId, const std::vector<std::shared_ptr<tafpa::location::taf_pa_location_GnssSVInfo_t>>& GnssSVInfo, std::any context);

            static void onGnssSignalInfo(tafpa::location::taf_pa_location_LocationId clientId, const std::shared_ptr<tafpa::location::taf_pa_location_GnssData_t>& gnssDatainfo, std::any context);

            static void onGnssNmeaInfo(tafpa::location::taf_pa_location_LocationId clientId, const std::shared_ptr<tafpa::location::taf_pa_location_NmeaInfoEvent_t>& nmeaEventInfo, std::any context);

            static void onDetailedEngineLocationUpdate(tafpa::location::taf_pa_location_LocationId clientId, const std::vector<std::shared_ptr<tafpa::location::taf_pa_location_LocEngineInfo_t>>& locationEngineInfo, std::any context);

            static void onGnssMeasurementsInfo(tafpa::location::taf_pa_location_LocationId clientId, const std::shared_ptr<tafpa::location::taf_pa_location_GnssMeasurements_t>& measurementInfo, std::any context);

            static void onLocationSystemInfo(tafpa::location::taf_pa_location_LocationId clientId, const std::shared_ptr<tafpa::location::taf_pa_location_LocationSystemInfo_t>& LocSysInfo, std::any context);

            static void onCapabilitiesInfo(tafpa::location::taf_pa_location_LocationId clientId, const std::shared_ptr<tafpa::location::taf_pa_location_CapabilityChangeEvent_t>& capEventInfo, std::any context);
            static void onXtraStatusUpdate(tafpa::location::taf_pa_location_LocationId clientId, const std::shared_ptr<tafpa::location::taf_pa_location_XtraStatus_t>& xtraStatus, std::any context);
            static void onDgnssStatusUpdate(const std::shared_ptr<tafpa::location::taf_pa_location_DgnssStatus_t>& dgnssStatus, std::any context);

    };
}
