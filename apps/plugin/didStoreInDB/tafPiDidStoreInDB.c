/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "tafPiDidStoreInDB.h"
#include "tafPiDiagDID.h"
#include "tafDIDDataAccessComp.h"


TAF_PI_DIAGDID_DATACHANGECALLBACK didCallback = NULL;
le_dls_List_t didEntryList = LE_DLS_LIST_INIT;

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
    valueChangeReq_t* req = (valueChangeReq_t*)(param1);
    if(req == NULL)
    {
        LE_ERROR("req is Null");
        return;
    }

    switch (req->Vrequest)
    {
        case VALUE_REQUEST_GET:
        {
            req->result = taf_DIDDataAccess_ReadDID(req->did, req->value, &req->len);
            LE_DEBUG("VALUE_REQUEST_GET Read DID:0x%x, size: %" PRIuS ", result: %d", req->did,
                req->len, req->result);

            if (req->result == LE_OK)
            {
                if(req->getHandler != NULL)
                {
                    req->getHandler(req->did, req->value, req->len, req->result, req->ctxPtr);
                }
                else
                {
                    LE_ERROR("getHandler is NULL.");
                }
            }
            break;
        }
        case VALUE_REQUEST_SET:
        {
            //Don't write data into DB if it's more than MAX_DID_REQ_LEN bytes
            if(req->len > MAX_DID_REQ_LEN)
            {
                LE_ERROR("Incorrect length.");
                req->result = LE_BAD_PARAMETER;
            }
            else
            {
                req->result = taf_DIDDataAccess_WriteDID(req->did, req->value, req->len);

                LE_DEBUG("VALUE_REQUEST_SET write DID: 0x%x, size: %" PRIuS ", result: %d",
                    req->did, req->len, req->result);

                if(req->setHandler != NULL)
                {
                    req->setHandler(req->did, req->result, req->ctxPtr);
                }
                else
                {
                    LE_ERROR("setHandler is NULL.");
                }
            }

            if(req->result != LE_OK)
            {
                LE_ERROR("Failed to write data for DID 0x%x in DB. Don't notify it", req->did);
                break;
            }

            // Check the DID in whitelist, is it registered for change notification.
            le_dls_Link_t* linkPtr = NULL;
            linkPtr = le_dls_Peek(&didEntryList);
            while (linkPtr)
            {
                DIDEntry_t* didEntryPtr = CONTAINER_OF(linkPtr, DIDEntry_t, link);
                if (didEntryPtr != NULL && didEntryPtr->did == req->did)
                {
                    DIDEvent_t didEvent;
                    didEvent.did = req->did;
                    memcpy(didEvent.value, req->value, req->len);

                    didEvent.len = req->len;

                    // Fire event for DID change notification
                    LE_DEBUG("DID %x change notified", req->did);
                    le_event_Report(NotifyDidEventId, &didEvent, sizeof(didEvent));
                    break;
                }
                linkPtr = le_dls_PeekNext(&didEntryList, linkPtr);
            }

            break;
        }
        default:
            LE_ERROR("Invalid request type: %u", req->Vrequest);
            break;
    }

    le_mem_Release(req);
}


//--------------------------------------------------------------------------------------------------
/**
* Gets DID value asynchronously.
*/
//--------------------------------------------------------------------------------------------------
le_result_t taf_pi_didStorg_GetAsync
(
    uint16_t dataID,
    TAF_PI_DIAGDID_GETHANDLER handler,
    void* ctxPtr
)
{
    LE_DEBUG("GetAsync for DID: 0x%x", dataID);
    if(handler == NULL || ctxPtr == NULL)
    {
        LE_ERROR("Invalid handler or context pointer");
        return LE_BAD_PARAMETER;
    }

    valueChangeReq_t* req = (valueChangeReq_t *)le_mem_ForceAlloc(ValueRequestPoolRef);

    req->Vrequest = VALUE_REQUEST_GET;
    req->did = dataID;
    req->len = MAX_DID_REQ_LEN;
    req->result = LE_OK;
    req->ctxPtr = ctxPtr;
    req->getHandler = handler;
    req->setHandler = NULL;

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
    TAF_PI_DIAGDID_SETHANDLER handler,
    void* ctxPtr
)
{
    LE_DEBUG("SetAsync for DID: 0x%x, len= %" PRIuS "", dataID, len);

    if(handler == NULL || ctxPtr == NULL || value == NULL || len == 0 || len > MAX_DID_REQ_LEN)
    {
        LE_ERROR("Invalid parameter");
        return LE_BAD_PARAMETER;
    }

    valueChangeReq_t* req = (valueChangeReq_t *)le_mem_ForceAlloc(ValueRequestPoolRef);

    req->Vrequest = VALUE_REQUEST_SET;
    req->did = dataID;

    memcpy(req->value, value, len);
    req->len = len;
    req->result = LE_OK;
    req->ctxPtr = ctxPtr;
    req->getHandler = NULL;
    req->setHandler = handler;

    le_event_QueueFunction(ValueChangeRequest, (void*)(req), NULL);

    return LE_OK;
}


static void NotifyDidRespHandler
(
    void* context
)
{
    DIDEvent_t* didEntryPtr = (DIDEvent_t*)context;
    if(didEntryPtr == NULL)
    {
        LE_ERROR("context is NULL");
        return;
    }

    uint16_t did = didEntryPtr->did;
    uint8_t* value = didEntryPtr->value;
    size_t len = didEntryPtr->len;

    LE_DEBUG("NotifyDidRespHandler did=0x%x, len=%" PRIuS "", did,len);

    if (didCallback != NULL)
    {
        didCallback(did, value, len);
    }
    else
    {
        LE_ERROR("didCallback is NULL.");
    }
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
    // Add DataID to the list
    DIDEntry_t* didEntryPtr = (DIDEntry_t *)le_mem_ForceAlloc(DidEntryPoolRef);
    memset(didEntryPtr, 0, sizeof(DIDEntry_t));

    didEntryPtr->did = dataID;
    didEntryPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&didEntryList, &didEntryPtr->link);

    return LE_OK;
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
    le_dls_Link_t* linkPtr = NULL;

    linkPtr = le_dls_Peek(&didEntryList);
    while (linkPtr)
    {
        DIDEntry_t* didEntryPtr = CONTAINER_OF(linkPtr, DIDEntry_t, link);
        linkPtr = le_dls_PeekNext(&didEntryList, linkPtr);

        if (didEntryPtr->did == dataID)
        {
            le_dls_Remove(&didEntryList, &(didEntryPtr->link));
            le_mem_Release(didEntryPtr);
            return LE_OK;
        }
    }

    return LE_FAULT;
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

    // Create the memory pools.
    ValueRequestPoolRef = le_mem_CreatePool("ValueRequestPoolRef", sizeof(valueChangeReq_t));

    DidEntryPoolRef = le_mem_CreatePool("didEntryPool", sizeof(DIDEntry_t));

    // Create an event Id for suspend response event.
    NotifyDidEventId = le_event_CreateId("NotifyDidEventId", sizeof(DIDEvent_t));

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