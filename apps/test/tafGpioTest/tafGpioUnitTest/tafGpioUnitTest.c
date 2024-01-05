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

#include "legato.h"
#include "interfaces.h"

static int pin61 = 61;
static int outPinNum = -1;
static int inPinNum = -1;
static taf_gpio_ChangeEventHandlerRef_t ref = NULL;
le_result_t res;
static void Test_taf_gpio_IsOutput(int pinNum) {
    bool res = taf_gpio_IsOutput(outPinNum);
    if (pinNum == outPinNum)
    {
        LE_TEST_INFO("Test taf_gpio_IsOutput for output PIN");
        LE_TEST_OK(res, "outpin direction is correct");
    }
    else if (pinNum == inPinNum)
    {
        LE_TEST_INFO("Test taf_gpio_IsOutput for input PIN");
        LE_TEST_OK(!res, "input direction is correct");
    }
}
static void Test_taf_gpio_Activate(int pinNum) {
    LE_TEST_INFO("Test_taf_gpio_Activate, for PIN %d sets the direction to OUT if its not out,"
           " and puts to active state", pinNum);
    res = taf_gpio_Activate(pinNum, true);
    LE_TEST_OK(res == LE_OK, "Activated the pin successfully");
    if(res == LE_OK) {
        LE_INFO("Gpio pin %d Activate Successful", pinNum);
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin %d Activate results in GPIO_BUSY", pinNum);
    } else if (res == LE_OUT_OF_RANGE) {
        LE_INFO("Gpio pin %d is out of range", pinNum);
    } else
        LE_INFO("Gpio pin %d Activate results in IO ERROR", pinNum);
    LE_INFO("======Test_taf_gpio_Activate done======");
}
static void Test_taf_gpio_Deactivate(int pinNum) {
    LE_TEST_INFO("Test_taf_gpio_Deactivate for PIN %d %s", pinNum,
            (pinNum == outPinNum ? "outPin" : "inPin"));
    res = taf_gpio_Deactivate(pinNum, true);
    LE_TEST_OK(res == LE_OK, "Deactivated successfully %s",
            (pinNum == outPinNum ? "outPin" : "inPin"));
    if(res == LE_OK) {
        LE_INFO("Gpio pin %d Deactivate Successful", pinNum);
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin %d Deactivate results in GPIO_BUSY", pinNum);
    } else if (res == LE_OUT_OF_RANGE) {
        LE_INFO("Gpio pin %d is out of range", pinNum);
    }  else
        LE_INFO("Gpio pin %d Deactivate results in IO ERROR", pinNum);
    LE_INFO("Test_taf_gpio_Deactivate done");
}
static void Test_taf_gpio_SetInput(int pinNum, bool isActiveLow) {
    if(isActiveLow)
    {
        LE_TEST_INFO("taf_gpio_SetInput %s with ACTIVE_LOW",
                (pinNum == outPinNum ? "outPin" : "inPin"));
        res = taf_gpio_SetInput(inPinNum, TAF_GPIO_ACTIVE_LOW, false);
        LE_TEST_OK(res == LE_OK, "Successfully changed %s pin to input with ACTIVE_LOW",
                (pinNum == outPinNum ? "outPin" : "inPin"));
    } else {
        LE_TEST_INFO("taf_gpio_SetInput %s with ACTIVE_HIGH",
                pinNum == outPinNum ? "outPin" : "inPin");
        res = taf_gpio_SetInput(inPinNum, TAF_GPIO_ACTIVE_HIGH, false);
        LE_TEST_OK(res == LE_OK, "Successfully changed %s pin to input with ACTIVE_HIGH",
                (pinNum == outPinNum ? "outPin" : "inPin"));
    }
    if(res == LE_OK) {
        LE_INFO("Gpio pin %d SetInput Successful", pinNum);
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin %d SetInput results in GPIO_BUSY", pinNum);
    } else if (res == LE_OUT_OF_RANGE) {
        LE_INFO("Gpio pin %d is out of range", pinNum);
    }  else
        LE_INFO("Gpio pin %d SetInput results in IO ERROR", pinNum);
    LE_INFO("Test_taf_gpio_SetInput done");
}
static void Test_taf_gpio_IsInput(int pinNum) {
    LE_TEST_INFO("Test taf_gpio_IsInput for %s", pinNum == outPinNum ? "outPin" : "inPin");
    bool res = taf_gpio_IsInput(pinNum);
    if (pinNum == outPinNum)
    {
        LE_TEST_INFO("Test taf_gpio_IsInput for outPin");
        LE_TEST_OK(!res, "taf_gpio_IsInput tested for outPin successfull");
    }
    else if (pinNum == inPinNum)
    {
        LE_TEST_INFO("Test taf_gpio_IsInput for inPin");
        LE_TEST_OK(res, "taf_gpio_IsInput tested for inPin successfull");
    }
    LE_INFO("Test_taf_gpio_IsInput done");
}
static void Test_taf_gpio_Read(int pinNum) {
    taf_gpio_State_t state = taf_gpio_Read(pinNum, false);
    if (pinNum == inPinNum)
    {
        LE_TEST_INFO("Test taf_gpio_Read on inPin");
        LE_TEST_OK(state != TAF_GPIO_BUSY, "Successfully tested taf_gpio_Read on inPin");
    }
    else if (pinNum == outPinNum)
    {
        LE_TEST_INFO("Test taf_gpio_Read on outPin");
        LE_TEST_OK(state == TAF_GPIO_BUSY, "Successfully tested taf_gpio_Read on outPin");
    }
    if (state == TAF_GPIO_HIGH) {
        LE_INFO("Test_taf_gpio_Read, value of PIN %d is 1", inPinNum);
    } else if (state == TAF_GPIO_LOW) {
        LE_INFO("Test_taf_gpio_Read, value of PIN %d is 0", inPinNum);
    } else if (state == TAF_GPIO_BUSY) {
        LE_INFO("Test_taf_gpio_Read, GPIO PIN %d is BUSY", inPinNum);
    }
    LE_INFO("Test_taf_gpio_Read done");
}
static void Test_taf_gpio_SetEdgeSense() {
    LE_TEST_INFO("Test taf_gpio_SetEdgeSense to rising");
    res = taf_gpio_SetEdgeSense(inPinNum, TAF_GPIO_EDGE_RISING, false);
    if(res == LE_OK) {
        LE_TEST_OK(true, "Gpio pin %d SetEdgeSense Successful", inPinNum);
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin %d SetEdgeSense results in GPIO_BUSY", inPinNum);
    } else if (res == LE_OUT_OF_RANGE) {
        LE_INFO("Gpio pin %d is out of range", outPinNum);
    }  else
        LE_INFO("Gpio pin %d SetEdgeSense results in IO ERROR", inPinNum);
}
static void Test_taf_gpio_DisableEdgeSense() {
    LE_TEST_INFO("Test taf_gpio_DisableEdgeSense for PIN %d", inPinNum);
    res = taf_gpio_DisableEdgeSense(inPinNum, false);
    if(res == LE_OK) {
        LE_TEST_OK(true, "Gpio pin %d DisableEdgeSense Successful", inPinNum);
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin %d DisableEdgeSense results in GPIO_BUSY", inPinNum);
    } else if (res == LE_OUT_OF_RANGE) {
        LE_INFO("Gpio pin %d is out of range", outPinNum);
    }  else
        LE_INFO("Gpio pin %d DisableEdgeSense results in IO ERROR", inPinNum);
    LE_INFO("Test_taf_gpio_DisableEdgeSense done");
}
static void GpioChangeCallback(uint8_t pinNum, bool state, void *ctx){
    LE_INFO("State change %s pinNum %d", state?"TRUE":"FALSE", pinNum);
    LE_INFO("Context pointer came back as %d", *(int *)ctx);
}
static void test_taf_gpio_ChangeCallback() {
    LE_INFO("Test_taf_gpio_ChangeCallback for PIN %d", inPinNum);

    LE_TEST_INFO("Test taf_gpio_AddChangeEventHandler with NULL Reference");
    ref = taf_gpio_AddChangeEventHandler(inPinNum, TAF_GPIO_EDGE_BOTH, false,
            NULL, NULL);
    LE_TEST_OK(ref != NULL, "Test taf_gpio_AddChangeEventHandler with NULL reference successfull");

    // Remove handlerRef created with NULL callback ref
    taf_gpio_RemoveChangeEventHandler(ref);

    LE_TEST_INFO("Test taf_gpio_AddChangeEventHandler with valid Reference");
    ref = taf_gpio_AddChangeEventHandler(inPinNum, TAF_GPIO_EDGE_BOTH, false,
            GpioChangeCallback, &pin61);
    LE_TEST_OK(ref != NULL,"Test taf_gpio_AddChangeEventHandler with valid reference successfull");
}
static void* Test_taf_gpio_ChangeCallback(void* ctxPtr) {
    LE_INFO("Test_taf_gpio_ChangeCallback for PIN %d", inPinNum);
    taf_gpio_ConnectService();
    ref = taf_gpio_AddChangeEventHandler(inPinNum, TAF_GPIO_EDGE_BOTH, false,
            GpioChangeCallback, &pin61);
    if (ref != NULL) {
        LE_INFO("registered successfully for gpio pin trigger");
    } else {
        LE_INFO("Couldn't register for gpio pin trigger");
    }
    le_event_RunLoop();
}
static void Test_taf_gpio_RemoveCallback() {
    if (ref != NULL)
    {
        LE_INFO("Test taf_gpio_RemoveCallback for PIN %d", inPinNum);
        taf_gpio_RemoveChangeEventHandler(ref);
        LE_TEST_OK(true, "Test_taf_gpio_RemoveCallback done");
    }
}
static void Test_taf_gpio_GetName(int pinNum) {
    LE_INFO("Test Test_taf_gpio_GetName for PIN %d", pinNum);
    char pinName[TAF_GPIO_PIN_NAME_MAX_BYTE] = {};
    le_result_t res = taf_gpio_GetName(pinNum, pinName, TAF_GPIO_PIN_NAME_MAX_BYTE);
    if(res == LE_OK)
    {
        LE_INFO("Test taf_gpio_GetName for PIN %d, name = %s", pinNum, pinName);
    }
    else if(res == LE_NOT_IMPLEMENTED)
    {
        LE_INFO("Test taf_gpio_GetName for PIN %d, not implemented", pinNum);
    }

    LE_TEST_OK(true, "Test_taf_gpio_GetName done");
}
static void Test_ExtremeValues(int pinNum)
{
    LE_INFO("================ Test_ExtremeValues ================");
    LE_TEST_INFO("Test taf_gpio_SetInput with %d", pinNum);
    le_result_t res = taf_gpio_SetInput(pinNum, TAF_GPIO_ACTIVE_HIGH, true);
    LE_TEST_OK(res == LE_OUT_OF_RANGE, "Test taf_gpio_SetInput with %d successfull", pinNum);

    LE_TEST_INFO("Test taf_gpio_Activate with %d", pinNum);
    res = taf_gpio_Activate(pinNum, true);
    LE_TEST_OK(res == LE_OUT_OF_RANGE, "Test taf_gpio_Activate with %d successfull", pinNum);

    LE_TEST_INFO("Test taf_gpio_Deactivate with %d", pinNum);
    res = taf_gpio_Deactivate(pinNum, true);
    LE_TEST_OK(res == LE_OUT_OF_RANGE, "Test taf_gpio_Deactivate with %d successfull", pinNum);

    LE_TEST_INFO("Test taf_gpio_Read with %d", pinNum);
    taf_gpio_State_t state = taf_gpio_Read(pinNum, true);
    LE_TEST_OK(state == TAF_GPIO_BUSY, "Test taf_gpio_Read with %d successfull", pinNum);

    LE_TEST_INFO("Test taf_gpio_IsActive with %d", pinNum);
    bool isActive = taf_gpio_IsActive(pinNum);
    LE_TEST_OK(!isActive, "Test taf_gpio_IsActive with %d successfull", pinNum);

    LE_TEST_INFO("Test taf_gpio_IsInput with %d", pinNum);
    bool isInput = taf_gpio_IsInput(pinNum);
    LE_TEST_OK(!isInput, "Test taf_gpio_IsInput with %d successfull", pinNum);

    LE_TEST_INFO("Test taf_gpio_IsOutput with %d", pinNum);
    bool isOutput = taf_gpio_IsOutput(pinNum);
    LE_TEST_OK(!isOutput, "Test taf_gpio_IsOutput with %d successfull", pinNum);

    LE_TEST_INFO("Test taf_gpio_GetEdgeSense with %d", pinNum);
    taf_gpio_Edge_t edge = taf_gpio_GetEdgeSense(pinNum);
    LE_TEST_OK(edge == TAF_GPIO_EDGE_UNKNOWN,"Test taf_gpio_IsOutput with %d successfull", pinNum);

    LE_TEST_INFO("Test taf_gpio_GetPolarity with %d", pinNum);
    taf_gpio_Polarity_t polarity = taf_gpio_GetPolarity(pinNum);
    LE_TEST_OK(polarity == -1,
            "Test taf_gpio_GetPolarity with %d successfull %d ", pinNum, polarity);

    LE_TEST_INFO("Test taf_gpio_AddChangeEventHandler with %d", pinNum);
    ref = taf_gpio_AddChangeEventHandler(pinNum, TAF_GPIO_EDGE_BOTH, false,
            GpioChangeCallback, &pin61);
    LE_TEST_OK(ref == NULL, "Test taf_gpio_AddChangeEventHandler with %d successfull", pinNum);

    LE_TEST_INFO("Test taf_gpio_SetEdgeSense with %d", pinNum);
    res = taf_gpio_SetEdgeSense(pinNum, TAF_GPIO_ACTIVE_HIGH, true);
    LE_TEST_OK(res == LE_OUT_OF_RANGE, "Test taf_gpio_SetEdgeSense with %d successfull", pinNum);

    LE_TEST_INFO("Test taf_gpio_DisableEdgeSense with %d", pinNum);
    res = taf_gpio_DisableEdgeSense(pinNum, true);
    LE_TEST_OK(res == LE_OUT_OF_RANGE, "Test taf_gpio_DisableEdgeSense with %d successfull", pinNum);
}
static void Test_gpio
(
    void
)
{
    LE_INFO("====== Start GPIO test ======");

    LE_INFO("====== Test IsOutput ======");
    if(outPinNum != -1 && outPinNum >=0)
    {

        LE_INFO("====== Test Activate ======");
        Test_taf_gpio_Activate(outPinNum);
        Test_taf_gpio_IsOutput(outPinNum);

        LE_INFO("====== Test IsActive ======");

        bool res;
        LE_TEST_INFO("Test_taf_gpio_IsActive after Activate outpin");
        res = taf_gpio_IsActive(outPinNum);
        LE_TEST_OK(res, "Test_taf_gpio_IsActive successfully after Activate");

        LE_INFO("====== Test Deactivate ======");
        Test_taf_gpio_Deactivate(outPinNum);

        LE_TEST_INFO("Test_taf_gpio_IsActive after Deactivate outpin");
        res = taf_gpio_IsActive(outPinNum);
        LE_TEST_OK(!res, "Test_taf_gpio_IsActive successfully after Deactivate");

        LE_INFO("====== Test Activate ======");
        Test_taf_gpio_Activate(outPinNum);

        LE_INFO("====== Test IsActive ======");

        LE_TEST_INFO("Test_taf_gpio_IsActive after Activate outpin");
        res = taf_gpio_IsActive(outPinNum);
        LE_TEST_OK(res, "Test_taf_gpio_IsActive successfully after Activate");

        Test_taf_gpio_IsInput(outPinNum);

        Test_taf_gpio_Read(outPinNum);

        LE_INFO("====== Test change output to input pin ======");
        Test_taf_gpio_SetInput(outPinNum, false);

        LE_INFO("====== Test GetPolarity ======");

        LE_TEST_INFO("Test taf_gpio_GetPolarity for outPin after setting ACTIVE_HIGH");
        taf_gpio_Polarity_t polarityType = taf_gpio_GetPolarity(outPinNum);
        LE_TEST_OK(polarityType == TAF_GPIO_ACTIVE_HIGH, "Test_taf_gpio_GetPolarity done");

        LE_INFO("====== Test get name for output pin ======");
        Test_taf_gpio_GetName(outPinNum);
    }
    else
    {
        printf("\nPlease enter proper Output Gpio pin num\n");
        exit(EXIT_FAILURE);
    }

    if(inPinNum != -1 && inPinNum >= 0)
    {
        LE_INFO("====== Test SetInput ======");
        Test_taf_gpio_SetInput(inPinNum, false);

        LE_TEST_INFO("Test taf_gpio_GetPolarity for inPin after setting ACTIVE_HIGH");
        taf_gpio_Polarity_t polarityType = taf_gpio_GetPolarity(inPinNum);
        LE_TEST_OK(polarityType == TAF_GPIO_ACTIVE_HIGH, "Test_taf_gpio_GetPolarity done");

        Test_taf_gpio_SetInput(inPinNum, true);

        LE_TEST_INFO("Test_taf_gpio_IsActive on inPin");
        res = taf_gpio_IsActive(inPinNum);
        LE_TEST_OK(!res, "Test_taf_gpio_IsActive successfull on inPin");

        LE_TEST_INFO("Test taf_gpio_GetPolarity for inPin after setting ACTIVE_LOW");
        polarityType = taf_gpio_GetPolarity(inPinNum);
        LE_TEST_OK(polarityType == TAF_GPIO_ACTIVE_LOW, "Test_taf_gpio_GetPolarity done");

        Test_taf_gpio_IsInput(inPinNum);

        LE_INFO("====== Test Read ======");
        Test_taf_gpio_Read(inPinNum);

        LE_INFO("====== Test ChangeCallback ======");
        test_taf_gpio_ChangeCallback();

        LE_INFO("====== Test SetEdgeSense and GetEdgeSence ======");
        Test_taf_gpio_SetEdgeSense();

        LE_TEST_INFO("Test taf_gpio_GetEdgeSense");
        taf_gpio_Edge_t edge = taf_gpio_GetEdgeSense(inPinNum);
        LE_TEST_OK(edge == TAF_GPIO_EDGE_RISING, "taf_gpio_GetEdgeSense successfull");

        LE_INFO("====== Test DisableEdgeSense ======");
        Test_taf_gpio_DisableEdgeSense();

        LE_TEST_INFO("Test taf_gpio_GetEdgeSense");
        edge = taf_gpio_GetEdgeSense(inPinNum);
        LE_TEST_OK(edge == TAF_GPIO_EDGE_NONE, "taf_gpio_GetEdgeSense successfull");

        Test_taf_gpio_SetEdgeSense();

        LE_INFO("====== Test Remove callback ======");
        Test_taf_gpio_RemoveCallback();

        LE_INFO("====== Test Activate and deactivate an Input pin ======");
        Test_taf_gpio_Deactivate(inPinNum);

        LE_TEST_INFO("Test_taf_gpio_IsActive after Deactivate inPin");
        res = taf_gpio_IsActive(inPinNum);
        LE_TEST_OK(!res, "Test_taf_gpio_IsActive successfully Deactivate inPin");

        Test_taf_gpio_Activate(inPinNum);

        LE_INFO("====== Test ChangeCallback ======");
        Test_taf_gpio_SetInput(inPinNum, false);
        le_thread_Ref_t threadRef = le_thread_Create("taf_GPIO_StateHandler",
                Test_taf_gpio_ChangeCallback, NULL);
        le_thread_Start(threadRef);

        LE_INFO("====== Test get name for input pin ======");
        Test_taf_gpio_GetName(inPinNum);
    }
    else
    {
        printf("\nPlease enter proper Input Gpio pin num\n");
        exit(EXIT_FAILURE);
    }
    Test_ExtremeValues(200);
}

static void DisplayUsage()
{
    printf("Usage of tafGpioUnitTest:");
    printf("NOTE: outputPinNum and inputPinNum should be different");
    printf("\napp runProc tafGpioUnitTest --exe=tafGpioUnitTest -- <outputPinNum>"
            " <inputPinNum> \n");
    printf("Usage of tafGpioUnitTest for automation testing:");
    printf("\napp runProc tafGpioUnitTest --exe=tafGpioUnitTest -- <outputPinNum>"
            " <inputPinNum> true\n");
    exit(EXIT_FAILURE);
}

COMPONENT_INIT
{
    LE_INFO("====== Start GPIO test ======");
    int NumberOfArgs = le_arg_NumArgs();
    const char *arg = NULL;
    if(NumberOfArgs >= 1)
    {
        arg = le_arg_GetArg(0);
        if(arg == NULL)
        {
            LE_ERROR("GPIO test: NULL argument received, Line %d", __LINE__);
            exit(EXIT_FAILURE);
        }
        outPinNum = atoi(arg);
    }
    if(NumberOfArgs >= 2)
    {
        arg = le_arg_GetArg(1);
        if(arg == NULL)
        {
            LE_ERROR("GPIO test: NULL argument received, Line %d", __LINE__);
            exit(EXIT_FAILURE);
        }
        inPinNum = atoi(arg);
        if (outPinNum == inPinNum)
        {
            DisplayUsage();
        }
    }

    if(NumberOfArgs >= 1)
    {
        arg = le_arg_GetArg(0);
        if(arg == NULL)
        {
            LE_ERROR("GPIO test: NULL argument received, Line %d", __LINE__);
            exit(EXIT_FAILURE);
        }
        if(strcmp(arg, "help") == 0)
        {
            DisplayUsage();
        }
        Test_gpio();
    }

    if(NumberOfArgs == 3) {
        arg = le_arg_GetArg(2);
        if(arg == NULL)
        {
            LE_ERROR("GPIO test: NULL argument received, Line %d", __LINE__);
            exit(EXIT_FAILURE);
        }
        if(strcmp(arg, "true") == 0)
        {
            LE_TEST_EXIT;
        }
    }
}
