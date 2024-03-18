/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#pragma once
#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

namespace telux {
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
}
