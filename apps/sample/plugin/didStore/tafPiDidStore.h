/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "tafPiDiagDID.h"

#ifndef DIDSTORG_PLUGIN_H
#define DIDSTORG_PLUGIN_H

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for suspend response.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NotifyDidEventId;
static le_mem_PoolRef_t ValueRequestPoolRef;

#define DID_NODE_LEN       100
#define DID_DATA_FORMAT    "data%d"
#define MAX_DID_REQ_LEN 256

typedef enum
{
    VALUE_REQUEST_GET,
    VALUE_REQUEST_SET
} ValueRequest;

typedef struct {
    ValueRequest Vrequest;
    uint16_t did;
    uint8_t* value;
    size_t len;
    le_result_t result;
    void* ctxPtr;
}valueChangeReq_t;

typedef struct {
    uint16_t did;
    size_t len;
    uint8_t value[MAX_DID_REQ_LEN];
    bool changeNotify;
} DIDEntry;

// Local buffer simulating storage of DID values
static DIDEntry did_entries[] = {
    {0xA5A5, 2, {0x01, 0x02}, false},
    {0xA5A6, 1, {0x03}, false},
    {0xF011, 10, {0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x79}, false},
    {0xF0D0, 3, {0x17, 0x34, 0x51}, false},
    {0xF0D2, 6, {0x17, 0x34, 0x51, 0x68, 0x85, 0x10}, false},
    {0xF401, 1, {0x17}, false},
};
static const size_t num_did_entries = sizeof(did_entries) / sizeof(DIDEntry);

typedef struct
{
    uint16_t did;
    le_result_t result;
} WriteDIDResp_t;


typedef struct
{
    uint16_t did;
    uint8_t value[4092];
    size_t len;
    le_result_t result;
} ReadDIDResp_t;

#endif