/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

taf_diagDidStore_ServiceRef_t diagStorgSvcRef = NULL;

typedef struct {
    uint16_t did;
    uint8_t *data;
    size_t len;
} did_entry_t;

uint8_t data_A5A5[] = {0x00, 0x00};
uint8_t data_A5A6[] = {0x01};
uint8_t data_A0A0[] = {0x02};
uint8_t data_A0A1[] = {0x03};
uint8_t data_ACC0[] = {0x04};
uint8_t data_ACC1[] = {0x01, 0x01};
uint8_t data_ACC2[] = {0x02, 0x02};
uint8_t data_ACC3[] = {0x03};
uint8_t data_ACC4[] = {0x04};
uint8_t data_ACC8[] = {0x08};
uint8_t data_ACC9[] = {0x09};

uint8_t data_F011[] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01
};

uint8_t data_F0D0[] = {0x02, 0x02, 0x02};
uint8_t data_F0D2[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

did_entry_t did_table[] = {
    {0xA5A5, data_A5A5, sizeof(data_A5A5)},
    {0xA5A6, data_A5A6, sizeof(data_A5A6)},
    {0xA0A0, data_A0A0, sizeof(data_A0A0)},
    {0xA0A1, data_A0A1, sizeof(data_A0A1)},
    {0xACC0, data_ACC0, sizeof(data_ACC0)},
    {0xACC1, data_ACC1, sizeof(data_ACC1)},
    {0xACC2, data_ACC2, sizeof(data_ACC2)},
    {0xACC3, data_ACC3, sizeof(data_ACC3)},
    {0xACC4, data_ACC4, sizeof(data_ACC4)},
    {0xACC8, data_ACC8, sizeof(data_ACC8)},
    {0xACC9, data_ACC9, sizeof(data_ACC9)},
    {0xF011, data_F011, sizeof(data_F011)},
    {0xF0D0, data_F0D0, sizeof(data_F0D0)},
    {0xF0D2, data_F0D2, sizeof(data_F0D2)}
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

//--------------------------------------------------------------------------------------------------
/**
 * Write default DID values to the database.
 */
//--------------------------------------------------------------------------------------------------
void WriteDefaultDIDValues(void)
{
    for (int i = 0; i < ARRAY_SIZE(did_table); i++)
    {
        LE_TEST_INFO("DID 0x%X, len = %" PRIuS "\n", did_table[i].did, did_table[i].len);

        le_result_t write_res = taf_diagDidStore_Write(diagStorgSvcRef, did_table[i].did,
                did_table[i].data, did_table[i].len);
        if (write_res == LE_OK)
        {
            LE_TEST_INFO("Successfully wrote to database 0x%04X", did_table[i].did);
            LE_TEST_OK(write_res == LE_OK, "Write operation for DID 0x%04X passed",
                did_table[i].did);
        }
        else
        {
            LE_TEST_INFO("Write operation for DID 0x%04X failed with result: %d", did_table[i].did,
            write_res);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Read DID values from the database.
 */
//--------------------------------------------------------------------------------------------------
void ReadDIDValues(void)
{
    LE_TEST_INFO("Read DID values from database");
    uint8_t read_data[4092] = {0};  // Buffer for read data
    size_t read_dataSize = sizeof(read_data);

    for (int i = 0; i < ARRAY_SIZE(did_table); i++)
    {
        memset(read_data, 0, sizeof(read_data));  // Clear the buffer before each read
        read_dataSize = sizeof(read_data);  // Reset buffer size for each read

        LE_TEST_INFO("Reading DID: 0x%04X", did_table[i].did);

        le_result_t read_res = taf_diagDidStore_Read(diagStorgSvcRef, did_table[i].did, read_data,
                &read_dataSize);

        if (read_res == LE_OK && read_dataSize > 0)
        {
            // Print read data for verification
            for (size_t j = 0; j < read_dataSize; j++)
            {
                LE_INFO("Read data[%zu]: 0x%02X", j, read_data[j]);
            }
        }
    }
}

COMPONENT_INIT
{
    LE_INFO("didStoreInitDB sample app init started !");

    // Get the diag storage service reference.
    diagStorgSvcRef = taf_diagDidStore_GetService();
    if(diagStorgSvcRef == NULL)
    {
        LE_ERROR("Get diagDidStorage service");
        return;
    }

    // Write the default values for all DIDs
    WriteDefaultDIDValues();

    ReadDIDValues();

    LE_INFO("didStoreInitDB sample app init completed !");
}
