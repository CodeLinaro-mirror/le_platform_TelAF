/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafFSCrypt.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * The FS-Crypt component initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    tafsvc::taf_FSCrypt& myFSCrypt = tafsvc::taf_FSCrypt::GetInstance();
    myFSCrypt.Init();
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference of FS-Crypt storage object.
 */
//--------------------------------------------------------------------------------------------------
taf_fsc_StorageRef_t taf_fsc_GetStorageRef
(
    const char  *dirPath,
    le_result_t *result
)
{
    return tafsvc::taf_FSCrypt::GetInstance().GetStorageRef(dirPath, result);
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes an FS-Crypt key from the kernel keyring to lock the directory.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_fsc_LockStorage
(
    taf_fsc_StorageRef_t StorageRef
)
{
    return tafsvc::taf_FSCrypt::GetInstance().LockStorage(StorageRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds an FS-Crypt key to the kernel keyring to unlock the directory.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_fsc_UnlockStorage
(
    taf_fsc_StorageRef_t StorageRef
)
{
    return tafsvc::taf_FSCrypt::GetInstance().UnlockStorage(StorageRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Cleans the entire FS-Crypt directory and encryption key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_fsc_DeleteStorage
(
    taf_fsc_StorageRef_t StorageRef
)
{
    return tafsvc::taf_FSCrypt::GetInstance().DeleteStorage(StorageRef);
}
