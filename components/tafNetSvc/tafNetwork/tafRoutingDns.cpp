/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <net/route.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "tafRoutingDns.hpp"

static le_mutex_Ref_t systemCallMutex = NULL;

void net_InitRoutingDnsMutex()
{
     LE_INFO("init systemCallMutex...");
     systemCallMutex = le_mutex_CreateNonRecursive("systemCallMutex");
}
static le_result_t CallLinuxPopen(const char *cmdPtr, unsigned int cmdlen)
{
    int result = LE_OK;
    FILE *streamPtr = NULL;
    unsigned int vallen = (unsigned int)strlen( cmdPtr );

    if( vallen != cmdlen ) {
        LE_ERROR( "system call length mismatch: %d != %d", cmdlen, vallen );
        return LE_FAULT;
    }

    LE_DEBUG("system call: %s", cmdPtr);
    le_mutex_Lock(systemCallMutex);
    streamPtr = popen( cmdPtr, "w" );
    if( streamPtr == NULL )
    {
        LE_ERROR("system command failed");
        le_mutex_Unlock(systemCallMutex);
        return LE_FAULT;
    }
    else
    {
        result = pclose( streamPtr );
        if (WIFEXITED(result))
        {
            result = WEXITSTATUS(result);
        }
    }

    if (result != 0)
    {
        LE_ERROR("system command failed, rc=%d errno: %d", result, errno);
        le_mutex_Unlock(systemCallMutex);
        return LE_FAULT;
    }
    else
    {
        le_mutex_Unlock(systemCallMutex);
        return LE_OK;
    }
}

static le_result_t ChangeIpv6Route(const char *destAddrPtr, const char *subMaskPtr, const char *intfPtr, uint32_t metric, taf_net_NetAction_t isAdd)
{
    const char *actionPtr, systemCallCmd[NET_SYSTEM_CALL_CMD_MAX_LENGTH] = {0};
    unsigned int retLen = 0;
    le_result_t ret;
    std::string destSubnetStr="";

    TAF_ERROR_IF_RET_VAL(destAddrPtr == NULL, LE_FAULT, "destAddrPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(intfPtr == NULL, LE_FAULT, "intfPtr is NULL!");

    actionPtr = isAdd ? "add":"delete";

    destSubnetStr=destSubnetStr.append(destAddrPtr);

    //network route
    if (subMaskPtr && (strlen(subMaskPtr) > 0))
    {
        destSubnetStr=destSubnetStr.append("/");
        destSubnetStr=destSubnetStr.append(subMaskPtr);
    }

    retLen=(unsigned int)snprintf((char *)systemCallCmd, sizeof(systemCallCmd),
                         IP_COMMAND " -6 route %s %s dev %s metric %d", actionPtr, destSubnetStr.c_str(), intfPtr,metric) ;

    if (retLen >= sizeof(systemCallCmd))
    {
        LE_ERROR("command length too long, '%s' can't be called", systemCallCmd);
        return LE_FAULT;
    }

    ret=CallLinuxPopen(systemCallCmd,retLen);
    return ret;

}

static le_result_t ChangeIpv4Route(const char *destAddrPtr,const char *subMaskPtr, const char *intfPtr, uint32_t metric, taf_net_NetAction_t isAdd)
{
    const char *actionPtr, systemCallCmd[NET_SYSTEM_CALL_CMD_MAX_LENGTH] = {0};
    unsigned int retLen=0;
    le_result_t ret;
    std::string destSubnetStr="";

    TAF_ERROR_IF_RET_VAL(destAddrPtr == NULL, LE_FAULT, "destAddrPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(intfPtr == NULL, LE_FAULT, "intfPtr is NULL!");

    actionPtr = isAdd ? "add":"delete";

    destSubnetStr=destSubnetStr.append(destAddrPtr);

    //network route
    if (subMaskPtr && (strlen(subMaskPtr) > 0))
    {
        destSubnetStr=destSubnetStr.append("/");
        destSubnetStr=destSubnetStr.append(subMaskPtr);
    }

    retLen=(unsigned int)snprintf((char *)systemCallCmd, sizeof(systemCallCmd),
                         IP_COMMAND " -4 route %s %s dev %s metric %d", actionPtr, destSubnetStr.c_str(), intfPtr,metric);

    if (retLen >= sizeof(systemCallCmd))
    {
        LE_ERROR("command length too long, '%s' can't be called", systemCallCmd);
        return LE_FAULT;
    }

    ret=CallLinuxPopen(systemCallCmd,retLen);

    return ret;
}

le_result_t net_ChangeLinuxRoute(const char *intfPtr, const char *destAddrPtr, const char *subMaskPtr, uint16_t metric, taf_net_NetAction_t isAdd)
{
    struct sockaddr_in6 addr6;
    struct sockaddr_in addr;
    le_result_t ret;

    TAF_ERROR_IF_RET_VAL(intfPtr == NULL, LE_FAULT, "intfPtr name is NULL!");
    TAF_ERROR_IF_RET_VAL(destAddrPtr == NULL, LE_FAULT, "destination address is NULL!");

    if (inet_pton(AF_INET, destAddrPtr, &(addr.sin_addr)))
    {
        ret = ChangeIpv4Route(destAddrPtr, subMaskPtr, intfPtr, metric, isAdd);
        return ret;
    }
    else if (inet_pton(AF_INET6, destAddrPtr, &(addr6.sin6_addr)))
    {
        ret = ChangeIpv6Route(destAddrPtr, subMaskPtr, intfPtr, metric, isAdd);
        return ret;
    }
    else
    {
        LE_ERROR("invalid ip address ");
        return LE_BAD_PARAMETER;
    }
}

static le_result_t GetIpv6DefaultGatewayFromFile(char *defaultGWPtr, size_t defaultGWSize, char *defaultIntfPtr, size_t defaultIntfPtrSize)
{
    le_result_t result = LE_NOT_FOUND;
    char lineStr[NET_ONE_LINE_IN_GW_FILE_MAX_LENGTH];
    char *savedPtr, *destAddrPtr, *iFacePtr;
    char *ipv6DestPrePtr, *ipv6SrcAddrPtr, *ipv6SrcPrePtr, *ipv6NextHopPtr;
    const char *ipv6ZeroPrefix = "00", *interval=" \t";
    const char *ipv6ZeroStr = "00000000000000000000000000000000";
    FILE  *resolvFPtr;
    uint16_t i;
    struct in6_addr sin6_addr;
    std::string parsed_str="", sub_str="", origin_str="";

    TAF_ERROR_IF_RET_VAL(defaultGWPtr == NULL, LE_FAULT, "defaultGWPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(defaultIntfPtr == NULL, LE_FAULT, "defaultIntfPtr is NULL!");

    resolvFPtr = le_flock_TryOpenStream(IPV6_ROUTE_INFO_FILE, LE_FLOCK_READ, &result);

    TAF_ERROR_IF_RET_VAL(resolvFPtr == NULL, LE_FAULT, "Open route information file '%s' failed,can't get default gateway", IPV6_ROUTE_INFO_FILE);

    while (fgets(lineStr, sizeof(lineStr), resolvFPtr))
    {
        //IPV6 route table format: dst| dst pfx | src| src pfx| nexthop|metric|refNum|UseNum|Flag|iFace
        //destination address
        destAddrPtr = strtok_r(lineStr,interval, &savedPtr);
        if(destAddrPtr == NULL)
        {
            result = LE_NOT_FOUND;
            continue;
        }
        //destination prefix
        ipv6DestPrePtr = strtok_r(NULL,interval, &savedPtr);
        if(ipv6DestPrePtr == NULL)
        {
            result = LE_NOT_FOUND;
            continue;
        }
        //source address
        ipv6SrcAddrPtr = strtok_r(NULL,interval, &savedPtr);
        if(ipv6SrcAddrPtr == NULL)
        {
            result = LE_NOT_FOUND;
            continue;
        }
        //source prefix
        ipv6SrcPrePtr = strtok_r(NULL,interval, &savedPtr);
        if(ipv6SrcPrePtr == NULL)
        {
            result = LE_NOT_FOUND;
            continue;
        }
        //nexthop
        ipv6NextHopPtr = strtok_r(NULL,interval,&savedPtr);
        if(ipv6NextHopPtr == NULL)
        {
            result = LE_NOT_FOUND;
            continue;
        }
        //don't get the values ,not useful
        strtok_r(NULL,interval, &savedPtr);//metric
        strtok_r(NULL,interval, &savedPtr);//reference number
        strtok_r(NULL,interval, &savedPtr);//use number
        strtok_r(NULL,interval, &savedPtr);//flag

        //iFace
        iFacePtr = strtok_r(NULL,interval, &savedPtr);
        if(iFacePtr == NULL)
        {
            result = LE_NOT_FOUND;
            continue;
        }

        if ((strlen(iFacePtr) > 0)  && (strlen(ipv6NextHopPtr) == NET_IPV6_HEX_LENGTH) &&(strncmp(destAddrPtr, ipv6ZeroStr, strlen(ipv6ZeroStr)) == 0 ) &&
            (strncmp(ipv6DestPrePtr, ipv6ZeroPrefix, strlen(ipv6ZeroPrefix)) == 0 ) && (strncmp(ipv6NextHopPtr, ipv6ZeroStr ,strlen(ipv6ZeroStr)) != 0) &&
            (strncmp(ipv6SrcAddrPtr , ipv6ZeroStr, strlen(ipv6ZeroStr)) == 0 ) && (strncmp(ipv6SrcPrePtr , ipv6ZeroPrefix, strlen(ipv6ZeroPrefix)) == 0 ) )
        {
            origin_str.clear();
            parsed_str.clear();
            origin_str = origin_str.append(ipv6NextHopPtr);

            //ipv6NextHop must be 32 bytes,add ":" between every 4 bytes
            for(i=0;i<7;i++)
            {
                sub_str=origin_str.substr(i*4,4);
                parsed_str=parsed_str.append(sub_str);
                parsed_str=parsed_str.append(":");
            }
            //the last one
            sub_str=origin_str.substr(i*4,4);
            parsed_str=parsed_str.append(sub_str);

            //remove redundant 0
            inet_pton(AF_INET6, parsed_str.c_str(), &sin6_addr);
            inet_ntop(AF_INET6, &sin6_addr, defaultGWPtr, NET_IPV6_ADDR_MAX_BYTES);

            result = le_utf8_Copy(defaultIntfPtr, iFacePtr, defaultIntfPtrSize, NULL);
            if (result != LE_OK)
            {
                LE_WARN("defaultIntfPtr buffer too small,can't save the iFace");
            }
            break;
        }
    }

    le_flock_CloseStream(resolvFPtr);

    return result;
}

static le_result_t GetIpv4DefaultGatewayFromFile(char *defaultGWPtr, size_t defaultGWSize, char *defaultIntfPtr, size_t defaultIntfPtrSize)
{
    le_result_t result = LE_NOT_FOUND;
    char lineStr[NET_ONE_LINE_IN_GW_FILE_MAX_LENGTH];
    char *savedPtr, *destAddrPtr, *iFacePtr, *ipv4GwPtr, *endPtr;
    const char *ipv4ZeroGwPtr = "00000000", *interval=" \t";
    FILE  *resolvFPtr;
    struct in_addr sin4_addr;

    TAF_ERROR_IF_RET_VAL(defaultGWPtr == NULL, LE_FAULT, "defaultGWPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(defaultIntfPtr == NULL, LE_FAULT, "defaultIntfPtr is NULL!");

    resolvFPtr = le_flock_TryOpenStream(IPV4_ROUTE_INFO_FILE, LE_FLOCK_READ, &result);

    TAF_ERROR_IF_RET_VAL(resolvFPtr == NULL, LE_FAULT, "Open route information file '%s' failed,can't get default gateway", IPV4_ROUTE_INFO_FILE);

    //find the default gatway
    while (fgets(lineStr, sizeof(lineStr), resolvFPtr))
    {
        // ipv4 route table : Iface |Destination | Gateway,we just only need to get these value
        //Iface
        iFacePtr = strtok_r(lineStr,interval, &savedPtr);
        if(iFacePtr == NULL)
        {
            result = LE_NOT_FOUND;
            continue;
        }
        //destination
        destAddrPtr = strtok_r(NULL,interval, &savedPtr);
        if(destAddrPtr == NULL || strncmp(destAddrPtr , ipv4ZeroGwPtr, strlen(ipv4ZeroGwPtr)) !=0 )
        {
            result = LE_NOT_FOUND;
            continue;
        }
        //gateway
        ipv4GwPtr = strtok_r(NULL,interval, &savedPtr);
        if(ipv4GwPtr == NULL )
        {
            result = LE_NOT_FOUND;
            continue;
        }
        //got interface
        result = le_utf8_Copy(defaultIntfPtr, iFacePtr, defaultIntfPtrSize, NULL);
        if (result != LE_OK)
        {
            LE_WARN("defaultIntfPtr size is less than the length of iFace");
            continue;
        }
        //got gateway
        sin4_addr.s_addr = (uint32_t)strtoul(ipv4GwPtr, &endPtr, 16);
        result = le_utf8_Copy(defaultGWPtr, inet_ntoa(sin4_addr), defaultGWSize, NULL);
        if(result == LE_OK)//got the gateway
            break;
        else
        {
            LE_WARN("defaultGWPtr size is less than the length of gateway");
            continue;
        }
    }

    le_flock_CloseStream(resolvFPtr);

    return result;
}

void net_GetLinuxDefaultGateway(taf_net_DfltGwBackup_t *defaultGwBackupPtr, le_result_t *ipv4RetPtr, le_result_t *ipv6RetPtr)
{
    TAF_ERROR_IF_RET_NIL( (defaultGwBackupPtr == NULL) || (ipv4RetPtr== NULL) || (ipv6RetPtr== NULL), "Input parameter is null");

    *ipv4RetPtr = GetIpv4DefaultGatewayFromFile(
        defaultGwBackupPtr->ipV4Gateway, sizeof(defaultGwBackupPtr->ipV4Gateway),
        defaultGwBackupPtr->ipV4InterfaceName, sizeof(defaultGwBackupPtr->ipV4InterfaceName));

    *ipv6RetPtr = GetIpv6DefaultGatewayFromFile(
        defaultGwBackupPtr->ipV6Gateway, sizeof(defaultGwBackupPtr->ipV6Gateway),
        defaultGwBackupPtr->ipV6InterfaceName, sizeof(defaultGwBackupPtr->ipV6InterfaceName));

}

static le_result_t SetLinuxIPv6DefaultGateway(const char *intfPtr, const char *gatewayPtr)
{
    char  systemCallDelCmd[NET_SYSTEM_CALL_CMD_MAX_LENGTH] = {0};
    char  systemCallAddCmd[NET_SYSTEM_CALL_CMD_MAX_LENGTH] = {0};
    le_result_t ret,delRet;
    uint16_t addRetLen=0,delRetLen=0;
    taf_net_DfltGwBackup_t defaultGwBackup;

    TAF_ERROR_IF_RET_VAL(intfPtr == NULL, LE_FAULT, "intfPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(gatewayPtr == NULL, LE_FAULT, "gatewayPtr is NULL!");

    addRetLen=(unsigned int)snprintf(systemCallAddCmd, sizeof(systemCallAddCmd),
                                     IP_COMMAND " -6 route add default via %s dev %s", gatewayPtr, intfPtr);

    TAF_ERROR_IF_RET_VAL((addRetLen >= sizeof(systemCallAddCmd)), LE_FAULT, "command length too long, execute command '%s' failed.", systemCallAddCmd);

    memset(&defaultGwBackup, 0x0, sizeof(taf_net_DfltGwBackup_t));

    //get current default gateway ipv6 address from system
    ret=GetIpv6DefaultGatewayFromFile(
            defaultGwBackup.ipV6Gateway, sizeof(defaultGwBackup.ipV6Gateway),
            defaultGwBackup.ipV6InterfaceName, sizeof(defaultGwBackup.ipV6InterfaceName));

    //default gateway exists,delete it
    if(ret == LE_OK && strlen(defaultGwBackup.ipV6Gateway) > 0 && strlen(defaultGwBackup.ipV6InterfaceName) >0 )
    {
        if( (strncmp(intfPtr, defaultGwBackup.ipV6InterfaceName, NET_INTERFACE_NAME_MAX_BYTES) == 0 ) &&
            (strncmp(gatewayPtr, defaultGwBackup.ipV6Gateway, NET_IPV6_ADDR_MAX_BYTES) == 0 ))
            {
                return LE_DUPLICATE;
            }

        // Delete the current default IPv6 gateway addr from the system
        delRetLen=snprintf(systemCallDelCmd, sizeof(systemCallDelCmd), IP_COMMAND " -6 route del default");
        delRet=CallLinuxPopen(systemCallDelCmd,delRetLen);
        LE_DEBUG("delete old default gateway :%s", systemCallDelCmd);
        if(delRet != LE_OK)
        {
            LE_ERROR("Failed to delete old default gateway");
            return LE_FAULT;
        }
    }
    //add new default gateway
    if(ret == LE_NOT_FOUND || ret == LE_OK)
    {
        ret=CallLinuxPopen(systemCallAddCmd,addRetLen);
        return ret;
    }
    else
    {
        LE_ERROR("Default gateway error");
        return LE_FAULT;
    }

}

static le_result_t SetLinuxIPv4DefaultGateway(const char *intfPtr, const char *gatewayPtr)
{
    char  systemCallDelCmd[NET_SYSTEM_CALL_CMD_MAX_LENGTH] = {0};
    char  systemCallAddCmd[NET_SYSTEM_CALL_CMD_MAX_LENGTH] = {0};
    le_result_t ret,delRet;
    uint16_t addRetLen=0,delRetLen=0;
    taf_net_DfltGwBackup_t defaultGwBackup;

    TAF_ERROR_IF_RET_VAL(intfPtr == NULL, LE_FAULT, "intfPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(gatewayPtr == NULL, LE_FAULT, "gatewayPtr is NULL!");

    addRetLen=(unsigned int)snprintf(systemCallAddCmd, sizeof(systemCallAddCmd),
                                     IP_COMMAND " -4 route add default via %s dev %s", gatewayPtr, intfPtr);

    TAF_ERROR_IF_RET_VAL((addRetLen >= sizeof(systemCallAddCmd)), LE_FAULT, "command length too long, execute command '%s' failed.", systemCallAddCmd);

    memset(&defaultGwBackup, 0x0, sizeof(taf_net_DfltGwBackup_t));

    //get current default gateway ipv6 address from system
    ret=GetIpv4DefaultGatewayFromFile(defaultGwBackup.ipV4Gateway,
            sizeof(defaultGwBackup.ipV4Gateway), defaultGwBackup.ipV4InterfaceName, sizeof(defaultGwBackup.ipV4InterfaceName));

    //default gateway exists,delete it
    if(ret == LE_OK && strlen(defaultGwBackup.ipV4Gateway) >0 && strlen(defaultGwBackup.ipV4InterfaceName) > 0 )
    {
        if( (strncmp(intfPtr, defaultGwBackup.ipV4InterfaceName, NET_INTERFACE_NAME_MAX_BYTES) == 0 ) &&
            (strncmp(gatewayPtr, defaultGwBackup.ipV4Gateway, NET_IPV6_ADDR_MAX_BYTES) == 0 ))
            {
                return LE_DUPLICATE;
            }

        // Delete the current default IPv4 gateway addr from the system
        delRetLen=snprintf(systemCallDelCmd, sizeof(systemCallDelCmd), IP_COMMAND " -4 route del default");
        delRet=CallLinuxPopen(systemCallDelCmd,delRetLen);

        if(delRet != LE_OK)
        {
            LE_ERROR("Failed to delete old default gateway");
            return LE_FAULT;
        }
    }
    //add new default gateway
    if(ret == LE_NOT_FOUND || ret == LE_OK)
    {
        ret=CallLinuxPopen(systemCallAddCmd,addRetLen);
        return ret;
    }
    else
    {
        LE_ERROR("Default gateway error");
        return LE_FAULT;
    }

}

le_result_t net_SetLinuxDefaultGateway(const char *intfPtr, const char *gatewayPtr,bool isIpv4)
{
    le_result_t ret;
    struct sockaddr_in6 addr6;
    struct sockaddr_in addr;

    TAF_ERROR_IF_RET_VAL(intfPtr == NULL, LE_FAULT, "intfPtr is NULL!");
    TAF_ERROR_IF_RET_VAL(gatewayPtr == NULL, LE_FAULT, "gatewayPtr is NULL!");
    TAF_ERROR_IF_RET_VAL((strlen(gatewayPtr) == 0) || (strlen(intfPtr) == 0), LE_FAULT, "Empty gateway or interface");

    if (isIpv4 && inet_pton(AF_INET, gatewayPtr, &(addr.sin_addr)) !=1 )
    {
        LE_ERROR("type is ipv4 but address is ipv6");
        return LE_FAULT;
    }

    if (!isIpv4 && inet_pton(AF_INET6, gatewayPtr, &(addr6.sin6_addr)) !=1 )
    {
        LE_ERROR("type is ipv6 but address is ipv4");
        return LE_FAULT;
    }

    if (!isIpv4 && inet_pton(AF_INET6, gatewayPtr, &(addr6.sin6_addr)) ==1)
    {
        ret = SetLinuxIPv6DefaultGateway(intfPtr, gatewayPtr);
        return ret;
    }
    else if (isIpv4 && inet_pton(AF_INET, gatewayPtr, &(addr.sin_addr)) ==1 )
    {
        ret = SetLinuxIPv4DefaultGateway(intfPtr, gatewayPtr);
        return ret;
    }
    else
    {
        LE_ERROR("invalid ip address ");
        return LE_FAULT;
    }

    return ret;
}

le_result_t net_SetLinuxDnsNameServers(const char *dns1Ptr, const char *dns2Ptr, bool isIpv4)
{
    le_result_t result;
    FILE  *resolvFPtr;
    struct sockaddr_in6 addr6;
    struct sockaddr_in addr;
    int file_handle;
    std::string filtered_str="";
    char temp_str[NET_IPV6_ADDR_MAX_BYTES]="";
    const char matchNameserver[] = "nameserver ";
    char ipv4Addr[NET_DNS_MAX_NUMBER_PER_INTERFACE][NET_IPV4_ADDR_MAX_BYTES] = {{0}, {0}};
    char ipv6Addr[NET_DNS_MAX_NUMBER_PER_INTERFACE][NET_IPV6_ADDR_MAX_BYTES] = {{0}, {0}};
    uint8_t ipv4Num=0,ipv6Num=0,i=0;

    TAF_ERROR_IF_RET_VAL(dns1Ptr == NULL, LE_BAD_PARAMETER, "dns1Ptr  is NULL!");
    TAF_ERROR_IF_RET_VAL(dns2Ptr == NULL, LE_BAD_PARAMETER, "dns2Ptr  is NULL!");

    //check if no ip address
    TAF_ERROR_IF_RET_VAL((strlen(dns1Ptr) == 0) && (strlen(dns2Ptr) == 0), LE_BAD_PARAMETER, "no ip address to be set");

    //check if first ip is a valid ip
    if( strlen(dns1Ptr) > 0 && ((!isIpv4 && inet_pton(AF_INET6, dns1Ptr, &(addr6.sin6_addr)) != 1 ) ||
        (isIpv4 && inet_pton(AF_INET, dns1Ptr, &(addr.sin_addr)) != 1)))
    {
        LE_ERROR("first dns is not a valid ip address ");
        return LE_BAD_PARAMETER;
    }
    //check if second ip is a valid ip
    if( strlen(dns2Ptr) > 0 && ((!isIpv4 && inet_pton(AF_INET6, dns2Ptr, &(addr6.sin6_addr)) != 1 ) ||
        (isIpv4 && inet_pton(AF_INET, dns2Ptr, &(addr.sin_addr)) != 1)))
    {
        LE_ERROR("second ip is not a valid ip address");
        return LE_BAD_PARAMETER;
    }

    resolvFPtr = le_flock_TryOpenStream(DNS_INFO_FILE, LE_FLOCK_READ_AND_WRITE, &result);

    TAF_ERROR_IF_RET_VAL(resolvFPtr == NULL, LE_FAULT, "Open DNS file '%s' failed", DNS_INFO_FILE);

    //find the dns address in the file
    while((fgets(temp_str, strlen(matchNameserver)+1,resolvFPtr)!=NULL))
    {
        if(strncmp(temp_str, matchNameserver, strlen(matchNameserver))==0)
        {
            fscanf(resolvFPtr, "%s", temp_str);
            // save old IPV4 DNS address
            if (inet_pton(AF_INET, temp_str, &(addr.sin_addr)) == 1)
            {
                if(ipv4Num >= NET_DNS_MAX_NUMBER_PER_INTERFACE)
                {
                    LE_ERROR("only support 2 ipv4 DNS address ");
                    break;
                }

                le_utf8_Copy(ipv4Addr[ipv4Num], temp_str, NET_IPV4_ADDR_MAX_BYTES, NULL);
                ipv4Num++;
            }
            // save old IPV6 DNS address
            if (inet_pton(AF_INET6, temp_str, &(addr6.sin6_addr)) == 1)
            {
                if(ipv6Num >= NET_DNS_MAX_NUMBER_PER_INTERFACE)
                {
                    LE_ERROR("only support 2 ipv6 DNS address");
                    break;
                }

                le_utf8_Copy(ipv6Addr[ipv6Num], temp_str, NET_IPV6_ADDR_MAX_BYTES, NULL);
                ipv6Num++;
            }
        }
    }

    filtered_str.clear();

    if(isIpv4)
    {
        //first dns need saving
        if(strlen(dns1Ptr) > 0 && strlen(dns2Ptr) == 0)
        {
            //dns1 is not present, save it
            if(strncmp(ipv4Addr[0],dns1Ptr,NET_IPV4_ADDR_MAX_BYTES) != 0 &&
               strncmp(ipv4Addr[1],dns1Ptr,NET_IPV4_ADDR_MAX_BYTES) != 0)
               le_utf8_Copy(ipv4Addr[0], dns1Ptr, NET_IPV4_ADDR_MAX_BYTES, NULL);
        }
        //second dns need saving
        else if(strlen(dns1Ptr) == 0 && strlen(dns2Ptr) > 0)
        {
            //dns2 is not present, save it
            if(strncmp(ipv4Addr[0],dns2Ptr,NET_IPV4_ADDR_MAX_BYTES) != 0 &&
               strncmp(ipv4Addr[1],dns2Ptr,NET_IPV4_ADDR_MAX_BYTES) != 0)
                le_utf8_Copy(ipv4Addr[1], dns2Ptr, NET_IPV4_ADDR_MAX_BYTES, NULL);
        }
        //first and second dns need saving
        else if(strlen(dns1Ptr) > 0 && strlen(dns2Ptr) > 0)
        {
            //dns1 exist
            le_utf8_Copy(ipv4Addr[0], dns1Ptr, NET_IPV4_ADDR_MAX_BYTES, NULL);
            le_utf8_Copy(ipv4Addr[1], dns2Ptr, NET_IPV4_ADDR_MAX_BYTES, NULL);
        }
    }
    if(!isIpv4)
    {
        //first dns need saving
        if(strlen(dns1Ptr) > 0 && strlen(dns2Ptr) == 0)
        {
            //dns1 is not present, save it
            if(strncmp(ipv6Addr[0],dns1Ptr,NET_IPV6_ADDR_MAX_BYTES) != 0 &&
               strncmp(ipv6Addr[1],dns1Ptr,NET_IPV6_ADDR_MAX_BYTES) != 0)
               le_utf8_Copy(ipv6Addr[0], dns1Ptr, NET_IPV6_ADDR_MAX_BYTES, NULL);
        }
        //second dns need saving
        else if(strlen(dns1Ptr) == 0 && strlen(dns2Ptr) > 0)
        {
            //dns2 is not present, save it
            if(strncmp(ipv6Addr[0],dns1Ptr,NET_IPV6_ADDR_MAX_BYTES) != 0 &&
               strncmp(ipv6Addr[1],dns1Ptr,NET_IPV6_ADDR_MAX_BYTES) != 0)
                le_utf8_Copy(ipv6Addr[1], dns2Ptr, NET_IPV6_ADDR_MAX_BYTES, NULL);
        }
        //first and second dns need saving
        else if(strlen(dns1Ptr) > 0 && strlen(dns2Ptr) > 0)
        {
            //dns1 exist
            le_utf8_Copy(ipv6Addr[0], dns1Ptr, NET_IPV6_ADDR_MAX_BYTES, NULL);
            le_utf8_Copy(ipv6Addr[1], dns2Ptr, NET_IPV6_ADDR_MAX_BYTES, NULL);
        }
    }

    for(i=0;i< NET_DNS_MAX_NUMBER_PER_INTERFACE;i++)
    {
        if(strlen(ipv4Addr[i]) > 0)
        {
            filtered_str.append(matchNameserver);
            filtered_str.append(ipv4Addr[i]);
            filtered_str.append("\n");
        }
    }
    for(i=0;i< NET_DNS_MAX_NUMBER_PER_INTERFACE;i++)
    {
        if(strlen(ipv6Addr[i]) > 0)
        {
            filtered_str.append(matchNameserver);
            filtered_str.append(ipv6Addr[i]);
            filtered_str.append("\n");
        }
    }
    //rewrite the dns file
    file_handle=fileno(resolvFPtr);
    ftruncate(file_handle, 0);
    lseek(file_handle, 0, SEEK_SET);

    if(fputs(filtered_str.c_str(), resolvFPtr) < 0)
    {
        le_flock_CloseStream(resolvFPtr);
        return LE_FAULT;
    }

    le_flock_CloseStream(resolvFPtr);
    return LE_OK;
}

