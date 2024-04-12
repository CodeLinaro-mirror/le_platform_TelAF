/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "tafMngdPMCommon.hpp"
#include "tafMngdPMGpio.hpp"
#include "tafMngdPMSvc.hpp"

static le_hashmap_Ref_t gpioPMMap = nullptr;

using namespace telux::tafsvc;

void tafMngdPMGpio::GpioChangeCallback(uint8_t pinNum, bool state, void *ctx){
    LE_INFO("State changed to %d for pinNum %d", (int)state, pinNum);

    le_hashmap_It_Ref_t iteratorRef = le_hashmap_GetIterator(gpioPMMap);
    while (le_hashmap_NextNode(iteratorRef) == LE_OK)
    {
        taf_MngdPM_Gpio_t* gpioPtr = (taf_MngdPM_Gpio_t*)le_hashmap_GetValue(iteratorRef);

        TAF_ERROR_IF_RET_NIL(gpioPtr == nullptr, "Invalid hashmap reference");

        if(gpioPtr->pinNum == pinNum && (gpioPtr-> value == state))
        {
            LE_INFO("Gpio %d matched for state registered for %s", gpioPtr->pinNum,
                    tafMngdPMSvc::TafStateToString(gpioPtr->state));

            taf_mngd_pm_State_t requestedState;
            switch((taf_pm_State_t)gpioPtr->state)
            {
                case TAF_PM_STATE_SUSPEND:
                    requestedState = TAF_MNGD_PM_STATE_SUSPENDING;
                    break;

                case TAF_PM_STATE_SHUTDOWN:
                    requestedState = TAF_MNGD_PM_STATE_SHUTTING_DOWN;
                    break;

                case TAF_PM_STATE_RESUME:
                    requestedState = TAF_MNGD_PM_STATE_WAKING_UP;
                    break;

                default:
                    break;
            }

            if(tafMngdPMSvc::RequestStateChange(requestedState) != LE_OK)
            {
                return;
            }

            taf_pm_SetAllVMPowerState((taf_pm_State_t)gpioPtr->state);

            tafMngdPMSvc::ProcessStateChange(requestedState);
        }
    }
}
void tafMngdPMGpio::RegisterGpioChangeCallback(uint8_t pinNum, bool value, taf_mngd_pm_State_t state) {
    LE_INFO("registerChangeCallback for PIN %d gpioMapPool is %p", pinNum, gpioMapPool);
    if(gpioMapPool == NULL) {
        gpioMapPool = le_mem_CreatePool("tafMngdPMGpioMapPool", sizeof(taf_MngdPM_Gpio_t));
    }
    taf_MngdPM_Gpio_t* gpioPMPtr = (taf_MngdPM_Gpio_t*)le_mem_ForceAlloc(gpioMapPool);
    memset(gpioPMPtr, 0, sizeof(taf_MngdPM_Gpio_t));
    gpioPMPtr->pinNum = pinNum;
    gpioPMPtr->value = value;
    gpioPMPtr->state = state;
    if(gpioPMMap == nullptr) {
        gpioPMMap =  le_hashmap_Create("tafMngdPMGpioMap", TAF_MNGD_PM_MAX_TRIGGER_REGISTERS,
                le_hashmap_HashVoidPointer, le_hashmap_EqualsVoidPointer);
    }
    gpioPMPtr->handlerRef = taf_gpio_AddChangeEventHandler(pinNum, TAF_GPIO_EDGE_BOTH, false,
            GpioChangeCallback, NULL);
    if (gpioPMPtr->handlerRef != NULL) {
        LE_INFO("registered successfully for gpio pin trigger");
    } else {
        LE_ERROR("Failed to register for gpio pin trigger");
        return;
    }
    le_hashmap_Put(gpioPMMap, gpioPMPtr, gpioPMPtr);
}

void tafMngdPMGpio::DeregisterGpioChangeCallback()
{
    LE_DEBUG("DeregisterGpioChangeCallback");
    if(!gpioPMMap)
        return;

    le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(gpioPMMap);
    while (LE_OK == le_hashmap_NextNode(iter))
    {
        taf_MngdPM_Gpio_t *gpioPtr = (taf_MngdPM_Gpio_t*)le_hashmap_GetValue(iter);
        TAF_ERROR_IF_RET_NIL(gpioPtr == nullptr, "Invalid hashmap reference");

        taf_gpio_RemoveChangeEventHandler(gpioPtr->handlerRef);
        le_hashmap_Remove(gpioPMMap, gpioPtr);
        le_mem_Release(gpioPtr);
    }
}

tafMngdPMGpio &tafMngdPMGpio::GetInstance()
{
    static tafMngdPMGpio instance;
    return instance;
}

