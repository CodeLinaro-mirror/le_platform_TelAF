/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "tafHalGpio.h"
#include "tafHalLib.hpp"

static gpio_Inf_t *gpioInf;
static le_thread_Ref_t threadRef;

static void gpioStateCb(int32_t pinNum, int32_t status)
{
    LE_INFO("*******gpio status change: %d, pin num: %d", status, pinNum);
}

static void TafSigTermEventHandler(int tafSigNum)
{
    LE_INFO("TafSigTermEventHandler :%d", tafSigNum);
    taf_devMgr_UnloadDrv(gpioInf);
    LE_INFO("unload successful");
    gpioInf = nullptr;
}

static void* Test_taf_gpio_ChangeCallback(void* ctxPtr)
{
    LE_INFO("Registered for GPIO callback");
    (*(gpioInf->regCallbackHAL))(61, GPIO_HAL_EDGE_BOTH, gpioStateCb);
    le_event_RunLoop();
}

COMPONENT_INIT
{
    le_sig_Block(SIGTERM);

    // Setup signal's event handler.
    le_sig_SetEventHandler(SIGTERM, TafSigTermEventHandler);

    size_t gpioCount;

    // Load the driver and does not care the version
    gpioInf = (gpio_Inf_t *)taf_devMgr_LoadDrv(TAF_GPIO_MODULE_NAME, nullptr);

    if(gpioInf == nullptr)
    {
        LE_ERROR("Can not load the driver %s", TAF_GPIO_MODULE_NAME);
    }
    else // successfully loaded
    {
        LE_INFO("Try to call gpioInf:%p", gpioInf);

        // init first
        (*(gpioInf->InitHAL))();

        // Get the GPIO number
        gpioCount = (*(gpioInf->getTotalGpioPinsHAL))();

        LE_INFO("Total %" PRIuS "GPIOs available in the system", gpioCount);

        if(gpioCount == 0)
        {
            LE_ERROR("No gpio available");
            taf_devMgr_UnloadDrv(gpioInf);
            gpioInf = nullptr;
        }
        else
        {
            LE_INFO("Total GPIOs available in the system: %" PRIuS, gpioCount);

            // regsiter callback
            LE_INFO("Register gpio statu change notification");

            threadRef = le_thread_Create("taf_GPIO_StateHandler",
                                         Test_taf_gpio_ChangeCallback,
                                         nullptr);
            le_thread_Start(threadRef);
        }
    }
}
