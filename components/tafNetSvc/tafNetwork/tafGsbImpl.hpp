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
#include <future>

namespace tafsvc {

    /*
     * @brief The struct of gsb list.
    */
    typedef struct
    {
        le_sls_List_t gsbList;
        le_sls_List_t safeRefList;
        le_sls_Link_t* currPtr;
    } taf_GsbList_t;

    /*
     * @brief The struct of gsb info.
    */
    typedef struct
    {
        char ifName[TAF_NET_INTERFACE_NAME_MAX_NUM];
        taf_net_GsbIfType_t ifType;
        uint32_t bandwidth;
    } taf_GsbInfo_t;

    /*
     * @brief The struct of gsb with link.
    */
    typedef struct
    {
        taf_GsbInfo_t info;
        le_sls_Link_t link;
    } taf_Gsb_t;


    /*
    * @brief The struct of safe reference for gsb.
    */
    typedef struct
    {
        void* safeRef;
        le_sls_Link_t link;
    } taf_GsbSafeRef_t;

    /*
     * @brief A callback class must be provided when invoke teladk API.
     */
    class tafGsbCallback
    {
        public:
            tafGsbCallback(){};
            ~tafGsbCallback(){};
            static le_sem_Ref_t semaphore;
    };
    /*
     * @brief taf_Gsb class defined as a middleware between interfaces and implementation.
     */
    class taf_Gsb :public ITafSvc
    {
        public:
            taf_Gsb() {};
            ~taf_Gsb() {};

            void Init(void);
            static taf_Gsb &GetInstance();

            le_result_t AddGsb(const char* ifName,
                                 taf_net_GsbIfType_t ifType,
                                 uint32_t bandwidth);
            le_result_t RemoveGsb(    const char* ifName);
            le_result_t EnableGsb(bool enable);
            taf_net_GsbListRef_t GetGsbList();
            taf_net_GsbRef_t GetFirstGsb( taf_net_GsbListRef_t gsbListRef);
            taf_net_GsbRef_t GetNextGsb( taf_net_GsbListRef_t gsbListRef );
            le_result_t DeleteGsbList( taf_net_GsbListRef_t gsbListRef );
            le_result_t GetGsbInterfaceName ( taf_net_GsbRef_t gsbRef,
                                                       char* ifNamePtr,
                                                       size_t ifNamePtrSize);
            taf_net_GsbIfType_t GetGsbInterfaceType( taf_net_GsbRef_t gsbRef);
            int32_t GetGsbBandWidth( taf_net_GsbRef_t gsbRef );
    };

}