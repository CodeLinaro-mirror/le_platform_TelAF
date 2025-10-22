/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAFMRC_HPP
#define TAFMRC_HPP

#include "legato.h"
#include "interfaces.h"

#include <future>
#include <condition_variable>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/PlatformFactory.hpp>
#include <telux/platform/FsDefines.hpp>
#include <telux/platform/FsManager.hpp>

#include "taf_pa_mrc.hpp"

#include "tafSvcIF.hpp"

#define DISABLE_INDICATION 0
#define ENABLE_INDICATION 1

#define TAF_MRC_MSG_RESP_TIMEOUT 180

#define TAF_MRC_METRICS_MAX_NUM 1

typedef enum
{
    TAF_MRC_OTA_MSG_TYPE_START,
    TAF_MRC_OTA_MSG_TYPE_RESUME,
    TAF_MRC_OTA_MSG_TYPE_END_SUCCESS,
    TAF_MRC_OTA_MSG_TYPE_END_FAILURE,
    TAF_MRC_OTA_MSG_TYPE_ABSYNC
} taf_MrcOtaMsgType_t;

typedef struct
{
    uint32_t maxCount;
    uint32_t minCount;
    uint32_t avgCount;
    uint32_t sdValue;
    uint32_t badBlockCount;
} taf_MrcEfsMetrics_t;

class Utility
{
    public:
        class Convert
        {
            public:
                static le_result_t Result
                (
                    pa_result_t result
                );
        };
};

namespace tafsvc {
    class taf_MrcOtaOperationsListener : public telux::platform::IFsListener {
    public:
        virtual void onServiceStatusChange(telux::common::ServiceStatus serviceStatus) override;
    };

    class taf_Mrc : public ITafSvc {
    public:
        taf_Mrc() {};
        ~taf_Mrc() {};

        static taf_Mrc &GetInstance();
        void Init(void);

        le_result_t SendOtaMsg(taf_MrcOtaMsgType_t type);

        std::shared_ptr<telux::platform::IFsManager> fsManager;
        le_sem_Ref_t syncSem;
        bool paReady = false;
        le_mem_PoolRef_t metricsPool;
        le_ref_MapRef_t metricsRefMap;
    private:
        std::shared_ptr<taf_MrcOtaOperationsListener> otaOperationsListener;
    };
}

#endif
