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
#include <algorithm>
#include <memory>

#include "legato.h"
#include "interfaces.h"

#include "tafDoIPStack.h"
#include "tafDoIPCommon.hpp"
#include "tafDoIPConnection.hpp"
#include "tafDoIPConnectionMgr.hpp"
#include "tafDoIPCommunicationMgr.hpp"
#include "tafDoIPVehicleMgr.hpp"
#include "tafDoIPVehDiscoveryAndParser.hpp"

using namespace taf::doip;

ConnectionManager::ConnectionManager
(
)
{
    LE_DEBUG("Connection Manager is created!!!\n");
}

ConnectionManager::~ConnectionManager
(
)
{
}

void ConnectionManager::Init
(
)
{
    if (inMsgPool == NULL)
    {
        inMsgPool = le_mem_CreatePool("DoipInMsgPool", sizeof(taf_doip_Buffer_t));
        //le_mem_ExpandPool(inMsgPool, MAX_MSG_BUFFER);
    }

    if (outMsgPool == NULL)
    {
        outMsgPool = le_mem_CreatePool("DoipOutMsgPool", sizeof(taf_doip_Buffer_t));
        //le_mem_ExpandPool(outMsgPool, MAX_MSG_BUFFER);
    }

    auto& vehicleMgr = VehicleManager::GetInstance();
    uint32_t length;

    if (vehicleMgr.GetMaxDataSize(&length) != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to get max data size, using default!\n");
        length = TAF_DOIP_MDS_DEFAULT;
    }

    if (length < TAF_DOIP_MAX_BUFFER_SIZE)
    {
        length = TAF_DOIP_MAX_BUFFER_SIZE;
    }

    if (udsMsgPool == NULL)
    {
        udsMsgPool = le_mem_CreatePool("UdsMsgPool", length);
        //communicateMgr = CommunicationMgr::GetInstance();
    }

    LE_DEBUG("Connection Manager is initialized!!!\n");
}

void ConnectionManager::ConnectionDeleter
(
    Connection* connPtr
)
{
    LE_DEBUG("Delete connection object%p", connPtr);
    if (connPtr != NULL)
    {
        delete connPtr;
    }
}

std::shared_ptr<Connection> ConnectionManager::FindOrCreateConnection
(
    le_socket_Ref_t sockRef,
    char*           ip,
    int             port,
    const char*     ifacePtr
)
{
    if (sockRef == NULL)
    {
        LE_ERROR("Socket reference is invalid.\n");
        return NULL;
    }

    std::shared_ptr<Connection> connection = FindConnectionBySocket(sockRef);
    if (connection != nullptr)
    {
        return connection;
    }

    std::string ipStr, ifaceStr;
    ipStr.assign(ip);
    ifaceStr.assign(ifacePtr);
    LE_DEBUG("Create connection(%s %s:%d)", ifacePtr, ip, port);

    return ServerCreateConnection(sockRef, ipStr, (uint16_t)port, ifaceStr);
}

std::shared_ptr<Connection> ConnectionManager::ServerCreateConnection
(
    le_socket_Ref_t sockRef,
    std::string&    ip,
    uint16_t        port,
    std::string&    iface
)
{
    uint32_t    mcts;
    auto&   vehicleMgr = VehicleManager::GetInstance();

    if (vehicleMgr.GetMaxConcurrentSockNum(&mcts) != TAF_DOIP_RESULT_OK)
    {
        LE_ERROR("Failed to get max concurrent socket number.\n");
        mcts = TAF_DOIP_MCTS_DEFAULT;
    }

    if (connectionBox.size() >= mcts)
    {
        LE_ERROR("Connection reaches MCTS. Connection number is %" PRIuS,
            connectionBox.size());
        return nullptr;
    }

    std::shared_ptr<Connection> connection(new Connection(shared_from_this(), sockRef,
        TAF_DOIP_CONNECTION_TYPE_SVR, ip, (uint16_t)port, iface), ConnectionDeleter);
    connectionBox.push_back(connection);
    LE_DEBUG("connection is %p", connection.get());
    connection->Start();

    return connection;
}

std::shared_ptr<Connection> ConnectionManager::FindConnectionBySocket
(
    le_socket_Ref_t sockRef
)
{
    std::shared_ptr<Connection> connTmp = nullptr;

    for (uint32_t i = 0; i < connectionBox.size(); i++)
    {
        if (connectionBox[i]->IsSocketRefMatch(sockRef))
        {
            LE_DEBUG("Find a connection(%d-%d).\n", connectionBox[i]->GetLogicalSourceAddr(),
                    connectionBox[i]->GetLogicalTargetAddr());
            connTmp = connectionBox[i];
            break;
        }
    }

    return connTmp;
}

std::shared_ptr<Connection> ConnectionManager::FindConnectionByLogicalAddr
(
    uint16_t    logicalAddr
)
{
    std::shared_ptr<Connection> connTmp = nullptr;

    for (uint32_t i = 0; i < connectionBox.size(); i++)
    {
        if (connectionBox[i]->IsLogicalAddressMatch(logicalAddr)) {
            LE_DEBUG("Find a connection(%d-%d).\n", connectionBox[i]->GetLogicalSourceAddr(),
                    connectionBox[i]->GetLogicalTargetAddr());
            connTmp = connectionBox[i];
            break;
        }
    }

    return connTmp;
}

taf_doip_Result_t ConnectionManager::DeleteConnection
(
    std::shared_ptr<Connection> connectPtr
)
{
    if (connectPtr == nullptr)
    {
        LE_ERROR("Socket reference is invalid.\n");
        return TAF_DOIP_RESULT_PARAM_ERROR;
    }

    connectPtr->Stop();
    LE_DEBUG("Connection reference count is %ld", connectPtr.use_count());

    for (auto iter = connectionBox.begin(); iter != connectionBox.end(); iter++)
    {
        if ((*iter)->IsSocketRefMatch(connectPtr->cliSockRef))
        {
            LE_DEBUG("Deletion socket%p connection.", connectPtr->cliSockRef);
            std::swap(*iter, connectionBox.back());
            connectionBox.pop_back();
            break;
        }
    }
    connectPtr.reset();
    LE_DEBUG("Now connection box size is %" PRIuS, connectionBox.size());

    return TAF_DOIP_RESULT_OK;
}

void ConnectionManager::DeleteAllConnection
(
)
{
    for (auto iter = connectionBox.begin(); iter != connectionBox.end(); iter++)
    {
        DeleteConnection(*iter);
    }
}

size_t ConnectionManager::GetConnectionNum()
{
    return connectionBox.size();
}

size_t ConnectionManager::GetRegisteredConnectionNum()
{
    uint32_t    cnt = 0;

    for (auto iter = connectionBox.begin(); iter != connectionBox.end(); iter++)
    {
        if ((*iter)->IsConnectionRegitered())
        {
            cnt++;
        }
    }

    return cnt;
}

void ConnectionManager::PerformAllAliveCheck
(
    std::shared_ptr<Connection> connection
)
{
    auto&   parser = ProtocolParser::GetInstance();

    for (uint32_t i = 0; i < connectionBox.size(); i++)
    {
        if (connectionBox[i]->IsConnectionRegitered())
        {
            PerformSigleAliveCheck(connection, connectionBox[i]->GetLogicalTargetAddr());

            if (connectionBox[i]->allAliveCheckFlag)
            {
                // Alive check is already running.
                continue;
            }

            connectionBox[i]->allAliveCheckFlag = true;
            connectionBox[i]->routingConnectionPtr = connection;
            taf_doipLink_t link;
            link.commType = TAF_DOIP_SOCKET_TYPE_TCP;
            link.sockRef = connectionBox[i]->cliSockRef;
            parser.AliveCheckReq(&link);

            // Start the timer that is in another connection.
            connectionBox[i]->StartAliveCheckTimer();
        }
    }
}

void ConnectionManager::PerformSigleAliveCheck
(
    std::shared_ptr<Connection> connection,
    uint16_t    ta
)
{
    auto&   parser = ProtocolParser::GetInstance();
    std::shared_ptr<Connection> connectionTmp = FindConnectionByLogicalAddr(ta);
    if (connectionTmp == connection)
    {
        // Skip itself.
        return;
    }

    if (connectionTmp->singleAliveCheckFlag)
    {
        // Alive check is already running.
        return;
    }

    connectionTmp->singleAliveCheckFlag = true;
    connectionTmp->routingConnectionPtr = connection;

    taf_doipLink_t link;
    link.commType = TAF_DOIP_SOCKET_TYPE_TCP;
    link.sockRef = connectionTmp->cliSockRef;
    parser.AliveCheckReq(&link);

    // Start the timer that is in another connection.
    connectionTmp->StartAliveCheckTimer();
}

taf_doip_Result_t ConnectionManager::InformUdsMessage
(
    uint16_t    sa,
    uint16_t    ta,
    char*       data,
    uint32_t    length,
    std::string& iface
)
{
    auto&   CommMgr = CommunicationMgr::GetInstance();

    return CommMgr.InformUdsMessage(sa, ta, data, length, iface.c_str());
}

void ConnectionManager::ReportConnectionEvent
(
    uint16_t sa,
    uint16_t ta,
    taf_doip_Result_t rgistResult,
    std::string& iface
)
{
    auto&   CommMgr = CommunicationMgr::GetInstance();

    CommMgr.ReportConnectionEvent(sa, ta, rgistResult, iface.c_str());
}
