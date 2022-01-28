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
static taf_gpio_ChangeEventHandlerRef_t ref;
le_result_t res;
static void Test_taf_gpio_IsOutput() {
    LE_INFO("Test_taf_gpio_IsOutput for PIN 42 is %s", taf_gpio_IsOutput(42) ? "TRUE" : "FALSE");
    LE_INFO("Test_taf_gpio_IsOutput done");
}
static void Test_taf_gpio_Activate() {
    LE_INFO("Test_taf_gpio_Activate, for PIN 42 sets the direction to OUT if its not out, and puts to active state");
    res = taf_gpio_Activate(42, false);
    if(res == LE_OK) {
        LE_INFO("Gpio pin 42 Activate Successful");
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin 42 Activate results in GPIO_BUSY");
    } else
        LE_INFO("Gpio pin 42 Activate results in IO ERROR");
    LE_INFO("======Test_taf_gpio_Activate done======");
}
static void Test_taf_gpio_IsActive() {
    LE_INFO("Test_taf_gpio_IsActive for PIN 42");
    LE_INFO("Test_taf_gpio_IsActive is %s", taf_gpio_IsActive(42) ? "TRUE" : "FALSE");
    LE_INFO("Test_taf_gpio_IsActive done");
}
static void Test_taf_gpio_Deactivate() {
    LE_INFO("Test_taf_gpio_Deactivate for PIN 42");
    res = taf_gpio_Deactivate(42, false);
    if(res == LE_OK) {
        LE_INFO("Gpio pin 42 Deactivate Successful");
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin 42 Deactivate results in GPIO_BUSY");
    } else
        LE_INFO("Gpio pin 42 Deactivate results in IO ERROR");
    LE_INFO("Test_taf_gpio_Deactivate done");
}
static void Test_taf_gpio_GetPolarity() {
    LE_INFO("Test_taf_gpio_GetPolarity for PIN 42 %s",(taf_gpio_GetPolarity(42) == TAF_GPIO_ACTIVE_HIGH) ? "Active High" : "Active Low");
    LE_INFO("Test_taf_gpio_GetPolarity done");
}
static void Test_taf_gpio_SetInput() {
    LE_INFO("Test_taf_gpio_SetInput for PIN 61 with Active High polarity");
    res = taf_gpio_SetInput(61, TAF_GPIO_ACTIVE_HIGH, false);
    if(res == LE_OK) {
        LE_INFO("Gpio pin 61 SetInput Successful");
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin 61 SetInput results in GPIO_BUSY");
    } else
        LE_INFO("Gpio pin 61 SetInput results in IO ERROR");
    LE_INFO("Test_taf_gpio_SetInput done");
}
static void Test_taf_gpio_IsInput() {
    LE_INFO("Test_taf_gpio_IsInput for PIN 61 is %s", taf_gpio_IsInput(61) ? "TRUE" : "FALSE");
    LE_INFO("Test_taf_gpio_IsInput done");
}
static void Test_taf_gpio_Read() {
    taf_gpio_State_t state = taf_gpio_Read(61, false);
    if (state == TAF_GPIO_HIGH) {
        LE_INFO("Test_taf_gpio_Read, value of PIN 61 is 1");
    } else if (state == TAF_GPIO_LOW) {
        LE_INFO("Test_taf_gpio_Read, value of PIN 61 is 0");
    } else if (state == TAF_GPIO_BUSY) {
        LE_INFO("Test_taf_gpio_Read, GPIO PIN 61 is BUSY");
    }
    LE_INFO("Test_taf_gpio_Read done");
}
static void Test_taf_gpio_SetEdgeSense() {
    LE_INFO("Test_taf_gpio_SetEdgeSense both for PIN 61");
    res = taf_gpio_SetEdgeSense(61, TAF_GPIO_EDGE_BOTH, false);
    if(res == LE_OK) {
        LE_INFO("Gpio pin 61 SetEdgeSense Successful");
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin 61 SetEdgeSense results in GPIO_BUSY");
    } else
        LE_INFO("Gpio pin 61 SetEdgeSense results in IO ERROR");
    LE_INFO("Test_taf_gpio_SetEdgeSense done");
}
static void Test_taf_gpio_GetEdgeSense() {
    LE_INFO("Test_taf_gpio_GetEdgeSense for PIN 61");
    taf_gpio_Edge_t edge = taf_gpio_GetEdgeSense(61);
    if (edge == TAF_GPIO_EDGE_FALLING)
    {
        LE_INFO("Pin 61 edge sense = falling");
    }
    else if (edge == TAF_GPIO_EDGE_RISING)
    {
        LE_INFO("Pin 61 edge sense = rising");
    }
    else if (edge == TAF_GPIO_EDGE_BOTH)
    {
        LE_INFO("Pin 61 edge sense = both");
    }
    else if (edge == TAF_GPIO_EDGE_NONE)
    {
        LE_INFO("Pin 61 edge sense = none");
    }
    LE_INFO("Test_taf_gpio_GetEdgeSense done");
}
static void Test_taf_gpio_DisableEdgeSense() {
    LE_INFO("Test_taf_gpio_DisableEdgeSense for PIN 61");
    res = taf_gpio_DisableEdgeSense(61, false);
    if(res == LE_OK) {
        LE_INFO("Gpio pin 61 DisableEdgeSense Successful");
    } else if (res == LE_BUSY) {
        LE_INFO("Gpio pin 61 DisableEdgeSense results in GPIO_BUSY");
    } else
        LE_INFO("Gpio pin 61 DisableEdgeSense results in IO ERROR");
    LE_INFO("Test_taf_gpio_DisableEdgeSense done");
}
static void GpioChangeCallback(uint8_t pinNum, bool state, void *ctx){
    LE_INFO("State change %s pinNum %d", state?"TRUE":"FALSE", pinNum);
    LE_INFO("Context pointer came back as %d", *(int *)ctx);
}
static void Test_taf_gpio_ChangeCallback() {
    LE_INFO("Test_taf_gpio_ChangeCallback for PIN 61");
    ref = taf_gpio_AddChangeEventHandler(61, TAF_GPIO_EDGE_BOTH, false, GpioChangeCallback, &pin61);
    if (ref != NULL) {
        LE_INFO("registered successfully for gpio pin trigger");
    } else {
        LE_INFO("Couldn't register for gpio pin trigger");
    }
}
static void Test_taf_gpio_RemoveCallback() {
    if (ref != NULL) {
    LE_INFO("Test_taf_gpio_RemoveCallback for PIN 61");
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

COMPONENT_INIT
{
    LE_INFO("====== Start GPIO test ======");
    Test_gpio();
}
