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
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafSAP.hpp"
using namespace telux::tel;
using namespace telux::common;
using namespace tafsvc;

void tafOpenConnectionCallback::commandResponse(ErrorCode errorCode)
{
    LE_DEBUG("Received open connection response from modem with errorcode %d",
            static_cast<int>(errorCode));

    auto &sap = taf_simSap::GetInstance();
    uint8_t connectStatus = CONNSTATUS_SERVER_NOK;
    if (errorCode == ErrorCode::SUCCESS) {
        sap.cardConnected = true;
        connectStatus = CONNSTATUS_OK;
    } else {
        sap.cardConnected = false;
    }
    sap.SendConnectResponse(errorCode, connectStatus);
    if (errorCode == ErrorCode::SUCCESS) {
        sap.SendStatusInd(STATUSCHANGE_CARD_RESET);
    }
}

void tafCloseConnectionCallback::commandResponse(ErrorCode errorCode)
{
    LE_DEBUG("Received close connection response from modem with errorcode %d.",
         static_cast<int>(errorCode));
        auto &sap = taf_simSap::GetInstance();
    if (errorCode == ErrorCode::SUCCESS) {
        sap.cardConnected = false;
    }
    sap.SendDisconnectResponse(errorCode);
}

void tafPowerOnCallback::commandResponse(ErrorCode errorCode)
{
    LE_DEBUG("Received power on response from modem with errorcode %d.",
                 static_cast<int>(errorCode));

        auto &sap = taf_simSap::GetInstance();
    if (errorCode == ErrorCode::SUCCESS) {
        sap.cardpoweredOn = true;
    }
    sap.SendPowerOnResponse(errorCode);
}

void tafPowerOffCallback::commandResponse(ErrorCode errorCode)
{
    LE_DEBUG("Received power off response from modem with errorcode %d.\n",
                static_cast<int>(errorCode));
    //Send power OFF response
    auto &sap = taf_simSap::GetInstance();
    if (errorCode == ErrorCode::SUCCESS) {
        sap.cardpoweredOn = false;
    }
    sap.SendPowerDownResponse(errorCode);
}

void tafResetCallback::commandResponse(ErrorCode errorCode)
{
    LE_DEBUG("Received reset response from modem with errorcode %d.\n",
         static_cast<int>(errorCode));

    auto &sap = taf_simSap::GetInstance();
    sap.SendCardResetResponse(errorCode);
}

tafApduResponseCallback::tafApduResponseCallback(uint8_t apduId)
{
    apduId = apduId;
}

void tafApduResponseCallback::onResponse(IccResult result, ErrorCode errorCode)
{
    LE_DEBUG("Received APDU response from modem with errorcode %d.\n", static_cast<int>(errorCode));

    auto &sap = taf_simSap::GetInstance();
    sap.SendAPDUResponse(result.data, apduId, errorCode);
    sap.EraseFromApduRespCbMap(apduId);
}

void tafAtrResponseCallback::atrResponse(std::vector<int> responseAtr, ErrorCode errorCode)
{
    LE_DEBUG("Received AtR response from modem with errorcode %d.\n", static_cast<int>(errorCode));
    auto &sap = taf_simSap::GetInstance();
    sap.SendATRResponse(responseAtr, errorCode);
}

void tafCardReaderCallback::cardReaderResponse(CardReaderStatus readerStatus,
        ErrorCode errorCode) {
    auto &sap = taf_simSap::GetInstance();
    sap.SendCardReaderResponse(errorCode, readerStatus);
}

void taf_simSap::NewSimStateHandler( taf_sim_Id_t simId, taf_sim_States_t simState,
                                void* contextPtr) {
    LE_DEBUG("New SIM event for SIM card: %d", simId);
    auto &sap = taf_simSap::GetInstance();
    if (sap.cardConnected) {
        if (simState == TAF_SIM_ABSENT) {
            sap.SendStatusInd(STATUSCHANGE_CARD_REMOVED);
        } else if (simState == TAF_SIM_READY) {
            sap.SendStatusInd(STATUSCHANGE_CARD_INSERTED);
        }
    }
}

void taf_simSap:: Init(void) {
    slotId = taf_sim_GetSelectedCard();
    sapCardMgr = PhoneFactory::getInstance().getSapCardManager(slotId);
    if (!sapCardMgr) {
        LE_ERROR("Failed to create SapCardManager!\n");
        return;
    }
    bool subSystemStatus = sapCardMgr->isReady();
    if(!subSystemStatus) {
        LE_INFO("Sap subsystem is not ready" );
        LE_INFO( "wait for it to be ready " );
        std::future<bool> f = sapCardMgr->onReady();
        subSystemStatus = f.get();
    }

    if(subSystemStatus) {

        MessageEventId = le_event_CreateId("MessageEventId", sizeof(taf_SapMsg_t));
        openConnCb = std::make_shared<tafOpenConnectionCallback>();
        closeConnCb = std::make_shared<tafCloseConnectionCallback>();
        powerOnCb = std::make_shared<tafPowerOnCallback>();
        powerOffCb = std::make_shared<tafPowerOffCallback>();
        resetCb = std::make_shared<tafResetCallback>();
        atrResetRespCb = std::make_shared<tafAtrResponseCallback>();
        cardReaderCb = std::make_shared<tafCardReaderCallback>();
    }
    taf_sim_AddNewStateHandler(NewSimStateHandler, NULL);
}

taf_simSap &taf_simSap::GetInstance()
{
    static taf_simSap instance;
    return instance;
}

taf_simSap_MessageHandlerRef_t taf_simSap::AddMessageHandler(taf_simSap_MessageHandlerFunc_t handlerPtr,
    void* contextPtr) {
    le_event_HandlerRef_t handlerRef;


    handlerRef = le_event_AddLayeredHandler("SapMessageHandler",
                                            MessageEventId,
                                            FirstLayerMessageHandler,
                                            (void*)handlerPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_simSap_MessageHandlerRef_t)(handlerRef);
}

void taf_simSap::RemoveMessageHandler( taf_simSap_MessageHandlerRef_t handlerRef) {

    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

void taf_simSap::FirstLayerMessageHandler( void* reportPtr, void* secondLayerHandlerFunc) {
    taf_SapMsg_t* messageEvent = (taf_SapMsg_t*)reportPtr;

    taf_simSap_MessageHandlerFunc_t clientHandlerFunc = (taf_simSap_MessageHandlerFunc_t)secondLayerHandlerFunc;

    clientHandlerFunc(messageEvent->msg, messageEvent->msgSize, le_event_GetContextPtr());
}

le_result_t taf_simSap::ProcessSAPRequestMessages(const uint8_t *buf, size_t msgLength)
{
    le_result_t result = LE_FAULT;
    switch (buf[0]) {
        case MSGID_CONNECT_REQ:
            result = OpenSAPConnection(buf, msgLength);
            break;
        case MSGID_DISCONNECT_REQ:
            result = DisconnectFromCard();
            break;
        case MSGID_TRANSFER_APDU_REQ:
            result = SendApduToSim(buf, msgLength);
            break;
        case MSGID_TRANSFER_ATR_REQ:
            result = RequestAtrAfterReset();
            break;
        case MSGID_POWER_SIM_OFF_REQ:
            result = RequestPowerOff();
            break;
        case MSGID_POWER_SIM_ON_REQ:
            result = RequestPowerOn();
            break;
        case MSGID_RESET_SIM_REQ:
            result = ResetCard();
            break;
        case MSGID_TRANSFER_CARD_READER_STATUS_REQ:
            result = RequestCardReaderStatus();
            break;
        case MSGID_SET_TRANSPORT_PROTOCOL_REQ:
            result = LE_UNSUPPORTED;
            break;
        default:
            LE_ERROR("Received an unknown msg type from the client!");
            SendErrorResponse();
            break;
    }
    return result;
}

le_result_t taf_simSap::OpenSAPConnection(const uint8_t* msg, uint8_t msgLength) {
    if (VerifyConnectRequest(msg, msgLength) !=LE_OK || (!taf_sim_IsPresent(slotId))) {
        LE_ERROR("Cannot open connection, card is not present!");
        SendConnectResponse((ErrorCode)-1, (uint8_t)CONNSTATUS_SERVER_NOK);
        return LE_FAULT;
    }

    //Check Max msg size in message buffer
    uint8_t maxMsgSizeByte = 8;
    uint16_t maxMsgSizeValue = (uint16_t)(((uint16_t)(msg[maxMsgSizeByte] << MSB_SHIFT)) | msg[maxMsgSizeByte + 1]);
    if (maxMsgSizeValue > TAF_SIMSAP_MAX_MSG_SIZE) {
        LE_ERROR("Cannot open connection, Server doesnot support maximum message size");
        SendConnectResponse((ErrorCode)-1, (uint8_t)CONNSTATUS_MAXMSGSIZE_NOK);
        return LE_FAULT;
    }
    if (maxMsgSizeValue < (uint16_t)TAF_SIMSAP_MIN_MSG_SIZE) {
        LE_ERROR("Cannot open connection, Maximum message size by client is too small maxMsgSizeValue = %d",maxMsgSizeValue);
        SendConnectResponse((ErrorCode)-1, (uint8_t)CONNSTATUS_SMALL_MAXMSGSIZE);
        return LE_FAULT;
    }

    //Open SAP connection
    if (sapCardMgr->openConnection(SapCondition::SAP_CONDITION_BLOCK_VOICE_OR_DATA, openConnCb)
            != Status::SUCCESS) {
        LE_ERROR("Failed to send open connection request to the modem!");
        SendConnectResponse((ErrorCode)-1, (uint8_t)CONNSTATUS_SERVER_NOK);
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_simSap::DisconnectFromCard()
{
    if (!cardConnected) {
        LE_ERROR("Cannot close connection - card is already not connected!");
        return LE_FAULT;
    }

    if (sapCardMgr->closeConnection(closeConnCb) != Status::SUCCESS) {
        LE_ERROR("Failed to send close connection request to the modem!");
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_simSap::RequestPowerOn()
{
    if (!cardConnected) {
        LE_ERROR("Cannot  power up card - no open connection to card!\n");
        //Send card not accessible response
        SendResultCodeResponse(MSGID_POWER_SIM_ON_RESP, RESULTCODE_ERROR_CARD_NOK);
        return LE_FAULT;
    }

    if (taf_sim_GetState(slotId) == TAF_SIM_ABSENT) {
        LE_ERROR("Cannot  power up card - card not present");
        SendResultCodeResponse(MSGID_POWER_SIM_ON_RESP, RESULTCODE_ERROR_CARD_REMOVED);
        return LE_FAULT;
    }

    if (cardpoweredOn) {
        LE_ERROR("Cannot  power up card - card powered on");
        SendResultCodeResponse(MSGID_POWER_SIM_ON_RESP, RESULTCODE_ERROR_CARD_ON);
        return LE_FAULT;
    }

    if (sapCardMgr->requestSimPowerOn(powerOnCb) != Status::SUCCESS) {
        LE_ERROR("Failed to send SIM power on request to the modem!\n");
        SendResultCodeResponse(MSGID_POWER_SIM_ON_RESP, RESULTCODE_ERROR_NO_REASON);
        return LE_FAULT;
    }

    return LE_OK;
}


le_result_t taf_simSap::RequestPowerOff()
{
 if (!cardConnected) {
        LE_ERROR("Cannot  power down card - no open connection to card!\n");
        //Send card not accessible response
        SendResultCodeResponse(MSGID_POWER_SIM_OFF_RESP, RESULTCODE_ERROR_CARD_NOK);
        return LE_FAULT;
    }

    if (taf_sim_GetState(slotId) == TAF_SIM_ABSENT) {
        LE_ERROR("Cannot  power down card - card not present");
        SendResultCodeResponse(MSGID_POWER_SIM_OFF_RESP, RESULTCODE_ERROR_CARD_REMOVED);
        return LE_FAULT;
    }

    if (!cardpoweredOn) {
        LE_ERROR("Cannot  power down card - card powered off");
        SendResultCodeResponse(MSGID_POWER_SIM_OFF_RESP, RESULTCODE_ERROR_CARD_OFF);
        return LE_FAULT;
    }


    if (sapCardMgr->requestSimPowerOff(powerOffCb) != Status::SUCCESS) {
        LE_ERROR("Failed to send power off request to the modem!\n");
        SendResultCodeResponse(MSGID_POWER_SIM_OFF_RESP, RESULTCODE_ERROR_NO_REASON);
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_simSap::ResetCard()
{
    if (!cardConnected) {
        LE_ERROR("Cannot reset card - no open connection to card!\n");
        //Send card not accessible response
        SendResultCodeResponse(MSGID_RESET_SIM_RESP,RESULTCODE_ERROR_CARD_NOK);
        return LE_FAULT;
    }

    if (taf_sim_GetState(slotId) == TAF_SIM_ABSENT) {
        LE_ERROR("Cannot reset card - card not present");
        SendResultCodeResponse(MSGID_RESET_SIM_RESP, RESULTCODE_ERROR_CARD_REMOVED);
        return LE_FAULT;
    }

    if (!cardpoweredOn) {
        LE_ERROR("Cannot reset card - card powered off");
        SendResultCodeResponse(MSGID_RESET_SIM_RESP, RESULTCODE_ERROR_CARD_OFF);
        return LE_FAULT;
    }


    if (sapCardMgr->requestSimReset(resetCb) != Status::SUCCESS) {
        LE_ERROR("Failed to send reset request to the modem!\n");
        SendResultCodeResponse(MSGID_RESET_SIM_RESP, RESULTCODE_ERROR_NO_REASON);
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_simSap::SendApduToSim(const uint8_t *buf, int bytes)
{
    /* APDU message format - ETSI TS 102 221 specification, section 10.1
     *
     * Case     Structure
     *  1       CLA INS P1 P2
     *  2       CLA INS P1 P2 Le
     *  3       CLA INS P1 P2 Lc Data
     *  4       CLA INS P1 P2 Lc Data Le
     *
     */

    if (!cardConnected) {
        LE_ERROR("Cannot transmit APDU - no open connection to card!");
        //Send card not accessible response
        SendResultCodeResponse(MSGID_TRANSFER_APDU_RESP,RESULTCODE_ERROR_CARD_NOK);
        return LE_FAULT;
    }

    if (taf_sim_GetState(slotId) == TAF_SIM_ABSENT) {
        LE_ERROR("Cannot transmit APDU - card not present");
        SendResultCodeResponse(MSGID_TRANSFER_APDU_RESP, RESULTCODE_ERROR_CARD_REMOVED);
        return LE_FAULT;
    }

    if (!cardpoweredOn) {
        LE_ERROR("Canot transmit APDU - card powered off");
        SendResultCodeResponse(MSGID_TRANSFER_APDU_RESP, RESULTCODE_ERROR_CARD_OFF);
        return LE_FAULT;
    }
    uint8_t apduId = buf[4];
    uint8_t lengthByte1 = LENGTH_SAP_HEADER + 2;
    uint8_t lengthByte2 = lengthByte1 + 1;
    uint16_t apduLength = (uint16_t)( ((uint16_t)(buf[lengthByte1] << MSB_SHIFT)) | buf[lengthByte2]);

    uint8_t cla = buf[8];
    uint8_t instruction = buf[9];
    uint8_t p1 = buf[10];
    uint8_t p2 = buf[11];

    // If the message is less than 8 bytes, Lc is not included.
    uint8_t lc;
    if (apduLength < APDU_CASE_3_HEADER_LENGTH) {
        lc = 0;
    } else {
        lc = buf[12];
    }

    // If Lc is present and the message is less than 7 + Lc bytes, there is less
    // data than Lc indicated.
    if (lc > 0 && apduLength < lc + 5) {
        LE_ERROR("APDU transfer msg length is shorter than indicated!\n");
        return LE_FAULT;
    }

    std::vector<uint8_t> data;

    for (unsigned int i = 0; i < lc; i++) {
        data.push_back(buf[i + 13]);
    }

    // If the message is Case 2 or 4 formats, Le is included.
    uint8_t le = 0;
    if (apduLength == 5) {
        le = buf[12];
    } else if (apduLength > lc + 5) {
        le = buf[lc + 5];
    }

    apduRespCbMap[apduId] = std::make_shared<tafApduResponseCallback>(apduId);
    apduRespCbMap[apduId]->setApduId(apduId);

    if (sapCardMgr->transmitApdu(cla, instruction, p1, p2, lc, data, le, apduRespCbMap[apduId])
        != Status::SUCCESS) {
        LE_ERROR("Failed to transmit APDU request to the modem!\n");
        SendResultCodeResponse(MSGID_TRANSFER_APDU_RESP, RESULTCODE_ERROR_NO_REASON);
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_simSap::RequestAtrAfterReset()
{

    if (taf_sim_GetState(slotId) == TAF_SIM_ABSENT) {
        LE_ERROR("Cannot transmit ATR - card not present");
        SendResultCodeResponse(MSGID_TRANSFER_ATR_RESP, RESULTCODE_ERROR_CARD_REMOVED);
        return LE_FAULT;
    }

    if (!cardpoweredOn) {
        LE_ERROR("Canot transmit ATR - card powered off");
        SendResultCodeResponse(MSGID_TRANSFER_ATR_RESP, RESULTCODE_ERROR_CARD_OFF);
        return LE_FAULT;
    }

    if (sapCardMgr->requestAtr(atrResetRespCb) != Status::SUCCESS) {
        LE_ERROR("Failed to send AtR request to the modem!\n");
        SendResultCodeResponse(MSGID_TRANSFER_ATR_RESP, RESULTCODE_ERROR_NO_REASON);
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t taf_simSap:: RequestCardReaderStatus()
{
    if (sapCardMgr->requestCardReaderStatus(cardReaderCb) != Status::SUCCESS) {
        LE_ERROR("Failed to send AtR request to the modem!\n");
        SendResultCodeResponse(MSGID_SET_TRANSPORT_PROTOCOL_RESP, RESULTCODE_ERROR_NO_REASON);
        return LE_FAULT;
    }
    return LE_OK;
}

void taf_simSap:: SendConnectResponse(ErrorCode errorCode, uint8_t connectionStatus) {
    LE_DEBUG(" Send connect response connectionStatus = %d", connectionStatus);
    taf_SapMsg_t SapMsg;
    memset(&SapMsg, 0, sizeof(taf_SapMsg_t));
    uint8_t paramCount = 0;
    SapMsg.msg[0]  = MSGID_CONNECT_RESP;
    SapMsg.msg[1]  = 0x00;
    SapMsg.msg[2]  = 0x00;
    SapMsg.msg[3]  = 0x00;

    paramCount++;
    SapMsg.msg[4] = PARAMID_CONNECTION_STATUS;
    SapMsg.msg[5] = 0x00;
    SapMsg.msg[6] = 0x00;
    SapMsg.msg[7] = LENGTH_CONNECTION_STATUS;

    SapMsg.msg[8] = connectionStatus;
    SapMsg.msg[9] = 0x00;
    SapMsg.msg[10] = 0x00;
    SapMsg.msg[11] = 0x00;

    SapMsg.msgSize = 12;
    if (errorCode != ErrorCode::SUCCESS) {

        if (connectionStatus == CONNSTATUS_MAXMSGSIZE_NOK ) {
            paramCount++;
            SapMsg.msg[12] = PARAMID_MAX_MSG_SIZE;
            SapMsg.msg[13] = 0x00;
            SapMsg.msg[14] = 0x00;
            SapMsg.msg[15] = LENGTH_MAX_MSG_SIZE;

            SapMsg.msg[16] = (TAF_SIMSAP_MAX_MSG_SIZE & 0xFF00U) >> MSB_SHIFT;
            SapMsg.msg[17] = (TAF_SIMSAP_MAX_MSG_SIZE & 0x00FF);
            SapMsg.msg[18] = 0x00;
            SapMsg.msg[19] = 0x00;
            SapMsg.msgSize = 20;
        }
    }
    SapMsg.msg[1]  = paramCount;

    le_event_Report(MessageEventId, &(SapMsg), sizeof(SapMsg));
}

void taf_simSap:: SendDisconnectInd() {
    uint8_t paramCount = 0;
    taf_SapMsg_t SapMsg;
    memset(&SapMsg, 0, sizeof(taf_SapMsg_t));
    SapMsg.msg[0]  = MSGID_DISCONNECT_IND;

    paramCount++;
    SapMsg.msg[4] = PARAMID_DISCONNECTION_TYPE;
    SapMsg.msg[5] = 0x00;
    SapMsg.msg[6] = 0x00;
    SapMsg.msg[7] = LENGTH_DISCONNECTION_TYPE;

    SapMsg.msg[8] = DISCONNECTTYPE_GRACEFUL;
    SapMsg.msg[9] = 0x00;
    SapMsg.msg[10] = 0x00;
    SapMsg.msg[11] = 0x00;

    SapMsg.msgSize = 12;

    SapMsg.msg[1]  = paramCount;
    le_event_Report(MessageEventId, &(SapMsg), sizeof(SapMsg));

}

void taf_simSap:: SendDisconnectResponse(ErrorCode errorCode) {
    SapState sapState;
    taf_SapMsg_t SapMsg;
    memset(&SapMsg, 0, sizeof(taf_SapMsg_t));
    uint8_t paramCount = 0;

    sapCardMgr->getState(sapState);
    if (errorCode == ErrorCode::SUCCESS) {
        SapMsg.msg[0]  = MSGID_DISCONNECT_RESP;
        SapMsg.msg[1]  = paramCount;
        SapMsg.msgSize = 4 ;
    } else {
        SendDisconnectInd();
    }
    le_event_Report(MessageEventId, &(SapMsg), sizeof(SapMsg));

}

void taf_simSap::SendPowerOnResponse(ErrorCode errorCode) {
    SapState sapState;
    taf_SapMsg_t SapMsg;
    memset(&SapMsg, 0, sizeof(taf_SapMsg_t));
    uint8_t paramCount = 0;
    SapMsg.msg[0]  = MSGID_POWER_SIM_ON_RESP;
    SapMsg.msg[2]  = 0x00;
    SapMsg.msg[3]  = 0x00;
    SapMsg.msg[4] = PARAMID_RESULT_CODE;
    SapMsg.msg[5] = 0x00;
    SapMsg.msg[6] = 0x00;
    SapMsg.msg[7] = LENGTH_RESULT_CODE;

    sapCardMgr->getState(sapState);
    if (errorCode == ErrorCode::SUCCESS) {
        paramCount = 1;

        SapMsg.msg[8] = RESULTCODE_OK;


    } else {
        SapMsg.msg[8] = RESULTCODE_ERROR_NO_REASON;
    }
    SapMsg.msg[1]  = paramCount;
    SapMsg.msg[9] = 0x00;
    SapMsg.msg[10] = 0x00;
    SapMsg.msg[11] = 0x00;

    SapMsg.msgSize = 12;
    le_event_Report(MessageEventId, &(SapMsg), sizeof(SapMsg));

}

void taf_simSap::SendPowerDownResponse(ErrorCode errorCode) {
    taf_SapMsg_t SapMsg;
    memset(&SapMsg, 0, sizeof(taf_SapMsg_t));
    uint8_t paramCount = 0;
    SapMsg.msg[0]  = MSGID_POWER_SIM_OFF_RESP;

    paramCount++;
    SapMsg.msg[4] = PARAMID_RESULT_CODE;
    SapMsg.msg[5] = 0x00;
    SapMsg.msg[6] = 0x00;
    SapMsg.msg[7] = LENGTH_RESULT_CODE;

    if (errorCode == ErrorCode::SUCCESS) {
        SapMsg.msg[8] = RESULTCODE_OK;
        cardpoweredOn = false;

    } else if (!cardpoweredOn) {
        SapMsg.msg[8] = RESULTCODE_ERROR_CARD_ON;
    }
    SapMsg.msg[1]  = paramCount;

    SapMsg.msgSize = 12;
    le_event_Report(MessageEventId, &(SapMsg), sizeof(SapMsg));
}

void taf_simSap::SendStatusInd(uint8_t status) {
    taf_SapMsg_t SapMsg;
    memset(&SapMsg, 0, sizeof(taf_SapMsg_t));
    SapMsg.msg[0]  = MSGID_STATUS_IND;
    SapMsg.msg[1]  = 0x01;

    SapMsg.msg[4] = PARAMID_STATUS_CHANGE;
    SapMsg.msg[5] = 0x00;
    SapMsg.msg[6] = 0x00;
    SapMsg.msg[7] = LENGTH_STATUS_CHANGE;

    SapMsg.msg[8]  = status;

    SapMsg.msgSize = 12;
    le_event_Report(MessageEventId, &(SapMsg), sizeof(SapMsg));
}

void taf_simSap::SendErrorResponse() {
    taf_SapMsg_t SapMsg;
    memset(&SapMsg, 0, sizeof(taf_SapMsg_t));
    SapMsg.msg[0]  = MSGID_ERROR_RESP;
    SapMsg.msg[1]  = 0x00;            //Parameter count

    SapMsg.msgSize = 4;
    le_event_Report(MessageEventId, &(SapMsg), sizeof(SapMsg));
}

void taf_simSap::SendAPDUResponse(const std::vector<int> &data, uint8_t apduId, ErrorCode errorCode) {

    taf_SapMsg_t SapMsg;
    memset(&SapMsg, 0, sizeof(taf_SapMsg_t));
    uint8_t paramCount = 0;
    uint16_t msgLength = 0;
    SapMsg.msg[0]  = MSGID_TRANSFER_APDU_RESP;

    paramCount++;
    SapMsg.msg[4] = PARAMID_RESULT_CODE;
    SapMsg.msg[5] = 0x00;
    SapMsg.msg[6] = 0x00;
    SapMsg.msg[7] = LENGTH_RESULT_CODE;
    msgLength = 12;

    if (errorCode == ErrorCode::SUCCESS) {
        paramCount++;

        SapMsg.msg[8] = RESULTCODE_OK;
        SapMsg.msg[9] = 0x00;
        SapMsg.msg[10] = 0x00;
        SapMsg.msg[11] = 0x00;

        SapMsg.msg[12] = apduId;
        SapMsg.msg[13] = 0x00;
        SapMsg.msg[14] = ((data.size() & 0xFF00U) >> MSB_SHIFT);
        SapMsg.msg[15] = (data.size() & 0x00FF);

        msgLength = 16;
        for (uint16_t i = 0; i < data.size(); i++) {
            SapMsg.msg[i + 16] = data[i];
            msgLength++;
        }
    } else {
        SapMsg.msg[8] = RESULTCODE_ERROR_NO_REASON;
    }
    SapMsg.msg[1]  = paramCount;

    msgLength = ((msgLength + 0x03) & (~0x03));
    SapMsg.msgSize = msgLength;
    le_event_Report(MessageEventId, &(SapMsg), sizeof(SapMsg));
}

void taf_simSap::SendResultCodeResponse(uint8_t msgId, uint8_t resultStatus) {
    taf_SapMsg_t SapMsg;
    memset(&SapMsg, 0, sizeof(taf_SapMsg_t));
    SapMsg.msg[0]  = msgId;
    SapMsg.msg[1]  = 0x01;

    SapMsg.msg[4] = PARAMID_RESULT_CODE;
    SapMsg.msg[5] = 0x00;
    SapMsg.msg[6] = 0x00;
    SapMsg.msg[7] = LENGTH_RESULT_CODE;

    SapMsg.msg[8]  = resultStatus;

    SapMsg.msgSize = 12;
    le_event_Report(MessageEventId, &(SapMsg), sizeof(SapMsg));
}

void taf_simSap::SendATRResponse(const std::vector<int> &responseAtr, ErrorCode errorCode) {
    SapState sapState;
    taf_SapMsg_t SapMsg;
    memset(&SapMsg, 0, sizeof(taf_SapMsg_t));
    uint8_t paramCount = 0;
    uint8_t msgLength = 0;
    SapMsg.msg[0]  = MSGID_TRANSFER_ATR_RESP;

    paramCount++;
    SapMsg.msg[4] = PARAMID_RESULT_CODE;
    SapMsg.msg[5] = 0x00;
    SapMsg.msg[6] = 0x00;
    SapMsg.msg[7] = LENGTH_RESULT_CODE;

    msgLength = 12;
    sapCardMgr->getState(sapState);
    if (errorCode == ErrorCode::SUCCESS) {
        if (responseAtr.size() < 0) {
            SapMsg.msg[8] = RESULTCODE_ERROR_NO_DATA;
        } else {
            paramCount++;

            SapMsg.msg[8] = RESULTCODE_OK;

            SapMsg.msg[12] = PARAMID_ATR;
            SapMsg.msg[13] = 0x00;
            SapMsg.msg[14] = ((responseAtr.size() & 0xFF00U) >> MSB_SHIFT);
            SapMsg.msg[15] = (responseAtr.size() & 0x00FF);

            msgLength = 16;
            for (uint8_t i = 0; i < responseAtr.size(); i++) {
                SapMsg.msg[i + 16] = responseAtr[i];
                msgLength++;
            }
        }

    } else {
        SapMsg.msg[8] = RESULTCODE_ERROR_NO_REASON;

    }
    SapMsg.msg[1]  = paramCount;
    msgLength = ((msgLength + 0x03) & (~0x03));

    SapMsg.msgSize = msgLength;
    le_event_Report(MessageEventId, &(SapMsg), sizeof(SapMsg));
}

void taf_simSap::SendCardResetResponse(ErrorCode errorCode) {
    taf_SapMsg_t SapMsg;
    memset(&SapMsg, 0, sizeof(taf_SapMsg_t));
    SapMsg.msg[0]  = MSGID_RESET_SIM_RESP;
    SapMsg.msg[1]  = 0x01;

    SapMsg.msg[4] = PARAMID_RESULT_CODE;
    SapMsg.msg[5] = 0x00;
    SapMsg.msg[6] = 0x00;
    SapMsg.msg[7] = LENGTH_RESULT_CODE;

    if (errorCode == ErrorCode::SUCCESS) {
        SapMsg.msg[8] = RESULTCODE_OK;
    } else if (cardpoweredOn) {
        SapMsg.msg[8] = RESULTCODE_ERROR_CARD_ON;
    }

    SapMsg.msgSize = 12;
    le_event_Report(MessageEventId, &(SapMsg), sizeof(SapMsg));
}

void taf_simSap::SendCardReaderResponse(ErrorCode errorCode, CardReaderStatus readerStatus) {

    SapState sapState;
    taf_SapMsg_t SapMsg;
    memset(&SapMsg, 0, sizeof(taf_SapMsg_t));
    uint8_t paramCount = 0;
    uint8_t msgLength = 0;
    SapMsg.msg[0]  = MSGID_TRANSFER_CARD_READER_STATUS_RESP;
    SapMsg.msg[2]  = 0x00;
    SapMsg.msg[3]  = 0x00;

    paramCount++;
    SapMsg.msg[4] = PARAMID_RESULT_CODE;
    SapMsg.msg[5] = 0x00;
    SapMsg.msg[6] = 0x00;
    SapMsg.msg[7] = LENGTH_RESULT_CODE;

    msgLength = 12;
    uint8_t cardReaderStatus = GetCardReaderStatus(readerStatus);
    sapCardMgr->getState(sapState);
    if (errorCode == ErrorCode::SUCCESS) {
        SapMsg.msg[8] = RESULTCODE_OK;

            paramCount++;
            SapMsg.msg[12] = PARAMID_CARD_READER_STATUS;
            SapMsg.msg[13] = 0x00;
            SapMsg.msg[14] = 0x00;
            SapMsg.msg[15] = LENGTH_CARD_READER_STATUS;

            SapMsg.msg[16] = cardReaderStatus; //Card reader status

            msgLength = 20;

    } else {
        SapMsg.msg[8] = RESULTCODE_ERROR_NO_DATA;
    }
    SapMsg.msg[1]  = paramCount;

    SapMsg.msgSize = msgLength;
    le_event_Report(MessageEventId, &(SapMsg), sizeof(SapMsg));

}

uint8_t taf_simSap::GetCardReaderStatus(CardReaderStatus readerStatus) {
    uint8_t id =  readerStatus.id;
    uint8_t isRemovable =  readerStatus.isRemovable;
    uint8_t isPresent =  readerStatus.isPresent;
    uint8_t isID1size =  readerStatus.isID1size;
    uint8_t isCardPresent = readerStatus.isCardPresent;
    uint8_t isCardPoweredOn = readerStatus.isCardPoweredOn;

    uint8_t cardReaderStatus = ((id << CARD_READER_ID) | (isRemovable << CARD_READER_REMOVABLE_SHIFT)
                                | (isPresent << CARD_READER_PRESENT_SHIFT) | (isID1size << CARD_READER_ID1_SIZE_SHIFT)
                                | (isCardPresent << CARD_PRESENT_SHIFT) | (isCardPoweredOn << CARD_POWER_ON_SHIFT));
    return cardReaderStatus;
}

le_result_t taf_simSap::VerifyMessageLength(const uint8_t* buf, size_t size, uint8_t numberOfParam){
    uint8_t minMsgLength =  LENGTH_SAP_HEADER + numberOfParam * LENGTH_PARAM;
    TAF_ERROR_IF_RET_VAL(size < minMsgLength, LE_FAULT,
            " Msg length is too short!\n");
    return LE_OK;
}

le_result_t taf_simSap::VerifyParameterCount(const uint8_t* buf, uint8_t numberOfParam){
    TAF_ERROR_IF_RET_VAL(buf[1] < numberOfParam, LE_FAULT,
            " Number of parameters are less!\n");
    return LE_OK;
}

le_result_t taf_simSap::VerifyParamId(const uint8_t* buf,uint8_t paramId, uint8_t parameter) {

    int paramIdByte = LENGTH_SAP_HEADER + (parameter - 1) * LENGTH_PARAM;
    uint8_t Id = buf[paramIdByte];
    TAF_ERROR_IF_RET_VAL(Id != paramId, LE_FAULT,
            " Invalid parameter identifier");
    return LE_OK;

}

le_result_t taf_simSap::VerifyParamLength(const uint8_t* buf, uint16_t paramLength, uint8_t parameter) {

    uint8_t lengthByte1 = LENGTH_SAP_HEADER + 2 + ((parameter-1) * LENGTH_PARAM);
    uint8_t lengthByte2 = lengthByte1 + 1;
    uint16_t length = (uint16_t)( ((uint16_t)(buf[lengthByte1] << MSB_SHIFT)) | buf[lengthByte2]);
    TAF_ERROR_IF_RET_VAL(length != paramLength, LE_FAULT,
            " Invalid parameter length");
    return LE_OK;
}

le_result_t taf_simSap::VerifyMaxMsgSizeParameter(const uint8_t* buf, size_t size) {
    uint8_t paramId = PARAMID_MAX_MSG_SIZE;
    uint8_t paramLength = LENGTH_MAX_MSG_SIZE;
    uint8_t nParam = 1;
    if (LE_OK != VerifyParamId(buf, paramId, nParam)) {
        return LE_FAULT;
    }
    return VerifyParamLength(buf, paramLength, nParam);
}

le_result_t taf_simSap::VerifyConnectRequest(const uint8_t* msg, uint8_t msgLength) {
    //Check message length
    if(VerifyMessageLength(msg, msgLength, 1) != LE_OK) {
        return LE_FAULT;
    }

    //Check number of parameters
    if (VerifyParameterCount(msg, 1) != LE_OK) {
        return LE_FAULT;
    }

    //Check PARAM ID is MAXMSG_SIZE or not
    //Check parameter length
    if (VerifyMaxMsgSizeParameter(msg, msgLength) != LE_OK) {
        return LE_FAULT;
    }
    return LE_OK;

}

void taf_simSap::EraseFromApduRespCbMap(uint8_t apduId)
{
    apduRespCbMap.erase(apduId);
}
