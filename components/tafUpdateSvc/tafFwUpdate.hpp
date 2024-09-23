/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TAFFWUPDATE_HPP
#define TAFFWUPDATE_HPP

#include <map>
#include "legato.h"
#include "interfaces.h"

#include "tafSvcIF.hpp"
#include "tafFlashAccess.hpp"

#define TAF_FWUPDATE_CMD_LEN 256
#define TAF_FWUPDATE_CMD_RESULT_LEN 32

#define TAF_FIRMWARE_VERSION_LINE_NUM 16
#define TAF_TELAF_VERSION_LEN 21

#define TAF_FWUPDATE_BYPASS_CHECK_TAG "NULL"

#define TAF_FWUPDATE_RECOVERY_LOG_FILE "/tmp/recovery.log"

#define TAF_TELAF_VERSION_FILE "/legato/systems/current/version"
#define TAF_ROOTFS_VERSION_FILE "/etc/version"
#define TAF_FIRMWARE_VERSION_FILE "/firmware/image/Ver_Info.txt"

#define TAF_FWUPDATE_INSTALL_CONTEXT "install_context"
#define TAF_FWUPDATE_INSTALL_IMGAE_NODE "install_context/image/%s"
#define TAF_FWUPDATE_POST_CHECK_SCRIPT "META-INF/com/google/android/updater-post-install-script"
#define TAF_FWUPDATE_POST_CHECK_SCRIPT_PATH "/data/updater-post-install-script"

#define TAF_FWUPDATE_ACTIVATE_CONTEXT "activate_context"
#define TAF_FWUPDATE_ACTIVATE_ITEM_NODE "activate_context/item/%s"

#define TAF_FWUPDATE_FOTA_STATE "/data/le_fs/fotaState"
#define TAF_FWUPDATE_LOCAL_PACAKAGE_PATH "/data/images/firmware"

const size_t kPageSize = 4 * 1024; //4k

const std::string kAreBlocksErased = "ARE_BLOCKS_ERASED";
const std::string kIsMTDSynced = "IS_MTD_SYNCED";
const std::string kIsUBISynced = "IS_UBI_SYNCED";
const std::string kPagesSynced = "PAGES_SYNCED";
const std::string kTotalPages = "TOTAL_PAGES";

//--------------------------------------------------------------------------------------------------
/**
 * Flash page size.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FWUPDATE_FLASH_PAGE_SIZE 0x1000

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

namespace telux {
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

        static void InstallTimerHandler(le_timer_Ref_t timerRef);

        void GetRootfsVersion(char* version);
        void GetTelafVersion(char* version);
        le_result_t GetFirmwareVersion(char* version);
        le_result_t GetMtdInformation(taf_lib_flash_Partition_t *partition, uint32_t* blocksNumber,
                uint32_t* badBlocksNumber, uint32_t* blockSize, uint32_t* pageSize);
        le_result_t GetUbiInformation(taf_lib_flash_Partition_t* partition, uint32_t* lebNumber,
                uint32_t* freeLebNumber, uint32_t* volumeSize);
        le_result_t InstallPreCheck(const char* manifest);
        void SetPauseAction(bool paused);
        bool GetPauseAction(void);
        void SetPageNumber(bool isTotal, uint32_t number);
        uint32_t GetPageNumber(bool isTotal);
        void SetImageDataPath(const char* image, const char* dataPath);
        void GetImageDataPath(const char* image, char* dataPath, size_t pathLen);
        bool GetImageStatus(const char* image);
        void SetImageStatus(const char* image, bool updated);
        uint32_t GetImagePageNumber(const char* image);
        void SetImagePageNumber(const char* image, uint32_t pageNum);
        bool GetImageForUpdate(char* name, size_t nameLen);
        bool IsPatchExist(const char* filePath, const char* patchPath);
        bool IsDeltaUpdate(const char* filePath);
        bool UnpackImage(const char* filePath, const char* imagePath, uint32_t* pageNum);
        void UpdateImage(void);
        void StartInstall(const char* filePath);
        void InstallFirmware(const char* filePath);

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

        le_result_t CalculateTotalPages(void);

        std::string FwUpdateStateToString(taf_update_State_t state);

        static void FwUpdateHandler(void* reqPtr);
        static void FwSyncHandler(void* reqPtr);
        static void FwStartSync(void* reqPtr);
        static void* FwUpdateThread(void* contextPtr);
        static void* FwSyncHandlerThread(void* contextPtr);
        static void* FwStartSyncThread(void* contextPtr);

        static le_event_Id_t fwUpdateEvId;
        static le_event_Id_t fwSyncHandlerEvId;
        static le_event_Id_t fwStartSyncEvId;

        uint32_t percent = 0;
        taf_update_Error_t error = TAF_UPDATE_NONE;
        bool isPartitionListInit = false;

    private:
        le_result_t InitPartitionList();
        le_result_t SyncMTD(taf_update_Bank_t activeBank);
        le_result_t EraseAllMTDBlocks(taf_update_Bank_t activeBank);
        le_result_t SyncUBI(taf_update_Bank_t activeBank);
        void PerformABSync();

        taf_lib_flash_PartitionList_t partitionList;
        std::map<std::string /* Partition name */, uint32_t /* Index*/> partitionMap;
    };
}
}

#endif
