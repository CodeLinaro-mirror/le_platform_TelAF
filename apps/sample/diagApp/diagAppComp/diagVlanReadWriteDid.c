/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
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
static taf_diagDataID_ServiceRef_t DiagVlanDIDSvcRef = {NULL};
static taf_diagDataID_RxReadDIDMsgHandlerRef_t DiagVlanRdDIDMsgRef = {NULL};
#ifndef LE_CONFIG_DIAG_VSTACK
static taf_diagDataID_RxWriteDIDMsgHandlerRef_t DiagVlanWrDIDMsgRef = {NULL};
#endif

/**
 * Callback function for read dataID request message
 */
void diagVlanRdDIDMsgHandler
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

    uint16_t vlanId = 0;
    if (taf_diagDataID_GetVlanIdFromMsg((taf_diagDataID_RxMsgRef_t)rxMsgRef, &vlanId) != LE_OK)
    {
        LE_ERROR("Failed to get vlan id");
        if(taf_diagDataID_SendReadDIDResp( rxMsgRef, TAF_DIAGDATAID_READ_DID_CONDITIONS_NOT_CORRECT,
                NULL, 0 ) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    LE_TEST_INFO("Received a read DID request from vlan0x%x", vlanId);

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
    if(taf_diagDataID_SendReadDIDResp(rxMsgRef,
            TAF_DIAGDATAID_READ_DID_NO_ERROR , sendBuf, sendBufLen ) != LE_OK)
    {
        LE_ERROR("Send response error");
    }
}

/**
 * Callback function for writeDataID request message
 */
void diagVlanWrDIDMsgHandler
(
    taf_diagDataID_RxWriteDIDMsgRef_t rxMsgRef,
    uint16_t dataId,
    void* contextPtr
)
{
    uint8_t recordData[TAF_DIAGDATAID_MAX_DID_DATA_RECORD_SIZE];
    size_t dataLen = 0;
    le_result_t result;
    uint16_t vlanId = 0;
    if (taf_diagDataID_GetVlanIdFromMsg((taf_diagDataID_RxMsgRef_t)rxMsgRef, &vlanId) != LE_OK)
    {
        LE_ERROR("Failed to get vlan id");
        if(taf_diagDataID_SendWriteDIDResp(rxMsgRef,
            TAF_DIAGDATAID_WRITE_DID_CONDITIONS_NOT_CORRECT,
            dataId) != LE_OK)
        {
            LE_ERROR("Send response error");
        }
        return;
    }

    LE_TEST_INFO("Received a write DID request from vlan0x%x", vlanId);

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

    result = writeDIDToConfigTree(dataId, recordData, dataLen);

    if(taf_diagDataID_SendWriteDIDResp(rxMsgRef, result, dataId) != LE_OK)
    {
        LE_ERROR("Send response error");
    }

}

static void* diagVlanRWDIDMsgThread(void* ctxPtr)
{
    taf_diagDataID_ConnectService();

    DiagVlanDIDSvcRef = taf_diagDataID_GetService();
    if(DiagVlanDIDSvcRef == NULL)
    {
        LE_ERROR("Get diagDataID service");
        return (void*)LE_FAULT;
    }

    DiagVlanRdDIDMsgRef = taf_diagDataID_AddRxReadDIDMsgHandler(
                                DiagVlanDIDSvcRef,
                                diagVlanRdDIDMsgHandler, NULL);
    LE_TEST_OK(DiagVlanRdDIDMsgRef != NULL,
               "Registered successfully for diagVlanRdDIDMsgHandler");
#ifndef LE_CONFIG_DIAG_VSTACK
    DiagVlanWrDIDMsgRef = taf_diagDataID_AddRxWriteDIDMsgHandler(
                                DiagVlanDIDSvcRef,
                                diagVlanWrDIDMsgHandler, NULL);
    LE_TEST_OK(DiagVlanWrDIDMsgRef != NULL,
               "Registered successfully for diagVlanWrDIDMsgHandler");
#endif
    le_sem_Post(semRef);
    le_event_RunLoop();
    return NULL;
}

le_result_t diagVlanReadWriteDid_Init(void)
{
    semRef = le_sem_Create("SemRef", 0);

    // Create diag RWDID message handle thread to handle read/wriet DID request
    le_thread_Ref_t rwDataIdThreadRef = le_thread_Create("rwDataIdTd",
                                                         diagVlanRWDIDMsgThread,
                                                         NULL);

    le_thread_Start(rwDataIdThreadRef);

    le_sem_Wait(semRef);

    return LE_OK;
}
