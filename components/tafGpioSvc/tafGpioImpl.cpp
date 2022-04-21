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
#include "tafGpio.hpp"

using namespace telux::tafsvc;


/**
 * Returns Gpio instance
 */
taf_Gpio &taf_Gpio::getInstance()
{
    static taf_Gpio instance;
    return instance;
}

void taf_Gpio::Init()
{
    LE_INFO("tafGpioSvc started");
    le_msg_AddServiceCloseHandler(taf_gpio_GetServiceRef(), OnClientDisconnection, NULL);
    HandlerPool = le_mem_CreatePool("tafpioHandlerPool", sizeof(taf_InterruptHandlerCtx_t));
    HandlerRefMap = le_ref_CreateMap("tafGpioHandler", MAX_TAF_GPIO_HANLDER*2);
    GpioHandlerList = LE_DLS_LIST_INIT;
    tafGpioEvent = le_event_CreateId("tafGpio Event",sizeof(taf_gpioEvent_t));
    le_event_AddHandler("tafgpio input pin interrupt", tafGpioEvent, callHandler);
}

bool taf_Gpio::checkGpioPathExist
(
    const char *gpioPath
)
{
    DIR* gpioDir;

    gpioDir = opendir(gpioPath);
    if (gpioDir)
    {
        /* If directory exists close & return true */
        closedir(gpioDir);
    }
    else if (ENOENT == errno)
    {
        return false;
    }

    return true;
}

le_result_t taf_Gpio::exportGpio
(
    const taf_GpioRef_t tafGpioRef
)
{
    char gpioPath[128];
    char exportPath[128];
    char gpioNum[8];

    // Return success, if its already exported
    snprintf(gpioPath, sizeof(gpioPath), "%s/%s", GPIO_PATH, tafGpioRef->gpioName);
    if (checkGpioPathExist(gpioPath))
    {
        return LE_OK;
    }

    snprintf(exportPath, sizeof(exportPath), "%s/%s", GPIO_PATH, "export");
    snprintf(gpioNum, sizeof(gpioNum), "%d", tafGpioRef->pinNum);

    int exportFd = le_fd_Open(exportPath, O_WRONLY );
    if (exportFd == -1) {
        return LE_IO_ERROR;
    }

    ssize_t written = le_fd_Write(exportFd, gpioNum, le_utf8_NumChars(gpioNum));
    le_fd_Close(exportFd);
    if (written == -1) {
        LE_WARN("Failed to export GPIO %s", gpioNum);
        return LE_IO_ERROR;
    } else if (written < le_utf8_NumChars(gpioNum)) {
        LE_WARN("Data truncated while exporting GPIO %s.", gpioNum);
        return LE_IO_ERROR;
    }

    if (checkGpioPathExist(gpioPath))
    {
        return LE_OK;
    }
    LE_WARN("Unable to export GPIO %s.", gpioNum);
    return LE_IO_ERROR;
}

le_result_t taf_Gpio::setGpioAttribute
(
    const char *gpioPath,
    const char *attribute
)
{
    int fileFd = le_fd_Open(gpioPath, O_WRONLY );
    if (fileFd == -1) {
        LE_ERROR("Failed to open file %s for writing", gpioPath);
        return LE_IO_ERROR;
    }

    ssize_t written = le_fd_Write(fileFd, attribute, le_utf8_NumChars(attribute));
    le_fd_Close(fileFd);

    if (written == -1) {
        LE_EMERG("Failed to write %s to GPIO config %s", attribute, gpioPath);
        return LE_IO_ERROR;
    } else if (written < le_utf8_NumChars(attribute)) {
        LE_EMERG("Data truncated while writing to %s GPIO config %s.", gpioPath, attribute);
        return LE_IO_ERROR;
    }
    return LE_OK;
}

le_result_t taf_Gpio::getGpioAttribute
(
    const char *gpioPath,
    int attr_size,
    char *gpioAttr
)
{
    char *res = gpioAttr;

    int gpioFd = le_fd_Open(gpioPath, O_RDONLY);
    if (gpioFd == -1)
    {
        LE_ERROR("Failed to open file %s for reading", gpioPath);
        return LE_IO_ERROR;
    }

    le_fd_Read(gpioFd, res, attr_size);

    LE_DEBUG("Read result: %s from %s", res, gpioPath);
    le_fd_Close(gpioFd);

    return LE_OK;
}

le_result_t taf_Gpio::writeGpioOutputValue
(
    taf_GpioRef_t tafGpioRef,
    taf_gpio_State_t value
)
{
    char gpioPath[64];
    char gpioAttr[16];

    TAF_ERROR_IF_RET_VAL(tafGpioRef == NULL, LE_BAD_PARAMETER,
            "tafGpioRef is NULL or gpio not initialized");

    snprintf(gpioPath, sizeof(gpioPath), "%s/%s/%s", GPIO_PATH,
                          tafGpioRef->gpioName, "value");
    snprintf(gpioAttr, sizeof(gpioAttr), "%d", value);
    LE_DEBUG("gpioPath:%s, gpioAttr:%s", gpioPath, gpioAttr);

    return setGpioAttribute(gpioPath, gpioAttr);
}

le_result_t taf_Gpio::setEdgeType
(
    taf_GpioRef_t tafGpioRef,
    taf_gpio_Edge_t tafEdge
)
{
    char filePath[64];
    const char *attr;

    TAF_ERROR_IF_RET_VAL(tafGpioRef == NULL, LE_BAD_PARAMETER,
            "tafGpioRef is NULL or gpio not initialized");

    snprintf(filePath, sizeof(filePath), "%s/%s/%s", GPIO_PATH,
             tafGpioRef->gpioName, "edge");

    switch(tafEdge)
    {
        case TAF_GPIO_EDGE_RISING:
            attr = "rising";
            break;
        case TAF_GPIO_EDGE_FALLING:
            attr = "falling";
            break;
        case TAF_GPIO_EDGE_BOTH:
            attr = "both";
            break;
        default:
            attr = "none";
            break;
    }
    LE_INFO("path:%s, attr:%s", filePath, attr);
    return setGpioAttribute(filePath, attr);
}

le_result_t taf_Gpio::setDirection
(
    taf_GpioRef_t tafGpioRef,
    taf_gpio_PinMode_t tafPinMode
)
{
    char filePath[64];
    const char *attr;
    taf_gpio_PinMode_t currentMode;

    TAF_ERROR_IF_RET_VAL(tafGpioRef == NULL, LE_BAD_PARAMETER,
            "tafGpioRef is NULL or gpio not initialized");

    taf_Gpio gpio = getInstance();
    currentMode = (gpio.isInput(tafGpioRef)) ? GPIO_PIN_MODE_INPUT : GPIO_PIN_MODE_OUTPUT;
    if (currentMode == tafPinMode)
    {
        // Direction is already correct, do nothing.
        return LE_OK;
    }

    snprintf(filePath, sizeof(filePath), "%s/%s/%s", GPIO_PATH,
            tafGpioRef->gpioName, "direction");
    attr = (tafPinMode == GPIO_PIN_MODE_OUTPUT) ? "out": "in";
    LE_INFO("path:%s, attribute:%s", filePath, attr);

    return setGpioAttribute(filePath, attr);
}

le_result_t taf_Gpio::setGpioAsInput
(
    taf_GpioRef_t tafGpioRef,
    taf_gpio_ActiveType_t polarity,
    bool lock
)
{
    TAF_ERROR_IF_RET_VAL(LE_OK != exportGpio(tafGpioRef), LE_BUSY,
            "Unable to export GPIO %s for use", tafGpioRef->gpioName);

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

    return setPolarity(tafGpioRef, polarity);
}

le_result_t taf_Gpio::setPolarity
(
    taf_GpioRef_t tafGpioRef,
    taf_gpio_ActiveType_t level
)
{
    char filePath[64];
    char attr[16];

    TAF_ERROR_IF_RET_VAL(tafGpioRef == NULL, LE_BAD_PARAMETER,
            "tafGpioRef is NULL or gpio not initialized");

    snprintf(filePath, sizeof(filePath), "%s/%s/%s", GPIO_PATH,
             tafGpioRef->gpioName, "active_low");
    snprintf(attr, sizeof(attr), "%d", level);
    LE_INFO("path:%s, attr:%s", filePath, attr);

    return setGpioAttribute(filePath, attr);
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
    char monFile[128];
    int monFd = -1;
    le_result_t leResult;

    TAF_ERROR_IF_RET_VAL(tafGpioRef == NULL, NULL,
            "tafGpioRef is NULL or object not initialized");

    TAF_ERROR_IF_RET_VAL(LE_OK != exportGpio(tafGpioRef), NULL,
            "Failed to export GPIO %s for use", tafGpioRef->gpioName);

    le_msg_SessionRef_t sessionRef = taf_gpio_GetClientSessionRef();
    if (tafGpioRef->isLocked && tafGpioRef->lockedSession != sessionRef) {
        LE_WARN("Attemp to use gpio pin %d, which is locked", tafGpioRef->pinNum);
        return NULL;
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

    leResult = setEdgeSense(tafGpioRef, edge, lock);
    // Set the edge detection mode
    if (leResult != LE_OK)
    {
        TAF_ERROR_IF_RET_VAL(leResult == LE_BAD_PARAMETER, NULL,
                "Path doesn't exist to set edge detection");
        TAF_ERROR_IF_RET_VAL(leResult != LE_BAD_PARAMETER, NULL,
                "Failed to set edge detection");
    }
    if (tafGpioRef->fdMonitor != -1) {
        LE_INFO("Monitor is already created for the GPIO %d", tafGpioRef->pinNum);
        return handlerCtxPtr->handlerRef;
    }
    // Start monitoring the fd for the correct GPIO
    snprintf(monFile, sizeof(monFile), "%s/%s/%s", GPIO_PATH,
             tafGpioRef->gpioName, "value");

    do
    {
        monFd = open(monFile, O_RDONLY);
    }
    while ((monFd < 0) && (errno == EINTR));

    TAF_ERROR_IF_RET_VAL(monFd < 0, NULL, "Failed to open GPIO file for monitoring");

    LE_DEBUG("Seek to start of file %d", monFd);

    TAF_ERROR_IF_RET_VAL(lseek(monFd, 0, SEEK_SET) == (off_t)(-1), NULL,
            "Failed to SEEK_SET for GPIO '%s'. %m.", tafGpioRef->gpioName );

    //We will read a single character
    char buf[1];

    if (read(monFd, buf, 1) != 1)
    {
        LE_ERROR("Unable to read value for GPIO %s. %m", tafGpioRef->gpioName);
    }

    LE_DEBUG("Setting up file monitor for fd %d and pin %s", monFd, tafGpioRef->gpioName);
    tafGpioRef->fdMonitorRef = le_fdMonitor_Create (tafGpioRef->gpioName, monFd, fdMonFunc,
            POLLPRI);
    tafGpioRef->fdMonitor = monFd;
    return handlerCtxPtr->handlerRef;
}

void taf_Gpio::removeChangeCallback
(
    void * handlerRef
)
{
    TAF_ERROR_IF_RET_NIL(NULL == handlerRef, "Invalid handler reference provided");

    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&GpioHandlerList);
    taf_GpioRef_t gpioRef;
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
    if (gpioRef->handlerCount == 0) {
        if (gpioRef->fdMonitorRef != NULL)
        {
            LE_INFO("Stopping fd monitor");
            const int fd = le_fdMonitor_GetFd(gpioRef->fdMonitorRef);
            le_fdMonitor_Delete(gpioRef->fdMonitorRef);
            gpioRef->fdMonitorRef = NULL;
            const int ret = close(fd);
            LE_WARN_IF(ret == -1, "Failed to close file descriptor for gpio %d",
                    gpioRef->pinNum);
        }
        gpioRef->fdMonitor = -1;
    }
    LE_INFO("removeChangeCallback handlerRef");
}

le_result_t taf_Gpio::disableEdgeSense
(
    taf_GpioRef_t tafGpioRef,
    bool lock
)
{
    TAF_ERROR_IF_RET_VAL(LE_OK != exportGpio(tafGpioRef), LE_BUSY,
            "Failed to export GPIO %s for use", tafGpioRef->gpioName);

    le_msg_SessionRef_t sessionRef = taf_gpio_GetClientSessionRef();
    if (tafGpioRef->isLocked && tafGpioRef->lockedSession != sessionRef) {
        LE_WARN("Attemp to use gpio pin %d, which is locked", tafGpioRef->pinNum);
        return LE_BUSY;
    }

    tafGpioRef->isLocked = lock;
    tafGpioRef->lockedSession = sessionRef;

    return setEdgeSense(tafGpioRef, TAF_GPIO_EDGE_NONE, lock);
}

taf_gpio_State_t taf_Gpio::readValue
(
    taf_GpioRef_t tafGpioRef,
    bool lock
)
{
    char path[64];
    char result[17];
    le_result_t leRes;
    taf_gpio_State_t type;

    TAF_ERROR_IF_RET_VAL(LE_OK != exportGpio(tafGpioRef), TAF_GPIO_BUSY,
            "Unable to export GPIO %s for use", tafGpioRef->gpioName);

    TAF_ERROR_IF_RET_VAL(!tafGpioRef, TAF_GPIO_BUSY,
            "gpioRef is NULL or object not initialized");

    le_msg_SessionRef_t sessionRef = taf_gpio_GetClientSessionRef();
    if (tafGpioRef->isLocked && tafGpioRef->lockedSession != sessionRef) {
        LE_WARN("Attemp to use gpio pin %d, which is locked", tafGpioRef->pinNum);
        return TAF_GPIO_BUSY;
    }

    snprintf(path, sizeof(path), "%s/%s/%s", GPIO_PATH,
             tafGpioRef->gpioName, "value");
    leRes = getGpioAttribute(path, sizeof(result), result);
    if (leRes != LE_OK)
    {
        return TAF_GPIO_BUSY;
    }

    tafGpioRef->isLocked = lock;
    tafGpioRef->lockedSession = sessionRef;

    type = (taf_gpio_State_t)atoi(result);
    LE_INFO("result:%s Value:%s", result, (type==1) ? "high": "low");

    return type;
}

le_result_t taf_Gpio::activate
(
    taf_GpioRef_t tafGpioRef,
    bool lock
)
{
    TAF_ERROR_IF_RET_VAL(LE_OK != exportGpio(tafGpioRef), LE_BUSY,
            "Unable to export GPIO %s for use", tafGpioRef->gpioName);

    le_msg_SessionRef_t sessionRef = taf_gpio_GetClientSessionRef();
    if(tafGpioRef->isLocked && tafGpioRef->lockedSession != sessionRef) {
        LE_WARN("Attemp to use gpio pin %d, which is locked", tafGpioRef->pinNum);
        return LE_BUSY;
    }

    TAF_ERROR_IF_RET_VAL(LE_OK != setDirection(tafGpioRef, GPIO_PIN_MODE_OUTPUT),
            LE_IO_ERROR, "Failed to set Direction on GPIO %s", tafGpioRef->gpioName);

    TAF_ERROR_IF_RET_VAL(LE_OK != writeGpioOutputValue(tafGpioRef, TAF_GPIO_HIGH),
            LE_IO_ERROR, "Failed to set GPIO %s to high", tafGpioRef->gpioName);

    tafGpioRef->isLocked = lock;
    tafGpioRef->lockedSession = sessionRef;

    return LE_OK;
}

le_result_t taf_Gpio::deactivate
(
    taf_GpioRef_t tafGpioRef,
    bool lock
)
{
    TAF_ERROR_IF_RET_VAL(LE_OK != exportGpio(tafGpioRef), LE_BUSY,
            "Unable to export GPIO %s for use", tafGpioRef->gpioName);

    le_msg_SessionRef_t sessionRef = taf_gpio_GetClientSessionRef();
    if(tafGpioRef->isLocked && tafGpioRef->lockedSession != sessionRef) {
        LE_WARN("Attemp to use gpio pin %d, which is locked", tafGpioRef->pinNum);
        return LE_BUSY;
    }

    TAF_ERROR_IF_RET_VAL(LE_OK != setDirection(tafGpioRef, GPIO_PIN_MODE_OUTPUT),
            LE_IO_ERROR, "Failed to set Direction on GPIO %s", tafGpioRef->gpioName);

    TAF_ERROR_IF_RET_VAL(LE_OK != writeGpioOutputValue(tafGpioRef, TAF_GPIO_LOW),
            LE_IO_ERROR, "Failed to set GPIO %s to low", tafGpioRef->gpioName);

    tafGpioRef->isLocked = lock;
    tafGpioRef->lockedSession = sessionRef;

    return LE_OK;
}

bool taf_Gpio::isActive
(
    taf_GpioRef_t tafGpioRef
)
{

    TAF_ERROR_IF_RET_VAL(LE_OK != exportGpio(tafGpioRef), false,
            "Unable to export GPIO %s for use", tafGpioRef->gpioName);

    if (isInput(tafGpioRef))
    {
        LE_WARN("Attempt to check if an input gpio pin is active");
        return false;
    }

    return (readValue(tafGpioRef, false) == TAF_GPIO_HIGH);
}

bool taf_Gpio::isInput
(
    taf_GpioRef_t tafGpioRef
)
{
    char gpioPath[64];
    char result[9];
    le_result_t leResult;

    TAF_ERROR_IF_RET_VAL(!tafGpioRef, false,
            "tafGpioRef is NULL or object not initialized");

    TAF_ERROR_IF_RET_VAL(LE_OK != exportGpio(tafGpioRef), false,
            "Unable to export GPIO %s for use", tafGpioRef->gpioName);

    snprintf(gpioPath, sizeof(gpioPath), "%s/%s/%s", GPIO_PATH,
             tafGpioRef->gpioName, "direction");
    leResult = getGpioAttribute(gpioPath, sizeof(result), result);
    if (leResult != LE_OK)
    {
        return false;
    }

    return (strncmp(result, "in", 2) == 0);
}

bool taf_Gpio::isOutput
(
    taf_GpioRef_t tafGpioRef
)
{
    char gpioPath[64];
    char result[9];
    le_result_t leRes;

    TAF_ERROR_IF_RET_VAL(!tafGpioRef, false,
            "gpioRef is NULL or object not initialized");

    TAF_ERROR_IF_RET_VAL(LE_OK != exportGpio(tafGpioRef), false,
            "Unable to export GPIO %s for use", tafGpioRef->gpioName);

    snprintf(gpioPath, sizeof(gpioPath), "%s/%s/%s", GPIO_PATH,
             tafGpioRef->gpioName, "direction");
    leRes = getGpioAttribute(gpioPath, sizeof(result), result);
    if (leRes != LE_OK)
    {
        return false;
    }

    return (strncmp(result, "out", 3) == 0);
}

taf_gpio_ActiveType_t taf_Gpio::getPolarity
(
    taf_GpioRef_t tafGpioRef
)
{
    char gpioPath[64];
    char result[17];
    le_result_t leRes;
    taf_gpio_ActiveType_t type;


    TAF_ERROR_IF_RET_VAL(!tafGpioRef, GPIO_ACTIVE_TYPE_UNKNOWN,
            "tafGpioRef is NULL or object not initialized");

    TAF_ERROR_IF_RET_VAL(LE_OK != exportGpio(tafGpioRef), GPIO_ACTIVE_TYPE_UNKNOWN,
            "Unable to export GPIO %s for use", tafGpioRef->gpioName);

    snprintf(gpioPath, sizeof(gpioPath), "%s/%s/%s", GPIO_PATH,
             tafGpioRef->gpioName, "active_low");
    leRes = getGpioAttribute(gpioPath, sizeof(result), result);
    if (leRes != LE_OK)
    {
        return GPIO_ACTIVE_TYPE_UNKNOWN;
    }

    type = (taf_gpio_ActiveType_t)atoi(result);
    LE_DEBUG("result: %s", result);

    if (type == 0)
    {
        return GPIO_ACTIVE_TYPE_HIGH;
    }

    return GPIO_ACTIVE_TYPE_LOW;
}

taf_gpio_Edge_t taf_Gpio::getEdgeSense
(
    taf_GpioRef_t tafGpioRef
)
{
    char gpioPath[64];
    char result[9];
    le_result_t leRes;

    TAF_ERROR_IF_RET_VAL(!tafGpioRef, TAF_GPIO_EDGE_UNKNOWN,
            "tafGpioRef is NULL or object not initialized");

    TAF_ERROR_IF_RET_VAL(LE_OK != exportGpio(tafGpioRef), TAF_GPIO_EDGE_UNKNOWN,
            "Unable to export GPIO %s for use", tafGpioRef->gpioName);

    if (isOutput(tafGpioRef))
    {
        LE_WARN("Attempt to read edge sense on an output");
        return TAF_GPIO_EDGE_NONE;
    }

    snprintf(gpioPath, sizeof(gpioPath), "%s/%s/%s", GPIO_PATH,
             tafGpioRef->gpioName, "edge");
    leRes = getGpioAttribute(gpioPath, sizeof(result), result);
    if (leRes != LE_OK)
    {
        return TAF_GPIO_EDGE_UNKNOWN;
    }

    LE_DEBUG("Read edge - result: %s", result);

    taf_gpio_Edge_t edge = TAF_GPIO_EDGE_NONE;

    if (strncmp(result, "rising", 6) == 0)
    {
        LE_DEBUG("Detected edge as rising");
        edge = TAF_GPIO_EDGE_RISING;
    }
    else if (strncmp(result, "falling", 7) == 0)
    {
        LE_DEBUG("Detected edge as rising");
        edge = TAF_GPIO_EDGE_FALLING;
    }
    else if (strncmp(result, "both", 4) == 0)
    {
        LE_DEBUG("Detected edge as both");
        edge = TAF_GPIO_EDGE_BOTH;
    }

    return edge;
}

le_result_t taf_Gpio::setEdgeSense
(
    taf_GpioRef_t tafGpioRef,
    taf_gpio_Edge_t edge,
    bool lock
)
{
    TAF_ERROR_IF_RET_VAL(LE_OK != exportGpio(tafGpioRef), LE_BUSY,
            "Unable to export GPIO %s for use", tafGpioRef->gpioName);

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

    return setEdgeType(tafGpioRef, edge);
}

void taf_Gpio::callHandler(void* reportPtr) {
    taf_gpioEvent_t *eventPtr = (taf_gpioEvent_t *)reportPtr;
    taf_Gpio gpio = taf_Gpio::getInstance();
    gpio.callClientHandlerFunc(eventPtr);
}

void taf_Gpio::callClientHandlerFunc(taf_gpioEvent_t *eventPtr)
{
    TAF_ERROR_IF_RET_NIL(eventPtr == NULL, "eventPtr is NULL");

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
            handlerCtxPtr->handlerPtr(eventPtr->pinNum, eventPtr->state, handlerCtxPtr->usrContext);
        }
    }
}

void taf_Gpio::inputMonitorHandlerFunc
(
    int fd,
    short events
)
{
    //We're reading a single character
    char buf[1];
    taf_GpioRef_t tafGpioRef;
    for(int i=0; i < MAX_PIN_NUMBER; i++) {
        if(tafGpioRefPin[i]->fdMonitor == fd) {
            tafGpioRef = tafGpioRefPin[i];
        }
    }

    // Make sure the pin is in use and has listeners, this isn't a spurious interrupt
    if (tafGpioRef->handlerCount == 0)
    {
        LE_WARN("Spurious interrupt handled - ignoring");
        return;
    }

    LE_DEBUG("Seek to start of file %d", fd);
    lseek(fd, 0, SEEK_SET);

    TAF_ERROR_IF_RET_NIL(read(fd, buf, 1) != 1,
            "Unable to read value for GPIO %s", tafGpioRef->gpioName);

    LE_DEBUG("Read value %c from value file for callback", buf[0]);
    taf_gpioEvent_t event = {fd, buf[0] == '1', tafGpioRef->pinNum};
    le_event_Report(tafGpioEvent, &event, sizeof(taf_gpioEvent_t));
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
        if(gpio.tafGpioRefPin[i]->lockedSession == sessionRef) {
            gpio.tafGpioRefPin[i]->isLocked = false;
            gpio.tafGpioRefPin[i]->lockedSession = NULL;
        }
    }
}