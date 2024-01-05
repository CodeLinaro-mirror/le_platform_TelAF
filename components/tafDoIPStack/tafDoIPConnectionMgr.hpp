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
                char* ip, int port);
            std::shared_ptr<Connection> FindConnectionByLogicalAddr(uint16_t logicalAddr);
            std::shared_ptr<Connection> FindConnectionBySocket(le_socket_Ref_t sockRef);
            taf_doip_Result_t DeleteConnection(std::shared_ptr<Connection> connectPtr);
            void DeleteAllConnection();

            size_t GetConnectionNum();
            size_t GetRegisteredConnectionNum();

            void PerformAllAliveCheck(std::shared_ptr<Connection> connection);
            void PerformSigleAliveCheck(std::shared_ptr<Connection> connection, uint16_t ta);

            taf_doip_Result_t InformUdsMessage(uint16_t sa, uint16_t ta,
                char* data, uint32_t length);

            void ReportConnectionEvent(uint16_t sa, uint16_t ta, taf_doip_Result_t rgistResult);

            // For DoIP stack message. except UDS message type.
            le_mem_PoolRef_t    inMsgPool = NULL;
            le_mem_PoolRef_t    outMsgPool = NULL;

            // For UDS message.
            le_mem_PoolRef_t    udsMsgPool = NULL;

            uint32_t            aliveCheckNcts = 0;  // NCTS: Currently open TCP_DATA sockets
        private:
            static void ConnectionDeleter(Connection* connPtr);
            std::shared_ptr<Connection> ServerCreateConnection(le_socket_Ref_t sockRef,
                std::string& ip, uint16_t port);

            //CommunicationMgr    &communicateMgr;

            // store all the connection created
            std::vector<std::shared_ptr<Connection>> connectionBox;
    };
}
}
#endif  // TAFDOIP_CONNECTION_MGR_HPP
