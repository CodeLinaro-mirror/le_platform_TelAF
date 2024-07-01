/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"


static taf_diagDidStore_DataIdChangeHandlerRef_t DidChangeHandlerRef = NULL;
taf_diagDidStore_ServiceRef_t diagStorgSvcRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Test DID storage Read API.
 */
//--------------------------------------------------------------------------------------------------
void TestDidStorgRead(void)
{
    LE_TEST_INFO("Test DID Storage Read API");
    uint16_t read_did = 0xA5A6;
    uint8_t read_data[4092] = {0};
    size_t read_dataSize = sizeof(read_data);
    if(diagStorgSvcRef == NULL)
    {
        LE_ERROR("Get diagDidStorage service");
    }

    le_result_t read_res = taf_diagDidStore_Read(diagStorgSvcRef, read_did, read_data,
        &read_dataSize);
    LE_TEST_OK(read_res == LE_OK, "Successfully read from DID Storage %u\n",read_did);
    for (int i = 0; i<read_dataSize; i++)
    {
        LE_INFO("Read data: %x", read_data[i]);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test DID storage Write API.
 */
//--------------------------------------------------------------------------------------------------
void TestDidStorgWrite(void)
{
    LE_TEST_INFO("Test DID Storage Write API");
    uint16_t write_did = 0xA5A6;
    uint8_t write_data[] = {0x34};
    size_t write_dataSize = sizeof(write_data)/sizeof(uint8_t);

    if(diagStorgSvcRef == NULL)
    {
        LE_ERROR("Get diagDidStorage service");
    }
    le_result_t write_res = taf_diagDidStore_Write(diagStorgSvcRef, write_did, write_data,
        write_dataSize);
    LE_TEST_OK(write_res == LE_OK, "Successfully wrote to DID Storage %u\n",write_did);
}


void DidChangeHandler(uint16_t DataId, const uint8_t* dataRecordPtr, size_t dataRecordSize,
    void* contextPtr)
{
    LE_TEST_INFO("DID change notification for registered did");
    LE_TEST_INFO("Change Notify for DID: %d", DataId);
    for (int i = 0; i< dataRecordSize; i++)
    {
        LE_TEST_INFO("Chnaged did data record : %x", dataRecordPtr[i]);
    }
}



COMPONENT_INIT
{
    uint16_t notify_did = 0xA5A6;

    //get diag storage reference
    diagStorgSvcRef = taf_diagDidStore_GetService();
    if(diagStorgSvcRef == NULL)
    {
        LE_ERROR("Get diagDidStorage service");
    }

    DidChangeHandlerRef = taf_diagDidStore_AddDataIdChangeHandler(diagStorgSvcRef, notify_did,
            DidChangeHandler, NULL);
    taf_diagDidStore_RemoveDataIdChangeHandler(DidChangeHandlerRef);

    TestDidStorgWrite();

    TestDidStorgRead();
}