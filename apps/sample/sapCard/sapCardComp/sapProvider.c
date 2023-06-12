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
#include <arpa/inet.h>

#define PORT 8080
static taf_sap_MessageHandlerRef_t  MsgHandlerRef;
static struct sockaddr_in clientAddr;
static int daemonConnectionFd = -1;
static bool daemonConnected = false;

static void SendResponse(const uint8_t* msgPtr, size_t msgSize) {
    if (!daemonConnected) {
        LE_ERROR("Daemon not connected, cannot send message!\n");
        return;
    }

    int bytes;
    if ((bytes = write(daemonConnectionFd, msgPtr, msgSize)) < 0) {
        LE_ERROR("Write to daemon failed! %s\n", strerror(errno));
        return;
    } else if (bytes != msgSize) {
        LE_ERROR("Write to daemon only sent %d bytes out of %" PRIuS, bytes, msgSize);
        return;
    }
    LE_INFO("Send response done");
}

static void SAPMessageHandler(const uint8_t* msgPtr,
                        size_t msgSize, void* contextPtr) {
    LE_INFO("Received SAP message");
    LE_DUMP(msgPtr, msgSize);

    //Send response messages
    SendResponse(msgPtr, msgSize);
}

static le_result_t ConnectToDaemon() {
    if ((daemonConnectionFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        LE_ERROR("Failed to create socket! %s\n", strerror(errno));
        return LE_FAULT;
    }

    if (connect(daemonConnectionFd, (struct sockaddr *)&clientAddr,
                sizeof(struct sockaddr_in)) < 0) {
        LE_ERROR("Failed to connect to daemon! %s\n", strerror(errno));
        daemonConnectionFd = -1;
        return LE_FAULT;
    }

    LE_DEBUG("Connected to daemon.\n");
    daemonConnected = true;

    return LE_OK;
}

static le_result_t SetupClientConnection(const char * ipAddress) {
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(PORT);

    if (inet_aton(ipAddress, (struct in_addr *)&(clientAddr.sin_addr.s_addr)) == 0) {
        LE_ERROR("Failed to read daemon IP address! Tried %s\n", ipAddress);
        return LE_FAULT;
    }

    while (ConnectToDaemon() != LE_OK) {
        LE_ERROR("Connecting to daemon failed, attempting again...\n");
        sleep(5);
    }

    return LE_OK;
}

static void DaemonSocketEventHandler( int fd) {
    uint8_t buffer[TAF_SAP_MAX_MSG_SIZE];
    int bytes;
    memset(buffer, 0, sizeof(buffer));
    if ((bytes = read(fd, buffer, TAF_SAP_MAX_MSG_SIZE)) <= 0) {
        LE_ERROR("Connection closed or failed to read from daemon! %s\n", strerror(errno));
        daemonConnected = false;
    } else {
        //Send message
        LE_INFO("Read %d bytes from daemon", bytes);
        taf_sap_SendMessage(buffer, bytes);

    }
}

static void SocketHandler( int fd, short events) {
    if (events & POLLERR) {
        LE_ERROR("Error socket");
        daemonConnected = false;
    }

    if (events & POLLIN) {
        DaemonSocketEventHandler(fd);
    }
}

static le_result_t RunClientSAPProvider() {

    const char* ipString = "";
    //Read arguments and check IP address
    if (le_arg_NumArgs() >= 1) {
        ipString = le_arg_GetArg(0);
    }
    if (NULL == ipString)
    {
        LE_ERROR("ip address is NULL");
        exit(EXIT_FAILURE);
    }

    //Add handler
    MsgHandlerRef = taf_sap_AddMessageHandler(SAPMessageHandler, NULL);
    if (MsgHandlerRef == NULL)
    {
        LE_ERROR("Message handler reference is NULL");
        exit(EXIT_FAILURE);
    }

    //Set up client connection
    if (SetupClientConnection(ipString) != LE_OK) {
        LE_ERROR("SetupClientConnection failed");
        exit(EXIT_FAILURE);
    }
    le_fdMonitor_Create("SapMessages", daemonConnectionFd, SocketHandler, POLLIN);

    return LE_OK;
}

static void SignalHandler( int sigNum)
{
    LE_INFO("Termiante sap sample test app");

    //unregister the handler
    taf_sap_RemoveMessageHandler(MsgHandlerRef);

    if (daemonConnected) {
        close(daemonConnectionFd);
        daemonConnectionFd = -1;
        daemonConnected = false;
    }

    exit(EXIT_SUCCESS);
}

COMPONENT_INIT {

    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, SignalHandler);

    LE_INFO("INIT sapProvider");
    RunClientSAPProvider();
}
