/*
 *  Copyright (c) 2022, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafVNWSvc.cpp
 * @brief      This file provides the taf CAN manager service as interfaces described
 *             in taf_can.api. The CAN service will be started automatically.
 */

#include "tafCAN.hpp"

using namespace tafsvc;

COMPONENT_INIT
{
    auto &can = taf_Can::GetInstance();
    can.Init();
    LE_INFO("CAN service init completed ...");
}

/**
 * FUNCTION     : CreateCanInf
 * DESCRIPTION  : Create CAN interface
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: CAN interface reference
 */
taf_can_CanInterfaceRef_t taf_can_CreateCanInf
(
    const char* infName,
    taf_can_InfProtocol_t canInfType
)
{
    LE_DEBUG("taf_can_CreateCanInf");
    auto &can = taf_Can::GetInstance();
    return can.CreateCanInf(infName, canInfType);
}

/**
 * FUNCTION     : SetFilter
 * DESCRIPTION  : Set filter to receive CAN frame for the given frameId
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: LE_OK: If the receive filter is set
                  LE_FAULT: For other errors
 */
le_result_t taf_can_SetFilter
(
    taf_can_CanInterfaceRef_t canInfRef,
    uint32_t frameId
)
{
    LE_DEBUG("taf_can_SetFilter");
    auto &can = taf_Can::GetInstance();
    return can.SetFilter(canInfRef, frameId);
}

/**
 * FUNCTION     : EnableLoopback
 * DESCRIPTION  : Enables sending messages to loopback from the given CAN interface (can0/can1/..)
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: LE_OK on success
                  LE_FAULT for all errors
 */
le_result_t taf_can_EnableLoopback
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("taf_can_EnableLoop");
    auto &can = taf_Can::GetInstance();
    return can.EnableLoopback(canInfRef);
}

/**
 * FUNCTION     : DisableLoopback
 * DESCRIPTION  : Disables sending messages to loopback from the given CAN interface (can0/can1/..)
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: LE_OK on success
                  LE_FAULT for all errors
 */
le_result_t taf_can_DisableLoopback
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("taf_can_EnableLoop");
    auto &can = taf_Can::GetInstance();
    return can.DisableLoopback(canInfRef);
}

/**
 * FUNCTION     : EnableRcvOwnMsg
 * DESCRIPTION  : To receive messages sent from the same CAN socket
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: LE_OK on success
                  LE_FAULT for all errors
 */
le_result_t taf_can_EnableRcvOwnMsg
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("taf_can_EnableRcvOwnMsg");
    auto &can = taf_Can::GetInstance();
    return can.EnableRcvOwnMsg(canInfRef);
}

/**
 * FUNCTION     : DisableRcvOwnMsg
 * DESCRIPTION  : To not receive messages sent from the same CAN socket
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: LE_OK on success
                  LE_FAULT for all errors
 */
le_result_t taf_can_DisableRcvOwnMsg
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("taf_can_DisableRcvOwnMsg");
    auto &can = taf_Can::GetInstance();
    return can.DisableRcvOwnMsg(canInfRef);
}

/**
 * FUNCTION     : IsFdSupported
 * DESCRIPTION  : Device support Fd Frame or not
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: TRUE: FD supported
                  FALSE: FD not supported
 */
bool taf_can_IsFdSupported
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("taf_can_IsFdSupported");
    auto &can = taf_Can::GetInstance();
    return can.IsFdSupported(canInfRef);
}

/**
 * FUNCTION     : EnableFdFrame
 * DESCRIPTION  : Enable CAN-FD frame
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: LE_OK on success
                  LE_FAULT for all errors
 */
le_result_t taf_can_EnableFdFrame
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("taf_can_EnableFdFrame");
    auto &can = taf_Can::GetInstance();
    return can.EnableFdFrame(canInfRef);
}

/**
 * FUNCTION     : GetFdStatus
 * DESCRIPTION  : FD frame is enabled or not
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: TRUE: FD Frame enabled
                  FALSE: FD Frame not enabled
 */
bool taf_can_GetFdStatus
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("taf_can_GetFdStatus");
    auto &can = taf_Can::GetInstance();
    return can.GetFdStatus(canInfRef);
}

/**
 * FUNCTION     : AddCanEventHandler
 * DESCRIPTION  : Register the callback function when msg is received in the registered frameId
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: handlerRef used to unregister the requested Id in RemoveCanEventHandler
 */
taf_can_CanEventHandlerRef_t taf_can_AddCanEventHandler
(
    taf_can_CanInterfaceRef_t canInfRef,
    uint32_t                  frameId,
    uint32_t                  frIdMask,
    taf_can_CallbackFunc_t    handlerPtr,
    void*                     contextPtr
)
{
    LE_INFO("taf_can_AddCanEventHandler");
    auto &can = taf_Can::GetInstance();
    return can.AddCanEventHandler(canInfRef, frameId, frIdMask, handlerPtr, contextPtr);
}

/**
 * FUNCTION     : RemoveCanEventHandler
 * DESCRIPTION  : It stops notifying when the msg is received for particular frameId.
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES:
 */
void taf_can_RemoveCanEventHandler
(
    taf_can_CanEventHandlerRef_t ref_t
)
{
    LE_INFO("taf_can_RemoveCanEventHandler");
    auto &can = taf_Can::GetInstance();
    return can.RemoveCanEventHandler(ref_t);
}

/**
 * FUNCTION     : CreateCanFrame
 * DESCRIPTION  : Create CAN frame
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: CAN frame reference
 */
taf_can_CanFrameRef_t taf_can_CreateCanFrame
(
    taf_can_CanInterfaceRef_t canInfRef,
    uint32_t                  frameId
)
{
    LE_DEBUG("taf_can_CreateCanFrame");
    auto &can = taf_Can::GetInstance();
    return can.CreateCanFrame(canInfRef, frameId);
}

/**
 * FUNCTION     : SetPayload
 * DESCRIPTION  : Set data Payload
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: LE_OK on success
                  LE_FAULT for all errors
 */
le_result_t taf_can_SetPayload
(
    taf_can_CanFrameRef_t frameRef,
    const uint8_t*        dataPtr,
    size_t                datalen
)
{
    LE_DEBUG("taf_can_SetPayload");
    auto &can = taf_Can::GetInstance();
    return can.SetPayload(frameRef, dataPtr, datalen);
}

/**
 * FUNCTION     : SetFrameType
 * DESCRIPTION  : Set frame type to be send
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: LE_OK on success
                  LE_FAULT for all errors
 */
le_result_t taf_can_SetFrameType
(
    taf_can_CanFrameRef_t frameRef,
    taf_can_FrameType_t frameType
)
{
    LE_DEBUG("taf_can_SetFrameType");
    auto &can = taf_Can::GetInstance();
    return can.SetFrameType(frameRef, frameType);
}

/**
 * FUNCTION     : SendFrame
 * DESCRIPTION  : send CAN frame
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: LE_OK on success
                  LE_FAULT for all errors
 */
le_result_t taf_can_SendFrame
(
    taf_can_CanFrameRef_t frameRef
)
{
    LE_DEBUG("SendFrame");
    auto &can = taf_Can::GetInstance();
    return can.SendFrame(frameRef);
}

/**
 * FUNCTION     : DeleteCanInf
 * DESCRIPTION  : Delete CAN Interface
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: LE_OK on success
 */
le_result_t taf_can_DeleteCanInf
(
    taf_can_CanInterfaceRef_t canInfRef
)
{
    LE_DEBUG("taf_can_DeleteCanInf");
    auto &can = taf_Can::GetInstance();
    return can.DeleteCanInf(canInfRef);
}

/**
 * FUNCTION     : DeleteCanFrame
 * DESCRIPTION  : Delete CAN Frame
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: LE_OK on success
 */
le_result_t taf_can_DeleteCanFrame
(
    taf_can_CanFrameRef_t frameRef
)
{
    LE_DEBUG("taf_can_DeleteCanFrame");
    auto &can = taf_Can::GetInstance();
    return can.DeleteCanFrame(frameRef);
}
