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
static taf_gpio_ChangeEventHandlerRef_t ref;
le_result_t res;
static void Test_taf_gpio_IsOutput() {
    LE_INFO("Test_taf_gpio_IsOutput for PIN %d is %s", outPinNum,
            taf_gpio_IsOutput(outPinNum) ? "TRUE" : "FALSE");
    LE_INFO("Test_taf_gpio_IsOutput done");
}
static void Test_taf_gpio_Activate() {
    LE_INFO("Test_taf_gpio_Activate, for PIN %d sets the direction to OUT if its not out,"
           " and puts to active state", outPinNum);
    res = taf_gpio_Activate(outPinNum, false);
    if(res == LE_OK) {
        LE_INFO("Gpio pin %d Activate Successful", outPinNum);
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin %d Activate results in GPIO_BUSY", outPinNum);
    } else if (res == LE_OUT_OF_RANGE) {
        LE_INFO("Gpio pin %d is out of range", outPinNum);
    } else
        LE_INFO("Gpio pin %d Activate results in IO ERROR", outPinNum);
    LE_INFO("======Test_taf_gpio_Activate done======");
}
static void Test_taf_gpio_IsActive() {
    LE_INFO("Test_taf_gpio_IsActive for PIN %d", outPinNum);
    LE_INFO("Test_taf_gpio_IsActive is %s", taf_gpio_IsActive(outPinNum) ? "TRUE" : "FALSE");
    LE_INFO("Test_taf_gpio_IsActive done");
}
static void Test_taf_gpio_Deactivate() {
    LE_INFO("Test_taf_gpio_Deactivate for PIN %d", outPinNum);
    res = taf_gpio_Deactivate(outPinNum, false);
    if(res == LE_OK) {
        LE_INFO("Gpio pin %d Deactivate Successful", outPinNum);
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin %d Deactivate results in GPIO_BUSY", outPinNum);
    } else if (res == LE_OUT_OF_RANGE) {
        LE_INFO("Gpio pin %d is out of range", outPinNum);
    }  else
        LE_INFO("Gpio pin %d Deactivate results in IO ERROR", outPinNum);
    LE_INFO("Test_taf_gpio_Deactivate done");
}
static void Test_taf_gpio_GetPolarity() {
    LE_INFO("Test_taf_gpio_GetPolarity for PIN %d %s",outPinNum, (taf_gpio_GetPolarity(outPinNum)
            == TAF_GPIO_ACTIVE_HIGH) ? "Active High" : "Active Low");
    LE_INFO("Test_taf_gpio_GetPolarity done");
}
static void Test_taf_gpio_SetInput() {
    LE_INFO("Test_taf_gpio_SetInput for PIN %d with Active High polarity", inPinNum);
    res = taf_gpio_SetInput(inPinNum, TAF_GPIO_ACTIVE_HIGH, false);
    if(res == LE_OK) {
        LE_INFO("Gpio pin %d SetInput Successful", inPinNum);
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin %d SetInput results in GPIO_BUSY", inPinNum);
    } else if (res == LE_OUT_OF_RANGE) {
        LE_INFO("Gpio pin %d is out of range", outPinNum);
    }  else
        LE_INFO("Gpio pin %d SetInput results in IO ERROR", inPinNum);
    LE_INFO("Test_taf_gpio_SetInput done");
}
static void Test_taf_gpio_IsInput() {
    LE_INFO("Test_taf_gpio_IsInput for PIN %d is %s", inPinNum,
            taf_gpio_IsInput(inPinNum) ? "TRUE" : "FALSE");
    LE_INFO("Test_taf_gpio_IsInput done");
}
static void Test_taf_gpio_Read() {
    taf_gpio_State_t state = taf_gpio_Read(inPinNum, false);
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
    LE_INFO("Test_taf_gpio_SetEdgeSense both for PIN %d", inPinNum);
    res = taf_gpio_SetEdgeSense(inPinNum, TAF_GPIO_EDGE_BOTH, false);
    if(res == LE_OK) {
        LE_INFO("Gpio pin %d SetEdgeSense Successful", inPinNum);
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin %d SetEdgeSense results in GPIO_BUSY", inPinNum);
    } else if (res == LE_OUT_OF_RANGE) {
        LE_INFO("Gpio pin %d is out of range", outPinNum);
    }  else
        LE_INFO("Gpio pin %d SetEdgeSense results in IO ERROR", inPinNum);
    LE_INFO("Test_taf_gpio_SetEdgeSense done");
}
static void Test_taf_gpio_GetEdgeSense() {
    LE_INFO("Test_taf_gpio_GetEdgeSense for PIN %d", inPinNum);
    taf_gpio_Edge_t edge = taf_gpio_GetEdgeSense(inPinNum);
    if (edge == TAF_GPIO_EDGE_FALLING)
    {
        LE_INFO("Pin %d edge sense = falling", inPinNum);
    }
    else if (edge == TAF_GPIO_EDGE_RISING)
    {
        LE_INFO("Pin %d edge sense = rising", inPinNum);
    }
    else if (edge == TAF_GPIO_EDGE_BOTH)
    {
        LE_INFO("Pin %d edge sense = both", inPinNum);
    }
    else if (edge == TAF_GPIO_EDGE_NONE)
    {
        LE_INFO("Pin %d edge sense = none", inPinNum);
    }
    LE_INFO("Test_taf_gpio_GetEdgeSense done");
}
static void Test_taf_gpio_DisableEdgeSense() {
    LE_INFO("Test_taf_gpio_DisableEdgeSense for PIN %d", inPinNum);
    res = taf_gpio_DisableEdgeSense(inPinNum, false);
    if(res == LE_OK) {
        LE_INFO("Gpio pin %d DisableEdgeSense Successful", inPinNum);
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
static void Test_taf_gpio_ChangeCallback() {
    LE_INFO("Test_taf_gpio_ChangeCallback for PIN %d", inPinNum);
    ref = taf_gpio_AddChangeEventHandler(inPinNum, TAF_GPIO_EDGE_BOTH, false,
            GpioChangeCallback, &pin61);
    if (ref != NULL) {
        LE_INFO("registered successfully for gpio pin trigger");
    } else {
        LE_INFO("Couldn't register for gpio pin trigger");
    }
}
static void Test_taf_gpio_RemoveCallback() {
    if (ref != NULL) {
    LE_INFO("Test_taf_gpio_RemoveCallback for PIN %d", inPinNum);
    taf_gpio_RemoveChangeEventHandler(ref);
    LE_INFO("Test_taf_gpio_RemoveCallback done");
    } else
        LE_INFO("Couldn't test taf_gpio_RemoveChangeEventHandler as ref is NULL");
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
        Test_taf_gpio_IsOutput();

        LE_INFO("====== Test Activate ======");
        Test_taf_gpio_Activate();

        LE_INFO("====== Test IsActive ======");
        Test_taf_gpio_IsActive();

        LE_INFO("====== Test Deactivate ======");
        Test_taf_gpio_Deactivate();
        Test_taf_gpio_IsActive();

        LE_INFO("====== Test GetPolarity ======");
        Test_taf_gpio_GetPolarity();
    }
    else
    {
        printf("\nPlease enter proper Output Gpio pin num\n");
        exit(EXIT_FAILURE);
    }

    if(inPinNum != -1 && inPinNum >= 0)
    {
        LE_INFO("====== Test SetInput ======");
        Test_taf_gpio_SetInput();
        Test_taf_gpio_GetPolarity();
        Test_taf_gpio_IsInput();

        LE_INFO("====== Test Read ======");
        Test_taf_gpio_Read();

        LE_INFO("====== Test ChangeCallback ======");
        Test_taf_gpio_ChangeCallback();

        LE_INFO("====== Test SetEdgeSense and GetEdgeSence ======");
        Test_taf_gpio_SetEdgeSense();
        Test_taf_gpio_GetEdgeSense();

        LE_INFO("====== Test DisableEdgeSense ======");
        Test_taf_gpio_DisableEdgeSense();
        Test_taf_gpio_GetEdgeSense();
        Test_taf_gpio_SetEdgeSense();

        LE_INFO("====== Test Remove callback ======");
        Test_taf_gpio_RemoveCallback();

        LE_INFO("====== Test ChangeCallback ======");
        Test_taf_gpio_ChangeCallback();
    }
    else
    {
        printf("\nPlease enter proper Input Gpio pin num\n");
        exit(EXIT_FAILURE);
    }
}

COMPONENT_INIT
{
    LE_INFO("====== Start GPIO test ======");
    int NumberOfArgs = le_arg_NumArgs();
    if(NumberOfArgs >= 1)
    {
        outPinNum = atoi(le_arg_GetArg(0));
    }
    if(NumberOfArgs >= 2)
    {
        inPinNum = atoi(le_arg_GetArg(1));
    }
    if(NumberOfArgs >= 1)
    {
        const char* arg = "";
        arg = le_arg_GetArg(0);
        if(strcmp(arg,"help") == 0)
        {
            printf("Usage of tafGpioUnitTest:");
            printf("\napp runProc tafGpioUnitTest --exe=tafGpioUnitTest -- <outputPinNum>"
                    " <inputPinNum>\n");
            exit(EXIT_FAILURE);
        }
    }
    Test_gpio();
}
