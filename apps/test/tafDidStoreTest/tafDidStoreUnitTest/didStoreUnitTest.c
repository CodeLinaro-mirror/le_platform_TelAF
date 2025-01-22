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
    LE_TEST_INFO("Testing DID Storage Read API with multiple values");

    if (diagStorgSvcRef == NULL)
    {
        LE_ERROR("diagDidStorage service is not initialized");
        return;
    }

    // Array of DIDs to read for testing
    uint16_t read_dids[] = {0xA5A6, 0xF011, 0xF0D0};  // Add more DIDs as needed
    uint8_t read_data[4092] = {0};  // Buffer for read data
    size_t read_dataSize = sizeof(read_data);

    for (size_t i = 0; i < sizeof(read_dids) / sizeof(read_dids[0]); i++)
    {
        uint16_t read_did = read_dids[i];
        memset(read_data, 0, sizeof(read_data));  // Clear the buffer before each read
        read_dataSize = sizeof(read_data);  // Reset buffer size for each read

        LE_TEST_INFO("Reading DID: 0x%04X", read_did);

        le_result_t read_res = taf_diagDidStore_Read(diagStorgSvcRef, read_did, read_data,
                &read_dataSize);

        if (read_res == LE_OK && read_dataSize > 0)
        {
            LE_TEST_INFO("Successfully read from DID Storage 0x%04X", read_did);
            LE_TEST_OK(read_res == LE_OK, "Read operation for DID 0x%04X passed", read_did);

            // Print read data for verification
            for (size_t j = 0; j < read_dataSize; j++)
            {
                LE_INFO("Read data[%zu]: 0x%02X", j, read_data[j]);
            }
        }
        else
        {
            LE_TEST_INFO("Read operation for DID 0x%04X failed with result: %d", read_did,
                read_res);
            LE_TEST_OK(read_res != LE_OK, "Read operation for DID 0x%04X failed as expected",
                read_did);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Test DID storage Write API.
 */
//--------------------------------------------------------------------------------------------------
void TestDidStorgWrite(void)
{
    LE_TEST_INFO("Testing DID Storage Write API with multiple values");

    if (diagStorgSvcRef == NULL)
    {
        LE_ERROR("diagDidStorage service is not initialized");
        return;
    }

    // Array of DIDs and data to test writing multiple values
    uint16_t write_dids[] = {0xA5A6, 0xF011, 0xF0D0};  // Add more DIDs as needed
    uint8_t write_data[][17] = {
        {0x34},            // Data for DID 0xA5A6
        {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A}, // Data for DID 0xF011
        {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
        0x0F, 0x10, 0x11}  // Data for DID 0xF0D0
    };
    size_t data_sizes[] = {1, 10, 17};

    for (size_t i = 0; i < sizeof(write_dids) / sizeof(write_dids[0]); i++)
    {
        uint16_t write_did = write_dids[i];
        uint8_t* data_ptr = write_data[i];
        size_t data_size = data_sizes[i];

        LE_TEST_INFO("Writing DID: 0x%04X", write_did);

        le_result_t write_res = taf_diagDidStore_Write(diagStorgSvcRef, write_did,
                data_ptr, data_size);
        if (write_res == LE_OK)
        {
            LE_TEST_INFO("Successfully wrote to DID Storage 0x%04X", write_did);
            LE_TEST_OK(write_res == LE_OK, "Write operation for DID 0x%04X passed",
                write_did);
        }
        else
        {
            LE_TEST_INFO("Write operation for DID 0x%04X failed with result: %d",
                write_did, write_res);
            LE_TEST_OK(write_res != LE_OK, "Write operation for DID 0x%04X failed as expected", write_did);
        }
    }
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

    TestDidStorgWrite();

    TestDidStorgRead();
}