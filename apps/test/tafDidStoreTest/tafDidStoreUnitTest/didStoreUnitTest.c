/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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
void TestDidStorgRead1(void)
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
void TestDidStorgWrite1(void)
{
    LE_TEST_INFO("TestDidStorgWrite1");

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
        {0x01, 0x02, 0x03}  // Data for DID 0xF0D0
    };
    size_t data_sizes[] = {1, 10, 3};

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

void TestDidStorgWrite2(void)
{
    LE_TEST_INFO("TestDidStorgWrite2");

    if (diagStorgSvcRef == NULL)
    {
        LE_ERROR("diagDidStorage service is not initialized");
        return;
    }

    // Array of DIDs and data to test writing multiple values
    uint16_t write_dids[] = {0xA5A6, 0xF011, 0xF0D0};  // Add more DIDs as needed
    uint8_t write_data[][17] = {
        {0x24}, // Data for DID 0xA5A6
        {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xAB}, // Data for DID 0xF011
        {0x11, 0x12, 0x13} // Data for DID 0xF0D0
    };
    size_t data_sizes[] = {1, 10, 3};

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
            LE_TEST_OK(write_res != LE_OK, "Write operation for DID 0x%04X failed as expected",
                write_did);
        }
    }
}

void DidChangeHandler(uint16_t DataId, const uint8_t* dataRecordPtr, size_t dataRecordSize,
    void* contextPtr)
{
    LE_TEST_INFO("Change Notified for DID: %x", DataId);
    for (int i = 0; i< dataRecordSize; i++)
    {
        LE_TEST_INFO("Chnaged did data record : %x", dataRecordPtr[i]);
    }
}



COMPONENT_INIT
{
    LE_INFO("DIDStore test app init started !");
    uint16_t did1 = 0xA5A6, did2 = 0xF0D0, did3 = 0xF011;

    // Get the diag storage service reference.
    diagStorgSvcRef = taf_diagDidStore_GetService();
    if(diagStorgSvcRef == NULL)
    {
        LE_ERROR("Get diagDidStorage service");
        return;
    }

    // Register for DID1 to get change notification.
    LE_TEST_INFO("Register DID change notification for DID1: %x", did1);
    DidChangeHandlerRef = taf_diagDidStore_AddDataIdChangeHandler(diagStorgSvcRef, did1,
            DidChangeHandler, NULL);

    // Add DID2, To get notification on change.
    LE_TEST_INFO("Add DID change notification for DID2: %x", did2);
    taf_diagDidStore_AddDIDToHandler(diagStorgSvcRef, did2);
    // Duplicate DID2, adding same DID again.
    le_result_t res1 = taf_diagDidStore_AddDIDToHandler(diagStorgSvcRef, did2);
    LE_TEST_OK(res1 == LE_DUPLICATE, "Requested DID %x is already added", did2);

    // Write the data record for DID1, DID2 and DID3.
    TestDidStorgWrite1();
    // Read the data record for DID1, DID2 and DID3.
    TestDidStorgRead1();

    // Add DID3, To get notification on change.
    LE_TEST_INFO("Add DID change notification for DID3: %x", did3);
    taf_diagDidStore_AddDIDToHandler(diagStorgSvcRef, did3);
    // Remove DID2.
    LE_TEST_INFO("Remove DID change notification for DID2: %x", did2);
    taf_diagDidStore_RemoveDIDFromHandler(diagStorgSvcRef, did2);

    // Remove DID4, Not added before.
    uint16_t did4 = 0xA0A1;
    le_result_t res2 = taf_diagDidStore_RemoveDIDFromHandler(diagStorgSvcRef, did4);
    LE_TEST_OK(res2 == LE_NOT_FOUND, "Requested DID %x to remove was not added", did4);

    // Write/Update the data record for DID1, DID2 and DID3.
    TestDidStorgWrite2();

    // Add some more DID to get notification on change as per DTool request.
    uint16_t did5 = 0xA5A5, did6 = 0xA0A0, did7 = 0xA0A2;
    taf_diagDidStore_AddDIDToHandler(diagStorgSvcRef, did5);
    taf_diagDidStore_AddDIDToHandler(diagStorgSvcRef, did6);
    taf_diagDidStore_AddDIDToHandler(diagStorgSvcRef, did7);

    LE_INFO("DIDStore test app init completed !");
}
