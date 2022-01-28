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

#ifndef _TAF_ROUTING_DNS_H_
#define _TAF_ROUTING_DNS_H_

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#define IPV4_ROUTE_INFO_FILE   "/proc/net/route"
#define IPV6_ROUTE_INFO_FILE   "/proc/net/ipv6_route"
#define IP_COMMAND             "/sbin/ip"
#define DNS_INFO_FILE          "/etc/resolv.conf"

#define NET_SYSTEM_CALL_CMD_MAX_LENGTH         200
#define NET_ONE_LINE_IN_GW_FILE_MAX_LENGTH     200
#define NET_INTERFACE_NAME_MAX_BYTES           20
#define NET_IPV4_ADDR_MAX_BYTES                16
#define NET_IPV6_ADDR_MAX_BYTES                46
#define NET_DNS_SERVER_MAX_NUMBER              50
#define NET_IPV6_HEX_LENGTH                    32
#define NET_DNS_MAX_NUMBER_PER_INTERFACE       2

typedef struct
{
    char ipV6Gateway[NET_IPV6_ADDR_MAX_BYTES];
    char ipV6InterfaceName[NET_INTERFACE_NAME_MAX_BYTES];
    bool isSetIpV6Gw;
    char ipV4Gateway[NET_IPV4_ADDR_MAX_BYTES];
    char ipV4InterfaceName[NET_INTERFACE_NAME_MAX_BYTES];
    bool isSetIpV4Gw;
    le_msg_SessionRef_t sessionRef;
}
taf_net_DfltGwBackup_t;

void net_InitRoutingDnsMutex();

le_result_t net_ChangeLinuxRoute(
    const char *intfPtr,
    const char *destAddrPtr,
    const char *subMaskPtr,
    uint16_t   metric,
    taf_net_NetAction_t isAdd
);

void net_GetLinuxDefaultGateway
(
    taf_net_DfltGwBackup_t *defaultGwBackupPtr,
    le_result_t *ipv4RetPtr,
    le_result_t *ipv6RetPtr
);

le_result_t net_SetLinuxDefaultGateway
(
    const char *intfPtr,
    const char *gatewayPtr,
    bool        isIpv4
);

le_result_t net_SetLinuxDnsNameServers
(
    const char *dns1Ptr,
    const char *dns2Ptr,
    bool isIpv4
);

#endif   //_TAF_ROUTING_DNS_H_
