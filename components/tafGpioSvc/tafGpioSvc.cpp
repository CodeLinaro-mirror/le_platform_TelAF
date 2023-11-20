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
    TAF_ERROR_IF_RET_VAL(pinNum < 0 || pinNum >= gpio.numOfGpios, LE_OUT_OF_RANGE,
            "Gpio pin %d is not available", pinNum);
    return gpio.setGpioAsInput(gpio.tafGpioRefPin[pinNum], (taf_gpio_ActiveType_t)polarity, lock);
}

le_result_t taf_gpio_Activate (uint8_t pinNum, bool lock)
{
    LE_DEBUG("taf_gpio_Activate, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    TAF_ERROR_IF_RET_VAL(pinNum < 0 || pinNum >= gpio.numOfGpios, LE_OUT_OF_RANGE,
            "Gpio pin %d is not available", pinNum);
    return gpio.activate(gpio.tafGpioRefPin[pinNum], lock);
}

le_result_t taf_gpio_Deactivate (uint8_t pinNum, bool lock)
{
    LE_DEBUG("taf_gpio_Deactivate, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    TAF_ERROR_IF_RET_VAL(pinNum < 0 || pinNum >= gpio.numOfGpios, LE_OUT_OF_RANGE,
            "Gpio pin %d is not available", pinNum);
    return gpio.deactivate(gpio.tafGpioRefPin[pinNum], lock);
}

taf_gpio_State_t taf_gpio_Read (uint8_t pinNum, bool lock)
{
    LE_DEBUG("taf_gpio_Read, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    TAF_ERROR_IF_RET_VAL(pinNum < 0 || pinNum >= gpio.numOfGpios, TAF_GPIO_BUSY,
            "Gpio pin %d is not available", pinNum);
    return gpio.readValue(gpio.tafGpioRefPin[pinNum], lock);
}

bool taf_gpio_IsActive (uint8_t pinNum)
{
    LE_DEBUG("taf_gpio_IsActive, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    TAF_ERROR_IF_RET_VAL(pinNum < 0 || pinNum >= gpio.numOfGpios, false,
            "Gpio pin %d is not available", pinNum);
    return gpio.isActive(gpio.tafGpioRefPin[pinNum]);
}

bool taf_gpio_IsInput (uint8_t pinNum)
{
    LE_DEBUG("taf_gpio_IsInput, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    TAF_ERROR_IF_RET_VAL(pinNum < 0 || pinNum >= gpio.numOfGpios, false,
            "Gpio pin %d is not available", pinNum);
    return gpio.isInput(gpio.tafGpioRefPin[pinNum]);
}

bool taf_gpio_IsOutput (uint8_t pinNum)
{
    LE_DEBUG("taf_gpio_IsOutput, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    TAF_ERROR_IF_RET_VAL(pinNum < 0 || pinNum >= gpio.numOfGpios, false,
            "Gpio pin %d is not available", pinNum);
    return gpio.isOutput(gpio.tafGpioRefPin[pinNum]);
}

le_result_t taf_gpio_GetName (uint8_t pinNum, char* name /* output */, size_t nameSize)
{
    LE_DEBUG("taf_gpio_GetName, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    TAF_ERROR_IF_RET_VAL(pinNum < 0 || pinNum >= gpio.numOfGpios, LE_OUT_OF_RANGE,
            "Gpio pin %d is not available", pinNum);
    TAF_ERROR_IF_RET_VAL(nameSize > TAF_GPIO_PIN_NAME_MAX_BYTE, LE_OVERFLOW,
            "The nameSize %d is overflowed", pinNum);
    return gpio.getName(gpio.tafGpioRefPin[pinNum], name, nameSize);
}

taf_gpio_Edge_t taf_gpio_GetEdgeSense (uint8_t pinNum)
{
    LE_DEBUG("taf_gpio_GetEdgeSense, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    TAF_ERROR_IF_RET_VAL(pinNum < 0 || pinNum >= gpio.numOfGpios, TAF_GPIO_EDGE_UNKNOWN,
            "Gpio pin %d is not available", pinNum);
    return (taf_gpio_Edge_t)gpio.getEdgeSense( gpio.tafGpioRefPin[pinNum]);
}

taf_gpio_Polarity_t taf_gpio_GetPolarity (uint8_t pinNum)
{
    LE_DEBUG("taf_gpio_GetPolarity, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    TAF_ERROR_IF_RET_VAL(pinNum < 0 || pinNum >= gpio.numOfGpios,
            (taf_gpio_Polarity_t)GPIO_ACTIVE_TYPE_UNKNOWN, "Gpio pin %d is not available", pinNum);
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
    TAF_ERROR_IF_RET_VAL(pinNum < 0 || pinNum >= gpio.numOfGpios,
            nullptr, "Gpio pin %d is not available", pinNum);

    TAF_ERROR_IF_RET_VAL(handlerPtr == nullptr, nullptr,
            "handlerPtr is NULL");

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
    TAF_ERROR_IF_RET_VAL(pinNum < 0 || pinNum >= gpio.numOfGpios, LE_OUT_OF_RANGE,
            "Gpio pin %d is not available", pinNum);
    return gpio.setEdgeSense(gpio.tafGpioRefPin[pinNum], trigger, lock,
            tafGpio_InputMonitorHandlerFunc);
}

le_result_t taf_gpio_DisableEdgeSense (uint8_t pinNum, bool lock)
{
    LE_DEBUG("taf_gpio_DisableEdgeSense, pinNum :%d",pinNum);
    auto &gpio = taf_Gpio::getInstance();
    TAF_ERROR_IF_RET_VAL(pinNum < 0 || pinNum >= gpio.numOfGpios, LE_OUT_OF_RANGE,
            "Gpio pin %d is not available", pinNum);
    return gpio.disableEdgeSense(gpio.tafGpioRefPin[pinNum], lock);
}

static void TafSigTermEventHandler(int tafSigNum)
{
    auto &gpio = taf_Gpio::getInstance();
    LE_INFO("TafSigTermEventHandler :%d", tafSigNum);

    if(gpio.isDrvPresent == true && gpio.gpioInf != nullptr)
    {
        (*(gpio.gpioInf->releaseHAL))();
        taf_devMgr_UnloadDrv(gpio.gpioInf);
    }
    LE_INFO("unload successful");

    gpio.isDrvPresent = false;
    gpio.gpioInf = nullptr;
}

COMPONENT_INIT
{
    LE_INFO("####### COMPONENT_INIT gpio svc ######....");

    auto &gpio = taf_Gpio::getInstance();
    gpio.Init();
    int fd, ret;
    struct gpiochip_info info;

    // open the device
    fd = le_fd_Open(DEV_NAME, O_RDONLY);
    if (fd < 0)
    {
        LE_ERROR("Unabled to open %s: %s", DEV_NAME, strerror(errno));
        return;
    }

    // Query GPIO chip information
    ret = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &info);
    if (ret == -1)
    {
        LE_ERROR("Unable to get chip info from ioctl: %s", strerror(errno));
        close(fd);
        return;
    }
    LE_INFO("Number of gpio lines available: %d\n", info.lines);
    gpio.numOfGpios = info.lines;
    le_fd_Close(fd);

    // Setup signal's event handler.
    le_sig_SetEventHandler(SIGTERM, TafSigTermEventHandler);

    // load driver
    gpio.gpioInf = (gpio_Inf_t *)taf_devMgr_LoadDrv(TAF_GPIO_MODULE_NAME, nullptr);

    if(gpio.gpioInf == nullptr)
    {
        LE_ERROR("Can not load the driver %s", TAF_GPIO_MODULE_NAME);
        gpio.isDrvPresent = false;
    }
    else // successfully loaded
    {
        LE_INFO("Driver loaded successfully....");

        gpio.isDrvPresent = true;

        // init VHAL module first
        (*(gpio.gpioInf->initHAL))();

        // Get the GPIO number
        size_t gpioCount = (*(gpio.gpioInf->getTotalGpioPins))();
        LE_INFO("Total GPIOs available in the system: %" PRIuS, gpioCount);

        if(gpioCount == 0)
        {
            LE_ERROR("No gpio available");
            taf_devMgr_UnloadDrv(gpio.gpioInf);
            gpio.isDrvPresent = false;
        }
        else
        {
            LE_INFO("available gpios: %" PRIuS, gpioCount);
        }
    }

    le_mem_PoolRef_t gpioRefPool = le_mem_CreatePool("gpioRefPool", sizeof(taf_gpio));
    for(int i = 0; i < gpio.numOfGpios; i++)
    {
        gpio.tafGpioRefPin[i] = (taf_gpio*)le_mem_ForceAlloc(gpioRefPool);
        gpio.tafGpioRefPin[i]->pinNum = i;
        gpio.tafGpioRefPin[i]->fdMonitor = -1;
        snprintf(gpio.tafGpioRefPin[i]->gpioName, sizeof(gpio.tafGpioRefPin[i]->gpioName), "gpio%d", i);
        gpio.tafGpioRefPin[i]->isLocked = false;
        gpio.tafGpioRefPin[i]->handlerCount = 0;
        gpio.tafGpioRefPin[i]->fdMonitorRef = nullptr;
        gpio.tafGpioRefPin[i]->lockedSession = nullptr;
        gpio.tafGpioRefPin[i]->edge = TAF_GPIO_EDGE_NONE;
        gpio.tafGpioRefPin[i]->clientHashMap =  le_hashmap_Create(gpio.tafGpioRefPin[i]->gpioName,
                31, le_hashmap_HashVoidPointer, le_hashmap_EqualsVoidPointer);
        gpio.tafGpioRefPin[i]->aliasName[0] = '\0';
    }
}
