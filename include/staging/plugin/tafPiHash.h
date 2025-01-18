/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __TAF_PI_HASH_H__
#define __TAF_PI_HASH_H__

#include "tafHalIF.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Hash plugin module name.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_HASH_MODULE_NAME "TafPiHash"

//--------------------------------------------------------------------------------------------------
/**
 * Hash type enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PI_HASH_TYPE_UNKNOWN = -1,
    TAF_PI_BUILDTIME_HASH,
    TAF_PI_RUNTIME_HASH
} taf_pi_hash_Type_t;

//--------------------------------------------------------------------------------------------------
/**
 * Hash algorithm enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PI_HASH_ALG_UNKNOWN = -1,
    TAF_PI_HASH_ALG_MD5,
    TAF_PI_HASH_ALG_SHA1,
    TAF_PI_HASH_ALG_SHA256
} taf_pi_hash_Algorithm_t;

//--------------------------------------------------------------------------------------------------
/**
 * Bank enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PI_HASH_BANK_A,
    TAF_PI_HASH_BANK_B,
    TAF_PI_HASH_NOT_DUAL_BANK
} taf_pi_hash_Bank_t;

//--------------------------------------------------------------------------------------------------
/**
 * Component enum.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_PI_HASH_COMP_BOOT,
    TAF_PI_HASH_COMP_ROOTFS,
    TAF_PI_HASH_COMP_FIRMWARE,
    TAF_PI_HASH_COMP_TELAF,
    TAF_PI_HASH_COMP_LXC
} taf_pi_hash_Comp_t;

//--------------------------------------------------------------------------------------------------
/**
 * Initialize hash plugin module.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TAF_PI_HASH_INIT)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Get hash type.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_HASH_GET_TYPE)(taf_pi_hash_Comp_t component, taf_pi_hash_Type_t* type);

//--------------------------------------------------------------------------------------------------
/**
 * Get hash algorithm.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_HASH_GET_ALG)(taf_pi_hash_Comp_t component, taf_pi_hash_Algorithm_t* alg);

//--------------------------------------------------------------------------------------------------
/**
 * Get hash of a component.
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_PI_HASH_GET_HASH)(taf_pi_hash_Comp_t component, taf_pi_hash_Bank_t bank,
    uint8_t* hashPtr, size_t* hashSizePtr);

//--------------------------------------------------------------------------------------------------
/**
 * Hash plugin information structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    // Initialization.
    TAF_PI_HASH_INIT init;

    // Get hash type.
    TAF_PI_HASH_GET_TYPE getType;

    // Get hash algorithm.
    TAF_PI_HASH_GET_ALG getAlg;

    // Get hash value.
    TAF_PI_HASH_GET_HASH getHash;

} hash_Inf_t;

//--------------------------------------------------------------------------------------------------
/**
 * Hash plugin information table structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    TAF_HAL_MGR_INF_t mgrInf; ///< Device manager information.
    hash_Inf_t hashInf;       ///< Hash plugin information.
} hash_InfoTab_t;

#endif