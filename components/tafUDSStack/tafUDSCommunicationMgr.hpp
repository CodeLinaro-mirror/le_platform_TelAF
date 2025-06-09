/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFUDS_COMMUNICATION_MGR_HPP
#define TAFUDS_COMMUNICATION_MGR_HPP

#include "tafDoIPStack.h"
#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "configuration.hpp"
#include <mutex>
#include "tafUDSStack.h"
#include <atomic>

using namespace tafsvc;

namespace taf{
namespace uds{

    #define UDS_DATA_SIZE 4095
    #define UDS_CERT_MAX_LEN 8192
    #define UDS_MAX_DATA_SIZE 2*UDS_CERT_MAX_LEN+16 //For PKI uni direction, 8192+8192+16
    #define UDS_P2_SERVER 50 // Default P2 server interval
    #define UDS_P2_SERVER_MAX 65535 //Maximal P2 server interval
    #define UDS_P2_STAR_SERVER 5000 // Default P2* server interval
    #define UDS_P2_STAR_SERVER_MAX 655350 //Maximal P2* server interval
    #define UDS_P2_STAR_SERVER_MIN 1500 //Minimal P2* server interval
    #define DELTA_UDS_P2_RESP 500 //Delta P2 RESP
    #define UDS_P2_STAR_SERVER_CNT 120
    #define UDS_S3_SERVER 5000
    #define AUTH_DEFAULT_MAX_ATT_CNT 10
    #define AUTH_DEFAULT_DELAY_TIME 60 // 1 minute. unit:second
    #define AUTH_DEFAULT_DELAY_TIME_MAX 16*60 // 16 minutes. unit:second
    #define TAF_UDS_HANDLER_REF_CNT 1
    #define SHORT_TERM_ADJUSTMENT 3
    #define MAX_INTERFACE_NAME_LEN 30
    #define TESTER_STATE_CHANGE_DATA_SIZE 3
    #define MAX_TIMER_NAME_LEN 50
    #define UDS_INDICATION_DATA_LEN_MAX 255
    #define TESTER_STATE_CHANGE_TIMER 5000 // Tester state timer
    #define CANCEL_FILE_TRANSFER_IND_LEN 2
    #define MAX_FILE_TRANSFER_STATE_MTX_NAME_LEN 30

    // UDS minimal len
    #define UDS_REQ_MIN_LEN 1

    // Negative Response (0x7F)
    #define UDS_NEGATIVE_RESP_SID 0x7F
    #define UDS_NEG_RESP_LEN 3

    // Session control service (0x10)
    #define UDS_SESSION_CTRL_REQ_MIN_LEN 2
    #define UDS_SESSION_CTRL_RESP_LEN 6
    #define UDS_SESSION_CHANGE_DATA_SIZE 3

    // ECUReset service (0x11)
    #define UDS_ECU_RESET_REQ_MIN_LEN 2
    #define UDS_ECU_RESET_RESP_BASE_LEN 2
    #define HARD_RESET 1

    // ReadDTCInformation service (0x19)
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN 2
    #define UDS_READ_DTC_INFO_RESP_BASE_LEN 2
    // Subfunction (0x19)
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_01_02 3
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_03 2
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_04 6
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_05 3
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_06 6
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_07_08 4
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_09 5
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_0A_0B_0C_0D_0E_14_15 2
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_16 3
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_17 4
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_18 7
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_19 7
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_1A 3
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_42 5
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_55 3
    #define UDS_READ_DTC_INFO_REQ_MIN_LEN_56 4

    // ReadDataByIdentifier service (0x22)
    #define UDS_READ_DID_REQ_MIN_LEN 3
    #define UDS_READ_DID_RESP_BASE_LEN 1
    #define UDS_READ_DID_RESP_MIN_LEN 3
    #define UDS_DID_LEN 2

    // Security access service (0x27)
    #define UDS_SECURITY_ACCESS_REQ_MIN_LEN 2
    #define UDS_SECURITY_ACCESS_REQ_SEED_MIN_LEN 2
    #define UDS_SECURITY_ACCESS_REQ_KEY_MIN_LEN 3
    #define UDS_SECURITY_ACCESS_SEND_KEY_REQ_MIN_LEN 3 //sid(1)+subfunc(1)+securitykey(1)
    #define UDS_SECURITY_ACCESS_RESP_MIN_LEN 2
    #define UDS_SECURITY_ACCESS_RESP_SEED_ZERO_LEN 4

    // Authentication service (0x29)
    #define UDS_AUTH_INFO_REQ_MIN_LEN 2
    #define UDS_AUTH_INFO_RESP_BASE_LEN 2
    #define UDS_AUTH_DATA_SIZE_MIN_LEN 1
    #define MAX_AUTH_TIME 30

    #define UDS_AUTH_EXPIRATION_DATA_SIZE 9
    #define AUTH_CFG_NODE_PATH_LEN 128
    #define AUTH_CONF_DATA "tafDiagSvc:/authentication/"
    #define AUTH_ROLE_READ_PATTERN "read_role"
    #define AUTH_ROLE_WRITE_PATTERN "write_role"
    #define AUTH_ROLE_IOCTL_PATTERN "io_role"
    #define AUTH_ROLE_ROUTINE_PATTERN "routine_role"

    // Request length of authentication service
    #define UDS_AUTH_DEAUTHENTICATE_EXACT_LEN 2       //Deauthenticate
    #define UDS_AUTH_VERIFY_CERT_UNIDIR_MIN_LEN 7     //verifyCertificateUnidirectional
    #define UDS_AUTH_VERIFY_CERT_BIDIR_MIN_LEN 9      //verifyCertificateBidirectional
    #define UDS_AUTH_POWN_MIN_LEN 6                   //proofOfOwnership
    #define UDS_AUTH_TRANSMIT_CERT_MIN_LEN    6       //TransmitCertificate
    #define UDS_AUTH_REQ_CHLNG_EXACT_LEN 19           //requestChallengeForAuthentication
    #define UDS_AUTH_VERIFY_POWN_UNIDIR_MIN_LEN 25    //verifyProofOfOwnershipUnidirectional
    #define UDS_AUTH_VERIFY_POWN_BIDIR_MIN_LEN 26     //verifyProofOfOwnershipBidirectional
    #define UDS_AUTH_CONFIGURATION_EXACT_LEN 2        //authenticationConfiguration

    // WriteDataByIdentifier service (0x2E)
    #define UDS_WRITE_DID_REQ_MIN_LEN 4
    #define UDS_WRITE_DID_REQ_BASE_LEN 3  // Service ID(1) + DID (2)
    #define UDS_WRITE_DID_RESP_LEN 3

    // InputOutputControlByIdentifier service (0x2F)
    #define UDS_IOCBID_REQ_MIN_LEN 4  // SI+DID+IOCP
    #define UDS_IOCBID_RESP_MIN_LEN 4

    // Routine control service (0x31)
    #define UDS_ROUTINE_CTRL_REQ_MIN_LEN 4
    #define UDS_ROUTINE_CTRL_RESP_MIN_LEN 4

    // TransferData service (0x36)
    #define UDS_REQ_XFER_DATA_BASE_LEN 2
    #define UDS_RESP_XFER_DATA_BASE_LEN 2

    // RequestTransferExit service (0x37)
    #define UDS_REQ_XFER_EXIT_BASE_LEN 1
    #define UDS_RESP_XFER_EXIT_BASE_LEN 1

    // RequestFileTranser service (0x38)
    #define SIZE_OF_SID    1 /* RequestFileTransfer Request SID */
    #define SIZE_OF_MOOP   1 /* modeOfOperation */
    #define SIZE_OF_FPL    2 /* filePathAndNameLength */
    #define SIZE_OF_FP_B1  1 /* first byte of filePathAndName */
    #define SIZE_OF_DFI_   1 /* dataFormatIdentifier */
    #define SIZE_OF_FSL    1 /*fileSizeParameterLength */

    #define RFT_MIN_LEN    (SIZE_OF_SID + SIZE_OF_MOOP + SIZE_OF_FPL + SIZE_OF_FP_B1)
    #define RFT_BASE_LEN   (RFT_MIN_LEN - SIZE_OF_FP_B1)
    #define INDEX_FP_B1    (SIZE_OF_SID + SIZE_OF_MOOP + SIZE_OF_FPL)

    #define SIZE_OF_R_SID  SIZE_OF_SID /* RequestFileTransfer Response SID  */
    #define SIZE_OF_LFID   1 /* lengthFormatIdentifier */
    #define RRFT_BASE_LEN  (SIZE_OF_R_SID + SIZE_OF_MOOP)

    // Tester present service (0x3E)
    #define UDS_TESTER_PRESENT_REQ_LEN 2
    #define UDS_TESTER_PRESENT_RESP_LEN 2

    // ClearDiagnosticInformation service (0x14)
    #define UDS_CLEAR_DIAG_INFO_REQ_MIN_LEN 4
    #define UDS_CLEAR_DIAG_INFO_REQ_MIN_LEN_AND_MEM_SELECTION_LEN 5
    #define UDS_CLEAR_DIAG_INFO_REQ_MAX_LEN 5
    #define UDS_CLEAR_DIAG_INFO_RESP_LEN 1

    // ControlDTCSetting service (0x85)
    #define UDS_CTRL_DTC_SETTING_REQ_MIN_LEN 2
    #define UDS_CTRL_DTC_SETTING_RESP_LEN 2

    // ResponseOnEvent service (0x86)
    #define UDS_ROE_REQ_MIN_LEN 3
    #define UDS_ROE_RESP_MIN_LEN 2
    #define UDS_ROE_RESP_RAE_MIN_LEN 1
    #define UDS_ROE_RESP_BASE_LEN 2
    #define UDS_ROE_ONDTCS_MIN_LEN 7
    #define UDS_ROE_MANUFACTURE_WIN_TIME 8
    #define UDS_ROE_INFINITE_TIME_TO_RESP 2
    #define UDS_ROE_DO_NOT_STORE_EVENT 0
    #define UDS_ROE_STORE_EVENT 1

    // S3 timer action
    typedef enum
    {
        TAF_UDS_S3_TIMER_STOP          = 0,
        TAF_UDS_S3_TIMER_START         = 0x01,
        TAF_UDS_S3_TIMER_RESTART       = 0x02,
        TAF_UDS_P2STAR_TIMER_STOP      = 0x03,
        TAF_UDS_P2STAR_TIMER_START     = 0x04,
        TAF_UDS_P2STAR_TIMER_RESTART   = 0x05,
        TAF_UDS_AUTH_TIMER_STOP        = 0x06,
        TAF_UDS_AUTH_TIMER_START       = 0x07,
        TAF_UDS_AUTH_TIMER_RESTART     = 0x08,
        TAF_UDS_AUTH_DELAY_TIMER_STOP  = 0x09,
        TAF_UDS_AUTH_DELAY_TIMER_START = 0x0A,
        TAF_UDS_TESTER_STATE_TIMER_STOP  = 0x0B,
        TAF_UDS_TESTER_STATE_TIMER_START = 0x0C,
        TAF_UDS_TESTER_STATE_TIMER_RESTART = 0x0D
    }taf_UDSTimer_EventType_t;

    typedef enum
    {
        AUTH_SUBFUNC_DEAUTHENTICATE        = 0x00,
        AUTH_SUBFUNC_VERIFY_CERT_UNIDIR    = 0x01,
        AUTH_SUBFUNC_VERIFY_CERT_BIDIR     = 0x02,
        AUTH_SUBFUNC_POWN                  = 0x03,
        AUTH_SUBFUNC_TRANSMIT_CERT         = 0x04,
        AUTH_SUBFUNC_REQ_CHLNG_FOR_AUTH    = 0x05,
        AUTH_SUBFUNC_VERIFY_POWN_UNIDIR    = 0x06,
        AUTH_SUBFUNC_VERIFY_POWN_BIDIR     = 0x07,
        AUTH_SUBFUNC_AUTH_CONF             = 0x08,
        AUTH_SUBFUNC_UNKNOWN               = 0xff
    }taf_UDSReqAuthSubFunc_t;

    typedef enum
    {
        AUTH_STATE_DEAUTHENTICATED        = 0x00,
        AUTH_STATE_AUTHENTICATED          = 0x01,
        AUTH_STATE_UNKNOWN                = 0xff
    }taf_UDSAuthState_t;

    typedef enum
    {
        ROE_SUBFUNC_STPROE                = 0x00,
        ROE_SUBFUNC_ONDTCS                = 0x01,
        ROE_SUBFUNC_OCODID                = 0x03,
        ROE_SUBFUNC_RAE                   = 0x04,
        ROE_SUBFUNC_STRTROE               = 0x05,
        ROE_SUBFUNC_CLRROE                = 0x06,
        ROE_SUBFUNC_OCOV                  = 0x07,
        ROE_SUBFUNC_RMRDOSC               = 0x08,
        ROE_SUBFUNC_RDRIODSC              = 0x09
    }taf_UDSReqROESubFunc_t;

    // RequestFileTranser service mode of operation type
    typedef enum
    {
        MOOP_ADD_FILE     = 0x01,
        MOOP_DELETE_FILE  = 0x02,
        MOOP_REPLACE_FILE = 0x03,
        MOOP_READ_FILE    = 0x04,
        MOOP_READ_DIR     = 0x05,
        MOOP_RESUME_FILE  = 0x06
    }taf_UDSReqFileXferMOOPType_t;

    // Diagnostic Request service ID
    typedef enum
    {
        SESSION_CONTROL_REQUEST_ID = 0x10,
        ECU_RESET_REQUEST_ID = 0x11,
        CLEAR_DIAG_INFO_REQUEST_ID = 0x14,
        READ_DTC_INFO_REQUEST_ID = 0x19,
        READ_DID_REQUEST_ID = 0x22,
        SECURITY_ACCESS_REQUEST_ID = 0x27,
        AUTHENTICATION_REQUEST_ID = 0x29,
        WRITE_DID_REQUEST_ID = 0x2E,
        INPUT_OUTPUT_CONTROL_REQUEST_ID = 0x2F,
        ROUTINE_CONTROL_REQUEST_ID = 0x31,
        TRANSFER_DATA_REQUEST_ID = 0x36,
        REQUEST_TRANSFER_EXIT_REQUEST_ID = 0x37,
        REQUEST_FILE_TRANSFER_REQUEST_ID = 0x38,
        TESTER_PRESENT_REQUEST_ID = 0x3E,
        CONTROL_DTC_SETTING_REQUEST_ID = 0x85,
        RESPONSE_ON_EVENT_REQUEST_ID = 0x86
    }taf_UDSReqSvcID_t;

    // Diagnostic Response service ID
    typedef enum
    {
        SESSION_CONTROL_RESPONSE_ID = 0x50,
        ECU_RESET_RESPONSE_ID = 0x51,
        CLEAR_DIAG_INFO_RESPONSE_ID = 0x54,
        READ_DTC_INFO_RESPONSE_ID = 0x59,
        READ_DID_RESPONSE_ID = 0x62,
        SECURITY_ACCESS_RESPONSE_ID = 0x67,
        AUTHENTICATION_RESPONSE_ID = 0x69,
        WRITE_DID_RESPONSE_ID = 0x6E,
        IOCBID_RESPONSE_ID = 0x6F,
        ROUTINE_CONTROL_RESPONSE_ID = 0x71,
        TRANSFER_DATA_RESPONSE_ID = 0x76,
        REQUEST_TRANSFER_EXIT_RESPONSE_ID = 0x77,
        REQUEST_FILE_TRANSFER_RESPONSE_ID = 0x78,
        TESTER_PRESENT_RESPONSE_ID = 0x7E,
        CONTROL_DTC_SETTING_RESPONSE_ID = 0xC5,
        RESPONSE_ON_EVENT_RESPONSE_ID = 0xC6
    }taf_UDSRespSvcID_t;

    // UDS error code.
    typedef enum
    {
        POSITIVE_RESPONSE = 0,
        SERVICE_NOT_SUPPORTED = 0x11,
        SUBFUNCTION_NOT_SUPPORTED = 0x12,
        INCORRECT_MSG_LEN_OR_INVALID_FORMAT = 0x13,
        RESP_TOO_LONG = 0x14,
        BUSY_REPEAT_REQ = 0x21,
        CONDITIONS_NOT_CORRECT = 0x22,
        REQ_SEQUENCE_ERROR = 0x24,
        REQ_OUT_OF_RANGE = 0x31,
        SECURITY_ACCESS_DENY = 0x33,
        AUTHENTICATION_REQUIRED = 0x34,
        INVALID_KEY = 0x35,
        EXCEEDED_NUMBER_OF_ATTEMPTS = 0x36,
        REQUIRED_TIME_DELAY_NOT_EXPIRED = 0x37,
        UPLOAD_DOWNLOAD_NOT_ACCEPTED = 0x70,
        GENERAL_PROGRAMMING_FAILURE = 0x72,
        REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING = 0x78,
        SUBFUNCTION_NOT_SUPPORTED_IN_ACTIVE_SESSION = 0x7E,
        SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION = 0x7F
    }taf_UDSErrorCode_t;

    // UDS notification message ID
    typedef enum
    {
        CANCEL_FILE_TRANSFER_RESULT = 0xFC,
        TESTER_STATE_MSG_ID = 0xFD,
        AUTHENTICATION_EXPIRATION_MSG_ID = 0xFE,
        SESSION_CHANGE_MSG_ID = 0xFF
    }taf_UDSIndicationMsg_t;

    typedef enum
    {
        TAF_CANCEL_FILEXFER_START,
        TAF_CANCEL_FILEXFER_END,
    } taf_UDSCancelFileXferEvent_t;

    typedef struct
    {
        taf_UDSTimer_EventType_t             event;
        uint32_t                             interval;
        char                                 ifName[30];
    } udsTimerEvent_t;

    // UDS stack indication handler structure.
    typedef struct
    {
        taf_doip_DiagIndicationHandlerFunc_t funcPtr;
        void*                                ctxPtr;
        void*                                safeRef;
    }taf_UDSIndicationHandler_t;

    // Internal cancelFileXfer event.
    typedef struct
    {
        taf_UDSCancelFileXferEvent_t event;
        char ifName[MAX_INTERFACE_NAME_LEN];
    } cancelFileXferEvent_t;

    // Vlan Id list for CancelFileXfer.
    typedef struct
    {
        uint16_t vlanId;
        char ifName[MAX_INTERFACE_NAME_LEN];
        le_dls_Link_t  link;
    }taf_CancelFileXferReq_t;

    // ENUM for session type.
    typedef enum
    {
        DEFAULT_SESSION = 0x01,
        PROGRAMMING_SESSION = 0x02,
        EXTENDED_DIAGNOSTIC_SESSION = 0x03,
        VEHICLE_MANUFACTURER_SPECIFIC_SESSION = 0x40,
        FOTA_SESSION = 0x42,
        DOWNLOADED_ENUMLATION_SESSION = 0x52,
        SYSTEM_SUPPLIER_SPECIFIC_SESSION = 0x60
    }taf_SessionType_t;

    // ENUM for tester present state.
    typedef enum
    {
        OFF = 0x00,
        ON = 0x01
    }taf_TesterState_t;

    class UdsCommunicationMgr{
        public:
            UdsCommunicationMgr(const char* ifName);
            UdsCommunicationMgr();
            ~UdsCommunicationMgr();

            static UdsCommunicationMgr * GetInstance(const char* ifName);
            static le_result_t GetIfNameByVlanId(uint16_t vlanId, char* ifName);

            void Init();
            static void InitInstances(le_dls_List_t* interfaceList);
            static void InitAuthData(le_dls_List_t* interfaceList);
            static le_result_t UdsStart(const char* configPathPtr);

            static void GetFileXferActiveStateList(le_dls_List_t* fileXferStateListPtr);
            static void GetVlanIdList(le_dls_List_t* vlanIDListPtr);

            static le_result_t UdsAddDiagIndicationHandler();
            static void DiagIndicationHandler( taf_doip_AddrInfo_t* addrInfoPtr,
                    taf_doip_DiagMsg_t* diagMsgPtr, taf_doip_Result_t result, void* userPtr);
            static void DiagConfirmHandler(const taf_doip_AddrInfo_t* addrInfoPtr,
                taf_doip_Result_t result, void* userPtr);

            le_result_t SendUDSResp(const char* ifName, uint8_t serviceId, uint8_t err,
                    const uint8_t* dataPtr, uint16_t dataSize);
            le_result_t SetUDSData(const char* ifName, uint8_t dataType, const uint8_t* dataPtr,
                        uint16_t dataSize);

            le_result_t SetNRC(uint8_t sid, uint8_t errorCode);
            le_result_t SendNRC(uint8_t sid, uint8_t errorCode, taf_doip_AddrInfo_t*  addrInfoPtr);
            void SendData(taf_doip_AddrInfo_t*  addrInfoPtr);
            le_result_t CheckAndSendInd(uint8_t sid, taf_doip_AddrInfo_t* addrInfoPtr,
                    taf_doip_DiagMsg_t* diagMsgPtr);

            static void P2StarTimeoutHandler(le_timer_Ref_t timerRef);
            static void S3TimeoutHandler(le_timer_Ref_t timerRef);
            static void AuthTimeoutHandler(le_timer_Ref_t timerRef);
            static void AuthDelayTimeoutHandler(le_timer_Ref_t timerRef);
            static void TesterStateTimeoutHandler(le_timer_Ref_t timerRef);
            static le_ref_MapRef_t udsHandlerRefMap;
            static taf_UDSIndicationHandler_t udsIndicationHandler;

            /* Security Access -- BEG -- */
            uint8_t nrcCode = 0x00;
            le_result_t remoteError = LE_OK;
            taf_doip_AddrInfo_t udsRespAddrInfo;
            struct AO_SecurityAccess_s * mSecurityAccess;
            /* Security Access -- END -- */

            uint8_t recvBuf[UDS_MAX_DATA_SIZE];
            uint8_t sendBuf[UDS_MAX_DATA_SIZE];
            uint16_t recvDataLen = 0;
            uint16_t sendDataLen = 0;
            std::atomic<bool> readyToRecvData = {true};
            std::atomic<bool> isPaused = {false};
            char interface[MAX_INTERFACE_NAME_LEN];
            uint16_t vlanId = 0;
            le_timer_Ref_t p2StarTimerRef;
            le_timer_Ref_t s3TimerRef;
            le_timer_Ref_t authTimerRef;
            le_timer_Ref_t authDelayTimerRef;
            le_timer_Ref_t testerStateTimerRef;
            taf_SessionType_t SessionType = DEFAULT_SESSION;
            taf_TesterState_t PreviousState = OFF;
            uint64_t currentRoleVal = 0;
            taf_UDSAuthState_t authState = AUTH_STATE_UNKNOWN;
            // update status parameter.
            bool isXferActive = false;

            static le_event_Id_t cancelFileXferEvId;
            static le_dls_List_t cancelFileXferReqList;
            static le_mutex_Ref_t cancelFileXferListMutex;
        private:
            // Indicate recevied service message to Diag service if necessary.
            le_result_t IndicateSessionCtrlReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // SessionCtrl service (0x10).
            le_result_t IndicateECUResetReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // ECUReset service (0x11).
            le_result_t IndicateReadDIDReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // ReadDID service (0x22).
            le_result_t IndicateWriteDIDReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // WriteDID service (0x2E).
            le_result_t IndicateSecAccessReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // SecurrityAccess service (0x27).
            le_result_t IndicateAuthReq(taf_doip_AddrInfo_t*  addrInfoPtr,
                    bool* isInternalHandle);    // Authentication service (0x29).
            le_result_t IndicateIOCBIDReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // InputOutputControlByIdentifier service (0x2F).
            le_result_t IndicateRoutinrCtrlReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // RoutineControl service (0x31).
            le_result_t IndicateRxFileXferReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // RequestFileTransfer service (0x38).
            le_result_t IndicateRxXferDataReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // TransferData service (0x36).
            le_result_t IndicateRxXferExitReq(taf_doip_AddrInfo_t* addrInfoPtr,
                    bool* isInternalHandle);    // RequestTransferExit service (0x37).
            le_result_t IndicateClearDiagInfoReq(taf_doip_AddrInfo_t*  addrInfoPtr,
                    bool* isInternalHandle);    // ClearDiagnosticInformation service (0x14)
            le_result_t IndicateCtrlDTCSettingReq(taf_doip_AddrInfo_t*  addrInfoPtr,
                    bool* isInternalHandle);    // ControlDTCSetting service (0x85)
            le_result_t IndicateReadDTCInfoReq(taf_doip_AddrInfo_t*  addrInfoPtr,
                    bool* isInternalHandle);    // ReadDTCInfo service (0x19)
            le_result_t IndicateROEReq(taf_doip_AddrInfo_t*  addrInfoPtr,
                    bool* isInternalHandle);    // ResponseOnEvent service(0x86)

            // Internally check and Respond UDS message to uds client (through DoIP stack).
            le_result_t TesterPresentResp(taf_doip_AddrInfo_t*  addrInfoPtr);    // (0x3E)

            // Send UDS response message from Diag service.
            le_result_t SessionCtrlResp(uint8_t serviceId, uint8_t err);
            le_result_t ECUResetResp(uint8_t serviceId, uint8_t err);
            le_result_t ReadDIDResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);
            le_result_t WriteDIDResp(uint8_t serviceId, uint8_t err);
            le_result_t SecurityAccessResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);
            le_result_t AuthenticationResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);
            le_result_t IOCBIDResp(uint8_t serviceId, const uint8_t* dataPtr, uint16_t dataSize,
                    uint8_t err);
            le_result_t RoutineCtrlResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);
            le_result_t XferDataResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);
            le_result_t ReqXferExitResp(uint8_t serviceId, uint8_t err);
            le_result_t ReqFileXferResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);

            static void IndicateWhenChangingToDefault(const char* ifName);
            le_result_t ReadDTCInfoResp(uint8_t serviceId, const uint8_t* dataPtr,
                    uint16_t dataSize, uint8_t err);
            le_result_t ClearDiagInfoResp(uint8_t serviceId, uint8_t err);
            le_result_t CtrlDTCSettingResp(uint8_t serviceId, uint8_t err);
            le_result_t ROEResp(uint8_t serviceId, const uint8_t* dataPtr, uint16_t dataSize,
                        uint8_t err);

            static void* UdsTimerThread(void* ctxPtr);
            static void UdsTimerHandler(void* reqPtr);
            void UdsTimerEventReport(taf_UDSTimer_EventType_t timerEvent, uint32_t interval,
                        const char* ifName);
            void CheckAndRestartS3Timer(uint8_t serviceId);
            void CheckAndRestartTesterStateTimer();
            bool IsSessTypeMatched(cfg::Node& node);
            bool IsSecurityAccessMatched(cfg::Node& node);
            bool IsAuthRoleMatched(taf_UDSReqSvcID_t serviceType, cfg::Node& node);
            bool IsRequestSubFuncSupported(cfg::Node& node, uint8_t subFunc);
            bool IsControlOptionRecordValid(uint16_t rid, uint8_t subFunc, const uint8_t* dataRec,
                    size_t dataRecLen);
            bool IsTotalLengthCheckValid(uint16_t rid, uint8_t subFunc, size_t dataRecLen);

            std::map<std::string, uint8_t>& GetSessionMap(void);
            bool IsServiceIDSupported(uint8_t sid);
            bool IsValidSvcActiveSession(uint8_t sid);
            bool IsSvcSecAccessMatched(uint8_t sid);
            bool IsAuthCheckOK(uint8_t sid);
            bool IsAuthReqLenCorrect(uint8_t subFunc);
            bool IsAuthSubFuncSupported(uint8_t subFunc);
            bool IsROESubFuncSupported(uint8_t subFunc);
            bool IsROEReqLenCorrect(uint8_t subFunc);
            bool IsROEReqOutOfRange(uint8_t subFunc);

            le_result_t GeneralServerResp(taf_doip_AddrInfo_t* addrInfoPtr, uint8_t sid);

            bool IsSubFuncSupported(uint8_t sid, uint8_t subFunc);
            bool IsSubFuncSessTypeValid(uint8_t sid, uint8_t subFunc);
            bool IsSubFuncSecAccessMatched(uint8_t sid, uint8_t subFunc);
            bool IsSubFuncAuthCheckOK(uint8_t sid, uint8_t subFunc);
            bool PrecheckForSwitchingSession(uint8_t originalSession, uint8_t targetedSession);

            //Internal cancelFileXfer event handler
            static void cancelFileXferHandler(void* reqPtr);
            void CheckAndSendCancelFileXferEvent();
            static le_result_t addCancelFileXferReqInList(uint16_t vlanId, const char* ifName);
            static bool IsCancelFileXferReqInList(uint16_t vlanId);
            void StoreAttCntToTree();
            void StoreDelayTimeToTree();

            //session change parameter.
            taf_doip_AddrInfo_t addrInfo;
            uint8_t sesChangeBuf[UDS_SESSION_CHANGE_DATA_SIZE];
            uint8_t dataIndBuf[UDS_INDICATION_DATA_LEN_MAX];
            uint8_t cancelFileXferBuf[CANCEL_FILE_TRANSFER_IND_LEN];
            taf_UDSReqAuthSubFunc_t authPreSucReq = AUTH_SUBFUNC_UNKNOWN;
            uint8_t authAttCnt = 0;
            uint16_t authDelayTime = 60; // 1 minute

            // Tester present state change notification.
            static void IndicateTesterStateChange(const char* ifName,
                    taf_TesterState_t currentState);
            uint8_t testerStateChangeBuf[TESTER_STATE_CHANGE_DATA_SIZE];
            taf_doip_DiagMsg_t stateChangeMsg;

            static taf_doip_Ref_t  DoipEntityRef;
            static taf_doip_DiagIndicationHandlerRef_t IndicationRef;
            static taf_doip_PowerModeQueryHandlerRef_t PmQueryRef;
            static taf_doip_DiagConfirmHandlerRef_t ConfirmRef;

            static bool isResetInProgress;
            static le_event_Id_t udsTimerEventId;
            static le_sem_Ref_t semRef;

            static std::map<std::string, UdsCommunicationMgr*> instances;
            static std::mutex mutex_instance;
            le_mutex_Ref_t fileXferStateMutex;
            taf_doip_DiagMsg_t sesChangeMsg;
            taf_doip_DiagMsg_t dataIndMsg;
            taf_doip_DiagMsg_t cancelFileXferMsg;
    };
}
}
#endif  // TAFUDS_COMMUNICATION_MGR_HPP
