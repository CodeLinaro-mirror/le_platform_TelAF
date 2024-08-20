/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//--------------------------------------------------------------------------------------------------
/**
 * @page c_tafRFSLib Robust File System
 *
 * @ref tafRFSLib.h "API Reference"
 *
 * <HR>
 *
 * To access files with reliability, the developer needs to include this component header file
 * in the service or application.
 *
 * @section RFS_impl Implement file access in compliance with Robust File System (RFS) APIs
 *
 * To implement file access with reliability in TelAF application, the developer needs to get
 * this file. The header tafRFSLib.hpp provides all file access APIs, which are compatible with
 * POSIX file access interfaces, and maintain file reliability in the background. The developer
 * does not need to change the logics of file access, just call taf_rfs_Init() to initailize RFS
 * component and call the corresponding APIs with RFS APIs.
 *
 * @code

    static void ErrCallback(taf_rfs_Error_t error, const char* filePath)
    {
        LE_ERROR("error %d happened with file %s", error, filePath);
    }

    le_result_t res = taf_rfs_Init(true, ErrCallback);

    int fd = taf_rfs_Open(TEST_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    char* data = "ABCDEFGH";

    ssize_t bytesWritten = taf_rfs_Write(fd, (uint8_t*)data, sizeof(data));

    if(taf_rfs_Close(fd) != 0)
    {
        LE_ERROR("failed to close the file");
    }

 * @endcode
 *
 * <HR>
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef __TAFRFSCOMP_H__
#define __TAFRFSCOMP_H__

#include "legato.h"
#include <fcntl.h>

#ifdef __cplusplus
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------------
/**
 * RFS error code.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    RFS_ERR_UNKNOWN,
    RFS_ERR_NO_MEMORY,
    RFS_ERR_FILE_TOO_LARGE,
    RFS_ERR_BACKUP,
    RFS_ERR_GET_HASH,
    RFS_ERR_SET_HASH,
    RFS_ERR_RESTORE
} taf_rfs_Error_t;

//--------------------------------------------------------------------------------------------------
/**
 * Callback for response of file robustness functions.
 * @param
 *      error    - Error code
 *      filePathPtr - Error to the file
 *
 * @return
 */
//--------------------------------------------------------------------------------------------------
typedef void (*taf_rfs_ErrorHandler_t)
(
    taf_rfs_Error_t error,
    const char* filePathPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Initializes RFS component.
 * @param
 *      enableBackup - Enable backup mechanism or not
 *      callback     - The callback function to respond error occurrence
 *
 * @return
 *      Result for initialization
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_rfs_Init
(
    bool enableBackup,
    taf_rfs_ErrorHandler_t handlerFunc
);

//--------------------------------------------------------------------------------------------------
/**
 * Sets backup storage for RFS component.
 * @param
 *      filePathPtr - The path name of backup storage
 *
 * @return
 *      Result for the setup
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_rfs_SetBackupStorage
(
    const char *filePathPtr,
    uint32_t maxFileSize,
    uint16_t maxFileCount
);

//--------------------------------------------------------------------------------------------------
/**
 * Opens a file and returns a file descriptor.
 * @param
 *      filePathPtr    - The path name of file
 *      flags       - The file open flags
 *      mode        - The file open mode
 *
 * @return
 *      The non-negative integer on successful open, others for failed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_rfs_Open
(
    const char *filePathPtr,
    int flags,
    mode_t mode
);

//--------------------------------------------------------------------------------------------------
/**
 * Closes the opened file descriptor.
 * @param
 *      fd    - Opened file descriptor
 *
 * @return
 *      0 on successful, -1 on failed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_rfs_Close
(
    int fd
);

//--------------------------------------------------------------------------------------------------
/**
 * Reads data with specified length for an opened file descriptor.
 * @param
 *      fd       - Opened file descriptor
 *      butPtr   - The buffer to save the read data
 *      sizePtr  - The maximal length of data buffer
 *
 * @return
 *      The actual read data length
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_rfs_Read
(
    int fd,
    uint8_t* bufPtr,
    size_t* sizePtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Writes data with specified length for an opened file descriptor.
 * @param
 *      fd       - Opened file descriptor
 *      butPtr   - The buffer to be written
 *      sizePtr  - The buffer length
 *
 * @return
 *      The actual written data length
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_rfs_Write
(
    int fd,
    const uint8_t* bufPtr,
    size_t sizePtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Deletes a file.
 * @param
 *      filePathPtr    - The path name of file
 *
 * @return
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_rfs_Delete
(
    const char* filePathPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Copy a file.
 * @param
 *      sourcePath  - The source file path
 *      destPath    - The destination file path
 *
 * @return
 *      Zero on successful copy, others to indicate errno
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_rfs_Copy
(
    const char *sourcePath,
    const char *destPath
);

//--------------------------------------------------------------------------------------------------
/**
 * Rename a file.
 * @param
 *      sourcePath  - The source file path
 *      destPath    - The destination file path
 *
 * @return
 *      Zero on successful copy, others to indicate errno
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int taf_rfs_Rename
(
    const char *sourcePath,
    const char *destPath
);

#ifdef __cplusplus
}
#endif

#endif
