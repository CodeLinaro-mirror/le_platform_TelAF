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
#include <vector>
#include <iostream>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/SocksManager.hpp>
#include "tafSvcIF.hpp"

using namespace telux::data;
using namespace telux::common;

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
    telux::common::ErrorCode                errorCode;
} taf_SocksEventType_t;

typedef struct
{
    taf_net_AsyncSocksHandlerFunc_t asyncHandler;  ///< async handler
    void *contextPtr;
    le_msg_SessionRef_t sessionRef;
    taf_SocksCmdType_t type;
    le_dls_Link_t handlerLink;                     ///< double link list's link element
}SocksHandlerMapping_t;


namespace telux{
namespace tafsvc {

    /*
     * @brief A callback class must be provided when invoke telsdk API.
     */
    class tafSocksCallback
    {
        public:
            static void enableSocksResponse(telux::common::ErrorCode error);
            static void disableSocksResponse(telux::common::ErrorCode error);
            static void enableSocksAsyncResponse(telux::common::ErrorCode error);
            static void disableSocksAsyncResponse(telux::common::ErrorCode error);
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

            void onInitComplete(telux::common::ServiceStatus status);

            le_result_t EnableSocks(taf_SocksCmdType_t type);

            le_result_t EnableSocksCmdSync();
            le_result_t DisableSocksCmdSync();
            void EnableSocksCmdAsync(taf_net_AsyncSocksHandlerFunc_t handlerPtr,
                                             void* contextPtr, le_msg_SessionRef_t sessionRef);
            void DisableSocksCmdAsync(taf_net_AsyncSocksHandlerFunc_t handlerPtr,
                                              void* contextPtr, le_msg_SessionRef_t sessionRef);

            std::promise<le_result_t> EnableSocksSyncPromise;
            std::promise<le_result_t> DisableSocksSyncPromise;

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
        private:
            std::shared_ptr<telux::data::net::ISocksManager> socksManager = nullptr;
#ifdef TARGET_SA515M //only sa515 using
            bool IsSubSystemStatusUpdated=false;
            std::mutex mMutex;
            std::condition_variable conVar;
#endif
    };

}
}

