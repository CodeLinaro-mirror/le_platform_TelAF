/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafPiDidStore.h"
#include "tafPiDiagDID.h"


TAF_PI_DIAGDID_DATACHANGECALLBACK didCallback = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Callback functions.
 */
//--------------------------------------------------------------------------------------------------
static TAF_PI_DIAGDID_GETHANDLER getCallBackFunc = NULL;
static TAF_PI_DIAGDID_SETHANDLER setCallbackFunc = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Get DID storage information table.
 *
 * @return
 * - NULL   -- Failed.
 * - Others -- DID Storage module information.
 */
//--------------------------------------------------------------------------------------------------
void* taf_hal_GetModInf()
{
    LE_INFO("Get DID Storage module information table.");

    return &(TAF_HAL_INFO_TAB.diagInf);
}


// Used in QueueFunction to process Value change request
static void ValueChangeRequest
(
    void* param1,
    void* param2
)
{
    LE_INFO("DIDStorg_PI: %s", __FUNCTION__);

    valueChangeReq_t* req = (valueChangeReq_t*)(param1);

    switch (req->Vrequest)
    {
        case VALUE_REQUEST_GET:
        {
            LE_INFO("send the value change request with request: %u ......",
                    req->Vrequest);
            // Initialize a ReadDIDResp_t structure to store the response of the read operation
            ReadDIDResp_t readDIDResp = {0};

            // Copy the request's DID, value, length, and result to the readDIDResp structure
            readDIDResp.did = req->did;
            memcpy(readDIDResp.value, req->value, req->len);
            readDIDResp.len = req->len;
            readDIDResp.result = req->result;

            // Call the callback function to handle the read DID response
            getCallBackFunc(readDIDResp.did,readDIDResp.value, readDIDResp.len,
                readDIDResp.result);
            break;
        }
        case VALUE_REQUEST_SET:
        {
            LE_INFO("send the value change request with request: %u ......",
                    req->Vrequest);

            // Initialize a WriteDIDResp_t structure to store the response of the write operation
            WriteDIDResp_t writeDIDResp = {0};

            // Copy the request's DID and result to the writeDIDResp structure
            writeDIDResp.did = req->did;
            writeDIDResp.result = req->result;

            // Call the callback function to handle the write DID response
            setCallbackFunc(writeDIDResp.did, writeDIDResp.result);
            break;
        }
        default:
            LE_ERROR("Invalid request type: %u", req->Vrequest);
            // Release the memory allocated for the request
            le_mem_Release(req);
            return;
    }

    le_mem_Release(req);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets DID value asynchronously.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t taf_pi_didStorg_GetAsync
(
    uint16_t dataID,
    TAF_PI_DIAGDID_GETHANDLER handler
)
{
    LE_INFO("taf_pi_didStorg_GetAsync");
    valueChangeReq_t* req = (valueChangeReq_t *)le_mem_ForceAlloc(ValueRequestPoolRef);
    if(req == NULL)
    {
        return LE_NO_MEMORY;
    }

    bool isAvailable = false;
    for (size_t i = 0; i < num_did_entries; ++i) {
        if (did_entries[i].did == dataID) {
            req->did = dataID;
            req->value = did_entries[i].value;
            req->len = did_entries[i].len;
            isAvailable = true;
            break;
        }
    }

    if (isAvailable){
        req->result = LE_OK;
    }
    else{
        req->result = LE_NOT_FOUND;
    }

    req->Vrequest = VALUE_REQUEST_GET;
    getCallBackFunc = handler;

    le_event_QueueFunction(ValueChangeRequest, (void*)(req), NULL);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets DID value asynchronously.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t taf_pi_didStorg_SetAsync
(
    uint16_t dataID,
    uint8_t *value,
    size_t len,
    TAF_PI_DIAGDID_SETHANDLER handler
)
{
    LE_INFO("taf_pi_didStorg_SetAsync");
    valueChangeReq_t* req = (valueChangeReq_t *)le_mem_ForceAlloc(ValueRequestPoolRef);
    if(req == NULL)
    {
        return LE_NO_MEMORY;
    }
    req->Vrequest = VALUE_REQUEST_SET;
    req->did = dataID;
    req->value = value;
    req->len = len;
    req->result = LE_OK;
    setCallbackFunc = handler;

    for (size_t i = 0; i < num_did_entries; ++i)
    {
        if (did_entries[i].did == req->did)
        {
            if (req->len > sizeof(did_entries[i].value))
            {
                req->result = LE_FAULT;
            }
            memcpy(did_entries[i].value,  req->value, req->len);
            did_entries[i].len = req->len;
            req->result = LE_OK;
        }
    }

    le_event_QueueFunction(ValueChangeRequest, (void*)(req), NULL);

    return LE_OK;
}


static void NotifyDidRespHandler
(
    void* context
)
{
    uint16_t did = ((DIDEntry*)context)->did;
    uint8_t value[] = {((DIDEntry*)context)->value[0]};
    uint8_t len = ((DIDEntry*)context)->len;
    didCallback(did, value, len);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add data change handler to plugin module.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t taf_pi_didStorg_AddHandler
(
    TAF_PI_DIAGDID_DATACHANGECALLBACK callback
)
{
    LE_INFO("taf_pi_didStorg_AddHandler...");
    didCallback = callback;
    return LE_OK;

}

//--------------------------------------------------------------------------------------------------
/**
 * Add DID change notify.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t taf_pi_didStorg_AddDidChangeNotify
(
    uint16_t dataID
)
{
    for (size_t i = 0; i < num_did_entries; ++i) {
        if (did_entries[i].did == dataID)
        {
            did_entries[i].changeNotify = true;
            return LE_OK;
        }
    }
	return LE_BAD_PARAMETER;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove DID change notify.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t taf_pi_didStorg_RemoveDidChangeNotify
(
    uint16_t dataID
)
{
    for (size_t i = 0; i < num_did_entries; ++i)
    {
        if (did_entries[i].did == dataID)
        {
            did_entries[i].changeNotify = false;
            return LE_OK;
        }
    }
    return LE_BAD_PARAMETER;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize DID Storage plugin.
 */
//--------------------------------------------------------------------------------------------------
void taf_pi_DidStorg_Init
(
    void
)
{
    LE_INFO("DID storage Plug-In Init...");

    LE_INFO("DID storage Plug-In Ready...");

    // Create the memory pools.
    ValueRequestPoolRef = le_mem_CreatePool("ValueRequestPoolRef", sizeof(valueChangeReq_t));

    // Create an event Id for suspend response event.
    NotifyDidEventId = le_event_CreateId("NotifyDidEventId", sizeof(DIDEntry));

    // Register handler for restart response events.
    le_event_AddHandler("NotifyDidRespHandler", NotifyDidEventId, NotifyDidRespHandler);

    LE_INFO("DID storage Plug-In started...");
    //le_event_RunLoop();
}


//--------------------------------------------------------------------------------------------------
/**
 * Did Storage information table.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED diagDID_InfoTab_t TAF_HAL_INFO_TAB =
{
    .mgrInf =
    {
        .name = TAF_DIAGDID_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .moduleType = TAF_MODULETYPE_PLUG_IN,
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    },

    .diagInf =
    {
        .init = taf_pi_DidStorg_Init,
        .diagDIDGetAsync = taf_pi_didStorg_GetAsync,
        .diagDIDSetAsync = taf_pi_didStorg_SetAsync,
        .addDataChangeHandler = taf_pi_didStorg_AddHandler,
        .diagDIDAddDataChangeNotification = taf_pi_didStorg_AddDidChangeNotify,
        .diagDIDRemoveDataChangeNotification = taf_pi_didStorg_RemoveDidChangeNotify,
    },
};

// DOT NOT USE COMPONET_INIT define module init for your service/apps
COMPONENT_INIT
{
    LE_INFO("Plugin is loading");
}