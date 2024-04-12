/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafHalGpio.h"
#include <linux/gpio.h>

//--------------------------------------------------------------------------------------------------
/**
 * GPIO definition.
 */
//--------------------------------------------------------------------------------------------------

#define DEV_NAME "/dev/gpiochip0"
#define COMP_NAME "VHAL_GPIO"

#define MAX_PIN_NUMBER 20

#define TAF_GPIO_PIN_NAME_MAX_BYTE 32

//--------------------------------------------------------------------------------------------------
/**
 * Resources for GPIO IOCTL.
 */
//--------------------------------------------------------------------------------------------------

int gpioFd = -1;

//--------------------------------------------------------------------------------------------------
/**
 * GPIO physical map.
 * This map can be cutomized to map to required physical GPIO pins if needed.
 * Please leave '-1' for the unused slot.
 */
//--------------------------------------------------------------------------------------------------
int8_t physicalPinMap[MAX_PIN_NUMBER] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                                        10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
//--------------------------------------------------------------------------------------------------
/**
 * Structures for pins
 */
//--------------------------------------------------------------------------------------------------
// Structure to keep information corresponding to each GPIO pin
typedef struct
{
    int8_t phyPinNum;
    char gpioName[10];
    char aliasName[TAF_GPIO_PIN_NAME_MAX_BYTE];
    int fdMonitor;
    int handlerCount;
    le_fdMonitor_Ref_t fdMonitorRef;
    taf_hal_gpio_Direction direction;
    taf_hal_gpio_Edge edge;
}
gpio_pinInfo_t;

// Structure to keep track of all the available GPIO pins
struct gpio_hal_info
{
    uint32_t numOfGpios;
    gpio_pinInfo_t pinInfo[MAX_PIN_NUMBER];
};

static struct gpio_hal_info gpio;

//--------------------------------------------------------------------------------------------------
/**
 * Resources for triggering callbacks.
 */
//--------------------------------------------------------------------------------------------------

static le_mem_PoolRef_t CallbackInfoPoolRef;

le_dls_List_t GpioHandlerList = LE_DLS_LIST_INIT;

// Structure to keep callback information
typedef struct
{
    TAF_HAL_GPIO_GPIOHANDLER  handlerPtr; // handler function pointer
    uint8_t                   pinNum;     // pinNum for which handler registered
    le_dls_Link_t             link;       // link to handler list
    taf_hal_gpio_Edge         edgeType;
}
callbackInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Resources for event.
 */
//--------------------------------------------------------------------------------------------------

static le_event_Id_t gpioEvent;

// Structure to return gpioEvent
typedef struct
{
    uint8_t pinNum;
    taf_hal_gpio_Edge edgeType;
}
gpioStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * Implement functional interfaces.
 */
//--------------------------------------------------------------------------------------------------

static void taf_hal_PowerOn()
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);
    return;
}

static void taf_hal_PowerOff()
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);
    return;
}

static int taf_hal_HwInit()
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);
    return 0;
}

static void taf_hal_Sleep()
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);
    return;
}

static void taf_hal_Wakeup()
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);
    return;
}

static int taf_hal_SelfTest()
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);
    return 0;
}

static size_t taf_hal_GetTotalGpioPins()
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);

    uint count = 0;
    for(uint i = 0; i < MAX_PIN_NUMBER; i++)
    {
        if(-1 != physicalPinMap[i])
        {
            count++;
        }
    }

    return count;
}

static taf_hal_gpio_Direction taf_hal_GetDirection
(
    uint8_t pinNum
)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);
    int ret;
    struct gpioline_info line_info;
    line_info.line_offset = pinNum;

    // Get the gpio line info (IN/OUT)
    ret = ioctl(gpioFd, GPIO_GET_LINEINFO_IOCTL, &line_info);

    if (ret == -1)
    {
        LE_ERROR("Unable to get line info from offset %d:%s",
            pinNum, strerror(errno));
        return GPIO_HAL_DIRECTION_UNKNOWN;
    }

    LE_INFO("the gpiopin %d is %s", pinNum,
            (line_info.flags & GPIOLINE_FLAG_IS_OUT) ? "OUT" : "IN");

    return (line_info.flags & GPIOLINE_FLAG_IS_OUT) ?
        GPIO_HAL_DIRECTION_OUTPUT : GPIO_HAL_DIRECTION_INPUT;
}

static taf_hal_gpio_ActiveType_t taf_hal_GetPolarity
(
    uint8_t pinNum
)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);
    int ret;
    struct gpioline_info line_info;

    line_info.line_offset = pinNum;
    snprintf(line_info.consumer, sizeof(line_info.consumer), COMP_NAME);

    ret = ioctl(gpioFd, GPIO_GET_LINEINFO_IOCTL, &line_info);

    if (ret == -1)
    {
        LE_ERROR("Unable to get line info from offset %d:%s",
            pinNum, strerror(errno));
        return GPIO_HAL_ACTIVE_TYPE_UNKNOWN;
    }
    LE_DEBUG("the gpiopin active_low is %d", (line_info.flags & GPIOLINE_FLAG_ACTIVE_LOW)
            ? GPIO_HAL_ACTIVE_TYPE_LOW : GPIO_HAL_ACTIVE_TYPE_HIGH);
    return (line_info.flags & GPIOLINE_FLAG_ACTIVE_LOW)
            ? GPIO_HAL_ACTIVE_TYPE_LOW : GPIO_HAL_ACTIVE_TYPE_HIGH;
}

static le_result_t taf_hal_SetPolarity
(
    uint8_t pinNum,
    taf_hal_gpio_ActiveType_t level
)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);
    int ret;
    struct gpiohandle_request rq;

    if(pinNum >= gpio.numOfGpios)
    {
        return LE_UNSUPPORTED;
    }

    if (gpio.pinInfo[pinNum].fdMonitor != -1)
    {
        le_fd_Close(gpio.pinInfo[pinNum].fdMonitor);
        gpio.pinInfo[pinNum].fdMonitor = -1;
    }

    rq.lineoffsets[0] = pinNum;
    if(level == GPIO_HAL_ACTIVE_TYPE_LOW)
    {
        rq.flags = GPIOHANDLE_REQUEST_ACTIVE_LOW | GPIOHANDLE_REQUEST_INPUT;
    }
    else if(level == GPIO_HAL_ACTIVE_TYPE_HIGH)
    {
        rq.flags = GPIOHANDLE_REQUEST_INPUT;
    }
    rq.lines = 1;
    snprintf(rq.consumer_label, sizeof(rq.consumer_label), COMP_NAME);
    ret = ioctl(gpioFd, GPIO_GET_LINEHANDLE_IOCTL, &rq);

    if (ret == -1)
    {
        LE_ERROR("Unable to get line handle from ioctl : %s", strerror(errno));
        return LE_IO_ERROR;
    }

    LE_INFO("Succesfully set the polarity");
    gpio.pinInfo[pinNum].fdMonitor = rq.fd;
    return LE_OK;
}

static taf_hal_gpio_State taf_hal_GetState
(
    uint8_t pinNum
)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);

    int ret;
    taf_hal_gpio_State state;
    struct gpiohandle_request rq;
    struct gpiohandle_data data;

    if(pinNum >= gpio.numOfGpios)
    {
        return GPIO_HAL_STATE_BUSY;
    }

    if (gpio.pinInfo[pinNum].fdMonitor == -1)
    {
        rq.lineoffsets[0] = gpio.pinInfo[pinNum].phyPinNum;
        rq.flags = GPIOHANDLE_REQUEST_INPUT;
        rq.lines = 1;
        snprintf(rq.consumer_label, sizeof(rq.consumer_label), COMP_NAME);
        ret = ioctl(gpioFd, GPIO_GET_LINEHANDLE_IOCTL, &rq);

        if (ret == -1)
        {
            LE_ERROR("Unable to get line handle from ioctl : %s", strerror(errno));
            return LE_BUSY;
        }
        gpio.pinInfo[pinNum].fdMonitor = rq.fd;
    }
    // Get the value
    ret = ioctl(gpio.pinInfo[pinNum].fdMonitor, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
    LE_INFO("ioctl fd is %d", gpio.pinInfo[pinNum].fdMonitor);

    if (ret == -1)
    {
        LE_ERROR("Unable to get line value using ioctl : %s", strerror(errno));
        return GPIO_HAL_STATE_BUSY;
    }
    else
    {
        LE_INFO("Value of GPIO pin %d on chip %s: %d", gpio.pinInfo[pinNum].phyPinNum, DEV_NAME,
                data.values[0]);
        state = (taf_hal_gpio_State)data.values[0];
    }

    return state;
}

static le_result_t taf_hal_SetOutputState
(
    uint8_t pinNum,
    taf_hal_gpio_State state
)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);

    struct gpiohandle_request rq;
    struct gpiohandle_data data;
    int ret;

    if(pinNum >= gpio.numOfGpios)
    {
        return LE_UNSUPPORTED;
    }

    if (gpio.pinInfo[pinNum].fdMonitor != -1)
    {
        le_fd_Close(gpio.pinInfo[pinNum].fdMonitor);
        gpio.pinInfo[pinNum].fdMonitor = -1;
    }

    rq.lineoffsets[0] = gpio.pinInfo[pinNum].phyPinNum;
    rq.flags = GPIOHANDLE_REQUEST_OUTPUT;
    rq.lines = 1;
    snprintf(rq.consumer_label, sizeof(rq.consumer_label), COMP_NAME);

    ret = ioctl(gpioFd, GPIO_GET_LINEHANDLE_IOCTL, &rq);

    if (ret == -1)
    {
        LE_ERROR("Unable to line handle from ioctl : %s", strerror(errno));
        return LE_IO_ERROR;
    }

    gpio.pinInfo[pinNum].fdMonitor = rq.fd;
    data.values[0] = state;
    ret = ioctl(gpio.pinInfo[pinNum].fdMonitor, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);

    if (ret == -1)
    {
        LE_ERROR("Unable to set line value using ioctl : %s", strerror(errno));
        le_fd_Close(rq.fd);
        gpio.pinInfo[pinNum].fdMonitor = -1;
        return LE_BUSY;
    }
        LE_INFO("Succesfully wrote %d to the GPIOPIN %d", data.values[0], gpio.pinInfo[pinNum].phyPinNum);

    gpio.pinInfo[pinNum].direction = GPIO_HAL_DIRECTION_OUTPUT;
    return LE_OK;
}

static le_result_t taf_hal_DisableEdgeSense
(
    uint8_t pinNum
)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);

    if(pinNum >= gpio.numOfGpios)
    {
        return LE_UNSUPPORTED;
    }
    if(gpio.pinInfo[pinNum].fdMonitorRef != NULL)
    {
        LE_INFO("Stopping fd monitor");
        le_fdMonitor_Delete(gpio.pinInfo[pinNum].fdMonitorRef);
        gpio.pinInfo[pinNum].fdMonitorRef = NULL;
    }
    if(gpio.pinInfo[pinNum].fdMonitor != -1)
    {
        le_fd_Close(gpio.pinInfo[pinNum].fdMonitor);
        gpio.pinInfo[pinNum].fdMonitor = -1;
    }

    int ret;
    struct gpioevent_request rq;

    rq.lineoffset = gpio.pinInfo[pinNum].phyPinNum;
    rq.handleflags = GPIOHANDLE_REQUEST_INPUT;
    snprintf(rq.consumer_label, sizeof(rq.consumer_label), COMP_NAME);

    rq.eventflags = GPIOEVENT_EVENT_FALLING_EDGE & GPIOEVENT_REQUEST_RISING_EDGE;

    ret = ioctl(gpioFd, GPIO_GET_LINEEVENT_IOCTL, &rq);

    if (ret == -1)
    {
        LE_INFO("Unable to get line event from ioctl : %s", strerror(errno));
        return LE_IO_ERROR;
    }

    LE_INFO("Successfully disable the edge sense");

    return LE_OK;
}

static le_result_t taf_hal_SetDirection
(
    uint8_t pinNum,
    taf_hal_gpio_Direction direction
)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);

    if(pinNum >= gpio.numOfGpios)
    {
        return LE_UNSUPPORTED;
    }

    // check if the direction is already set as the input value
    if(gpio.pinInfo[pinNum].direction == direction)
    {
        return LE_OK;
    }

    if(gpio.pinInfo[pinNum].fdMonitorRef != NULL)
    {
        LE_INFO("Stopping fd monitor");
        le_fdMonitor_Delete(gpio.pinInfo[pinNum].fdMonitorRef);
        gpio.pinInfo[pinNum].fdMonitorRef = NULL;
    }
    if(gpio.pinInfo[pinNum].fdMonitor != -1)
    {
        le_fd_Close(gpio.pinInfo[pinNum].fdMonitor);
        gpio.pinInfo[pinNum].fdMonitor = -1;
    }

    int ret;
    struct gpiohandle_request rq;
    rq.lineoffsets[0] = gpio.pinInfo[pinNum].phyPinNum;
    rq.lines = 1;
    snprintf(rq.consumer_label, sizeof(rq.consumer_label), COMP_NAME);
    rq.flags = (direction == GPIO_HAL_DIRECTION_OUTPUT) ?
            GPIOHANDLE_REQUEST_OUTPUT : GPIOHANDLE_REQUEST_INPUT;
    ret = ioctl(gpioFd, GPIO_GET_LINEHANDLE_IOCTL, &rq);

    if (ret == -1)
    {
        LE_INFO("Unable to line handle from ioctl : %s", strerror(errno));
        return LE_IO_ERROR;
    }

    gpio.pinInfo[pinNum].fdMonitor = rq.fd;
    LE_INFO("Gpio %d direction changed to %s", gpio.pinInfo[pinNum].phyPinNum,
            ((direction == GPIO_HAL_DIRECTION_OUTPUT) ? "OUT" : "IN"));

    gpio.pinInfo[pinNum].direction = direction;

    return LE_OK;
}

static char* taf_hal_GetName
(
    uint8_t pinNum
)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);

    if(pinNum >= gpio.numOfGpios)
    {
        return "";
    }

    if(strlen(gpio.pinInfo[pinNum].aliasName) > 0)
    {
        return gpio.pinInfo[pinNum].aliasName;
    }
    else
    {
        LE_ERROR("not implemented");
        return "";
    }
}

static void* taf_hal_GetModInf(void)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);
    return &(TAF_HAL_INFO_TAB.gpioInf);
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal function for monitoring GPIO device events.
 */
//--------------------------------------------------------------------------------------------------
static void inputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    gpioStatus_t gpioStatus;

    for(uint i = 0; i < gpio.numOfGpios; i++)
    {
        if(gpio.pinInfo[i].fdMonitor == fd)
        {
            gpioStatus.pinNum = gpio.pinInfo[i].phyPinNum;
        }
    }

    struct gpioevent_data event;
    int ret = le_fd_Read(fd, &event, sizeof(event));
    if (ret == -1) {
        if (errno == -EAGAIN)
        {
            LE_ERROR("nothing available");
        }
        else
        {
            ret = -errno;
            LE_ERROR("Failed to read event (%d)", ret);
        }
    }

    if (ret != sizeof(event))
    {
        LE_ERROR("Reading event failed");
        ret = -EIO;
    }

    switch (event.id)
    {
    case GPIOEVENT_EVENT_RISING_EDGE:
        LE_INFO("rising edge");
        gpioStatus.edgeType = GPIO_HAL_EDGE_RISING;
        break;

    case GPIOEVENT_EVENT_FALLING_EDGE:
        gpioStatus.edgeType = GPIO_HAL_EDGE_FALLING;
        LE_INFO("falling edge");
        break;

    default:
        LE_ERROR("unknown event");
    }

    // trigger the le_event
    le_event_Report(gpioEvent, &gpioStatus, sizeof(gpioStatus_t));

    return;
}

static le_result_t taf_hal_SetEdgeSense
(
    uint8_t pinNum,
    taf_hal_gpio_Edge edgeType
)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);

    if(pinNum >= gpio.numOfGpios)
    {
        return LE_UNSUPPORTED;
    }

    // Make sure the pin is in use and has listeners
    if (gpio.pinInfo[pinNum].direction != GPIO_HAL_DIRECTION_INPUT)
    {
        LE_ERROR("Pin%d is set as IN", pinNum);
        return LE_BAD_PARAMETER;
    }
    if (gpio.pinInfo[pinNum].handlerCount == 0)
    {
        LE_ERROR("Handler count is 0 for the pin");
        return LE_BAD_PARAMETER;
    }

    if(gpio.pinInfo[pinNum].fdMonitorRef != NULL)
    {
        LE_INFO("Stopping fd monitor");
        le_fdMonitor_Delete(gpio.pinInfo[pinNum].fdMonitorRef);
        gpio.pinInfo[pinNum].fdMonitorRef = NULL;
    }
    if(gpio.pinInfo[pinNum].fdMonitor != -1)
    {
        le_fd_Close(gpio.pinInfo[pinNum].fdMonitor);
        gpio.pinInfo[pinNum].fdMonitor = -1;
    }

    int ret;
    struct gpioevent_request rq;

    rq.lineoffset = gpio.pinInfo[pinNum].phyPinNum;
    rq.handleflags = GPIOHANDLE_REQUEST_INPUT;
    snprintf(rq.consumer_label, sizeof(rq.consumer_label), COMP_NAME);
    switch(edgeType)
    {
        case GPIO_HAL_EDGE_RISING:
            rq.eventflags = GPIOEVENT_REQUEST_RISING_EDGE;
            break;
        case GPIO_HAL_EDGE_FALLING:
            rq.eventflags = GPIOEVENT_REQUEST_FALLING_EDGE;
            break;
        case GPIO_HAL_EDGE_BOTH:
            rq.eventflags = GPIOEVENT_REQUEST_BOTH_EDGES;
            break;
        default:
            rq.eventflags = GPIOEVENT_EVENT_FALLING_EDGE & GPIOEVENT_REQUEST_RISING_EDGE;
            break;
    }

    ret = ioctl(gpioFd, GPIO_GET_LINEEVENT_IOCTL, &rq);

    if (ret == -1)
    {
        LE_INFO("Unable to get line event from ioctl : %s", strerror(errno));
        return LE_IO_ERROR;
    }

    gpio.pinInfo[pinNum].fdMonitorRef = le_fdMonitor_Create(
                                            gpio.pinInfo[pinNum].gpioName,
                                            rq.fd,
                                            inputMonitorHandlerFunc,
                                            POLLIN);
    gpio.pinInfo[pinNum].edge = edgeType;
    gpio.pinInfo[pinNum].fdMonitor = rq.fd;

    LE_INFO("Successfully set the edge type");

    return LE_OK;
}

static taf_hal_gpio_Edge taf_hal_GetEdgeSense
(
    uint8_t pinNum
)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);

    if(pinNum >= gpio.numOfGpios)
    {
        return GPIO_HAL_EDGE_UNKNOWN;
    }

    // Edge type is not valid for output pin
    if (gpio.pinInfo[pinNum].direction != GPIO_HAL_DIRECTION_INPUT)
    {
        return GPIO_HAL_EDGE_NONE;
    }

    return gpio.pinInfo[pinNum].edge;
}

static le_result_t taf_hal_RegisterCallback
(
    uint8_t pinNum,
    taf_hal_gpio_Edge edgeType,
    TAF_HAL_GPIO_GPIOHANDLER handler
)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);

    if(pinNum >= gpio.numOfGpios)
    {
        return LE_UNSUPPORTED;
    }

    if (gpio.pinInfo[pinNum].fdMonitorRef != NULL && edgeType == gpio.pinInfo[pinNum].edge)
    {
        LE_INFO("Monitor is already created for the same edge type for GPIO %d",
                gpio.pinInfo[pinNum].phyPinNum);
    }
    else
    {
        // Start monitoring the fd for the correct GPIO
        int ret;
        struct gpioevent_request rq;

        if(gpio.pinInfo[pinNum].fdMonitorRef != NULL)
        {
            LE_INFO("Stopping fd monitor");
            le_fdMonitor_Delete(gpio.pinInfo[pinNum].fdMonitorRef);
            gpio.pinInfo[pinNum].fdMonitorRef = NULL;
        }
        if(gpio.pinInfo[pinNum].fdMonitor != -1)
        {
            le_fd_Close(gpio.pinInfo[pinNum].fdMonitor);
            gpio.pinInfo[pinNum].fdMonitor = -1;
        }
        rq.lineoffset = gpio.pinInfo[pinNum].phyPinNum;
        if (edgeType == GPIO_HAL_EDGE_RISING)
        {
            rq.eventflags = GPIOEVENT_REQUEST_RISING_EDGE;
        }
        else if(edgeType == GPIO_HAL_EDGE_FALLING)
        {
            rq.eventflags = GPIOEVENT_REQUEST_FALLING_EDGE;
        }
        else if(edgeType == GPIO_HAL_EDGE_BOTH)
        {
            rq.eventflags = GPIOEVENT_REQUEST_BOTH_EDGES;
        }
        else if(edgeType == GPIO_HAL_EDGE_NONE)
        {
            rq.eventflags = GPIOEVENT_REQUEST_RISING_EDGE & GPIOEVENT_REQUEST_FALLING_EDGE;
        }
        rq.handleflags = GPIOHANDLE_REQUEST_INPUT;
        snprintf(rq.consumer_label, sizeof(rq.consumer_label), COMP_NAME);
        ret = ioctl(gpioFd, GPIO_GET_LINEEVENT_IOCTL, &rq);

        if (ret == -1)
        {
            LE_ERROR("Unable to get line event from ioctl : %s", strerror(errno));
            return LE_FAULT;
        }

        // Create fd Monitor for the input pin number
        gpio.pinInfo[pinNum].fdMonitorRef = le_fdMonitor_Create(
                                                gpio.pinInfo[pinNum].gpioName,
                                                rq.fd,
                                                inputMonitorHandlerFunc,
                                                POLLIN);

        gpio.pinInfo[pinNum].fdMonitor = rq.fd;
    }

    // Allocate callback info struct and add it to handler list
    callbackInfo_t* callbackInfo = (callbackInfo_t *)le_mem_ForceAlloc(CallbackInfoPoolRef);

    callbackInfo->pinNum = pinNum;
    callbackInfo->handlerPtr = handler;
    callbackInfo->edgeType = edgeType;
    callbackInfo->link = LE_DLS_LINK_INIT;

    le_dls_Queue(&GpioHandlerList, &(callbackInfo->link));

    gpio.pinInfo[pinNum].edge = edgeType;
    gpio.pinInfo[pinNum].direction = GPIO_HAL_DIRECTION_INPUT;
    gpio.pinInfo[pinNum].handlerCount++;

    return LE_OK;
}

static le_result_t taf_hal_RemoveCallback
(
    uint8_t pinNum,
    taf_hal_gpio_Edge edgeType
)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);

    le_result_t res = LE_BAD_PARAMETER;

    // Scan handler list to find the corresponding callbackInfo
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&GpioHandlerList);

    while (linkHandlerPtr)
    {
        callbackInfo_t* callbackInfo =
                CONTAINER_OF(linkHandlerPtr, callbackInfo_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&GpioHandlerList, linkHandlerPtr);

        if (callbackInfo &&
            callbackInfo->pinNum == pinNum &&
            callbackInfo->edgeType == edgeType)
        {
            LE_INFO("Found callback for [pin%d, edge:%d], remove it", pinNum, edgeType);

            le_dls_Remove(&GpioHandlerList, &(callbackInfo->link));
            le_mem_Release((void*)callbackInfo);
            gpio.pinInfo[pinNum].handlerCount--;

            res = LE_OK;
        }
    }

    if(gpio.pinInfo[pinNum].handlerCount == 0 && gpio.pinInfo[pinNum].fdMonitorRef != NULL)
    {
        LE_INFO("Stopping fd monitor");
        le_fdMonitor_Delete(gpio.pinInfo[pinNum].fdMonitorRef);
        gpio.pinInfo[pinNum].fdMonitorRef = NULL;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal function for local event handler.
 */
//--------------------------------------------------------------------------------------------------
static void localEventHandler(void* report)
{
    gpioStatus_t* gpioStatus = (gpioStatus_t*)report;

    // Check returned information and trigger the corresponding callback function
    if (gpioStatus != NULL)
    {
        le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&GpioHandlerList);

        while (linkHandlerPtr)
        {
            callbackInfo_t* callbackInfo =
                    CONTAINER_OF(linkHandlerPtr, callbackInfo_t, link);
            linkHandlerPtr = le_dls_PeekPrev(&GpioHandlerList, linkHandlerPtr);

            if (callbackInfo->handlerPtr && (callbackInfo->pinNum == gpioStatus->pinNum))
            {
                LE_INFO("Notify handlerPtr for pinNum %d", gpioStatus->pinNum);
                callbackInfo->handlerPtr(gpioStatus->pinNum, gpioStatus->edgeType);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal function for removing all the callback info.
 */
//--------------------------------------------------------------------------------------------------
static void removeAllCallbackInfo(void)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);

    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&GpioHandlerList);

    while (linkHandlerPtr)
    {
        callbackInfo_t* callbackInfo =
                CONTAINER_OF(linkHandlerPtr, callbackInfo_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&GpioHandlerList, linkHandlerPtr);

        le_dls_Remove(&GpioHandlerList, &(callbackInfo->link));
        le_mem_Release((void*)callbackInfo);
    }
}


// Initialization function
static void Init(void)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);

    // Create the callback info memory pools
    CallbackInfoPoolRef = le_mem_CreatePool("CallbackInfo", sizeof(callbackInfo_t));

    // 100 callbacks supported max
    le_mem_ExpandPool(CallbackInfoPoolRef, 100);

    // create an event for notificaiton
    gpioEvent = le_event_CreateId("GPIO Event", sizeof(gpioStatus_t));

    // Add handler for GPIO event
    le_event_AddHandler("GPIO Event Handler", gpioEvent, localEventHandler);

    int ret;
    struct gpiochip_info info;

    // open the device
    gpioFd = le_fd_Open(DEV_NAME, O_RDONLY);
    if (gpioFd < 0)
    {
        LE_ERROR("Cannot open GPIO device");
    }

    // Query GPIO chip information
    ret = ioctl(gpioFd, GPIO_GET_CHIPINFO_IOCTL, &info);
    if (ret == -1)
    {
        LE_ERROR("Cannot communicate with GPIO device");
        close(gpioFd);
    }

    gpio.numOfGpios = info.lines;

    // Limit GPIO pin control within 20 in maximum
    if(gpio.numOfGpios > MAX_PIN_NUMBER)
    {
        gpio.numOfGpios = MAX_PIN_NUMBER;
    }

    LE_INFO("System supports %u GPIO pins", gpio.numOfGpios);

    for(int i = 0; i < gpio.numOfGpios; ++i)
    {
        gpio.pinInfo[i].phyPinNum = physicalPinMap[i];
        gpio.pinInfo[i].fdMonitor = -1;
        snprintf(gpio.pinInfo[i].gpioName,
            sizeof(gpio.pinInfo[i].gpioName), "gpio%d", i);
        gpio.pinInfo[i].handlerCount = 0;
        gpio.pinInfo[i].fdMonitorRef = NULL;
        gpio.pinInfo[i].direction = GPIO_HAL_DIRECTION_UNKNOWN;
        gpio.pinInfo[i].edge = GPIO_HAL_EDGE_NONE;
        gpio.pinInfo[i].aliasName[0] = '\0';
    }
}

static void Release(void)
{
    LE_INFO("gpioVHalDrv: %s", __FUNCTION__);

    removeAllCallbackInfo();

    // relsease the pin resources
    for(int i = 0; i < gpio.numOfGpios; ++i)
    {
        if(gpio.pinInfo[i].fdMonitor != -1)
        {
            le_fd_Close(gpio.pinInfo[i].fdMonitor);
            gpio.pinInfo[i].fdMonitor = -1;
        }

        if(gpio.pinInfo[i].fdMonitorRef != NULL)
        {
            le_fdMonitor_Delete(gpio.pinInfo[i].fdMonitorRef);
            gpio.pinInfo[i].fdMonitorRef = NULL;
        }

        gpio.pinInfo[i].handlerCount = 0;
        gpio.pinInfo[i].direction = GPIO_HAL_DIRECTION_UNKNOWN;
        gpio.pinInfo[i].edge = GPIO_HAL_EDGE_NONE;
        gpio.pinInfo[i].aliasName[0] = '\0';
    }

    // Close the fd of gpiochip
    if(gpioFd != -1)
    {
        le_fd_Close(gpioFd);
        gpioFd = -1;
    }

    gpio.numOfGpios = 0;
}

LE_SHARED gpio_InfoTab_t TAF_HAL_INFO_TAB = {
    // always come first
    .mgrInf = {
        .name = TAF_GPIO_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .moduleType = TAF_MODULETYPE_HAL,
        .hwInitInf = taf_hal_HwInit,
        .powerOffInf = taf_hal_PowerOff,
        .powerOnInf = taf_hal_PowerOn,
        .selfTest = taf_hal_SelfTest,
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    },

    .gpioInf = {
        .initHAL = Init,
        .releaseHAL = Release,
        .getTotalGpioPins = taf_hal_GetTotalGpioPins,
        .registerCallback = taf_hal_RegisterCallback,
        .removeCallback = taf_hal_RemoveCallback,
        .sleep = taf_hal_Sleep,
        .wakeUp = taf_hal_Wakeup,
        .getDirection = taf_hal_GetDirection,
        .getPolarity = taf_hal_GetPolarity,
        .setPolarity = taf_hal_SetPolarity,
        .setDirection = taf_hal_SetDirection,
        .getState = taf_hal_GetState,
        .setOutputState = taf_hal_SetOutputState,
        .setEdgeSense = taf_hal_SetEdgeSense,
        .getEdgeSense = taf_hal_GetEdgeSense,
        .disableEdgeSense = taf_hal_DisableEdgeSense,
        .getName = taf_hal_GetName,
    },
};

// DOT NOT USE COMPONET_INIT define module init for your service/apps
COMPONENT_INIT
{
    // Do not put your specific init in here, define your init
    LE_INFO("Test Drv is loading");
}
