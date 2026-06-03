/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFDOIP_CONNECTION_HPP
#define TAFDOIP_CONNECTION_HPP

#include <string>
#include <functional>
#include <stdint.h>
#include <memory>

#include "legato.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "le_socketLib.h"
#ifdef __cplusplus
}
#endif

#include "tafDoIPStack.h"
#include "tafDoIPCommon.hpp"
#include "tafDoIPVehDiscoveryAndParser.hpp"

namespace taf{
namespace doip{

    typedef enum {
        TAF_DOIP_CONNECT_STATE_LISTEN   = 0x00,
        TAF_DOIP_CONNECT_STATE_INITIALIZED,
        TAF_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_AUTH,
        TAF_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_CONF,
        TAF_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE,
        TAF_DOIP_CONNECT_STATE_FINALIZE
    } taf_doip_ConnectState_t;

    // Payload length.
    #define TAF_DOIP_PAYLOAD_RA_MAND_LEN    7   // SA + acivation type + reserve
    #define TAF_DOIP_PAYLOAD_RA_ALL_LEN     11  // SA + acivation type + reserve + reserve

    #define TAF_DOIP_RA_ACTIVATION_TYPE_LEN 1
    #define TAF_DOIP_RA_RESERVED_LEN        4

    #define TAF_DOIP_MCTS_DEFAULT           10

    #define TAF_DOIP_ALIVE_CHECK_TIMEOUT_DEF    500
    #define TAF_DOIP_INITIAL_TIMEOUT_DEF        2000
    #define TAF_DOIP_GENERAL_TIMEOUT_DEF        300000
    #define TAF_DOIP_BATCH_SIZE             5

    typedef struct {
        char        data[TAF_DOIP_MAX_BUFFER_SIZE];
        uint32_t    dataSize;
        uint32_t    dataPos;
    } taf_doip_Buffer_t;

    typedef enum {
        TAF_DOIP_CONNECTION_TYPE_CLI,   // Client side.
        TAF_DOIP_CONNECTION_TYPE_SVR    // Server side.
    } taf_doip_ConnectType_t;

    //std::function<void(le_socket_Ref_t sockRef, short events, void* userPtr)> sockEventCb;

    extern "C" {
        void SocketEventCallbackWrapper(le_socket_Ref_t sockRef, short events, void* userPtr);
    }

    class ConnectionManager;
    class Connection;
    class Connection : public std::enable_shared_from_this<Connection> {
        public:
            Connection(std::shared_ptr<ConnectionManager> mgr,
                    le_socket_Ref_t socketRef, taf_doip_ConnectType_t type,
                    std::string& ip, uint16_t port, std::string& ifname);

            ~Connection();

            // Initialize and Start the initial timer.
            taf_doip_Result_t Start();

             // Stop all running timers. Stop to receive message from socket.
            taf_doip_Result_t Shutdown();
            taf_doip_Result_t Stop();

            void SetLogicalSourceAddr(uint16_t sa);
            uint16_t GetLogicalSourceAddr();
            void SetLogicalTargetAddr(uint16_t sa);
            uint16_t GetLogicalTargetAddr();

            bool IsLogicalAddressMatch(uint16_t logicalAddr);
            bool IsSocketRefMatch(le_socket_Ref_t sockRef);


            taf_doip_Result_t SendDoipMessage(taf_doip_Buffer_t* buffer);
            taf_doip_Result_t SendDiagMessage(char* buffer, uint32_t length);

            taf_doip_Result_t ReceiveTCPData(taf_doip_Buffer_t* buffer);
            taf_doip_Result_t ReceiveDiagMessage();

            // Socket lib will call this callback when it receives message on the sockRef.
            static void SocketEventCallback(le_socket_Ref_t sockRef, short events, void* userPtr);

            bool IsConnectionRegitered();

            static void InitialTimerHandler(le_timer_Ref_t timerRef);
            static void GeneralTimerHandler(le_timer_Ref_t timerRef);
            static void AliveCheckTimerHandler(le_timer_Ref_t timerRef);

            void StartAliveCheckTimer();

            bool            allAliveCheckFlag = false;
            bool            singleAliveCheckFlag = false;
            le_socket_Ref_t cliSockRef;

            // If need alive check, This variable will store who actived the alive check.
            std::shared_ptr<Connection> routingConnectionPtr = nullptr;

        private:
            taf_doip_Result_t SendTCPData(char* buffer, uint32_t length);

            taf_doip_Result_t CheckDoipHeader(taf_doipHeader_t& header, taf_doip_Buffer_t* buffer);
            taf_doip_Result_t ProcessDoipMessage(uint16_t payloadType, taf_doip_Buffer_t* buffer,
                    uint32_t payloadLen);
            void RoutingActiveReqHandler(char *payload, uint32_t payloadLen);

            // When state is TAF_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_AUTH,
            // authenInfo shall be provided.
            void ConnectionStateMachine(taf_doip_ConnectState_t state, uint32_t authenInfo);

            void CheckRoutingActivationAuthen(uint32_t authenInfo);
            void CheckRoutingActivationConfirm();
            void AliveCheckResHandler(char *payload, uint32_t payloadLen);
            taf_doip_Result_t DiagnosticMsgFirstHandler(char *payload, uint32_t payloadLen,
                    uint32_t receivedLen);
            taf_doip_Result_t DiagnosticMsgSecondHandler();
            taf_doip_Result_t DiagnosticMsgCliSecondHandler();
            taf_doip_Result_t DiagnosticMsgSvrSecondHandler();
            void RespondHeaderNegativeACK(taf_doipHeaderNACKCode_t nackCode, uint32_t left);
            void ReadAndDiscardMsg(size_t len);
            static void ConsumeTCPData(void* param1Ptr, void* param2Ptr);

            uint16_t                testerSA;   // Tester source logical address.
                                                // It will be assigned after regitered.
            uint16_t                entitySA;   // Entity source logical address.
                                                // If doip stack runs on the gateway,
                                                // It is the gateway logical address.

            // Tester source logical address at routine activation stage.
            uint16_t                raTesterSa;

            std::string             localIface;
            std::string             remoteIp;
            uint16_t                remotePort;
            taf_doip_ConnectState_t connState = TAF_DOIP_CONNECT_STATE_INITIALIZED;
            taf_doip_ConnectType_t  connType;
            bool                    isConsuming = false;

            // point to connection manager.
            std::shared_ptr<ConnectionManager>  connectionMgr;

            le_timer_Ref_t  initialTimerRef;
            le_timer_Ref_t  generalTimerRef;
            le_timer_Ref_t  aliveCheckTimerRef;
            uint32_t        authInfo;

            taf_doip_Buffer_t*  inBuf = NULL;
            //taf_doip_Buffer_t*   outBuf;

            char*               udsBuf = NULL;
            uint32_t            udsTotalLen = 0;
            uint32_t            udsReceived = 0;
        };
}
}
#endif  // TAFDOIP_CONNECTION_HPP
