/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <sys/time.h>

#include "legato.h"
#include "interfaces.h"

#define MILLIARCSECONDS_IN_A_DEGREE 3.6

static taf_ecall_StateChangeHandlerRef_t stateHandlerRef;
static taf_ecall_CallRef_t eCallRef = NULL;

static taf_audio_RouteRef_t routeRef;
static taf_audio_StreamRef_t mdmRxAudioRef;
static taf_audio_StreamRef_t mdmTxAudioRef;
static taf_audio_StreamRef_t playerRef;
static taf_audio_ConnectorRef_t playerConnectorRef;
static taf_audio_StreamRef_t feInRef = NULL;
static taf_audio_StreamRef_t feOutRef = NULL;
static taf_audio_ConnectorRef_t audioInputConnectorRef;
static taf_audio_ConnectorRef_t audioOutputConnectorRef;
static taf_audio_MediaHandlerRef_t mediaHandlerRef = NULL;
static char audioFilePath[] = "/data/record.wav";
static le_result_t res;

static le_thread_Ref_t positionThreadRef;
static le_thread_Ref_t positionExThreadRef;
static taf_locGnss_PositionHandlerRef_t positionHandlerRef;
static taf_locGnss_PositionExHandlerRef_t PositionExHandlerRef;
static int32_t prevLatitude = INT32_MAX, prevLongitude = INT32_MAX;
static int32_t latitude = INT32_MAX, longitude = INT32_MAX, hAccuracy = INT32_MAX;
static int32_t latitudeDeltaN1 = 0, longitudeDeltaN1 = 0;
static int32_t latitudeDeltaN2 = 0, longitudeDeltaN2 = 0;
static uint32_t direction = UINT32_MAX, dirAccuracy = UINT32_MAX;
static bool isPosTrusted = false;

static taf_mngdPm_wsRef_t wsRef;

static le_thread_Ref_t configDetectorRef;
static le_cfg_ChangeHandlerRef_t callTriggerHandlerRef;
static le_event_Id_t eCallEventId;
static le_event_HandlerRef_t eCallHandlerRef;

static void InitializeWakeSource()
{
    wsRef = taf_mngdPm_CreateWakeupSource(TAF_MNGDPM_STAY_AWAKE_REASON_ECALL_ACTIVE,
                                          TAF_MNGDPM_WS_OPT_DEFAULT,
                                          "eCallws");
    if(wsRef != NULL) {
        LE_INFO("Wakeup source created successfully");
    } else {
        LE_ERROR("Failed to create thewakeup source");
    }
}

static void AcquireWakeSource()
{
    res = taf_mngdPm_StayAwake(wsRef);
    if(res == LE_OK) {
        LE_INFO("Wake source acquired successfully");
    } else {
        LE_ERROR("Failed to acquire the wake source");
    }
}

static void ReleaseWakeSource()
{
    res = taf_mngdPm_Relax(wsRef);
    if(res == LE_OK) {
        LE_INFO("wake source released successfully");
    } else {
        LE_ERROR("Failed to release the wake source");
    }
}

static void PositionHandlerFunction
(
    taf_locGnss_SampleRef_t positionSampleRef,
    void* contextPtr
)
{
    le_result_t result;

    prevLatitude = latitude;
    prevLongitude = longitude;
    latitudeDeltaN2 = latitudeDeltaN1;
    longitudeDeltaN2 = longitudeDeltaN1;

    //Get 2D location
    result = taf_locGnss_GetLocation(positionSampleRef,
                                  &latitude,
                                  &longitude,
                                  &hAccuracy);

    if (result == LE_OK)
    {
        latitude = (int32_t)(latitude * MILLIARCSECONDS_IN_A_DEGREE);
        longitude = (int32_t)(longitude * MILLIARCSECONDS_IN_A_DEGREE);
        latitudeDeltaN1 = prevLatitude == INT32_MAX ? 0 : prevLatitude - latitude;
        longitudeDeltaN1 = prevLongitude == INT32_MAX ? 0 : prevLongitude - longitude;
        // TODO: check logic for isPosTrusted
        isPosTrusted = (2*hAccuracy/100 <= 150);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_WARN("Location invalid [%d, %d, %d]", latitude, longitude, hAccuracy);

        latitudeDeltaN1 = 0;
        latitudeDeltaN2 = 0;
        isPosTrusted = false;

        if (latitude != INT32_MAX)
        {
            latitude = (int32_t)(latitude * MILLIARCSECONDS_IN_A_DEGREE);
            latitudeDeltaN1 = prevLatitude == INT32_MAX ? 0 : prevLatitude - latitude;
        }
        if (longitude != INT32_MAX)
        {
            longitude = (int32_t)(longitude * MILLIARCSECONDS_IN_A_DEGREE);
            longitudeDeltaN1 = prevLongitude == INT32_MAX ? 0 : prevLongitude - longitude;
        }
    }
    else
    {
        LE_ERROR("Failed! to get 2D Location information");

        latitude = INT32_MAX;
        longitude = INT32_MAX;
        latitudeDeltaN1 = 0;
        longitudeDeltaN1 = 0;
        isPosTrusted = false;
    }

    //Get direction
    result = taf_locGnss_GetDirection(positionSampleRef,
                                   &direction,
                                   &dirAccuracy);

    if (result == LE_OK)
    {
        direction = direction/20;
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        LE_WARN("Direction invalid [%u, %u]", direction, dirAccuracy);
    }
    else
    {
        LE_ERROR("Failed to get position direction information");
    }

    taf_locGnss_ReleaseSampleRef(positionSampleRef);
}

static void* SamplePositionThread
(
    void* context
)
{
    //connect the position service to the current running thread
    taf_locGnss_ConnectService();

    if (taf_locGnss_SetAcquisitionRate(1000) == LE_OK) {
        LE_INFO("Acquisition rate set to 1Hz successfully");
    }
    else {
        LE_ERROR("Failed to set acquisition rate");
    }

    le_result_t result = taf_locGnss_Start();
    if (result == LE_OK) {
        LE_INFO("Location GNSS started successfully");
    }
    else {
        LE_ERROR("Failed to start Location GNSS");
        return NULL;
    }

    //Position Handler
    positionHandlerRef = taf_locGnss_AddPositionHandler(PositionHandlerFunction, NULL);
    if(positionHandlerRef != NULL) {
        LE_INFO("Confirm position handler was added successfully %p", positionHandlerRef);
        le_event_RunLoop();
    }
    else {
        LE_ERROR("Failed to add position handler");
    }

    return NULL;
}

static void PositionExHandlerFunction
(
    taf_locGnss_SampleExRef_t SampleExRef,
    const taf_locGnss_PositionSampleEx_t* locationData,
    void* contextPtr
)
{
    LE_INFO("*****Basic Location information***********");

    LE_INFO("Epoch Time                : %" PRIu64 "\n", locationData->epochTime);

    struct timeval tv;

    struct tm* tm_info;

    gettimeofday(&tv, NULL);

    tm_info = localtime(&tv.tv_sec);

    LE_INFO("Time: %02d:%02d:%02d.%03ld\n",

           tm_info->tm_hour,

           tm_info->tm_min,

           tm_info->tm_sec,

           tv.tv_usec / 1000);  // convert microseconds to milliseconds


    LE_INFO("Latitude(positive->north) : %.6f\n"
               "Longitude(positive->east) : %.6f\n"
               "hAccuracy                 : %.2fm\n",
                (float)locationData->latitude/1e6,
                (float)locationData->longitude/1e6,
                (float)locationData->hAccuracy/1e2);

    LE_INFO("Direction(0 degree is True North) : %.1f degrees\n"
               "Direction Accuracy                : %.1f degrees\n",
               (float)locationData->direction/10.0,
               (float)locationData->directionAccuracy/10.0);


    LE_INFO("Altitude                  : %.3fm\n"
               "vAccuracy                 : %.1fm\n",
               (float)locationData->altitude/1e3,
               (float)locationData->vAccuracy/10.0);

    LE_INFO("hSpeed        : %.2fm/s\n"
               "Accuracy      : %.1fcm/s\n",
                locationData->hSpeed/100.0,
                locationData->hSpeedAccuracy/10.0);

    LE_INFO("Elapsed real time              : %" PRIu64 " ns\n",locationData->realTime);
    LE_INFO("Elapsed real time uncertainity : %" PRIu64 " ns\n",locationData->realTimeUnc);

    LE_INFO("SatsInView: %d, SatsTracking: %d and SatsUsed: %d\n",
               (locationData->satsInViewCount == UINT8_MAX) ? 0: locationData->satsInViewCount,
               (locationData->satsTrackingCount == UINT8_MAX) ? 0: locationData->satsTrackingCount,
               (locationData->satsUsedCount == UINT8_MAX) ? 0: locationData->satsUsedCount);

    LE_INFO("%.1f degrees\n", (float)(locationData->magneticDeviation/10.0));
    LE_INFO("Altitude with respect to mean sea level: %lfm\n",(float)locationData->altMeanSeaLevel);

    LE_INFO("\nTechnology used to compute fix: The ");
    if(locationData->techMask & TAF_LOCGNSS_LOC_GNSS)
    {
        LE_INFO("location calculated using GNSS\n");
    }
    if(locationData->techMask & TAF_LOCGNSS_LOC_CELL)
    {
        LE_INFO("location calculated using CELL\n");
    }
    if(locationData->techMask & TAF_LOCGNSS_LOC_WIFI)
    {
        LE_INFO("location calculated using WIFI\n");
    }
    if(locationData->techMask & TAF_LOCGNSS_LOC_SENSORS)
    {
        LE_INFO("location calculated using SENSORS\n");
    }
    if(locationData->techMask & TAF_LOCGNSS_LOC_REFERENCE_LOCATION)
    {
        LE_INFO("location calculated using reference location\n");
    }
    if(locationData->techMask & TAF_LOCGNSS_LOC_INJECTED_COARSE_POSITION)
    {
        LE_INFO("location calculated using Coarse position injected\n");
    }
    if(locationData->techMask & TAF_LOCGNSS_LOC_AFLT)
    {
        LE_INFO("location calculated using AFLT\n");
    }
    if(locationData->techMask & TAF_LOCGNSS_LOC_HYBRID)
    {
        LE_INFO("location calculated using GNSS and network-provided measurements\n");
    }
    if(locationData->techMask & TAF_LOCGNSS_LOC_PPE)
    {
        LE_INFO("location calculated using Precise position engine\n");
    }
    if(locationData->techMask & TAF_LOCGNSS_LOC_VEH)
    {
        LE_INFO("location calculated using Vehicular data\n");
    }
    if(locationData->techMask & TAF_LOCGNSS_LOC_VIS)
    {
        LE_INFO("location calculated using Visual data\n");
    }
    if(locationData->techMask & TAF_LOCGNSS_LOC_PROPAGATED)
    {
        LE_INFO("location calculated using propagation logic\n");
    }

    LE_INFO("\n** Location Info Validity Information ***\n");
    if(locationData->validityMask & TAF_LOCGNSS_HAS_LAT_LONG_BIT)
    {
        LE_INFO("valid latitude longitude\n");
    }
    if(locationData->validityMask & TAF_LOCGNSS_HAS_ALTITUDE_BIT)
    {
        LE_INFO("valid altitude\n");
    }
    if(locationData->validityMask & TAF_LOCGNSS_HAS_SPEED_BIT)
    {
        LE_INFO("valid speed\n");
    }
    if(locationData->validityMask & TAF_LOCGNSS_HAS_HEADING_BIT)
    {
        LE_INFO("valid heading\n");
    }
    if(locationData->validityMask & TAF_LOCGNSS_HAS_HORIZONTAL_ACCURACY_BIT)
    {
        LE_INFO("valid horizontal accuracy\n");
    }
    if(locationData->validityMask & TAF_LOCGNSS_HAS_VERTICAL_ACCURACY_BIT)
    {
        LE_INFO("valid vertical accuracy\n");
    }
    if(locationData->validityMask & TAF_LOCGNSS_HAS_SPEED_ACCURACY_BIT)
    {
        LE_INFO("valid speed accuracy \n");
    }
    if(locationData->validityMask & TAF_LOCGNSS_HAS_HEADING_ACCURACY_BIT)
    {
        LE_INFO("valid heading accuracy\n");
    }
    if(locationData->validityMask & TAF_LOCGNSS_HAS_TIMESTAMP_BIT)
    {
        LE_INFO("valid timestamp\n");
    }
    if(locationData->validityMask & TAF_LOCGNSS_HAS_ELAPSED_REAL_TIME_BIT)
    {
        LE_INFO("valid elapsed real time\n");
    }
    if(locationData->validityMask & TAF_LOCGNSS_HAS_ELAPSED_REAL_TIME_UNC_BIT)
    {
        LE_INFO("valid elapsed real time Uncertainity\n");
    }
    if(locationData->validityMask & TAF_LOCGNSS_HAS_GPTP_TIME_BIT)
    {
        LE_INFO("valid gptp time\n");
    }
    if(locationData->validityMask & TAF_LOCGNSS_HAS_GPTP_TIME_UNC_BIT)
    {
        LE_INFO("valid gptp time Uncertainity\n");
    }

    taf_locGnss_ReleaseSampleExRef(SampleExRef);
}

static void* ExtendPositionThread
(
    void* context
)
{
    //connect the position service to the current running thread
    taf_locGnss_ConnectService();

    if (taf_locGnss_SetAcquisitionRate(1000) == LE_OK) {
        LE_INFO("Acquisition rate set to 1Hz successfully");
    }
    else {
        LE_ERROR("Failed to set acquisition rate");
    }

    le_result_t result = taf_locGnss_Start();
    if (result == LE_OK) {
        LE_INFO("Location GNSS started successfully");
    }
    else {
        LE_ERROR("Failed to start Location GNSS");
        return NULL;
    }

    //Position Handler
    PositionExHandlerRef = taf_locGnss_AddPositionExHandler(PositionExHandlerFunction, NULL);
    if(PositionExHandlerRef != NULL) {
        LE_INFO("Confirm position handler was added successfully %p", PositionExHandlerRef);
        le_event_RunLoop();
    }
    else {
        LE_ERROR("Failed to add position handler");
    }

    return NULL;
}

static void MyMediaEventHandler
(
    taf_audio_StreamRef_t          streamRef,
    taf_audio_MediaEvent_t         event,
    void*                          contextPtr
)
{
    switch(event)
    {
        case TAF_AUDIO_MEDIA_STOPPED:
        LE_INFO("File event is TAF_AUDIO_MEDIA_STOPPED.");
        break;

        case TAF_AUDIO_MEDIA_ENDED:
        LE_INFO("File event is taf_audio_MEDIA_ENDED.");
        res = taf_audio_PlayFile(playerRef, audioFilePath);
        LE_ERROR_IF((res!=LE_OK), "Failed to play the file!");
        if(res == LE_OK)
            LE_INFO("Successfully started file playback");
        break;

        case TAF_AUDIO_MEDIA_ERROR:
        LE_INFO("File event is taf_audio_MEDIA_ERROR.");
        break;

        case TAF_AUDIO_MEDIA_NO_MORE_SAMPLES:
        LE_INFO("File event is taf_audio_MEDIA_NO_MORE_SAMPLES.");
        break;

        default:
        LE_INFO("File event is %d", event);
        break;
    }
}

static void DisconnectAllAudio()
{
    LE_INFO("DisconnectAllAudio");

    if(mediaHandlerRef)
    {
        taf_audio_RemoveMediaHandler(mediaHandlerRef);
        mediaHandlerRef = NULL;
    }
    if (audioInputConnectorRef)
    {
        if (feInRef)
        {
            LE_INFO("Disconnect %p from connector.%p", feInRef, audioInputConnectorRef);
            taf_audio_Disconnect(audioInputConnectorRef, feInRef);
        }
        if(mdmTxAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", mdmTxAudioRef, audioInputConnectorRef);
            taf_audio_Disconnect(audioInputConnectorRef, mdmTxAudioRef);
        }
    }
    if(audioOutputConnectorRef)
    {
        if(feOutRef)
        {
            LE_INFO("Disconnect %p from connector.%p", feOutRef, audioOutputConnectorRef);
            taf_audio_Disconnect(audioOutputConnectorRef, feOutRef);
        }
        if(mdmRxAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", mdmRxAudioRef, audioOutputConnectorRef);
            taf_audio_Disconnect(audioOutputConnectorRef, mdmRxAudioRef);
        }
    }

    if(playerConnectorRef)
    {
        if(playerRef)
        {
            LE_INFO("Disconnect %p from connector.%p", playerRef, playerConnectorRef);
            taf_audio_Disconnect(playerConnectorRef, playerRef);
        }
    }

    if(audioInputConnectorRef)
    {
        taf_audio_DeleteConnector(audioInputConnectorRef);
        audioInputConnectorRef = NULL;
    }

    if(audioOutputConnectorRef)
    {
        taf_audio_DeleteConnector(audioOutputConnectorRef);
        audioOutputConnectorRef = NULL;
    }

    if(playerConnectorRef)
    {
        taf_audio_DeleteConnector(playerConnectorRef);
        playerConnectorRef = NULL;
    }

    if(mdmRxAudioRef)
    {
        taf_audio_Close(mdmRxAudioRef);
        mdmRxAudioRef = NULL;
    }
    if(mdmTxAudioRef)
    {
        taf_audio_Close(mdmTxAudioRef);
        mdmTxAudioRef = NULL;
    }
    if(playerRef)
    {
        taf_audio_Close(playerRef);
        playerRef = NULL;
    }
    if(routeRef)
    {
        le_result_t result = taf_audio_CloseRoute(routeRef);
        if (result == LE_OK)
        {
            LE_INFO("Successfully closed the voice call route");
        }
        routeRef = NULL;
    }
}

static void OpenVoiceAudio()
{
    LE_INFO("OpenVoiceAudio");

    mdmRxAudioRef = taf_audio_OpenModemVoiceRx(1); // SlotId input param
    LE_ERROR_IF((mdmRxAudioRef==NULL), "taf_audio_OpenModemVoiceRx returns NULL!");
    LE_DEBUG("OpenAudio mdmRxAudioRef %p", mdmRxAudioRef);
    LE_INFO("Connect Speaker");

    audioOutputConnectorRef = taf_audio_CreateConnector();
    LE_ERROR_IF((audioOutputConnectorRef==NULL), "audioOutputConnectorRef is NULL!");

    mdmTxAudioRef =  taf_audio_OpenModemVoiceTx(1, false);
    LE_ERROR_IF((mdmTxAudioRef==NULL), "taf_audio_OpenModemVoiceTx returns NULL!");
    LE_DEBUG("OpenAudio mdmTxAudioRef %p", mdmTxAudioRef);

    audioInputConnectorRef = taf_audio_CreateConnector();
    LE_ERROR_IF((audioInputConnectorRef==NULL), "audioInputConnectorRef is NULL!");

    if (mdmTxAudioRef && feInRef && audioInputConnectorRef)
    {
        res = taf_audio_Connect(audioInputConnectorRef, feInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect RX on Input connector!");
        res = taf_audio_Connect(audioInputConnectorRef, mdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
    }

    if (mdmRxAudioRef && feOutRef  && audioOutputConnectorRef)
    {
        res = taf_audio_Connect(audioOutputConnectorRef, feOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect TX on Output connector!");
        res = taf_audio_Connect(audioOutputConnectorRef, mdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
    LE_INFO("Set volume to modem RX");
    res = taf_audio_SetVolume(mdmRxAudioRef, 1);
    LE_ERROR_IF((res!=LE_OK), "Failed to set volume on modem RX!");
    if(res == LE_OK)
        LE_INFO("Successfully set the volume to modem RX");
    LE_INFO("Set mute to modem RX");
    res = taf_audio_SetMute(mdmRxAudioRef, true);
    LE_ERROR_IF((res!=LE_OK), "Failed to mute modem RX!");
    if(res == LE_OK)
        LE_INFO("Successfully muted modem RX");
    LE_INFO("Set mute to modem TX");
    res = taf_audio_SetMute(mdmTxAudioRef, true);
    LE_ERROR_IF((res!=LE_OK), "Failed to mute modem TX!");
    if(res == LE_OK)
        LE_INFO("Successfully muted modem TX");

    return;
}

static void ConnectAudio()
{
    LE_INFO("ConnectAudio");

    routeRef = taf_audio_OpenRoute(TAF_AUDIO_ROUTE_1, TAF_AUDIO_VOICE_CALL, &feOutRef, &feInRef);
    LE_ERROR_IF((routeRef == NULL), "routeRef  is NULL!");

    //Open player ConnectorRef
    playerConnectorRef  = taf_audio_CreateConnector();
    LE_ERROR_IF((playerConnectorRef ==NULL), "playerConnectorRef  is NULL!");
    //Open playerRef
    playerRef = taf_audio_OpenPlayer(TAF_AUDIO_RX);
    LE_ERROR_IF((playerRef==NULL), "OpenFilePlayback returns NULL!");
    //Add playerRef to AddMediaHandler to get the audio playback callbacks.
    mediaHandlerRef = taf_audio_AddMediaHandler(playerRef, MyMediaEventHandler, NULL);
    LE_ERROR_IF((mediaHandlerRef ==NULL), "mediaHandlerRef  is NULL!");

    if (feOutRef && playerConnectorRef && playerRef )
    {
        //connect both speaker and playerref to player connectorRef
        res = taf_audio_Connect(playerConnectorRef, feOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect Speaker on Output connector (res %s)!",
                    LE_RESULT_TXT(res));
        res = taf_audio_Connect(playerConnectorRef , playerRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect FilePlayback on input connector!");

        //To playfile audio file after connecting connectors
        res = taf_audio_PlayFile(playerRef, audioFilePath);
        LE_ERROR_IF((res!=LE_OK), "Failed to play the file!");
        res = taf_audio_SetVolume(playerRef, 1);
        LE_ERROR_IF((res!=LE_OK), "Failed to set volume on player!");
        if(res == LE_OK)
            LE_INFO("Successfully set the volume to player");
    }

    OpenVoiceAudio();
}

static void SignalHandler (int sigNum)
{
    LE_INFO("Exit eCall reference app");
    if (eCallRef)
    {
        taf_ecall_End(eCallRef);
        taf_ecall_Delete(eCallRef);
        eCallRef = NULL;
    }

    taf_locGnss_Stop();
    _exit(EXIT_SUCCESS);
}

static void ECallStateHandler( taf_ecall_CallRef_t eCallReference,
        taf_ecall_State_t state, void* cntxtPtr)
{
    LE_INFO("Ecall state change event, state = %d", state );
    LE_INFO("Ecall state change event, reference = %p", eCallReference );

    switch (state)
    {
        case TAF_ECALL_STATE_UNKNOWN:
        {
            LE_INFO("TAF_ECALL_STATE_UNKNOWN");
            break;
        }
        case TAF_ECALL_STATE_ALERTING:
        {
            LE_INFO("TAF_ECALL_STATE_ALERTING");
            break;
        }
        case TAF_ECALL_STATE_ACTIVE:
        {
            LE_INFO("TAF_ECALL_STATE_ACTIVE");
            break;
        }
        case TAF_ECALL_STATE_IDLE:
        {
            LE_INFO("TAF_ECALL_STATE_IDLE");
            break;
        }
        case TAF_ECALL_STATE_WAITING_PSAP_START_IND:
        {
            LE_INFO("TAF_ECALL_STATE_WAITING_PSAP_START_IND");
            break;
        }
        case TAF_ECALL_STATE_PSAP_START_RECEIVED:
        {
            LE_INFO("TAF_ECALL_STATE_PSAP_START_RECEIVED");
            break;
        }
        case TAF_ECALL_STATE_MSD_TRANSMISSION_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_MSD_TRANSMISSION_STARTED");
            break;
        }
        case TAF_ECALL_STATE_LLNACK_RECEIVED:
        {
            LE_INFO("TAF_ECALL_STATE_LLNACK_RECEIVED");
            break;
        }
        case TAF_ECALL_STATE_LL_ACK_RECEIVED:
        {
            LE_INFO("TAF_ECALL_STATE_LL_ACK_RECEIVED");
            break;
        }
        case TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS:
        {
            LE_INFO("TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS");
            taf_audio_Stop(playerRef);
            res = taf_audio_SetMute(mdmRxAudioRef, false);
            LE_ERROR_IF((res!=LE_OK), "Failed to unmute modem RX!");
            res = taf_audio_SetMute(mdmTxAudioRef, false);
            LE_ERROR_IF((res!=LE_OK), "Failed to unmute modem TX!");
            break;
        }
        case TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED:
        {
            LE_INFO("TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED");
            taf_audio_Stop(playerRef);
            res = taf_audio_SetMute(mdmRxAudioRef, false);
            LE_ERROR_IF((res!=LE_OK), "Failed to unmute modem RX!");
            res = taf_audio_SetMute(mdmTxAudioRef, false);
            LE_ERROR_IF((res!=LE_OK), "Failed to unmute modem TX!");
            break;
        }
        case TAF_ECALL_STATE_ALACK_RECEIVED_POSITIVE:
        {
            LE_INFO("TAF_ECALL_STATE_ALACK_RECEIVED_POSITIVE");
            break;
        }
        case TAF_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN:
        {
            LE_INFO("TAF_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN");
            break;
        }
        case TAF_ECALL_STATE_MSD_UPDATE_REQ:
        {
            LE_INFO("TAF_ECALL_STATE_MSD_UPDATE_REQ");
            int res = taf_ecall_SendMsd(eCallReference);
            LE_INFO("Send MSD update, res: %d", res);
            break;
        }
        case TAF_ECALL_STATE_ENDED:
        {
            LE_INFO("TAF_ECALL_STATE_ENDED");
            if (eCallReference != NULL)
            {
                taf_ecall_TerminationReason_t lcf = taf_ecall_GetTerminationReason(eCallReference);
                LE_INFO("ECall ENDed, terminate reason  = %d", lcf);
            }
            DisconnectAllAudio();
            break;
        }
        case TAF_ECALL_STATE_RESET:
        {
            LE_INFO("TAF_ECALL_STATE_RESET");
            break;
        }
        case TAF_ECALL_STATE_COMPLETED:
        {
            LE_INFO("TAF_ECALL_STATE_COMPLETED");
            break;
        }
        case TAF_ECALL_STATE_FAILED:
        {
            LE_INFO("TAF_ECALL_STATE_FAILED");
            break;
        }
        case TAF_ECALL_STATE_END_OF_REDIAL_PERIOD:
        {
            LE_INFO("TAF_ECALL_STATE_END_OF_REDIAL_PERIOD");
            break;
        }
        case TAF_ECALL_STATE_T2_EXPIRED:
        {
            LE_INFO("TAF_ECALL_STATE_T2_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_TIMEOUT_T3:
        {
            LE_INFO("TAF_ECALL_STATE_TIMEOUT_T3");
            break;
        }
        case TAF_ECALL_STATE_T5_EXPIRED:
        {
            LE_INFO("TAF_ECALL_STATE_T5_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_T6_EXPIRED:
        {
            LE_INFO("TAF_ECALL_STATE_T6_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_T7_EXPIRED:
        {
            LE_INFO("TAF_ECALL_STATE_T7_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_T9_EXPIRED:
        {
            LE_INFO("TAF_ECALL_STATE_T9_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_T10_EXPIRED:
        {
            LE_INFO("TAF_ECALL_STATE_T10_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_DIALING:
        {
            LE_INFO("TAF_ECALL_STATE_DIALING");
            break;
        }
        case TAF_ECALL_STATE_NACK_OUT_OF_ORDER:
        {
            LE_INFO("TAF_ECALL_STATE_NACK_OUT_OF_ORDER");
            break;
        }
        case TAF_ECALL_STATE_ACK_OUT_OF_ORDER:
        {
            LE_INFO("TAF_ECALL_STATE_ACK_OUT_OF_ORDER");
            break;
        }
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_STARTED");
            break;
        }
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_SUCCESS:
        {
            LE_INFO("TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_SUCCESS");
            break;
        }
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_FAILURE:
        {
            LE_INFO("TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_FAILURE");
            break;
        }
        case TAF_ECALL_STATE_T2_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_T2_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T5_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_T5_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T6_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_T6_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T7_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_T7_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T9_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_T9_STARTED");
            ReleaseWakeSource();
            break;
        }
        case TAF_ECALL_STATE_T10_STARTED:
        {
            LE_INFO("TAF_ECALL_STATE_T10_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T2_STOPPED:
        {
            LE_INFO("TAF_ECALL_STATE_T2_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T5_STOPPED:
        {
            LE_INFO("TAF_ECALL_STATE_T5_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T6_STOPPED:
        {
            LE_INFO("TAF_ECALL_STATE_T6_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T7_STOPPED:
        {
            LE_INFO("TAF_ECALL_STATE_T7_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T9_STOPPED:
        {
            LE_INFO("TAF_ECALL_STATE_T9_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T10_STOPPED:
        {
            LE_INFO("TAF_ECALL_STATE_T10_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_INCOMING:
        {
            LE_INFO("TAF_ECALL_STATE_INCOMING");
            break;
        }
        case TAF_ECALL_STATE_LL_NACK_DUE_TO_T7_EXPIRY:
        {
            LE_INFO("TAF_ECALL_STATE_LL_NACK_DUE_TO_T7_EXPIRY");
            break;
        }
        case TAF_ECALL_STATE_T9_RESUMED:
        {
            LE_INFO("TAF_ECALL_STATE_T9_RESUMED");
            break;
        }
        default:
        {
            LE_INFO("Unknown state");
            break;
        }
    }
    LE_INFO("==============================================");
}

static void UpdateMsdVehicleInformation()
{
    // TODO: currently hardcoded but maybe use vhal in future to get information?
    taf_ecall_MsdVehicleType_t vehType = TAF_ECALL_PASSENGER_VEHICLE_CLASS_M1;
    taf_ecall_PropulsionStorageType_t propulsionStorage = TAF_ECALL_PROP_TYPE_GASOLINE_TANK;

    uint32_t msdVersion = 0;
    if (taf_ecall_GetMsdVersion(&msdVersion) != LE_OK)
    {
        msdVersion = 2;
        if (taf_ecall_SetMsdVersion(msdVersion) != LE_OK)
        {
            LE_ERROR("Unable to set MSD version");
        }
    }

    if (taf_ecall_SetVIN("ECALLEXAMPLE02022") != LE_OK)
    {
        LE_ERROR("Unable to set vehicle identification number");
    }

    if (taf_ecall_SetVehicleType(vehType) != LE_OK)
    {
        LE_ERROR("Unable to set vehicle type");
    }

    if (taf_ecall_SetPropulsionType(propulsionStorage) != LE_OK)
    {
        LE_ERROR("Unable to set propulsion type");
    }
}

static void DetectECallType
(
    void* context
)
{
    char nameBuffer[] = "/tafECallRefApp";
    le_cfg_IteratorRef_t iteratorReadRef = le_cfg_CreateReadTxn(nameBuffer);

    if (le_cfg_NodeExists(iteratorReadRef, "eCallType") == false)
    {
        le_cfg_CancelTxn(iteratorReadRef);
        return;
    }

    int32_t eCallType = le_cfg_GetInt(iteratorReadRef, "eCallType", 0);

    switch (eCallType)
    {
        case TAF_ECALL_TYPE_AUTO:
        case TAF_ECALL_TYPE_MANUAL:
        case TAF_ECALL_TYPE_TEST:
        {
            le_event_Report(eCallEventId, &eCallType, sizeof(int32_t));
            break;
        }
        default:
        {
            LE_INFO("1: TEST, 2: AUTO, 3: MANUAL");
        }
    }

    le_cfg_CancelTxn(iteratorReadRef);
}

static void* SampleConfigThread
(
    void* context
)
{
    le_cfg_ConnectService();
    callTriggerHandlerRef = le_cfg_AddChangeHandler("/tafECallRefApp/eCallType", DetectECallType, NULL);
    if (callTriggerHandlerRef != NULL) {
        LE_INFO("Confirm call trigger handler was added successfully");
    }

    le_event_RunLoop();
    return NULL;
}

static void UpdateLocationInformation(taf_ecall_CallRef_t eCallRef)
{
    LE_INFO("UpdateLocationInformation latitude = %d ",latitude);
    LE_INFO("UpdateLocationInformation longitude = %d ",longitude);
    LE_INFO("UpdateLocationInformation direction = %d ",direction);
    LE_INFO("UpdateLocationInformation isPosTrusted = %d ",isPosTrusted);

    le_result_t result = taf_ecall_SetMsdPosition(eCallRef, isPosTrusted,
                                                         latitude,
                                                         longitude, direction);
    if (result != LE_OK)
    {
        LE_ERROR("Unable to set location information");
    }

    result = taf_ecall_SetMsdPositionN1(eCallRef, latitudeDeltaN1, longitudeDeltaN1);
    if (result != LE_OK)
    {
        LE_ERROR("Unable to set the position delta N-1 for MSD transmission.");
    }

    result = taf_ecall_SetMsdPositionN2(eCallRef, latitudeDeltaN2, longitudeDeltaN2);
    if (result != LE_OK)
    {
        LE_ERROR("Unable to set the position delta N-2 for MSD transmission.");
    }
}

static void StartECall
(
    void* reportPtr
)
{
    LE_INFO("****************************** STARTING ECALL");

    int32_t* eCallType = (int32_t*) reportPtr;

    // AcquireWakeSource();

    if (eCallRef == NULL)
    {
        eCallRef = taf_ecall_Create();
    }

    UpdateLocationInformation(eCallRef);

    // TODO: fix hardcoding
    taf_ecall_SetMsdPassengersCount(eCallRef, 2);

    switch(*eCallType)
    {
        case TAF_ECALL_TYPE_AUTO:
        {
            LE_INFO("TAF_ECALL_TYPE_AUTO");
            // TODO: add logic instead of hardcoding
            taf_ecall_SetMsdEuroNCAPLocationOfImpact(eCallRef, TAF_ECALL_LOI_FRONT);
            taf_ecall_SetMsdEuroNCAPIIDeltaV(eCallRef, 125, -45, 10);
            ConnectAudio();
            taf_ecall_StartAutomatic(eCallRef);
            break;
        }
        case TAF_ECALL_TYPE_MANUAL:
        {
            LE_INFO("TAF_ECALL_TYPE_MANUAL");
            taf_ecall_ResetMsdAdditionalData(eCallRef);
            ConnectAudio();
            taf_ecall_StartManual(eCallRef);
            break;
        }
        case TAF_ECALL_TYPE_TEST:
        {
            LE_INFO("TAF_ECALL_TYPE_TEST");
            ConnectAudio();
            taf_ecall_StartTest(eCallRef);
            break;
        }
        default:
        {
            LE_ERROR("Should not reach here");
            return;
        }
    }
    LE_INFO("****************************** STARTED ECALL");
}

COMPONENT_INIT
{
    LE_INFO("ECall Reference App COMPONENT_INIT...");
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    InitializeWakeSource();
    AcquireWakeSource();

    UpdateMsdVehicleInformation();

    // position listener
    positionThreadRef = le_thread_Create("positionThreadRef", SamplePositionThread, NULL);
    LE_INFO("fetchLocationInfo positionThreadRef: %p", positionThreadRef);
    le_thread_Start(positionThreadRef);

    // position listener
    positionExThreadRef = le_thread_Create("positionExThreadRef", ExtendPositionThread, NULL);
    LE_INFO("fetchLocationInfo positionExThreadRef: %p", positionExThreadRef);
    le_thread_Start(positionExThreadRef);

    // eCall config detector
    configDetectorRef = le_thread_Create("configDetectorRef", SampleConfigThread, NULL);
    LE_INFO("detectECallTrigger configDetectorRef: %p", configDetectorRef);
    le_thread_Start(configDetectorRef);

    // eCall event handler
    eCallEventId = le_event_CreateId("ECall event", sizeof(int32_t));
    eCallHandlerRef = le_event_AddHandler("ECall handler", eCallEventId, StartECall);

    // state change handler
    stateHandlerRef = taf_ecall_AddStateChangeHandler(ECallStateHandler, NULL);
}