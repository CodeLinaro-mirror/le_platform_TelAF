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
#include "legato.h"
#include "interfaces.h"
#include <telux/tel/PhoneFactory.hpp>
#include "telux/common/CommonDefines.hpp"
#include "tafSvcIF.hpp"
#include <unordered_map>

/**
 * Message id to establish SAP connection
 */
#define MSGID_CONNECT_REQ                         0x00
#define MSGID_CONNECT_RESP                        0x01

/**
 * Message id to  terminate SAP connection
 */
#define MSGID_DISCONNECT_REQ                      0x02
#define MSGID_DISCONNECT_RESP                     0x03
#define MSGID_DISCONNECT_IND                      0x04

/**
 * Message id to tranfer APDU between server and client
 */
#define MSGID_TRANSFER_APDU_REQ                   0x05
#define MSGID_TRANSFER_APDU_RESP                  0x06

/**
 * Message id to content of ATR between server and client
 */
#define MSGID_TRANSFER_ATR_REQ                    0x07
#define MSGID_TRANSFER_ATR_RESP                   0x08

/**
 * Message id to power off the subscribtion module remotely
 */
#define MSGID_POWER_SIM_OFF_REQ                   0x09
#define MSGID_POWER_SIM_OFF_RESP                  0x0A

/**
 * Message id to power on the subscribtion module remotely
 */
#define MSGID_POWER_SIM_ON_REQ                    0x0B
#define MSGID_POWER_SIM_ON_RESP                   0x0C

/**
 * Message id to reset the SIM remotely
 */
#define MSGID_RESET_SIM_REQ                       0x0D
#define MSGID_RESET_SIM_RESP                      0x0E

/**
 * Message id to send card reader status
 */
#define MSGID_TRANSFER_CARD_READER_STATUS_REQ     0x0F
#define MSGID_TRANSFER_CARD_READER_STATUS_RESP    0x10

/**
 * Message id to indicate status of physical sim to client
 */
#define MSGID_STATUS_IND                          0x11


/**
 * Message id to handle invalid formatted messages
 */
#define MSGID_ERROR_RESP                          0x12

/**
 * Message id to request to use another transport protocol and get
 * response
 */
#define MSGID_SET_TRANSPORT_PROTOCOL_REQ          0x13
#define MSGID_SET_TRANSPORT_PROTOCOL_RESP         0x14

/**
 * Parameters identifiers - SAP specification
 */
#define PARAMID_MAX_MSG_SIZE            0x00    // To negotiate maximum message size between server and client.
#define PARAMID_CONNECTION_STATUS       0x01    // To inticate status of connection.
#define PARAMID_RESULT_CODE             0x02    // To indicate errors or success of requested messages.
#define PARAMID_DISCONNECTION_TYPE      0x03    // To indicate type of disconnection graceful or immediate.
#define PARAMID_COMMAND_APDU            0x04    // C-APDU coded in accordance to GSM 11.11 specification.
#define PARAMID_COMMAND_APDU_7816       0x10    // C-APDU coded in accordance to ISO/IEC 7816-4.
#define PARAMID_RESPONSE_APDU           0x05    // APDU response parameter.
#define PARAMID_ATR                     0x06    // ATR coded as described in ISO/IEC 7816-3 specification.
#define PARAMID_CARD_READER_STATUS      0x07    // Card reader status as described in the GSM 11.14 specification.
#define PARAMID_STATUS_CHANGE           0x08    // Reason for change in the status of SIM.
#define PARAMID_TRANSPORT_PROTOCOL      0x09    // Contains ID for protocol which is used between client and server.

/**
 *  Each parameter length in bytes.
 */
#define LENGTH_SAP_HEADER                4
#define LENGTH_PARAM_HEADER              4
#define LENGTH_MAX_MSG_SIZE              2
#define LENGTH_CONNECTION_STATUS         1
#define LENGTH_RESULT_CODE               1
#define LENGTH_DISCONNECTION_TYPE        1
#define LENGTH_CARD_READER_STATUS        1
#define LENGTH_STATUS_CHANGE             1
#define LENGTH_TRANSPORT_PROTOCOL        1
#define LENGTH_PARAM_PAYLOAD             4
#define LENGTH_PARAM                     (LENGTH_PARAM_HEADER + LENGTH_PARAM_PAYLOAD)

/**
 *  Status of connection
 */
#define CONNSTATUS_OK               0x00
#define CONNSTATUS_SERVER_NOK       0x01
#define CONNSTATUS_MAXMSGSIZE_NOK   0x02
#define CONNSTATUS_SMALL_MAXMSGSIZE 0x03
#define CONNSTATUS_OK_ONGOING_CALL  0x04

/**
 *  Type of disconnection i.e graceful or immediate
 */
#define DISCONNECTTYPE_GRACEFUL        0x00
#define DISCONNECTTYPE_IMMEDIATE       0x01

/**
 *  Result code
 */
#define RESULTCODE_OK                   0x00
#define RESULTCODE_ERROR_NO_REASON      0x01
#define RESULTCODE_ERROR_CARD_NOK       0x02
#define RESULTCODE_ERROR_CARD_OFF       0x03
#define RESULTCODE_ERROR_CARD_REMOVED   0x04
#define RESULTCODE_ERROR_CARD_ON        0x05
#define RESULTCODE_ERROR_NO_DATA        0x06
#define RESULTCODE_ERROR_NOT_SUPPORTED  0x07

/**
 * Change in status of SIM
 */
#define STATUSCHANGE_UNKNOWN_ERROR    0x00
#define STATUSCHANGE_CARD_RESET       0x01
#define STATUSCHANGE_CARD_NOK         0x02
#define STATUSCHANGE_CARD_REMOVED     0x03
#define STATUSCHANGE_CARD_INSERTED    0x04
#define STATUSCHANGE_CARD_RECOVERED   0x05

#define MSB_SHIFT                   8
#define CARD_POWER_ON_SHIFT         0
#define CARD_PRESENT_SHIFT          1
#define CARD_READER_ID1_SIZE_SHIFT  2
#define CARD_READER_PRESENT_SHIFT   3
#define CARD_READER_REMOVABLE_SHIFT 4
#define CARD_READER_ID              5

using namespace telux::tel;
using namespace telux::common;
using namespace std;

namespace telux {
    namespace tafsvc {
        typedef struct
        {
            uint8_t msg[TAF_SAP_MAX_MSG_SIZE];
            size_t  msgSize;
        } taf_SapMsg_t;

        class tafOpenConnectionCallback : public telux::common::ICommandResponseCallback {
            public:
                void commandResponse(telux::common::ErrorCode errorCode) override;
        };

        class tafCloseConnectionCallback : public telux::common::ICommandResponseCallback {
            public:
                void commandResponse(telux::common::ErrorCode errorCode) override;
        };
        class tafPowerOnCallback : public telux::common::ICommandResponseCallback {
            public:
                void commandResponse(telux::common::ErrorCode errorCode) override;
        };

        class tafPowerOffCallback : public telux::common::ICommandResponseCallback {
            public:
                void commandResponse(telux::common::ErrorCode errorCode) override;
        };

        class tafResetCallback : public telux::common::ICommandResponseCallback {
            public:
                void commandResponse(telux::common::ErrorCode errorCode) override;
        };

        class tafApduResponseCallback : public telux::tel::ISapCardCommandCallback {
            public:
                tafApduResponseCallback(uint8_t apduId);

                void onResponse(telux::tel::IccResult result, telux::common::ErrorCode error) override;
                void setApduId(uint8_t apdu) {
                    apduId = apdu;
                }
                uint8_t getApduId(){
                     return apduId;
                }

            private:
                uint8_t apduId;
        };

        class tafAtrResponseCallback : public telux::tel::IAtrResponseCallback {
            public:
                void atrResponse(std::vector<int> responseAtr, telux::common::ErrorCode error) override;

        };

        class tafCardReaderCallback : public telux::tel::ICardReaderCallback {
            public:
                void cardReaderResponse(CardReaderStatus cardReaderStatus, telux::common::ErrorCode error) override;
        };

        class taf_sap :public ITafSvc {
            public:
                taf_sap() {};
                ~taf_sap() {};
                void Init(void);
                static taf_sap &GetInstance();
                bool cardConnected = false;
                bool cardpoweredOn = true;
                taf_sap_MessageHandlerRef_t AddMessageHandler(taf_sap_MessageHandlerFunc_t handlerPtr,
                        void* contextPtr);
                void RemoveMessageHandler( taf_sap_MessageHandlerRef_t handlerRef);
                le_result_t ProcessSAPRequestMessages(const uint8_t *buf, size_t msgLength);

                void SendConnectResponse(ErrorCode errorCode, uint8_t connectStatus);
                void SendDisconnectResponse(ErrorCode errorCode);
                void SendPowerOnResponse(ErrorCode errorCode);
                void SendPowerDownResponse(ErrorCode errorCode);
                void SendStatusInd(uint8_t status);
                void SendErrorResponse();
                void SendAPDUResponse(const std::vector<int> &data, uint8_t apduId, ErrorCode errorCode);
                void SendATRResponse(const std::vector<int> &responseAtr, ErrorCode errorCode);
                void SendCardResetResponse(ErrorCode errorCode);
                void SendCardReaderResponse(ErrorCode errorCode, CardReaderStatus readerStatus);
                void SendDisconnectInd();
                void EraseFromApduRespCbMap(uint8_t apduId);

            private:
                std::shared_ptr<telux::tel::ISapCardManager> sapCardMgr;
                le_thread_Ref_t MainThread;
                le_event_Id_t MessageEventId;

                taf_sim_Id_t slotId;

                static void FirstLayerMessageHandler( void* reportPtr, void* secondLayerHandlerFunc);
                static void eventCallback(ErrorCode errorCode);
                static void HandleClientMsg(void* param1Ptr, void* param2Ptr);

                std::shared_ptr<tafOpenConnectionCallback> openConnCb;
                std::shared_ptr<tafCloseConnectionCallback> closeConnCb;
                std::shared_ptr<tafPowerOnCallback> powerOnCb;
                std::shared_ptr<tafPowerOffCallback> powerOffCb;
                std::shared_ptr<tafResetCallback> resetCb;
                std::shared_ptr<tafAtrResponseCallback> atrResetRespCb;
                std::shared_ptr<tafCardReaderCallback> cardReaderCb;
                std::unordered_map<uint8_t, std::shared_ptr<tafApduResponseCallback>> apduRespCbMap;

                le_result_t OpenSAPConnection(const uint8_t* buf, uint8_t msgLength);
                le_result_t DisconnectFromCard();
                le_result_t RequestPowerOn();
                le_result_t RequestPowerOff();
                le_result_t ResetCard();
                le_result_t SendApduToSim(const uint8_t *buf, int bytes);
                le_result_t RequestAtrAfterReset();
                le_result_t RequestCardReaderStatus();
                void SendResultCodeResponse(uint8_t msgId, uint8_t resultStatus);

                uint8_t GetCardReaderStatus(CardReaderStatus readerStatus);

                le_result_t VerifyParameterCount(const uint8_t* buf, uint8_t numberOfParam);
                le_result_t VerifyMessageLength(const uint8_t* buf, size_t size, uint8_t numberOfParam);
                le_result_t VerifyParamId(const uint8_t* buf,uint8_t paramId, uint8_t parameter);
                le_result_t VerifyParamLength(const uint8_t* buf, uint16_t paramLength, uint8_t parameter);
                le_result_t VerifyMaxMsgSizeParameter(const uint8_t* buf, size_t size);
                le_result_t VerifyConnectRequest(const uint8_t* msg, uint8_t msgLength);
                static void NewSimStateHandler( taf_sim_Id_t simId, taf_sim_States_t simState,
                                void* contextPtr);


        };
    }
}

