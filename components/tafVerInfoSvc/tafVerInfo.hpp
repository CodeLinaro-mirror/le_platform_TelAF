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

    /**
     * Gets the singleton instance.
     *
     * @return
     *  - Reference to the singleton instance.
     */
    static taf_verInfo& GetInstance();

    /**
     * Initialization.
     */
    void Init();

    /**
     * Gets revisions.
     *
     * @return
     *  - LE_FAULT -- Version plug-in does not define the format callback.
     *  - LE_OK -- Revision string was generated, or a fallback revision such as
     *    0.0.0 / 0.0 / 0 was appended when major, minor, or patch retrieval
     *    failed.
     */
    le_result_t GetRevisions
    (
        taf_pi_version_Comp_t component, ///< [IN] Component.
        char* versionPtr,                ///< [OUT] Version string.
        size_t versionSize               ///< [IN] Size of the version string.
    );

    /**
     * Gets hash.
     *
     * @return
     *  - LE_FAULT -- Hash plug-in getHash callback reported failure.
     *  - LE_OK -- Hash information was obtained, or no hash callback is
     *    provided by the plug-in.
     */
    le_result_t GetHash
    (
        taf_pi_hash_Comp_t component, ///< [IN] Component.
        taf_pi_hash_Bank_t bank,      ///< [IN] Bank.
        uint8_t* hashPtr,             ///< [OUT] Hash string.
        size_t* hashSizePtr           ///< [IN] Size of the hash string.
    );

    /**
     * Gets boot bank.
     *
     * @return
     *  - LE_FAULT -- Failed to execute/read the boot-slot command, or the
     *    reported slot is not mapped to bank A or bank B.
     *  - LE_OK -- Boot bank was detected successfully.
     */
    le_result_t GetBootBank
    (
        taf_verInfo_Bank_t* bank ///< [OUT] Bank.
    );

    /**
     * Converts string to hash.
     *
     * @return
     *  - LE_OK -- Conversion completed. Parsing stops at the first non-hex
     *    character and the decoded byte count is returned through
     *    hashSizePtr.
     */
    le_result_t StringToHash
    (
        char* strPtr,       ///< [IN] Character string.
        uint8_t* hashPtr,   ///< [OUT] Hash string.
        size_t* hashSizePtr ///< [OUT] Size of the hash string.
    );

    version_Inf_t* versionInfPtr;
    hash_Inf_t* hashInfPtr;
};

} // namespace tafsvc

#endif