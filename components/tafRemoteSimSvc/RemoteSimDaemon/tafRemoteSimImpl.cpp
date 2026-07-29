/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafRemoteSim.hpp"

using namespace tafsvc;

LE_MEM_DEFINE_STATIC_POOL(RsimMsgs, MSG_POOL_SIZE, sizeof(taf_RsimMsg_Client_t));
LE_MEM_DEFINE_STATIC_POOL(RsimApduTx, APDU_TX_POOL_SIZE, sizeof(taf_RsimApduTx_t));

// PA Event Listener Callbacks
static void onApduTransferCallback(const std::shared_ptr<taf_pa_remotesim_ApduTransfer_t>& apduEvent) {
    LE_INFO("Received Apdu transfer notification from modem.\n");
    auto &rSim = taf_simRsim::GetInstance();
    rSim.SendApduRequest(apduEvent->id, apduEvent->apdu);
}

static void onCardConnectCallback() {
    LE_INFO("Received Card Connect notification from modem.\n");
    auto &rSim = taf_simRsim::GetInstance();
    rSim.SendCardConnectRequest();
}

static void onCardDisconnectCallback() {
    LE_INFO("Received Card Disconnect notification from modem.\n");
    auto &rSim = taf_simRsim::GetInstance();
    rSim.SendCardDisconnectRequest();
}

static void onCardPowerUpCallback() {
    LE_INFO("Received Card Power Up notification from modem.\n");
    auto &rSim = taf_simRsim::GetInstance();
    rSim.SendCardPowerUpRequest();
}

static void onCardPowerDownCallback() {
    LE_INFO("Received Card Power Down notification from modem.\n");
    auto &rSim = taf_simRsim::GetInstance();
    rSim.SendCardPowerDownRequest();
}

static void onCardResetCallback() {
    LE_INFO("Received Card Reset notification from modem.\n");
    auto &rSim = taf_simRsim::GetInstance();
    rSim.SendCardResetRequest();
}

static void onServiceStatusChangeCallback(const std::shared_ptr<taf_pa_remotesim_ServiceStatus_t>& statusEvent) {
    auto &rSim = taf_simRsim::GetInstance();
    if (statusEvent->status == TAF_PA_REMOTESIM_SERVICE_UNAVAILABLE) {
        LE_INFO("Received Service Unavailable notification.\n");
        rSim.NotifyConnectionUnavailable();
    } else if (statusEvent->status == TAF_PA_REMOTESIM_SERVICE_AVAILABLE) {
        LE_INFO("Received Service Available notification.\n");
        rSim.NotifyConnectionAvailable();
    } else {
        LE_INFO("Received unknown service status notification.\n");
    }
}

void taf_simRsim:: Init(void) {
    MainThread = le_thread_GetCurrent();

    pa_result_t result = taf_pa_remotesim_Init(1);
    if (result != PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=Init result=%d", static_cast<int>(result));
    }

    MessageEventId = le_event_CreateId("MessageEventId", sizeof(taf_RsimMsg_t));
    memset(&RsimObj, 0, sizeof(RsimObj));
    RsimObj.handlerRef = NULL;
    RsimObj.sapState = SAP_STATE_NOT_CONNECTED;
    RsimObj.sapSubState = SAP_CONNECTED_IDLE;
    RsimObj.maxMsgSize = TAF_SIMRSIM_MAX_MSG_SIZE;

    RSimMsgPool = le_mem_InitStaticPool(RsimMsgs, MSG_POOL_SIZE,sizeof(taf_RsimMsg_Client_t));
    ApduTxPool = le_mem_InitStaticPool(RsimApduTx, APDU_TX_POOL_SIZE, sizeof(taf_RsimApduTx_t));

    PaTxThread = le_thread_Create("rsimPaTx", PaTxThreadMain, NULL);
    le_thread_Start(PaTxThread);
    paEventListener.onApduTransfer = onApduTransferCallback;
    paEventListener.onCardConnect = onCardConnectCallback;
    paEventListener.onCardDisconnect = onCardDisconnectCallback;
    paEventListener.onCardPowerUp = onCardPowerUpCallback;
    paEventListener.onCardPowerDown = onCardPowerDownCallback;
    paEventListener.onCardReset = onCardResetCallback;
    paEventListener.onServiceStatusChange = onServiceStatusChangeCallback;

    result = taf_pa_remotesim_RegisterEventListener(&paEventListener, {});
    if (result != PA_OK) {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_remotesim_RegisterEventListener result=%d", static_cast<int>(result));
        return;
    }
}

taf_simRsim &taf_simRsim::GetInstance()
{
    static taf_simRsim instance;
    return instance;
}

taf_simRsim_MessageHandlerRef_t taf_simRsim::AddMessageHandler(taf_simRsim_MessageHandlerFunc_t handlerPtr,
    void* contextPtr) {
    le_event_HandlerRef_t handlerRef;

    TAF_ERROR_IF_RET_VAL(NULL != RsimObj.handlerRef, NULL," Message handler already registered");

    handlerRef = le_event_AddLayeredHandler("RsimMessageHandler",
                                            MessageEventId,
                                            FirstLayerMessageHandler,
                                            (void*)handlerPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);

    //Send connection available request to modem
    pa_result_t result = taf_pa_remotesim_SendConnectionAvailable(nullptr, {});
    if (result != PA_OK) {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_remotesim_SendConnectionAvailable result=%d", static_cast<int>(result));
        LE_KILL_CLIENT("Failed to send connection available event request to the modem!\n");
    }

    RsimObj.handlerRef = (taf_simRsim_MessageHandlerRef_t)handlerRef;

    return (taf_simRsim_MessageHandlerRef_t)(handlerRef);
}

void taf_simRsim::RemoveMessageHandler( taf_simRsim_MessageHandlerRef_t handlerRef) {

    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
    pa_result_t result = taf_pa_remotesim_SendConnectionUnavailable(nullptr, {});
    if (result != PA_OK) {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_remotesim_SendConnectionUnavailable result=%d", static_cast<int>(result));
        LE_KILL_CLIENT("Failed to send connection unavailable event request to the modem!\n");
    }

    // Remove stored handler reference
    RsimObj.handlerRef = NULL;
}

le_result_t taf_simRsim::SendApduRequest(const unsigned int id, const std::vector<uint8_t> &apdu)
{
    taf_RsimMsg_t RsimMsg;
    size_t length = 0;
    memset(&RsimMsg, 0, sizeof(taf_RsimMsg_t));

    RsimMsg.msg[0]  = MSGID_TRANSFER_APDU_REQ;
    RsimMsg.msg[1]  = 0x01;
    RsimMsg.msg[2]  = 0x00;
    RsimMsg.msg[3]  = 0x00;
    RsimMsg.msg[4] = id;
    RsimMsg.msg[5]  = 0x00;
    RsimMsg.msg[6]  = ((apdu.size() & 0xFF00U) >> MSB_SHIFT);
    RsimMsg.msg[7]  = (apdu.size() & 0x00FF);

    length = 8;
    for (unsigned int i = 0; i < apdu.size(); i++) {
        RsimMsg.msg[i + length] = apdu[i];
    }
    length += apdu.size();

    //Message length should be 4 byte aligned
    length = ((length + 0x03) & (~0x3));
    RsimMsg.msgSize = length;

    if (length > RsimObj.maxMsgSize) {
        LE_ERROR("SAP message too long! Size=%zu, MaxSize=%d",
                length, RsimObj.maxMsgSize);
        return LE_FAULT;
    } else {
        RsimObj.sapSubState = SAP_CONNECTED_APDU;
        le_event_Report(MessageEventId, &(RsimMsg), sizeof(RsimMsg));
    }

    return LE_OK;

}

le_result_t taf_simRsim::SendATRRequest(taf_SapSubState_t subState)
{
    taf_RsimMsg_t RsimMsg;
    memset(&RsimMsg, 0, sizeof(taf_RsimMsg_t));

    RsimMsg.msg[0]  = MSGID_TRANSFER_ATR_REQ;
    RsimMsg.msg[1]  = 0x00;

    RsimMsg.msgSize = 4;

    RsimObj.sapSubState = subState;
    le_event_Report(MessageEventId, &(RsimMsg), sizeof(RsimMsg));

    return LE_OK;
}

le_result_t taf_simRsim::SendCardConnectRequest()
{
    TAF_ERROR_IF_RET_VAL(SAP_STATE_CONNECTED == RsimObj.sapState,
                         LE_FAULT,
                         "SAP is already connected");

    taf_RsimMsg_t RsimMsg;
    memset(&RsimMsg, 0, sizeof(taf_RsimMsg_t));

    RsimMsg.msg[0]  = MSGID_CONNECT_REQ;
    RsimMsg.msg[1]  = 0x01;

    RsimMsg.msg[4]  = PARAMID_MAX_MSG_SIZE;
    RsimMsg.msg[5]  = 0x00;
    RsimMsg.msg[6]  = 0x00;
    RsimMsg.msg[7]  = LENGTH_MAX_MSG_SIZE;

    RsimMsg.msg[8]  = ((RsimObj.maxMsgSize & 0xFF00U) >> MSB_SHIFT);
    RsimMsg.msg[9]  = (RsimObj.maxMsgSize & 0x00FF);
    RsimMsg.msg[10] = 0x00;
    RsimMsg.msg[11] = 0x00;

    RsimMsg.msgSize = 12;

    RsimObj.sapState = SAP_STATE_CONNECTING;
    le_event_Report(MessageEventId, &(RsimMsg), sizeof(RsimMsg));

    return LE_OK;
}

le_result_t taf_simRsim::SendCardDisconnectRequest() {

    taf_RsimMsg_t RsimMsg;
    memset(&RsimMsg, 0, sizeof(taf_RsimMsg_t));

    RsimMsg.msg[0]  = MSGID_DISCONNECT_REQ;
    RsimMsg.msg[1]  = 0x00;

    RsimMsg.msgSize = 4;

    RsimObj.sapSubState = SAP_CONNECTED_DISCONNECT;
    le_event_Report(MessageEventId, &(RsimMsg), sizeof(RsimMsg));

    return LE_OK;
}

le_result_t taf_simRsim::SendCardPowerUpRequest() {

    TAF_ERROR_IF_RET_VAL(RsimObj.sapState != SAP_STATE_CONNECTED
                            || RsimObj.sapSubState != SAP_CONNECTED_IDLE, LE_FAULT,
                         "SAP is not connected or not in idle state");
    taf_RsimMsg_t RsimMsg;
    memset(&RsimMsg, 0, sizeof(taf_RsimMsg_t));

    RsimMsg.msg[0]  = MSGID_POWER_SIM_ON_REQ;
    RsimMsg.msg[1]  = 0x00;

    RsimMsg.msgSize = 4;

    RsimObj.sapSubState = SAP_CONNECTED_POWER_ON;
    le_event_Report(MessageEventId, &(RsimMsg), sizeof(RsimMsg));

    return LE_OK;
}

le_result_t taf_simRsim::SendCardPowerDownRequest() {

    TAF_ERROR_IF_RET_VAL(SAP_STATE_CONNECTED != RsimObj.sapState || RsimObj.sapSubState != SAP_CONNECTED_IDLE,
                         LE_FAULT,
                         "SAP not connected or not in idle state");
    taf_RsimMsg_t RsimMsg;
    memset(&RsimMsg, 0, sizeof(taf_RsimMsg_t));

    RsimMsg.msg[0]  = MSGID_POWER_SIM_OFF_REQ;
    RsimMsg.msg[1]  = 0x00;

    RsimMsg.msgSize = 4;

    RsimObj.sapSubState = SAP_CONNECTED_POWER_OFF;
    le_event_Report(MessageEventId, &(RsimMsg), sizeof(RsimMsg));

    return LE_OK;
}

le_result_t taf_simRsim::SendCardResetRequest() {

    TAF_ERROR_IF_RET_VAL(RsimObj.sapState != SAP_STATE_CONNECTED
                            || RsimObj.sapSubState == SAP_CONNECTED_POWER_ON
                            ||  RsimObj.sapSubState == SAP_CONNECTED_POWER_OFF, LE_FAULT,
                         "SAP is not connected");
    taf_RsimMsg_t RsimMsg;
    memset(&RsimMsg, 0, sizeof(taf_RsimMsg_t));

    RsimMsg.msg[0]  = MSGID_RESET_SIM_REQ;
    RsimMsg.msg[1]  = 0x00;

    RsimMsg.msgSize = 4;

    RsimObj.sapSubState = SAP_CONNECTED_RESET;
    le_event_Report(MessageEventId, &(RsimMsg), sizeof(RsimMsg));

    return LE_OK;
}

void taf_simRsim::FirstLayerMessageHandler( void* reportPtr, void* secondLayerHandlerFunc) {
    taf_RsimMsg_t* messageEvent = (taf_RsimMsg_t*)reportPtr;

    taf_simRsim_MessageHandlerFunc_t clientHandlerFunc = (taf_simRsim_MessageHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(messageEvent->msg, messageEvent->msgSize, le_event_GetContextPtr());
}

le_result_t taf_simRsim::SendMessage(const uint8_t* messagePtr, size_t messageNumElements,
    taf_simRsim_CallbackHandlerFunc_t callbackPtr, void* contextPtr) {

    TAF_ERROR_IF_RET_VAL(RsimObj.maxMsgSize < messageNumElements, LE_BAD_PARAMETER,
            "SAP message length is long");
    LE_DEBUG("SendMessage ");

    taf_RsimMsg_Client_t *clientMsgPtr =( taf_RsimMsg_Client_t *) le_mem_ForceAlloc(RSimMsgPool);
    memcpy(clientMsgPtr->message.msg, messagePtr, messageNumElements);
    clientMsgPtr->message.msgSize = messageNumElements;
    clientMsgPtr->callbackRef = callbackPtr;
    clientMsgPtr->context = contextPtr;

    le_event_QueueFunctionToThread(MainThread, HandleClientMsg, clientMsgPtr, NULL);

    return LE_OK;
}

le_result_t taf_simRsim::VerifyConnectionParameter(const uint8_t* buf, size_t size) {
    uint8_t paramId = PARAMID_CONNECTION_STATUS;
    uint8_t paramLength = LENGTH_CONNECTION_STATUS;
    uint8_t nParam = 1;
    if (LE_OK != VerifyParamId(buf, paramId, nParam)) {
        return LE_FAULT;
    }

    return VerifyParamLength(buf, paramLength, nParam);
}

le_result_t taf_simRsim::VerifyMaxMsgSizeParameter(const uint8_t* buf, size_t size) {
    uint8_t paramId = PARAMID_MAX_MSG_SIZE;
    uint8_t paramLength = LENGTH_MAX_MSG_SIZE;
    uint8_t nParam = 2;
    if (LE_OK != VerifyParamId(buf, paramId, nParam)) {
        return LE_FAULT;
    }
    return VerifyParamLength(buf, paramLength, nParam);
}

le_result_t taf_simRsim::VerifyParameterCount(const uint8_t* buf, uint8_t numberOfParam){
    TAF_ERROR_IF_RET_VAL(buf[1] < numberOfParam, LE_FAULT,
            " Number of parameters are less!\n");
    return LE_OK;
}

le_result_t taf_simRsim::VerifyMessageLength(const uint8_t* buf, size_t size, uint8_t numberOfParam){
    uint8_t minMsgLength =  LENGTH_SAP_HEADER + numberOfParam * LENGTH_PARAM;
    TAF_ERROR_IF_RET_VAL(size < minMsgLength, LE_FAULT,
            " Msg length is too short!\n");
    return LE_OK;
}

le_result_t taf_simRsim::VerifyResultCodeParameter(const uint8_t* buf, size_t size) {
    uint8_t paramId = PARAMID_RESULT_CODE;
    uint8_t paramLength = LENGTH_RESULT_CODE;
    uint8_t nParam = 1;
    if (LE_OK != VerifyParamId(buf, paramId, nParam)) {
        return LE_FAULT;
    }
    return VerifyParamLength(buf, paramLength, nParam);
}

le_result_t taf_simRsim::VerifyDisconnectParameter(const uint8_t* buf, size_t size) {
    uint8_t paramId = PARAMID_DISCONNECTION_TYPE;
    uint8_t paramLength = LENGTH_DISCONNECTION_TYPE;
    uint8_t nParam = 1;
    if (LE_OK != VerifyParamId(buf, paramId, nParam)) {
        return LE_FAULT;
    }
    return VerifyParamLength(buf, paramLength, nParam);
}

le_result_t taf_simRsim::VerifyAPDUParameter(const uint8_t* buf, size_t size) {
    uint8_t paramId = PARAMID_COMMAND_APDU;
    uint8_t nParam = 2;
    return VerifyParamId(buf, paramId, nParam);
}

le_result_t taf_simRsim::VerifyStatusChangeParameter(const uint8_t* buf, size_t size) {
    uint8_t paramId = PARAMID_STATUS_CHANGE;
    uint8_t paramLength = LENGTH_STATUS_CHANGE;
    uint8_t nParam = 1;
    if (LE_OK != VerifyParamId(buf, paramId, nParam)) {
        return LE_FAULT;
    }
    return VerifyParamLength(buf, paramLength, nParam);
}

le_result_t taf_simRsim::VerifyATRParameter(const uint8_t* buf, size_t size) {
    uint8_t paramId = PARAMID_ATR;
    uint8_t nParam = 2;
    return VerifyParamId(buf, paramId, nParam);
}

le_result_t taf_simRsim::VerifyParamId(const uint8_t* buf,uint8_t paramId, uint8_t parameter) {

    int paramIdByte = LENGTH_SAP_HEADER + (parameter - 1) * LENGTH_PARAM;
    uint8_t Id = buf[paramIdByte];
    TAF_ERROR_IF_RET_VAL(Id != paramId, LE_FAULT,
            " Invalid parameter identifier");
    return LE_OK;

}

le_result_t taf_simRsim::VerifyParamLength(const uint8_t* buf, uint16_t paramLength, uint8_t parameter) {

    uint8_t lengthByte1 = LENGTH_SAP_HEADER + 2 + ((parameter-1) * LENGTH_PARAM);
    uint8_t lengthByte2 = lengthByte1 + 1;
    uint16_t length = (uint16_t)( ((uint16_t)(buf[lengthByte1] << MSB_SHIFT)) | buf[lengthByte2]);
    TAF_ERROR_IF_RET_VAL(length != paramLength, LE_FAULT,
            " Invalid parameter length");
    return LE_OK;
}

void taf_simRsim::HandleClientMsg(void* param1Ptr, void* param2Ptr) {

    le_result_t result = LE_OK;
    taf_RsimMsg_Client_t* rsimMsgPtr = (taf_RsimMsg_Client_t*) param1Ptr;
    uint8_t* messagePtr = rsimMsgPtr->message.msg;
    taf_simRsim_CallbackHandlerFunc_t callbackRef = rsimMsgPtr->callbackRef;
    size_t msgSize = rsimMsgPtr->message.msgSize;
    uint8_t msgId = messagePtr[0];
    auto &rSim = taf_simRsim::GetInstance();
    switch (msgId)
    {
        case MSGID_TRANSFER_APDU_RESP:
            result = rSim.HandleApduTransfer(messagePtr, msgSize);
            break;

        case MSGID_CONNECT_RESP:
            result = rSim.HandleCardConnect(messagePtr, msgSize);
            break;

        case MSGID_DISCONNECT_IND:
            result = rSim.HandleCardDisconnectInd(messagePtr, msgSize);
            break;
        case MSGID_DISCONNECT_RESP:
            result = rSim.HandleCardDisconnectResp(messagePtr, msgSize);
            break;

        case MSGID_POWER_SIM_OFF_RESP:
            result = rSim.HandleCardPowerDown(messagePtr, msgSize);
            break;

        case MSGID_POWER_SIM_ON_RESP:
            result = rSim.HandleCardPowerUp(messagePtr, msgSize);
            break;

        case MSGID_RESET_SIM_RESP:
            result = rSim.HandleResetResponse(messagePtr, msgSize);
            break;

        case MSGID_STATUS_IND:
            result = rSim.HandleStatusInd(messagePtr, msgSize);
            break;

        case MSGID_TRANSFER_ATR_RESP:
            result = rSim.HandleATRResponse(messagePtr, msgSize);
            break;

        case MSGID_ERROR_RESP:
            result = rSim.HandleErrorResp();
            break;

        case MSGID_TRANSFER_CARD_READER_STATUS_RESP:
        case MSGID_SET_TRANSPORT_PROTOCOL_RESP:
            LE_ERROR("Received unsupported SAP message");
            result = LE_UNSUPPORTED;
            break;

        default:
            LE_ERROR("Unknown SAP message with id %d received", msgId);
            result = LE_BAD_PARAMETER;
            break;
    }

    if (callbackRef != NULL) {
        callbackRef(msgId, result, rsimMsgPtr->context);
    } else {
        LE_WARN("No callback registered %d", msgId);
    }

    le_mem_Release(rsimMsgPtr);
}

le_result_t taf_simRsim::HandleApduTransfer(const uint8_t* msgPtr, size_t msgSize) {
    LE_DEBUG("Received APDU transfer response msg sap state = %d Sap subState = %d", RsimObj.sapState, RsimObj.sapSubState );
    if ((SAP_STATE_CONNECTED != RsimObj.sapState) || (SAP_CONNECTED_APDU != RsimObj.sapSubState))
    {
        LE_ERROR("Receied apdu transfer response in invalid state");
        return LE_FAULT;
    }

    if (VerifyMessageLength(msgPtr, msgSize, 1) != LE_OK
            ||(VerifyParameterCount( msgPtr, 1) != LE_OK)
            || (VerifyResultCodeParameter( msgPtr, msgSize) != LE_OK)) {
        return LE_FORMAT_ERROR;
    }

    le_result_t result = LE_OK;
    uint8_t resultCode = msgPtr[8];
    RsimObj.sapSubState = SAP_CONNECTED_IDLE;

    switch (resultCode)
    {
        case RESULTCODE_OK:
            result = SendApduResp(msgPtr, msgSize);
            break;
        case RESULTCODE_ERROR_CARD_REMOVED:
            result = HandleCardRemoved();
            break;
        case RESULTCODE_ERROR_NO_REASON:
        case RESULTCODE_ERROR_CARD_NOK:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_UNKNOWN);
            break;
        case RESULTCODE_ERROR_CARD_OFF:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_POWER_DOWN);
            break;
        default:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_INVALID);
            break;
    }

    return result;
}

void* taf_simRsim::PaTxThreadMain(void* contextPtr) {
    LE_INFO("PA sender thread started");
    le_event_RunLoop();
    return NULL;
}

void taf_simRsim::DoSendApduToPa(void* param1Ptr, void* param2Ptr) {
    taf_RsimApduTx_t* txPtr = (taf_RsimApduTx_t*) param1Ptr;

    std::vector<uint8_t> apdu(txPtr->apdu, txPtr->apdu + txPtr->apduSize);

    pa_result_t result = taf_pa_remotesim_SendApdu(txPtr->id, apdu, true, apdu.size(),
                                                  0, nullptr, {});
    if (result != PA_OK) {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_remotesim_SendApdu result=%d",
                static_cast<int>(result));
    }

    le_mem_Release(txPtr);
}

le_result_t taf_simRsim::SendApduResp(const uint8_t* msgPtr, size_t msgSize)
{
    if (VerifyMessageLength(msgPtr, msgSize, 1) != LE_OK
            ||(VerifyParameterCount( msgPtr, 1) != LE_OK)) {
        return LE_FORMAT_ERROR;
    }
    uint8_t apduId = msgPtr[12];
    uint8_t LengthByte1  = 14;
    uint8_t LengthByte2  = 15;
    uint8_t apduStartByte = 16;
    uint16_t apduLength = (uint16_t)(((uint16_t)(msgPtr[LengthByte1] << MSB_SHIFT))
                    | msgPtr[LengthByte2]);

    if ((size_t)apduStartByte + apduLength > TAF_SIMRSIM_MAX_MSG_SIZE) {
        LE_ERROR("APDU length %u exceeds the message buffer", apduLength);
        return LE_FORMAT_ERROR;
    }

    taf_RsimApduTx_t* txPtr = (taf_RsimApduTx_t*) le_mem_ForceAlloc(ApduTxPool);
    txPtr->id = apduId;
    txPtr->apduSize = apduLength;
    memcpy(txPtr->apdu, &msgPtr[apduStartByte], apduLength);

    le_event_QueueFunctionToThread(PaTxThread, DoSendApduToPa, txPtr, NULL);

    return LE_OK;
}

le_result_t taf_simRsim::HandleCardConnect(const uint8_t* msgPtr, size_t messageNumElements)
{
    LE_INFO("Received Card Connect msg response\n");
    TAF_ERROR_IF_RET_VAL(SAP_STATE_CONNECTING != RsimObj.sapState,LE_FAULT,
                         "SAP is not in connecting state");
    if (VerifyMessageLength(msgPtr, messageNumElements, 1) != LE_OK
            ||(VerifyParameterCount( msgPtr, 1) != LE_OK)
            || (VerifyConnectionParameter( msgPtr, messageNumElements) != LE_OK)) {
        return LE_FORMAT_ERROR;
    }
    le_result_t result = LE_OK;
    uint8_t status = msgPtr[8];

    LE_INFO("Card Connect Response - Status: %d, Current SAP State: %d, SubState: %d",
            status, RsimObj.sapState, RsimObj.sapSubState);

    RsimObj.sapState = SAP_STATE_NOT_CONNECTED;
    bool connectionSuccess = false;
    switch (status)
    {
        case CONNSTATUS_OK:
        case CONNSTATUS_OK_ONGOING_CALL:
            RsimObj.sapState = SAP_STATE_CONNECTED;
            RsimObj.sapSubState = SAP_CONNECTED_IDLE;
            connectionSuccess = true;
            break;
        case CONNSTATUS_SERVER_NOK:
            LE_ERROR("Server unable to establish link");
            break;
        case CONNSTATUS_MAXMSGSIZE_NOK:
            if (VerifyMessageLength(msgPtr, messageNumElements, 2) != LE_OK
                    ||(VerifyParameterCount( msgPtr, 2) != LE_OK)
                    || (VerifyMaxMsgSizeParameter( msgPtr, messageNumElements) != LE_OK)) {
                uint8_t  sizeByte1 = 16;
                uint8_t  sizeByte2 = 17;
                uint16_t serverMaxMsgSize = (uint16_t)(((uint16_t)(msgPtr[sizeByte1] << MSB_SHIFT))
                      | msgPtr[sizeByte2]);

                if (serverMaxMsgSize > TAF_SIMRSIM_MAX_MSG_SIZE) {
                    LE_DEBUG("Proposed message size is too big");
                } else if (serverMaxMsgSize < TAF_SIMRSIM_MIN_MSG_SIZE) {
                    LE_DEBUG("Proposed message size is too small");
                } else {
                    RsimObj.maxMsgSize = serverMaxMsgSize;
                    SendCardConnectRequest();
                }
            } else {
                LE_ERROR("In connect response, max msg size parameter is improper");
                result = LE_FORMAT_ERROR;
            }
            break;
        case CONNSTATUS_SMALL_MAXMSGSIZE:
            LE_ERROR("Maximum message size of client is too small");
            break;
        default:
            LE_ERROR("Unknown connection status value %d", status);
            result = LE_FAULT;
            break;
    }
    if (!connectionSuccess) {
        LE_ERROR("Unable to establish link");
        HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_NO_LINK);
    }
    LE_INFO("HandleCardConnect exit ");
    return result;
}

le_result_t taf_simRsim::HandleStatusInd(const uint8_t* msgPtr, size_t messageNumElements)
{
    if ((SAP_STATE_CONNECTED != RsimObj.sapState)) {
        LE_ERROR("Received status Indication in invalid state ");
        return LE_FAULT;
    }
    if (VerifyMessageLength(msgPtr, messageNumElements, 1) != LE_OK
            ||(VerifyParameterCount( msgPtr, 1) != LE_OK)
            || (VerifyStatusChangeParameter( msgPtr, messageNumElements) != LE_OK)) {
        return LE_FORMAT_ERROR;
    }
    LE_DEBUG("Received status_ind");
    le_result_t result = LE_OK;
    uint8_t status = msgPtr[8];
    RsimObj.sapSubState = SAP_CONNECTED_IDLE;
    switch (status) {
        case STATUSCHANGE_UNKNOWN_ERROR:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_UNKNOWN);
            break;
        case STATUSCHANGE_CARD_RESET:
            result = SendATRRequest(SAP_CONNECTED_ATR_RESET);
            break;
        case STATUSCHANGE_CARD_NOK:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_POWER_DOWN);
            break;
        case STATUSCHANGE_CARD_REMOVED:
            result = HandleCardRemoved();
            break;
        case STATUSCHANGE_CARD_INSERTED:
            result = SendATRRequest(SAP_CONNECTED_ATR_INSERT);
            break;
        case STATUSCHANGE_CARD_RECOVERED:
            result = HandleCardWakeUp();
            break;
        default:
            LE_ERROR("Unknown Status %d", status);
            result = LE_FAULT;
            break;
    }
    return result;
}

le_result_t taf_simRsim::HandleCardDisconnectInd(const uint8_t* msgPtr, size_t msgSize)
{
    TAF_ERROR_IF_RET_VAL(RsimObj.sapState != SAP_STATE_CONNECTED, LE_FAULT,
            "Received disconnect ind in invalid state");
    if (VerifyMessageLength(msgPtr, msgSize, 1) != LE_OK
            ||(VerifyParameterCount( msgPtr, 1) != LE_OK)
            || (VerifyDisconnectParameter( msgPtr, msgSize) != LE_OK)) {
            return LE_FORMAT_ERROR;
    }
    LE_DEBUG("Received disconnect Ind ");
    uint8_t disconnectionType = msgPtr[8];
    if (disconnectionType == DISCONNECTTYPE_GRACEFUL) {
        LE_INFO("Gracefuli Disconnection");
        SendCardDisconnectRequest();
    } else if (disconnectionType == DISCONNECTTYPE_IMMEDIATE) {
        LE_INFO("Immediate Disconnection");
     RsimObj.sapState = SAP_STATE_NOT_CONNECTED;
            RsimObj.sapSubState = SAP_CONNECTED_IDLE;
    } else {
        LE_ERROR("Unknown disconnection type");
        return LE_FAULT;
    }

    return LE_OK;
}

le_result_t taf_simRsim::HandleCardDisconnectResp(const uint8_t* messagePtr, size_t messageNumElements)
{
    LE_DEBUG("Received Card Disconnect msg response from client.\n");
    if ((SAP_STATE_CONNECTED != RsimObj.sapState)
            || (SAP_CONNECTED_DISCONNECT != RsimObj.sapSubState)) {
        LE_ERROR("DISCONNECT_RESP received in incoherent state %d / sub-state %d",
                RsimObj.sapState, RsimObj.sapSubState);
        return LE_FAULT;
    }

    LE_DEBUG("DISCONNECT_RESP received");

    RsimObj.sapState = SAP_STATE_NOT_CONNECTED;
    RsimObj.sapSubState = SAP_CONNECTED_IDLE;
    RsimObj.maxMsgSize = TAF_SIMRSIM_MAX_MSG_SIZE;

    return LE_OK;
}

le_result_t taf_simRsim::HandleCardPowerDown(const uint8_t* msgPtr, size_t messageNumElements)
{
    LE_DEBUG("Received Card Power down msg response\n");
    if ((SAP_STATE_CONNECTED != RsimObj.sapState) || (SAP_CONNECTED_POWER_OFF != RsimObj.sapSubState))
    {
        LE_ERROR("Receied power down response in invalid state");
        return LE_FAULT;
    }

    if (VerifyMessageLength(msgPtr, messageNumElements, 1) != LE_OK
            ||(VerifyParameterCount( msgPtr, 1) != LE_OK)
            || (VerifyResultCodeParameter( msgPtr, messageNumElements) != LE_OK)) {
        return LE_FORMAT_ERROR;
    }

    le_result_t result = LE_OK;
    uint8_t resultCode = msgPtr[8];
    LE_DEBUG("POWER_SIM_OFF_RESP received: ResultCode=%d", resultCode);

    RsimObj.sapSubState = SAP_CONNECTED_IDLE;
    switch (resultCode)
    {
        case RESULTCODE_OK:
            LE_DEBUG(" Card power down success response");
            break;
        case RESULTCODE_ERROR_CARD_REMOVED:
            result = HandleCardRemoved();
            break;
        case RESULTCODE_ERROR_NO_REASON:
        case RESULTCODE_ERROR_NO_DATA:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_UNKNOWN);
            break;
        case RESULTCODE_ERROR_CARD_OFF:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_POWER_DOWN);
            break;
        default:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_INVALID);
            break;
    }

    return result;
}

le_result_t taf_simRsim::HandleCardPowerUp(const uint8_t* msgPtr, size_t messageNumElements)
{
    LE_DEBUG("Received Card Power up msg response \n");
    if ((SAP_STATE_CONNECTED != RsimObj.sapState) || (SAP_CONNECTED_POWER_ON != RsimObj.sapSubState))
    {
        LE_ERROR("Received card power on response in invalid statei");
        return LE_FAULT;
    }

    if (VerifyMessageLength(msgPtr, messageNumElements, 1) != LE_OK
            ||(VerifyParameterCount( msgPtr, 1) != LE_OK)
            || (VerifyResultCodeParameter( msgPtr, messageNumElements) != LE_OK)) {
        return LE_FORMAT_ERROR;
    }

    le_result_t result = LE_OK;
    uint8_t resultCode = msgPtr[8];
    RsimObj.sapSubState = SAP_CONNECTED_IDLE;
    LE_DEBUG("Received card power up response : ResultCode=%d", resultCode);
    switch (resultCode)
    {
        case RESULTCODE_OK:
            SendATRRequest(SAP_CONNECTED_ATR_RESET);
            break;
        case RESULTCODE_ERROR_CARD_REMOVED:
            result = HandleCardRemoved();
            break;
        case RESULTCODE_ERROR_NO_REASON:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_UNKNOWN);
            break;
        case RESULTCODE_ERROR_CARD_OFF:
        case RESULTCODE_ERROR_CARD_NOK:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_POWER_DOWN);
            break;
        case RESULTCODE_ERROR_CARD_ON:
            result = HandleCardWakeUp();
            break;
        default:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_INVALID);
            break;
    }

    return result;
}

le_result_t taf_simRsim::HandleResetResponse(const uint8_t* msgPtr, size_t msgLength)
{
    LE_DEBUG("Received reset response\n");

    if ((SAP_STATE_CONNECTED != RsimObj.sapState) ||
         (SAP_CONNECTED_RESET != RsimObj.sapSubState)) {
        LE_ERROR("Received reset responce in invalid state ");
        return LE_FAULT;
    }

    if (VerifyMessageLength(msgPtr, msgLength, 1) != LE_OK
            ||(VerifyParameterCount( msgPtr, 1) != LE_OK)
            || (VerifyResultCodeParameter( msgPtr, msgLength) != LE_OK)) {
        return LE_FORMAT_ERROR;
    }
    RsimObj.sapSubState = SAP_CONNECTED_IDLE;

    le_result_t result = LE_OK;
    uint8_t resultCode = msgPtr[8];
    switch (resultCode) {
        case RESULTCODE_OK:
            SendATRRequest(SAP_CONNECTED_ATR_RESET);
            break;
        case RESULTCODE_ERROR_CARD_REMOVED:
            result = HandleCardRemoved();
            break;
        case RESULTCODE_ERROR_NO_REASON:
        case RESULTCODE_ERROR_NO_DATA:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_UNKNOWN);
            break;
        case RESULTCODE_ERROR_CARD_OFF:
        case RESULTCODE_ERROR_CARD_NOK:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_POWER_DOWN);
            break;
        default:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_INVALID);
            break;
    }
    return result;
}

le_result_t taf_simRsim::HandleATRResponse(const uint8_t* msgPtr, size_t msgLength)
{
    LE_DEBUG("Received ATR response\n");

    if ((SAP_STATE_CONNECTED != RsimObj.sapState) ||
         ((SAP_CONNECTED_ATR_RESET != RsimObj.sapSubState)
          && (SAP_CONNECTED_ATR_INSERT != RsimObj.sapSubState))) {
        LE_ERROR("Received ATR responce in invalid state ");
        return LE_FAULT;
    }

    if (VerifyMessageLength(msgPtr, msgLength, 1) != LE_OK
            ||(VerifyParameterCount( msgPtr, 1) != LE_OK)
            || (VerifyResultCodeParameter( msgPtr, msgLength) != LE_OK)) {
        return LE_FORMAT_ERROR;
    }

    le_result_t result = LE_OK;
    uint8_t resultCode = msgPtr[8];
    switch (resultCode) {
        case RESULTCODE_OK:
            if (VerifyMessageLength(msgPtr, msgLength, 2) == LE_OK
                    &&(VerifyParameterCount( msgPtr, 2) == LE_OK)
                    && (VerifyATRParameter( msgPtr, msgLength) == LE_OK)) {

                if (RsimObj.sapSubState == SAP_CONNECTED_ATR_RESET) {
                    result = HandleCardReset(msgPtr, msgLength);
                } else if (RsimObj.sapSubState == SAP_CONNECTED_ATR_INSERT) {
                    result = HandleCardInserted(msgPtr, msgLength);
                } else {
                    result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_UNKNOWN);
                }
            } else {
                LE_ERROR("Improper ATR message response");
                result = LE_FORMAT_ERROR;
            }
            break;
        case RESULTCODE_ERROR_CARD_REMOVED:
            result = HandleCardRemoved();
            break;
        case RESULTCODE_ERROR_NO_REASON:
        case RESULTCODE_ERROR_NO_DATA:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_UNKNOWN);
            break;
        case RESULTCODE_ERROR_CARD_OFF:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_POWER_DOWN);
            break;
        default:
            result = HandleCardError(TAF_PA_REMOTESIM_CARD_ERROR_INVALID);
            break;
    }
    if (RsimObj.sapSubState == SAP_CONNECTED_ATR_RESET ||
            RsimObj.sapSubState == SAP_CONNECTED_ATR_INSERT) {
        RsimObj.sapSubState = SAP_CONNECTED_IDLE;
    }
    return result;
}

le_result_t taf_simRsim::HandleCardReset(const uint8_t* messagePtr, size_t messageNumElements)
{
    LE_DEBUG("Received Card Reset msg");

        std::vector<uint8_t> atr;
        uint8_t atrLenByte1  = 14;
        uint8_t atrLenByte2  = 15;
        uint8_t atrFirstByte = 16;
        uint16_t atrLength = (uint16_t) (((uint16_t)(messagePtr[atrLenByte1] << MSB_SHIFT))
                | messagePtr[atrLenByte2]);

        for (int i = atrFirstByte; i < atrFirstByte + atrLength; i++) {
            atr.push_back(messagePtr[i]);
        }

        pa_result_t result = taf_pa_remotesim_SendCardReset(atr, nullptr, {});
        if (result != PA_OK) {
            LE_ERROR("[PA_CALL_FAILED] api=taf_pa_remotesim_SendCardReset result=%d", static_cast<int>(result));
            return LE_FAULT;
        }
        return LE_OK;
}

le_result_t taf_simRsim::HandleErrorResp()
{
    le_result_t result = LE_OK;

    switch (RsimObj.sapState)
    {
        case SAP_STATE_NOT_CONNECTED:
            LE_ERROR("ERROR_RESP received in incoherent state ");
            result = LE_FAULT;
        break;
        case SAP_STATE_CONNECTING:
            RsimObj.sapState = SAP_STATE_NOT_CONNECTED;
        break;
        case SAP_STATE_CONNECTED:
            RsimObj.sapSubState = SAP_CONNECTED_IDLE;
        break;
        default:
            LE_ERROR("Unknown SAP state");
            result = LE_FAULT;
        break;
    }

    return result;
}

le_result_t taf_simRsim::HandleCardInserted(const uint8_t* messagePtr, size_t messageNumElements)
{
    LE_DEBUG("Received Card Inserted msg from client.\n");
    std::vector<uint8_t> atr;

    uint8_t atrLenByte1  = 14;
    uint8_t atrLenByte2  = 15;
    uint8_t atrFirstByte = 16;
    uint16_t atrLength = (uint16_t) (((uint16_t)(messagePtr[atrLenByte1] << MSB_SHIFT))
                | messagePtr[atrLenByte2]);

    for (int i = atrFirstByte; i < atrFirstByte + atrLength; i++) {
        atr.push_back(messagePtr[i]);
    }

    pa_result_t result = taf_pa_remotesim_SendCardInserted(atr, nullptr, {});
    if (result != PA_OK) {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_remotesim_SendCardInserted result=%d", static_cast<int>(result));
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_simRsim::HandleCardRemoved()
{
    LE_DEBUG("Received Card Removed msg from client.\n");

    pa_result_t result = taf_pa_remotesim_SendCardRemoved(nullptr, {});
    if (result != PA_OK) {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_remotesim_SendCardRemoved result=%d", static_cast<int>(result));
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_simRsim::HandleCardError(taf_pa_remotesim_CardErrorCause_t cardError)
{
    pa_result_t result = taf_pa_remotesim_SendCardError(cardError, nullptr, {});
    if (result != PA_OK) {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_remotesim_SendCardError result=%d", static_cast<int>(result));
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_simRsim::HandleCardWakeUp()
{
    LE_DEBUG("Received Card Wake Up");
    pa_result_t result = taf_pa_remotesim_SendCardWakeup(nullptr, {});
    if (result != PA_OK) {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_remotesim_SendCardWakeup result=%d", static_cast<int>(result));
        return LE_FAULT;
    }
    return LE_OK;
}

void taf_simRsim::SendConnectionAvailableToPa(void* param1Ptr, void* param2Ptr) {
    pa_result_t result = taf_pa_remotesim_SendConnectionAvailable(nullptr, {});
    if (result != PA_OK) {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_remotesim_SendConnectionAvailable result=%d", static_cast<int>(result));
    }
}

void taf_simRsim::SendConnectionUnavailableToPa(void* param1Ptr, void* param2Ptr) {
    pa_result_t result = taf_pa_remotesim_SendConnectionUnavailable(nullptr, {});
    if (result != PA_OK) {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_remotesim_SendConnectionUnavailable result=%d", static_cast<int>(result));
    }
}

void taf_simRsim::NotifyConnectionAvailable() {
    le_event_QueueFunctionToThread(MainThread, SendConnectionAvailableToPa, NULL, NULL);
}

void taf_simRsim::NotifyConnectionUnavailable() {
    le_event_QueueFunctionToThread(MainThread, SendConnectionUnavailableToPa, NULL, NULL);
}
