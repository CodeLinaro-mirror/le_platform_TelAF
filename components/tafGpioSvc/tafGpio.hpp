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

#ifndef TAFGPIO_HPP
#define TAFGPIO_HPP

#include <string>
#include <vector>
#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#define GPIO_PATH           "/sys/class/gpio"
#define NUM_OF_GPIOS_PATH   "/sys/class/gpio/gpiochip0/ngpio"

#define MAX_PIN_NUMBER 120
#define MIN_PIN_NUMBER 0

// Max handlers support simultaneously
#define MAX_TAF_GPIO_HANLDER 20

using namespace std;

struct taf_gpio{
    uint8_t pinNum;
    char gpioName[10];
    int fdMonitor;
    int handlerCount;
    bool isLocked;
    le_fdMonitor_Ref_t fdMonitorRef;
    le_msg_SessionRef_t lockedSession;
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

namespace telux{
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
            bool checkGpioPathExist(const char *gpioPath);
            le_result_t exportGpio(const taf_GpioRef_t gpioRef);
            le_result_t setGpioAttribute(const char *gpioPath, const char *attr);
            le_result_t writeGpioOutputValue(taf_GpioRef_t gpioRef,
                    taf_gpio_State_t value);
            le_result_t setDirection(taf_GpioRef_t gpioRef, taf_gpio_PinMode_t mode);
            le_result_t setEdgeType(taf_GpioRef_t gpioRef,
                    taf_gpio_Edge_t edge);
            static void callHandler(void* reportPtr);
            le_result_t setPolarity(taf_GpioRef_t gpioRef, taf_gpio_ActiveType_t level);
        public:
            taf_Gpio() {};
            ~taf_Gpio() {};
            static taf_Gpio &getInstance();
            static void OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *contextPtr);
            void Init();
            le_result_t getGpioAttribute(const char *gpioPath,
                    int attr_size, char *attribute);
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
            taf_gpio_ActiveType_t getPolarity(taf_GpioRef_t gpioRef);
            taf_gpio_Edge_t getEdgeSense(taf_GpioRef_t gpioRef);
            le_result_t setEdgeSense(taf_GpioRef_t gpioRef, taf_gpio_Edge_t edge, bool lock);

            taf_GpioRef_t tafGpioRefPin[MAX_PIN_NUMBER];
            le_mem_PoolRef_t HandlerPool = NULL;
            le_ref_MapRef_t  HandlerRefMap = NULL;
            le_dls_List_t GpioHandlerList;
            le_event_Id_t tafGpioEvent;
            int numOfGpios = -1;
    };
}
}
#endif
