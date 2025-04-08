/*
 *  Copyright (c) 2022, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/*
 * @file       tafCanUnitTest.cpp
 * @brief      This file includes unit test functions of the CAN Service.
 */

#include "legato.h"
#include "interfaces.h"

static taf_can_CanInterfaceRef_t CanInfRef = NULL;
static taf_can_CanFrameRef_t FrameRef = NULL;
static taf_can_CanEventHandlerRef_t HandlerRef = NULL;
static le_sem_Ref_t SemRef;

/*
 * Create CAN interface
 */
taf_can_CanInterfaceRef_t test_taf_can_CreateCanInf
(
)
{
    LE_TEST_INFO("Testing create CAN interface reference");

    const char* infNamePtr = "can0";
    taf_can_InfProtocol_t canInfType = TAF_CAN_RAW_SOCK;
    CanInfRef = taf_can_CreateCanInf(infNamePtr, canInfType);

    LE_TEST_OK(CanInfRef != NULL, "CAN interface created successfully");
    return CanInfRef;
}

/*
 * Create CAN Frame
 */
taf_can_CanFrameRef_t test_taf_can_CreateCanFrame
(
)
{
    LE_TEST_INFO("Testing create CAN Frame reference");

    uint32_t frameId = 0x123;
    FrameRef = taf_can_CreateCanFrame(CanInfRef, frameId);

    LE_TEST_OK(FrameRef != NULL, "CAN Frame created successfully");
    return FrameRef;
}

static void test_taf_can_SetFilter
(
)
{
    LE_TEST_INFO("Testing Set Filter");

    le_result_t result;
    uint32_t frameId = 0x123;

    result = taf_can_SetFilter(CanInfRef, frameId);
    LE_TEST_OK(result == LE_OK, "taf_can_SetFilter - LE_OK");

    return;
}

static void test_taf_can_EnableLoopback
(
)
{
    LE_TEST_INFO("Testing Enable Loopback");
    le_result_t result;

    result = taf_can_EnableLoopback(CanInfRef);
    LE_TEST_OK(result == LE_OK, "taf_can_EnableLoopback - LE_OK");

    return;
}

static void test_taf_can_DisableLoopback
(
)
{
    LE_TEST_INFO("Testing Disable Loopback");
    le_result_t result;

    result = taf_can_DisableLoopback(CanInfRef);
    LE_TEST_OK(result == LE_OK, "taf_can_DisableLoopback - LE_OK");

    return;
}

static void test_taf_can_EnableRcvOwnMsg
(
)
{
    LE_TEST_INFO("Testing EnableRcvOwnMsg");
    le_result_t result;

    result = taf_can_EnableRcvOwnMsg(CanInfRef);
    LE_TEST_OK(result == LE_OK, "taf_can_EnableRcvOwnMsg - LE_OK");

    return;
}

static void test_taf_can_DisableRcvOwnMsg
(
)
{
    LE_TEST_INFO("Testing DisableRcvOwnMsg");
    le_result_t result;

    result = taf_can_DisableRcvOwnMsg(CanInfRef);
    LE_TEST_OK(result == LE_OK, "taf_can_DisableRcvOwnMsg - LE_OK");

    return;
}

static void test_VerifyFdFrame
(
)
{
    LE_TEST_INFO("Testing to verify FD frame");
    le_result_t result;
    bool fdEnableStatus;

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

static void test_taf_can_SetPayload
(
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
        }
        data[i] = (tmp << 4);

        if ((tmp = AscHexToDecimal(dataPtr[idx++])) > 0x0F)
        {
            LE_ERROR("data Formate Error");
        }
        data[i] |= tmp;
        dataLen++;
    }

    result = taf_can_SetPayload(FrameRef, data, dataLen);
    LE_TEST_OK(result == LE_OK, "taf_can_SetPayload - LE_OK");

    return;
}

static void test_taf_can_SetFrameType
(
    taf_can_FrameType_t frameType
)
{
    LE_TEST_INFO("Testing Set frame type");
    le_result_t result;

    result = taf_can_SetFrameType(FrameRef, frameType);
    LE_TEST_OK(result == LE_OK, "taf_can_SetFrameType - LE_OK");

    return;
}

/*
 * send CAN Frame
 */
static void test_taf_can_SendCanFrame
(
)
{
    LE_TEST_INFO("Testing TelAF Send CAN Frame - TAF_CAN_CAN_FRAME");

    le_result_t result;
    const char* dataPtr = "12345678abcdef09";
    int datalen = strlen(dataPtr);

    taf_can_FrameType_t frameType = TAF_CAN_CAN_FRAME;
    test_taf_can_SetFrameType(frameType);

    test_taf_can_SetPayload(dataPtr, datalen);

    result = taf_can_SendFrame(FrameRef);
    LE_TEST_OK(result == LE_OK, "taf_can_SendFrame - LE_OK");

    return;
}

/*
 * send CAN-FD Frame
 */
static void test_taf_can_SendCanFdFrame
(
)
{
    LE_TEST_INFO("Testing TelAF Send CAN Frame - TAF_CAN_CAN_FD_FRAME");

    le_result_t result;
    const char* dataPtr = "12345678abcdef09abcd";
    int datalen = strlen(dataPtr);

    if (taf_can_IsFdSupported(CanInfRef) == true)
    {
        taf_can_FrameType_t frameType = TAF_CAN_CAN_FD_FRAME;
        test_taf_can_SetFrameType(frameType);

        test_taf_can_SetPayload(dataPtr, datalen);

        result = taf_can_SendFrame(FrameRef);
        LE_TEST_OK(result == LE_OK, "taf_can_SendFrame - LE_OK");
    }
    else
    {
        LE_ERROR("Fd frame can not send because device does not support");
    }

    return;
}

/*
 * send CAN Auto Frame
 */
static void test_taf_can_SendAutoFrame
(
)
{
    LE_TEST_INFO("Testing TelAF Send CAN Frame - TAF_CAN_AUTO_FRAME");

    le_result_t result;
    const char* dataPtr = "12345678abcdef";
    int datalen = strlen(dataPtr);

    taf_can_FrameType_t frameType = TAF_CAN_AUTO_FRAME;
    test_taf_can_SetFrameType(frameType);

    test_taf_can_SetPayload(dataPtr, datalen);

    result = taf_can_SendFrame(FrameRef);
    LE_TEST_OK(result == LE_OK, "taf_can_SendFrame - LE_OK");

    return;
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

/*
 * Callback function
 */
static void TestTafCanCallback
(
    taf_can_CanInterfaceRef_t CanInfRef,
    bool isCanFdFrame,
    uint32_t frameId,
    const uint8_t* dataPtr,
    size_t size,
    void* contextPtr
)
{
    LE_INFO("Received frameType: %s, FrameId: 0x%03X, CAN Interface reference: %p\n",
            (isCanFdFrame ? "CAN FD frames":"CAN 2.0 frames"), frameId, CanInfRef);

    char rcvData[(2*size) + 1];
    le_hex_BinaryToString(dataPtr, size, rcvData, (2*size) + 1);
    LE_INFO("Received CAN data is %s", rcvData);

    le_sem_Post(SemRef);
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

static void test_taf_can_RemoveCanEventHandler
(
)
{
    LE_TEST_INFO(" Test Remove CAN event Handler for HandlerRef = %p", HandlerRef);
    taf_can_RemoveCanEventHandler(HandlerRef);

    return;
}

static void test_taf_can_DeleteCanFrame
(
)
{
    LE_TEST_INFO("Testing Delete CAN Frame reference");
    le_result_t result;

    result = taf_can_DeleteCanFrame(FrameRef);
    LE_TEST_OK(result == LE_OK, "taf_can_DeleteCanFrame - LE_OK");

    return;
}

static void test_taf_can_DeleteCanInf
(
)
{
    LE_TEST_INFO("Testing Delete CAN interface reference");
    le_result_t result;

    result = taf_can_DeleteCanInf(CanInfRef);
    LE_TEST_OK(result == LE_OK, "taf_can_DeleteCanInf - LE_OK");

    return;
}

static void* StartUnitTestThread
(
    void* contextPtr
)
{
    le_result_t result;

    // Test to Create CAN interface reference
    test_taf_can_CreateCanInf();

    // Test to Set Filter
    test_taf_can_SetFilter();

    // Test to Disable loopback
    test_taf_can_DisableLoopback();

    // Test to Enable loopback
    test_taf_can_EnableLoopback();

    // Test to DisableRcvOwnMsg
    test_taf_can_DisableRcvOwnMsg();

    // Test to EnableRcvOwnMsg
    test_taf_can_EnableRcvOwnMsg();

    // Test to verify FD frame
    test_VerifyFdFrame();

    // Test to Create CAN frame reference
    test_taf_can_CreateCanFrame();

    // Test to receive CAN message
    SemRef = le_sem_Create("SemRef", 0);
    le_thread_Ref_t threadRef = le_thread_Create("taf_Can_AddCanEventHandler",
            test_taf_can_AddCanEventHandler, NULL);
    le_thread_Start(threadRef);

    // Test to send CAN 2.0 Frame
    test_taf_can_SendCanFrame();
    result = WaitForSemTimeout(SemRef, 5);
    LE_ASSERT(result == LE_OK);

    // Test to send CAN-Fd Frame
    test_taf_can_SendCanFdFrame();
    result = WaitForSemTimeout(SemRef, 5);
    LE_ASSERT(result == LE_OK);

    // Test to send CAN auto Frame
    test_taf_can_SendAutoFrame();
    result = WaitForSemTimeout(SemRef, 5);
    LE_ASSERT(result == LE_OK);

    sleep(2);

    // Test to remove registered handler
    test_taf_can_RemoveCanEventHandler();

    // Test to delete created CAN Frame reference
    test_taf_can_DeleteCanFrame();

    // Test to delete created CAN interface reference
    test_taf_can_DeleteCanInf();

    LE_DEBUG("Test completed");

    return NULL;
}

COMPONENT_INIT
{
    StartUnitTestThread(NULL);
    exit(EXIT_SUCCESS);
}
