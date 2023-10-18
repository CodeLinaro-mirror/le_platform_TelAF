/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __GPIO_H__

#define __GPIO_H__

#include "tafHalIF.hpp"

// Define the name or ID for HAL module
#define TAF_GPIO_MODULE_NAME "TafHalGpio"

typedef enum
{
    GPIO_HAL_OK,
    GPIO_HAL_BUSY,
    GPIO_HAL_ERROR
} taf_hal_gpio_Status;

typedef enum
{
    GPIO_HAL_INPUT,
    GPIO_HAL_OUTPUT
} taf_hal_gpio_Direction;

typedef enum
{
    GPIO_HAL_STATE_HIGH,
    GPIO_HAL_STATE_LOW
} taf_hal_gpio_State;

typedef enum
{
    GPIO_HAL_EDGE_UNKNOWN,
    GPIO_HAL_EDGE_NONE,
    GPIO_HAL_EDGE_RISING,
    GPIO_HAL_EDGE_FALLING,
    GPIO_HAL_EDGE_BOTH
} taf_hal_gpio_Edge;

typedef enum
{
    GPIO_HAL_ACTIVE_TYPE_UNKNOWN = -1,
    GPIO_HAL_ACTIVE_TYPE_HIGH,
    GPIO_HAL_ACTIVE_TYPE_LOW
} taf_hal_gpio_ActiveType_t;

// init all the required parameters here
typedef void (*INIT)(void);

// returns the number of available GPIOs in the device
typedef size_t (*GETTOTALGPIOPINS)();

typedef void (*GPIOHANDLER)(int32_t pinNum, int32_t status);

// registers the callback to be called in case of a trigger event
// Input params:
//      pinNum      - pin number for which callback needs to be registered
//      edgeType    - trigger for callback
//      handler     - callback handler
typedef bool (*TAF_HAL_REGCALLBACK)(int32_t pinNum,
    taf_hal_gpio_Edge edgeType, GPIOHANDLER handler);

// get the direction for requested pin number
// Input params:
//      pinNum      - pin number for which direction is needed
typedef taf_hal_gpio_Direction (*TAF_HAL_GETDIRECTION)(uint8_t pinNum);

// get the polarity for requested pin number
// Input params:
//      pinNum      - pin number for which polarity is needed
typedef taf_hal_gpio_ActiveType_t (*TAF_HAL_GETPOLARITY)(uint8_t pinNum);

// set the polarity for requested pin number
// Input params:
//      pinNum      - pin number for which polarity is needed to be set
//      type        - polarity
typedef taf_hal_gpio_Status (*TAF_HAL_SETPOLARITY)(uint8_t pinNum, taf_hal_gpio_ActiveType_t type);

// set the direction for requested pin number
// Input params:
//      pinNum      - pin number for which direction is needed to be set
//      dir         - direction
typedef taf_hal_gpio_Status (*TAF_HAL_SETDIRECTION)(uint8_t pinNum, uint32_t dir);

// get the value for requested pin number
// Input params:
//      pinNum      - pin number for which value is needed to be read
//      lock        - true to block gpio pin usage by other clients, else false
typedef int (*TAF_HAL_GETVALUE)(uint8_t pinNum, bool lock);

// write a specified value to requested output pin number
// Input params:
//      pinNum      - pin number for which value is needed to be read
//      value       - value that is needed to be wrote
typedef taf_hal_gpio_Status (*TAF_HAL_WRITEOUTPUTVALUE)(uint8_t pinNum,
    taf_hal_gpio_State value);

// set edge type for a requested output pin number
// Input params:
//      pinNum      - pin number for which edge type is needed to be set
//      edge        - edge type
typedef taf_hal_gpio_Status (*TAF_HAL_SETEDGETYPE)(uint8_t pinNum,
    taf_hal_gpio_Edge edge);

// get name for a requested output pin number
// Input params:
//      pinNum      - pin number for which edge type is needed to be set
typedef char* (*TAF_HAL_GETNAME)(uint8_t pinNum);

typedef struct
{
    // the number has to be consective.  return 1, mean gpio number 0 and 1 is avaible.
    // hal module do the translation

    INIT InitHAL;

    GETTOTALGPIOPINS getTotalGpioPinsHAL;

    // register state change callback
    TAF_HAL_REGCALLBACK regCallbackHAL;

    // let the app/service to handle pm, not device manager
    TAF_HAL_SLEEP sleepHAL;
    TAF_HAL_WAKEUP wakeUpHAL;

    TAF_HAL_GETDIRECTION getDirectionHAL;

    TAF_HAL_GETPOLARITY getPolarityHAL;
    TAF_HAL_SETPOLARITY setPolarityHAL;

    TAF_HAL_SETDIRECTION setDirectionHAL;

    TAF_HAL_GETVALUE getValueHAL;

    TAF_HAL_WRITEOUTPUTVALUE writeOutputValueHAL;

    TAF_HAL_SETEDGETYPE setEdgeTypeHAL;

    TAF_HAL_GETNAME getNameHAL;

} gpio_Inf_t;

typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf; // for device manager
    gpio_Inf_t gpioInf; // for app/service

} gpio_InfoTab_t;

extern gpio_InfoTab_t TAF_HAL_INFO_TAB;

#endif
