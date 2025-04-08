/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_VER_INFO_HPP
#define TAF_VER_INFO_HPP

#include "legato.h"
#include "interfaces.h"

#include "tafSvcIF.hpp"
#include "tafHalLib.hpp"

#include "tafPiVersion.h"
#include "tafPiHash.h"

//--------------------------------------------------------------------------------------------------
/**
 * Boot slot bytes.
 */
//--------------------------------------------------------------------------------------------------
#define BOOT_SLOT_BYTES 4

//--------------------------------------------------------------------------------------------------
/**
 * Version file path.
 */
//--------------------------------------------------------------------------------------------------
#define KERNEL_VERSION_FILE "/proc/version"
#define FIRMWARE_VERSION_FILE "/firmware/image/Ver_Info.txt"
#define ROOTFS_VERSION_FILE "/etc/version"
#define TELAF_VERSION_FILE "/legato/systems/current/version"
#define LXC_VERSION_FILE "/lxcrootfs/etc/version"

//--------------------------------------------------------------------------------------------------
/**
 * Hash file path.
 */
//--------------------------------------------------------------------------------------------------
#define LXC_HASH_FILE "/lxcrootfs/etc/hash"

    namespace tafsvc
    {
        class taf_verInfo : public ITafSvc
        {
            public:
                taf_verInfo() = default;
                ~taf_verInfo() = default;

                static taf_verInfo& GetInstance();
                void Init();
                le_result_t GetRevisions(taf_pi_version_Comp_t component, char* versionPtr,
                    size_t versionSize);
                le_result_t GetHash(taf_pi_hash_Comp_t component, taf_pi_hash_Bank_t bank,
                    uint8_t* hashPtr, size_t* hashSizePtr);
                le_result_t GetBootBank(taf_verInfo_Bank_t* bank);
                le_result_t StringToHash(char* strPtr, uint8_t* hashPtr, size_t* hashSizePtr);

                version_Inf_t* versionInfPtr;
                hash_Inf_t* hashInfPtr;
		};
    }

#endif
