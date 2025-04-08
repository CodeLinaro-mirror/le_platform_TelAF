/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "diagPrivate.h"

// DID length definition
#define DID_LEN                      2

// DID Config tree definition
#define DID_NODE_LEN                 100
#define DID_CONFIG_TREE_NODE         "diag/DID"
#define DID_CONFIG_TREE_VALUE_FORMAT "diag/DID/%2x/value"
#define DID_DATA_FORMAT              "data%d"

static le_sem_Ref_t semRef;

// Diag RDBI/WDBI
static taf_diagDataID_ServiceRef_t DiagDataIDSvcRef = NULL;
static taf_diagDataID_RxReadDIDMsgHandlerRef_t DiagReadDataIDMsgRef = NULL;
static taf_diagDataID_RxWriteDIDMsgHandlerRef_t DiagWriteDataIDMsgRef = NULL;

/**
 * Read DID from ConfigTree.
 */
uint8_t readDIDFromConfigTree
(
    const uint16_t dataId,
    uint8_t* sendBuf,
    size_t* sendBufLen
)
{
    le_cfg_ConnectService();

    if(sendBuf == NULL || sendBufLen == NULL)
    {
        LE_ERROR("Null pointer.");
        return TAF_DIAGDATAID_READ_DID_CONDITIONS_NOT_CORRECT;
    }

    char node[DID_NODE_LEN] = { 0 };
    snprintf(node, sizeof(node), DID_CONFIG_TREE_VALUE_FORMAT, dataId);

    le_cfg_IteratorRef_t iteratorRef_r = le_cfg_CreateReadTxn(node);

    if (iteratorRef_r == NULL || (le_cfg_GoToFirstChild (iteratorRef_r) != LE_OK))
    {
        LE_ERROR("No DID node.");
        le_cfg_CancelTxn(iteratorRef_r);
        return TAF_DIAGDATAID_READ_DID_CONDITIONS_NOT_CORRECT;
    }

    int i = 0;
    uint8_t data;
    do{

        if((*sendBufLen + i) >= TAF_DIAGDATAID_MAX_READ_DID_PAYLOAD_SIZE)
        {
            LE_DEBUG("Data length is too long");
            le_cfg_CancelTxn(iteratorRef_r);
            return TAF_DIAGDATAID_READ_DID_CONDITIONS_NOT_CORRECT;
        }

        data = le_cfg_GetInt(iteratorRef_r, "", 0);
        sendBuf[*sendBufLen+i] = data;

        i++;
    }while (le_cfg_GoToNextSibling(iteratorRef_r) == LE_OK);

    *sendBufLen =*sendBufLen + i;
    le_cfg_CancelTxn(iteratorRef_r);

    return TAF_DIAGDATAID_READ_DID_NO_ERROR ;
}

/**
 * Write DID to ConfigTree.
 */
uint8_t writeDIDToConfigTree
(
    const uint16_t dataId,
    const uint8_t* dataPtr,
    uint16_t dataSize
)
{
    le_cfg_ConnectService();

    if(dataPtr == NULL)
    {
        LE_ERROR("Null pointer.");
        return TAF_DIAGDATAID_WRITE_DID_GENERAL_PROGRAMMING_FAILURE;
    }

    char node[DID_NODE_LEN] = { 0 };
    snprintf(node, sizeof(node), DID_CONFIG_TREE_VALUE_FORMAT, dataId);
    le_cfg_IteratorRef_t wrIter = le_cfg_CreateWriteTxn(node);

    if (wrIter == NULL)
    {
        LE_ERROR("Get write node failed.");
        return TAF_DIAGDATAID_WRITE_DID_CONDITIONS_NOT_CORRECT;
    }

    // Need to clear the config tree since the length of the value written may be less than the old.
    if(le_cfg_IsEmpty(wrIter, "") == false)
    {
        le_cfg_SetEmpty(wrIter, "");
        LE_DEBUG("after clear, dataId=0x%x",dataId);
        le_cfg_CommitTxn(wrIter);
        wrIter = le_cfg_CreateWriteTxn(node);
    }

    for(int i=0; i < dataSize; i++)
    {
        char nodeDataStr[DID_NODE_LEN] = {0};
        snprintf(nodeDataStr, sizeof(nodeDataStr), DID_DATA_FORMAT, i+1);
        le_cfg_SetInt(wrIter, nodeDataStr, dataPtr[i]); //Store data
    }
    le_cfg_CommitTxn(wrIter);

    return TAF_DIAGDATAID_WRITE_DID_NO_ERROR ;
}

/**
 * Callback function for read dataID request message
 */
void readDataIDMsgHandler
(
    taf_diagDataID_RxReadDIDMsgRef_t rxMsgRef,
    const uint16_t* dataIdPtr,
    size_t dataIdSize,
    void* contextPtr
)
{
    uint8_t sendBuf[TAF_DIAGDATAID_MAX_READ_DID_PAYLOAD_SIZE];
    size_t sendBufLen = 0;
    uint8_t result;

    if(dataIdSize == 0 || dataIdPtr == NULL)
    {
        if(taf_diagDataID_SendReadDIDResp( rxMsgRef, TAF_DIAGDATAID_READ_DID_CONDITIONS_NOT_CORRECT,
                NULL, 0 ) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    for( int i=0; i< (int)dataIdSize; i++)
    {
        if(sendBufLen + DID_LEN > TAF_DIAGDATAID_MAX_READ_DID_PAYLOAD_SIZE)
        {
            if(taf_diagDataID_SendReadDIDResp( rxMsgRef,
                    TAF_DIAGDATAID_READ_DID_CONDITIONS_NOT_CORRECT, NULL, 0 ) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

        // Fill data id
        sendBuf[sendBufLen] = (dataIdPtr[i] & 0xff00) >> 8;
        sendBuf[sendBufLen+1] = dataIdPtr[i] & 0xff;
        sendBufLen = sendBufLen + DID_LEN;

        // Get record data for dataId[i] and fill record data into sendBuf
        result = readDIDFromConfigTree(dataIdPtr[i], sendBuf, &sendBufLen);
        if(result != TAF_DIAGDATAID_READ_DID_NO_ERROR)
        {
            if(taf_diagDataID_SendReadDIDResp( rxMsgRef, result, NULL, 0 ) != LE_OK)
            {
                LE_ERROR("Send response error");
            }
            return;
        }

    }

    // All data are got, send response
    if(taf_diagDataID_SendReadDIDResp( rxMsgRef,
            TAF_DIAGDATAID_READ_DID_NO_ERROR , sendBuf, sendBufLen ) != LE_OK)
    {
        LE_ERROR("Send response error");
    }

}

/**
 * Callback function for writeDataID request message
 */
void writeDataIDMsgHandler
(
    taf_diagDataID_RxWriteDIDMsgRef_t rxMsgRef,
    uint16_t dataId,
    void* contextPtr
)
{
    uint8_t recordData[TAF_DIAGDATAID_MAX_DID_DATA_RECORD_SIZE];
    size_t dataLen = 0;
    le_result_t result;
    uint8_t ret;

    result = taf_diagDataID_GetWriteDataRecord(rxMsgRef, recordData, &dataLen);
    if( result != LE_OK)
    {
        LE_ERROR("Getting data record");
        if(taf_diagDataID_SendWriteDIDResp(
                rxMsgRef,
                TAF_DIAGDATAID_WRITE_DID_CONDITIONS_NOT_CORRECT, dataId) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    ret = writeDIDToConfigTree(dataId, recordData, dataLen);

    if(taf_diagDataID_SendWriteDIDResp( rxMsgRef, ret, dataId) != LE_OK)
    {
        LE_ERROR("Send response error");
    }

}

static void* diagRWDataIdMsgThread(void* ctxPtr)
{
    taf_diagDataID_ConnectService();

    DiagReadDataIDMsgRef = taf_diagDataID_AddRxReadDIDMsgHandler(
                                DiagDataIDSvcRef,
                                readDataIDMsgHandler, NULL);
    LE_TEST_OK(DiagReadDataIDMsgRef != NULL,
               "Registered successfully for readDataIDMsgHandler");
 
    DiagWriteDataIDMsgRef = taf_diagDataID_AddRxWriteDIDMsgHandler(
                                DiagDataIDSvcRef,
                                writeDataIDMsgHandler, NULL);
    LE_TEST_OK(DiagWriteDataIDMsgRef != NULL,
               "Registered successfully for writeDataIDMsgHandler");
 
    le_sem_Post(semRef);
    le_event_RunLoop();
    return NULL;
}

le_result_t diagReadWriteDid_Init(void)
{
    semRef = le_sem_Create("SemRef", 0);

    //get diag Data ID reference
    DiagDataIDSvcRef = taf_diagDataID_GetService();
    if(DiagDataIDSvcRef == NULL)
    {
        LE_ERROR("Get diagDataID service");
        return LE_FAULT;
    }

    // Create diag RWDID message handle thread to handle read/wriet DID request
    le_thread_Ref_t rwDataIdThreadRef = le_thread_Create("rwDataIdTd",
                                                         diagRWDataIdMsgThread, NULL);

    le_thread_Start(rwDataIdThreadRef);
    le_sem_Wait(semRef);

    return LE_OK;
}
