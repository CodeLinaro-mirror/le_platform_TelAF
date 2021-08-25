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

#define MSB_SHIFT   8
#define MSG_POOL_SIZE 2

using namespace telux::tel;
using namespace telux::common;

namespace telux {
    namespace tafsvc {
        typedef enum
        {
            SAP_STATE_NOT_CONNECTED = 0,
            SAP_STATE_CONNECTING,
            SAP_STATE_CONNECTED
        } taf_SapState_t;

        typedef enum
        {
            SAP_CONNECTED_IDLE = 0,
            SAP_CONNECTED_APDU,
            SAP_CONNECTED_ATR_RESET,
            SAP_CONNECTED_ATR_INSERT,
            SAP_CONNECTED_RESET,
            SAP_CONNECTED_POWER_OFF,
            SAP_CONNECTED_POWER_ON,
            SAP_CONNECTED_DISCONNECT
        }taf_SapSubState_t;

        typedef struct
        {
            uint8_t msg[TAF_RSIM_MAX_MSG_SIZE];
            size_t  msgSize;
        } taf_RsimMsg_t;

        typedef struct
        {
            taf_rsim_MessageHandlerRef_t  handlerRef;
            taf_SapState_t                sapState;
            taf_SapSubState_t             sapSubState;
            uint16_t                      maxMsgSize;
        } taf_RsimObj_t;

        typedef struct
        {
            taf_RsimMsg_t message;
            taf_rsim_CallbackHandlerFunc_t callbackRef;
            void* context;
        } taf_RsimMsg_Client_t;

        class tafRemoteSimListener : public telux::tel::IRemoteSimListener {
            public:
                void onApduTransfer(const unsigned int id, const std::vector<uint8_t> &apdu) override;

                void onCardConnect() override;

                void onCardDisconnect() override;

                void onCardPowerUp() override;

                void onCardPowerDown() override;

                void onCardReset() override;

                void onServiceStatusChange(telux::common::ServiceStatus status) override;
        };

        class taf_rsim :public ITafSvc {
            public:
                taf_rsim() {};
                ~taf_rsim() {};
                void Init(void);
                static taf_rsim &GetInstance();
                taf_rsim_MessageHandlerRef_t AddMessageHandler(taf_rsim_MessageHandlerFunc_t handlerPtr,
                        void* contextPtr);
                void RemoveMessageHandler( taf_rsim_MessageHandlerRef_t handlerRef);
                le_result_t SendMessage(const uint8_t* messagePtr, size_t messageNumElements,
                        taf_rsim_CallbackHandlerFunc_t callbackPtr, void* contextPtr);

                le_result_t SendApduRequest(const unsigned int id, const std::vector<uint8_t> &apdu);
                le_result_t SendCardConnectRequest();
                le_result_t SendCardDisconnectRequest();
                le_result_t SendCardPowerUpRequest();
                le_result_t SendCardPowerDownRequest();
                le_result_t SendCardResetRequest();
                void NotifyConnectionAvailable();
                void NotifyConnectionUnavailable();
            private:
                std::shared_ptr<telux::tel::IRemoteSimManager> remoteSimMgr;
                std::shared_ptr<telux::tel::IRemoteSimListener> listener;

                le_thread_Ref_t MainThread;
                le_event_Id_t MessageEventId;
                taf_RsimObj_t RsimObj;

                static void FirstLayerMessageHandler( void* reportPtr, void* secondLayerHandlerFunc);
                static void eventCallback(ErrorCode errorCode);
                static void HandleClientMsg(void* param1Ptr, void* param2Ptr);

                le_result_t VerifyConnectionParameter(const uint8_t* buf, size_t size);
                le_result_t VerifyMaxMsgSizeParameter(const uint8_t* buf, size_t size);
                le_result_t VerifyParameterCount(const uint8_t* buf, uint8_t numberOfParam);
                le_result_t VerifyMessageLength(const uint8_t* buf, size_t size, uint8_t numberOfParam);
                le_result_t VerifyResultCodeParameter(const uint8_t* buf, size_t size);
                le_result_t VerifyDisconnectParameter(const uint8_t* buf, size_t size);
                le_result_t VerifyAPDUParameter(const uint8_t* buf, size_t size);
                le_result_t VerifyStatusChangeParameter(const uint8_t* buf, size_t size);
                le_result_t VerifyATRParameter(const uint8_t* buf, size_t size);
                le_result_t VerifyParamId(const uint8_t* buf,uint8_t paramId, uint8_t parameter);
                le_result_t VerifyParamLength(const uint8_t* buf, uint16_t paramLength, uint8_t parameter);
                le_result_t HandleApduTransfer(const uint8_t* messagePtr, size_t messageNumElements);
                le_result_t SendApduResp(const uint8_t* msgPtr, size_t msgSize);
                le_result_t HandleATRResponse(const uint8_t* msgPtr, size_t messageNumElements);
                le_result_t HandleCardConnect(const uint8_t* messagePtr, size_t messageNumElements);
                le_result_t HandleCardDisconnectInd(const uint8_t* msgPtr, size_t msgSize);
                le_result_t HandleCardDisconnectResp(const uint8_t* messagePtr, size_t messageNumElements);
                le_result_t HandleCardPowerDown(const uint8_t* messagePtr, size_t messageNumElements);
                le_result_t HandleCardPowerUp(const uint8_t* messagePtr, size_t messageNumElements);
                le_result_t HandleCardReset(const uint8_t* messagePtr, size_t messageNumElements);
                le_result_t HandleCardInserted(const uint8_t* messagePtr, size_t messageNumElements);
                le_result_t HandleCardRemoved();
                le_result_t HandleCardError(CardErrorCause cardError);
                le_result_t HandleCardWakeUp();
                le_result_t HandleStatusInd(const uint8_t* msgPtr, size_t messageNumElements);
                le_result_t HandleResetResponse(const uint8_t* msgPtr, size_t msgLength);
                le_result_t SendATRRequest(taf_SapSubState_t subState);
                le_result_t HandleErrorResp();

                le_mem_PoolRef_t RSimMsgPool = NULL;

        };
    }
}

