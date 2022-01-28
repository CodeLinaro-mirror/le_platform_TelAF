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

#include "tafGpio.hpp"

using namespace telux::tafsvc;

void tafGpio_InputMonitorHandlerFunc (int fd, short events)
{
    LE_DEBUG("tafGpio_InputMonitorHandlerFunc");
    auto &gpio = taf_Gpio::getInstance();
    gpio.inputMonitorHandlerFunc(fd, events);
}

le_result_t taf_gpio_SetInput (uint8_t pinNum, taf_gpio_Polarity_t polarity, bool lock)
{
    LE_DEBUG("taf_gpio_SetInput, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    return gpio.setGpioAsInput(gpio.tafGpioRefPin[pinNum], (taf_gpio_ActiveType_t)polarity, lock);
}

le_result_t taf_gpio_Activate (uint8_t pinNum, bool lock)
{
    LE_DEBUG("taf_gpio_Activate, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    return gpio.activate(gpio.tafGpioRefPin[pinNum], lock);
}

le_result_t taf_gpio_Deactivate (uint8_t pinNum, bool lock)
{
    LE_DEBUG("taf_gpio_Deactivate, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    return gpio.deactivate(gpio.tafGpioRefPin[pinNum], lock);
}

taf_gpio_State_t taf_gpio_Read (uint8_t pinNum, bool lock)
{
    LE_DEBUG("taf_gpio_Read, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    return gpio.readValue(gpio.tafGpioRefPin[pinNum], lock);
}

bool taf_gpio_IsActive (uint8_t pinNum)
{
    LE_DEBUG("taf_gpio_IsActive, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    return gpio.isActive(gpio.tafGpioRefPin[pinNum]);
}

bool taf_gpio_IsInput (uint8_t pinNum)
{
    LE_DEBUG("taf_gpio_IsInput, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    return gpio.isInput(gpio.tafGpioRefPin[pinNum]);
}

bool taf_gpio_IsOutput (uint8_t pinNum)
{
    LE_DEBUG("taf_gpio_IsOutput, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    return gpio.isOutput(gpio.tafGpioRefPin[pinNum]);
}

taf_gpio_Edge_t taf_gpio_GetEdgeSense (uint8_t pinNum)
{
    LE_DEBUG("taf_gpio_GetEdgeSense, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    return (taf_gpio_Edge_t)gpio.getEdgeSense( gpio.tafGpioRefPin[pinNum]);
}

taf_gpio_Polarity_t taf_gpio_GetPolarity (uint8_t pinNum)
{
    LE_DEBUG("taf_gpio_GetPolarity, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    return (taf_gpio_Polarity_t)gpio.getPolarity( gpio.tafGpioRefPin[pinNum]);
}

taf_gpio_ChangeEventHandlerRef_t taf_gpio_AddChangeEventHandler
(
    uint8_t pinNum,
    taf_gpio_Edge_t trigger,
    bool lock,
    taf_gpio_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_DEBUG("taf_gpio_AddChangeEventHandler, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    return (taf_gpio_ChangeEventHandlerRef_t)gpio.setChangeCallback(
                gpio.tafGpioRefPin[pinNum], tafGpio_InputMonitorHandlerFunc,
                trigger, lock, handlerPtr, contextPtr);
}

void taf_gpio_RemoveChangeEventHandler(taf_gpio_ChangeEventHandlerRef_t handlerRef)
{
    LE_DEBUG("taf_gpio_RemoveChangeEventHandler");
    auto &gpio = taf_Gpio::getInstance();
    gpio.removeChangeCallback(handlerRef);
}

le_result_t taf_gpio_SetEdgeSense (uint8_t pinNum, taf_gpio_Edge_t trigger, bool lock)
{
    LE_DEBUG("taf_gpio_SetEdgeSense, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    return gpio.setEdgeSense(gpio.tafGpioRefPin[pinNum], trigger, lock);
}

le_result_t taf_gpio_DisableEdgeSense (uint8_t pinNum, bool lock)
{
    LE_DEBUG("taf_gpio_DisableEdgeSense, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    return gpio.disableEdgeSense(gpio.tafGpioRefPin[pinNum], lock);
}

COMPONENT_INIT{
    auto &gpio = taf_Gpio::getInstance();
    gpio.Init();
    le_mem_PoolRef_t gpioRefPool = le_mem_CreatePool("gpioRefPool", sizeof(taf_gpio));
    for(int i = 0; i < MAX_PIN_NUMBER; i++) {
        gpio.tafGpioRefPin[i] = (taf_gpio*)le_mem_ForceAlloc(gpioRefPool);
        gpio.tafGpioRefPin[i]->pinNum = i;
        gpio.tafGpioRefPin[i]->fdMonitor = -1;
        snprintf(gpio.tafGpioRefPin[i]->gpioName, sizeof(gpio.tafGpioRefPin[i]->gpioName), "gpio%d", i);
        gpio.tafGpioRefPin[i]->isLocked = false;
        gpio.tafGpioRefPin[i]->handlerCount = 0;
        gpio.tafGpioRefPin[i]->fdMonitorRef = NULL;
        gpio.tafGpioRefPin[i]->lockedSession = NULL;
    }
}
