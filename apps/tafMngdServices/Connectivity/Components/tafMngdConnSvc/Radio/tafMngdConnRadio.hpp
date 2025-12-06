/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#pragma once
#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

namespace tafsvc {

typedef enum  _telafcm_Radio_Events_t
{
    TELAFCM_RADIO_SIGNAL_GOOD,
    TELAFCM_RADIO_SIGNAL_BAD,
    TELAFCM_RADIO_SIGNAL_UNKNOWN
}telafcm_Radio_Events_t;


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
        le_result_t SetAllRat(uint8_t phoneId);
        bool IsAllRatSet(uint8_t phoneId);
        le_result_t SetAutoRegMode(uint8_t phoneId);
        bool IsAutoRegMode(uint8_t phoneId);
        void RegisterEvents();
        void UnregisterEvents();
        bool IsNetworkRegistered(uint8_t phoneId);
        static void GsmSsChangeHandler(int32_t ss, int32_t rsrp, uint8_t phoneId, void *contextPtr);
        static void UmtsSsChangeHandler(int32_t ss, int32_t rsrp,
                                        uint8_t phoneId, void *contextPtr);
        static void CdmaSsChangeHandler(int32_t ss, int32_t rsrp,
                                        uint8_t phoneId, void *contextPtr);
        static void TdscdmaSsChangeHandler(int32_t ss, int32_t rsrp,
                                           uint8_t phoneId, void *contextPtr);
        static void LteSsChangeHandler(int32_t ss, int32_t rsrp, uint8_t phoneId, void *contextPtr);
        static void Nr5gSsChangeHandler(int32_t ss, int32_t rsrp,
                                        uint8_t phoneId, void *contextPtr);
        static void PackSwStateHandler(taf_radio_NetRegStateInd_t* packSwStateIndPtr,
                                       void* contextPtr);

    private:
        taf_radio_SignalStrengthChangeHandlerRef_t gsmSsChangeHandlerRef = NULL;
        taf_radio_SignalStrengthChangeHandlerRef_t umtsSsChangeHandlerRef = NULL;
        taf_radio_SignalStrengthChangeHandlerRef_t cdmaSsChangeHandlerRef = NULL;
        taf_radio_SignalStrengthChangeHandlerRef_t tdscdmaSsChangeHandlerRef = NULL;
        taf_radio_SignalStrengthChangeHandlerRef_t lteSsChangeHandlerRef = NULL;
        taf_radio_SignalStrengthChangeHandlerRef_t nr5gSsChangeHandlerRef = NULL;
        taf_radio_PacketSwitchedChangeHandlerRef_t packSwStateHandlerRef = NULL;

};

}
