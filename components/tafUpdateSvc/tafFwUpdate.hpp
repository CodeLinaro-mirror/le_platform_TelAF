/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAFFWUPDATE_HPP
#define TAFFWUPDATE_HPP

#include <vector>
#include "legato.h"
#include "interfaces.h"

#include "tafSvcIF.hpp"

#include "tafFlashPa.hpp"

#define TAF_FWUPDATE_CMD_LEN 256
#define TAF_FWUPDATE_CMD_RESULT_LEN 32

#define TAF_FIRMWARE_VERSION_LINE_NUM 16
#define TAF_TELAF_VERSION_LEN 21

#define TAF_FWUPDATE_DIRNAME_LEN 128
#define TAF_FWUPDATE_PARTITION_SUFFIX_LEN 2

#define TAF_FWUPDATE_BYPASS_CHECK_TAG "NULL"

#define TAF_FWUPDATE_RECOVERY_LOG_FILE "/tmp/recovery.log"

#define TAF_TELAF_VERSION_FILE "/legato/systems/current/version"
#define TAF_ROOTFS_VERSION_FILE "/etc/version"
#define TAF_FIRMWARE_VERSION_FILE "/firmware/image/Ver_Info.txt"
#define TAF_FWUPDATE_CFG_FILE "tafUpdate.json"

#define TAF_FWUPDATE_INSTALL_CONTEXT "install_context"
#define TAF_FWUPDATE_INSTALL_IMGAE_NODE "install_context/image/%s"
#define TAF_FWUPDATE_POST_SCRIPT_PATH_LEN 128

#define TAF_FWUPDATE_ACTIVATE_CONTEXT "activate_context"
#define TAF_FWUPDATE_ACTIVATE_ITEM_NODE "activate_context/item/%s"

#define TAF_FWUPDATE_SYNC_CONTEXT "sync_context"
#define TAF_FWUPDATE_SYNC_IMGAE_NODE "sync_context/image/%s"

#define TAF_FWUPDATE_FOTA_STATE "/data/le_fs/fotaState"
#define TAF_FWUPDATE_LOCAL_PACAKAGE_PATH "/data/images/firmware"

//--------------------------------------------------------------------------------------------------
/**
 * Flash page size.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FWUPDATE_FLASH_PAGE_SIZE 0x1000

//--------------------------------------------------------------------------------------------------
/**
 * MTD page is erased
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FWUPDATE_FLASH_PAGE_ERASED -255

//--------------------------------------------------------------------------------------------------
/**
 * MTD erased block size.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FWUPDATE_FLASH_MTD_EB_SIZE 0x40000

//--------------------------------------------------------------------------------------------------
/**
 * Firmware update event.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_FWUPDATE_EV_START_INSTALL,
    TAF_FWUPDATE_EV_PAUSE_INSTALL,
    TAF_FWUPDATE_EV_RESUME_INSTALL,
    TAF_FWUPDATE_EV_INSTALL_POST_CHECK,
    TAF_FWUPDATE_EV_START_ACTIVATION,
    TAF_FWUPDATE_EV_RESUME_ACTIVATION,
    TAF_FWUPDATE_EV_SYNC,
    TAF_FWUPDATE_EV_START_SYNC,
    TAF_FWUPDATE_EV_PAUSE_SYNC,
    TAF_FWUPDATE_EV_RESUME_SYNC,
    TAF_FWUPDATE_EV_ROLLBACK
} taf_FwUpdateEvent_t;

// Firmware update request
typedef struct {
    taf_FwUpdateEvent_t event;
    char filePath[TAF_UPDATE_FILE_PATH_LEN];
} taf_FwUpdateReq_t;

//--------------------------------------------------------------------------------------------------
/**
 * Partition structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    const char* partition;
    const bool hasSuffix;
    const char* dataPath;
    const char* patchPath;
} taf_FwUpdateParition_t;

//--------------------------------------------------------------------------------------------------
/**
 * Partition structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char name[TAF_FLASH_PARTITION_NAME_MAX_BYTES];
    taf_update_Bank_t bank;
    bool isUbi;
} taf_FlashPartition_t;

namespace tafsvc {
    class taf_FwUpdate : public ITafSvc {
    public:
        taf_FwUpdate() {};
        ~taf_FwUpdate() {};

        /**
         * Returns the singleton instance of taf_FwUpdate.
         *
         * @return
         *  - Reference to the singleton taf_FwUpdate instance.
         */
        static taf_FwUpdate &GetInstance
        (
            void
        );

        /**
         * Sends a shell command and checks its exit status.
         *
         * @return
         *  - LE_OK -- The command exited successfully.
         *  - LE_FAULT -- popen/pclose failed or the command exited with a non-zero status.
         */
        le_result_t SendPipeCmd
        (
            const char* cmd, ///< [IN] Shell command to execute.
            const char* mod  ///< [IN] Mode string for the command.
        );

        /**
         * Gets the current firmware update state.
         *
         * @return
         *  - The current firmware update state.
         */
        taf_update_State_t GetState();

        /**
         * Sets the firmware update state.
         *
         * @note This helper has no return value.
         */
        void SetState
        (
            taf_update_State_t state ///< [IN] New state to set.
        );

        /**
         * Reports the current update status.
         *
         * @note This helper has no return value.
         */
        void ReportStatus
        (
            taf_update_State_t state,   ///< [IN] Current update state.
            uint32_t           percent, ///< [IN] Progress percentage.
            taf_update_Error_t error    ///< [IN] Error code.
        );

        /**
         * Updates the progress for the given state.
         *
         * @note This helper has no return value.
         */
        void UpdateProgress
        (
            taf_update_State_t state ///< [IN] Current update state.
        );

        /**
         * Sets the error code.
         *
         * @note This helper has no return value.
         */
        void SetErrorCode
        (
            int err ///< [IN] Error code to set.
        );

        /**
         * Retrieves the rootfs version string.
         *
         * @note This helper has no return value.
         */
        void GetRootfsVersion
        (
            char* version ///< [OUT] Buffer to receive the rootfs version string.
        );

        /**
         * Retrieves the telaf version string.
         *
         * @note This helper has no return value.
         */
        void GetTelafVersion
        (
            char* version ///< [OUT] Buffer to receive the telaf version string.
        );

        /**
         * Retrieves the current firmware version string.
         *
         * @return
         *  - LE_OK -- The firmware version string was parsed and copied successfully.
         *  - LE_FAULT -- The version file could not be opened, or the expected firmware version
         *                 tokens could not be parsed from its contents.
         */
        le_result_t GetFirmwareVersion
        (
            char* version ///< [OUT] Buffer to receive the firmware version string.
        );

        /**
         * Gets the unpack directory path.
         *
         * @return
         *  - true  -- The unpack directory was found successfully.
         *  - false -- The unpack directory could not be determined.
         */
        bool GetUnpackDir
        (
            char*  unpackDir, ///< [OUT] Buffer to receive the unpack directory path.
            size_t dirLen     ///< [IN]  Size of the unpackDir buffer.
        );

        /**
         * Validates a manifest against the current firmware version state.
         *
         * @return
         *  - LE_OK -- Pre-check completed successfully or was bypassed by the special bypass tag.
         *  - LE_FAULT -- The manifest could not be opened, current firmware information could not be
         *                 obtained, manifest version lines could not be parsed, or downgrade
         *                 detection failed.
         */
        le_result_t InstallPreCheck
        (
            const char* manifest ///< [IN] Path to the manifest file.
        );

        /**
         * Cleans up the context for the given update state.
         *
         * @note This helper has no return value.
         */
        void CleanupContext
        (
            taf_update_State_t state ///< [IN] Update state whose context to clean up.
        );

        /**
         * Sets the pause action for the given update state.
         *
         * @note This helper has no return value.
         */
        void SetPauseAction
        (
            taf_update_State_t state,  ///< [IN] Update state to configure.
            bool               paused  ///< [IN] True to pause, false to resume.
        );

        /**
         * Gets the pause action for the given update state.
         *
         * @return
         *  - true  -- The update is paused.
         *  - false -- The update is not paused.
         */
        bool GetPauseAction
        (
            taf_update_State_t state ///< [IN] Update state to query.
        );

        /**
         * Sets the cancel action for the given update state.
         *
         * @note This helper has no return value.
         */
        void SetCancelAction
        (
            taf_update_State_t state,  ///< [IN] Update state to configure.
            bool               cancel  ///< [IN] True to cancel, false to clear.
        );

        /**
         * Gets the cancel action for the given update state.
         *
         * @return
         *  - true  -- The update is cancelled.
         *  - false -- The update is not cancelled.
         */
        bool GetCancelAction
        (
            taf_update_State_t state ///< [IN] Update state to query.
        );

        /**
         * Sets the page number for the given update state.
         *
         * @note This helper has no return value.
         */
        void SetPageNumber
        (
            taf_update_State_t state,   ///< [IN] Update state to configure.
            bool               isTotal, ///< [IN] True for total pages, false for current.
            uint32_t           number   ///< [IN] Page number to set.
        );

        /**
         * Gets the page number for the given update state.
         *
         * @return
         *  - The page number for the given state.
         */
        uint32_t GetPageNumber
        (
            taf_update_State_t state,  ///< [IN] Update state to query.
            bool               isTotal ///< [IN] True for total pages, false for current.
        );

        /**
         * Checks whether a string ends with a partition suffix.
         *
         * @return
         *  - true  -- The string ends with a partition suffix.
         *  - false -- The string does not end with a partition suffix.
         */
        bool HasSuffix
        (
            const char* str ///< [IN] String to check for a partition suffix.
        );

        /**
         * Stores the package data path.
         *
         * @note This helper has no return value.
         */
        void SetPackageDataPath
        (
            const char* dataPath ///< [IN] Package data path to store.
        );

        /**
         * Retrieves the package data path.
         *
         * @note This helper has no return value.
         */
        void GetPackageDataPath
        (
            char*  dataPath, ///< [OUT] Buffer to receive the package data path.
            size_t pathLen   ///< [IN]  Size of the dataPath buffer.
        );

        /**
         * Associates a data path with the given image.
         *
         * @note This helper has no return value.
         */
        void SetImageDataPath
        (
            const char* image,    ///< [IN] Image name.
            const char* dataPath  ///< [IN] Data path to associate with the image.
        );

        /**
         * Retrieves the data path for the given image.
         *
         * @note This helper has no return value.
         */
        void GetImageDataPath
        (
            const char* image,    ///< [IN]  Image name.
            char*       dataPath, ///< [OUT] Buffer to receive the image data path.
            size_t      pathLen   ///< [IN]  Size of the dataPath buffer.
        );

        /**
         * Sets the update status for the given image.
         *
         * @note This helper has no return value.
         */
        void SetImageStatus
        (
            taf_update_State_t state,   ///< [IN] Update state to configure.
            const char*        image,   ///< [IN] Image name.
            bool               updated  ///< [IN] True if the image has been updated.
        );

        /**
         * Gets the page number for the given image.
         *
         * @return
         *  - The page number for the given image.
         */
        uint32_t GetImagePageNumber
        (
            taf_update_State_t state, ///< [IN] Update state to query.
            const char*        image  ///< [IN] Image name.
        );

        /**
         * Sets the page number for the given image.
         *
         * @note This helper has no return value.
         */
        void SetImagePageNumber
        (
            taf_update_State_t state,   ///< [IN] Update state to configure.
            const char*        image,   ///< [IN] Image name.
            uint32_t           pageNum  ///< [IN] Page number to set.
        );

        /**
         * Gets the next image to be updated.
         *
         * @return
         *  - true  -- An image was found for update.
         *  - false -- No more images to update.
         */
        bool GetImageForUpdate
        (
            taf_update_State_t state,   ///< [IN]  Update state to query.
            char*              name,    ///< [OUT] Buffer to receive the image name.
            size_t             nameLen  ///< [IN]  Size of the name buffer.
        );

        /**
         * Checks whether a patch file exists for the given image.
         *
         * @return
         *  - true  -- The patch file exists.
         *  - false -- The patch file does not exist.
         */
        bool IsPatchExist
        (
            const char* filePath,  ///< [IN] Path to the image file.
            const char* patchPath  ///< [IN] Path to the patch file.
        );

        /**
         * Checks whether the given package is a delta update.
         *
         * @return
         *  - true  -- The package is a delta update.
         *  - false -- The package is a full update.
         */
        bool IsDeltaUpdate
        (
            const char* filePath ///< [IN] Path to the update package.
        );

        /**
         * Unpacks an image archive to the destination path.
         *
         * @return
         *  - true  -- The image was unpacked successfully.
         *  - false -- The image could not be unpacked.
         */
        bool UnpackImage
        (
            const char* filePath,  ///< [IN]  Path to the image archive.
            const char* imagePath, ///< [IN]  Destination path for unpacked image.
            uint32_t*   pageNum    ///< [OUT] Number of pages unpacked.
        );

        /**
         * Synchronizes all partitions.
         *
         * @note This helper has no return value.
         */
        void SyncPartition
        (
            void
        );

        /**
         * Applies the pending image update.
         *
         * @note This helper has no return value.
         */
        void UpdateImage
        (
            void
        );

        /**
         * Initializes the partition table by scanning flash devices and UBI volumes.
         *
         * @return
         *  - LE_OK -- The partition list was initialized successfully or had already been initialized.
         *  - LE_FAULT -- Flash access initialization failed, /proc/mtd could not be opened, or
         *                 partition discovery could not proceed.
         */
        le_result_t InitPartitionList
        (
            void
        );

        /**
         * Starts the firmware installation process.
         *
         * @note This helper has no return value.
         */
        void StartInstall
        (
            const char* filePath ///< [IN] Path to the firmware package to install.
        );

        /**
         * Starts the partition synchronization process.
         *
         * @note This helper has no return value.
         */
        void StartSync
        (
            void
        );

        /**
         * Flashes the firmware image to the target partition.
         *
         * @note This helper has no return value.
         */
        void InstallFirmware
        (
            const char* filePath ///< [IN] Path to the firmware image to flash.
        );

        /**
         * Retrieves the post-processing script path for the given state.
         *
         * @return
         *  - LE_OK    -- The post-processing script path was retrieved successfully.
         *  - LE_FAULT -- The script path could not be obtained.
         */
        le_result_t GetPostScript
        (
            taf_update_State_t state,      ///< [IN]  Update state.
            char*              scriptPath, ///< [OUT] Buffer to receive the post-script path.
            size_t             pathLen     ///< [IN]  Size of the scriptPath buffer.
        );

        /**
         * Retrieves the cancel post-processing script path for the given state.
         *
         * @return
         *  - LE_OK    -- The cancel post-processing script path was retrieved successfully.
         *  - LE_FAULT -- The script path could not be obtained.
         */
        le_result_t GetCancelPostScript
        (
            taf_update_State_t state,      ///< [IN]  Update state.
            char*              scriptPath, ///< [OUT] Buffer to receive the cancel post-script path.
            size_t             pathLen     ///< [IN]  Size of the scriptPath buffer.
        );

        /**
         * Executes a post-processing hook for the given update state.
         *
         * @return
         *  - LE_OK -- Post-processing completed successfully or was skipped because no script exists.
         *  - LE_FAULT -- The script path could not be obtained or the post-processing command failed.
         */
        le_result_t PostProcess
        (
            taf_update_State_t state ///< [IN] Update state for which to run post-processing.
        );

        /**
         * Executes the cancel hook for installation, if present.
         *
         * @return
         *  - LE_OK -- Cancel-post-install processing completed successfully or was skipped because
         *             no script exists.
         *  - LE_FAULT -- The cancel-hook script path could not be obtained or the cancel command
         *                 failed.
         */
        le_result_t CancelPostInstall
        (
            void
        );

        /**
         * Calculates SHA1 hash of a file.
         *
         * @return
         *  - LE_OK -- The file hash was calculated successfully.
         *  - LE_FAULT -- The digest context could not be created or initialized, or the source file
         *                 could not be opened.
         */
        le_result_t CalFileHash
        (
            const char*   filePath, ///< [IN]  File path of the image data.
            uint32_t*     calSize,  ///< [OUT] Size of file for calculation.
            uint8_t*      hash,     ///< [OUT] Hash of a file.
            unsigned int* hashLen   ///< [OUT] Hash length.
        );

        /**
         * Calculates SHA1 hash of a partition.
         *
         * @return
         *  - LE_OK -- The partition hash was calculated successfully.
         *  - LE_FAULT -- Partition discovery failed, the target partition could not be found,
         *                 digest context creation/initialization failed, or one of the lower-layer
         *                 open/info/read/close operations failed.
         */
        le_result_t CalPartitionHash
        (
            const char*   partition, ///< [IN]  Partition name.
            uint32_t      calSize,   ///< [IN]  Size to calculate.
            uint8_t*      hash,      ///< [OUT] Hash of the partition.
            unsigned int* hashLen    ///< [OUT] Hash length.
        );

        /**
         * Verifies the hash of a partition against an image file.
         *
         * @return
         *  - true  -- The hash matches.
         *  - false -- The hash does not match.
         */
        bool VerifyHash
        (
            const char* partition, ///< [IN] Partition name.
            const char* filePath   ///< [IN] Path to the image file.
        );

        /**
         * Performs post-installation checks on the firmware package.
         *
         * @note This helper has no return value.
         */
        void InstallPostCheck
        (
            const char* filePath ///< [IN] Path to the installed firmware package.
        );

        /**
         * Erases the inactive bank.
         *
         * @return
         *  - LE_OK -- The requested inactive bank erase traversal completed successfully.
         *  - LE_FAULT -- Partition discovery failed, one of the lower-layer erase operations failed,
         *                 or the lower layer rejected an open/info/erase/close request.
         */
        le_result_t EraseBank
        (
            taf_update_Bank_t bank ///< [IN] Bank to erase.
        );

        /**
         * Performs bank synchronization for the inactive bank content.
         *
         * @return
         *  - LE_OK -- The bank synchronization traversal completed successfully.
         *  - LE_FAULT -- The active bank could not be determined, partition discovery failed, or a
         *                 lower-layer open/info/copy/close request failed while processing one of the
         *                 partitions.
         */
        le_result_t PerformBankSync
        (
            void
        );

        /**
         * Rolls back to the previously active bank.
         *
         * @return
         *  - LE_OK -- The rollback bank switch request completed successfully.
         *  - LE_FAULT -- The bank had not been switched yet, the active bank could not be
         *                 determined, or the service failed to select the previous bank as active.
         */
        le_result_t Rollback
        (
            void
        );

        /**
         * Sets the activation context for the given state and bank.
         *
         * @note This helper has no return value.
         */
        void SetActivationContext
        (
            taf_update_State_t state, ///< [IN] Activation state.
            taf_update_Bank_t  bank   ///< [IN] Bank to activate.
        );

        /**
         * Checks whether the active bank has been switched.
         *
         * @return
         *  - true  -- The bank has been switched.
         *  - false -- The bank has not been switched.
         */
        bool IsBankSwitched
        (
            void
        );

        /**
         * Retrieves the currently active bank.
         *
         * @return
         *  - LE_OK    -- The active bank was retrieved successfully.
         *  - LE_FAULT -- The active bank could not be determined.
         */
        le_result_t GetActiveBank
        (
            taf_update_Bank_t* bankPtr ///< [OUT] Pointer to receive the active bank.
        );

        /**
         * Sets the active bank.
         *
         * @return
         *  - LE_OK    -- The active bank was set successfully.
         *  - LE_FAULT -- The active bank could not be set.
         */
        le_result_t SetActiveBank
        (
            taf_update_Bank_t bank ///< [IN] Bank to set as active.
        );

        /**
         * Sets the activation paused flag.
         *
         * @note This helper has no return value.
         */
        void SetActivationPaused
        (
            bool paused ///< [IN] True to pause activation, false to resume.
        );

        /**
         * Gets the activation paused flag.
         *
         * @return
         *  - true  -- Activation is paused.
         *  - false -- Activation is not paused.
         */
        bool GetActivationPaused
        (
            void
        );

        /**
         * Gets the activation status of the given item.
         *
         * @return
         *  - true  -- The item has been activated.
         *  - false -- The item has not been activated.
         */
        bool GetActivateItemStatus
        (
            const char* item ///< [IN] Activation item name.
        );

        /**
         * Sets the activation status of the given item.
         *
         * @note This helper has no return value.
         */
        void SetActivateItemStatus
        (
            const char* item,      ///< [IN] Activation item name.
            bool        activated  ///< [IN] True if the item has been activated.
        );

        /**
         * Stores the manifest file path.
         *
         * @note This helper has no return value.
         */
        void SetManifest
        (
            const char* manifest ///< [IN] Path to the manifest file.
        );

        /**
         * Retrieves the manifest file path.
         *
         * @note This helper has no return value.
         */
        void GetManifest
        (
            char*  manifest, ///< [OUT] Buffer to receive the manifest path.
            size_t pathLen   ///< [IN]  Size of the manifest buffer.
        );

        /**
         * Retrieves the previously active bank.
         *
         * @note This helper has no return value.
         */
        void GetPreviousBank
        (
            taf_update_Bank_t* bank ///< [OUT] Pointer to receive the previous bank.
        );

        /**
         * Retrieves the previous version string for the given item.
         *
         * @note This helper has no return value.
         */
        void GetPreviousVersion
        (
            const char* item,    ///< [IN]  Activation item name.
            char*       version, ///< [OUT] Buffer to receive the previous version string.
            size_t      verSize  ///< [IN]  Size of the version buffer.
        );

        /**
         * Retrieves the current activation state.
         *
         * @note This helper has no return value.
         */
        void GetActivationState
        (
            taf_update_State_t* state ///< [OUT] Pointer to receive the activation state.
        );

        /**
         * Gets the next item to be activated.
         *
         * @return
         *  - true  -- An item was found for activation.
         *  - false -- No more items to activate.
         */
        bool GetItemForActivation
        (
            char*     item,    ///< [OUT] Buffer to receive the item name.
            size_t    itemLen, ///< [IN]  Size of the item buffer.
            uint32_t* index    ///< [OUT] Pointer to receive the item index.
        );

        /**
         * Gets the total number of activation items.
         *
         * @return
         *  - The total number of activation items.
         */
        uint32_t GetActivationItemCount();

        /**
         * Activates the next pending component.
         *
         * @note This helper has no return value.
         */
        void ActivateComponent
        (
            void
        );

        /**
         * Starts the activation process using the given manifest.
         *
         * @note This helper has no return value.
         */
        void StartActivation
        (
            const char* manifest ///< [IN] Path to the manifest file.
        );

        /**
         * Initializes the firmware update service.
         *
         * @note This helper has no return value.
         */
        void Init
        (
            void
        );

        /**
         * Handles firmware update requests in the event loop.
         *
         * @note This helper has no return value.
         */
        static void FwUpdateHandler
        (
            void* reqPtr ///< [IN] Pointer to the firmware update request.
        );

        /**
         * Firmware update thread entry point.
         *
         * @note This thread has no return value.
         */
        static void* FwUpdateThread
        (
            void* contextPtr ///< [IN] Thread context pointer.
        );

        static le_event_Id_t fwUpdateEvId;

        std::vector<taf_FlashPartition_t> partitions;
        uint32_t percent = 0;
        taf_update_Error_t error = TAF_UPDATE_NONE;
    };
}

#endif