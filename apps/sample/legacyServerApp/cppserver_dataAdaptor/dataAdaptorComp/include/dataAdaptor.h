/**
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATAADAPTOR_H
#define DATAADAPTOR_H


#include <stdio.h>

#include "legato.h"
extern "C" {
#include "taf_dcs_interface.h"
}

using namespace std;

class DataAdaptor
{
    public:
        DataAdaptor(){};
        ~DataAdaptor(){};
        static void Connect();
        static DataAdaptor &GetInstance();
        static void OnClientSessionDisconnected
        (
            le_msg_SessionRef_t sessionRef,
            void* contextPtr
        );
        static void OnClientSessionConnected
        (
            le_msg_SessionRef_t sessionRef,
            void* contextPtr
        );

        void RegisterEventLoop(void);

        le_result_t StartDataCallOnDefaultProfile(taf_dcs_Pdp_t ipType);
        void DumpDataProfile(void);

        static void DataCallEventHandler
        (
        taf_dcs_ProfileRef_t profileRef,
        taf_dcs_ConState_t callEvent,
        const taf_dcs_StateInfo_t *infoPtr,
        void* contextPtr
        );
        static const char* CallEventToString
        (
            taf_dcs_ConState_t callEvent
        );

};

#endif