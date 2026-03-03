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

#include "taf_pa_flash.h"

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

        static taf_FwUpdate &GetInstance();

        le_result_t SendPipeCmd(const char* cmd, const char* mod);

        taf_update_State_t GetState();
        void SetState(taf_update_State_t state);

        void ReportStatus(taf_update_State_t state, uint32_t percent, taf_update_Error_t error);
        void UpdateProgress(taf_update_State_t state);
        void SetErrorCode(int err);

        void GetRootfsVersion(char* version);
        void GetTelafVersion(char* version);
        le_result_t GetFirmwareVersion(char* version);

        bool GetUnpackDir(char* unpackDir, size_t dirLen);
        le_result_t InstallPreCheck(const char* manifest);
        void CleanupContext(taf_update_State_t state);
        void SetPauseAction(taf_update_State_t state, bool paused);
        bool GetPauseAction(taf_update_State_t state);
        void SetCancelAction(taf_update_State_t state, bool cancel);
        bool GetCancelAction(taf_update_State_t state);
        void SetPageNumber(taf_update_State_t state, bool isTotal, uint32_t number);
        uint32_t GetPageNumber(taf_update_State_t state, bool isTotal);
        bool HasSuffix(const char *str);
        void SetPackageDataPath(const char* dataPath);
        void GetPackageDataPath(char* dataPath, size_t pathLen);
        void SetImageDataPath(const char* image, const char* dataPath);
        void GetImageDataPath(const char* image, char* dataPath, size_t pathLen);
        void SetImageStatus(taf_update_State_t state, const char* image, bool updated);
        uint32_t GetImagePageNumber(taf_update_State_t state, const char* image);
        void SetImagePageNumber(taf_update_State_t state, const char* image, uint32_t pageNum);
        bool GetImageForUpdate(taf_update_State_t state, char* name, size_t nameLen);
        bool IsPatchExist(const char* filePath, const char* patchPath);
        bool IsDeltaUpdate(const char* filePath);
        bool UnpackImage(const char* filePath, const char* imagePath, uint32_t* pageNum);
        void SyncPartition(void);
        void UpdateImage(void);
        le_result_t InitPartitionList(void);
        void StartInstall(const char* filePath);
        void StartSync(void);
        void InstallFirmware(const char* filePath);
        le_result_t GetPostScript(taf_update_State_t state, char* scriptPath, size_t pathLen);
        le_result_t GetCancelPostScript(taf_update_State_t state, char* scriptPath, size_t pathLen);
        le_result_t PostProcess(taf_update_State_t state);
        le_result_t CancelPostInstall(void);

        le_result_t CalFileHash (const char* filePath, uint32_t* calSize, uint8_t* hash,
            unsigned int* hashLen);
        le_result_t CalPartitionHash (const char* partition, uint32_t calSize, uint8_t* hash,
            unsigned int* hashLen);
        bool VerifyHash(const char* partition, const char* filePath);
        void InstallPostCheck(const char* filePath);

        le_result_t EraseBank(taf_update_Bank_t bank);
        le_result_t PerformBankSync(void);
        le_result_t Rollback(void);

        void SetActivationContext(taf_update_State_t state, taf_update_Bank_t bank);
        bool IsBankSwitched(void);
        le_result_t GetActiveBank(taf_update_Bank_t* bankPtr);
        le_result_t SetActiveBank(taf_update_Bank_t bank);
        void SetActivationPaused(bool paused);
        bool GetActivationPaused(void);
        bool GetActivateItemStatus(const char* item);
        void SetActivateItemStatus(const char* item, bool activated);
        void SetManifest(const char* manifest);
        void GetManifest(char* manifest, size_t pathLen);
        void GetPreviousBank(taf_update_Bank_t* bank);
        void GetPreviousVersion(const char* item, char* version, size_t verSize);
        void GetActivationState(taf_update_State_t* state);
        bool GetItemForActivation(char* item, size_t itemLen, uint32_t* index);
        uint32_t GetActivationItemCount();
        void ActivateComponent(void);
        void StartActivation(const char* manifest);

        void Init(void);

        static void FwUpdateHandler(void* reqPtr);
        static void* FwUpdateThread(void* contextPtr);

        static le_event_Id_t fwUpdateEvId;

        std::vector<taf_FlashPartition_t> partitions;
        uint32_t percent = 0;
        taf_update_Error_t error = TAF_UPDATE_NONE;
    };
}

#endif
