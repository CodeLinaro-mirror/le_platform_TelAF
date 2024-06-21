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
    GPIO_HAL_DIRECTION_UNKNOWN,
    GPIO_HAL_DIRECTION_INPUT,
    GPIO_HAL_DIRECTION_OUTPUT
} taf_hal_gpio_Direction;

typedef enum
{
    GPIO_HAL_STATE_BUSY = -1,
    GPIO_HAL_STATE_LOW,
    GPIO_HAL_STATE_HIGH
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

//--------------------------------------------------------------------------------------------------
/**
 * Initializes all the required resources
 * @param
 *
 * @return
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*INIT)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Releases the allocated resources
 * @param
 *
 * @return
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*RELEASE)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Send sleep request to device.
 * @param
 *
 * @return
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_HAL_GPIO_SLEEP)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Send wake-up request to device.
 * @param
 *
 * @return
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_HAL_GPIO_WAKEUP)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Gets GPIO pins number
 * @param
 *
 * @return
 *      returns the number of available GPIOs in the device
 * 
 * @note
 *      the number has to be consective, returning 1 means gpio number 0 and 1 is available.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef size_t (*TAF_HAL_GPIO_GETTOTALGPIOPINS)
(
);

//--------------------------------------------------------------------------------------------------
/**
 * Hander to get gpio pin state change 
 * @param
 *      pinNum      - pin number with state change
 *      edgeType    - edge type for the state change
 *
 * @return
 *
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_HAL_GPIO_GPIOHANDLER)
(
    uint8_t pinNum,
    taf_hal_gpio_Edge edgeType
);

//--------------------------------------------------------------------------------------------------
/**
 * Registers callback for pin state change with the input edge type
 * @param
 *      pinNum      - pin number for which callback needs to be registered
 *      edgeType    - edge type to trigger callback
 *      handler     - callback handler
 *
 * @return
 *      result of registering call back
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_GPIO_REGISTERCALLBACK)
(
    uint8_t pinNum,
    taf_hal_gpio_Edge edgeType,
    TAF_HAL_GPIO_GPIOHANDLER handler
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove callback for the requested pin and edge type
 * @param
 *      pinNum      - requested pin number
 *      edgeType    - requested edge type
 *
 * @return
 *      result of removing call back
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_GPIO_REMOVECALLBACK)
(
    uint8_t pinNum,
    taf_hal_gpio_Edge edgeType
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets direction for requested pin number
 * @param
 *      pinNum      - requested pin number
 *
 * @return
 *      direction of the requested pin number
 */
//--------------------------------------------------------------------------------------------------
typedef taf_hal_gpio_Direction (*TAF_HAL_GPIO_GETDIRECTION)
(
    uint8_t pinNum
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets polarity for requested output pin number
 * @param
 *      pinNum      - requested pin number
 *
 * @return
 *      polarity of the requested pin number
 */
//--------------------------------------------------------------------------------------------------
typedef taf_hal_gpio_ActiveType_t (*TAF_HAL_GPIO_GETPOLARITY)
(
    uint8_t pinNum
);

//--------------------------------------------------------------------------------------------------
/**
 * Sets polarity for requested output pin number
 * @param
 *      pinNum      - requested pin number
 *      type        - polarity
 *
 * @return
 *      result of setting the polarity for the requested pin number
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_GPIO_SETPOLARITY)
(
    uint8_t pinNum,
    taf_hal_gpio_ActiveType_t type
);

//--------------------------------------------------------------------------------------------------
/**
 * Sets direction for requested pin number
 * @param
 *      pinNum      - requested pin number
 *      direction   - direction
 *
 * @return
 *      result of setting the direction for the requested pin number
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_GPIO_SETDIRECTION)
(
    uint8_t pinNum,
    taf_hal_gpio_Direction direction
);

//--------------------------------------------------------------------------------------------------
/**
 * Gets state for the requested input pin number
 * @param
 *      pinNum      - requested pin number
 *
 * @return
 *      state of the requested pin number
 */
//--------------------------------------------------------------------------------------------------
typedef taf_hal_gpio_State (*TAF_HAL_GPIO_GETSTATE)
(
    uint8_t pinNum
);

//--------------------------------------------------------------------------------------------------
/**
 * Sets state for the requested output pin number
 * @param
 *      pinNum      - requested pin number
 *      state       - state that is needed to be written
 *
 * @return
 *      result of setting the value for the requested pin number
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_GPIO_SETOUTPUTSTATE)
(
    uint8_t pinNum,
    taf_hal_gpio_State state
);

//--------------------------------------------------------------------------------------------------
/**
 * Set detection mode of edge type for the requested input pin number
 * @param
 *      pinNum      - requested pin number
 *      edge        - edge type
 *
 * @return
 *      result of setting the edge for the requested pin number
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_GPIO_SETEDGESENSE)
(
    uint8_t pinNum,
    taf_hal_gpio_Edge edge
);

//--------------------------------------------------------------------------------------------------
/**
 * Get detection mode of edge type for the requested input pin number
 * @param
 *      pinNum      - requested pin number
 *      edge        - edge type
 *
 * @return
 *      result of getting the edge for the requested pin number
 */
//--------------------------------------------------------------------------------------------------
typedef taf_hal_gpio_Edge (*TAF_HAL_GPIO_GETEDGESENSE)
(
    uint8_t pinNum
);

//--------------------------------------------------------------------------------------------------
/**
 * Disable detection mode of edge type for the input pin number
 * @param
 *      pinNum      - requested pin number
 *      edge        - edge type
 *
 * @return
 *      result of disabling the edge for the requested pin number
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*TAF_HAL_GPIO_DISABLEEDGESENSE)
(
    uint8_t pinNum
);

//--------------------------------------------------------------------------------------------------
/**
 * Get alias name for the requested output pin number
 * @param
 *      pinNum      - pin number for which name is needed
 *
 * @return
 *      name corresponding to the requested pin
 */
//--------------------------------------------------------------------------------------------------
typedef char* (*TAF_HAL_GPIO_GETNAME)
(
    uint8_t pinNum
);

typedef struct
{
    INIT initHAL;

    RELEASE releaseHAL;

    TAF_HAL_GPIO_SLEEP sleep;

    TAF_HAL_GPIO_WAKEUP wakeUp;

    TAF_HAL_GPIO_GETTOTALGPIOPINS getTotalGpioPins;

    TAF_HAL_GPIO_REGISTERCALLBACK registerCallback;

    TAF_HAL_GPIO_REMOVECALLBACK removeCallback;

    TAF_HAL_GPIO_GETDIRECTION getDirection;

    TAF_HAL_GPIO_GETPOLARITY getPolarity;

    TAF_HAL_GPIO_SETPOLARITY setPolarity;

    TAF_HAL_GPIO_SETDIRECTION setDirection;

    TAF_HAL_GPIO_GETSTATE getState;

    TAF_HAL_GPIO_SETOUTPUTSTATE setOutputState;

    TAF_HAL_GPIO_SETEDGESENSE setEdgeSense;

    TAF_HAL_GPIO_GETEDGESENSE getEdgeSense;

    TAF_HAL_GPIO_DISABLEEDGESENSE disableEdgeSense;

    TAF_HAL_GPIO_GETNAME getName;

} gpio_Inf_t;

typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf;   // management interface for device manager
    gpio_Inf_t gpioInf;         // module interface for application/service

} gpio_InfoTab_t;

extern gpio_InfoTab_t TAF_HAL_INFO_TAB;

#endif
