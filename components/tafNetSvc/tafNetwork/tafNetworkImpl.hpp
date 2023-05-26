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

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include "tafSvcIF.hpp"
#include "tafRoutingDns.hpp"
#include <telux/tel/PhoneFactory.hpp>

#define TAF_NET_MAX_CLIENT_APPS         20
#define DEFAULT_PHONE_ID_1 1
#define MIN_SLOT_COUNT 1
#define MAX_SLOT_COUNT 2

namespace telux {
namespace tafsvc {

/**
*
* Data structure used to backup the system default gateway addresses,sessionref mapped with this structure
*/
typedef struct
{
    taf_net_DfltGwBackup_t backupConfig;   ///<  Data structure for default gateway backup configurations
    le_dls_Link_t dbLink;                  ///<   Double link is used to order elements as a LIFO stack
} taf_net_DfltGwConfDb_t;

// Network component implementation
class taf_Net: public ITafSvc
{
    public:
        void Init(void);
        static taf_Net &GetInstance();
        static void ClientCloseSessionHandler(le_msg_SessionRef_t sessionRef, void *contextPtr);
        le_result_t ChangeRoute(const char *intfNamePtr, const char *destAddrPtr, const char *preLenPtr, uint16_t metric, taf_net_NetAction_t isAdd);
        le_result_t BackupDefaultGW(le_msg_SessionRef_t sessionRef);
        le_result_t RestoreDefaultGW(le_msg_SessionRef_t sessionRef);
        le_result_t SetDefaultGW(le_msg_SessionRef_t sessionRef, const char *intfNamePtr, const char *ipv4AddrPtr, const char *ipv6AddrPtr);
        le_result_t SetDNS(le_msg_SessionRef_t sessionRef, const char *ipv4Addr1Ptr, const char *ipv4Addr2Ptr, const char *ipv6Addr1Ptr, const char *ipv6Addr2Ptr);
        static void FirstLayerRouteChangeHandler(void* reportPtr, void* secondLayerHandlerFunc);
        static void FirstLayerGatewayChangeHandler(void* reportPtr, void* secondLayerHandlerFunc);
        static void FirstLayerDNSChangeHandler(void* reportPtr, void* secondLayerHandlerFunc);
        le_result_t getPhoneIdFromSlotId(uint8_t slotId, uint8_t *phoneIdPtr);
        le_result_t getSlotIdFromPhoneId(uint8_t phoneId, uint8_t *slotIdPtr);

        taf_Net() {};
        ~taf_Net() {};
        le_event_Id_t RouteChangeEvId;
        le_event_Id_t GatewayChangeEvId;
        le_event_Id_t DNSChangeEvId;
        le_mem_PoolRef_t RouteChangePool;
        le_mem_PoolRef_t GatewayChangePool;
        le_mem_PoolRef_t DNSChangePool;

    private:

        taf_net_DfltGwConfDb_t *GetDefaultGwConfDbBySessionRef(le_msg_SessionRef_t sessionRef);
        void BackupDefaultGwToDb( le_msg_SessionRef_t sessionRef, taf_net_DfltGwBackup_t *backupDataPtr);

        le_mem_PoolRef_t DefaultGwConfDbPool = NULL;
        le_dls_List_t DefaultGwConfDbList = LE_DLS_LIST_INIT;
        std::shared_ptr<telux::tel::IPhoneManager> PhoneMgr;

    };

  }
}
