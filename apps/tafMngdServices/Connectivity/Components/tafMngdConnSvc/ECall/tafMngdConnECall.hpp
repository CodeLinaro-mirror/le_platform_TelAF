/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#pragma once
#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

namespace tafsvc {

class tafMngdConnECall: public ITafSvc
{
    public:
        tafMngdConnECall() {};
        ~tafMngdConnECall() {};

        void Init(void);
        static tafMngdConnECall &GetInstance();
        //Check ECall in Progress
        bool IsECallInProgress();
};

}
