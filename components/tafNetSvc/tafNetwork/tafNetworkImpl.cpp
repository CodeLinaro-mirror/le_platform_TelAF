/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafNetImpl.cpp
 * @brief      This file provides the implementation of taf net component.
 */
#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include "tafSvcIF.hpp"
#include "tafNetworkImpl.hpp"
#include "tafNetUtility.hpp"
#include "tafNetPa.hpp"
#include <arpa/inet.h>
#include <time.h>

using namespace tafsvc;

void taf_Net::Init(void)
{
    LE_INFO("data net component init...");

    le_result_t isReady;

    isReady =  PA_TO_LE_RESULT(taf_pa_net_Init());

    if(isReady == LE_OK)
    {
        LE_INFO("netManager component is ready...");
    }
    else
    {
        LE_CRIT("unable to init netManager component!");
    }

    RouteChangeEvId = le_event_CreateIdWithRefCounting("RouteChange");

    GatewayChangeEvId = le_event_CreateIdWithRefCounting("GatewayChange");

    DNSChangeEvId = le_event_CreateIdWithRefCounting("DNSChange");

    net_InitRoutingDnsMutex();

    DefaultGwConfDbPool = le_mem_CreatePool("DefaultGwConfDbPool",sizeof(taf_net_DfltGwConfDb_t));

    le_mem_ExpandPool(DefaultGwConfDbPool, TAF_NET_MAX_CLIENT_APPS);

    RouteChangePool = le_mem_CreatePool("RouteChangePool", sizeof(taf_net_RouteChangeInd_t));

    le_mem_ExpandPool(RouteChangePool, TAF_NET_MAX_CLIENT_APPS);

    GatewayChangePool = le_mem_CreatePool("GatewayChangePool", sizeof(taf_net_GatewayChangeInd_t));

    le_mem_ExpandPool(GatewayChangePool, TAF_NET_MAX_CLIENT_APPS);

    DNSChangePool = le_mem_CreatePool("DNSChangePool", sizeof(taf_net_DNSChangeInd_t));

    le_mem_ExpandPool(DNSChangePool, TAF_NET_MAX_CLIENT_APPS);

    // Add a handler for client session closes
    le_msg_AddServiceCloseHandler( taf_net_GetServiceRef(), ClientCloseSessionHandler, NULL );

}

taf_Net &taf_Net::GetInstance()
{
    static taf_Net instance;

    return instance;
}

void taf_Net::FirstLayerRouteChangeHandler(void *reportPtr, void *secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == nullptr, "Null ptr(secondLayerHandlerFunc)");

    taf_net_RouteChangeHandlerFunc_t handlerFunc = (taf_net_RouteChangeHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc((taf_net_RouteChangeInd_t*)reportPtr, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

void taf_Net::FirstLayerGatewayChangeHandler(void *reportPtr, void *secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == nullptr, "Null ptr(secondLayerHandlerFunc)");

    taf_net_GatewayChangeHandlerFunc_t handlerFunc = (taf_net_GatewayChangeHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc((taf_net_GatewayChangeInd_t*)reportPtr, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

void taf_Net::FirstLayerDNSChangeHandler(void *reportPtr, void *secondLayerHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(secondLayerHandlerFunc == nullptr, "Null ptr(secondLayerHandlerFunc)");

    taf_net_DNSChangeHandlerFunc_t handlerFunc = (taf_net_DNSChangeHandlerFunc_t)secondLayerHandlerFunc;
    handlerFunc((taf_net_DNSChangeInd_t*)reportPtr, le_event_GetContextPtr());

    le_mem_Release(reportPtr);
}

le_result_t taf_Net::ChangeRoute(const char *intfNamePtr, const char *destAddrPtr, const char *preLenPtr,uint16_t metric, taf_net_NetAction_t isAdd)
{
    le_result_t ret;
    uint16_t i, length;
    taf_net_RouteChangeInd_t *reportPtr = NULL;

    TAF_ERROR_IF_RET_VAL(intfNamePtr == NULL, LE_BAD_PARAMETER, "interface name is NULL!");
    TAF_ERROR_IF_RET_VAL(destAddrPtr == NULL, LE_BAD_PARAMETER, "destination address is NULL!");
    TAF_ERROR_IF_RET_VAL(preLenPtr == NULL, LE_BAD_PARAMETER, "subnet Prefix Length is NULL!");

    length = strlen(destAddrPtr);

    //remove space from subnet mask string
    if (preLenPtr)
    {
        length = strlen(preLenPtr);
        for (i=0; (i<length) && isspace(*preLenPtr); i++)
        {
            preLenPtr++;
        }
    }

    ret = net_ChangeLinuxRoute(intfNamePtr, destAddrPtr, preLenPtr, metric, isAdd);

    if(ret == LE_OK)
    {
        reportPtr = (taf_net_RouteChangeInd_t*)le_mem_ForceAlloc(RouteChangePool);
        le_utf8_Copy(reportPtr->interfaceName, intfNamePtr, TAF_NET_INTERFACE_NAME_MAX_LEN, NULL);
        le_utf8_Copy(reportPtr->destAddr, destAddrPtr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        le_utf8_Copy(reportPtr->prefixLength, preLenPtr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
        reportPtr->metric = metric;
        reportPtr->action = isAdd;
        le_event_ReportWithRefCounting(RouteChangeEvId, (void*)reportPtr);
    }

    return ret;

}

le_result_t taf_Net::BackupDefaultGW(le_msg_SessionRef_t sessionRef)
{
    taf_net_DfltGwBackup_t defaultGwBackupConf;
    le_result_t v4Result = LE_FAULT, v6Result = LE_FAULT;

    memset(&defaultGwBackupConf, 0x0, sizeof(taf_net_DfltGwBackup_t));

    //get system gateway addresses
    net_GetLinuxDefaultGateway(&defaultGwBackupConf, &v4Result, &v6Result);

    if ( (v4Result != LE_OK || strlen(defaultGwBackupConf.ipV4Gateway) == 0) &&
         (v6Result != LE_OK || strlen(defaultGwBackupConf.ipV6Gateway) == 0) )
    {
        return LE_NOT_FOUND;
    }
    //backup the addresses into db link
    BackupDefaultGwToDb(sessionRef, &defaultGwBackupConf);

    return LE_OK;
}

le_result_t taf_Net::RestoreDefaultGW(le_msg_SessionRef_t sessionRef)
{
    le_result_t  v4Result = LE_FAULT, v6Result = LE_FAULT;
    taf_net_DfltGwBackup_t *defaultGwBackupConfPtr = NULL;
    taf_net_GatewayChangeInd_t *reportPtr = NULL;
    taf_net_DfltGwConfDb_t  *defaultGwConfDbPtr = NULL;

    defaultGwConfDbPtr = GetDefaultGwConfDbBySessionRef(sessionRef);
    TAF_ERROR_IF_RET_VAL(defaultGwConfDbPtr == NULL, LE_NOT_FOUND, "Can't find default gateway backup info!");

    defaultGwBackupConfPtr= &defaultGwConfDbPtr->backupConfig;

    //get the backed up gateway addresses and set them into system
    if (defaultGwBackupConfPtr->isSetIpV6Gw)
    {
        v6Result = net_SetLinuxDefaultGateway(defaultGwBackupConfPtr->ipV6InterfaceName, defaultGwBackupConfPtr->ipV6Gateway, false);

        if(v6Result == LE_OK)
        {
            reportPtr = (taf_net_GatewayChangeInd_t*)le_mem_ForceAlloc(GatewayChangePool);
            le_utf8_Copy(reportPtr->interfaceName, defaultGwBackupConfPtr->ipV6InterfaceName, TAF_NET_INTERFACE_NAME_MAX_LEN, NULL);
            le_utf8_Copy(reportPtr->gatewayAddr, defaultGwBackupConfPtr->ipV6Gateway, TAF_NET_IP_ADDR_MAX_LEN, NULL);
            reportPtr->ipType = TAF_NET_IPV6;
            le_event_ReportWithRefCounting(GatewayChangeEvId, (void*)reportPtr);
        }
    }

    if (defaultGwBackupConfPtr->isSetIpV4Gw)
    {
        v4Result = net_SetLinuxDefaultGateway(defaultGwBackupConfPtr->ipV4InterfaceName, defaultGwBackupConfPtr->ipV4Gateway, true);

        if(v4Result == LE_OK)
        {
            reportPtr = (taf_net_GatewayChangeInd_t*)le_mem_ForceAlloc(GatewayChangePool);
            le_utf8_Copy(reportPtr->interfaceName, defaultGwBackupConfPtr->ipV4InterfaceName, TAF_NET_INTERFACE_NAME_MAX_LEN, NULL);
            le_utf8_Copy(reportPtr->gatewayAddr, defaultGwBackupConfPtr->ipV4Gateway, TAF_NET_IP_ADDR_MAX_LEN, NULL);
            reportPtr->ipType = TAF_NET_IPV4;
            le_event_ReportWithRefCounting(GatewayChangeEvId, (void*)reportPtr);
        }
    }

    le_dls_Remove(&DefaultGwConfDbList, &defaultGwConfDbPtr->dbLink);

    le_mem_Release(defaultGwConfDbPtr);

    if ((v6Result == LE_OK) || (v4Result == LE_OK))
    {
        LE_DEBUG("Restore default gateway for sessionRef %p successfully v6Result=%d,v4Result=%d", sessionRef,v6Result,v4Result);
        return LE_OK;
    }

    return LE_FAULT;
}

le_result_t taf_Net::SetDefaultGW(le_msg_SessionRef_t sessionRef,const char  *intfNamePtr,const char  *ipv4AddrPtr,const char  *ipv6AddrPtr)
{
    le_result_t v4Result = LE_FAULT, v6Result = LE_FAULT;
    taf_net_GatewayChangeInd_t *reportPtr = NULL;
    taf_net_DfltGwConfDb_t *defaultGwConfDbPtr = NULL;

    TAF_ERROR_IF_RET_VAL(intfNamePtr == NULL, LE_BAD_PARAMETER, "interface Name is NULL");

    defaultGwConfDbPtr = GetDefaultGwConfDbBySessionRef(sessionRef);

    //set system gateway addresses
    if (ipv6AddrPtr != NULL  &&  strlen(ipv6AddrPtr) > 0)
    {
        v6Result = net_SetLinuxDefaultGateway(intfNamePtr, ipv6AddrPtr, false);

        if (v6Result == LE_OK)
        {
            reportPtr = (taf_net_GatewayChangeInd_t*)le_mem_ForceAlloc(GatewayChangePool);
            le_utf8_Copy(reportPtr->interfaceName, intfNamePtr, TAF_NET_INTERFACE_NAME_MAX_LEN, NULL);
            le_utf8_Copy(reportPtr->gatewayAddr, ipv6AddrPtr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
            reportPtr->ipType = TAF_NET_IPV6;
            le_event_ReportWithRefCounting(GatewayChangeEvId, (void*)reportPtr);

            if(defaultGwConfDbPtr != NULL)
            {
                defaultGwConfDbPtr->backupConfig.isSetIpV6Gw = true;
            }
        }
        else
        {
            LE_ERROR("Failed to set iPv6 default gateway for interface %s", intfNamePtr);
        }
    }

    if (ipv4AddrPtr != NULL  && strlen(ipv4AddrPtr) > 0)
    {
        v4Result = net_SetLinuxDefaultGateway(intfNamePtr, ipv4AddrPtr, true);

        if (v4Result == LE_OK)
        {
            reportPtr = (taf_net_GatewayChangeInd_t*)le_mem_ForceAlloc(GatewayChangePool);
            le_utf8_Copy(reportPtr->interfaceName, intfNamePtr, TAF_NET_INTERFACE_NAME_MAX_LEN, NULL);
            le_utf8_Copy(reportPtr->gatewayAddr, ipv4AddrPtr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
            reportPtr->ipType = TAF_NET_IPV4;
            le_event_ReportWithRefCounting(GatewayChangeEvId, (void*)reportPtr);

            if(defaultGwConfDbPtr != NULL)
            {
                defaultGwConfDbPtr->backupConfig.isSetIpV4Gw = true;
            }
        }
        else
        {
            LE_ERROR("Failed to set iPv4 default gateway for interface %s", intfNamePtr);
        }
    }

    if ((v6Result == LE_OK) || (v4Result == LE_OK))
    {
        LE_DEBUG("Set default gateway address for interface %s successfully v6Result=%d,v4Result=%d", intfNamePtr,v6Result,v4Result);
        return LE_OK;
    }

    return LE_FAULT;
}

le_result_t taf_Net::SetDNS(le_msg_SessionRef_t sessionRef,const char *ipv4Addr1Ptr, const char *ipv4Addr2Ptr, const char *ipv6Addr1Ptr, const char *ipv6Addr2Ptr)
{
    le_result_t v4Result = LE_NOT_FOUND, v6Result = LE_NOT_FOUND;
    struct sockaddr_in6 addr6;
    struct sockaddr_in addr;
    taf_net_DNSChangeInd_t *reportPtr = NULL;

    TAF_ERROR_IF_RET_VAL( (ipv4Addr1Ptr == NULL) || (ipv4Addr2Ptr == NULL) || (ipv6Addr1Ptr == NULL) || (ipv6Addr2Ptr == NULL), LE_BAD_PARAMETER, "invalid ip address");

    if ((inet_pton(AF_INET, ipv4Addr1Ptr, &(addr.sin_addr)) != 1) && (inet_pton(AF_INET, ipv4Addr2Ptr, &(addr.sin_addr)) != 1) &&
       (inet_pton(AF_INET6, ipv6Addr1Ptr, &(addr6.sin6_addr)) != 1) && (inet_pton(AF_INET6, ipv6Addr2Ptr, &(addr6.sin6_addr)) != 1))
       {
           return LE_BAD_PARAMETER;
       }

    //set the system DNS addresses
    if( (strlen(ipv6Addr1Ptr) > 0) || (strlen(ipv6Addr2Ptr) > 0))
    {
        v6Result = net_SetLinuxDnsNameServers(ipv6Addr1Ptr, ipv6Addr2Ptr, false);

        if(v6Result == LE_OK)
        {
            reportPtr = (taf_net_DNSChangeInd_t*)le_mem_ForceAlloc(DNSChangePool);
            if(strlen(ipv6Addr1Ptr)>0)
                le_utf8_Copy(reportPtr->ipAddr1, ipv6Addr1Ptr, NET_IPV6_ADDR_MAX_BYTES, NULL);
            else
                memset(reportPtr->ipAddr1,0,NET_IPV6_ADDR_MAX_BYTES);

            if(strlen(ipv6Addr2Ptr)>0)
                le_utf8_Copy(reportPtr->ipAddr2, ipv6Addr2Ptr, NET_IPV6_ADDR_MAX_BYTES, NULL);
            else
                memset(reportPtr->ipAddr2,0,NET_IPV6_ADDR_MAX_BYTES);

            reportPtr->ipType = TAF_NET_IPV6;
            le_event_ReportWithRefCounting(DNSChangeEvId, (void*)reportPtr);
        }

        if ((v6Result != LE_OK) && (v6Result != LE_DUPLICATE))
        {
            LE_ERROR("Failed to set IPv6 DNS address,v6Result=%d",v6Result);
        }

    }

    if( (strlen(ipv4Addr1Ptr) > 0) || (strlen(ipv4Addr2Ptr) > 0))
    {
        v4Result = net_SetLinuxDnsNameServers(ipv4Addr1Ptr, ipv4Addr2Ptr, true);

        if(v4Result == LE_OK)
        {
            reportPtr = (taf_net_DNSChangeInd_t*)le_mem_ForceAlloc(DNSChangePool);

            if(strlen(ipv4Addr1Ptr)>0)
                le_utf8_Copy(reportPtr->ipAddr1, ipv4Addr1Ptr, NET_IPV4_ADDR_MAX_BYTES, NULL);
            else
                memset(reportPtr->ipAddr1,0,NET_IPV4_ADDR_MAX_BYTES);

            if(strlen(ipv4Addr2Ptr)>0)
                le_utf8_Copy(reportPtr->ipAddr2, ipv4Addr2Ptr, NET_IPV4_ADDR_MAX_BYTES, NULL);
            else
                memset(reportPtr->ipAddr2,0,NET_IPV4_ADDR_MAX_BYTES);

            reportPtr->ipType = TAF_NET_IPV4;
            le_event_ReportWithRefCounting(DNSChangeEvId, (void*)reportPtr);
        }

        if ((v4Result != LE_OK) && (v4Result != LE_DUPLICATE))
        {
            LE_ERROR("Failed to set IPv4 DNS address,v4Result=%d",v4Result);
        }
    }

    if(v4Result == LE_OK || v6Result == LE_OK)
    {
        return LE_OK;
    }
    else if(v4Result == LE_DUPLICATE && (v6Result == LE_DUPLICATE || v6Result == LE_NOT_FOUND))
    {
        return LE_DUPLICATE;
    }
    else if(v6Result == LE_DUPLICATE && (v4Result == LE_DUPLICATE || v4Result == LE_NOT_FOUND))
    {
        return LE_DUPLICATE;
    }
    else
    {
        return LE_FAULT;
    }

    return LE_OK;
}

void taf_Net::BackupDefaultGwToDb(le_msg_SessionRef_t sessionRef, taf_net_DfltGwBackup_t *backupDataPtr)
{
    bool defaultGwConfExistInDb = false;
    taf_net_DfltGwBackup_t *defaultGwBackupPtr = NULL;
    taf_net_DfltGwConfDb_t *defaultGwConfDbPtr = NULL;

    TAF_ERROR_IF_RET_NIL(backupDataPtr == NULL, "backupDataPtr is NULL");

    defaultGwConfDbPtr = GetDefaultGwConfDbBySessionRef(sessionRef);
    if (defaultGwConfDbPtr == NULL )
    {
        defaultGwConfDbPtr = (taf_net_DfltGwConfDb_t*)le_mem_ForceAlloc(DefaultGwConfDbPool);

        memset(defaultGwConfDbPtr, 0x0, sizeof(taf_net_DfltGwConfDb_t));

        defaultGwConfDbPtr->backupConfig.sessionRef = sessionRef;
    }
    else
    {
        defaultGwConfExistInDb = true;

        le_dls_Remove(&DefaultGwConfDbList, &defaultGwConfDbPtr->dbLink);
    }

    defaultGwBackupPtr = &defaultGwConfDbPtr->backupConfig;
    if (defaultGwConfExistInDb)
    {
        if( strncmp(defaultGwBackupPtr->ipV6Gateway, backupDataPtr->ipV6Gateway, NET_IPV6_ADDR_MAX_BYTES) == 0 &&
            strncmp(defaultGwBackupPtr->ipV6InterfaceName, backupDataPtr->ipV6InterfaceName, NET_INTERFACE_NAME_MAX_BYTES) == 0)
        {
            defaultGwBackupPtr->isSetIpV6Gw = backupDataPtr->isSetIpV6Gw;
        }
        else
        {
            defaultGwBackupPtr->isSetIpV6Gw=false;
        }

        if( strncmp(defaultGwBackupPtr->ipV4Gateway, backupDataPtr->ipV4Gateway, NET_IPV4_ADDR_MAX_BYTES) == 0 &&
            strncmp(defaultGwBackupPtr->ipV4InterfaceName, backupDataPtr->ipV4InterfaceName, NET_INTERFACE_NAME_MAX_BYTES) == 0)
        {
            defaultGwBackupPtr->isSetIpV4Gw = backupDataPtr->isSetIpV4Gw;
        }
        else
        {
            defaultGwBackupPtr->isSetIpV4Gw=false;
        }

    }
    else
    {
        defaultGwBackupPtr->isSetIpV4Gw = false;
        defaultGwBackupPtr->isSetIpV6Gw = false;
    }

    le_utf8_Copy(defaultGwBackupPtr->ipV6Gateway, backupDataPtr->ipV6Gateway, NET_IPV6_ADDR_MAX_BYTES, NULL);

    le_utf8_Copy(defaultGwBackupPtr->ipV6InterfaceName, backupDataPtr->ipV6InterfaceName, NET_INTERFACE_NAME_MAX_BYTES, NULL);

    le_utf8_Copy(defaultGwBackupPtr->ipV4Gateway, backupDataPtr->ipV4Gateway, NET_IPV4_ADDR_MAX_BYTES, NULL);

    le_utf8_Copy(defaultGwBackupPtr->ipV4InterfaceName, backupDataPtr->ipV4InterfaceName, NET_INTERFACE_NAME_MAX_BYTES, NULL);

    le_dls_Stack(&DefaultGwConfDbList, &defaultGwConfDbPtr->dbLink);
}

taf_net_DfltGwConfDb_t* taf_Net::GetDefaultGwConfDbBySessionRef(le_msg_SessionRef_t sessionRef)
{
    taf_net_DfltGwConfDb_t *defaultGwConfDbPtr = NULL;
    taf_net_DfltGwBackup_t *backedupDbPtr = NULL;
    le_dls_Link_t *defaultGwConfDbLinkPtr = le_dls_Peek(&DefaultGwConfDbList);

    while(defaultGwConfDbLinkPtr!= NULL)
    {
        defaultGwConfDbPtr = CONTAINER_OF(defaultGwConfDbLinkPtr, taf_net_DfltGwConfDb_t, dbLink);

        if(defaultGwConfDbPtr != NULL)
            backedupDbPtr = &defaultGwConfDbPtr->backupConfig;
        else
            continue;

        if (backedupDbPtr->sessionRef == sessionRef)
        {
            LE_DEBUG("Default gateway backup exists in db for sessionRef %p", sessionRef);
            return defaultGwConfDbPtr;
        }

        defaultGwConfDbLinkPtr = le_dls_PeekNext(&DefaultGwConfDbList, defaultGwConfDbLinkPtr);
    }

    return NULL;
}

le_result_t taf_Net::getPhoneIdFromSlotId(uint8_t slotId, uint8_t *phoneIdPtr)
{
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(phoneIdPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(phoneIdPtr)");

    result = PA_TO_LE_RESULT(taf_pa_net_GetPhoneIdFromSlotId(slotId, phoneIdPtr));

    LE_DEBUG("result =%d, slotId = %d, phoneId = %d", result, slotId, *phoneIdPtr);

    return result;
}

le_result_t taf_Net::getSlotIdFromPhoneId(uint8_t phoneId, uint8_t *slotIdPtr)
{
    le_result_t result;

    TAF_ERROR_IF_RET_VAL(slotIdPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(slotIdPtr)");

    result = PA_TO_LE_RESULT(taf_pa_net_GetSlotIdFromPhoneId(phoneId, slotIdPtr));

    LE_DEBUG("result =%d, slotId = %d, phoneId = %d",result, *slotIdPtr, phoneId);

    return result;
}

void taf_Net::ClientCloseSessionHandler(le_msg_SessionRef_t sessionRef, void  *contextPtr)
{
    taf_net_DfltGwConfDb_t  *defaultGwConfDbPtr = NULL;
    taf_net_DfltGwBackup_t *backedupDefaultGwDbPtr = NULL;
    auto &netWork = taf_Net::GetInstance();

    TAF_ERROR_IF_RET_NIL(sessionRef == NULL, "sessionRef is NULL");

    //remove default gateway data
    le_dls_Link_t *defaultGwConfDbLinkPtr = le_dls_Peek(&netWork.DefaultGwConfDbList);

    while(defaultGwConfDbLinkPtr!= NULL)
    {

        defaultGwConfDbPtr = CONTAINER_OF(defaultGwConfDbLinkPtr, taf_net_DfltGwConfDb_t, dbLink);

        if(defaultGwConfDbPtr == NULL)
            continue;

        backedupDefaultGwDbPtr = &defaultGwConfDbPtr->backupConfig;

        if (backedupDefaultGwDbPtr->sessionRef == sessionRef)
        {
            le_dls_Remove(&netWork.DefaultGwConfDbList, &defaultGwConfDbPtr->dbLink);
            le_mem_Release(defaultGwConfDbPtr);
            break;
        }

        defaultGwConfDbLinkPtr = le_dls_PeekNext(&netWork.DefaultGwConfDbList, defaultGwConfDbLinkPtr);
    }

}
