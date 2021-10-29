/*
* Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the
* disclaimer below) provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following
* disclaimer in the documentation and/or other materials provided
* with the distribution.
*
* * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
* contributors may be used to endorse or promote products derived
* from this software without specific prior written permission.
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
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

static taf_rsim_MessageHandlerRef_t  MsgHandlerRef;
static int clientConnectionFd = -1;
static bool ClientConnected = false;

static void SendSAPMessageRequest(const uint8_t* msgPtr, size_t msgLen) {
    //Check client connected or not
    if (false == ClientConnected) {
        LE_ERROR("client not connected send request failed");
        return;
    }

    //Send request
    int bytes;
    if (clientConnectionFd != -1) {
        if ((bytes = write(clientConnectionFd, msgPtr, msgLen)) < 0) {
            LE_ERROR("Write to client failed! %m\n");
        } else if (bytes != msgLen) {
            LE_ERROR("Write to client only sent %d bytes out of %d!\n", bytes, msgLen);
        }
    }

}

static void SAPMessageHandler(const uint8_t* msgPtr,
                        size_t msgSize, void* contextPtr) {
    LE_INFO("Received SAP message ClientConnected = %d ", ClientConnected);
    LE_DUMP(msgPtr, msgSize);
    SendSAPMessageRequest(msgPtr, msgSize);
}

static void CallbackHandler( uint8_t messageId, le_result_t result, void* contextPtr) {
    LE_DEBUG("Sending result: messageId=%d, result=%d, context=%p", messageId, result, contextPtr);
}

static void SocketEventHandler (int fd) {
    uint8_t buffer[TAF_RSIM_MAX_MSG_SIZE];
    int bytes;
    memset(buffer, 0, sizeof(buffer));
    bytes = read(fd, buffer, TAF_RSIM_MAX_MSG_SIZE);
    if (bytes <= 0) {
        LE_ERROR("Connection closed or failed to read from client! %m\n" );
    } else {
        //Send message
        taf_rsim_SendMessage(buffer, bytes, CallbackHandler, NULL);
    }
}

static void SocketHandler( int fd, short events)
{
    if (events & POLLERR) {
        LE_ERROR("Error socket");
    }
    if (events & POLLIN) {
        SocketEventHandler(fd);
    }
}

static void SignalHandler( int sigNum)
{
    LE_INFO("End remote sim sample test app");

    //unregister the handler
    taf_rsim_RemoveMessageHandler(MsgHandlerRef);

    if (ClientConnected) {
        close(clientConnectionFd);
        clientConnectionFd = -1;
        ClientConnected = false;
    }

    exit(EXIT_SUCCESS);
}


COMPONENT_INIT
{
    int socketFd;
    int ret;
    int optVal = 1;
    struct sockaddr_in sockAddress;

    LE_INFO("***Start tafRemoteSim***");

    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, SignalHandler);

    //Initialize socket and check for connection
    socketFd =  socket(AF_INET, SOCK_STREAM, 0);

    if (socketFd < 0) {
        LE_ERROR("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    ret = setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));
    if (ret < 0) {
        LE_ERROR("Failed to set socket options");
        close(socketFd);
        exit(EXIT_FAILURE);
    }

    sockAddress.sin_family = AF_INET;
    sockAddress.sin_port = htons(PORT);
    sockAddress.sin_addr.s_addr = INADDR_ANY;

    ret = bind(socketFd, (struct sockaddr *)&sockAddress, sizeof(sockAddress));
    if (ret < 0) {
        LE_ERROR("Failed to bind socket %m");
        close(socketFd);
        exit(EXIT_FAILURE);
    }

    ret = listen(socketFd, 1);

    if (ret < 0) {
        LE_ERROR("Failed to set socket options");
        close(socketFd);
        exit(EXIT_FAILURE);
    }

    socklen_t addressLength = sizeof(sockAddress);

    clientConnectionFd = accept(socketFd, (struct sockaddr *)&sockAddress, &addressLength);

    if (clientConnectionFd == -1) {
        LE_ERROR("Failed to accept a client connection: %m");
        exit(EXIT_FAILURE);
    }
    LE_INFO("Accepted client connection");
    ClientConnected = true;

    //Add message handler
    MsgHandlerRef = taf_rsim_AddMessageHandler(SAPMessageHandler, NULL);
    LE_ASSERT(MsgHandlerRef != NULL);
    LE_INFO("Message Handler Added successfully MsgHandlerRef = %p", MsgHandlerRef);


    le_fdMonitor_Create("rSimSapMessages", clientConnectionFd, SocketHandler, POLLIN);
}
