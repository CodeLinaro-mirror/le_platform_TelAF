/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef TAFDOIP_VEHICLE_MGR_HPP
#define TAFDOIP_VEHICLE_MGR_HPP

#include "legato.h"
#include "interfaces.h"

#include "tafDoIPStack.h"
#include "tafDoIPCommon.hpp"

typedef struct
{
    le_dls_Link_t link;
    uint16_t groupAddr;
}taf_doip_FuncGroup_t;

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
    le_dls_List_t ifaceList;  // Interface list
    char netType[TAF_DOIP_IPTYPE_MAX_LEN];
    uint16_t udpPort;
    uint16_t tcpPort;
    uint16_t udpSrc;    // UDP source port.
    le_dls_List_t funcGroupList;
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

            taf_doip_Result_t GetUdpSrcPort(uint16_t *udpSrcPortPtr);

            taf_doip_Result_t GetNetType(char* netTypePtr);

            taf_doip_Result_t SetVin(const char* vinPtr);
            taf_doip_Result_t GetVin(char* vinPtr);

            taf_doip_Result_t SetGid(const char* gidPtr);
            taf_doip_Result_t GetGid(char* gidPtr);

            bool IsFunctionalAddress(uint16_t logicalAddr);
            le_dls_List_t *GetIfaceList();
        private:
            le_mem_PoolRef_t tafDoipConfigPool = NULL;
            le_mem_PoolRef_t funcGroupPool = NULL;
            le_mem_PoolRef_t ifNamePool = NULL;
            taf_doip_Config_t *doipConfigPtr = NULL;
    };
}
}
#endif  // TAFDOIP_VEHICLE_MGR_HPP