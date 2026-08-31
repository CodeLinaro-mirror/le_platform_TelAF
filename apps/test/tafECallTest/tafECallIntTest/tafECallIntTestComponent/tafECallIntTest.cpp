/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <string>

extern "C" {
#include <sys/time.h>
}

#include "legato.h"
#include "interfaces.h"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define BUFSIZE 48
#define MILLIARCSECONDS_IN_A_DEGREE 3.6

const std::string GREEN = "\033[0;32m";
const std::string RED = "\033[0;31m";
const std::string YELLOW = "\033[0;33m";
const std::string DONE = "\033[0m";  // No color

using namespace std;

static le_sem_Ref_t TestSemRef;
static le_thread_Ref_t threadRef = NULL;
static taf_ecall_State_t ECallState;
static taf_ecall_StateChangeHandlerRef_t HandlerRef;
static taf_ecall_CallRef_t ECallRef = NULL;
static uint8_t PhoneId;
static int TC_No = 1;
static bool exitApp = true;
static uint8_t msdRawData[48] = {2, 46, 92, 6, 128, 227, 10, 81, 67, 158, 41, 85, 212, 56, 0,
        128, 8, 39, 248, 13, 31, 51, 146, 80, 154, 147, 217, 179, 14, 85, 131, 96, 0, 0, 1,
        255, 255, 224, 64, 65, 0, 32, 161, 6, 90, 80, 144, 0};
static size_t msdLength = sizeof(msdRawData);

static taf_locGnss_PositionHandlerRef_t PositionHandlerRef;
static int32_t latitude = INT32_MAX, longitude = INT32_MAX, hAccuracy = INT32_MAX;
static uint32_t direction = UINT32_MAX, dirAccuracy = UINT32_MAX;

string getCurrentTime() {
   timeval tod;
   gettimeofday(&tod, NULL);
   time_t tt = tod.tv_sec;
   char buffer[BUFSIZE];
   strftime(buffer, BUFSIZE, "%Y-%m-%d %H:%M:%S", localtime(&tt));
   std::string currTime;
   int ms = (int) tod.tv_usec / 1000;
   if (ms < 10) {
       currTime = currTime + buffer + "." + "00" + std::to_string(ms);
   } else if (ms < 100) {
       currTime = currTime + buffer + "." + "0" + std::to_string(ms);
   } else {
       currTime = currTime + buffer + "." + std::to_string(ms);
   }
   return currTime;
}

const char* return_val(le_result_t result)
{
    const char* ret_val = "LE_UNKNOWN";
    switch (result)
    {
        case 0:
            ret_val = "LE_OK";
            break;
        case -1:
            ret_val = "LE_NOT_FOUND";
            break;
        case -2:
            ret_val = "LE_NOT_POSSIBLE";
            break;
        case -3:
            ret_val = "LE_OUT_OF_RANGE";
            break;
        case -4:
            ret_val = "LE_NO_MEMORY";
            break;
        case -5:
            ret_val = "LE_NOT_PERMITTED";
            break;
        case -6:
            ret_val = "LE_FAULT";
            break;
        case -7:
            ret_val = "LE_COMM_ERROR";
            break;
        case -8:
            ret_val = "LE_TIMEOUT";
            break;
        case -9:
            ret_val = "LE_OVERFLOW";
            break;
        case -10:
            ret_val = "LE_UNDERFLOW";
            break;
        case -11:
            ret_val = "LE_WOULD_BLOCK";
            break;
        case -12:
            ret_val = "LE_DEADLOCK";
            break;
        case -13:
            ret_val = "LE_FORMAT_ERROR";
            break;
        case -14:
            ret_val = "LE_DUPLICATE";
            break;
        case -15:
            ret_val = "LE_BAD_PARAMETER";
            break;
        case -16:
            ret_val = "LE_CLOSED";
            break;
        case -17:
            ret_val = "LE_BUSY";
            break;
        case -18:
            ret_val = "LE_UNSUPPORTED";
            break;
        case -19:
            ret_val = "LE_IO_ERROR";
            break;
        case -20:
            ret_val = "LE_NOT_IMPLEMENTED";
            break;
        case -21:
            ret_val = "LE_UNAVAILABLE";
            break;
        case -22:
            ret_val = "LE_TERMINATED";
            break;
        case -23:
            ret_val = "LE_IN_PROGRESS";
            break;
        case -24:
            ret_val = "LE_SUSPENDED";
            break;
    }
    return ret_val;
}

void printResultMsg()
{
    std::cout <<endl;
    std::cout <<"*******************RESULTS**********************" << endl;
    std::cout <<"ECall Integration Tests are executed successfully." << endl;
    std::cout <<"Please check test results above and logs." << endl;
    std::cout <<"The pre-conditions and usages are mentioned at begining." << endl;
    std::cout <<"These help to learn test environment and identify false-alarm." << endl;
    std::cout <<"******************THANK YOU*********************" << endl;
    std::cout <<endl;
}

void printPreCondition()
{
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Pre-Conditions:" << endl;
    std::cout <<"Make sure device/Call-box/Agilent setup/NAD are in proper state." << endl;
    std::cout <<"Device should have valid IMEI, serial number programmed." << endl;
    std::cout <<"A SIM card (with valid IMSI, SDN/FDN numbers) should be inserted in device." << endl;
    std::cout <<"GPS/GNSS should be working fine to fetch location info for the eCall." << endl;
    std::cout <<"************************************************" << endl;

}

void printUsages()
{
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Usages:" << endl;
    std::cout <<"First, install this tafECallIntTest app (if not already installed)." << endl;
    std::cout <<"Go to device path: /legato/systems/current/appsWriteable/tafECallIntTest/bin/" << endl;
    std::cout <<"Run: ./tafECallIntTest <SLOT1/SLOT2> <AUTO/MANUAL/TEST> <True/False> <PSAP Number>" << endl;
    std::cout <<"************************************************" << endl;
}

void report(le_result_t expected_result, le_result_t actual_result, string API_Name)
{
    if (LE_UNSUPPORTED == actual_result)
    {
        std::cout<<TC_No<<". "<<API_Name<<" - "<<"Result: "<<return_val(actual_result)
                <<YELLOW + " - Not Supported" + DONE<<endl;
    }
    else if (expected_result == actual_result)
    {
        std::cout<<TC_No<<". "<<API_Name<<" - "<<return_val(expected_result)<<GREEN + " - Pass" + DONE<<endl;
    }
    else
    {
        std::cout<<TC_No<<". "<<API_Name<<" - "<<"Expected: "<< return_val(expected_result)
                <<RED + " - Fail" + DONE + " (Actual: "<<return_val(actual_result) <<")"<<endl;
    }
    TC_No += 1;
}

static void PositionHandlerFunction
(
    taf_locGnss_SampleRef_t positionSampleRef,
    void* contextPtr
)
{
    le_result_t result;

    //Get 2D location
    result = taf_locGnss_GetLocation(positionSampleRef,
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
    result = taf_locGnss_GetDirection(positionSampleRef,
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
    taf_locGnss_RemovePositionHandler(PositionHandlerRef);

    //Stop receiving GNSS reports
    taf_locGnss_Stop();
}

static void* SamplePositionThread
(
    void* context
)
{
    //connect the position service to the current running thread
    taf_locGnss_ConnectService();

    le_result_t result = taf_locGnss_Start();

    LE_INFO("Result of gnss start: %d", (int)result);

    //Position Handler
    PositionHandlerRef = taf_locGnss_AddPositionHandler(PositionHandlerFunction, NULL);
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

    //Wait for 2 seconds to init gnss client session and trigger PositionHandlerFunction callback
    LE_TEST_INFO("Wait for 2 seconds");
    le_thread_Sleep(2);

    //cancel the running thread
    le_thread_Cancel(positionThreadRef);
}

static void* taf_ecall_endCall_test
(
    taf_ecall_CallRef_t    ecallRef
)
{
    // Test Case
    le_result_t result = taf_ecall_End(ecallRef);
    LE_TEST_OK(result == LE_OK, "taf_ecall_endCall_test - LE_OK");
    report(LE_OK,result,"taf_ecall_endCall_test");
    LE_TEST_INFO("taf_ecall_endCall_test done");
    return NULL;
}

static void* taf_ecall_getState_test
(
    taf_ecall_CallRef_t    ecallRef
)
{
    // Test Case
    taf_ecall_State_t retrievedState = taf_ecall_GetState(ecallRef);
    std::cout<<"*** eCall current state: "<<retrievedState<<endl;
    LE_TEST_INFO("taf_ecall_getState_test done");

    return NULL;
}

static void* taf_ecall_SendMsd_test
(
    taf_ecall_CallRef_t    ecallRef
)
{
    // Test Case
    le_result_t result = taf_ecall_SendMsd(ecallRef);
    LE_TEST_OK(result == LE_OK, "taf_ecall_SendMsd_test - LE_OK");
    report(LE_OK,result,"taf_ecall_SendMsd_test");
    LE_TEST_INFO("taf_ecall_SendMsd_test done");
    return NULL;
}

static void* taf_ecall_ImportMsd_test
(
    taf_ecall_CallRef_t    ecallRef
)
{
    // Test Case
    le_result_t result = taf_ecall_ImportMsd(ecallRef, msdRawData, msdLength);
    LE_TEST_OK(result == LE_OK, "taf_ecall_ImportMsd_test - LE_OK");
    report(LE_OK,result,"taf_ecall_ImportMsd_test");
    LE_TEST_INFO("taf_ecall_ImportMsd_test done");
    return NULL;
}

static void* taf_ecall_ExportMsd_test
(
    taf_ecall_CallRef_t    ecallRef
)
{
    // Test Case
    le_result_t result = taf_ecall_ExportMsd(ecallRef, msdRawData, &msdLength);
    LE_TEST_OK(result == LE_OK, "taf_ecall_ExportMsd_test - LE_OK");
    report(LE_OK,result,"taf_ecall_ExportMsd_test");
    LE_TEST_INFO("taf_ecall_ExportMsd_test done");
    return NULL;
}

static void* taf_ecall_getTerminationReason_test
(
    taf_ecall_CallRef_t    ecallRef
)
{
    // Test Case
    taf_ecall_TerminationReason_t callEndReason = taf_ecall_GetTerminationReason(ecallRef);
    bool isLcfValid = callEndReason >= TAF_ECALL_REASON_UNOBTAINABLE_NUMBER &&
            callEndReason <= TAF_ECALL_REASON_ERROR_UNSPECIFIED;
    LE_TEST_OK(isLcfValid, "taf_ecall_getTerminationReason_test - LE_OK");
    report(LE_OK, isLcfValid ? LE_OK : LE_FAULT,"taf_ecall_getTerminationReason_test");
    std::cout<<"*** eCall Termination Reason: "<<callEndReason<<endl;
    LE_TEST_INFO("taf_ecall_getTerminationReason_test done");

    return NULL;
}

static void* taf_ecall_RemoveStateChangeHandler_test()
{
    taf_ecall_RemoveStateChangeHandler(HandlerRef);
    LE_TEST_OK(true, "taf_ecall_RemoveStateChangeHandler_test - LE_OK");
    report(LE_OK, LE_OK,"taf_ecall_RemoveStateChangeHandler_test");
    LE_TEST_INFO("taf_ecall_RemoveStateChangeHandler_test done");
    return NULL;
}

static void SignalHandler (int sigNum)
{
    LE_INFO("Exit eCallIntegration test app");
    if (ECallRef)
    {
        taf_ecall_endCall_test(ECallRef);
        taf_ecall_Delete(ECallRef);
        ECallRef = NULL;
    }
    exit(EXIT_SUCCESS);
}

static void tafECallStateHandler( taf_ecall_CallRef_t eCallReference,
        taf_ecall_State_t state, void* cntxtPtr)
{
    LE_DEBUG("Ecall state change event state = %d", state );
    LE_DEBUG("Ecall state change event reference = %p", eCallReference );
    std::cout <<endl;
    PRINT_NOTIFICATION << getCurrentTime() <<" Received ECall state change event. New state is: " << state << endl;
    ECallState = state;

    switch (state)
    {
        case TAF_ECALL_STATE_UNKNOWN:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_UNKNOWN"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_ALERTING:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_ALERTING"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_ACTIVE:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_ACTIVE"<<endl;
            std::cout <<endl;
            PRINT_NOTIFICATION << getCurrentTime() <<" Wait! The active call shall be hangup after 10 sec... "<<endl;
            sleep(10);
            PRINT_NOTIFICATION << getCurrentTime() <<" Sending hangup... "<<endl;
            std::cout <<endl;
            taf_ecall_endCall_test(eCallReference);
            break;
        }
        case TAF_ECALL_STATE_IDLE:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_IDLE"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_WAITING_PSAP_START_IND:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_WAITING_PSAP_START_IND"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_PSAP_START_RECEIVED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_PSAP_START_RECEIVED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_MSD_TRANSMISSION_STARTED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_MSD_TRANSMISSION_STARTED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_LLNACK_RECEIVED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_LLNACK_RECEIVED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_LL_ACK_RECEIVED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_LL_ACK_RECEIVED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_MSD_TRANSMISSION_SUCCESS"<<endl;
            std::cout <<endl;
            taf_ecall_ExportMsd_test(eCallReference);
            break;
        }
        case TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_MSD_TRANSMISSION_FAILED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_ALACK_RECEIVED_POSITIVE:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_ALACK_RECEIVED_POSITIVE"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_ENDED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_ENDED "<<endl;
            std::cout << endl;

            //Reset Nad Deregistration Time
            le_result_t result = taf_ecall_SetNadDeregistrationTime(6*60); // 6 hrs
            LE_TEST_OK(result == LE_OK, "taf_ecall_SetNadDeregTime_test - LE_OK");
            report(LE_OK,result,"taf_ecall_SetNadDeregTime_test");

            taf_ecall_getTerminationReason_test(eCallReference);
            exitApp = true;
            break;
        }
        case TAF_ECALL_STATE_RESET:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_RESET"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_COMPLETED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_COMPLETED"<<endl;
            std::cout <<endl;
            exitApp = true;
            break;
        }
        case TAF_ECALL_STATE_FAILED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_FAILED"<<endl;
            std::cout <<endl;
            exitApp = true;
            break;
        }
        case TAF_ECALL_STATE_END_OF_REDIAL_PERIOD:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_END_OF_REDIAL_PERIOD"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T2_EXPIRED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T2_EXPIRED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_TIMEOUT_T3:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_TIMEOUT_T3"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T5_EXPIRED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T5_EXPIRED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T6_EXPIRED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T6_EXPIRED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T7_EXPIRED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T7_EXPIRED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T9_EXPIRED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T9_EXPIRED"<<endl;
            break;
        }
        case TAF_ECALL_STATE_T10_EXPIRED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T10_EXPIRED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_DIALING:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_DIALING"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_NACK_OUT_OF_ORDER:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_NACK_OUT_OF_ORDER"<<endl;
            break;
        }
        case TAF_ECALL_STATE_ACK_OUT_OF_ORDER:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_ACK_OUT_OF_ORDER"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_STARTED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_STARTED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_SUCCESS:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_SUCCESS"<<endl;
            std::cout <<endl;
            taf_ecall_ExportMsd_test(eCallReference);
            break;
        }
        case TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_FAILURE:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_OUTBAND_MSD_TRANSMISSION_FAILURE"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_MSD_UPDATE_REQ:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_MSD_UPDATE_REQ"<<endl;
            std::cout <<endl;
            taf_ecall_SendMsd_test(eCallReference);
            break;
        }
        case TAF_ECALL_STATE_T2_STARTED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T2_STARTED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T5_STARTED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T5_STARTED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T6_STARTED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T6_STARTED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T7_STARTED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T7_STARTED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T9_STARTED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T9_STARTED"<<endl;
            std::cout <<endl;
            taf_ecall_getState_test(eCallReference);
            break;
        }
        case TAF_ECALL_STATE_T10_STARTED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T10_STARTED"<<endl;
            std::cout <<endl;
            taf_ecall_getState_test(eCallReference);
            break;
        }
        case TAF_ECALL_STATE_T2_STOPPED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T2_STOPPED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T5_STOPPED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T5_STOPPED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T6_STOPPED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T6_STOPPED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T7_STOPPED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T7_STOPPED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T9_STOPPED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T9_STOPPED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T10_STOPPED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T10_STOPPED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_INCOMING:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_INCOMING"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_LL_NACK_DUE_TO_T7_EXPIRY:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_LL_NACK_DUE_TO_T7_EXPIRY"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T9_RESUMED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T9_RESUMED"<<endl;
            std::cout <<endl;
            break;
        }
        case TAF_ECALL_STATE_T10_RESUMED:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" TAF_ECALL_STATE_T10_RESUMED"<<endl;
            std::cout <<endl;
            break;
        }
        default:
        {
            PRINT_NOTIFICATION << getCurrentTime() <<" Unknown state"<<endl;
            std::cout <<endl;
            break;
        }
    }
    if (exitApp) {
        le_sem_Post(TestSemRef);
    }
}

static uint8_t get_phone_id(const char* phoneId) {
    if (strcmp(phoneId, "SLOT2") == 0)
    {
        return 2;
    }
    else
    {
        return 1;
    }
}

void taf_ecall_setOpMode_test()
{
    // Test Case
    le_result_t result;
    taf_ecall_OpMode_t opMode = TAF_ECALL_NONE;
    result = taf_ecall_GetConfiguredOperationMode(PhoneId, &opMode);
    LE_TEST_OK(result == LE_OK, "taf_ecall_getOpMode_test - LE_OK");
    report(LE_OK,result,"taf_ecall_getOpMode_test");
    LE_TEST_INFO("taf_ecall_getOpMode_test done");
    std::cout<<"*** opMode (0-NORMAL, 1-ECALL ONLY, 2-NONE): "<<opMode<<endl;

    // Test Case
    result=taf_ecall_ForcePersistentOnlyMode(PhoneId);
    LE_TEST_OK(result == LE_OK, "taf_ecall_ForcePersistentOnlyMode - LE_OK");
    report(LE_OK,result,"taf_ecall_ForcePersistentOnlyMode");

    // Test Case
    opMode = TAF_ECALL_MODE_NORMAL;
    result = taf_ecall_GetConfiguredOperationMode(PhoneId, &opMode);
    LE_TEST_OK(result == LE_OK, "taf_ecall_getOpMode_test - LE_OK");
    report(LE_OK,result,"taf_ecall_getOpMode_test");
    std::cout<<"*** opMode (0-NORMAL, 1-ECALL ONLY, 2-NONE): "<<opMode<<endl;

    // Test Case
    LE_TEST_OK(opMode == TAF_ECALL_MODE_ECALL, "Checking if opMode set properly");
    if(opMode == TAF_ECALL_MODE_ECALL)
    {
        std::cout<<TC_No<<". Checking if opMode set as ecall only properly " + GREEN + "- Pass" + DONE<<endl;
    }
    else
    {
        std::cout<<TC_No<<". Checking if opMode set as ecall only properly " + RED + "- Fail" + DONE<<endl;
    }
    TC_No += 1;

    //Test set opMode normal
    result = taf_ecall_GetConfiguredOperationMode(PhoneId, &opMode);
    LE_TEST_OK(result == LE_OK, "taf_ecall_getOpMode_test - LE_OK");
    report(LE_OK,result,"taf_ecall_getOpMode_test");
    LE_TEST_INFO("taf_ecall_getOpMode_test done");
    std::cout<<"*** opMode (0-NORMAL, 1-ECALL ONLY, 2-NONE): "<<opMode<<endl;

    // Test Case
    result=taf_ecall_ExitOnlyMode(PhoneId);
    LE_TEST_OK(result == LE_OK, "taf_ecall_ExitOnlyMode - LE_OK");
    report(LE_OK,result,"taf_ecall_ExitOnlyMode");

    // Test Case
    opMode = TAF_ECALL_MODE_ECALL;
    result = taf_ecall_GetConfiguredOperationMode(PhoneId, &opMode);
    LE_TEST_OK(result == LE_OK, "taf_ecall_getOpMode_test - LE_OK");
    report(LE_OK,result,"taf_ecall_getOpMode_test");
    std::cout<<"*** opMode (0-NORMAL, 1-ECALL ONLY, 2-NONE): "<<opMode<<endl;

    // Test Case
    LE_TEST_OK(opMode == TAF_ECALL_MODE_NORMAL, "Checking if opMode set properly");
    if(opMode == TAF_ECALL_MODE_NORMAL)
    {
        std::cout<<TC_No<<". Checking if opMode set as normal properly " + GREEN + "- Pass" + DONE<<endl;
    }
    else
    {
        std::cout<<TC_No<<". Checking if opMode set as normal properly " + RED + "- Fail" + DONE<<endl;
    }
    TC_No += 1;
}

static void* taf_ecall_getOpMode_test()
{
    le_result_t result;

    taf_ecall_OpMode_t opMode = TAF_ECALL_MODE_NORMAL;
    result = taf_ecall_GetConfiguredOperationMode(PhoneId, &opMode);
    LE_TEST_OK(result == LE_OK, "taf_ecall_getOpMode_test - LE_OK");
    report(LE_OK,result,"taf_ecall_getOpMode_test");
    LE_TEST_INFO("taf_ecall_getOpMode_test done");

    return NULL;
}

static void* taf_ecall_getMsdVersion_test()
{
    uint32_t msdVersion = 0;
    le_result_t result = taf_ecall_GetMsdVersion(&msdVersion);
    LE_TEST_OK(result == LE_OK, "taf_ecall_getMsdVersion_test - LE_OK");
    report(LE_OK,result,"taf_ecall_getMsdVersion_test");
    std::cout<<"*** msdVersion: "<<msdVersion<<endl;
    LE_TEST_INFO("taf_ecall_getMsdVersion_test done");
    return NULL;
}

static void* taf_ecall_setMsdVersion_test()
{
    uint32_t msdVersion = 0;
    le_result_t result = taf_ecall_GetMsdVersion(&msdVersion);
    result = taf_ecall_SetMsdVersion(msdVersion+1);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setMsdVersion_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setMsdVersion_test");

    // Test Case
    uint32_t getMsdVersion = 0;
    result = taf_ecall_GetMsdVersion(&getMsdVersion);
    LE_TEST_OK(result == LE_OK, "taf_ecall_GetMsdTxMode - LE_OK");
    report(LE_OK,result,"taf_ecall_setMsdVersion_test");
    std::cout<<"*** msdVersion: "<<getMsdVersion<<endl;

    // Test Case
    LE_TEST_OK(getMsdVersion == msdVersion+1, "Checking if msdVersion set properly");
    if(getMsdVersion == msdVersion+1)
    {
        std::cout<<TC_No<<". Checking if msdVersion set properly " + GREEN + "- Pass" + DONE<<endl;
    }
    else
    {
        std::cout<<TC_No<<". Checking if msdVersion set properly " + RED + "- Fail" + DONE<<endl;
    }
    //Put back the original version
    result = taf_ecall_SetMsdVersion(msdVersion);
    TC_No += 1;

    return NULL;
}

static void* taf_ecall_getVehicleType_test()
{
    taf_ecall_MsdVehicleType_t vehicleTypePtr;
    le_result_t result = taf_ecall_GetVehicleType(&vehicleTypePtr);
    LE_TEST_OK(result == LE_OK, "taf_ecall_getVehicleType_test - LE_OK");
    report(LE_OK,result,"taf_ecall_getVehicleType_test");
    std::cout<<"*** vehicleType: "<<vehicleTypePtr<<endl;
    LE_TEST_INFO("taf_ecall_getVehicleType_test done");
    return NULL;
}

static void* taf_ecall_setVehicleType_test()
{
    taf_ecall_MsdVehicleType_t vehicleTypePtr;
    le_result_t result = taf_ecall_GetVehicleType(&vehicleTypePtr);
    result = taf_ecall_SetVehicleType(TAF_ECALL_HEAVY_DUTY_VEHICLES_CLASS_N2);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setVehicleType_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setVehicleType_test");

    // Test Case
    taf_ecall_MsdVehicleType_t getVehicleTypePtr;
    result = taf_ecall_GetVehicleType(&getVehicleTypePtr);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setVehicleType_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setVehicleType_test");
    std::cout<<"*** vehicleType: "<<getVehicleTypePtr<<endl;

    // Test Case
    result = taf_ecall_SetVehicleType(TAF_ECALL_MOTOR_CYCLES_CLASS_L1E);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setVehicleType_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setVehicleType_test");

    result = taf_ecall_GetVehicleType(&getVehicleTypePtr);
    LE_TEST_OK(result == LE_OK && getVehicleTypePtr == TAF_ECALL_MOTOR_CYCLES_CLASS_L1E, "taf_ecall_setVehicleType_test - LE_OK");
    report(LE_OK,getVehicleTypePtr == TAF_ECALL_MOTOR_CYCLES_CLASS_L1E ? LE_OK:LE_FAULT,"taf_ecall_setVehicleType_test");
    std::cout<<"*** vehicleType: "<<getVehicleTypePtr<<endl;

    // Test Case
    result = taf_ecall_SetVehicleType(TAF_ECALL_MOTOR_CYCLES_CLASS_L7E);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setVehicleType_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setVehicleType_test");

    result = taf_ecall_GetVehicleType(&getVehicleTypePtr);
    LE_TEST_OK(result == LE_OK && getVehicleTypePtr == TAF_ECALL_MOTOR_CYCLES_CLASS_L7E, "taf_ecall_setVehicleType_test - LE_OK");
    report(LE_OK,getVehicleTypePtr == TAF_ECALL_MOTOR_CYCLES_CLASS_L7E ? LE_OK:LE_FAULT,"taf_ecall_setVehicleType_test");
    std::cout<<"*** vehicleType: "<<getVehicleTypePtr<<endl;

    // Test Case
    result = taf_ecall_SetVehicleType(TAF_ECALL_BUSES_AND_COACHES_CLASS_M3);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setVehicleType_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setVehicleType_test");

    result = taf_ecall_GetVehicleType(&getVehicleTypePtr);
    LE_TEST_OK(result == LE_OK && getVehicleTypePtr == TAF_ECALL_BUSES_AND_COACHES_CLASS_M3, "taf_ecall_setVehicleType_test - LE_OK");
    report(LE_OK,getVehicleTypePtr == TAF_ECALL_BUSES_AND_COACHES_CLASS_M3? LE_OK:LE_FAULT,"taf_ecall_setVehicleType_test");
    std::cout<<"*** vehicleType: "<<getVehicleTypePtr<<endl;

    // Test Case
    result = taf_ecall_SetVehicleType(TAF_ECALL_LIGHT_COMMERCIAL_VEHICLES_CLASS_N1);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setVehicleType_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setVehicleType_test");

    result = taf_ecall_GetVehicleType(&getVehicleTypePtr);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setVehicleType_test - LE_OK");
    report(LE_OK,getVehicleTypePtr == TAF_ECALL_LIGHT_COMMERCIAL_VEHICLES_CLASS_N1? LE_OK:LE_FAULT,"taf_ecall_setVehicleType_test");
    std::cout<<"*** vehicleType: "<<getVehicleTypePtr<<endl;

    // Test Case
    LE_TEST_OK(getVehicleTypePtr == TAF_ECALL_LIGHT_COMMERCIAL_VEHICLES_CLASS_N1, "Checking if msdVersion set properly");
    if(getVehicleTypePtr == TAF_ECALL_LIGHT_COMMERCIAL_VEHICLES_CLASS_N1)
    {
        std::cout<<TC_No<<". Checking if vehicleType set properly " + GREEN + "- Pass" + DONE<<endl;
    }
    else
    {
        std::cout<<TC_No<<". Checking if vehicleType set properly " + RED + "- Fail" + DONE<<endl;
    }
    //Set back the original one
    result = taf_ecall_SetVehicleType(vehicleTypePtr);
    TC_No += 1;

    return NULL;
}

static void* taf_ecall_getVIN_test()
{
    char existingVin[TAF_ECALL_MAX_VIN_LENGTH];
    le_result_t result = taf_ecall_GetVIN(existingVin, TAF_ECALL_MAX_VIN_BYTES);
    LE_TEST_OK(result == LE_OK, "taf_ecall_GetVIN - LE_OK");
    report(LE_OK,result,"taf_ecall_GetVIN");
    std::cout<<"*** Vehicle Identification Number: "<<existingVin<<endl;
    LE_TEST_INFO("taf_ecall_GetVIN done");
    return NULL;
}

static void* taf_ecall_setVIN_test()
{
    char existingVin[TAF_ECALL_MAX_VIN_BYTES];
    le_result_t result = taf_ecall_GetVIN(existingVin, TAF_ECALL_MAX_VIN_BYTES);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setVIN_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setVIN_test");
    std::cout<<"*** Vehicle Identification Number: "<<existingVin<<endl;

    // Test Case
    const char* vin1 = "1M8GDM9AXKP042788"; //Must be TAF_ECALL_MAX_VIN_LENGTH(17) chars
    result = taf_ecall_SetVIN(vin1);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setVIN_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setVIN_test");

    char newVin[TAF_ECALL_MAX_VIN_BYTES];
    result = taf_ecall_GetVIN(newVin, TAF_ECALL_MAX_VIN_BYTES);
    LE_TEST_OK(result == LE_OK && strcmp(newVin, vin1) == 0, "taf_ecall_setVIN_test - LE_OK");
    report(LE_OK,strcmp(newVin, vin1) == 0 ? LE_OK:LE_FAULT,"taf_ecall_setVIN_test");
    std::cout<<"*** Vehicle Identification Number: "<<newVin<<endl;

    // Test Case
    const char* vin = "1HGCM82633A004352"; //Must be TAF_ECALL_MAX_VIN_LENGTH(17) chars
    result = taf_ecall_SetVIN(vin);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setVIN_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setVIN_test");

    result = taf_ecall_GetVIN(newVin, TAF_ECALL_MAX_VIN_BYTES);
    LE_TEST_OK(result == LE_OK && strcmp(newVin, vin) == 0, "taf_ecall_setVIN_test - LE_OK");
    report(LE_OK,strcmp(newVin, vin) == 0 ? LE_OK:LE_FAULT,"taf_ecall_setVIN_test");
    std::cout<<"*** Vehicle Identification Number: "<<newVin<<endl;

    // Test Case
    LE_TEST_OK(strcmp(newVin, vin) == 0, "Checking if VIN set properly");
    if(strcmp(newVin, vin) == 0)
    {
        std::cout<<TC_No<<". Checking if VIN set properly " + GREEN + "- Pass" + DONE<<endl;
    }
    else
    {
        std::cout<<TC_No<<". Checking if VIN set properly " + RED + "- Fail" + DONE<<endl;
    }
    //Set back the original one
    result = taf_ecall_SetVIN(existingVin);
    TC_No += 1;

    return NULL;
}

static void* taf_ecall_getPropulsionType_test()
{
    taf_ecall_PropulsionStorageType_t propulsionStorageType;
    le_result_t result = taf_ecall_GetPropulsionType(&propulsionStorageType);
    LE_TEST_OK(result == LE_OK, "taf_ecall_getPropulsionType_test - LE_OK");
    report(LE_OK,result,"taf_ecall_getPropulsionType_test");
    std::cout<<"*** PropulsionStorageType: "<<propulsionStorageType<<endl;
    LE_TEST_INFO("taf_ecall_getPropulsionType_test done");
    return NULL;
}

static void* taf_ecall_setPropulsionType_test()
{
    taf_ecall_PropulsionStorageType_t propulsionStorageType;
    le_result_t result = taf_ecall_GetPropulsionType(&propulsionStorageType);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setPropulsionType_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setPropulsionType_test");

    // Test Case
    result = taf_ecall_SetPropulsionType(TAF_ECALL_PROP_TYPE_ELECTRIC);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setPropulsionType_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setPropulsionType_test");

    taf_ecall_PropulsionStorageType_t getStorageType;
    result = taf_ecall_GetPropulsionType(&getStorageType);
    LE_TEST_OK(result == LE_OK && getStorageType == TAF_ECALL_PROP_TYPE_ELECTRIC, "taf_ecall_setPropulsionType_test - LE_OK");
    report(LE_OK,getStorageType == TAF_ECALL_PROP_TYPE_ELECTRIC ? LE_OK:LE_FAULT,"taf_ecall_setPropulsionType_test");
    std::cout<<"*** PropulsionStorageType: "<<getStorageType<<endl;

    // Test Case
    result = taf_ecall_SetPropulsionType(TAF_ECALL_PROP_TYPE_DIESEL_TANK);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setPropulsionType_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setPropulsionType_test");

    result = taf_ecall_GetPropulsionType(&getStorageType);
    LE_TEST_OK(result == LE_OK && getStorageType == TAF_ECALL_PROP_TYPE_DIESEL_TANK, "taf_ecall_setPropulsionType_test - LE_OK");
    report(LE_OK,getStorageType == TAF_ECALL_PROP_TYPE_DIESEL_TANK ? LE_OK:LE_FAULT,"taf_ecall_setPropulsionType_test");
    std::cout<<"*** PropulsionStorageType: "<<getStorageType<<endl;

    // Test Case
    result = taf_ecall_SetPropulsionType(TAF_ECALL_PROP_TYPE_COMPRESSED_NATURALGAS);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setPropulsionType_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setPropulsionType_test");

    result = taf_ecall_GetPropulsionType(&getStorageType);
    LE_TEST_OK(result == LE_OK && getStorageType == TAF_ECALL_PROP_TYPE_COMPRESSED_NATURALGAS, "taf_ecall_setPropulsionType_test - LE_OK");
    report(LE_OK,getStorageType == TAF_ECALL_PROP_TYPE_COMPRESSED_NATURALGAS ? LE_OK:LE_FAULT,"taf_ecall_setPropulsionType_test");
    std::cout<<"*** PropulsionStorageType: "<<getStorageType<<endl;

    // Test Case
    result = taf_ecall_SetPropulsionType(TAF_ECALL_PROP_TYPE_HYDROGEN);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setPropulsionType_test - LE_OK");
    report(LE_OK,result,"taf_ecall_setPropulsionType_test");

    result = taf_ecall_GetPropulsionType(&getStorageType);
    LE_TEST_OK(result == LE_OK && getStorageType == TAF_ECALL_PROP_TYPE_HYDROGEN, "taf_ecall_setPropulsionType_test - LE_OK");
    report(LE_OK,getStorageType == TAF_ECALL_PROP_TYPE_HYDROGEN ? LE_OK:LE_FAULT,"taf_ecall_setPropulsionType_test");
    std::cout<<"*** PropulsionStorageType: "<<getStorageType<<endl;

    // Test Case
    LE_TEST_OK(getStorageType == TAF_ECALL_PROP_TYPE_HYDROGEN, "Checking if propulsion storage set properly");
    if(getStorageType == TAF_ECALL_PROP_TYPE_HYDROGEN)
    {
        std::cout<<TC_No<<". Checking if propulsion storage set properly " + GREEN + "- Pass" + DONE<<endl;
    }
    else
    {
        std::cout<<TC_No<<". Checking if propulsion storage set properly " + RED + "- Fail" + DONE<<endl;
    }
    //Set back the original one
    result = taf_ecall_SetPropulsionType(propulsionStorageType);
    TC_No += 1;

    return NULL;
}

static void* taf_ecall_getMsdTxMode_test()
{
    le_result_t result;

    // Test Case
    int initialTxMode = 4;
    taf_ecall_MsdTransmissionMode_t getTxMode = (taf_ecall_MsdTransmissionMode_t) initialTxMode;
    result = taf_ecall_GetMsdTxMode(&getTxMode);
    LE_TEST_OK(result == LE_UNSUPPORTED, "taf_ecall_getMsdTxMode_test - LE_UNSUPPORTED");
    report(LE_UNSUPPORTED,result,"taf_ecall_getMsdTxMode_test");
    LE_TEST_INFO("taf_ecall_getMsdTxMode_test done");

    return NULL;
}

void taf_ecall_setMsdTxMode_test()
{
    // Test Case
    le_result_t result;
    int initialTxMode = 3;
    taf_ecall_MsdTransmissionMode_t txMode = (taf_ecall_MsdTransmissionMode_t) initialTxMode;
    result = taf_ecall_GetMsdTxMode(&txMode);
    LE_TEST_OK(result == LE_UNSUPPORTED, "taf_ecall_GetMsdTxMode - LE_UNSUPPORTED");
    report(LE_UNSUPPORTED,result,"taf_ecall_GetMsdTxMode");
    LE_TEST_INFO("taf_ecall_GetMsdTxMode done");

    // Test Case
    txMode = TAF_ECALL_MSD_TX_MODE_PUSH;
    result=taf_ecall_SetMsdTxMode(txMode);
    LE_TEST_OK(result == LE_UNSUPPORTED, "taf_ecall_SetMsdTxMode - LE_UNSUPPORTED");
    report(LE_UNSUPPORTED,result,"taf_ecall_SetMsdTxMode");
    LE_TEST_INFO("taf_ecall_SetMsdTxMode done");

    std::cout<<TC_No<<".txMode set/get properly " + GREEN + "- Pass" + DONE<<endl;
    TC_No += 1;
}

static void* taf_ecall_setMsdPosition_tests()
{
    le_result_t result;

    bool isPosTrusted = false;

    printf("Fetching location information ...\n" );
    fetchLocationInfo();
    printf("Location fetched, updating MSD position now ...\n" );

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

    // Test Case
    result = taf_ecall_SetMsdPosition(ECallRef, isPosTrusted, latitude, longitude, direction/20);

    LE_TEST_OK(result == LE_OK, "taf_ecall_SetMsdPosition - LE_OK");
    report(LE_OK,result,"taf_ecall_SetMsdPosition");
    LE_TEST_INFO("taf_ecall_SetMsdPosition done");

    result = taf_ecall_SetMsdPositionN1(ECallRef, 511, 511);
    LE_TEST_OK(result == LE_OK, "taf_ecall_SetMsdPositionN1 - LE_OK");
    report(LE_OK,result,"taf_ecall_SetMsdPositionN1");
    LE_TEST_INFO("taf_ecall_SetMsdPositionN1 done");

    result = taf_ecall_SetMsdPositionN2(ECallRef, -512, -512);
    LE_TEST_OK(result == LE_OK, "taf_ecall_SetMsdPositionN2 - LE_OK");
    report(LE_OK,result,"taf_ecall_SetMsdPositionN2");
    LE_TEST_INFO("taf_ecall_SetMsdPositionN2 done");

    return NULL;
}

static void* taf_ecall_setMsdPassengersCount_tests()
{
    le_result_t result;

    // Test Case
    result = taf_ecall_SetMsdPassengersCount(ECallRef, 3);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setMsdPassengersCount_tests - LE_OK");
    report(LE_OK,result,"taf_ecall_setMsdPassengersCount_tests");
    LE_TEST_INFO("taf_ecall_setMsdPassengersCount_tests done");

    // Test Case
    result = taf_ecall_SetMsdPassengersCount(ECallRef, 2);
    LE_TEST_OK(result == LE_OK, "taf_ecall_setMsdPassengersCount_tests - LE_OK");
    report(LE_OK,result,"taf_ecall_setMsdPassengersCount_tests");
    LE_TEST_INFO("taf_ecall_setMsdPassengersCount_tests done");

    return NULL;
}

static void updateMsdInformation()
{
    taf_ecall_MsdVehicleType_t vehType = TAF_ECALL_PASSENGER_VEHICLE_CLASS_M1;
    taf_ecall_PropulsionStorageType_t propulsionStorage = TAF_ECALL_PROP_TYPE_GASOLINE_TANK;

    uint32_t msdVersion = 2;
    if (taf_ecall_SetMsdVersion(msdVersion) != LE_OK)
    {
        LE_ERROR("Unable to set MSD version");
    } else {
        std::cout<<"Updating MSD info ..."<<endl;
    }

    if (taf_ecall_SetMsdTxMode(TAF_ECALL_MSD_TX_MODE_PUSH) != LE_OK)
    {
        LE_ERROR("Unable to set MSD transmission mode");
    } else {
        std::cout<<"Updating MSD info ......"<<endl;
    }

    if (taf_ecall_SetVIN("ECALLEXAMPLE02022") != LE_OK)
    {
        LE_ERROR("Unable to set vehicle identification number");
    } else {
        std::cout<<"Updating MSD info ........"<<endl;
    }

    if (taf_ecall_SetVehicleType(vehType) != LE_OK)
    {
        LE_ERROR("Unable to set vehicle type");
    } else {
        std::cout<<"Updating MSD info ..........."<<endl;
    }

    if (taf_ecall_SetPropulsionType(propulsionStorage) != LE_OK)
    {
        LE_ERROR("Unable to set propulsion type");
    } else {
        std::cout<<"Updating MSD info .............."<<endl;
    }
    std::cout<<endl;
}

static void* taf_ecall_GetPsapNumber_test()
{
    // Test Case
    char psapNum[TAF_SIM_PHONE_NUM_MAX_LEN] = "Nil";
    le_result_t result = taf_ecall_GetPsapNumber(psapNum, TAF_SIM_PHONE_NUM_MAX_LEN);
    LE_TEST_OK(result == LE_OK || strcmp("None", psapNum) == 0, "taf_ecall_GetPsapNumber - LE_OK");
    report(LE_OK,result,"taf_ecall_GetPsapNumber");
    std::cout<<"*** psapNumber: "<<psapNum<<endl;
    LE_TEST_INFO("taf_ecall_GetPsapNumber done");

    return NULL;
}

static void* taf_ecall_UseUSimNumbers_test()
{
    // Test Case
    le_result_t result = taf_ecall_UseUSimNumbers();
    LE_TEST_OK(result == LE_OK, "taf_ecall_UseUSimNumbers_test - LE_OK");
    report(LE_OK,result,"taf_ecall_UseUSimNumbers_test");
    LE_TEST_INFO("taf_ecall_UseUSimNumbers_test done");

    return NULL;
}

static void* taf_ecall_SetPsapNumber_test()
{
    char psapNum[TAF_SIM_PHONE_NUM_MAX_LEN] = "None";
    le_result_t result = taf_ecall_GetPsapNumber(psapNum, TAF_SIM_PHONE_NUM_MAX_LEN);
    result = taf_ecall_SetPsapNumber("911");
    LE_TEST_OK(result == LE_OK, "taf_ecall_SetPsapNumber_test - LE_OK");
    report(LE_OK,result,"taf_ecall_SetPsapNumber_test");

    // Test Case
    char psapNumber[TAF_SIM_PHONE_NUM_MAX_LEN];
    result = taf_ecall_GetPsapNumber(psapNumber, TAF_SIM_PHONE_NUM_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_ecall_GetPsapNumber - LE_OK");
    report(LE_OK,result,"taf_ecall_GetPsapNumber");
    std::cout<<"*** psapNumber: "<<psapNumber<<endl;

    // Test Case
    LE_TEST_OK(strcmp("911", psapNumber) == 0, "Checking if psapNumber set properly");
    if(strcmp("911", psapNumber) == 0)
    {
        std::cout<<TC_No<<". Checking if psapNumber set properly " + GREEN + "- Pass" + DONE<<endl;
    }
    else
    {
        std::cout<<TC_No<<". Checking if psapNumber set properly " + RED + "- Fail" + DONE<<endl;
    }

    result = taf_ecall_SetPsapNumber("1800233233");
    LE_TEST_OK(result == LE_OK, "taf_ecall_SetPsapNumber_test - LE_OK");
    report(LE_OK,result,"taf_ecall_SetPsapNumber_test");

    // Test Case
    result = taf_ecall_GetPsapNumber(psapNumber, TAF_SIM_PHONE_NUM_MAX_LEN);
    LE_TEST_OK(result == LE_OK, "taf_ecall_GetPsapNumber - LE_OK");
    report(LE_OK,result,"taf_ecall_GetPsapNumber");
    std::cout<<"*** psapNumber: "<<psapNumber<<endl;

    // Test Case
    LE_TEST_OK(strcmp("1800233233", psapNumber) == 0, "Checking if psapNumber set properly");
    if(strcmp("1800233233", psapNumber) == 0)
    {
        std::cout<<TC_No<<". Checking if psapNumber set properly " + GREEN + "- Pass" + DONE<<endl;
    }
    else
    {
        std::cout<<TC_No<<". Checking if psapNumber set properly " + RED + "- Fail" + DONE<<endl;
    }

    //Put back the psap number
    if(strcmp("None", psapNum) != 0)
    {
        result = taf_ecall_SetPsapNumber(psapNum);
    }

    TC_No += 1;

    return NULL;
}

static void* taf_ecall_GetNadDeregTime_test()
{
    // Test Case
    uint16_t deregTimeOrg = 0;
    le_result_t result = taf_ecall_GetNadDeregistrationTime(&deregTimeOrg);
    LE_TEST_OK(result == LE_OK, "taf_ecall_GetNadDeregTime_test - LE_OK");
    report(LE_OK,result,"taf_ecall_GetNadDeregTime_test");
    std::cout<<"*** Existing deregTime (in minutes): "<< deregTimeOrg <<endl;
    LE_TEST_INFO("taf_ecall_GetNadDeregTime_test done");

    return NULL;
}

static void* taf_ecall_SetNadDeregTime_test()
{
    uint16_t deregTimeOrg = 0;
    le_result_t result = taf_ecall_GetNadDeregistrationTime(&deregTimeOrg);
    result = taf_ecall_SetNadDeregistrationTime(5*60); // 5 hrs
    LE_TEST_OK(result == LE_OK, "taf_ecall_SetNadDeregTime_test - LE_OK");
    report(LE_OK,result,"taf_ecall_SetNadDeregTime_test");

    // Test Case
    uint16_t deregTime = 0;
    result = taf_ecall_GetNadDeregistrationTime(&deregTime);
    LE_TEST_OK(result == LE_OK, "taf_ecall_GetNadDeregTime_test - LE_OK");
    report(LE_OK,result,"taf_ecall_GetNadDeregTime_test");
    std::cout<<"*** deregTime (in minutes): "<< deregTime <<endl;

    // Test Case
    LE_TEST_OK(deregTime == 5*60, "Checking if Nad Deregistration Time set properly");
    if(deregTime == 5*60)
    {
        std::cout<<TC_No<<". Checking if Nad Deregistration Time set properly " + GREEN + "- Pass" + DONE<<endl;
    }
    else
    {
        std::cout<<TC_No<<". Checking if Nad Deregistration Time set properly " + RED + "- Fail" + DONE<<endl;
    }

    result = taf_ecall_SetNadDeregistrationTime(8*60); // 8 hrs
    LE_TEST_OK(result == LE_OK, "taf_ecall_SetNadDeregTime_test - LE_OK");
    report(LE_OK,result,"taf_ecall_SetNadDeregTime_test");

    // Test Case
    result = taf_ecall_GetNadDeregistrationTime(&deregTime);
    LE_TEST_OK(result == LE_OK, "taf_ecall_GetNadDeregTime_test - LE_OK");
    report(LE_OK,result,"taf_ecall_GetNadDeregTime_test");
    std::cout<<"*** deregTime (in minutes): "<< deregTime <<endl;

    // Test Case
    LE_TEST_OK(deregTime == 8*60, "Checking if Nad Deregistration Time set properly");
    if(deregTime == 8*60)
    {
        std::cout<<TC_No<<". Checking if Nad Deregistration Time set properly " + GREEN + "- Pass" + DONE<<endl;
    }
    else
    {
        std::cout<<TC_No<<". Checking if Nad Deregistration Time set properly " + RED + "- Fail" + DONE<<endl;
    }


    //Put back the original Nad Deregistration Time
    result = taf_ecall_SetNadDeregistrationTime(deregTimeOrg);

    TC_No += 1;

    return NULL;
}

static le_result_t taf_ecall_startECall_test
(
    const char* eCallType,
    const char* useUsimNumber,
    const char* psapNumber
)
{
    LE_INFO("Start eCall test with eCallType: %s, useUsimNumber: %s, psapNumber: %s",
            eCallType, useUsimNumber, psapNumber);
    le_result_t result;

    updateMsdInformation();

    taf_ecall_SetMsdPassengersCount(ECallRef, 2);
    taf_ecall_ImportMsd_test(ECallRef);

    if (strcmp(eCallType, "TEST") == 0)
    {
        if (useUsimNumber != NULL && strcmp(useUsimNumber, "True") == 0)
        {
            taf_ecall_UseUSimNumbers_test();
        }
        if (psapNumber != NULL && strcmp(psapNumber, "None") != 0)
        {
            result = taf_ecall_SetPsapNumber(psapNumber);
            LE_TEST_OK(result == LE_OK, "taf_ecall_SetPsapNumber - LE_OK");
            report(LE_OK,result,"taf_ecall_SetPsapNumber");
        }
        result = taf_ecall_StartTest(ECallRef);
    }
    else if (strcmp(eCallType, "MANUAL") == 0)
    {
        result = taf_ecall_StartManual(ECallRef);
    }
    else
    {
        result = taf_ecall_StartAutomatic(ECallRef);
    }

    LE_TEST_OK(result == LE_OK, "taf_ecall_startECall_test - LE_OK");
    report(LE_OK,result,"taf_ecall_startECall_test");

    return result;
}

static void* tafECallHandlerThread
(
    void* contextPtr
)
{
    taf_ecall_ConnectService();

    LE_TEST_INFO("Test register eCall state change handler");
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Test register eCall state change handler" << endl;
    std::cout <<"************************************************" << endl;
    HandlerRef = taf_ecall_AddStateChangeHandler(tafECallStateHandler, NULL);
    LE_TEST_OK(HandlerRef != NULL, "taf_ecall_AddStateChangeHandler - LE_OK");
    report(LE_OK,HandlerRef != NULL?LE_OK:LE_FAULT,"taf_ecall_AddStateChangeHandler");
    LE_TEST_INFO("taf_ecall_AddStateChangeHandler done");

    le_event_RunLoop();

    return NULL;
}

COMPONENT_INIT
{
    int status = EXIT_SUCCESS;
    le_result_t result;

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    const char* inputPhoneId = "SLOT1";
    const char* eCallType =  "AUTO";
    int NumberOfArgs = le_arg_NumArgs();
    LE_INFO("Total NumberOfArgs: %d", NumberOfArgs);
    if (NumberOfArgs >= 1)
    {
        inputPhoneId = le_arg_GetArg(0);
        if (NULL == inputPhoneId)
        {
            LE_ERROR("slotId input is NULL, input correct slot id. Check usages for details.");
            std::cout <<"slotId input is NULL, input correct slot id. Check usages for details." << endl;

            printUsages();

            exit(EXIT_SUCCESS);
        } else {
            LE_INFO("Input slotId: %s", inputPhoneId);
        }
    }
    else // if no arguements passed in the command line argument
    {
        LE_ERROR("No Parameter passed, provide slotId. Check usages for details.");
        std::cout <<"No Parameter passed, provide slotId. Check usages for details." << endl;

        printUsages();

        exit(EXIT_SUCCESS);
    }

    PhoneId = get_phone_id(inputPhoneId);
    LE_INFO("PhoneId of this test run is: SIM%d", (int) PhoneId);

    if (NumberOfArgs > 1)
    {
        eCallType = le_arg_GetArg(1);
        LE_INFO("Input eCallType: %s", eCallType);
        if (NULL == eCallType) {
            eCallType = "AUTO";
        }
    }

    const char* useUsimNumber =  "False";
    if (NumberOfArgs > 2)
    {
        useUsimNumber = le_arg_GetArg(2);
        LE_INFO("Input useUsimNumber: %s", useUsimNumber);
    }

    const char* psapNumber =  "None";
    if (NumberOfArgs > 3)
    {
        psapNumber = le_arg_GetArg(3);
        LE_INFO("Input psapNumber: %s", psapNumber);
    }

    TestSemRef = le_sem_Create("tafeCallTestAppSem", 0);

    printPreCondition();
    printUsages();

    ECallRef = taf_ecall_Create();

    LE_TEST_INFO("Test MSD transmition mode of ECall");
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Test MSD transmition mode of ECall" << endl;
    std::cout <<"************************************************" << endl;
    taf_ecall_setMsdTxMode_test();
    taf_ecall_getMsdTxMode_test();

    LE_TEST_INFO("Test operational mode of ECall");
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Test operational mode of ECall" << endl;
    std::cout <<"************************************************" << endl;
    taf_ecall_setOpMode_test();
    taf_ecall_getOpMode_test();

    LE_TEST_INFO("Test MSD version of ECall");
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Test MSD version of ECall" << endl;
    std::cout <<"************************************************" << endl;
    taf_ecall_getMsdVersion_test();
    taf_ecall_setMsdVersion_test();

    LE_TEST_INFO("Test vehicle type of ECall");
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Test vehicle type of ECall" << endl;
    std::cout <<"************************************************" << endl;
    taf_ecall_getVehicleType_test();
    taf_ecall_setVehicleType_test();

    LE_TEST_INFO("Test Vehicle Identification Number of ECall");
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Test Vehicle Identification Number of ECall" << endl;
    std::cout <<"************************************************" << endl;
    taf_ecall_getVIN_test();
    taf_ecall_setVIN_test();

    LE_TEST_INFO("Test vehicle propulsion storage type of ECall");
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Test vehicle propulsion storage type of ECall" << endl;
    std::cout <<"************************************************" << endl;
    taf_ecall_getPropulsionType_test();
    taf_ecall_setPropulsionType_test();

    if(threadRef == NULL) {
        threadRef = le_thread_Create("taf_ecall_state_thread", tafECallHandlerThread, NULL);
        le_thread_Start(threadRef);
    }

    sleep(1); //Wait to register handler...

    LE_TEST_INFO("Test MSD position of an ECall");
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Test MSD position of an ECall" << endl;
    std::cout <<"************************************************" << endl;
    taf_ecall_setMsdPosition_tests();

    LE_TEST_INFO("Test MSD Passengers Count of an ECall");
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Test MSD Passengers of an ECall" << endl;
    std::cout <<"************************************************" << endl;
    taf_ecall_setMsdPassengersCount_tests();

    LE_TEST_INFO("Test PSAP Number of ECall");
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Test PSAP Number of ECall" << endl;
    std::cout <<"************************************************" << endl;
    taf_ecall_GetPsapNumber_test();
    taf_ecall_SetPsapNumber_test();

    LE_TEST_INFO("Test Nad Deregistration Time of ECall");
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Test Nad Deregistration Time of ECall" << endl;
    std::cout <<"************************************************" << endl;
    taf_ecall_GetNadDeregTime_test();
    taf_ecall_SetNadDeregTime_test();

    LE_TEST_INFO("Test start eCall (AUTO/MANUAL)");
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Test start eCall (AUTO/MANUAL)" << endl;
    std::cout <<"************************************************" << endl;
    result = taf_ecall_startECall_test(eCallType, useUsimNumber, psapNumber);
    if (result == LE_OK) {
        exitApp = false;
        le_sem_Wait(TestSemRef);
    }

    le_thread_Cancel(threadRef);

    LE_TEST_INFO("Test remove handler of ECall");
    std::cout <<endl;
    std::cout <<"************************************************" << endl;
    std::cout <<"Test remove handler of ECall" << endl;
    std::cout <<"************************************************" << endl;
    taf_ecall_RemoveStateChangeHandler_test();
    printResultMsg();

    if (exitApp) {
        le_sem_Delete(TestSemRef);
        exit(status);
    }
}
