/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFUDS_IDPS_HPP
#define TAFUDS_IDPS_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafDoIPStack.h"
#include "tafUDSStack.h"
#include "tafUDSCommunicationMgr.hpp"

using namespace tafsvc;

namespace taf{
namespace uds{

#define  TAF_IDPS_HANDLER_REF_CNT 3

    // IDPS message event.
    typedef struct {
        uint16_t sa;
        uint16_t ta;
        uint16_t vlanId;
        uint8_t sid;
        uint8_t status;
        uint8_t dataBuf[MAX_IDPS_DATA_LEN]; ///< Partial recvBuf data is needed
        uint8_t dataLen;
    } tafIdpsMsg_t;

    // IDPS callback info event.
    typedef struct {
        taf_uds_IdpsAddrInfo_t idpsAddrInfo;
        taf_uds_IdpsStatusInfo_t idpsStatusInfo;
    } tafIdpsCallbackInfo_t;

    // UDS stack indication handler structure.
    typedef struct
    {
        taf_uds_IdpsIndicationHandlerFunc_t  funcPtr;
        void*                                ctxPtr;
        void*                                safeRef;
    }taf_udsIdpsHandler_t;

    class UdsIdps{
        public:
            UdsIdps() {};
            ~UdsIdps() {};
            void Init();
            static UdsIdps& GetInstance();

            void SendIdpsIndMsg(const uint8_t* sendBuf, uint16_t sendDataLen,
                const uint8_t* recvBuf, uint16_t recvDataLen, taf_doip_AddrInfo_t* addrInfoPtr);

            static le_event_Id_t IdpsEventId;
            static le_event_Id_t IdpsCbEventId;
            static le_ref_MapRef_t udsIdpsHandlerRefMap;
            static taf_udsIdpsHandler_t idpsIndicationHandler;
            le_thread_Ref_t udsIdpsThRef = NULL;
        private:
            static void* UdsIdpsThread(void* ctxPtr);
            static void IdpsHandler(void* reqPtr);
            static void IdpsCallbackHandler(void* reqPtr);

    };
}
}
#endif  // TAFUDS_IDPS_HPP
