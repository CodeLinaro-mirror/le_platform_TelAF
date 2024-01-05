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

#ifndef TAFDOIP_VEHICLE_MGR_HPP
#define TAFDOIP_VEHICLE_MGR_HPP

#include "legato.h"
#include "interfaces.h"

#include "tafDoIPStack.h"
#include "tafDoIPCommon.hpp"

typedef struct
{
    char vin[TAF_DOIP_VIN_SIZE + 1];
    char eid[TAF_DOIP_EID_SIZE];
    char gid[TAF_DOIP_GID_SIZE];
    uint32_t entityLA;
    uint32_t maxSockNum;
    uint32_t maxDataSize;
    uint32_t maxAnnounceCount;
    uint32_t announceItrvalTime;
    uint32_t genInactiveTime;
    uint32_t initInactiveTime;
    uint32_t aliveCheckTime;
    bool authEnableStatus;
    uint32_t authInfo;
    char ifName[TAF_DOIP_INTERFACE_NAME_MAX_LEN];
    char netType[TAF_DOIP_IPTYPE_MAX_LEN];
    uint16_t udpPort;
    uint16_t tcpPort;
    bool parseStatus = false;     // This flag will indicate that whether json is parsed or not
}taf_doip_Config_t;


namespace taf{
namespace doip{
    class VehicleManager{
        public:
            VehicleManager() {};
            ~VehicleManager() {};

            static VehicleManager &GetInstance();

            void Init(const char* configPathPtr);

            taf_doip_Result_t DeInit();

            void ParseJsonConfig(const char* configPathPtr);

            uint8_t GetProtocolVersion();

            taf_doip_Result_t GetEid(char* eidPtr);

            taf_doip_Result_t SetEid(const char* eidPtr);

            taf_doip_Result_t GetMaxConcurrentSockNum(uint32_t *maxCTSPtr);

            taf_doip_Result_t GetMaxDataSize(uint32_t *maxDataSizePtr);

            taf_doip_Result_t GetMaxAnnounceCount(uint32_t *announceCountPtr);

            taf_doip_Result_t GetAnnounceIntervalTime(uint32_t *announceIntTimePtr);

            taf_doip_Result_t GetGenInactivityTime(uint32_t *genInactiveTimePtr);

            taf_doip_Result_t GetInitialInactivityTime(uint32_t *initInactiveTimePtr);

            taf_doip_Result_t GetAliveCheckTime(uint32_t *aliveCheckTimePtr);

            taf_doip_Result_t GetEntityLogicalAddr(uint16_t *entityLAPtr);

            taf_doip_Result_t SetEntityLogicalAddr(uint16_t entityLA);

            bool GetAuthEnableStatus();

            taf_doip_Result_t GetAuthInfo(uint32_t *authInfoPtr);

            taf_doip_Result_t GetMulticast(char* multicastPtr);

            taf_doip_Result_t GetUdpPort(uint16_t *udpPortPtr);

            taf_doip_Result_t GetTcpPort(uint16_t *tcpPortPtr);

            taf_doip_Result_t GetNetType(char* netTypePtr);

            taf_doip_Result_t GetIfName(char* ifNamePtr);

            taf_doip_Result_t SetVin(const char* vinPtr);
            taf_doip_Result_t GetVin(char* vinPtr);

            taf_doip_Result_t SetGid(const char* gidPtr);
            taf_doip_Result_t GetGid(char* gidPtr);

        private:
            le_mem_PoolRef_t tafDoipConfigPool = NULL;
            taf_doip_Config_t *doipConfigPtr = NULL;
    };
}
}
#endif  // TAFDOIP_VEHICLE_MGR_HPP