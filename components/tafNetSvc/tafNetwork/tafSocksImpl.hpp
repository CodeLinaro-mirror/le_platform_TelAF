/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include "tafSvcIF.hpp"
#include "tafCommonPa.h"


/**
* @brief The emum of async socks command type.
*/
typedef enum
{
    ASYNC_DISABLE_SOCKS  = 0,
    ASYNC_ENABLE_SOCKS = 1
} taf_SocksCmdType_t;

/*
* @brief The struct of async socks command request.
*/
typedef struct
{
    taf_SocksCmdType_t cmdType;
    void* contextPtr;
    le_msg_SessionRef_t sessionRef;
    taf_net_AsyncSocksHandlerFunc_t handlerFuncPtr;
} taf_SocksCmdReq_t;

typedef enum
{
    EVT_ENABLE_SOCKS_ASYNC_CALLBACK,
    EVT_DISABLE_SOCKS_ASYNC_CALLBACK
}taf_SocksEvtType_t;

typedef struct
{
    taf_SocksEvtType_t                      event;
    le_result_t                errorCode;
} taf_SocksEventType_t;

typedef struct
{
    taf_net_AsyncSocksHandlerFunc_t asyncHandler;  ///< async handler
    void *contextPtr;
    le_msg_SessionRef_t sessionRef;
    taf_SocksCmdType_t type;
    le_dls_Link_t handlerLink;                     ///< double link list's link element
}SocksHandlerMapping_t;


namespace tafsvc {

    /*
     * @brief A callback class must be provided when invoke telsdk API.
     */
    class tafSocksCallback
    {
        public:
            static void enableSocksAsyncResponse(taf_pa_result_t error,void *contextPtr);
            static void disableSocksAsyncResponse(taf_pa_result_t error,void *contextPtr);
            tafSocksCallback(){};
            ~tafSocksCallback(){};
    };

    /*
     * @brief taf_Socks class defined as a middleware between interfaces and implementation.
     */
    class taf_Socks :public ITafSvc
    {
        public:
            taf_Socks() {};
            ~taf_Socks() {};

            void Init(void);

            static taf_Socks &GetInstance();

            static void* SocksCmdThread(void* contextPtr);
            static void SocksProcCmdHandler(void* cmdReqPtr);

            static void* SocksEvtThread(void* contextPtr);
            static void SocksEvtHandler(void* cmdReqPtr);

            le_result_t EnableSocks(taf_SocksCmdType_t type);

            le_result_t EnableSocksCmdSync();
            le_result_t DisableSocksCmdSync();
            void EnableSocksCmdAsync(taf_net_AsyncSocksHandlerFunc_t handlerPtr,
                                             void* contextPtr, le_msg_SessionRef_t sessionRef);
            void DisableSocksCmdAsync(taf_net_AsyncSocksHandlerFunc_t handlerPtr,
                                              void* contextPtr, le_msg_SessionRef_t sessionRef);

            static le_event_Id_t socksCmdId;
            static le_event_Id_t socksEvId;

            le_dls_List_t SocksHandlerMappingList = LE_DLS_LIST_INIT;
            le_mem_PoolRef_t HandlerMappingPool = NULL;

            SocksHandlerMapping_t* FindAsyncHandler(taf_SocksCmdType_t type);
            void DeleteSessionHandlersInfo(le_msg_SessionRef_t sessionRef);
            void AddHandlerSessionMapping( taf_net_AsyncSocksHandlerFunc_t asyncHandler,
                                                     void *contextPtr,
                                                     le_msg_SessionRef_t sessionRef,
                                                     taf_SocksCmdType_t type);
            void DeleteHandlerInfo(taf_net_AsyncSocksHandlerFunc_t asyncHandler);
            int GetHandlerNumberInMappingList( taf_SocksCmdType_t type);

            static void CloseEventHandler(le_msg_SessionRef_t sessionRef,void* contextPtr);
    };

}

