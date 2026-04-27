/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_FLASH_HPP
#define TAF_FLASH_HPP

#include "legato.h"
#include "interfaces.h"

#include "tafFlashPa.hpp"

#define PAGE_ERASED -255

#define MAX_MTD_NUM 64
#define MAX_UBI_NUM 64

typedef struct
{
    char name[TAF_FLASH_VOLUME_NAME_MAX_BYTES];
    taf_pa_flash_OpenModeBitMask_t mode;
    taf_pa_flash_UbiVolumeRef_t ubiRef;
} taf_flash_Ubi_t;

class taf_FlashAccess
{
    public:
        taf_FlashAccess() = default;
        ~taf_FlashAccess() = default;

        /**
         * Returns the singleton instance of taf_FlashAccess.
         *
         * @return
         *  - Reference to the singleton taf_FlashAccess instance.
         */
        static taf_FlashAccess &GetInstance
        (
            void
        );

        /**
         * Initializes the flash access reference maps and memory pools.
         *
         * @note This helper has no return value.
         */
        void Init
        (
            void
        );

        le_ref_MapRef_t mtdMap;
        le_ref_MapRef_t ubiMap;

        le_mem_PoolRef_t mtdPool;
        le_mem_PoolRef_t ubiPool;
};

#endif