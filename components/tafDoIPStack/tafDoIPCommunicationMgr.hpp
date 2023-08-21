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

#ifndef TAFDOIP_COMMUNICATION_MGR_HPP
#define TAFDOIP_COMMUNICATION_MGR_HPP

#include <arpa/inet.h>
#include <string>
#include "legato.h"
#include "interfaces.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "le_socketLib.h"
#ifdef __cplusplus
}
#endif

#include "tafDoIPStack.h"
#include "tafDoIPCommon.hpp"
#include "tafDoIPConnectionMgr.hpp"
#include "tafDoIPVehDiscoveryAndParser.hpp"

namespace taf{
namespace doip{
    #define TAF_DOIP_MAX_ENTITY_NUM         1
    #define TAF_DOIP_MAX_USER_HANDLER_NUM   10

    #define TAF_DOIP_MDS_DEFAULT            4096
    #define TAF_DOIP_SA_DEFAULT             0x201

    //-------------------------------------------------------------------------------------------------
    /**
     * Enumeration of routing activation confirmation result.
     */
    //-------------------------------------------------------------------------------------------------
    typedef enum {
        TAF_DOIP_USER_CONFIRM_RESULT_REJECT = 0,    ///< Rejects confirm.
        TAF_DOIP_USER_CONFIRM_RESULT_NO_NEED,       ///< No need confirm.
        TAF_DOIP_USER_CONFIRM_RESULT_PASS           ///< Passes confirm.
    }taf_doip_UserConfirmResult_t;

    typedef void (*taf_doip_UserConfirmHandlerFunc_t)(uint16_t sa, uint16_t ta,
        taf_doip_UserConfirmResult_t* result, void* userPtr);
    typedef struct taf_doip_UserConfirmHandlerRef* taf_doip_UserConfirmHandlerRef_t;

    typedef enum
    {
        TAF_DOIP_STATE_INIT   = 0x00,
        TAF_DOIP_STATE_START,
        TAF_DOIP_STATE_RUNNING,
        TAF_DOIP_STATE_STOP,
        TAF_DOIP_STATE_FINAL
    } taf_doipState_t;

    typedef enum
    {
        TAF_DOIP_MESSAGE_TYPE_DoIP  = 0,
        TAF_DOIP_MESSAGE_TYPE_UDS,
        TAF_DOIP_MESSAGE_TYPE_UNKNOWN
    } taf_doipMessageType_t;

    typedef enum
    {
        TAF_DOIP_INSTANCE_TYPE_NODE     = 0x00, // Currently DoIP stack supported as Node type.
        TAF_DOIP_INSTANCE_TYPE_GATEWAY,
        TAF_DOIP_INSTANCE_TYPE_TESTER,
        TAF_DOIP_INSTANCE_TYPE_UNKOWN
    }taf_doipInstanceType_t;

    typedef struct
    {
        uint16_t  sa;
        uint16_t  ta;
        int       taType;
        char*     data;
        uint32_t  len;
    }taf_doipDiagDataInfo_t;

    typedef struct
    {
        le_mutex_Ref_t                          mutexRef;
        taf_doip_DiagIndicationHandlerFunc_t    funcPtr;
        void*                                   ctxPtr;
        void*                                   safeRef;
    }taf_doipIndicationHandler_t;

    typedef struct
    {
        le_mutex_Ref_t                      mutexRef;
        taf_doip_DiagConfirmHandlerFunc_t   funcPtr;
        void*                               ctxPtr;
        void*                               safeRef;
    }taf_doipDiagConfirmHandler_t;

    typedef struct
    {
        taf_doip_UserConfirmHandlerFunc_t   funcPtr;
        void*                               ctxPtr;
        void*                               safeRef;
    }taf_doipUserConfirmHandler_t;

    typedef struct
    {
        taf_doip_PowerModeQueryHandlerFunc_t    funcPtr;
        void*                                   ctxPtr;
        void*                                   safeRef;
    }taf_doipPmQueryHandler_t;

    typedef struct
    {
        uint16_t                        logicalSrcAddr;

        // Upper layer registered this callback.
        taf_doipIndicationHandler_t   diagIndicationHandler;
        taf_doipDiagConfirmHandler_t  diagConfirmHandler;
        taf_doipUserConfirmHandler_t  userConfirmHandler;

        taf_doipPmQueryHandler_t      pmQueryhandler;

        taf_doip_Ref_t                doipRef;
    }taf_doipSession_t;

    class CommunicationMgr {
        public:
            static CommunicationMgr &GetInstance();

            CommunicationMgr();
            virtual ~CommunicationMgr();

            void Init();
            taf_doip_Result_t SessionInit();
            taf_doip_Result_t SessionStart();
            taf_doip_Result_t SessionStop();
            taf_doip_Result_t SessionDeInit();

            // Socket lib will call this callback when a connection is coming on the sockRef.
            static void TcpDataSocketEventCallback(le_socket_Ref_t sockRef, short events,
                void* userPtr);

            // Receive UDP messages.
            static void UdpDiscoverSocketEventCallback(le_socket_Ref_t sockRef, short events,
                void* userPtr);

            // For unicast.
            taf_doip_Result_t SendUDPData(le_socket_Ref_t sockRef, char *desIpPtr,
                uint16_t desPort, char *messagePtr, uint32_t len);

            // For multicast
            taf_doip_Result_t SendUDPData(le_socket_Ref_t sockRef, char *messagePtr, uint32_t len);

            // For TCP
            taf_doip_Result_t SendTCPData(le_socket_Ref_t sockRef, char *messagePtr, uint32_t len);

            // When Connection receive a uds messsage.
            // It will call this function to send uds message to upper layer.
            taf_doip_Result_t InformUdsMessage(uint16_t sa, uint16_t ta, char* dataPtr,
                uint32_t length);
            taf_doip_Result_t TransUdsMessage(uint16_t sa, uint16_t ta, taf_doip_TaType_t taType,
                char* dataPtr, uint32_t length);

            void ReportConnectionEvent(uint16_t sa, uint16_t ta, taf_doip_Result_t rgistResult);

            //VehicleManager& GetVehicleManager();

            uint16_t GetEntitySourceAddress();

            le_thread_Ref_t mainThrRef = NULL; // Current thread.
            le_thread_Ref_t udsThrRef = NULL;  // Handle uds message thread.

            static void* UdsHandleThread(void* contextPtr);

            // Lister and accept client connect via this socket reference.
            le_socket_Ref_t tcpDataSockRef;

            // Send and receive UDP massage(Vehicle discovery) via this socket reference.
            le_socket_Ref_t udpDiscoverSockRef;

            // For upper layer create doip and register handler.
            le_mem_PoolRef_t doipSessionPool;
            le_ref_MapRef_t doipSessionRefMap;
            le_ref_MapRef_t doipHandlerRefMap;

            taf_doipSession_t*  FindDoipSession(uint16_t sa);
            taf_doip_PowerMode_t QueryPowerMode();
            taf_doip_UserConfirmResult_t ConfirmRoutingActivation(uint16_t sa, uint16_t ta);
            std::shared_ptr<ConnectionManager> GetConnectionMgr();
        private:
            taf_doip_Result_t GetLocalIPv4Addr(std::string& ifname, char *ip, socklen_t size);
            taf_doip_Result_t GetLocalIPv6Addr(std::string& ifname, char *ip, socklen_t size);
            taf_doip_Result_t GetLocalIPAddr(int af, std::string& ifname, char *ip, socklen_t size);

            taf_doip_Result_t RecvUdpData(le_socket_Ref_t sockRef);
            taf_doip_Result_t CheckDoipHeaderOverUdp(taf_doipHeader_t& header, char* ipPtr,
                    uint16_t port);
            void EntityProcessDoipMessageOverUdp(uint16_t payloadType, char* payload,
                    uint32_t payloadLen, const char* ipPtr, uint16_t port);

            void VehicleIdentifyReqHandler(const char* ipPtr, uint16_t port);
            void VehicleIdentifyReqWithEidHandler(char* payload, uint32_t payloadLen,
                    const char* ipPtr, uint16_t port);
            void VehicleIdentifyReqWithVinHandler(char* payload, uint32_t payloadLen,
                    const char* ipPtr, uint16_t port);
            void EntityStatusReqHandler(const char* ipPtr, uint16_t port);
            void PowerModeReqHandler(const char* ipPtr, uint16_t port);

            static void IndicateUdsMessage(void* param1Ptr, void* param2Ptr);
            static void RequestUdsMessage(void* param1Ptr, void* param2Ptr);

            void RespondHeaderNegativeACK(const char* ipPtr, uint16_t port,
                    taf_doipHeaderNACKCode_t nackCode);

            // Create Doip Connection Manager
            std::shared_ptr<ConnectionManager> connectionMgrPtr;

            // Create Doip Vehicle Manager
            //std::unique_ptr<VehicleManager> vehicleMgrPtr;

            //uint16_t sa;     // Source logical address.
            //uint32_t authenInfo;
            //char multiAddr[TAF_DOIP_IP_ADDR_MAX_LEN];
            char localIp[TAF_DOIP_IP_ADDR_MAX_LEN];

            taf_doipState_t state = TAF_DOIP_STATE_FINAL;

            taf_doipInstanceType_t  instanceType = TAF_DOIP_INSTANCE_TYPE_NODE;

            //le_event_Id_t udsEvtId;

            // For receiving and sending Udp data. Only one DoIP message in one package[DoIP-122].
            le_mem_PoolRef_t    udpMsgPool;
            char*               udpDataBuffer = NULL;
            uint32_t            udpDataLen = 0;

            le_mem_PoolRef_t    udsInfoPool;
    };

}
}
#endif  // TAFDOIP_COMMUNICATION_MGR_HPP
