/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAFMRC_HPP
#define TAFMRC_HPP

#include "legato.h"
#include "interfaces.h"

#include "taf_pa_mrc.hpp"

#define DISABLE_INDICATION 0
#define ENABLE_INDICATION 1

#define METRICS_MAX_NUM 1
#define RESP_TIMEOUT 180

typedef struct
{
    uint32_t maxCount;
    uint32_t minCount;
    uint32_t avgCount;
    uint32_t sdValue;
    uint32_t badBlockCount;
} Metrics_t;

typedef struct
{
    le_sem_Ref_t abSync;
} Semaphore_t;

typedef struct
{
    le_mem_PoolRef_t metrics;
} Pool_t;

typedef struct
{
    le_ref_MapRef_t metrics;
} Map_t;

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

                static taf_pa_mrc_Status_t Status
                (
                    taf_mrc_OtaOperationStatus_t status
                );

                static taf_pa_mrc_Status_t Status
                (
                    taf_mrc_SyncStatus_t status
                );
        };
};


class MRCFactory
{
    public:
        static MRCFactory& GetInstance
        (
            void
        );

        Semaphore_t semaphores;
        Pool_t pools;
        Map_t maps;
};

#endif
