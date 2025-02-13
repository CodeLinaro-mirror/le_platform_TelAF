/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef TAF_ROE_SERVER_HPP
#define TAF_ROE_SERVER_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafDiagBackend.hpp"

#define MAX_ROE_RESP_LEN        4092
#define DEFAULT_RX_MSG_REF_CNT  16
#define ROE_EVENT_MASK          0x3F
#define ROE_STORAGE_STATE_BIT   6

#define MAX_ROE_EVENT_TYPE_RECORD   10
#define MAX_ROE_RESPOND_TO_RECORD   3

#define MIN_ROE_ON_DTC_STATUS_CHANGE    7

typedef enum
{
    STOP_RESPONSE_ON_EVENT = 0x00,      ///< stopResponseOnEvent.
    ON_DTC_STATUS_CHANGE = 0x01,        ///< onDTCStatusChange.
    ON_CHANGE_OF_DID = 0x03,            ///< onChangeOfDataIdentifier.
    REPORT_ACTIVATED_EVENTS = 0x04,     ///< reportActivatedEvents.
    START_RESPONSE_ON_EVENT = 0x05,     ///< startResponseOnEvent.
    CLEAR_RESPONSE_ON_EVENT = 0x06,     ///< clearResponseOnEvent.
    ON_COMPARISON_OF_VALUES = 0x07,     ///< onComparisonOfValues.
    REPORT_MOST_RECENT_DTC = 0x08,      ///< reportMostRecentDtcOnStatusChange.
    REPORT_DTC_RECORD_INFO = 0x09       ///< reportDTCRecordInformationOnDtcStatusChange.
}taf_ROE_EventType_t;

typedef struct
{
    void *rxMsgRef;                     ///< Own reference.
    taf_uds_AddrInfo_t addrInfo;        ///< Rx logical address information structure.
    uint8_t subFunc;                    ///< Rx subFunction.
    uint8_t evWinTime;                  ///< Event window time
    le_dls_Link_t link;                 ///< Link to the Rx message list.
    uint8_t evTypeRec[MAX_ROE_EVENT_TYPE_RECORD];
    uint8_t evTypeRecLen;
    uint8_t respToRec[MAX_ROE_RESPOND_TO_RECORD];
    uint8_t respToRecLen;
    uint8_t resp[MAX_ROE_RESP_LEN];
    size_t respLen;
}taf_ROERxMsg_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag ResponseOnEvent Server Service Class
 */
//--------------------------------------------------------------------------------------------------
    namespace tafsvc
    {
        class taf_ROESvr : public ITafSvc, public taf_UDSInterface
        {
            public:
                taf_ROESvr() {};
                ~taf_ROESvr() {};
                void Init();
                static taf_ROESvr &GetInstance();

                // UDS message handler.
                void UDSMsgHandler(const taf_uds_AddrInfo_t* addrPtr, uint8_t sid, uint8_t* msgPtr,
                        size_t msgLen) override;
                static void RxROEEventHandler(void* reportPtr);

            private:
                void SendNRCResp(uint8_t sid, const taf_uds_AddrInfo_t* addrInfoPtr,
                        uint8_t errCode);
                le_result_t SendResp(taf_ROERxMsg_t* rxMsgPtr, uint8_t errCode);

                le_dls_List_t rxMsgList;

                uint8_t reqSvcId = 0x86;   // ResponseOnEvent request service ID.
                uint8_t respSvcId = 0xC6;  // ResponseOnEvent response service ID.

                // Rx message resource
                le_mem_PoolRef_t RxMsgPool;
                le_ref_MapRef_t RxMsgRefMap;

                // Event for service.
                le_event_Id_t ReqEvent;
                le_event_HandlerRef_t ReqEventHandlerRef;
        };
    }
#endif /* TAF_ROE_SERVER_HPP */
