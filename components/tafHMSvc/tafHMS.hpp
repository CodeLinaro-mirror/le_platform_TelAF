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
            le_result_t GetRamMemInfo(uint32_t* ramTotalMemPtr, uint32_t* ramUsedMemPtr, uint32_t* ramFreeMemPtr);
    };
  }
}