/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_FLASH_HPP
#define TAF_FLASH_HPP

#include <string>
#include <map>

#include "legato.h"

#include "tafSvcIF.hpp"
#include "tafFlashAccess.hpp"

    namespace tafsvc
    {
        class taf_FlashAccess : public ITafSvc
        {
            public:
                taf_FlashAccess() {};
                ~taf_FlashAccess() {};

                static taf_FlashAccess &GetInstance();

                void Init();
                taf_lib_flash_PartitionList_t partitionList;
                le_ref_MapRef_t partitionRefMap;

                std::map<std::string, uint32_t> partitionMap;
        };
    }

#endif
