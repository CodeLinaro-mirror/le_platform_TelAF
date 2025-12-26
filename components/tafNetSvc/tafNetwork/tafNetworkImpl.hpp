/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include "tafSvcIF.hpp"
#include "tafRoutingDns.hpp"

#define TAF_NET_MAX_CLIENT_APPS         20
#define DEFAULT_SLOT_ID_1 1    
#define MIN_SLOT_COUNT 1
#define MAX_SLOT_COUNT 2

//telsdk
/*typedef enum {
     INVALID_SLOT_ID = -1,
     DEFAULT_SLOT_ID =  1,
     SLOT_ID_1 = DEFAULT_SLOT_ID,
     SLOT_ID_2 = 2,
     MAX_SLOT_ID = SLOT_ID_2,
  }SlotId;*/

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

    };

  }
