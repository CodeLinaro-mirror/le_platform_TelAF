/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAFUPDATESVR_HPP
#define TAFUPDATESVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#define DEFAULT_SVC_REF_CNT 16
#define DEFAULT_RXMSG_REF_CNT 16
#define DEFAULT_RXHANDLER_REF_CNT 16
#define TAF_DIAG_UPDATE_ADD_FILE        0x01    ///< Add a file.
#define TAF_DIAG_UPDATE_DELETE_FILE     0x02    ///< Delete a file.
#define TAF_DIAG_UPDATE_REPLACE_FILE    0x03    ///< Replace or add a file.
#define TAF_DIAG_UPDATE_READ_FILE       0x04    ///< Read the file data.
#define TAF_DIAG_UPDATE_READ_DIR        0x05    ///< Read the directory.
#define TAF_DIAG_UPDATE_RESUME_FILE     0x06    ///< Resume downloading the file.
#define NRC_NOTIFICATION_MIN_LEN 3              ///< The minimal length of NRC

//--------------------------------------------------------------------------------------------------
/**
 * Diag update service structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagUpdate_ServiceRef_t svcRef;                 ///< own reference.
    le_msg_SessionRef_t sessionRef;                     ///< Client that represents the service.
    le_dls_List_t reqFileXferMsgList;
    le_dls_List_t xferDataMsgList;
    le_dls_List_t reqXferExitMsgList;
    taf_diagUpdate_RxFileXferMsgHandlerRef_t fileXferRef; ///< Rx Msg Handler of RequestFileTransfer.
    taf_diagUpdate_RxXferDataMsgHandlerRef_t xferDataRef; ///< Rx Msg Handler of TransferData.
    taf_diagUpdate_RxXferExitMsgHandlerRef_t xferExitRef; ///< Rx Msg Handler of RequestTransferExit.

    le_dls_List_t supportedVlanList;
}taf_UpdateSvc_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag update service Rx message for RequestFileTransfer.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef;                     ///< Own reference.
    taf_uds_AddrInfo_t addrInfo;
    uint8_t serviceId;                                              ///< Service Identifier.
    taf_diagUpdate_ModeOfOpsType_t operationType;                   ///< Mode od operation type.
    uint16_t filePathAndNameLen;                                    ///< File path and name length.
    uint8_t filePathAndName[TAF_DIAGUPDATE_MAX_PATH_AND_NAME_SIZE]; ///< file path and name.
    uint8_t dataFormatID;                                           ///< Data format ID.
    uint8_t fileSizeParamLen;                                       ///< file size parameter length.
    uint32_t unCompFileSize;                                        ///< Uncompressed file size.
    uint32_t compFileSize;                                          ///< Compressed file size.
}taf_FileXferRxMsg_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag update RequestFileTransfer service handler structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagUpdate_ServiceRef_t svcRef;                     ///< Service reference.
    taf_diagUpdate_RxFileXferMsgHandlerFunc_t func;         ///< Handler function.
    void* context;                                          ///< Handler context.
    taf_diagUpdate_RxFileXferMsgHandlerRef_t handlerRef;    ///< own reference.
}taf_FileXferHandler_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag update service Rx message for TransferData.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;
    taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef;                     ///< Own reference.
    taf_uds_AddrInfo_t addrInfo;
    uint8_t serviceId;                                              ///< Service Identifier.
    uint8_t blockSeqCount;                                          ///< Block sequence count.
    uint8_t xferDataRec[TAF_DIAGUPDATE_MAX_XFER_PARAM_REC_SIZE];    ///< Payload Data Buffer.
    size_t xferDataSize;                                            ///< The size of xferDataRec.
}taf_XferDataRxMsg_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag update TransferData service handler structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagUpdate_ServiceRef_t svcRef;                   ///< Service reference.
    taf_diagUpdate_RxXferDataMsgHandlerFunc_t func;       ///< Handler function.
    void* context;                                        ///< Handler context.
    taf_diagUpdate_RxXferDataMsgHandlerRef_t handlerRef;  ///< own reference.
}taf_XferDataHandler_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag update service Rx message for RequestTransferExit.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;
    taf_diagUpdate_RxXferExitMsgRef_t rxMsgRef;                     ///< Own reference.
    taf_uds_AddrInfo_t addrInfo;
    uint8_t serviceId;                                              ///< Service Identifier.
    uint8_t exitDataRec[TAF_DIAGUPDATE_MAX_XFER_PARAM_REC_SIZE];    ///< Xfer exit data record.
    size_t exitDataSize;                                            ///< The size of exitDataRec.
}taf_XferExitRxMsg_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag update RequestTransferExit service handler structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagUpdate_ServiceRef_t svcRef;                     ///< Service reference.
    taf_diagUpdate_RxXferExitMsgHandlerFunc_t func;         ///< Handler function.
    void* context;                                          ///< Handler context.
    taf_diagUpdate_RxXferExitMsgHandlerRef_t handlerRef;    ///< own reference.
}taf_XferExitHandler_t;

typedef struct
{
    le_dls_Link_t   link;
    uint16_t        vlanId;
}taf_UpdateVlanIdNode_t;

//-------------------------------------------------------------------------------------------------
/**
 * NRC status message.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    taf_diagUpdate_NrcStatusRef_t nrcStatusRef; ///< Own reference.
    taf_uds_AddrInfo_t addrInfo;                ///< Rx logical address information structure.
    uint8_t sid;                                ///< Service ID.
    uint8_t nrc;                                ///< NRC.
}taf_UpdateNrcStatusMsg_t;

//--------------------------------------------------------------------------------------------------
/**
 * Diag update Server Service Class
 */
//--------------------------------------------------------------------------------------------------
namespace tafsvc
{
    class taf_UpdateSvr : public ITafSvc, public taf_UDSInterface
    {
        public:
            taf_UpdateSvr() {};
            ~taf_UpdateSvr() {};
            void Init();

            static taf_UpdateSvr &GetInstance();
            static void OnClientDisconnection(le_msg_SessionRef_t sessionRef,
                                                       void *contextPtr);

            taf_diagUpdate_ServiceRef_t CreateUpdateSvc();

            // UDS message handler. It will be called if receive this service message.
            void UDSMsgHandler(const taf_uds_AddrInfo_t* addrPtr,
                uint8_t sid, uint8_t* msgPtr, size_t msgLen) override;

            static void RxFileXferEventHandler(void* reportPtr);
            taf_diagUpdate_RxFileXferMsgHandlerRef_t AddRxFileXferReqHandler(
                taf_diagUpdate_ServiceRef_t svcRef, taf_diagUpdate_RxFileXferMsgHandlerFunc_t
                    handlerPtr, void* contextPtr);
            void RemoveRxFileXferReqHandler(taf_diagUpdate_RxFileXferMsgHandlerRef_t handlerRef);
            le_result_t GetFilePathAndNameLen(taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
                uint16_t* fileLenPtr);
            le_result_t GetFilePathAndName(taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
                uint8_t* fileNamePtr, size_t* fileNameSizePtr);
            le_result_t GetDataFormatID(taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
                uint8_t* dataFormatIDPtr);
            le_result_t GetFileSizeParamLen(taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
                uint8_t* fileParamLenPtr);
            le_result_t GetUnCompFileSize(taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
                uint32_t* unCompFileSizePtr);
            le_result_t GetCompFileSize(taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
                uint32_t* compFileSizePtr);
            le_result_t SetFilePosition(taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
                uint64_t filePosition);
            le_result_t SetFileSizeOrDirInfoLength(taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
                uint64_t fileSizeUncompressedOrDirInfoLength, uint64_t fileSizeCompressed);
            le_result_t SendFileXferResp(taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
                taf_diagUpdate_FileXferErrorCode_t errCode);
            le_result_t ReleaseRxFileXferMsg(taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef);

            static void RxXferDataEventHandler(void* reportPtr);
            taf_diagUpdate_RxXferDataMsgHandlerRef_t AddRxXferDataReqHandler(
                taf_diagUpdate_ServiceRef_t svcRef, taf_diagUpdate_RxXferDataMsgHandlerFunc_t
                    handlerPtr, void* contextPtr);
            void RemoveRxXferDataReqHandler(taf_diagUpdate_RxXferDataMsgHandlerRef_t handlerRef);
            le_result_t GetblockSeqCount(taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef,
                uint8_t* countPtr);
            le_result_t GetXferDataParamRecLen(taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef,
                uint16_t* xferDataRecLenPtr);
            le_result_t GetXferDataParamRec(taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef,
                uint8_t* xferDataRecPtr, size_t* xferDataRecSizePtr);
            le_result_t SendXferDataResp(taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef,
                taf_diagUpdate_XferDataErrorCode_t errCode, const uint8_t* dataPtr, size_t dataSize);
            le_result_t ReleaseRxXferDataMsg(taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef);

            static void RxXferExitEventHandler(void* reportPtr);
            taf_diagUpdate_RxXferExitMsgHandlerRef_t AddRxXferExitReqHandler(
                taf_diagUpdate_ServiceRef_t svcRef, taf_diagUpdate_RxXferExitMsgHandlerFunc_t
                    handlerPtr, void* contextPtr);
            void RemoveRxXferExitReqHandler(taf_diagUpdate_RxXferExitMsgHandlerRef_t handlerRef);
            le_result_t GetXferExitParamRecLen(taf_diagUpdate_RxXferExitMsgRef_t rxMsgRef,
                uint16_t* exitDataRecLenPtr);
            le_result_t GetXferExitParamRec(taf_diagUpdate_RxXferExitMsgRef_t rxMsgRef,
                uint8_t* exitDataRecPtr, size_t* exitDataRecSizePtr);
            le_result_t SendXferExitResp(taf_diagUpdate_RxXferExitMsgRef_t rxMsgRef,
                taf_diagUpdate_XferExitErrorCode_t errCode, const uint8_t* dataPtr, size_t dataSize);
            le_result_t ReleaseRxXferExitMsg(taf_diagUpdate_RxXferExitMsgRef_t rxMsgRef);

            le_result_t RemoveUpdateSvc(taf_diagUpdate_ServiceRef_t svcRef);

            le_result_t SetVlanId(taf_diagUpdate_ServiceRef_t svcRef, uint16_t vlanId);
            le_result_t GetVlanIdFromMsg(taf_diagUpdate_RxMsgRef_t rxMsgRef, uint16_t* vlanIdPtr);

            static void FirstLayerNrcStatusHandler(void* reportPtr, void* secondLayerHandlerFunc);
            le_result_t ReleaseNrcStatusMsg(taf_diagUpdate_NrcStatusRef_t statusRef);

            //Event for NRC
            le_event_Id_t NrcEventId;

        private:
            // Internal search functions.
            taf_UpdateSvc_t* FindSvcInList(le_msg_SessionRef_t sessionRef);
            taf_UpdateSvc_t* FindSvcInList(uint16_t vlanId);
            le_result_t RspNegativeMsg(taf_uds_AddrInfo_t* addrPtr, uint8_t sid, uint8_t nrc,
                    bool isInternal);
            le_result_t RspPositiveMsg(taf_uds_AddrInfo_t* addrPtr, uint8_t sid,
                const uint8_t* dataPtr,size_t dataSize);

        #ifdef LE_CONFIG_DIAG_FEATURE_A
            void ReportNrcStatus(const taf_uds_AddrInfo_t* addrPtr, uint8_t sid, uint8_t nrc);
        #endif

            void ClearFileXferMsgList(taf_UpdateSvc_t* svcPtr);
            void ClearXferDataMsgList(taf_UpdateSvc_t* svcPtr);
            void ClearXferExitMsgList(taf_UpdateSvc_t* svcPtr);
            void ClearVlanList(taf_UpdateSvc_t* svcPtr);

            const uint8_t reqFileXferSvcId = 0x38;
            const uint8_t fileDataXferSvcId = 0x36;
            const uint8_t fileXferExitSvcId = 0x37;
            const uint8_t nrcStatusMsgId = 0xFB;  //NRC status notification ID

            // Service and event object
            le_ref_MapRef_t SvcRefMap;
            le_mem_PoolRef_t SvcPool;

            // Rx message object.
            le_ref_MapRef_t RxFileXferMsgRefMap;
            le_mem_PoolRef_t RxFileXferMsgPool;
            le_ref_MapRef_t RxXferDataMsgRefMap;
            le_mem_PoolRef_t RxXferDataMsgPool;
            le_ref_MapRef_t RxXferExitMsgRefMap;
            le_mem_PoolRef_t RxXferExitMsgPool;

            // Rx message handler object.
            le_ref_MapRef_t RxFileXferHandlerRefMap;
            le_mem_PoolRef_t RxFileXferHandlerPool;
            le_ref_MapRef_t RxXferDataHandlerRefMap;
            le_mem_PoolRef_t RxXferDataHandlerPool;
            le_ref_MapRef_t RxXferExitHandlerRefMap;
            le_mem_PoolRef_t RxXferExitHandlerPool;
            le_mem_PoolRef_t vlanPool;

            //Event for service
            le_event_Id_t FileXferEvent;
            le_event_HandlerRef_t FileXferEventHandlerRef;
            le_event_Id_t XferDataEvent;
            le_event_HandlerRef_t XferDataEventHandlerRef;
            le_event_Id_t XferExitEvent;
            le_event_HandlerRef_t XferExitEventHandlerRef;

            // NRC status resource
            le_mem_PoolRef_t NrcStatusMsgPool;
            le_ref_MapRef_t RxNrcStatusRefMap;

            uint8_t mFilePosition[TAF_DIAGUPDATE_FILE_POSITION_SIZE] = {0};
            uint16_t nCharsToSave = 0;
            uint16_t nCharsToNotUsed = 0;
            uint8_t mFileSizeUncompressedOrDirInfoLength[TAF_DIAGUPDATE_FILE_SIZE_OR_DIR_INFO_LEN] = {0};
            uint8_t mFileSizeCompressed[TAF_DIAGUPDATE_FILE_SIZE_OR_DIR_INFO_LEN] = {0};
            #define SIZE_OF_FSDIL 2
            uint8_t mFileSizeOrDirInfoParameterLength[SIZE_OF_FSDIL] = {0};
    };
}
#endif /* #ifndef TAFUPDATESVR_HPP */
