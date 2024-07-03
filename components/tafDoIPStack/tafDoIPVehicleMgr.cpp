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

#include <iostream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "tafDoIPVehicleMgr.hpp"

using namespace taf::doip;
namespace pt = boost::property_tree;

/**
 * Returns DoipPack instance
 */
VehicleManager &VehicleManager::GetInstance()
{
    static VehicleManager instance;
    return instance;
}

void RemoveColon
(
    const char* id,
    char* outId
)
{
    int i = 0, j = 0;

    while (id[i] != '\0')
    {
        // Skip colon
        if (id[i] != ':')
        {
            outId[j] = id[i];
            j++;
        }

        i++;
    }
    outId[j] = '\0';
}

void VehicleManager::ParseJsonConfig
(
    const char* configPathPtr
)
{
    LE_INFO("ParseJsonConfig");

    if (configPathPtr == NULL)
    {
        LE_ERROR("configPathPtr is null!");
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if (vehicleMgr.tafDoipConfigPool == NULL)
    {
        vehicleMgr.tafDoipConfigPool = le_mem_CreatePool("tafDoipConfigPool",
                sizeof(taf_doip_Config_t));
    }

    vehicleMgr.doipConfigPtr
            = (taf_doip_Config_t *)le_mem_ForceAlloc(vehicleMgr.tafDoipConfigPool);

    // Read json config file
    try{
        // Create a root
        pt::ptree root;

        // Load the json file in this ptree
        pt::read_json(configPathPtr, root);

        std::string vin = root.get<std::string>("vehicleInfo.VIN");
        le_utf8_Copy(vehicleMgr.doipConfigPtr->vin, vin.c_str(), TAF_DOIP_VIN_SIZE + 1, NULL);
        LE_INFO("Parsed VIN: %s", vehicleMgr.doipConfigPtr->vin);

        std::string eidIn = root.get<std::string>("vehicleInfo.EID");
        char eidOut[eidIn.length() + 1] = {0};
        RemoveColon(eidIn.c_str(), eidOut);
        if (strlen(eidOut) != TAF_DOIP_EID_SIZE*2)
        {
            LE_ERROR("Incorrect EID length");
        }
        le_hex_StringToBinary(eidOut, TAF_DOIP_EID_SIZE*2,
                (uint8_t *)vehicleMgr.doipConfigPtr->eid, TAF_DOIP_EID_SIZE);

        std::string gidIn = root.get<std::string>("vehicleInfo.GID");
        char gidOut[gidIn.length() + 1] = {0};
        RemoveColon(gidIn.c_str(), gidOut);
        if (strlen(gidOut) != TAF_DOIP_EID_SIZE*2)
        {
            LE_ERROR("Incorrect GID length");
        }
        le_hex_StringToBinary(gidOut, TAF_DOIP_GID_SIZE*2,
                (uint8_t *)vehicleMgr.doipConfigPtr->gid, TAF_DOIP_GID_SIZE);

        std::string srcAdd = root.get<std::string>("vehicleInfo.source_address");
        vehicleMgr.doipConfigPtr->entityLA = std::stoul(srcAdd, nullptr, 16);
        LE_INFO("Parsed Source address: %x", vehicleMgr.doipConfigPtr->entityLA);

        vehicleMgr.doipConfigPtr->maxSockNum
                = root.get<uint32_t>("parameters.MCTS");

        vehicleMgr.doipConfigPtr->maxDataSize
                = root.get<uint32_t>("parameters.MDS");

        vehicleMgr.doipConfigPtr->maxAnnounceCount
                = root.get<uint32_t>("parameters.timer.A_DoIP_Announce_Num");

        vehicleMgr.doipConfigPtr->announceItrvalTime
                = root.get<uint32_t>("parameters.timer.A_DoIP_Announce_Interval");

        vehicleMgr.doipConfigPtr->genInactiveTime
                = root.get<uint32_t>("parameters.timer.T_TCP_General_Inactivity");

        vehicleMgr.doipConfigPtr->initInactiveTime
                = root.get<uint32_t>("parameters.timer.T_TCP_Initial_Inactivity");

        vehicleMgr.doipConfigPtr->aliveCheckTime
                = root.get<uint32_t>("parameters.timer.T_TCP_Alive_Check");

        vehicleMgr.doipConfigPtr->authEnableStatus
                = false;

        vehicleMgr.doipConfigPtr->authInfo
                = 0;

        std::string ifName = root.get<std::string>("network.ifname");
        le_utf8_Copy(vehicleMgr.doipConfigPtr->ifName, ifName.c_str(),
                TAF_DOIP_INTERFACE_NAME_MAX_LEN, NULL);
        LE_INFO("ifName: %s", vehicleMgr.doipConfigPtr->ifName);

        std::string netType = root.get<std::string>("network.type");
        le_utf8_Copy(vehicleMgr.doipConfigPtr->netType, netType.c_str(),
                TAF_DOIP_IPTYPE_MAX_LEN, NULL);

        vehicleMgr.doipConfigPtr->tcpPort
                = root.get<uint32_t>("network.TCP_DATA");

        vehicleMgr.doipConfigPtr->udpPort
                = root.get<uint32_t>("network.UDP_DISCOVERY");

        vehicleMgr.doipConfigPtr->parseStatus = true;
    }
    catch (std::exception const& exp)
    {
        LE_FATAL("Exception caught while reading json file: %s", exp.what());
    }

}

uint8_t VehicleManager::GetProtocolVersion
(
)
{
    LE_DEBUG("GetProtocolVersion!");

    uint8_t protocolVer;
    protocolVer = TAF_DOIP_PROTOCOL_VERSION_2012;

    return protocolVer;
}

taf_doip_Result_t VehicleManager::SetEid
(
    const char* eidPtr
)
{
    LE_DEBUG("GetEid!");

    if (eidPtr == NULL)
    {
        LE_ERROR("eidPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    char eidOut[strlen(eidPtr) + 1] = {0};
    RemoveColon(eidPtr, eidOut);
    if (strlen(eidOut) != TAF_DOIP_EID_SIZE*2)
    {
        LE_ERROR("Incorrect EID length. String is %s", eidOut);
        return TAF_DOIP_RESULT_OUT_OF_MEMORY;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    le_hex_StringToBinary(eidOut, TAF_DOIP_EID_SIZE*2, (uint8_t *)vehicleMgr.doipConfigPtr->eid,
            TAF_DOIP_EID_SIZE);

    return TAF_DOIP_RESULT_OK;
}

taf_doip_Result_t VehicleManager::GetEid
(
    char* eidPtr
)
{
    LE_DEBUG("GetEid!");

    if (eidPtr == NULL)
    {
        LE_ERROR("eidPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if(vehicleMgr.doipConfigPtr->eid[0] == '\0')
    {
        LE_ERROR("eid is not set and parsed!");
        return TAF_DOIP_RESULT_UNSET;
    }

    memcpy(eidPtr, vehicleMgr.doipConfigPtr->eid, TAF_DOIP_EID_SIZE);
    for (int i = 0; i<TAF_DOIP_EID_SIZE; i++)
    {
        LE_DEBUG("Get Eid: %02x", eidPtr[i]);
    }

    return TAF_DOIP_RESULT_OK;
}

taf_doip_Result_t VehicleManager::GetMaxConcurrentSockNum
(
    uint32_t *maxCTSPtr
)
{
    LE_DEBUG("GetMaxConcurrentSockNum!");

    if (maxCTSPtr == NULL)
    {
        LE_ERROR("maxCTSPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if ( vehicleMgr.doipConfigPtr->parseStatus == true )
    {
        *maxCTSPtr = vehicleMgr.doipConfigPtr->maxSockNum;
        return TAF_DOIP_RESULT_OK;
    }
    else
    {
        LE_ERROR("json configuration is not parsed!");
        return TAF_DOIP_RESULT_UNSET;
    }
}

taf_doip_Result_t VehicleManager::GetMaxDataSize
(
    uint32_t *maxDataSizePtr
)
{
    LE_DEBUG("GetMaxDataSize!");

    if (maxDataSizePtr == NULL)
    {
        LE_ERROR("maxDataSizePtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if ( vehicleMgr.doipConfigPtr->parseStatus == true )
    {
        *maxDataSizePtr = vehicleMgr.doipConfigPtr->maxDataSize;
        return TAF_DOIP_RESULT_OK;
    }
    else
    {
        LE_ERROR("json configuration is not parsed!");
        return TAF_DOIP_RESULT_UNSET;
    }
}

taf_doip_Result_t VehicleManager::GetMaxAnnounceCount
(
    uint32_t *announceCountPtr
)
{
    LE_DEBUG("GetMaxAnnounceCount!");

    if (announceCountPtr == NULL)
    {
        LE_ERROR("announceCountPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if ( vehicleMgr.doipConfigPtr->parseStatus == true )
    {
        *announceCountPtr = vehicleMgr.doipConfigPtr->maxAnnounceCount;
        return TAF_DOIP_RESULT_OK;
    }
    else
    {
        LE_ERROR("json configuration is not parsed!");
        return TAF_DOIP_RESULT_UNSET;
    }
}

taf_doip_Result_t VehicleManager::GetAnnounceIntervalTime
(
    uint32_t *announceIntTimePtr
)
{
    LE_DEBUG("GetAnnounceIntervalTime!");

    if (announceIntTimePtr == NULL)
    {
        LE_ERROR("announceIntTimePtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if ( vehicleMgr.doipConfigPtr->parseStatus == true )
    {
        *announceIntTimePtr = vehicleMgr.doipConfigPtr->announceItrvalTime;
        return TAF_DOIP_RESULT_OK;
    }
    else
    {
        LE_ERROR("json configuration is not parsed!");
        return TAF_DOIP_RESULT_UNSET;
    }
}

taf_doip_Result_t VehicleManager::GetGenInactivityTime
(
    uint32_t *genInactiveTimePtr
)
{
    LE_DEBUG("GetGenInactivityTime!");

    if (genInactiveTimePtr == NULL)
    {
        LE_ERROR("genInactiveTimePtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if ( vehicleMgr.doipConfigPtr->parseStatus == true )
    {
        *genInactiveTimePtr = vehicleMgr.doipConfigPtr->genInactiveTime;
        return TAF_DOIP_RESULT_OK;
    }
    else
    {
        LE_ERROR("json configuration is not parsed!");
        return TAF_DOIP_RESULT_UNSET;
    }
}

taf_doip_Result_t VehicleManager::GetInitialInactivityTime
(
    uint32_t *initInactiveTimePtr
)
{
    LE_DEBUG("GetInitialInactivityTime!");

    if (initInactiveTimePtr == NULL)
    {
        LE_ERROR("initInactiveTimePtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if ( vehicleMgr.doipConfigPtr->parseStatus == true )
    {
        *initInactiveTimePtr = vehicleMgr.doipConfigPtr->initInactiveTime;
        return TAF_DOIP_RESULT_OK;
    }
    else
    {
        LE_ERROR("json configuration is not parsed!");
        return TAF_DOIP_RESULT_UNSET;
    }
}

taf_doip_Result_t VehicleManager::GetAliveCheckTime
(
    uint32_t *aliveCheckTimePtr
)
{
    LE_DEBUG("GetAliveCheckTime!");

    if (aliveCheckTimePtr == NULL)
    {
        LE_ERROR("aliveCheckTimePtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if ( vehicleMgr.doipConfigPtr->parseStatus == true )
    {
        *aliveCheckTimePtr = vehicleMgr.doipConfigPtr->aliveCheckTime;
        return TAF_DOIP_RESULT_OK;
    }
    else
    {
        LE_ERROR("json configuration is not parsed!");
        return TAF_DOIP_RESULT_UNSET;
    }
}

taf_doip_Result_t VehicleManager::SetEntityLogicalAddr
(
    uint16_t entityLA
)
{
    LE_DEBUG("SetEntityLogicalAddr!");

    auto &vehicleMgr = VehicleManager::GetInstance();

    vehicleMgr.doipConfigPtr->entityLA = entityLA;
    return TAF_DOIP_RESULT_OK;
}

taf_doip_Result_t VehicleManager::GetEntityLogicalAddr
(
    uint16_t *entityLAPtr
)
{
    LE_DEBUG("GetEntityLogicalAddr!");

    if (entityLAPtr == NULL)
    {
        LE_ERROR("entityLAPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if ( vehicleMgr.doipConfigPtr->parseStatus == true )
    {
        *entityLAPtr = vehicleMgr.doipConfigPtr->entityLA;
        return TAF_DOIP_RESULT_OK;
    }
    else
    {
        LE_ERROR("json configuration is not parsed!");
        return TAF_DOIP_RESULT_UNSET;
    }
}

bool VehicleManager::GetAuthEnableStatus
(
)
{
    LE_DEBUG("GetAuthEnableStatus!");

    auto &vehicleMgr = VehicleManager::GetInstance();

    return vehicleMgr.doipConfigPtr->authEnableStatus;
}

taf_doip_Result_t VehicleManager::GetAuthInfo
(
    uint32_t *authInfoPtr
)
{
    LE_DEBUG("GetAuthInfo!");

    if (authInfoPtr == NULL)
    {
        LE_ERROR("authInfoPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if ( vehicleMgr.doipConfigPtr->parseStatus == true )
    {
        *authInfoPtr = vehicleMgr.doipConfigPtr->authInfo;
        return TAF_DOIP_RESULT_OK;
    }
    else
    {
        LE_ERROR("json configuration is not parsed!");
        return TAF_DOIP_RESULT_UNSET;
    }
}

taf_doip_Result_t VehicleManager::GetIfName
(
    char* ifNamePtr
)
{
    LE_DEBUG("GetIfName!");

    if (ifNamePtr == NULL)
    {
        LE_ERROR("ifNamePtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if ( vehicleMgr.doipConfigPtr->parseStatus == true )
    {
        le_utf8_Copy(ifNamePtr, vehicleMgr.doipConfigPtr->ifName,
                TAF_DOIP_INTERFACE_NAME_MAX_LEN, NULL);
    }
    else
    {
        LE_ERROR("json configuration is not parsed!");
        return TAF_DOIP_RESULT_ERROR;
    }

    return TAF_DOIP_RESULT_OK;
}

taf_doip_Result_t VehicleManager::GetNetType
(
    char* netTypePtr
)
{
    LE_DEBUG("GetNetType!");

    if (netTypePtr == NULL)
    {
        LE_ERROR("netTypePtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if ( vehicleMgr.doipConfigPtr->parseStatus == true )
    {
        le_utf8_Copy(netTypePtr, vehicleMgr.doipConfigPtr->netType,
                TAF_DOIP_IPTYPE_MAX_LEN, NULL);
    }
    else
    {
        LE_ERROR("json configuration is not parsed!");
        return TAF_DOIP_RESULT_ERROR;
    }

    return TAF_DOIP_RESULT_OK;
}

taf_doip_Result_t VehicleManager::GetTcpPort
(
    uint16_t *tcpPortPtr
)
{
    LE_DEBUG("GetTcpPort!");

    if (tcpPortPtr == NULL)
    {
        LE_ERROR("tcpPortPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if ( vehicleMgr.doipConfigPtr->parseStatus == true )
    {
        *tcpPortPtr = vehicleMgr.doipConfigPtr->tcpPort;
        return TAF_DOIP_RESULT_OK;
    }
    else
    {
        LE_ERROR("json configuration is not parsed!");
        return TAF_DOIP_RESULT_UNSET;
    }
}

taf_doip_Result_t VehicleManager::GetUdpPort
(
    uint16_t *udpPortPtr
)
{
    LE_DEBUG("GetUdpPort!");

    if (udpPortPtr == NULL)
    {
        LE_ERROR("udpPortPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if ( vehicleMgr.doipConfigPtr->parseStatus == true )
    {
        *udpPortPtr = vehicleMgr.doipConfigPtr->udpPort;
        return TAF_DOIP_RESULT_OK;
    }
    else
    {
        LE_ERROR("json configuration is not parsed!");
        return TAF_DOIP_RESULT_UNSET;
    }
}

taf_doip_Result_t VehicleManager::GetMulticast
(
    char* multicastPtr
)
{
    LE_DEBUG("GetMulticast!");

    if (multicastPtr == NULL)
    {
        LE_ERROR("multicastPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    le_utf8_Copy(multicastPtr, TAF_DOIP_UDP_BROADCAST_IP, TAF_DOIP_IP_ADDR_MAX_LEN, NULL);

    return TAF_DOIP_RESULT_OK;
}

taf_doip_Result_t VehicleManager::SetVin
(
    const char* vinPtr
)
{
    LE_DEBUG("SetVin!");

    if (vinPtr == NULL || strlen(vinPtr) != TAF_DOIP_VIN_SIZE)
    {
        LE_ERROR("VinPtr is null or vinPtr length is not correct");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    le_utf8_Copy(vehicleMgr.doipConfigPtr->vin, vinPtr, TAF_DOIP_VIN_SIZE + 1, NULL);
    LE_DEBUG("Set VIN is %s!", vehicleMgr.doipConfigPtr->vin);

    return TAF_DOIP_RESULT_OK;
}

taf_doip_Result_t VehicleManager::GetVin
(
    char* vinPtr
)
{
    LE_DEBUG("GetVin!");

    if (vinPtr == NULL)
    {
        LE_ERROR("vinPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if(vehicleMgr.doipConfigPtr->vin[0] == '\0')
    {
        LE_ERROR("vin is not set and parsed!");
        return TAF_DOIP_RESULT_UNSET;
    }

    le_utf8_Copy(vinPtr, vehicleMgr.doipConfigPtr->vin, TAF_DOIP_VIN_SIZE + 1, NULL);
    LE_DEBUG("Get VIN is %s!", vinPtr);

    return TAF_DOIP_RESULT_OK;
}

taf_doip_Result_t VehicleManager::SetGid
(
    const char* gidPtr
)
{
    LE_DEBUG("SetGid!");

    if (gidPtr == NULL)
    {
        LE_ERROR("gidPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    char gidOut[strlen(gidPtr) + 1] = {0};
    RemoveColon(gidPtr, gidOut);
    if (strlen(gidOut) != TAF_DOIP_GID_SIZE*2)
    {
        LE_ERROR("Incorrect GID length. String is %s", gidOut);
        return TAF_DOIP_RESULT_OUT_OF_MEMORY;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    le_hex_StringToBinary(gidPtr, strlen(gidPtr), (uint8_t *)vehicleMgr.doipConfigPtr->gid,
            TAF_DOIP_GID_SIZE);

    LE_DEBUG("Set Gid pointer: %s", gidPtr);

    return TAF_DOIP_RESULT_OK;
}

taf_doip_Result_t VehicleManager::GetGid
(
    char* gidPtr
)
{
    LE_DEBUG("GetGid!");

    if (gidPtr == NULL)
    {
        LE_ERROR("gidPtr is null!");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    if(vehicleMgr.doipConfigPtr->gid[0] == '\0')
    {
        LE_ERROR("Gid is not set and paresed!");
        return TAF_DOIP_RESULT_UNSET;
    }

    memcpy(gidPtr, vehicleMgr.doipConfigPtr->gid, TAF_DOIP_GID_SIZE);
    for (int i = 0; i<TAF_DOIP_GID_SIZE; i++)
    {
        LE_DEBUG("Get Gid: %02x", gidPtr[i]);
    }

    return TAF_DOIP_RESULT_OK;
}

taf_doip_Result_t VehicleManager::DeInit
(
)
{
    LE_INFO("DeInitialization!");

    auto &vehicleMgr = VehicleManager::GetInstance();

    if(vehicleMgr.doipConfigPtr != NULL)
    {
        le_mem_Release(vehicleMgr.doipConfigPtr);
        vehicleMgr.doipConfigPtr = NULL;
    }

    return TAF_DOIP_RESULT_OK;
}

void VehicleManager::Init
(
    const char* configPathPtr
)
{
    LE_INFO("VehicleManager Initialization!");

    if (configPathPtr == NULL)
    {
        LE_ERROR("configPathPtr is null!");
    }

    auto &vehicleMgr = VehicleManager::GetInstance();

    // parse json configuration
    vehicleMgr.ParseJsonConfig(configPathPtr);
}