/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

static taf_locGnss_PositionHandlerRef_t PositionHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Callback function to trigger on receiving the location information.
 */
//--------------------------------------------------------------------------------------------------
static void PositionHandlerFunction
(
    taf_locGnss_SampleRef_t positionSampleRef,
    void* contextPtr
)
{
    le_result_t result;
    taf_locGnss_DopType_t dopType = TAF_LOCGNSS_PDOP;
    size_t gnssMeasLen = TAF_LOCGNSS_MEASUREMENT_INFO_MAX;
    size_t gnssMeasLen_zero = 0;
    int32_t reportStatus = -1;
    double altMSeaLevel;
    uint16_t svIds[TAF_LOCGNSS_MEASUREMENT_INFO_MAX];
    size_t svIdlen_zero = 0;
    int constellation = TAF_LOCGNSS_SV_CONSTELLATION_GLONASS;
    taf_locGnss_SvInfo_t svInfo[TAF_LOCGNSS_SV_INFO_MAX_SATS_IN_CONSTELLATIONS];
    size_t satlen_zero = 0;
    LE_TEST_INFO("PositionHandlerFunction is triggered");

    //1.taf_locGnss_GetDilutionOfPrecision - LE_OUT_OF_RANGE scenario
    result = taf_locGnss_GetDilutionOfPrecision(positionSampleRef,dopType,NULL);
    LE_TEST_OK(result == LE_OUT_OF_RANGE, "***taf_locGnss_GetDilutionOfPrecision***-LE_OUT_OF_RANGE");
    if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("taf_locGnss_GetDilutionOfPrecision -> returning LE_OUT_OF_RANGE");
    }

    //2.taf_locGnss_GetConformityIndex - LE_FAULT scenario
    result = taf_locGnss_GetConformityIndex(positionSampleRef,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetConformityIndex***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetConformityIndex -> returning LE_FAULT");
    }

    //3.taf_locGnss_GetCalibrationData - LE_FAULT scenario
    result = taf_locGnss_GetCalibrationData(positionSampleRef,NULL,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetCalibrationData***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetCalibrationData -> returning LE_FAULT");
    }

    //4.taf_locGnss_GetBodyFrameData - LE_FAULT scenario
    result = taf_locGnss_GetBodyFrameData(positionSampleRef,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetBodyFrameData***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetBodyFrameData -> returning LE_FAULT");
    }

    //5.taf_locGnss_GetVRPBasedLLA - LE_FAULT scenario
    result = taf_locGnss_GetVRPBasedLLA(positionSampleRef,NULL,NULL,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetVRPBasedLLA***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetVRPBasedLLA -> returning LE_FAULT");
    }

    //6.taf_locGnss_GetVRPBasedLLA - LE_FAULT scenario
    result = taf_locGnss_GetVRPBasedLLA(positionSampleRef,NULL,NULL,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetVRPBasedLLA***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetVRPBasedLLA -> returning LE_FAULT");
    }

    //6.taf_locGnss_GetVRPBasedVelocity - LE_FAULT scenario
    result = taf_locGnss_GetVRPBasedVelocity(positionSampleRef,NULL,NULL,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetVRPBasedVelocity***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetVRPBasedVelocity -> returning LE_FAULT");
    }

    //7.taf_locGnss_GetSvUsedInPosition - LE_FAULT scenario
    result = taf_locGnss_GetSvUsedInPosition(positionSampleRef,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetSvUsedInPosition***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetSvUsedInPosition -> returning LE_FAULT");
    }

    //8.taf_locGnss_GetSbasCorrection - LE_FAULT scenario
    result = taf_locGnss_GetSbasCorrection(positionSampleRef,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetSbasCorrection***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetSbasCorrection -> returning LE_FAULT");
    }

    //9.taf_locGnss_GetPositionTechnology - LE_FAULT scenario
    result = taf_locGnss_GetPositionTechnology(positionSampleRef,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetPositionTechnology***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetPositionTechnology -> returning LE_FAULT");
    }

    //10.taf_locGnss_GetLocationInfoValidity - LE_FAULT scenario
    result = taf_locGnss_GetLocationInfoValidity(positionSampleRef,NULL,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetLocationInfoValidity***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetLocationInfoValidity -> returning LE_FAULT");
    }

    //11.taf_locGnss_GetLocationOutputEngParams - LE_FAULT scenario
    result = taf_locGnss_GetLocationOutputEngParams(positionSampleRef,NULL,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetLocationOutputEngParams***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetLocationOutputEngParams -> returning LE_FAULT");
    }

    //12.taf_locGnss_GetReliabilityInformation - LE_FAULT scenario
    result = taf_locGnss_GetReliabilityInformation(positionSampleRef,NULL,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetReliabilityInformation***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetReliabilityInformation -> returning LE_FAULT");
    }

    //13.taf_locGnss_GetStdDeviationAzimuthInfo - LE_FAULT scenario
    result = taf_locGnss_GetStdDeviationAzimuthInfo(positionSampleRef,NULL,NULL,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetStdDeviationAzimuthInfo***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetStdDeviationAzimuthInfo -> returning LE_FAULT");
    }

    //14.taf_locGnss_GetRealTimeInformation - LE_FAULT scenario
    result = taf_locGnss_GetRealTimeInformation(positionSampleRef,NULL,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetRealTimeInformation***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetRealTimeInformation -> returning LE_FAULT");
    }

    //15.taf_locGnss_GetDRSolutionStatus - LE_FAULT scenario
    result = taf_locGnss_GetDRSolutionStatus(positionSampleRef,NULL);
    LE_TEST_OK(result == LE_FAULT, "***taf_locGnss_GetDRSolutionStatus***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetDRSolutionStatus -> returning LE_FAULT");
    }

    //16.taf_locGnss_GetMeasurementUsageInfo - LE_NO_MEMORY scenario
    result = taf_locGnss_GetMeasurementUsageInfo(positionSampleRef,NULL,&gnssMeasLen);
    LE_TEST_OK(result == LE_NO_MEMORY, "***taf_locGnss_GetMeasurementUsageInfo***-LE_NO_MEMORY");
    if(result == LE_NO_MEMORY)
    {
        LE_TEST_INFO("taf_locGnss_GetMeasurementUsageInfo -> returning LE_NO_MEMORY");
    }

    //17.taf_locGnss_GetMeasurementUsageInfo - LE_OUT_OF_RANGE scenario
    taf_locGnss_GnssMeasurementInfo_t measInfo[TAF_LOCGNSS_MEASUREMENT_INFO_MAX];
    result = taf_locGnss_GetMeasurementUsageInfo(positionSampleRef,measInfo,&gnssMeasLen_zero);
    LE_TEST_OK(result == LE_OUT_OF_RANGE, "***taf_locGnss_GetMeasurementUsageInfo***-LE_OUT_OF_RANGE");
    if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("taf_locGnss_GetMeasurementUsageInfo -> returning LE_OUT_OF_RANGE");
    }

    //18.taf_locGnss_GetMeasurementUsageInfo - LE_BAD_PARAMETER scenario
    result = taf_locGnss_GetMeasurementUsageInfo(NULL,measInfo,&gnssMeasLen);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_locGnss_GetMeasurementUsageInfo***-LE_BAD_PARAMETER");
    if(result == LE_BAD_PARAMETER)
    {
        LE_TEST_INFO("taf_locGnss_GetMeasurementUsageInfo -> returning LE_BAD_PARAMETER");
    }

    //19.taf_locGnss_GetReportStatus - LE_NO_MEMORY scenario
    result = taf_locGnss_GetReportStatus(positionSampleRef,NULL);
    LE_TEST_OK(result == LE_NO_MEMORY, "***taf_locGnss_GetReportStatus***-LE_NO_MEMORY");
    if(result == LE_NO_MEMORY)
    {
        LE_TEST_INFO("taf_locGnss_GetReportStatus -> returning LE_NO_MEMORY");
    }

    //20.taf_locGnss_GetReportStatus - LE_BAD_PARAMETER scenario
    result = taf_locGnss_GetReportStatus(NULL,&reportStatus);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_locGnss_GetReportStatus***-LE_BAD_PARAMETER");
    if(result == LE_BAD_PARAMETER)
    {
        LE_TEST_INFO("taf_locGnss_GetReportStatus -> returning LE_BAD_PARAMETER");
    }


    //21.taf_locGnss_GetAltitudeMeanSeaLevel - LE_NO_MEMORY scenario
    result = taf_locGnss_GetAltitudeMeanSeaLevel(positionSampleRef,NULL);
    LE_TEST_OK(result == LE_NO_MEMORY, "***taf_locGnss_GetAltitudeMeanSeaLevel***-LE_NO_MEMORY");
    if(result == LE_NO_MEMORY)
    {
        LE_TEST_INFO("taf_locGnss_GetAltitudeMeanSeaLevel -> returning LE_NO_MEMORY");
    }

    //22.taf_locGnss_GetAltitudeMeanSeaLevel - LE_BAD_PARAMETER scenario
    result = taf_locGnss_GetAltitudeMeanSeaLevel(NULL,&altMSeaLevel);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_locGnss_GetAltitudeMeanSeaLevel***-LE_BAD_PARAMETER");
    if(result == LE_BAD_PARAMETER)
    {
        LE_TEST_INFO("taf_locGnss_GetAltitudeMeanSeaLevel -> returning LE_BAD_PARAMETER");
    }


    //23.taf_locGnss_GetSVIds - LE_OUT_OF_RANGE scenario
    result = taf_locGnss_GetSVIds(positionSampleRef,svIds,&svIdlen_zero);
    LE_TEST_OK(result == LE_OUT_OF_RANGE, "***taf_locGnss_GetSVIds***-LE_OUT_OF_RANGE");
    if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("taf_locGnss_GetSVIds -> returning LE_OUT_OF_RANGE");
    }

    //24.taf_locGnss_GetSVIds - LE_BAD_PARAMETER scenario
    result = taf_locGnss_GetSVIds(NULL,svIds,&svIdlen_zero);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_locGnss_GetSVIds***-LE_BAD_PARAMETER");
    if(result == LE_BAD_PARAMETER)
    {
        LE_TEST_INFO("taf_locGnss_GetSVIds -> returning LE_BAD_PARAMETER");
    }

    //25.taf_locGnss_GetSVIds - LE_NO_MEMORY scenario
    result = taf_locGnss_GetSVIds(positionSampleRef,NULL,&svIdlen_zero);
    LE_TEST_OK(result == LE_NO_MEMORY, "***taf_locGnss_GetSVIds***-LE_NO_MEMORY");
    if(result == LE_NO_MEMORY)
    {
        LE_TEST_INFO("taf_locGnss_GetSVIds -> returning LE_NO_MEMORY");
    }


    //26.taf_locGnss_GetSatellitesInfoEx - LE_OUT_OF_RANGE scenario
    result = taf_locGnss_GetSatellitesInfoEx(positionSampleRef,constellation,svInfo,&satlen_zero);
    LE_TEST_OK(result == LE_OUT_OF_RANGE, "***taf_locGnss_GetSatellitesInfoEx***-LE_OUT_OF_RANGE");
    if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("taf_locGnss_GetSatellitesInfoEx -> returning LE_OUT_OF_RANGE");
    }

    //27.taf_locGnss_GetSatellitesInfoEx - LE_BAD_PARAMETER scenario
    result = taf_locGnss_GetSatellitesInfoEx(NULL,constellation,svInfo,&svIdlen_zero);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "***taf_locGnss_GetSatellitesInfoEx***-LE_BAD_PARAMETER");
    if(result == LE_BAD_PARAMETER)
    {
        LE_TEST_INFO("taf_locGnss_GetSatellitesInfoEx -> returning LE_BAD_PARAMETER");
    }

    //28.taf_locGnss_GetSatellitesInfoEx - LE_NO_MEMORY scenario
    result = taf_locGnss_GetSatellitesInfoEx(positionSampleRef,constellation,NULL,&svIdlen_zero);
    LE_TEST_OK(result == LE_NO_MEMORY, "***taf_locGnss_GetSatellitesInfoEx***-LE_NO_MEMORY");
    if(result == LE_NO_MEMORY)
    {
        LE_TEST_INFO("taf_locGnss_GetSatellitesInfoEx -> returning LE_NO_MEMORY");
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Position thread function to add position handler.
 */
//--------------------------------------------------------------------------------------------------
static void* PositionThread
(
    void* context
)
{
    LE_TEST_INFO("======== Position Handler thread  ========");
    taf_locGnss_ConnectService();

    //set the retrieval frequency as 1sec
    le_result_t result = taf_locGnss_SetAcquisitionRate(1000);

    result = taf_locGnss_Start();

    LE_TEST_INFO("Result of taf_locGnss_start: %d", (int)result);

    PositionHandlerRef = taf_locGnss_AddPositionHandler(PositionHandlerFunction, NULL);

    //137.Position Handler
    LE_TEST_OK((PositionHandlerRef != NULL),
            "Confirm position handler was added successfully");

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to create position thread.
 */
//--------------------------------------------------------------------------------------------------
static void TestTafGnssPositionHandler
(
    void
)
{
    le_thread_Ref_t positionThreadRef;
    LE_TEST_INFO("TestTafGnssPositionHandler");

    le_result_t result = taf_locGnss_Start();
    LE_TEST_INFO("Result of taf_locGnss_start: %d", (int)result);

    positionThreadRef = le_thread_Create("PositionThreadTest",PositionThread,NULL);
    le_thread_Start(positionThreadRef);
    le_thread_Sleep(1);

    taf_locGnss_RemovePositionHandler(PositionHandlerRef);
    le_thread_Cancel(positionThreadRef);

    result = taf_locGnss_Stop();
    LE_TEST_INFO("Result of taf_locGnss_stop: %d", (int)result);

}

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate location service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void locRetTest_RunApis
(
   void
)
{
    le_result_t cold_result = LE_FAULT;
    le_result_t warm_result = LE_FAULT;
    le_result_t result = LE_FAULT;
    uint32_t ttff = 0;
    uint16_t minGpsWeek = 1;
    taf_locGnss_NmeaBitMask_t nmeaMaskPtr;
    taf_locGnss_ConstellationBitMask_t constellationMask;
    taf_locGnss_NmeaBitMask_t nmeaMask = TAF_LOCGNSS_NMEA_MASK_GPGGA;
    taf_locGnss_GeodeticDatumType_t datumType = TAF_LOCGNSS_GEODETIC_TYPE_WGS_84;
    uint16_t engineType = TAF_LOCGNSS_LOC_ENGINE_FUSED;
    taf_locGnss_XtraStatusParams_t *XtraParamsPtr;
    le_mem_PoolRef_t XtraFramePool = NULL;
    taf_locGnss_DRConfigValidityType_t drParamsMask = 0;
    drParamsMask |= TAF_LOCGNSS_BODY_TO_SENSOR_MOUNT_PARAMS_VALID;

    LE_TEST_INFO("locRetTest_RunApis");

	//1.taf_locGnss_StartMode -LE_FAULT scenario.
    result = taf_locGnss_StartMode(TAF_LOCGNSS_COLD_START);
    result = taf_locGnss_Stop();
    result = taf_locGnss_StartMode(TAF_LOCGNSS_COLD_START);
    LE_TEST_OK(result==LE_FAULT, "****taf_locGnss_StartMode***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_StartMode -> returning LE_FAULT");
    }
    le_thread_Sleep(10);

    //2.taf_locGnss_GetState() -> TAF_LOCGNSS_STATE_READY
    LE_TEST_OK(((taf_locGnss_GetState()) == TAF_LOCGNSS_STATE_READY), "***taf_locGnss_GetState***-Get GNSS state as READY");

    //3.taf_locGnss_GetState() -> TAF_LOCGNSS_STATE_DISABLED
	result = taf_locGnss_Disable();
    LE_TEST_OK(((taf_locGnss_GetState()) == TAF_LOCGNSS_STATE_DISABLED ), "***taf_locGnss_GetState***-Get GNSS state as DISABLED");
    result = taf_locGnss_Enable();

    //4.taf_locGnss_SetNmeaSentences - LE_NOT_PERMITTED scenario
    result = taf_locGnss_Disable();
    result = taf_locGnss_SetNmeaSentences(TAF_LOCGNSS_NMEA_MASK_GPGGA);
    LE_TEST_OK(result==LE_NOT_PERMITTED, "****taf_locGnss_SetNmeaSentences***-LE_NOT_PERMITTED");
    if(result == LE_NOT_PERMITTED)
    {
        LE_TEST_INFO("setNmeaSentences -> so returning LE_NOT_PERMITTED");
    }
    result = taf_locGnss_Enable();

    //5.taf_locGnss_GetSupportedNmeaSentences - LE_NOT_PERMITTED scenario
    result = taf_locGnss_Disable();
    result = taf_locGnss_GetSupportedNmeaSentences(&nmeaMaskPtr);
    LE_TEST_OK(result==LE_NOT_PERMITTED, "****taf_locGnss_GetSupportedNmeaSentences***-LE_NOT_PERMITTED");
    if(result == LE_NOT_PERMITTED)
    {
        LE_TEST_INFO("taf_locGnss_GetSupportedNmeaSentences -> so returning LE_NOT_PERMITTED");
    }
    result = taf_locGnss_Enable();

    //6.taf_locGnss_GetTtff() - LE_BUSY scenario
    result = taf_locGnss_GetTtff(&ttff);
    LE_TEST_OK(result == LE_BUSY, "***taf_locGnss_GetTtff***-LE_BUSY");
    if(result == LE_BUSY)
    {
        LE_TEST_INFO("TTFF -> returning LE_BUSY");
    }

    //7.taf_locGnss_ForceColdRestart() - LE_FAULT scenario
    taf_locGnss_Start();
    cold_result = taf_locGnss_ForceColdRestart();
    taf_locGnss_Stop();

    taf_locGnss_Start();
    cold_result = taf_locGnss_ForceColdRestart();
    LE_TEST_OK(cold_result == LE_FAULT, "***taf_locGnss_ForceColdRestart***-LE_FAULT");
    if(cold_result == LE_FAULT)
    {
        LE_TEST_INFO("Force cold restart->Request was made within 5 seconds -> so returning LE_FAULT");
    }
    le_thread_Sleep(10);

    //8.taf_locGnss_ForceWarmRestart() - LE_FAULT scenario
    taf_locGnss_Start();
	warm_result = taf_locGnss_ForceWarmRestart();
    taf_locGnss_Stop();

    taf_locGnss_Start();
    warm_result = taf_locGnss_ForceWarmRestart();
    LE_TEST_OK(warm_result == LE_FAULT, "***taf_locGnss_ForceWarmRestart***-LE_FAULT");
    if(warm_result == LE_FAULT)
    {
        LE_TEST_INFO("Force warm restart->Request was made within 5 seconds -> so returning LE_FAULT");
    }

    //9.taf_locGnss_GetConstellation() - LE_FAULT scenario
    result = taf_locGnss_SetConstellation(TAF_LOCGNSS_CONSTELLATION_DEFAULT);
    result = taf_locGnss_GetConstellation(&constellationMask);
    LE_TEST_OK(result == LE_FAULT,"***taf_locGnss_GetConstellation***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("Get constellation->returning LE_FAULT");
    }

    //10.taf_locGnss_SetMinGpsWeek() - LE_NOT_PERMITTED scenario
    taf_locGnss_Start();
    result = taf_locGnss_SetMinGpsWeek(minGpsWeek);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"***taf_locGnss_SetMinGpsWeek***-LE_NOT_PERMITTED");
    if(result == LE_NOT_PERMITTED)
    {
        LE_TEST_INFO("Set min GPS week->returning LE_NOT_PERMITTED");
    }
    taf_locGnss_Stop();

    //11.taf_locGnss_GetMinGpsWeek() - LE_FAULT scenario
    result = taf_locGnss_GetMinGpsWeek(NULL);
    LE_TEST_OK(result == LE_FAULT,"***taf_locGnss_GetMinGpsWeek***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("Get min GPS week->returning LE_FAULT");
    }

    //12.taf_locGnss_GetMinGpsWeek() - LE_NOT_PERMITTED scenario
    taf_locGnss_Disable();
    result = taf_locGnss_GetMinGpsWeek(&minGpsWeek);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"***taf_locGnss_GetMinGpsWeek***-LE_NOT_PERMITTED");
    if(result == LE_NOT_PERMITTED)
    {
        LE_TEST_INFO("Get min GPS week->returning LE_NOT_PERMITTED");
    }
    taf_locGnss_Enable();

    //13.taf_locGnss_GetCapabilities - LE_FAULT scenario
    result = taf_locGnss_GetCapabilities(NULL);
    LE_TEST_OK(result == LE_FAULT,"***taf_locGnss_GetCapabilities***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("Get capabilities->returning LE_FAULT");
    }

    //14. taf_locGnss_SetNmeaConfiguration - LE_NOT_PERMITTED scenario
    taf_locGnss_Disable();
    result = taf_locGnss_SetNmeaConfiguration(nmeaMask, datumType, engineType);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"***taf_locGnss_SetNmeaConfiguration***-LE_NOT_PERMITTED");
    if(result == LE_NOT_PERMITTED)
    {
        LE_TEST_INFO("SetNmeaConfiguration->returning LE_NOT_PERMITTED");
    }
    taf_locGnss_Enable();

    //15. taf_locGnss_SetNmeaConfiguration - LE_FAULT scenario
    datumType = TAF_LOCGNSS_GEODETIC_TYPE_NONE;
    result = taf_locGnss_SetNmeaConfiguration(nmeaMask, datumType, engineType);
    LE_TEST_OK(result == LE_FAULT,"***taf_locGnss_SetNmeaConfiguration***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("SetNmeaConfiguration->returning LE_FAULT");
    }

   //16. taf_locGnss_SetNmeaConfiguration - LE_BAD_PARAMETER scenario
    nmeaMask = 0;
    datumType = TAF_LOCGNSS_GEODETIC_TYPE_WGS_84;
    result = taf_locGnss_SetNmeaConfiguration(nmeaMask, datumType, engineType);
    LE_TEST_OK(result == LE_BAD_PARAMETER,"***taf_locGnss_SetNmeaConfiguration***-LE_BAD_PARAMETER");
    if(result == LE_BAD_PARAMETER)
    {
        LE_TEST_INFO("SetNmeaConfiguration->returning LE_BAD_PARAMETER");
    }

    //17.taf_locGnss_GetXtraStatus - LE_NOT_PERMITTED scenario
    taf_locGnss_Disable();
    XtraFramePool = le_mem_CreatePool("XtraFramePool", sizeof(taf_locGnss_XtraStatusParams_t));
    XtraParamsPtr = (taf_locGnss_XtraStatusParams_t*) le_mem_ForceAlloc(XtraFramePool);

    if(XtraParamsPtr != NULL)
    {
        LE_TEST_INFO("taf_locGnss_GetXtraStatus API is called to get xtra information");
        result = taf_locGnss_GetXtraStatus(XtraParamsPtr);
        LE_TEST_OK(result == LE_NOT_PERMITTED,"***taf_locGnss_GetXtraStatus***-LE_NOT_PERMITTED");
        if(result == LE_NOT_PERMITTED)
        {
            LE_TEST_INFO("taf_locGnss_GetXtraStatus->returning LE_NOT_PERMITTED");
        }
        le_mem_Release(XtraParamsPtr);
    }
    else
    {
        LE_TEST_INFO("XtraParamPtr is NULL pointer");
    }

    //18. taf_locGnss_SetDRConfigValidity - LE_NOT_PERMITTED scenario
    taf_locGnss_Disable();
    result = taf_locGnss_SetDRConfigValidity(drParamsMask);
    LE_TEST_OK(result == LE_NOT_PERMITTED,"***taf_locGnss_SetDRConfigValidity***-LE_NOT_PERMITTED");
    if(result == LE_NOT_PERMITTED)
    {
        LE_TEST_INFO("taf_locGnss_SetDRConfigValidity->returning LE_NOT_PERMITTED");
    }
    taf_locGnss_Enable();

    //19. taf_locGnss_GetLeapSeconds - LE_FAULT scenario
    result =taf_locGnss_GetLeapSeconds(NULL,NULL,NULL,NULL);
    LE_TEST_OK(result == LE_FAULT,"***taf_locGnss_GetLeapSeconds***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetLeapSeconds->returning LE_FAULT");
    }

    //20 taf_locGnss_GetSupportedConstellations- LE_FAULT scenario
    result = taf_locGnss_GetSupportedConstellations(NULL);
    LE_TEST_OK(result == LE_FAULT,"***taf_locGnss_GetSupportedConstellations***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetSupportedConstellations->returning LE_FAULT");
    }

    //21.taf_locPos_SetAcquisitionRate - LE_OUT_OF_RANGE scenario
    result = taf_locPos_SetAcquisitionRate(0);
    LE_TEST_OK(result == LE_OUT_OF_RANGE,"***taf_locPos_SetAcquisitionRate***-LE_OUT_OF_RANGE");
    if(result == LE_OUT_OF_RANGE)
    {
        LE_TEST_INFO("taf_locPos_GetMOtion->returning LE_OUT_OF_RANGE");
    }

    //22 taf_locGnss_GetSupportedNmeaSentences- LE_FAULT scenario
    result = taf_locGnss_GetSupportedNmeaSentences(NULL);
    LE_TEST_OK(result == LE_FAULT,"***taf_locGnss_GetSupportedNmeaSentences***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetSupportedNmeaSentences->returning LE_FAULT");
    }

    //23 taf_locGnss_RobustLocationInformation- LE_FAULT scenario
    result = taf_locGnss_RobustLocationInformation(NULL,NULL,NULL,NULL);
    LE_TEST_OK(result == LE_FAULT,"***taf_locGnss_RobustLocationInformation***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_RobustLocationInformation->returning LE_FAULT");
    }

    //24 taf_locGnss_GetXtraStatus- LE_FAULT scenario
    result = taf_locGnss_GetXtraStatus(NULL);
    LE_TEST_OK(result == LE_FAULT,"***taf_locGnss_GetXtraStatus***-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_GetXtraStatus->returning LE_FAULT");
    }

    //25. taf_locGnss_SetConstellation = LE_FAULT scenario
    result = taf_locGnss_SetConstellation(256);
    LE_TEST_OK(result == LE_FAULT,"taf_locGnss_SetConstellation-LE_FAULT");
    if(result == LE_FAULT)
    {
        LE_TEST_INFO("taf_locGnss_SetConstellation->returning LE_FAULT");
    }

    TestTafGnssPositionHandler();
}