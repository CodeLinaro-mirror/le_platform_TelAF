/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include "tafSocksImpl.hpp"
#include "tafSvcIF.hpp"

#define ENABLE_SOCKS_TIMEOUT 30
#define ENABLE_SOCKS_MAX_NUMBER_AT_THE_SAME_TIME 1

using namespace telux::tafsvc;

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
    bool isReady = false;

    // 1. Get the DataFactory and socksManager instances
    if (socksManager == nullptr)
    {
        auto &dataFactory = telux::data::DataFactory::getInstance();
//SA415 using old telsdk,without initCb parameter
#ifdef TARGET_SA515M
        auto initCb = std::bind(&taf_Socks::onInitComplete, this, std::placeholders::_1);

        socksManager = dataFactory.getSocksManager(telux::data::OperationType::DATA_LOCAL, initCb);
#else
        socksManager = dataFactory.getSocksManager(telux::data::OperationType::DATA_LOCAL);
#endif
    }

    if(socksManager == nullptr )
    {
        LE_INFO("Socks manager initialize error...");
        return ;
    }

#ifdef TARGET_SA515M
    // 2. Check subsystem status
    std::unique_lock<std::mutex> lck(mMutex);

    telux::common::ServiceStatus subSystemStatus = socksManager->getServiceStatus();

    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE)
    {
        LE_INFO("Socks manager initialize...");
        conVar.wait(lck, [this]{return this->IsSubSystemStatusUpdated;});
        subSystemStatus = socksManager->getServiceStatus();
    }

    //At this point, initialization should be either AVAILABLE or Failure
    if (subSystemStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
    {
        LE_ERROR("Socks Manager initialization failed");
        socksManager = nullptr;
        return ;
    }
#endif

    isReady = socksManager->isSubsystemReady();

    if(isReady == false)
    {
        LE_INFO("Socks component is not ready, wait for it unconditionally...");
        std::future<bool> readyFunc = socksManager->onSubsystemReady();
        isReady = readyFunc.get();
    }

    if(isReady)
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
            if (cmdReq->errorCode != telux::common::ErrorCode::SUCCESS)
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
            if (cmdReq->errorCode != telux::common::ErrorCode::SUCCESS)
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

 FUNCTION        tafSocksCallback::enableSocksResponse

 DESCRIPTION     Call back function for enabling SOCKS.

 DEPENDENCIES    The initialization of Socks.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafSocksCallback::enableSocksResponse(telux::common::ErrorCode error)
{
    auto &tafSocks = taf_Socks::GetInstance();
    le_result_t result = LE_OK;

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR( "Request failed with errorCode: %d " , static_cast<int>(error));
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("Request processed successfully \n");
    }

    tafSocks.EnableSocksSyncPromise.set_value(result);
}

/*======================================================================

 FUNCTION        tafSocksCallback::disableSocksResponse

 DESCRIPTION     Call back function for disabling SOCKS.

 DEPENDENCIES    The initialization of Socks.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafSocksCallback::disableSocksResponse(telux::common::ErrorCode error)
{
    auto &tafSocks = taf_Socks::GetInstance();
    le_result_t result = LE_OK;

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR( "Request failed with errorCode: %d " , static_cast<int>(error));
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("Request processed successfully \n");
    }

    tafSocks.DisableSocksSyncPromise.set_value(result);
}

/*======================================================================

 FUNCTION        tafSocksCallback::enableSocksAsyncResponse

 DESCRIPTION     Call back function for enabling SOCKS asynchronously.

 DEPENDENCIES    The initialization of Socks.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafSocksCallback::enableSocksAsyncResponse(telux::common::ErrorCode error)
{
    auto &tafSocks = taf_Socks::GetInstance();
    taf_SocksEventType_t socksEvent;

    socksEvent.event         = EVT_ENABLE_SOCKS_ASYNC_CALLBACK;
    socksEvent.errorCode     = error;

    le_event_Report(tafSocks.socksEvId, &socksEvent,sizeof(taf_SocksEventType_t));
}

/*======================================================================

 FUNCTION        tafSocksCallback::disableSocksAsyncResponse

 DESCRIPTION     Call back function for disabling SOCKS asynchronously.

 DEPENDENCIES    The initialization of Socks.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafSocksCallback::disableSocksAsyncResponse(telux::common::ErrorCode error)
{
    auto &tafSocks = taf_Socks::GetInstance();
    taf_SocksEventType_t socksEvent;

    socksEvent.event         = EVT_DISABLE_SOCKS_ASYNC_CALLBACK;
    socksEvent.errorCode     = error;

    le_event_Report(tafSocks.socksEvId, &socksEvent,sizeof(taf_SocksEventType_t));
}

#ifdef TARGET_SA515M
/*======================================================================

 FUNCTION        taf_Socks::onInitComplete

 DESCRIPTION     Call back function of socksManager.

 DEPENDENCIES    The initialization of Socks.

 PARAMETERS      [IN] telux::common::ServiceStatus status : Socks manager service status.

 RETURN VALUE    None

======================================================================*/
void taf_Socks::onInitComplete(telux::common::ServiceStatus status)
{
    std::lock_guard<std::mutex> lock(mMutex);
    IsSubSystemStatusUpdated = true;
    conVar.notify_all();
}
#endif

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
    std::chrono::seconds span(ENABLE_SOCKS_TIMEOUT);
    TAF_ERROR_IF_RET_VAL(socksManager == NULL, LE_NOT_FOUND, "socksManager is null");

    EnableSocksSyncPromise = std::promise<le_result_t>();
    Status status = socksManager->enableSocks(true, tafSocksCallback::enableSocksResponse);

    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = EnableSocksSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Enable SOCKS timeout for %d seconds", ENABLE_SOCKS_TIMEOUT);
            result = LE_TIMEOUT;
        }
        else
        {
            result = futureResult.get();
        }

        return result;
    }
    else
    {
        LE_ERROR( "ERROR - Failed to enable SOCKS, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }

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
    std::chrono::seconds span(ENABLE_SOCKS_TIMEOUT);
    TAF_ERROR_IF_RET_VAL(socksManager == NULL, LE_NOT_FOUND, "socksManager is null");

    DisableSocksSyncPromise = std::promise<le_result_t>();
    Status status = socksManager->enableSocks(false, tafSocksCallback::disableSocksResponse);

    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = DisableSocksSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Disable SOCKS timeout for %d seconds", ENABLE_SOCKS_TIMEOUT);
            result = LE_TIMEOUT;
        }
        else
        {
            result = futureResult.get();
        }

        return result;
    }
    else
    {
        LE_ERROR( "ERROR - Failed to disable SOCKS, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }

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
    Status status = Status::SUCCESS;

    TAF_ERROR_IF_RET_VAL(socksManager == NULL, LE_NOT_FOUND, "socksManager is null");

    switch(type)
    {
        case ASYNC_ENABLE_SOCKS:

            status = socksManager->enableSocks(true, tafSocksCallback::enableSocksAsyncResponse);
        break;
        case ASYNC_DISABLE_SOCKS:

            status = socksManager->enableSocks(false, tafSocksCallback::disableSocksAsyncResponse);
        break;
        default:
            return LE_FAULT;
    }

    if (status == Status::SUCCESS)
    {
        return LE_OK;
    }
    else
    {
        LE_ERROR( "ERROR - Failed to enable/disable socks, Status:%d ", static_cast<int>(status));
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
