/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/* @file       tafHMS.hpp
* @brief      Interface for health monitor Service object. The functions
*             in this file are impletmented internally.
*/

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"


// Max number of Handler
#define MAX_HMS_HANLDER 32

#define MAX_CORES 8
#define MAX_FIELDS 10

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold the HmsInfo
*/
//-------------------------------------------------------------------------------------------------
typedef struct
{
    double cpuLoadInfo;
    uint32_t ramMemfreeInfo;
} tafHmsInfo_t;

//-------------------------------------------------------------------------------------------------
/**
* Structure to hold CPU load information for each core
*/
//-------------------------------------------------------------------------------------------------
struct CPUCore {
    uint32_t user;
    uint32_t nice;
    uint32_t system;
    uint32_t idle;
    uint32_t iowait;
    uint32_t irq;
    uint32_t softirq;
    uint32_t steal;
    uint32_t guest;
};


namespace telux {
namespace tafsvc {
class taf_Hms: public ITafSvc
    {
        public:
            taf_Hms() {};
            ~taf_Hms() {};
            static taf_Hms& GetInstance();
            void Init();
            static void OnClientDisconnection(le_msg_SessionRef_t sessionRef, void *contextPtr);
            le_result_t GetCpuLoad(double* cpuCurrentLoadPtr);
            uint32_t GetCpuCoreNum(void);
            le_result_t GetIndvCoreUsage(uint32_t coreID, double* cpuUsagePtr);
            le_result_t GetRamMemInfo(uint32_t* ramTotalMemPtr, uint32_t* ramUsedMemPtr,
                uint32_t* ramFreeMemPtr);
    };
  }
}
