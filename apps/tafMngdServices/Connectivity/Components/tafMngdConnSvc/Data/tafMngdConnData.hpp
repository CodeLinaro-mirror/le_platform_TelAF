/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#pragma once
#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "tafMngdConnAdmin.hpp"
#include <map>
#include <utility>

namespace tafsvc {

    class tafMngdConnData: public ITafSvc
    {
        public:
            tafMngdConnData() {};
            ~tafMngdConnData() {};

            void Init(void);
            void Deinit(void);
            static tafMngdConnData &GetInstance();
            le_result_t SetInterface(uint8_t phoneId, uint32_t profileId, const char* namePtr);
            le_result_t Startdata(uint8_t phoneId, uint32_t profileId);
            le_result_t Startdata(uint8_t phoneId, uint32_t profileId, uint8_t timeout);
            le_result_t Stopdata(uint8_t phoneId, uint32_t profileId);
            le_result_t Stopdata(uint8_t phoneId, uint32_t profileId, uint8_t timeout);
            le_result_t GetConnectionInfo(mcs_DataCtx_t* dataCtxPtr);
            le_result_t GetAllConnectionInfo(profileInfo_t *profileNumberList, int listSize);
            void RegisterEvents();
            void UnregisterEvents();

        private:
            // Promise to sync async commands
            static std::promise<le_result_t> AsyncAPIPromise;
            // Global variables for Asynchronous thread
            static le_thread_Ref_t dataThreadRef;
            static le_sem_Ref_t semRef;


            static void SessionStateChangeHandler (taf_dcs_ProfileRef_t profileRef,
                                                   taf_dcs_ConState_t state,
                                                   const taf_dcs_StateInfo_t *stateInfoPtr,
                                                   void *contextPtr);

            static void StartDataAsync(void *contextPtr);
            static void StopDataAsync(void *contextPtr);
            static void StartSessionAsyncHandlerFunc(taf_dcs_ProfileRef_t profileRef,
                                              le_result_t result,
                                              void* contextPtr);
            static void StopSessionAsyncHandlerFunc(taf_dcs_ProfileRef_t profileRef,
            le_result_t result,
            void* contextPtr);
            static void* DataThreadHandler(void *contextPtr);
            static void DataThreadDestructor(void *contextPtr);

        private:
            // Map to store session state handlers: key is (phoneId,profileId), value is handler ref
            std::map<std::pair<int, uint32_t>, taf_dcs_SessionStateHandlerRef_t>
                                                                           sessionStateHandlerMap_;

    };

}
