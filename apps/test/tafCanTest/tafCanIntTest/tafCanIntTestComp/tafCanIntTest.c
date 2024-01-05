/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

/*
 * @file       tafCanIntTest.cpp
 * @brief      This file includes integration test functions of the CAN Service.
 */

#include "legato.h"
#include "interfaces.h"

taf_can_CanEventHandlerRef_t HandlerRef = NULL;
static taf_can_CanInterfaceRef_t CanInfRef = NULL;
static taf_can_CanFrameRef_t FrameRef = NULL;
static le_sem_Ref_t SemRef;

void DisplayAppUsage(void)
{
    printf("\nUsage of the 'tafCanIntTest' application is:\n");
    printf("Please follow the instructions mentioned\n");
    printf("Test: is Fd frame Supported\n");
    printf("=== eg: ./tafCanIntTest isFdSupported can0 === \n");
    printf("Test: getFdStatus\n");
    printf("=== eg: ./tafCanIntTest getFdStatus can0 === \n");
    printf("Test: RegisterListener\n");
    printf("=== eg: ./tafCanIntTest rcvFrame can0 123 7ff === \n");
    printf("Test: UnRegisterListener\n");
    printf("=== eg: ./tafCanIntTest unregisterFrame can0 123 7ff ===\n");
    printf("Test: setPayload\n");
    printf("=== eg: ./tafCanIntTest setPayload can0 123 1234ABCF ===\n");
    printf("Test: SendCanFrame\n");
    printf("=== eg: ./tafCanIntTest sendCanFrame enableLoopback can0 123 1234af56 ===\n");
    printf("Test: SendCanFdFrame\n");
    printf("=== eg: ./tafCanIntTest sendCanFdFrame enableLoopback can0 123 1234af56 ===\n");
    printf("Test: SendAutoFrame\n");
    printf("=== eg: ./tafCanIntTest sendAutoFrame enableLoopback can0 123 1234af56 ===\n\n");
}

/*
 * Callback function
 */
static void TestTafCanCallback
(
    taf_can_CanInterfaceRef_t canInfRef,
    bool isCanFdFrame,
    uint32_t frameId,
    const uint8_t* dataPtr,
    size_t size,
    void* contextPtr
)
{
    printf("Received frameType: %s, FrameId: 0x%03X, CAN Interface reference: %p\n",
            (isCanFdFrame ? "CAN FD frames":"CAN 2.0 frames"), frameId, canInfRef);

    printf("Received CAN data is: ");
    for(int i= 0; i< size; i++)
    {
        printf("%02x ", dataPtr[i]);
    }

    char rcvData[(2*size) + 1];
    le_hex_BinaryToString(dataPtr, size, rcvData, (2*size) + 1);
    LE_INFO("Received CAN data is %s", rcvData);

    le_sem_Post(SemRef);
    printf("\n=== Message Received Successfully ===\n\n");
}

/*
 * Create CAN interface
 */
taf_can_CanInterfaceRef_t test_taf_can_CreateCanInf
(
    const char* infNamePtr,
    taf_can_InfProtocol_t canInfType
)
{
    taf_can_CanInterfaceRef_t CanInfRef = taf_can_CreateCanInf(infNamePtr, canInfType);

    LE_TEST_OK(CanInfRef != NULL, "CAN interface created successfully");
    return CanInfRef;
}

/*
 * Create CAN Frame
 */
taf_can_CanFrameRef_t test_taf_can_CreateCanFrame
(
    taf_can_CanInterfaceRef_t canInfRef,
    uint32_t frameId
)
{
    taf_can_CanFrameRef_t frameRef = taf_can_CreateCanFrame(canInfRef, frameId);

    LE_TEST_OK(frameRef != NULL, "CAN Frame created successfully");
    return frameRef;
}

/*
 * Returns the decimal value of a given ASCII hex character.
 *
 * While 0..9, a..f, A..F are valid ASCII hex characters.
 * On invalid characters the value 16 is returned for error handling.
 */
static unsigned char AscHexToDecimal
(
    char c
)
{
    if ((c >= '0') && (c <= '9'))
        return c - '0';

    if ((c >= 'A') && (c <= 'F'))
        return c - 'A' + 10;

    if ((c >= 'a') && (c <= 'f'))
        return c - 'a' + 10;

    return 16;
}

static le_result_t test_taf_can_SetPayload
(
    taf_can_CanFrameRef_t frameRef,
    const char* dataPtr,
    size_t size
)
{
    le_result_t result;
    int maxDataLen = TAF_CAN_DATA_MAX_LENGTH;
    int i, dataLen;
    int idx = 0;
    unsigned char tmp;
    uint8_t data[TAF_CAN_DATA_MAX_LENGTH];
    int len = size;

    for (i=0, dataLen=0; i < maxDataLen; i++)
    {
        if(idx >= len) /* end of string => end of data */
            break;

        if ((tmp = AscHexToDecimal(dataPtr[idx++])) > 0x0F)
        {
            LE_ERROR("data Formate Error");
            exit(EXIT_FAILURE);
        }
        data[i] = (tmp << 4);

        if ((tmp = AscHexToDecimal(dataPtr[idx++])) > 0x0F)
        {
            LE_ERROR("data Formate Error");
            exit(EXIT_FAILURE);
        }
        data[i] |= tmp;
        dataLen++;
    }

    result = taf_can_SetPayload(frameRef, data, dataLen);
    LE_TEST_OK(result == LE_OK, "taf_can_SetPayload - LE_OK");
    if(result == LE_OK)
    {
        LE_INFO("CAN data set successfully to send");
        return LE_OK;
    }
    else
    {
        LE_ERROR("Set payload failed");
        return LE_FAULT;
    }
}

static void* test_CanEventHandler
(
    void* ctxPtr
)
{
    le_result_t result;
    taf_can_ConnectService();

    const char* infNamePtr = le_arg_GetArg(1);
    if (infNamePtr == NULL)
    {
        LE_ERROR("CAN test: NULL argument received, Line %d", __LINE__);
        return NULL;
    }

    taf_can_InfProtocol_t canInfType = TAF_CAN_RAW_SOCK;

    const char *frameIdPtr = le_arg_GetArg(2);
    const char *frIdMaskPtr = le_arg_GetArg(3);

    if (frameIdPtr == NULL || frIdMaskPtr == NULL)
    {
        LE_ERROR("CAN test: NULL argument received, Line %d", __LINE__);
        return NULL;
    }

    uint32_t frameId = le_hex_HexaToInteger(frameIdPtr);
    uint32_t frIdMask = le_hex_HexaToInteger(frIdMaskPtr);

    uint8_t test = 1;

    taf_can_CanInterfaceRef_t canInfRef = test_taf_can_CreateCanInf(infNamePtr, canInfType);

    result = taf_can_SetFilter(canInfRef, frameId);
    LE_TEST_OK(result == LE_OK, "taf_can_SetFilter - LE_OK");
    if (result == LE_OK)
    {
        printf("\n=== Filter is set to receive CAN frame for frameId: 0x%03X ===\n", frameId);
        LE_INFO("Filter is set to receive CAN frame for frameId: 0x%03X", frameId);
    }
    else
    {
        printf("\n== CAN msg will not receive because filter is not set for frameId: 0x%03X ==\n",
                frameId);
    }

    HandlerRef = taf_can_AddCanEventHandler(canInfRef, frameId, frIdMask,
            TestTafCanCallback, &test);

    LE_TEST_OK(HandlerRef != NULL, "CanEventHandler Registered successfully");
    if (HandlerRef != NULL)
    {
        printf("\n=== Waiting to receive Can Frame for reqested frameId ===\n");
    }

    le_event_RunLoop();
}

static le_result_t WaitForSemTimeout
(
    le_sem_Ref_t SemRef,
    uint32_t seconds
)
{
    le_clk_Time_t timeToWait = {seconds, 0};
    return le_sem_WaitWithTimeOut(SemRef, timeToWait);
}

static void* test_taf_can_AddCanEventHandler
(
    void* ctxPtr
)
{
    LE_TEST_INFO("Test Add CAN event handler");
    taf_can_ConnectService();

    uint32_t frameId = 0x123;
    uint32_t frIdMask = 0x7ff;
    uint8_t test = 1;

    HandlerRef = taf_can_AddCanEventHandler(CanInfRef, frameId, frIdMask,
            TestTafCanCallback, &test);

    LE_TEST_OK(HandlerRef != NULL, "CanEventHandler Registered successfully");

    le_event_RunLoop();
    return NULL;
}

static void TestDefaultFunc
(
)
{
    const char* infNamePtr = "can0";
    uint32_t frameId = 0x123;
    le_result_t result;
    bool fdEnableStatus;
    taf_can_InfProtocol_t canInfType = TAF_CAN_RAW_SOCK;

    CanInfRef = test_taf_can_CreateCanInf(infNamePtr, canInfType);

    FrameRef = test_taf_can_CreateCanFrame(CanInfRef, frameId);

    result = taf_can_SetFilter(CanInfRef, frameId);
    LE_TEST_OK(result == LE_OK, "taf_can_SetFilter - LE_OK");

    result = taf_can_DisableLoopback(CanInfRef);
    LE_TEST_OK(result == LE_OK, "taf_can_DisableLoopback - LE_OK");

    result = taf_can_EnableLoopback(CanInfRef);
    LE_TEST_OK(result == LE_OK, "taf_can_EnableLoopback - LE_OK");

    result = taf_can_DisableRcvOwnMsg(CanInfRef);
    LE_TEST_OK(result == LE_OK, "taf_can_DisableRcvOwnMsg - LE_OK");

    result = taf_can_EnableRcvOwnMsg(CanInfRef);
    LE_TEST_OK(result == LE_OK, "taf_can_EnableRcvOwnMsg - LE_OK");

    if (taf_can_IsFdSupported(CanInfRef) == true)
    {
        LE_TEST_INFO("Device support CAN-FD Frame");

        result = taf_can_EnableFdFrame(CanInfRef);
        LE_TEST_OK(result == LE_OK, "taf_can_EnableFdFrame - LE_OK");

        fdEnableStatus = taf_can_GetFdStatus(CanInfRef);
        LE_TEST_OK(fdEnableStatus == true, "taf_can_GetFdStatus - LE_OK");
    }
    else
    {
        LE_TEST_INFO("Device does not support CAN-FD Frame");
    }

    // To receive CAN message
    SemRef = le_sem_Create("SemRef", 0);
    le_thread_Ref_t threadRef = le_thread_Create("taf_Can_AddCanEventHandler",
            test_taf_can_AddCanEventHandler, NULL);
    le_thread_Start(threadRef);

    //send CAN 2.0 Frame
    const char* canDataPtr = "12345678abcdef09";
    int datalen = strlen(canDataPtr);
    test_taf_can_SetPayload(FrameRef, canDataPtr, datalen);
    taf_can_FrameType_t frameType = TAF_CAN_CAN_FRAME;
    result = taf_can_SetFrameType(FrameRef, frameType);
    LE_TEST_OK(result == LE_OK, "taf_can_SetFrameType - LE_OK");
    result = taf_can_SendFrame(FrameRef);
    LE_TEST_OK(result == LE_OK, "taf_can_SendFrame - LE_OK");
    result = WaitForSemTimeout(SemRef, 5);
    LE_ASSERT(result == LE_OK);

    //send CAN FD Frame
    const char* canFdDataPtr = "12345678abcdef09abcd";
    datalen = strlen(canFdDataPtr);
    test_taf_can_SetPayload(FrameRef, canFdDataPtr, datalen);
    frameType = TAF_CAN_CAN_FD_FRAME;
    result = taf_can_SetFrameType(FrameRef, frameType);
    LE_TEST_OK(result == LE_OK, "taf_can_SetFrameType - LE_OK");
    result = taf_can_SendFrame(FrameRef);
    LE_TEST_OK(result == LE_OK, "taf_can_SendFrame - LE_OK");
    result = WaitForSemTimeout(SemRef, 5);
    LE_ASSERT(result == LE_OK);

    //send CAN auto Frame
    const char* canAutoDataPtr = "12345678abcdef";
    datalen = strlen(canAutoDataPtr);
    test_taf_can_SetPayload(FrameRef, canAutoDataPtr, datalen);
    frameType = TAF_CAN_AUTO_FRAME;
    result = taf_can_SetFrameType(FrameRef, frameType);
    LE_TEST_OK(result == LE_OK, "taf_can_SetFrameType - LE_OK");
    result = taf_can_SendFrame(FrameRef);
    LE_TEST_OK(result == LE_OK, "taf_can_SendFrame - LE_OK");
    result = WaitForSemTimeout(SemRef, 5);
    LE_ASSERT(result == LE_OK);

    sleep(2);

    taf_can_RemoveCanEventHandler(HandlerRef);

    result = taf_can_DeleteCanFrame(FrameRef);
    LE_TEST_OK(result == LE_OK, "taf_can_DeleteCanFrame - LE_OK");

    result = taf_can_DeleteCanInf(CanInfRef);
    LE_TEST_OK(result == LE_OK, "taf_can_DeleteCanInf - LE_OK");
}

COMPONENT_INIT
{
    bool exitApplication = false;
    const char* paramType = "";

    uint32_t frameId;

    int NumberOfArgs = le_arg_NumArgs();
    LE_INFO("Total NumberOfArgs: %d", NumberOfArgs);

    if (NumberOfArgs >= 1)
    {
        paramType = le_arg_GetArg(0);

        if (NULL == paramType)
        {
            LE_ERROR("paramType is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
    }
    else //Run default function if no arguements passed in the command line argument
    {
        LE_TEST_INFO("Test defualt Function");
        TestDefaultFunc();

        LE_INFO("Application exits successfully");
        printf("\n=== Application exits successfully ===\n");
        exit(EXIT_SUCCESS);
    }

    /* isFdSupported
    * Command Line Arguement Format :
    * ./tafCanIntTest <isFdSupported> <interfaceName>
    * e.g: ./tafCanIntTest isFdSupported can0
    */
    if (strcmp(paramType,"isFdSupported") == 0)
    {
        if (NumberOfArgs != 2)
        {
            printf("\n === NumberOfArgs is wrong. Number of required argument is 2 ===\n");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }

        LE_TEST_INFO("Testing to know whether the device support FD frame or not");

        const char* infNamePtr = le_arg_GetArg(1);
        if (infNamePtr == NULL)
        {
            LE_ERROR("CAN test: NULL argument received, Line %d", __LINE__);
            exit(EXIT_FAILURE);
        }
        taf_can_InfProtocol_t canInfType = TAF_CAN_RAW_SOCK;

        taf_can_CanInterfaceRef_t canInfRef = test_taf_can_CreateCanInf(infNamePtr, canInfType);

        if (taf_can_IsFdSupported(canInfRef) == true)
        {
            printf("\n=== %s supports CAN-FD Frame ===\n", infNamePtr);
            LE_INFO("%s support CAN-FD Frame", infNamePtr);
        }
        else
        {
            printf("\n=== %s does not supports CAN-FD Frame ===\n", infNamePtr);
            LE_INFO("%s does not support CAN-FD Frame", infNamePtr);
        }

        exitApplication = true;
    }
    /* getFdStatus
    * Command Line Arguement Format :
    * ./tafCanIntTest <getFdStatus> <interfaceName>
    * e.g: ./tafCanIntTest getFdStatus can0
    */
    else if (strcmp(paramType,"getFdStatus") == 0)
    {
        if (NumberOfArgs != 2)
        {
            printf("\n === NumberOfArgs is wrong. Number of required argument is 2 ===\n");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }

        LE_TEST_INFO("Testing to know whether the FD frame is enabled or not");

        const char* infNamePtr = le_arg_GetArg(1);
        if (infNamePtr == NULL)
        {
            LE_ERROR("CAN test: NULL argument received, Line %d", __LINE__);
            exit(EXIT_FAILURE);
        }
        taf_can_InfProtocol_t canInfType = TAF_CAN_RAW_SOCK;

        taf_can_CanInterfaceRef_t canInfRef = test_taf_can_CreateCanInf(infNamePtr, canInfType);

        if (taf_can_GetFdStatus(canInfRef) == true)
        {
            printf("\n=== CAN-Fd frame is enabled ===\n");
        }
        else
        {
            printf("\n=== CAN-Fd frame is not enabled  ===\n");
            LE_INFO("CAN-Fd frame is not enabled");
        }

        exitApplication = true;
    }
    /* RegisterListener
    * Command Line Arguement Format :
    * ./tafCanIntTest <registerListener> <interfaceName> <frameId> <frIdMask>
    * e.g: ./tafCanIntTest rcvFrame can0 123 7ff
    */
    else if (strcmp(paramType,"rcvFrame") == 0)
    {
        if (NumberOfArgs != 4)
        {
            printf("\n === NumberOfArgs is wrong. Number of required argument is 4 ===\n");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }

        printf("\n=== AddCanEventHandler ===\n");
        LE_TEST_INFO("Testing receive CAN Frame - AddCanEventHandler");

        SemRef = le_sem_Create("SemRef", 0);
        le_thread_Ref_t threadRef = le_thread_Create("taf_Can_AddCanEventHandler",
                test_CanEventHandler, NULL);
        le_thread_Start(threadRef);
    }
    /* UnRegisterListener
    * Command Line Arguement Format :
    * ./tafCanIntTest <unRegisterListener> <interfaceName> <frameId> <frIdMask>
    * e.g: ./tafCanIntTest unregisterFrame can0 123 7ff
    */
    else if (strcmp(paramType,"unregisterFrame") == 0)
    {
        if (NumberOfArgs != 4)
        {
            printf("\n === NumberOfArgs is wrong. Number of required argument is 4 ===\n");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }

        le_result_t result;
        SemRef = le_sem_Create("SemRef", 0);
        le_thread_Ref_t threadRef = le_thread_Create("taf_Can_AddCanEventHandler",
                test_CanEventHandler, NULL);
        le_thread_Start(threadRef);
        result = WaitForSemTimeout(SemRef, 5);
        LE_ASSERT(result == LE_OK);

        LE_INFO("called taf_can_RemoveCanEventHandler in app");
        sleep(5);
        taf_can_RemoveCanEventHandler(HandlerRef);

        printf("\n=== CanEventHandler UnRegistered successfully ===\n");
        LE_INFO("CanEventHandler Removed successfully");
    }
    /* SetPayload
    * Command Line Arguement Format :
    * ./tafCanIntTest <setPayload> <interfaceName> <frameId> <CAN_Data>
    * e.g: ./tafCanIntTest setPayload can0 123 1234ABCF
    */
    else if (strcmp(paramType,"setPayload") == 0)
    {
        if (NumberOfArgs != 4)
        {
            printf("\n === NumberOfArgs is wrong. Number of required argument is 4 ===\n");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }

        printf("\n=== Set Payload ===\n");
        LE_TEST_INFO("Testing Set Payload");

        le_result_t result;
        const char* infNamePtr = le_arg_GetArg(1);
        const char *frameIdPtr = le_arg_GetArg(2);
        const char* dataPtr = le_arg_GetArg(3);

        if (infNamePtr == NULL || frameIdPtr == NULL || dataPtr == NULL)
        {
            LE_ERROR("CAN test: NULL argument received, Line %d", __LINE__);
            exit(EXIT_FAILURE);
        }
        taf_can_InfProtocol_t canInfType = TAF_CAN_RAW_SOCK;
        frameId = le_hex_HexaToInteger(frameIdPtr);
        int datalen = strlen(dataPtr);

        taf_can_CanInterfaceRef_t canInfRef = test_taf_can_CreateCanInf(infNamePtr, canInfType);
        taf_can_CanFrameRef_t frameRef = test_taf_can_CreateCanFrame(canInfRef, frameId);

        result = test_taf_can_SetPayload(frameRef, dataPtr, datalen);
        LE_TEST_OK(result == LE_OK, "test_taf_can_SetPayload - LE_OK");

        exitApplication = true;
    }
    /* sendCanFrame
    * Command Line Arguement Format :
    * ./tafCanIntTest <sendCanFrame> <enable/disable loopback> <infName> <frameId> <CAN_Data>
    * e.g: ./tafCanIntTest sendCanFrame enableLoopback can0 123 12ab56CF
    */
    else if (strcmp(paramType,"sendCanFrame") == 0)
    {
        if (NumberOfArgs != 5)
        {
            printf("\n === NumberOfArgs is wrong. Number of required argument is 4 ===\n");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }

        printf("\n=== sendCanFrame ===\n");
        LE_TEST_INFO("Testing TelAF Send CAN Frame - TAF_CAN_CAN_FRAME");

        le_result_t result;

        const char* loopbackPtr = le_arg_GetArg(1);
        const char* infNamePtr = le_arg_GetArg(2);
        const char *frameIdPtr = le_arg_GetArg(3);
        const char* dataPtr = le_arg_GetArg(4);

        if (loopbackPtr == NULL || infNamePtr == NULL || frameIdPtr == NULL || dataPtr == NULL)
        {
            LE_ERROR("CAN test: NULL argument received, Line %d", __LINE__);
            exit(EXIT_FAILURE);
        }

        frameId = le_hex_HexaToInteger(frameIdPtr);
        int datalen = strlen(dataPtr);

        taf_can_InfProtocol_t canInfType = TAF_CAN_RAW_SOCK;
        taf_can_FrameType_t frameType = TAF_CAN_CAN_FRAME;

        taf_can_CanInterfaceRef_t canInfRef = test_taf_can_CreateCanInf(infNamePtr, canInfType);
        taf_can_CanFrameRef_t frameRef = test_taf_can_CreateCanFrame(canInfRef, frameId);

        if(strcmp("enableLoopback", loopbackPtr) == 0)
        {
            result = taf_can_EnableLoopback(canInfRef);
            LE_TEST_OK(result == LE_OK, "taf_can_EnableLoopback - LE_OK");
            printf("\n Loopback enabled \n");
        }
        else if(strcmp("disableLoopback", loopbackPtr) == 0)
        {
            result = taf_can_DisableLoopback(canInfRef);
            LE_TEST_OK(result == LE_OK, "taf_can_DisableLoopback - LE_OK");
            printf("\n Loopback disabled \n");
        }

        result = taf_can_SetFrameType(frameRef, frameType);
        LE_TEST_OK(result == LE_OK, "taf_can_SetFrameType - LE_OK");
        if(result == LE_OK)
        {
            printf("\nCAN 2.0 frame type is set to send\n");
            LE_INFO("CAN 2.0 frame tyep is set to send");
        }

        test_taf_can_SetPayload(frameRef, dataPtr, datalen);
        LE_TEST_OK(result == LE_OK, "test_taf_can_SetPayload - LE_OK");

        result = taf_can_SendFrame(frameRef);
        LE_TEST_OK(result == LE_OK, "taf_can_SendFrame - LE_OK");
        if (result == LE_OK)
        {
            printf("\n=== Message sent successfully ===\n");
            LE_INFO("Legacy Can Frame sent successfully");
        }
        else
        {
            printf("\n=== Send Message Error ===\n");
            exit(EXIT_FAILURE);
        }

        result = taf_can_DeleteCanInf(canInfRef);
        LE_TEST_OK(result == LE_OK, "taf_can_DeleteCanInf - LE_OK");
        if (result == LE_OK)
        {
            printf("\n=== Created CAN interface reference deleted and all memory released ===\n");
        }
        else
        {
            printf("\n=== Delete CAN interface reference Error ===\n");
            exit(EXIT_FAILURE);
        }
        exitApplication = true;
    }
    /* sendCanFdFrame
    * Command Line Arguement Format :
    * ./tafCanIntTest <sendCanFdFrame> <enable/disable loopback> <infName> <frameId> <CAN_Data>
    * e.g: ./tafCanIntTest sendCanFdFrame enableLoopback can0 123 12ab56CF
    */
    else if (strcmp(paramType,"sendCanFdFrame") == 0)
    {
        if (NumberOfArgs != 5)
        {
            printf("\n === NumberOfArgs is wrong. Number of required argument is 4 ===\n");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }

        printf("\n=== sendCanFdFrame ===\n");
        LE_TEST_INFO("Testing TelAF Send CAN-FD Frame - TAF_CAN_CAN_FD_FRAME");

        le_result_t result;
        const char* loopbackPtr = le_arg_GetArg(1);
        const char* infNamePtr = le_arg_GetArg(2);
        const char *frameIdPtr = le_arg_GetArg(3);
        const char* dataPtr = le_arg_GetArg(4);
        if (loopbackPtr == NULL || infNamePtr == NULL || frameIdPtr == NULL || dataPtr == NULL)
        {
            LE_ERROR("CAN test: NULL argument received, Line %d", __LINE__);
            exit(EXIT_FAILURE);
        }

        frameId = le_hex_HexaToInteger(frameIdPtr);
        int datalen = strlen(dataPtr);

        taf_can_InfProtocol_t canInfType = TAF_CAN_RAW_SOCK;
        taf_can_FrameType_t frameType = TAF_CAN_CAN_FD_FRAME;

        taf_can_CanInterfaceRef_t canInfRef = test_taf_can_CreateCanInf(infNamePtr, canInfType);
        taf_can_CanFrameRef_t frameRef = test_taf_can_CreateCanFrame(canInfRef, frameId);

        if(strcmp("enableLoopback", loopbackPtr) == 0)
        {
            result = taf_can_EnableLoopback(canInfRef);
            LE_TEST_OK(result == LE_OK, "taf_can_EnableLoopback - LE_OK");
            printf("\n Loopback enabled \n");
        }
        else if(strcmp("disableLoopback", loopbackPtr) == 0)
        {
            result = taf_can_DisableLoopback(canInfRef);
            LE_TEST_OK(result == LE_OK, "taf_can_DisableLoopback - LE_OK");
            printf("\n Loopback disabled \n");
        }

        result = taf_can_SetFrameType(frameRef, frameType);
        LE_TEST_OK(result == LE_OK, "taf_can_SetFrameType - LE_OK");
        if(result == LE_OK)
        {
            printf("\nCAN-FD frame type is set to send\n");
            LE_INFO("CAN-FD frame tyep is set to send");
        }

        result = test_taf_can_SetPayload(frameRef, dataPtr, datalen);
        LE_TEST_OK(result == LE_OK, "test_taf_can_SetPayload - LE_OK");

        result = taf_can_EnableFdFrame(canInfRef);
        LE_TEST_OK(result == LE_OK, "taf_can_EnableFdFrame - LE_OK");

        result = taf_can_SendFrame(frameRef);
        LE_TEST_OK(result == LE_OK, "taf_can_SendFrame - LE_OK");
        if (result == LE_OK)
        {
            printf("\n=== Message sent successfully ===\n");
            LE_INFO("CAN-FD Frame sent successfully");
        }
        else
        {
            printf("\n=== Send Message Error ===\n");
            exit(EXIT_FAILURE);
        }

        result = taf_can_DeleteCanInf(canInfRef);
        LE_TEST_OK(result == LE_OK, "taf_can_DeleteCanInf - LE_OK");
        if (result == LE_OK)
        {
            printf("\n=== Created CAN interface reference deleted and all memory released ===\n");
        }
        else
        {
            printf("\n=== Delete CAN interface reference Error ===\n");
            exit(EXIT_FAILURE);
        }
        exitApplication = true;
    }
    /* sendAutoFrame
    * Command Line Arguement Format :
    * ./tafCanIntTest <sendAutoFrame> <enable/disable loopback> <infName> <frameId> <CAN_Data>
    * e.g: ./tafCanIntTest sendAutoFrame enableLoopback can0 123 12ab56CF
    */
    else if (strcmp(paramType,"sendAutoFrame") == 0)
    {
        if (NumberOfArgs != 5)
        {
            printf("\n === NumberOfArgs is wrong. Number of required argument is 4 ===\n");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }

        printf("\n=== sendAutoFrame ===\n");
        LE_TEST_INFO("Testing TelAF Send CAN-FD Frame - TAF_CAN_AUTO_FRAME");

        le_result_t result;

        const char* loopbackPtr = le_arg_GetArg(1);
        const char* infNamePtr = le_arg_GetArg(2);
        const char *frameIdPtr = le_arg_GetArg(3);
        const char* dataPtr = le_arg_GetArg(4);
        if (loopbackPtr == NULL || infNamePtr == NULL || frameIdPtr == NULL || dataPtr == NULL)
        {
            LE_ERROR("CAN test: NULL argument received, Line %d", __LINE__);
            exit(EXIT_FAILURE);
        }

        frameId = le_hex_HexaToInteger(frameIdPtr);
        int datalen = strlen(dataPtr);

        taf_can_InfProtocol_t canInfType = TAF_CAN_RAW_SOCK;
        taf_can_FrameType_t frameType= TAF_CAN_AUTO_FRAME;

        taf_can_CanInterfaceRef_t canInfRef = test_taf_can_CreateCanInf(infNamePtr, canInfType);
        taf_can_CanFrameRef_t frameRef = test_taf_can_CreateCanFrame(canInfRef, frameId);

        if(strcmp("enableLoopback", loopbackPtr) == 0)
        {
            result = taf_can_EnableLoopback(canInfRef);
            LE_TEST_OK(result == LE_OK, "taf_can_EnableLoopback - LE_OK");
            printf("\n Loopback enabled \n");
        }
        else if(strcmp("disableLoopback", loopbackPtr) == 0)
        {
            result = taf_can_DisableLoopback(canInfRef);
            LE_TEST_OK(result == LE_OK, "taf_can_DisableLoopback - LE_OK");
            printf("\n Loopback disabled \n");
        }

        result = taf_can_SetFrameType(frameRef, frameType);
        LE_TEST_OK(result == LE_OK, "taf_can_SetFrameType - LE_OK");
        if(result == LE_OK)
        {
            printf("\n Auto CAN frame type is set to send\n");
            LE_INFO("Auto CAN frame tyep is set to send");
        }

        test_taf_can_SetPayload(frameRef, dataPtr, datalen);
        LE_TEST_OK(result == LE_OK, "test_taf_can_SetPayload - LE_OK");

        result = taf_can_SendFrame(frameRef);
        LE_TEST_OK(result == LE_OK, "taf_can_SendFrame - LE_OK");
        if (result == LE_OK)
        {
            printf("\n=== Message sent successfully ===\n");
            LE_INFO("Auto CAN Frame sent successfully");
        }
        else
        {
            printf("\n=== Send Message Error ===\n");
            exit(EXIT_FAILURE);
        }

        result = taf_can_DeleteCanInf(canInfRef);
        LE_TEST_OK(result == LE_OK, "taf_can_DeleteCanInf - LE_OK");
        if (result == LE_OK)
        {
            printf("\n=== Created CAN interface reference deleted and all memory released ===\n");
        }
        else
        {
            printf("\n=== Delete CAN interface reference Error ===\n");
            exit(EXIT_FAILURE);
        }
        exitApplication = true;
    }
    else
    {
        LE_ERROR("header is not matching, exiting application");
        printf("\n=== header is not matching, exiting application ===\n");
        DisplayAppUsage();
        exit(EXIT_FAILURE);
    }

    if (exitApplication)
    {
        LE_INFO("Application exits successfully");
        printf("\n=== Application exits successfully ===\n");
        exit(EXIT_SUCCESS);
    }
}
