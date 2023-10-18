/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafHalGpio.h"
#include <linux/gpio.h>

#define DEV_NAME "/dev/gpiochip0"

static le_event_Id_t gpioEvent;
static GPIOHANDLER appHandler = NULL;
static int32_t monGpioNum = -1;
static le_fdMonitor_Ref_t fdMonitorRef = NULL;

typedef struct
{
    int32_t num;
    bool status;
} gpioStatus_t;

// declare
static void* MonitorReguarFile(void* ctxPtr);

static void taf_hal_PowerOn()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return;
}

static void taf_hal_PowerOff()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return;
}

static int taf_hal_HwInit()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return 0;
}

static void taf_hal_Sleep()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return;
}

static void tal_hal_Wakeup()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return;
}

static int taf_hal_SelfTest()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return 0;
}

static size_t taf_hal_GetTotalGpioPins()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);

    size_t testSize = 4;
    return testSize;
}

static taf_hal_gpio_Direction tal_hal_GetDirection(uint8_t pinNum)
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return GPIO_HAL_INPUT;
}

static taf_hal_gpio_ActiveType_t tal_hal_GetPolarity(uint8_t pinNum)
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return GPIO_HAL_ACTIVE_TYPE_HIGH;
}

static taf_hal_gpio_Status tal_hal_SetPolarity(uint8_t pinNum, taf_hal_gpio_ActiveType_t type)
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return GPIO_HAL_OK;
}

static int tal_hal_GetValue(uint8_t pinNum, bool lock)
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    // should work for both input and output pins
    return 1; // or 0
    // return -1; // failure
}

static taf_hal_gpio_Status tal_hal_WriteOutputValue
(
    uint8_t pinNum,
    taf_hal_gpio_State value
)
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return GPIO_HAL_OK;
}

static taf_hal_gpio_Status tal_hal_SetEdgeTypeHAL
(
    uint8_t pinNum,
    taf_hal_gpio_Edge edge
)
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return GPIO_HAL_OK;
}

static taf_hal_gpio_Status tal_hal_SetDirection(uint8_t pinNum, uint32_t dir)
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return GPIO_HAL_OK;
}

static char* tal_hal_GetName
(
    uint8_t pinNum
)
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return "GPIO_NAME_EXAMPLE";
}

static void* taf_hal_GetModInf(void)
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return &(TAF_HAL_INFO_TAB.gpioInf);
}

// callback impmentation
static bool taf_hal_RegisterCallback
(
    int32_t num,
    taf_hal_gpio_Edge edgeType,
    GPIOHANDLER handler
)
{
    // simply store the callback. list is needed in real case
    LE_INFO("TestDrv: %s", __FUNCTION__);
    appHandler = handler;

    monGpioNum = num;

    return true;
}

static void localEventHandler(void* report)
{
    gpioStatus_t* gpioStatus;

    gpioStatus = (gpioStatus_t*)report;

    if (gpioStatus != NULL)
    {
        if ((monGpioNum == gpioStatus->num) && (appHandler != NULL))
        {
            appHandler(gpioStatus->num, gpioStatus->status);
        }
    }
}

static void GpioMonHandler
(
    int fd,
    short events
)
//--------------------------------------------------------------------------------------------------
{
    gpioStatus_t gpioStatus;

    static uint8_t status = 0;

    struct gpioevent_data event;
    int ret = le_fd_Read(fd, &event, sizeof(event));
    if (ret == -1)
    {
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
    switch (event.id)
    {
    case GPIOEVENT_EVENT_RISING_EDGE:
        LE_INFO("rising edge");
        gpioStatus.status = 1;
        break;
    case GPIOEVENT_EVENT_FALLING_EDGE:
        gpioStatus.status = 0;
        LE_INFO("falling edge");
        break;
    default:
        LE_ERROR("unknown event");
    }

    if (status != gpioStatus.status)
    {
        // trigger the le event
        le_event_Report(gpioEvent, &gpioStatus, sizeof(gpioStatus_t));
        status = gpioStatus.status;
    }
}

static void* MonitorReguarFile(void* ctxPtr)
{
    // monitor this file for any changes
    struct gpioevent_request rq;
    int fd = le_fd_Open(DEV_NAME, O_RDONLY);
    if (fd < 0)
    {
        LE_ERROR("Unabled to open %s: %s", DEV_NAME, strerror(errno));
        return NULL;
    }
    rq.lineoffset = monGpioNum;
    rq.eventflags = GPIOEVENT_REQUEST_BOTH_EDGES;
    rq.handleflags = GPIOHANDLE_REQUEST_INPUT;
    int ret = ioctl(fd, GPIO_GET_LINEEVENT_IOCTL, &rq);
    le_fd_Close(fd);
    if (ret == -1)
    {
        LE_ERROR("Unable to get line event from ioctl : %s", strerror(errno));
        return NULL;
    }

    fdMonitorRef = le_fdMonitor_Create("gpio_monitor",
                                        rq.fd, GpioMonHandler,
                                        POLLIN);

    LE_INFO("fdMonitorRef : %p rq.fd: %d", fdMonitorRef, rq.fd);
    le_event_RunLoop();
}

static void Init(void)
{
    le_thread_Ref_t threadRef = le_thread_Create("taf_GPIO_monitor",
                                                    MonitorReguarFile,
                                                    NULL);
    le_thread_Start(threadRef);

    // create an event for notificaiton
    gpioEvent = le_event_CreateId("GPIO Event", sizeof(gpioStatus_t));

    le_event_AddHandler("GPIO Event Handler", gpioEvent, localEventHandler);
}

LE_SHARED gpio_InfoTab_t TAF_HAL_INFO_TAB = {
    // always come first
    .mgrInf = {
        .name = TAF_GPIO_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .hwInitInf = taf_hal_HwInit,
        .powerOffInf = taf_hal_PowerOff,
        .powerOnInf = taf_hal_PowerOn,
        .selfTest = taf_hal_SelfTest, // tafModule will send test command
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    },

    .gpioInf = {
        .InitHAL = Init,
        .getTotalGpioPinsHAL = taf_hal_GetTotalGpioPins,
        .regCallbackHAL = taf_hal_RegisterCallback,
        .sleepHAL = taf_hal_Sleep,
        .wakeUpHAL = tal_hal_Wakeup,
        .getDirectionHAL = tal_hal_GetDirection,
        .getPolarityHAL = tal_hal_GetPolarity,
        .setPolarityHAL = tal_hal_SetPolarity,
        .setDirectionHAL = tal_hal_SetDirection,
        .getValueHAL = tal_hal_GetValue,
        .writeOutputValueHAL = tal_hal_WriteOutputValue,
        .setEdgeTypeHAL = tal_hal_SetEdgeTypeHAL,
        .getNameHAL = tal_hal_GetName,
    },
};

// DOT NOT USE COMPONET_INIT define module init for your service/apps
COMPONENT_INIT
{
    // Do not put your specfici init in here, define your init
    LE_INFO("Test Drv is loading\n");
}
