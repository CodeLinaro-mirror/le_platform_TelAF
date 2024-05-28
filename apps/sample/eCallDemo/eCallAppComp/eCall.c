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
static taf_gnss_PositionHandlerRef_t PositionHandlerRef;
static int32_t latitude = INT32_MAX, longitude = INT32_MAX, hAccuracy = INT32_MAX;
static uint32_t direction = UINT32_MAX, dirAccuracy = UINT32_MAX;
static bool exitApp = true;
bool isMsgPrinted = false;
uint32_t msdVersion = 2;

static uint8_t msdRawData[39] = {2, 37, 28, 6, 128, 227, 10, 81, 67, 158, 41, 85, 212, 56, 0,
        128, 8, 55, 248, 12, 159, 215, 07, 240, 154, 148, 189, 211, 14, 85, 224, 128, 0, 0, 1,
        255, 255, 224, 64};

/*CEN ECall MSD EN 15722:2015
MSD PDU
02251C0680E30A51439E2955D43800800837F80C9FD707F09A94BDD30E55E080000001FFFFE040

Byte0:0x02
msd version:2

Byte1:0x25
msd content length:37

Byte2(bit7-bit2):0x1C
Extension of MSD message:0(Absent)
Optional Additional Data:0(Absent)
Extension of MSD structure:0(Absent)
recentVehickeLocationN1:1(Present)
recentVehickeLocationN2:1(Present)
number of Passengers:1(Present)

Byte2(bit1-bit0)/Byte3(bit7-bit2):0x1C 0x06
message ID:1

Byte3(bit1-bit0)/Byte4(bit7):0x06 0x80
Automatic Activation:1
Test Call:0
Position Can Be Trusted:1

Byte4(bit6-bit2):0x80
Vehicle Type:0(passengerVehicleClassM1)

Byte4(bit1-bit0)/Byte5-Byte17(bit7-bit4)
VIN:ECALLEXAMPLE02023

Byte17(bit3-bit0)-Byte19(bit7-bit5)
Gasoline Tank:1
Diese Tank:0
Compressed Natural Gas:0
Liquid Propane Gas:0
Electric Energy Storage:0
Hydrogen storage:0
Other storage:0

Byte19(bit4-bit0)-Byte23(bit7-bit5)
TimeStamp:1694414911

Byte23(bit4-bit0)-Byte27(bit7-bit5)
PositionLatitude:81044974

Byte27(bit4-bit0)-Byte31(bit7-bit5)
PositionLongitude:410169092

Byte31(bit4-bit0)-Byte32(bit7-bit5)
Vehicle Direction:0

Byte32(bit4-bit0)-Byte33(bit7-bit3)
recentVehickeLocationN1(Latitude):-512

Byte33(bit2-bit0)-Byte34(bit7-bit1)
recentVehickeLocationN1(Longitude):-512

Byte34(bit0)-Byte36(bit7)
recentVehickeLocationN2(Latitude):511

Byte36(bit6-bit0)-Byte37(bit7-bit5)
recentVehickeLocationN2(Longitude):511

Byte37(bit4-bit0)-Byte38(bit7-bit5)
Number of passenger:2
*/

static uint8_t msdLength = 39;

static void PositionHandlerFunction
(
    taf_gnss_SampleRef_t positionSampleRef,
    void* contextPtr
)
{
    le_result_t result;

    //Get 2D location
    result = taf_gnss_GetLocation(positionSampleRef,
                                  &latitude,
                                  &longitude,
                                  &hAccuracy);

    if (result == LE_OK)
    {
        printf("Latitude(positive->north) : %.6f\n"
               "Longitude(positive->east) : %.6f\n"
               "hAccuracy                 : %.2fm\n",
                (float)latitude/1e6,
                (float)longitude/1e6,
                (float)hAccuracy/1e2);
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
    result = taf_gnss_GetDirection(positionSampleRef,
                                   &direction,
                                   &dirAccuracy);

    if (result == LE_OK)
    {
        printf("Direction(0 degree is True North) : %.1f degrees\n"
               "Direction Accuracy                : %.1f degrees\n",
               (float)direction/10.0,
               (float)dirAccuracy/10.0);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("Direction invalid [%u, %u]\n", direction, dirAccuracy);
    }
    else
    {
        LE_TEST_INFO("Failed to get position direction information\n");
    }

    //Remove the handler assigned
    taf_gnss_RemovePositionHandler(PositionHandlerRef);

    //Stop receiving GNSS reports
    taf_gnss_Stop();
}

static void* SamplePositionThread
(
    void* context
)
{
    //connect the position service to the current running thread
    taf_gnss_ConnectService();

    le_result_t result = taf_gnss_Start();

    LE_INFO("Result of gnss start: %d", (int)result);

    //Position Handler
    PositionHandlerRef = taf_gnss_AddPositionHandler(PositionHandlerFunction, NULL);
    if(PositionHandlerRef != NULL) {
        LE_INFO("Confirm position handler was added successfully");
    }
    le_event_RunLoop();

    return NULL;
}

static void fetchLocationInfo
(
    void
)
{
    le_thread_Ref_t positionThreadRef;

    //create a thread
    positionThreadRef = le_thread_Create("PosThreadTest", SamplePositionThread,NULL);
    LE_INFO("fetchLocationInfo positionThreadRef :%p", positionThreadRef);
    le_thread_Start(positionThreadRef);

    //Wait for 2 seconds to init client session and trigger PositionHandlerFunction callback
    LE_TEST_INFO("Wait for 2 seconds");
    le_thread_Sleep(2);

    //cancel the running thread
    le_thread_Cancel(positionThreadRef);
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

static int terminateRegistration()
{
    // Test Case
    le_result_t result = taf_ecall_TerminateRegistration();
    if (result == LE_OK)
    {
        printf("TerminateRegistration SUCCESS!!!\n");
    } else {
        printf("TerminateRegistration FAILED. Error: %d\n", (int) result);
    }
    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void* CommandInput(void* contextPtr)
{
    char input_str[5];

    taf_ecall_ConnectService();

    do {
        if (!isMsgPrinted) {
            isMsgPrinted = true;
            printf("\n-------------------------------------------------\n");
            printf("\t\teCall Menu\t\t\n");
            printf("-------------------------------------------------\n");
            printf("\th - Hangup the eCall\n");
            printf("\tt - Terminate registration\n");
            printf("\ts - Import and send MSD\n");
            printf("\tu - Send MSD\n");
            printf("\tg - Get hlap timer state\n");
            printf("\tq - Quit test\n");
            printf("-------------------------------------------------\n");
        }
        printf("eCall> ");

        char *p = fgets(input_str,sizeof(input_str),stdin);

        if (p != NULL && input_str[0]=='h') {
            printf("User input: %c, so hanging up the call...\n", input_str[0]);
            le_result_t result =  ECallRef != NULL ? taf_ecall_End(ECallRef) : LE_FAULT;
            printf("Hangup %s\n", result == LE_OK ? "success." : "failed!!");
            LE_INFO("CommandInput: hanging up the call, result %d\n", (int) result);
        } else if (p != NULL && input_str[0]=='t') {
            printf("User input: %c, so terminate registration...\n", input_str[0]);
            int res = terminateRegistration();
            LE_INFO("CommandInput: terminate registration, res: %d\n", res);
        } else if (p != NULL && input_str[0]=='s') {
            printf("User input: %c, so import MSD eg:02251C0680E30A51439E2955D43800800837F80C9FD707F09A94BDD30E55E080000001FFFFE040\n", input_str[0]);
            char msd[2*TAF_ECALL_MAX_MSD_LENGTH+1];
            char *msdData = fgets(msd,sizeof(msd),stdin);
            if (msdData != NULL)
            {
                uint8_t msdPdu[TAF_ECALL_MAX_MSD_LENGTH];
                int byte = 0;
                int res = 0;
                for (int k =0; k < TAF_ECALL_MAX_MSD_LENGTH; k++)
                {
                    msdPdu[k] = 0;
                }

                for (int i = 0, j = 0; i < strlen(msd)/2; i++)
                {
                    for (; j < (2*i+2); j++)
                    {
                        if (msd[j] >= '0' && msd[j] <= '9'){
                            byte = msd[j] - '0';
                        } else if (msd[j] >= 'a' && msd[j] <= 'f'){
                            byte = msd[j] - 'a' + 10;
                        } else if (msd[j] >= 'A' && msd[j] <= 'F'){
                            byte = msd[j] - 'A' + 10;
                        } else {
                            printf("User input wrong\n");
                            res = -1;
                            break;
                        }
                        msdPdu[i] = msdPdu[i] * 16 + byte;
                     }
                }

                if (res == 0)
                {
                    res = taf_ecall_ImportMsd(ECallRef, msdPdu, strlen(msd)/2);
                    LE_INFO("CommandInput: import MSD, res: %d\n", res);
                    res = taf_ecall_SendMsd(ECallRef);
                    LE_INFO("CommandInput: send MSD, res: %d\n", res);
                }
            }
        } else if (p != NULL && input_str[0]=='u') {
            uint8_t msdRawDataExport[TAF_ECALL_MAX_MSD_LENGTH];
            size_t msdLengthExport = TAF_ECALL_MAX_MSD_LENGTH;
            le_result_t result = LE_FAULT;
            result = taf_ecall_ExportMsd(ECallRef, msdRawDataExport, &msdLengthExport);
            if (LE_NOT_FOUND == result)
            {
                printf("User input: %c, so send MSD by SetMsdxxx...\n", input_str[0]);
                int res = taf_ecall_SendMsd(ECallRef);
                LE_INFO("CommandInput: send MSD, res: %d\n", res);
            } else if (LE_OK == result) {
                printf("User input: %c, so send MSD by ImportMsd...\n", input_str[0]);
            }
        } else if (p != NULL && input_str[0]=='g') {
            printf("User input: %c, so enter the hlap timer type eg: 2...\n", input_str[0]);
            p = fgets(input_str,sizeof(input_str),stdin);
            taf_ecall_HlapTimerType_t timerType;
            taf_ecall_HlapTimerStatus_t timerStatus;
            uint16_t elapsedTime;
            if (p != NULL && input_str[0]=='2')
            {
               timerType = TAF_ECALL_TIMER_TYPE_T2;
            } else if (p != NULL && input_str[0]=='9') {
               timerType = TAF_ECALL_TIMER_TYPE_T9;
            } else if (p != NULL && input_str[0]=='1' && input_str[1]=='0'){
               timerType = TAF_ECALL_TIMER_TYPE_T10;
            } else {
               timerType = TAF_ECALL_TIMER_TYPE_UNKNOWN;
            }
            le_result_t result = taf_ecall_GetHlapTimerState(timerType, &timerStatus, &elapsedTime);
            printf("Get hlap timer state %s\n", result == LE_OK ? "success." : "failed!!");
            printf("Hlap timer status is %d and the elapsed time is %d\n", timerStatus, elapsedTime);
        }else if (p != NULL && input_str[0]=='q') {
            exitApp = true;
            le_thread_Cancel(ECallCmdThreadRef);
            ECallCmdThreadRef = NULL;
        } else {
            isMsgPrinted = false;
        }
    } while(input_str[0]!='q');

    exit(EXIT_SUCCESS);
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
    printf("\n=================\033[1;35mNOTIFICATION\033[0m=================\n");
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
        case TAF_ECALL_STATE_MSD_UPDATE_REQ:
        {
            printf("TAF_ECALL_STATE_MSD_UPDATE_REQ");
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
            CloseAudio();
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
            exitApp = false;
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
        case TAF_ECALL_STATE_T2_STARTED:
        {
            printf("TAF_ECALL_STATE_T2_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T5_STARTED:
        {
            printf("TAF_ECALL_STATE_T5_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T6_STARTED:
        {
            printf("TAF_ECALL_STATE_T6_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T7_STARTED:
        {
            printf("TAF_ECALL_STATE_T7_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T9_STARTED:
        {
            printf("TAF_ECALL_STATE_T9_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T10_STARTED:
        {
            printf("TAF_ECALL_STATE_T10_STARTED");
            break;
        }
        case TAF_ECALL_STATE_T2_STOPPED:
        {
            printf("TAF_ECALL_STATE_T2_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T5_STOPPED:
        {
            printf("TAF_ECALL_STATE_T5_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T6_STOPPED:
        {
            printf("TAF_ECALL_STATE_T6_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T7_STOPPED:
        {
            printf("TAF_ECALL_STATE_T7_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T9_STOPPED:
        {
            printf("TAF_ECALL_STATE_T9_STOPPED");
            break;
        }
        case TAF_ECALL_STATE_T10_STOPPED:
        {
            printf("TAF_ECALL_STATE_T10_STOPPED");
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
        ECallCmdThreadRef = NULL;
        exit(EXIT_SUCCESS);
    } else {
        le_thread_Cancel(ECallCmdThreadRef);
        ECallCmdThreadRef = le_thread_Create("ECalltTh", CommandInput, NULL);
        le_thread_Start(ECallCmdThreadRef);
    }
}

static void PrintUsage ()
{
    puts("\n"
            "tafECallApp -- setOpMode <NORMAL/ECALL_ONLY> <SLOT1/SLOT2>\n"
            "tafECallApp -- getOpMode <SLOT1/SLOT2>\n"
            "tafECallApp -- setPsapNumber <NUMBER>\n"
            "tafECallApp -- getPsapNumber\n"
            "tafECallApp -- importMsd <MSD bytes in decimal e.g. 2 41 68 6 128 227 10 ...>\n"
            "\t\t[Note: If no MSD input, then default MSD shall be used]\n"
            "tafECallApp -- exportMsd\n"
            "tafECallApp -- useUSimNumbers\n"
            "tafECallApp -- setNadDeregTime <time in minutes>\n"
            "tafECallApp -- getNadDeregTime\n"
            "tafECallApp -- setMsdVersion <2/3>\n"
            "tafECallApp -- getMsdVersion\n"
            "tafECallApp -- setNadClearDownFallbackTime <time in minutes>\n"
            "tafECallApp -- getNadClearDownFallbackTime\n"
            "tafECallApp -- setNadMinNetworkRegistrationTime <time in minutes>\n"
            "tafECallApp -- getNadMinNetworkRegistrationTime\n"
            "tafECallApp -- start <AUTO/MANUAL/TEST>\n"
            "tafECallApp -- start <PRIVATE> <NUMBER> [contentType] [acceptInfo]\n"
            "tafECallApp -- end\n"
            "tafECallApp -- terminateReg\n"
            "tafECallApp -- gpio <PIN>\n"
            "tafECallApp -- getHlapTimerState <hlap timer type>\n"
            "\n");
}

static int setPsapNumber()
{
    if (le_arg_NumArgs() < 3)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    const char* psapNum =  le_arg_GetArg(2);
    if (psapNum == NULL)
    {
        printf("Input psap number is not vaild!\n");
        printf("Failed!! try again...\n");
        return EXIT_FAILURE;
    }

    le_result_t result = taf_ecall_SetPsapNumber(psapNum);
    LE_TEST_OK(result == LE_OK, "setPsapNumber - LE_OK");
    printf("Update PSAP number as %s is %s\n", psapNum, result == LE_OK ? "Success." : "Failed!!");

    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int getPsapNumber()
{
    char psapNumber[TAF_SIM_PHONE_NUM_MAX_LEN];
    le_result_t result = taf_ecall_GetPsapNumber(psapNumber, TAF_SIM_PHONE_NUM_MAX_LEN);
    LE_TEST_OK(result == LE_OK || strcmp("None", psapNumber) == 0, "getPsapNumber - LE_OK");
    printf("Result: %s\n", result == LE_OK ? "Success." : "Failed!!");
    if (result == LE_OK) {
        printf("The PSAP Number is %s\n", psapNumber);
    }
    LE_TEST_INFO("getPsapNumber done");

    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int exportMsd()
{
    // Test Case
    uint8_t msdRawDataExport[TAF_ECALL_MAX_MSD_LENGTH];
    size_t msdLengthExport = TAF_ECALL_MAX_MSD_LENGTH;

    memset(msdRawDataExport, 0, TAF_ECALL_MAX_MSD_LENGTH);

    ECallRef = taf_ecall_Create();
    le_result_t result = taf_ecall_ExportMsd(ECallRef, msdRawDataExport, &msdLengthExport);
    LE_TEST_OK(result == LE_OK, "ExportMsd - LE_OK");
    if (result == LE_NOT_FOUND) {
        printf("MSD not found! May not set it.\n");
    } else {
        printf("Result of ExportMsd is %s\n", result == LE_OK ? "Success." : "Failed!!");
    }

    if(result == LE_OK) {
        printf("Retrieved MSD[%d] is: ", (int)msdLengthExport);
        for (int i = 0; i < msdLengthExport; i++) {
            printf("%d ", msdRawDataExport[i]);
        }
        printf("\n");
    }

    LE_TEST_INFO("ExportMsd done.");

    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int importMsd()
{
    int count = le_arg_NumArgs();
    le_result_t result = LE_FAULT;

    LE_INFO("ImportMsd NumArgs = %d", count);

    if (count < 3)
    {
        printf("No MSD input! Default MSD will be set.\n");
        ECallRef = taf_ecall_Create();
        result = taf_ecall_ImportMsd(ECallRef, msdRawData, msdLength);
        printf("Result of importMsd is %s\n", result == LE_OK ? "Success." : "Failed!!");
        return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    uint8_t msdPdu[TAF_ECALL_MAX_MSD_LENGTH];
    for (int k = 0; k < TAF_ECALL_MAX_MSD_LENGTH; k++)
    {
        msdPdu[k] = 0;
    }

    if ((count-2) > TAF_ECALL_MAX_MSD_LENGTH)
    {
        printf("OVERFLOW: Input beyond 255 bytes of MSD shall be ignore.\n");
        printf("Failed!! try again...\n");;
        return EXIT_FAILURE;
    }

    for (int i = 0; i < (count-2); i++) {
        const char* bytePtr = le_arg_GetArg(i+2);

        if (bytePtr == NULL)
        {
            printf("Input at position %d is NULL!\n", i+2);
            printf("Failed!! try again...\n");
            return EXIT_FAILURE;
        }

        int byte = atoi(bytePtr);

        if (byte < 0 || byte > 255) {
            printf("Wrong input as %d (Range 0 to 255).\n", byte);
            printf("Failed!! try again...\n");
            return EXIT_FAILURE;
        }

        msdPdu[i] = (uint8_t) byte;
    }

    ECallRef = taf_ecall_Create();
    result = taf_ecall_ImportMsd(ECallRef, msdPdu, count-2);
    LE_TEST_OK(result == LE_OK, "importMsd - LE_OK");
    printf("Result of importMsd is %s\n", result == LE_OK ? "Success." : "Failed!!");

    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int useUSimNumbers()
{
    le_result_t result = taf_ecall_UseUSimNumbers();
    LE_TEST_OK(result == LE_OK, "useUSimNumbers - LE_OK");
    printf("Result: %s\n", result == LE_OK ? "Success." : "Failed!!");
    LE_TEST_INFO("useUSimNumbers done");

    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int getNadDeregTime()
{
    // Test Case
    uint16_t deregTimeOrg = 0;
    le_result_t result = taf_ecall_GetNadDeregistrationTime(&deregTimeOrg);
    LE_TEST_OK(result == LE_OK, "getNadDeregTime - LE_OK");
    printf("Result: %s\n", result == LE_OK ? "Success." : "Failed!!");
    if (result == LE_OK) {
        printf("NAD de-registration time: %d min.\n", deregTimeOrg);
    }

    LE_TEST_INFO("getNadDeregTime done");

    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int setNadDeregTime()
{
    if (le_arg_NumArgs() < 3)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    uint16_t deregTime = atoi(le_arg_GetArg(2));
    le_result_t result = taf_ecall_SetNadDeregistrationTime(deregTime);
    LE_TEST_OK(result == LE_OK, "setNadDeregTime - LE_OK");
    printf("Set de-reg time as %d is %s\n", deregTime, result == LE_OK ? "Success." : "Failed!");

    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int getNadClearDownFallbackTime()
{
    // Test Case
    uint16_t ccftTime = 0;
    le_result_t result = taf_ecall_GetNadClearDownFallbackTime(&ccftTime);
    LE_TEST_OK(result == LE_OK, "getNadClearDownFallbackTime - LE_OK");
    printf("Result: %s\n", result == LE_OK ? "Success." : "Failed!!");
    if (result == LE_OK) {
        printf("NAD clear down fallback time: %d min.\n", ccftTime);
    }

    LE_TEST_INFO("getNadClearDownFallbackTime done");

    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int setNadClearDownFallbackTime()
{
    if (le_arg_NumArgs() < 3)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    uint16_t ccftTime = atoi(le_arg_GetArg(2));
    le_result_t result = taf_ecall_SetNadClearDownFallbackTime(ccftTime);
    LE_TEST_OK(result == LE_OK, "setNadClearDownFallbackTime - LE_OK");
    printf("Set clear down fallback time as %d is %s\n", ccftTime, result == LE_OK ? "Success." : "Failed!");

    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int getNadMinNetworkRegistrationTime()
{
    // Test Case
    uint16_t minNwRegTime = 0;
    le_result_t result = taf_ecall_GetNadMinNetworkRegistrationTime(&minNwRegTime);
    LE_TEST_OK(result == LE_OK, "getNadMinNetworkRegistrationTime - LE_OK");
    printf("Result: %s\n", result == LE_OK ? "Success." : "Failed!!");
    if (result == LE_OK) {
        printf("NAD min network registration time: %d min.\n", minNwRegTime);
    }

    LE_TEST_INFO("getNadMinNetworkRegistrationTime done");

    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int setNadMinNetworkRegistrationTime()
{
    if (le_arg_NumArgs() < 3)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    uint16_t minNwRegTime = atoi(le_arg_GetArg(2));
    le_result_t result = taf_ecall_SetNadMinNetworkRegistrationTime(minNwRegTime);
    LE_TEST_OK(result == LE_OK, "setNadMinNetworkRegistrationTime - LE_OK");
    printf("Set min network registration time as %d is %s\n", minNwRegTime, result == LE_OK ? "Success." : "Failed!");

    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
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

static int getMsdVersion()
{
    le_result_t result = taf_ecall_GetMsdVersion(&msdVersion);
    LE_TEST_OK(result == LE_OK, "getMsdVersion - LE_OK");
    printf("Result: %s\n", result == LE_OK ? "Success." : "Failed!!");
    if (result == LE_OK) {
        printf("Get msd version: %d\n", msdVersion);
    }

    LE_TEST_INFO("getMsdVersion done");

    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int setMsdVersion()
{
    if (le_arg_NumArgs() < 3)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    uint32_t msdVersion = atoi(le_arg_GetArg(2));
    le_result_t result = taf_ecall_SetMsdVersion(msdVersion);
    LE_TEST_OK(result == LE_OK, "setMsdVersion - LE_OK");
    printf("Set msd version as %d %s\n", msdVersion, result == LE_OK ? "Success." : "Failed!");

    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void updateMsdInformation()
{
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

static void updateLocationInformation(taf_ecall_CallRef_t eCallRef)
{
    bool isPosTrusted = false;

    printf("Fetching location information ...\n" );

    fetchLocationInfo();

    printf("Location fetched, dialing eCall now ...\n" );

    if ((hAccuracy/1e2 < 100) && (dirAccuracy/10 < 360))
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
                                                         longitude, direction/20);
    if (result != LE_OK)
    {
        LE_ERROR("Unable to set location information");
    }
 
    result = taf_ecall_SetMsdPositionN1(eCallRef, -512, -512);
    if (result != LE_OK)
    {
       LE_ERROR("Unable to set the position delta N-1 for MSD transmission.");
    }

    result = taf_ecall_SetMsdPositionN2(eCallRef, 511, 511);
    if (result != LE_OK)
    {
        LE_ERROR("Unable to set the position delta N-2 for MSD transmission.");
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

    if (strcmp(eCallType, "AUTO") == 0)
    {
        taf_ecall_SetMsdEuroNCAPLocationOfImpact(ECallRef, TAF_ECALL_LOI_FRONT);
        taf_ecall_SetMsdEuroNCAPIIDeltaV(ECallRef, 125, -45, 10);
        taf_ecall_StartAutomatic(ECallRef);
    }
    else if (strcmp(eCallType, "MANUAL") == 0)
    {
        taf_ecall_ResetMsdAdditionalData(ECallRef);
        taf_ecall_StartManual(ECallRef);
    }
    else if (strcmp(eCallType, "TEST") == 0)
    {
        taf_ecall_StartTest(ECallRef);
    }
    else if (strcmp(eCallType, "PRIVATE") == 0)
    {
        const char* psapNumber = "";
        const char* contentType = "application/EmergencyCallData.eCall.MSD";
        const char* acceptInfo = "";
        if (le_arg_NumArgs() >= 4)
        {
            psapNumber = le_arg_GetArg(3);
            if (psapNumber == NULL)
            {
                printf("Input psap number is not vaild!\n");
                printf("Failed!! try again...\n");
                return EXIT_FAILURE;
            }

            if (le_arg_NumArgs() >= 5)
            {
                contentType = le_arg_GetArg(4);
                if (contentType == NULL)
                {
                    printf("Input content type is not vaild!\n");
                    printf("Failed!! try again...\n");
                    return EXIT_FAILURE;
                }
            }
            if (le_arg_NumArgs() == 6)
            {
                acceptInfo = le_arg_GetArg(5);
                if (acceptInfo == NULL)
                {
                    printf("Input accept info is not vaild!\n");
                    printf("Failed!! try again...\n");
                    return EXIT_FAILURE;
                }
            }
        } else {
            PrintUsage();
            return EXIT_FAILURE;
        }
        taf_ecall_StartPrivate(ECallRef, psapNumber, contentType, acceptInfo);
    }
    else
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    taf_ecall_Type_t type = taf_ecall_GetType(ECallRef);
    printf("eCall type = %d!\n", (int)type);

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

static int getHlapTimerState()
{
    if (le_arg_NumArgs() < 3)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    taf_ecall_HlapTimerType_t hlapTimerType = atoi(le_arg_GetArg(2));
    taf_ecall_HlapTimerStatus_t timerStatus;
    uint16_t elapsedTime;
    le_result_t result = taf_ecall_GetHlapTimerState(hlapTimerType, &timerStatus, &elapsedTime);
    LE_TEST_OK(result == LE_OK, "getHlapTimerState - LE_OK");
    printf("Get eCall hlap timer status as: %d, elapsedTime as: %d\n", timerStatus, elapsedTime);

    return result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

COMPONENT_INIT
{
    int status = EXIT_SUCCESS;
    le_result_t result = LE_FAULT;
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
    HandlerRef = taf_ecall_AddStateChangeHandler(tafECallStateHandler, NULL);
    if (command == NULL || strcmp(command, "help") == 0)
    {
        PrintUsage();
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "setOpMode") == 0)
    {
        status = setOpMode();
    }
    else if (strcmp(command, "getOpMode") == 0)
    {
        status = getOpMode();
    }
    else if (strcmp(command, "setPsapNumber") == 0)
    {
        status = setPsapNumber();
    }
    else if (strcmp(command, "getPsapNumber") == 0)
    {
        status = getPsapNumber();
    }
    else if (strcmp(command, "importMsd") == 0)
    {
        status = importMsd();
    }
    else if (strcmp(command, "exportMsd") == 0)
    {
        status = exportMsd();
    }
    else if (strcmp(command, "useUSimNumbers") == 0)
    {
        status = useUSimNumbers();
    }
    else if (strcmp(command, "setNadDeregTime") == 0)
    {
        status = setNadDeregTime();
    }
    else if (strcmp(command, "getNadDeregTime") == 0)
    {
        status = getNadDeregTime();
    }
    else if (strcmp(command, "setMsdVersion") == 0)
    {
        status = setMsdVersion();
    }
    else if (strcmp(command, "getMsdVersion") == 0)
    {
        status = getMsdVersion();
    }
    else if (strcmp(command, "setNadClearDownFallbackTime") == 0)
    {
        status = setNadClearDownFallbackTime();
    }
    else if (strcmp(command, "getNadClearDownFallbackTime") == 0)
    {
        status = getNadClearDownFallbackTime();
    }
    else if (strcmp(command, "setNadMinNetworkRegistrationTime") == 0)
    {
        status = setNadMinNetworkRegistrationTime();
    }
    else if (strcmp(command, "getNadMinNetworkRegistrationTime") == 0)
    {
        status = getNadMinNetworkRegistrationTime();
    }
    else if (strcmp(command, "start") == 0)
    {
        status = startECall();
        exitApp = status == EXIT_SUCCESS ? false : true;
    }
    else if (strcmp(command, "end") == 0)
    {
        ECallRef = taf_ecall_Create();
        if (ECallRef) {
            result = taf_ecall_End(ECallRef);
        }
        printf("Hangup %s\n", result == LE_OK ? "success." : "failed!!" );
        isMsgPrinted = true;
        status = result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
        exitApp = result == LE_OK ? false : true;
    }
    else if (strcmp(command, "terminateReg") == 0)
    {
        status = terminateRegistration();
        isMsgPrinted = true;
        exitApp = status == EXIT_SUCCESS ? false : true;
    }
    else if (strcmp(command, "gpio") == 0)
    {
        status = addGPIOHandler();
        exitApp = false;
    }
    else if (strcmp(command, "getHlapTimerState") == 0)
    {
        status = getHlapTimerState();
    } else {
        PrintUsage();
    }

    if (exitApp) {
        le_thread_Cancel(ECallCmdThreadRef);
        ECallCmdThreadRef = NULL;
        exit(status);
    } else {
        le_thread_Cancel(ECallCmdThreadRef);
        ECallCmdThreadRef = le_thread_Create("ECalltTh", CommandInput, NULL);
        le_thread_Start(ECallCmdThreadRef);
    }
}
