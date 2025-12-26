/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include "tafSocksImpl.hpp"
#include "taf_pa_socks.hpp"
#include "tafNetUtility.hpp"
#include "tafSvcIF.hpp"

#define ENABLE_SOCKS_MAX_NUMBER_AT_THE_SAME_TIME 1

using namespace tafsvc;

LE_MEM_DEFINE_STATIC_POOL(HandlerMappingPool, TAF_NET_SOCKSV5_MAX_MAPPING_POOL,
                          sizeof(SocksHandlerMapping_t));

le_event_Id_t taf_Socks::socksCmdId = nullptr;
le_event_Id_t taf_Socks::socksEvId = nullptr;

/*======================================================================

 FUNCTION        taf_Socks::Init

 DESCRIPTION     Initialization of the taf Socks component

 DEPENDENCIES    The initialization of telaf.

 PARAMETERS      None

 RETURN VALUE    None

======================================================================*/
void taf_Socks::Init(void)
{

    le_result_t isReady = LE_OK;

    isReady =  PA_TO_LE_RESULT(taf_pa_socks_Init());

    if(isReady == LE_OK)
    {
        LE_INFO("socksManager component is ready...");
    }
    else
    {
        LE_CRIT("unable to init socksManager component!");
    }


    // 3. Initiate the memory pool
    HandlerMappingPool = le_mem_InitStaticPool(HandlerMappingPool, TAF_NET_SOCKSV5_MAX_MAPPING_POOL,
                                               sizeof(SocksHandlerMapping_t));

    // 4. Create and start callback event thread.
    le_sem_Ref_t socksEvtSemRef = le_sem_Create("SocksEvtThreadSem", 0);
    socksEvId = le_event_CreateId("socksEvId", sizeof(taf_SocksEventType_t));
    le_thread_Ref_t socksEvtThreadRef = le_thread_Create("socksEvtThread", SocksEvtThread,
                                                         (void*)socksEvtSemRef);
    le_thread_Start(socksEvtThreadRef);

    le_sem_Wait(socksEvtSemRef);

    // Delete semaphore.
    le_sem_Delete(socksEvtSemRef);

    // 5. Create and start SOCKS command thread.
    le_sem_Ref_t socksCmdThreadSem = le_sem_Create("socksCmdThreadSem", 0);
    socksCmdId = le_event_CreateId("socksCmd", sizeof(taf_SocksCmdReq_t));
    le_thread_Ref_t socksCmdThreadRef = le_thread_Create("socksCmdThread", SocksCmdThread,
                                                         (void*)socksCmdThreadSem);
    le_thread_Start(socksCmdThreadRef);

    le_sem_Wait(socksCmdThreadSem);

    // Delete semaphore.
    le_sem_Delete(socksCmdThreadSem);

    //6. Add close handler
    le_msg_AddServiceCloseHandler(taf_net_GetServiceRef(), CloseEventHandler, NULL);
    return;
}

/*======================================================================

 FUNCTION        taf_Socks::GetInstance

 DESCRIPTION     Get the instance of Socks.

 DEPENDENCIES    The initialization of Socks.

 PARAMETERS      None

 RETURN VALUE    taf_Socks &

======================================================================*/
taf_Socks &taf_Socks::GetInstance()
{
    static taf_Socks instance;
    return instance;
}

/*======================================================================
 FUNCTION        taf_Socks::SocksEvtHandler
 DESCRIPTION     Asynchrous callback event handler.

 DEPENDENCIES    The initialization of socks event thread.

 PARAMETERS      [IN] void* cmdReqPtr: Command request pointer.

 RETURN VALUE    None

======================================================================*/
void taf_Socks::SocksEvtHandler(void* cmdReqPtr)
{
    le_result_t result = LE_OK;
    taf_SocksEventType_t* cmdReq = (taf_SocksEventType_t*)cmdReqPtr;
    SocksHandlerMapping_t *socksHandlerMappingPtr = NULL;
    TAF_ERROR_IF_RET_NIL(cmdReqPtr == NULL, "Input parameter is NULL");

    auto &tafSocks = taf_Socks::GetInstance();

    LE_DEBUG(" Received event = %d",cmdReq->event);

    switch(cmdReq->event)
    {
        case EVT_ENABLE_SOCKS_ASYNC_CALLBACK:
            if (cmdReq->errorCode != LE_OK)
            {
                LE_ERROR( "ENABLE_SOCKS_ASYNC_EVT failed with errorCode: %d ",
                           static_cast<int>(cmdReq->errorCode));
                result = LE_FAULT;
            }
            else
            {
                LE_DEBUG("ENABLE_SOCKS_ASYNC_EVT processed successfully \n");
                result = LE_OK;
            }

            //Find the handler function and then call it
            socksHandlerMappingPtr = tafSocks.FindAsyncHandler(ASYNC_ENABLE_SOCKS);

            if(socksHandlerMappingPtr != NULL)
            {
                //Call handler function, and then remove it from mapping list
                socksHandlerMappingPtr->asyncHandler(result,socksHandlerMappingPtr->contextPtr);
                tafSocks.DeleteHandlerInfo(socksHandlerMappingPtr->asyncHandler);
            }

        break;
        case EVT_DISABLE_SOCKS_ASYNC_CALLBACK:
            if (cmdReq->errorCode != LE_OK)
            {
                LE_ERROR( "DISABLE_SOCKS_ASYNC_EVT failed with errorCode: %d ",
                           static_cast<int>(cmdReq->errorCode));
                result = LE_FAULT;
            }
            else
            {
                LE_DEBUG("DISABLE_SOCKS_ASYNC_EVT processed successfully \n");
                result = LE_OK;
            }

            //Find the handler function and then call it
            socksHandlerMappingPtr = tafSocks.FindAsyncHandler(ASYNC_DISABLE_SOCKS);

            if(socksHandlerMappingPtr != NULL)
            {
                //Call handler function, and then remove it from mapping list
                socksHandlerMappingPtr->asyncHandler(result,socksHandlerMappingPtr->contextPtr);
                tafSocks.DeleteHandlerInfo(socksHandlerMappingPtr->asyncHandler);
            }
        break;
        default:
                LE_ERROR("Command error");
        break;
    }

}

/*======================================================================

 FUNCTION        taf_Socks::SocksEvtThread

 DESCRIPTION     Socks event thread for handling asynchronous request.

 DEPENDENCIES    The initialization of Socks.

 PARAMETERS      [IN] void* contextPtr: Context pointer.

 RETURN VALUE    void*
                     NULL: Success.

======================================================================*/
void* taf_Socks::SocksEvtThread(void* contextPtr)
{
    le_event_AddHandler("SocksEvtHandler", socksEvId,
                                                         SocksEvtHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return nullptr;
}

/*======================================================================
 FUNCTION        taf_Socks::SocksProcCmdHandler
 DESCRIPTION     Socks command handler.

 DEPENDENCIES    The initialization of socks command thread.

 PARAMETERS      [IN] void* cmdReqPtr: Command request pointer.

 RETURN VALUE    None

======================================================================*/
void taf_Socks::SocksProcCmdHandler(void* cmdReqPtr)
{
    le_result_t result = LE_OK;
    taf_SocksCmdReq_t* cmdReq = (taf_SocksCmdReq_t*)cmdReqPtr;
    auto &tafSocks = taf_Socks::GetInstance();

    TAF_ERROR_IF_RET_NIL(cmdReqPtr == NULL, "Input parameter is NULL");

    LE_DEBUG(" Received command = %d",cmdReq->cmdType);

    switch(cmdReq->cmdType)
    {
        case ASYNC_ENABLE_SOCKS:
            //Only one client can enable socks at the same time.
            if( tafSocks.GetHandlerNumberInMappingList(ASYNC_ENABLE_SOCKS) >=
                ENABLE_SOCKS_MAX_NUMBER_AT_THE_SAME_TIME )
            {
                LE_DEBUG("Only one client can enable SOCKS at the same time");
                return;
            }

            //Add handler function into mapping list, will be used later
            if(cmdReq->handlerFuncPtr != NULL)
                tafSocks.AddHandlerSessionMapping(cmdReq->handlerFuncPtr, cmdReq->contextPtr,
                                                  cmdReq->sessionRef, ASYNC_ENABLE_SOCKS);

            //Call telsdk API
            result = tafSocks.EnableSocks(ASYNC_ENABLE_SOCKS);

            if (result != LE_OK)
            {
                LE_ERROR("Enable socks asynchronously error %d", result);
                if(cmdReq->handlerFuncPtr != NULL)
                    tafSocks.DeleteHandlerInfo(cmdReq->handlerFuncPtr);
            }
        break;
        case ASYNC_DISABLE_SOCKS:
            //Only one client can disable socks at the same time.
            if( tafSocks.GetHandlerNumberInMappingList(ASYNC_DISABLE_SOCKS) >=
                ENABLE_SOCKS_MAX_NUMBER_AT_THE_SAME_TIME )
            {
                LE_DEBUG("Only one client can disable SOCKS at the same time");
                return;
            }

            //Add handler function into mapping list, will be used later
            if(cmdReq->handlerFuncPtr != NULL)
                tafSocks.AddHandlerSessionMapping(cmdReq->handlerFuncPtr, cmdReq->contextPtr,
                                                  cmdReq->sessionRef, ASYNC_DISABLE_SOCKS);

            //Call telsdk API
            result = tafSocks.EnableSocks(ASYNC_DISABLE_SOCKS);

            if (result != LE_OK)
            {
                LE_ERROR("Disable socks asynchronously error %d", result);
                if(cmdReq->handlerFuncPtr != NULL)
                    tafSocks.DeleteHandlerInfo(cmdReq->handlerFuncPtr);
            }
        break;
        default:
                LE_ERROR("Command error");
        break;
    }
}

/*======================================================================

 FUNCTION        taf_Socks::SocksAsyncCmdThread

 DESCRIPTION     Socks command thread for handling command request.

 DEPENDENCIES    The initialization of Socks.

 PARAMETERS      [IN] void* contextPtr: Context pointer.

 RETURN VALUE    void*
                     NULL: Success.

======================================================================*/
void* taf_Socks::SocksCmdThread(void* contextPtr)
{
    le_event_AddHandler("SocksProcAsyncCmdHandler", socksCmdId,
                                                         SocksProcCmdHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return nullptr;
}

/*======================================================================

 FUNCTION        tafSocksCallback::enableSocksAsyncResponse

 DESCRIPTION     Call back function for enabling SOCKS asynchronously.

 DEPENDENCIES    The initialization of Socks.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafSocksCallback::enableSocksAsyncResponse(pa_result_t error,void *contextPtr)
{
    auto &tafSocks = taf_Socks::GetInstance();
    taf_SocksEventType_t socksEvent;
    LE_UNUSED(contextPtr);
    le_result_t le_error = PA_TO_LE_RESULT(error);

    socksEvent.event         = EVT_ENABLE_SOCKS_ASYNC_CALLBACK;
    socksEvent.errorCode     = le_error;

    le_event_Report(tafSocks.socksEvId, &socksEvent,sizeof(taf_SocksEventType_t));
}

/*======================================================================

 FUNCTION        tafSocksCallback::disableSocksAsyncResponse

 DESCRIPTION     Call back function for disabling SOCKS asynchronously.

 DEPENDENCIES    The initialization of Socks.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafSocksCallback::disableSocksAsyncResponse(pa_result_t error,void *contextPtr)
{
    auto &tafSocks = taf_Socks::GetInstance();
    taf_SocksEventType_t socksEvent;
    LE_UNUSED(contextPtr);
    le_result_t le_error = PA_TO_LE_RESULT(error);

    socksEvent.event         = EVT_DISABLE_SOCKS_ASYNC_CALLBACK;
    socksEvent.errorCode     = le_error;

    le_event_Report(tafSocks.socksEvId, &socksEvent,sizeof(taf_SocksEventType_t));
}

/*======================================================================

 FUNCTION        taf_Socks::EnableSocksCmdSync

 DESCRIPTION     Synchronously enable socks.

 DEPENDENCIES    The initialization of socks.

 PARAMETERS      None

 RETURN VALUE    le_result_t
                     LE_OK:                      Success.
                     LE_TIMEOUT                  Timeout.
                     LE_FAULT                    Failure.

======================================================================*/
le_result_t taf_Socks::EnableSocksCmdSync()
{
    le_result_t result;
    result = PA_TO_LE_RESULT(taf_pa_net_EnableSocksCmdSync());
    return result;
}

/*======================================================================

 FUNCTION        taf_Socks::DisableSocksCmdSync

 DESCRIPTION     Synchronously disable socks.

 DEPENDENCIES    The initialization of socks.

 PARAMETERS      None

 RETURN VALUE    le_result_t
                     LE_OK:                      Success.
                     LE_TIMEOUT                  Timeout.
                     LE_FAULT                    Failure.

======================================================================*/
le_result_t taf_Socks::DisableSocksCmdSync()
{
    le_result_t result;
    result = PA_TO_LE_RESULT(taf_pa_net_DisableSocksCmdSync());
    return result;

}

/*======================================================================

 FUNCTION        taf_Socks::EnableSocksCmdAsync

 DESCRIPTION     Asynchronously enable socks.

 DEPENDENCIES    The initialization of socks.

 PARAMETERS      [IN] handlerPtr: The handler function.
                 [IN] contextPtr: The context pointer.
                 [IN] sessionRef: The client sessionRef.

 RETURN VALUE    None

======================================================================*/
void taf_Socks::EnableSocksCmdAsync
(
    taf_net_AsyncSocksHandlerFunc_t handlerPtr,
    void* contextPtr,
    le_msg_SessionRef_t sessionRef
)
{
    taf_SocksCmdReq_t cmdReq;

    TAF_ERROR_IF_RET_NIL(handlerPtr == NULL, "Handler function is NULL");
    TAF_ERROR_IF_RET_NIL(sessionRef == NULL, "sessionRef is NULL");

    cmdReq.cmdType = ASYNC_ENABLE_SOCKS;
    cmdReq.contextPtr = contextPtr;
    cmdReq.handlerFuncPtr = handlerPtr;
    cmdReq.sessionRef = sessionRef;

    // Send ASYNC_ENABLE_SOCKS command
    le_event_Report(taf_Socks::socksCmdId, &cmdReq, sizeof(cmdReq));
}

/*======================================================================

 FUNCTION        taf_Socks::DisableSocksCmdAsync

 DESCRIPTION     Asynchronously disable socks.

 DEPENDENCIES    The initialization of socks.

 PARAMETERS      [IN] handlerPtr: The handler function.
                 [IN] contextPtr: The context pointer.
                 [IN] sessionRef: The client sessionRef.

 RETURN VALUE    None

======================================================================*/
void taf_Socks::DisableSocksCmdAsync
(
    taf_net_AsyncSocksHandlerFunc_t handlerPtr,
    void* contextPtr,
    le_msg_SessionRef_t sessionRef
)
{
    taf_SocksCmdReq_t cmdReq;

    TAF_ERROR_IF_RET_NIL(handlerPtr == NULL, "Handler function is NULL");
    TAF_ERROR_IF_RET_NIL(sessionRef == NULL, "sessionRef is NULL");

    cmdReq.cmdType = ASYNC_DISABLE_SOCKS;
    cmdReq.contextPtr = contextPtr;
    cmdReq.sessionRef = sessionRef;
    cmdReq.handlerFuncPtr = handlerPtr;

    // Send ASYNC_DISABLE_SOCKS command
    le_event_Report(taf_Socks::socksCmdId, &cmdReq, sizeof(cmdReq));
}

/*======================================================================

 FUNCTION        taf_Socks::EnableSocks

 DESCRIPTION     Enable socks by calling telsdk API.

 DEPENDENCIES    The initialization of socksManager.

 PARAMETERS      None

 RETURN VALUE    le_result_t
                     LE_OK:                      Success.
                     LE_NOT_FOUND                SocksManager is not found.
                     LE_FAULT                    Failure.

======================================================================*/
le_result_t taf_Socks::EnableSocks(taf_SocksCmdType_t type)
{
    le_result_t result;

    switch(type)
    {
        case ASYNC_ENABLE_SOCKS:
             result = PA_TO_LE_RESULT(taf_pa_net_EnableSocksCmdASync(tafSocksCallback::enableSocksAsyncResponse,nullptr));
        break;

        case ASYNC_DISABLE_SOCKS:
            result = PA_TO_LE_RESULT(taf_pa_net_DisableSocksCmdASync(tafSocksCallback::disableSocksAsyncResponse,nullptr));
        break;

        default:
            return LE_FAULT;
    }

    if (result == LE_OK)
    {
        return LE_OK;
    }
    else
    {
        LE_ERROR( "ERROR - Failed to enable/disable socks, Status:%d ", static_cast<int>(result));
        return LE_FAULT;
    }
}

/*======================================================================

 FUNCTION        taf_Socks::AddHandlerSessionMapping

 DESCRIPTION     Add handler and session into mapping list.

 DEPENDENCIES    The initialization of socks.

 PARAMETERS      [IN] asyncHandler: The handler function.
                 [IN] contextPtr: The context pointer.
                 [IN] sessionRef: The client session reference.
                 [IN] type: The command type.

 RETURN VALUE    None

======================================================================*/
void taf_Socks::AddHandlerSessionMapping
(
    taf_net_AsyncSocksHandlerFunc_t asyncHandler,
    void *contextPtr,
    le_msg_SessionRef_t sessionRef,
    taf_SocksCmdType_t type
)
{
    SocksHandlerMapping_t *handlerSessionMapping;

    TAF_ERROR_IF_RET_NIL(asyncHandler == nullptr, "Null ptr");

    handlerSessionMapping = (SocksHandlerMapping_t *)le_mem_ForceAlloc(HandlerMappingPool);

    TAF_ERROR_IF_RET_NIL(handlerSessionMapping == nullptr ,
                         "Failed to alloc memory for handlerSessionMapping");

    memset(handlerSessionMapping, 0, sizeof(SocksHandlerMapping_t));

    handlerSessionMapping->asyncHandler = asyncHandler;
    handlerSessionMapping->contextPtr = contextPtr;
    handlerSessionMapping->sessionRef = sessionRef;
    handlerSessionMapping->type = type;
    handlerSessionMapping->handlerLink = LE_DLS_LINK_INIT;

    le_dls_Queue(&SocksHandlerMappingList, &handlerSessionMapping->handlerLink);
}

/*======================================================================

 FUNCTION        taf_Socks::FindAsyncHandler

 DESCRIPTION     Find handler function by type.

 DEPENDENCIES    The initialization of SocksHandlerMappingList.

 PARAMETERS      [IN] type: The command type.

 RETURN VALUE    SocksHandlerMapping_t*   The pointer.

======================================================================*/
SocksHandlerMapping_t* taf_Socks::FindAsyncHandler
(
    taf_SocksCmdType_t type
)
{
    SocksHandlerMapping_t *handlerSessionInfo;
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&SocksHandlerMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, SocksHandlerMapping_t, handlerLink);
        if (handlerSessionInfo->asyncHandler != NULL && handlerSessionInfo->type == type)
        {
            LE_DEBUG("Found async handler %p ", handlerSessionInfo->asyncHandler);
            return handlerSessionInfo;
        }
        handlerLinkPtr = le_dls_PeekNext(&SocksHandlerMappingList, handlerLinkPtr);
    }

    return NULL;
}

/*======================================================================

 FUNCTION        taf_Socks::GetHandlerNumberInMappingList

 DESCRIPTION     Get the number of handler function in mapping list.

 DEPENDENCIES    The initialization of SocksHandlerMappingList.

 PARAMETERS      [IN] type: The command type.

 RETURN VALUE    int         The number of handler function in mapping list.

======================================================================*/
int taf_Socks::GetHandlerNumberInMappingList
(
    taf_SocksCmdType_t type
)
{
    int counter=0;
    SocksHandlerMapping_t *handlerSessionInfo;
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&SocksHandlerMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, SocksHandlerMapping_t, handlerLink);
        if (handlerSessionInfo->asyncHandler != NULL && handlerSessionInfo->type == type)
        {
            counter++;
        }
        handlerLinkPtr = le_dls_PeekNext(&SocksHandlerMappingList, handlerLinkPtr);
    }

    return counter;
}

/*======================================================================

 FUNCTION        taf_Socks::DeleteSessionHandlersInfo

 DESCRIPTION     Delete the handler found with sessionRef in mapping list.

 DEPENDENCIES    The initialization of SocksHandlerMappingList.

 PARAMETERS      [IN] sessionRef: The client session reference.

 RETURN VALUE    None

======================================================================*/
void taf_Socks::DeleteSessionHandlersInfo
(
    le_msg_SessionRef_t sessionRef
)
{
    SocksHandlerMapping_t *handlerSessionInfo;
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&SocksHandlerMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, SocksHandlerMapping_t, handlerLink);
        handlerLinkPtr = le_dls_PeekNext(&SocksHandlerMappingList, handlerLinkPtr);
        if (handlerSessionInfo->sessionRef == sessionRef)
        {
            le_dls_Remove(&SocksHandlerMappingList, &handlerSessionInfo->handlerLink);

            le_mem_Release(handlerSessionInfo);
        }
    }
}

/*======================================================================

 FUNCTION        taf_Socks::DeleteHandlerInfo

 DESCRIPTION     Delete the handler in mapping list.

 DEPENDENCIES    The initialization of SocksHandlerMappingList.

 PARAMETERS      [IN] asyncHandler: The handler function.

 RETURN VALUE    None

======================================================================*/
void taf_Socks::DeleteHandlerInfo(taf_net_AsyncSocksHandlerFunc_t asyncHandler)
{
    SocksHandlerMapping_t *handlerSessionInfo;
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&SocksHandlerMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, SocksHandlerMapping_t, handlerLink);
        handlerLinkPtr = le_dls_PeekNext(&SocksHandlerMappingList, handlerLinkPtr);
        if (handlerSessionInfo->asyncHandler == asyncHandler)
        {
            le_dls_Remove(&SocksHandlerMappingList, &handlerSessionInfo->handlerLink);

            le_mem_Release(handlerSessionInfo);
            break;
        }
    }
}

/*======================================================================

 FUNCTION        taf_Socks::CloseEventHandler

 DESCRIPTION     Function to be called when the client closed.

 DEPENDENCIES    The initialization of SOCKS.

 PARAMETERS      [IN] sessionRef: The client session reference.
                 [IN] contextPtr: The context pointer.

 RETURN VALUE    None

======================================================================*/
void taf_Socks::CloseEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    auto &tafSocks = taf_Socks::GetInstance();
    tafSocks.DeleteSessionHandlersInfo(sessionRef);
}
