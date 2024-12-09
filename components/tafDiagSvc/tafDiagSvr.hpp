/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_DIAG_SVR_HPP
#define TAF_DIAG_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#include "tafEventSvr.hpp"

//-------------------------------------------------------------------------------------------------
/**
 * Enable condition status structure.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t  enableConditionID;
    bool  conditionFulfilled = false;
    le_dls_Link_t link;
}taf_DiagEnableStatus_t;

// Diag service class
namespace telux {
    namespace tafsvc {
        class taf_DiagSvr : public ITafSvc{
            public:
                taf_DiagSvr() {};
                ~taf_DiagSvr() {};
                void Init();
                static taf_DiagSvr& GetInstance();

                static void OnClientDisconnection(le_msg_SessionRef_t sessionRef,
                        void *contextPtr);

                le_result_t SetEnableCondition(uint8_t enableConditionID, bool conditionFulfilled);
                bool GetEnableConditionStatus(uint8_t enableConditionID);

            private:
                le_dls_List_t enableStatusList = LE_DLS_LIST_INIT;
                le_mem_PoolRef_t EnableMemPool;
        };
    }
}
#endif /* #ifndef TAF_DIAG_SVR_HPP */