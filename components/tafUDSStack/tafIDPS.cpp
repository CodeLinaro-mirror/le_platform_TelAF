/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafIDPS.hpp"
#include "configuration.hpp"

// Minimum UDS response length: SID (1 byte)
#define UDS_RESP_MIN_LEN        1
// Negative response length: 0x7F + SID + NRC (3 bytes)
#define UDS_NEG_RESP_LEN        3
// Negative response SID
#define UDS_NEGATIVE_RESP_SID   0x7F

using namespace tafsvc;

namespace taf {
namespace uds {
le_event_Id_t UdsIdps::IdpsEventId = NULL;
le_event_Id_t UdsIdps::IdpsCbEventId = NULL;
le_ref_MapRef_t UdsIdps::udsIdpsHandlerRefMap = NULL;
taf_udsIdpsHandler_t UdsIdps::idpsIndicationHandler;


//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of TelAF IDPS.
 */
//--------------------------------------------------------------------------------------------------
UdsIdps &UdsIdps::GetInstance
(
)
{
    static UdsIdps instance;

    return instance;
}

// The IDPS handler which runs in IDPS thread
void UdsIdps::IdpsHandler
(
    void* reqPtr
)
{
    tafIdpsMsg_t* idpsPtr = (tafIdpsMsg_t*)reqPtr;
    TAF_ERROR_IF_RET_NIL(idpsPtr == NULL, "Null pointer");
    tafIdpsCallbackInfo_t idpsCallbackInfo = {};

    //Positive response, fill the param buffer
    if(idpsPtr->status == 0)
    {
        switch (idpsPtr->sid)
        {
            case ECU_RESET_REQUEST_ID:  // 0x11
            {
                if(idpsPtr->dataLen < UDS_ECU_RESET_REQ_MIN_LEN)
                {
                    LE_ERROR("data length is small");
                    return;
                }

                //param1 = sub function
                idpsCallbackInfo.idpsStatusInfo.data[0] = idpsPtr->dataBuf[1];
                idpsCallbackInfo.idpsStatusInfo.dataLen = 1;
            }
            break;
            case SECURITY_ACCESS_REQUEST_ID:  // 0x27
            {
                if(idpsPtr->dataLen < UDS_SECURITY_ACCESS_REQ_MIN_LEN)
                {
                    LE_ERROR("data length is small");
                    return;
                }

                uint8_t securityAccessType = idpsPtr->dataBuf[1] & 0x7F;
                //Not Sendkey, no need to report
                if (securityAccessType % 2 != 0)
                    return;

                //param1 = sub function
                idpsCallbackInfo.idpsStatusInfo.data[0] = idpsPtr->dataBuf[1];
                idpsCallbackInfo.idpsStatusInfo.dataLen = 1;

            }
            break;
            case WRITE_DID_REQUEST_ID:  // 0x2E
            {
                if(idpsPtr->dataLen < UDS_WRITE_DID_REQ_MIN_LEN)
                {
                    LE_ERROR("data length is small");
                    return;
                }

                //param1 = did
                idpsCallbackInfo.idpsStatusInfo.data[0] = idpsPtr->dataBuf[1];
                idpsCallbackInfo.idpsStatusInfo.data[1] = idpsPtr->dataBuf[2];
                idpsCallbackInfo.idpsStatusInfo.dataLen = 2;
            }
            break;
            case INPUT_OUTPUT_CONTROL_REQUEST_ID:  // 0x2F
            {
                if(idpsPtr->dataLen < UDS_IOCBID_REQ_MIN_LEN)
                {
                    LE_ERROR("data length is small");
                    return;
                }

                //param1 = did
                idpsCallbackInfo.idpsStatusInfo.data[0] = idpsPtr->dataBuf[1];
                idpsCallbackInfo.idpsStatusInfo.data[1] = idpsPtr->dataBuf[2];
                idpsCallbackInfo.idpsStatusInfo.dataLen = 2;

                //param2 = byte 4 request
                idpsCallbackInfo.idpsStatusInfo.extraData[0] = idpsPtr->dataBuf[3];
                idpsCallbackInfo.idpsStatusInfo.extraDataLen = 1;
            }
            break;
            case ROUTINE_CONTROL_REQUEST_ID: // 0x31
            {
                if(idpsPtr->dataLen < UDS_ROUTINE_CTRL_REQ_MIN_LEN)
                {
                    LE_ERROR("data length is small");
                    return;
                }

                //param1 = sub function
                idpsCallbackInfo.idpsStatusInfo.data[0] = idpsPtr->dataBuf[1];
                idpsCallbackInfo.idpsStatusInfo.dataLen = 1;

                //param2 = routine id
                idpsCallbackInfo.idpsStatusInfo.extraData[0] = idpsPtr->dataBuf[2];
                idpsCallbackInfo.idpsStatusInfo.extraData[1] = idpsPtr->dataBuf[3];
                idpsCallbackInfo.idpsStatusInfo.extraDataLen = 2;
            }
            break;
            case REQUEST_TRANSFER_EXIT_REQUEST_ID:  // 0x37
                //No data needed
            break;
            case REQUEST_FILE_TRANSFER_REQUEST_ID:  // 0x38
            {
                if(idpsPtr->dataLen < RFT_BASE_LEN)
                {
                    LE_ERROR("data length is small");
                    return;
                }
                //param1 modeOfOperation
                idpsCallbackInfo.idpsStatusInfo.data[0] = idpsPtr->dataBuf[1];
                idpsCallbackInfo.idpsStatusInfo.dataLen = 1;
            }
            break;
            default:
                return;
        }
    }
    else //NRC, fill the param1 buffer for WDBI
    {
        // Send DID with NRC for WDBI
        if(idpsPtr->sid == WRITE_DID_REQUEST_ID)  // 0x2E
        {
            if(idpsPtr->dataLen < UDS_WRITE_DID_REQ_MIN_LEN)
            {
                LE_ERROR("data length is small");
                return;
            }

            //param1 = did
            idpsCallbackInfo.idpsStatusInfo.data[0] = idpsPtr->dataBuf[1];
            idpsCallbackInfo.idpsStatusInfo.data[1] = idpsPtr->dataBuf[2];
            idpsCallbackInfo.idpsStatusInfo.dataLen = 2;
        }
        else
        {
            // For other SIDs, only report NRC if they are in the monitored set
            switch (idpsPtr->sid)
            {
                case ECU_RESET_REQUEST_ID:
                case SECURITY_ACCESS_REQUEST_ID:
                case AUTHENTICATION_REQUEST_ID:
                case INPUT_OUTPUT_CONTROL_REQUEST_ID:
                case ROUTINE_CONTROL_REQUEST_ID:
                case REQUEST_TRANSFER_EXIT_REQUEST_ID:
                case REQUEST_FILE_TRANSFER_REQUEST_ID:
                case REQUEST_DOWNLOAD_REQUEST_ID:
                case REQUEST_UPLOAD_REQUEST_ID:
                case READ_MEMORY_BY_ADDR_REQUEST_ID:
                case WRITE_MEMORY_BY_ADDR_REQUEST_ID:
                case DYNAMICALLY_DEFINE_DID_REQUEST_ID:
                    break; // allow fall-through to report
                default:
                    return; // not monitored
            }
        }
    }

    idpsCallbackInfo.idpsStatusInfo.sid = idpsPtr->sid;
    idpsCallbackInfo.idpsStatusInfo.status = idpsPtr->status;
    idpsCallbackInfo.idpsAddrInfo.sa = idpsPtr->sa;
    idpsCallbackInfo.idpsAddrInfo.ta = idpsPtr->ta;
    idpsCallbackInfo.idpsAddrInfo.vlanId = idpsPtr->vlanId;

    if( idpsPtr->sid == SECURITY_ACCESS_REQUEST_ID || idpsPtr->sid == AUTHENTICATION_REQUEST_ID )
    {
        idpsCallbackInfo.idpsStatusInfo.securityLevel = 2; //taf_diagIDPS.api HIGH_LEVEL
    }
    else
    {
        idpsCallbackInfo.idpsStatusInfo.securityLevel = 1; //taf_diagIDPS.api LOW_LEVEL
    }
    //Send to main thread after handling IDPS data
    le_event_Report(IdpsCbEventId, &idpsCallbackInfo, sizeof(tafIdpsCallbackInfo_t));
}

// The IDPS callback handler which runs in main thread
void UdsIdps::IdpsCallbackHandler
(
    void* reqPtr
)
{
    tafIdpsCallbackInfo_t* idpsCbInfoPtr = (tafIdpsCallbackInfo_t*)reqPtr;
    TAF_ERROR_IF_RET_NIL(idpsCbInfoPtr == NULL, "Null pointer");

    taf_udsIdpsHandler_t* udsIdpsHandler =
            (taf_udsIdpsHandler_t*)le_ref_Lookup(UdsIdps::udsIdpsHandlerRefMap,
            UdsIdps::idpsIndicationHandler.safeRef);

    if(udsIdpsHandler == NULL || udsIdpsHandler->funcPtr == NULL)
    {
        LE_ERROR("Not find handler to send notification");
        return;
    }

    udsIdpsHandler->funcPtr(&idpsCbInfoPtr->idpsAddrInfo, &idpsCbInfoPtr->idpsStatusInfo,
        udsIdpsHandler->ctxPtr);
}

/* IDPS thread */
void* UdsIdps::UdsIdpsThread
(
    void* ctxPtr
)
{

    IdpsEventId = le_event_CreateId("IdpsEvt", sizeof(tafIdpsMsg_t));
    le_event_AddHandler("IdpsEvtHdlr", IdpsEventId, IdpsHandler);

    /* post to be ready */
    le_sem_Post((le_sem_Ref_t) ctxPtr);

    le_event_RunLoop();
    return NULL;
}

void UdsIdps::SendIdpsIndMsg
(
    const uint8_t* sendBuf,
    uint16_t sendDataLen,
    const uint8_t* recvBuf,
    uint16_t recvDataLen,
    taf_doip_AddrInfo_t* addrInfoPtr
)
{
    uint8_t sid;
    uint8_t status;
    tafIdpsMsg_t idpsMsg = {};

    TAF_ERROR_IF_RET_NIL(sendBuf == NULL || recvBuf == NULL || addrInfoPtr == NULL, "Null pointer");
    TAF_ERROR_IF_RET_NIL(sendDataLen < UDS_RESP_MIN_LEN || recvDataLen < UDS_REQ_MIN_LEN,
        "Wrong data length");

    // Negative response
    if (sendBuf[0] == UDS_NEGATIVE_RESP_SID)
    {
        TAF_ERROR_IF_RET_NIL(sendDataLen != UDS_NEG_RESP_LEN, "Wrong NRC length");
        sid = sendBuf[1];
        status = sendBuf[2];
        //No need to send IDPS message for 0x78
        if(status == REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING)
            return;
     }
    else // Positive response
    {
        sid = sendBuf[0] & ~0x40;
        status = 0;
    }

    if(! cfg::IsIDPSSupported(sid))
        return;

    idpsMsg.sa = addrInfoPtr->sa;
    idpsMsg.ta = addrInfoPtr->ta;
    idpsMsg.vlanId = addrInfoPtr->vlanId;
    idpsMsg.sid = sid;
    idpsMsg.status = status;

    if(recvDataLen < MAX_IDPS_DATA_LEN)
    {
        memcpy(idpsMsg.dataBuf, recvBuf, recvDataLen);
        idpsMsg.dataLen = recvDataLen;
    }
    else
    {
        memcpy(idpsMsg.dataBuf, recvBuf, MAX_IDPS_DATA_LEN);
        idpsMsg.dataLen = MAX_IDPS_DATA_LEN;
    }

    LE_DEBUG("IDPS report sid =0x%x, status = 0x%x", sid, status);
    // Report to main thread to send session change to diag service.
    le_event_Report(IdpsEventId, &idpsMsg, sizeof(idpsMsg));
}

void UdsIdps::Init(void)
{

    if(cfg::IsIDPSAvailable())
    {
        le_sem_Ref_t semRef = le_sem_Create("idpsReady", 0);

        udsIdpsThRef = le_thread_Create("udsIdpsTh", UdsIdpsThread, (void*)semRef);
        le_thread_Start(udsIdpsThRef);

        le_sem_Wait(semRef); /* waiting for post-action */
        LE_INFO("Init IDPS successfully");

        le_sem_Delete(semRef);

        udsIdpsHandlerRefMap = le_ref_CreateMap("udsIdpsHandlerRefMap", TAF_IDPS_HANDLER_REF_CNT);
        IdpsCbEventId = le_event_CreateId("IdpsCbEvt", sizeof(tafIdpsCallbackInfo_t));
        le_event_AddHandler("IdpsCbEvtHdlr", IdpsCbEventId, IdpsCallbackHandler);
    }
}

} /* uds */
} /* taf */
