/*
 *  Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafCAN.hpp"

LE_MEM_DEFINE_STATIC_POOL(tafCanInf, MAX_CAN_INTERFACE, sizeof(taf_canInterface_t));
LE_MEM_DEFINE_STATIC_POOL(tafCanFrame, MAX_CAN_FRAME, sizeof(taf_canFrame_t));
LE_MEM_DEFINE_STATIC_POOL(tafCanHandlerPool, MAX_CAN_HANLDER, sizeof(taf_CallbackHandler_t));

using namespace tafsvc;
using namespace std;

struct sockaddr_can scAddr;

/**
 * Returns CAN instance
 */
taf_Can &taf_Can::GetInstance()
{
    static taf_Can instance;
    return instance;
}

void taf_Can::Init(void)
{
    LE_INFO("tafCanSvc started");

    CanInfPool = le_mem_InitStaticPool(tafCanInf, MAX_CAN_INTERFACE,
            sizeof(taf_canInterface_t));
    CanInfRefMap = le_ref_CreateMap("tafCanInterface", MAX_CAN_INTERFACE);

    CanFramePool = le_mem_InitStaticPool(tafCanFrame, MAX_CAN_FRAME,
            sizeof(taf_canFrame_t));
    CanFrameRefMap = le_ref_CreateMap("tafCanFrame", MAX_CAN_FRAME);
    CanFrameList = LE_DLS_LIST_INIT;

    HandlerPool = le_mem_InitStaticPool(tafCanHandlerPool, MAX_CAN_HANLDER,
            sizeof(taf_CallbackHandler_t));
    HandlerRefMap = le_ref_CreateMap("tafCanHandler", MAX_CAN_HANLDER);
    CanHandlerList = LE_DLS_LIST_INIT;

    tafCanEventPool = le_mem_CreatePool("tafCanEventPool", sizeof(taf_canEvent_t));
    tafCanEvent = le_event_CreateIdWithRefCounting("tafCanEvent");
    le_event_AddHandler("tafCanCallback", tafCanEvent, HandleCanCallback);

    // Set session close handler
    le_msg_AddServiceCloseHandler(taf_can_GetServiceRef(), OnClientDisconnection, NULL);
}

/**
 * Create CAN interface
 */
taf_can_CanInterfaceRef_t taf_Can::CreateCanInf
(
    const char* infNamePtr,
    taf_can_InfProtocol_t canInfType
)
{
    LE_DEBUG("CreateCanInf");

    TAF_ERROR_IF_RET_VAL(infNamePtr == NULL, NULL, "infNamePtr is nullptr!");
    TAF_ERROR_IF_RET_VAL(canInfType != TAF_CAN_RAW_SOCK, NULL,
            "Currently only RAW socket is supported.");

    int sock = -1;
    struct ifreq ifr;
    struct sockaddr_can addr;

    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;

    if (le_utf8_Copy(ifr.ifr_name, infNamePtr, TAF_CAN_INTERFACE_NAME_MAX_LEN, NULL) != LE_OK)
    {
        LE_ERROR("Invalid lengh of interface name.");
        return NULL;
    }

    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0)
    {
        LE_ERROR("Cannot open the socket using CAN_RAW protocol. Errno: %d %s", errno,
                LE_ERRNO_TXT(errno));
        return NULL;
    }

    if(ioctl(sock, SIOCGIFINDEX, &ifr) == -1)
    {
        close(sock);
        LE_ERROR("CAN_RAW socket Cannot map CAN interface name to index. Errno: %d %s",
                errno, LE_ERRNO_TXT(errno));
        return NULL;
    }

    addr.can_ifindex = ifr.ifr_ifindex;

    int scBind = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (scBind < 0)
    {
        close(sock);
        LE_ERROR("CAN_RAW socket binding Error. Errno: %d %s", errno,
                LE_ERRNO_TXT(errno));
        return NULL;
    }

    // To disable the reception of CAN frames on the selected CAN_RAW socket
    if(setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0) < 0)
    {
        close(sock);
        LE_ERROR("Disable filter error");
        return NULL;
    }

    taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_mem_ForceAlloc(CanInfPool);
    TAF_ERROR_IF_RET_VAL(canInfCtxPtr == NULL, NULL, "cannot alloc memory for canInfCtxPtr!");

    if (le_utf8_Copy(canInfCtxPtr->infName, infNamePtr, TAF_CAN_INTERFACE_NAME_MAX_LEN, NULL)
            != LE_OK)
    {
        LE_ERROR("Invalid lengh of interface name.");
        return NULL;
    }

    canInfCtxPtr->sockFd = sock;
    canInfCtxPtr->ifNo = ifr.ifr_ifindex;
    canInfCtxPtr->canInfType = canInfType;
    canInfCtxPtr->canFdEnabled = false;
    canInfCtxPtr->loopackEnabled = true;
    canInfCtxPtr->recvOwnMsgEnabled = false;
    canInfCtxPtr->fdMonitorRef = NULL;
    canInfCtxPtr->sessionRef = taf_can_GetClientSessionRef();
    canInfCtxPtr->canInfRef = (taf_can_CanInterfaceRef_t)le_ref_CreateRef(CanInfRefMap,
            canInfCtxPtr);

    return canInfCtxPtr->canInfRef;
}

le_result_t taf_Can::SetFilter
(
    taf_can_CanInterfaceRef_t canInfRef,
    uint32_t frameId
)
{
    LE_DEBUG("SetFilter");

    taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_ref_Lookup(CanInfRefMap,
            canInfRef);
    TAF_ERROR_IF_RET_VAL(canInfCtxPtr == NULL, LE_BAD_PARAMETER,
            "Invalid CAN interface provided!");

    struct can_filter rfilter;
    int setFilter = -1;
    int sock = -1;
    sock = canInfCtxPtr->sockFd;

    if(frameId <= MAX_SFF_FRAME_ID)
    {
        rfilter.can_id   = frameId;
        rfilter.can_mask = (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_SFF_MASK);
    }
    else
    {
        rfilter.can_id   = frameId | CAN_EFF_FLAG;
        rfilter.can_mask = (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_EFF_MASK);
    }

    if(canInfCtxPtr->canInfType == TAF_CAN_RAW_SOCK)
    {
        setFilter = setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter,
                sizeof(rfilter));
    }

    if(setFilter == -1)
    {
        LE_ERROR("Failed to set CAN frame filter");
        return LE_FAULT;
    }

    return LE_OK;
}

le_result_t taf_Can::EnableLoopback
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("EnableLoop");

    taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_ref_Lookup(CanInfRefMap,
            canInfRef);
    TAF_ERROR_IF_RET_VAL(canInfCtxPtr == NULL, LE_BAD_PARAMETER,
            "Invalid CAN interface provided");

    if(canInfCtxPtr->loopackEnabled == true)
    {
        return LE_OK;
    }

    int enableLoop = -1;
    const int loopback = 1;

    int sock = -1;
    sock = canInfCtxPtr->sockFd;

    if(canInfCtxPtr->canInfType == TAF_CAN_RAW_SOCK)
    {
        enableLoop = setsockopt(sock, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback,
                sizeof(loopback));
    }

    if(enableLoop == -1)
    {
        LE_ERROR("Failed to enable local loopback");
        return LE_FAULT;
    }
    else
    {
        canInfCtxPtr->loopackEnabled = true;
        return LE_OK;
    }
}

le_result_t taf_Can::DisableLoopback
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("DisableLoop");

    taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_ref_Lookup(CanInfRefMap,
            canInfRef);
    TAF_ERROR_IF_RET_VAL(canInfCtxPtr == NULL, LE_BAD_PARAMETER,
            "Invalid CAN interface provided");

    if(canInfCtxPtr->loopackEnabled == false)
    {
        return LE_OK;
    }

    int disableLoop = -1;
    const int loopback = 0;

    int sock = -1;
    sock = canInfCtxPtr->sockFd;

    if(canInfCtxPtr->canInfType == TAF_CAN_RAW_SOCK)
    {
        disableLoop = setsockopt(sock, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback,
                sizeof(loopback));
    }

    if(disableLoop == -1)
    {
        LE_ERROR("Failed to disable local loopback");
        return LE_FAULT;
    }
    else
    {
        canInfCtxPtr->loopackEnabled = false;
        return LE_OK;
    }
}

le_result_t taf_Can::EnableRcvOwnMsg
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("EnableRcvOwnMsg");

    taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_ref_Lookup(CanInfRefMap,
            canInfRef);
    TAF_ERROR_IF_RET_VAL(canInfCtxPtr == NULL, LE_BAD_PARAMETER,
            "Invalid CAN interface provided");

    if(canInfCtxPtr->recvOwnMsgEnabled == true)
    {
        return LE_OK;
    }

    int enableRcvMsg = -1;
    const int recvOwnMsgs = 1;

    int sock = -1;
    sock = canInfCtxPtr->sockFd;

    if(canInfCtxPtr->canInfType == TAF_CAN_RAW_SOCK)
    {
        enableRcvMsg = setsockopt(sock, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recvOwnMsgs,
                sizeof(recvOwnMsgs));
    }

    if(enableRcvMsg == -1)
    {
        LE_ERROR("Failed to enable receive own message");
        return LE_FAULT;
    }
    else
    {
        canInfCtxPtr->recvOwnMsgEnabled = true;
        return LE_OK;
    }
}

le_result_t taf_Can::DisableRcvOwnMsg
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("DisableRcvOwnMsg");

    taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_ref_Lookup(CanInfRefMap,
            canInfRef);
    TAF_ERROR_IF_RET_VAL(canInfCtxPtr == NULL, LE_BAD_PARAMETER,
            "Invalid CAN interface provided");

    if(canInfCtxPtr->recvOwnMsgEnabled == false)
    {
        return LE_OK;
    }

    int disableRcvMsg = -1;
    const int recvOwnMsgs = 0;

    int sock = -1;
    sock = canInfCtxPtr->sockFd;

    if(canInfCtxPtr->canInfType == TAF_CAN_RAW_SOCK)
    {
        disableRcvMsg = setsockopt(sock, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recvOwnMsgs,
                sizeof(recvOwnMsgs));
    }

    if(disableRcvMsg == -1)
    {
        LE_ERROR("Failed to disable receive own message");
        return LE_FAULT;
    }
    else
    {
        canInfCtxPtr->recvOwnMsgEnabled = false;
        return LE_OK;
    }
}

bool taf_Can::IsFdSupported
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("IsFdSupported");

    taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_ref_Lookup(CanInfRefMap,
            canInfRef);
    TAF_ERROR_IF_RET_VAL(canInfCtxPtr == NULL, false, "Invalid CAN interface provided");

    int sock = -1;
    int mtu = -1;
    struct ifreq ifr;
    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;

    sock = canInfCtxPtr->sockFd;
    ifr.ifr_ifindex = canInfCtxPtr->ifNo;
    addr.can_ifindex = ifr.ifr_ifindex;

    ioctl(sock, SIOCGIFNAME, &ifr);

    if(canInfCtxPtr->canInfType == TAF_CAN_RAW_SOCK)
    {
        if(ioctl(sock, SIOCGIFMTU, &ifr) == -1)
        {
            LE_ERROR("Cannot get MTU of a device. Errno: %d %s", errno,
                    LE_ERRNO_TXT(errno));
            return false;
        }

        mtu = ifr.ifr_mtu;
    }

    if(mtu == CANFD_MTU)
    {
        LE_INFO("Device support CAN Fd-frame");
        return true;
    }
    else
    {
        LE_INFO("Device does not support CAN Fd-frame");
        return false;
    }

}

le_result_t taf_Can::EnableFdFrame
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("EnableFdFrame");

    taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_ref_Lookup(CanInfRefMap,
            canInfRef);
    TAF_ERROR_IF_RET_VAL(canInfCtxPtr == NULL, LE_BAD_PARAMETER,
            "Invalid CAN interface provided");

    if (canInfCtxPtr->canFdEnabled == true)
    {
        return LE_OK;
    }

    int rc = -1;
    int sock = -1;
    const int enableCanFd = 1;

    sock = canInfCtxPtr->sockFd;

    if(canInfCtxPtr->canInfType == TAF_CAN_RAW_SOCK)
    {
        if(IsFdSupported(canInfRef) == true)
        {
            rc = setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enableCanFd,
                    sizeof(enableCanFd));
        }
        else if(IsFdSupported(canInfRef) == false)
        {
            LE_ERROR("Since Fd frame is not supported so, no need to alloc CAN fd frame.");
            return LE_UNSUPPORTED;
        }
    }

    if(rc == -1)
    {
        LE_ERROR("CAN raw layer failed to alloc can fd");
        canInfCtxPtr->canFdEnabled = false;
        return LE_FAULT;
    }
    else
    {
        canInfCtxPtr->canFdEnabled = true;
        return LE_OK;
    }
}

bool taf_Can::GetFdStatus
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("IsFdSupported");

    taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_ref_Lookup(CanInfRefMap,
            canInfRef);
    TAF_ERROR_IF_RET_VAL(canInfCtxPtr == NULL, false, "Invalid CAN interface provided");

    if (canInfCtxPtr->canFdEnabled == true)
    {
        LE_INFO("FD farme is enabled");
        return true;
    }
    else
    {
        LE_INFO("FD farme is not enable");
        return false;
    }
}

le_result_t taf_Can::createFdMonitor
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("createFdMonitor");

    taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_ref_Lookup(CanInfRefMap,
            canInfRef);
    TAF_ERROR_IF_RET_VAL(canInfCtxPtr == NULL, LE_BAD_PARAMETER,
            "Invalid CAN interface provided");

    if (canInfCtxPtr->fdMonitorRef != NULL)
    {
        return LE_DUPLICATE;
    }

    int sock = -1;
    scAddr.can_family  = AF_CAN;

    sock = canInfCtxPtr->sockFd;
    scAddr.can_ifindex = canInfCtxPtr->ifNo;

    // Tell the RAW layer to alloc can fd, if supported.
    if(canInfCtxPtr->canFdEnabled == true)
    {
        LE_INFO("CAN FD frame layer allocated");
    }
    else if(canInfCtxPtr->canFdEnabled == false)
    {
        if(IsFdSupported(canInfRef) == true)
        {
            if (EnableFdFrame(canInfRef) == LE_OK)
            {
                LE_INFO("CAN FD frame layer allocated");
            }
        }
        else
        {
            LE_INFO("Since FD frame is not supported so, only Non-fd frame type will receive");
        }
    }

    if(canInfCtxPtr->canInfType == TAF_CAN_RAW_SOCK)
    {
        canInfCtxPtr->fdMonitorRef = le_fdMonitor_Create("CanSockMonitor", sock,
                CanRawSockHandler, POLLIN);
        le_fdMonitor_SetContextPtr(canInfCtxPtr->fdMonitorRef, canInfRef);
        LE_DEBUG("FdMonitor is created");
    }

    return LE_OK;
}

void taf_Can::CanRawSockHandler(int sock, short events)
{
    LE_FATAL_IF(sock < 0, "Socket Error.");

    taf_can_CanInterfaceRef_t canInfRef  = (taf_can_CanInterfaceRef_t)le_fdMonitor_GetContextPtr();

    if (events & POLLIN)
    {
        struct canfd_frame scFrame;
        struct ifreq ifr;
        memset(&scFrame, 0, sizeof(canfd_frame));

        struct iovec ioVec;
        struct msghdr message;
        ioVec.iov_base = &scFrame;
        message.msg_name = &scAddr;
        message.msg_iov = &ioVec;
        message.msg_iovlen = 1;

        ioVec.iov_len = sizeof(scFrame);
        message.msg_namelen = sizeof(scAddr);
        message.msg_flags = 0;

        int receiveFrame = recvmsg(sock, &message, 0);
        if (receiveFrame < 0)
        {
            LE_ERROR("receive failed. Errno: %d %s", errno, LE_ERRNO_TXT(errno));
        }
        LE_INFO("Receive msg frame %d", receiveFrame);

        // get name and index number of the received CAN frame interface
        ifr.ifr_ifindex = scAddr.can_ifindex;
        ioctl(sock, SIOCGIFNAME, &ifr);
        LE_INFO("Received CAN frame from interface name %s", ifr.ifr_name);

        ioctl(sock, SIOCGIFINDEX, &ifr);
        LE_INFO("Received CAN frame from interface number %d", ifr.ifr_ifindex);

        taf_Can can = GetInstance();

        taf_canEvent_t* canEventPtr = (taf_canEvent_t *)le_mem_ForceAlloc(can.tafCanEventPool);
        TAF_ERROR_IF_RET_NIL(canEventPtr == NULL, "cannot alloc memory for event!");

        canEventPtr->canInfRef = canInfRef;
        canEventPtr->receiveFrame = receiveFrame;
        canEventPtr->frame = scFrame;

        le_event_ReportWithRefCounting(can.tafCanEvent, canEventPtr);
    }
}

taf_can_CanEventHandlerRef_t taf_Can::AddCanEventHandler
(
    taf_can_CanInterfaceRef_t canInfRef,
    uint32_t frameId,
    uint32_t frIdMask,
    taf_can_CallbackFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_DEBUG("AddCanEventHandler");

    taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_ref_Lookup(CanInfRefMap,
            canInfRef);
    TAF_ERROR_IF_RET_VAL(canInfCtxPtr == NULL, NULL, "Invalid CAN interface provided");

    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    if(canInfCtxPtr->fdMonitorRef == NULL)
    {
        if(createFdMonitor(canInfRef) != LE_OK)
        {
            LE_ERROR("FdMonitor is not created");
            return NULL;
        }
    }

    // Setting hardware filter
    struct canHwFilter canHwFilter;
    struct ifreq ifrHw;

    char ch;
    size_t size = strlen(canInfCtxPtr->infName);
    uint8_t digit, infSuffix = 0;

    for(size_t i=0; i<size; i++)
    {
        ch = canInfCtxPtr->infName[i];
        if(ch >= '0' && ch <= '9')
        {
            digit = ch - '0';
            infSuffix = infSuffix*10 + digit;
        }
    }

    le_utf8_Copy(ifrHw.ifr_name, canInfCtxPtr->infName, TAF_CAN_INTERFACE_NAME_MAX_LEN, NULL);

    canHwFilter.ifNo = infSuffix;
    canHwFilter.frameId = frameId;
    canHwFilter.frIdMask = frIdMask | CAN_RTR_FLAG;
    ifrHw.ifr_data = (char *)&canHwFilter;

    LE_DEBUG("IfaceNo:%i, frameId:0x%x, mask:0x%x", infSuffix, canHwFilter.frameId,
            canHwFilter.frIdMask);

    int rc = -1;
    rc = ioctl(canInfCtxPtr->sockFd, IOCTL_ADD_FRAME_FILTER, &ifrHw);
    if (rc != 0)
    {
        LE_ERROR("FrameFilter ioctl for ifNo %d returned error: %d %s\n",
                ifrHw.ifr_ifindex, rc, strerror(errno));
    }
    else
    {
        LE_INFO("HW filter is set");
    }

    // Store the callback function and context pointer
    taf_CallbackHandler_t* handlerCtxPtr = (taf_CallbackHandler_t *)le_mem_ForceAlloc(HandlerPool);
    TAF_ERROR_IF_RET_VAL(handlerCtxPtr == NULL, NULL, "cannot alloc memory for handler context!");

    handlerCtxPtr->frameId = frameId;
    handlerCtxPtr->frIdMask = frIdMask;
    handlerCtxPtr->canInfRef = canInfRef;
    handlerCtxPtr->handlerPtr = handlerPtr;
    handlerCtxPtr->contextPtr = contextPtr;
    handlerCtxPtr->handlerRef = (taf_can_CanEventHandlerRef_t)le_ref_CreateRef(HandlerRefMap,
            handlerCtxPtr);
    handlerCtxPtr->link = LE_DLS_LINK_INIT;

    le_dls_Queue(&CanHandlerList, &handlerCtxPtr->link);

    return handlerCtxPtr->handlerRef;
}

void taf_Can::HandleCanCallback(void* reportPtr)
{
    LE_DEBUG("HandleCanCallback");

    taf_Can can = GetInstance();

    taf_canEvent_t* eventPtr = (taf_canEvent_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(eventPtr == NULL, "eventPtr is Null");

    bool isCanFdFrame = false;
    if (eventPtr->receiveFrame == CANFD_MTU)
    {
        isCanFdFrame = true;
    }
    else if (eventPtr->receiveFrame == CAN_MTU)
    {
        isCanFdFrame = false;
    }

    uint32_t frameId = eventPtr->frame.can_id;
    int8_t datalen = eventPtr->frame.len;
    uint8_t data[datalen];
    memcpy(data, eventPtr->frame.data, datalen);

    // Notify the repective callbackFunc
    le_dls_Link_t* linkHandlerPtr = NULL;
    linkHandlerPtr = le_dls_PeekTail(&can.CanHandlerList);
    while(linkHandlerPtr)
    {
        taf_CallbackHandler_t * handlerCtxPtr = CONTAINER_OF(linkHandlerPtr,
                taf_CallbackHandler_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&can.CanHandlerList, linkHandlerPtr);
        if(handlerCtxPtr->handlerPtr && handlerCtxPtr->canInfRef == eventPtr->canInfRef)
        {
            if((handlerCtxPtr->frameId & handlerCtxPtr->frIdMask)
                    == (frameId & handlerCtxPtr->frIdMask))
            {
                handlerCtxPtr->handlerPtr(eventPtr->canInfRef ,isCanFdFrame, frameId, data, datalen,
                        handlerCtxPtr->contextPtr);
                LE_INFO("Received frameType: %s, FrameId: 0x%03X, CAN interface reference: 0x%p",
                        (isCanFdFrame ? "CAN FD frames":"CAN 2.0 frames"), frameId,
                                eventPtr->canInfRef);
            }
        }
    }
    le_mem_Release(eventPtr);
}

void taf_Can::RemoveCanEventHandler
(
    taf_can_CanEventHandlerRef_t handlerRef
)
{
    LE_DEBUG("RemoveCanEventHandler");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");

    // Unregister CAN callback and remove the respective handlerRef from canHandlerList
    le_dls_Link_t* linkHandlerPtr = le_dls_PeekTail(&CanHandlerList);
    while (linkHandlerPtr)
    {
        taf_CallbackHandler_t* handlerCtxPtr = CONTAINER_OF(linkHandlerPtr,
                taf_CallbackHandler_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&CanHandlerList, linkHandlerPtr);

        if (handlerCtxPtr && handlerCtxPtr->handlerRef == handlerRef)
        {
            LE_DEBUG("Removed handlerRef: 0x%p).", handlerRef);

            le_ref_DeleteRef(HandlerRefMap, handlerRef);
            le_dls_Remove(&CanHandlerList, &(handlerCtxPtr->link));
            le_mem_Release((void*)handlerCtxPtr);
            break;
        }
    }
}

/**
 * Create CAN frame
 */
taf_can_CanFrameRef_t taf_Can::CreateCanFrame
(
    taf_can_CanInterfaceRef_t canInfRef,
    uint32_t                  frameId
)
{
    LE_DEBUG("CreateCanFrame");

    taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_ref_Lookup(CanInfRefMap,
            canInfRef);
    TAF_ERROR_IF_RET_VAL(canInfCtxPtr == NULL, NULL, "Invalid CAN interface provided");

    taf_canFrame_t* canFrameCtxPtr = (taf_canFrame_t *)le_mem_ForceAlloc(CanFramePool);
    TAF_ERROR_IF_RET_VAL(canFrameCtxPtr == NULL, NULL, "Invalid CAN Frame provided!");

    if(frameId <= MAX_SFF_FRAME_ID)
    {
        canFrameCtxPtr->frameId = frameId;
    }
    else
    {
        canFrameCtxPtr->frameId = frameId | CAN_EFF_FLAG;
    }

    canFrameCtxPtr->canInfRef = canInfRef;
    canFrameCtxPtr->frameType = (taf_can_FrameType_t)-1;
    canFrameCtxPtr->canFrameLink = LE_DLS_LINK_INIT;
    canFrameCtxPtr->canFrameRef = (taf_can_CanFrameRef_t)le_ref_CreateRef(CanFrameRefMap,
            canFrameCtxPtr);
    le_dls_Queue(&CanFrameList, &canFrameCtxPtr->canFrameLink);

    return canFrameCtxPtr->canFrameRef;
}

le_result_t taf_Can::SetPayload
(
    taf_can_CanFrameRef_t frameRef,
    const uint8_t*        dataPtr,
    size_t                datalen
)
{
    LE_DEBUG("SetPayload");

    taf_canFrame_t* canFrameCtxPtr = (taf_canFrame_t *)le_ref_Lookup(CanFrameRefMap, frameRef);
    TAF_ERROR_IF_RET_VAL(canFrameCtxPtr == NULL, LE_BAD_PARAMETER,
            "Invalid CAN frame provided");

    TAF_ERROR_IF_RET_VAL(dataPtr == NULL, LE_BAD_PARAMETER,"dataPtr is nullptr!");

    memcpy(canFrameCtxPtr->dataPtr, dataPtr, datalen);
    canFrameCtxPtr->dataLen = datalen;

    return LE_OK;
}

le_result_t taf_Can::SetFrameType
(
    taf_can_CanFrameRef_t frameRef,
    taf_can_FrameType_t frameType
)
{
    LE_DEBUG("SetRcvOwnMsg");

    taf_canFrame_t* canFrameCtxPtr = (taf_canFrame_t *)le_ref_Lookup(CanFrameRefMap, frameRef);
    TAF_ERROR_IF_RET_VAL(canFrameCtxPtr == NULL, LE_BAD_PARAMETER,
            "Invalid CAN frame provided");

    canFrameCtxPtr->frameType = frameType;

    return LE_OK;
}

le_result_t taf_Can::SendFrame
(
    taf_can_CanFrameRef_t frameRef
)
{
    LE_DEBUG("SendFrame");

    taf_canFrame_t* canFrameCtxPtr = (taf_canFrame_t *)le_ref_Lookup(CanFrameRefMap, frameRef);
    TAF_ERROR_IF_RET_VAL(canFrameCtxPtr == NULL, LE_BAD_PARAMETER,
            "Invalid CAN frame provided");

    taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_ref_Lookup(CanInfRefMap,
            canFrameCtxPtr->canInfRef);
    TAF_ERROR_IF_RET_VAL(canInfCtxPtr == NULL, LE_BAD_PARAMETER, "Invalid CAN interface!");

    if(canFrameCtxPtr->dataLen <= 0)
    {
        LE_ERROR("Data is not set, Set data first!");
        return LE_FAULT;
    }

    int send;
    taf_can_FrameType_t frameType;

    struct can_frame nFrame;
    struct canfd_frame pFrame;

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = canInfCtxPtr->ifNo;

    int sock = -1;
    sock = canInfCtxPtr->sockFd;
    frameType = canFrameCtxPtr->frameType;

    if(frameType == -1)
    {
        LE_INFO("Frame type is not set. So, data will send as can 2.0 frame by default");
        frameType = TAF_CAN_CAN_FRAME;
    }

    switch(frameType)
    {
        case TAF_CAN_CAN_FRAME:
        {
            if(canFrameCtxPtr->dataLen > CAN_MAX_DLEN)
            {
                LE_ERROR("CAN 2.0 does not support datalength more than 8 bytes!");
                return LE_FAULT;
            }

            nFrame.can_id = canFrameCtxPtr->frameId;
            nFrame.can_dlc = canFrameCtxPtr->dataLen;
            memcpy(nFrame.data, canFrameCtxPtr->dataPtr, nFrame.can_dlc);

            send = sendto(sock, &nFrame, sizeof(struct can_frame), 0,
                    (struct sockaddr*)&addr, sizeof(addr));
        }
        break;

        case TAF_CAN_CAN_FD_FRAME:
        {
            if (canFrameCtxPtr->dataLen > CANFD_MAX_DLEN)
            {
                LE_ERROR("CAN FD does not support data length more than 64 bytes.");
                return LE_FAULT;
            }

            if(canInfCtxPtr->canFdEnabled == true)
            {
                LE_INFO("FD frame is enabled");
            }
            else if(EnableFdFrame(canFrameCtxPtr->canInfRef) == LE_OK)
            {
                LE_INFO("FD frame is enable");
            }
            else
            {
                LE_ERROR("FD frame type can not send because FD Frame is not supported");
                return LE_FAULT;
            }

            pFrame.can_id = canFrameCtxPtr->frameId;
            pFrame.len = canFrameCtxPtr->dataLen;
            memcpy(pFrame.data, canFrameCtxPtr->dataPtr, pFrame.len);

            send = sendto(sock, &pFrame, sizeof(struct canfd_frame), 0,
                    (struct sockaddr*)&addr, sizeof(addr));
        }
        break;

        case TAF_CAN_AUTO_FRAME:
        {
            if(canInfCtxPtr->canFdEnabled == true)
            {
                if (canFrameCtxPtr->dataLen > CANFD_MAX_DLEN)
                {
                    LE_ERROR("CAN FD does not support data length more than 64 bytes.");
                    return LE_FAULT;
                }

                pFrame.can_id = canFrameCtxPtr->frameId;
                pFrame.len = canFrameCtxPtr->dataLen;
                memcpy(pFrame.data, canFrameCtxPtr->dataPtr, pFrame.len);

                send = sendto(sock, &pFrame, sizeof(struct canfd_frame), 0,
                        (struct sockaddr*)&addr, sizeof(addr));
            }
            else if (EnableFdFrame(canFrameCtxPtr->canInfRef) == LE_OK)
            {
                if (canFrameCtxPtr->dataLen > CANFD_MAX_DLEN)
                {
                    LE_ERROR("CAN FD does not support data length more than 64 bytes.");
                    return LE_FAULT;
                }

                pFrame.can_id = canFrameCtxPtr->frameId;
                pFrame.len = canFrameCtxPtr->dataLen;
                memcpy(pFrame.data, canFrameCtxPtr->dataPtr, pFrame.len);

                send = sendto(sock, &pFrame, sizeof(struct canfd_frame), 0,
                        (struct sockaddr*)&addr, sizeof(addr));
            }
            else
            {
                if(canFrameCtxPtr->dataLen > CAN_MAX_DLEN)
                {
                    LE_ERROR("CAN 2.0 does not support data length more than 8 bytes!");
                    return LE_FAULT;
                }

                nFrame.can_id = canFrameCtxPtr->frameId;
                nFrame.can_dlc = canFrameCtxPtr->dataLen;
                memcpy(nFrame.data, canFrameCtxPtr->dataPtr, nFrame.can_dlc);

                send = sendto(sock, &nFrame, sizeof(struct can_frame), 0,
                        (struct sockaddr*)&addr, sizeof(addr));
            }
        }
        break;
    }

    if (send < 0)
    {
        LE_ERROR("send failed. Errno: %d %s", errno, LE_ERRNO_TXT(errno));
        return LE_FAULT;
    }
    LE_DEBUG("send: %d", send);

    return LE_OK;
}

le_result_t taf_Can::DeleteCanInf
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("DeleteCanInf");

    taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_ref_Lookup(CanInfRefMap,
            canInfRef);
    TAF_ERROR_IF_RET_VAL(canInfCtxPtr == NULL, LE_BAD_PARAMETER,
            "Invalid CAN interface provided!");

    int sock = -1;
    sock = canInfCtxPtr->sockFd;

    // Release all the allocated memory of canFrameCtxPtr for the given CAN interface reference
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_PeekTail(&CanFrameList);
    while (linkPtr)
    {
        taf_canFrame_t* canFrameCtxPtr = CONTAINER_OF(linkPtr,
                taf_canFrame_t, canFrameLink);
        linkPtr = le_dls_PeekPrev(&CanFrameList, linkPtr);
        if (canFrameCtxPtr && canFrameCtxPtr->canInfRef == canInfRef)
        {
            le_ref_DeleteRef(CanFrameRefMap, canFrameCtxPtr->canFrameRef);
            le_dls_Remove(&CanFrameList, &(canFrameCtxPtr->canFrameLink));
            le_mem_Release(canFrameCtxPtr);
            break;
        }
    }

    // Delete the created Fd-monitor for the given CAN interface reference
    if(canInfCtxPtr->fdMonitorRef != NULL)
    {
        LE_INFO("Deleting fd monitor");
        const int fd = le_fdMonitor_GetFd(canInfCtxPtr->fdMonitorRef);

        LE_DEBUG("Delete fdMonitorRef 0x%p",canInfCtxPtr->fdMonitorRef);
        le_fdMonitor_Delete(canInfCtxPtr->fdMonitorRef);

        canInfCtxPtr->fdMonitorRef = NULL;

        const int ret = close(fd);
        LE_WARN_IF(ret == -1, "Failed to close file descriptor");
    }

    // Release all the allocated memory of handlerCtxPtr for the given CAN interface reference
    le_dls_Link_t* linkHandlerPtr = NULL;
    linkHandlerPtr = le_dls_PeekTail(&CanHandlerList);
    while (linkHandlerPtr)
    {
        taf_CallbackHandler_t* handlerCtxPtr = CONTAINER_OF(linkHandlerPtr,
                taf_CallbackHandler_t, link);
        linkHandlerPtr = le_dls_PeekPrev(&CanHandlerList, linkHandlerPtr);

        if(handlerCtxPtr && handlerCtxPtr->canInfRef == canInfRef)
        {
            LE_DEBUG("Removed handlerRef: 0x%p).", handlerCtxPtr->handlerRef);

            le_ref_DeleteRef(HandlerRefMap, handlerCtxPtr->handlerRef);
            le_dls_Remove(&CanHandlerList, &(handlerCtxPtr->link));
            le_mem_Release(handlerCtxPtr);
            break;
        }
    }

    close(sock);  //closing the socket

    // Deletet the created CAN interface
    le_ref_DeleteRef(CanInfRefMap, canInfRef);
    le_mem_Release(canInfCtxPtr);

    return LE_OK;
}

le_result_t taf_Can::DeleteCanFrame
(
    taf_can_CanFrameRef_t frameRef
)
{
    LE_DEBUG("DeleteCanFrame");

    taf_canFrame_t* canFrameCtxPtr = (taf_canFrame_t *)le_ref_Lookup(CanFrameRefMap, frameRef);
    TAF_ERROR_IF_RET_VAL(canFrameCtxPtr == NULL, LE_BAD_PARAMETER,
        "Invalid CAN frame provided");

    if(canFrameCtxPtr->canFrameRef != frameRef)
    {
        LE_FATAL("Error: Not valid CAN frame %p.", frameRef);
    }

    le_ref_DeleteRef(CanFrameRefMap, frameRef);
    le_dls_Remove(&CanFrameList, &(canFrameCtxPtr->canFrameLink));
    le_mem_Release(canFrameCtxPtr);

    return LE_OK;
}

/**
* Callback on client disconnection
*/
void taf_Can::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    LE_DEBUG("OnClientDisconnection");

    TAF_ERROR_IF_RET_NIL(sessionRef == NULL, "sessionRef is NULL");

    auto &can = taf_Can::GetInstance();

    le_ref_IterRef_t iterRef = le_ref_GetIterator(can.CanInfRefMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        taf_canInterface_t* canInfCtxPtr = (taf_canInterface_t *)le_ref_GetValue(iterRef);
        LE_ASSERT(canInfCtxPtr != NULL);

        if (canInfCtxPtr->sessionRef == sessionRef)
        {
            taf_can_CanInterfaceRef_t safeRef =
                    (taf_can_CanInterfaceRef_t)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release taf_can_CanInterfaceRef_t 0x%p, Session 0x%p",
                    safeRef, sessionRef);

            le_result_t res = taf_can_DeleteCanInf(safeRef);
            if (res == LE_OK)
            {
                LE_INFO("Session deleted successfully on client disconnection");
            }
            else
            {
                LE_ERROR("Session Delete Error on client disconnection");
            }
        }

        result = le_ref_NextNode(iterRef);
    }
}