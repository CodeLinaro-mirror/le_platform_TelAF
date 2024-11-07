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

#include "legato.h"
#include "interfaces.h"
#include "messagingMessage.h"
#include "rpcProxyConfig.h"

//--------------------------------------------------------------------------------------------------
/**
 * Config system Link data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct RpcConfigSystem
{
    SystemLink_t system;                    ///< Config system info
    le_dls_Link_t link;                     ///< Link to the system list
}RpcConfigSystem_t;


//--------------------------------------------------------------------------------------------------
/**
 * Config rpc_service link struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct RpcConfigService
{
    ServiceLink_t service;                  ///< Config service info
    uint16_t portNum;                       ///< Config port number
    uint16_t responseTimeout;               ///< Config response timeout
    bool isReliable;                        ///< Config connection type
    le_dls_List_t offerSystemList;          ///< Config offer systemId list
    le_dls_Link_t link;                     ///< Link to service list
}RpcConfigService_t;


//--------------------------------------------------------------------------------------------------
/**
 * Config OfferSystemId Link data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct OfferSystemId
{
    SystemId_t  id;                         ///< Offer systemId
    le_dls_Link_t link;                     ///< Link to the offer system list of service
}OfferSystemId_t;


//--------------------------------------------------------------------------------------------------
/**
 * Baned service data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct BanedService
{
    char user[LIMIT_MAX_USER_NAME_BYTES];           ///< User name
    char name[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES];  ///< Interface name
}BanedService_t;


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool to store the system link info in JSON config file
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(RpcConfigSystemPool, RPC_MAX_SYSTEMS,
                                   sizeof(RpcConfigSystem_t));
static le_mem_PoolRef_t RpcConfigSystemPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool to store the offer system ID in JSON config file
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(OfferSystemIdPool, RPC_MAX_SYSTEMS*RPC_MAX_SERVICES,
                                   sizeof(OfferSystemId_t));
static le_mem_PoolRef_t RpcOfferSystemIdPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool to store the rpc service in JSON config file
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(RpcConfigServicePool, RPC_MAX_SERVICES,
                                   sizeof(RpcConfigService_t));
static le_mem_PoolRef_t RpcConfigServicePoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Config system list.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t ConfigSystemList = LE_DLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * Config service list.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t ConfigServiceList = LE_DLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * Config response timeout.
 */
//--------------------------------------------------------------------------------------------------
static uint16_t ResponseTimeoutSecs = TAF_RPC_DEFAULT_RESP_TIMEOUT;


//--------------------------------------------------------------------------------------------------
/**
 * Config base service ID.
 */
//--------------------------------------------------------------------------------------------------
static ServiceId_t BaseServiceId = TAF_RPC_DEFAULT_BASE_SERVICE_ID;


//--------------------------------------------------------------------------------------------------
/**
 * Config base port number.
 */
//--------------------------------------------------------------------------------------------------
static uint16_t BasePortNumber = TAF_RPC_DEFAULT_BASE_PORT_NUMBER;


//--------------------------------------------------------------------------------------------------
/**
 * Config routing dev.
 */
//--------------------------------------------------------------------------------------------------
static char RoutingDev[TAF_SOMEIPDEF_MAX_IFNAME_LENGTH] = { 0 };
static const char* RoutingDevPtr = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * The static configuration data struct, can be overrided by the configuration in JSON file is a
 * JSON file is specified.
 */
//--------------------------------------------------------------------------------------------------
static RpcConfigData_t MyConfiguration;


//--------------------------------------------------------------------------------------------------
/**
 * Black services list which are not allowed to declare as RPC services.
 */
//--------------------------------------------------------------------------------------------------
static const BanedService_t BanedServiceList[] =
{
    // Core service APIs.
    {"root", "sdirTool"},
    {"root", "logFd"},
    {"root", "logDaemonWdog"},
    {"root", "LogClient"},
    {"root", "LogControl"},
    {"root", "le_cfg"},
    {"root", "le_cfgAdmin"},
    {"root", "configTreeWdog"},
    {"root", "le_update"},
    {"root", "le_appRemove"},
    {"root", "le_instStat"},
    {"root", "le_updateCtrl"},
    {"root", "updateDaemonWdog"},
    {"root", "DeviceManager"},
    {"root", "DeviceManagerTool"},
    {"root", "le_appCtrl"},
    {"root", "le_framework"},
    {"root", "wdog"},
    {"root", "supervisorWdog"},
    {"root", "le_appInfo"},
    {"root", "le_appProc"},
    {"root", "le_ima"},
    {"root", "le_kernelModule"},
    {"root", "le_wdog"},

    // Platform service APIs.
    {"telaf", "taf_someipSvr"},
    {"telaf", "taf_someipClnt"},
    {"telaf", "taf_ks"},
    {"telaf", "taf_fsc"}
};


//--------------------------------------------------------------------------------------------------
/**
 * Data struct initialization function.
 */
//--------------------------------------------------------------------------------------------------
static void DataInit
(
    void
)
{
    // Initialize the RPC configSystem pool.
    RpcConfigSystemPoolRef = le_mem_InitStaticPool(RpcConfigSystemPool,
                                                   RPC_MAX_SYSTEMS,
                                                   sizeof(RpcConfigSystem_t));

    // Initialize the Offer systemId pool.
    RpcOfferSystemIdPoolRef = le_mem_InitStaticPool(OfferSystemIdPool,
                                                    RPC_MAX_SYSTEMS*RPC_MAX_SERVICES,
                                                    sizeof(OfferSystemId_t));

    // Initialize the RPC configService pool.
    RpcConfigServicePoolRef = le_mem_InitStaticPool(RpcConfigServicePool,
                                                    RPC_MAX_SERVICES,
                                                    sizeof(RpcConfigService_t));
                       
    // Initialize the global configuration data struct.
    memset(&MyConfiguration, 0, sizeof(struct RpcConfigData));
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if the systemName is a valid string. It only contains [A-Z][a-z][0-9] and "_".
 */
//--------------------------------------------------------------------------------------------------
static bool IsValidSystemNameString(const char* str)
{
    while (*str)
    {
        if (!isalnum((unsigned char)*str) && *str != '_')
        {
            return false;
        }

        str++;
    }
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set my system link configuration.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetMySystemLinkInfo
(
    SystemId_t id,                  /// <IN> my system ID
    const char* nameStr,            /// <IN> my system name string
    uint16_t index                  /// <IN> my system index
)
{
    // Parameter check.
    if ((index == 0) || (nameStr == NULL) || (id == 0))
    {
        LE_ERROR("Invalid parameters.");
        return LE_BAD_PARAMETER;
    }

    // Check if my system link info is already loaded.
    if ((MyConfiguration.mySystem.id != 0) || (MyConfiguration.mySystem.index != 0))
    {
        LE_ERROR("My system link configuration is already loaded.");
        return LE_DUPLICATE;
    }

    // Save my system link configuration.
    MyConfiguration.mySystem.id = id;
    MyConfiguration.mySystem.index = index;
    le_utf8_Copy(MyConfiguration.mySystem.name, nameStr,
                 LIMIT_MAX_SYSTEM_NAME_BYTES, NULL);

    LE_INFO("Set my system link info (index=%d, id=0x%x, name=%s).", index, id, nameStr);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set a remote system link configuration.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetRemoteSystemLinkInfo
(
    SystemId_t id,                 /// <IN> remote system ID
    const char* nameStr,           /// <IN> remote system name string
    uint16_t index                 /// <IN> remote system index
)
{
    // Parameter check.
    if ((index == 0) || (nameStr == NULL) || (id == 0))
    {
        LE_ERROR("Invalid parameters.");
        return LE_BAD_PARAMETER;
    }

    // Check the system number.
    uint8_t cnt = MyConfiguration.remoteSystemCnt;
    if (cnt >= RPC_MAX_SYSTEMS)
    {
        LE_ERROR("System number gets overflowed.");
        return LE_OVERFLOW;
    }

    // Add the system link info.
    MyConfiguration.remoteSystems[cnt].id = id;
    MyConfiguration.remoteSystems[cnt].index = index;
    le_utf8_Copy(MyConfiguration.remoteSystems[cnt].name, nameStr,
                 LIMIT_MAX_SYSTEM_NAME_BYTES, NULL);

    cnt++;
    MyConfiguration.remoteSystemCnt = cnt;

    LE_INFO("Set remote system link info (index=%d, id=0x%x, name=%s).", index, id, nameStr);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set routing_name configuration.
 */
//--------------------------------------------------------------------------------------------------
static void SetRoutingName
(
    const char* devNamePtr           /// <IN> Routing device name
)
{
    if (devNamePtr != NULL)
    {
        le_utf8_Copy(RoutingDev, devNamePtr, sizeof(RoutingDev), NULL);
        RoutingDevPtr = RoutingDev;
        LE_INFO("Set routing_name = %s", RoutingDevPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set base_service_id configuration.
 */
//--------------------------------------------------------------------------------------------------
static void SetBaseServiceId
(
    ServiceId_t id                        /// <IN> base service ID
)
{
    BaseServiceId = id;

    LE_INFO("Set base_service_id = 0x%X.", BaseServiceId);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set base_port_number configuration.
 */
//--------------------------------------------------------------------------------------------------
static void SetBasePortNumber
(
    uint16_t num                         /// <IN> base port number
)
{
    BasePortNumber = num;

    LE_INFO("Set base_port_number = %d.", BasePortNumber);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set response_timeout configuration.
 */
//--------------------------------------------------------------------------------------------------
static void SetResponseTimeout
(
    uint16_t secs                        /// <IN> response timeout seconds
)
{
    ResponseTimeoutSecs = secs;

    LE_INFO("Set response_timeout = %d.", ResponseTimeoutSecs);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set an offerService entry
 */
//--------------------------------------------------------------------------------------------------
static void SetOfferServiceEntry
(
    OfferServiceConfigEntry_t* dstPtr,       /// <IN> Destination entry addr.
    const RpcConfigService_t* srcPtr         /// <IN> Source entry addr
)
{
    uint8_t cnt;

    LE_ASSERT((dstPtr != NULL) && (srcPtr != NULL));

    dstPtr->service.id = srcPtr->service.id;
    le_utf8_Copy(dstPtr->service.name, srcPtr->service.name,
                 LIMIT_MAX_IPC_INTERFACE_NAME_BYTES, NULL);
    le_utf8_Copy(dstPtr->service.user, srcPtr->service.user,
                 LIMIT_MAX_USER_NAME_BYTES, NULL);
    le_utf8_Copy(dstPtr->service.protocolId, srcPtr->service.protocolId,
                 LIMIT_MAX_PROTOCOL_ID_BYTES, NULL);

    dstPtr->port.isReliable = srcPtr->isReliable;
    dstPtr->port.number = srcPtr->portNum;

    dstPtr->systemCnt = MyConfiguration.remoteSystemCnt;

    for (cnt = 0; cnt < MyConfiguration.remoteSystemCnt; cnt++)
    {
        dstPtr->clientSystems[cnt] = MyConfiguration.remoteSystems[cnt].id;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set a requestService entry
 */
//--------------------------------------------------------------------------------------------------
static void SetRequestServiceEntry
(
    RequestServiceConfigEntry_t* dstPtr,      /// <IN> Destination entry addr
    const RpcConfigService_t* srcPtr,         /// <IN> Source entry addr
    const SystemId_t* serverSystemTablePtr,   /// <IN> Server systemId table
    const uint8_t tableSize                   /// <IN> Server systemId table size.
)
{
    uint8_t cnt;

    LE_ASSERT((dstPtr != NULL) && (srcPtr != NULL));
    LE_ASSERT((serverSystemTablePtr != NULL) && (tableSize > 0));

    dstPtr->service.id = srcPtr->service.id;
    le_utf8_Copy(dstPtr->service.name, srcPtr->service.name,
                 LIMIT_MAX_IPC_INTERFACE_NAME_BYTES, NULL);
    le_utf8_Copy(dstPtr->service.user, srcPtr->service.user,
                 LIMIT_MAX_USER_NAME_BYTES, NULL);
    le_utf8_Copy(dstPtr->service.protocolId, srcPtr->service.protocolId,
                 LIMIT_MAX_PROTOCOL_ID_BYTES, NULL);

    dstPtr->isReliable = srcPtr->isReliable;
    dstPtr->responseTimeout = srcPtr->responseTimeout;

    dstPtr->systemCnt = tableSize;

    for (cnt = 0; cnt < tableSize; cnt++)
    {
        dstPtr->serverSystems[cnt] = serverSystemTablePtr[cnt];
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * populate offerService configuration array.
 */
//--------------------------------------------------------------------------------------------------
static void PopulateOfferServiceConfigArray
(
    void
)
{
    OfferServiceConfigEntry_t* offerServicePtr = &MyConfiguration.offerServices[0];
    uint8_t offerServiceCnt = 0;

    le_dls_Link_t* linkPtr = NULL;
    le_dls_Link_t* subLinkPtr = NULL;
    RpcConfigService_t* serviceLinkPtr = NULL;
    OfferSystemId_t* systemIdPtr = NULL;

    SystemId_t mySystemId = MyConfiguration.mySystem.id;


    // Sanity check for the size of configService list.
    LE_ASSERT(le_dls_NumLinks(&ConfigServiceList) <= RPC_MAX_SERVICES)
    linkPtr = le_dls_Peek(&ConfigServiceList);
    while (linkPtr != NULL)
    {
        serviceLinkPtr = CONTAINER_OF(linkPtr, RpcConfigService_t, link);

        // Sanity check for the size of offerSystem list.
        LE_ASSERT(le_dls_NumLinks(&serviceLinkPtr->offerSystemList) <= RPC_MAX_SYSTEMS);
        subLinkPtr = le_dls_Peek(&serviceLinkPtr->offerSystemList);
        while (subLinkPtr != NULL)
        {
            systemIdPtr = CONTAINER_OF(subLinkPtr, OfferSystemId_t, link);

            if (systemIdPtr->id == mySystemId)
            {
                // My system offers this service, thus create a offer service entry
                // and add it into the offerService list.
                SetOfferServiceEntry(offerServicePtr, serviceLinkPtr);
                offerServicePtr++;
                offerServiceCnt++;
                break;
            }

            subLinkPtr = le_dls_PeekNext(&serviceLinkPtr->offerSystemList, subLinkPtr);
        }

        linkPtr = le_dls_PeekNext(&ConfigServiceList, linkPtr);
    }

    MyConfiguration.offerServiceCnt = offerServiceCnt;

    LE_INFO("populated offer service configuration array (number=%u).", offerServiceCnt);
}


//--------------------------------------------------------------------------------------------------
/**
 * populate requestService configuration array.
 */
//--------------------------------------------------------------------------------------------------
static void PopulateRequestServiceConfigArray
(
    void
)
{
    RequestServiceConfigEntry_t* requestServicePtr = &MyConfiguration.requestServices[0];
    uint8_t requestServiceCnt = 0;

    le_dls_Link_t* linkPtr = NULL;
    le_dls_Link_t* subLinkPtr = NULL;
    RpcConfigService_t* serviceLinkPtr = NULL;
    OfferSystemId_t* systemIdPtr = NULL;

    SystemId_t mySystemId = MyConfiguration.mySystem.id;
    SystemId_t serverSystem[RPC_MAX_SYSTEMS] = { 0 };
    uint8_t serverSystemCnt = 0;


    // Sanity check for the size of configService list.
    LE_ASSERT(le_dls_NumLinks(&ConfigServiceList) <= RPC_MAX_SERVICES);
    linkPtr = le_dls_Peek(&ConfigServiceList);
    while (linkPtr != NULL)
    {
        serviceLinkPtr = CONTAINER_OF(linkPtr, RpcConfigService_t, link);

        // Sanity check for the size of offerSystem list.
        LE_ASSERT(le_dls_NumLinks(&serviceLinkPtr->offerSystemList) <= RPC_MAX_SYSTEMS);
        subLinkPtr = le_dls_Peek(&serviceLinkPtr->offerSystemList);
        serverSystemCnt = 0;
        while (subLinkPtr != NULL)
        {
            systemIdPtr = CONTAINER_OF(subLinkPtr, OfferSystemId_t, link);

            if (systemIdPtr->id != mySystemId)
            {
                // The service is offered by a remote system,
                // thus we need to add it into the requestService list.
                serverSystem[serverSystemCnt] = systemIdPtr->id;
                serverSystemCnt++;
            }

            subLinkPtr = le_dls_PeekNext(&serviceLinkPtr->offerSystemList, subLinkPtr);
        }

        if (serverSystemCnt > 0)
        {
            // Create a requestService config entry for this service.
            SetRequestServiceEntry(requestServicePtr, serviceLinkPtr,
                                   serverSystem, serverSystemCnt);
            requestServicePtr++;
            requestServiceCnt++;
        }

        linkPtr = le_dls_PeekNext(&ConfigServiceList, linkPtr);
    }

    MyConfiguration.requestServiceCnt = requestServiceCnt;

    LE_INFO("populated request service configuration array (number=%u).", requestServiceCnt);
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if the system link info is valid and return systemId if so.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ValidateSystemLinkInfo
(
    const char* idStr,                  ///< [IN] system ID string
    const char* nameStr,                ///< [IN] system name string
    SystemId_t* idPtr                   ///< [OUT] system ID
)
{
    // Parameter check.
    if ((idStr == NULL) || (nameStr == NULL) || (idPtr == NULL))
    {
        LE_ERROR("Invalid prameters.");
        return LE_BAD_PARAMETER;
    }

    unsigned long idValue = 0;
    sscanf(idStr, "%lx", &idValue);

    // Check system ID range. Shall be 0x01 - 0xFFFF.
    if ((idValue < 1) || (idValue > 65535))
    {
        LE_ERROR("Invalid system id.");
        return LE_OVERFLOW;
    }

    // Check system name size.
    if (strlen(nameStr) > LIMIT_MAX_SYSTEM_NAME_LEN)
    {
        LE_ERROR("Invalid system name size.");
        return LE_OVERFLOW;
    }

    // Check if system name is [A-Z][a-z][0-9]and "_"
    if (!IsValidSystemNameString(nameStr))
    {
        LE_ERROR("Invalid system name string.");
        return LE_FAULT;
    }

    // Return system Id.
    *idPtr = (SystemId_t)idValue;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check base service Id in configuration file.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ValidateBaseServiceId
(
    const char* idStr,                    /// <IN> base service ID string
    ServiceId_t* idPtr                    /// <OUT> base service ID
)
{
    // Parameter check.
    if ((idStr == NULL) || (idPtr == NULL))
    {
        LE_ERROR("Invalid parameters.");
        return LE_BAD_PARAMETER;
    }

    unsigned long value = 0;
    sscanf(idStr, "%lx", &value);

    // Check the service ID range.
    if ((value > 0xFF00 - RPC_MAX_SERVICES) || (value < 0x0001))
    {
        LE_ERROR("Invalid base_service_id.");
        return LE_OVERFLOW;
    }

    // Return service Id.
    *idPtr = (ServiceId_t)value;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check base port number in configuration file.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ValidateBasePortNumber
(
    const char* numStr,                  /// <IN> port number string
    uint16_t* numPtr                     /// <OUT> port number
)
{
    // Parameter check.
    if ((numStr == NULL) || (numPtr == NULL))
    {
        LE_ERROR("Invalid parameters.");
        return LE_BAD_PARAMETER;
    }

    char *endptr;
    uint32_t num = 0;
    num = (uint32_t)strtoul(numStr, &endptr, 10);

    // Check the port number range.
    if ((num < 1024) || (num > 65535))
    {
        LE_ERROR("Invalid base_port_number.");
        return LE_OVERFLOW;
    }

    // Return the port number.
    *numPtr = (uint16_t)num;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check response timeout seconds in configuration file.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ValidateResponseTimeout
(
    const char* secStr,               /// <IN> timeout second string
    uint16_t* secPtr                  /// <OUT> timeout second
)
{
    // Parameter check.
    if ((secStr == NULL) || (secPtr == NULL))
    {
        LE_ERROR("Invalid parameters.");
        return LE_BAD_PARAMETER;
    }

    char *endptr;
    uint32_t secs = 0;
    secs = (uint32_t)strtoul(secStr, &endptr, 10);

    // Check the timeout seconds range.
    if ((secs < 1) || (secs > 300))
    {
        LE_ERROR("Invalid response_timeout seconds.");
        return LE_OVERFLOW;
    }

    // Return the timeout secs.
    *secPtr = (uint16_t)secs;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Validate mandatory RPC serviceLink info.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ValidateServiceLinkInfo
(
    const char* nameStr,                    /// <IN> service name string
    const char* userStr,                    /// <IN> service user string
    const char* versionStr                  /// <IN> service version string
)
{
    // Parameter check.
    if ((nameStr == NULL) || (userStr == NULL) || (versionStr == NULL))
    {
        LE_ERROR("Invalid parameters.");
        return LE_BAD_PARAMETER;
    }

    if (strlen(nameStr) > LIMIT_MAX_IPC_INTERFACE_NAME_BYTES -1)
    {
        LE_ERROR("Invalid service name size.");
        return LE_OVERFLOW;
    }

    if (strlen(userStr) > LIMIT_MAX_USER_NAME_LEN)
    {
        LE_ERROR("Invalid service user size.");
        return LE_OVERFLOW;
    }

    if (strlen(versionStr) > LIMIT_MAX_PROTOCOL_ID_BYTES - 1)
    {
        LE_ERROR("Invalid service version size.");
        return LE_OVERFLOW;
    }

    // Check if the service is a baned service.
    const BanedService_t* servicePtr = BanedServiceList;
    uint32_t count = NUM_ARRAY_MEMBERS(BanedServiceList);
    while(count--)
    {
        if ((strcmp(userStr, servicePtr->user) == 0) &&
            (strcmp(nameStr, servicePtr->name) == 0))
        {
            LE_ERROR("Baned service(<%s>.%s) found.", userStr, nameStr);
            return LE_NOT_PERMITTED;
        }

        servicePtr++;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * validate RPC service optional info.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ValidateServiceOptInfo
(
    const char* connectionTypeStr,
    const char* responseTimeoutStr,
    bool* isReliablePtr,
    uint16_t* responseTimeoutPtr
)
{
    // Parameter check.
    if ((isReliablePtr == NULL) || (responseTimeoutPtr == NULL))
    {
        LE_ERROR("Invalid parameter.");
        return LE_BAD_PARAMETER;
    }

    // Set default values for connection type and response timeout secs.
    *isReliablePtr = true;
    *responseTimeoutPtr = ResponseTimeoutSecs;

    // connection_type field is set.
    if (connectionTypeStr != NULL)
    {
        if ((strcmp(connectionTypeStr, "TCP") == 0) ||
            (strcmp(connectionTypeStr, "tcp") == 0))
        {
            *isReliablePtr = true;
        }
        else if ((strcmp(connectionTypeStr, "UDP") == 0) ||
                 (strcmp(connectionTypeStr, "udp") == 0))
        {
            *isReliablePtr = false;
        }
        else
        {
            LE_ERROR("Invalid connection_type.");
            return LE_FAULT;
        }
    }

    // response_timeout field is set.
    if (responseTimeoutStr != NULL)
    {
        char *endptr;
        uint32_t secs = 0;
        secs = (uint32_t)strtoul(responseTimeoutStr, &endptr, 10);

        // Check the response timeout secs range.
        if ((secs < 1) || (secs > 300))
        {
            LE_ERROR("Invalid response_timeout.");
            return LE_OVERFLOW;
        }

        // Return the response timeout secs for this service.
        *responseTimeoutPtr = (uint16_t)secs;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if the offer systemId string is valid and return the systemId if so.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ValidateOfferSystemId
(
    const char* idStr,                  ///< [IN] system ID string
    SystemId_t* idPtr                   ///< [OUT] system ID
)
{
    SystemId_t systemId = 0;
    le_dls_Link_t* linkPtr = NULL;
    RpcConfigSystem_t* systemLinkPtr = NULL;


    // Parameter check.
    if ((idStr == NULL) || (idPtr == NULL))
    {
        LE_ERROR("Invalid prameters.");
        return LE_BAD_PARAMETER;
    }

    // Check system name size.
    if (strlen(idStr) > LIMIT_MAX_SYSTEM_NAME_LEN)
    {
        LE_ERROR("Invalid system name size.");
        return LE_OVERFLOW;
    }

    // First try to parse the idStr as a system name.
    linkPtr = le_dls_Peek(&ConfigSystemList);
    while (linkPtr != NULL)
    {
        systemLinkPtr = CONTAINER_OF(linkPtr, RpcConfigSystem_t, link);

        if (strcmp(systemLinkPtr->system.name, idStr) == 0)
        {
            systemId = systemLinkPtr->system.id;
            break;
        }

        linkPtr = le_dls_PeekNext(&ConfigSystemList, linkPtr);
    }

    // Valid system name found, return the systemId.
    if (systemId != 0)
    {
        // Return system Id.
        *idPtr = systemId;
        return LE_OK;
    }

    // Then parse the idStr as a hex value ID .
    unsigned long idValue = 0;
    sscanf(idStr, "%lx", &idValue);

    linkPtr = le_dls_Peek(&ConfigSystemList);
    while (linkPtr != NULL)
    {
        systemLinkPtr = CONTAINER_OF(linkPtr, RpcConfigSystem_t, link);

        if (systemLinkPtr->system.id == (uint16_t)idValue)
        {
            systemId = systemLinkPtr->system.id;
            break;
        }

        linkPtr = le_dls_PeekNext(&ConfigSystemList, linkPtr);
    }

    if (systemId != 0)
    {
        // Return system Id.
        *idPtr = (SystemId_t)idValue;
        return LE_OK;
    }

    LE_INFO("idValueo=0x%x", (SystemId_t)idValue);
    return LE_NOT_FOUND;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check a given config system is already in the list.
 */
//--------------------------------------------------------------------------------------------------
static bool IsConfigSystemInList
(
    le_dls_List_t* configSystemList,
    RpcConfigSystem_t* configSystemPtr
)
{
    LE_ASSERT((configSystemList != NULL) && (configSystemPtr != NULL));

    // Find the configSystem in the list.
    le_dls_Link_t* linkPtr = le_dls_Peek(configSystemList);
    while (linkPtr != NULL)
    {
        RpcConfigSystem_t* systemLinkPtr = CONTAINER_OF(linkPtr, RpcConfigSystem_t, link);

        if ((systemLinkPtr->system.index == configSystemPtr->system.index) ||
            (systemLinkPtr->system.id == configSystemPtr->system.id) ||
            (strcmp(systemLinkPtr->system.name, configSystemPtr->system.name) == 0))
        {
            return true;
        }

        linkPtr = le_dls_PeekNext(configSystemList, linkPtr);
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check a given config service is already in the list.
 */
//--------------------------------------------------------------------------------------------------
static bool IsConfigServiceInList
(
    le_dls_List_t* configServiceList,
    RpcConfigService_t* configServicePtr
)
{
    LE_ASSERT((configServiceList != NULL) && (configServicePtr != NULL));

    // Find the configSystem in the list.
    le_dls_Link_t* linkPtr = le_dls_Peek(configServiceList);
    while(linkPtr != NULL)
    {
        RpcConfigService_t* serviceLinkPtr = CONTAINER_OF(linkPtr, RpcConfigService_t, link);

        if ((serviceLinkPtr->service.id == configServicePtr->service.id) ||
            ((strcmp(serviceLinkPtr->service.name, configServicePtr->service.name) == 0) &&
             (strcmp(serviceLinkPtr->service.user, configServicePtr->service.user) == 0)) ||
            ((serviceLinkPtr->isReliable == configServicePtr->isReliable) &&
             (serviceLinkPtr->portNum == configServicePtr->portNum)))
        {
            return true;
        }

        linkPtr = le_dls_PeekNext(configServiceList, linkPtr);
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check a given offer systemId is already in the list.
 */
//--------------------------------------------------------------------------------------------------
static bool IsOfferSystemIdInList
(
    le_dls_List_t* offerSystemIdList,
    uint16_t systemId
)
{
    LE_ASSERT(offerSystemIdList != NULL);

    // Find the offer system Id in the list.
    le_dls_Link_t* linkPtr = le_dls_Peek(offerSystemIdList);
    while (linkPtr != NULL)
    {
        OfferSystemId_t* offerSystemIdPtr = CONTAINER_OF(linkPtr, OfferSystemId_t, link);

        if (systemId == offerSystemIdPtr->id)
        {
            return true;
        }

        linkPtr = le_dls_PeekNext(offerSystemIdList, linkPtr);
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get my vsomeip client ID based on RPC configuration.
 */
//--------------------------------------------------------------------------------------------------
static uint16_t GetVsomeipClientId
(
    void
)
{
    uint16_t id;

    if (RoutingDevPtr == NULL)
    {
        return taf_someipClnt_GetClientId();
    }

    if (LE_OK != taf_someipClnt_GetClientIdEx(RoutingDevPtr, &id))
    {
        LE_FATAL("Failed to get the vsomeip client id for routingDev:'%s'.", RoutingDevPtr);
    }

    return id;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse and load system link info from JSON file.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseSystemLinkInfo
(
    const json_t* root
)
{
    json_t *js_systems = NULL;
    json_t *js_system = NULL;
    size_t index = 0;

    json_t* js_id = NULL;
    json_t* js_name = NULL;
    SystemId_t systemId = 0;

    bool mySystemFound = false;
    bool remoteSystemFound = false;

    RpcConfigSystem_t* systemLinkPtr = NULL;


    LE_ASSERT(root != NULL);

    // Get my system Id.
    SystemId_t mySystemId = GetVsomeipClientId();

    // Load system array.
    js_systems = json_object_get(root, "systems");
    if (!json_is_array(js_systems))
    {
        LE_ERROR("systems is not an array.");
        return LE_FAULT;
    }
    // Check system array size.
    if ((json_array_size(js_systems) > RPC_MAX_SYSTEMS) ||
        (json_array_size(js_systems) == 0))
    {
        LE_ERROR("Invalid system array size.");
        return LE_FAULT;
    }

    // Get each system entry in the array.
    json_array_foreach(js_systems, index, js_system)
    {
        // Get "id", "name".
        js_id = json_object_get(js_system, "id");
        js_name = json_object_get(js_system, "name");
        if (!json_is_string(js_id) || !json_is_string(js_name) ||
            (LE_OK != ValidateSystemLinkInfo(json_string_value(js_id),
                                             json_string_value(js_name),
                                             &systemId)))
        {
            LE_ERROR("system index %d has invalid fields.", (int)index);	
            return LE_FAULT;
        }

        // Create a systemLink object.
        systemLinkPtr = le_mem_TryAlloc(RpcConfigSystemPoolRef);
        if (systemLinkPtr == NULL)
        {
            LE_ERROR("systemLinkPtr is NULL.");
            return LE_NO_MEMORY;
        }

        // Initialize the systemLink object.
        memset(systemLinkPtr, 0, sizeof(RpcConfigSystem_t));
        systemLinkPtr->link = LE_DLS_LINK_INIT;
        systemLinkPtr->system.id = systemId;
        systemLinkPtr->system.index = index + 1;
        le_utf8_Copy(systemLinkPtr->system.name, json_string_value(js_name),
                     LIMIT_MAX_SYSTEM_NAME_BYTES, NULL);

        // Check if duplicate systemLink objects are found in JSON file.
        if (IsConfigSystemInList(&ConfigSystemList, systemLinkPtr))
        {
            LE_ERROR("Duplicate systemLinks are found in the JSON file.");
            return LE_DUPLICATE;
        }

        // Save the systemLink object into the list and Hash map.
        le_dls_Queue(&ConfigSystemList, &systemLinkPtr->link);

        // Save my systemLink.
        if (systemId == mySystemId)
        {
            if (LE_OK != SetMySystemLinkInfo(systemLinkPtr->system.id,
                                             systemLinkPtr->system.name,
                                             systemLinkPtr->system.index))
            {
                LE_ERROR("Failed to set my systemLink Info.");
                return LE_FAULT;
            }

            mySystemFound = true;
        }
        // Otherwise save as remote system configuration.
        else
        {
            if (LE_OK != SetRemoteSystemLinkInfo(systemLinkPtr->system.id,
                                                 systemLinkPtr->system.name,
                                                 systemLinkPtr->system.index))
            {
                LE_ERROR("Failed to add remote systemLink Info.");
                return LE_FAULT;
            }

            remoteSystemFound = true;
        }
    }

    // We must have at least two systemLink object, one for local system , one for remote system.
    if (!mySystemFound)
    {
        LE_ERROR("My SystemLink is not found in JSON file.");
        return LE_NOT_FOUND;
    }

    if (!remoteSystemFound)
    {
        LE_ERROR("Remote SystemLink is not found in JSON file.");
        return LE_NOT_FOUND;
    }

    LE_INFO("ConfigSystemList size=%" PRIuS ".", le_dls_NumLinks(&ConfigSystemList));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse and load rpc service info from JSON file.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseServiceLinkInfo
(
    const json_t* root
)
{
    json_t *js_rpc_services = NULL;
    json_t *js_rpc_service = NULL;
    size_t index = 0, i = 0;

    json_t *js_name = NULL, *js_user = NULL, *js_version = NULL;
    json_t *js_offer_systems = NULL, *js_offer_system = NULL;

    json_t *js_connection_type = NULL, *js_response_timeout = NULL;

    RpcConfigService_t* serviceLinkPtr = NULL;
    const char* versionStr = TAF_RPC_DEFAULT_SERVICE_VER;
    bool isReliable = false;
    uint16_t timeoutSec = TAF_RPC_DEFAULT_RESP_TIMEOUT;
    SystemId_t systemId;
    OfferSystemId_t* offerSystemIdPtr;
 

    LE_ASSERT(root != NULL);

    // Load rpc_services array.
    js_rpc_services = json_object_get(root, "rpc_services");
    if (!json_is_array(js_rpc_services))
    {
        LE_ERROR("rpc_services is not an array.");
        return LE_FAULT;
    }
    if ((json_array_size(js_rpc_services) > RPC_MAX_SERVICES) ||
        (json_array_size(js_rpc_services) == 0))
    {
        LE_ERROR("Invalid rpc_service array size.");
        return LE_FAULT;
    }

    // Get each service Link entry in the array.
    json_array_foreach(js_rpc_services, index, js_rpc_service)
    {
        // Get name, user and version.
        js_name = json_object_get(js_rpc_service, "name");
        js_user = json_object_get(js_rpc_service, "user");
        js_version = json_object_get(js_rpc_service, "version");
        js_connection_type = json_object_get(js_rpc_service, "connection_type");
        js_response_timeout = json_object_get(js_rpc_service, "response_timeout");

        if (!json_is_string(js_name) || !json_is_string(js_user) ||
            ((js_version != NULL) && !json_is_string(js_version)) ||
            ((js_connection_type != NULL) && !json_is_string(js_connection_type)) ||
            ((js_response_timeout != NULL) && !json_is_string(js_response_timeout)))
        {
            LE_ERROR("rpc_service index %d has invalid fields.", (int)index);
            return LE_FAULT;
        }

        // version shall be mandatory to have.
        if (js_version != NULL)
        {
            versionStr = json_string_value(js_version);
        }
        else
        {
            versionStr = TAF_RPC_DEFAULT_SERVICE_VER;
        }

        // Validate serviceLink info fields.
        if (LE_OK != ValidateServiceLinkInfo(json_string_value(js_name),
                                              json_string_value(js_user),
                                              versionStr))
        {
            LE_ERROR("ServiceLinkInfo validation failed.");
            return LE_FAULT;
        }

        // Validate serviceOptional info fields.
        if (LE_OK != ValidateServiceOptInfo(json_string_value(js_connection_type),
                                            json_string_value(js_response_timeout),
                                            &isReliable, &timeoutSec))
        {
            LE_ERROR("ServiceOptInfo validation failed.");
            return LE_FAULT;
        }

        // Create a serviceLink object.
        serviceLinkPtr = le_mem_TryAlloc(RpcConfigServicePoolRef);
        if (serviceLinkPtr == NULL)
        {
            LE_ERROR("serviceLinkPtr is NULL.");
            return LE_NO_MEMORY;
        }

        // Initialize the serviceLink object.
        memset(serviceLinkPtr, 0, sizeof(RpcConfigService_t));

        serviceLinkPtr->service.id = BaseServiceId + index + 1;
        serviceLinkPtr->portNum = BasePortNumber + index + 1;
        serviceLinkPtr->isReliable = isReliable;
        serviceLinkPtr->responseTimeout = timeoutSec;
        le_utf8_Copy(serviceLinkPtr->service.name, json_string_value(js_name),
                     LIMIT_MAX_IPC_INTERFACE_NAME_BYTES, NULL);
        le_utf8_Copy(serviceLinkPtr->service.user, json_string_value(js_user),
                     LIMIT_MAX_USER_NAME_BYTES, NULL);
        le_utf8_Copy(serviceLinkPtr->service.protocolId, versionStr,
                     LIMIT_MAX_PROTOCOL_ID_BYTES, NULL);

        serviceLinkPtr->offerSystemList = LE_DLS_LIST_INIT;

        // Check if duplicate serviceLink objects are found in JSON file.
        if (IsConfigServiceInList(&ConfigServiceList, serviceLinkPtr))
        {
            LE_ERROR("Duplicate serviceLinks are found in the JSON file.");
            return LE_DUPLICATE;
        }

        // Load offer_systems array.
        js_offer_systems = json_object_get(js_rpc_service, "offer_systems");
        if (!json_is_array(js_offer_systems))
        {
            LE_ERROR("offer_systems in the service is not an array.");
            return LE_FAULT;
        }
        if ((json_array_size(js_offer_systems) > RPC_MAX_SYSTEMS) ||
            (json_array_size(js_offer_systems) == 0))
        {
            LE_ERROR("offer_systems array size overflows.");
            return LE_FAULT;
        }

        // Get each offerSystem Link entry in the array.
        json_array_foreach(js_offer_systems, i, js_offer_system)
        {
            if (!json_is_string(js_offer_system))
            {
                LE_ERROR("offer_system %d is not a string", (int)i);
                return LE_FAULT;
            }

            // Validate this offerSystem entry. It must be a valid systemName string or systemId
            // string and the corresponding systemId must be avaliable in the configSystem list.

            // Return the actual systemId if validation is successful.
            if (LE_OK != ValidateOfferSystemId(json_string_value(js_offer_system), &systemId))
            {
                LE_ERROR("offerSystemId validation failed.");
                return LE_FAULT;
            }

            // Check if the systemId is already in the offerSystem list of this service.
            if (IsOfferSystemIdInList(&serviceLinkPtr->offerSystemList, systemId))
            {
                LE_ERROR("Duplicate offerSystemId in the service is found in JSON file.");
                return LE_FAULT;
            }

            // Create a new offerSystemId object and link to offerSystem list of this service.
            offerSystemIdPtr = le_mem_TryAlloc(RpcOfferSystemIdPoolRef);
            if (offerSystemIdPtr == NULL)
            {
                LE_ERROR("offerSystemIdPtr is NULL.");
                return LE_FAULT;
            }

            memset(offerSystemIdPtr, 0, sizeof(OfferSystemId_t));

            offerSystemIdPtr->id = systemId;
            offerSystemIdPtr->link = LE_DLS_LINK_INIT;
            le_dls_Queue(&serviceLinkPtr->offerSystemList, &offerSystemIdPtr->link);
        }

        // Save the serviceLink object into the list and Hash map.
        serviceLinkPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&ConfigServiceList, &serviceLinkPtr->link);
    }

    LE_INFO("ConfigServiceList size=%" PRIuS ".", le_dls_NumLinks(&ConfigServiceList));

    // populate offerService configuration array.
    PopulateOfferServiceConfigArray();

    // Set request service config array.
    PopulateRequestServiceConfigArray();

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse and load optional configuration items from JSON file.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseOptInfo
(
    const json_t* root
)
{
    // Load "base_service_id", "base_port_number" and "response_timeout".
    json_t *js_base_service_id = NULL;
    json_t *js_base_port_number = NULL;
    json_t *js_response_timeout = NULL;

    ServiceId_t baseServiceId;
    uint16_t basePortNumber;
    uint16_t secs;


    LE_ASSERT(root != NULL);

    js_base_service_id = json_object_get(root, "base_service_id");
    js_base_port_number = json_object_get(root, "base_port_number");
    js_response_timeout = json_object_get(root, "response_timeout");

    // Load "base_service_id" which is optional.
    if ((js_base_service_id != NULL) && json_is_string(js_base_service_id))
    {
        if (LE_OK != ValidateBaseServiceId(json_string_value(js_base_service_id), &baseServiceId))
        {
            LE_ERROR("Invalid base_service_id.");
            return LE_FAULT;
        }

        // Update base_service_id.
        SetBaseServiceId(baseServiceId);
    }

    // Load "base_port_number" which is optional.
    if ((js_base_port_number != NULL) && json_is_string(js_base_port_number))
    {
        if (LE_OK != ValidateBasePortNumber(json_string_value(js_base_port_number),
                                            &basePortNumber))
        {
            LE_ERROR("Invalid base_port_number.");
            return LE_FAULT;
        }

        // Update base_port_number.
        SetBasePortNumber(basePortNumber);
    }

    // Load "response_timeout" which is optional.
    if ((js_response_timeout != NULL) && json_is_string(js_response_timeout))
    {
        if (LE_OK != ValidateResponseTimeout(json_string_value(js_response_timeout), &secs))
        {
            LE_ERROR("Invalid response_timeout.");
            return LE_FAULT;
        }

        // Update response_timeout.
        SetResponseTimeout(secs);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * JSON parser function.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseJsonFile
(
    const char* filePathPtr
)
{
    json_t *root;
    json_t *js_routing_name;
    json_error_t error;

    // Load entire JSON file.
    root = json_load_file(filePathPtr, 0, &error);
    if (root == NULL)
    {
        LE_ERROR("JSON error: line: %d, column: %d, position: %d, source: '%s', error: %s",
                 error.line, error.column, error.position, error.source, error.text);
        return LE_NOT_FOUND;
    }
    // Check if "root" is an object.
    if (!json_is_object(root))
    {
        LE_ERROR("root is not an object.");
        json_decref(root);
        return LE_FAULT;
    }

    // Parse "routing_name". By default the routing dev is NULL, which
    // means RPC is running on vsomeip default routing manager.
    js_routing_name = json_object_get(root, "routing_name");
    if ((js_routing_name != NULL) && json_is_string(js_routing_name))
    {
        SetRoutingName(json_string_value(js_routing_name));
    }

    // Parse and load system link info.
    if (LE_OK != ParseSystemLinkInfo(root))
    {
        json_decref(root);
        return LE_FAULT;
    }

    // Parse and load optional fields.
    if (LE_OK != ParseOptInfo(root))
    {
        json_decref(root);
        return LE_FAULT;
    }

    // Parse and load service link info.
    if (LE_OK != ParseServiceLinkInfo(root))
    {
        json_decref(root);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The async queue function for loading RPC configuration.
 */
//--------------------------------------------------------------------------------------------------
static void LoadRpcConfigFunc
(
    void* param1Ptr,    ///< [IN] param 1, user callback.
    void* param2Ptr     ///< [IN] param 2, config file path.
)
{
    le_result_t result = LE_OK;
    RpcConfigCallbackFunc_t userCallback = (RpcConfigCallbackFunc_t)param1Ptr;
    const char* filePathPtr = (const char*)param2Ptr;

    // Initialize configuration data struct.
    DataInit();

    // Parse the JSON file
    result = ParseJsonFile(filePathPtr);

    userCallback(result, &MyConfiguration);

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Dump all RPC configurations.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyConfig_ShowConfiguration
(
    void
)
{
    int i, j;

    LE_INFO("=========================================================");
    LE_INFO("====================RPC configuration====================");
    LE_INFO("=========================================================");
    LE_INFO(" ");

    // Dump basic configurations.
    LE_INFO("--------------------Basic configurations-----------------");

    if (RoutingDevPtr != NULL)
    {
        LE_INFO("Using on-demand vsomeip routing manager '%s'", RoutingDevPtr);
    }
    else
    {
        LE_INFO("Using by-default vsomeip routing manager.");
    }
    LE_INFO("base_service_id: 0x%x", BaseServiceId);
    LE_INFO("base_port_number: %d", BasePortNumber);
    LE_INFO("response_timeout: %d", ResponseTimeoutSecs);
    LE_INFO(" ");

    // Dump my system info.
    LE_INFO("--------------------My system information----------------");
    LE_INFO("My system index: %d", MyConfiguration.mySystem.index);
    LE_INFO("My system ID: 0x%x", MyConfiguration.mySystem.id);
    LE_INFO("My system name: '%s'", MyConfiguration.mySystem.name);
    LE_INFO(" ");

    LE_INFO("--------------------Remote system list-------------------");
    for (i = 0; i < MyConfiguration.remoteSystemCnt; i++)
    {
        LE_INFO("Remote system entry:");
        LE_INFO("    System index: %d", MyConfiguration.remoteSystems[i].index);
        LE_INFO("    System ID: 0x%x", MyConfiguration.remoteSystems[i].id);
        LE_INFO("    System name: '%s'", MyConfiguration.remoteSystems[i].name);
        LE_INFO(" ");
    }
    LE_INFO("---------------------------------------------------------");
    LE_INFO(" ");

    // Dump offer service list.
    LE_INFO("--------------------Offer service list-------------------");
    for (i = 0; i < MyConfiguration.offerServiceCnt; i++)
    {
        LE_INFO("Offer service entry:");
        LE_INFO("    Service ID: 0x%x", MyConfiguration.offerServices[i].service.id);
        LE_INFO("    Service name: '%s'", MyConfiguration.offerServices[i].service.name);
        LE_INFO("    Service user: '%s'", MyConfiguration.offerServices[i].service.user);
        LE_INFO("    Service version: '%s'", MyConfiguration.offerServices[i].service.protocolId);
        LE_INFO("    Network config: '%s' port %d",
            MyConfiguration.offerServices[i].port.isReliable ? "TCP" : "UDP",
            MyConfiguration.offerServices[i].port.number);
        LE_INFO("    Client system list:");
        for (j = 0; j < MyConfiguration.offerServices[i].systemCnt; j++)
        {
            LE_INFO("        System ID: 0x%x", MyConfiguration.offerServices[i].clientSystems[j]);
        }
        LE_INFO(" ");
    }
    LE_INFO("---------------------------------------------------------");
    LE_INFO(" ");

    // Dump request service list.
    LE_INFO("--------------------request service list-----------------");
    for (i = 0; i < MyConfiguration.requestServiceCnt; i++)
    {
        LE_INFO("request service entry:");
        LE_INFO("    Service ID: 0x%x", MyConfiguration.requestServices[i].service.id);
        LE_INFO("    Service name: '%s'", MyConfiguration.requestServices[i].service.name);
        LE_INFO("    Service user: '%s'", MyConfiguration.requestServices[i].service.user);
        LE_INFO("    Service version: '%s'", MyConfiguration.requestServices[i].service.protocolId);
        LE_INFO("    Service responseTimeout: '%d'",
            MyConfiguration.requestServices[i].responseTimeout);
        LE_INFO("    Network config: '%s'",
            MyConfiguration.requestServices[i].isReliable ? "TCP" : "UDP");
        LE_INFO("    Server system list:");
        for (j = 0; j < MyConfiguration.requestServices[i].systemCnt; j++)
        {
            LE_INFO("        System ID: 0x%x", MyConfiguration.requestServices[i].serverSystems[j]);
        }
        LE_INFO(" ");
    }
    LE_INFO("--------------------------------------------------------");
    LE_INFO(" ");
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the RPC configuration from the JSON file. The configuration callback function will be called
 * once JSON file parsing is done.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyConfig_LoadConfiguration
(
    const char* filePathPtr,                  ///< [IN] The full path of the JSON file.
    RpcConfigCallbackFunc_t funcPtr     ///< [IN] The callback function pointer.
)
{
    if (filePathPtr == NULL)
    {
        LE_FATAL("filePathPtr is NULL.");
    }

    if (funcPtr == NULL)
    {
        LE_FATAL("function callback is NULL.");
    }

    // Start loading configuration in aysnc mode.
    le_event_QueueFunction(LoadRpcConfigFunc, (void*)funcPtr, (void*)filePathPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get my vsomeip client ID.
 */
//--------------------------------------------------------------------------------------------------
uint16_t rpcProxyConfig_GetVsomeipClientId
(
    void
)
{
    return GetVsomeipClientId();
}

//--------------------------------------------------------------------------------------------------
/**
 * Get routing name.
 */
//--------------------------------------------------------------------------------------------------
const char* rpcProxyConfig_GetRoutingName
(
    void
)
{
    return RoutingDevPtr;
}

