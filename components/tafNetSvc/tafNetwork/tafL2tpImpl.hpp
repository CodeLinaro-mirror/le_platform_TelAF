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
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/L2tpManager.hpp>
#include "tafSvcIF.hpp"

#define DEFAULT_MTU_SIZE       1422
using namespace telux::data;
using namespace telux::common;

/*
 * @brief The struct of l2tp configuration.
 */
typedef struct
{
    bool enableL2tp;
    bool enableMtu;
    bool enableTcpMss;
    uint32_t mtuSize;
} taf_L2tpConfig_t;

/*
 * @brief The struct of Tunnel.
 */
typedef struct
{
    uint32_t locTunnelId;  ///< Local tunnel Id
    uint32_t peerTunnelId;  ///< Peer tunnel Id
    uint32_t localUdpPort;  ///< Local udp port
    uint32_t peerUdpPort;  ///< Peer udp port
    taf_net_L2tpEncapProtocol_t encaProto;  ///< Encapsulation protocol
    char peerIpAddr[TAF_NET_IP_ADDR_MAX_LEN];  ///< Peer ip address
    char interfaceName[TAF_NET_INTERFACE_NAME_MAX_LEN]; ///< Interface name to create L2TP tunnel on
    le_dls_List_t      l2tpSessionList;  ///< Session list
    bool               isStarted;     ///< Tunnel is started by current Session
    le_msg_SessionRef_t sessionRef;  ///< Session reference
} taf_Tunnel_t;

/*
 * @brief The struct of l2tp session.
 */
typedef struct
{
    uint32_t locSessionId;  ///< Local session Id
    uint32_t peerSessionId;  ///< Peer session Id
    le_dls_Link_t        link;
} taf_L2tpSession_t;

/*
 * @brief The struct of tunnel info.
 */
typedef struct
{
    uint32_t locTunnelId;   ///< Local tunnel Id
    uint32_t peerTunnelId;  ///< Peer tunnel Id
    uint32_t localUdpPort;  ///< Local udp port
    uint32_t peerUdpPort;   ///< Peer udp port
    taf_net_L2tpEncapProtocol_t encaProto;  ///< Encapsulation protocol
    char peerIpv4Addr[TAF_NET_IPV4_ADDR_MAX_LEN];  ///< Peer ipv4 address
    char peerIpv6Addr[TAF_NET_IPV6_ADDR_MAX_LEN];  ///< Peer ipv6 address
    char interfaceName[TAF_NET_INTERFACE_NAME_MAX_LEN]; ///< Interface name to create L2TP tunnel on
    taf_net_IpFamilyType_t ipType;       ///< Ip family type
    uint16_t sessionNum;                 ///Session number
    taf_net_L2tpSessionConfig_t  l2tpSessionConfig[TAF_NET_L2TP_MAX_SESSION_NUMBER_PER_TUNNEL];
                                                                                   ///< Session list
} taf_TunnelInfo_t;

/*
 * @brief The struct of tunnel entry list.
 */
typedef struct
{
    le_sls_List_t tunnelEntryList;
    le_sls_List_t safeRefList;
    le_sls_Link_t* currPtr;
} taf_TunnelEntryList_t;

/*
 * @brief The struct of tunnel entry with link.
 */
typedef struct
{
    taf_TunnelInfo_t info;
    le_sls_Link_t link;
} taf_TunnelEntry_t;

/*
 * @brief The struct of safe reference for tunnel entry.
 */
typedef struct
{
    void* safeRef;
    le_sls_Link_t link;
} taf_TunnelEntrySafeRef_t;

/**
* @brief The emum of l2tp command type.
*/
typedef enum
{
    ASYNC_DISABLE_L2TP  = 0,
    ASYNC_ENABLE_L2TP = 1,
    ASYNC_STOP_TUNNEL  = 2,
    ASYNC_START_TUNNEL = 3
} taf_L2tpCmdType_t;

/*
* @brief The struct of l2tp command request.
*/
typedef struct
{
    taf_L2tpCmdType_t cmdType;
    union
    {
        taf_net_TunnelRef_t tunnelRef;
        struct
        {
            bool enableMss;
            bool enableMtu;
            uint32_t mtuSize;
        }enableParam;
    };
    void* contextPtr;
    le_msg_SessionRef_t sessionRef;
    union
    {
        taf_net_AsyncL2tpHandlerFunc_t l2tpHandlerFuncPtr;
        taf_net_AsyncTunnelHandlerFunc_t tunnelHandlerFuncPtr;
    };
} taf_L2tpCmdReq_t;

/**
* @brief The emum of l2tp event type.
*/
typedef enum
{
    EVT_ENABLE_L2TP_ASYNC_CALLBACK,
    EVT_DISABLE_L2TP_ASYNC_CALLBACK,
    EVT_START_TUNNEL_ASYNC_CALLBACK,
    EVT_STOP_TUNNEL_ASYNC_CALLBACK
}taf_L2tpEvtType_t;

typedef struct
{
    taf_L2tpEvtType_t                      event;
    telux::common::ErrorCode                errorCode;
} taf_L2tpEventReq_t;

typedef struct
{
    taf_net_AsyncL2tpHandlerFunc_t asyncHandler;  ///< async handler
    void *contextPtr;
    le_msg_SessionRef_t sessionRef;
    taf_L2tpCmdType_t type;
    le_dls_Link_t handlerLink;                     ///< double link list's link element
}L2tpHandlerMapping_t;

typedef struct
{
    taf_net_AsyncTunnelHandlerFunc_t asyncHandler;  ///< async handler
    taf_net_TunnelRef_t tunnelRef;  ///< tunnel reference
    void *contextPtr;
    le_msg_SessionRef_t sessionRef;
    taf_L2tpCmdType_t type;
    le_dls_Link_t handlerLink;                     ///< double link list's link element
}TunnelHandlerMapping_t;

namespace telux{
namespace tafsvc {

    /*
     * @brief A callback class must be provided when invoke telsdk API.
     */
    class tafL2tpCallback
    {
        public:
           static void enableL2tpResponse(telux::common::ErrorCode error);
           static void disableL2tpResponse(telux::common::ErrorCode error);
           static void enableL2tpAsyncResponse(telux::common::ErrorCode error);
           static void disableL2tpAsyncResponse(telux::common::ErrorCode error);
           static void startTunnelSyncResponse(telux::common::ErrorCode error);
           static void stopTunnelSyncResponse(telux::common::ErrorCode error);
           static void startTunnelAsyncResponse(telux::common::ErrorCode error);
           static void stopTunnelAsyncResponse(telux::common::ErrorCode error);
           static void requestConfigResponse(const telux::data::net::L2tpSysConfig &l2tpSysConfig,
                                                     telux::common::ErrorCode error);

            tafL2tpCallback(){};
            ~tafL2tpCallback(){};
            static taf_L2tpConfig_t l2tpConfig;
            static std::vector<telux::data::net::L2tpTunnelConfig> configList;
            static le_sem_Ref_t semaphore;
    };

    /*
     * @brief taf_L2tp class defined as a middleware between interfaces and implementation.
     */
    class taf_L2tp :public ITafSvc
    {
        public:
            taf_L2tp() {};
            ~taf_L2tp() {};

            void Init(void);
            static taf_L2tp &GetInstance();

            static void* L2tpEventThread(void* contextPtr);
            static void L2tpProcEvtHandler(void* cmdReqPtr);
            static void* L2tpCmdThread(void* contextPtr);
            static void L2tpProcCmdHandler(void* cmdReqPtr);

            void SetTunnelStatus(taf_net_TunnelRef_t tunnelRef, bool isStarted);

            le_result_t EnableL2tpCmdSync(bool enableMss, bool enableMtu, uint32_t mtuSize);
            le_result_t DisableL2tpCmdSync(le_msg_SessionRef_t sessionRef);
            void EnableL2tpCmdAsync(bool enableMss, bool enableMtu, uint32_t mtuSize,
                                    taf_net_AsyncL2tpHandlerFunc_t handlerPtr, void* contextPtr,
                                    le_msg_SessionRef_t sessionRef);
            void DisableL2tpCmdAsync(taf_net_AsyncL2tpHandlerFunc_t handlerPtr, void* contextPtr,
                                     le_msg_SessionRef_t sessionRef);

            le_result_t StartTunnelCmdSync(taf_net_TunnelRef_t tunnelRef);
            le_result_t StopTunnelCmdSync(taf_net_TunnelRef_t tunnelRef);
            void StartTunnelCmdAsync(taf_net_TunnelRef_t tunnelRef,
                                     taf_net_AsyncTunnelHandlerFunc_t handlerPtr, void* contextPtr,
                                     le_msg_SessionRef_t sessionRef);
            void StopTunnelCmdAsync(taf_net_TunnelRef_t tunnelRef,
                                    taf_net_AsyncTunnelHandlerFunc_t handlerPtr, void* contextPtr,
                                    le_msg_SessionRef_t sessionRef);

            static void ClientCloseSessionHandler(le_msg_SessionRef_t sessionRef, void *contextPtr);
            void onInitComplete(telux::common::ServiceStatus status);
            le_result_t CleanListRef(taf_net_TunnelEntryListRef_t tunnelEntryListRef);
            uint16_t GetCurSessionNum(taf_net_TunnelRef_t tunnelRef);
            bool IsSessionIdValid(taf_net_TunnelRef_t tunnelRef, uint32_t locSessionId,
                                        uint32_t peerSessionId);
            bool IsTunnelStartedByOtherClient(le_msg_SessionRef_t sessionRef);
            taf_net_TunnelRef_t CreateTunnelIfExistsInDb(uint32_t tunnelId,
                                                                   le_msg_SessionRef_t sessionRef);
            le_result_t EnableL2tp(bool enableMss, bool enableMtu, uint32_t mtuSize,
                                   taf_L2tpCmdType_t type);
            le_result_t DisableL2tp(le_msg_SessionRef_t sessionRef);
            bool IsL2tpEnabled(void);
            bool IsL2tpMssEnabled(void);
            bool IsL2tpMtuEnabled(void);
            uint32_t GetL2tpMtuSize(void);
            taf_net_TunnelRef_t CreateTunnel(taf_net_L2tpEncapProtocol_t encaProto, uint32_t locId,
                                                  uint32_t peerId, const char* peerIpAddrPtr,
                                                  const char* ifNamePtr,
                                                  le_msg_SessionRef_t sessionRef);
            le_result_t RemoveTunnel(taf_net_TunnelRef_t tunnelRef);
            le_result_t SetTunnelUdpPort(taf_net_TunnelRef_t tunnelRef, uint32_t localUdpPort,
                                               uint32_t peerUdpPort);
            le_result_t AddSession(taf_net_TunnelRef_t tunnelRef, uint32_t locSessionId,
                                       uint32_t peerSessionId);
            le_result_t RemoveSession(taf_net_TunnelRef_t tunnelRef, uint32_t locId,
                                           uint32_t peerId);
            le_result_t AddTunnelAsync(taf_net_TunnelRef_t tunnelRef);
            le_result_t RemoveTunnelAsync(taf_net_TunnelRef_t tunnelRef);
            taf_net_TunnelRef_t GetTunnelRefById(uint32_t locTunnelId,
                                                 le_msg_SessionRef_t sessionRef);
            taf_net_TunnelEntryListRef_t GetTunnelEntryList(void);
            taf_net_TunnelEntryRef_t GetFirstTunnelEntry(
                                                   taf_net_TunnelEntryListRef_t tunnelEntryListRef);
            taf_net_TunnelEntryRef_t GetNextTunnelEntry(
                                                   taf_net_TunnelEntryListRef_t tunnelEntryListRef);
            le_result_t DeleteTunnelEntryList(taf_net_TunnelEntryListRef_t tunnelEntryListRef);
            taf_net_L2tpEncapProtocol_t GetTunnelEncapProto(
                                                           taf_net_TunnelEntryRef_t tunnelEntryRef);
            uint32_t GetTunnelLocalId(taf_net_TunnelEntryRef_t tunnelEntryRef);
            uint32_t GetTunnelPeerId(taf_net_TunnelEntryRef_t tunnelEntryRef);
            uint32_t GetTunnelLocalUdpPort(taf_net_TunnelEntryRef_t tunnelEntryRef);
            uint32_t GetTunnelPeerUdpPort(taf_net_TunnelEntryRef_t tunnelEntryRef);
            le_result_t GetTunnelPeerIpv6Addr(taf_net_TunnelEntryRef_t tunnelEntryRef,
                                                      char* peerIpv6Addr, size_t peerIpv6AddrSize);
            le_result_t GetTunnelPeerIpv4Addr(taf_net_TunnelEntryRef_t tunnelEntryRef,
                                              char* peerIpv4Addr, size_t peerIpv4AddrSize);
            le_result_t GetTunnelInterfaceName(taf_net_TunnelEntryRef_t tunnelEntryRef,
                                               char* ifName, size_t ifNameSize);
            taf_net_IpFamilyType_t GetTunnelIpType(taf_net_TunnelEntryRef_t tunnelEntryRef);
            le_result_t GetSessionConfig(taf_net_TunnelEntryRef_t tunnelEntryRef,
                                               taf_net_L2tpSessionConfig_t* sessionConfigPtr,
                                               size_t* sessionConfigSizePtr);

            void AddL2tpHandlerSessionMapping(taf_net_AsyncL2tpHandlerFunc_t asyncHandler,
                                              void *contextPtr, le_msg_SessionRef_t sessionRef,
                                              taf_L2tpCmdType_t type);
            L2tpHandlerMapping_t* FindL2tpAsyncHandler(taf_L2tpCmdType_t type);
            int GetL2tpHandlerNumberInMappingList(taf_L2tpCmdType_t type);
            void DeleteL2tpHandlerInfo(taf_net_AsyncL2tpHandlerFunc_t asyncHandler);

            void AddTunnelHandlerSessionMapping(taf_net_AsyncTunnelHandlerFunc_t asyncHandler,
                                                taf_net_TunnelRef_t tunnelRef, void *contextPtr,
                                                le_msg_SessionRef_t sessionRef,
                                                taf_L2tpCmdType_t type);

            TunnelHandlerMapping_t* FindTunnelAsyncHandler(taf_L2tpCmdType_t type);
            void DeleteTunnelHandlerInfo(taf_net_AsyncTunnelHandlerFunc_t asyncHandler);

            std::promise<le_result_t> L2tpEnableSyncPromise;
            std::promise<le_result_t> L2tpDisableSyncPromise;
            std::promise<le_result_t> L2tpStartTunnelSyncPromise;
            std::promise<le_result_t> L2tpStopTunnelSyncPromise;

            le_mem_PoolRef_t tunnelPool = NULL;
            le_ref_MapRef_t tunnelRefMap = NULL;
            le_mem_PoolRef_t l2tpSessionPool = NULL;

            le_mem_PoolRef_t tunnelEntryListPool;
            le_mem_PoolRef_t tunnelEntryPool;
            le_mem_PoolRef_t tunnelEntrySafeRefPool;
            le_ref_MapRef_t tunnelEntryListRefMap;
            le_ref_MapRef_t tunnelEntrySafeRefMap;
            static le_event_Id_t l2tpCmdId;
            static le_event_Id_t l2tpEventId;
            le_dls_List_t L2tpHandlerMappingList = LE_DLS_LIST_INIT;
            le_mem_PoolRef_t L2tpHandlerMappingPool = NULL;
            le_dls_List_t TunnelHandlerMappingList = LE_DLS_LIST_INIT;
            le_mem_PoolRef_t TunnelHandlerMappingPool = NULL;

        private:
            std::shared_ptr<telux::data::net::IL2tpManager> l2tpManager = nullptr;
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
            bool IsSubSystemStatusUpdated=false;
            std::mutex mMutex;
            std::condition_variable conVar;
#endif
    };

}
}

