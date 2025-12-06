/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#pragma once
#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

namespace tafsvc {

class tafMngdConnSim: public ITafSvc
{
    public:
        tafMngdConnSim() {};
        ~tafMngdConnSim() {};

        void Init(void);
        static tafMngdConnSim &GetInstance();
        bool IsSimReady(uint8_t slotId);
        void RegisterEvents();
        void UnregisterEvents();
        static void SimStateHandler(taf_sim_Id_t simId, taf_sim_States_t simState,
                                    void *contextPtr);
        le_result_t PowerOn(uint8_t slotId);
        le_result_t PowerOff(uint8_t slotId);

    private:
        taf_sim_NewStateHandlerRef_t simtateHandlerRef = NULL;
};

}
