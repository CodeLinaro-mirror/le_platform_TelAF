/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include "tafGpio.hpp"
#include <sys/stat.h>

#define wakeupPinConfFile "/legato/systems/current/appsWriteable/tafGpioSvc/gpioWakeupPin.conf"
#define COMPONENT_NAME "tafGpioSvc"

static le_json_ParsingSessionRef_t JsonParsingSessionRef = nullptr;
static int jsonFd = -1;
static bool isWakeupPin;

using namespace tafsvc;

/**
 * Returns Gpio instance
 */
taf_Gpio &taf_Gpio::getInstance()
{
    static taf_Gpio instance;
    return instance;
}

int taf_Gpio::popen_call(const char *cmd)
{
    FILE *stream = nullptr;
    int rst = -1;

    stream = popen(cmd, "w");
    if (stream == nullptr)
    {
        LE_ERROR("cmd %s running failed", cmd);
        return rst;
    }

    rst = pclose(stream);
    if (WIFEXITED(rst))
    {
        rst = WEXITSTATUS(rst);
    }
    return rst;
}

void taf_Gpio::Init()
{
    LE_INFO("tafGpioSvc started");
    le_msg_AddServiceCloseHandler(taf_gpio_GetServiceRef(), OnClientDisconnection, nullptr);
    HandlerPool = le_mem_CreatePool("tafgpioHandlerPool", sizeof(taf_InterruptHandlerCtx_t));
    HandlerRefMap = le_ref_CreateMap("tafGpioHandler", MAX_TAF_GPIO_HANLDER*2);
    GpioHandlerList = LE_DLS_LIST_INIT;
    tafGpioEvent = le_event_CreateId("tafGpio Event",sizeof(taf_gpioEvent_t));
    le_event_AddHandler("tafgpio input pin interrupt", tafGpioEvent, callHandler);
    jsonFd = le_fd_Open(wakeupPinConfFile, O_RDONLY);
    if(jsonFd != -1)
        JsonParsingSessionRef = le_json_Parse(jsonFd, JsonEventHandler, JsonErrorHandler, nullptr);
    if(JsonParsingSessionRef == nullptr)
    {
        LE_ERROR("JsonParsingSessionRef is nullptr");
    }
}

le_result_t taf_Gpio::writeGpioOutputValue
(
    taf_GpioRef_t tafGpioRef,
    taf_gpio_State_t value
)
{

    TAF_ERROR_IF_RET_VAL(tafGpioRef == nullptr, LE_BAD_PARAMETER,
            "tafGpioRef is nullptr or gpio not initialized");
    if(isDrvPresent)
    {
        LE_DEBUG("Write value %d to GPIOPIN %d (OUTPUT mode)\n", value, tafGpioRef->pinNum);

        taf_hal_gpio_State state;
        switch (value)
        {
            case TAF_GPIO_HIGH:
                state = GPIO_HAL_STATE_HIGH;
                break;
            default:
                state = GPIO_HAL_STATE_LOW;
        }

        if((*(gpioInf->setOutputState)) == nullptr)
        {
            LE_ERROR("writeOutputValueHAL not initialized");
            return LE_IO_ERROR;
        }
        le_result_t status = (*(gpioInf->setOutputState))(tafGpioRef->pinNum, state);
        if(status == LE_BUSY)
        {
            return LE_BUSY;
        }
        else if(status == LE_IO_ERROR)
        {
            return LE_IO_ERROR;
        }

        LE_INFO("Succesfully wrote %d on GPIOPIN %d", value, tafGpioRef->pinNum);
        return LE_OK;
    }
    else
    {
        struct gpiohandle_request rq;
        struct gpiohandle_data data;
        int fd, ret;
        LE_DEBUG("Write value %d to GPIOPIN %d (OUTPUT mode)\n", value, tafGpioRef->pinNum);

        if (tafGpioRef->fdMonitor != -1) {
            le_fd_Close(tafGpioRef->fdMonitor);
            tafGpioRef->fdMonitor = -1;
        }

        fd = le_fd_Open(DEV_NAME, O_RDONLY);
        if (fd < 0)
        {
            LE_ERROR("Unabled to open %s: %s", DEV_NAME, strerror(errno));
            return LE_IO_ERROR;
        }
        rq.lineoffsets[0] = tafGpioRef->pinNum;
        rq.flags = GPIOHANDLE_REQUEST_OUTPUT;
        rq.lines = 1;
        snprintf(rq.consumer_label, sizeof(rq.consumer_label), COMPONENT_NAME);
        ret = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &rq);
        le_fd_Close(fd);
        if (ret == -1)
        {
            LE_ERROR("Unable to line handle from ioctl : %s", strerror(errno));
            return LE_IO_ERROR;
        }

        tafGpioRef->fdMonitor = rq.fd;
        data.values[0] = value;
        ret = ioctl(tafGpioRef->fdMonitor, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);

        if (ret == -1)
        {
            LE_ERROR("Unable to set line value using ioctl : %s", strerror(errno));
            le_fd_Close(rq.fd);
            tafGpioRef->fdMonitor = -1;
            return LE_BUSY;
        }
        else
        {
            LE_INFO("Succesfully wrote %d the GPIOPIN %d", data.values[0], tafGpioRef->pinNum);
            return LE_OK;
        }
    }
}

le_result_t taf_Gpio::setEdgeType
(
    taf_GpioRef_t tafGpioRef,
    taf_gpio_Edge_t tafEdge,
    le_fdMonitor_HandlerFunc_t fdMonFunc
)
{

    TAF_ERROR_IF_RET_VAL(tafGpioRef == nullptr, LE_BAD_PARAMETER,
            "tafGpioRef is nullptr or gpio not initialized");

    if (tafGpioRef->edge == tafEdge)
    {
        LE_INFO("Edge type is already set");
        return LE_OK;
    }
    if(tafGpioRef->fdMonitorRef != nullptr) {
        LE_INFO("Stopping fd monitor");
        le_fdMonitor_Delete(tafGpioRef->fdMonitorRef);
        tafGpioRef->fdMonitorRef = nullptr;
    }
    if (tafGpioRef->fdMonitor != -1)
    {
        le_fd_Close(tafGpioRef->fdMonitor);
        tafGpioRef->fdMonitor = -1;
    }

    if(isDrvPresent)
    {
        taf_hal_gpio_Edge edgeType;
        switch(tafEdge)
        {
            case TAF_GPIO_EDGE_NONE:
                edgeType = GPIO_HAL_EDGE_NONE;
                break;
            case TAF_GPIO_EDGE_RISING:
                edgeType = GPIO_HAL_EDGE_RISING;
                break;
            case TAF_GPIO_EDGE_FALLING:
                edgeType = GPIO_HAL_EDGE_FALLING;
                break;
            case TAF_GPIO_EDGE_BOTH:
                edgeType = GPIO_HAL_EDGE_BOTH;
                break;
            default:
                edgeType = GPIO_HAL_EDGE_UNKNOWN;
                break;
        }
        if((*(gpioInf->setEdgeSense)) == nullptr)
        {
            LE_ERROR("setEdgeSense not initialized");
            return LE_IO_ERROR;
        }
        le_result_t status = (*(gpioInf->setEdgeSense))(tafGpioRef->pinNum, edgeType);
        if(status == LE_BUSY)
        {
            return LE_BUSY;
        }
        else if(status == LE_IO_ERROR)
        {
            return LE_IO_ERROR;
        }
        LE_INFO("Successfully set the edge type");
        return LE_OK;
    }
    else
    {
        int fd, ret;
        struct gpioevent_request rq;

        // open the device
        fd = le_fd_Open(DEV_NAME, O_RDONLY);

        rq.lineoffset = tafGpioRef->pinNum;
        rq.handleflags = GPIOHANDLE_REQUEST_INPUT;
        snprintf(rq.consumer_label, sizeof(rq.consumer_label), COMPONENT_NAME);
        switch(tafEdge)
        {
            case TAF_GPIO_EDGE_RISING:
                rq.eventflags = GPIOEVENT_REQUEST_RISING_EDGE;
                break;
            case TAF_GPIO_EDGE_FALLING:
                rq.eventflags = GPIOEVENT_REQUEST_FALLING_EDGE;
                break;
            case TAF_GPIO_EDGE_BOTH:
                rq.eventflags = GPIOEVENT_REQUEST_BOTH_EDGES;
                break;
            default:
                rq.eventflags = GPIOEVENT_EVENT_FALLING_EDGE & GPIOEVENT_REQUEST_RISING_EDGE;
                break;
        }
        ret = ioctl(fd, GPIO_GET_LINEEVENT_IOCTL, &rq);
        le_fd_Close(fd);
        if (ret == -1)
        {
            LE_INFO("Unable to get line event from ioctl : %s", strerror(errno));
            return LE_IO_ERROR;
        }
        else
        {
            tafGpioRef->fdMonitorRef = le_fdMonitor_Create (tafGpioRef->gpioName, rq.fd, fdMonFunc,
                    POLLIN);
            tafGpioRef->edge = tafEdge;
            tafGpioRef->fdMonitor = rq.fd;
            LE_INFO("Successfully set the edge type");
            le_msg_SessionRef_t sessionRef = taf_gpio_GetClientSessionRef();
            le_hashmap_Put(tafGpioRef->clientHashMap, sessionRef, sessionRef);
            return LE_OK;
        }
    }
}

le_result_t taf_Gpio::setDirection
(
    taf_GpioRef_t tafGpioRef,
    taf_gpio_PinMode_t tafPinMode
)
{
    taf_gpio_PinMode_t currentMode;

    TAF_ERROR_IF_RET_VAL(tafGpioRef == nullptr, LE_BAD_PARAMETER,
            "tafGpioRef is nullptr or gpio not initialized");

    taf_Gpio gpio = getInstance();
    currentMode = (gpio.isInput(tafGpioRef)) ? GPIO_PIN_MODE_INPUT : GPIO_PIN_MODE_OUTPUT;
    if (currentMode == tafPinMode)
    {
        // Direction is already correct, do nothing.
        return LE_OK;
    }
    if (tafGpioRef->fdMonitor != -1)
    {
        le_fd_Close(tafGpioRef->fdMonitor);
        tafGpioRef->fdMonitor = -1;
    }

    if(isDrvPresent)
    {
        taf_hal_gpio_Direction dir = (tafPinMode == GPIO_PIN_MODE_OUTPUT) ?
            GPIO_HAL_DIRECTION_OUTPUT : GPIO_HAL_DIRECTION_INPUT;

        if((*(gpioInf->setDirection)) == nullptr)
        {
            LE_ERROR("setDirectionHAL not initialized");
            return LE_IO_ERROR;
        }
        le_result_t status = (*(gpioInf->setDirection))(tafGpioRef->pinNum, dir);
        if(status == LE_BUSY)
        {
            return LE_BUSY;
        }
        else if(status == LE_IO_ERROR)
        {
            return LE_IO_ERROR;
        }
        LE_INFO("Gpio %d direction changed to %s", tafGpioRef->pinNum,
                ((tafPinMode == GPIO_PIN_MODE_OUTPUT) ? "OUT" : "IN"));
        return LE_OK;
    }
    else
    {
        int fd, ret;
        fd = le_fd_Open(DEV_NAME, O_RDONLY);
        struct gpiohandle_request rq;
        rq.lineoffsets[0] = tafGpioRef->pinNum;
        rq.lines = 1;
        snprintf(rq.consumer_label, sizeof(rq.consumer_label), COMPONENT_NAME);
        rq.flags = (tafPinMode == GPIO_PIN_MODE_OUTPUT) ?
                GPIOHANDLE_REQUEST_OUTPUT : GPIOHANDLE_REQUEST_INPUT;
        ret = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &rq);
        le_fd_Close(fd);
        if (ret == -1)
        {
            LE_INFO("Unable to line handle from ioctl : %s", strerror(errno));
            return LE_IO_ERROR;
        }
        else
        {
            tafGpioRef->fdMonitor = rq.fd;
            LE_INFO("Gpio %d direction changed to %s", tafGpioRef->pinNum,
                    ((tafPinMode == GPIO_PIN_MODE_OUTPUT) ? "OUT" : "IN"));
            return LE_OK;
        }
    }
}

le_result_t taf_Gpio::setGpioAsInput
(
    taf_GpioRef_t tafGpioRef,
    taf_gpio_ActiveType_t polarity,
    bool lock
)
{

    le_msg_SessionRef_t sessionRef = taf_gpio_GetClientSessionRef();
    if (tafGpioRef->isLocked && tafGpioRef->lockedSession != sessionRef) {
        LE_WARN("Attemp to use gpio pin %d, which is locked", tafGpioRef->pinNum);
        return LE_BUSY;
    }

    le_result_t result = LE_OK;

    result = setDirection(tafGpioRef, GPIO_PIN_MODE_INPUT);

    if (LE_OK != result)
    {
        LE_ERROR("Failed to set GPIO %s as input", tafGpioRef->gpioName);
        return result;
    }

    tafGpioRef->isLocked = lock;
    tafGpioRef->lockedSession = sessionRef;

    le_hashmap_Put(tafGpioRef->clientHashMap, sessionRef, sessionRef);

    if (getPolarity(tafGpioRef) != polarity)
    {
        return setPolarity(tafGpioRef, polarity);
    }
    return LE_OK;
}

le_result_t taf_Gpio::setPolarity
(
    taf_GpioRef_t tafGpioRef,
    taf_gpio_ActiveType_t level
)
{
    TAF_ERROR_IF_RET_VAL(tafGpioRef == nullptr, LE_BAD_PARAMETER,
            "tafGpioRef is nullptr or gpio not initialized");
    if(isDrvPresent)
    {
        taf_hal_gpio_ActiveType_t type = GPIO_HAL_ACTIVE_TYPE_UNKNOWN;
        if(level == GPIO_ACTIVE_TYPE_LOW)
        {
            type = GPIO_HAL_ACTIVE_TYPE_LOW;
        }
        else if(level == GPIO_ACTIVE_TYPE_HIGH)
        {
            type = GPIO_HAL_ACTIVE_TYPE_HIGH;
        }

        if((*(gpioInf->setPolarity)) == nullptr)
        {
            LE_ERROR("setPolarityHAL not initialized");
            return LE_IO_ERROR;
        }

        le_result_t status = (*(gpioInf->setPolarity))(tafGpioRef->pinNum, type);
        if(status == LE_BUSY)
        {
            return LE_BUSY;
        }
        else if(status == LE_IO_ERROR)
        {
            return LE_IO_ERROR;
        }

        LE_INFO("Succesfully set the polarity");
        return LE_OK;
    }
    else
    {
        int fd, ret;
        struct gpiohandle_request rq;
        fd = le_fd_Open(DEV_NAME, O_RDONLY);
        if (fd < 0)
        {
            LE_ERROR("Unabled to open %s: %s", DEV_NAME, strerror(errno));
            return LE_IO_ERROR;
        }
        if (tafGpioRef->fdMonitor != -1)
        {
            le_fd_Close(tafGpioRef->fdMonitor);
            tafGpioRef->fdMonitor = -1;
        }
        rq.lineoffsets[0] = tafGpioRef->pinNum;
        if(level == GPIO_ACTIVE_TYPE_LOW)
        {
            rq.flags = GPIOHANDLE_REQUEST_ACTIVE_LOW|GPIOHANDLE_REQUEST_INPUT;
        }
        else if(level == GPIO_ACTIVE_TYPE_HIGH)
        {
            rq.flags = GPIOHANDLE_REQUEST_INPUT;
        }
        rq.lines = 1;
        snprintf(rq.consumer_label, sizeof(rq.consumer_label), COMPONENT_NAME);
        ret = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &rq);
        le_fd_Close(fd);
        if (ret == -1)
        {
            LE_ERROR("Unable to get line handle from ioctl : %s", strerror(errno));
            return LE_IO_ERROR;
        }
        else
        {
            LE_INFO("Succesfully set the polarity");
            tafGpioRef->fdMonitor = rq.fd;
        }

        return LE_OK;
    }
}

static void gpioStateCb(uint8_t pin, taf_hal_gpio_Edge edgeType)
{
    LE_INFO("*******gpio status change for pin %d", pin);
    auto &gpio = taf_Gpio::getInstance();

    taf_gpioEvent_t event = {-1, (edgeType == GPIO_HAL_EDGE_RISING), pin};
    le_event_Report(gpio.tafGpioEvent, &event, sizeof(taf_gpioEvent_t));
}

void* taf_Gpio::setChangeCallback
(
    taf_GpioRef_t tafGpioRef,
    le_fdMonitor_HandlerFunc_t fdMonFunc,
    taf_gpio_Edge_t edge,
    bool lock,
    taf_gpio_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr
)
{
    TAF_ERROR_IF_RET_VAL(tafGpioRef == nullptr, nullptr,
            "tafGpioRef is nullptr or object not initialized");

    le_msg_SessionRef_t sessionRef = taf_gpio_GetClientSessionRef();
    if (tafGpioRef->isLocked && tafGpioRef->lockedSession != sessionRef) {
        LE_WARN("Attemp to use gpio pin %d, which is locked", tafGpioRef->pinNum);
        return nullptr;
    }

    tafGpioRef->isLocked = lock;
    tafGpioRef->lockedSession = sessionRef;

    // Store the callback function and context pointer
    taf_InterruptHandlerCtx_t * handlerCtxPtr =
            (taf_InterruptHandlerCtx_t *)le_mem_ForceAlloc(HandlerPool);
    handlerCtxPtr->handlerPtr = handlerPtr;
    handlerCtxPtr->usrContext = contextPtr;
    handlerCtxPtr->pinNum = tafGpioRef->pinNum;
    handlerCtxPtr->handlerRef =
            (taf_gpio_ChangeEventHandlerRef_t)le_ref_CreateRef(HandlerRefMap, handlerCtxPtr);
    handlerCtxPtr->sessionCtxPtr = sessionRef;
    handlerCtxPtr->link = LE_DLS_LINK_INIT;

    le_dls_Queue(&GpioHandlerList, &handlerCtxPtr->link);
    tafGpioRef->handlerCount++;

    if(isDrvPresent)
    {
        if((*(gpioInf->registerCallback)) == nullptr)
        {
            LE_ERROR("regCallbackHAL not initialized");
            return nullptr;
        }

        taf_hal_gpio_Edge edgeType;
        switch(edge)
        {
            case TAF_GPIO_EDGE_NONE:
                edgeType = GPIO_HAL_EDGE_NONE;
                break;
            case TAF_GPIO_EDGE_RISING:
                edgeType = GPIO_HAL_EDGE_RISING;
                break;
            case TAF_GPIO_EDGE_FALLING:
                edgeType = GPIO_HAL_EDGE_FALLING;
                break;
            case TAF_GPIO_EDGE_BOTH:
                edgeType = GPIO_HAL_EDGE_BOTH;
                break;
            default:
                edgeType = GPIO_HAL_EDGE_UNKNOWN;
                break;
        }

        (*(gpioInf->registerCallback))(tafGpioRef->pinNum, edgeType, gpioStateCb);
    }
    else
    {
        if (tafGpioRef->fdMonitorRef != nullptr && edge == tafGpioRef->edge) {
            LE_INFO("Monitor is already created for the same edge type for GPIO %d",
                    tafGpioRef->pinNum);
            le_hashmap_Put(tafGpioRef->clientHashMap, sessionRef, sessionRef);
            return handlerCtxPtr->handlerRef;
        }
        // Start monitoring the fd for the correct GPIO
        int fd, ret;
        struct gpioevent_request rq;
        fd = le_fd_Open(DEV_NAME, O_RDONLY);
        if (fd < 0)
        {
            LE_ERROR("Unabled to open %s: %s", DEV_NAME, strerror(errno));
            return nullptr;
        }
        if(tafGpioRef->fdMonitorRef != nullptr)
        {
            LE_INFO("Stopping fd monitor");
            le_fdMonitor_Delete(tafGpioRef->fdMonitorRef);
            tafGpioRef->fdMonitorRef = nullptr;
        }
        if(tafGpioRef->fdMonitor != -1)
        {
            le_fd_Close(tafGpioRef->fdMonitor);
            tafGpioRef->fdMonitor = -1;
        }
        rq.lineoffset = tafGpioRef->pinNum;
        if (edge == TAF_GPIO_EDGE_RISING)
        {
            rq.eventflags = GPIOEVENT_REQUEST_RISING_EDGE;
        }
        else if(edge == TAF_GPIO_EDGE_FALLING)
        {
            rq.eventflags = GPIOEVENT_REQUEST_FALLING_EDGE;
        }
        else if(edge == TAF_GPIO_EDGE_BOTH)
        {
            rq.eventflags = GPIOEVENT_REQUEST_BOTH_EDGES;
        }
        else if(edge == TAF_GPIO_EDGE_NONE)
        {
            rq.eventflags = GPIOEVENT_REQUEST_RISING_EDGE & GPIOEVENT_REQUEST_FALLING_EDGE;
        }
        rq.handleflags = GPIOHANDLE_REQUEST_INPUT;
        snprintf(rq.consumer_label, sizeof(rq.consumer_label), COMPONENT_NAME);
        ret = ioctl(fd, GPIO_GET_LINEEVENT_IOCTL, &rq);
        le_fd_Close(fd);
        if (ret == -1)
        {
            LE_ERROR("Unable to get line event from ioctl : %s", strerror(errno));
            return nullptr;
        }

        tafGpioRef->fdMonitorRef = le_fdMonitor_Create (tafGpioRef->gpioName, rq.fd, fdMonFunc,
                POLLIN);
        tafGpioRef->fdMonitor = rq.fd;
    }

    tafGpioRef->edge = edge;
    le_hashmap_Put(tafGpioRef->clientHashMap, sessionRef, sessionRef);
    return handlerCtxPtr->handlerRef;
}

void taf_Gpio::removeChangeCallback
(
    void * handlerRef
)
{
    TAF_ERROR_IF_RET_NIL(nullptr == handlerRef, "Invalid handler reference provided");

    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&GpioHandlerList);
    taf_GpioRef_t gpioRef = nullptr;
    while (linkHandlerPtr)
    {
        taf_InterruptHandlerCtx_t * handlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_InterruptHandlerCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&GpioHandlerList, linkHandlerPtr);
        if (handlerCtxPtr && handlerCtxPtr->handlerRef == handlerRef)
        {
            le_ref_DeleteRef(HandlerRefMap, handlerRef);
            le_dls_Remove(&GpioHandlerList, &(handlerCtxPtr->link));
            le_mem_Release((void*)handlerCtxPtr);
            gpioRef = tafGpioRefPin[handlerCtxPtr->pinNum];
            gpioRef->handlerCount--;
        }
    }

    if (isDrvPresent)
    {
        if ((*(gpioInf->removeCallback)) == nullptr)
        {
            LE_ERROR("removeCallback not initialized");
            return;
        }

        if (gpioRef == nullptr)
        {
            LE_ERROR("gpioRef not initialized");
            return;
        }

        taf_hal_gpio_Edge edgeType;
        switch(gpioRef->edge)
        {
            case TAF_GPIO_EDGE_NONE:
                edgeType = GPIO_HAL_EDGE_NONE;
                break;
            case TAF_GPIO_EDGE_RISING:
                edgeType = GPIO_HAL_EDGE_RISING;
                break;
            case TAF_GPIO_EDGE_FALLING:
                edgeType = GPIO_HAL_EDGE_FALLING;
                break;
            case TAF_GPIO_EDGE_BOTH:
                edgeType = GPIO_HAL_EDGE_BOTH;
                break;
            default:
                edgeType = GPIO_HAL_EDGE_UNKNOWN;
                break;
        }

        (*(gpioInf->removeCallback))(gpioRef->pinNum, edgeType);
    }
    else
    {
        if (gpioRef && gpioRef->handlerCount == 0 && gpioRef->fdMonitorRef != nullptr) {
                LE_INFO("Stopping fd monitor");
                le_fdMonitor_Delete(gpioRef->fdMonitorRef);
                gpioRef->fdMonitorRef = nullptr;
        }
    }
    LE_INFO("removeChangeCallback handlerRef");
}

le_result_t taf_Gpio::disableEdgeSense
(
    taf_GpioRef_t tafGpioRef,
    bool lock
)
{
    le_msg_SessionRef_t sessionRef = taf_gpio_GetClientSessionRef();
    if (tafGpioRef->isLocked && tafGpioRef->lockedSession != sessionRef) {
        LE_WARN("Attemp to use gpio pin %d, which is locked", tafGpioRef->pinNum);
        return LE_BUSY;
    }

    tafGpioRef->isLocked = lock;
    tafGpioRef->lockedSession = sessionRef;
    le_hashmap_Put(tafGpioRef->clientHashMap, sessionRef, sessionRef);

    return setEdgeSense(tafGpioRef, TAF_GPIO_EDGE_NONE, lock, nullptr);
}

taf_gpio_State_t taf_Gpio::readValue
(
    taf_GpioRef_t tafGpioRef,
    bool lock
)
{
    int result;

    TAF_ERROR_IF_RET_VAL(!tafGpioRef, TAF_GPIO_BUSY,
            "gpioRef is nullptr or object not initialized");

    le_msg_SessionRef_t sessionRef = taf_gpio_GetClientSessionRef();
    if (tafGpioRef->isLocked && tafGpioRef->lockedSession != sessionRef) {
        LE_WARN("Attemp to use gpio pin %d, which is locked", tafGpioRef->pinNum);
        return TAF_GPIO_BUSY;
    }

    if (isOutput(tafGpioRef))
    {
        LE_ERROR("Attemp to read the pin which is not input");
        return TAF_GPIO_BUSY;
    }
    if(isDrvPresent)
    {
        tafGpioRef->isLocked = lock;
        tafGpioRef->lockedSession = sessionRef;

        if((*(gpioInf->getState)) == nullptr)
        {
            LE_ERROR("getValueHAL not initialized");
            return TAF_GPIO_BUSY;
        }
        result = (*(gpioInf->getState))(tafGpioRef->pinNum);
        if(result < 0)
        {
            return TAF_GPIO_BUSY;
        }
        LE_INFO("result:%d Value:%s", result, (result == 1) ? "high": "low");
        le_hashmap_Put(tafGpioRef->clientHashMap, sessionRef, sessionRef);
        return (taf_gpio_State_t)result;
    }
    else
    {
        taf_gpio_State_t type;
        int fd,ret;
        struct gpiohandle_request rq;
        struct gpiohandle_data data;
        if (tafGpioRef->fdMonitor == -1) {
            fd = le_fd_Open(DEV_NAME, O_RDONLY);
            if (fd < 0)
            {
                LE_ERROR("Unabled to open %s: %s", DEV_NAME, strerror(errno));
                return TAF_GPIO_BUSY;
            }
            rq.lineoffsets[0] = tafGpioRef->pinNum;
            rq.flags = GPIOHANDLE_REQUEST_INPUT;
            rq.lines = 1;
            snprintf(rq.consumer_label, sizeof(rq.consumer_label), COMPONENT_NAME);
            ret = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &rq);
            le_fd_Close(fd);
            if (ret == -1)
            {
                LE_ERROR("Unable to get line handle from ioctl : %s", strerror(errno));
                return TAF_GPIO_BUSY;
            }
            tafGpioRef->fdMonitor = rq.fd;
        }
        // Get the value
        ret = ioctl(tafGpioRef->fdMonitor, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
        LE_INFO("ioctl fd is %d",tafGpioRef->fdMonitor);
        if (ret == -1)
        {
            LE_ERROR("Unable to get line value using ioctl : %s", strerror(errno));
            return TAF_GPIO_BUSY;
        }
        else
        {
            LE_INFO("Value of GPIO pin %d (INPUT mode) on chip %s: %d\n", tafGpioRef->pinNum, DEV_NAME,
                    data.values[0]);
            result = data.values[0];
        }

        tafGpioRef->isLocked = lock;
        tafGpioRef->lockedSession = sessionRef;

        type = (taf_gpio_State_t)result;
        LE_INFO("result:%d Value:%s", result, (type==1) ? "high": "low");
        le_hashmap_Put(tafGpioRef->clientHashMap, sessionRef, sessionRef);

        return type;
    }
}

le_result_t taf_Gpio::activate
(
    taf_GpioRef_t tafGpioRef,
    bool lock
)
{
    le_msg_SessionRef_t sessionRef = taf_gpio_GetClientSessionRef();
    if(tafGpioRef->isLocked && tafGpioRef->lockedSession != sessionRef) {
        LE_WARN("Attemp to use gpio pin %d, which is locked", tafGpioRef->pinNum);
        return LE_BUSY;
    }

    // Stop monitoring the trigger if running
    if(tafGpioRef->fdMonitorRef != nullptr)
    {
        LE_INFO("Stopping fd monitor");
        le_fdMonitor_Delete(tafGpioRef->fdMonitorRef);
        tafGpioRef->fdMonitorRef = nullptr;
    }
    if (tafGpioRef->fdMonitor != -1)
    {
        le_fd_Close(tafGpioRef->fdMonitor);
        tafGpioRef->fdMonitor = -1;
    }

    TAF_ERROR_IF_RET_VAL(LE_OK != writeGpioOutputValue(tafGpioRef, TAF_GPIO_HIGH),
            LE_IO_ERROR, "Failed to set GPIO %s to high", tafGpioRef->gpioName);

    tafGpioRef->isLocked = lock;
    tafGpioRef->lockedSession = sessionRef;

    le_hashmap_Put(tafGpioRef->clientHashMap, sessionRef, sessionRef);

    return LE_OK;
}

le_result_t taf_Gpio::deactivate
(
    taf_GpioRef_t tafGpioRef,
    bool lock
)
{

    le_msg_SessionRef_t sessionRef = taf_gpio_GetClientSessionRef();
    if(tafGpioRef->isLocked && tafGpioRef->lockedSession != sessionRef) {
        LE_WARN("Attemp to use gpio pin %d, which is locked", tafGpioRef->pinNum);
        return LE_BUSY;
    }

    // Stop monitoring the trigger if running
    if(tafGpioRef->fdMonitorRef != nullptr)
    {
        LE_INFO("Stopping fd monitor");
        le_fdMonitor_Delete(tafGpioRef->fdMonitorRef);
        tafGpioRef->fdMonitorRef = nullptr;
    }
    if (tafGpioRef->fdMonitor != -1)
    {
        le_fd_Close(tafGpioRef->fdMonitor);
        tafGpioRef->fdMonitor = -1;
    }

    TAF_ERROR_IF_RET_VAL(LE_OK != writeGpioOutputValue(tafGpioRef, TAF_GPIO_LOW),
            LE_IO_ERROR, "Failed to set GPIO %s to low", tafGpioRef->gpioName);

    le_hashmap_Put(tafGpioRef->clientHashMap, sessionRef, sessionRef);

    tafGpioRef->isLocked = lock;
    tafGpioRef->lockedSession = sessionRef;

    return LE_OK;
}

bool taf_Gpio::isActive
(
    taf_GpioRef_t tafGpioRef
)
{
    if (isInput(tafGpioRef))
    {
        LE_WARN("Attempt to check if an input gpio pin is active");
        return false;
    }

    if(isDrvPresent)
    {
        if((*(gpioInf->getState)) == nullptr)
        {
            LE_ERROR("getValueHAL not initialized");
            return false;
        }
        return (*(gpioInf->getState))(tafGpioRef->pinNum) == 1;
    }
    else
    {
        if (tafGpioRef-> fdMonitor == -1)
        {
            int fd, ret;
            struct gpiohandle_request rq;
            fd = le_fd_Open(DEV_NAME, O_RDONLY);
            if (fd < 0)
            {
                LE_ERROR("Unabled to open %s: %s", DEV_NAME, strerror(errno));
                return false;
            }

            rq.lineoffsets[0] = tafGpioRef->pinNum;
            rq.flags = GPIOHANDLE_REQUEST_OUTPUT;
            rq.lines = 1;
            snprintf(rq.consumer_label, sizeof(rq.consumer_label), COMPONENT_NAME);
            ret = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &rq);
            le_fd_Close(fd);
            if (ret == -1)
            {
                LE_ERROR("Unable to line handle from ioctl : %s", strerror(errno));
                return false;
            }
            tafGpioRef->fdMonitor = rq.fd;
        }
        // Get the gpio line values (0/1)
        int value, ret;
        struct gpiohandle_data data;
        ret = ioctl(tafGpioRef->fdMonitor, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);

        if (ret == -1)
        {
            LE_ERROR("Unable to get line value using ioctl : %s", strerror(errno));
            return false;
        }
        else
        {
            value = data.values[0];
            LE_INFO("Value of GPIO at offset %d on chip %s: value is %d", tafGpioRef->pinNum, DEV_NAME,
                    value);
            le_msg_SessionRef_t sessionRef = taf_gpio_GetClientSessionRef();
            le_hashmap_Put(tafGpioRef->clientHashMap, sessionRef, sessionRef);
            return value == 1;
        }
    }
}

bool taf_Gpio::isInput
(
    taf_GpioRef_t tafGpioRef
)
{
    TAF_ERROR_IF_RET_VAL(!tafGpioRef, false,
            "tafGpioRef is nullptr or object not initialized");

    if(isDrvPresent)
    {
        if((*(gpioInf->getDirection)) == nullptr)
        {
            LE_ERROR("getDirectionHAL not initialized");
            return false;
        }
        return (*(gpioInf->getDirection))(tafGpioRef->pinNum) == GPIO_HAL_DIRECTION_INPUT;
    }
    else
    {
        int fd, ret;
        struct gpioline_info line_info;
        fd = le_fd_Open(DEV_NAME, O_RDONLY);
        if (fd < 0)
        {
            LE_ERROR("Unabled to open %s: %s", DEV_NAME, strerror(errno));
            return false;
        }
        line_info.line_offset = tafGpioRef->pinNum;
        // Get the gpio line info (IN/OUT)
        ret = ioctl(fd, GPIO_GET_LINEINFO_IOCTL, &line_info);
        le_fd_Close(fd);
        if (ret == -1)
        {
            LE_ERROR("Unable to get line info from offset %d:%s", tafGpioRef->pinNum, strerror(errno));
            return false;
        }
        else
        {
            LE_INFO("the gpiopin %d isInput %d",tafGpioRef->pinNum,
                    (line_info.flags & GPIOLINE_FLAG_IS_OUT) ? 0 : 1);
            return (line_info.flags & GPIOLINE_FLAG_IS_OUT) ? false : true;
        }
    }
}

bool taf_Gpio::isOutput
(
    taf_GpioRef_t tafGpioRef
)
{
    TAF_ERROR_IF_RET_VAL(!tafGpioRef, false,
            "gpioRef is nullptr or object not initialized");

    if(isDrvPresent)
    {
        if((*(gpioInf->getDirection)) == nullptr)
        {
            LE_ERROR("getDirectionHAL not initialized");
            return false;
        }
        return (*(gpioInf->getDirection))(tafGpioRef->pinNum) == GPIO_HAL_DIRECTION_OUTPUT;
    }
    else
    {
        int fd, ret;
        struct gpioline_info line_info;
        fd = le_fd_Open(DEV_NAME, O_RDONLY);
        if (fd < 0)
        {
            LE_ERROR("Unabled to open %s: %s", DEV_NAME, strerror(errno));
            return false;
        }
        line_info.line_offset = tafGpioRef->pinNum;
        // Get the gpio line info (IN/OUT)
        ret = ioctl(fd, GPIO_GET_LINEINFO_IOCTL, &line_info);
        le_fd_Close(fd);
        if (ret == -1)
        {
            LE_ERROR("Unable to get line info from offset %d:%s", tafGpioRef->pinNum, strerror(errno));
            return false;
        }
        else
        {
            LE_INFO("the gpiopin value isOut %d",(line_info.flags & GPIOLINE_FLAG_IS_OUT) ? 1 : 0);
            return (line_info.flags & GPIOLINE_FLAG_IS_OUT);
        }
    }
}

le_result_t taf_Gpio::getName
(
    taf_GpioRef_t   tafGpioRef,
    char*           name, /* output */
    size_t          nameSize
)
{
    if(name == nullptr)
    {
        LE_ERROR("name ptr is null");
        return LE_BAD_PARAMETER;
    }

    if(isDrvPresent)
    {
        if((*(gpioInf->getName)) == nullptr)
        {
            LE_ERROR("getNameHAL not initialized");
            return LE_FAULT;
        }
        le_utf8_Copy(name, (*(gpioInf->getName))(tafGpioRef->pinNum), nameSize, nullptr);;
    }
    else
    {
        if(strlen(tafGpioRef->aliasName) > 0)
        {
            le_utf8_Copy(name, tafGpioRef->aliasName, nameSize, nullptr);
        }
        else
        {
            return LE_NOT_IMPLEMENTED;
        }
    }

    
    return LE_OK;
}

taf_gpio_ActiveType_t taf_Gpio::getPolarity
(
    taf_GpioRef_t tafGpioRef
)
{

    TAF_ERROR_IF_RET_VAL(!tafGpioRef, GPIO_ACTIVE_TYPE_UNKNOWN,
            "tafGpioRef is nullptr or object not initialized");

    if(isDrvPresent)
    {
        taf_gpio_ActiveType_t result;
        if((*(gpioInf->getPolarity)) == nullptr)
        {
            LE_ERROR("getPolarityHAL not initialized");
            return GPIO_ACTIVE_TYPE_UNKNOWN;
        }
        taf_hal_gpio_ActiveType_t type = (*(gpioInf->getPolarity))(tafGpioRef->pinNum);
        switch(type)
        {
            case GPIO_HAL_ACTIVE_TYPE_LOW:
                result = GPIO_ACTIVE_TYPE_LOW;
                break;
            case GPIO_HAL_ACTIVE_TYPE_HIGH:
                result = GPIO_ACTIVE_TYPE_HIGH;
                break;
            default:
                result = GPIO_ACTIVE_TYPE_UNKNOWN;
        }
        return result;
    }
    else
    {
        int fd, ret;
        struct gpioline_info line_info;
        fd = le_fd_Open(DEV_NAME, O_RDONLY);
        if (fd < 0)
        {
            LE_ERROR("Unabled to open %s: %s", DEV_NAME, strerror(errno));
            return GPIO_ACTIVE_TYPE_UNKNOWN;
        }
        line_info.line_offset = tafGpioRef->pinNum;
        snprintf(line_info.consumer, sizeof(line_info.consumer), COMPONENT_NAME);
        ret = ioctl(fd, GPIO_GET_LINEINFO_IOCTL, &line_info);
        le_fd_Close(fd);
        if (ret == -1)
        {
            LE_ERROR("Unable to get line info from offset %d:%s", tafGpioRef->pinNum, strerror(errno));
            return GPIO_ACTIVE_TYPE_UNKNOWN;
        }
        else
        {
            LE_DEBUG("the gpiopin active_low is %d", (line_info.flags & GPIOLINE_FLAG_ACTIVE_LOW)
                    ? GPIO_ACTIVE_TYPE_LOW : GPIO_ACTIVE_TYPE_HIGH);
            return (line_info.flags & GPIOLINE_FLAG_ACTIVE_LOW)
                    ? GPIO_ACTIVE_TYPE_LOW : GPIO_ACTIVE_TYPE_HIGH;
        }
    }
}

taf_gpio_Edge_t taf_Gpio::getEdgeSense
(
    taf_GpioRef_t tafGpioRef
)
{
    TAF_ERROR_IF_RET_VAL(!tafGpioRef, TAF_GPIO_EDGE_UNKNOWN,
            "tafGpioRef is nullptr or object not initialized");

    if(isDrvPresent)
    {
        taf_gpio_Edge_t result;
        if((*(gpioInf->getEdgeSense)) == nullptr)
        {
            LE_ERROR("getEdgeSense not initialized");
            return TAF_GPIO_EDGE_UNKNOWN;
        }
        taf_hal_gpio_Edge type = (*(gpioInf->getEdgeSense))(tafGpioRef->pinNum);
        switch(type)
        {
            case GPIO_HAL_EDGE_FALLING:
                result = TAF_GPIO_EDGE_FALLING;
                break;
            case GPIO_HAL_EDGE_RISING:
                result = TAF_GPIO_EDGE_RISING;
                break;
            case GPIO_HAL_EDGE_BOTH:
                result = TAF_GPIO_EDGE_BOTH;
                break;
            default:
                result = TAF_GPIO_EDGE_NONE;
        }
        return result;
    }
    else
    {
        // Edge type is not valid for OUT pin
        if (isOutput(tafGpioRef))
        {
            LE_WARN("Attempt to read edge sense on an output");
            return TAF_GPIO_EDGE_NONE;
        }
    }

    return tafGpioRef->edge;
}

le_result_t taf_Gpio::setEdgeSense
(
    taf_GpioRef_t tafGpioRef,
    taf_gpio_Edge_t edge,
    bool lock,
    le_fdMonitor_HandlerFunc_t fdMonFunc
)
{

    le_msg_SessionRef_t sessionRef = taf_gpio_GetClientSessionRef();
    if (tafGpioRef->isLocked && tafGpioRef->lockedSession != sessionRef) {
        LE_WARN("Attemp to use gpio pin %d, which is locked", tafGpioRef->pinNum);
        return LE_BUSY;
    }

    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&GpioHandlerList);
    int registeredHandlers = 0;
    while (linkHandlerPtr)
    {
        taf_InterruptHandlerCtx_t * handlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_InterruptHandlerCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&GpioHandlerList, linkHandlerPtr);
        if (handlerCtxPtr->sessionCtxPtr == sessionRef
                && (handlerCtxPtr->pinNum == tafGpioRef->pinNum))
        {
            registeredHandlers++;
        }
    }
    if (registeredHandlers == 0)
    {
        LE_ERROR("Attempt to change edge sense value without a registered handler");
        return LE_FAULT;
    }

    tafGpioRef->isLocked = lock;
    tafGpioRef->lockedSession = sessionRef;

    return setEdgeType(tafGpioRef, edge, fdMonFunc);
}

void taf_Gpio::callHandler(void* reportPtr) {
    taf_gpioEvent_t *eventPtr = (taf_gpioEvent_t *)reportPtr;
    taf_Gpio gpio = taf_Gpio::getInstance();
    gpio.callClientHandlerFunc(eventPtr);
}

void taf_Gpio::callClientHandlerFunc(taf_gpioEvent_t *eventPtr)
{
    TAF_ERROR_IF_RET_NIL(eventPtr == nullptr, "eventPtr is nullptr");

    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&GpioHandlerList);
    taf_GpioRef_t gpioRef = tafGpioRefPin[eventPtr->pinNum];
    while (linkHandlerPtr)
    {
        taf_InterruptHandlerCtx_t * handlerCtxPtr =
                CONTAINER_OF(linkHandlerPtr, taf_InterruptHandlerCtx_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&GpioHandlerList, linkHandlerPtr);
        if (gpioRef->isLocked && gpioRef->lockedSession != handlerCtxPtr->sessionCtxPtr) {
            continue;
        }
        if (handlerCtxPtr->handlerPtr && (handlerCtxPtr->pinNum == eventPtr->pinNum))
        {
            LE_INFO("Notify handlerPtr for pinNum %d", eventPtr->pinNum);
            handlerCtxPtr->handlerPtr(eventPtr->pinNum, eventPtr->state,
                    handlerCtxPtr->usrContext);
        }
    }
}

void taf_Gpio::inputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    taf_GpioRef_t tafGpioRef = nullptr;
    taf_Gpio gpio = getInstance();
    for(int i=0; i < gpio.numOfGpios; i++) {
        if(tafGpioRefPin[i]->fdMonitor == fd) {
            tafGpioRef = tafGpioRefPin[i];
        }
    }

    // Make sure the pin is in use and has listeners, this isn't a spurious interrupt
    if (!tafGpioRef || tafGpioRef->handlerCount == 0)
    {
        LE_WARN("Spurious interrupt handled - ignoring");
        return;
    }

    struct gpioevent_data event;
    int ret = le_fd_Read(fd, &event, sizeof(event));
    if (ret == -1) {
        if (errno == -EAGAIN)
        {
            LE_ERROR("nothing available\n");
        }
        else
        {
            ret = -errno;
            LE_ERROR("Failed to read event (%d)\n", ret);
        }
    }

    if (ret != sizeof(event))
    {
        LE_ERROR("Reading event failed\n");
        ret = -EIO;
    }
    switch (event.id) {
    case GPIOEVENT_EVENT_RISING_EDGE:
        LE_INFO("rising edge");
        break;
    case GPIOEVENT_EVENT_FALLING_EDGE:
        LE_INFO( "falling edge");
        break;
    default:
        LE_ERROR("unknown event");
    }
    taf_gpioEvent_t fallingEvt = {fd, event.id == GPIOEVENT_EVENT_RISING_EDGE, tafGpioRef->pinNum};
    le_event_Report(gpio.tafGpioEvent, &fallingEvt, sizeof(taf_gpioEvent_t));

    return;
}

/**
 * Callback on client disconnection
 */
void taf_Gpio::OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *ctxPtr)
{
    taf_Gpio gpio = getInstance();
    LE_INFO("OnClientDisconnection");

    for(int i=0; i < gpio.numOfGpios; i++) {
        // reset the gpio ref data on client disconnection
        taf_GpioRef_t gpioRef = gpio.tafGpioRefPin[i];
        if(gpioRef->lockedSession == sessionRef) {
            gpioRef->isLocked = false;
            gpioRef->lockedSession = nullptr;
        }
        le_hashmap_Remove(gpioRef->clientHashMap, sessionRef);
        if (le_hashmap_isEmpty(gpioRef->clientHashMap) && gpioRef->fdMonitor != -1)
        {
            le_fd_Close(gpioRef->fdMonitor);
            LE_INFO("hash map size is 0 close fd %d", gpioRef->fdMonitor);
            gpioRef->fdMonitor = -1;
        }
    }
}

void taf_Gpio::JsonEventHandler(le_json_Event_t event)
{
    const char* memberName;
    taf_Gpio gpio = getInstance();
    switch (event)
    {
        case LE_JSON_ARRAY_END:
            if(isWakeupPin)
                isWakeupPin = false;
            break;

        case LE_JSON_OBJECT_MEMBER:
            memberName = le_json_GetString();
            if(strcmp(memberName, "wakeupPins") == 0) {
                isWakeupPin = true;
            }
            break;

        case LE_JSON_NUMBER:
            if(isWakeupPin) {
                int gpioPin = (int)le_json_GetNumber();
                if(gpioPin < 0 || gpioPin > gpio.numOfGpios) {
                    LE_ERROR("Read invalid gpio pin %d from conf file", gpioPin);
                    break;
                }
                char cmd[1024];
                char path[512];
                DIR *dir;
                struct dirent *entry;

                dir = opendir("/usr/lib/modules/");
                if (dir == NULL) {
                    perror("opendir");
                    return;
                }

                while ((entry = readdir(dir)) != NULL) {
                    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                        continue;
                    }
                    snprintf(path, sizeof(path), "/usr/lib/modules/%s", entry->d_name);
                    LE_INFO("path is %s", path);
                    if (strcmp(entry->d_name, "gpioWakeup.ko") == 0) {
                     LE_INFO("KO module is available");
                        snprintf(cmd, sizeof(cmd),
                                "/sbin/insmod %s gpioChipName=\"f000000.pinctrl\""
                                " gpioOffset=%d", path, gpioPin);
                        popen_call(cmd);
                        snprintf(cmd, sizeof(cmd), "/sbin/rmmod %s", path);
                        popen_call(cmd);
                        closedir(dir);
                        return;
                    }
                }

                closedir(dir);
                dir = opendir("/usr/lib/modules/4.14.206/extra/");
                if (dir == NULL) {
                    perror("opendir");
                    return;
                }

                while ((entry = readdir(dir)) != NULL) {
                    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                        continue;
                    }

                    snprintf(path, sizeof(path), "/usr/lib/modules/4.14.206/extra/%s", entry->d_name);
                    if (strcmp(entry->d_name, "gpioWakeup.ko") == 0) {
                     LE_INFO("KO module is avaialble");
                        snprintf(cmd, sizeof(cmd),
                                "/sbin/insmod %s gpioChipName=\"f100000.pinctrl\""
                                " gpioOffset=%d", path, gpioPin);
                        popen_call(cmd);
                        snprintf(cmd, sizeof(cmd), "/sbin/rmmod %s", path);
                        popen_call(cmd);
                        closedir(dir);
                        return;
                    }
                }
                closedir(dir);
                LE_ERROR("gpioWakeup.ko not found.\n");
                return;
            }
            break;
        case LE_JSON_TRUE:
        case LE_JSON_FALSE:
        case LE_JSON_NULL:
        case LE_JSON_OBJECT_START:
        case LE_JSON_OBJECT_END:
        case LE_JSON_STRING:
        case LE_JSON_ARRAY_START:
            break;
        case LE_JSON_DOC_END:
            le_json_Cleanup(JsonParsingSessionRef);
            le_fd_Close(jsonFd);
            jsonFd = -1;
        break;
    }
}

void taf_Gpio::JsonErrorHandler(le_json_Error_t error, const char* msg)
{
    switch (error)
    {
        case LE_JSON_SYNTAX_ERROR:
            LE_ERROR("JSON error message: %s", msg);

        case LE_JSON_READ_ERROR:
            LE_ERROR("JSON error message: %s", msg);
            le_json_Cleanup(JsonParsingSessionRef);
            le_fd_Close(jsonFd);
            jsonFd = -1;
            break;

        default:
            break;
    }
}
