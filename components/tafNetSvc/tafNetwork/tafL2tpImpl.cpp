/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <vector>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "tafL2tpImpl.hpp"
#include "tafSvcIF.hpp"

#define REQUEST_L2TP_CONF_TIMEOUT 30
#define ENABLE_L2TP_TIMEOUT 30
#define START_TUNNEL_TIMEOUT 30
#define ENABLE_L2TP_MAX_NUMBER_AT_THE_SAME_TIME 1

using namespace tafsvc;

//TUNNEL definition

LE_MEM_DEFINE_STATIC_POOL(tunnelPool, TAF_NET_L2TP_MAX_TUNNEL_NUMBER, sizeof(taf_Tunnel_t));

LE_REF_DEFINE_STATIC_MAP(tunnelRefMap, TAF_NET_L2TP_MAX_TUNNEL_NUMBER);

LE_MEM_DEFINE_STATIC_POOL(l2tpSessionPool,
                          TAF_NET_L2TP_MAX_SESSION_NUMBER_PER_TUNNEL*TAF_NET_L2TP_MAX_TUNNEL_NUMBER,
                          sizeof(taf_L2tpSession_t));

//tunnel entry list definition
LE_MEM_DEFINE_STATIC_POOL(tunnelEntryListPool, TAF_NET_L2TP_MAX_TUNNEL_NUMBER,
                          sizeof(taf_TunnelEntryList_t));

LE_MEM_DEFINE_STATIC_POOL(tunnelEntryPool, TAF_NET_L2TP_MAX_TUNNEL_NUMBER,
                          sizeof(taf_TunnelEntry_t));

LE_MEM_DEFINE_STATIC_POOL(tunnelEntrySafeRefPool, TAF_NET_L2TP_MAX_TUNNEL_NUMBER,
                          sizeof(taf_TunnelEntrySafeRef_t));

LE_REF_DEFINE_STATIC_MAP(tunnelEntryListRefMap, TAF_NET_L2TP_MAX_TUNNEL_NUMBER);

LE_REF_DEFINE_STATIC_MAP(tunnelEntrySafeRefMap, TAF_NET_L2TP_MAX_TUNNEL_NUMBER);

LE_MEM_DEFINE_STATIC_POOL(L2tpHandlerMappingPool, TAF_NET_L2TP_MAX_TUNNEL_NUMBER,
                          sizeof(L2tpHandlerMapping_t));

LE_MEM_DEFINE_STATIC_POOL(TunnelHandlerMappingPool, TAF_NET_L2TP_MAX_TUNNEL_NUMBER,
                          sizeof(TunnelHandlerMapping_t));

std::vector<telux::data::net::L2tpTunnelConfig> tafL2tpCallback::configList;
taf_L2tpConfig_t tafL2tpCallback::l2tpConfig;

le_sem_Ref_t tafL2tpCallback::semaphore = nullptr;

le_event_Id_t taf_L2tp::l2tpEventId = nullptr;

le_event_Id_t taf_L2tp::l2tpCmdId = nullptr;

/*======================================================================

 FUNCTION        taf_L2tp::Init

 DESCRIPTION     Initialization of the taf L2tp component

 DEPENDENCIES    The initialization of telaf.

 PARAMETERS      None

 RETURN VALUE    None

======================================================================*/
void taf_L2tp::Init(void)
{

    bool isReady = false;

    // 1. Initiate the semaphore
    tafL2tpCallback::semaphore = le_sem_Create("taf_L2tpRespCbSem", 0);

    // 2. Initiate the memory pool

    tunnelPool = le_mem_InitStaticPool(tunnelPool,
                           TAF_NET_L2TP_MAX_TUNNEL_NUMBER, sizeof(taf_Tunnel_t));

    l2tpSessionPool = le_mem_InitStaticPool(l2tpSessionPool,
                      TAF_NET_L2TP_MAX_SESSION_NUMBER_PER_TUNNEL*TAF_NET_L2TP_MAX_TUNNEL_NUMBER,
                           sizeof(taf_L2tpSession_t));

    tunnelEntryListPool = le_mem_InitStaticPool(tunnelEntryListPool,
                               TAF_NET_L2TP_MAX_TUNNEL_NUMBER, sizeof(taf_TunnelEntryList_t));

    tunnelEntryPool = le_mem_InitStaticPool(tunnelEntryPool,
                           TAF_NET_L2TP_MAX_TUNNEL_NUMBER, sizeof(taf_TunnelEntry_t));

    tunnelEntrySafeRefPool = le_mem_InitStaticPool(tunnelEntrySafeRefPool,
                                  TAF_NET_L2TP_MAX_TUNNEL_NUMBER, sizeof(taf_TunnelEntrySafeRef_t));

    // 3. Initiate the reference map.

    tunnelRefMap = le_ref_InitStaticMap(tunnelRefMap, TAF_NET_L2TP_MAX_TUNNEL_NUMBER);

    tunnelEntryListRefMap = le_ref_InitStaticMap(tunnelEntryListRefMap,
                                                 TAF_NET_L2TP_MAX_TUNNEL_NUMBER);

    tunnelEntrySafeRefMap = le_ref_InitStaticMap(tunnelEntrySafeRefMap,
                                                 TAF_NET_L2TP_MAX_TUNNEL_NUMBER);

    L2tpHandlerMappingPool = le_mem_InitStaticPool(L2tpHandlerMappingPool,
                                                   TAF_NET_L2TP_MAX_TUNNEL_NUMBER,
                                                   sizeof(L2tpHandlerMapping_t));
    TunnelHandlerMappingPool = le_mem_InitStaticPool(TunnelHandlerMappingPool,
                                                     TAF_NET_L2TP_MAX_TUNNEL_NUMBER,
                                               sizeof(TunnelHandlerMapping_t));

    // 4. Get the DataFactory and l2tpManager instances
    if (l2tpManager == nullptr)
    {
        auto &dataFactory = telux::data::DataFactory::getInstance();
//SA415 using old telsdk,without initCb parameter
#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
        auto initCb = std::bind(&taf_L2tp::onInitComplete, this, std::placeholders::_1);
        l2tpManager = dataFactory.getL2tpManager(initCb);
#else
        l2tpManager = dataFactory.getL2tpManager();
#endif
    }

    if(l2tpManager == nullptr )
    {
        LE_INFO("L2tp manager initialize error...");
        return ;
    }

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    // 5. Check subsystem status
    std::unique_lock<std::mutex> lck(mMutex);

    telux::common::ServiceStatus subSystemStatus = l2tpManager->getServiceStatus();

    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE)
    {
        LE_INFO("L2tp manager initialize...");
        conVar.wait(lck, [this]{return this->IsSubSystemStatusUpdated;});
        subSystemStatus = l2tpManager->getServiceStatus();
    }

    //At this point, initialization should be either AVAILABLE or Failure
    if (subSystemStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
    {
        LE_ERROR("L2tp Manager initialization failed");
        l2tpManager = nullptr;
        return ;
    }
#endif

    isReady = l2tpManager->isSubsystemReady();

    if(isReady == false)
    {
        LE_INFO("L2tp component is not ready, wait for it unconditionally...");
        std::future<bool> readyFunc = l2tpManager->onSubsystemReady();
        isReady = readyFunc.get();
    }

    if(isReady)
    {
        LE_INFO("l2tpManager component is ready...");
    }
    else
    {
        LE_CRIT("unable to init l2tpManager component!");
    }

    // Create and start l2tp event thread.
    le_sem_Ref_t l2tpEvtThreadSem = le_sem_Create("l2tpEvtThreadSem", 0);
    l2tpEventId = le_event_CreateId("l2tpEvent", sizeof(taf_L2tpEventReq_t));
    le_thread_Ref_t l2tpEvtThreadRef = le_thread_Create("l2tpEvtThread", L2tpEventThread,
                                                         (void*)l2tpEvtThreadSem);
    le_thread_Start(l2tpEvtThreadRef);

    le_sem_Wait(l2tpEvtThreadSem);

    // Delete semaphore.
    le_sem_Delete(l2tpEvtThreadSem);

    // Create and start l2tp command thread.
    le_sem_Ref_t l2tpCmdThreadSem = le_sem_Create("l2tpCmdThreadSem", 0);
    l2tpCmdId = le_event_CreateId("l2tpCmd", sizeof(taf_L2tpCmdReq_t));
    le_thread_Ref_t l2tpCmdThreadRef = le_thread_Create("l2tpCmdThread", L2tpCmdThread,
                                                         (void*)l2tpCmdThreadSem);
    le_thread_Start(l2tpCmdThreadRef);

    le_sem_Wait(l2tpCmdThreadSem);

    // Delete semaphore.
    le_sem_Delete(l2tpCmdThreadSem);

    // Add a handler for client session close
    le_msg_AddServiceCloseHandler( taf_net_GetServiceRef(), ClientCloseSessionHandler, NULL );

    return;
}

/*======================================================================

 FUNCTION        taf_L2tp::GetInstance

 DESCRIPTION     Get the instance of L2tp.

 DEPENDENCIES    The initialization of L2tp.

 PARAMETERS      None

 RETURN VALUE    taf_L2tp &

======================================================================*/
taf_L2tp &taf_L2tp::GetInstance()
{
    static taf_L2tp instance;
    return instance;
}

/*======================================================================

 FUNCTION        taf_L2tp::SetTunnelStatus

 DESCRIPTION     Set tunnel status.

 DEPENDENCIES    The initialization of tunnel.

 PARAMETERS      taf_net_TunnelRef_t tunnelRef:        Tunnel reference.
                 bool isStarted:                       Is started or not.

 RETURN VALUE    true:             Tunnel is started
                 false:            Tunnel is not started

======================================================================*/
void taf_L2tp::SetTunnelStatus(taf_net_TunnelRef_t tunnelRef, bool isStarted)
{
    TAF_ERROR_IF_RET_NIL(tunnelRef == NULL, "tunnelRef is null");
    taf_Tunnel_t *tunnelPtr = NULL;
    tunnelPtr = (taf_Tunnel_t*)le_ref_Lookup(tunnelRefMap, tunnelRef);

    TAF_ERROR_IF_RET_NIL(tunnelPtr == NULL, "tunnel is not present");

    tunnelPtr->isStarted = isStarted;
}

/*======================================================================
 FUNCTION        taf_L2tp::L2tpProcEvtHandler
 DESCRIPTION     Asynchrous callback event handler.

 DEPENDENCIES    The initialization of l2tp event thread.

 PARAMETERS      [IN] void* cmdReqPtr: Command request pointer.

 RETURN VALUE    None

======================================================================*/
void taf_L2tp::L2tpProcEvtHandler(void* cmdReqPtr)
{
    le_result_t result = LE_OK;
    taf_L2tpEventReq_t* cmdReq = (taf_L2tpEventReq_t*)cmdReqPtr;
    L2tpHandlerMapping_t *l2tpHandlerMappingPtr = NULL;
    TunnelHandlerMapping_t *tunnelHandlerMappingPtr = NULL;
    TAF_ERROR_IF_RET_NIL(cmdReqPtr == NULL, "Input parameter is NULL");

    auto &tafL2tp = taf_L2tp::GetInstance();

    LE_DEBUG(" Received event = %d",cmdReq->event);

    switch(cmdReq->event)
    {
        case EVT_ENABLE_L2TP_ASYNC_CALLBACK:
            if (cmdReq->errorCode != telux::common::ErrorCode::SUCCESS)
            {
                LE_ERROR( "EVT_ENABLE_L2TP_ASYNC_CALLBACK failed with errorCode: %d ",
                           static_cast<int>(cmdReq->errorCode));
                result = LE_FAULT;
            }
            else
            {
                LE_DEBUG("EVT_ENABLE_L2TP_ASYNC_CALLBACK processed successfully \n");
                result = LE_OK;
            }

            //Find the handler function and then call it
            l2tpHandlerMappingPtr = tafL2tp.FindL2tpAsyncHandler(ASYNC_ENABLE_L2TP);

            if(l2tpHandlerMappingPtr != NULL && l2tpHandlerMappingPtr->asyncHandler != NULL)
            {
                //Call handler function, and then remove it from mapping list
                l2tpHandlerMappingPtr->asyncHandler(result,l2tpHandlerMappingPtr->contextPtr);
                tafL2tp.DeleteL2tpHandlerInfo(l2tpHandlerMappingPtr->asyncHandler);
            }

        break;
        case EVT_DISABLE_L2TP_ASYNC_CALLBACK:
            if (cmdReq->errorCode != telux::common::ErrorCode::SUCCESS)
            {
                LE_ERROR( "EVT_DISABLE_L2TP_ASYNC_CALLBACK failed with errorCode: %d ",
                           static_cast<int>(cmdReq->errorCode));
                result = LE_FAULT;
            }
            else
            {
                LE_DEBUG("EVT_DISABLE_L2TP_ASYNC_CALLBACK processed successfully \n");
                result = LE_OK;
            }

            //Find the handler function and then call it
            l2tpHandlerMappingPtr = tafL2tp.FindL2tpAsyncHandler(ASYNC_DISABLE_L2TP);

            if(l2tpHandlerMappingPtr != NULL && l2tpHandlerMappingPtr->asyncHandler != NULL)
            {
                //Call handler function, and then remove it from mapping list
                l2tpHandlerMappingPtr->asyncHandler(result,l2tpHandlerMappingPtr->contextPtr);
                tafL2tp.DeleteL2tpHandlerInfo(l2tpHandlerMappingPtr->asyncHandler);
            }
        case EVT_START_TUNNEL_ASYNC_CALLBACK:
            if (cmdReq->errorCode != telux::common::ErrorCode::SUCCESS)
            {
                LE_ERROR( "EVT_START_TUNNEL_ASYNC_CALLBACK failed with errorCode: %d ",
                           static_cast<int>(cmdReq->errorCode));
                result = LE_FAULT;
            }
            else
            {
                LE_DEBUG("EVT_START_TUNNEL_ASYNC_CALLBACK processed successfully \n");
                result = LE_OK;
            }

            //Find the handler function and then call it
            tunnelHandlerMappingPtr = tafL2tp.FindTunnelAsyncHandler(ASYNC_START_TUNNEL);

            if(tunnelHandlerMappingPtr != NULL && tunnelHandlerMappingPtr->asyncHandler != NULL)
            {
                //Set tunnel status.
                tafL2tp.SetTunnelStatus( tunnelHandlerMappingPtr->tunnelRef, true);
                //Call handler function, and then remove it from mapping list
                tunnelHandlerMappingPtr->asyncHandler(tunnelHandlerMappingPtr->tunnelRef, result,
                                                      tunnelHandlerMappingPtr->contextPtr);
                tafL2tp.DeleteTunnelHandlerInfo(tunnelHandlerMappingPtr->asyncHandler);
            }

        break;

        case EVT_STOP_TUNNEL_ASYNC_CALLBACK:
            if (cmdReq->errorCode != telux::common::ErrorCode::SUCCESS)
            {
                LE_ERROR( "EVT_STOP_TUNNEL_ASYNC_CALLBACK failed with errorCode: %d ",
                           static_cast<int>(cmdReq->errorCode));
                result = LE_FAULT;
            }
            else
            {
                LE_DEBUG("EVT_STOP_TUNNEL_ASYNC_CALLBACK processed successfully \n");
                result = LE_OK;
            }

            //Find the handler function and then call it
            tunnelHandlerMappingPtr = tafL2tp.FindTunnelAsyncHandler(ASYNC_STOP_TUNNEL);

            if(tunnelHandlerMappingPtr != NULL && tunnelHandlerMappingPtr->asyncHandler != NULL)
            {
                //Set tunnel status.
                tafL2tp.SetTunnelStatus( tunnelHandlerMappingPtr->tunnelRef, false);
                //Call handler function, and then remove it from mapping list
                tunnelHandlerMappingPtr->asyncHandler(tunnelHandlerMappingPtr->tunnelRef, result,
                                                      tunnelHandlerMappingPtr->contextPtr);
                tafL2tp.DeleteTunnelHandlerInfo(tunnelHandlerMappingPtr->asyncHandler);
            }

        break;
        default:
                LE_ERROR("Command error");
        break;
    }
}

/*======================================================================

 FUNCTION        taf_L2tp::L2tpEventThread

 DESCRIPTION     L2tp event thread for handling asynchronous request.

 DEPENDENCIES    The initialization of L2tp.

 PARAMETERS      [IN] void* contextPtr: Context pointer.

 RETURN VALUE    void*
                     NULL: Success.

======================================================================*/
void* taf_L2tp::L2tpEventThread(void* contextPtr)
{
    le_event_AddHandler("L2tpEventThread", l2tpEventId, L2tpProcEvtHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return nullptr;
}

/*======================================================================
 FUNCTION        taf_L2tp::L2tpProcAsyncCmdHandler
 DESCRIPTION     Asynchrous l2tp command handler.

 DEPENDENCIES    The initialization of l2tp command thread.

 PARAMETERS      [IN] void* cmdReqPtr: Command request pointer.

 RETURN VALUE    None

======================================================================*/
void taf_L2tp::L2tpProcCmdHandler(void* cmdReqPtr)
{
    le_result_t result = LE_OK;
    taf_L2tpCmdReq_t* cmdReq = (taf_L2tpCmdReq_t*)cmdReqPtr;
    auto &tafL2tp = taf_L2tp::GetInstance();

    TAF_ERROR_IF_RET_NIL(cmdReqPtr == NULL, "Input parameter is NULL");

    LE_DEBUG(" Received cmd = %d",cmdReq->cmdType);

    switch(cmdReq->cmdType)
    {
        case ASYNC_ENABLE_L2TP:
            //Only one client can enable L2TP at the same time.
            if( tafL2tp.GetL2tpHandlerNumberInMappingList(ASYNC_ENABLE_L2TP) >=
                ENABLE_L2TP_MAX_NUMBER_AT_THE_SAME_TIME )
            {
                LE_DEBUG("Only one client can enable L2TP at the same time");
                return;
            }

            //Add handler function into mapping list, will be used later
            if(cmdReq->l2tpHandlerFuncPtr != NULL)
                tafL2tp.AddL2tpHandlerSessionMapping(cmdReq->l2tpHandlerFuncPtr, cmdReq->contextPtr,
                                                     cmdReq->sessionRef, ASYNC_ENABLE_L2TP);

            //Call telsdk API
            result = tafL2tp.EnableL2tp(cmdReq->enableParam.enableMss,
                                        cmdReq->enableParam.enableMtu, cmdReq->enableParam.mtuSize,
                                        ASYNC_ENABLE_L2TP);

            if (result != LE_OK)
            {
                LE_ERROR("Enable L2TP asynchronously error %d", result);
                if(cmdReq->l2tpHandlerFuncPtr != NULL)
                    tafL2tp.DeleteL2tpHandlerInfo(cmdReq->l2tpHandlerFuncPtr);
            }

        break;
        case ASYNC_DISABLE_L2TP:
            //check if another client session enabled the tunnel
            if(tafL2tp.IsTunnelStartedByOtherClient(cmdReq->sessionRef))
            {
                return;
            }
            //Only one client can disable L2TP at the same time.
            if( tafL2tp.GetL2tpHandlerNumberInMappingList(ASYNC_DISABLE_L2TP) >=
                ENABLE_L2TP_MAX_NUMBER_AT_THE_SAME_TIME )
            {
                LE_DEBUG("Only one client can disable L2TP at the same time");
                return;
            }

            //Add handler function into mapping list, will be used later
            if(cmdReq->l2tpHandlerFuncPtr != NULL)
                tafL2tp.AddL2tpHandlerSessionMapping(cmdReq->l2tpHandlerFuncPtr, cmdReq->contextPtr,
                                                  cmdReq->sessionRef, ASYNC_DISABLE_L2TP);

            //Call telsdk API
            result = tafL2tp.EnableL2tp(0, 0, 0, ASYNC_DISABLE_L2TP);

            if (result != LE_OK)
            {
                LE_ERROR("Disable L2TP asynchronously error %d", result);
                if(cmdReq->l2tpHandlerFuncPtr != NULL)
                    tafL2tp.DeleteL2tpHandlerInfo(cmdReq->l2tpHandlerFuncPtr);
            }

        break;
        case ASYNC_START_TUNNEL:
            //Add handler function into mapping list, will be used later
            if(cmdReq->tunnelHandlerFuncPtr != NULL)
                tafL2tp.AddTunnelHandlerSessionMapping(cmdReq->tunnelHandlerFuncPtr,
                                                       cmdReq->tunnelRef, cmdReq->contextPtr,
                                                       cmdReq->sessionRef, ASYNC_START_TUNNEL);

            //Call telsdk API
            result = tafL2tp.AddTunnelAsync(cmdReq->tunnelRef);

            if (result != LE_OK)
            {
                LE_ERROR("Start tunnel asynchronously error %d", result);
                if(cmdReq->tunnelHandlerFuncPtr != NULL)
                    tafL2tp.DeleteTunnelHandlerInfo(cmdReq->tunnelHandlerFuncPtr);
            }

        break;
        case ASYNC_STOP_TUNNEL:
            //Add handler function into mapping list, will be used later
            if(cmdReq->tunnelHandlerFuncPtr != NULL)
                tafL2tp.AddTunnelHandlerSessionMapping(cmdReq->tunnelHandlerFuncPtr,
                                                       cmdReq->tunnelRef, cmdReq->contextPtr,
                                                       cmdReq->sessionRef, ASYNC_STOP_TUNNEL);

            //Call telsdk API
            result = tafL2tp.RemoveTunnelAsync(cmdReq->tunnelRef);

            if (result != LE_OK)
            {
                LE_ERROR("Stop tunnel asynchronously error %d", result);
                if(cmdReq->tunnelHandlerFuncPtr != NULL)
                    tafL2tp.DeleteTunnelHandlerInfo(cmdReq->tunnelHandlerFuncPtr);
            }

        break;

        default:
                LE_ERROR("Command error");
        break;
    }
}

/*======================================================================

 FUNCTION        taf_L2tp::L2tpCmdThread

 DESCRIPTION     L2tp command thread for handling asynchronous request.

 DEPENDENCIES    The initialization of L2tp.

 PARAMETERS      [IN] void* contextPtr: Context pointer.

 RETURN VALUE    void*
                     NULL: Success.

======================================================================*/
void* taf_L2tp::L2tpCmdThread(void* contextPtr)
{
    le_event_AddHandler("L2tpProcCmdHandler", l2tpCmdId, L2tpProcCmdHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return nullptr;
}

/*======================================================================

 FUNCTION        tafL2tpCallback::enableL2tpResponse

 DESCRIPTION     Call back function for setting config.

 DEPENDENCIES    The initialization of L2tp.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafL2tpCallback::enableL2tpResponse(telux::common::ErrorCode error)
{
    le_result_t result = LE_OK;
    auto &tafL2tp = taf_L2tp::GetInstance();

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR( "Request failed with errorCode: %d " , static_cast<int>(error));
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("Request processed successfully \n");
    }

    tafL2tp.L2tpEnableSyncPromise.set_value(result);
}

/*======================================================================

 FUNCTION        tafL2tpCallback::disableL2tpResponse

 DESCRIPTION     Call back function for setting config.

 DEPENDENCIES    The initialization of L2tp.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafL2tpCallback::disableL2tpResponse(telux::common::ErrorCode error)
{
    le_result_t result = LE_OK;
    auto &tafL2tp = taf_L2tp::GetInstance();

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR( "Request failed with errorCode: %d " , static_cast<int>(error));
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("Request processed successfully \n");
    }

    tafL2tp.L2tpDisableSyncPromise.set_value(result);
}

/*======================================================================

 FUNCTION        tafL2tpCallback::enableL2tpAsyncResponse

 DESCRIPTION     Call back function for setting config.

 DEPENDENCIES    The initialization of L2tp.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafL2tpCallback::enableL2tpAsyncResponse(telux::common::ErrorCode error)
{
    auto &tafL2tp = taf_L2tp::GetInstance();

    taf_L2tpEventReq_t l2tpEvent;

    l2tpEvent.event         = EVT_ENABLE_L2TP_ASYNC_CALLBACK;
    l2tpEvent.errorCode     = error;

    le_event_Report(tafL2tp.l2tpEventId, &l2tpEvent,sizeof(taf_L2tpEventReq_t));
}

/*======================================================================

 FUNCTION        tafL2tpCallback::disableL2tpAsyncResponse

 DESCRIPTION     Call back function for setting config.

 DEPENDENCIES    The initialization of L2tp.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafL2tpCallback::disableL2tpAsyncResponse(telux::common::ErrorCode error)
{

    auto &tafL2tp = taf_L2tp::GetInstance();

    taf_L2tpEventReq_t l2tpEvent;

    l2tpEvent.event         = EVT_DISABLE_L2TP_ASYNC_CALLBACK;
    l2tpEvent.errorCode     = error;

    le_event_Report(tafL2tp.l2tpEventId, &l2tpEvent,sizeof(taf_L2tpEventReq_t));
}

/*======================================================================

 FUNCTION        tafL2tpCallback::startTunnelSyncResponse

 DESCRIPTION     Call back function for synchronous starting tunnel.

 DEPENDENCIES    The initialization of L2tp.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafL2tpCallback::startTunnelSyncResponse(telux::common::ErrorCode error)
{
    le_result_t result = LE_OK;
    auto &tafL2tp = taf_L2tp::GetInstance();

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR( "Request failed with errorCode: %d " , static_cast<int>(error));
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("Request processed successfully \n");
    }

    tafL2tp.L2tpStartTunnelSyncPromise.set_value(result);
}

/*======================================================================

 FUNCTION        tafL2tpCallback::stopTunnelSyncResponse

 DESCRIPTION     Call back function for synchronous stopping tunnel.

 DEPENDENCIES    The initialization of L2tp.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafL2tpCallback::stopTunnelSyncResponse(telux::common::ErrorCode error)
{
    le_result_t result = LE_OK;
    auto &tafL2tp = taf_L2tp::GetInstance();

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR( "Request failed with errorCode: %d " , static_cast<int>(error));
        result = LE_FAULT;
    }
    else
    {
        LE_DEBUG("Request processed successfully \n");
    }

    tafL2tp.L2tpStopTunnelSyncPromise.set_value(result);
}

/*======================================================================

 FUNCTION        tafL2tpCallback::startTunnelAsyncResponse

 DESCRIPTION     Call back function for asynchronous starting tunnel.

 DEPENDENCIES    The initialization of L2tp.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafL2tpCallback::startTunnelAsyncResponse(telux::common::ErrorCode error)
{

    auto &tafL2tp = taf_L2tp::GetInstance();

    taf_L2tpEventReq_t l2tpEvent;

    l2tpEvent.event         = EVT_START_TUNNEL_ASYNC_CALLBACK;
    l2tpEvent.errorCode     = error;

    le_event_Report(tafL2tp.l2tpEventId, &l2tpEvent,sizeof(taf_L2tpEventReq_t));
}

/*======================================================================

 FUNCTION        tafL2tpCallback::stopTunnelAsyncResponse

 DESCRIPTION     Call back function for asynchronous stopping tunnel.

 DEPENDENCIES    The initialization of L2tp.

 PARAMETERS      [IN] telux::common::ErrorCode error: The error code.

 RETURN VALUE    None.

======================================================================*/
void tafL2tpCallback::stopTunnelAsyncResponse(telux::common::ErrorCode error)
{

    auto &tafL2tp = taf_L2tp::GetInstance();

    taf_L2tpEventReq_t l2tpEvent;

    l2tpEvent.event         = EVT_STOP_TUNNEL_ASYNC_CALLBACK;
    l2tpEvent.errorCode     = error;

    le_event_Report(tafL2tp.l2tpEventId, &l2tpEvent,sizeof(taf_L2tpEventReq_t));
}

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
/*======================================================================

 FUNCTION        taf_L2tp::onInitComplete

 DESCRIPTION     Call back function of l2tpManager.

 DEPENDENCIES    The initialization of L2tp.

 PARAMETERS      [IN] telux::common::ServiceStatus status : L2tp manager service status.

 RETURN VALUE    None

======================================================================*/
void taf_L2tp::onInitComplete(telux::common::ServiceStatus status)
{
    std::lock_guard<std::mutex> lock(mMutex);
    IsSubSystemStatusUpdated = true;
    conVar.notify_all();
}
#endif

/*======================================================================

 FUNCTION        taf_Tunnel::GetTunnelRefById

 DESCRIPTION     Get tunnel reference by tunnel id and client session reference.

 DEPENDENCIES    The initialization of l2tp.

 PARAMETERS      [IN] uint32_t locTunnelId : Local tunnel id.
                 [IN] le_msg_SessionRef_t sessionRef : Client session reference.

 RETURN VALUE    taf_net_TunnelRef_t
                     NULL:          Not found
                     Others:        The reference of a created tunnel

======================================================================*/
taf_net_TunnelRef_t taf_L2tp::GetTunnelRefById
(
    uint32_t locTunnelId,
    le_msg_SessionRef_t sessionRef
)
{
    taf_Tunnel_t* tunnelPtr = NULL;

    le_ref_IterRef_t iterRef = le_ref_GetIterator(tunnelRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        tunnelPtr = (taf_Tunnel_t*)le_ref_GetValue(iterRef);

        if (tunnelPtr != NULL && tunnelPtr->locTunnelId == locTunnelId)
        {
            if (tunnelPtr->sessionRef == sessionRef)
            {
                LE_DEBUG("found tunnel for tunnel id %d", locTunnelId);
                return (taf_net_TunnelRef_t)le_ref_GetSafeRef(iterRef);
            }
            else
            {
                LE_DEBUG("session mismatch for local tunnel id %d", locTunnelId);
                return NULL;
            }
        }
    }

    //Create a tunnel reference if the tunnel is present in telsdk
    return CreateTunnelIfExistsInDb(locTunnelId, sessionRef);
}

/*======================================================================

 FUNCTION        taf_L2tp::CreateTunnelIfExistsInDb

 DESCRIPTION     Create a tunnel if the tunnel is present in telsdk.

 DEPENDENCIES    The initialization of tunnel.

 PARAMETERS      None.

 RETURN VALUE    taf_net_TunnelRef_t
                     NULL:          Not found
                     Others:        The reference of a created tunnel

======================================================================*/
taf_net_TunnelRef_t taf_L2tp::CreateTunnelIfExistsInDb
(
    uint32_t tunnelId,
    le_msg_SessionRef_t sessionRef
)
{
    taf_Tunnel_t* tunnelPtr = NULL;
    taf_L2tpSession_t* l2tpSessionInfo = NULL;

    TAF_ERROR_IF_RET_VAL(l2tpManager == NULL, NULL, "l2tpManager is null");

    telux::common::Status status = l2tpManager->requestConfig(
                                                tafL2tpCallback::requestConfigResponse);

    if (status == telux::common::Status::SUCCESS)
    {
        le_clk_Time_t timeToWait = {REQUEST_L2TP_CONF_TIMEOUT, 0};
        le_result_t res = le_sem_WaitWithTimeOut(tafL2tpCallback::semaphore, timeToWait);
        TAF_ERROR_IF_RET_VAL(res != LE_OK, NULL, "Wait semaphore timeout\n");

        if(tafL2tpCallback::l2tpConfig.enableL2tp == false ||
           tafL2tpCallback::configList.size() == 0)
        {
            LE_DEBUG("No tunnel entry");
            return NULL;
        }

        for (auto info : tafL2tpCallback::configList)
        {
            //found tunnel info
            if(tunnelId == info.locId)
            {
                //Create a new tunnel reference
                tunnelPtr = (taf_Tunnel_t*)le_mem_ForceAlloc(tunnelPool);
                tunnelPtr->locTunnelId=info.locId;

                tunnelPtr->peerTunnelId = info.peerId;
                tunnelPtr->localUdpPort = info.localUdpPort;
                tunnelPtr->peerUdpPort = info.peerUdpPort;
                tunnelPtr->encaProto = (taf_net_L2tpEncapProtocol_t)info.prot;
                if(info.ipType == telux::data::IpFamilyType::IPV4)
                    le_utf8_Copy(tunnelPtr->peerIpAddr, info.peerIpv4Addr.c_str(),
                                 TAF_NET_IP_ADDR_MAX_LEN, NULL);
                else if(info.ipType == telux::data::IpFamilyType::IPV6)
                    le_utf8_Copy(tunnelPtr->peerIpAddr, info.peerIpv6Addr.c_str(),
                                 TAF_NET_IP_ADDR_MAX_LEN, NULL);

                le_utf8_Copy(tunnelPtr->interfaceName, info.locIface.c_str(),
                             TAF_NET_INTERFACE_NAME_MAX_LEN, NULL);

                for (auto session : info.sessionConfig)
                {
                    l2tpSessionInfo = (taf_L2tpSession_t *)le_mem_ForceAlloc(l2tpSessionPool);

                    l2tpSessionInfo->locSessionId = session.locId;
                    l2tpSessionInfo->peerSessionId = session.peerId;

                    // add this profile context to list
                    le_dls_Queue(&(tunnelPtr->l2tpSessionList), &(l2tpSessionInfo->link));
                }

                tunnelPtr->sessionRef = sessionRef;
                tunnelPtr->isStarted = true;

                //Save the tunnel reference.
                return (taf_net_TunnelRef_t)le_ref_CreateRef(tunnelRefMap, tunnelPtr);
            }
        }
    }
    else
    {
        LE_ERROR("Request tunnel info failed, status: %d",int(status));
        return NULL;
    }

    return NULL;
}

/*======================================================================

 FUNCTION        taf_L2tp::CleanListRef

 DESCRIPTION     Clean the tunnel entry list stored in the map.

 DEPENDENCIES    The initialization of tunnel.

 PARAMETERS      [IN] taf_net_TunnelEntryListRef_t tunnelEntryListRef: Tunnel entry list reference.

 RETURN VALUE    le_result_t
                     LE_OK:                      Succeeded to clean the list
                     LE_NOT_FOUND                Not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to clean the list.

======================================================================*/
le_result_t taf_L2tp::CleanListRef
(
    taf_net_TunnelEntryListRef_t tunnelEntryListRef
)
{
    taf_TunnelEntry_t* tunnelEntryPtr;
    taf_TunnelEntrySafeRef_t* safeRefPtr;
    le_sls_Link_t *linkPtr;

    TAF_ERROR_IF_RET_VAL(tunnelEntryListRef == NULL, LE_BAD_PARAMETER,
                         "Null reference(tunnelEntryListRef)");

    taf_TunnelEntryList_t* listPtr = (taf_TunnelEntryList_t*)le_ref_Lookup(tunnelEntryListRefMap,
                                                                           tunnelEntryListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == NULL, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    while ((linkPtr = le_sls_Pop(&(listPtr->tunnelEntryList))) != NULL)
    {
        tunnelEntryPtr = CONTAINER_OF(linkPtr, taf_TunnelEntry_t, link);
        le_mem_Release(tunnelEntryPtr);
    }

    while ((linkPtr = le_sls_Pop(&(listPtr->safeRefList))) != NULL)
    {
        safeRefPtr = CONTAINER_OF(linkPtr, taf_TunnelEntrySafeRef_t, link);
        le_ref_DeleteRef(tunnelEntrySafeRefMap, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(tunnelEntryListRefMap, tunnelEntryListRef);

    le_mem_Release(listPtr);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_L2tp::IsTunnelStartedByOtherClient

 DESCRIPTION     Check if tunnel is started by other client.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelRef_t tunnelRef : Tunnel reference.
                 [IN] uint32_t locSessionId : Local session id.
                 [IN] uint32_t peerSessionId : Peer session id.

 RETURN VALUE    bool
                     true           Session id is valid.
                     false          Session id is invalid.

======================================================================*/
bool taf_L2tp::IsTunnelStartedByOtherClient(le_msg_SessionRef_t sessionRef)
{
    taf_Tunnel_t* tunnelPtr = NULL;
    auto &tafTunnel = taf_L2tp::GetInstance();

    TAF_ERROR_IF_RET_VAL(sessionRef == NULL, false, "sessionRef is invalid");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(tafTunnel.tunnelRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        tunnelPtr = (taf_Tunnel_t*)le_ref_GetValue(iterRef);

        //check if tunnel is started by other client
        if (tunnelPtr != NULL && tunnelPtr->isStarted && tunnelPtr->sessionRef != sessionRef)
            return true;
    }

    return false;
}

/*======================================================================

 FUNCTION        taf_L2tp::IsSessionIdValid

 DESCRIPTION     Check if session id is present and session number exceeds the max value.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelRef_t tunnelRef : Tunnel reference.
                 [IN] uint32_t locSessionId : Local session id.
                 [IN] uint32_t peerSessionId : Peer session id.

 RETURN VALUE    bool
                     true           Session id is valid.
                     false          Session id is invalid.

======================================================================*/
bool taf_L2tp::IsSessionIdValid
(
    taf_net_TunnelRef_t tunnelRef,
    uint32_t locSessionId,
    uint32_t peerSessionId
)
{
    le_dls_Link_t* linkPtr = NULL;
    taf_Tunnel_t *tunnelPtr = NULL;
    uint16_t sessionNum=0;

    TAF_ERROR_IF_RET_VAL(tunnelRef == NULL, false, "tunnelRef is null");
    tunnelPtr = (taf_Tunnel_t*)le_ref_Lookup(tunnelRefMap, tunnelRef);

    TAF_ERROR_IF_RET_VAL(tunnelPtr == NULL, false, "tunnel is not present");

    linkPtr = le_dls_Peek(&(tunnelPtr->l2tpSessionList));
    while (linkPtr)
    {
        taf_L2tpSession_t* sessionPtr = CONTAINER_OF(linkPtr, taf_L2tpSession_t, link);

        if(sessionPtr->locSessionId == locSessionId || sessionPtr->peerSessionId == peerSessionId )
        {
            LE_ERROR("Local session id or peer session id is present");
            return false;
        }

        linkPtr = le_dls_PeekNext(&(tunnelPtr->l2tpSessionList), linkPtr);

        sessionNum++;
    }

    if(sessionNum >= TAF_NET_L2TP_MAX_SESSION_NUMBER_PER_TUNNEL)
    {
        LE_ERROR("Session number exceeds the max value");
        return false;
    }

    return true;
}

/*======================================================================

 FUNCTION        taf_L2tp::EnableL2tpCmdSync

 DESCRIPTION     Enable l2tp.

 DEPENDENCIES    The initialization of l2tp.

 PARAMETERS      [IN] bool enableMss : Enable or disable MSS.
                 [IN] bool enableMtu : Enable or disable MTU.
                 [IN] uint32_t mtuSize : Mtu size, if value is 0, then use default size 1422.

 RETURN VALUE    le_result_t
                     LE_OK:                      Succeeded to enable l2tp
                     LE_NOT_FOUND                L2TP is not found.
                     LE_FAULT                    Failed to enable L2TP.

======================================================================*/
le_result_t taf_L2tp::EnableL2tpCmdSync(bool enableMss, bool enableMtu, uint32_t mtuSize)
{
    le_result_t result;

    std::chrono::seconds span(ENABLE_L2TP_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(l2tpManager == NULL, LE_NOT_FOUND, "l2tpManager is null");

    L2tpEnableSyncPromise = std::promise<le_result_t>();

    if(mtuSize == 0)
        mtuSize=DEFAULT_MTU_SIZE;

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    Status status = l2tpManager->setConfig(true, enableMss, enableMtu,
                                                   tafL2tpCallback::enableL2tpResponse, mtuSize);
#else
    Status status = l2tpManager->setConfig(true, enableMss, enableMtu,
                                                   tafL2tpCallback::enableL2tpResponse);
#endif

    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = L2tpEnableSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Enable l2tp timeout for %d seconds", ENABLE_L2TP_TIMEOUT);
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
        LE_ERROR( "ERROR - Failed to enable l2tp, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }

}

/*======================================================================

 FUNCTION        taf_L2tp::DisableL2tpCmdSync

 DESCRIPTION     Disable l2tp.

 DEPENDENCIES    The initialization of l2tp.

 PARAMETERS      [IN] le_msg_SessionRef_t sessionRef : Client session reference

 RETURN VALUE    le_result_t
                     LE_OK:                      Succeeded to disable l2tp
                     LE_NOT_FOUND                L2TP is not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to disable L2TP.

======================================================================*/
le_result_t taf_L2tp::DisableL2tpCmdSync(le_msg_SessionRef_t sessionRef)
{
    le_result_t result;

    std::chrono::seconds span(ENABLE_L2TP_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(l2tpManager == NULL, LE_NOT_FOUND, "l2tpManager is null");

    TAF_ERROR_IF_RET_VAL(sessionRef == NULL, LE_BAD_PARAMETER, "sessionRef is null");

    //check if another client session enabled the tunnel
    if(IsTunnelStartedByOtherClient(sessionRef))
    {
        LE_ERROR( "L2tp is enabled by other client");
        return LE_FAULT;
    }

    L2tpDisableSyncPromise = std::promise<le_result_t>();

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
    Status status = l2tpManager->setConfig(false, false, false,
                                                   tafL2tpCallback::disableL2tpResponse, 0);
#else
    Status status = l2tpManager->setConfig(false, false, false,
                                                   tafL2tpCallback::disableL2tpResponse);
#endif

    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = L2tpDisableSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Disable l2tp timeout for %d seconds", ENABLE_L2TP_TIMEOUT);
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
        LE_ERROR( "ERROR - Failed to disable l2tp, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }

}

/*======================================================================

 FUNCTION        taf_L2tp::EnableL2tpCmdAsync

 DESCRIPTION     Asynchronously enable l2tp.

 DEPENDENCIES    The initialization of l2tp.

 PARAMETERS      [IN] bool enableMss : Enable or disable MSS.
                 [IN] bool enableMtu : Enable or disable MTU.
                 [IN] uint32_t mtuSize : Mtu size, if value is 0, then use default size 1422.
                 [IN] taf_net_AsyncL2tpHandlerFunc_t handlerPtr : L2TP handler function.
                 [IN] void* contextPtr : Context pointer.
                 [IN] le_msg_SessionRef_t sessionRef : Session reference.

 RETURN VALUE    None

======================================================================*/
void taf_L2tp::EnableL2tpCmdAsync
(
    bool enableMss,
    bool enableMtu,
    uint32_t mtuSize,
    taf_net_AsyncL2tpHandlerFunc_t handlerPtr,
    void* contextPtr,
    le_msg_SessionRef_t sessionRef
)
{
    taf_L2tpCmdReq_t cmdReq;

    TAF_ERROR_IF_RET_NIL(handlerPtr == NULL, "Handler function is NULL");
    TAF_ERROR_IF_RET_NIL(sessionRef == NULL, "SessionRef is NULL");

    cmdReq.cmdType = ASYNC_ENABLE_L2TP;
    cmdReq.enableParam.enableMss = enableMss;
    cmdReq.enableParam.enableMtu = enableMtu;

    if(mtuSize == 0)
        cmdReq.enableParam.mtuSize = DEFAULT_MTU_SIZE;
    else
        cmdReq.enableParam.mtuSize = mtuSize;

    cmdReq.contextPtr = contextPtr;
    cmdReq.sessionRef = sessionRef;
    cmdReq.l2tpHandlerFuncPtr = handlerPtr;

    // Send ASYNC_ENABLE_L2TP command
    le_event_Report(taf_L2tp::l2tpCmdId, &cmdReq, sizeof(cmdReq));
}

/*======================================================================

 FUNCTION        taf_L2tp::DisableL2tpCmdAsync

 DESCRIPTION     Asynchronously disable l2tp.

 DEPENDENCIES    The initialization of l2tp.

 PARAMETERS      [IN] taf_net_AsyncL2tpHandlerFunc_t handlerPtr : L2TP handler function.
                 [IN] void* contextPtr : Context pointer.
                 [IN] le_msg_SessionRef_t sessionRef : Session reference.

 RETURN VALUE    None

======================================================================*/
void taf_L2tp::DisableL2tpCmdAsync
(
    taf_net_AsyncL2tpHandlerFunc_t handlerPtr,
    void* contextPtr,
    le_msg_SessionRef_t sessionRef
)
{
    taf_L2tpCmdReq_t cmdReq;

    TAF_ERROR_IF_RET_NIL(handlerPtr == NULL, "Handler function is NULL");
    TAF_ERROR_IF_RET_NIL(sessionRef == NULL, "SessionRef is NULL");

    cmdReq.cmdType = ASYNC_DISABLE_L2TP;
    cmdReq.enableParam.enableMss = 0;
    cmdReq.enableParam.enableMtu = 0;
    cmdReq.enableParam.mtuSize = 0;
    cmdReq.contextPtr = contextPtr;
    cmdReq.sessionRef = sessionRef;
    cmdReq.l2tpHandlerFuncPtr = handlerPtr;

    // Send ASYNC_DISABLE_L2TP command
    le_event_Report(taf_L2tp::l2tpCmdId, &cmdReq, sizeof(cmdReq));
}

/*======================================================================

 FUNCTION        taf_L2tp::EnableL2tp

 DESCRIPTION     Enable l2tp.

 DEPENDENCIES    The initialization of l2tp.

 PARAMETERS      [IN] bool enableMss : Enable or disable MSS.
                 [IN] bool enableMtu : Enable or disable MTU.
                 [IN] uint32_t mtuSize : Mtu size, if value is 0, then use default size 1422.
                 [IN] taf_L2tpCmdType_t type : The command type.

 RETURN VALUE    le_result_t
                     LE_OK:                      Succeeded to enable l2tp
                     LE_NOT_FOUND                L2TP is not found.
                     LE_FAULT                    Failed to enable L2TP.

======================================================================*/
le_result_t taf_L2tp::EnableL2tp
(
    bool enableMss,
    bool enableMtu,
    uint32_t mtuSize,
    taf_L2tpCmdType_t type
)
{
    Status status = Status::SUCCESS;

    TAF_ERROR_IF_RET_VAL(l2tpManager == NULL, LE_NOT_FOUND, "l2tpManager is null");

    switch(type)
    {
        case ASYNC_ENABLE_L2TP:

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
            status = l2tpManager->setConfig(true, enableMss, enableMtu,
                                                 tafL2tpCallback::enableL2tpAsyncResponse, mtuSize);
#else
            status = l2tpManager->setConfig(true, enableMss, enableMtu,
                                                   tafL2tpCallback::enableL2tpAsyncResponse);
#endif

        break;
        case ASYNC_DISABLE_L2TP:

#if defined(TARGET_SA515M) || defined(TARGET_SA525M)
            status = l2tpManager->setConfig(false, enableMss, enableMtu,
                                                tafL2tpCallback::disableL2tpAsyncResponse, mtuSize);
#else
            status = l2tpManager->setConfig(false, enableMss, enableMtu,
                                                   tafL2tpCallback::disableL2tpAsyncResponse);
#endif

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
        LE_ERROR( "ERROR - Failed to enable/disable L2TP, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }
}

/*======================================================================

 FUNCTION        taf_L2tp::IsL2tpEnabled

 DESCRIPTION     Is L2TP enabled or not.

 DEPENDENCIES    The initialization of l2tp.

 PARAMETERS      None.

 RETURN VALUE    bool
                     true:      L2tp is enabled
                     false      L2tp is disabled

======================================================================*/
bool taf_L2tp::IsL2tpEnabled
(
    void
)
{
    TAF_ERROR_IF_RET_VAL(l2tpManager == NULL, false, "l2tpManager is null");

    telux::common::Status status = l2tpManager->requestConfig(
                                                tafL2tpCallback::requestConfigResponse);

    if (status == telux::common::Status::SUCCESS)
    {
        le_clk_Time_t timeToWait = {REQUEST_L2TP_CONF_TIMEOUT, 0};
        le_result_t res = le_sem_WaitWithTimeOut(tafL2tpCallback::semaphore, timeToWait);

        TAF_ERROR_IF_RET_VAL(res != LE_OK, false, "Wait semaphore timeout\n");

        return tafL2tpCallback::l2tpConfig.enableL2tp;
    }
    else
    {
        LE_ERROR("Request l2tp info failed, status: %d",int(status));
        return false;
    }
}

/*======================================================================

 FUNCTION        taf_L2tp::IsL2tpMssEnabled

 DESCRIPTION     Is L2TP MSS enabled or not.

 DEPENDENCIES    The initialization of l2tp.

 PARAMETERS      None.

 RETURN VALUE    bool
                     true:      L2tp MSS is enabled
                     false      L2tp MSS is disabled

======================================================================*/
bool taf_L2tp::IsL2tpMssEnabled
(
    void
)
{
    TAF_ERROR_IF_RET_VAL(l2tpManager == NULL, false, "l2tpManager is null");

    telux::common::Status status = l2tpManager->requestConfig(
                                                tafL2tpCallback::requestConfigResponse);

    if (status == telux::common::Status::SUCCESS)
    {
        le_clk_Time_t timeToWait = {REQUEST_L2TP_CONF_TIMEOUT, 0};
        le_result_t res = le_sem_WaitWithTimeOut(tafL2tpCallback::semaphore, timeToWait);

        TAF_ERROR_IF_RET_VAL(res != LE_OK, false, "Wait semaphore timeout\n");

        return tafL2tpCallback::l2tpConfig.enableTcpMss;
    }
    else
    {
        LE_ERROR("Request l2tp info failed, status: %d",int(status));
        return false;
    }
}

/*======================================================================

 FUNCTION        taf_L2tp::IsL2tpMtuEnabled

 DESCRIPTION     Is L2TP MTU enabled or not.

 DEPENDENCIES    The initialization of l2tp.

 PARAMETERS      None.

 RETURN VALUE    bool
                     true:      L2tp MTU is enabled
                     false      L2tp MTU is disabled

======================================================================*/
bool taf_L2tp::IsL2tpMtuEnabled
(
    void
)
{
    TAF_ERROR_IF_RET_VAL(l2tpManager == NULL, false, "l2tpManager is null");

    telux::common::Status status = l2tpManager->requestConfig(
                                                tafL2tpCallback::requestConfigResponse);

    if (status == telux::common::Status::SUCCESS)
    {
        le_clk_Time_t timeToWait = {REQUEST_L2TP_CONF_TIMEOUT, 0};
        le_result_t res = le_sem_WaitWithTimeOut(tafL2tpCallback::semaphore, timeToWait);

        TAF_ERROR_IF_RET_VAL(res != LE_OK, false, "Wait semaphore timeout\n");

        return tafL2tpCallback::l2tpConfig.enableMtu;
    }
    else
    {
        LE_ERROR("Request l2tp info failed, status: %d",int(status));
        return false;
    }
}

/*======================================================================

 FUNCTION        taf_L2tp::GetL2tpMtuSize

 DESCRIPTION     Get l2tp MTU size.

 DEPENDENCIES    The creation of l2tp.

 PARAMETERS      None.

 RETURN VALUE    le_result_t
                     others:           The l2tp MTU size
                     0:                Failed to get l2tp MTU size

======================================================================*/
uint32_t taf_L2tp::GetL2tpMtuSize
(
    void
)
{
    TAF_ERROR_IF_RET_VAL(l2tpManager == NULL, 0, "l2tpManager is null");

    telux::common::Status status = l2tpManager->requestConfig(
                                                tafL2tpCallback::requestConfigResponse);

    if (status == telux::common::Status::SUCCESS)
    {
        le_clk_Time_t timeToWait = {REQUEST_L2TP_CONF_TIMEOUT, 0};
        le_result_t res = le_sem_WaitWithTimeOut(tafL2tpCallback::semaphore, timeToWait);

        TAF_ERROR_IF_RET_VAL(res != LE_OK, 0, "Wait semaphore timeout\n");

        return tafL2tpCallback::l2tpConfig.mtuSize;
    }
    else
    {
        LE_ERROR("Request l2tp info failed, status: %d",int(status));
        return 0;
    }
}

/*======================================================================

 FUNCTION        taf_L2tp::CreateTunnel

 DESCRIPTION     Create l2tp tunnel.

 DEPENDENCIES    The enablement of l2tp.

 PARAMETERS      [IN] taf_net_L2tpEncapProtocol_t encaProto : Encapsulation protocol.
                 [IN] uint32_t locId : Local tunnel id.
                 [IN] uint32_t peerId : Peer tunnel id.
                 [IN] char* peerIpAddrPtr : Peer ip address.
                 [IN] char* ifNamePtr: Interface name.
                 [IN] le_msg_SessionRef_t sessionRef : Client session reference.

 RETURN VALUE    le_result_t
                     NULL              Failed to create tunnel.
                     Others:           Reference of tunnel.

======================================================================*/
taf_net_TunnelRef_t taf_L2tp::CreateTunnel
(
    taf_net_L2tpEncapProtocol_t encaProto,
    uint32_t locId,
    uint32_t peerId,
    const char* peerIpAddrPtr,
    const char* ifNamePtr,
    le_msg_SessionRef_t sessionRef
)
{
    struct sockaddr_in6 addr6;
    struct sockaddr_in addr;
    taf_Tunnel_t *tunnelPtr=NULL;
    uint16_t tunnelNum = 0;

    TAF_ERROR_IF_RET_VAL(peerIpAddrPtr == NULL, NULL, "peerIpAddrPtr is invalid");
    TAF_ERROR_IF_RET_VAL(ifNamePtr == NULL, NULL, "ifNamePtr is invalid");
    TAF_ERROR_IF_RET_VAL(sessionRef == NULL, NULL, "sessionRef is invalid");

    //check if ip address is valid
    if ( !inet_pton(AF_INET, peerIpAddrPtr, &(addr.sin_addr) ) &&
         !inet_pton(AF_INET6, peerIpAddrPtr, &(addr6.sin6_addr)))
    {
        LE_ERROR("PeerIpAddrPtr is invalid");
        return NULL;
    }

    //Get tunnel reference by local id, only one tunnel can be created for the same tunnel id
    le_ref_IterRef_t iterRef = le_ref_GetIterator(tunnelRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        tunnelPtr = (taf_Tunnel_t*)le_ref_GetValue(iterRef);

        if (tunnelPtr != NULL && tunnelPtr->locTunnelId == locId)
        {
            if(tunnelPtr->sessionRef != sessionRef)
            {
                //Other client created this tunnel
                LE_ERROR("Tunnel is already created by other client for tunnel id %d", locId);
                return NULL;
            }
            else
            {
                LE_DEBUG("Found tunnel for tunnel id %d", locId);
                //updated tunnel information and return tunnerRef

                tunnelPtr->encaProto = encaProto;
                tunnelPtr->peerTunnelId = peerId;

                le_utf8_Copy(tunnelPtr->peerIpAddr, peerIpAddrPtr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
                le_utf8_Copy(tunnelPtr->interfaceName, ifNamePtr, TAF_NET_INTERFACE_NAME_MAX_LEN,
                             NULL);

                return (taf_net_TunnelRef_t)le_ref_GetSafeRef(iterRef);
            }
        }

        if(tunnelPtr != NULL)
            tunnelNum++;
    }

    //Check if the tunnel exceeds the max value
    TAF_ERROR_IF_RET_VAL(tunnelNum >=TAF_NET_L2TP_MAX_TUNNEL_NUMBER, NULL,
                         "Tunnel number exceeds the max value");

    //Not found, create a new tunnel reference
    tunnelPtr = (taf_Tunnel_t*)le_mem_ForceAlloc(tunnelPool);
    tunnelPtr->locTunnelId=locId;
    tunnelPtr->encaProto = encaProto;
    if(encaProto == TAF_NET_L2TP_IP)
    {
        tunnelPtr->localUdpPort = 0;
        tunnelPtr->localUdpPort = 0;
    }
    tunnelPtr->peerTunnelId = peerId;
    le_utf8_Copy(tunnelPtr->peerIpAddr, peerIpAddrPtr, TAF_NET_IP_ADDR_MAX_LEN, NULL);
    le_utf8_Copy(tunnelPtr->interfaceName, ifNamePtr, TAF_NET_INTERFACE_NAME_MAX_LEN, NULL);
    tunnelPtr->sessionRef=sessionRef;

    //Save the tunnel reference.
    return (taf_net_TunnelRef_t)le_ref_CreateRef(tunnelRefMap, tunnelPtr);
}

/*======================================================================

 FUNCTION        taf_L2tp::RemoveTunnel

 DESCRIPTION     Remove l2tp tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelRef_t tunnelRef : Tunnel reference.

 RETURN VALUE    le_result_t
                     LE_OK:                      Success.
                     LE_NOT_FOUND                Tunnel is not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to remove tunnel.

======================================================================*/
le_result_t taf_L2tp::RemoveTunnel
(
    taf_net_TunnelRef_t tunnelRef
)
{
    le_dls_Link_t* linkPtr = NULL;
    taf_Tunnel_t *tunnelPtr = NULL;

    TAF_ERROR_IF_RET_VAL(tunnelRef == NULL, LE_BAD_PARAMETER, "tunnelRef is null");

    tunnelPtr = (taf_Tunnel_t*)le_ref_Lookup(tunnelRefMap, tunnelRef);

    TAF_ERROR_IF_RET_VAL(tunnelPtr == NULL, LE_NOT_FOUND, "tunnel is not present");

    //remove sessions in this tunnel to avoid memory leak
    linkPtr = le_dls_Peek(&(tunnelPtr->l2tpSessionList));
    while (linkPtr)
    {
        taf_L2tpSession_t* sessionPtr = CONTAINER_OF(linkPtr, taf_L2tpSession_t, link);
        linkPtr = le_dls_PeekNext(&(tunnelPtr->l2tpSessionList), linkPtr);

        if(sessionPtr != NULL)
        {
            le_dls_Remove(&(tunnelPtr->l2tpSessionList), &(sessionPtr->link));
            le_mem_Release(sessionPtr);
        }
    }

    le_ref_DeleteRef(tunnelRefMap, tunnelRef);

    le_mem_Release(tunnelPtr);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_L2tp::SetTunnelUdpPort

 DESCRIPTION     Set tunnel udp port.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelRef_t tunnelRef : Tunnel reference.
                 [IN] uint32_t localUdpPort : Local udp port.
                 [IN] uint32_t peerUdpPort : Peer udp port.

 RETURN VALUE    le_result_t
                     LE_OK:                      Success
                     LE_NOT_FOUND                Tunnel is not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Encapsulation protol is not UDP.

======================================================================*/
le_result_t taf_L2tp::SetTunnelUdpPort
(
    taf_net_TunnelRef_t tunnelRef,
    uint32_t localUdpPort,
    uint32_t peerUdpPort
)
{
    taf_Tunnel_t *tunnelPtr = NULL;
    TAF_ERROR_IF_RET_VAL(tunnelRef == NULL, LE_BAD_PARAMETER, "tunnelRef is null");

    tunnelPtr = (taf_Tunnel_t*)le_ref_Lookup(tunnelRefMap, tunnelRef);

    TAF_ERROR_IF_RET_VAL(tunnelPtr == NULL, LE_NOT_FOUND, "tunnel is not present");

    //Check if encapsulation proto is udp
    TAF_ERROR_IF_RET_VAL(tunnelPtr->encaProto != TAF_NET_L2TP_UDP, LE_FAULT,
                         "The encapsulation protocol is not UDP");

    tunnelPtr->localUdpPort = localUdpPort;
    tunnelPtr->peerUdpPort = peerUdpPort;

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_L2tp::AddSession

 DESCRIPTION     Add a session into tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelRef_t tunnelRef : Tunnel reference.
                 [IN] uint32_t locSessionId : Local session id.
                 [IN] uint32_t peerSessionId : Peer session id.

 RETURN VALUE    le_result_t
                     LE_OK:                      Success
                     LE_NOT_FOUND                Tunnel is not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to add session.

======================================================================*/
le_result_t taf_L2tp::AddSession
(
    taf_net_TunnelRef_t tunnelRef,
    uint32_t locSessionId,
    uint32_t peerSessionId
)
{
    taf_Tunnel_t *tunnelPtr = NULL;
    TAF_ERROR_IF_RET_VAL(tunnelRef == NULL, LE_BAD_PARAMETER, "tunnelRef is null");

    tunnelPtr = (taf_Tunnel_t*)le_ref_Lookup(tunnelRefMap, tunnelRef);

    TAF_ERROR_IF_RET_VAL(tunnelPtr == NULL, LE_NOT_FOUND, "tunnel is not present");

    //Check if the session id is present or session number exceeds the max value for one tunnel
    if(!IsSessionIdValid(tunnelRef, locSessionId, peerSessionId))
        return LE_FAULT;

    taf_L2tpSession_t* l2tpSessionInfo = NULL;

    l2tpSessionInfo = (taf_L2tpSession_t *)le_mem_ForceAlloc(l2tpSessionPool);

    l2tpSessionInfo->locSessionId = locSessionId;
    l2tpSessionInfo->peerSessionId = peerSessionId;

    // add this profile context to list
    le_dls_Queue(&(tunnelPtr->l2tpSessionList), &(l2tpSessionInfo->link));

    return LE_OK;

}

/*======================================================================

 FUNCTION        taf_L2tp::RemoveSession

 DESCRIPTION     Remove a session from tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelRef_t tunnelRef : Tunnel reference.
                 [IN] uint32_t locSessionId : Local session id.
                 [IN] uint32_t peerSessionId : Peer session id.

 RETURN VALUE    le_result_t
                     LE_OK:                      Success
                     LE_NOT_FOUND                Tunnel is not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to remove session.

======================================================================*/
le_result_t taf_L2tp::RemoveSession
(
    taf_net_TunnelRef_t tunnelRef,
    uint32_t locSessionId,
    uint32_t peerSessionId
)
{
    le_dls_Link_t* linkPtr = NULL;
    taf_Tunnel_t *tunnelPtr = NULL;

    TAF_ERROR_IF_RET_VAL(tunnelRef == NULL, LE_BAD_PARAMETER, "tunnelRef is null");
    tunnelPtr = (taf_Tunnel_t*)le_ref_Lookup(tunnelRefMap, tunnelRef);

    TAF_ERROR_IF_RET_VAL(tunnelPtr == NULL, LE_NOT_FOUND, "tunnel is not present");

    linkPtr = le_dls_Peek(&(tunnelPtr->l2tpSessionList));
    while (linkPtr)
    {
        taf_L2tpSession_t* sessionPtr = CONTAINER_OF(linkPtr, taf_L2tpSession_t, link);
        linkPtr = le_dls_PeekNext(&(tunnelPtr->l2tpSessionList), linkPtr);

        if (sessionPtr->locSessionId == locSessionId && sessionPtr->peerSessionId == peerSessionId)
        {
            le_dls_Remove(&(tunnelPtr->l2tpSessionList), &(sessionPtr->link));
            le_mem_Release(sessionPtr);
            return LE_OK;
        }
    }
    return LE_FAULT;
}

/*======================================================================

 FUNCTION        taf_L2tp::StartTunnelCmdSync

 DESCRIPTION     Start a tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelRef_t tunnelRef : Tunnel reference.

 RETURN VALUE    le_result_t
                     LE_OK:                      Success
                     LE_NOT_FOUND                Tunnel is not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to start a tunnel.

======================================================================*/
le_result_t taf_L2tp::StartTunnelCmdSync(taf_net_TunnelRef_t tunnelRef)
{
    le_result_t result;
    le_dls_Link_t* linkPtr = NULL;
    telux::data::net::L2tpTunnelConfig l2tpTunnelConfig;
    struct sockaddr_in6 addr6;
    struct sockaddr_in addr;
    int sessionNum=0;
    std::chrono::seconds span(START_TUNNEL_TIMEOUT);
    taf_Tunnel_t *tunnelPtr = NULL;

    TAF_ERROR_IF_RET_VAL(tunnelRef == NULL, LE_BAD_PARAMETER, "tunnelRef is null");
    TAF_ERROR_IF_RET_VAL(l2tpManager == NULL, LE_NOT_FOUND, "l2tpManager is null");

    tunnelPtr = (taf_Tunnel_t*)le_ref_Lookup(tunnelRefMap, tunnelRef);

    TAF_ERROR_IF_RET_VAL(tunnelPtr == NULL, LE_NOT_FOUND, "tunnel is not present");

    linkPtr = le_dls_Peek(&(tunnelPtr->l2tpSessionList));

    if((tunnelPtr->encaProto == TAF_NET_L2TP_UDP) &&
       ((tunnelPtr->localUdpPort == 0) || (tunnelPtr->localUdpPort == 0)))
    {
        LE_ERROR( "Local or peer udp port is not set");
        return LE_FAULT;
    }

    while (linkPtr)
    {
        telux::data::net::L2tpSessionConfig l2tpSessionConfig;
        taf_L2tpSession_t* sessionPtr = CONTAINER_OF(linkPtr, taf_L2tpSession_t, link);
        linkPtr = le_dls_PeekNext(&(tunnelPtr->l2tpSessionList), linkPtr);

        l2tpSessionConfig.locId = sessionPtr->locSessionId;
        l2tpSessionConfig.peerId = sessionPtr->peerSessionId;
        l2tpTunnelConfig.sessionConfig.emplace_back(l2tpSessionConfig);
        sessionNum++;
    }

    if(sessionNum == 0)
    {
        LE_ERROR( "There is no session config in tunnel");
        return LE_FAULT;
    }

    l2tpTunnelConfig.prot = (telux::data::net::L2tpProtocol) tunnelPtr->encaProto;
    l2tpTunnelConfig.locId = tunnelPtr->locTunnelId;
    l2tpTunnelConfig.peerId = tunnelPtr->peerTunnelId;

    l2tpTunnelConfig.localUdpPort = tunnelPtr->localUdpPort;
    l2tpTunnelConfig.peerUdpPort = tunnelPtr->peerUdpPort;

    if (inet_pton(AF_INET, tunnelPtr->peerIpAddr, &(addr.sin_addr)))
    {
        l2tpTunnelConfig.peerIpv4Addr = tunnelPtr->peerIpAddr;
        l2tpTunnelConfig.ipType = telux::data::IpFamilyType::IPV4;
    }
    else if (inet_pton(AF_INET6, tunnelPtr->peerIpAddr, &(addr6.sin6_addr)))
    {
        l2tpTunnelConfig.peerIpv6Addr = tunnelPtr->peerIpAddr;
        l2tpTunnelConfig.ipType = telux::data::IpFamilyType::IPV6;
    }

    l2tpTunnelConfig.locIface =  tunnelPtr->interfaceName;

    L2tpStartTunnelSyncPromise = std::promise<le_result_t>();

    Status status = l2tpManager->addTunnel(l2tpTunnelConfig, tafL2tpCallback::startTunnelSyncResponse);

    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = L2tpStartTunnelSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Start tunnel timeout for %d seconds", START_TUNNEL_TIMEOUT);
            result = LE_TIMEOUT;
        }
        else
        {
            result = futureResult.get();
            if(result == LE_OK)
                tunnelPtr->isStarted = true;
        }

        return result;
    }
    else
    {
        LE_ERROR( "ERROR - Failed to start tunnel, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }

    return LE_OK;

}

/*======================================================================

 FUNCTION        taf_L2tp::StopTunnelCmdSync

 DESCRIPTION     Stop a tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelRef_t tunnelRef : Tunnel reference.

 RETURN VALUE    le_result_t
                     LE_OK:                      Success
                     LE_NOT_FOUND                Tunnel is not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to stop a tunnel.

======================================================================*/
le_result_t taf_L2tp::StopTunnelCmdSync(taf_net_TunnelRef_t tunnelRef)
{
    le_result_t result;
    taf_Tunnel_t *tunnelPtr = NULL;
    std::chrono::seconds span(START_TUNNEL_TIMEOUT);

    TAF_ERROR_IF_RET_VAL(tunnelRef == NULL, LE_BAD_PARAMETER, "tunnelRef is null");
    TAF_ERROR_IF_RET_VAL(l2tpManager == NULL, LE_NOT_FOUND, "l2tpManager is null");

    tunnelPtr = (taf_Tunnel_t*)le_ref_Lookup(tunnelRefMap, tunnelRef);

    TAF_ERROR_IF_RET_VAL(tunnelPtr == NULL, LE_NOT_FOUND, "tunnel is not present");

    L2tpStopTunnelSyncPromise = std::promise<le_result_t>();

    Status status = l2tpManager->removeTunnel(tunnelPtr->locTunnelId,
                                           tafL2tpCallback::stopTunnelSyncResponse);

    if (status == Status::SUCCESS)
    {
        std::future<le_result_t> futureResult = L2tpStopTunnelSyncPromise.get_future();
        std::future_status waitStatus = futureResult.wait_for(span);

        if (std::future_status::timeout == waitStatus)
        {
            LE_ERROR("Stop tunnel timeout for %d seconds", START_TUNNEL_TIMEOUT);
            result = LE_TIMEOUT;
        }
        else
        {
            result = futureResult.get();
            if(result == LE_OK)
                tunnelPtr->isStarted = false;
        }

        return result;
    }
    else
    {
        LE_ERROR( "ERROR - Failed to stop tunnel, Status:%d ", static_cast<int>(status));
        return LE_FAULT;
    }

    return LE_OK;

}

/*======================================================================

 FUNCTION        taf_L2tp::StartTunnelCmdAsync

 DESCRIPTION     Asynchronously start a tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelRef_t tunnelRef : Tunnel reference.
                 [IN] taf_net_AsyncTunnelHandlerFunc_t handlerPtr : Tunnel handler function.
                 [IN] void* contextPtr : Context pointer.
                 [IN] le_msg_SessionRef_t sessionRef : Session reference.

 RETURN VALUE    le_result_t
                     LE_OK:                      Success
                     LE_NOT_FOUND                Tunnel is not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to start a tunnel.

======================================================================*/
void taf_L2tp::StartTunnelCmdAsync
(
    taf_net_TunnelRef_t tunnelRef,
    taf_net_AsyncTunnelHandlerFunc_t handlerPtr,
    void* contextPtr,
    le_msg_SessionRef_t sessionRef
)
{
    taf_L2tpCmdReq_t cmdReq;
    TAF_ERROR_IF_RET_NIL(tunnelRef == NULL, "TunnelRef is NULL");
    TAF_ERROR_IF_RET_NIL(handlerPtr == NULL, "Handler function is NULL");
    TAF_ERROR_IF_RET_NIL(sessionRef == NULL, "SessionRef is NULL");

    cmdReq.cmdType = ASYNC_START_TUNNEL;
    cmdReq.tunnelRef = tunnelRef;
    cmdReq.contextPtr = contextPtr;
    cmdReq.sessionRef = sessionRef;
    cmdReq.tunnelHandlerFuncPtr = handlerPtr;

    // Send ASYNC_START_TUNNEL command
    le_event_Report(taf_L2tp::l2tpCmdId, &cmdReq, sizeof(cmdReq));

}

/*======================================================================

 FUNCTION        taf_L2tp::StopTunnelCmdAsync

 DESCRIPTION     Asynchronously stop a tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelRef_t tunnelRef : Tunnel reference.
                 [IN] taf_net_AsyncTunnelHandlerFunc_t handlerPtr : Tunnel handler function.
                 [IN] void* contextPtr : Context pointer.
                 [IN] le_msg_SessionRef_t sessionRef : Session reference.

 RETURN VALUE    le_result_t
                     LE_OK:                      Success
                     LE_NOT_FOUND                Tunnel is not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to stop a tunnel.

======================================================================*/
void taf_L2tp::StopTunnelCmdAsync
(
    taf_net_TunnelRef_t tunnelRef,
    taf_net_AsyncTunnelHandlerFunc_t handlerPtr,
    void* contextPtr,
    le_msg_SessionRef_t sessionRef
)
{
    taf_L2tpCmdReq_t cmdReq;

    TAF_ERROR_IF_RET_NIL(tunnelRef == NULL, "TunnelRef is NULL");
    TAF_ERROR_IF_RET_NIL(handlerPtr == NULL, "Handler function is NULL");
    TAF_ERROR_IF_RET_NIL(sessionRef == NULL, "SessionRef is NULL");

    cmdReq.cmdType = ASYNC_STOP_TUNNEL;
    cmdReq.tunnelRef = tunnelRef;
    cmdReq.contextPtr = contextPtr;
    cmdReq.tunnelHandlerFuncPtr = handlerPtr;

    // Send ASYNC_STOP_TUNNEL command
    le_event_Report(taf_L2tp::l2tpCmdId, &cmdReq, sizeof(cmdReq));

}

/*======================================================================

 FUNCTION        taf_L2tp::AddTunnelAsync

 DESCRIPTION     Start a tunnel by call telsdk API.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelRef_t tunnelRef : Tunnel reference.

 RETURN VALUE    le_result_t
                     LE_OK:                      Success
                     LE_NOT_FOUND                Tunnel is not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to start a tunnel.

======================================================================*/
le_result_t taf_L2tp::AddTunnelAsync(taf_net_TunnelRef_t tunnelRef)
{
    le_result_t result;
    le_dls_Link_t* linkPtr = NULL;
    telux::data::net::L2tpTunnelConfig l2tpTunnelConfig;
    struct sockaddr_in6 addr6;
    struct sockaddr_in addr;
    int sessionNum=0;

    taf_Tunnel_t *tunnelPtr = NULL;
    Status status = Status::SUCCESS;

    TAF_ERROR_IF_RET_VAL(tunnelRef == NULL, LE_BAD_PARAMETER, "tunnelRef is null");
    TAF_ERROR_IF_RET_VAL(l2tpManager == NULL, LE_NOT_FOUND, "l2tpManager is null");

    tunnelPtr = (taf_Tunnel_t*)le_ref_Lookup(tunnelRefMap, tunnelRef);

    TAF_ERROR_IF_RET_VAL(tunnelPtr == NULL, LE_NOT_FOUND, "tunnel is not present");

    linkPtr = le_dls_Peek(&(tunnelPtr->l2tpSessionList));

    if((tunnelPtr->encaProto == TAF_NET_L2TP_UDP) &&
       ((tunnelPtr->localUdpPort == 0) || (tunnelPtr->localUdpPort == 0)))
    {
        LE_ERROR( "Local or peer udp port is not set");
        return LE_FAULT;
    }

    while (linkPtr)
    {
        telux::data::net::L2tpSessionConfig l2tpSessionConfig;
        taf_L2tpSession_t* sessionPtr = CONTAINER_OF(linkPtr, taf_L2tpSession_t, link);
        linkPtr = le_dls_PeekNext(&(tunnelPtr->l2tpSessionList), linkPtr);

        l2tpSessionConfig.locId = sessionPtr->locSessionId;
        l2tpSessionConfig.peerId = sessionPtr->peerSessionId;
        l2tpTunnelConfig.sessionConfig.emplace_back(l2tpSessionConfig);
        sessionNum++;
    }

    if(sessionNum == 0)
    {
        LE_ERROR( "There is no session config in tunnel");
        return LE_FAULT;
    }

    l2tpTunnelConfig.prot = (telux::data::net::L2tpProtocol) tunnelPtr->encaProto;
    l2tpTunnelConfig.locId = tunnelPtr->locTunnelId;
    l2tpTunnelConfig.peerId = tunnelPtr->peerTunnelId;

    l2tpTunnelConfig.localUdpPort = tunnelPtr->localUdpPort;
    l2tpTunnelConfig.peerUdpPort = tunnelPtr->peerUdpPort;

    if (inet_pton(AF_INET, tunnelPtr->peerIpAddr, &(addr.sin_addr)))
    {
        l2tpTunnelConfig.peerIpv4Addr = tunnelPtr->peerIpAddr;
        l2tpTunnelConfig.ipType = telux::data::IpFamilyType::IPV4;
    }
    else if (inet_pton(AF_INET6, tunnelPtr->peerIpAddr, &(addr6.sin6_addr)))
    {
        l2tpTunnelConfig.peerIpv6Addr = tunnelPtr->peerIpAddr;
        l2tpTunnelConfig.ipType = telux::data::IpFamilyType::IPV6;
    }

    l2tpTunnelConfig.locIface =  tunnelPtr->interfaceName;

    status = l2tpManager->addTunnel(l2tpTunnelConfig,
                                    tafL2tpCallback::startTunnelAsyncResponse);

    if (status == Status::SUCCESS)
    {
        result = LE_OK;
    }
    else
    {
        LE_ERROR( "ERROR - Failed to start tunnel, Status:%d ", static_cast<int>(status));
        result = LE_FAULT;
    }

    return result;
}

/*======================================================================

 FUNCTION        taf_L2tp::RemoveTunnelAsync

 DESCRIPTION     Stop a tunnel by call telsdk API.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelRef_t tunnelRef : Tunnel reference.

 RETURN VALUE    le_result_t
                     LE_OK:                      Success
                     LE_NOT_FOUND                Tunnel is not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to stop a tunnel.

======================================================================*/
le_result_t taf_L2tp::RemoveTunnelAsync(taf_net_TunnelRef_t tunnelRef)
{
    le_result_t result;
    taf_Tunnel_t *tunnelPtr = NULL;

    Status status = Status::SUCCESS;

    TAF_ERROR_IF_RET_VAL(tunnelRef == NULL, LE_BAD_PARAMETER, "tunnelRef is null");
    TAF_ERROR_IF_RET_VAL(l2tpManager == NULL, LE_NOT_FOUND, "l2tpManager is null");

    tunnelPtr = (taf_Tunnel_t*)le_ref_Lookup(tunnelRefMap, tunnelRef);

    TAF_ERROR_IF_RET_VAL(tunnelPtr == NULL, LE_NOT_FOUND, "tunnel is not present");

    status = l2tpManager->removeTunnel(tunnelPtr->locTunnelId,
                                           tafL2tpCallback::stopTunnelAsyncResponse);

    if (status == Status::SUCCESS)
    {
        result = LE_OK;
    }
    else
    {
        LE_ERROR( "ERROR - Failed to stop tunnel, Status:%d ", static_cast<int>(status));
        result = LE_FAULT;
    }

    return result;
}

/*======================================================================

 FUNCTION        taf_L2tp::GetTunnelEntryList

 DESCRIPTION     Get tunnel entry list.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      None.

 RETURN VALUE    taf_net_TunnelEntryListRef_t
                     NULL              Failed to get tunnel entry list.
                     Others:           Reference of tunnel entry list.

======================================================================*/
taf_net_TunnelEntryListRef_t taf_L2tp::GetTunnelEntryList
(
    void
)
{
    le_ref_IterRef_t iterRef;
    int sessionIndex = 0;

    TAF_ERROR_IF_RET_VAL(l2tpManager == NULL, NULL, "l2tpManager is null");

    telux::common::Status status = l2tpManager->requestConfig(
                                                tafL2tpCallback::requestConfigResponse);

    if (status == telux::common::Status::SUCCESS)
    {
        le_clk_Time_t timeToWait = {REQUEST_L2TP_CONF_TIMEOUT, 0};
        le_result_t res = le_sem_WaitWithTimeOut(tafL2tpCallback::semaphore, timeToWait);
        TAF_ERROR_IF_RET_VAL(res != LE_OK, NULL, "Wait semaphore timeout\n");

        if(tafL2tpCallback::l2tpConfig.enableL2tp == false ||
           tafL2tpCallback::configList.size() == 0)
        {
            LE_DEBUG("No tunnel entry");
            return NULL;
        }

        TAF_ERROR_IF_RET_VAL(tunnelEntryListRefMap == NULL, NULL, "tunnelEntryListRefMap is null");

        iterRef = (le_ref_IterRef_t)le_ref_GetIterator(tunnelEntryListRefMap);

        if(iterRef != NULL && le_ref_GetValue(iterRef) != NULL
                           && le_ref_GetSafeRef(iterRef) != NULL)
            CleanListRef((taf_net_TunnelEntryListRef_t)le_ref_GetSafeRef(iterRef));

        taf_TunnelEntryList_t* tunnelEntriesList =
                                     (taf_TunnelEntryList_t*)le_mem_ForceAlloc(tunnelEntryListPool);

        tunnelEntriesList->tunnelEntryList = LE_SLS_LIST_INIT;
        tunnelEntriesList->safeRefList = LE_SLS_LIST_INIT;
        tunnelEntriesList->currPtr = NULL;

        taf_TunnelEntry_t* tunnelEntryPtr;

        for (auto info : tafL2tpCallback::configList)
        {
            tunnelEntryPtr = (taf_TunnelEntry_t*)le_mem_ForceAlloc(tunnelEntryPool);
            tunnelEntryPtr->info.locTunnelId=info.locId;
            tunnelEntryPtr->info.peerTunnelId=info.peerId;
            tunnelEntryPtr->info.localUdpPort=info.localUdpPort;
            tunnelEntryPtr->info.peerUdpPort=info.peerUdpPort;
            tunnelEntryPtr->info.encaProto=(taf_net_L2tpEncapProtocol_t)info.prot;

            if(info.ipType == telux::data::IpFamilyType::IPV4)
            {
                le_utf8_Copy(tunnelEntryPtr->info.peerIpv4Addr, info.peerIpv4Addr.c_str(),
                             TAF_NET_IPV4_ADDR_MAX_LEN, NULL);
            }
            else if(info.ipType == telux::data::IpFamilyType::IPV6)
            {
                le_utf8_Copy(tunnelEntryPtr->info.peerIpv6Addr, info.peerIpv6Addr.c_str(),
                             TAF_NET_IPV6_ADDR_MAX_LEN, NULL);
            }

            le_utf8_Copy(tunnelEntryPtr->info.interfaceName, info.locIface.c_str(),
                         TAF_NET_INTERFACE_NAME_MAX_LEN, NULL);

            tunnelEntryPtr->info.ipType = (taf_net_IpFamilyType_t)info.ipType;

            sessionIndex = 0;

            for (auto session : info.sessionConfig)
            {
                if(sessionIndex >= TAF_NET_L2TP_MAX_SESSION_NUMBER_PER_TUNNEL)
                {
                    LE_ERROR("Exceed max session number per TUNNEL: %d",
                              TAF_NET_L2TP_MAX_SESSION_NUMBER_PER_TUNNEL);
                    break;
                }
                tunnelEntryPtr->info.l2tpSessionConfig[sessionIndex].locId = session.locId;
                tunnelEntryPtr->info.l2tpSessionConfig[sessionIndex].peerId = session.peerId;
                sessionIndex ++;
            }

            tunnelEntryPtr->info.sessionNum = sessionIndex;

            tunnelEntryPtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(tunnelEntriesList->tunnelEntryList), &(tunnelEntryPtr->link));

        }

        return (taf_net_TunnelEntryListRef_t)le_ref_CreateRef(tunnelEntryListRefMap,
                                                            (void*)tunnelEntriesList);
    }
    else
    {
        LE_ERROR("Request tunnel entry list failed, status: %d",int(status));
        return NULL;
    }

}

/*======================================================================

 FUNCTION        taf_L2tp::GetFirstTunnelEntry

 DESCRIPTION     Get first tunnel entry.

 DEPENDENCIES    Initialization of tunnel entry list

 PARAMETERS      [IN] taf_net_TunnelEntryListRef_t tunnelEntryListRef : Tunnel entry list reference.

 RETURN VALUE    taf_net_TunnelEntryRef_t
                     NULL:             Failed to get reference of first L2TP tunnel entry.
                     Others:           Reference of first L2TP tunnel entry.

======================================================================*/
taf_net_TunnelEntryRef_t taf_L2tp::GetFirstTunnelEntry
(
    taf_net_TunnelEntryListRef_t tunnelEntryListRef
)
{
    taf_TunnelEntryList_t* listPtr = (taf_TunnelEntryList_t*)le_ref_Lookup(tunnelEntryListRefMap,
                                                                           tunnelEntryListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == NULL, NULL,
        "failed to look up the reference:%p", tunnelEntryListRef);

    le_sls_Link_t* linkPtr = le_sls_Peek(&(listPtr->tunnelEntryList));

    TAF_ERROR_IF_RET_VAL(linkPtr == NULL, NULL, "Empty list");

    taf_TunnelEntry_t* tunnelEntryPtr = CONTAINER_OF(linkPtr, taf_TunnelEntry_t , link);
    listPtr->currPtr = linkPtr;

    taf_TunnelEntrySafeRef_t* safeRefPtr =
                           (taf_TunnelEntrySafeRef_t*)le_mem_ForceAlloc(tunnelEntrySafeRefPool);

    safeRefPtr->safeRef = le_ref_CreateRef(tunnelEntrySafeRefMap, (void*)tunnelEntryPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;

    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_net_TunnelEntryRef_t)safeRefPtr->safeRef;
}

/*======================================================================

 FUNCTION        taf_L2tp::GetNextTunnelEntry

 DESCRIPTION     Get next tunnel entry.

 DEPENDENCIES    Initialization of tunnel entry list

 PARAMETERS      [IN] taf_net_TunnelEntryListRef_t tunnelEntryListRef : Tunnel entry list reference.

 RETURN VALUE    taf_net_TunnelEntryRef_t
                    NULL:             Failed to get reference of next L2TP tunnel entry.
                    Others:           Reference of next L2TP tunnel entry.

======================================================================*/
taf_net_TunnelEntryRef_t taf_L2tp::GetNextTunnelEntry
(
    taf_net_TunnelEntryListRef_t tunnelEntryListRef
)
{
    taf_TunnelEntryList_t* listPtr = (taf_TunnelEntryList_t*)le_ref_Lookup(tunnelEntryListRefMap,
        tunnelEntryListRef);

    TAF_ERROR_IF_RET_VAL(listPtr == NULL, NULL,
        "failed to look up the reference:%p", tunnelEntryListRef);

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(listPtr->tunnelEntryList), listPtr->currPtr);

    if(linkPtr == nullptr)
    {
        LE_DEBUG("Reach to the end of list");
        return NULL;
    }

    taf_TunnelEntry_t* tunnelEntryPtr = CONTAINER_OF(linkPtr, taf_TunnelEntry_t , link);
    listPtr->currPtr = linkPtr;

    taf_TunnelEntrySafeRef_t* safeRefPtr =
                           (taf_TunnelEntrySafeRef_t*)le_mem_ForceAlloc(tunnelEntrySafeRefPool);

    safeRefPtr->safeRef = le_ref_CreateRef(tunnelEntrySafeRefMap, (void*)tunnelEntryPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;

    le_sls_Queue(&(listPtr->safeRefList), &(safeRefPtr->link));

    return (taf_net_TunnelEntryRef_t)safeRefPtr->safeRef;
}

/*======================================================================

 FUNCTION        taf_L2tp::DeleteTunnelEntryList

 DESCRIPTION     Delete tunnel entry list.

 PARAMETERS      [IN] taf_net_TunnelEntryListRef_t tunnelEntryListRef : Tunnel entry list reference.

 RETURN VALUE    le_result_t
                     LE_OK:                      Success
                     LE_NOT_FOUND                Not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to delete the tunnel entry list

======================================================================*/
le_result_t taf_L2tp::DeleteTunnelEntryList
(
    taf_net_TunnelEntryListRef_t tunnelEntryListRef
)
{
    TAF_ERROR_IF_RET_VAL(tunnelEntryListRef == NULL, LE_BAD_PARAMETER,
                         "Null reference(tunnelEntryListRef)");

    return CleanListRef(tunnelEntryListRef);
}

/*======================================================================

 FUNCTION        taf_L2tp::GetTunnelEncapProto

 DESCRIPTION     Get encapsulation protocol of a tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelEntryRef_t tunnelEntryRef : Reference of a L2TP tunnel entry.

 RETURN VALUE    taf_net_L2tpEncapProtocol_t

======================================================================*/
taf_net_L2tpEncapProtocol_t taf_L2tp::GetTunnelEncapProto
(
    taf_net_TunnelEntryRef_t tunnelEntryRef
)
{
    TAF_ERROR_IF_RET_VAL(tunnelEntryRef == NULL, TAF_NET_L2TP_NONE,
                         "Null reference(tunnelEntryRef)");

    taf_TunnelEntry_t* tunnelEntryPtr = (taf_TunnelEntry_t*)le_ref_Lookup(tunnelEntrySafeRefMap,
                                                                          tunnelEntryRef);
    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr == NULL, TAF_NET_L2TP_NONE,
                         "Invalid para(null reference ptr)");

    return tunnelEntryPtr->info.encaProto;
}

/*======================================================================

 FUNCTION        taf_L2tp::GetTunnelLocalId

 DESCRIPTION     Get local id of a tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelEntryRef_t tunnelEntryRef : Reference of a L2TP tunnel entry.

 RETURN VALUE    uint32_t
                  0:                             Failed to get local id.
                  Others                         Success.


======================================================================*/
uint32_t taf_L2tp::GetTunnelLocalId
(
    taf_net_TunnelEntryRef_t tunnelEntryRef
)
{
    TAF_ERROR_IF_RET_VAL(tunnelEntryRef == NULL, 0, "Null reference(tunnelEntryRef)");

    taf_TunnelEntry_t* tunnelEntryPtr = (taf_TunnelEntry_t*)le_ref_Lookup(tunnelEntrySafeRefMap,
                                                                          tunnelEntryRef);
    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr == NULL, 0, "Invalid para(null reference ptr)");

    return tunnelEntryPtr->info.locTunnelId;
}

/*======================================================================

 FUNCTION        taf_L2tp::GetTunnelPeerId

 DESCRIPTION     Get peer id of a tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelEntryRef_t tunnelEntryRef : Reference of a L2TP tunnel entry.

 RETURN VALUE    uint32_t
                  0:                             Failed to get peer id.
                  Others                         Success.


======================================================================*/
uint32_t taf_L2tp::GetTunnelPeerId
(
    taf_net_TunnelEntryRef_t tunnelEntryRef
)
{
    TAF_ERROR_IF_RET_VAL(tunnelEntryRef == NULL, 0, "Null reference(tunnelEntryRef)");

    taf_TunnelEntry_t* tunnelEntryPtr = (taf_TunnelEntry_t*)le_ref_Lookup(tunnelEntrySafeRefMap,
                                                                          tunnelEntryRef);
    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr == NULL, 0, "Invalid para(null reference ptr)");

    return tunnelEntryPtr->info.peerTunnelId;
}


/*======================================================================

 FUNCTION        taf_L2tp::GetTunnelLocalUdpPort

 DESCRIPTION     Get local udp port of a tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelEntryRef_t tunnelEntryRef : Reference of a L2TP tunnel entry.

 RETURN VALUE    uint32_t
                  0:                             Failed to get local udp port.
                  Others                         Success.

======================================================================*/
uint32_t taf_L2tp::GetTunnelLocalUdpPort
(
    taf_net_TunnelEntryRef_t tunnelEntryRef
)
{
    TAF_ERROR_IF_RET_VAL(tunnelEntryRef == NULL, 0, "Null reference(tunnelEntryRef)");

    taf_TunnelEntry_t* tunnelEntryPtr = (taf_TunnelEntry_t*)le_ref_Lookup(tunnelEntrySafeRefMap,
                                                                          tunnelEntryRef);
    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr == NULL, 0, "Invalid para(null reference ptr)");

    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr->info.encaProto != TAF_NET_L2TP_UDP, 0,
                         "Encapsulation protocol is not UDP");

    return tunnelEntryPtr->info.localUdpPort;
}

/*======================================================================

 FUNCTION        taf_L2tp::GetTunnelPeerUdpPort

 DESCRIPTION     Get peer udp port of a tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelEntryRef_t tunnelEntryRef : Reference of a L2TP tunnel entry.

 RETURN VALUE    uint32_t
                  0:                             Failed to get peer udp port.
                  Others                         Success.

======================================================================*/
uint32_t taf_L2tp::GetTunnelPeerUdpPort
(
    taf_net_TunnelEntryRef_t tunnelEntryRef
)
{
    TAF_ERROR_IF_RET_VAL(tunnelEntryRef == NULL, 0, "Null reference(tunnelEntryRef)");

    taf_TunnelEntry_t* tunnelEntryPtr = (taf_TunnelEntry_t*)le_ref_Lookup(tunnelEntrySafeRefMap,
                                                                          tunnelEntryRef);
    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr == NULL, 0, "Invalid para(null reference ptr)");

    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr->info.encaProto != TAF_NET_L2TP_UDP, 0,
                         "Encapsulation protocol is not UDP");

    return tunnelEntryPtr->info.peerUdpPort;
}

/*======================================================================

 FUNCTION        taf_L2tp::GetTunnelPeerIpv6Addr

 DESCRIPTION     Get peer IP v6 address of a tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelEntryRef_t tunnelEntryRef : Reference of a L2TP tunnel entry.
                 [OUT] char* peerIpv6Addr : Peer ipv6 address.
                 [IN] size_t peerIpv6AddrSize : Peer ipv6 address size.

 RETURN VALUE    le_result_t
                     LE_OK:                      Success.
                     LE_NOT_FOUND                Not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to get peer ipv6 address.

======================================================================*/
le_result_t taf_L2tp::GetTunnelPeerIpv6Addr
(
    taf_net_TunnelEntryRef_t tunnelEntryRef,
    char* peerIpv6Addr,
    size_t peerIpv6AddrSize
)
{
    TAF_ERROR_IF_RET_VAL(tunnelEntryRef == NULL, LE_BAD_PARAMETER, "Invalid para(tunnelEntryRef)");

    TAF_ERROR_IF_RET_VAL(peerIpv6Addr == NULL, LE_BAD_PARAMETER, "Invalid para(peerIpv6Addr)");

    taf_TunnelEntry_t* tunnelEntryPtr = (taf_TunnelEntry_t*)le_ref_Lookup(tunnelEntrySafeRefMap,
                                                                          tunnelEntryRef);
    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr == NULL, LE_NOT_FOUND, "Can't find the info");

    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr->info.ipType != TAF_NET_L2TP_IPV6, LE_FAULT,
                         "IP type is not version 6");

    le_utf8_Copy(peerIpv6Addr, tunnelEntryPtr->info.peerIpv6Addr, peerIpv6AddrSize, NULL);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_L2tp::GetTunnelPeerIpv4Addr

 DESCRIPTION     Get peer IP v4 address of a tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelEntryRef_t tunnelEntryRef : Reference of a L2TP tunnel entry.
                 [OUT] char* peerIpv4Addr : Peer ipv4 address.
                 [IN] size_t peerIpv4AddrSize : Peer ipv4 address size.

 RETURN VALUE    le_result_t
                     LE_OK:                      Success.
                     LE_NOT_FOUND                Not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to get peer ipv4 address.

======================================================================*/
le_result_t taf_L2tp::GetTunnelPeerIpv4Addr
(
    taf_net_TunnelEntryRef_t tunnelEntryRef,
    char* peerIpv4Addr,
    size_t peerIpv4AddrSize
)
{
    TAF_ERROR_IF_RET_VAL(tunnelEntryRef == NULL, LE_BAD_PARAMETER, "Invalid para(tunnelEntryRef)");

    TAF_ERROR_IF_RET_VAL(peerIpv4Addr == NULL, LE_BAD_PARAMETER, "Invalid para(peerIpv4Addr)");

    taf_TunnelEntry_t* tunnelEntryPtr = (taf_TunnelEntry_t*)le_ref_Lookup(tunnelEntrySafeRefMap,
                                                                          tunnelEntryRef);
    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr == NULL, LE_NOT_FOUND, "Can't find the info");

    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr->info.ipType != TAF_NET_L2TP_IPV4, LE_FAULT,
                         "IP type is not version 4");

    le_utf8_Copy(peerIpv4Addr, tunnelEntryPtr->info.peerIpv4Addr, peerIpv4AddrSize, NULL);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_L2tp::GetTunnelInterfaceName

 DESCRIPTION     Get interface name of a tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelEntryRef_t tunnelEntryRef : Reference of a L2TP tunnel entry.
                 [OUT] char* ifName : Interface name.
                 [IN] size_t ifNameSize : Interface name size.

 RETURN VALUE    le_result_t
                     LE_OK:                      Success.
                     LE_NOT_FOUND                Not found.
                     LE_BAD_PARAMETER            Invalid parameter.

======================================================================*/
le_result_t taf_L2tp::GetTunnelInterfaceName
(
    taf_net_TunnelEntryRef_t tunnelEntryRef,
    char* ifName,
    size_t ifNameSize
)
{
    TAF_ERROR_IF_RET_VAL(tunnelEntryRef == NULL, LE_BAD_PARAMETER, "Invalid para(tunnelEntryRef)");

    TAF_ERROR_IF_RET_VAL(ifName == NULL, LE_BAD_PARAMETER, "Invalid para(ifName)");

    taf_TunnelEntry_t* tunnelEntryPtr = (taf_TunnelEntry_t*)le_ref_Lookup(tunnelEntrySafeRefMap,
                                                                          tunnelEntryRef);
    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr == NULL, LE_NOT_FOUND, "Can't find the info");

    le_utf8_Copy(ifName, tunnelEntryPtr->info.interfaceName, ifNameSize, NULL);

    return LE_OK;
}


/*======================================================================

 FUNCTION        taf_L2tp::GetTunnelIpType

 DESCRIPTION     Get ip type of a tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelEntryRef_t tunnelEntryRef : Reference of a L2TP tunnel entry.

 RETURN VALUE    taf_net_IpFamilyType_t

======================================================================*/
taf_net_IpFamilyType_t  taf_L2tp::GetTunnelIpType
(
    taf_net_TunnelEntryRef_t tunnelEntryRef
)
{
    TAF_ERROR_IF_RET_VAL(tunnelEntryRef == NULL, TAF_NET_L2TP_UNKNOWN,
                         "Null reference(tunnelEntryRef)");

    taf_TunnelEntry_t* tunnelEntryPtr = (taf_TunnelEntry_t*)le_ref_Lookup(tunnelEntrySafeRefMap,
                                                                          tunnelEntryRef);
    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr == NULL, TAF_NET_L2TP_UNKNOWN,
                         "Invalid para(null reference ptr)");

    return tunnelEntryPtr->info.ipType;
}

/*======================================================================

 FUNCTION        taf_L2tp::GetSessionConfig

 DESCRIPTION     Get session config of a tunnel.

 DEPENDENCIES    The creation of tunnel.

 PARAMETERS      [IN] taf_net_TunnelEntryRef_t tunnelEntryRef : Reference of a L2TP tunnel entry.
                 [OUT] taf_net_L2tpSessionConfig_t* sessionConfigPtr : Session config
                 [IN] size_t* sessionConfigSizePtr : Session config size

 RETURN VALUE    le_result_t
                     LE_OK:                      Success.
                     LE_NOT_FOUND                Not found.
                     LE_BAD_PARAMETER            Invalid parameter.
                     LE_FAULT                    Failed to get session config.

======================================================================*/
le_result_t taf_L2tp::GetSessionConfig
(
    taf_net_TunnelEntryRef_t tunnelEntryRef,
    taf_net_L2tpSessionConfig_t* sessionConfigPtr,
    size_t* sessionConfigSizePtr
)
{
    int sessionNum=0;

    TAF_ERROR_IF_RET_VAL(sessionConfigPtr == NULL, LE_BAD_PARAMETER,
                         "Null pointer(sessionConfigPtr)");
    TAF_ERROR_IF_RET_VAL(tunnelEntryRef == NULL, LE_BAD_PARAMETER,
                         "Null reference(tunnelEntryRef)");

    taf_TunnelEntry_t* tunnelEntryPtr = (taf_TunnelEntry_t*)le_ref_Lookup(tunnelEntrySafeRefMap,
                                                                          tunnelEntryRef);

    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr == NULL, LE_NOT_FOUND, "Session is not found");

    TAF_ERROR_IF_RET_VAL(tunnelEntryPtr->info.sessionNum == 0, LE_FAULT,
                         "Session is not found");

    //sessionConfigSizePtr is a IN/OUT value, if the value of *sessionConfigSizePtr is less than
    //the actual size of session config, just return the size of *sessionConfigSizePtr
    if(*sessionConfigSizePtr < tunnelEntryPtr->info.sessionNum)
        sessionNum=*sessionConfigSizePtr;
    else
        sessionNum=tunnelEntryPtr->info.sessionNum;

    for( int i=0; i < sessionNum ; i++ )
    {
        sessionConfigPtr[i].locId = tunnelEntryPtr->info.l2tpSessionConfig[i].locId;
        sessionConfigPtr[i].peerId = tunnelEntryPtr->info.l2tpSessionConfig[i].peerId;
    }

    *sessionConfigSizePtr = sessionNum ;

    return LE_OK;
}

/*======================================================================

 FUNCTION        tafL2tpCallback::requestConfigResponse

 DESCRIPTION     Call back function for request l2tp config.

 DEPENDENCIES    The initialization of l2tp.

 PARAMETERS      [IN] const telux::data::net::L2tpSysConfig &l2tpSysConfig:
                      L2tp Config information.
                 [IN] telux::common::ErrorCode error: error code.
 RETURN VALUE    None.

======================================================================*/
void tafL2tpCallback::requestConfigResponse
(
    const telux::data::net::L2tpSysConfig &l2tpSysConfig,
    telux::common::ErrorCode error
)
{
    LE_DEBUG("<SDK Callback> tafL2tpCallback --> requestConfigResponse");
    LE_INFO("Get L2TP Config Response from SDK %s",
                        (telux::common::ErrorCode::SUCCESS == error) ? "is successful" : "failed");
    // SUCCESS means that L2TP is enabled
    if(error == telux::common::ErrorCode::SUCCESS)
    {
        l2tpConfig.enableL2tp = l2tpSysConfig.enableMtu || l2tpSysConfig.enableTcpMss;
        l2tpConfig.enableMtu=l2tpSysConfig.enableMtu;
        l2tpConfig.enableTcpMss=l2tpSysConfig.enableTcpMss;
        l2tpConfig.mtuSize=l2tpSysConfig.mtuSize;
        configList.assign(l2tpSysConfig.configList.begin(), l2tpSysConfig.configList.end());
        le_sem_Post(semaphore);
        return;
    }
    // ERROR
    LE_ERROR("ErrorCode %d", static_cast<int>(error));
    // NOT_SUPPORTED means that L2TP is disabled
    if(error == telux::common::ErrorCode::NOT_SUPPORTED){
        LE_ERROR("L2TP Unmanaged tunnel state is not enabled");
    }
    l2tpConfig.enableL2tp = false;
    l2tpConfig.enableMtu=false;
    l2tpConfig.enableTcpMss=false;
    l2tpConfig.mtuSize=0;
    configList.clear();
    le_sem_Post(semaphore);
}

/*======================================================================

 FUNCTION        taf_L2tp::AddHandlerSessionMapping

 DESCRIPTION     Add handler and session into mapping list.

 DEPENDENCIES    The initialization of l2tp.

 PARAMETERS      [IN] asyncHandler: The handler function.
                 [IN] contextPtr: The context pointer.
                 [IN] sessionRef: The client session reference.
                 [IN] type: The command type.

 RETURN VALUE    None

======================================================================*/
void taf_L2tp::AddL2tpHandlerSessionMapping
(
    taf_net_AsyncL2tpHandlerFunc_t asyncHandler,
    void *contextPtr,
    le_msg_SessionRef_t sessionRef,
    taf_L2tpCmdType_t type
)
{
    L2tpHandlerMapping_t *handlerSessionMapping;

    TAF_ERROR_IF_RET_NIL(asyncHandler == nullptr, "Null ptr");
    TAF_ERROR_IF_RET_NIL(sessionRef == nullptr, "Null ptr");

    handlerSessionMapping = (L2tpHandlerMapping_t *)le_mem_ForceAlloc(L2tpHandlerMappingPool);

    TAF_ERROR_IF_RET_NIL(handlerSessionMapping == nullptr ,
                         "Failed to alloc memory for handlerSessionMapping");

    memset(handlerSessionMapping, 0, sizeof(L2tpHandlerMapping_t));

    handlerSessionMapping->asyncHandler = asyncHandler;
    handlerSessionMapping->contextPtr = contextPtr;
    handlerSessionMapping->sessionRef = sessionRef;
    handlerSessionMapping->type = type;
    handlerSessionMapping->handlerLink = LE_DLS_LINK_INIT;

    le_dls_Queue(&L2tpHandlerMappingList, &handlerSessionMapping->handlerLink);
}

/*======================================================================

 FUNCTION        taf_L2tp::FindL2tpAsyncHandler

 DESCRIPTION     Find handler function by type.

 DEPENDENCIES    The initialization of L2tpHandlerMappingList.

 PARAMETERS      [IN] type: The command type.

 RETURN VALUE    L2tpHandlerMapping_t*   The pointer.

======================================================================*/
L2tpHandlerMapping_t* taf_L2tp::FindL2tpAsyncHandler
(
    taf_L2tpCmdType_t type
)
{
    L2tpHandlerMapping_t *handlerSessionInfo;
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&L2tpHandlerMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, L2tpHandlerMapping_t, handlerLink);
        if (handlerSessionInfo->asyncHandler != NULL && handlerSessionInfo->type == type)
        {
            LE_DEBUG("Found async handler %p ", handlerSessionInfo->asyncHandler);
            return handlerSessionInfo;
        }
        handlerLinkPtr = le_dls_PeekNext(&L2tpHandlerMappingList, handlerLinkPtr);
    }

    return NULL;
}

/*======================================================================

 FUNCTION        taf_L2tp::GetL2tpHandlerNumberInMappingList

 DESCRIPTION     Get the number of handler function in mapping list.

 DEPENDENCIES    The initialization of L2tpHandlerMappingList.

 PARAMETERS      [IN] type: The command type.

 RETURN VALUE    int         The number of handler function in mapping list.

======================================================================*/
int taf_L2tp::GetL2tpHandlerNumberInMappingList
(
    taf_L2tpCmdType_t type
)
{
    int counter=0;
    L2tpHandlerMapping_t *handlerSessionInfo;
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&L2tpHandlerMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, L2tpHandlerMapping_t, handlerLink);
        if (handlerSessionInfo->asyncHandler != NULL && handlerSessionInfo->type == type)
        {
            counter++;
        }
        handlerLinkPtr = le_dls_PeekNext(&L2tpHandlerMappingList, handlerLinkPtr);
    }

    return counter;
}

/*======================================================================

 FUNCTION        taf_L2tp::DeleteL2tpHandlerInfo

 DESCRIPTION     Delete the handler in mapping list.

 DEPENDENCIES    The initialization of L2tpHandlerMappingList.

 PARAMETERS      [IN] asyncHandler: The handler function.

 RETURN VALUE    None

======================================================================*/
void taf_L2tp::DeleteL2tpHandlerInfo(taf_net_AsyncL2tpHandlerFunc_t asyncHandler)
{

    TAF_ERROR_IF_RET_NIL(asyncHandler == nullptr, "Null ptr");

    L2tpHandlerMapping_t *handlerSessionInfo;
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&L2tpHandlerMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, L2tpHandlerMapping_t, handlerLink);
        handlerLinkPtr = le_dls_PeekNext(&L2tpHandlerMappingList, handlerLinkPtr);
        if (handlerSessionInfo->asyncHandler == asyncHandler)
        {
            le_dls_Remove(&L2tpHandlerMappingList, &handlerSessionInfo->handlerLink);

            le_mem_Release(handlerSessionInfo);
            break;
        }
    }
}

/*======================================================================

 FUNCTION        taf_L2tp::AddTunnelHandlerSessionMapping

 DESCRIPTION     Add handler and session into mapping list.

 DEPENDENCIES    The initialization of l2tp.

 PARAMETERS      [IN] asyncHandler: The handler function.
                 [IN] tunnelRef: The tunnel reference.
                 [IN] contextPtr: The context pointer.
                 [IN] sessionRef: The client session reference.
                 [IN] type: The command type.

 RETURN VALUE    None

======================================================================*/
void taf_L2tp::AddTunnelHandlerSessionMapping
(
    taf_net_AsyncTunnelHandlerFunc_t asyncHandler,
    taf_net_TunnelRef_t tunnelRef,
    void *contextPtr,
    le_msg_SessionRef_t sessionRef,
    taf_L2tpCmdType_t type
)
{
    TunnelHandlerMapping_t *handlerSessionMapping;

    TAF_ERROR_IF_RET_NIL(asyncHandler == nullptr, "Null ptr");
    TAF_ERROR_IF_RET_NIL(tunnelRef == nullptr, "Null ptr");
    TAF_ERROR_IF_RET_NIL(sessionRef == nullptr, "Null ptr");

    handlerSessionMapping = (TunnelHandlerMapping_t *)le_mem_ForceAlloc(TunnelHandlerMappingPool);

    TAF_ERROR_IF_RET_NIL(handlerSessionMapping == nullptr ,
                         "Failed to alloc memory for handlerSessionMapping");

    memset(handlerSessionMapping, 0, sizeof(TunnelHandlerMapping_t));

    handlerSessionMapping->asyncHandler = asyncHandler;
    handlerSessionMapping->tunnelRef = tunnelRef;
    handlerSessionMapping->contextPtr = contextPtr;
    handlerSessionMapping->sessionRef = sessionRef;
    handlerSessionMapping->type = type;
    handlerSessionMapping->handlerLink = LE_DLS_LINK_INIT;

    le_dls_Queue(&TunnelHandlerMappingList, &handlerSessionMapping->handlerLink);
}

/*======================================================================

 FUNCTION        taf_L2tp::FindTunnelAsyncHandler

 DESCRIPTION     Find handler function by type.

 DEPENDENCIES    The initialization of TunnelHandlerMappingList.

 PARAMETERS      [IN] type: The command type.

 RETURN VALUE    L2tpHandlerMapping_t*   The pointer.

======================================================================*/
TunnelHandlerMapping_t* taf_L2tp::FindTunnelAsyncHandler(taf_L2tpCmdType_t type)
{
    TunnelHandlerMapping_t *handlerSessionInfo;
    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&TunnelHandlerMappingList);

    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, TunnelHandlerMapping_t, handlerLink);

        if (handlerSessionInfo->asyncHandler != NULL && handlerSessionInfo->type == type)
        {
            LE_DEBUG("Found async handler %p ", handlerSessionInfo->asyncHandler);
            return handlerSessionInfo;
        }
        handlerLinkPtr = le_dls_PeekNext(&TunnelHandlerMappingList, handlerLinkPtr);
    }

    return NULL;
}

/*======================================================================

 FUNCTION        taf_L2tp::DeleteTunnelHandlerInfo

 DESCRIPTION     Delete the handler in mapping list.

 DEPENDENCIES    The initialization of TunnelHandlerMappingList.

 PARAMETERS      [IN] asyncHandler: The handler function.

 RETURN VALUE    None

======================================================================*/
void taf_L2tp::DeleteTunnelHandlerInfo(taf_net_AsyncTunnelHandlerFunc_t asyncHandler)
{
    TunnelHandlerMapping_t *handlerSessionInfo;

    TAF_ERROR_IF_RET_NIL(asyncHandler == nullptr, "Null ptr");

    le_dls_Link_t *handlerLinkPtr = le_dls_Peek(&TunnelHandlerMappingList);
    while (handlerLinkPtr)
    {
        handlerSessionInfo = CONTAINER_OF(handlerLinkPtr, TunnelHandlerMapping_t, handlerLink);
        handlerLinkPtr = le_dls_PeekNext(&TunnelHandlerMappingList, handlerLinkPtr);
        if (handlerSessionInfo->asyncHandler == asyncHandler)
        {
            le_dls_Remove(&TunnelHandlerMappingList, &handlerSessionInfo->handlerLink);

            le_mem_Release(handlerSessionInfo);
            break;
        }
    }
}

void taf_L2tp::ClientCloseSessionHandler(le_msg_SessionRef_t sessionRef, void  *contextPtr)
{
    void* tunnelRef;
    le_dls_Link_t* linkPtr = NULL;
    taf_Tunnel_t* tunnelPtr = NULL;
    auto &tafTunnel = taf_L2tp::GetInstance();
    auto &tafL2tp = taf_L2tp::GetInstance();
    L2tpHandlerMapping_t *l2tpHandlerSessionInfo;
    TunnelHandlerMapping_t *tunnelHandlerSessionInfo;

    TAF_ERROR_IF_RET_NIL(sessionRef == NULL, "sessionRef is invalid");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(tafTunnel.tunnelRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        tunnelPtr = (taf_Tunnel_t*)le_ref_GetValue(iterRef);

        if (tunnelPtr != NULL && tunnelPtr->sessionRef == sessionRef)
        {
            tunnelRef = (void*)le_ref_GetSafeRef(iterRef);
            LE_ASSERT(tunnelRef != NULL);

            // Remove sessions in the tunnel
            linkPtr = le_dls_Peek(&(tunnelPtr->l2tpSessionList));
            while (linkPtr)
            {
                taf_L2tpSession_t* sessionPtr = CONTAINER_OF(linkPtr, taf_L2tpSession_t, link);
                linkPtr = le_dls_PeekNext(&(tunnelPtr->l2tpSessionList), linkPtr);
                le_dls_Remove(&(tunnelPtr->l2tpSessionList), &(sessionPtr->link));
                le_mem_Release(sessionPtr);
            }

            le_ref_DeleteRef(tafTunnel.tunnelRefMap, tunnelRef);

            le_mem_Release(tunnelPtr);
        }
    }

    le_dls_Link_t *l2tpHandlerLinkPtr = le_dls_Peek(&tafL2tp.L2tpHandlerMappingList);

    while (l2tpHandlerLinkPtr)
    {
        l2tpHandlerSessionInfo = CONTAINER_OF(l2tpHandlerLinkPtr, L2tpHandlerMapping_t,
                                              handlerLink);
        l2tpHandlerLinkPtr = le_dls_PeekNext(&tafL2tp.L2tpHandlerMappingList, l2tpHandlerLinkPtr);
        if (l2tpHandlerSessionInfo->sessionRef == sessionRef)
        {
            le_dls_Remove(&tafL2tp.L2tpHandlerMappingList, &l2tpHandlerSessionInfo->handlerLink);

            le_mem_Release(l2tpHandlerSessionInfo);
        }
    }

    le_dls_Link_t *tunnelHandlerLinkPtr = le_dls_Peek(&tafL2tp.TunnelHandlerMappingList);

    while (tunnelHandlerLinkPtr)
    {
        tunnelHandlerSessionInfo = CONTAINER_OF(tunnelHandlerLinkPtr, TunnelHandlerMapping_t,
                                                handlerLink);
        tunnelHandlerLinkPtr = le_dls_PeekNext(&tafL2tp.TunnelHandlerMappingList,
                                               tunnelHandlerLinkPtr);
        if (tunnelHandlerSessionInfo->sessionRef == sessionRef)
        {
            le_dls_Remove(&tafL2tp.TunnelHandlerMappingList,
                          &tunnelHandlerSessionInfo->handlerLink);

            le_mem_Release(tunnelHandlerSessionInfo);
        }
    }

}


