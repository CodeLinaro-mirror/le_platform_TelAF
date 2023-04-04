/*
* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the
* disclaimer below) provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following
* disclaimer in the documentation and/or other materials provided
* with the distribution.
*
* * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
* contributors may be used to endorse or promote products derived
* from this software without specific prior written permission.
*
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

#include <sys/time.h>

#include "legato.h"
#include "interfaces.h"

#define MILLIARCSECONDS_IN_A_DEGREE 3.6

static taf_ecall_State_t ECallState;
static taf_ecall_StateChangeHandlerRef_t HandlerRef;
static le_thread_Ref_t ECallCmdThreadRef;
static taf_ecall_CallRef_t ECallRef = NULL;

static taf_audio_StreamRef_t ModemRxAudioReference;
static taf_audio_StreamRef_t SpeakerAudioRef;
static taf_audio_ConnectorRef_t  AudioOutConnectorRef;

static taf_gpio_ChangeEventHandlerRef_t GpioHandlerRef;
static taf_pos_MovementHandlerRef_t  SamplePositionHandlerRef = NULL;
static int32_t latitude = INT32_MAX, longitude = INT32_MAX, hAccuracy = INT32_MAX;
static uint32_t direction = UINT32_MAX, dirAccuracy = UINT32_MAX;
static bool exitApp = true;

static void SamplePositionHandler
(
    taf_pos_SampleRef_t positionSampleRef,
    void* contextPtr
)
{
    le_result_t result;

    result = taf_pos_sample_Get2DLocation(positionSampleRef, &latitude, &longitude, &hAccuracy);
    if(result == LE_OK)
    {
        printf("Latitude(positive->north) : %.6f\n",(float)latitude/1e6);
        printf("Longitude(positive->east) : %.6f\n",(float)longitude/1e6);
        printf("hAccuracy                 : %.2fm\n",(float)hAccuracy);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("Location invalid [%d, %d, %d]\n", latitude, longitude, hAccuracy);
    }
    else
    {
        printf("Failed! to get 2D Location information\n");
    }

    //Get direction
    result = taf_pos_sample_GetDirection(positionSampleRef, &direction, &dirAccuracy);
    if(result == LE_OK)
    {
        printf("GetDirection: direction: %u, accuracy: %u\n", direction, dirAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed to get position sample direction information");
    }

}

static void* SamplePositionThread
(
    void* context
)
{
    //connect the position service to the current running thread
    taf_pos_ConnectService();

    //Sample Position Handler
    SamplePositionHandlerRef = taf_pos_AddMovementHandler(0, 0, SamplePositionHandler, NULL);
    if(SamplePositionHandlerRef != NULL) {
        LE_INFO("Confirm sample position handler was added successfully");
    }
    le_event_RunLoop();

    return NULL;
}

static void fetchLocationInfo
(
    void
)
{
    taf_posCtrl_ActivationRef_t activationRef;
    le_thread_Ref_t positionThreadRef;

    activationRef = taf_posCtrl_Request();

    //create a thread
    positionThreadRef = le_thread_Create("PosThreadTest", SamplePositionThread,NULL);
    LE_INFO("fetchLocationInfo positionThreadRef :%p", positionThreadRef);
    le_thread_Start(positionThreadRef);

    //Wait for 1 second to trigger SamplePositionHandler callback function
    LE_TEST_INFO("Wait for 1 second");
    le_thread_Sleep(1);

    //Remove the handler assigned
    taf_pos_RemoveMovementHandler(SamplePositionHandlerRef);

    //cancel the running thread
    le_thread_Cancel(positionThreadRef);

    //Stop receiving GNSS reports
    taf_gnss_Stop();

    //release the position control reference
    taf_posCtrl_Release(activationRef);
}

char* getCurrentTime() {
   time_t tm;
   time(&tm);
   return ctime(&tm);
}

static void OpenAudio() {
    le_result_t result;

    ModemRxAudioReference = taf_audio_OpenModemVoiceRx(1);

    LE_ERROR_IF((ModemRxAudioReference==NULL), " ModemRxAudioReference NULL");

    SpeakerAudioRef = taf_audio_OpenSpeaker();

    LE_ERROR_IF((SpeakerAudioRef == NULL), " open speaker reference is NULL");

    AudioOutConnectorRef = taf_audio_CreateConnector();

    LE_ERROR_IF((AudioOutConnectorRef == NULL), "Audio output connector ref is NULL");

    if (ModemRxAudioReference && SpeakerAudioRef  && AudioOutConnectorRef)
    {
        result = taf_audio_Connect(AudioOutConnectorRef, SpeakerAudioRef);

        LE_ERROR_IF((result != LE_OK), "Connect speaker to output connector failed ");

        result = taf_audio_Connect(AudioOutConnectorRef, ModemRxAudioReference);

        LE_ERROR_IF((result != LE_OK), "Connect modem Rx to output connector failed");
    }

}

static void CloseAudio() {
    if(AudioOutConnectorRef)
    {
        if(SpeakerAudioRef)
        {
            taf_audio_Disconnect(AudioOutConnectorRef, SpeakerAudioRef);
        }
        if(ModemRxAudioReference)
        {
            taf_audio_Disconnect(AudioOutConnectorRef, ModemRxAudioReference);
        }
    }

    if(AudioOutConnectorRef)
    {
        taf_audio_DeleteConnector(AudioOutConnectorRef);
        AudioOutConnectorRef = NULL;
    }

    if(AudioOutConnectorRef)
    {
        if(SpeakerAudioRef)
        {
            taf_audio_Disconnect(AudioOutConnectorRef, SpeakerAudioRef);
        }
    }

    if(SpeakerAudioRef)
    {
        taf_audio_Close(SpeakerAudioRef);
        SpeakerAudioRef = NULL;
    }

    if(ModemRxAudioReference)
    {
        taf_audio_Close(ModemRxAudioReference);
        ModemRxAudioReference = NULL;
    }
}

static void taf_ecall_TerminateRegistration_test()
{
    // Test Case
    le_result_t result = taf_ecall_TerminateRegistration();
    if (result == LE_OK)
    {
        LE_INFO("TerminateRegistration SUCCESS!!!\n");
    } else {
        LE_ERROR("TerminateRegistration FAILED. Error: %d\n", (int) result);
    }
}

static void* CommandInput(void* contextPtr)
{
    char input_str[5];

    taf_ecall_ConnectService();

    do {
        printf("The call is in progress... Press 'h' to hangup.\n");
        char *p = fgets(input_str,sizeof(input_str),stdin);

        if (p != NULL && input_str[0]=='h') {
            printf("User input: %c, so hanging up the call...\n", input_str[0]);
            le_result_t result = taf_ecall_End(ECallRef);
            LE_INFO("CommandInput: hanging up the call, result %d\n", (int) result);
        } else {
            printf("Invalid input just ignore it!\n");
        }
    } while(input_str[0]!='h');

    le_thread_Cancel(ECallCmdThreadRef);

    return NULL;
}

static void SignalHandler (int sigNum)
{
    LE_INFO("Exit eCallDemo app");
    if (ECallRef)
    {
        taf_ecall_End(ECallRef);
        taf_ecall_Delete(ECallRef);
        ECallRef = NULL;
    }

    if (GpioHandlerRef)
    {
        taf_gpio_RemoveChangeEventHandler(GpioHandlerRef);
    }

    taf_gnss_Stop();
    exit(EXIT_SUCCESS);
}

static void tafECallStateHandler( taf_ecall_CallRef_t eCallReference,
        taf_ecall_State_t state, void* cntxtPtr)
{

    LE_INFO("Ecall state change event, state = %d", state );
    LE_INFO("Ecall state change event, reference = %p", eCallReference );
    printf("=================\033[1;35mNOTIFICATION\033[0m=================\n");
    printf("Time: %s",  getCurrentTime());
    printf("Ecall state change event, state = %d\n", state );
    ECallState = state;
    exitApp = false;

    switch (state)
    {
        case TAF_ECALL_STATE_UNKNOWN:
        {
            printf("TAF_ECALL_STATE_UNKNOWN");
            break;
        }
        case TAF_ECALL_STATE_ALERTING:
        {
            printf("TAF_ECALL_STATE_ALERTING");
            OpenAudio();
            break;
        }
        case TAF_ECALL_STATE_ACTIVE:
        {
            printf("TAF_ECALL_STATE_ACTIVE");
            break;
        }
        case TAF_ECALL_STATE_IDLE:
        {
            printf("TAF_ECALL_STATE_IDLE");
            exitApp = true;
            break;
        }
        case TAF_ECALL_STATE_WAITING_PSAP_START_IND:
        {
            printf("TAF_ECALL_STATE_WAITING_PSAP_START_IND");
            break;
        }
        case TAF_ECALL_STATE_PSAP_START_RECEIVED:
        {
            printf("TAF_ECALL_STATE_PSAP_START_RECEIVED");
            break;
        }
        case TAF_ECALL_STATE_MSD_TRANSMISSION_STARTED:
        {
            printf("TAF_ECALL_STATE_MSD_TRANSMISSION_STARTED");
            break;
        }
        case TAF_ECALL_STATE_LLNACK_RECEIVED:
        {
            printf("TAF_ECALL_STATE_LLNACK_RECEIVED");
            break;
        }
        case TAF_ECALL_STATE_LL_ACK_RECEIVED:
        {
            printf("TAF_ECALL_STATE_LL_ACK_RECEIVED");
            break;
        }
        case TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS:
        {
            printf("TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS");
            break;
        }
        case TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED:
        {
            printf("TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED");
            break;
        }
        case TAF_ECALL_STATE_ALACK_RECEIVED_POSITIVE:
        {
            printf("TAF_ECALL_STATE_ALACK_RECEIVED_POSITIVE");
            break;
        }
        case TAF_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN:
        {
            printf("TAF_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN");
            break;
        }
        case TAF_ECALL_STATE_ENDED:
        {
            printf("TAF_ECALL_STATE_ENDED\n");
            if (eCallReference != NULL)
            {
                taf_ecall_TerminationReason_t lcf = taf_ecall_GetTerminationReason(eCallReference);
                LE_INFO("ECall ENDed, terminate reason  = %d", lcf );
                printf("Call Termination reason: %d", lcf);
            }
            taf_ecall_TerminateRegistration_test();
            CloseAudio();
            exitApp = true;
            break;
        }
        case TAF_ECALL_STATE_RESET:
        {
            printf("TAF_ECALL_STATE_RESET");
            break;
        }
        case TAF_ECALL_STATE_COMPLETED:
        {
            printf("TAF_ECALL_STATE_COMPLETED");
            break;
        }
        case TAF_ECALL_STATE_FAILED:
        {
            printf("TAF_ECALL_STATE_FAILED");
            exitApp = true;
            break;
        }
        case TAF_ECALL_STATE_END_OF_REDIAL_PERIOD:
        {
            printf("TAF_ECALL_STATE_END_OF_REDIAL_PERIOD");
            break;
        }
        case TAF_ECALL_STATE_T2_EXPIRED:
        {
            printf("TAF_ECALL_STATE_T2_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_TIMEOUT_T3:
        {
            printf("TAF_ECALL_STATE_TIMEOUT_T3");
            break;
        }
        case TAF_ECALL_STATE_T5_EXPIRED:
        {
            printf("TAF_ECALL_STATE_T5_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_T6_EXPIRED:
        {
            printf("TAF_ECALL_STATE_T6_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_T7_EXPIRED:
        {
            printf("TAF_ECALL_STATE_T7_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_T9_EXPIRED:
        {
            printf("TAF_ECALL_STATE_T9_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_T10_EXPIRED:
        {
            printf("TAF_ECALL_STATE_T10_EXPIRED");
            break;
        }
        case TAF_ECALL_STATE_DIALING:
        {
            printf("TAF_ECALL_STATE_DIALING");
            break;
        }
        case TAF_ECALL_STATE_NACK_OUT_OF_ORDER:
        {
            printf("TAF_ECALL_STATE_NACK_OUT_OF_ORDER");
            break;
        }
        case TAF_ECALL_STATE_ACK_OUT_OF_ORDER:
        {
            printf("TAF_ECALL_STATE_ACK_OUT_OF_ORDER");
            break;
        }
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_STARTED:
        {
            printf("TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_STARTED");
            break;
        }
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_SUCCESS:
        {
            printf("TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_SUCCESS");
            break;
        }
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_FAILURE:
        {
            printf("TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_FAILURE");
            break;
        }
        default:
        {
            printf("Unknown state");
            break;
        }
    }
    printf("\n==============================================\n");
    if (exitApp) {
        le_thread_Cancel(ECallCmdThreadRef);
        exit(EXIT_SUCCESS);
    } else {
        if (ECallState == TAF_ECALL_STATE_DIALING) {
            le_thread_Cancel(ECallCmdThreadRef);
            ECallCmdThreadRef = le_thread_Create("ECalltTh", CommandInput, NULL);
            le_thread_Start(ECallCmdThreadRef);
        } else {
            printf("The call is in progress... Press 'h' to hangup.\n");
        }
    }
}

static void PrintUsage ()
{
    puts("\n"
            "tafECallApp -- setOpMode <NORMAL/ECALL_ONLY> <SLOT1/SLOT2>\n"
            "tafECallApp -- getOpMode <SLOT1/SLOT2>\n"
            "tafECallApp -- start <AUTO/MANUAL/TEST>\n"
            "tafECallApp -- gpio <PIN>"
            "\n");
}

static int setOpMode()
{

    if (le_arg_NumArgs() < 4)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }
    const char* opMode =  le_arg_GetArg(2);
    uint8_t phoneId = 1;
    const char* inputPhoneId = le_arg_GetArg(3);
    le_result_t result = LE_FAULT;

    if (strcmp(inputPhoneId, "SLOT2") == 0)
    {
        phoneId = 2;
    }

    if (strcmp(opMode, "NORMAL") == 0)
    {
        result = taf_ecall_ExitOnlyMode(phoneId);
    }
    else if (strcmp(opMode, "ECALL_ONLY") == 0)
    {
        result = taf_ecall_ForcePersistentOnlyMode(phoneId);
    }
    else
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    if (result == LE_OK)
    {
        printf("SUCCESS\n");
        return EXIT_SUCCESS;
    }
    printf("FAILED\n");

    return EXIT_FAILURE;
}

static int getOpMode()
{
    if (le_arg_NumArgs() < 3)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    const char* inputPhoneId = le_arg_GetArg(2);

    uint8_t phoneId = 1;

    le_result_t result = LE_FAULT;

    if (strcmp(inputPhoneId,"SLOT2") == 0)
    {
        phoneId = 2;
    }

    taf_ecall_OpMode_t opMode = TAF_ECALL_MODE_NORMAL;
    result = taf_ecall_GetConfiguredOperationMode(phoneId, &opMode);

    if ( result == LE_OK)
    {
        printf("SUCCESS\n");
        printf("ECall operation mode = ");
        switch(opMode)
        {
            case TAF_ECALL_MODE_NORMAL:
                printf("TAF_ECALL_MODE_NORMAL\n");
                break;
            case TAF_ECALL_MODE_ECALL:
                printf("TAF_ECALL_MODE_ECALL\n");
                break;
            case TAF_ECALL_NONE:
                printf("TAF_ECALL_MODE_NONE\n");
                break;
            default:
                printf("Unknown mode\n");
        }
        return EXIT_SUCCESS;
    }

    printf("FAILED\n");
    return EXIT_FAILURE;
}

static void updateMsdInformation()
{
    taf_ecall_MsdVehicleType_t vehType = TAF_ECALL_PASSENGER_VEHICLE_CLASS_M1;
    taf_ecall_PropulsionStorageType_t propulsionStorage = TAF_ECALL_PROP_TYPE_GASOLINE_TANK;

    uint32_t msdVersion = 2;
    if (taf_ecall_SetMsdVersion(msdVersion) != LE_OK)
    {
        LE_ERROR("Unable to set MSD version");
    }

    if (taf_ecall_SetMsdTxMode(TAF_ECALL_MSD_TX_MODE_PUSH) != LE_OK)
    {
        LE_ERROR("Unable to set MSD transmission mode");
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

static void taf_ecall_GetNadDeregTime_test()
{
    // Test Case
    uint16_t deregTimeOrg = 0;
    le_result_t result = taf_ecall_GetNadDeregistrationTime(&deregTimeOrg);
    if (result == LE_OK)
    {
        LE_INFO("GetNadDeregTime SUCCESS!!! DeregTime (in minutes): %d\n", deregTimeOrg);
    } else {
        LE_ERROR("GetNadDeregTime FAILED. Error: %d\n", (int) result);
    }
}

static void taf_ecall_SetNadDeregTime_test()
{
    // Test Case
    le_result_t result = taf_ecall_SetNadDeregistrationTime(7*60); // 7 hrs
    if (result == LE_OK)
    {
        LE_INFO("SetNadDeregistrationTime as 7 hrs SUCCESS!!!\n");
    } else {
        LE_ERROR("SetNadDeregistrationTime FAILED. Error: %d\n", (int) result);
    }
}

static void updateLocationInformation(taf_ecall_CallRef_t eCallRef)
{
    bool isPosTrusted = false;

    printf("Fetching location information ...\n" );

    fetchLocationInfo();

    printf("Location fetched, dialing eCall now ...\n" );

    if ((hAccuracy < 100) && (dirAccuracy < 360))
    {
        isPosTrusted = true;
    }
    LE_INFO("updateLocationInformation latitude = %d ",latitude);
    LE_INFO("updateLocationInformation longitude = %d ",longitude);
    LE_INFO("updateLocationInformation hAccuracy = %d ",hAccuracy);
    LE_INFO("updateLocationInformation dirAccuracy = %d ",dirAccuracy);
    LE_INFO("updateLocationInformation isPosTrusted = %d ",isPosTrusted);

    latitude = (int32_t)(latitude * MILLIARCSECONDS_IN_A_DEGREE);
    longitude = (int32_t)(longitude * MILLIARCSECONDS_IN_A_DEGREE);

    //Not able to get proper location, use mock location to dial eCall
    if (latitude < -324000000 || latitude > 324000000)
    {
        latitude = 12985849;
        longitude = 77596432;
        hAccuracy = 12;
    }

    le_result_t result = taf_ecall_SetMsdPosition(eCallRef, isPosTrusted,
                                                         latitude,
                                                         longitude, direction/2 );
    if (result != LE_OK)
    {
        LE_ERROR("Unable to set location information");
    }

}

static int startECall()
{

    if (le_arg_NumArgs() < 3)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    const char* eCallType =  le_arg_GetArg(2);

    ECallRef = taf_ecall_Create();

    updateLocationInformation(ECallRef);

    updateMsdInformation();

    taf_ecall_SetMsdPassengersCount(ECallRef, 2);

    taf_ecall_GetNadDeregTime_test();
    taf_ecall_SetNadDeregTime_test();

    if (strcmp(eCallType, "AUTO") == 0)
    {
        taf_ecall_StartAutomatic(ECallRef);

    }
    else if (strcmp(eCallType, "MANUAL") == 0)
    {
        taf_ecall_StartManual(ECallRef);
    }
    else if (strcmp(eCallType, "TEST") == 0)
    {
        taf_ecall_StartTest(ECallRef);
    }
    else
    {
        PrintUsage();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static void StartAutoECall()
{
    if (ECallRef != NULL)
    {
        LE_ERROR("ECall in progress");
        return;
    }
    ECallRef = taf_ecall_Create();

    updateLocationInformation(ECallRef);

    updateMsdInformation();

    taf_ecall_SetMsdPassengersCount(ECallRef, 2);

    taf_ecall_StartAutomatic(ECallRef);

}

static void GpioChangeCallback(uint8_t pinNum, bool state, void *ctx)
{
    LE_INFO("State change %s pinNum %d", state?"TRUE":"FALSE", pinNum);
    StartAutoECall();
}

static int addGPIOHandler()
{
    if (le_arg_NumArgs() < 3) {
        PrintUsage();
        return EXIT_FAILURE;
    }
    const char* pinNum = le_arg_GetArg(2);
    uint32_t pin = atoi(pinNum);

    taf_gpio_SetInput(pin, TAF_GPIO_ACTIVE_HIGH, false);

    GpioHandlerRef = taf_gpio_AddChangeEventHandler(pin, TAF_GPIO_EDGE_RISING,
                                            false, GpioChangeCallback, NULL);

    return EXIT_SUCCESS;
}

COMPONENT_INIT
{
    int status = EXIT_SUCCESS;
    exitApp = true;

    LE_INFO("ECallTestApp COMPONENT_INIT...");
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);


    if (le_arg_NumArgs() < 2 )
    {
        PrintUsage();
        exit(EXIT_SUCCESS);
    }

    const char* command = le_arg_GetArg(1);
    if (command == NULL || strcmp(command, "help") == 0)
    {
        PrintUsage();
        exit(EXIT_SUCCESS);
    }
    if (strcmp(command, "setOpMode") == 0)
    {
        status = setOpMode();
    }
    if (strcmp(command, "getOpMode") == 0)
    {
        status = getOpMode();
    }
    HandlerRef = taf_ecall_AddStateChangeHandler(tafECallStateHandler, NULL);
    if (strcmp(command, "start") == 0)
    {
        status = startECall();
        exitApp = false;
    }

    if (strcmp(command, "gpio") == 0)
    {
        status = addGPIOHandler();
        exitApp = false;
    }

    if (exitApp) {
        exit(status);
    }
}
