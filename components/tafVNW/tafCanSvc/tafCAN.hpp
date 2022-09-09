/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef TAFCAN_HPP
#define TAFCAN_HPP

#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <time.h>
#include <net/if.h>
#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#define SIOCDEVPRIVATE 0x89F0
#define IOCTL_ADD_FRAME_FILTER (SIOCDEVPRIVATE + 2)

// Base frame_id is upto 11 bits can_id
#define MAX_SFF_FRAME_ID 0x7ff

// Extended frame_id is upto 29 bits can_id
#define MAX_EFF_FRAME_ID 0x1FFFFFFF

// Max number of Handler
#define MAX_CAN_HANLDER 32

// Max number of interface reference
#define MAX_CAN_INTERFACE 32

// Max number of frame reference
#define MAX_CAN_FRAME 32

typedef struct
{
    uint32_t                     frameId;
    uint32_t                     frIdMask;
    taf_can_CanInterfaceRef_t    canInfRef;
    taf_can_CanEventHandlerRef_t handlerRef;
    taf_can_CallbackFunc_t       handlerPtr;
    void*                        contextPtr;
    le_dls_Link_t                link;
}taf_CallbackHandler_t;

/**
 * Create interface record definition
 */
typedef struct
{
    int                       sockFd;
    int8_t                    ifNo;
    bool                      canFdEnabled;
    bool                      loopackEnabled;
    bool                      recvOwnMsgEnabled;
    taf_can_InfProtocol_t     canInfType;
    taf_can_CanInterfaceRef_t canInfRef;
    le_fdMonitor_Ref_t        fdMonitorRef;
    le_msg_SessionRef_t       sessionRef;
}
taf_canInterface_t;

/**
 * Create frame record definition
 */
typedef struct
{
    taf_can_CanInterfaceRef_t canInfRef;
    uint32_t                  frameId;
    uint8_t                   dataPtr[TAF_CAN_DATA_MAX_LENGTH];
    uint8_t                   dataLen;
    taf_can_FrameType_t       frameType;
    taf_can_CanFrameRef_t     canFrameRef;
    le_dls_Link_t             canFrameLink;
}
taf_canFrame_t;

typedef struct
{
    uint8_t  ifNo;
    uint32_t frameId;
    uint32_t frIdMask;
}
taf_canHwFilter_t;

namespace telux {
namespace tafsvc {
    class taf_Can : public ITafSvc{

        typedef struct {
            taf_can_CanInterfaceRef_t canInfRef;
            int                       receiveFrame;
            struct canfd_frame        frame;
        }taf_canEvent_t;

        public:
            taf_Can() {};
            ~taf_Can() {};
            void Init();
            static taf_Can &GetInstance();

            static void OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *contextPtr);

            taf_can_CanInterfaceRef_t CreateCanInf(const char* infNamePtr,
                    taf_can_InfProtocol_t canInfType);
            le_result_t SetFilter(taf_can_CanInterfaceRef_t canInfRef, uint32_t frameId);
            le_result_t EnableLoopback(taf_can_CanInterfaceRef_t canInfRef);
            le_result_t DisableLoopback(taf_can_CanInterfaceRef_t canInfRef);
            le_result_t EnableRcvOwnMsg(taf_can_CanInterfaceRef_t canInfRef);
            le_result_t DisableRcvOwnMsg(taf_can_CanInterfaceRef_t canInfRef);
            bool IsFdSupported(taf_can_CanInterfaceRef_t canInfRef);
            le_result_t EnableFdFrame(taf_can_CanInterfaceRef_t canInfRef);
            bool GetFdStatus(taf_can_CanInterfaceRef_t canInfRef);

            static void HandleCanCallback(void* reportPtr);
            taf_can_CanEventHandlerRef_t AddCanEventHandler(taf_can_CanInterfaceRef_t canInfRef,
                    uint32_t frameId, uint32_t frIdMask, taf_can_CallbackFunc_t handlerPtr,
                            void* contextPtr);
            void RemoveCanEventHandler(taf_can_CanEventHandlerRef_t ref_t);
            le_result_t createFdMonitor(taf_can_CanInterfaceRef_t canInfRef);
            static void CanRawSockHandler(int sock, short events);

            taf_can_CanFrameRef_t CreateCanFrame(taf_can_CanInterfaceRef_t canInfRef,
                    uint32_t frameId);
            le_result_t SetPayload(taf_can_CanFrameRef_t frameRef, const uint8_t* dataPtr,
                    size_t datalen);
            le_result_t SetFrameType(taf_can_CanFrameRef_t frameRef,
                    taf_can_FrameType_t frameType);
            le_result_t SendFrame(taf_can_CanFrameRef_t frameRef);

            le_result_t DeleteCanInf(taf_can_CanInterfaceRef_t canInfRef);
            le_result_t DeleteCanFrame(taf_can_CanFrameRef_t frameRef);

        private:
            le_ref_MapRef_t CanInfRefMap     = NULL;
            le_mem_PoolRef_t CanInfPool      = NULL;

            le_ref_MapRef_t CanFrameRefMap   = NULL;
            le_mem_PoolRef_t CanFramePool    = NULL;
            le_dls_List_t CanFrameList;

            le_mem_PoolRef_t HandlerPool     = NULL;
            le_ref_MapRef_t HandlerRefMap    = NULL;
            le_mem_PoolRef_t tafCanEventPool = NULL;
            le_dls_List_t CanHandlerList;
            le_event_Id_t tafCanEvent;

    };
}
}
#endif /* #ifndef TAFCAN_HPP */