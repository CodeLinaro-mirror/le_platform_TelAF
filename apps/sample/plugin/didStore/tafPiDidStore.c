/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "le_cfg_interface.h"
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

//--------------------------------------------------------------------------------------------------
/**
* Used in QueueFunction to process Value change request
*/
//--------------------------------------------------------------------------------------------------
static void ValueChangeRequest
(
    void* param1,
    void* param2
)
{
    LE_INFO("DIDStorg_PI: %s", __FUNCTION__);

    valueChangeReq_t* req = (valueChangeReq_t*)(param1);

    // Ensure the le_cfg service is connected
    le_cfg_ConnectService();

    char didPath[128] = {0};
    snprintf(didPath, sizeof(didPath), "diag/DID/%u", req->did);

    switch (req->Vrequest)
    {
        case VALUE_REQUEST_GET:
        {
            LE_INFO("Processing VALUE_REQUEST_GET for DID: %u", req->did);

            le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn(didPath);
            if (!iteratorRef)
            {
                LE_ERROR("Failed to create read transaction for DID path: %s", didPath);
                req->result = LE_FAULT;
                break;
            }

            if (le_cfg_NodeExists(iteratorRef, "value"))
            {
                uint8_t defaultValue[256] = {0}; // Default value in case the key doesn't exist
                size_t valueSize = sizeof(req->value);

                le_cfg_GetBinary(iteratorRef, "value", req->value, &valueSize, defaultValue,
                    sizeof(defaultValue));

                req->len = valueSize;
                req->result = LE_OK;

                LE_INFO("Retrieved value for DID %u, size: %zu", req->did, valueSize);
            }
            else
            {
                LE_ERROR("DID %u not found in configTree.", req->did);
                req->result = LE_NOT_FOUND;
            }

            le_cfg_CancelTxn(iteratorRef);

            if (req->result == LE_OK)
            {
                if(getCallBackFunc != NULL)
                {
                    getCallBackFunc(req->did, req->value, req->len, req->result);
                }
                else
                {
                    LE_ERROR("getCallBackFunc is NULL.");
                }
            }
            break;
        }
        case VALUE_REQUEST_SET:
        {
            LE_INFO("Processing VALUE_REQUEST_SET for DID: %u", req->did);

            le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn(didPath);
            if (!iteratorRef)
            {
                LE_ERROR("Failed to create write transaction for DID path: %s", didPath);
                req->result = LE_FAULT;
                break;
            }

            // Write the value into the configTree
            le_cfg_SetBinary(iteratorRef, "value", req->value, req->len);

            // Commit the transaction
            le_cfg_CommitTxn(iteratorRef);

            LE_INFO("Successfully set value for DID %u, size: %zu", req->did, req->len);

            req->result = LE_OK;

            if(setCallbackFunc != NULL)
            {
                setCallbackFunc(req->did, req->result);
            }
            else
            {
                LE_ERROR("setCallbackFunc is NULL.");
            }
            // Fire event for DID change notification
            le_event_Report(NotifyDidEventId, (void*)&req->did, sizeof(req->did));

            break;
        }
        default:
            LE_ERROR("Invalid request type: %u", req->Vrequest);
            le_mem_Release(req);
            return;
    }
}


//--------------------------------------------------------------------------------------------------
/**
* Gets DID value asynchronously.
*/
//--------------------------------------------------------------------------------------------------
le_result_t taf_pi_didStorg_GetAsync
(
    uint16_t dataID,
    TAF_PI_DIAGDID_GETHANDLER handler
)
{
    LE_INFO("taf_pi_didStorg_GetAsync for DID: %u", dataID);

    valueChangeReq_t* req = (valueChangeReq_t *)le_mem_ForceAlloc(ValueRequestPoolRef);
    if (req == NULL)
    {
        return LE_NO_MEMORY;
    }

    req->Vrequest = VALUE_REQUEST_GET;
    req->did = dataID;
    req->value = calloc(1, 256); // Allocate space for value
    req->len = 0;
    req->result = LE_OK;

    getCallBackFunc = handler;

    le_event_QueueFunction(ValueChangeRequest, (void*)(req), NULL);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
* Sets DID value asynchronously.
*/
//--------------------------------------------------------------------------------------------------
le_result_t taf_pi_didStorg_SetAsync
(
    uint16_t dataID,
    uint8_t *value,
    size_t len,
    TAF_PI_DIAGDID_SETHANDLER handler
)
{
    LE_INFO("taf_pi_didStorg_SetAsync for DID: %u", dataID);

    valueChangeReq_t* req = (valueChangeReq_t *)le_mem_ForceAlloc(ValueRequestPoolRef);
    if (req == NULL)
    {
        return LE_NO_MEMORY;
    }

    req->Vrequest = VALUE_REQUEST_SET;
    req->did = dataID;
    req->value = calloc(1, len); // Allocate space for value
    if (req->value == NULL)
    {
        le_mem_Release(req);
        return LE_NO_MEMORY;
    }
    memcpy(req->value, value, len);
    req->len = len;
    req->result = LE_OK;

    setCallbackFunc = handler;

    // Notify all clients of the DID change
    LE_INFO("Notifying clients about the DID change: %u", dataID);
    le_event_Report(NotifyDidEventId, &dataID, sizeof(dataID));

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
