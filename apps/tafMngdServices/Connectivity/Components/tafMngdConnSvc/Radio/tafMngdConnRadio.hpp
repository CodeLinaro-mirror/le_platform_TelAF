/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#pragma once
#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

namespace tafsvc {

class tafMngdConnRadio: public ITafSvc
{
    public:
        tafMngdConnRadio() {};
        ~tafMngdConnRadio() {};

        void Init(void);
        static tafMngdConnRadio &GetInstance();
        le_result_t StartUp(uint8_t phoneId);
        bool IsPowerOn(uint8_t phoneId);
        le_result_t PowerOn(uint8_t phoneId);
        le_result_t PowerOff(uint8_t phoneId);
        le_result_t SetAutoRegMode(uint8_t phoneId);
        bool IsAutoRegMode(uint8_t phoneId);
        bool IsNetworkRegistered(uint8_t phoneId);
};

}
