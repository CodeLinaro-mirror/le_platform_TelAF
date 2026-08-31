/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "tafPiDiagDID.h"

#ifndef DIDSTOREINDB_PLUGIN_H
#define DIDSTOREINDB_PLUGIN_H

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for suspend response.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NotifyDidEventId;
static le_mem_PoolRef_t ValueRequestPoolRef;
static le_mem_PoolRef_t DidEntryPoolRef;

#define MAX_DID_REQ_LEN 4092

typedef enum
{
    VALUE_REQUEST_GET,
    VALUE_REQUEST_SET
} ValueRequest;

typedef struct {
    ValueRequest Vrequest;
    uint16_t did;
    uint8_t value[MAX_DID_REQ_LEN];
    size_t len;
    le_result_t result;
    void* ctxPtr;
    TAF_PI_DIAGDID_GETHANDLER getHandler;
    TAF_PI_DIAGDID_SETHANDLER setHandler;
}valueChangeReq_t;

typedef struct {
    uint16_t did;
    le_dls_Link_t link;
} DIDEntry_t;

typedef struct {
    uint16_t did;
    size_t len;
    uint8_t value[MAX_DID_REQ_LEN];
} DIDEvent_t;

typedef struct
{
    uint16_t did;
    le_result_t result;
} WriteDIDResp_t;

typedef struct
{
    uint16_t did;
    uint8_t value[MAX_DID_REQ_LEN];
    size_t len;
    le_result_t result;
} ReadDIDResp_t;

#endif