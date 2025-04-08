/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAFDOIP_CONNECTION_MGR_HPP
#define TAFDOIP_CONNECTION_MGR_HPP

#include <vector>
#include <memory>

#include "legato.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "le_socketLib.h"
#ifdef __cplusplus
}
#endif

#include "tafDoIPConnection.hpp"

namespace taf{
namespace doip{

    #define MAX_MSG_BUFFER    2

    class CommunicationMgr;

    class ConnectionManager : public std::enable_shared_from_this<ConnectionManager> {
        public:
            ConnectionManager();
            ~ConnectionManager();

            void Init();

            // Function to create new connection and add to connection box
            // to handle doip request and response.
            std::shared_ptr<Connection> FindOrCreateConnection(le_socket_Ref_t sockRef,
                    char* ip, int port, const char *ifacePtr);
            std::shared_ptr<Connection> FindConnectionByLogicalAddr(uint16_t logicalAddr);
            std::shared_ptr<Connection> FindConnectionBySocket(le_socket_Ref_t sockRef);
            taf_doip_Result_t DeleteConnection(std::shared_ptr<Connection> connectPtr);
            void DeleteAllConnection();

            size_t GetConnectionNum();
            size_t GetRegisteredConnectionNum();

            void PerformAllAliveCheck(std::shared_ptr<Connection> connection);
            void PerformSigleAliveCheck(std::shared_ptr<Connection> connection, uint16_t ta);

            taf_doip_Result_t InformUdsMessage(uint16_t sa, uint16_t ta,
                    char* data, uint32_t length, std::string& iface);

            void ReportConnectionEvent(uint16_t sa, uint16_t ta,
                    taf_doip_Result_t rgistResult, std::string& iface);

            // For DoIP stack message. except UDS message type.
            le_mem_PoolRef_t    inMsgPool = NULL;
            le_mem_PoolRef_t    outMsgPool = NULL;

            // For UDS message.
            le_mem_PoolRef_t    udsMsgPool = NULL;

            uint32_t            aliveCheckNcts = 0;  // NCTS: Currently open TCP_DATA sockets
        private:
            static void ConnectionDeleter(Connection* connPtr);
            std::shared_ptr<Connection> ServerCreateConnection(le_socket_Ref_t sockRef,
                    std::string& ip, uint16_t port, std::string& iface);

            //CommunicationMgr    &communicateMgr;

            // store all the connection created
            std::vector<std::shared_ptr<Connection>> connectionBox;
    };
}
}
#endif  // TAFDOIP_CONNECTION_MGR_HPP
