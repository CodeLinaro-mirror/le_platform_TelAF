/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAFGPIO_HPP
#define TAFGPIO_HPP

#include <errno.h>
#include <linux/gpio.h>
#include <string>
#include <vector>
#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafHalLib.hpp"
#include "tafHalGpio.h"

#define DEV_NAME "/dev/gpiochip0"

#define MAX_PIN_NUMBER 150
#define MIN_PIN_NUMBER 0

// Max handlers support simultaneously
#define MAX_TAF_GPIO_HANLDER 20

using namespace std;

struct taf_gpio{
    uint8_t pinNum;
    char gpioName[10];
    char aliasName[TAF_GPIO_PIN_NAME_MAX_BYTE];
    int fdMonitor;
    int handlerCount;
    bool isLocked;
    le_fdMonitor_Ref_t fdMonitorRef;
    le_msg_SessionRef_t lockedSession;
    le_hashmap_Ref_t clientHashMap;
    taf_gpio_Edge_t edge;
};

typedef struct taf_gpio* taf_GpioRef_t;

typedef struct
{
    taf_gpio_ChangeEventHandlerRef_t  handlerRef;     // handler ref
    taf_gpio_ChangeCallbackFunc_t     handlerPtr;     // function ptr
    void *                            usrContext;     // context pointer
    uint8_t                           pinNum;         // pinNum for which handler registered
    le_msg_SessionRef_t               sessionCtxPtr;  // handler's session
    le_dls_Link_t                     link;           // link to handler list
} taf_InterruptHandlerCtx_t;

namespace tafsvc{

    typedef enum
    {
        GPIO_PIN_MODE_OUTPUT,
        GPIO_PIN_MODE_INPUT
    } taf_gpio_PinMode_t;
    typedef enum
    {
        GPIO_ACTIVE_TYPE_UNKNOWN = -1,
        GPIO_ACTIVE_TYPE_HIGH,
        GPIO_ACTIVE_TYPE_LOW
    } taf_gpio_ActiveType_t;

    typedef struct
    {
        int               fd;
        bool              state;
        uint8_t           pinNum;
    } taf_gpioEvent_t;

    class taf_Gpio : public ITafSvc {
        private:
            le_result_t writeGpioOutputValue(taf_GpioRef_t gpioRef,
                    taf_gpio_State_t value);
            le_result_t setDirection(taf_GpioRef_t gpioRef, taf_gpio_PinMode_t mode);
            le_result_t setEdgeType(taf_GpioRef_t gpioRef,
                    taf_gpio_Edge_t edge, le_fdMonitor_HandlerFunc_t fdMonFunc);
            static void callHandler(void* reportPtr);
            static void JsonEventHandler(le_json_Event_t event);
            static void JsonErrorHandler(le_json_Error_t error, const char* msg);
            static int popen_call(const char *cmd);
            le_result_t setPolarity(taf_GpioRef_t gpioRef, taf_gpio_ActiveType_t level);
        public:
            taf_Gpio() {};
            ~taf_Gpio() {};
            static taf_Gpio &getInstance();
            static void OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *contextPtr);
            void Init();
            void inputMonitorHandlerFunc(int fd, short events);
            void callClientHandlerFunc(taf_gpioEvent_t *eventPtr);
            le_result_t setGpioAsInput(taf_GpioRef_t gpioRef, taf_gpio_ActiveType_t polarity, bool lock);
            void* setChangeCallback(taf_GpioRef_t gpioRef, le_fdMonitor_HandlerFunc_t fdMonFunc,
                    taf_gpio_Edge_t edge, bool lock, taf_gpio_ChangeCallbackFunc_t handlerPtr,
                    void* contextPtr);
            void removeChangeCallback(void * addHandlerRef);
            le_result_t disableEdgeSense(taf_GpioRef_t gpioRef, bool lock);
            taf_gpio_State_t readValue(taf_GpioRef_t gpioRef, bool lock);
            le_result_t activate(taf_GpioRef_t gpioRef, bool lock);
            le_result_t deactivate(taf_GpioRef_t gpioRef, bool lock);
            bool isActive(taf_GpioRef_t gpioRef);
            bool isInput(taf_GpioRef_t gpioRef);
            bool isOutput(taf_GpioRef_t gpioRef);
            le_result_t getName(taf_GpioRef_t gpioRef, char* name, size_t nameSize);
            taf_gpio_ActiveType_t getPolarity(taf_GpioRef_t gpioRef);
            taf_gpio_Edge_t getEdgeSense(taf_GpioRef_t gpioRef);
            le_result_t setEdgeSense(taf_GpioRef_t gpioRef, taf_gpio_Edge_t edge, bool lock,
                    le_fdMonitor_HandlerFunc_t fdMonFunc);

            taf_GpioRef_t tafGpioRefPin[MAX_PIN_NUMBER];
            le_mem_PoolRef_t HandlerPool = nullptr;
            le_ref_MapRef_t  HandlerRefMap = nullptr;
            le_dls_List_t GpioHandlerList;
            le_event_Id_t tafGpioEvent;
            int numOfGpios = -1;
            bool isDrvPresent = false;
            gpio_Inf_t *gpioInf = nullptr;
    };
}
#endif
